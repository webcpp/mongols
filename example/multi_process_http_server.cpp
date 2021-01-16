#include <mongols/http_server.hpp>
#include <mongols/util.hpp>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>

int main(int, char **)
{
    //    daemon(1, 0);

    auto f = [](const mongols::request &) {
        return true;
    };
    auto g = [](const mongols::request &req, mongols::response &res) {
        res.content = std::move("hello,world");
        res.status = 200;
    };
    int port = 9090;
    const char *host = "127.0.0.1";
    mongols::http_server
        server(host, port, 5000, 8192, 0 /*2*/);
    server.set_enable_session(false);
    server.set_enable_cache(false);
    //    if (!server.set_openssl("openssl/localhost.crt", "openssl/localhost.key")) {
    //        return -1;
    //    }

    std::function<void(pthread_mutex_t *, size_t *)> ff = [&](pthread_mutex_t *mtx, size_t *data) {
        std::string i;
        pthread_mutex_lock(mtx);
        if (*data > std::thread::hardware_concurrency() - 1)
        {
            *data = 0;
        }
        i = std::move(std::to_string(*data));
        *data = (*data) + 1;
        pthread_mutex_unlock(mtx);
        server.set_db_path("html/leveldb/" + i);
        server.set_shutdown([&, i]() {
            std::cout << i << "\tprocess\t" << getpid() << "\texit.\n";
        });
        server.run(f, g);
    };

    std::function<bool(int)> gg = [&](int status) {
        return false;
    };

    mongols::multi_process main_process;
    main_process.run(ff, gg);
    return 0;
}
