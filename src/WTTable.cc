#include <node.h>
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

NAN_METHOD(WTTable::Put) {
	NanScope();
	NanReturnUndefined();
}
NAN_METHOD(WTTable::Search) {
	NanScope();
	NanReturnUndefined();
}
}
