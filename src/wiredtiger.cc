/*-
 * Copyright (c) 2014- WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

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
}

NODE_MODULE(wiredtiger, InitAll)

}
