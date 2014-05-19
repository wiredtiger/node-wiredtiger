const wiredtiger = require('../')
var _ = require('underscore')

const numPuts = 10
var didPut = _.after(numPuts, afterPuts);

var conn = wiredtiger('/tmp/test', 'create')
conn.Open( function(err) {
	if (err)
		throw err
	console.log("Opened database")
	var table = conn.OpenTable(
	    'table:test', 'create,key_format=s,value_format=s',
	    function(err) {
		    if (err)
			throw err
		console.log("Created table")
		var inserted = 0;
		for (var i = 0; i < numPuts; i++) {
			//table.Put('abc' + i, 'def', function(err) {
			//	if (err)
			//		throw err
				didPut();
			//	console.log("Put into table")
			//});
		}
	});
});

function afterPuts() {
}
