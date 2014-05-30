var wiredtiger = require('../'),
     crypto = require('crypto'),
     du = require('du'),
     fs = require('fs'),
     argv = require('optimist').argv,
     options = {
	 compression	: argv.compression	|| "zlib",
	 db		: argv.db		|| "/tmp/test",
	 numGets	: argv.numGets		|| 1000000,
	 numPuts	: argv.numPuts		|| 1000000,
	 getConcurrency	: argv.getConcurrency	|| 4,
	 putConcurrency	: argv.putConcurrency	|| 4,
	 valueSize	: argv.valueSize	|| 100
     },
     randomString = require('randomstring'),
     _ = require('underscore');

// Verify the command line options are OK.
if (options.compression != "zlib" && options.compression != "snappy" &&
    options.compression != "bzip2" && options.compression != "none") {
	console.error("Invalid compression engine: %s", options.compression);
	return
}

if (!fs.existsSync(options.db)) {
	console.error("Database directory %s must exist", options.db);
	return
}

// Setup some global variables.
const numPutsPerThread = options.numPuts / options.putConcurrency;
const numGetsPerThread = options.numGets / options.getConcurrency;
var startTime;
var queryStartTime;

//var stringOptions = JSON.stringify(options);
//var parsedOptions = JSON.parse(stringOptions);
//console.log(parsedOptions.numGets);
//console.log(JSON.stringify(parsedOptions, null, " "));
//console.log(options);

function logProgress(start, optype, ops) {
	du(options.db, function(err, size) {
		if (err)
			throw err
		console.log("%s %s in %d s, db size %d MB",
		    ops, optype,
		    Math.floor((Date.now() - start) / 1000),
		    Math.floor(size / 1024 / 1024));
	});
}

var conn = new wiredtiger.WTConnection(options.db, 'create');
conn.Open( function(err) {
	if (err)
		throw err

	var didPut = _.after(options.numPuts, afterPuts);
	var didGet = _.after(options.numGets, afterGets);
	var totalWrites = 0;
	var totalSearches = 0;
	var configString = "create";
	if (options.compression != "none")
		configString += ",block_compressor=" + options.compression;
	var table = new wiredtiger.WTTable(conn, 'table:test', configString);

	function doPut (threadNum, itemNum) {
		if (totalWrites++ % 100000 === 0)
			logProgress(startTime, "puts", totalWrites);
		if (itemNum == numPutsPerThread)
			return
		var keyOffset =
       		    (threadNum * options.numPutsPerThread) + itemNum;
		var data = randomString.generate(options.valueSize);
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
		for (var i = 0; i < options.putConcurrency; i++)
			doPut(i, 0);
		var cursor = new wiredtiger.WTCursor(table);
		cursor.Next( function(err, key, value) {
			if (err)
				throw err
			console.log("cursor next key: " + key);
		});

	});

	function doGet (threadNum, itemNum) {
		if (totalSearches++ % 100000 === 0)
			logProgress(queryStartTime, "searches", totalSearches);
		if (itemNum == numGetsPerThread)
			return
		var keyOffset =
	       	    (threadNum * options.numGetsPerThread) + itemNum;
		keyOffset = keyOffset % options.numPuts;
		table.Search('abc' + keyOffset, function(err, result) {
			if (err)
				throw err
			//console.log('Did put: ' + threadNum + ':' + itemNum);
			didGet();
			itemNum++;
			process.nextTick(function() {
				doGet(threadNum, itemNum) })
		});
	}
	function afterPuts() {
		console.log("Finished puts! Yay!");
		queryStartTime = Date.now()
		for (var i = 0; i < options.getConcurrency; i++) {
			doGet(i, 0);
		}
	}
});

function afterGets() {
	console.log("Retrieved " + options.numGets + " items");
}

