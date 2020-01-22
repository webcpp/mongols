import * as std from "std";
import * as os from "os";
import * as bjson from "bjson";
import * as mongols from "mongols";

mongols.status(200);
mongols.header('Content-Type', 'text/plain');
mongols.content("hello,world\n");