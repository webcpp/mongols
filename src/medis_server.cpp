
#include <sstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <list>
#include <queue>
#include <set>
#include <stack>
#include <functional>
#include <algorithm>

#include "medis_server.hpp"
#include "lib/msgpack.hpp"
#include "lib/leveldb/cache.h"
#include "lib/re2/re2.h"
#include "posix_regex.hpp"
#include "util.hpp"


namespace mongols {

    medis_server::medis_server(const std::string& host
            , int port
            , int timeout
            , size_t buffer_size
            , size_t thread_size
            , int max_event_size)
    : server(0), db(0), options(), sqldb(0), vm(), resp_decoder(), resp_encoder(), op()
    , lru_string_max_size(1024)
    , lru_map_max_size(1024)
    , lru_list_max_size(1024)
    , lru_set_max_size(1024)
    , lru_queue_max_size(1024)
    , lru_stack_max_size(1024)
    , string_data()
    , map_data()
    , list_data()
    , set_data()
    , queue_data()
    , stack_data() {
        if (thread_size > 0) {
            this->server = new tcp_threading_server(host, port, timeout, buffer_size, thread_size, max_event_size);
        } else {
            this->server = new tcp_server(host, port, timeout, buffer_size, max_event_size);
        }

        this->op["GET"] = &medis_server::get;
        this->op["SET"] = &medis_server::set;
        this->op["DEL"] = &medis_server::del;
        this->op["EXISTS"] = &medis_server::exists;

        this->op["MGET"] = &medis_server::mget;
        this->op["MSET"] = &medis_server::mset;
        this->op["GETSET"] = &medis_server::getset;
        this->op["STRLEN"] = &medis_server::strlen;
        this->op["APPEND"] = &medis_server::append;
        this->op["SETRANGE"] = &medis_server::setrange;
        this->op["GETRANGE"] = &medis_server::getrange;

        this->op["HGET"] = &medis_server::hget;
        this->op["HSET"] = &medis_server::hset;
        this->op["HDEL"] = &medis_server::hdel;
        this->op["HEXISTS"] = &medis_server::hexists;
        this->op["HGETALL"] = &medis_server::hgetall;
        this->op["HLEN"] = &medis_server::hlen;
        this->op["HMGET"] = &medis_server::hmget;
        this->op["HMSET"] = &medis_server::hmset;


        this->op["LFRONT"] = &medis_server::lfront;
        this->op["LBACK"] = &medis_server::lback;
        this->op["LPUSH_FRONT"] = &medis_server::lpush_front;
        this->op["LPUSH_BACK"] = &medis_server::lpush_back;
        this->op["LPOP_FRONT"] = &medis_server::lpop_front;
        this->op["LPOP_BACK"] = &medis_server::lpop_back;
        this->op["LLEN"] = &medis_server::llen;
        this->op["LRANGE"] = &medis_server::lrange;


        this->op["SADD"] = &medis_server::sadd;
        this->op["SDEL"] = &medis_server::sdel;
        this->op["SMEMBERS"] = &medis_server::smembers;
        this->op["SLEN"] = &medis_server::slen;
        this->op["SEXISTS"] = &medis_server::sexists;
        this->op["SDIFF"] = &medis_server::sdifference;
        this->op["SINTER"] = &medis_server::sintersection;
        this->op["SUNION"] = &medis_server::sunion;
        this->op["SSYDIFF"] = &medis_server::ssymmetric_difference;


        this->op["INCR"] = &medis_server::incr;
        this->op["INCRBY"] = &medis_server::incrby;
        this->op["DECR"] = &medis_server::decr;
        this->op["DECRBY"] = &medis_server::decrby;

        this->op["PING"] = &medis_server::ping;
        this->op["ECHO"] = &medis_server::echo;

        this->op["FLUSHALL"] = &medis_server::flushall;


        // the vesion with memory
        this->op["_FLUSHALL"] = &medis_server::_flushall;
        this->op["_HGET"] = &medis_server::_hget;
        this->op["_HSET"] = &medis_server::_hset;
        this->op["_HDEL"] = &medis_server::_hdel;
        this->op["_HEXISTS"] = &medis_server::_hexists;
        this->op["_HERASE"] = &medis_server::_herase;
        this->op["_HGETALL"] = &medis_server::_hgetall;
        this->op["_HLEN"] = &medis_server::_hlen;
        this->op["_HMGET"] = &medis_server::_hmget;
        this->op["_HMSET"] = &medis_server::_hmset;
        this->op["_HEXPIRE"] = &medis_server::_hexpire;
        this->op["_HTTL"] = &medis_server::_httl;

        this->op["_LFRONT"] = &medis_server::_lfront;
        this->op["_LBACK"] = &medis_server::_lback;
        this->op["_LPUSH_FRONT"] = &medis_server::_lpush_front;
        this->op["_LPUSH_BACK"] = &medis_server::_lpush_back;
        this->op["_LPOP_FRONT"] = &medis_server::_lpop_front;
        this->op["_LPOP_BACK"] = &medis_server::_lpop_back;
        this->op["_LLEN"] = &medis_server::_llen;
        this->op["_LERASE"] = &medis_server::_lerase;
        this->op["_LRANGE"] = &medis_server::_lrange;
        this->op["_LEXISTS"] = &medis_server::_lexists;
        this->op["_LEXPIRE"] = &medis_server::_lexpire;
        this->op["_LTTL"] = &medis_server::_lttl;

        this->op["_SADD"] = &medis_server::_sadd;
        this->op["_SDEL"] = &medis_server::_sdel;
        this->op["_SREM"] = &medis_server::_sdel;
        this->op["_SMEMBERS"] = &medis_server::_smembers;
        this->op["_SLEN"] = &medis_server::_slen;
        this->op["_SEXISTS"] = &medis_server::_sexists;
        this->op["_SDIFF"] = &medis_server::_sdifference;
        this->op["_SINTER"] = &medis_server::_sintersection;
        this->op["_SUNION"] = &medis_server::_sunion;
        this->op["_SSYDIFF"] = &medis_server::_ssymmetric_difference;
        this->op["_SERASE"] = &medis_server::_serase;
        this->op["_SEXPIRE"] = &medis_server::_sexpire;
        this->op["_STTL"] = &medis_server::_sttl;

        this->op["_QPUSH"] = &medis_server::_qpush;
        this->op["_QPOP"] = &medis_server::_qpop;
        this->op["_QFRONT"] = &medis_server::_qfront;
        this->op["_QBACK"] = &medis_server::_qback;
        this->op["_QEMPTY"] = &medis_server::_qempty;
        this->op["_QERASE"] = &medis_server::_qerase;
        this->op["_QLEN"] = &medis_server::_qlen;
        this->op["_QEXPIRE"] = &medis_server::_qexpire;
        this->op["_QTTL"] = &medis_server::_qttl;


        this->op["_ZPOP"] = &medis_server::_zpop;
        this->op["_ZTOP"] = &medis_server::_ztop;
        this->op["_ZPUSH"] = &medis_server::_zpush;
        this->op["_ZEMPTY"] = &medis_server::_zempty;
        this->op["_ZERASE"] = &medis_server::_zerase;
        this->op["_ZLEN"] = &medis_server::_zlen;
        this->op["_ZEXPIRE"] = &medis_server::_zexpire;
        this->op["_ZTTL"] = &medis_server::_zttl;


        this->op["_GET"] = &medis_server::_get;
        this->op["_SET"] = &medis_server::_set;
        this->op["_DEL"] = &medis_server::_del;
        this->op["_EXISTS"] = &medis_server::_exists;
        this->op["_EXPIRE"] = &medis_server::_expire;
        this->op["_TTL"] = &medis_server::_ttl;

        this->op["_MGET"] = &medis_server::_mget;
        this->op["_MSET"] = &medis_server::_mset;
        this->op["_GETSET"] = &medis_server::_getset;
        this->op["_STRLEN"] = &medis_server::_strlen;
        this->op["_APPEND"] = &medis_server::_append;
        this->op["_SETRANGE"] = &medis_server::_setrange;
        this->op["_GETRANGE"] = &medis_server::_getrange;

        this->op["_INCR"] = &medis_server::_incr;
        this->op["_INCRBY"] = &medis_server::_incrby;
        this->op["_DECR"] = &medis_server::_decr;
        this->op["_DECRBY"] = &medis_server::_decrby;


        // sqlite

        this->op["SQLCMD"] = &medis_server::sql_cmd;
        this->op["SQLBINDCMD"] = &medis_server::sql_bind_cmd;
        this->op["SQLTRANSACTION"] = &medis_server::sql_transaction;
        this->op["SQLQUERY"] = &medis_server::sql_query;

        // lua 
        this->op["LUACONTENT"] = &medis_server::lua_content;
        this->op["LUASCRIPT"] = &medis_server::lua_script;


        // lua embed function
        this->vm.setErrorHandler([&](int errCode, const char * szError) {

        });
        this->vm["mongols_regex"] = kaguya::NewTable();
        kaguya::LuaTable regex_tbl = this->vm["mongols_regex"];
        regex_tbl["full_match"] = kaguya::function([](const std::string& pattern, const std::string & str) {
            return RE2::FullMatch(str, pattern);
        });
        regex_tbl["partial_match"] = kaguya::function([](const std::string& pattern, const std::string & str) {
            return RE2::PartialMatch(str, pattern);
        });
        regex_tbl["match"] = kaguya::function([](const std::string& pattern, const std::string & str) {
            mongols::posix_regex regex(pattern);
            std::vector<std::string> v;
            if (regex.match(str, v)) {
                return v;
            }
            return v;
        });
        this->vm["medis"] = kaguya::NewTable();
        kaguya::LuaTable medis_tbl = this->vm["medis"];
        medis_tbl["CMD"] = kaguya::function([&](kaguya::VariadicArgType args) {
            if (!args.empty()) {
                std::unordered_map<std::string, op_funcion>::const_iterator iterator = this->op.find(args[0]);
                if (iterator != this->op.end()) {
                    simple_resp::decode_result ret = this->resp_decoder.decode((this->*(iterator->second))(args));
                    return ret.response;
                }
            }
            return std::vector<std::string>();
        });



    }

