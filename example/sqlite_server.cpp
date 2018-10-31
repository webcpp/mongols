#include <mongols/sqlite_server.hpp>

int main(int, char**) {
    int port = 9090;
    const char* host = "127.0.0.1";
    mongols::sqlite_server
    server(host, port, 5000, 8096, 0/*2*/);
    server.run("html/sqlite/test.db");

}