ACLOCAL_AMFLAGS = -I build_posix/aclocal

# BEGIN SUBDIRS, maintained by makemake and Make.subdirs
SUBDIRS = .
SUBDIRS += bench/tcbench
SUBDIRS += bench/wtperf
SUBDIRS += examples/c
SUBDIRS += ext/collators/reverse
if BZIP2
    SUBDIRS += ext/compressors/bzip2
endif
SUBDIRS += ext/compressors/nop
if SNAPPY
    SUBDIRS += ext/compressors/snappy
endif
if ZLIB
    SUBDIRS += ext/compressors/zlib
endif
if HAVE_HELIUM
    SUBDIRS += ext/datasources/helium
endif
if HAVE_BERKELEY_DB
    SUBDIRS += ext/test/kvs_bdb
endif
if JAVA
    SUBDIRS += lang/java
endif
if PYTHON
    SUBDIRS += lang/python
endif
SUBDIRS += test/bloom
SUBDIRS += test/fops
SUBDIRS += test/huge
# END SUBDIRS

lib_LTLIBRARIES = libwiredtiger.la
LDADD = $(lib_LTLIBRARIES)

bin_PROGRAMS = wt
wt_SOURCES =\
	src/utilities/util_backup.c \
	src/utilities/util_cpyright.c \
	src/utilities/util_compact.c \
	src/utilities/util_create.c \
	src/utilities/util_drop.c \
	src/utilities/util_dump.c \
	src/utilities/util_getopt.c \
	src/utilities/util_list.c \
	src/utilities/util_load.c \
	src/utilities/util_loadtext.c \
	src/utilities/util_main.c \
	src/utilities/util_misc.c \
	src/utilities/util_printlog.c \
	src/utilities/util_read.c \
	src/utilities/util_rename.c \
	src/utilities/util_salvage.c \
	src/utilities/util_stat.c \
	src/utilities/util_upgrade.c \
	src/utilities/util_verbose.c \
	src/utilities/util_verify.c \
	src/utilities/util_write.c

include_HEADERS= wiredtiger.h src/include/wiredtiger_ext.h
AM_CPPFLAGS = -I$(srcdir)/src/include

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = wiredtiger.pc

$(srcdir)/Makefile.am: $(srcdir)/build_posix/Make.base $(srcdir)/build_posix/makemake $(srcdir)/dist/filelist
	@cd $(srcdir)/build_posix && sh makemake

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool

$(top_srcdir)/src/include/extern.h: auto-includes.chk
$(top_srcdir)/src/include/wt_internal.h: auto-includes.chk

auto-includes.chk: $(libwiredtiger_la_SOURCES)
	@(cd $(srcdir)/dist && sh s_prototypes && sh s_typedef -b) && touch $@

clean-local:
	rm -rf WT_TEST

