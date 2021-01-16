#include "lib/hash/hash_engine.hpp"
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <string.h>

namespace mongols
{
    namespace hash_engine
    {
        std::string bin2hex(const std::string &input)
        {
            std::string res;
            const char hex[] = "0123456789ABCDEF";
            for (auto &sc : input)
            {
                unsigned char c = static_cast<unsigned char>(sc);
                res += hex[c >> 4];
                res += hex[c & 0xf];
            }

            return res;
        }

        std::string bin2hex(const char *input, size_t len)
        {
            std::string res;
            const char hex[] = "0123456789ABCDEF";
            for (size_t i = 0; i < len; ++i)
            {
                unsigned char c = static_cast<unsigned char>(input[i]);
                res += hex[c >> 4];
                res += hex[c & 0xf];
            }

            return res;
        }

        std::string md5(const std::string &plain)
        {
            char buffer[MD5_DIGEST_LENGTH];
            MD5_CTX ctx;
            MD5_Init(&ctx);
            MD5_Update(&ctx, plain.c_str(), plain.size());
            MD5_Final(reinterpret_cast<unsigned char *>(buffer), &ctx);
            return bin2hex(buffer, MD5_DIGEST_LENGTH);
        }
        std::string md5(const char *plain, size_t len)
        {
            char buffer[MD5_DIGEST_LENGTH];
            MD5_CTX ctx;
            MD5_Init(&ctx);
            MD5_Update(&ctx, plain, len);
            MD5_Final(reinterpret_cast<unsigned char *>(buffer), &ctx);
            return bin2hex(buffer, MD5_DIGEST_LENGTH);
        }
        std::string sha1(const std::string &plain)
        {
            char buffer[SHA_DIGEST_LENGTH];
            SHA_CTX ctx;
            SHA1_Init(&ctx);
            SHA1_Update(&ctx, plain.c_str(), plain.size());
            SHA1_Final(reinterpret_cast<unsigned char *>(buffer), &ctx);
            return bin2hex(buffer, SHA_DIGEST_LENGTH);
        }
        std::string sha1(const char *plain, size_t len)
        {
            char buffer[SHA_DIGEST_LENGTH];
            SHA_CTX ctx;
            SHA1_Init(&ctx);
            SHA1_Update(&ctx, plain, len);
            SHA1_Final(reinterpret_cast<unsigned char *>(buffer), &ctx);
            return bin2hex(buffer, SHA_DIGEST_LENGTH);
        }
        std::string sha256(const std::string &plain)
        {
            char buffer[SHA256_DIGEST_LENGTH];
            SHA256_CTX ctx;
            SHA256_Init(&ctx);
            SHA256_Update(&ctx, plain.c_str(), plain.size());
            SHA256_Final(reinterpret_cast<unsigned char *>(buffer), &ctx);
            return bin2hex(buffer, SHA256_DIGEST_LENGTH);
        }
        std::string sha256(const char *plain, size_t len)
        {
            char buffer[SHA256_DIGEST_LENGTH];
            SHA256_CTX ctx;
            SHA256_Init(&ctx);
            SHA256_Update(&ctx, plain, len);
            SHA256_Final(reinterpret_cast<unsigned char *>(buffer), &ctx);
            return bin2hex(buffer, SHA256_DIGEST_LENGTH);
        }
        std::string sha512(const std::string &plain)
        {
            char buffer[SHA512_DIGEST_LENGTH];
            SHA512_CTX ctx;
            SHA512_Init(&ctx);
            SHA512_Update(&ctx, plain.c_str(), plain.size());
            SHA512_Final(reinterpret_cast<unsigned char *>(buffer), &ctx);
            return bin2hex(buffer, SHA512_DIGEST_LENGTH);
        }
        std::string sha512(const char *plain, size_t len)
        {
            char buffer[SHA512_DIGEST_LENGTH];
            SHA512_CTX ctx;
            SHA512_Init(&ctx);
            SHA512_Update(&ctx, plain, len);
            SHA512_Final(reinterpret_cast<unsigned char *>(buffer), &ctx);
            return bin2hex(buffer, SHA512_DIGEST_LENGTH);
        }
    } // namespace hash_engine
} // namespace mongols