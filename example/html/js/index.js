/*
var foo = require('foo')
mongols_res.header('Content-Type','text/plain;charset=UTF-8')
mongols_res.content(foo.hello())
mongols_res.status(200)
*/

/*
var math = require('math/math')
var v=math.minus(1,3)/math.add(100,3)
mongols_res.header('Content-Type','text/plain;charset=UTF-8')
mongols_res.content(v.toString())
mongols_res.status(200)
*/


/*
var loaded = mongols_module.require('adder/libadder.so','adder')
mongols_res.header('Content-Type','text/plain;charset=UTF-8')
mongols_res.content(loaded?adder(1,2).toString():'failed load c module.')
mongols_res.status(200)
*/

/*
var loaded = mongols_module.require('concat/libconcat.so','concat')
mongols_res.header('Content-Type','text/plain;charset=UTF-8')
mongols_res.content(loaded?concat('Hello,','world'):'failed load c module.')
mongols_res.status(200)
*/

/*
var mustache = require('mustache')
var template = '<h1>Test mustache engine</h1><p>{{body}}</p>';
var model = {body:'Hello World'};
mongols_res.content(mustache.to_html(template,model))
mongols_res.status(200)
*/

/*
var handlebars = require('handlebars')
var template = handlebars.compile('<h1>Test handlebars engine</h1><p>{{body}}</p>')
var model = {body:'Hello World'};
mongols_res.content(template(model))
mongols_res.status(200)
*/


///*
mongols_res.header('Content-Type','text/plain;charset=UTF-8')
mongols_res.content('hello,world')
mongols_res.status(200)
//*/