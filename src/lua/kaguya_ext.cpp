#include <string>
#include <vector>
#include "lib/re2/re2.h"
#include "posix_regex.hpp"
#include "util.hpp"
#include "lib/hash/md5.hpp"
#include "lib/hash/sha1.hpp"
#include "lib/lua/kaguya_ext.hpp"
#include "ext/json.hpp"

namespace mongols {

    void lua_ext(kaguya::State& vm) {
        vm["mongols_regex"] = kaguya::NewTable();
        kaguya::LuaTable regex_tbl = vm["mongols_regex"];
        regex_tbl["full_match"] = kaguya::function([](const std::string& pattern, const std::string & str) {
            return RE2::FullMatch(str, pattern);
        });
        regex_tbl["partial_match"] = kaguya::function([](const std::string& pattern, const std::string & str) {
            return RE2::PartialMatch(str, pattern);
        });
        regex_tbl["match"] = kaguya::function([](const std::string& pattern, const std::string & str) {
            mongols::posix_regex regex(pattern);
            std::vector<std::string> v;
            if (regex.match(str, v)) {
                return v;
            }
            return v;
        });
        regex_tbl["match_find"] = kaguya::function([](const std::string& pattern, const std::string & str) {
            std::vector<std::string> v;
            bool b = regex_find(pattern, str, v);
            return std::tuple<bool, std::vector < std::string >> (b, v);
        });

        vm["mongols_hash"] = kaguya::NewTable();
        kaguya::LuaTable hash_tbl = vm["mongols_hash"];
        hash_tbl["md5"] = kaguya::function([](const std::string & str) {
            return mongols::md5(str);
        });
        hash_tbl["sha1"] = kaguya::function([](const std::string & str) {
            return mongols::sha1(str);
        });

        vm["mongols_json"].setClass(
                kaguya::UserdataMetatable<json>()
                .setConstructors < json(), json(json)>()
                .addOverloadedFunctions("set", &json::set_json, &json::set_double, &json::set_long, &json::set_string)
                .addOverloadedFunctions("get", &json::get_element, &json::get_object)
                .addOverloadedFunctions("append", &json::append_double, &json::append_long, &json::append_string)
                .addFunction("parse_string", &json::parse_string)
                .addFunction("parse_file", &json::parse_file)
                .addFunction("as_double", &json::as_double)
                .addFunction("as_long", &json::as_long)
                .addFunction("as_string", &json::as_string)
                .addFunction("to_string", &json::to_string)
                );
    }

    











}
