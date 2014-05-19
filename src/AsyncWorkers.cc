
#include <string>
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

void OpenTableWorker::Execute() {
	WT_SESSION *session;

	string final_config;
	char errbuf[256];
	size_t create_start;
	int ret;

	/* If there is no create in the config, nothing more to do. */
	if (config_ == NULL)
	       return;
	final_config = string(config_);
	if ((create_start = final_config.find("create")) == string::npos)
		return;

	/* There was a "create" - strip it from the original. */
	final_config = final_config.erase(create_start, 6);

	if ((ret = conn_->conn()->open_session(
	    conn_->conn(), NULL, NULL, &session)) != 0) {
		snprintf(errbuf, 256, "WiredTiger session create failed: %s",
		    wiredtiger_strerror(ret));
		this->errmsg = strdup(errbuf);
		return;
	}
	if ((ret = session->create(
	    session, uri_, final_config.c_str())) != 0) {
		snprintf(errbuf, 256, "WiredTiger create failed: %s",
		    wiredtiger_strerror(ret));
		this->errmsg = strdup(errbuf);
		return;
	}
}
}
