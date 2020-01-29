#include "lib/hash/sha256.hpp"
extern "C" {
#include "lib/WjCryptLib/WjCryptLib_Sha256.h"
}

namespace mongols {

std::string sha256(const std::string& str)
{
    std::string res;
    const char hex[] = "0123456789ABCDEF";
    Sha256Context Sha256Context;
    SHA256_HASH Sha256Hash;
    Sha256Initialise(&Sha256Context);
    Sha256Update(&Sha256Context, str.c_str(), str.size());
    Sha256Finalise(&Sha256Context, &Sha256Hash);
    for (int i = 0; i < SHA256_HASH_SIZE; ++i) {
        unsigned char c = static_cast<unsigned char>(Sha256Hash.bytes[i]);
        res += hex[c >> 4];
        res += hex[c & 0xf];
    }
    return res;
}
}