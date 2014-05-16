#include <node.h>
#include <nan.h>
#include <v8.h>

#include "wiredtiger.h"
#include "NodeWiredTiger.hpp"

using namespace v8;

namespace wiredtiger {

NAN_METHOD(createDatabase) {
	NanScope();

	if (args.Length() < 2) {
		ThrowException(Exception::TypeError(
		    String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	if (!args[0]->IsString() || !args[1]->IsString()) {
		ThrowException(Exception::TypeError(
		    String::New("Wrong argument type")));
		return scope.Close(Undefined());
	}

#if 0
	WT_CONNECTION *conn;
	String::AsciiValue homeDir(args[0]->ToString());
	String::AsciiValue openOptions(args[1]->ToString());

	if ((ret = wiredtiger_open(
	    *homeDir, NULL, *openOptions, &conn)) != 0) {
		ThrowException(Exception::TypeError(
		    String::New("WiredTiger open failed")));
		return scope.Close(Undefined());
	}
#endif
	v8::Local<v8::String> home = args[0]->ToString();
	v8::Local<v8::String> options = args[1]->ToString();
	NanReturnValue(WTConnection::NewInstance(home, options));
}

void init(Handle<Object> target) {
	WTConnection::Init();
	v8::Local<v8::Function> wiredtiger =
	    v8::FunctionTemplate::New(WiredTiger)->GetFunction();
	target->Set(NanSymbol("wiredtiger"), wiredtiger);
}
NODE_MODULE(wiredtiger, init)

} // namespace wiredtiger
