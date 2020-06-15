# mongols

A high performance network library for c++:

- Libevent, libev and libuv are outdated.

- Both apache and nginx are very slow.

![mongols.png](https://raw.githubusercontent.com/webcpp/mongols/master/mongols.drawio.png)

## benchmark

```txt

mongols@mongols ~/Downloads $ grep MemTotal /proc/meminfo
MemTotal:        3939312 kB
mongols@mongols ~/Downloads $ grep 'model name' /proc/cpuinfo
model name	: Intel(R) Core(TM) i5-5200U CPU @ 2.20GHz
model name	: Intel(R) Core(TM) i5-5200U CPU @ 2.20GHz
model name	: Intel(R) Core(TM) i5-5200U CPU @ 2.20GHz
model name	: Intel(R) Core(TM) i5-5200U CPU @ 2.20GHz
mongols@mongols ~/Downloads $ uname -a
Linux mongols 4.4.0-21-generic #37-Ubuntu SMP Mon Apr 18 18:33:37 UTC 2016 x86_64 x86_64 x86_64 GNU/Linux


```


mongols web_server 1 worker VS nginx 1 worker:

![mongolsVSnginx_2.png](https://raw.githubusercontent.com/webcpp/mongols/master/benchmark/mongolsVSnginx_2.png)

![mongolsVSnginx_3.png](https://raw.githubusercontent.com/webcpp/mongols/master/benchmark/mongolsVSnginx_3.png)

mongols web_server 4 worker VS nginx 4 worker:

![mongolsVSnginx_4_worker.png](https://raw.githubusercontent.com/webcpp/mongols/master/benchmark/mongolsVSnginx_4_worker.png)


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
