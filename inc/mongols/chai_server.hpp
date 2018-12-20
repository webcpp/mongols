#ifndef CHAI_SERVER_HPP
#define CHAI_SERVER_HPP

#include <string>

#include "chaiscript/chaiscript.hpp"
#include "http_server.hpp"

namespace mongols {

    class chai_server {
    public:
        chai_server() = delete;
        chai_server(const std::string& host, int port
                , int timeout = 5000
                , size_t buffer_size = 8092
                , size_t thread_size = std::thread::hardware_concurrency()
                , size_t max_body_size = 4096
                , int max_event_size = 64);
        virtual~chai_server();
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
        void set_package_path(const std::string&);
        void set_package_cpath(const std::string&);
        void run();

        template<typename T>
        void add(const T &t_t, const std::string &t_name) {
            this->chai->add(t_t, t_name);
        }

        void add(const chaiscript::Type_Conversion &d);

        void add(const chaiscript::ModulePtr &t_p);

        void register_namespace(const std::function<void(chaiscript::Namespace&)>& t_namespace_generator, const std::string& t_namespace_name);


    private:
        chaiscript::ChaiScript *chai;
        mongols::http_server *server;
        std::string root_path;
        bool enable_bootstrap;
    private:
        void work(const mongols::request& req, mongols::response& res);
        bool filter(const mongols::request& req);
    };
}



#endif /* CHAI_SERVER_HPP */

