/*-
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

static void __inmem_col_fix(WT_SESSION_IMPL *, WT_PAGE *);
static void __inmem_col_int(WT_SESSION_IMPL *, WT_PAGE *);
static int  __inmem_col_var(WT_SESSION_IMPL *, WT_PAGE *, size_t *);
static int  __inmem_row_int(WT_SESSION_IMPL *, WT_PAGE *, size_t *);
static int  __inmem_row_leaf(WT_SESSION_IMPL *, WT_PAGE *);
static int  __inmem_row_leaf_entries(
	WT_SESSION_IMPL *, WT_PAGE_HEADER *, uint32_t *);

/*
 * __wt_page_in_func --
 *	Acquire a hazard pointer to a page; if the page is not in-memory,
 *	read it from the disk and build an in-memory version.
 */
int
__wt_page_in_func(
    WT_SESSION_IMPL *session, WT_PAGE *parent, WT_REF *ref
#ifdef HAVE_DIAGNOSTIC
    , const char *file, int line
#endif
    )
{
	WT_DECL_RET;
	WT_PAGE *page;
	int busy, force_attempts, oldgen;

	for (force_attempts = oldgen = 0;;) {
		switch (ref->state) {
		case WT_REF_DISK:
		case WT_REF_DELETED:
			/*
			 * The page isn't in memory, attempt to read it.
			 * Make sure there is space in the cache.
			 */
			WT_RET(__wt_cache_full_check(session));
			WT_RET(__wt_cache_read(session, parent, ref));
			oldgen = F_ISSET(session, WT_SESSION_NO_CACHE) ? 1 : 0;
			continue;
		case WT_REF_LOCKED:
		case WT_REF_READING:
			/*
			 * The page is being read or considered for eviction --
			 * wait for that to be resolved.
			 */
			break;
		case WT_REF_EVICT_WALK:
		case WT_REF_MEM:
			/*
			 * The page is in memory: get a hazard pointer, update
			 * the page's LRU and return.  The expected reason we
			 * can't get a hazard pointer is because the page is
			 * being evicted; yield and try again.
			 */
#ifdef HAVE_DIAGNOSTIC
			WT_RET(
			    __wt_hazard_set(session, ref, &busy, file, line));
#else
			WT_RET(__wt_hazard_set(session, ref, &busy));
#endif
			if (busy)
				break;

			page = ref->page;
			WT_ASSERT(session,
			    page != NULL && !WT_PAGE_IS_ROOT(page));

			/*
			 * Force evict pages that are too big.  Only do this
			 * check if there is a chance of eviction succeeding.
			 * That is, if the updates on the page are visible to
			 * the running transaction.
			 */
			if (force_attempts < 10 &&
			    __wt_eviction_force_check(session, page) &&
			    __wt_eviction_force_txn_check(session, page)) {
				++force_attempts;
				WT_RET(__wt_eviction_force(session, page));
				break;
			}

			/* Check if we need an autocommit transaction. */
			if ((ret = __wt_txn_autocommit_check(session)) != 0) {
				WT_TRET(__wt_hazard_clear(session, page));
				return (ret);
			}

			/*
			 * If we read the page and we are configured to not
			 * trash the cache, set the oldest read generation so
			 * the page is forcibly evicted as soon as possible.
			 *
			 * Otherwise, update the page's read generation.
			 */
			if (oldgen && page->read_gen == WT_READ_GEN_NOTSET)
				page->read_gen = WT_READ_GEN_OLDEST;
			else if (page->read_gen < __wt_cache_read_gen(session))
				page->read_gen =
				    __wt_cache_read_gen_set(session);

			return (0);
		WT_ILLEGAL_VALUE(session);
		}

		/* We failed to get the page -- yield before retrying. */
		__wt_yield();
	}
}

/*
 * __wt_page_alloc --
 *	Create or read a page into the cache.
 */
