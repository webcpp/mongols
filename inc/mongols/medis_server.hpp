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
#include "lib/lrucache.hpp"
#include "lib/sqlite/sqlite3pp.h"
#include "lib/sqlite/sqlite3ppext.h"
#include "lib/lua/kaguya.hpp"


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


        void run(const std::string&, const std::string&);
        void ready();
        void set_max_open_files(int);
        void set_write_buffer_size(size_t);
        void set_max_file_size(size_t);
        void set_cache_size(size_t);
        void set_enable_compression(bool);

        void set_lru_str_max_size(size_t);
        void set_lru_list_max_size(size_t);
        void set_lru_map_max_size(size_t);
        void set_lru_set_max_size(size_t);
        void set_lru_queue_max_size(size_t);
        void set_lru_stack_max_size(size_t);

        void set_lua_package_path(const std::string& package_path, const std::string& package_cpath);
    private:

        tcp_server *server;
        leveldb::DB *db;
        leveldb::Options options;
        sqlite3pp::database *sqldb;
        kaguya::State vm;

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

        size_t lru_str_max_size
        , lru_list_max_size
        , lru_map_max_size
        , lru_set_max_size
        , lru_queue_max_size
        , lru_stack_max_size;
        mongols::cache::lru_cache<std::string, size_t> *sr, *lt, *mp, *st, *qe, *sk;
        mongols_map sr_data;
        mongols_hash_list lt_data;
        mongols_hash_map mp_data;
        mongols_hash_set st_data;
        mongols_hash_queue qe_data;
        mongols_hash_stack sk_data;

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
        virtual std::string getrange(const std::vector<std::string>&);
        virtual std::string setrange(const std::vector<std::string>&);

        //hash map
        virtual std::string hget(const std::vector<std::string>&);
        virtual std::string hset(const std::vector<std::string>&);
        virtual std::string hdel(const std::vector<std::string>&);
        virtual std::string hexists(const std::vector<std::string>&);
        virtual std::string hgetall(const std::vector<std::string>&);
        virtual std::string hlen(const std::vector<std::string>&);
        virtual std::string hmget(const std::vector<std::string>&);
        virtual std::string hmset(const std::vector<std::string>&);

        //list
        virtual std::string lpush_front(const std::vector<std::string>&);
        virtual std::string lpop_front(const std::vector<std::string>&);
        virtual std::string lpush_back(const std::vector<std::string>&);
        virtual std::string lpop_back(const std::vector<std::string>&);
        virtual std::string lfront(const std::vector<std::string>&);
        virtual std::string lback(const std::vector<std::string>&);
        virtual std::string llen(const std::vector<std::string>&);
        virtual std::string lrange(const std::vector<std::string>&);

        // set
        virtual std::string sadd(const std::vector<std::string>&);
        virtual std::string sdel(const std::vector<std::string>&);
        virtual std::string smembers(const std::vector<std::string>&);
        virtual std::string slen(const std::vector<std::string>&);
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
        std::string flushall(const std::vector<std::string>&);


        // the version with memory

        std::string _flushall(const std::vector<std::string>&);
        //string
        virtual std::string _get(const std::vector<std::string>&);
        virtual std::string _set(const std::vector<std::string>&);
        virtual std::string _del(const std::vector<std::string>&);
        virtual std::string _exists(const std::vector<std::string>&);
        virtual std::string _getset(const std::vector<std::string>&);
        virtual std::string _mget(const std::vector<std::string>&);
        virtual std::string _mset(const std::vector<std::string>&);
        virtual std::string _strlen(const std::vector<std::string>&);
        virtual std::string _append(const std::vector<std::string>&);
        virtual std::string _getrange(const std::vector<std::string>&);
        virtual std::string _setrange(const std::vector<std::string>&);

        //hash map
        virtual std::string _hget(const std::vector<std::string>&);
        virtual std::string _hset(const std::vector<std::string>&);
        virtual std::string _hdel(const std::vector<std::string>&);
        virtual std::string _hexists(const std::vector<std::string>&);
        virtual std::string _herase(const std::vector<std::string>&);
        virtual std::string _hgetall(const std::vector<std::string>&);
        virtual std::string _hlen(const std::vector<std::string>&);
        virtual std::string _hmget(const std::vector<std::string>&);
        virtual std::string _hmset(const std::vector<std::string>&);

        //list
        virtual std::string _lpush_front(const std::vector<std::string>&);
        virtual std::string _lpop_front(const std::vector<std::string>&);
        virtual std::string _lpush_back(const std::vector<std::string>&);
        virtual std::string _lpop_back(const std::vector<std::string>&);
        virtual std::string _lfront(const std::vector<std::string>&);
        virtual std::string _lback(const std::vector<std::string>&);
        virtual std::string _llen(const std::vector<std::string>&);
        virtual std::string _lrange(const std::vector<std::string>&);
        virtual std::string _lerase(const std::vector<std::string>&);
        virtual std::string _lexists(const std::vector<std::string>&);

        // set
        virtual std::string _sadd(const std::vector<std::string>&);
        virtual std::string _sdel(const std::vector<std::string>&);
        virtual std::string _smembers(const std::vector<std::string>&);
        virtual std::string _sexists(const std::vector<std::string>&);
        virtual std::string _sdifference(const std::vector<std::string>&);
        virtual std::string _sintersection(const std::vector<std::string>&);
        virtual std::string _sunion(const std::vector<std::string>&);
        virtual std::string _ssymmetric_difference(const std::vector<std::string>&);
        virtual std::string _serase(const std::vector<std::string>&);
        virtual std::string _slen(const std::vector<std::string>&);


        //queue
        virtual std::string _qpush(const std::vector<std::string>&);
        virtual std::string _qpop(const std::vector<std::string>&);
        virtual std::string _qfront(const std::vector<std::string>&);
        virtual std::string _qback(const std::vector<std::string>&);
        virtual std::string _qempty(const std::vector<std::string>&);
        virtual std::string _qerase(const std::vector<std::string>&);
        virtual std::string _qlen(const std::vector<std::string>&);

        //stack
        virtual std::string _zpush(const std::vector<std::string>&);
        virtual std::string _zpop(const std::vector<std::string>&);
        virtual std::string _ztop(const std::vector<std::string>&);
        virtual std::string _zempty(const std::vector<std::string>&);
        virtual std::string _zerase(const std::vector<std::string>&);
        virtual std::string _zlen(const std::vector<std::string>&);

        // incr and  decr
        virtual std::string _incrby(const std::vector<std::string>&);
        virtual std::string _incr(const std::vector<std::string>&);
        virtual std::string _decrby(const std::vector<std::string>&);
        virtual std::string _decr(const std::vector<std::string>&);

        // sqlite

        virtual std::string sql_cmd(const std::vector<std::string>&);
        virtual std::string sql_bind_cmd(const std::vector<std::string>&);
        virtual std::string sql_transaction(const std::vector<std::string>&);
        virtual std::string sql_query(const std::vector<std::string>&);

        // lua

        virtual std::string lua_content(const std::vector<std::string>&);
        virtual std::string lua_script(const std::vector<std::string>&);


    };

} // namespace mongols


#endif