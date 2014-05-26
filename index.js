//
// Copyright (c) 2014- WiredTiger, Inc.
// All rights reserved.
//
// See the file LICENSE for redistribution information.
//

//module.exports = require('bindings')('wiredtiger.node')

var EventEmitter = require('events').EventEmitter,
    addon = require('bindings')('wiredtiger.node');

addon.WTTable.prototype.__proto__ = EventEmitter.prototype;

module.exports = addon;
