/*
var math = require('math/math')
var v=math.minus(1,3)/math.add(100,3)
mongols_res.header('Content-Type','text/plain;charset=UTF-8')
mongols_res.content(v.toString())
mongols_res.status(200)
*/

var foo = require('foo')
mongols_res.header('Content-Type','text/plain;charset=UTF-8')
mongols_res.content(foo.hello())
mongols_res.status(200)

