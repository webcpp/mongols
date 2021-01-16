#include <mongols/qjs_server.hpp>
#include <mongols/util.hpp>

int main(int, char **)
{
    int port = 9090;
    const char *host = "127.0.0.1";
    mongols::qjs_server
        server(host, port, 5000, 8192, 0);
    server.set_root_path("html/qjs");
    server.set_enable_bootstrap(true);
    server.set_enable_lru_cache(false);
    server.set_lru_cache_expires(1);
    //    if (!server.set_openssl("openssl/localhost.crt", "openssl/localhost.key")) {
    //        return -1;
    //    }

    // server.set_shutdown([&]() {
    //     std::cout << "process " << getpid() << " exit.\n";
    // });
    // server.run();

    std::function<void(pthread_mutex_t *, size_t *)> ff = [&](pthread_mutex_t *mtx, size_t *data) {
        server.set_shutdown([&]() {
            std::cout << "process " << getpid() << " exit.\n";
        });
        server.run();
    };

    std::function<bool(int)> g = [&](int status) {
        return false;
    };

    mongols::multi_process main_process;
    main_process.run(ff, g);
}