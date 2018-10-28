#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <mongols/util.hpp>
#include <mongols/web_server.hpp>
#include <iostream>
#include <algorithm>


static void signal_cb(int sig, siginfo_t *, void *);
static std::vector<pid_t> pids;

int main(int, char**) {
    //    daemon(1, 0);

    std::vector<int> sigs = {SIGHUP, SIGTERM, SIGINT, SIGQUIT};
    struct sigaction act;
    for (auto& i : sigs) {
        memset(&act, 0, sizeof (struct sigaction));
        sigemptyset(&act.sa_mask);
        act.sa_sigaction = signal_cb;
        if (sigaction(i, &act, NULL) < 0) {
            perror("sigaction error");
            return -1;
        }
    }

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


    int child_process_len = std::thread::hardware_concurrency();
    mongols::forker(child_process_len
            , [&]() {
                server.run(f);
            }
    , pids);
    for (int i = 0; i < child_process_len; ++i) {
        mongols::process_bind_cpu(pids[i], i);
    }


    for (size_t i = 0; i < pids.size(); ++i) {
        pid_t pid;
        int status;
        if ((pid = wait(&status)) > 0) {
            if (WIFSIGNALED(status)) {
                std::vector<int>::iterator p = std::find(pids.begin(), pids.end(), pid);
                if (p != pids.end()) {
                    *p = -1 * pid;
                }
                if (std::find(sigs.begin(), sigs.end(), WTERMSIG(status)) == sigs.end())
                    mongols::forker(1
                        , [&]() {
                            server.run(f);
                        }
                , pids);
            }
        }
    }

}

static void signal_cb(int sig, siginfo_t *, void *) {
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