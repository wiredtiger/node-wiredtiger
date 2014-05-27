var wiredtiger = require('../'),
     _ = require('underscore');

const concurrency = 50;
const numPuts = 500000;
const numPutsPerThread = numPuts / concurrency;

var conn = new wiredtiger.WTConnection(
    '/tmp/test', 'create,async=(enabled=true,ops_max=4096)');
conn.Open( function(err) {
	if (err)
		throw err

	var didPut = _.after(numPuts, afterPuts);
	var didGet = _.after(numPuts, afterGets);
	var totalWrites = 0;
	var totalSearches = 0;
	var table = new wiredtiger.WTTable(
	    conn, 'table:test', 'create,key_format=S,value_format=S');

	function doPut (threadNum, itemNum) {
		if (totalWrites == numPuts) {
			console.log("Finished " + numPuts + " writes");
			return
		}
		if (itemNum >= numPutsPerThread)
			return
		var keyOffset = (threadNum * numPutsPerThread) + itemNum;
		table.Put('abc' + keyOffset, 'def' + keyOffset, function(err) {
			if (err)
				throw err
			//console.log('Did put: ' + threadNum + ':' + itemNum);
			didPut();
			totalWrites++;
			itemNum++;
			process.nextTick(function() {
				doPut(threadNum, itemNum) })
		});
	}
	table.Open( function(err) {
		if (err)
			throw err
		for (var i = 0; i < concurrency; i++)
			doPut(i, 0);
	});

	function doGet (threadNum, itemNum) {
		if (totalSearches++ == numPuts) {
			console.log("Finished " + numPuts + " searches");
			return
		}
		if (itemNum >= numPutsPerThread)
			return
		var keyOffset = (threadNum * numPutsPerThread) + itemNum;
		table.Search('abc' + keyOffset, function(err, result) {
			if (err)
				throw err
			//console.log('Did put: ' + threadNum + ':' + itemNum);
			didGet();
			totalSearches++;
			itemNum++;
			process.nextTick(function() {
				doGet(threadNum, itemNum) })
		});
	}
	function afterPuts() {
		console.log("Finished puts! Yay!");
		for (var i = 0; i < concurrency; i++) {
			doGet(i, 0);
		}
	}
});

function afterGets() {
	console.log("Retrieved " + numPuts + " items");
}

