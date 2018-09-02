#ifndef LUA_SERVER_HPP
#define LUA_SERVER_HPP

#include <string>
#include "http_server.hpp"
#include "lib/lua/kaguya.hpp"

namespace mongols {

    class lua_server {
    public:
        lua_server() = delete;
        lua_server(const std::string& host, int port
                , int timeout = 5000
                , size_t buffer_size = 8092
                , size_t thread_size = std::thread::hardware_concurrency()
                , size_t max_body_size = 4096
                , int max_event_size = 64);
        virtual~lua_server();
        void set_root_path(const std::string& path);
        void set_enable_session(bool);
        void set_enable_cache(bool);
        void set_session_expires(long long);
        void set_max_open_files(int);
        void set_write_buffer_size(size_t);
        void set_max_file_size(size_t);
        void set_db_path(const std::string&);
        void run(const std::string& package_path, const std::string& package_cpath);
    private:
        kaguya::State vm;
        mongols::http_server *server;
        std::string root_path;

    private:
        virtual void work(const mongols::request& req, mongols::response& res);
        virtual bool filter(const mongols::request& req);
    };
}

#endif /* LUA_SERVER_HPP */

