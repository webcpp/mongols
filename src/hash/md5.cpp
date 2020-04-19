#include "lib/hash/md5.hpp"
extern "C" {
#include "lib/WjCryptLib/WjCryptLib_Md5.h"
}
#include "util.hpp"
#include <cstring>

namespace mongols {

std::string md5(const std::string& str)
{
    std::string res;
    const char hex[] = "0123456789ABCDEF";
    Md5Context md5Context;
    MD5_HASH md5Hash;
    Md5Initialise(&md5Context);
    Md5Update(&md5Context, str.c_str(), str.size());
    Md5Finalise(&md5Context, &md5Hash);
    for (int i = 0; i < MD5_HASH_SIZE; ++i) {
        unsigned char c = static_cast<unsigned char>(md5Hash.bytes[i]);
        res += hex[c >> 4];
        res += hex[c & 0xf];
    }
    return res;
}

md5_engine::md5_engine()
    : ctx()
    , out()
    , buffer(0)
{
    MD5_Init(&this->ctx);
    this->buffer = new char[MD5_DIGEST_LENGTH];
}

md5_engine::~md5_engine()
{
    if (this->buffer) {
        delete[] this->buffer;
    }
}

const std::string& md5_engine::get(const std::string& plain)
{
    memset(this->buffer, 0, MD5_DIGEST_LENGTH);
    MD5_Update(&this->ctx, plain.c_str(), plain.size());
    MD5_Final(reinterpret_cast<unsigned char*>(this->buffer), &this->ctx);
    this->out = std::move(mongols::bin2hex(this->buffer, MD5_DIGEST_LENGTH));
    return this->out;
}

const std::string& md5_engine::get(const char* plain, size_t len)
{
    memset(this->buffer, 0, MD5_DIGEST_LENGTH);
    MD5_Update(&this->ctx, plain, len);
    MD5_Final(reinterpret_cast<unsigned char*>(this->buffer), &this->ctx);
    this->out = std::move(mongols::bin2hex(this->buffer, MD5_DIGEST_LENGTH));
    return this->out;
}

}