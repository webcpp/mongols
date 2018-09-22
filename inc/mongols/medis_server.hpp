#ifndef MEDIS_SERVER_HPP
#define MEDIS_SERVER_HPP

#include <unordered_map>
#include <string>
#include <vector>
#include <list>
#include <queue>
#include <set>
#include <stack>
#include <functional>

#include "tcp_threading_server.hpp"
#include "lib/simple_resp.h"
#include "lib/leveldb/db.h"
#include "lib/leveldb/options.h"


namespace mongols {

    class medis_server {
    public:
        medis_server() = delete;
        medis_server(const std::string &host
                , int port
                , int timeout = 5000
                , size_t buffer_size = 8092
                , size_t thread_size = std::thread::hardware_concurrency()
                , int max_event_size = 64);
        virtual ~medis_server();


        void run(const std::string& path);
        void set_max_open_files(int);
        void set_write_buffer_size(size_t);
        void set_max_file_size(size_t);
    private:

        tcp_server *server;
        leveldb::DB *db;
        leveldb::Options options;

    protected:
        typedef std::string(medis_server::*op_funcion)(const std::vector<std::string>&);
        simple_resp::decoder resp_decoder;
        simple_resp::encoder resp_encoder;
        std::unordered_map<std::string, op_funcion> op;
    private:
        std::string work(
                const std::pair<char *, size_t> &
                , bool &
                , bool &
                , tcp_server::client_t &
                , tcp_server::filter_handler_function &
                );

        template<typename T>
        std::string serialize(const T&);
        template<typename T>
        void deserialize(const std::string&, T&);


    protected:
        typedef std::unordered_map<std::string, std::string> mongols_map;
        typedef std::unordered_map<std::string, mongols_map> mongols_hash_map;
        typedef std::list<std::string> mongols_list;
        typedef std::unordered_map<std::string, mongols_list> mongols_hash_list;
        typedef std::set<std::string> mongols_set;
        typedef std::unordered_map<std::string, mongols_set> mongols_hash_set;
        typedef std::queue<std::string> mongols_queue;
        typedef std::unordered_map<std::string, mongols_queue> mongols_hash_queue;
        typedef std::stack<std::string> mongols_stack;
        typedef std::unordered_map<std::string, mongols_stack> mongols_hash_stack;

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

        // incr and  decr
        virtual std::string incrby(const std::vector<std::string>&);
        virtual std::string incr(const std::vector<std::string>&);
        virtual std::string decrby(const std::vector<std::string>&);
        virtual std::string decr(const std::vector<std::string>&);

        // echo and ping
        virtual std::string echo(const std::vector<std::string>&);
        virtual std::string ping(const std::vector<std::string>&);





    };





} // namespace mongols
#endif