libwiredtiger_la_LDFLAGS = -release @VERSION@
libwiredtiger_la_SOURCES=\
	src/api/api_strerror.c \
	src/api/api_version.c \
	src/block/block_addr.c \
	src/block/block_ckpt.c \
	src/block/block_compact.c \
	src/block/block_ext.c \
	src/block/block_map.c \
	src/block/block_mgr.c \
	src/block/block_open.c \
	src/block/block_read.c \
	src/block/block_session.c \
	src/block/block_slvg.c \
	src/block/block_vrfy.c \
	src/block/block_write.c \
	src/bloom/bloom.c \
	src/btree/bt_bulk.c \
	src/btree/bt_compact.c \
	src/btree/bt_curnext.c \
	src/btree/bt_curprev.c \
	src/btree/bt_cursor.c \
	src/btree/bt_debug.c \
	src/btree/bt_discard.c \
	src/btree/bt_evict.c \
	src/btree/bt_handle.c \
	src/btree/bt_huffman.c \
	src/btree/bt_io.c \
	src/btree/bt_misc.c \
	src/btree/bt_ovfl.c \
	src/btree/bt_page.c \
	src/btree/bt_read.c \
	src/btree/bt_ret.c \
	src/btree/bt_slvg.c \
	src/btree/bt_stat.c \
	src/btree/bt_sync.c \
	src/btree/bt_upgrade.c \
	src/btree/bt_vrfy.c \
	src/btree/bt_vrfy_dsk.c \
	src/btree/bt_walk.c \
	src/btree/col_modify.c \
	src/btree/col_srch.c \
	src/btree/rec_evict.c \
	src/btree/rec_merge.c \
	src/btree/rec_split.c \
	src/btree/rec_track.c \
	src/btree/rec_write.c \
	src/btree/row_key.c \
	src/btree/row_modify.c \
	src/btree/row_srch.c \
	src/config/config.c \
	src/config/config_api.c \
	src/config/config_check.c \
	src/config/config_collapse.c \
	src/config/config_concat.c \
	src/config/config_def.c \
	src/config/config_ext.c \
	src/conn/conn_api.c \
	src/conn/conn_cache.c \
	src/conn/conn_cache_pool.c \
	src/conn/conn_ckpt.c \
	src/conn/conn_dhandle.c \
	src/conn/conn_handle.c \
	src/conn/conn_log.c \
	src/conn/conn_open.c \
	src/conn/conn_stat.c \
	src/cursor/cur_backup.c \
	src/cursor/cur_bulk.c \
	src/cursor/cur_config.c \
	src/cursor/cur_ds.c \
	src/cursor/cur_dump.c \
	src/cursor/cur_file.c \
	src/cursor/cur_index.c \
	src/cursor/cur_stat.c \
	src/cursor/cur_std.c \
	src/cursor/cur_table.c \
	src/log/log.c \
	src/log/log_auto.c \
	src/log/log_slot.c \
	src/lsm/lsm_cursor.c \
	src/lsm/lsm_merge.c \
	src/lsm/lsm_meta.c \
	src/lsm/lsm_stat.c \
	src/lsm/lsm_tree.c \
	src/lsm/lsm_worker.c \
	src/meta/meta_apply.c \
	src/meta/meta_ckpt.c \
	src/meta/meta_ext.c \
	src/meta/meta_table.c \
	src/meta/meta_track.c \
	src/meta/meta_turtle.c \
	src/os_posix/os_abort.c \
	src/os_posix/os_alloc.c \
	src/os_posix/os_dir.c \
	src/os_posix/os_dlopen.c \
	src/os_posix/os_errno.c \
	src/os_posix/os_exist.c \
	src/os_posix/os_fallocate.c \
	src/os_posix/os_filesize.c \
	src/os_posix/os_flock.c \
	src/os_posix/os_fsync.c \
	src/os_posix/os_ftruncate.c \
	src/os_posix/os_getline.c \
	src/os_posix/os_map.c \
	src/os_posix/os_mtx.c \
	src/os_posix/os_open.c \
	src/os_posix/os_priv.c \
	src/os_posix/os_remove.c \
	src/os_posix/os_rename.c \
	src/os_posix/os_rw.c \
	src/os_posix/os_sleep.c \
	src/os_posix/os_strtouq.c \
	src/os_posix/os_thread.c \
	src/os_posix/os_time.c \
	src/os_posix/os_yield.c \
	src/packing/pack_api.c \
	src/packing/pack_impl.c \
	src/packing/pack_stream.c \
	src/schema/schema_create.c \
	src/schema/schema_drop.c \
	src/schema/schema_list.c \
	src/schema/schema_open.c \
	src/schema/schema_plan.c \
	src/schema/schema_project.c \
	src/schema/schema_rename.c \
	src/schema/schema_stat.c \
	src/schema/schema_truncate.c \
	src/schema/schema_util.c \
	src/schema/schema_worker.c \
	src/session/session_api.c \
	src/session/session_compact.c \
	src/session/session_dhandle.c \
	src/session/session_salvage.c \
	src/support/cksum.c \
	src/support/err.c \
	src/support/filename.c \
	src/support/global.c \
	src/support/hash_city.c \
	src/support/hash_fnv.c \
	src/support/hazard.c \
	src/support/hex.c \
	src/support/huffman.c \
	src/support/mutex.c \
	src/support/pow.c \
	src/support/rand.c \
	src/support/scratch.c \
	src/support/sess_dump.c \
	src/support/stat.c \
	src/txn/txn.c \
	src/txn/txn_ckpt.c \
	src/txn/txn_ext.c \
	src/txn/txn_log.c \
	src/txn/txn_recover.c
