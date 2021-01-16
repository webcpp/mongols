#pragma once

#include <string>

namespace mongols
{

    namespace hash_engine
    {

        std::string bin2hex(const std::string &);
        std::string bin2hex(const char *, size_t);
        std::string md5(const std::string &);
        std::string md5(const char *, size_t);
        std::string sha1(const std::string &);
        std::string sha1(const char *, size_t);
        std::string sha256(const std::string &);
        std::string sha256(const char *, size_t);
        std::string sha512(const std::string &);
        std::string sha512(const char *, size_t);
    } // namespace hash_engine

} // namespace mongols
