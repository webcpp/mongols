#include <sys/stat.h>
#include <utility>
#include <algorithm>
#include <functional>
#include <memory>
#include <chrono>

#include "version.hpp"
#include "tcp_threading_server.hpp"
#include "http_server.hpp"
#include "util.hpp"
#include "MPFDParser/Parser.h"

#define form_urlencoded_type "application/x-www-form-urlencoded"
#define form_urlencoded_type_len (sizeof(form_urlencoded_type) - 1)
#define TEMP_DIRECTORY "temp"
#define SESSION_NAME "SESSIONID"

namespace mongols {

    http_server::cache_t::cache_t() : data(), t(time(0)), expires(600) {

    }

    http_server::cache_t::cache_t(const std::string& v, long long expires) : data(v), t(time(0)), expires(expires) {

    }

    bool http_server::cache_t::expired()const {
        return difftime(time(0), this->t)>this->expires;
    }

    const std::string& http_server::cache_t::get() const {
        return this->data;
    }

    http_server::cache_t& http_server::cache_t::set_data(const std::string& str) {
        this->data = str;
        return *this;
    }

    http_server::cache_t& http_server::cache_t::set_expires(long long expires) {
        this->expires = expires;
        return *this;
    }

    http_server::http_server(const std::string& host, int port
            , int timeout
            , size_t buffer_size
            , size_t thread_size
            , size_t max_body_size
            , int max_event_size)
    : server(0), max_body_size(max_body_size), redis(), session_expires(3600), cache_expores(600), enable_session(false), enable_cache(false) {
        if (thread_size > 0) {
            this->server = new tcp_threading_server(host, port, timeout, buffer_size, thread_size, max_event_size);
        } else {
            this->server = new tcp_server(host, port, timeout, buffer_size, max_event_size);
        }
        if (this->server) {
            if (this->enable_session || this->enable_cache) {
                this->redis.connect();
            }
        }

    }

    http_server::~http_server() {
        if (this->server) {
            delete this->server;
        }
    }

    void http_server::run(const std::function<bool(const mongols::request&)>& req_filter
            , const std::function<void(const mongols::request&, mongols::response&)>& res_filter) {

        tcp_server::handler_function g = std::bind(&http_server::work, this
                , std::cref(req_filter)
                , std::cref(res_filter)
                , std::placeholders::_1
                , std::placeholders::_2
                , std::placeholders::_3
                , std::placeholders::_4
                , std::placeholders::_5);

        this->server->run(g);
    }

    bool http_server::parse_reqeust(const std::string& str, mongols::request& req, std::string& body) {
        mongols::http_request_parser parser(req);
        if (parser.parse(str)) {
            body = std::move(parser.get_body());
            return true;
        }
        return false;
    }

    std::string http_server::create_response(mongols::response& res, bool b) {
        std::string output;
        output.append("HTTP/1.1 ").append(std::to_string(res.status)).append(" " + this->get_status_text(res.status)).append("\r\n");
        res.headers.insert(std::move(std::make_pair("Server", mongols_http_server_version)));
        if (b == KEEPALIVE_CONNECTION) {
            res.headers.insert(std::move(std::make_pair("Connection", "keep-alive")));
        } else {
            res.headers.insert(std::move(std::make_pair("Connection", "close")));
        }
        res.headers.insert(std::move(std::make_pair("Content-Length", std::to_string(res.content.size() + 2))));
        for (auto& i : res.headers) {
            output.append(i.first).append(": ").append(i.second).append("\r\n");
        }
        output.append("\r\n").append(res.content).append("\r\n");
        return output;
    }

    std::string http_server::get_status_text(int status) {
        switch (status) {
            case 100: return "Continue";
            case 101: return "Switching Protocols";
            case 200: return "OK";
            case 201: return "Created";
            case 202: return "Accepted";
            case 203: return "Non-Authoritative Information";
            case 204: return "No Content";
            case 205: return "Reset Content";
            case 206: return "Partial Content";
            case 300: return "Multiple Choices";
            case 301: return "Moved Permanently";
            case 302: return "Found";
            case 303: return "See Other";
            case 304: return "Not Modified";
            case 305: return "Use Proxy";
            case 306: return "Switch Proxy";
            case 307: return "Temporary Redirect";
            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 402: return "Payment Required";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 406: return "Not Acceptable";
            case 407: return "Proxy Authentication Required";
            case 408: return "Request Timeout";
            case 409: return "Conflict";
            case 410: return "Gone";
            case 411: return "Length Required";
            case 412: return "Precondition Failed";
            case 413: return "Request Entity Too Large";
            case 414: return "Request-URI Too Long";
            case 415: return "Unsupported Media Type";
            case 416: return "Requested Range Not Satisfiable";
            case 417: return "Expectation Failed";
            case 500: return "Internal Server Error";
            case 501: return "Not Implemented";
            case 502: return "Bad Gateway";
            case 503: return "Service Unavailable";
            case 504: return "Gateway Timeout";
            case 505: return "HTTP Version Not Supported";
            default: return ("Not supported status code.");
        }
    }

    std::string http_server::tolower(std::string& str) {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        return str;
    }

