#include "posix_regex.hpp"

namespace mongols
{

    int posix_regex::flags = REG_EXTENDED | REG_NOSUB;
    posix_regex::posix_regex(const std::string &pattern)
        : reg(), ok(false)
    {
        this->ok = (regcomp(&this->reg, pattern.c_str(), posix_regex::flags) == 0);
    }

    posix_regex::~posix_regex()
    {
        if (this->ok)
        {
            regfree(&this->reg);
        }
    }

    bool posix_regex::match(const std::string &subject, std::vector<std::string> &matches, size_t n)
    {
        bool result = false;
        if (n > 1)
        {
            regmatch_t m[n];
            if (this->ok && regexec(&this->reg, subject.c_str(), n, m, 0) == REG_NOERROR)
            {
                result = true;
                if (posix_regex::flags & REG_NOSUB)
                {
                    return result;
                }
                for (size_t i = 0, len = 0; i < this->reg.re_nsub; ++i)
                {
                    len = m[i].rm_eo - m[i].rm_so;
                    matches.emplace_back(std::move(subject.substr(m[i].rm_so, len)));
                }
            }
        }

        return result;
    }
} // namespace mongols