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


static void signal_cb(int sig);
static std::vector<pid_t> pids;
static void set_signal();

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

    pthread_mutex_t *mtx = 0;
    pthread_mutexattr_t *mtx_attr = 0;
    size_t *data = 0;

    mtx = (pthread_mutex_t*) mmap(0, sizeof (pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mtx == MAP_FAILED) {
        return -1;
    }

    mtx_attr = (pthread_mutexattr_t*) mmap(0, sizeof (pthread_mutexattr_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mtx_attr == MAP_FAILED) {
        return -1;
    } else {
        pthread_mutexattr_init(mtx_attr);
        pthread_mutexattr_setpshared(mtx_attr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_settype(mtx_attr, PTHREAD_MUTEX_DEFAULT);
        pthread_mutex_init(mtx, mtx_attr);
    }

    data = (size_t*) mmap(0, sizeof (size_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (data == MAP_FAILED) {
        return -1;
    } else {
        pthread_mutex_lock(mtx);
        *data = 0;
        pthread_mutex_unlock(mtx);
    }


    std::function<void() > process_work = [&]() {
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
    mongols::forker(std::thread::hardware_concurrency(), process_work, pids);
    set_signal();
    for (size_t i = 0; i < pids.size(); ++i) {
        mongols::process_bind_cpu(pids[i], i);
    }

    std::function<void(pid_t) > refork = [&](pid_t pid) {
        std::vector<int>::iterator p = std::find(pids.begin(), pids.end(), pid);
        if (p != pids.end()) {
            *p = -1 * pid;
        }
        mongols::forker(1, process_work, pids);
    };
    pid_t pid;
    int status;
    while ((pid = wait(&status)) > 0) {
        if (WIFSIGNALED(status)) {
            if (WCOREDUMP(status)) {
                std::cout << strsignal(WTERMSIG(status)) << std::endl;
                // or 
                // refork(pid);
            } else if (WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS) {
                refork(pid);
            }
        }
    }

    pthread_mutex_destroy(mtx);
    pthread_mutexattr_destroy(mtx_attr);
    munmap(mtx_attr, sizeof (pthread_mutexattr_t));
    munmap(mtx, sizeof (pthread_mutex_t));
    munmap(data, sizeof (size_t));


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
        case SIGUSR1:
            for (auto & i : pids) {
                if (i > 0) {
                    kill(i, SIGSEGV);
                }
            }
            break;
        case SIGUSR2:
            for (auto & i : pids) {
                if (i > 0) {
                    kill(i, SIGSEGV);
                }
            }
            break;
        default:break;
    }
}

static void set_signal() {
    std::vector<int> sigs = {SIGHUP, SIGTERM, SIGINT, SIGQUIT, SIGUSR1, SIGUSR2};
    for (size_t i = 0; i < sigs.size(); ++i) {
        signal(sigs[i], signal_cb);
    }
}