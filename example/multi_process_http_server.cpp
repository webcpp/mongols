#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <mongols/util.hpp>
#include <mongols/http_server.hpp>

#include <iostream>
#include <algorithm>

int main(int, char**) {
    //    daemon(1, 0);

    auto f = [](const mongols::request&) {
        return true;
    };
    auto g = [](const mongols::request& req, mongols::response & res) {
        res.content = std::move("hello,world");
        res.status = 200;
    };
    int port = 9090;
    const char* host = "127.0.0.1";
    mongols::http_server
    server(host, port, 5000, 8096, 0/*2*/);
    server.set_enable_session(false);
    server.set_enable_cache(false);

    std::function<void(pthread_mutex_t*, size_t*) > ff = [&](pthread_mutex_t* mtx, size_t * data) {
        prctl(PR_SET_NAME, "mongols: worker");
        std::string i;
        pthread_mutex_lock(mtx);
        if (*data > std::thread::hardware_concurrency() - 1) {
            *data = 0;
        }
        i = std::move(std::to_string(*data));
        *data = (*data) + 1;
        pthread_mutex_unlock(mtx);
        server.set_db_path("html/leveldb/" + i);
        server.run(f, g);
    };

    std::function<bool(int) > gg = [&](int status) {
        std::cout << strsignal(WTERMSIG(status)) << std::endl;
        return false;
    };

    mongols::multi_process main_precess;
    main_precess.run(ff, gg);





}
