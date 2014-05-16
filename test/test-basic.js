const wiredtiger = require('../')

var conn = wiredtiger('/tmp/test', 'create')
conn.Open( function(err) {
	if (err)
		throw err
	console.log("Opened database")
	conn.Create( 'table:test', 'key_format=s,value_format=s',
	    function(err) {
		    if (err)
			throw err
		console.log("Created table")
	    })
})
