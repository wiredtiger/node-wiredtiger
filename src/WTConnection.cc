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
Handle<Value> WiredTiger(const Arguments& args) {
	HandleScope scope;

	v8::Local<v8::String> home;
	v8::Local<v8::String> config;
	if (args.Length() < 2 || args.Length() > 3 || !args[0]->IsString())
		NODE_WT_THROW_EXCEPTION(
		    "Constructor requires a home argument");
	home = args[0].As<v8::String>();
	if (args.Length() == 2) {
		if (!args[1]->IsString())
			NODE_WT_THROW_EXCEPTION(
			    "Constructor option must be a string");
		config = args[1].As<v8::String>();
	}
	return scope.Close(WTConnection::NewInstance(home, config));
}

void WTConnection::Init(Handle<Object> target) {
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	Local<String> name = String::NewSymbol("WTConnection");

	fprintf(stderr, "Calling connection init\n");
	tpl->SetClassName(name);
	tpl->InstanceTemplate()->SetInternalFieldCount(2);
	wtconnection_constructor = Persistent<FunctionTemplate>::New(tpl);

	NODE_SET_PROTOTYPE_METHOD(tpl, "Open", WTConnection::Open);

	target->Set(name, wtconnection_constructor->GetFunction());
}

Handle<Value> WTConnection::New(const Arguments &args) {
	HandleScope scope;

	char *home = NULL;
	char *config = NULL;
	if (args.Length() == 0 || !args[0]->IsString())
		NODE_WT_THROW_EXCEPTION(
		    "constructor requires a home argument");
	home = NanFromV8String(args[0].As<v8::Object>(),
	    Nan::UTF8, NULL, NULL, 0, v8::String::NO_OPTIONS);
	if (args.Length() == 2) {
		if (!args[1]->IsString())
			NODE_WT_THROW_EXCEPTION(
			    "Constructor option must be a string");
		config = NanFromV8String(args[1].As<v8::Object>(),
		    Nan::UTF8, NULL, NULL, 0, v8::String::NO_OPTIONS);
	}

	WTConnection *conn = new WTConnection(home, config);
	conn->Wrap(args.This());
	conn->Ref();

	return args.This();
}

v8::Handle<v8::Value> WTConnection::NewInstance(
    v8::Local<v8::String> &home, v8::Local<v8::String> &config) {

	HandleScope scope();
	v8::Local<v8::Object> instance;

	v8::Local<v8::FunctionTemplate> constructorHandle =
	    NanPersistentToLocal(wtconnection_constructor);

	if (home.IsEmpty())
		NODE_WT_THROW_EXCEPTION("constructor requires home argument");

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

Handle<Value> WTConnection::Open(const Arguments& args) {
	HandleScope scope();
	if (args.Length() == 0 || !args[0]->IsFunction())
		NODE_WT_THROW_EXCEPTION(
		    "Open() requires a callback argument");
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
}
