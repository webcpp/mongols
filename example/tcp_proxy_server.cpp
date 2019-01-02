#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> 
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/prctl.h>
#include <netdb.h> 


#include <unordered_map>
#include <memory>
#include <mongols/util.hpp>
#include <mongols/tcp_proxy_server.hpp>



int main(int, char**) {
    auto f = [](const mongols::tcp_server::client_t & client) {
        return true;
    };
    int port = 9090;
    const char* host = "127.0.0.1";

    mongols::tcp_proxy_server server(host, port);

    server.set_back_server(host, 8888);
    server.set_back_server(host, 8889);
    
    server.set_default_http_content();

    //    server.run(f);


    std::function<void(pthread_mutex_t*, size_t*) > ff = [&](pthread_mutex_t* mtx, size_t * data) {
        server.run(f);
    };

    std::function<bool(int) > g = [&](int status) {
        std::cout << strsignal(WTERMSIG(status)) << std::endl;
        return false;
    };

    mongols::multi_process main_process;
    main_process.run(ff, g);
}
