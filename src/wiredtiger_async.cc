
#include <node.h>
#include "wiredtiger.h"
#include "NodeWiredTiger.hpp"

namespace wiredtiger {

ConnectionWorker::ConnectionWorker(
    WTConnection *conn,
    NanCallback *callback,
    const char *home,
    const char *options
    ) : NanAsyncWorker(callback)
{
	NanScope();
	v8::Local<v8::Object> obj = v8::Object::New();
	NanAssignPersistent(v8::Object, persistentHandle, obj);
	conn_ = conn;
	home_ = home;
	options_ = options;
};

void ConnectionWorker::Execute() {
	char errbuf[256];
	int ret;
	if ((ret = conn_->OpenConnection(home_, options_)) != 0) {
		snprintf(errbuf, 256, "WiredTiger open failed: %s",
		    wiredtiger_strerror(ret));
		this->errmsg = strdup(errbuf);
	}
}
}
