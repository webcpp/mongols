//import * as std from "std";
//import * as os from "os";
//import * as bjson from "bjson";
//import * as hash from "hash";
import * as mongols from "mongols";
//import Mustache from "./mustache.mjs"
//import route from "./route.mjs"


/*
var view = {
    "musketeers": ["Athos", "Aramis", "Porthos", "D'Artagnan"]
  };

var tpl = "{{#musketeers}}* {{.}}{{/musketeers}}";

var content = Mustache.render(tpl, view);

mongols.status(200);
mongols.header('Content-Type', 'text/html;charset=utf-8');
mongols.content(content);
*/

/*
var r = route.get_instance();

r.get('^\/', function (m, param) {
  m.status(200);
  m.header('Content-Type', 'text/plain;charset=utf-8');
  m.content('hello,world\n');
});

r.get('^\/get/?(\\w+)$', function (m, param) {
  m.header('Content-Type', 'text/plain;charset=UTF8')
  m.content(m.method() + '\n' + m.uri() + '\n' + param.toString())
  m.status(200)
});

r.run(mongols)
*/

mongols.status(200);
mongols.header('Content-Type', 'text/plain;charset=utf-8');
mongols.content('hello,world\n');