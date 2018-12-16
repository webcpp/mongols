#ifndef JS_SERVER_HPP
#define JS_SERVER_HPP

#include <string>
#include <unordered_map>
#include <sys/stat.h>
#include "http_server.hpp"
#include "lib/dukglue/duktape.h"
#include "lib/dukglue/dukglue.h"
#include "file_mmap.hpp"

namespace mongols {

    class js_object {
    public:
        js_object() = default;
        virtual~js_object() = default;
    };

    class js_tool {
    public:

        js_tool();

        virtual~js_tool();

        std::string read(const std::string& path);
        bool require(const std::string&, const std::string&);
        void init(duk_context*);
        void set_cpackage_path(const std::string&);
        void free(js_object* p);
    private:
        typedef duk_ret_t native_fun(duk_context *ctx);
        file_mmap data;
        std::unordered_map<std::string, std::pair<void*, native_fun*>> dl_map;
        duk_context* ctx;
        std::string cpackage_path;

    };

    class js_server {
    public:
        js_server() = delete;
        js_server(const std::string& host, int port
                , int timeout = 5000
                , size_t buffer_size = 8092
                , size_t thread_size = std::thread::hardware_concurrency()
                , size_t max_body_size = 4096
                , int max_event_size = 64);
        virtual~js_server();
        void set_root_path(const std::string& path);
        void set_enable_session(bool);
        void set_enable_cache(bool);
        void set_enable_lru_cache(bool);
        void set_enable_bootstrap(bool);
        void set_lru_cache_expires(long long);
        void set_lru_cache_size(size_t);
        void set_session_expires(long long);
        void set_max_open_files(int);
        void set_write_buffer_size(size_t);
        void set_max_file_size(size_t);
        void set_db_path(const std::string&);
        void set_uri_rewrite(const std::pair<std::string, std::string>&);
        void run(const std::string& package_path, const std::string& cpackage_path);

        template<class Cls, typename... Ts>
        void register_class_constructor(const std::string& name) {
            dukglue_register_constructor<Cls, Ts...>(this->ctx, name.c_str());
        }

        template<class Cls, typename RetType, typename... Ts>
        void register_class_method(RetType(Cls::*method)(Ts...), const std::string& name) {
            dukglue_register_method<Cls, RetType, Ts...>(this->ctx, method, name.c_str());
        }

        template<class Cls, typename RetType, typename... Ts>
        void register_class_method(RetType(Cls::*method)(Ts...)const, const std::string& name) {
            dukglue_register_method<Cls, RetType, Ts...>(this->ctx, method, name.c_str());
        }

        template<typename RetType, typename... Ts>
        void register_function(RetType(*funcToCall)(Ts...), const std::string& name) {
            dukglue_register_function<RetType, Ts...>(this->ctx, funcToCall, name.c_str());
        }

        template<class Base, class Derived>
        void set_base_class() {
            dukglue_set_base_class<Base, Derived>(this->ctx);
        }

    private:
        void work(const mongols::request& req, mongols::response& res);
        bool filter(const mongols::request& req);
    private:
        duk_context* ctx;
        mongols::http_server *server;
        std::string root_path;
        bool enable_bootstrap;
        std::unordered_map<std::string, std::pair<char*, struct stat>> file_mmap;
        js_tool tool;
    };
}

#endif /* JS_SERVER_HPP */

