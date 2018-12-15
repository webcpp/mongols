#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>
#include <functional>
#include "util.hpp"
#include "server_bind_script.hpp"
#include "lib/dukglue/duktape.h"
#include "lib/dukglue/duk_print_alert.h"
#include "lib/dukglue/duk_module_duktape.h"
#include "lib/dukglue/duk_console.h"
#include "lib/dukglue/dukglue.h"
#include "lib/hash/md5.hpp"
#include "js_server.hpp"


namespace mongols {

    js_tool::js_tool() : data(), dl_map(), ctx(0), cpackage_path() {

    }

    js_tool::~js_tool() {
        for (auto& i : this->dl_map) {
            if (i.second.first != NULL) {
                dlclose(i.second.first);
            }
        }
    }

    void js_tool::init(duk_context* ctx) {
        this->ctx = ctx;
    }

    void js_tool::free(js_object* p) {
        delete p;
        dukglue_invalidate_object(this->ctx, p);
    }

    void js_tool::set_cpackage_path(const std::string& path) {
        this->cpackage_path = path;
    }

    std::string js_tool::read(const std::string& path) {
        std::string text;
        std::pair<char*, struct stat> ele;
        if (this->data.get(path, ele)) {
            text.assign(ele.first, ele.second.st_size);
            return text;
        }
        return text;
    }

    bool js_tool::require(const std::string& path, const std::string& fun_name) {
        if (this->dl_map.find(fun_name) != this->dl_map.end()) {
            return true;
        }
        void* handler = dlopen((this->cpackage_path + "/" + path + ".so").c_str(), RTLD_NOW);
        if (handler != NULL) {
            dlerror();
            native_fun* fun = (native_fun*) dlsym(handler, fun_name.c_str());
            if (dlerror() == NULL) {
                duk_push_c_function(this->ctx, *fun, DUK_VARARGS);
                duk_put_global_string(this->ctx, fun_name.c_str());
                this->dl_map[fun_name] = std::move(std::make_pair(handler, fun));
                return true;
            } else {
                dlclose(handler);
            }
        }
        return false;
    }

