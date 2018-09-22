
#include "medis_server_with_memory.hpp"



namespace mongols {

    medis_server_with_memory::medis_server_with_memory(const std::string& host, int port
            , int timeout
            , size_t buffer_size
            , size_t thread_size
            , int max_event_size)
    : medis_server_mixed_memory(host, port, timeout, buffer_size, thread_size, max_event_size) {
        this->op["GET"] = reinterpret_cast<op_funcion> (&medis_server_with_memory::get);
        this->op["SET"] = reinterpret_cast<op_funcion> (&medis_server_with_memory::set);
        this->op["DEL"] = reinterpret_cast<op_funcion> (&medis_server_with_memory::del);
        this->op["EXISTS"] = reinterpret_cast<op_funcion> (&medis_server_with_memory::exists);

        this->op["MGET"] = reinterpret_cast<op_funcion> (&medis_server_with_memory::mget);
        this->op["MSET"] = reinterpret_cast<op_funcion> (&medis_server_with_memory::mset);
        this->op["GETSET"] = reinterpret_cast<op_funcion> (&medis_server_with_memory::getset);
        this->op["STRLEN"] = reinterpret_cast<op_funcion> (&medis_server_with_memory::strlen);
        this->op["APPEND"] = reinterpret_cast<op_funcion> (&medis_server_with_memory::append);

        this->op["INCR"] = reinterpret_cast<op_funcion> (&medis_server_with_memory::incr);
        this->op["INCRBY"] = reinterpret_cast<op_funcion> (&medis_server_with_memory::incrby);
        this->op["DECR"] = reinterpret_cast<op_funcion> (&medis_server_with_memory::decr);
        this->op["DECRBY"] = reinterpret_cast<op_funcion> (&medis_server_with_memory::decrby);

    }

    std::string medis_server_with_memory::set(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            this->sr->put(ret[1], 0);
            this->sr_data[ret[1]] = ret[2];
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server_with_memory::get(const std::vector<std::string>& ret) {
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

    std::string medis_server_with_memory::exists(const std::vector<std::string>& ret) {
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

    std::string medis_server_with_memory::del(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            if (this->sr->exists(ret[1])) {
                this->sr->erase(ret[1]);
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            } else if (this->sr_data.find(ret[1]) != this->sr_data.end()) {
                this->sr_data.erase(ret[1]);
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"0"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server_with_memory::append(const std::vector<std::string>& ret) {
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

    std::string medis_server_with_memory::getset(const std::vector<std::string>& ret) {
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

    std::string medis_server_with_memory::mget(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2) {
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

    std::string medis_server_with_memory::mset(const std::vector<std::string>& ret) {
        size_t len = ret.size();
        if (len > 2 && (len - 1) % 2 == 0) {
            for (size_t i = 1; i < len - 1; ++i) {
                this->sr->put(ret[i], 0);
                this->sr_data[ret[i]] = ret[i + 1];
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::SIMPLE_STRINGS,{"OK"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server_with_memory::strlen(const std::vector<std::string>& ret) {
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

    std::string medis_server_with_memory::incr(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->sr->exists(ret[1])) {
                std::string &v = this->sr_data[ret[1]];
                long s = std::stol(v);
                v = std::move(std::to_string(++s));
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
            } else {
                this->sr->put(ret[1], 0);
                this->sr_data[ret[1]] = std::move("1");
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server_with_memory::incrby(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v;
            if (this->sr->exists(ret[1])) {
                std::string &v = this->sr_data[ret[1]];

                long s = std::stol(v) + std::stol(ret[2]);
                v = std::move(std::to_string(s));
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
            } else {
                this->sr->put(ret[1], 0);
                this->sr_data[ret[1]] = ret[2];
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{ret[2]}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server_with_memory::decr(const std::vector<std::string>& ret) {
        if (ret.size() == 2) {
            std::string v;
            if (this->sr->exists(ret[1])) {
                std::string &v = this->sr_data[ret[1]];
                long s = std::stol(v);
                v = std::move(std::to_string(--s));
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
            } else {
                this->sr->put(ret[1], 0);
                this->sr_data[ret[1]] = std::move("-1");
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{"-1"}).response;
            }
            return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
        }
        return this->resp_encoder.encode(simple_resp::RESP_TYPE::ERRORS,{"ERROR"}).response;
    }

    std::string medis_server_with_memory::decrby(const std::vector<std::string>& ret) {
        if (ret.size() == 3) {
            std::string v;
            if (this->sr->exists(ret[1])) {
                std::string &v = this->sr_data[ret[1]];

                long s = std::stol(v) - std::stol(ret[2]);
                v = std::move(std::to_string(s));
                return this->resp_encoder.encode(simple_resp::RESP_TYPE::INTEGERS,{v}).response;
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















}

