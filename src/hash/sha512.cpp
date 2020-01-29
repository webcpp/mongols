#include "lib/hash/sha512.hpp"
extern "C" {
#include "lib/WjCryptLib/WjCryptLib_Sha512.h"
}

namespace mongols {

std::string sha512(const std::string& str)
{
    std::string res;
    const char hex[] = "0123456789ABCDEF";
    Sha512Context Sha512Context;
    SHA512_HASH Sha512Hash;
    Sha512Initialise(&Sha512Context);
    Sha512Update(&Sha512Context, str.c_str(), str.size());
    Sha512Finalise(&Sha512Context, &Sha512Hash);
    for (int i = 0; i < SHA512_HASH_SIZE; ++i) {
        unsigned char c = static_cast<unsigned char>(Sha512Hash.bytes[i]);
        res += hex[c >> 4];
        res += hex[c & 0xf];
    }
    return res;
}
}