#ifndef MEDIS_SERVER_WITH_MEMORY_HPP
#define MEDIS_SERVER_WITH_MEMORY_HPP

#include "medis_server_mixed_memory.hpp"

namespace mongols {

    class medis_server_with_memory : public medis_server_mixed_memory {
    public:
        medis_server_with_memory() = delete;
        medis_server_with_memory(const std::string &host
                , int port
                , int timeout = 5000
                , size_t buffer_size = 8092
                , size_t thread_size = std::thread::hardware_concurrency()
                , int max_event_size = 64);
        virtual~medis_server_with_memory() = default;

    protected:
        //string
        virtual std::string get(const std::vector<std::string>&);
        virtual std::string set(const std::vector<std::string>&);
        virtual std::string del(const std::vector<std::string>&);
        virtual std::string exists(const std::vector<std::string>&);
        virtual std::string getset(const std::vector<std::string>&);
        virtual std::string mget(const std::vector<std::string>&);
        virtual std::string mset(const std::vector<std::string>&);
        virtual std::string strlen(const std::vector<std::string>&);
        virtual std::string append(const std::vector<std::string>&);

        // incr and  decr
        virtual std::string incrby(const std::vector<std::string>&);
        virtual std::string incr(const std::vector<std::string>&);
        virtual std::string decrby(const std::vector<std::string>&);
        virtual std::string decr(const std::vector<std::string>&);
    };
}


#endif /* MEDIS_SERVER_WITH_MEMORY_HPP */

