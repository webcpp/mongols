#include <sys/stat.h>
#include <utility>
#include <algorithm>
#include <functional>
#include <memory>
#include <chrono>
#include <sstream>

#include "version.hpp"
#include "tcp_threading_server.hpp"
#include "http_server.hpp"
#include "util.hpp"
#include "MPFDParser/Parser.h"
#include "lib/msgpack.hpp"
#include "lib/hash/md5.hpp"
#include "lib/leveldb/cache.h"

#define form_urlencoded_type "application/x-www-form-urlencoded"
#define form_multipart_type "multipart/form-data"

#define TEMP_DIRECTORY "temp"
#define SESSION_NAME "SESSIONID"
#define LEVELDB_PATH "mongols_leveldb"

namespace mongols {

    http_server::http_server(const std::string& host, int port
            , int timeout
            , size_t buffer_size
            , size_t thread_size
            , size_t max_body_size
            , int max_event_size)
    : server(0), max_body_size(max_body_size), lru_cache_size(1024), db(0), db_options()
    , session_expires(3600), cache_expires(3600), lru_cache_expires(300)
    , enable_session(false), enable_cache(false), enable_lru_cache(false)
    , db_path(LEVELDB_PATH), lru_cache(0) {
        if (thread_size > 0) {
            this->server = new tcp_threading_server(host, port, timeout, buffer_size, thread_size, max_event_size);
        } else {
            this->server = new tcp_server(host, port, timeout, buffer_size, max_event_size);
        }
        if (this->server) {
            this->db_options.create_if_missing = true;
        }

    }

