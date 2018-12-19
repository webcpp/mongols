# mongols
a library for c++

Libevent, libev and libuv are outdated.

## dependency

- linux
- gcc (-std=c11)
- g++ (-std=c++14)


## feature
epoll or epoll + multi-threading + multi-process
- tcp sever 
- http server
- websocket server 
- web server 
- leveldb server 
- lua server
- sqlite server
- medis_server (like redis but support sqlite and leveldb)
- javascript server
- chaiscript server
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

2 core,4 GB,linux mint 18:

### http

`ab -kc 10000 -n 500000 -H 'Connection: keep-alive' http://127.0.0.1:9090/`

![ab_http.png](https://raw.githubusercontent.com/webcpp/mongols/master/benchmark/ab_http.png)

`wrk -t 50 -d 30s -c 10000 -H 'Connection: keep-alive' http://127.0.0.1:9090/`

![wrk_http.png](https://raw.githubusercontent.com/webcpp/mongols/master/benchmark/wrk_http.png)

### web

![ab_web.png](https://raw.githubusercontent.com/webcpp/mongols/master/benchmark/ab_web.png)

![wrk_web.png](https://raw.githubusercontent.com/webcpp/mongols/master/benchmark/wrk_web.png)

#### web_server VS nginx(1 worker)

![mongolsVSnginx.png](https://raw.githubusercontent.com/webcpp/mongols/master/benchmark/mongolsVSnginx.png)

### multi-process web


![ab_multi_process_web_server.png](https://raw.githubusercontent.com/webcpp/mongols/master/benchmark/ab_multi_process_web_server.png)

![wrk_multi_process_web_server.png](https://raw.githubusercontent.com/webcpp/mongols/master/benchmark/wrk_multi_process_web_server.png)

![nginxVSmongols.png](https://raw.githubusercontent.com/webcpp/mongols/master/benchmark/nginxVSmongols.png)

### medis

![medis.png](https://raw.githubusercontent.com/webcpp/mongols/master/benchmark/medis.png)

## example

[example](https://github.com/webcpp/mongols/tree/master/example)
