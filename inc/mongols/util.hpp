#ifndef UTIL_HPP
#define UTIL_HPP


#include <functional>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>


namespace mongols {


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
    std::vector<std::string> split(const std::string& s, char delimiter);

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

    struct regular_expression {
        static std::string INTEGER, NUMBER, EMAIL, URL;
    };


    std::string base64_encode(const std::string&, bool = false);
    std::string base64_decode(const std::string&, bool = false);


    std::string bin2hex(const std::string&);

    std::string url_encode(const std::string& str);

    std::string url_decode(const std::string& str);

    void forker(int, const std::function<void()>&, std::vector<pid_t>&);

    bool process_bind_cpu(pid_t pid, int cpu);
    
    bool regex_match(const std::string & pattern, const std::string & str, std::vector<std::string> & results);
    
    bool regex_find(const std::string & pattern, const std::string & str, std::vector<std::string> & results);
}

#endif /* UTIL_HPP */

