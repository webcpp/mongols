
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

namespace mongols {

    medis_server::medis_server(const std::string& host
            , int port
            , int timeout
            , size_t buffer_size
            , size_t thread_size
            , int max_event_size)
    : server(0), db(0), options(), resp_decoder(), resp_encoder(), op() {
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

        this->op["HGET"] = &medis_server::hget;
        this->op["HSET"] = &medis_server::hset;
        this->op["HDEL"] = &medis_server::hdel;
        this->op["HEXISTS"] = &medis_server::hexists;


        this->op["LFRONT"] = &medis_server::lfront;
        this->op["LBACK"] = &medis_server::lback;
        this->op["LPUSH_FRONT"] = &medis_server::lpush_front;
        this->op["LPUSH"] = &medis_server::lpush_front;
        this->op["LPUSH_BACK"] = &medis_server::lpush_back;
        this->op["RPUSH"] = &medis_server::lpush_back;
        this->op["LPOP_FRONT"] = &medis_server::lpop_front;
        this->op["LPOP"] = &medis_server::lpop_front;
        this->op["LPOP_BACK"] = &medis_server::lpop_back;
        this->op["RPOP"] = &medis_server::lpop_back;
        this->op["LLEN"] = &medis_server::llen;


        this->op["SADD"] = &medis_server::sadd;
        this->op["SDEL"] = &medis_server::sdel;
        this->op["SMEMBERS"] = &medis_server::smembers;
        this->op["SCARD"] = &medis_server::scard;
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


    }

    medis_server::~medis_server() {
        if (this->db) {
            delete this->db;
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

    void medis_server::run(const std::string& path) {
        this->options.create_if_missing = true;
        if (leveldb::DB::Open(this->options, path, &this->db).ok()) {

            this->server->run(std::bind(&medis_server::work, this
                    , std::placeholders::_1
                    , std::placeholders::_2
                    , std::placeholders::_3
                    , std::placeholders::_4
                    , std::placeholders::_5));

        }
    }

    std::string medis_server::work(const std::pair<char*, size_t>& input
            , bool& keepalive
            , bool& send_to_other
            , tcp_server::client_t& client
            , tcp_server::filter_handler_function& f) {
        send_to_other = false;
        keepalive = CLOSE_CONNECTION;
        simple_resp::decode_result de_ret = std::move(this->resp_decoder.decode(std::string(input.first, input.second)));

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
        if (len > 2) {
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
        if (len > 2 && (len - 1) % 2 == 0) {
            for (size_t i = 1; i < len - 1; ++i) {
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
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
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
                    std::string v = std::move(l.front());
                    l.pop_front();
                    if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(l)).ok()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v}).response;
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
                    std::string v = std::move(l.back());
                    l.pop_back();
                    if (this->db->Put(leveldb::WriteOptions(), ret[1], this->serialize(l)).ok()) {
                        return this->resp_encoder.encode(simple_resp::RESP_TYPE::BULK_STRINGS,{v}).response;
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

    std::string medis_server::scard(const std::vector<std::string>& ret) {
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






















} // namespace mongols