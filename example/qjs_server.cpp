#include <mongols/qjs_server.hpp>

int main(int, char**)
{
    int port = 9090;
    const char* host = "127.0.0.1";
    mongols::qjs_server
        server(host, port, 5000, 8192, 0 /*2*/);
    server.set_root_path("html/qjs");
    server.set_enable_bootstrap(true);
    server.set_enable_lru_cache(true);
    server.set_lru_cache_expires(1);
    //    if (!server.set_openssl("openssl/localhost.crt", "openssl/localhost.key")) {
    //        return -1;
    //    }

    server.set_shutdown([&]() {
        std::cout << "process " << getpid() << " exit.\n";
    });
    server.run();
}