int
__wt_page_alloc(WT_SESSION_IMPL *session,
    uint8_t type, uint32_t alloc_entries, WT_PAGE **pagep)
{
	WT_CACHE *cache;
	WT_PAGE *page;
	size_t size;
	void *p;

	*pagep = NULL;

	cache = S2C(session)->cache;

	/*
	 * Allocate a page, and for most page types, the additional information
	 * it needs to describe the disk image.
	 */
	size = sizeof(WT_PAGE);
	switch (type) {
	case WT_PAGE_COL_FIX:
		break;
	case WT_PAGE_COL_INT:
	case WT_PAGE_ROW_INT:
		size += alloc_entries * sizeof(WT_REF);
		break;
	case WT_PAGE_COL_VAR:
		size += alloc_entries * sizeof(WT_COL);
		break;
	case WT_PAGE_ROW_LEAF:
		size += alloc_entries * sizeof(WT_ROW);
		break;
	WT_ILLEGAL_VALUE(session);
	}

	WT_RET(__wt_calloc(session, 1, size, &page));
	p = (uint8_t *)page + sizeof(WT_PAGE);

	switch (type) {
	case WT_PAGE_COL_FIX:
		break;
	case WT_PAGE_COL_INT:
	case WT_PAGE_ROW_INT:
		page->u.intl.t = p;
		break;
	case WT_PAGE_COL_VAR:
		page->u.col_var.d = p;
		break;
	case WT_PAGE_ROW_LEAF:
		page->u.row.d = p;
		break;
	WT_ILLEGAL_VALUE(session);
	}

	/* Increment the cache statistics. */
	__wt_cache_page_inmem_incr(session, page, size);
	(void)WT_ATOMIC_ADD(cache->pages_inmem, 1);

	/* The one page field we set is the type. */
	page->type = type;

	*pagep = page;
	return (0);
}

/*
 * __wt_page_inmem --
 *	Build in-memory page information.
 */
int
__wt_page_inmem(
    WT_SESSION_IMPL *session, WT_PAGE *parent, WT_REF *parent_ref,
    WT_PAGE_HEADER *dsk, uint32_t flags, WT_PAGE **pagep)
{
	WT_DECL_RET;
	WT_PAGE *page;
	uint32_t alloc_entries;
	size_t size;

	alloc_entries = 0;
	*pagep = NULL;

	/*
	 * Figure out how many underlying objects the page references so
	 * we can allocate them along with the page.
	 */
	switch (dsk->type) {
	case WT_PAGE_COL_FIX:
		break;
	case WT_PAGE_COL_INT:
		/*
		 * Column-store internal page entries map one-to-one to the
		 * number of physical entries on the page (each physical entry
		 * is an offset object).
		 */
		alloc_entries = dsk->u.entries;
		break;
	case WT_PAGE_COL_VAR:
		/*
		 * Column-store leaf page entries map one-to-one to the number
		 * of physical entries on the page (each physical entry is a
		 * value item).
		 */
		alloc_entries = dsk->u.entries;
		break;
	case WT_PAGE_ROW_INT:
		/*
		 * Row-store internal page entries map one-to-two to the number
		 * of physical entries on the page (each in-memory entry is a
		 * key item and location cookie).
		 */
		alloc_entries = dsk->u.entries / 2;
		break;
	case WT_PAGE_ROW_LEAF:
		/*
		 * If the "no empty values" flag is set, row-store leaf page
		 * entries map one-to-one to the number of physical entries
		 * on the page (each physical entry is a key or value item).
		 * If that flag is not set, there are more keys than values,
		 * we have to walk the page to figure it out.
		 */
		if (F_ISSET(dsk, WT_PAGE_EMPTY_V_ALL))
			alloc_entries = dsk->u.entries;
		else if (F_ISSET(dsk, WT_PAGE_EMPTY_V_NONE))
			alloc_entries = dsk->u.entries / 2;
		else
			WT_RET(__inmem_row_leaf_entries(
			    session, dsk, &alloc_entries));
		break;
	WT_ILLEGAL_VALUE(session);
	}

	/* Allocate and initialize a new WT_PAGE. */
	WT_RET(__wt_page_alloc(session, dsk->type, alloc_entries, &page));
	page->dsk = dsk;
	page->read_gen = WT_READ_GEN_NOTSET;
	F_SET_ATOMIC(page, flags);

	/*
	 * Track the memory allocated to build this page so we can update the
	 * cache statistics in a single call.
	 */
	size = LF_ISSET(WT_PAGE_DISK_ALLOC) ? dsk->mem_size : 0;

	switch (page->type) {
	case WT_PAGE_COL_FIX:
		page->entries = dsk->u.entries;
		page->u.col_fix.recno = dsk->recno;
		__inmem_col_fix(session, page);
		break;
	case WT_PAGE_COL_INT:
		page->entries = dsk->u.entries;
		page->u.intl.recno = dsk->recno;
		__inmem_col_int(session, page);
		break;
	case WT_PAGE_COL_VAR:
		page->entries = dsk->u.entries;
		page->u.col_var.recno = dsk->recno;
		WT_ERR(__inmem_col_var(session, page, &size));
		break;
	case WT_PAGE_ROW_INT:
		page->entries = dsk->u.entries / 2;
		WT_ERR(__inmem_row_int(session, page, &size));
		break;
	case WT_PAGE_ROW_LEAF:
		page->entries = alloc_entries;
		WT_ERR(__inmem_row_leaf(session, page));
		break;
	WT_ILLEGAL_VALUE_ERR(session);
	}

	/* Update the page's in-memory size and the cache statistics. */
	__wt_cache_page_inmem_incr(session, page, size);

	/* Link the new page into the parent. */
	if (parent_ref != NULL)
		WT_LINK_PAGE(parent, parent_ref, page);

	*pagep = page;
	return (0);

err:	__wt_page_out(session, &page);
	return (ret);
}

