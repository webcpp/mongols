#ifndef A110EE56_3466_4773_A8CC_D9432C2A814B
#define A110EE56_3466_4773_A8CC_D9432C2A814B

#include <iostream>
#include <regex.h>
#include <string>
#include <vector>

namespace mongols {

class posix_regex {
public:
    static int flags;

public:
    posix_regex() = delete;

    posix_regex(const std::string& pattern);

    virtual ~posix_regex();

    bool match(const std::string& subject, std::vector<std::string>& matches, size_t n = 30);

private:
    regex_t reg;
    bool ok;
};
}

#endif /* A110EE56_3466_4773_A8CC_D9432C2A814B */
