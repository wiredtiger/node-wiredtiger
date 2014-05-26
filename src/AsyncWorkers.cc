/*-
 * Copyright (c) 2014- WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include <node.h>
#include "wiredtiger.h"
#include "NodeWiredTiger.hpp"

using namespace std;

namespace wiredtiger {

/* Worker for wiredtiger_open calls via WTConnection::Open. */
ConnectionWorker::ConnectionWorker(
    WTConnection *conn,
    NanCallback *callback,
    const char *home,
    const char *config
    ) : NanAsyncWorker(callback)
{
	NanScope();
	v8::Local<v8::Object> obj = v8::Object::New();
	NanAssignPersistent(v8::Object, persistentHandle, obj);
	conn_ = conn;
	home_ = home;
	config_ = config;
};

void ConnectionWorker::Execute() {
	char errbuf[256];
	int ret;
	if ((ret = conn_->OpenConnection(home_, config_)) != 0) {
		snprintf(errbuf, 256, "WiredTiger open failed: %s",
		    wiredtiger_strerror(ret));
		this->errmsg = strdup(errbuf);
	}
}

/* Worker for wiredtiger_open calls via WTConnection::OpenTable. */
OpenTableWorker::OpenTableWorker(
    WTTable *table,
    NanCallback *callback
    ) : NanAsyncWorker(callback)
{
	NanScope();
	v8::Local<v8::Object> obj = v8::Object::New();
	NanAssignPersistent(v8::Object, persistentHandle, obj);
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
