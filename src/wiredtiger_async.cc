
#include <node.h>
#include "wiredtiger.h"
#include "NodeWiredTiger.hpp"

namespace wiredtiger {

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

CreateWorker::CreateWorker(
    WTConnection *conn,
    NanCallback *callback,
    const char *uri,
    const char *config
    ) : NanAsyncWorker(callback)
{
	NanScope();
	v8::Local<v8::Object> obj = v8::Object::New();
	NanAssignPersistent(v8::Object, persistentHandle, obj);
	conn_ = conn;
	uri_ = uri;
	config_ = config;
};

void CreateWorker::Execute() {
	WT_SESSION *session;

	char errbuf[256];
	int ret;
	if ((ret = conn_->conn()->open_session(
	    conn_->conn(), NULL, NULL, &session)) != 0) {
		snprintf(errbuf, 256, "WiredTiger session create failed: %s",
		    wiredtiger_strerror(ret));
		this->errmsg = strdup(errbuf);
		return;
	}
	if ((ret = session->create(session, uri_, config_)) != 0) {
		snprintf(errbuf, 256, "WiredTiger create failed: %s",
		    wiredtiger_strerror(ret));
		this->errmsg = strdup(errbuf);
		return;
	}
}

}
