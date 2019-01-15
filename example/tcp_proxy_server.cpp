#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/prctl.h>

#include <mongols/util.hpp>
#include <mongols/tcp_proxy_server.hpp>

#include <cstring>
#include <iostream>
#include <functional>

int main(int, char**) {
    //    daemon(1, 0);
    auto f = [](const mongols::tcp_server::client_t & client) {
        return true;
    };

    int port = 9090;
    const char* host = "127.0.0.1";

    mongols::tcp_proxy_server server(host, port, 5000, 8192, 0/*2*/);


    //see example/nodejs
    server.set_backend_server(host, 8886);
    server.set_backend_server(host, 8887);
    if (!server.set_openssl("openssl/localhost.crt", "openssl/localhost.key")) {
        return -1;
    }

    server.run(f);

}
