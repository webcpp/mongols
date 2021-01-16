#include <mongols/sqlite_server.hpp>

int main(int, char **)
{
    int port = 9090;
    const char *host = "127.0.0.1";
    mongols::sqlite_server
        server(host, port, 5000, 8192, 0 /*2*/);
    //    if (!server.set_openssl("openssl/localhost.crt", "openssl/localhost.key")) {
    //        return -1;
    //    }
    server.set_shutdown([&]() {
        std::cout << "process " << getpid() << " exit.\n";
    });
    server.run("html/sqlite/test.db");
}