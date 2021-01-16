#include "qjs_server.hpp"

extern "C"
{
#include "qjs/bjson.h"
#include "qjs/cutils.h"
#include "qjs/quickjs-libc.h"
}

#include "qjs/quickjs-crypto.hpp"
#include "qjs/quickjs-hash.hpp"
#include "qjs/quickjs-mongols.hpp"
#include "util.hpp"
#include <functional>

namespace mongols
{

    size_t qjs_server::memory_limit = 1048576 * 1024 /*1GB*/, qjs_server::ctx_called_limit = 10240, qjs_server::stack_limit = 1048576 * 8 /*8MB*/;

    qjs_server::qjs_server(const std::string &host, int port, int timeout, size_t buffer_size, size_t thread_size, size_t max_body_size, int max_event_size)
        : vm(0), ctx(0), ctx_called_count(0), server(0), root_path(), enable_bootstrap(false), jsfile_mmap()
    {
        this->vm = JS_NewRuntime();
        this->config_vm();
        this->ctx = JS_NewContext(this->vm);
        this->config_ctx();
        this->server = new http_server(host, port, timeout, buffer_size, thread_size, max_body_size, max_event_size);
    }

    qjs_server::~qjs_server()
    {
        if (this->vm)
        {
            js_std_free_handlers(this->vm);
        }
        if (this->ctx)
        {
            JS_FreeContext(this->ctx);
        }
        if (this->vm)
        {
            JS_FreeRuntime(this->vm);
        }
        if (this->server)
        {
            delete this->server;
        }
    }

    void qjs_server::run()
    {
        this->server->run(std::bind(&qjs_server::filter, this, std::placeholders::_1), std::bind(&qjs_server::work, this, std::placeholders::_1, std::placeholders::_2));
    }

    bool qjs_server::filter(const mongols::request &req)
    {
        return true;
    }

    void qjs_server::work(const mongols::request &req, mongols::response &res)
    {
        if (this->ctx)
        {
            std::pair<const mongols::request *, mongols::response *> http_data;
            http_data.first = &req;
            http_data.second = &res;
            JS_SetContextOpaque(this->ctx, &http_data);

            std::string path = std::move(this->enable_bootstrap ? this->root_path + "/index.js" : this->root_path + req.uri);

            std::pair<char *, struct stat> ele;
            if (this->jsfile_mmap.get(path, ele))
            {
                JSValue ret = JS_Eval(this->ctx, ele.first, ele.second.st_size, path.c_str(), JS_EVAL_TYPE_MODULE);
                if (JS_IsException(ret))
                {
                    js_std_dump_error(this->ctx);
                    res.status = 500;
                    res.content = std::move("Internal Server Error");
                }
                JS_FreeValue(ctx, ret);
                js_std_loop(this->ctx);
                JS_SetContextOpaque(this->ctx, NULL);
                if (++this->ctx_called_count >= qjs_server::ctx_called_limit)
                {
                    JS_FreeContext(this->ctx);
                    this->ctx = JS_NewContext(this->vm);
                    this->config_ctx();
                    this->ctx_called_count = 0;
                }
            }
        }
    }

    void qjs_server::config_vm()
    {
        if (this->vm)
        {
            JS_SetMemoryLimit(this->vm, qjs_server::memory_limit);
            JS_SetMaxStackSize(this->vm, qjs_server::stack_limit);
            js_std_init_handlers(this->vm);
            JS_SetModuleLoaderFunc(this->vm, NULL, js_module_loader, NULL);
        }
    }

    void qjs_server::config_ctx()
    {
        if (this->ctx)
        {
            JS_AddIntrinsicBigFloat(this->ctx);
            JS_AddIntrinsicBigDecimal(this->ctx);
            JS_AddIntrinsicOperators(this->ctx);
            JS_EnableBignumExt(this->ctx, TRUE);
            js_init_module_std(this->ctx, "std");
            js_init_module_os(this->ctx, "os");
            js_init_module_bjson(this->ctx, "bjson");
            js_init_module_mongols(this->ctx, "mongols");
            js_init_module_hash(this->ctx, "hash");
            js_init_module_crypto(this->ctx, "crypto");
        }
    }

    void qjs_server::set_root_path(const std::string &path)
    {
        this->root_path = path;
    }

    void qjs_server::set_db_path(const std::string &path)
    {
        this->server->set_db_path(path);
    }

    void qjs_server::set_enable_bootstrap(bool b)
    {
        this->enable_bootstrap = b;
    }

    void qjs_server::set_enable_cache(bool b)
    {
        this->server->set_enable_cache(b);
    }

    void qjs_server::set_enable_session(bool b)
    {
        this->server->set_enable_session(b);
    }

    void qjs_server::set_enable_lru_cache(bool b)
    {
        this->server->set_enable_lru_cache(b);
    }

    void qjs_server::set_max_file_size(size_t len)
    {
        this->server->set_max_file_size(len);
    }

    void qjs_server::set_max_open_files(int len)
    {
        this->server->set_max_open_files(len);
    }

    void qjs_server::set_session_expires(long long expires)
    {
        this->server->set_session_expires(expires);
    }

    void qjs_server::set_lru_cache_expires(long long expires)
    {
        this->server->set_lru_cache_expires(expires);
    }

    void qjs_server::set_lru_cache_size(size_t len)
    {
        this->server->set_lru_cache_size(len);
    }

    void qjs_server::set_write_buffer_size(size_t len)
    {
        this->server->set_write_buffer_size(len);
    }

    void qjs_server::set_uri_rewrite(const std::pair<std::string, std::string> &p)
    {
        this->server->set_uri_rewrite(p);
    }

    bool qjs_server::set_openssl(const std::string &crt_file, const std::string &key_file, openssl::version_t v, const std::string &ciphers, long flags)
    {
        return this->server->set_openssl(crt_file, key_file, v, ciphers, flags);
    }

    void qjs_server::set_enable_blacklist(bool b)
    {
        this->server->set_enable_blacklist(b);
    }

    void qjs_server::set_enable_whitelist(bool b)
    {
        this->server->set_enable_whitelist(b);
    }
    void qjs_server::set_whitelist(const std::string &ip)
    {
        this->server->set_whitelist(ip);
    }
    void qjs_server::del_whitelist(const std::string &ip)
    {
        this->server->del_whitelist(ip);
    }

    void qjs_server::set_whitelist_file(const std::string &path)
    {
        this->server->set_whitelist_file(path);
    }

    void qjs_server::set_enable_security_check(bool b)
    {
        this->server->set_enable_security_check(b);
    }

    void qjs_server::set_shutdown(const tcp_server::shutdown_function &f)
    {
        this->server->set_shutdown(f);
    }
} // namespace mongols