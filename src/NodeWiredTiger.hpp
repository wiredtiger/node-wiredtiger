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
	    v8::Local<v8::String> &home, v8::Local<v8::String> &config);
	int OpenConnection(const char *home, const char *config);

	WTConnection(char *home, char *config);
	~WTConnection();

	const char *home() const;
	const char *config() const;
	WT_CONNECTION *conn() const;

private:
	static NAN_METHOD(New);
	static NAN_METHOD(Create);
	static NAN_METHOD(Open);
	static NAN_METHOD(Put);
	static NAN_METHOD(Search);

	char *home_;
	char *config_;
	WT_CONNECTION *conn_;
};

class ConnectionWorker : public NanAsyncWorker {
public:
	ConnectionWorker(
	    WTConnection *conn,
	    NanCallback *callback,
	    const char *home,
	    const char *config
	);

	ConnectionWorker();
	virtual void Execute();
private:
	WTConnection *conn_;
	const char *home_;
	const char *config_;
};

class CreateWorker : public NanAsyncWorker {
public:
	CreateWorker(
	    WTConnection *conn,
	    NanCallback *callback,
	    const char *home,
	    const char *config
	);

	CreateWorker();
	virtual void Execute();
private:
	WTConnection *conn_;
	const char *uri_;
	const char *config_;
};
} // namespace wiredtiger

#endif
