#include "lib/hash/hash_engine.hpp"
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <string.h>

namespace mongols {

std::string hash_engine::bin2hex(const std::string& input)
{
    std::string res;
    const char hex[] = "0123456789ABCDEF";
    for (auto& sc : input) {
        unsigned char c = static_cast<unsigned char>(sc);
        res += hex[c >> 4];
        res += hex[c & 0xf];
    }

    return res;
}

std::string hash_engine::bin2hex(const char* input, size_t len)
{
    std::string res;
    const char hex[] = "0123456789ABCDEF";
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        res += hex[c >> 4];
        res += hex[c & 0xf];
    }

    return res;
}
hash_engine::hash_engine(hash_t t)
    : t(t)
    , DIGEST_LENGTH(0)
    , ctx(0)
    , out()
    , buffer(0)
{
    if (this->t == hash_t::MD5) {
        this->DIGEST_LENGTH = MD5_DIGEST_LENGTH;
        this->ctx = malloc(sizeof(MD5_CTX));
        MD5_Init(reinterpret_cast<MD5_CTX*>(this->ctx));
        this->buffer = new char[this->DIGEST_LENGTH];
    } else if (this->t == hash_t::SHA1) {
        this->DIGEST_LENGTH = SHA_DIGEST_LENGTH;
        this->ctx = malloc(sizeof(SHA_CTX));
        SHA1_Init(reinterpret_cast<SHA_CTX*>(this->ctx));
        this->buffer = new char[this->DIGEST_LENGTH];

    } else if (this->t == hash_t::SHA256) {
        this->DIGEST_LENGTH = SHA256_DIGEST_LENGTH;
        this->ctx = malloc(sizeof(SHA256_CTX));
        SHA256_Init(reinterpret_cast<SHA256_CTX*>(this->ctx));
        this->buffer = new char[this->DIGEST_LENGTH];
    } else if (this->t == hash_t::SHA512) {
        this->DIGEST_LENGTH = SHA512_DIGEST_LENGTH;
        this->ctx = malloc(sizeof(SHA512_CTX));
        SHA512_Init(reinterpret_cast<SHA512_CTX*>(this->ctx));
        this->buffer = new char[this->DIGEST_LENGTH];
    } else {
        perror("init error");
    }
}

hash_engine::~hash_engine()
{
    if (this->ctx) {
        free(this->ctx);
    }
    if (this->buffer) {
        delete[] this->buffer;
    }
}

const std::string& hash_engine::get(const std::string& plain)
{
    if (this->t == hash_t::MD5) {
        memset(this->buffer, 0, this->DIGEST_LENGTH);
        MD5_Update(reinterpret_cast<MD5_CTX*>(this->ctx), plain.c_str(), plain.size());
        MD5_Final(reinterpret_cast<unsigned char*>(this->buffer), reinterpret_cast<MD5_CTX*>(this->ctx));
    } else if (this->t == hash_t::SHA1) {
        memset(this->buffer, 0, this->DIGEST_LENGTH);
        SHA1_Update(reinterpret_cast<SHA_CTX*>(this->ctx), plain.c_str(), plain.size());
        SHA1_Final(reinterpret_cast<unsigned char*>(this->buffer), reinterpret_cast<SHA_CTX*>(this->ctx));
    } else if (this->t == hash_t::SHA256) {
        memset(this->buffer, 0, this->DIGEST_LENGTH);
        SHA256_Update(reinterpret_cast<SHA256_CTX*>(this->ctx), plain.c_str(), plain.size());
        SHA256_Final(reinterpret_cast<unsigned char*>(this->buffer), reinterpret_cast<SHA256_CTX*>(this->ctx));
    } else if (this->t == hash_t::SHA512) {
        memset(this->buffer, 0, this->DIGEST_LENGTH);
        SHA512_Update(reinterpret_cast<SHA512_CTX*>(this->ctx), plain.c_str(), plain.size());
        SHA512_Final(reinterpret_cast<unsigned char*>(this->buffer), reinterpret_cast<SHA512_CTX*>(this->ctx));
    }
    this->out = std::move(hash_engine::bin2hex(this->buffer, this->DIGEST_LENGTH));
    return this->out;
}

const std::string& hash_engine::get(const char* plain, size_t len)
{
    if (this->t == hash_t::MD5) {
        memset(this->buffer, 0, this->DIGEST_LENGTH);
        MD5_Update(reinterpret_cast<MD5_CTX*>(this->ctx), plain, len);
        MD5_Final(reinterpret_cast<unsigned char*>(this->buffer), reinterpret_cast<MD5_CTX*>(this->ctx));
    } else if (this->t == hash_t::SHA1) {
        memset(this->buffer, 0, this->DIGEST_LENGTH);
        SHA1_Update(reinterpret_cast<SHA_CTX*>(this->ctx), plain, len);
        SHA1_Final(reinterpret_cast<unsigned char*>(this->buffer), reinterpret_cast<SHA_CTX*>(this->ctx));
    } else if (this->t == hash_t::SHA256) {
        memset(this->buffer, 0, this->DIGEST_LENGTH);
        SHA256_Update(reinterpret_cast<SHA256_CTX*>(this->ctx), plain, len);
        SHA256_Final(reinterpret_cast<unsigned char*>(this->buffer), reinterpret_cast<SHA256_CTX*>(this->ctx));
    } else if (this->t == hash_t::SHA512) {
        memset(this->buffer, 0, this->DIGEST_LENGTH);
        SHA512_Update(reinterpret_cast<SHA512_CTX*>(this->ctx), plain, len);
        SHA512_Final(reinterpret_cast<unsigned char*>(this->buffer), reinterpret_cast<SHA512_CTX*>(this->ctx));
    }
    this->out = std::move(hash_engine::bin2hex(this->buffer, this->DIGEST_LENGTH));
    return this->out;
}

}