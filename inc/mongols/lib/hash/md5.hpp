#ifndef MD5_HPP
#define MD5_HPP

#include <openssl/md5.h>
#include <string>

namespace mongols {
std::string md5(const std::string& str);

class md5_engine {
public:
    md5_engine();
    virtual ~md5_engine();
    const std::string& get(const std::string&);
    const std::string& get(const char*, size_t);

private:
    MD5_CTX ctx;
    std::string out;
    char* buffer;
};

}

#endif /* MD5_HPP */
