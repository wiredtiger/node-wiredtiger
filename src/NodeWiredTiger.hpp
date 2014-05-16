#ifndef NODE_WIREDTIGER_H
#define NODE_WIREDTIGER_H

#include <node.h>
#include <nan.h>
#include "wiredtiger.h"

namespace wiredtiger {

NAN_METHOD(WiredTiger);

class WTConnection : public node::ObjectWrap {
public:
	static void Init();
	static v8::Handle<v8::Value> NewInstance(
	    v8::Local<v8::String> &home, v8::Local<v8::String> &options);
	int OpenConnection(const char *home, const char *options);

	WTConnection(char *home, char *options);
	~WTConnection();

	const char *home() const;
	const char *options() const;

private:
	static NAN_METHOD(New);
	static NAN_METHOD(Open);
	static NAN_METHOD(Put);
	static NAN_METHOD(Search);

	char *home_;
	char *options_;
	WT_CONNECTION *conn_;
};

class ConnectionWorker : public NanAsyncWorker {
public:
	ConnectionWorker(
	    WTConnection *conn,
	    NanCallback *callback,
	    const char *home,
	    const char *options
	);

	ConnectionWorker();
	virtual void Execute();
private:
	WTConnection *conn_;
	const char *home_;
	const char *options_;
};
} // namespace wiredtiger

#endif
