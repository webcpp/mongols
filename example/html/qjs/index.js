//import * as std from "std";
//import * as os from "os";
//import * as bjson from "bjson";
import * as mongols from "mongols";
//import * as hash from "hash";
//import Mustache from "./mustache.mjs"


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


mongols.status(200);
mongols.header('Content-Type', 'text/plain;charset=utf-8');
mongols.content('hello,world\n');