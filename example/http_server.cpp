#include <mongols/http_server.hpp>

int main(int, char **)
{
    auto f = [](const mongols::request &) {
        return true;
    };
    auto g = [](const mongols::request &req, mongols::response &res) {
        res.content = std::move("hello,world");
        res.status = 200;
    };
    int port = 9090;
    const char *host = "127.0.0.1";
    mongols::http_server
        server(host, port, 5000, 8192, 0 /*2*/);
    server.set_enable_session(false);
    server.set_enable_cache(false);
    //    if (!server.set_openssl("openssl/localhost.crt", "openssl/localhost.key")) {
    //        return -1;
    //    }
    server.set_shutdown([&]() {
        std::cout << "process " << getpid() << " exit.\n";
    });
    server.run(f, g);

    // or
    /*
    server.add_route({"GET"}, "^/get/([a-zA-Z]+)/?$"
    , [](const mongols::request& req, mongols::response& res, const std::vector<std::string>& param) {
        res.content = req.method + "<br/>" + param[1];
        res.status = 200;
    });

    server.add_route({"POST"}, "^/post/([0-9]+)/?$"
    , [](const mongols::request& req, mongols::response& res, const std::vector<std::string>& param) {
        res.content = req.method + "<br/>" + param[1];
        res.status = 200;
    });
    server.run_with_route(f);
    */
}