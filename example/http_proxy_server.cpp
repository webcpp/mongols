#include <mongols/tcp_proxy_server.hpp>
#include <mongols/util.hpp>

#include <cstring>
#include <functional>
#include <iostream>

int main(int, char **)
{
    //    daemon(1, 0);
    auto f = [](const mongols::tcp_server::client_t &client) {
        return true;
    };

    auto h = [&](const mongols::request &req) {
        return true;
    };

    int port = 9090;
    const char *host = "127.0.0.1";

    mongols::tcp_proxy_server server(host, port, 5000, 8192, 0);

    server.set_enable_http_lru_cache(false);
    server.set_http_lru_cache_expires(1);
    server.set_default_http_content();

    //see example/nodejs
    server.set_backend_server(host, 8887);
    //    if (!server.set_openssl("openssl/localhost.crt", "openssl/localhost.key"
    //            ,mongols::openssl::version_t::TLSv12,"AES128-GCM-SHA256")) {
    //        return -1;
    //    }

    std::function<void(pthread_mutex_t *, size_t *)> ff = [&](pthread_mutex_t *mtx, size_t *data) {
        server.set_shutdown([&]() {
            std::cout << "process " << getpid() << " exit.\n";
        });
        server.run(f, h);
    };

    std::function<bool(int)> g = [&](int status) {
        return false;
    };

    mongols::multi_process main_process;
    main_process.run(ff, g);
}