    js_server::js_server(const std::string& host, int port, int timeout, size_t buffer_size, size_t thread_size, size_t max_body_size, int max_event_size)
    : ctx(0), server(0), root_path(), enable_bootstrap(false), file_mmap(), tool() {
        this->ctx = duk_create_heap_default();
        this->tool.init(this->ctx);


        duk_module_duktape_init(this->ctx);
        duk_print_alert_init(this->ctx, 0 /*flags*/);
        duk_console_init(this->ctx, 0 /*flags*/);


        dukglue_register_constructor<server_bind_script_request>(this->ctx, "mongols_request");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::uri, "uri");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::method, "method");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::client, "client");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::user_agent, "user_agent");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::param, "param");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::has_header, "has_header");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::has_cookie, "has_cookie");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::has_form, "has_form");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::has_cache, "has_cache");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::has_session, "has_session");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::get_header, "get_header");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::get_cookie, "get_cookie");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::get_form, "get_form");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::get_session, "get_session");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_request::get_cache, "get_cache");

        dukglue_register_constructor<server_bind_script_response>(this->ctx, "mongols_response");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_response::status, "status");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_response::content, "content");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_response::header, "header");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_response::session, "session");
        dukglue_register_method(this->ctx, &mongols::server_bind_script_response::cache, "cache");


        dukglue_register_constructor<js_tool>(this->ctx, "mongols_tool");
        dukglue_register_method(this->ctx, &js_tool::read, "read");
        dukglue_register_method(this->ctx, &js_tool::require, "require");
        dukglue_register_method(this->ctx, &js_tool::free, "free");


        dukglue_register_global(this->ctx, &this->tool, "mongols_module");

        this->server = new http_server(host, port, timeout, buffer_size, thread_size, max_body_size, max_event_size);

    }

    js_server::~js_server() {
        for (auto& i : this->file_mmap) {
            munmap(i.second.first, i.second.second.st_size);
        }
        dukglue_invalidate_object(this->ctx, &this->tool);
        if (this->ctx) {
            duk_destroy_heap(this->ctx);
        }
        if (this->server) {
            delete this->server;
        }
    }

    bool js_server::filter(const mongols::request& req) {
        return true;
    }

    void js_server::work(const mongols::request& req, mongols::response& res) {
        mongols::server_bind_script_request bind_req;
        bind_req.init(const_cast<mongols::request*> (&req));
        mongols::server_bind_script_response bind_res;
        bind_res.init(&res);
        dukglue_register_global(this->ctx, &bind_req, "mongols_req");
        dukglue_register_global(this->ctx, &bind_res, "mongols_res");
        std::string path = std::move(this->enable_bootstrap ? this->root_path + "/index.js" : this->root_path + req.uri),
                mmap_key = std::move(mongols::md5(path));
        std::unordered_map<std::string, std::pair<char*, struct stat>>::const_iterator iter;
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            if (S_ISREG(st.st_mode)) {
                if ((iter = this->file_mmap.find(mmap_key)) != this->file_mmap.end()) {
                    if (iter->second.second.st_mtime != st.st_mtime) {
                        munmap(iter->second.first, iter->second.second.st_size);
                        this->file_mmap.erase(iter);
                        goto js_read;
                    }
                    duk_peval_lstring_noresult(this->ctx, iter->second.first, iter->second.second.st_size);
                } else {
js_read:
                    int ffd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
                    if (ffd > 0) {
                        char *mmap_ptr = (char*) mmap(0, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, ffd, 0);
                        if (mmap_ptr == MAP_FAILED) {
                            close(ffd);
                            goto js_500;
                        } else {
                            close(ffd);
                            if (madvise(mmap_ptr, st.st_size, MADV_SEQUENTIAL) == 0) {

                                duk_peval_lstring_noresult(this->ctx, mmap_ptr, st.st_size);
                                this->file_mmap[mmap_key] = std::move(std::make_pair(mmap_ptr, st));

                            } else {
                                munmap(mmap_ptr, st.st_size);
                                goto js_500;
                            }
                        }
                    } else {
js_500:
                        res.status = 500;
                        res.content = std::move("Internal Server Error");
                    }

                }
            } else if (S_ISDIR(st.st_mode)) {
                res.status = 403;
                res.content = std::move("Forbidden");
            }
        } else if ((iter = this->file_mmap.find(mmap_key)) != this->file_mmap.end()) {
            munmap(iter->second.first, iter->second.second.st_size);
            this->file_mmap.erase(iter);
        }
    }

    void js_server::run(const std::string& package_path, const std::string& cpackage_path) {
        std::string mod_search = "Duktape.modSearch = function (id, require, exports, module) {return mongols_module.read('";
        mod_search += package_path;
        mod_search += "/'+id+'.js'); };";
        duk_peval_lstring_noresult(this->ctx, mod_search.c_str(), mod_search.size());
        this->tool.set_cpackage_path(cpackage_path);
        this->server->run(std::bind(&js_server::filter, this, std::placeholders::_1)
                , std::bind(&js_server::work, this
                , std::placeholders::_1
                , std::placeholders::_2));
    }

    void js_server::set_root_path(const std::string& path) {
        this->root_path = path;
    }

    void js_server::set_db_path(const std::string& path) {
        this->server->set_db_path(path);
    }

    void js_server::set_enable_bootstrap(bool b) {
        this->enable_bootstrap = b;
    }

    void js_server::set_enable_cache(bool b) {
        this->server->set_enable_cache(b);
    }

    void js_server::set_enable_session(bool b) {
        this->server->set_enable_session(b);
    }

    void js_server::set_enable_lru_cache(bool b) {
        this->server->set_enable_lru_cache(b);
    }

    void js_server::set_lru_cache_size(size_t len) {
        this->server->set_lru_cache_size(len);
    }

    void js_server::set_max_file_size(size_t len) {
        this->server->set_max_file_size(len);
    }

    void js_server::set_max_open_files(int len) {
        this->server->set_max_open_files(len);
    }

    void js_server::set_session_expires(long long expires) {
        this->server->set_session_expires(expires);
    }

    void js_server::set_lru_cache_expires(long long expires) {
        this->server->set_lru_cache_expires(expires);
    }

    void js_server::set_write_buffer_size(size_t len) {
        this->server->set_write_buffer_size(len);
    }

    void js_server::set_uri_rewrite(const std::pair<std::string, std::string>& p) {
        this->server->set_uri_rewrite(p);
    }
}
