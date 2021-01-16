# mongols

A high performance network library for c++:

- Libevent, libev and libuv are outdated.

- Both apache and nginx are very slow.


![nginx_4_worker](benchmark/nginx_4_worker.png)
![mongols_4_worker](benchmark/mongols_4_worker.png)
![nginx_vs_mongols](benchmark/nginx_vs_mongols.png)


## dependency

- linux
- gcc (-std=c11)
- g++ (-std=c++11)
- openssl

## feature

[mongols document](https://mongols.hi-nginx.com)

## install

`make clean && make -j2 && sudo make install && sudo ldconfig`

## usage

`pkg-config --libs --cflags mongols openssl`

## binding

[pymongols](https://github.com/webcpp/pymongols)

## example

[example](https://github.com/webcpp/mongols/tree/master/example)
