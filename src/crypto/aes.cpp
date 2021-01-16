#include "lib/crypto/aes.hpp"
#include "util.hpp"

namespace mongols
{
    aes::aes(const std::string &key)
        : key(key), en_key(), de_key()
    {
        AES_set_encrypt_key((const unsigned char *)this->key.c_str(), 128, &this->en_key);
        AES_set_decrypt_key((const unsigned char *)this->key.c_str(), 128, &this->de_key);
    }

    std::string aes::encode16(const std::string &str)
    {
        unsigned char out[AES_BLOCK_SIZE];
        AES_encrypt((const unsigned char *)str.c_str(), out, &this->en_key);
        return std::string((char *)out, AES_BLOCK_SIZE);
    }

    std::string aes::decode16(const std::string &cipher)
    {
        unsigned char out[AES_BLOCK_SIZE];
        AES_decrypt((const unsigned char *)cipher.c_str(), out, &this->de_key);
        return std::string((char *)out, AES_BLOCK_SIZE);
    }

    const std::string &aes::encode(const std::string &str)
    {
        this->cipher.clear();
        size_t len = str.size(), count = len / AES_BLOCK_SIZE, j = 0;
        if (count > 0)
        {
            for (size_t i = 0; i < count; ++i)
            {
                this->cipher.append(this->encode16(str.substr(j, AES_BLOCK_SIZE)));
                j += AES_BLOCK_SIZE;
            }
            this->cipher.append(this->encode16(str.substr(j)));
        }
        else
        {
            this->cipher = this->encode16(str);
        }
        return this->cipher;
    }
    const std::string &aes::decode(const std::string &str)
    {
        this->plain.clear();
        size_t len = str.size(), count = len / AES_BLOCK_SIZE, j = 0;
        if (count > 0)
        {
            for (size_t i = 0; i < count; ++i)
            {
                this->plain.append(this->decode16(str.substr(j, AES_BLOCK_SIZE)));
                j += AES_BLOCK_SIZE;
            }
        }
        else
        {
            this->plain = this->decode16(str);
        }
        trim(std::ref(this->plain));
        return this->plain;
    }
} // namespace mongols