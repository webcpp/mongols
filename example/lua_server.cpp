#include <mongols/lib/hash/sha1.hpp>
#include <mongols/lib/hash/md5.hpp>
#include <mongols/lua_server.hpp>

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

    server.run("html/lua/package/?.lua;", "html/lua/package/?.so;");
}