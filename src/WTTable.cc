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

/*
 * A container class that is used to store information passed through
 * the callbacks required to execute a WiredTiger async operation.
 */
class AsyncOpData
{
public:
	AsyncOpData(
	    Persistent<Function> javaCallback,
	    Persistent<Object> savedThis,
	    Persistent<String> key,
	    Persistent<String> value,
	    uv_async_t *req,
	    int argc,
	    Handle<Value> *argv) :
	    javaCallback_(javaCallback), savedThis_(savedThis),
	    key_(key), value_(value), req_(req), argc_(argc), argv_(argv)
	{}

	~AsyncOpData() {
		javaCallback_.Dispose();
		savedThis_.Dispose();
		key_.Dispose();
		value_.Dispose();
		uv_close((uv_handle_t *)req_, NULL);
		delete argv_;
	}

	Persistent<Function> getJavaCallback() { return javaCallback_; }
	Persistent<Object> getSavedThis() { return savedThis_; }
	Persistent<String> getKey() { return key_; }
	Persistent<String> getValue() { return value_; }
	uv_async_t *getReq() { return req_; }
	int getArgc() { return argc_; }
	Handle<Value> *getArgv() { return argv_; }
	int getOpRet() { return opRet_; }

	/* Only some things can be altered after create. */
	void setOpRet(int opRet) { opRet_ = opRet; }
private:
	/* Disable default constructor. */
	AsyncOpData();

	Persistent<Function> javaCallback_;
	Persistent<Object> savedThis_;
	Persistent<String> key_;
	Persistent<String> value_;
	uv_async_t *req_;
	int opRet_;
	/* Setup in WT callback */
	int argc_;
	Handle<Value> *argv_;
};

static void
HandleInsertOp(uv_async_t *handle, int status /* unused */) {
	// This is called in the Java thread, so it can call
	// the Java callback.
	AsyncOpData *cookie = static_cast<AsyncOpData *>(handle->data);
	if (cookie->getOpRet() != 0)
		cookie->getArgv()[0] = node::UVException(0,
		    "WTTable::Put", wiredtiger_strerror(cookie->getOpRet()));
	else
		cookie->getArgv()[0] = Local<Value>::New(Null());
	cookie->getJavaCallback()->Call(Context::GetCurrent()->Global(),
	    cookie->getArgc(), cookie->getArgv());
	delete cookie;
}

static int
WTAsyncCallbackFunction(
    WT_ASYNC_CALLBACK *cb, WT_ASYNC_OP *op, int ret, uint32_t flags)
{
	//HandleScope scope;
	AsyncOpData *cookie = (AsyncOpData *)op->app_data;

	cookie->setOpRet(ret);

	uv_async_send(cookie->getReq());

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

	// Get setup to call the WiredTiger async operation.
	uv_async_t *req =
	    (uv_async_t *)malloc(sizeof(uv_async_t));
	uv_async_init(uv_default_loop(), req, HandleInsertOp);
	AsyncOpData *cookie = new AsyncOpData(
	    Persistent<Function>::New(callback),
	    Persistent<Object>::New(args.This()),
	    Persistent<String>::New(args[0].As<String>()),
	    Persistent<String>::New(args[1].As<String>()),
	    req,
	    1,
	    new Local<Value>[1]);
	req->data = cookie;
	// Setup the WiredTiger async operation
	WTConnection *wtconn;
	WT_ASYNC_OP *wtOp = NULL;
	wtconn = table->wtconn();
	if ((ret = wtconn->conn()->async_new_op(wtconn->conn(),
	    table->uri(), NULL, &WTAsyncCallback, &wtOp)) != 0) {
		ThrowException(
		    Exception::Error(String::Concat(String::New(
		    "WTTable::Put() WiredTiger async_new_op error"),
		    String::New(wiredtiger_strerror(ret)))));
		return Undefined();
	}
	wtOp->app_data = cookie;
	wtOp->set_key(wtOp, *String::Utf8Value(cookie->getKey()));
	wtOp->set_value(wtOp, *String::Utf8Value(cookie->getValue()));
	if ((ret = wtOp->insert(wtOp)) != 0) {
		ThrowException(
		    Exception::Error(String::Concat(String::New(
		    "WTTable::Put() WiredTiger insert error"),
		    String::New(wiredtiger_strerror(ret)))));
		return Undefined();
	}

	return Undefined();
}

NAN_METHOD(WTTable::Search) {
	NanScope();
	NanReturnUndefined();
}
}
