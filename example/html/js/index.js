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
 var loaded = mongols_module.require('adder/libadder','adder')
 mongols_res.header('Content-Type','text/plain;charset=UTF-8')
 mongols_res.content(loaded?adder(1,2).toString():'failed load c module.')
 mongols_res.status(200)
 */

/*
 var loaded = mongols_module.require('concat/libconcat','concat')
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


/*
 var echo = require('echo')
 var t=new echo()
 t.set('HELLO, ')
 mongols_res.content(t.concat('Tom'))
 mongols_res.status(200)
 */


/*
 var handlebars = require('handlebars')
 var s=new studest()
 s.set_name("Jerry")
 s.set_age(14)
 s.set_score(74.6)
 var text='hello,world'
 var tpl=handlebars.compile('name: {{name}}\nage: {{age}}\nscore: {{score}}\ntext:{{text}}\ntext_md5: {{md5}}\ntext_sha1: {{sha1}}')
 var content=tpl({name:s.get_name(),age:s.get_age(),score:s.get_score(),text:text,md5:md5(text),sha1:sha1(text)})
 mongols_module.free(s)
 mongols_res.header('Content-Type','text/plain;charset=UTF-8')
 mongols_res.content(content)
 mongols_res.status(200)
 */


/*

var route = require('route').get_instance()

route.get('^\/get/?(\\w+)$', function (req, res, param) {
    res.header('Content-Type', 'text/plain;charset=UTF8')
    res.content(req.method()+'\n'+req.uri() + '\n' + param.toString())
    res.status(200)
})

route.post('^\/post/?([a-zA-Z]+)?$', function (req, res, param) {
    res.header('Content-Type', 'text/plain;charset=UTF8')
    res.content(req.method()+'\n'+req.uri() + '\n' + param.toString())
    res.status(200)
})

route.put('^\/put/?([0-9]+)?', function (req, res, param) {
    res.header('Content-Type', 'text/plain;charset=UTF8')
    res.content(req.method()+'\n'+req.uri() + '\n' + param.toString())
    res.status(200)
})

route.add(['GET', 'POST', 'PUT'], '^\/(.*)$', function (req, res, param) {
    res.header('Content-Type', 'text/plain;charset=UTF8')
    res.content(req.method()+'\n'+req.uri() + '\n' + param.toString())
    res.status(200)
})

route.run(mongols_req, mongols_res)



*/


///*
 mongols_res.header('Content-Type','text/plain;charset=UTF-8')
 mongols_res.content('hello,world')
 mongols_res.status(200)
//*/
