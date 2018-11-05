#ifndef TCP_THREADING_SERVER_HPP
#define TCP_THREADING_SERVER_HPP

#include "tcp_server.hpp"
#include "thread_pool.hpp"
#include <mutex>

namespace mongols {

    class tcp_threading_server : public tcp_server {
    public:
        tcp_threading_server() = delete;

        tcp_threading_server(const std::string& host, int port
                , int timeout = 5000
                , size_t buffer_size = 8192
                , size_t thread_size = std::thread::hardware_concurrency()
                , int max_event_size = 64);
        virtual~tcp_threading_server() = default;
    private:

        virtual void add_client(int, const std::string&, int);
        virtual void del_client(int);
        virtual bool send_to_all_client(int, const std::string&, const filter_handler_function&);
        virtual bool work(int, const handler_function&);
        bool send_to_other_client(int, int, const client_t&, const std::string&, const filter_handler_function&);
    private:

        std::mutex main_mtx;





    };
}

#endif /* TCP_THREADING_SERVER_HPP */