/*
 * __inmem_col_fix --
 *	Build in-memory index for fixed-length column-store leaf pages.
 */
static void
__inmem_col_fix(WT_SESSION_IMPL *session, WT_PAGE *page)
{
	WT_BTREE *btree;
	WT_PAGE_HEADER *dsk;

	btree = S2BT(session);
	dsk = page->dsk;

	page->u.col_fix.bitf = WT_PAGE_HEADER_BYTE(btree, dsk);
}

/*
 * __inmem_col_int --
 *	Build in-memory index for column-store internal pages.
 */
static void
__inmem_col_int(WT_SESSION_IMPL *session, WT_PAGE *page)
{
	WT_BTREE *btree;
	WT_CELL *cell;
	WT_CELL_UNPACK *unpack, _unpack;
	WT_PAGE_HEADER *dsk;
	WT_REF *ref;
	uint32_t i;

	btree = S2BT(session);
	dsk = page->dsk;
	unpack = &_unpack;

	/*
	 * Walk the page, building references: the page contains value items.
	 * The value items are on-page items (WT_CELL_VALUE).
	 */
	ref = page->u.intl.t;
	WT_CELL_FOREACH(btree, dsk, cell, unpack, i) {
		__wt_cell_unpack(cell, unpack);
		ref->addr = cell;
		ref->key.recno = unpack->v;
		++ref;
	}
}

/*
 * __inmem_col_var_repeats --
 *	Count the number of repeat entries on the page.
 */
static int
__inmem_col_var_repeats(WT_SESSION_IMPL *session, WT_PAGE *page, uint32_t *np)
{
	WT_BTREE *btree;
	WT_CELL *cell;
	WT_CELL_UNPACK *unpack, _unpack;
	WT_PAGE_HEADER *dsk;
	uint32_t i;

	btree = S2BT(session);
	dsk = page->dsk;
	unpack = &_unpack;

	/* Walk the page, counting entries for the repeats array. */
	*np = 0;
	WT_CELL_FOREACH(btree, dsk, cell, unpack, i) {
		__wt_cell_unpack(cell, unpack);
		if (__wt_cell_rle(unpack) > 1)
			++*np;
	}
	return (0);
}

/*
 * __inmem_col_var --
 *	Build in-memory index for variable-length, data-only leaf pages in
 *	column-store trees.
 */
