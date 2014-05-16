#include <node.h>
#include "NodeWiredTiger.hpp"

using namespace v8;

namespace wiredtiger {

static v8::Persistent<v8::FunctionTemplate> wtconnection_constructor;

WTConnection::WTConnection(char *home, char *config) : home_(home), config_(config) {
	conn_ = NULL;
}

WTConnection::~WTConnection() {
	if (conn_ != NULL)
		conn_->close(conn_, NULL);
}

const char * WTConnection::home() const { return home_; }
const char * WTConnection::config() const { return config_; }
WT_CONNECTION * WTConnection::conn() const { return conn_; }

/* Calls from worker threads. */

int WTConnection::OpenConnection(const char *home, const char *config) {
	return (wiredtiger_open(home, NULL, config, &conn_));
}

/* V8 exposed functions */
NAN_METHOD(WiredTiger) {
	NanScope();

	v8::Local<v8::String> home;
	v8::Local<v8::String> config;
	if (args.Length() < 2 || args.Length() > 3 || !args[0]->IsString())
		return NanThrowError("Constructor requires a home argument");
	home = args[0].As<v8::String>();
	if (args.Length() == 2) {
		if (!args[1]->IsString())
			return NanThrowError(
			    "Constructor option must be a string");
		config = args[1].As<v8::String>();
	}
	NanReturnValue(WTConnection::NewInstance(home, config));
}

void WTConnection::Init() {
	v8::Local<v8::FunctionTemplate> tpl =
	    v8::FunctionTemplate::New(WTConnection::New);

	NanAssignPersistent(v8::FunctionTemplate,
	    wtconnection_constructor, tpl);
	tpl->SetClassName(NanSymbol("WTConnection"));
	tpl->InstanceTemplate()->SetInternalFieldCount(2);
	NODE_SET_PROTOTYPE_METHOD(tpl, "Open", WTConnection::Open);
	NODE_SET_PROTOTYPE_METHOD(tpl, "Create", WTConnection::Create);
	NODE_SET_PROTOTYPE_METHOD(tpl, "Put", WTConnection::Put);
	NODE_SET_PROTOTYPE_METHOD(tpl, "Search", WTConnection::Search);
}

NAN_METHOD(WTConnection::New) {
	NanScope();

	char *home = NULL;
	char *config = NULL;
	if (args.Length() == 0 || !args[0]->IsString())
		return NanThrowError("constructor requires a home argument");
	home = NanFromV8String(args[0].As<v8::Object>(),
	    Nan::UTF8, NULL, NULL, 0, v8::String::NO_OPTIONS);
	if (args.Length() == 2) {
		if (!args[1]->IsString())
			return NanThrowError(
			    "Constructor option must be a string");
		config = NanFromV8String(args[1].As<v8::Object>(),
		    Nan::UTF8, NULL, NULL, 0, v8::String::NO_OPTIONS);
	}

	WTConnection *conn = new WTConnection(home, config);
	conn->Wrap(args.This());
	NanReturnValue(args.This());
}

v8::Handle<v8::Value> WTConnection::NewInstance(
    v8::Local<v8::String> &home, v8::Local<v8::String> &config) {

	NanScope();
	v8::Local<v8::Object> instance;

	v8::Local<v8::FunctionTemplate> constructorHandle =
	    NanPersistentToLocal(wtconnection_constructor);

	if (home.IsEmpty())
		return NanThrowError("constructor requires home argument");

	if (config.IsEmpty()) {
		v8::Handle<v8::Value> argv[] = { home };
		instance =
		    constructorHandle->GetFunction()->NewInstance(1, argv);
	} else {
		v8::Handle<v8::Value> argv[] = { home, config };
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
	    conn, new NanCallback(callback), conn->home(), conn->config());

	// Avoid GC
	v8::Local<v8::Object> _this = args.This();
	worker->SavePersistent("connection", _this);
	NanAsyncQueueWorker(worker);
	NanReturnUndefined();
}

NAN_METHOD(WTConnection::Create) {
	NanScope();
	if (args.Length() < 2 || !args[0]->IsString())
		return NanThrowError(
		    "Open() requires a uri and a callback argument");
	if (args.Length() == 3 &&
	    (!args[1]->IsString() || !args[2]->IsFunction()))
		return NanThrowError("Open() invalid config argument");

	/* Extract the arguments */
	char *uri = NanFromV8String(args[0].As<v8::Object>(),
	    Nan::UTF8, NULL, NULL, 0, v8::String::NO_OPTIONS);
	char *config = NULL;
	v8::Local<v8::Function> callback;
	if (args.Length() == 3) {
		config = NanFromV8String(args[1].As<v8::Object>(),
		    Nan::UTF8, NULL, NULL, 0, v8::String::NO_OPTIONS);
		callback = args[2].As<v8::Function>();
	} else
		callback = args[1].As<v8::Function>();

	/* Retrieve our WTConnection handle. */
	wiredtiger::WTConnection *conn =
	    node::ObjectWrap::Unwrap<wiredtiger::WTConnection>(args.This());

	CreateWorker *worker = new CreateWorker(
	    conn, new NanCallback(callback), uri, config);

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
