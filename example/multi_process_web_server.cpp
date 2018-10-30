#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <mongols/util.hpp>
#include <mongols/web_server.hpp>
#include <iostream>
#include <algorithm>

#include "util.hpp"


static void signal_cb(int sig);
static std::vector<pid_t> pids;
static void set_signal();

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
    server.set_enable_mmap(false);

    std::function<void() > process_work = [&]() {
        server.run(f);
    };
    mongols::forker(std::thread::hardware_concurrency(), process_work, pids);
    set_signal();
    for (size_t i = 0; i < pids.size(); ++i) {
        mongols::process_bind_cpu(pids[i], i);
    }

    pid_t pid;
    int status;
    while ((pid = wait(&status)) > 0) {
        if (WIFSIGNALED(status)) {
            if (WTERMSIG(status) == SIGSEGV) {
                std::vector<int>::iterator p = std::find(pids.begin(), pids.end(), pid);
                if (p != pids.end()) {
                    *p = -1 * pid;
                }
                mongols::forker(1, process_work, pids);
            }
        }
    }


}

static void signal_cb(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGHUP:
        case SIGQUIT:
        case SIGINT:
            for (auto & i : pids) {
                if (i > 0) {
                    kill(i, SIGTERM);
                }
            }
            break;
        default:break;
    }
}

static void set_signal() {
    std::vector<int> sigs = {SIGHUP, SIGTERM, SIGINT, SIGQUIT};
    for (size_t i = 0; i < sigs.size(); ++i) {
        signal(sigs[i], signal_cb);
    }
}