    http_server::~http_server() {
        if (this->lru_cache) {
            delete this->lru_cache;
        }
        if (this->db) {
            delete this->db;
        }
        if (this->db_options.block_cache) {
            delete this->db_options.block_cache;
        }
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
        if (this->enable_cache || this->enable_session) {
            leveldb::DB::Open(this->db_options, this->db_path, &this->db);
        }
        if (this->enable_lru_cache) {
            this->lru_cache = new lru11::Cache<std::string, std::shared_ptr < cache_t >> (this->lru_cache_size, 0);
        }
        this->server->run(g);
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
        res.headers.insert(std::move(std::make_pair("Content-Length", std::to_string(res.content.size()))));
        for (auto& i : res.headers) {
            output.append(i.first).append(": ").append(i.second).append("\r\n");
        }
        output.append("\r\n").append(res.content);
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
            , const std::pair<char*, size_t>& input
            , bool& keepalive
            , bool& send_to_other
            , tcp_server::client_t& client
            , tcp_server::filter_handler_function&) {
        send_to_other = false;
        mongols::request req;
        mongols::response res;
        mongols::http_request_parser parser(req);
        if (parser.parse(input.first, input.second)) {
            std::unordered_map<std::string, std::string>::const_iterator tmp_iterator;
            std::string& body = parser.get_body();
            req.client = client.ip;
            if ((tmp_iterator = req.headers.find("User-Agent")) != req.headers.end()) {
                req.user_agent = tmp_iterator->second;
            }
            if (body.size()>this->max_body_size) {
                body.clear();
                res.content = std::move("Not allowed to upload this resource.");
                res.status = 500;
                keepalive = CLOSE_CONNECTION;
                return this->create_response(res, keepalive);
            }
            if (req_filter(req)) {
                if ((tmp_iterator = req.headers.find("Connection")) != req.headers.end()) {
                    if (tmp_iterator->second == "keep-alive") {
                        keepalive = KEEPALIVE_CONNECTION;
                    }
                }

                if ((tmp_iterator = req.headers.find("If-Modified-Since")) != req.headers.end()
                        && difftime(time(0), mongols::parse_http_time((u_char*) tmp_iterator->second.c_str(), tmp_iterator->second.size()))
                        <= this->lru_cache_expires) {
                    res.status = 304;
                    res.content.clear();
                    return this->create_response(res, keepalive);
                }
                std::string cache_k = std::move(mongols::md5(req.method + req.uri + "?" + req.param));
                if (req.method == "GET" && this->enable_lru_cache) {
                    if (this->lru_cache->contains(cache_k)) {
                        auto cache_ele = this->lru_cache->get(cache_k);
                        if (cache_ele->expired(this->lru_cache_expires)) {
                            this->lru_cache->remove(cache_k);
                        } else {
                            res.status = cache_ele->status;
                            res.content = cache_ele->content;
                            res.headers.find("Content-Type")->second = cache_ele->content_type;
                            time_t now = time(0);
                            res.headers.insert(std::move(std::make_pair("Last-Modified", mongols::http_time(&now))));
                            return this->create_response(res, keepalive);
                        }
                    }
                }

                if (!req.param.empty()) {
                    mongols::parse_param(req.param, req.form);
                }

                std::string session_val, cache_v;

                if (this->db) {
                    if (this->enable_session) {
                        if ((tmp_iterator = req.headers.find("Cookie")) != req.headers.end()) {
                            mongols::parse_param(tmp_iterator->second, req.cookies, ';');
                            if ((tmp_iterator = req.cookies.find(SESSION_NAME)) != req.cookies.end()) {
                                session_val = tmp_iterator->second;
                                std::string v;
                                if (this->db->Get(leveldb::ReadOptions(), tmp_iterator->second, &v).ok()) {
                                    this->deserialize(v, req.session);
                                } else {
                                    this->db->Put(leveldb::WriteOptions(), tmp_iterator->second, this->serialize(req.session));
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
                        if (this->db->Get(leveldb::ReadOptions(), cache_k, &cache_v).ok()) {
                            this->deserialize(cache_v, req.cache);
                        } else {
                            this->db->Put(leveldb::WriteOptions(), cache_k, this->serialize(req.cache));
                        }
                    }
                }

                if (!body.empty()&& (tmp_iterator = req.headers.find("Content-Type")) != req.headers.end()) {
                    if (tmp_iterator->second.find(form_multipart_type) != std::string::npos) {
                        this->upload(req, body);
                        body.clear();
                    } else if (tmp_iterator->second.find(form_urlencoded_type) != std::string::npos) {
                        mongols::parse_param(body, req.form);
                    } else {
                        req.form["__body__"] = std::move(body);
                    }
                }

                res_filter(req, res);

                std::unordered_map<std::string, std::string>* ptr = 0;
                if (!res.session.empty() && this->db) {

                    if (!req.session.empty()) {
                        for (auto &i : res.session) {
                            req.session[i.first] = std::move(i.second);
                        }
                        ptr = &req.session;
                    } else {
                        ptr = &res.session;
                    }
                    this->db->Put(leveldb::WriteOptions(), session_val, this->serialize(*ptr));

                }

                if (!res.cache.empty() && this->db) {
                    if (!req.cache.empty()) {
                        for (auto &i : res.cache) {
                            req.cache[i.first] = std::move(i.second);
                        }
                        ptr = &req.cache;
                    } else {
                        ptr = &res.cache;
                    }
                    this->db->Put(leveldb::WriteOptions(), cache_k, this->serialize(*ptr));
                }
                if (req.method == "GET" && this->enable_lru_cache && res.status == 200 && this->lru_cache_expires > 0) {
                    std::shared_ptr<cache_t> cache_ele = std::make_shared<cache_t>();
                    cache_ele->content = res.content;
                    cache_ele->status = res.status;
                    cache_ele->content_type = res.headers.find("Content-Type")->second;
                    this->lru_cache->insert(cache_k, cache_ele);
                    time_t now = time(0);
                    res.headers.insert(std::move(std::make_pair("Last-Modified", mongols::http_time(&now))));
                }

            }
        }


        return this->create_response(res, keepalive);
    }

    void http_server::set_session_expires(long long expires) {
        this->session_expires = expires;
    }

    void http_server::set_cache_expires(long long expires) {
        this->cache_expires = expires;
    }

    void http_server::set_enable_cache(bool b) {
        this->enable_cache = b;
    }

    void http_server::set_enable_session(bool b) {
        this->enable_session = b;
    }

    void http_server::set_max_file_size(size_t len) {
        this->db_options.max_file_size = len;
    }

    void http_server::set_max_open_files(int len) {
        this->db_options.max_open_files = len;
    }

    void http_server::set_write_buffer_size(size_t len) {
        this->db_options.write_buffer_size = len;
    }

    void http_server::set_cache_size(size_t len) {
        this->db_options.block_cache = leveldb::NewLRUCache(len);
    }

    void http_server::set_enable_compression(bool b) {
        if (!b) {
            this->db_options.compression = leveldb::kNoCompression;
        }
    }

    void http_server::set_db_path(const std::string& path) {
        this->db_path = path;
    }

    void http_server::set_enable_lru_cache(bool b) {
        this->enable_lru_cache = b;
    }

    void http_server::set_lru_cache_expires(long long expires) {
        this->lru_cache_expires = expires;
    }

    void http_server::set_lru_cache_size(size_t len) {
        this->lru_cache_size = len;
    }

    std::string http_server::serialize(const std::unordered_map<std::string, std::string>& m) {
        std::stringstream ss;
        msgpack::pack(ss, m);
        ss.seekg(0);
        return ss.str();
    }

    void http_server::deserialize(const std::string& str, std::unordered_map<std::string, std::string>& m) {
        msgpack::unpack(str.c_str(), str.size()).get().convert(m);
    }

    http_server::cache_t::cache_t() : status(200), t(time(0)), content_type(), content() {
    }

    bool http_server::cache_t::expired(long long expires) const {
        return difftime(time(0), this->t) > expires;
    }

}


