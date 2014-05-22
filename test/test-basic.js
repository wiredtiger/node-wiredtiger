var wiredtiger = require('../'),
     _ = require('underscore');

const numPuts = 10;
var didPut = _.after(numPuts, afterPuts);

console.log("About to create connection");
var conn = new wiredtiger.WTConnection('/tmp/test', 'create');
console.log("About to open connection");
conn.Open( function(err) {
	if (err)
		throw err
	console.log("Opened database");
	var table = new wiredtiger.WTTable(
	    conn, 'table:test', 'create,key_format=s,value_format=s');
	table.Open( function(err) {
		console.log('Got open event');
		var inserted = 0;
		for (var i = 0; i < numPuts; i++) {
			table.Put('abc' + i, 'def', function(err) {
				if (err)
					throw err
				didPut();
				console.log("Put into table")
			});
		}
	});
});

function afterPuts() {
}
