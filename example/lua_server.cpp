#include <mongols/lua_server.hpp>

int main(int, char**) {
    int port = 9090;
    const char* host = "127.0.0.1";
    mongols::lua_server
    server(host, port, 5000, 8096, 2/*0*/);
    server.set_root_path("html/lua");
    server.run("html/lua/package/?.lua;", "html/lua/package/?.so;");
}