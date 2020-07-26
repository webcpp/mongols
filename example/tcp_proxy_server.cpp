#include <sys/prctl.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <mongols/tcp_proxy_server.hpp>
#include <mongols/util.hpp>

#include <cstring>
#include <functional>
#include <iostream>

int main(int, char**)
{
    //    daemon(1, 0);
    auto f = [](const mongols::tcp_server::client_t& client) {
        return true;
    };

    int port = 9090;
    const char* host = "127.0.0.1";

    mongols::tcp_proxy_server server(host, port, 5000, 8192, 0);

    server.set_enable_tcp_send_to_other(false);
    //see example/nodejs
    server.set_backend_server(host, 8888);
    server.set_backend_server(host, 8889);
    //    if (!server.set_openssl("openssl/localhost.crt", "openssl/localhost.key")) {
    //        return -1;
    //    }
    server.set_shutdown([&]() {
        std::cout << "process " << getpid() << " exit.\n";
    });
    server.run(f);
}
