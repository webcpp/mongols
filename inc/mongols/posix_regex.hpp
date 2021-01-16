#pragma once

#include <iostream>
#include <regex.h>
#include <string>
#include <vector>

namespace mongols
{

    class posix_regex
    {
    public:
        static int flags;

    public:
        posix_regex() = delete;

        posix_regex(const std::string &pattern);

        virtual ~posix_regex();

        bool match(const std::string &subject, std::vector<std::string> &matches, size_t n = 30);

    private:
        regex_t reg;
        bool ok;
    };
} // namespace mongols
