#ifndef A58377A7_6C75_4297_B3B5_9FE44A15B849
#define A58377A7_6C75_4297_B3B5_9FE44A15B849

#include <string>

namespace mongols {

class hash_engine {
public:
    static std::string bin2hex(const std::string&);
    static std::string bin2hex(const char*, size_t);
    enum hash_t {
        MD5 = 0,
        SHA1,
        SHA256,
        SHA512
    };

public:
    hash_engine() = delete;
    hash_engine(hash_t);
    virtual ~hash_engine();
    const std::string& get(const std::string&);
    const std::string& get(const char*, size_t);

private:
    hash_t t;
    size_t DIGEST_LENGTH;
    void* ctx;
    std::string out;
    char* buffer;
};

}

#endif /* A58377A7_6C75_4297_B3B5_9FE44A15B849 */
