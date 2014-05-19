#include <node.h>
#include <nan.h>
#include <v8.h>

#include "wiredtiger.h"
#include "NodeWiredTiger.hpp"

using namespace v8;

namespace wiredtiger {

void init(Handle<Object> target) {
	WTConnection::Init();
	WTTable::Init();
	v8::Local<v8::Function> wiredtiger =
	    v8::FunctionTemplate::New(WiredTiger)->GetFunction();
	target->Set(NanSymbol("wiredtiger"), wiredtiger);
}
NODE_MODULE(wiredtiger, init)

} // namespace wiredtiger
