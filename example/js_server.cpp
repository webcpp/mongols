#include <mongols/lib/hash/md5.hpp>
#include <mongols/js_server.hpp>

#include "hash/sha1.hpp"

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

    studest* set_score(double score) {
        this->score = score;
        return this;
    }
private:
    double score;
};

int main(int, char**) {
    int port = 9090;
    const char* host = "127.0.0.1";
    mongols::js_server
    server(host, port, 5000, 8096, 0/*2*/);
    server.set_root_path("html/js");
    server.set_enable_bootstrap(true);

    server.register_class_constructor<person>("person");
    server.register_class_method(&person::set_age, "set_age");
    server.register_class_method(&person::get_age, "get_age");
    server.register_class_method(&person::set_name, "set_name");
    server.register_class_method(&person::get_name, "get_name");

    server.register_class_constructor<studest>("studest");
    server.register_class_method(&studest::get_score, "get_score");
    server.register_class_method(&studest::set_score, "set_score");
    server.set_base_class<person, studest>();

    server.register_function(&mongols::md5, "md5");
    server.register_function(&mongols::sha1, "sha1");

    server.run("html/js/package", "html/js/package");
}


