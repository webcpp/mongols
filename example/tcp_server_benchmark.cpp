#include <algorithm>
#include <ctime>
#include <iostream>
#include <memory>
#include <mongols/lib/args.hxx>
#include <mongols/tcp_proxy_server.hpp>
#include <mongols/thread_pool.hpp>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

int main(int argc, char **argv)
{
    args::ArgumentParser parser("This is a tcp server benchmark program.", "This goes after the options.");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<std::string> host(parser, "host", "The host flag", {"host"});
    args::ValueFlag<int> port(parser, "port", "The port flag", {"port"});
    args::ValueFlag<int> client(parser, "client", "The client flag", {"client"});
    args::ValueFlag<int> buffer(parser, "buffer", "The buffer flag", {"buffer"});
    args::ValueFlag<int> loop(parser, "loop", "The loop flag", {"loop"});
    args::ValueFlag<int> ssl(parser, "ssl", "The ssl flag", {"ssl"});

    std::string _host = "localhost";
    int _port = 9090, _client = 100, _buffer = 4096, _loop = 1000;
    bool _ssl = false;
    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (args::Help &)
    {
        std::cout << parser;
        return 0;
    }
    catch (args::ParseError &e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return -1;
    }
    catch (args::ValidationError &e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return -1;
    }
    if (host)
    {
        _host = args::get(host);
    }
    if (port)
    {
        _port = args::get(port);
    }
    if (client)
    {
        _client = args::get(client);
    }
    if (buffer)
    {
        _buffer = args::get(buffer);
    }
    if (loop)
    {
        _loop = args::get(loop);
    }
    if (ssl)
    {
        _ssl = args::get(ssl) == 0 ? false : true;
    }
    std::cout << "host: " << _host << std::endl;
    std::cout << "port: " << _port << std::endl;
    std::cout << "client: " << _client << std::endl;
    std::cout << "buffer: " << _buffer << std::endl;
    std::cout << "loop: " << _loop << std::endl;
    std::cout << "ssl: " << std::boolalpha << _ssl << std::endl;

    std::vector<std::shared_ptr<mongols::tcp_client>> cli;

    std::string str(_buffer, 'c');

    for (int i = 0; i < _client; ++i)
    {
        cli.emplace_back(std::make_shared<mongols::tcp_client>(_host, _port, _ssl));
    }

    mongols::thread_pool<std::function<void(void)>> th_pool;

    for (int i = 0; i < _client; ++i)
    {
        th_pool.submit([&, i, _loop]() {
            if (cli[i]->ok())
            {
                for (int j = 0; j < _loop; ++j)
                {
                    if (cli[i]->send(str.c_str(), str.size()) > 0)
                    {
                        char buf[_buffer];
                        if (cli[i]->recv(&buf[0], _buffer) > 0)
                        {
                            std::this_thread::yield();
                        }
                    }
                }
            }
        });
    }

    return 0;
}
