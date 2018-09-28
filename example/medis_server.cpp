#include <mongols/medis_server.hpp>

int main(int, char**) {
    int port = 9090;
    const char* host = "127.0.0.1";
    mongols::medis_server
    server(host, port, 5000, 8096, 2/*0*/);
    server.set_lua_package_path("html/lua/package/?.lua;","html/lua/package/?.so;");
    server.ready();
    server.run("html/leveldb", "html/sqlite/test.db");
}