    medis_server::~medis_server() {

        if (this->db) {
            delete this->db;
        }
        if (this->options.block_cache) {
            delete this->options.block_cache;
        }
        if (this->sqldb) {
            delete this->sqldb;
        }
        if (this->server) {
            delete this->server;
        }

    }

    void medis_server::set_max_file_size(size_t len) {
        this->options.max_file_size = len;
    }

    void medis_server::set_max_open_files(int len) {
        this->options.max_open_files = len;
    }

    void medis_server::set_write_buffer_size(size_t len) {
        this->options.write_buffer_size = len;
    }

    void medis_server::set_cache_size(size_t len) {
        this->options.block_cache = leveldb::NewLRUCache(len);
    }

    void medis_server::set_enable_compression(bool b) {
        if (!b) {
            this->options.compression = leveldb::kNoCompression;
        }
    }

    void medis_server::set_lua_package_path(const std::string& package_path, const std::string& package_cpath) {
        if (!package_path.empty()) {
            this->vm("package.path='" + package_path + "'.. package.path");
        }
        if (!package_cpath.empty()) {
            this->vm("package.cpath='" + package_cpath + "'.. package.cpath");
        }

    }

    void medis_server::run(const std::string& path, const std::string& db_name) {
        this->options.create_if_missing = true;
        if (leveldb::DB::Open(this->options, path, &this->db).ok()) {
            this->string_data = std::make_shared<lru11::Cache < std::string, shared_mongols_string >> (this->lru_string_max_size);
            this->map_data = std::make_shared<lru11::Cache < std::string, shared_mongols_map >> (this->lru_map_max_size);
            this->list_data = std::make_shared<lru11::Cache < std::string, shared_mongols_list >> (this->lru_list_max_size);
            this->queue_data = std::make_shared<lru11::Cache < std::string, shared_mongols_queue >> (this->lru_queue_max_size);
            this->stack_data = std::make_shared<lru11::Cache < std::string, shared_mongols_stack >> (this->lru_stack_max_size);
            this->set_data = std::make_shared<lru11::Cache < std::string, shared_mongols_set >> (this->lru_set_max_size);
            try {
                this->sqldb = new sqlite3pp::database(db_name.c_str());
                this->sqldb->enable_extended_result_codes(true);
                this->sqldb->enable_foreign_keys(true);
                this->sqldb->enable_triggers(true);
                this->server->run(std::bind(&medis_server::work, this
                        , std::placeholders::_1
                        , std::placeholders::_2
                        , std::placeholders::_3
                        , std::placeholders::_4
                        , std::placeholders::_5));
            } catch (std::exception&) {
            }

        }
    }

