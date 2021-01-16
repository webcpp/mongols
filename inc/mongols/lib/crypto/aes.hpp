#ifndef B1A981F8_389F_49B7_B7C6_490C4007C8A8
#define B1A981F8_389F_49B7_B7C6_490C4007C8A8

#include <openssl/aes.h>
#include <string>

namespace mongols {
class aes {
public:
    aes(const std::string& key);
    virtual ~aes() = default;

private:
    std::string encode16(const std::string& str);
    std::string decode16(const std::string& cipher);

public:
    const std::string& encode(const std::string& str);
    const std::string& decode(const std::string& str);

private:
    std::string key, cipher, plain;
    AES_KEY en_key, de_key;
};
}

#endif /* B1A981F8_389F_49B7_B7C6_490C4007C8A8 */
