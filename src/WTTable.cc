#include <node.h>
#include "wiredtiger.h"
#include <unistd.h> // For sleep
#include <stdlib.h> // For malloc

#include "NodeWiredTiger.hpp"

using namespace v8;

namespace wiredtiger {

static v8::Persistent<v8::FunctionTemplate> wttable_constructor;

WTTable::WTTable(WTConnection *wtconn, char *uri, char *config)
    : wtconn_(wtconn), uri_(uri), config_(config) {
}

WTTable::~WTTable() {
}

const char * WTTable::uri() const { return uri_; }
const char * WTTable::config() const { return config_; }
WTConnection * WTTable::wtconn() const { return wtconn_; }

/* Calls from worker threads. */


/* V8 exposed functions */

void WTTable::Init() {
	v8::Local<v8::FunctionTemplate> tpl =
	    v8::FunctionTemplate::New(WTTable::New);

	NanAssignPersistent(v8::FunctionTemplate,
	    wttable_constructor, tpl);
	tpl->SetClassName(NanSymbol("WTTable"));
	tpl->InstanceTemplate()->SetInternalFieldCount(3);
	NODE_SET_PROTOTYPE_METHOD(tpl, "New", WTTable::New);
	NODE_SET_PROTOTYPE_METHOD(tpl, "Put", WTTable::Put);
	NODE_SET_PROTOTYPE_METHOD(tpl, "Search", WTTable::Search);
}

NAN_METHOD(WTTable::New) {
	NanScope();

	char *uri = NULL;
	char *config = NULL;

	if (args.Length() < 2 || !args[0]->IsObject() || !args[1]->IsString())
		return NanThrowError(
		    "constructor requires connection and uri arguments");
	WTConnection *wtconn =
	    node::ObjectWrap::Unwrap<WTConnection>(args[0]->ToObject());
	uri = NanFromV8String(args[1].As<v8::Object>(),
	    Nan::UTF8, NULL, NULL, 0, v8::String::NO_OPTIONS);
	if (args.Length() == 3) {
		if (!args[2]->IsString())
			return NanThrowError(
			    "Constructor option must be a string");
		config = NanFromV8String(args[1].As<v8::Object>(),
		    Nan::UTF8, NULL, NULL, 0, v8::String::NO_OPTIONS);
	}

	WTTable *table = new WTTable(wtconn, uri, config);
	table->Wrap(args.This());
	NanReturnValue(args.This());
}

v8::Handle<v8::Value> WTTable::NewInstance(
    v8::Local<v8::Object> &wtconn,
    v8::Local<v8::String> &uri,
    v8::Local<v8::String> &config) {

	NanScope();
	v8::Local<v8::Object> instance;

	v8::Local<v8::FunctionTemplate> constructorHandle =
	    NanPersistentToLocal(wttable_constructor);

	if (wtconn.IsEmpty() || uri.IsEmpty())
		return NanThrowError(
		    "constructor requires connection and uri arguments");

	if (config.IsEmpty()) {
		v8::Handle<v8::Value> argv[] = { wtconn, uri };
		instance =
		    constructorHandle->GetFunction()->NewInstance(2, argv);
	} else {
		v8::Handle<v8::Value> argv[] = { wtconn, uri, config };
		instance =
		    constructorHandle->GetFunction()->NewInstance(3, argv);
	}
	return instance;
}

typedef struct {
	Persistent<Function> javaCallback;
	uv_async_t *req;
	/* Setup in WT callback */
	int argc;
	Handle<Value> *argv;
} ASYNC_OP_COOKIE;

static void
HandleInsertOp(uv_async_t *handle, int status /* unused */) {
	// This is called in the Java thread, so it can call
	// the Java callback.
	ASYNC_OP_COOKIE *cookie =
	    static_cast<ASYNC_OP_COOKIE *>(handle->data);

	cookie->javaCallback->Call(
	    Context::GetCurrent()->Global(), cookie->argc, cookie->argv);
	cookie->javaCallback.Dispose();
}

static int
WTAsyncCallbackFunction(
    WT_ASYNC_CALLBACK *cb, WT_ASYNC_OP *op, int ret, uint32_t flags)
{
	ASYNC_OP_COOKIE *cookie = (ASYNC_OP_COOKIE *)op->app_data;

	if (op->get_type(op) == WT_AOP_INSERT) {
		Handle<Value> argv[1];
		if (ret != 0)
			argv[0] = node::UVException(
			    0, "WTTable::Put", wiredtiger_strerror(ret));
		else
			argv[0] = Local<Value>::New(Null());
		cookie->argc = 1;
		cookie->argv = argv;
	}

	// TODO: How do we pass in parameters to the callback?
	uv_async_send(cookie->req);

	// Cheat - for now wait until the callback will be finished
	// so we don't need to know when to free resources. We'll need
	// to figure out how to hold the argv around too when solving this
	// for real.
	sleep(1);
	// TODO: Can close_cb be NULL, will this cancel our async_send?
	// See: https://github.com/joyent/libuv/blob/master/include/uv.h
	// This is invalid - calling close will cancel the callback, it needs
	// to be done as part of a finalization or something. How can we
	// know that the callback has been called in the main thread?
	uv_close((uv_handle_t *)cookie->req, NULL);
	return (0);
}

static WT_ASYNC_CALLBACK WTAsyncCallback = { WTAsyncCallbackFunction };

Handle<Value> WTTable::Put(const Arguments& args) {
	int ret;

	if (args.Length() != 3 ||
	    !args[0]->IsString() ||
	    !args[1]->IsString() ||
	    !args[2]->IsFunction())
		return NanThrowError(
		    "Put() requires key/value and callback argument");

	wiredtiger::WTTable *table =
	    node::ObjectWrap::Unwrap<WTTable>(args.This());
	Handle<Function> callback = Handle<Function>::Cast(args[2]);

	char *key = NanFromV8String(args[0].As<v8::Object>(),
	    Nan::UTF8, NULL, NULL, 0, v8::String::NO_OPTIONS);
	char *value = NanFromV8String(args[1].As<v8::Object>(),
	    Nan::UTF8, NULL, NULL, 0, v8::String::NO_OPTIONS);

	// TODO: Free this memory.
	// Get setup to call the WiredTiger async operation.
	ASYNC_OP_COOKIE *cookie =
	    (ASYNC_OP_COOKIE *)malloc(sizeof(ASYNC_OP_COOKIE));
	uv_async_t *req =
	    (uv_async_t *)malloc(sizeof(uv_async_t));
	uv_async_init(uv_default_loop(), req, HandleInsertOp);
	// Make sure callback isn't garbage collected.
	cookie->javaCallback = Persistent<Function>::New(callback);
	req->data = cookie;
	// Setup the WiredTiger async operation
	WTConnection *wtconn;
	WT_ASYNC_OP *wtOp;
	wtconn = table->wtconn();
	if ((ret = wtconn->conn()->async_new_op(wtconn->conn(),
	    table->uri(), NULL, &WTAsyncCallback, &wtOp)) != 0) {
	}
	wtOp->app_data = cookie;
	wtOp->set_key(wtOp, key);
	wtOp->set_value(wtOp, value);
	if ((ret = wtOp->insert(wtOp)) != 0) {
	}

	// Avoid GC
	//v8::Local<v8::Object> _this = args.This();
	return v8::Undefined();
}

NAN_METHOD(WTTable::Search) {
	NanScope();
	NanReturnUndefined();
}
}