static int
__inmem_col_var(WT_SESSION_IMPL *session, WT_PAGE *page, size_t *sizep)
{
	WT_BTREE *btree;
	WT_COL *cip;
	WT_COL_RLE *repeats;
	WT_CELL *cell;
	WT_CELL_UNPACK *unpack, _unpack;
	WT_PAGE_HEADER *dsk;
	uint64_t recno, rle;
	size_t bytes_allocated;
	uint32_t i, indx, n, repeat_off;

	btree = S2BT(session);
	dsk = page->dsk;
	unpack = &_unpack;
	repeats = NULL;
	repeat_off = 0;
	bytes_allocated = 0;
	recno = page->u.col_var.recno;

	/*
	 * Walk the page, building references: the page contains unsorted value
	 * items.  The value items are on-page (WT_CELL_VALUE), overflow items
	 * (WT_CELL_VALUE_OVFL) or deleted items (WT_CELL_DEL).
	 */
	indx = 0;
	cip = page->u.col_var.d;
	WT_CELL_FOREACH(btree, dsk, cell, unpack, i) {
		__wt_cell_unpack(cell, unpack);
		(cip++)->__value = WT_PAGE_DISK_OFFSET(page, cell);

		/*
		 * Add records with repeat counts greater than 1 to an array we
		 * use for fast lookups.  The first entry we find needing the
		 * repeats array triggers a re-walk from the start of the page
		 * to determine the size of the array.
		 */
		rle = __wt_cell_rle(unpack);
		if (rle > 1) {
			if (repeats == NULL) {
				WT_RET(
				    __inmem_col_var_repeats(session, page, &n));
				WT_RET(__wt_realloc_def(session,
				    &bytes_allocated, n + 1, &repeats));

				page->u.col_var.repeats = repeats;
				page->u.col_var.nrepeats = n;
				*sizep += bytes_allocated;
			}
			repeats[repeat_off].indx = indx;
			repeats[repeat_off].recno = recno;
			repeats[repeat_off++].rle = rle;
		}
		indx++;
		recno += rle;
	}

	return (0);
}

/*
 * __inmem_row_int --
 *	Build in-memory index for row-store internal pages.
 */
static int
__inmem_row_int(WT_SESSION_IMPL *session, WT_PAGE *page, size_t *sizep)
{
	WT_BTREE *btree;
	WT_CELL *cell;
	WT_CELL_UNPACK *unpack, _unpack;
	WT_DECL_ITEM(current);
	WT_DECL_RET;
	WT_PAGE_HEADER *dsk;
	WT_REF *ref;
	uint32_t i;

	btree = S2BT(session);
	unpack = &_unpack;
	dsk = page->dsk;

	WT_ERR(__wt_scr_alloc(session, 0, &current));

	/*
	 * Walk the page, instantiating keys: the page contains sorted key and
	 * location cookie pairs.  Keys are on-page/overflow items and location
	 * cookies are WT_CELL_ADDR_XXX items.
	 */
	ref = page->u.intl.t;
	WT_CELL_FOREACH(btree, dsk, cell, unpack, i) {
		__wt_cell_unpack(cell, unpack);
		switch (unpack->type) {
		case WT_CELL_KEY:
			__wt_ref_key_onpage_set(page, ref, unpack);
			break;
		case WT_CELL_KEY_OVFL:
			/* Instantiate any overflow records. */
			WT_ERR(__wt_dsk_cell_data_ref(
			    session, page->type, unpack, current));

			WT_ERR(__wt_row_ikey(session,
			    WT_PAGE_DISK_OFFSET(page, cell),
			    current->data, current->size, &ref->key.ikey));

			*sizep += sizeof(WT_IKEY) + current->size;
			break;
		case WT_CELL_ADDR_DEL:
		case WT_CELL_ADDR_INT:
		case WT_CELL_ADDR_LEAF:
		case WT_CELL_ADDR_LEAF_NO:
			ref->addr = cell;

			/*
			 * A cell may reference a deleted leaf page: if a leaf
			 * page was deleted without first being read, and the
			 * deletion committed, but older transactions in the
			 * system required the previous version of the page to
			 * be available, a special deleted-address type cell is
			 * written.  If we crash and recover to a page with a
			 * deleted-address cell, we now want to delete the leaf
			 * page (because it was never deleted, but by definition
			 * no earlier transaction might need it).
			 *
			 * Re-create the WT_REF state of a deleted node and give
			 * the page a modify structure.
			 *
			 * If the tree is already dirty and so will be written,
			 * mark the page dirty.  (We'd like to free the deleted
			 * pages, but if the handle is read-only or if the
			 * application never modifies the tree, we're not able
			 * to do so.)
			 */
			if (unpack->raw == WT_CELL_ADDR_DEL) {
				ref->state = WT_REF_DELETED;
				ref->txnid = WT_TXN_NONE;

				if (btree->modified) {
					WT_ERR(__wt_page_modify_init(
					    session, page));
					__wt_page_modify_set(session, page);
				}
			}

			++ref;
			break;
		WT_ILLEGAL_VALUE_ERR(session);
		}
	}

err:	__wt_scr_free(&current);
	return (ret);
}

