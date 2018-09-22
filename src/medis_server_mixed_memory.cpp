#include <iostream>
#include "medis_server_mixed_memory.hpp"
#include "lib/json/json.h"



namespace mongols {

    medis_server_mixed_memory::medis_server_mixed_memory(const std::string& host, int port
            , int timeout
            , size_t buffer_size
            , size_t thread_size
            , int max_event_size)
    : medis_server(host, port, timeout, buffer_size, thread_size, max_event_size)
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
        this->op["FLUSHALL"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::flushall);
        this->op["HGET"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::hget);
        this->op["HSET"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::hset);
        this->op["HDEL"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::hdel);
        this->op["HEXISTS"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::hexists);

        this->op["LFRONT"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::lfront);
        this->op["LBACK"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::lback);
        this->op["LPUSH_FRONT"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::lpush_front);
        this->op["LPUSH"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::lpush_front);
        this->op["LPUSH_BACK"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::lpush_back);
        this->op["LPOP_FRONT"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::lpop_front);
        this->op["LPOP"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::lpop_front);
        this->op["LPOP_BACK"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::lpop_back);
        this->op["LLEN"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::llen);

        this->op["SADD"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::sadd);
        this->op["SDEL"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::sdel);
        this->op["SREM"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::sdel);
        this->op["SMEMBERS"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::smembers);
        this->op["SCARD"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::scard);
        this->op["SEXISTS"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::sexists);
        this->op["SDIFF"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::sdifference);
        this->op["SINTER"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::sintersection);
        this->op["SUNION"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::sunion);
        this->op["SSYDIFF"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::ssymmetric_difference);

        this->op["QPUSH"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::qpush);
        this->op["QPOP"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::qpop);
        this->op["QFRONT"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::qfront);
        this->op["QBACK"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::qback);
        this->op["QEMPTY"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::qempty);

        this->op["ZPOP"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::zpop);
        this->op["ZTOP"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::ztop);
        this->op["ZPUSH"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::zpush);
        this->op["ZEMPTY"] = reinterpret_cast<op_funcion> (&medis_server_mixed_memory::zempty);

    }

    medis_server_mixed_memory::~medis_server_mixed_memory() {
        if (this->sr)delete this->sr;
        if (this->lt)delete this->lt;
        if (this->mp)delete this->mp;
        if (this->st)delete this->st;
        if (this->sk)delete this->sk;
        if (this->qe)delete this->qe;
    }

    void medis_server_mixed_memory::set_lru_str_max_size(size_t len) {
        this->lru_str_max_size = len;
    }

    void medis_server_mixed_memory::set_lru_list_max_size(size_t len) {
        this->lru_list_max_size = len;
    }

    void medis_server_mixed_memory::set_lru_map_max_size(size_t len) {
        this->lru_map_max_size = len;
    }

    void medis_server_mixed_memory::set_lru_set_max_size(size_t len) {
        this->lru_set_max_size = len;
    }

    void medis_server_mixed_memory::set_lru_queue_max_size(size_t len) {
        this->lru_queue_max_size = len;
    }

    void medis_server_mixed_memory::set_lru_stack_max_size(size_t len) {
        this->lru_stack_max_size = len;
    }

    void medis_server_mixed_memory::ready() {
        if (!this->sr)this->sr = new mongols::cache::lru_cache<std::string, size_t>(this->lru_str_max_size);
        if (!this->lt)this->lt = new mongols::cache::lru_cache<std::string, size_t>(this->lru_list_max_size);
        if (!this->mp)this->mp = new mongols::cache::lru_cache<std::string, size_t>(this->lru_map_max_size);
        if (!this->st)this->st = new mongols::cache::lru_cache<std::string, size_t>(this->lru_set_max_size);
        if (!this->sk)this->sk = new mongols::cache::lru_cache<std::string, size_t>(this->lru_stack_max_size);
        if (!this->qe)this->qe = new mongols::cache::lru_cache<std::string, size_t>(this->lru_queue_max_size);
    }

    std::string medis_server_mixed_memory::flushall(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::hset(const std::vector<std::string>& ret) {
        if (ret.size() == 4) {
            this->mp->put(ret[1], 0);
            this->mp_data[ret[1]][ret[2]] = ret[3];
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server_mixed_memory::hget(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::hdel(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::hexists(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::lfront(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::lback(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::lpop_front(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::lpop_back(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::lpush_front(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::lpush_back(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::llen(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::sadd(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            std::string v;
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

    std::string medis_server_mixed_memory::sdel(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
            std::string v;
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

    std::string medis_server_mixed_memory::sexists(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::sdifference(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::sintersection(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::ssymmetric_difference(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::sunion(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::scard(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
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

    std::string medis_server_mixed_memory::smembers(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::qfront(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::qback(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::qempty(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::qpop(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::qpush(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::zpush(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::zempty(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::zpop(const std::vector<std::string>& ret) {
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

    std::string medis_server_mixed_memory::ztop(const std::vector<std::string>& ret) {
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






















}