#include <node.h>
#include <nan.h>
#include <v8.h>

#include "wiredtiger.h"
#include "NodeWiredTiger.hpp"

using namespace v8;

extern "C" {
void InitAll(Handle<Object> target) {
	HandleScope scope;
	wiredtiger::WTConnection::Init(target);
	wiredtiger::WTTable::Init(target);
	//Local<Function> wiredtiger = FunctionTemplate::New(WiredTiger)->GetFunction();
	//target->Set(String::NewSymbol("wiredtiger"), wiredtiger);
}

NODE_MODULE(wiredtiger, InitAll)

}
