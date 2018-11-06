#include "lib/hash/md5.hpp"
extern "C" {
#include "lib/WjCryptLib/WjCryptLib_Md5.h"
}


namespace mongols {

    std::string md5(const std::string str) {
        std::string res;
        const char hex[] = "0123456789ABCDEF";
        Md5Context md5Context;
        MD5_HASH md5Hash;
        Md5Initialise(&md5Context);
        Md5Update(&md5Context, str.c_str(), str.size());
        Md5Finalise(&md5Context, &md5Hash);
        for (int i = 0; i < MD5_HASH_SIZE; ++i) {
            unsigned char c = static_cast<unsigned char> (md5Hash.bytes[i]);
            res += hex[c >> 4];
            res += hex[c & 0xf];
        }
        return res;
    }

}