var wiredtiger = require('../'),
     _ = require('underscore');

const numPuts = 5;

var conn = new wiredtiger.WTConnection(
    '/tmp/test', 'create,async=(enabled=true)');
conn.Open( function(err) {
	if (err)
		throw err

	var didPut = _.after(numPuts, afterPuts);
	var didGet = _.after(numPuts, afterGets);
	var table = new wiredtiger.WTTable(
	    conn, 'table:test', 'create,key_format=S,value_format=S');
	table.Open( function(err) {
		if (err)
			throw err
		var inserted = 0;
		for (var i = 0; i < numPuts; i++) {
			table.Put('abc' + i, 'def' + i, function(err) {
				if (err)
					throw err
				didPut();
				console.log("Put into table");
			});
		}
	});

	function afterPuts() {
		console.log("Finished puts! Yay!");
		for (var i = 0; i < numPuts; i++) {
			console.log('About to get');
			table.Search('abc' + i, function(err, result) {
				if (err)
					throw err
				console.log("Got from table " + result);
				didGet();
			});
		}
	}
});

function afterGets() {
	console.log("Retrieved " + numPuts + " items");
}

