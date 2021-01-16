#include "lib/lua/kaguya_ext.hpp"
#include "ext/json.hpp"
#include "lib/hash/hash_engine.hpp"
#include "lib/re2/re2.h"
#include "posix_regex.hpp"
#include "tcp_proxy_server.hpp"
#include "util.hpp"
#include "version.hpp"
#include <string>
#include <vector>

namespace mongols
{

    class tcp_client_wrap
    {
    public:
        tcp_client_wrap(const std::string &host, int port, bool enable_ssl)
            : cli(host, port, enable_ssl)
        {
        }

        bool ok()
        {
            return this->cli.ok();
        }

        bool send(const std::string &str)
        {
            if (str.empty())
            {
                return false;
            }
            return this->cli.send(str.c_str(), str.size()) > 0;
        }

        std::string recv(size_t len)
        {
            char buffer[len];
            ssize_t ret = this->cli.recv(buffer, len);
            if (ret > 0)
            {
                return std::string(buffer, ret);
            }
            return "";
        }

    private:
        tcp_client cli;
    };

    void lua_ext(kaguya::State &vm)
    {
        vm["mongols_regex"] = kaguya::NewTable();
        kaguya::LuaTable regex_tbl = vm["mongols_regex"];
        regex_tbl["full_match"] = kaguya::function([](const std::string &pattern, const std::string &str) {
            return RE2::FullMatch(str, pattern);
        });
        regex_tbl["partial_match"] = kaguya::function([](const std::string &pattern, const std::string &str) {
            return RE2::PartialMatch(str, pattern);
        });
        regex_tbl["match"] = kaguya::function([](const std::string &pattern, const std::string &str) {
            mongols::posix_regex regex(pattern);
            std::vector<std::string> v;
            if (regex.match(str, v))
            {
                return v;
            }
            return v;
        });
        regex_tbl["match_find"] = kaguya::function([](const std::string &pattern, const std::string &str) {
            std::vector<std::string> v;
            bool b = regex_find(pattern, str, v);
            return std::tuple<bool, std::vector<std::string>>(b, v);
        });

        vm["mongols_hash"] = kaguya::NewTable();
        kaguya::LuaTable hash_tbl = vm["mongols_hash"];
        hash_tbl["md5"] = kaguya::function([](const std::string &str) {
            return mongols::hash_engine::md5(str);
        });
        hash_tbl["sha1"] = kaguya::function([](const std::string &str) {
            return mongols::hash_engine::sha1(str);
        });

        vm["mongols_json"].setClass(
            kaguya::UserdataMetatable<json>()
                .setConstructors<json(), json(json)>()
                .addOverloadedFunctions("set", &json::set_bool, &json::set_json, &json::set_double, &json::set_long, &json::set_string)
                .addOverloadedFunctions("get", &json::get_element, &json::get_object)
                .addOverloadedFunctions("append", &json::append_bool, &json::append_json, &json::append_double, &json::append_long, &json::append_string)
                .addFunction("parse_string", &json::parse_string)
                .addFunction("parse_file", &json::parse_file)
                .addFunction("as_double", &json::as_double)
                .addFunction("as_long", &json::as_long)
                .addFunction("as_string", &json::as_string)
                .addFunction("as_bool", &json::as_bool)
                .addFunction("to_string", &json::to_string)
                .addFunction("is_double", &json::is_double)
                .addFunction("is_bool", &json::is_bool)
                .addFunction("is_long", &json::is_long)
                .addFunction("is_object", &json::is_object)
                .addFunction("is_array", &json::is_array)
                .addFunction("size", &json::size));

        vm["mongols_tcp_client"].setClass(
            kaguya::UserdataMetatable<mongols::tcp_client_wrap>()
                .setConstructors<mongols::tcp_client_wrap(const std::string &, int, bool)>()
                .addFunction("ok", &mongols::tcp_client_wrap::ok)
                .addFunction("send", &mongols::tcp_client_wrap::send)
                .addFunction("recv", &mongols::tcp_client_wrap::recv));
    }

} // namespace mongols
