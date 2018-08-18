#ifndef POSIX_REGEX_HPP
#define POSIX_REGEX_HPP

#include <regex.h>
#include <string>
#include <list>

namespace mongols {

    class posix_regex {
    public:

        posix_regex() = delete;

        posix_regex(const std::string& pattern) : reg(), ok(false) {
            this->ok = (regcomp(&this->reg, pattern.c_str(), REG_EXTENDED) == 0);
        }

        virtual~posix_regex() {
            if (this->ok)regfree(&this->reg);
        }

        void match_all(const std::string& subject, std::list<std::string>& matches, size_t n = 30) {
            std::string tmp = subject;
            size_t len = subject.size();
            while (this->match(tmp, matches, n)) {
                const std::list<std::string>::const_reference item = matches.back();
                std::string::size_type p = subject.find(item) + item.size();
                if (p + 1 < len) {
                    tmp = std::move(subject.substr(p + 1));
                } else {
                    break;
                }
            }

        }

        bool match(const std::string& subject, std::list<std::string>& matches, size_t n = 30) {
            bool result = false;
            if (n > 1) {
                regmatch_t m[n];
                if (this->ok && regexec(&this->reg, subject.c_str(), n, m, 0) == 0) {
                    result = true;
                    for (size_t i = 0, len = 0; i < n && m[i].rm_so != -1; ++i) {
                        len = m[i].rm_eo - m[i].rm_so;
                        matches.push_back(std::move(subject.substr(m[i].rm_so, len)));
                    }
                }
            }

            return result;
        }
    private:
        regex_t reg;
        bool ok;
    };
}


#endif /* POSIX_REGEX_HPP */

