#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/prctl.h>
#include <mongols/util.hpp>
#include <mongols/web_server.hpp>

#include <iostream>
#include <algorithm>

int main(int, char**) {
    //    daemon(1, 0);
    auto f = [](const mongols::request & req) {
        if (req.method == "GET" && req.uri.find("..") == std::string::npos) {
            return true;
        }
        return false;
    };
    int port = 9090;
    const char* host = "127.0.0.1";
    mongols::web_server
    server(host, port, 5000, 512000, 0/*2*/);
    server.set_root_path("html");
    server.set_mime_type_file("html/mime.conf");
    server.set_list_directory(true);
    server.set_enable_mmap(true);

    std::function<void(pthread_mutex_t*, size_t*) > ff = [&](pthread_mutex_t* mtx, size_t * data) {
        prctl(PR_SET_NAME, "mongols: worker");
        server.run(f);
    };

    std::function<bool(int) > g = [&](int status) {
        std::cout << strsignal(WTERMSIG(status)) << std::endl;
        return false;
    };

    mongols::multi_process main_precess;
    main_precess.run(ff, g);


}