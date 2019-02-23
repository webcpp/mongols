# mongols

- A high performance network library for c++.

  - Libevent, libev and libuv are outdated.

  - Both apache and nginx are very slow.

![mongols.png](https://raw.githubusercontent.com/webcpp/mongols/master/example/html/image/mongols.png)

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


## example

[example](https://github.com/webcpp/mongols/tree/master/example)





