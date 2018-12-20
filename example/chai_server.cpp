#include <string>
#include <mongols/lib/hash/md5.hpp>
#include <mongols/lib/hash/sha1.hpp>
#include <mongols/chai_server.hpp>

class person {
public:

    person() : name("Tom"), age(0) {
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
    int port = 9090;
    const char* host = "127.0.0.1";
    mongols::chai_server
    server(host, port, 5000, 8096, 0/*2*/);
    server.set_root_path("html/chai");
    server.set_enable_bootstrap(true);
    server.set_enable_lru_cache(true);
    server.set_lru_cache_expires(1);

    server.add(chaiscript::user_type<person>(), "person");
    server.add(chaiscript::constructor < person()>(), "person");
    server.add(chaiscript::fun(&person::get_age), "get_age");
    server.add(chaiscript::fun(&person::set_age), "set_age");
    server.add(chaiscript::fun(&person::get_name), "get_name");
    server.add(chaiscript::fun(&person::set_name), "set_name");


    server.add(chaiscript::user_type < studest>(), "studest");
    server.add(chaiscript::constructor < studest()>(), "studest");
    server.add(chaiscript::fun(&studest::get_score), "get_score");
    server.add(chaiscript::fun(&studest::set_score), "set_score");


    server.add(chaiscript::base_class<person, studest>());

    server.add(chaiscript::fun(&mongols::md5), "md5");
    server.add(chaiscript::fun(&mongols::sha1), "sha1");

    std::vector<std::string> package_paths = {"html/chai/package"}
    , package_cpaths = {"html/chai/package/test"};

    for (auto& item : package_paths) {
        server.set_package_path(item);
    }
    for (auto& item : package_cpaths) {
        server.set_package_cpath(item);
    }
    server.run();
}