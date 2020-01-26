import * as mongols from "mongols";
import Mustache from "./lib/mustache.mjs"

var view = {
    "musketeers": ["Athos", "Aramis", "Porthos", "Artagnan"]
};

var tpl = "{{#musketeers}}* {{.}}{{/musketeers}}";



var tpl_test = function (m) {
    m.status(200);
    m.header('Content-Type', 'text/plain;charset=utf-8');
    m.content(Mustache.render(tpl, view));
};


export default tpl_test;

