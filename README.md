WiredTiger Node.js API
===============

Fast transactional storage - a Node.js wrapper around the
[WiredTiger](http://wiredtiger.com) storage engine library.

Introduction
------------

WiredTiger is a fast transactional data storage engine that provides
high throughput data access and storage in a concurrent environment.

WiredTiger stores data on local disks. A single WiredTiger database can
contain multiple tables. Tables can be created in different formats to suit
the intended workload. Available table types are:
 * Log Structured Merge (LSM) tree. Especially good for high insert/update
   workloads, where the volume of data is large.
 * Btree. Especially good for smaller datasets, or workloads that have a
   relatively higher rate of queries compared to updates.

Platforms
---------

WiredTiger currently supports Linux, FreeBSD and Mac OS.

Usage
-----

The best way to see usage is to view the test code. The code at
test/test-basic.js is the recommended starting point.

The following might help (though it could be outdated).

```
var wiredtiger = require('wiredtiger');

var conn = new wiredtiger.WTConnection('/path/to/database/directory', 'create');
conn.Open( function(err) {
	var table = new wiredtiger.WTTable(
	    conn, "table:test", "create,key_format=S,value_format=S");
	table.Open( function(err) {
		table.Put('abc', 'def', function(err) {
			table.Search('abc', function(err, result) {
				console.log("Woo! got: " + result);
			});
		});
	});
});
```
