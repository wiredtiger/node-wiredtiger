const wiredtiger = require('../')

var conn = wiredtiger('/tmp/test', 'create')
conn.Open( function(err) {
	if (err)
		throw err
	console.log("Opened database")
})
