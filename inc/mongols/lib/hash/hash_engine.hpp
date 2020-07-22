#ifndef A58377A7_6C75_4297_B3B5_9FE44A15B849
#define A58377A7_6C75_4297_B3B5_9FE44A15B849

#include <string>

namespace mongols {

namespace hash_engine {

    std::string bin2hex(const std::string&);
    std::string bin2hex(const char*, size_t);
    std::string md5(const std::string&);
    std::string md5(const char*, size_t);
    std::string sha1(const std::string&);
    std::string sha1(const char*, size_t);
    std::string sha256(const std::string&);
    std::string sha256(const char*, size_t);
    std::string sha512(const std::string&);
    std::string sha512(const char*, size_t);
}

}

#endif /* A58377A7_6C75_4297_B3B5_9FE44A15B849 */
