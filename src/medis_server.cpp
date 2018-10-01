
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
#include "util.hpp"


namespace mongols {

    medis_server::medis_server(const std::string& host
            , int port
            , int timeout
            , size_t buffer_size
            , size_t thread_size
            , int max_event_size)
    : server(0), db(0), options(), sqldb(0), vm(), resp_decoder(), resp_encoder(), op()
    , lru_str_max_size(1024)
    , lru_list_max_size(1024)
    , lru_map_max_size(1024)
    , lru_set_max_size(1024)
    , lru_queue_max_size(1024)
    , lru_stack_max_size(1024)
    , sr(0)
    , lt(0)
    , mp(0)
    , st(0)
    , qe(0)
    , sk(0)
    , sr_data()
    , lt_data()
    , mp_data()
    , st_data()
    , qe_data()
    , sk_data() {
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

        this->op["_QPUSH"] = &medis_server::_qpush;
        this->op["_QPOP"] = &medis_server::_qpop;
        this->op["_QFRONT"] = &medis_server::_qfront;
        this->op["_QBACK"] = &medis_server::_qback;
        this->op["_QEMPTY"] = &medis_server::_qempty;
        this->op["_QERASE"] = &medis_server::_qerase;
        this->op["_QLEN"] = &medis_server::_qlen;


        this->op["_ZPOP"] = &medis_server::_zpop;
        this->op["_ZTOP"] = &medis_server::_ztop;
        this->op["_ZPUSH"] = &medis_server::_zpush;
        this->op["_ZEMPTY"] = &medis_server::_zempty;
        this->op["_ZERASE"] = &medis_server::_zerase;
        this->op["_ZLEN"] = &medis_server::_zlen;


        this->op["_GET"] = &medis_server::_get;
        this->op["_SET"] = &medis_server::_set;
        this->op["_DEL"] = &medis_server::_del;
        this->op["_EXISTS"] = &medis_server::_exists;

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
        if (this->sr)delete this->sr;
        if (this->lt)delete this->lt;
        if (this->mp)delete this->mp;
        if (this->st)delete this->st;
        if (this->sk)delete this->sk;
        if (this->qe)delete this->qe;

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
        this->lru_str_max_size = len;
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

    void medis_server::ready() {
        if (!this->sr)this->sr = new mongols::cache::lru_cache<std::string, size_t>(this->lru_str_max_size);
        if (!this->lt)this->lt = new mongols::cache::lru_cache<std::string, size_t>(this->lru_list_max_size);
        if (!this->mp)this->mp = new mongols::cache::lru_cache<std::string, size_t>(this->lru_map_max_size);
        if (!this->st)this->st = new mongols::cache::lru_cache<std::string, size_t>(this->lru_set_max_size);
        if (!this->sk)this->sk = new mongols::cache::lru_cache<std::string, size_t>(this->lru_stack_max_size);
        if (!this->qe)this->qe = new mongols::cache::lru_cache<std::string, size_t>(this->lru_queue_max_size);
    }

    std::string medis_server::_flushall(const std::vector<std::string>& ret) {
        if (ret.size() == 1) {
            this->sr->clear();
            this->sr_data.clear();
            this->mp->clear();
            this->mp_data.clear();
            this->lt->clear();
            this->lt_data.clear();
            this->st->clear();
            this->st_data.clear();
            this->sk->clear();
            this->sk_data.clear();
            this->qe->clear();
            this->qe_data.clear();
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hset(const std::vector<std::string>& ret) {
        if (ret.size() == 4) {
            this->mp->put(ret[1], 0);
            this->mp_data[ret[1]][ret[2]] = ret[3];
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hget(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            if (this->mp->exists(ret[1])) {
                mongols_map& v = this->mp_data[ret[1]];
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v[ret[2]]}).response;
            } else if (this->mp_data.find(ret[1]) != this->mp_data.end()) {
                this->mp_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hdel(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            if (this->mp->exists(ret[1])) {
                size_t n = 0;
                mongols_map& v = this->mp_data[ret[1]];
                for (size_t i = 2; i < len; ++i) {
                    n += v.erase(ret[i]);
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(n)}).response;
            } else if (this->mp_data.find(ret[1]) != this->mp_data.end()) {
                this->mp_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hexists(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            if (this->mp->exists(ret[1])) {
                mongols_map& v = this->mp_data[ret[1]];
                if (v.find(ret[2]) != v.end()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                }
            } else if (this->mp_data.find(ret[1]) != this->mp_data.end()) {
                this->mp_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_herase(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->mp->exists(ret[1])) {
                this->mp->erase(ret[1]);
                this->mp_data.erase(ret[1]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            } else if (this->mp_data.find(ret[1]) != this->mp_data.end()) {
                this->mp_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hgetall(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->mp->exists(ret[1])) {
                std::vector<std::string> vs;
                mongols_map &m = this->mp_data[ret[1]];
                for (auto& item : m) {
                    vs.emplace_back(item.first);
                    vs.emplace_back(item.second);
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, vs).response;
            } else if (this->mp_data.find(ret[1]) != this->mp_data.end()) {
                this->mp_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS,{}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hlen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->mp->exists(ret[1])) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(this->mp_data[ret[1]].size())}).response;
            } else if (this->mp_data.find(ret[1]) != this->mp_data.end()) {
                this->mp_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hmset(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2 && (len - 2) % 2 == 0) {
            if (this->mp->exists(ret[1])) {
                mongols_map &m = this->mp_data[ret[1]];
                for (size_t i = 2; i < len - 1; ++++i) {
                    m[ret[i]] = ret[i + 1];
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
            } else {
                mongols_map m;
                for (size_t i = 2; i < len - 1; ++++i) {
                    m[ret[i]] = ret[i + 1];
                }
                this->mp->put(ret[1], 0);
                this->mp_data[ret[1]] = std::move(m);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_hmget(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            if (this->mp->exists(ret[1])) {
                mongols_map &m = this->mp_data[ret[1]];
                std::vector<std::string> vs;
                for (size_t i = 2; i < len; ++i) {
                    vs.emplace_back(m[ret[i]]);
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, vs).response;
            } else if (this->mp_data.find(ret[1]) != this->mp_data.end()) {
                this->mp_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS,{}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lfront(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->lt->exists(ret[1])) {
                mongols_list& v = this->lt_data[ret[1]];
                if (!v.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v.front()}).response;
                }
            } else if (this->lt_data.find(ret[1]) != this->lt_data.end()) {

                this->lt_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lback(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->lt->exists(ret[1])) {
                mongols_list& v = this->lt_data[ret[1]];
                if (!v.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v.back()}).response;
                }
            } else if (this->lt_data.find(ret[1]) != this->lt_data.end()) {

                this->lt_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lpop_front(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->lt->exists(ret[1])) {
                mongols_list& v = this->lt_data[ret[1]];
                if (!v.empty()) {
                    std::string temp = std::move(v.front());
                    v.pop_front();
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{temp}).response;
                }
            } else if (this->lt_data.find(ret[1]) != this->lt_data.end()) {

                this->lt_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lpop_back(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->lt->exists(ret[1])) {
                mongols_list& v = this->lt_data[ret[1]];
                if (!v.empty()) {
                    std::string temp = std::move(v.back());
                    v.pop_back();
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{temp}).response;
                }
            } else if (this->lt_data.find(ret[1]) != this->lt_data.end()) {

                this->lt_data.erase(ret[1]);
            }

            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lpush_front(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            if (this->lt->exists(ret[1])) {
                mongols_list& v = this->lt_data[ret[1]];
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    v.emplace_front(ret[j]);
                    ++i;
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;

            } else {
                mongols_list v;
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    v.emplace_front(ret[j]);
                    ++i;
                }
                this->lt->put(ret[1], 0);
                this->lt_data[ret[1]] = std::move(v);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lpush_back(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            if (this->lt->exists(ret[1])) {
                mongols_list& v = this->lt_data[ret[1]];
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    v.emplace_back(ret[j]);
                    ++i;
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;

            } else {
                mongols_list v;
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    v.emplace_back(ret[j]);
                    ++i;
                }
                this->lt->put(ret[1], 0);
                this->lt_data[ret[1]] = std::move(v);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_llen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->lt->exists(ret[1])) {
                mongols_list& v = this->lt_data[ret[1]];
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(v.size())}).response;
            } else if (this->lt_data.find(ret[1]) != this->lt_data.end()) {

                this->lt_data.erase(ret[1]);
            }

            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lrange(const std::vector<std::string>& ret) {
        if (ret.size() == 4) {
            if (this->lt->exists(ret[1])) {
                try {
                    long start = std::stol(ret[2]), count = std::stol(ret[3]), i = 0;
                    mongols_list &l = this->lt_data[ret[1]];
                    std::vector<std::string> vs;
                    if (start >= 0) {
                        for (auto& item : l) {
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


                } catch (std::exception&) {
                }

            }
        }

        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lerase(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->lt->exists(ret[1])) {
                this->lt->erase(ret[1]);
                this->lt_data.erase(ret[1]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            } else if (this->lt_data.find(ret[1]) != this->lt_data.end()) {

                this->lt_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_lexists(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            if (this->lt->exists(ret[1])) {
                mongols_list &l = this->lt_data[ret[1]];
                if (std::find(l.begin(), l.end(), ret[2]) != l.end()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                }
            } else if (this->lt_data.find(ret[1]) != this->lt_data.end()) {

                this->lt_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sadd(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            if (this->st->exists(ret[1])) {
                mongols_set& s = this->st_data[ret[1]];
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    auto tmp = s.emplace(ret[j]);
                    if (tmp.second) {
                        ++i;
                    }
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            } else {
                mongols_set s;
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    auto tmp = s.emplace(ret[j]);
                    if (tmp.second) {
                        ++i;
                    }
                }
                this->st->put(ret[1], 0);
                this->st_data[ret[1]] = std::move(s);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sdel(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            if (this->st->exists(ret[1])) {
                mongols_set& s = this->st_data[ret[1]];
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    i += s.erase(ret[j]);
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            } else if (this->st_data.find(ret[1]) != this->st_data.end()) {

                this->st_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sexists(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            if (this->st->exists(ret[1])) {
                mongols_set &s = this->st_data[ret[1]];
                if (s.find(ret[2]) != s.end()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                }
            } else if (this->st_data.find(ret[1]) != this->st_data.end()) {

                this->st_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sdifference(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {

            bool b1 = this->st->exists(ret[1]), b2 = this->st->exists(ret[2]);
            if (b1 && b2) {
                mongols_set& s1 = this->st_data[ret[1]], &s2 = this->st_data[ret[2]];
                std::vector<std::string> v(s1.size() + s2.size());
                auto iter = std::set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), v.begin());
                v.resize(iter - v.begin());
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, v).response;
            } else if (!b1 && this->st_data.find(ret[1]) != this->st_data.end()) {
                this->st_data.erase(ret[1]);
            } else if (!b2 && this->st_data.find(ret[2]) != this->st_data.end()) {

                this->st_data.erase(ret[2]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sintersection(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            bool b1 = this->st->exists(ret[1]), b2 = this->st->exists(ret[2]);
            if (b1 && b2) {
                mongols_set& s1 = this->st_data[ret[1]], &s2 = this->st_data[ret[2]];
                std::vector<std::string> v(s1.size() + s2.size());
                auto iter = std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), v.begin());
                v.resize(iter - v.begin());
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, v).response;
            } else if (!b1 && this->st_data.find(ret[1]) != this->st_data.end()) {
                this->st_data.erase(ret[1]);
            } else if (!b2 && this->st_data.find(ret[2]) != this->st_data.end()) {

                this->st_data.erase(ret[2]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_ssymmetric_difference(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            bool b1 = this->st->exists(ret[1]), b2 = this->st->exists(ret[2]);
            if (b1 && b2) {
                mongols_set& s1 = this->st_data[ret[1]], &s2 = this->st_data[ret[2]];
                std::vector<std::string> v(s1.size() + s2.size());
                auto iter = std::set_symmetric_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), v.begin());
                v.resize(iter - v.begin());
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, v).response;
            } else if (!b1 && this->st_data.find(ret[1]) != this->st_data.end()) {
                this->st_data.erase(ret[1]);
            } else if (!b2 && this->st_data.find(ret[2]) != this->st_data.end()) {

                this->st_data.erase(ret[2]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_sunion(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            bool b1 = this->st->exists(ret[1]), b2 = this->st->exists(ret[2]);
            if (b1 && b2) {
                mongols_set& s1 = this->st_data[ret[1]], &s2 = this->st_data[ret[2]];
                std::vector<std::string> v(s1.size() + s2.size());
                auto iter = std::set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), v.begin());
                v.resize(iter - v.begin());
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, v).response;
            } else if (!b1 && this->st_data.find(ret[1]) != this->st_data.end()) {
                this->st_data.erase(ret[1]);
            } else if (!b2 && this->st_data.find(ret[2]) != this->st_data.end()) {

                this->st_data.erase(ret[2]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_slen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->st->exists(ret[1])) {
                mongols_set &s = this->st_data[ret[1]];
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(s.size())}).response;

            } else if (this->st_data.find(ret[1]) != this->st_data.end()) {
                this->st_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_smembers(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->st->exists(ret[1])) {
                mongols_set &s = this->st_data[ret[1]];
                std::vector<std::string> sv;
                for (auto& i : s) {
                    sv.emplace_back(i);
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS, sv).response;
            } else if (this->st_data.find(ret[1]) != this->st_data.end()) {

                this->st_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ARRAYS,{}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_serase(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->st->exists(ret[1])) {
                this->st->erase(ret[1]);
                this->st_data.erase(ret[1]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            } else if (this->st_data.find(ret[1]) != this->st_data.end()) {

                this->st_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qfront(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->qe->exists(ret[1])) {
                mongols_queue& q = this->qe_data[ret[1]];
                if (!q.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{q.front()}).response;
                }
            } else if (this->qe_data.find(ret[1]) != this->qe_data.end()) {

                this->qe_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qback(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->qe->exists(ret[1])) {
                mongols_queue& q = this->qe_data[ret[1]];
                if (!q.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{q.back()}).response;
                }
            } else if (this->qe_data.find(ret[1]) != this->qe_data.end()) {

                this->qe_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qempty(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->qe->exists(ret[1])) {
                mongols_queue& q = this->qe_data[ret[1]];
                if (q.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                }
            } else if (this->qe_data.find(ret[1]) != this->qe_data.end()) {

                this->qe_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qpop(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->qe->exists(ret[1])) {
                mongols_queue& q = this->qe_data[ret[1]];
                if (!q.empty()) {
                    std::string v = std::move(q.front());
                    q.pop();
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v}).response;
                }
            } else if (this->qe_data.find(ret[1]) != this->qe_data.end()) {

                this->qe_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qpush(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            if (this->qe->exists(ret[1])) {
                mongols_queue& q = this->qe_data[ret[1]];
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    q.emplace(ret[j]);
                    ++i;
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            } else {
                mongols_queue q;
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    q.emplace(ret[j]);
                    ++i;
                }
                this->qe->put(ret[1], 0);
                this->qe_data[ret[1]] = std::move(q);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qerase(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->qe->exists(ret[1])) {
                this->qe->erase(ret[1]);
                this->qe_data.erase(ret[1]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            } else if (this->qe_data.find(ret[1]) != this->qe_data.end()) {

                this->qe_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_qlen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->qe->exists(ret[1])) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(this->qe_data[ret[1]].size())}).response;
            } else if (this->qe_data.find(ret[1]) != this->qe_data.end()) {
                this->qe_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_zpush(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            if (this->sk->exists(ret[1])) {
                mongols_stack& z = this->sk_data[ret[1]];
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    z.emplace(ret[j]);
                    ++i;
                }
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            } else {
                mongols_stack z;
                size_t i = 0;
                for (size_t j = 2; j < len; ++j) {
                    z.emplace(ret[j]);
                    ++i;
                }
                this->sk->put(ret[1], 0);
                this->sk_data[ret[1]] = std::move(z);

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(i)}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_zempty(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->sk->exists(ret[1])) {
                mongols_stack& z = this->sk_data[ret[1]];
                if (z.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
                }
            } else if (this->sk_data.find(ret[1]) != this->sk_data.end()) {

                this->sk_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_zpop(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->sk->exists(ret[1])) {
                mongols_stack& z = this->sk_data[ret[1]];
                if (!z.empty()) {
                    std::string v = std::move(z.top());
                    z.pop();
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v}).response;
                }
            } else if (this->sk_data.find(ret[1]) != this->sk_data.end()) {

                this->sk_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_ztop(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->sk->exists(ret[1])) {
                mongols_stack& z = this->sk_data[ret[1]];
                if (!z.empty()) {
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{z.top()}).response;
                }
            } else if (this->sk_data.find(ret[1]) != this->sk_data.end()) {

                this->sk_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_zerase(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->sk->exists(ret[1])) {
                this->sk->erase(ret[1]);
                this->sk_data.erase(ret[1]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            } else if (this->sk_data.find(ret[1]) != this->sk_data.end()) {

                this->sk_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_zlen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->sk->exists(ret[1])) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(this->sk_data[ret[1]].size())}).response;
            } else if (this->sk_data.find(ret[1]) != this->sk_data.end()) {
                this->sk_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_set(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            this->sr->put(ret[1], 0);
            this->sr_data[ret[1]] = ret[2];

            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_get(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->sr->exists(ret[1])) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{this->sr_data[ret[1]]}).response;
            } else if (this->sr_data.find(ret[1]) != this->sr_data.end()) {

                this->sr_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_exists(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->sr->exists(ret[1])) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            } else if (this->sr_data.find(ret[1]) != this->sr_data.end()) {

                this->sr_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_del(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->sr->exists(ret[1])) {
                this->sr->erase(ret[1]);
                this->sr_data.erase(ret[1]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            } else if (this->sr_data.find(ret[1]) != this->sr_data.end()) {

                this->sr_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_append(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            if (this->sr->exists(ret[1])) {
                std::string &v = this->sr_data[ret[1]];
                v.append(ret[2]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(v.size())}).response;
            } else {
                this->sr->put(ret[1], 0);
                this->sr_data[ret[1]] = ret[2];

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(ret[2].size())}).response;

            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_getrange(const std::vector<std::string>& ret) {
        if (ret.size() == 4) {
            if (this->sr->exists(ret[1])) {
                try {
                    std::string& v = this->sr_data[ret[1]];
                    size_t start = std::stoul(ret[2]), count = std::stoul(ret[3]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v.substr(start, count)}).response;

                } catch (std::exception&) {

                }

            } else if (this->sr_data.find(ret[1]) != this->sr_data.end()) {
                this->sr_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_setrange(const std::vector<std::string>& ret) {
        if (ret.size() == 4) {
            if (this->sr->exists(ret[1])) {
                try {
                    std::string& v = this->sr_data[ret[1]];
                    size_t start = std::stoul(ret[2]);
                    v.replace(start, ret[3].size(), ret[3]);
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(v.size())}).response;
                } catch (std::exception&) {
                }
            } else if (this->sr_data.find(ret[1]) != this->sr_data.end()) {
                this->sr_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"nil"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_getset(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v;
            if (this->sr->exists(ret[1])) {
                std::string v = std::move(this->sr_data[ret[1]]);
                this->sr_data[ret[1]] = ret[2];
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v}).response;
            } else if (this->sr_data.find(ret[1]) != this->sr_data.end()) {

                this->sr_data.erase(ret[1]);
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
                if (this->sr->exists(ret[i])) {
                    vs.emplace_back(this->sr_data[ret[i]]);
                } else {
                    if (this->sr_data.find(ret[1]) != this->sr_data.end()) {

                        this->sr_data.erase(ret[1]);
                    }
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

                this->sr->put(ret[i], 0);
                this->sr_data[ret[i]] = ret[i + 1];
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_strlen(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->sr->exists(ret[1])) {
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{std::to_string(this->sr_data[ret[1]].size())}).response;
            } else if (this->sr_data.find(ret[1]) != this->sr_data.end()) {

                this->sr_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_incr(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->sr->exists(ret[1])) {
                try {
                    std::string &v = this->sr_data[ret[1]];
                    long s = std::stol(v);
                    v = std::move(std::to_string(++s));
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
                } catch (std::exception&) {
                }
            } else {
                this->sr->put(ret[1], 0);
                this->sr_data[ret[1]] = std::move("1");

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_incrby(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v;
            if (this->sr->exists(ret[1])) {
                try {
                    std::string &v = this->sr_data[ret[1]];
                    long s = std::stol(v) + std::stol(ret[2]);
                    v = std::move(std::to_string(s));
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
                } catch (std::exception&) {

                }
            } else {
                this->sr->put(ret[1], 0);
                this->sr_data[ret[1]] = ret[2];

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{ret[2]}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_decr(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->sr->exists(ret[1])) {
                try {
                    std::string &v = this->sr_data[ret[1]];
                    long s = std::stol(v);
                    v = std::move(std::to_string(--s));
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
                } catch (std::exception&) {

                }
            } else {
                this->sr->put(ret[1], 0);
                this->sr_data[ret[1]] = std::move("-1");

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server::_decrby(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v;
            if (this->sr->exists(ret[1])) {
                try {
                    std::string &v = this->sr_data[ret[1]];
                    long s = std::stol(v) - std::stol(ret[2]);
                    v = std::move(std::to_string(s));
                    return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
                } catch (std::exception&) {

                }
            } else {
                std::string v = std::move("-" + ret[2]);
                this->sr->put(ret[1], 0);
                this->sr_data[ret[1]] = v;

                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
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





















} // namespace mongols