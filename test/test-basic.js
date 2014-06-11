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
const verbose = 1;
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
	var didTraverse = _.after(options.numGets, afterTraverse);
	var didCursorSearch = _.after(options.numGets, afterCursorSearch);
	var totalWrites = 0;
	var totalSearches = 0;
	var totalTraverse = 0;
	var configString = "create";
	if (options.compression != "none")
		configString += ",block_compressor=" + options.compression;
	var table = new wiredtiger.WTTable(conn, 'table:test', configString);

	function doPut (threadNum, itemNum) {
		if (itemNum == numPutsPerThread)
			return
		if (totalWrites++ % 100000 === 0 && verbose > 1)
			logProgress(startTime, "puts", totalWrites);
		var keyOffset =
       		    (threadNum * numPutsPerThread) + itemNum;
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
	});

	function doGet (threadNum, itemNum) {
		if (itemNum == numGetsPerThread)
			return
		if (totalSearches++ % 100000 === 0 && verbose > 1)
			logProgress(queryStartTime, "searches", totalSearches);
		var keyOffset =
	       	    (threadNum * numGetsPerThread) + itemNum;
		keyOffset = keyOffset % options.numPuts;
		table.Search('abc' + keyOffset, function(err, result) {
			if (err)
				throw err
			didGet();
			itemNum++;
			process.nextTick(function() {
				doGet(threadNum, itemNum) })
		});
	}

	function afterPuts() {
		console.log("Finished puts");
		logProgress(startTime, "puts", totalWrites);
		queryStartTime = Date.now()
		for (var i = 0; i < options.getConcurrency; i++) {
			doGet(i, 0);
		}
	}

	// traverse with an iterator
	function getNext (cursor) {
		if (totalTraverse++ % 100000 === 0 && verbose > 1)
			logProgress(queryStartTime, "iterator next", totalTraverse);
		cursor.Next( function(err, key, value) {
			if (err)
				throw err
			if (key == null) {
				console.log("Reached end of cursor traverse");
				cursor.Close();
			} else {
				didTraverse();
				process.nextTick(function() {
				    getNext(cursor) })
			}
		});
	}

	function afterGets() {
		console.log("Retrieved " + options.numGets + " items");
		logProgress(queryStartTime, "searches", totalSearches);
		console.log("About to traverse with cursor");
		queryStartTime = Date.now()
		var cursor = new wiredtiger.WTCursor(table);
		getNext(cursor);
	}

	function doCursorSearch(cursor, threadNum, itemNum) {
		if (itemNum == numGetsPerThread) {
			cursor.Close();
			return;
		}
		if (totalSearches++ % 100000 === 0 && verbose > 1)
			logProgress(queryStartTime, "cursor searches", totalSearches);
		var keyOffset =
	       	    (threadNum * numGetsPerThread) + itemNum;
		keyOffset = keyOffset % options.numPuts;
		cursor.Search('abc' + keyOffset, function(err, result) {
			if (err)
				throw err
			didCursorSearch();
			itemNum++;
			process.nextTick(function() {
				doCursorSearch(cursor, threadNum, itemNum) })
		});
	}

	function afterTraverse() {
		logProgress(queryStartTime, "iterator next", totalTraverse);
		totalSearches = 0;
		queryStartTime = Date.now()
		for (var i = 0; i < options.getConcurrency; i++) {
			var cursor = new wiredtiger.WTCursor(table);
			doCursorSearch(cursor, i, 0);
		}
	}
	function afterCursorSearch() {
		logProgress(queryStartTime, "cursor searches", totalSearches);
		console.log("Finished cursor searches");
	}
});

