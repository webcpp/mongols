# mongols
a library for c++

The best server platform is not nginx,but mongols.

## dependency

- linux
- gcc (-std=c11)
- g++ (-std=c++11)


## feature
epoll or epoll + multi-threading
- tcp sever 
- http server
- websocket server 
- web server 
- leveldb server 
- lua server
- sqlite server
- medis_server (like redis but support sqlite and leveldb)
- utils


## install 

`make clean && make -j2 && sudo make install && sudo ldconfig`

## usage

`pkg-config --libs --cflags mongols`

### leveldb_server usage
  - successful: 200
  - failed: 500

- POST `curl -d'key=value'  http://host/key`
- GET  `curl http://host/key`
- DELETE `curl -X DELETE http://host/key`

### sqlite_server usage
 - successful: 200
 - failed: 500
 - sql_type: cmd,transaction,query
 - sql: SQL statement
 - result: JSON

POST `curl -d 'sql_type=x' -d 'sql=sql_statement' http://127.0.0.1:9090/`

example: `curl -d'sql_type=query' -d'sql=select * from test limit 3;' http://127.0.0.1:9090/`

result: `{"error":null,"result":[{"id":1,"name":"a"},{"id":2,"name":"b"},{"id":3,"name":"c"}]}`

## benchmark

2 core,4 GB,linux mint 18,2 thread:

### http

`ab -kc 10000 -n 500000 -H 'Connection: keep-alive' http://127.0.0.1:9090/`

![ab_http.png](https://github.com/webcpp/mongols/blob/master/ab/ab_http.png)

`wrk -t 50 -d 30s -c 10000 -H 'Connection: keep-alive' http://127.0.0.1:9090/`

![wrk_http.png](https://github.com/webcpp/mongols/blob/master/ab/wrk_http.png)

### web

![ab_web.png](https://github.com/webcpp/mongols/blob/master/ab/ab_web.png)

![wrk_web.png](https://github.com/webcpp/mongols/blob/master/ab/wrk_web.png)

### medis

![medis.png](https://github.com/webcpp/mongols/blob/master/ab/medis.png)

## example

```cpp

#include <mongols/tcp_server.hpp>
#include <mongols/tcp_threading_server.hpp>
#include <mongols/http_server.hpp>
#include <mongols/ws_server.hpp>
#include <mongols/web_server.hpp>
#include <mongols/leveldb_server.hpp>
#include <mongols/lua_server.hpp>
#include <mongols/sqlite_server.hpp>
#include <mongols/medis_server.hpp>
#include <mongols/util.hpp>
#include <iostream>

/*
//websocket server
int main(int,char**){
	int port=9090;
	const char* host="127.0.0.1";
	mongols::ws_server server(host,port,5000,8096,4);

	auto f=[](const std::string& input
            , bool& keepalive
            , bool& send_to_other
            , mongols::tcp_server::client_t& client
            , mongols::tcp_server::filter_handler_function& send_to_other_filter){
			keepalive = KEEPALIVE_CONNECTION;
			send_to_other=true;
			if(input=="close"){
				keepalive = CLOSE_CONNECTION;
				send_to_other = false;
			}
	};
	server.run(f);
	//server.run();
}
*/


/*
//tcp server or tcp multi-threading server
int main(int,char**)
{
	auto f=[](const std::pair<char*,size_t>& input
					 , bool & keepalive
                , bool& send_to_other
                , mongols::tcp_server::client_t& client
                , mongols::tcp_server::filter_handler_function& send_to_other_filter){
					keepalive= KEEPALIVE_CONNECTION;
					send_to_other=true;
					return std::string(input.first,input.second);
				};
	int port=9090;
	const char* host="127.0.0.1";
	mongols::tcp_server
           //server(host,port,5000,8096,2);
	server(host,port);
	server.run(f);

}
*/




//http server or multi-threading server
int main(int,char**)
{
	auto f=[](const mongols::request&){
		return true;
	};
	auto g=[](const mongols::request& req,mongols::response& res){
		//std::unordered_map<std::string, std::string>::const_iterator i;
		//if((i=req.session.find("test"))!=req.session.end()){
		//	long test=std::stol(i->second)+1;
		//	res.content=std::to_string(test);
		//	res.session["test"]=res.content;
		//}else{
		//	res.content=std::to_string(0);;
		//	res.session["test"]=res.content;
		//}
		res.content=std::move("hello,world");
		res.status=200;
	};
	int port=9090;
	const char* host="127.0.0.1";
	mongols::http_server 
	//server(host,port,5000,8096,4);
	server(host,port);
	server.set_enable_session(false);
	server.set_enable_cache(false);
	server.run(f,g);
}

/*
//web server or multi-threading server
int main(int,char**)
{
	auto f=[](const mongols::request& req){
		if(req.method=="GET"&&req.uri.find("..")==std::string::npos){
			return true;
		}
		return false;
	};
	int port=9090;
	const char* host="127.0.0.1";
	mongols::web_server 
	//server(host,port,5000,8096,4);
	server(host,port);
	server.set_root_path("html");
	server.set_mime_type_file("mime.conf");
	server.set_list_directory(true);
	server.run(f);
}
*/

/*
//leveldb_server or multi-threading server
int main(int,char**){
	int port=9090;
	const char* host="127.0.0.1";
	mongols::leveldb_server 
	//server(host,port,5000,8096,4);
	server(host,port);
	server.run("html/leveldb");
}
*/


/*
//lua_server or multi-threading server
int main(int,char**){
	int port = 9090;
	const char* host="127.0.0.1";
	mongols::lua_server 
	//server(host,port,5000,8096,2);
	server(host,port);
	server.set_root_path("html/lua");
	server.run("html/lua/package/?.lua;","html/lua/package/?.so;");

}
*/

/*
//sqlite_server or multi-threading server
int main(int,char**){
	int port = 9090;
	const char* host="127.0.0.1";
	mongols::sqlite_server 
	//server(host,port,5000,8096,2);
	server(host,port);
	server.run("html/sqlite/test.db");

}
*/

/*
// medis_server or multi-threading server
int main(int,char**){
	int port = 9090;
	const char* host="127.0.0.1";
	mongols::medis_server
	server(host,port,5000,8096,0);
	server.ready();
	server.run("html/leveldb","html/sqlite/test.db");

}
*/

```