    std::string medis_server::work(const std::pair<char*, size_t>& input
            , bool& keepalive
            , bool& send_to_other
            , tcp_server::client_t& client
            , tcp_server::filter_handler_function& f) {
        send_to_other = false;
        keepalive = CLOSE_CONNECTION;
        simple_resp::decode_result de_ret;
        try {
            de_ret = this->resp_decoder.decode(std::string(input.first, input.second));
        } catch (std::exception&e) {
            goto medis_error;
        }
        if (de_ret.status == simple_resp::STATUS::OK) {
            std::unordered_map<std::string, op_funcion>::const_iterator iterator = this->op.find(de_ret.response[0]);
            if (iterator != this->op.end()) {
                keepalive = KEEPALIVE_CONNECTION;
                return (this->*(iterator->second))(de_ret.response);
            } else {
                goto medis_error;
            }
        } else {
            goto medis_error;
        }

medis_error:
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;

    }

    std::string medis_server::get(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;

    }

    std::string medis_server::set(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            if (this->db->Put(leveldb::WriteOptions(), ret[1], ret[2]).ok()) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;

    }

    std::string medis_server::exists(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::del(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->db->Delete(leveldb::WriteOptions(), ret[1]).ok()) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::append(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                if (this->db->Put(leveldb::WriteOptions(), ret[1], v.append(ret[2])).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(v.size())}).response;
                }
            } else {
                if (this->db->Put(leveldb::WriteOptions(), ret[1], ret[2]).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(ret[2].size())}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::getrange(const std::vector<std::string>& ret) {
        if (ret.size() == 4) {
            try {
                std::string v;
                size_t start = std::stoul(ret[2]), count = std::stoul(ret[3]);
                if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v.substr(start, count)}).response;
                }
            } catch (std::exception&) {

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::setrange(const std::vector<std::string>& ret) {
        if (ret.size() == 4) {
            try {
                std::string v;
                size_t start = std::stoul(ret[2]);
                if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                    v.replace(start, ret[3].size(), ret[3]);
                    if (this->db->Put(leveldb::WriteOptions(), ret[1], v).ok()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(v.size())}).response;
                    }
                }
            } catch (std::exception&) {
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::getset(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                if (this->db->Put(leveldb::WriteOptions(), ret[1], ret[2]).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::mget(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 1) {
            std::vector<std::string> vs;
            for (size_t i = 1; i < len; ++i) {
                std::string v;
                if (this->db->Get(leveldb::ReadOptions(), ret[i], &v).ok()) {
                    vs.emplace_back(v);
                } else {
                    vs.emplace_back("nil");
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, vs).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::mset(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len >= 2 && (len - 1) % 2 == 0) {
            for (size_t i = 1; i < len - 1; ++++i) {
                if (this->db->Put(leveldb::WriteOptions(), ret[i], ret[i + 1]).ok()) {

                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::strlen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(v.size())}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    template<typename T>
    void medis_server::deserialize(const std::string& str, T& m) {
        msgpack::unpack(str.c_str(), str.size()).get().convert(m);
    }

    template<typename T>
    std::string medis_server::serialize(const T& m) {
        std::stringstream ss;
        msgpack::pack(ss, m);
        ss.seekg(0);
        return ss.str();
    }

    std::string medis_server::hget(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_map m;
                    this->deserialize(v, m);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{m[ret[2]]}).response;
                } catch (std::exception& e) {
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::hset(const std::vector<std::string>& ret) {
        if (ret.size() == 4) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_map m;
                    this->deserialize(v, m);
                    m[ret[2]] = ret[3];
                    if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(m)).ok()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                    }
                } catch (std::exception& e) {
                }
            } else {
                mongols_map m;
                m[ret[2]] = ret[3];
                if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(m)).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::hdel(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_map m;
                    this->deserialize(v, m);
                    size_t n = 0;
                    mongols_map::const_iterator iter;
                    for (size_t i = 2; i < len; ++i) {
                        if ((iter = m.find(ret[i])) != m.end()) {
                            m.erase(iter);
                            ++n;
                        }
                    }
                    if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(m)).ok()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(n)}).response;
                    }
                } catch (std::exception& e) {
                }

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::hexists(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_map m;
                    this->deserialize(v, m);
                    if (m.find(ret[2]) != m.end()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                    }
                } catch (std::exception& e) {

                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::hgetall(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                mongols_map m;
                this->deserialize(v, m);
                std::vector<std::string> vs;
                for (auto & item : m) {
                    vs.emplace_back(item.first);
                    vs.emplace_back(item.second);
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, vs).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS,{}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::hlen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                mongols_map m;
                this->deserialize(v, m);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(m.size())}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::hmset(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2 && (len - 2) % 2 == 0) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                mongols_map m;
                this->deserialize(v, m);
                for (size_t i = 2; i < len - 1; ++++i) {
                    m[ret[i]] = ret[i + 1];
                }
                if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(m)).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
                }
            } else {
                mongols_map m;
                for (size_t i = 2; i < len - 1; ++++i) {
                    m[ret[i]] = ret[i + 1];
                }
                if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(m)).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
                }

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::hmget(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                mongols_map m;
                this->deserialize(v, m);
                std::vector<std::string> vs;
                for (size_t i = 2; i < len; ++i) {
                    vs.emplace_back(m[ret[i]]);
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, vs).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS,{}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::lfront(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_list l;
                    this->deserialize(v, l);
                    if (!l.empty()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{l.front()}).response;
                    }
                } catch (std::exception& e) {
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::lback(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_list l;
                    this->deserialize(v, l);
                    if (!l.empty()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{l.back()}).response;
                    }
                } catch (std::exception& e) {
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::lpush_front(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_list l;
                    this->deserialize(v, l);
                    size_t i = 0;
                    for (size_t j = 2; j < len; ++j) {
                        l.emplace_front(ret[j]);
                        ++i;
                    }
                    if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(l)).ok()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
                    }
                } catch (std::exception& e) {
                }
            } else {
                mongols_list l;
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    l.emplace_front(ret[j]);
                    ++i;
                }
                if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(l)).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::lpush_back(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_list l;
                    this->deserialize(v, l);
                    size_t i = 0;
                    for (size_t j = 2; j < len; ++j) {
                        l.emplace_back(ret[j]);
                        ++i;
                    }
                    if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(l)).ok()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
                    }
                } catch (std::exception& e) {
                }

            } else {
                mongols_list l;
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    l.emplace_back(ret[j]);
                    ++i;
                }
                if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(l)).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::lpop_front(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_list l;
                    this->deserialize(v, l);
                    if (!l.empty()) {
                        std::string v = std::move(l.front());
                        l.pop_front();
                        if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(l)).ok()) {
                            return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v}).response;
                        }
                    }
                } catch (std::exception& e) {
                }

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::lpop_back(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_list l;
                    this->deserialize(v, l);
                    if (!l.empty()) {
                        std::string v = std::move(l.back());
                        l.pop_back();
                        if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(l)).ok()) {
                            return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v}).response;
                        }
                    }
                } catch (std::exception& e) {

                }

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::llen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_list l;
                    this->deserialize(v, l);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(l.size())}).response;
                } catch (std::exception& e) {

                }

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::lrange(const std::vector<std::string>& ret) {
        if (ret.size() == 4) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    long start = std::stol(ret[2]), count = std::stol(ret[3]), i = 0;
                    if (count >= 0) {
                        mongols_list l;
                        this->deserialize(v, l);
                        std::vector<std::string> vs;
                        if (start >= 0) {
                            for (auto& item : l) {
                                long tmp = i - start;
                                if (tmp >= count) {
                                    break;
                                } else if (tmp >= 0 && tmp < count) {
                                    vs.emplace_back(item);
                                }
                                ++i;

                            }
                        } else {
                            i = -1;
                            for (mongols_list::reverse_iterator ritem = l.rbegin(); ritem != l.rend(); ++ritem) {
                                long tmp = start - i;
                                if (tmp >= count) {
                                    break;
                                } else if (tmp >= 0 && tmp < count) {
                                    vs.emplace_back(*ritem);
                                }
                                --i;
                            }
                        }
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, vs).response;
                    }

                } catch (std::exception&) {
                }

            }
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::sadd(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_set s;
                    this->deserialize(v, s);
                    size_t i = 0;
                    for (size_t j = 2; j < len; ++j) {
                        auto tmp = s.emplace(ret[j]);
                        if (tmp.second) {
                            ++i;
                        }

                    }
                    if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(s)).ok()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
                    }
                } catch (std::exception& e) {
                }

            } else {
                mongols_set s;
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    auto tmp = s.emplace(ret[j]);
                    if (tmp.second) {
                        ++i;
                    }
                }
                if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(s)).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::sdel(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_set s;
                    this->deserialize(v, s);
                    size_t i = 0;
                    for (size_t j = 2; j < len; ++j) {
                        i += s.erase(ret[j]);
                    }
                    if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(s)).ok()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
                    }
                } catch (std::exception& e) {

                }

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::sexists(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_set s;
                    this->deserialize(v, s);
                    if (s.find(ret[2]) != s.end()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                    }
                } catch (std::exception& e) {

                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::sdifference(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v1, v2;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v1).ok()
                    && this->db->Get(leveldb::ReadOptions(), ret[2], &v2).ok()) {
                try {
                    mongols_set s1, s2;
                    this->deserialize(v1, s1);
                    this->deserialize(v2, s2);
                    std::vector<std::string> v(s1.size() + s2.size());
                    auto iter = std::set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), v.begin());
                    v.resize(iter - v.begin());
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, v).response;
                } catch (std::exception& e) {

                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::sintersection(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v1, v2;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v1).ok()
                    && this->db->Get(leveldb::ReadOptions(), ret[2], &v2).ok()) {
                try {
                    mongols_set s1, s2;
                    this->deserialize(v1, s1);
                    this->deserialize(v2, s2);
                    std::vector<std::string> v(s1.size() + s2.size());
                    auto iter = std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), v.begin());
                    v.resize(iter - v.begin());
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, v).response;
                } catch (std::exception& e) {

                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::ssymmetric_difference(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v1, v2;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v1).ok()
                    && this->db->Get(leveldb::ReadOptions(), ret[2], &v2).ok()) {
                try {
                    mongols_set s1, s2;
                    this->deserialize(v1, s1);
                    this->deserialize(v2, s2);
                    std::vector<std::string> v(s1.size() + s2.size());
                    auto iter = std::set_symmetric_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), v.begin());
                    v.resize(iter - v.begin());
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, v).response;
                } catch (std::exception& e) {

                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::sunion(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v1, v2;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v1).ok()
                    && this->db->Get(leveldb::ReadOptions(), ret[2], &v2).ok()) {
                try {
                    mongols_set s1, s2;
                    this->deserialize(v1, s1);
                    this->deserialize(v2, s2);
                    std::vector<std::string> v(s1.size() + s2.size());
                    auto iter = std::set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), v.begin());
                    v.resize(iter - v.begin());

                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, v).response;

                } catch (std::exception& e) {

                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::slen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_set s;
                    this->deserialize(v, s);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(s.size())}).response;
                } catch (std::exception& e) {

                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::smembers(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    mongols_set s;
                    this->deserialize(v, s);
                    std::vector<std::string> sv;
                    for (auto& i : s) {
                        sv.emplace_back(i);
                    }
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, sv).response;
                } catch (std::exception& e) {

                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS,{}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::incr(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    long s = std::stol(v);
                    std::string v = std::move(std::to_string(++s));
                    if (this->db->Put(leveldb::WriteOptions(), ret[1], v).ok()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
                    }
                } catch (std::exception& e) {

                }
            } else {
                std::string v("1");
                if (this->db->Put(leveldb::WriteOptions(), ret[1], v).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::incrby(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    long s = std::stol(v) + std::stol(ret[2]);
                    std::string v = std::move(std::to_string(s));
                    if (this->db->Put(leveldb::WriteOptions(), ret[1], v).ok()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
                    }
                } catch (std::exception& e) {

                }
            } else {
                if (this->db->Put(leveldb::WriteOptions(), ret[1], ret[2]).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{ret[2]}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::decr(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    long s = std::stol(v);
                    std::string v = std::move(std::to_string(--s));
                    if (this->db->Put(leveldb::WriteOptions(), ret[1], v).ok()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
                    }
                } catch (std::exception& e) {

                }
            } else {
                std::string v("-1");
                if (this->db->Put(leveldb::WriteOptions(), ret[1], v).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::decrby(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v;
            if (this->db->Get(leveldb::ReadOptions(), ret[1], &v).ok()) {
                try {
                    long s = std::stol(v) - std::stol(ret[2]);
                    std::string v = std::move(std::to_string(s));
                    if (this->db->Put(leveldb::WriteOptions(), ret[1], v).ok()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
                    }
                } catch (std::exception& e) {

                }
            } else {
                std::string v = std::move("-" + ret[2]);
                if (this->db->Put(leveldb::WriteOptions(), ret[1], v).ok()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::echo(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{ret[1]}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::ping(const std::vector<std::string>& ret) {
        if (ret.size() == 1) {
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"PONG"}).response;
        }
        if (ret.size() == 2) {
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{ret[1]}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::flushall(const std::vector<std::string>&ret) {
        if (ret.size() == 1) {
            leveldb::Iterator* it = this->db->NewIterator(leveldb::ReadOptions());
            for (it->SeekToFirst(); it->Valid(); it->Next()) {
                this->db->Delete(leveldb::WriteOptions(), it->key());
            }

            if (it->status().ok()) {
                delete it;
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
            }

            delete it;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }




    // the version with memory

    void medis_server::set_lru_str_max_size(size_t len) {
        this->lru_string_max_size = len;
    }

    void medis_server::set_lru_list_max_size(size_t len) {
        this->lru_list_max_size = len;
    }

    void medis_server::set_lru_map_max_size(size_t len) {
        this->lru_map_max_size = len;
    }

    void medis_server::set_lru_set_max_size(size_t len) {
        this->lru_set_max_size = len;
    }

    void medis_server::set_lru_queue_max_size(size_t len) {
        this->lru_queue_max_size = len;
    }

    void medis_server::set_lru_stack_max_size(size_t len) {
        this->lru_stack_max_size = len;
    }

    std::string medis_server::_flushall(const std::vector<std::string>& ret) {
        if (ret.size() == 1) {
            this->string_data->clear();
            this->map_data->clear();
            this->list_data->clear();
            this->set_data->clear();
            this->stack_data->clear();
            this->queue_data->clear();
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hset(const std::vector<std::string>& ret) {
        if (ret.size() == 4) {
            shared_mongols_map m;
            if (this->map_data->contains(ret[1]) && this->map_data->tryGet(ret[1], m) && !m->expired()) {
                m->data[ret[2]] = ret[3];
            } else {
                m = std::make_shared<cache_t < mongols_map >> ();
                m->data.insert(std::make_pair(ret[2], ret[3]));
                this->map_data->insert(ret[1], m);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hget(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_map m;
            if (this->map_data->contains(ret[1]) && this->map_data->tryGet(ret[1], m) && !m->expired()) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{m->data[ret[2]]}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hdel(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            shared_mongols_map m;
            if (this->map_data->contains(ret[1]) && this->map_data->tryGet(ret[1], m) && !m->expired()) {
                size_t n = 0;
                for (size_t i = 2; i < len; ++i) {
                    n += m->data.erase(ret[i]);
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(n)}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hexists(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_map m;
            if (this->map_data->contains(ret[1]) && this->map_data->tryGet(ret[1], m) && !m->expired()) {
                if (m->data.find(ret[2]) != m->data.end()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_herase(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->map_data->contains(ret[1])) {
                this->map_data->remove(ret[1]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hgetall(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_map m;
            if (this->map_data->contains(ret[1]) && this->map_data->tryGet(ret[1], m) && !m->expired()) {
                std::vector<std::string> vs;
                for (auto& item : m->data) {
                    vs.emplace_back(item.first);
                    vs.emplace_back(item.second);
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, vs).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS,{}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hlen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_map m;
            if (this->map_data->contains(ret[1]) && this->map_data->tryGet(ret[1], m) && !m->expired()) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(m->data.size())}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hmset(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2 && (len - 2) % 2 == 0) {
            shared_mongols_map m;
            if (this->map_data->contains(ret[1]) && this->map_data->tryGet(ret[1], m) && !m->expired()) {
                for (size_t i = 2; i < len - 1; ++++i) {
                    m->data[ret[i]] = ret[i + 1];
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
            } else {
                m = std::make_shared<cache_t < mongols_map >> ();
                for (size_t i = 2; i < len - 1; ++++i) {
                    m->data[ret[i]] = ret[i + 1];
                }
                this->map_data->insert(ret[1], m);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hmget(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            shared_mongols_map m;
            if (this->map_data->contains(ret[1]) && this->map_data->tryGet(ret[1], m) && !m->expired()) {
                std::vector<std::string> vs;
                for (size_t i = 2; i < len; ++i) {
                    vs.emplace_back(m->data[ret[i]]);
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, vs).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS,{}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lfront(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_list l;
            if (this->list_data->contains(ret[1]) && this->list_data->tryGet(ret[1], l) && !l->expired()) {
                if (!l->data.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{l->data.front()}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lback(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_list l;
            if (this->list_data->contains(ret[1]) && this->list_data->tryGet(ret[1], l) && !l->expired()) {
                if (!l->data.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{l->data.back()}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lpop_front(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_list l;
            if (this->list_data->contains(ret[1]) && this->list_data->tryGet(ret[1], l) && !l->expired()) {
                if (!l->data.empty()) {
                    std::string temp = std::move(l->data.front());
                    l->data.pop_front();
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{temp}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lpop_back(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_list l;
            if (this->list_data->contains(ret[1]) && this->list_data->tryGet(ret[1], l) && !l->expired()) {
                if (!l->data.empty()) {
                    std::string temp = std::move(l->data.back());
                    l->data.pop_back();
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{temp}).response;
                }
            }

            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lpush_front(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            shared_mongols_list l;
            if (this->list_data->contains(ret[1]) && this->list_data->tryGet(ret[1], l) && !l->expired()) {
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    l->data.emplace_front(ret[j]);
                    ++i;
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;

            } else {
                l = std::make_shared<cache_t < mongols_list >> ();
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    l->data.emplace_front(ret[j]);
                    ++i;
                }
                this->list_data->insert(ret[1], l);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lpush_back(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            shared_mongols_list l;
            if (this->list_data->contains(ret[1]) && this->list_data->tryGet(ret[1], l) && !l->expired()) {
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    l->data.emplace_back(ret[j]);
                    ++i;
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;

            } else {
                l = std::make_shared<cache_t < mongols_list >> ();
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    l->data.emplace_back(ret[j]);
                    ++i;
                }
                this->list_data->insert(ret[1], l);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_llen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_list l;
            if (this->list_data->contains(ret[1]) && this->list_data->tryGet(ret[1], l) && !l->expired()) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(l->data.size())}).response;
            }

            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lrange(const std::vector<std::string>& ret) {
        if (ret.size() == 4) {
            shared_mongols_list l;
            if (this->list_data->contains(ret[1]) && this->list_data->tryGet(ret[1], l) && !l->expired()) {
                try {
                    long start = std::stol(ret[2]), count = std::stol(ret[3]), i = 0;
                    std::vector<std::string> vs;
                    if (start >= 0) {
                        for (auto& item : l->data) {
                            long tmp = i - start;
                            if (tmp >= count) {
                                break;
                            } else
                                if (tmp >= 0 && tmp < count) {
                                vs.emplace_back(item);
                            }
                            ++i;
                        }
                    } else {
                        i = -1;
                        for (mongols_list::reverse_iterator ritem = l->data.rbegin(); ritem != l->data.rend(); ++ritem) {
                            long tmp = start - i;
                            if (tmp >= count) {
                                break;
                            } else if (tmp >= 0 && tmp < count) {
                                vs.emplace_back(*ritem);
                            }
                            --i;
                        }
                    }
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, vs).response;


                } catch (std::exception&) {
                }

            }
        }

        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lerase(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_list l;
            if (this->list_data->contains(ret[1])) {
                this->list_data->remove(ret[1]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lexists(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_list l;
            if (this->list_data->contains(ret[1]) && this->list_data->tryGet(ret[1], l) && !l->expired()) {
                if (std::find(l->data.begin(), l->data.end(), ret[2]) != l->data.end()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sadd(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            shared_mongols_set s;
            if (this->set_data->contains(ret[1]) && this->set_data->tryGet(ret[1], s) && !s->expired()) {
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    auto tmp = s->data.emplace(ret[j]);
                    if (tmp.second) {
                        ++i;
                    }
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            } else {
                s = std::make_shared<cache_t < mongols_set >> ();
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    auto tmp = s->data.emplace(ret[j]);
                    if (tmp.second) {
                        ++i;
                    }
                }
                this->set_data->insert(ret[1], s);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sdel(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            shared_mongols_set s;
            if (this->set_data->contains(ret[1]) && this->set_data->tryGet(ret[1], s) && !s->expired()) {
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    i += s->data.erase(ret[j]);
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sexists(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_set s;
            if (this->set_data->contains(ret[1]) && this->set_data->tryGet(ret[1], s) && !s->expired()) {
                if (s->data.find(ret[2]) != s->data.end()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sdifference(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_set s1, s2;
            bool b1 = this->set_data->contains(ret[1]), b2 = this->set_data->contains(ret[2]);
            if (b1 && b2 && this->set_data->tryGet(ret[1], s1) && !s1->expired() && this->set_data->tryGet(ret[2], s2) && !s2->expired()) {
                std::vector<std::string> v(s1->data.size() + s2->data.size());
                auto iter = std::set_difference(s1->data.begin(), s1->data.end(), s2->data.begin(), s2->data.end(), v.begin());
                v.resize(iter - v.begin());
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, v).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sintersection(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_set s1, s2;
            bool b1 = this->set_data->contains(ret[1]), b2 = this->set_data->contains(ret[2]);
            if (b1 && b2 && this->set_data->tryGet(ret[1], s1) && !s1->expired() && this->set_data->tryGet(ret[2], s2) && !s2->expired()) {
                std::vector<std::string> v(s1->data.size() + s2->data.size());
                auto iter = std::set_intersection(s1->data.begin(), s1->data.end(), s2->data.begin(), s2->data.end(), v.begin());
                v.resize(iter - v.begin());
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, v).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_ssymmetric_difference(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_set s1, s2;
            bool b1 = this->set_data->contains(ret[1]), b2 = this->set_data->contains(ret[2]);
            if (b1 && b2 && this->set_data->tryGet(ret[1], s1) && !s1->expired() && this->set_data->tryGet(ret[2], s2) && !s2->expired()) {
                std::vector<std::string> v(s1->data.size() + s2->data.size());
                auto iter = std::set_symmetric_difference(s1->data.begin(), s1->data.end(), s2->data.begin(), s2->data.end(), v.begin());
                v.resize(iter - v.begin());
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, v).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sunion(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_set s1, s2;
            bool b1 = this->set_data->contains(ret[1]), b2 = this->set_data->contains(ret[2]);
            if (b1 && b2 && this->set_data->tryGet(ret[1], s1) && !s1->expired() && this->set_data->tryGet(ret[2], s2) && !s2->expired()) {
                std::vector<std::string> v(s1->data.size() + s2->data.size());
                auto iter = std::set_union(s1->data.begin(), s1->data.end(), s2->data.begin(), s2->data.end(), v.begin());
                v.resize(iter - v.begin());
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, v).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_slen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_set s;
            if (this->set_data->contains(ret[1]) && this->set_data->tryGet(ret[1], s) && !s->expired()) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(s->data.size())}).response;

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_smembers(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_set s;
            if (this->set_data->contains(ret[1]) && this->set_data->tryGet(ret[1], s) && !s->expired()) {
                std::vector<std::string> sv;
                for (auto& i : s->data) {
                    sv.emplace_back(i);
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, sv).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS,{}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_serase(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->set_data->contains(ret[1])) {
                this->set_data->remove(ret[1]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qfront(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_queue q;
            if (this->queue_data->contains(ret[1]) && this->queue_data->tryGet(ret[1], q) && !q->expired()) {
                if (!q->data.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{q->data.front()}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qback(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_queue q;
            if (this->queue_data->contains(ret[1]) && this->queue_data->tryGet(ret[1], q) && !q->expired()) {
                if (!q->data.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{q->data.back()}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qempty(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_queue q;
            if (this->queue_data->contains(ret[1]) && this->queue_data->tryGet(ret[1], q) && !q->expired()) {
                if (q->data.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qpop(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_queue q;
            if (this->queue_data->contains(ret[1]) && this->queue_data->tryGet(ret[1], q) && !q->expired()) {
                if (!q->data.empty()) {
                    std::string v = std::move(q->data.front());
                    q->data.pop();
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qpush(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            shared_mongols_queue q;
            if (this->queue_data->contains(ret[1]) && this->queue_data->tryGet(ret[1], q) && !q->expired()) {
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    q->data.emplace(ret[j]);
                    ++i;
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            } else {
                q = std::make_shared<cache_t < mongols_queue >> ();
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    q->data.emplace(ret[j]);
                    ++i;
                }
                this->queue_data->insert(ret[1], q);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qerase(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->queue_data->contains(ret[1])) {
                this->queue_data->remove(ret[1]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qlen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_queue q;
            if (this->queue_data->contains(ret[1]) && this->queue_data->tryGet(ret[1], q) && !q->expired()) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(q->data.size())}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_zpush(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            shared_mongols_stack z;
            if (this->stack_data->contains(ret[1]) && this->stack_data->tryGet(ret[1], z) && !z->expired()) {
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    z->data.emplace(ret[j]);
                    ++i;
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            } else {
                z = std::make_shared<cache_t < mongols_stack >> ();
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    z->data.emplace(ret[j]);
                    ++i;
                }
                this->stack_data->insert(ret[1], z);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_zempty(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_stack z;
            if (this->stack_data->contains(ret[1]) && this->stack_data->tryGet(ret[1], z) && !z->expired()) {
                if (z->data.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_zpop(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_stack z;
            if (this->stack_data->contains(ret[1]) && this->stack_data->tryGet(ret[1], z) && !z->expired()) {
                if (!z->data.empty()) {
                    std::string v = std::move(z->data.top());
                    z->data.pop();
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_ztop(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_stack z;
            if (this->stack_data->contains(ret[1]) && this->stack_data->tryGet(ret[1], z) && !z->expired()) {
                if (!z->data.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{z->data.top()}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_zerase(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->stack_data->contains(ret[1])) {
                this->stack_data->remove(ret[1]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_zlen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_stack z;
            if (this->stack_data->contains(ret[1]) && this->stack_data->tryGet(ret[1], z) && !z->expired()) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(z->data.size())}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_set(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_string v;
            if (this->string_data->contains(ret[1]) && this->string_data->tryGet(ret[1], v) && !v->expired()) {
                v->data.assign(ret[2]);
            } else {
                v = std::make_shared<cache_t < std::string >> ();
                v->data.assign(ret[2]);
                this->string_data->insert(ret[1], v);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_get(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_string v;
            if (this->string_data->contains(ret[1]) && this->string_data->tryGet(ret[1], v) && !v->expired()) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v->data}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_exists(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->string_data->contains(ret[1])) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_del(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->string_data->contains(ret[1])) {
                this->string_data->remove(ret[1]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_append(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_string v;
            if (this->string_data->contains(ret[1]) && this->string_data->tryGet(ret[1], v) && !v->expired()) {
                v->data.append(ret[2]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(v->data.size())}).response;
            } else {
                v = std::make_shared<cache_t < std::string >> ();
                v->data.assign(ret[2]);
                this->string_data->insert(ret[1], v);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(v->data.size())}).response;

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_getrange(const std::vector<std::string>& ret) {
        if (ret.size() == 4) {
            shared_mongols_string v;
            if (this->string_data->contains(ret[1]) && this->string_data->tryGet(ret[1], v) && !v->expired()) {
                try {
                    size_t start = std::stoul(ret[2]), count = std::stoul(ret[3]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v->data.substr(start, count)}).response;

                } catch (std::exception&) {

                }

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_setrange(const std::vector<std::string>& ret) {
        if (ret.size() == 4) {
            shared_mongols_string v;
            if (this->string_data->contains(ret[1]) && this->string_data->tryGet(ret[1], v) && !v->expired()) {
                try {
                    size_t start = std::stoul(ret[2]);
                    v->data.replace(start, ret[3].size(), ret[3]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(v->data.size())}).response;
                } catch (std::exception&) {
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_getset(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_string v;
            if (this->string_data->contains(ret[1]) && this->string_data->tryGet(ret[1], v) && !v->expired()) {
                std::string tmp_v = std::move(v->data);
                v->data.assign(ret[2]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{tmp_v}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_mget(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 1) {
            std::vector<std::string> vs;
            for (size_t i = 1; i < len; ++i) {
                shared_mongols_string v;
                if (this->string_data->contains(ret[i]) && this->string_data->tryGet(ret[i], v) && !v->expired()) {
                    vs.emplace_back(v->data);
                } else {
                    vs.emplace_back("nil");
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, vs).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_mset(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len >= 2 && (len - 1) % 2 == 0) {
            for (size_t i = 1; i < len - 1; ++++i) {
                shared_mongols_string v = std::make_shared<cache_t < std::string >> (ret[i + 1]);
                this->string_data->insert(ret[1], v);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_strlen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_string v;
            if (this->string_data->contains(ret[1]) && this->string_data->tryGet(ret[1], v) && !v->expired()) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(v->data.size())}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_incr(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_string v;
            if (this->string_data->contains(ret[1]) && this->string_data->tryGet(ret[1], v) && !v->expired()) {
                try {
                    long s = std::stol(v->data);
                    v->data = std::move(std::to_string(++s));
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v->data}).response;
                } catch (std::exception&) {
                }
            } else {
                v = std::make_shared<cache_t < std::string >> ("1");
                this->string_data->insert(ret[1], v);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_incrby(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_string v;
            if (this->string_data->contains(ret[1]) && this->string_data->tryGet(ret[1], v) && !v->expired()) {
                try {
                    long s = std::stol(v->data) + std::stol(ret[2]);
                    v->data = std::move(std::to_string(s));
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v->data}).response;
                } catch (std::exception&) {

                }
            } else {
                v = std::make_shared<cache_t < std::string >> (ret[2]);
                this->string_data->insert(ret[1], v);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{ret[2]}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_decr(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_string v;
            if (this->string_data->contains(ret[1]) && this->string_data->tryGet(ret[1], v) && !v->expired()) {
                try {
                    long s = std::stol(v->data);
                    v->data = std::move(std::to_string(--s));
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v->data}).response;
                } catch (std::exception&) {

                }
            } else {
                v = std::make_shared<cache_t < std::string >> ("-1");
                this->string_data->insert(ret[1], v);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_decrby(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_string v;
            if (this->string_data->contains(ret[1]) && this->string_data->tryGet(ret[1], v) && !v->expired()) {
                try {
                    long s = std::stol(v->data) - std::stol(ret[2]);
                    v->data = std::move(std::to_string(s));
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v->data}).response;
                } catch (std::exception&) {

                }
            } else {
                v = std::make_shared<cache_t < std::string >> ("-" + ret[2]);
                this->string_data->insert(ret[1], v);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v->data}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_expire(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_string v;
            if (this->string_data->contains(ret[1]) && this->string_data->tryGet(ret[1], v) && !v->expired()) {
                try {
                    v->expires = std::stoll(ret[2]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                } catch (std::exception&) {

                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
            }
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hexpire(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_map v;
            if (this->map_data->contains(ret[1]) && this->map_data->tryGet(ret[1], v) && !v->expired()) {
                try {
                    v->expires = std::stoll(ret[2]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                } catch (std::exception&) {

                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
            }
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lexpire(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_list v;
            if (this->list_data->contains(ret[1]) && this->list_data->tryGet(ret[1], v) && !v->expired()) {
                try {
                    v->expires = std::stoll(ret[2]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                } catch (std::exception&) {

                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
            }
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qexpire(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_queue v;
            if (this->queue_data->contains(ret[1]) && this->queue_data->tryGet(ret[1], v) && !v->expired()) {
                try {
                    v->expires = std::stoll(ret[2]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                } catch (std::exception&) {

                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
            }
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sexpire(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_set v;
            if (this->set_data->contains(ret[1]) && this->set_data->tryGet(ret[1], v) && !v->expired()) {
                try {
                    v->expires = std::stoll(ret[2]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                } catch (std::exception&) {

                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
            }
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_zexpire(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            shared_mongols_stack v;
            if (this->stack_data->contains(ret[1]) && this->stack_data->tryGet(ret[1], v) && !v->expired()) {
                try {
                    v->expires = std::stoll(ret[2]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                } catch (std::exception&) {

                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
            }
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_ttl(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_string v;
            if (this->string_data->contains(ret[1]) && this->string_data->tryGet(ret[1], v)) {
                long long ttl = v->ttl();
                if (ttl > 0) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(ttl)}).response;
                } else {
                    this->string_data->remove(ret[1]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-1"}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-2"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_httl(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_map v;
            if (this->map_data->contains(ret[1]) && this->map_data->tryGet(ret[1], v)) {
                long long ttl = v->ttl();
                if (ttl > 0) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(ttl)}).response;
                } else {
                    this->map_data->remove(ret[1]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-1"}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-2"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lttl(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_list v;
            if (this->list_data->contains(ret[1]) && this->list_data->tryGet(ret[1], v)) {
                long long ttl = v->ttl();
                if (ttl > 0) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(ttl)}).response;
                } else {
                    this->list_data->remove(ret[1]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-1"}).response;
                }

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-2"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qttl(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_queue v;
            if (this->queue_data->contains(ret[1]) && this->queue_data->tryGet(ret[1], v)) {
                long long ttl = v->ttl();
                if (ttl > 0) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(ttl)}).response;
                } else {
                    this->queue_data->remove(ret[1]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-1"}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-2"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sttl(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_set v;
            if (this->set_data->contains(ret[1]) && this->set_data->tryGet(ret[1], v)) {
                long long ttl = v->ttl();
                if (ttl > 0) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(ttl)}).response;
                } else {
                    this->set_data->remove(ret[1]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-1"}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-2"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_zttl(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            shared_mongols_stack v;
            if (this->stack_data->contains(ret[1]) && this->stack_data->tryGet(ret[1], v)) {
                long long ttl = v->ttl();
                if (ttl > 0) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(ttl)}).response;
                } else {
                    this->stack_data->remove(ret[1]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-1"}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-2"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::sql_cmd(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            int rc = this->sqldb->execute(ret[1].c_str());
            bool b = true;
            if (rc != SQLITE_OK && rc != SQLITE_DONE) {

                b = false;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{(b ? "1" : "0")}).response;

        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::sql_bind_cmd(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2 && (len - 2) % 2 == 0) {
            sqlite3pp::command cmd(*this->sqldb, ret[1].c_str());
            for (size_t i = 2; i < len - 1; ++++i) {
                cmd.bind(ret[i].c_str(), ret[i + 1].c_str(), sqlite3pp::nocopy);
            }
            bool b = (cmd.execute_all() == SQLITE_OK);

            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{(b ? "1" : "0")}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::sql_transaction(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            try {
                {
                    sqlite3pp::transaction xct(*this->sqldb);
                    {
                        sqlite3pp::command cmd(*this->sqldb, ret[1].c_str());
                        cmd.execute_all();
                    }
                    xct.commit();
                }

            } catch (std::exception& e) {

            }

            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;

        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::sql_query(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::vector<std::string> v;
            try {
                sqlite3pp::query qry(*this->sqldb, ret[1].c_str());
                for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                    for (int j = 0; j < qry.column_count(); ++j) {
                        v.emplace_back(qry.column_name(j));
                        switch ((*i).column_type(j)) {
                            case SQLITE_NULL:
                                v.emplace_back("");
                                break;
                            default:
                                v.emplace_back((*i).get<std::string>(j));
                                break;
                        }
                    }
                }
            } catch (std::exception& e) {

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, v).response;

        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::lua_content(const std::vector<std::string>& ret) {
        if (ret.size() >= 2) {
            kaguya::LuaTable medis_tbl = this->vm["medis"];
            medis_tbl["ARGS"] = ret;
            medis_tbl["RESULT"] = nullptr;
            if (this->vm.dostring(ret[1])) {
                kaguya::LuaRef result = medis_tbl["RESULT"];
                int result_t = result.type();
                if (result_t == LUA_TSTRING) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{result}).response;
                } else if (result_t == LUA_TNUMBER) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{result}).response;
                } else if (result_t == LUA_TBOOLEAN) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{(result.get<bool>() ? "1" : "0")}).response;
                } else if (result_t == LUA_TTABLE) {
                    std::vector<std::string> vs;
                    if (result.isConvertible<std::map < std::string, std::string >> ()) {
                        std::map<std::string, std::string> m = result;
                        for (auto& i : m) {
                            vs.emplace_back(i.first);
                            vs.emplace_back(i.second);
                        }
                    }
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, vs).response;
                } else {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"FAILED"}).response;

        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::lua_script(const std::vector<std::string>& ret) {
        if (ret.size() >= 2) {
            kaguya::LuaTable medis_tbl = this->vm["medis"];
            medis_tbl["ARGS"] = ret;
            medis_tbl["RESULT"] = nullptr;
            if (this->vm.dofile(ret[1])) {
                kaguya::LuaRef result = medis_tbl["RESULT"];
                int result_t = result.type();
                if (result_t == LUA_TSTRING) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{result}).response;
                } else if (result_t == LUA_TNUMBER) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{result}).response;
                } else if (result_t == LUA_TBOOLEAN) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{(result.get<bool>() ? "1" : "0")}).response;
                } else if (result_t == LUA_TTABLE) {
                    std::vector<std::string> vs;
                    if (result.isConvertible<std::map < std::string, std::string >> ()) {
                        std::map<std::string, std::string> m = result;
                        for (auto& i : m) {
                            vs.emplace_back(i.first);
                            vs.emplace_back(i.second);
                        }
                    }
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, vs).response;
                } else {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
                }
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"FAILED"}).response;

        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    template<class value_t>
    medis_server::cache_t<value_t>::cache_t() : data(), t(time(0)), expires(-1) {
    }

    template<class value_t>
    medis_server::cache_t<value_t>::cache_t(const value_t& v) : data(v), t(time(0)), expires(-1) {
    }

    template<class value_t>
    bool medis_server::cache_t<value_t>::expired() const {
        return difftime(time(0), this->t) > (this->expires > 0 ? this->expires : 300);
    }

    template<class value_t>
    long long medis_server::cache_t<value_t>::ttl() const {
        return (this->expires > 0 ? this->expires : 300)-difftime(time(0), this->t);
    }























} // namespace mongols