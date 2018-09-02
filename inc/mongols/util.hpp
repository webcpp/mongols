#ifndef UTIL_HPP
#define UTIL_HPP

#include <openssl/evp.h>  
#include <openssl/bio.h>  
#include <openssl/buffer.h> 
#include <openssl/sha.h>

#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>


namespace mongols {
    std::string md5(const std::string& str);

    std::string random_string(const std::string& s);

    void read_file(const std::string& path, std::string& out);

    bool is_file(const std::string& s);

    bool is_dir(const std::string& s);


    std::string http_time(time_t *t);

    time_t parse_http_time(u_char* value, size_t len);

    std::string trim(const std::string& s);
    void trim(std::string& s);

    void parse_param(const std::string& data, std::unordered_map<std::string, std::string>& result, char c = '&', char cc = '=');

    void split(const std::string& s, char delim, std::vector<std::string>& v);
    void split(const std::string& s, const std::string& delim, std::vector<std::string>& v);

    class random {
    public:

        random() : now(time(0)) {
            srand(this->now);
        }

        template<class T>
        T create(T left, T right) {
            return static_cast<T> (this->create()*(right - left) + left);
        }
    private:
        time_t now;

        double create() {
            return static_cast<double> (rand() / (RAND_MAX + 1.0));
        }
    };

    static std::string INTEGER = R"(^[+-]?[1-9]+[0-9]*$)"
            , NUMBER = R"(^[+-]?[1-9]+[0-9]*\.?[0-9]*$)"
            , EMAIL = R"(^[0-9a-zA-Z]+(([-_\.])?[0-9a-zA-Z]+)?\@[0-9a-zA-Z]+[-_]?[0-9a-zA-Z]+(\.[0-9a-zA-Z]+)+$)"
            , URL = R"(^(http[s]?|ftp)://[0-9a-zA-Z\._-]([0-9a-zA-Z]+/?)+\??.*$)";

    std::string base64_encode(const std::string&, bool = false);
    std::string base64_decode(const std::string&, bool = false);

    std::string sha1(const std::string&);

    std::string bin2hex(const std::string&);

    std::string url_encode(const std::string& str);
    
    std::string url_decode(const std::string& str);
}

#endif /* UTIL_HPP */