/*
 * __inmem_row_leaf_entries --
 *	Return the number of entries for row-store leaf pages.
 */
static int
__inmem_row_leaf_entries(
    WT_SESSION_IMPL *session, WT_PAGE_HEADER *dsk, uint32_t *nindxp)
{
	WT_BTREE *btree;
	WT_CELL *cell;
	WT_CELL_UNPACK *unpack, _unpack;
	uint32_t i, nindx;

	btree = S2BT(session);
	unpack = &_unpack;

	/*
	 * Leaf row-store page entries map to a maximum of one-to-one to the
	 * number of physical entries on the page (each physical entry might be
	 * a key without a subsequent data item).  To avoid over-allocation in
	 * workloads without empty data items, first walk the page counting the
	 * number of keys, then allocate the indices.
	 *
	 * The page contains key/data pairs.  Keys are on-page (WT_CELL_KEY) or
	 * overflow (WT_CELL_KEY_OVFL) items, data are either non-existent or a
	 * single on-page (WT_CELL_VALUE) or overflow (WT_CELL_VALUE_OVFL) item.
	 */
	nindx = 0;
	WT_CELL_FOREACH(btree, dsk, cell, unpack, i) {
		__wt_cell_unpack(cell, unpack);
		switch (unpack->type) {
		case WT_CELL_KEY:
		case WT_CELL_KEY_OVFL:
			++nindx;
			break;
		case WT_CELL_VALUE:
		case WT_CELL_VALUE_OVFL:
			break;
		WT_ILLEGAL_VALUE(session);
		}
	}

	*nindxp = nindx;
	return (0);
}

/*
 * __inmem_row_leaf --
 *	Build in-memory index for row-store leaf pages.
 */
static int
__inmem_row_leaf(WT_SESSION_IMPL *session, WT_PAGE *page)
{
	WT_BTREE *btree;
	WT_CELL *cell;
	WT_CELL_UNPACK *unpack, _unpack;
	WT_PAGE_HEADER *dsk;
	WT_ROW *rip;
	uint32_t i;

	btree = S2BT(session);
	dsk = page->dsk;
	unpack = &_unpack;

	/* Walk the page, building indices. */
	rip = page->u.row.d;
	WT_CELL_FOREACH(btree, dsk, cell, unpack, i) {
		__wt_cell_unpack(cell, unpack);
		switch (unpack->type) {
		case WT_CELL_KEY:
		case WT_CELL_KEY_OVFL:
			WT_ROW_KEY_SET(rip, cell);
			++rip;
			break;
		case WT_CELL_VALUE:
		case WT_CELL_VALUE_OVFL:
			break;
		WT_ILLEGAL_VALUE(session);
		}
	}

	/*
	 * We do not currently instantiate keys on leaf pages when the page is
	 * loaded, they're instantiated on demand.
	 */
	return (0);
}
