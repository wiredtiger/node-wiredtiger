#include <node.h>
#include "wiredtiger.h"
#include <unistd.h> // For sleep
#include <stdlib.h> // For malloc
#include <string>

#include "NodeWiredTiger.hpp"

using namespace v8;

namespace wiredtiger {

static v8::Persistent<v8::FunctionTemplate> wttable_constructor;
static v8::Persistent<v8::String> emit_symbol;

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

void WTTable::Init(Handle<Object> target) {
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	Local<String> name = String::NewSymbol("WTTable");

	wttable_constructor = Persistent<FunctionTemplate>::New(tpl);
	wttable_constructor->InstanceTemplate()->SetInternalFieldCount(3);
	wttable_constructor->SetClassName(name);
	NODE_SET_PROTOTYPE_METHOD(tpl, "New", WTTable::New);
	NODE_SET_PROTOTYPE_METHOD(tpl, "Open", WTTable::Open);
	NODE_SET_PROTOTYPE_METHOD(tpl, "Put", WTTable::Put);
	NODE_SET_PROTOTYPE_METHOD(tpl, "Search", WTTable::Search);
	emit_symbol = NODE_PSYMBOL("emit");
	target->Set(name, wttable_constructor->GetFunction());
}

Handle<Value> WTTable::New(const Arguments &args) {
	HandleScope scope;

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
		config = NanFromV8String(args[2].As<v8::Object>(),
		    Nan::UTF8, NULL, NULL, 0, v8::String::NO_OPTIONS);
	}

	WTTable *table = new WTTable(wtconn, uri, config);

	table->Wrap(args.This());
	table->Ref();
	table->Emit = Persistent<Function>::New(
	    Local<Function>::Cast(table->handle_->Get(emit_symbol)));
	// Notify that the table is ready.
#if 0
	Handle<Value> eArgs[1] = { String::New("open") };
	fprintf(stderr, "Calling open event (I hope)\n");
	//table->Emit->Call(table->handle_, 1, eArgs);
	node::MakeCallback(args.This(), "emit", 1, eArgs);
#endif

	return args.This();
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

/* Called from callback thread. */
int WTTable::OpenTable() {
	WT_CONNECTION *conn;
	WT_SESSION *session;

	std::string final_config;
	size_t create_start;
	int ret;

	/* If there is no create in the config, nothing more to do. */
	if (config() == NULL)
	       return (0);
	final_config = std::string(config());
	if ((create_start = final_config.find("create")) == std::string::npos)
		return (0);

	/* There was a "create" - strip it from the original. */
	final_config = final_config.erase(create_start, 6);

	conn = wtconn()->conn();
	fprintf(stderr, "Creating table: %s, %s\n",
	    uri(), final_config.c_str());
	if ((ret = conn->open_session(conn, NULL, NULL, &session)) != 0)
		return (ret);
	if ((ret = session->create(
	    session, uri(), final_config.c_str())) != 0)
		return (ret);
	return (0);
}

Handle<Value> WTTable::Open(const Arguments &args) {
	HandleScope scope;

	wiredtiger::WTTable *table =
	    node::ObjectWrap::Unwrap<WTTable>(args.This());

	if (args.Length() != 1)
		return NanThrowError(
		    "WTTable::Open() requires a callback argument");
	v8::Local<v8::Function> callback = args[0].As<v8::Function>();
	OpenTableWorker *worker = new OpenTableWorker(
	    table,
	    new NanCallback(callback));

	// Avoid GC
	v8::Local<v8::Object> _this = args.This();
	worker->SavePersistent("table", _this);
	NanAsyncQueueWorker(worker);
	NanReturnUndefined();
}

typedef struct {
	Persistent<Function> javaCallback;
	uv_async_t *req;
	int op_ret;
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

	if (cookie->op_ret != 0)
		cookie->argv[0] = node::UVException(
		    0, "WTTable::Put", wiredtiger_strerror(cookie->op_ret));
	else
		cookie->argv[0] = Local<Value>::New(Null());
	cookie->javaCallback->Call(
	    Context::GetCurrent()->Global(), cookie->argc, cookie->argv);
	cookie->javaCallback.Dispose();
	uv_close((uv_handle_t *)cookie->req, NULL);
	delete cookie->argv;
	free(cookie);
}

static int
WTAsyncCallbackFunction(
    WT_ASYNC_CALLBACK *cb, WT_ASYNC_OP *op, int ret, uint32_t flags)
{
	//HandleScope scope;
	ASYNC_OP_COOKIE *cookie = (ASYNC_OP_COOKIE *)op->app_data;


	cookie->op_ret = ret;

	uv_async_send(cookie->req);

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

	// Get setup to call the WiredTiger async operation.
	ASYNC_OP_COOKIE *cookie =
	    (ASYNC_OP_COOKIE *)malloc(sizeof(ASYNC_OP_COOKIE));
	uv_async_t *req =
	    (uv_async_t *)malloc(sizeof(uv_async_t));
	uv_async_init(uv_default_loop(), req, HandleInsertOp);
	// Make sure callback isn't garbage collected.
	cookie->javaCallback = Persistent<Function>::New(callback);
	cookie->req = req;
	cookie->argv = new Local<Value>[1];
	cookie->argc = 1;
	req->data = cookie;
	// Setup the WiredTiger async operation
	WTConnection *wtconn;
	WT_ASYNC_OP *wtOp = NULL;
	wtconn = table->wtconn();
	if ((ret = wtconn->conn()->async_new_op(wtconn->conn(),
	    table->uri(), NULL, &WTAsyncCallback, &wtOp)) != 0) {
		fprintf(stderr, "Async op start failed: %s\n", wiredtiger_strerror(ret));
	}
	wtOp->app_data = cookie;
	wtOp->set_key(wtOp, key);
	wtOp->set_value(wtOp, value);
	if ((ret = wtOp->insert(wtOp)) != 0) {
		fprintf(stderr, "WTTable::Put async insert error: %d\n", ret);
	}

	// TODO: Save "this" into the cookie so it isn't GC'ed
	return Undefined();
}

NAN_METHOD(WTTable::Search) {
	NanScope();
	NanReturnUndefined();
}
}
