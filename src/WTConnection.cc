#include <node.h>
#include "NodeWiredTiger.hpp"

using namespace v8;

namespace wiredtiger {

static v8::Persistent<v8::FunctionTemplate> wtconnection_constructor;

WTConnection::WTConnection(char *home, char *options) : home_(home), options_(options) {
	conn_ = NULL;
}

WTConnection::~WTConnection() {
	if (conn_ != NULL)
		conn_->close(conn_, NULL);
}

const char * WTConnection::home() const { return home_; }
const char * WTConnection::options() const { return options_; }

/* Calls from worker threads. */

int WTConnection::OpenConnection(const char *home, const char *options) {
	return (wiredtiger_open(home, NULL, options, &conn_));
}

/* V8 exposed functions */
NAN_METHOD(WiredTiger) {
	NanScope();

	v8::Local<v8::String> home;
	v8::Local<v8::String> options;
	if (args.Length() < 2 || args.Length() > 3 || !args[0]->IsString())
		return NanThrowError("Constructor requires a home argument");
	home = args[0].As<v8::String>();
	if (args.Length() == 2) {
		if (!args[1]->IsString())
			return NanThrowError(
			    "Constructor option must be a string");
		options = args[1].As<v8::String>();
	}
	NanReturnValue(WTConnection::NewInstance(home, options));
}

void WTConnection::Init() {
	v8::Local<v8::FunctionTemplate> tpl =
	    v8::FunctionTemplate::New(WTConnection::New);

	NanAssignPersistent(v8::FunctionTemplate,
	    wtconnection_constructor, tpl);
	tpl->SetClassName(NanSymbol("WTConnection"));
	tpl->InstanceTemplate()->SetInternalFieldCount(2);
	NODE_SET_PROTOTYPE_METHOD(tpl, "Open", WTConnection::Open);
	NODE_SET_PROTOTYPE_METHOD(tpl, "Put", WTConnection::Put);
	NODE_SET_PROTOTYPE_METHOD(tpl, "Search", WTConnection::Search);
}

NAN_METHOD(WTConnection::New) {
	NanScope();

	char *home = NULL;
	char *options = NULL;
	if (args.Length() == 0 || !args[0]->IsString())
		return NanThrowError("constructor requires a home argument");
	home = NanFromV8String(args[0].As<v8::Object>(),
	    Nan::UTF8, NULL, NULL, 0, v8::String::NO_OPTIONS);
	if (args.Length() == 2) {
		if (!args[1]->IsString())
			return NanThrowError(
			    "Constructor option must be a string");
		options = NanFromV8String(args[1].As<v8::Object>(),
		    Nan::UTF8, NULL, NULL, 0, v8::String::NO_OPTIONS);
	}

	WTConnection *conn = new WTConnection(home, options);
	conn->Wrap(args.This());
	NanReturnValue(args.This());
}

v8::Handle<v8::Value> WTConnection::NewInstance(
    v8::Local<v8::String> &home, v8::Local<v8::String> &options) {

	NanScope();
	v8::Local<v8::Object> instance;

	v8::Local<v8::FunctionTemplate> constructorHandle =
	    NanPersistentToLocal(wtconnection_constructor);

	if (home.IsEmpty())
		return NanThrowError("constructor requires home argument");

	if (options.IsEmpty()) {
		v8::Handle<v8::Value> argv[] = { home };
		instance =
		    constructorHandle->GetFunction()->NewInstance(1, argv);
	} else {
		v8::Handle<v8::Value> argv[] = { home, options };
		instance =
		    constructorHandle->GetFunction()->NewInstance(2, argv);
	}
	return instance;
}

NAN_METHOD(WTConnection::Open) {
	NanScope();
	if (args.Length() == 0 || !args[0]->IsFunction())
		return NanThrowError("Open() requires a callback argument");
	wiredtiger::WTConnection *conn =
	    node::ObjectWrap::Unwrap<wiredtiger::WTConnection>(args.This());
	v8::Local<v8::Function> callback = args[0].As<v8::Function>();

	ConnectionWorker *worker = new ConnectionWorker(
	    conn, new NanCallback(callback), conn->home(), conn->options());

	// Avoid GC
	v8::Local<v8::Object> _this = args.This();
	worker->SavePersistent("connection", _this);
	NanAsyncQueueWorker(worker);
	NanReturnUndefined();
}

NAN_METHOD(WTConnection::Put) {
	NanScope();
	NanReturnUndefined();
}
NAN_METHOD(WTConnection::Search) {
	NanScope();
	NanReturnUndefined();
}
}
