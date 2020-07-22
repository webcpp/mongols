#ifndef CC2A4155_173C_4E4D_A8D2_4C4FFF952BDC
#define CC2A4155_173C_4E4D_A8D2_4C4FFF952BDC

#include <string>
#include <sys/stat.h>
#include <unordered_map>
#include <utility>

namespace mongols {
class file_mmap {
public:
    file_mmap();
    virtual ~file_mmap();
    bool get(const std::string&, std::pair<char*, struct stat>&);
    std::pair<char*, struct stat> get(const std::string&);

private:
    std::unordered_map<std::string, std::pair<char*, struct stat>> data;
};
}

#endif /* CC2A4155_173C_4E4D_A8D2_4C4FFF952BDC */