    void http_server::upload(mongols::request& req, const std::string& body) {
        try {
            if ((is_dir(TEMP_DIRECTORY) || mkdir(TEMP_DIRECTORY, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)) {
                std::shared_ptr<MPFD::Parser> POSTParser(new MPFD::Parser());
                POSTParser->SetTempDirForFileUpload(TEMP_DIRECTORY);
                POSTParser->SetUploadedFilesStorage(MPFD::Parser::StoreUploadedFilesInFilesystem);
                POSTParser->SetMaxCollectedDataLength(this->server->get_buffer_size());
                POSTParser->SetContentType(req.headers["Content-Type"]);
                POSTParser->AcceptSomeData(body.c_str(), body.size());
                auto fields = POSTParser->GetFieldsMap();

                for (auto &item : fields) {
                    if (item.second->GetType() == MPFD::Field::TextType) {
                        req.form.insert(std::make_pair(item.first, item.second->GetTextTypeContent()));
                    } else {
                        std::string upload_file_name = item.second->GetFileName(), ext;
                        std::string::size_type p = upload_file_name.find_last_of(".");
                        if (p != std::string::npos) {
                            ext = std::move(upload_file_name.substr(p));
                        }
                        std::string temp_file = std::move(TEMP_DIRECTORY + ("/" + mongols::random_string(req.client + item.second->GetFileName()).append(ext)));
                        rename(item.second->GetTempFileName().c_str(), temp_file.c_str());
                        req.form.insert(std::make_pair(item.first, temp_file));
                    }
                }
            }
        } catch (MPFD::Exception& err) {

        }
    }

    std::string http_server::work(
            const std::function<bool(const mongols::request&)>& req_filter
            , const std::function<void(const mongols::request&, mongols::response&)>& res_filter
            , const std::string& input
            , bool& keepalive
            , bool& send_to_other
            , tcp_server::client_t& client
            , tcp_server::filter_handler_function&) {
        send_to_other = false;
        mongols::request req;
        mongols::response res;
        std::string body;
        if (this->parse_reqeust(input, req, body)) {
            req.client = client.ip;

            if (body.size()>this->max_body_size) {
                body.clear();
            }
            if (req_filter(req)) {

                std::unordered_map<std::string, std::string>::const_iterator tmp;
                if ((tmp = req.headers.find("Connection")) != req.headers.end()) {
                    if (tmp->second == "keep-alive") {
                        keepalive = KEEPALIVE_CONNECTION;
                    }
                }
                if (!req.param.empty()) {
                    mongols::parse_param(req.param, req.form);
                }

                std::string session_val, cache_k;

                if (this->enable_session) {
                    if ((tmp = req.headers.find("Cookie")) != req.headers.end()) {
                        mongols::parse_param(tmp->second, req.cookies, ';');
                        if (this->redis.is_connected()&&(tmp = req.cookies.find(SESSION_NAME)) != req.cookies.end()) {
                            session_val = tmp->second;
                            if (this->redis.exists(tmp->second)) {
                                this->redis.hgetall(tmp->second, req.session);
                            } else {
                                this->redis.hset(tmp->second, SESSION_NAME, tmp->second);
                                this->redis.expire(tmp->second, this->session_expires);
                            }
                        }
                    } else {
                        std::chrono::system_clock::time_point now_time = std::chrono::system_clock::now();
                        std::time_t expire_time = std::chrono::system_clock::to_time_t(now_time + std::chrono::seconds(this->session_expires));
                        std::string session_cookie;
                        session_cookie.append(SESSION_NAME).append("=")
                                .append(mongols::random_string(""))
                                .append("; HttpOnly; Path=/; Expires=")
                                .append(mongols::http_time(&expire_time));
                        res.headers.insert(std::move(std::make_pair("Set-Cookie", session_cookie)));
                    }
                }

                if (this->enable_cache) {
                    cache_k = std::move(mongols::md5(req.method + req.uri + "?" + req.param));
                    if (this->redis.is_connected()) {
                        if (this->redis.exists(cache_k)) {
                            this->redis.hgetall(cache_k, req.cache);
                            std::unordered_map<std::string, http_server::cache_t> cache_v;
                        } else {
                            this->redis.hset(cache_k, "cache_key", cache_k);
                            this->redis.expire(cache_k, this->cache_expores);
                            req.cache["cache_key"] = cache_k;
                        }
                    }
                }
                if (!body.empty()&& (tmp = req.headers.find("Content-Type")) != req.headers.end()) {
                    if (tmp->second.size() != form_urlencoded_type_len
                            || tmp->second != form_urlencoded_type) {
                        this->upload(req, body);
                        body.clear();
                    } else {
                        mongols::parse_param(body, req.form);
                    }
                }

                res_filter(req, res);

                if (this->enable_session) {
                    if (this->redis.is_connected()&&!res.session.empty()) {
                        this->redis.hmset(session_val, res.session);
                    }
                }
                if (this->enable_cache) {
                    if (!res.cache.empty() && this->redis.is_connected()) {
                        this->redis.hmset(cache_k, res.cache);
                    }
                }

                if (res.headers.count("Content-Type") > 1) {
                    auto range = res.headers.equal_range("Content-Type");
                    for (auto & it = range.first; it != range.second; ++it) {
                        if (it->second == "text/html;charset=UTF-8") {
                            res.headers.erase(it);
                            break;
                        }
                    }
                }

            }
        }


        return this->create_response(res, keepalive);
    }

    void http_server::set_cache_expires(long long expires) {
        this->cache_expores = expires;
    }

    void http_server::set_session_expires(long long expires) {
        this->session_expires = expires;
    }

    void http_server::set_enable_cache(bool b) {
        this->enable_cache = b;
        if (!this->redis.is_connected()) {
            this->redis.connect();
        }
    }

    void http_server::set_enable_session(bool b) {
        this->enable_session = b;
        if (!this->redis.is_connected()) {
            this->redis.connect();
        }
    }



}


