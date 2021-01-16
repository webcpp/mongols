#include <mongols/util.hpp>
#include <mongols/web_server.hpp>
#include <sys/prctl.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <functional>
#include <iostream>

int main(int, char **)
{
    //    daemon(1, 0);
    auto f = [](const mongols::request &req) {
        if (req.method == "GET" && req.uri.find("..") == std::string::npos)
        {
            return true;
        }
        return false;
    };
    int port = 9090;
    const char *host = "127.0.0.1";
    mongols::web_server
        server(host, port, 5000, 512000, 0 /*2*/);
    server.set_root_path("html");
    server.set_mime_type_file("html/mime.conf");
    server.set_list_directory(true);
    server.set_enable_mmap(true);
    //    if (!server.set_openssl("openssl/localhost.crt", "openssl/localhost.key")) {
    //        return -1;
    //    }

    std::function<void(pthread_mutex_t *, size_t *)> ff = [&](pthread_mutex_t *mtx, size_t *data) {
        server.set_shutdown([&]() {
            std::cout << "process " << getpid() << " exit.\n";
        });
        server.run(f);
    };

    std::function<bool(int)> g = [&](int status) {
        return false;
    };

    mongols::multi_process main_process;
    main_process.run(ff, g);
}