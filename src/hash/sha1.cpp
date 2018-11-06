#include "lib/hash/sha1.hpp"
extern "C" {
#include "lib/WjCryptLib/WjCryptLib_Sha1.h"
}

namespace mongols {

    std::string sha1(const std::string &str) {
        std::string res;
        const char hex[] = "0123456789ABCDEF";
        Sha1Context sha1Context;
        SHA1_HASH sha1Hash;
        Sha1Initialise(&sha1Context);
        Sha1Update(&sha1Context, str.c_str(), str.size());
        Sha1Finalise(&sha1Context, &sha1Hash);
        for (int i = 0; i < SHA1_HASH_SIZE; ++i) {
            unsigned char c = static_cast<unsigned char> (sha1Hash.bytes[i]);
            res += hex[c >> 4];
            res += hex[c & 0xf];
        }
        return res;
    }
}