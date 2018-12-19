#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <mongols/util.hpp>
#include <mongols/lib/hash/md5.hpp>
#include <mongols/lib/hash/sha1.hpp>
#include <mongols/js_server.hpp>

#include <iostream>
#include <algorithm>

class person : public mongols::js_object {
public:

    person() : mongols::js_object(), name("Tom"), age(0) {
    }
    virtual~person() = default;

    void set_name(const std::string& name) {
        this->name = name;
    }

    void set_age(unsigned int age) {
        this->age = age;
    }

    const std::string& get_name() {
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

    void set_score(double score) {
        this->score = score;
    }
private:
    double score;
};

int main(int, char**) {
    //    daemon(1, 0);

    int port = 9090;
    const char* host = "127.0.0.1";
    mongols::js_server
    server(host, port, 5000, 8096, 0/*2*/);
    server.set_root_path("html/js");
    server.set_enable_bootstrap(true);
    server.set_enable_lru_cache(true);
    server.set_lru_cache_expires(1);

    server.register_class_constructor<person>("person");
    server.register_class_method(&person::set_age, "set_age");
    server.register_class_method(&person::get_age, "get_age");
    server.register_class_method(&person::set_name, "set_name");
    server.register_class_method(&person::get_name, "get_name");

    server.register_class_constructor<studest>("studest");
    server.register_class_method(&studest::get_score, "get_score");
    server.register_class_method(&studest::set_score, "set_score");
    server.set_base_class<mongols::js_object, person>();
    server.set_base_class<person, studest>();

    server.register_function(&mongols::md5, "md5");
    server.register_function(&mongols::sha1, "sha1");


    std::function<void(pthread_mutex_t*, size_t*) > f = [&](pthread_mutex_t* mtx, size_t * data) {
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
        server.run("html/js/package", "html/js/package");
    };

    std::function<bool(int) > g = [&](int status) {
        std::cout << strsignal(WTERMSIG(status)) << std::endl;
        return false;
    };

    mongols::multi_process main_process;
    main_process.run(f, g);



}

