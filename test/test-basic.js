var wiredtiger = require('../'),
     crypto = require('crypto'),
     du = require('du'),
     randomString = require('randomstring'),
     _ = require('underscore');

const concurrency = 10;
const numPuts = 1000000;
const numPutsPerThread = numPuts / concurrency;
var startTime;

var conn = new wiredtiger.WTConnection(
    '/tmp/test', 'create,async=(enabled=true,ops_max=4096),extensions=[lib/libwiredtiger_zlib.so,lib/libwiredtiger_bzip2.so,lib/libwiredtiger_snappy.so]');
conn.Open( function(err) {
	if (err)
		throw err

	var didPut = _.after(numPuts, afterPuts);
	var didGet = _.after(numPuts, afterGets);
	var totalWrites = 0;
	var totalSearches = 0;
	var table = new wiredtiger.WTTable(
	    conn, 'table:test', 'create,key_format=S,value_format=S,block_compressor=snappy');
	//var data = crypto.pseudoRandomBytes(100)

	function doPut (threadNum, itemNum) {
		if (totalWrites++ == numPuts) {
			console.log("Finished " + numPuts + " puts in: " +
			    Math.floor((Date.now() - startTime) / 1000) + 's');
			du('/tmp/test', function(err, size) {
				if (err)
					throw err
				console.log("Database size: " +
				    Math.floor(size / 1024 / 1024) + "M");
			});
			return
		}
		if (totalWrites % 100000 === 0)
			console.log("Finished " + totalWrites + " puts in: " +
			    Math.floor((Date.now() - startTime) / 1000) + 's');
		if (itemNum >= numPutsPerThread)
			return
		var keyOffset = (threadNum * numPutsPerThread) + itemNum;
		var data = randomString.generate(100);
		table.Put('abc' + keyOffset, data, function(err) {
			if (err)
				throw err
			//console.log('Did put: ' + threadNum + ':' + itemNum);
			didPut();
			itemNum++;
			process.nextTick(function() {
				doPut(threadNum, itemNum) })
		});
	}
	table.Open( function(err) {
		if (err)
			throw err
		startTime = Date.now()
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

