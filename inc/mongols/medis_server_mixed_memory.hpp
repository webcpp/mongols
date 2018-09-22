#ifndef MEDIS_SERVER_MIXED_MEMORY_HPP
#define MEDIS_SERVER_MIXED_MEMORY_HPP

#include "medis_server.hpp"
#include "lib/lrucache.hpp"

namespace mongols {

    class medis_server_mixed_memory : public medis_server {
    public:
        medis_server_mixed_memory() = delete;
        medis_server_mixed_memory(const std::string &host
                , int port
                , int timeout = 5000
                , size_t buffer_size = 8092
                , size_t thread_size = std::thread::hardware_concurrency()
                , int max_event_size = 64);
        virtual~medis_server_mixed_memory();

        void set_lru_str_max_size(size_t);
        void set_lru_list_max_size(size_t);
        void set_lru_map_max_size(size_t);
        void set_lru_set_max_size(size_t);
        void set_lru_queue_max_size(size_t);
        void set_lru_stack_max_size(size_t);
        void ready();

    protected:
        size_t lru_str_max_size, lru_list_max_size, lru_map_max_size, lru_set_max_size, lru_queue_max_size, lru_stack_max_size;
        mongols::cache::lru_cache<std::string, size_t> *sr, *lt, *mp, *st, *qe, *sk;
        mongols_map sr_data;
        mongols_hash_list lt_data;
        mongols_hash_map mp_data;
        mongols_hash_set st_data;
        mongols_hash_queue qe_data;
        mongols_hash_stack sk_data;
    protected:
        
        virtual std::string flushall(const std::vector<std::string>&);

        //hash map
        virtual std::string hget(const std::vector<std::string>&);
        virtual std::string hset(const std::vector<std::string>&);
        virtual std::string hdel(const std::vector<std::string>&);
        virtual std::string hexists(const std::vector<std::string>&);

        //list
        virtual std::string lpush_front(const std::vector<std::string>&);
        virtual std::string lpop_front(const std::vector<std::string>&);
        virtual std::string lpush_back(const std::vector<std::string>&);
        virtual std::string lpop_back(const std::vector<std::string>&);
        virtual std::string lfront(const std::vector<std::string>&);
        virtual std::string lback(const std::vector<std::string>&);
        virtual std::string llen(const std::vector<std::string>&);


        // set
        virtual std::string sadd(const std::vector<std::string>&);
        virtual std::string sdel(const std::vector<std::string>&);
        virtual std::string smembers(const std::vector<std::string>&);
        virtual std::string scard(const std::vector<std::string>&);
        virtual std::string sexists(const std::vector<std::string>&);
        virtual std::string sdifference(const std::vector<std::string>&);
        virtual std::string sintersection(const std::vector<std::string>&);
        virtual std::string sunion(const std::vector<std::string>&);
        virtual std::string ssymmetric_difference(const std::vector<std::string>&);

        //queue
        virtual std::string qpush(const std::vector<std::string>&);
        virtual std::string qpop(const std::vector<std::string>&);
        virtual std::string qfront(const std::vector<std::string>&);
        virtual std::string qback(const std::vector<std::string>&);
        virtual std::string qempty(const std::vector<std::string>&);

        //stack
        virtual std::string zpush(const std::vector<std::string>&);
        virtual std::string zpop(const std::vector<std::string>&);
        virtual std::string ztop(const std::vector<std::string>&);
        virtual std::string zempty(const std::vector<std::string>&);

    };

}

#endif /* MEDIS_SERVER_MIXED_MEMORY_HPP */

