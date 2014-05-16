#include <node.h>
#include <v8.h>

#include "wiredtiger.h"

using namespace v8;

Handle<Value> createDatabase(const Arguments& args) {
	HandleScope scope;
	WT_CONNECTION *conn;
	int ret;

	if ((ret = wiredtiger_open("/tmp/test", NULL, "create", &conn)) != 0)
		fprintf(stderr, "Failed to open WiredTiger: %d\n", ret);

	return scope.Close(String::New("Created WiredTiger database"));
}

void init(Handle<Object> target) {
	NODE_SET_METHOD(target, "createDatabase", createDatabase);
}
NODE_MODULE(wiredtiger, init)
