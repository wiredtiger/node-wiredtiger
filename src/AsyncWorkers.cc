/*-
 * Copyright (c) 2014- WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include <node.h>
#include <stdlib.h>
#include <string.h>
#include "wiredtiger.h"
#include "NodeWiredTiger.hpp"
#include "AsyncWorkers.hpp"

using namespace std;
using namespace v8;

namespace wiredtiger {

AsyncWorker::AsyncWorker(
    Persistent<Function> callback, Persistent<Object> owner)
    : callback_(callback), owner_(owner), errmsg(NULL)
{
	request.data = this;
}

AsyncWorker::~AsyncWorker()
{
	callback_.Dispose();
	owner_.Dispose();
}

void AsyncWorker::makeCallback(int argc, Handle<Value> *argv)
{
    node::MakeCallback(
        Context::GetCurrent()->Global(), callback_, argc, argv);
}

// The default worker returns with no additional parameters.
void AsyncWorker::ExecuteComplete()
{
	Local<Value> argv[] = {
	    Local<Value>::New(Null())
	};
	makeCallback(1, argv);
}

/* Worker for wiredtiger_open calls via WTConnection::Open. */
ConnectionWorker::ConnectionWorker(
    Persistent<Function> callback,
    Persistent<Object> object,
    WTConnection *conn,
    const char *home,
    const char *config
    ) : AsyncWorker(callback, object),
      	conn_(conn), home_(home), config_(config)
{
}

void ConnectionWorker::Execute() {
	char errbuf[256];
	int ret;
	if ((ret = conn_->OpenConnection(home_, config_)) != 0) {
		snprintf(errbuf, 256, "WiredTiger open failed: %s",
		    wiredtiger_strerror(ret));
		this->errmsg = strdup(errbuf);
	}
}

/* Worker for WT_CURSOR::next calls via WTCursor::NextImpl. */
CursorNextWorker::CursorNextWorker(
    Persistent<Function> callback,
    Persistent<Object> object,
    WTCursor *cursor
    ) : AsyncWorker(callback, object),
       	cursor_(cursor), key_(NULL), value_(NULL)
{
}

CursorNextWorker::~CursorNextWorker()
{
	if (key_ != NULL)
		free(key_);
	if (value_ != NULL)
		free(value_);
	if (errmsg != NULL)
		free(errmsg);
}

void CursorNextWorker::Execute() {
	char errbuf[256];
	int ret;
	ret = cursor_->NextImpl(&key_, &value_);
	if (ret != 0) {
		snprintf(errbuf, 256, "WTCursor::Next failed: %s",
		    wiredtiger_strerror(ret));
		this->errmsg = strdup(errbuf);
	}
}

void CursorNextWorker::ExecuteComplete()
{
	if (errmsg != NULL) {
		Local<Value> argv[] = {
		    Exception::Error(String::New(errmsg))
		};
		makeCallback(1, argv);
	/* A nul key is equivalent to WT_NOTFOUND */
	} else if (key_ == NULL) {
		Local<Value> argv[] = {
		    Local<Value>::New(Null()),
		    Local<Value>::New(Null()),
		    Local<Value>::New(Null())
		};
		makeCallback(3, argv);
	} else {
		Local<Value> argv[] = {
		    Local<Value>::New(Null()),
		    Local<Value>::New(String::New(key_)),
		    Local<Value>::New(String::New(value_))
		};
		makeCallback(3, argv);
	}
}

/* Worker for WT_CURSOR::next calls via WTCursor::SearchImpl. */
CursorSearchWorker::CursorSearchWorker(
    Persistent<Function> callback,
    Persistent<Object> object,
    WTCursor *cursor,
    char *key
    ) : AsyncWorker(callback, object),
       	cursor_(cursor), key_(key), value_(NULL)
{
}

CursorSearchWorker::~CursorSearchWorker() {
	free(key_);
	if (value_ != NULL)
		free(value_);
	if (errmsg != NULL)
		free(errmsg);
}

void CursorSearchWorker::Execute() {
	char errbuf[256];
	int ret;
	if ((ret = cursor_->SearchImpl(key_, &value_)) != 0) {
		snprintf(errbuf, 256, "WTCursor::Search failed: %s",
		    wiredtiger_strerror(ret));
		this->errmsg = strdup(errbuf);
	}
}

void CursorSearchWorker::ExecuteComplete()
{
	if (errmsg != NULL) {
		Local<Value> argv[] = {
		    Exception::Error(String::New(errmsg))
		};
		makeCallback(1, argv);
		return;
	/* A nul key is equivalent to WT_NOTFOUND */
	} else if (value_ == NULL) {
		Local<Value> argv[] = {
		    Local<Value>::New(Null()),
		    Local<Value>::New(Null())
		};
		makeCallback(2, argv);
	} else {
		Local<Value> argv[] = {
		    Local<Value>::New(Null()),
		    Local<Value>::New(String::New(value_))
		};
		makeCallback(2, argv);
	}
}

/* Worker for wiredtiger_open calls via WTConnection::OpenTable. */
OpenTableWorker::OpenTableWorker(
    Persistent<Function> callback,
    Persistent<Object> object,
    WTTable *table
    ) : AsyncWorker(callback, object)
{
	HandleScope scope;
	table_ = table;
};

void OpenTableWorker::Execute() {
	char errbuf[256];
	int ret;
	if ((ret = table_->OpenTable()) != 0) {
		snprintf(errbuf, 256, "WTTable::Open failed: %s",
		    wiredtiger_strerror(ret));
		this->errmsg = strdup(errbuf);
	}
}
}
