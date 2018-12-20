#include <functional>
#include "util.hpp"
#include "server_bind_script.hpp"

#include "chai_server.hpp"
#include "chaiscript/extras/math.hpp"
#include "chaiscript/extras/string_methods.hpp"

namespace mongols {

    chai_server::chai_server(const std::string& host, int port, int timeout, size_t buffer_size, size_t thread_size, size_t max_body_size, int max_event_size)
    : chai(0), server(0), root_path(""), enable_bootstrap(false) {

        this->chai = new chaiscript::ChaiScript();

        auto mathlib = chaiscript::extras::math::bootstrap();
        this->chai->add(mathlib);
        auto stringmethods = chaiscript::extras::string_methods::bootstrap();
        this->chai->add(stringmethods);


        this->chai->add(chaiscript::user_type<mongols::server_bind_script_request>(), "mongols_request");
        this->chai->add(chaiscript::constructor < mongols::server_bind_script_request()>(), "mongols_request");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::uri), "uri");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::method), "method");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::client), "client");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::user_agent), "user_agent");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::param), "param");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::has_header), "has_header");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::has_cookie), "has_cookie");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::has_form), "has_form");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::has_cache), "has_cache");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::has_session), "has_session");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::get_header), "get_header");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::get_cookie), "get_cookie");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::get_form), "get_form");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::get_session), "get_session");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_request::get_cache), "get_cache");

        this->chai->add(chaiscript::user_type<mongols::server_bind_script_response>(), "mongols_response");
        this->chai->add(chaiscript::constructor < mongols::server_bind_script_response()>(), "mongols_response");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_response::status), "status");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_response::content), "content");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_response::header), "header");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_response::session), "session");
        this->chai->add(chaiscript::fun(&mongols::server_bind_script_response::cache), "cache");

        this->server = new http_server(host, port, timeout, buffer_size, thread_size, max_body_size, max_event_size);
    }

    chai_server::~chai_server() {
        if (this->chai) {
            delete this->chai;
        }
        if (this->server) {
            delete this->server;
        }
    }

    void chai_server::set_root_path(const std::string& path) {
        this->root_path = path;
    }

    void chai_server::set_db_path(const std::string& path) {
        this->server->set_db_path(path);
    }

    void chai_server::set_enable_bootstrap(bool b) {
        this->enable_bootstrap = b;
    }

    void chai_server::set_enable_cache(bool b) {
        this->server->set_enable_cache(b);
    }

    void chai_server::set_enable_session(bool b) {
        this->server->set_enable_session(b);
    }

    void chai_server::set_enable_lru_cache(bool b) {
        this->server->set_enable_lru_cache(b);
    }

    void chai_server::set_max_file_size(size_t len) {
        this->server->set_max_file_size(len);
    }

    void chai_server::set_max_open_files(int len) {
        this->server->set_max_open_files(len);
    }

    void chai_server::set_session_expires(long long expires) {
        this->server->set_session_expires(expires);
    }

    void chai_server::set_lru_cache_expires(long long expires) {
        this->server->set_lru_cache_expires(expires);
    }

    void chai_server::set_lru_cache_size(size_t len) {
        this->server->set_lru_cache_size(len);
    }

    void chai_server::set_write_buffer_size(size_t len) {
        this->server->set_write_buffer_size(len);
    }

    void chai_server::set_uri_rewrite(const std::pair<std::string, std::string>& p) {
        this->server->set_uri_rewrite(p);
    }

    bool chai_server::filter(const mongols::request& req) {
        return true;
    }

    void chai_server::work(const mongols::request& req, mongols::response& res) {
        mongols::server_bind_script_request bind_req;
        bind_req.init(const_cast<mongols::request*> (&req));
        mongols::server_bind_script_response bind_res;
        bind_res.init(&res);
        this->chai->set_global(chaiscript::var(std::ref(bind_req)), "mongols_req");
        this->chai->set_global(chaiscript::var(std::ref(bind_res)), "mongols_res");
        try {
            this->chai->eval_file(this->enable_bootstrap ? this->root_path + "/index.chai" : this->root_path + req.uri
                    , chaiscript::exception_specification<const std::exception &>());
        } catch (const chaiscript::exception::eval_error &e) {
            res.content = e.what();
            res.status = 500;
        } catch (const chaiscript::exception::bad_boxed_cast & e) {
            res.content = e.what();
            res.status = 500;
        } catch (const std::exception &e) {
            res.content = e.what();
            res.status = 500;
        }
    }

    void chai_server::set_package_path(const std::string& path) {
        this->chai->set_use_path(path.back() == '/' ? path : path + "/");
    }

    void chai_server::set_package_cpath(const std::string& cpath) {
        this->chai->set_module_path(cpath.back() == '/' ? cpath : cpath + "/");
    }

    void chai_server::run() {
        this->server->run(std::bind(&chai_server::filter, this, std::placeholders::_1)
                , std::bind(&chai_server::work, this
                , std::placeholders::_1
                , std::placeholders::_2));
    }

    void chai_server::add(const chaiscript::Type_Conversion& d) {
        this->chai->add(d);
    }

    void chai_server::add(const chaiscript::ModulePtr& t_p) {
        this->chai->add(t_p);
    }

    void chai_server::register_namespace(const std::function<void(chaiscript::Namespace&)>& t_namespace_generator, const std::string& t_namespace_name) {
        this->chai->register_namespace(t_namespace_generator, t_namespace_name);
    }







}