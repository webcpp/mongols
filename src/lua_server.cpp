#include <functional>
#include "lua_server.hpp"
#include "util.hpp"
#include "server_bind_script.hpp"
#include "lib/lua/kaguya_ext.hpp"




namespace mongols {

    lua_server::lua_server(const std::string& host, int port, int timeout
            , size_t buffer_size, size_t thread_size, size_t max_body_size, int max_event_size)
    : vm(), server(0), root_path(), enable_bootstrap(false) {

        this->server = new http_server(host, port, timeout, buffer_size, thread_size, max_body_size, max_event_size);

        this->vm["mongols_request"].setClass(
                kaguya::UserdataMetatable<server_bind_script_request>()
                .setConstructors < server_bind_script_request()>()
                .addFunction("uri", &mongols::server_bind_script_request::uri)
                .addFunction("method", &mongols::server_bind_script_request::method)
                .addFunction("client", &mongols::server_bind_script_request::client)
                .addFunction("user_agent", &mongols::server_bind_script_request::user_agent)
                .addFunction("param", &mongols::server_bind_script_request::param)
                .addFunction("has_header", &mongols::server_bind_script_request::has_header)
                .addFunction("has_cookie", &mongols::server_bind_script_request::has_cookie)
                .addFunction("has_form", &mongols::server_bind_script_request::has_form)
                .addFunction("has_cache", &mongols::server_bind_script_request::has_cache)
                .addFunction("has_session", &mongols::server_bind_script_request::has_session)
                .addFunction("get_header", &mongols::server_bind_script_request::get_header)
                .addFunction("get_cookie", &mongols::server_bind_script_request::get_cookie)
                .addFunction("get_form", &mongols::server_bind_script_request::get_form)
                .addFunction("get_session", &mongols::server_bind_script_request::get_session)
                .addFunction("get_cache", &mongols::server_bind_script_request::get_cache)
                );
        this->vm["mongols_response"].setClass(
                kaguya::UserdataMetatable<server_bind_script_response>()
                .addFunction("status", &mongols::server_bind_script_response::status)
                .addFunction("content", &mongols::server_bind_script_response::content)
                .addFunction("header", &mongols::server_bind_script_response::header)
                .addFunction("session", &mongols::server_bind_script_response::session)
                .addFunction("cache", &mongols::server_bind_script_response::cache)
                );

        mongols::lua_ext(this->vm);

    }

    lua_server::~lua_server() {
        if (this->server) {
            delete this->server;
        }
    }

    void lua_server::run(const std::string& package_path, const std::string& package_cpath) {
        if (!package_path.empty()) {
            this->vm("package.path='" + package_path + "'.. package.path");
        }
        if (!package_cpath.empty()) {
            this->vm("package.cpath='" + package_cpath + "'.. package.cpath");
        }


        this->server->run(std::bind(&lua_server::filter, this, std::placeholders::_1)
                , std::bind(&lua_server::work, this
                , std::placeholders::_1
                , std::placeholders::_2));
    }

    bool lua_server::filter(const mongols::request& req) {
        return true;
    }

    void lua_server::work(const mongols::request& req, mongols::response& res) {
        mongols::server_bind_script_request bind_req;
        bind_req.init(const_cast<mongols::request*> (&req));
        mongols::server_bind_script_response bind_res;
        bind_res.init(&res);
        this->vm["mongols_req"] = &bind_req;
        this->vm["mongols_res"] = &bind_res;
        this->vm.setErrorHandler([&](int errCode, const char * szError) {
            res.content = szError;
            res.status = 500;
        });
        this->vm.dofile(this->enable_bootstrap ? this->root_path + "/index.lua" : this->root_path + req.uri);
    }

    void lua_server::set_root_path(const std::string& path) {
        this->root_path = path;
    }

    void lua_server::set_db_path(const std::string& path) {
        this->server->set_db_path(path);
    }

    void lua_server::set_enable_bootstrap(bool b) {
        this->enable_bootstrap = b;
    }

    void lua_server::set_enable_cache(bool b) {
        this->server->set_enable_cache(b);
    }

    void lua_server::set_enable_session(bool b) {
        this->server->set_enable_session(b);
    }

    void lua_server::set_enable_lru_cache(bool b) {
        this->server->set_enable_lru_cache(b);
    }

    void lua_server::set_max_file_size(size_t len) {
        this->server->set_max_file_size(len);
    }

    void lua_server::set_max_open_files(int len) {
        this->server->set_max_open_files(len);
    }

    void lua_server::set_session_expires(long long expires) {
        this->server->set_session_expires(expires);
    }

    void lua_server::set_lru_cache_expires(long long expires) {
        this->server->set_lru_cache_expires(expires);
    }

    void lua_server::set_write_buffer_size(size_t len) {
        this->server->set_write_buffer_size(len);
    }












}