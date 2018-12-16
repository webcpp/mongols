#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <mongols/util.hpp>
#include <mongols/lib/hash/sha1.hpp>
#include <mongols/lib/hash/md5.hpp>
#include <mongols/lua_server.hpp>

#include <iostream>
#include <algorithm>

class person {
public:

    person() : name("Tom"), age(0) {
    }
    virtual~person() = default;

    person* set_name(const std::string& name) {
        this->name = name;
        return this;
    }

    person* set_age(unsigned int age) {
        this->age = age;
        return this;
    }

    const std::string& get_name() const {
        return this->name;
    }

    unsigned int get_age() {
        return this->age;
    }
private:
    std::string name;
    unsigned int age;
};

class studest : public person {
public:

    studest() : person() {
    }
    virtual~studest() = default;

    double get_score() {
        return this->score;
    }

    studest* set_score(double score) {
        this->score = score;
        return this;
    }
private:
    double score;
};

static void signal_cb(int sig);
static std::vector<pid_t> pids;
static void set_signal();

int main(int, char**) {
    //    daemon(1, 0);

    int port = 9090;
    const char* host = "127.0.0.1";
    mongols::lua_server
    server(host, port, 5000, 8096, 0/*2*/);
    server.set_root_path("html/lua");
    server.set_enable_bootstrap(true);
    server.set_enable_lru_cache(true);
    server.set_lru_cache_expires(1);

    server.set_function(&mongols::sha1, "sha1");
    server.set_function(&mongols::md5, "md5");

    server.set_class(
            kaguya::UserdataMetatable<person>()
            .setConstructors < person()>()
            .addFunction("get_age", &person::get_age)
            .addFunction("get_name", &person::get_name)
            .addFunction("set_age", &person::set_age)
            .addFunction("set_name", &person::set_name)
            , "person");

    server.set_class(
            kaguya::UserdataMetatable<studest, person>()
            .setConstructors < studest()>()
            .addFunction("get_score", &studest::get_score)
            .addFunction("set_score", &studest::set_score)
            , "studest");


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
        server.run("html/lua/package/?.lua;", "html/lua/package/?.so;");
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
            } else if (WTERMSIG(status) == SIGHUP || WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS) {
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
                    kill(i, sig);
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

