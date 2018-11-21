#include <unistd.h>
#include <algorithm>
#include <functional>

#include "util.hpp"

#include "lib/leveldb/cache.h"

#include "leveldb_server.hpp"


namespace mongols {

    leveldb_server::leveldb_server(const std::string& host, int port, int timeout, size_t buffer_size, size_t thread_size, size_t max_body_size, int max_event_size)
    : server(0), db(0), options() {

        this->server = new http_server(host, port, timeout, buffer_size, thread_size, max_body_size, max_event_size);

        this->options.create_if_missing = true;
    }

    leveldb_server::~leveldb_server() {
        if (this->db) {
            delete this->db;
        }
        if (this->options.block_cache) {
            delete this->options.block_cache;
        }
        if (this->server) {
            delete this->server;
        }

    }
    //default: 2MB

    void leveldb_server::set_max_file_size(size_t len) {
        this->options.max_file_size = len;
    }
    //default: 1000

    void leveldb_server::set_max_open_files(int len) {
        this->options.max_open_files = len;
    }

    //default: 4MB

    void leveldb_server::set_write_buffer_size(size_t len) {
        this->options.write_buffer_size = len;
    }

    void leveldb_server::set_cache_size(size_t len) {
        this->options.block_cache = leveldb::NewLRUCache(len);
    }

    void leveldb_server::set_enable_compression(bool b) {
        if (!b) {
            this->options.compression = leveldb::kNoCompression;
        }
    }

    void leveldb_server::work(const mongols::request& req, mongols::response& res) {
        if (req.uri.size() < 2) {
            goto leveldb_error;
        }
        if (req.method == "GET") {
            std::string value;
            if (this->db->Get(leveldb::ReadOptions(), req.uri.substr(1), &value).ok()) {
                res.headers.find("Content-Type")->second = "application/octet-stream";
                res.status = 200;
                res.content = std::move(value);
            } else {
                goto leveldb_error;
            }
        } else if (req.method == "POST") {
            std::unordered_map<std::string, std::string>::const_iterator item;
            if ((item = req.form.find(req.uri.substr(1))) != req.form.end()) {

                if (this->db->Put(leveldb::WriteOptions(), item->first, item->second).ok()) {
                    res.headers.find("Content-Type")->second = "text/plain;charset=UTF-8";
                    res.status = 200;
                    res.content = std::move("OK");
                } else {
                    goto leveldb_error;
                }
            } else {
                goto leveldb_error;
            }
        } else if (req.method == "DELETE") {
            if (this->db->Delete(leveldb::WriteOptions(), req.uri.substr(1)).ok()) {
                res.headers.find("Content-Type")->second = "text/plain;charset=UTF-8";
                res.status = 200;
                res.content = std::move("OK");
            } else {
                goto leveldb_error;
            }
        } else {
leveldb_error:
            res.headers.find("Content-Type")->second = "text/plain;charset=UTF-8";
            res.status = 500;
            res.content = std::move("ERROR");
        }
    }

    bool leveldb_server::filter(const mongols::request& req) {
        return true;
    }

    void leveldb_server::run(const std::string& path) {
        if (leveldb::DB::Open(this->options, path, &this->db).ok()) {
            this->server->run(std::bind(&leveldb_server::filter, this, std::placeholders::_1)
                    , std::bind(&leveldb_server::work, this
                    , std::placeholders::_1
                    , std::placeholders::_2));
        }
    }

    void leveldb_server::set_enable_lru_cache(bool b) {
        this->server->set_enable_lru_cache(b);
    }

    void leveldb_server::set_lru_cache_expires(long long expires) {
        this->server->set_lru_cache_expires(expires);
    }














}
