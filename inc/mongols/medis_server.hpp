#ifndef MEDIS_SERVER_HPP
#define MEDIS_SERVER_HPP

#include <ctime>
#include <unordered_map>
#include <string>
#include <vector>
#include <list>
#include <queue>
#include <set>
#include <stack>
#include <functional>
#include <memory>

#include "tcp_threading_server.hpp"
#include "lib/simple_resp.h"
#include "lib/leveldb/db.h"
#include "lib/leveldb/options.h"
#include "lib/LRUCache11.hpp"
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

        typedef std::string(medis_server::*op_funcion)(const std::vector<std::string>&);
        simple_resp::decoder resp_decoder;
        simple_resp::encoder resp_encoder;
        std::unordered_map<std::string, op_funcion> op;

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




        size_t lru_string_max_size
        , lru_map_max_size
        , lru_list_max_size
        , lru_set_max_size
        , lru_queue_max_size
        , lru_stack_max_size;

        template<class value_t>
        class cache_t {
        public:

            cache_t();

            cache_t(const value_t& v);
            virtual~cache_t() = default;

            bool expired()const;
            long long ttl()const;
        public:
            value_t data;
            time_t t;
            long long expires;
        };

        typedef std::shared_ptr<cache_t<std::string>> shared_mongols_string;
        typedef std::unordered_map<std::string, std::string> mongols_map;
        typedef std::shared_ptr<cache_t<mongols_map>> shared_mongols_map;
        typedef std::list<std::string> mongols_list;
        typedef std::shared_ptr<cache_t<mongols_list>> shared_mongols_list;
        typedef std::set<std::string> mongols_set;
        typedef std::shared_ptr<cache_t<mongols_set>> shared_mongols_set;
        typedef std::queue<std::string> mongols_queue;
        typedef std::shared_ptr<cache_t<mongols_queue>> shared_mongols_queue;
        typedef std::stack<std::string> mongols_stack;
        typedef std::shared_ptr<cache_t<mongols_stack>> shared_mongols_stack;

        std::shared_ptr<lru11::Cache<std::string, shared_mongols_string>> string_data;
        std::shared_ptr<lru11::Cache<std::string, shared_mongols_map>> map_data;
        std::shared_ptr<lru11::Cache<std::string, shared_mongols_list>> list_data;
        std::shared_ptr<lru11::Cache<std::string, shared_mongols_set>> set_data;
        std::shared_ptr<lru11::Cache<std::string, shared_mongols_queue>> queue_data;
        std::shared_ptr<lru11::Cache<std::string, shared_mongols_stack>> stack_data;


        //string
        std::string get(const std::vector<std::string>&);
        std::string set(const std::vector<std::string>&);
        std::string del(const std::vector<std::string>&);
        std::string exists(const std::vector<std::string>&);
        std::string getset(const std::vector<std::string>&);
        std::string mget(const std::vector<std::string>&);
        std::string mset(const std::vector<std::string>&);
        std::string strlen(const std::vector<std::string>&);
        std::string append(const std::vector<std::string>&);
        std::string getrange(const std::vector<std::string>&);
        std::string setrange(const std::vector<std::string>&);

        //hash map
        std::string hget(const std::vector<std::string>&);
        std::string hset(const std::vector<std::string>&);
        std::string hdel(const std::vector<std::string>&);
        std::string hexists(const std::vector<std::string>&);
        std::string hgetall(const std::vector<std::string>&);
        std::string hlen(const std::vector<std::string>&);
        std::string hmget(const std::vector<std::string>&);
        std::string hmset(const std::vector<std::string>&);

        //list
        std::string lpush_front(const std::vector<std::string>&);
        std::string lpop_front(const std::vector<std::string>&);
        std::string lpush_back(const std::vector<std::string>&);
        std::string lpop_back(const std::vector<std::string>&);
        std::string lfront(const std::vector<std::string>&);
        std::string lback(const std::vector<std::string>&);
        std::string llen(const std::vector<std::string>&);
        std::string lrange(const std::vector<std::string>&);

        // set
        std::string sadd(const std::vector<std::string>&);
        std::string sdel(const std::vector<std::string>&);
        std::string smembers(const std::vector<std::string>&);
        std::string slen(const std::vector<std::string>&);
        std::string sexists(const std::vector<std::string>&);
        std::string sdifference(const std::vector<std::string>&);
        std::string sintersection(const std::vector<std::string>&);
        std::string sunion(const std::vector<std::string>&);
        std::string ssymmetric_difference(const std::vector<std::string>&);

        // incr and  decr
        std::string incrby(const std::vector<std::string>&);
        std::string incr(const std::vector<std::string>&);
        std::string decrby(const std::vector<std::string>&);
        std::string decr(const std::vector<std::string>&);

        // echo and ping
        std::string echo(const std::vector<std::string>&);
        std::string ping(const std::vector<std::string>&);
        std::string flushall(const std::vector<std::string>&);


        // the version with memory

        std::string _flushall(const std::vector<std::string>&);
        //string
        std::string _get(const std::vector<std::string>&);
        std::string _set(const std::vector<std::string>&);
        std::string _del(const std::vector<std::string>&);
        std::string _exists(const std::vector<std::string>&);
        std::string _getset(const std::vector<std::string>&);
        std::string _mget(const std::vector<std::string>&);
        std::string _mset(const std::vector<std::string>&);
        std::string _strlen(const std::vector<std::string>&);
        std::string _append(const std::vector<std::string>&);
        std::string _getrange(const std::vector<std::string>&);
        std::string _setrange(const std::vector<std::string>&);
        std::string _expire(const std::vector<std::string>&);
        std::string _ttl(const std::vector<std::string>&);

        //hash map
        std::string _hget(const std::vector<std::string>&);
        std::string _hset(const std::vector<std::string>&);
        std::string _hdel(const std::vector<std::string>&);
        std::string _hexists(const std::vector<std::string>&);
        std::string _herase(const std::vector<std::string>&);
        std::string _hgetall(const std::vector<std::string>&);
        std::string _hlen(const std::vector<std::string>&);
        std::string _hmget(const std::vector<std::string>&);
        std::string _hmset(const std::vector<std::string>&);
        std::string _hexpire(const std::vector<std::string>&);
        std::string _httl(const std::vector<std::string>&);

        //list
        std::string _lpush_front(const std::vector<std::string>&);
        std::string _lpop_front(const std::vector<std::string>&);
        std::string _lpush_back(const std::vector<std::string>&);
        std::string _lpop_back(const std::vector<std::string>&);
        std::string _lfront(const std::vector<std::string>&);
        std::string _lback(const std::vector<std::string>&);
        std::string _llen(const std::vector<std::string>&);
        std::string _lrange(const std::vector<std::string>&);
        std::string _lerase(const std::vector<std::string>&);
        std::string _lexists(const std::vector<std::string>&);
        std::string _lexpire(const std::vector<std::string>&);
        std::string _lttl(const std::vector<std::string>&);

        // set
        std::string _sadd(const std::vector<std::string>&);
        std::string _sdel(const std::vector<std::string>&);
        std::string _smembers(const std::vector<std::string>&);
        std::string _sexists(const std::vector<std::string>&);
        std::string _sdifference(const std::vector<std::string>&);
        std::string _sintersection(const std::vector<std::string>&);
        std::string _sunion(const std::vector<std::string>&);
        std::string _ssymmetric_difference(const std::vector<std::string>&);
        std::string _serase(const std::vector<std::string>&);
        std::string _slen(const std::vector<std::string>&);
        std::string _sexpire(const std::vector<std::string>&);
        std::string _sttl(const std::vector<std::string>&);


        //queue
        std::string _qpush(const std::vector<std::string>&);
        std::string _qpop(const std::vector<std::string>&);
        std::string _qfront(const std::vector<std::string>&);
        std::string _qback(const std::vector<std::string>&);
        std::string _qempty(const std::vector<std::string>&);
        std::string _qerase(const std::vector<std::string>&);
        std::string _qlen(const std::vector<std::string>&);
        std::string _qexpire(const std::vector<std::string>&);
        std::string _qttl(const std::vector<std::string>&);

        //stack
        std::string _zpush(const std::vector<std::string>&);
        std::string _zpop(const std::vector<std::string>&);
        std::string _ztop(const std::vector<std::string>&);
        std::string _zempty(const std::vector<std::string>&);
        std::string _zerase(const std::vector<std::string>&);
        std::string _zlen(const std::vector<std::string>&);
        std::string _zexpire(const std::vector<std::string>&);
        std::string _zttl(const std::vector<std::string>&);

        // incr and  decr
        std::string _incrby(const std::vector<std::string>&);
        std::string _incr(const std::vector<std::string>&);
        std::string _decrby(const std::vector<std::string>&);
        std::string _decr(const std::vector<std::string>&);

        // sqlite

        std::string sql_cmd(const std::vector<std::string>&);
        std::string sql_bind_cmd(const std::vector<std::string>&);
        std::string sql_transaction(const std::vector<std::string>&);
        std::string sql_query(const std::vector<std::string>&);

        // lua

        std::string lua_content(const std::vector<std::string>&);
        std::string lua_script(const std::vector<std::string>&);


    };

} // namespace mongols


#endif