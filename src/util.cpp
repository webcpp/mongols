#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>

#include <openssl/evp.h>  
#include <openssl/bio.h>  
#include <openssl/buffer.h> 
#include <openssl/sha.h>
#include <openssl/md5.h>

#include <string>
#include <cstring>
#include <fstream>



#include "util.hpp"




namespace mongols {

    std::string md5(const std::string& str) {
        unsigned char digest[16] = {0};
        MD5_CTX ctx;
        MD5_Init(&ctx);
        MD5_Update(&ctx, str.c_str(), str.size());
        MD5_Final(digest, &ctx);

        unsigned char tmp[32] = {0}, *dst = &tmp[0], *src = &digest[0];
        unsigned char hex[] = "0123456789abcdef";
        int len = 16;
        while (len--) {
            *dst++ = hex[*src >> 4];
            *dst++ = hex[*src++ & 0xf];
        }

        return std::string((char*) tmp, 32);
    }

    std::string random_string(const std::string& s) {
        time_t now = time(NULL);
        char* now_str = ctime(&now);
        return mongols::md5(s + now_str);
    }

    void read_file(const std::string& path, std::string& out) {
        std::ifstream fs(path, std::ios_base::binary);
        fs.seekg(0, std::ios_base::end);
        auto size = fs.tellg();
        fs.seekg(0);
        out.resize(static_cast<size_t> (size));
        fs.read(&out[0], size);
    }

    bool is_file(const std::string& s) {
        struct stat st;
        return stat(s.c_str(), &st) >= 0 && S_ISREG(st.st_mode);
    }

    bool is_dir(const std::string& s) {
        struct stat st;
        return stat(s.c_str(), &st) >= 0 && S_ISDIR(st.st_mode);
    }

    static unsigned mday[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    std::string http_time(time_t *t) {
        struct tm * timeinfo = gmtime(t);
        char buffer [80] = {0};
        size_t n = strftime(buffer, 80, "%a, %d %b %Y %T GMT", timeinfo);
        return std::string(buffer, n);
    }

    time_t parse_http_time(u_char* value, size_t len) {
        u_char *p, *end;
        int month;
        uint day = 0, year = 0, hour = 0, min = 0, sec = 0;
        uint64_t time = 0;

        enum {
            no = 0,
            rfc822, /* Tue, 10 Nov 2002 23:50:13   */
            rfc850, /* Tuesday, 10-Dec-02 23:50:13 */
            isoc /* Tue Dec 10 23:50:13 2002    */
        } fmt;

        fmt = no;
        end = value + len;


        for (p = value; p < end; p++) {
            if (*p == ',') {
                break;
            }

            if (*p == ' ') {
                fmt = isoc;
                break;
            }
        }

        for (p++; p < end; p++) {
            if (*p != ' ') {
                break;
            }
        }

        if (end - p < 18) {
            return -1;
        }

        if (fmt != isoc) {
            if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9') {
                return -1;
            }

            day = (*p - '0') * 10 + (*(p + 1) - '0');
            p += 2;

            if (*p == ' ') {
                if (end - p < 18) {
                    return -1;
                }
                fmt = rfc822;

            } else if (*p == '-') {
                fmt = rfc850;

            } else {
                return -1;
            }

            p++;
        }

        switch (*p) {

            case 'J':
                month = *(p + 1) == 'a' ? 0 : *(p + 2) == 'n' ? 5 : 6;
                break;

            case 'F':
                month = 1;
                break;

            case 'M':
                month = *(p + 2) == 'r' ? 2 : 4;
                break;

            case 'A':
                month = *(p + 1) == 'p' ? 3 : 7;
                break;

            case 'S':
                month = 8;
                break;

            case 'O':
                month = 9;
                break;

            case 'N':
                month = 10;
                break;

            case 'D':
                month = 11;
                break;

            default:
                return -1;
        }

        p += 3;

        if ((fmt == rfc822 && *p != ' ') || (fmt == rfc850 && *p != '-')) {
            return -1;
        }

        p++;

        if (fmt == rfc822) {
            if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9'
                    || *(p + 2) < '0' || *(p + 2) > '9'
                    || *(p + 3) < '0' || *(p + 3) > '9') {
                return -1;
            }

            year = (*p - '0') * 1000 + (*(p + 1) - '0') * 100
                    + (*(p + 2) - '0') * 10 + (*(p + 3) - '0');
            p += 4;

        } else if (fmt == rfc850) {
            if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9') {
                return -1;
            }

            year = (*p - '0') * 10 + (*(p + 1) - '0');
            year += (year < 70) ? 2000 : 1900;
            p += 2;
        }

        if (fmt == isoc) {
            if (*p == ' ') {
                p++;
            }

            if (*p < '0' || *p > '9') {
                return -1;
            }

            day = *p++ -'0';

            if (*p != ' ') {
                if (*p < '0' || *p > '9') {
                    return -1;
                }

                day = day * 10 + (*p++ -'0');
            }

            if (end - p < 14) {
                return -1;
            }
        }

        if (*p++ != ' ') {
            return -1;
        }

        if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9') {
            return -1;
        }

        hour = (*p - '0') * 10 + (*(p + 1) - '0');
        p += 2;

        if (*p++ != ':') {
            return -1;
        }

        if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9') {
            return -1;
        }

        min = (*p - '0') * 10 + (*(p + 1) - '0');
        p += 2;

        if (*p++ != ':') {
            return -1;
        }

        if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9') {
            return -1;
        }

        sec = (*p - '0') * 10 + (*(p + 1) - '0');

        if (fmt == isoc) {
            p += 2;

            if (*p++ != ' ') {
                return -1;
            }

            if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9'
                    || *(p + 2) < '0' || *(p + 2) > '9'
                    || *(p + 3) < '0' || *(p + 3) > '9') {
                return -1;
            }

            year = (*p - '0') * 1000 + (*(p + 1) - '0') * 100
                    + (*(p + 2) - '0') * 10 + (*(p + 3) - '0');
        }

        if (hour > 23 || min > 59 || sec > 59) {
            return -1;
        }

        if (day == 29 && month == 1) {
            if ((year & 3) || ((year % 100 == 0) && (year % 400) != 0)) {
                return -1;
            }

        } else if (day > mday[month]) {
            return -1;
        }

        /*
         * shift new year to March 1 and start months from 1 (not 0),
         * it is needed for Gauss' formula
         */

        if (--month <= 0) {
            month += 12;
            year -= 1;
        }

        /* Gauss' formula for Gregorian days since March 1, 1 BC */

        time = (uint64_t) (
                /* days in years including leap years since March 1, 1 BC */

                365 * year + year / 4 - year / 100 + year / 400

                /* days before the month */

                + 367 * month / 12 - 30

                /* days before the day */

                + day - 1

                /*
                 * 719527 days were between March 1, 1 BC and March 1, 1970,
                 * 31 and 28 days were in January and February 1970
                 */

                - 719527 + 31 + 28) * 86400 + hour * 3600 + min * 60 + sec;

        return (time_t) time;
    }

    void trim(std::string& s) {
        if (!s.empty()) {
            s.erase(0, s.find_first_not_of(" "));
            s.erase(s.find_last_not_of(" ") + 1);
        }
    }

    std::string trim(const std::string& s) {
        std::string ret(s);
        trim(ret);
        return ret;
    }

    void parse_param(const std::string& data, std::unordered_map<std::string, std::string>& result, char c, char cc) {
        if (data.empty())return;
        size_t start = 0, p, q;
        while (true) {
            p = data.find(c, start);
            if (p == std::string::npos) {
                q = data.find(cc, start);
                if (q != std::string::npos) {
                    result[std::move(trim(data.substr(start, q - start)))] = std::move(trim(data.substr(q + 1)));
                }
                break;
            } else {
                q = data.find(cc, start);
                if (q != std::string::npos) {
                    result[std::move(trim(data.substr(start, q - start)))] = std::move(trim(data.substr(q + 1, p - q - 1)));
                }
                start = p + 1;
            }
        }
    }

    void split(const std::string& s, char delim, std::vector<std::string>& v) {
        auto i = 0;
        auto pos = s.find(delim);
        std::string tmp;
        while (pos != std::string::npos) {
            tmp = std::move(s.substr(i, pos - i));
            if (!tmp.empty()) {
                v.push_back(std::move(tmp));
            }
            i = ++pos;
            pos = s.find(delim, pos);
            if (pos == std::string::npos) {
                tmp = std::move(s.substr(i));
                if (!tmp.empty()) {
                    v.push_back(std::move(tmp));
                }
            }
        }
    }

    void split(const std::string& s, const std::string& delim, std::vector<std::string>& v) {
        size_t last = 0;
        size_t index = s.find_first_of(delim, last);
        std::string tmp;
        while (index != std::string::npos) {
            tmp = std::move(s.substr(last, index - last));
            if (!tmp.empty()) {
                v.push_back(std::move(tmp));
            }
            last = index + 1;
            index = s.find_first_of(delim, last);
        }
        if (index - last > 0) {
            tmp = std::move(s.substr(last, index - last));
            if (!tmp.empty()) {
                v.push_back(std::move(tmp));
            }
        }
    }

    std::string regular_expression::INTEGER = R"(^[+-]?[1-9]+[0-9]*$)"
            , regular_expression::NUMBER = R"(^[+-]?[1-9]+[0-9]*\.?[0-9]*$)"
            , regular_expression::EMAIL = R"(^[0-9a-zA-Z]+(([-_\.])?[0-9a-zA-Z]+)?\@[0-9a-zA-Z]+[-_]?[0-9a-zA-Z]+(\.[0-9a-zA-Z]+)+$)"
            , regular_expression::URL = R"(^(http[s]?|ftp)://[0-9a-zA-Z\._-]([0-9a-zA-Z]+/?)+\??.*$)";

    std::string base64_encode(const std::string& str, bool newline) {
        const char* buffer = str.c_str();
        size_t length = str.size();
        BIO *bmem = NULL;
        BIO *b64 = NULL;
        BUF_MEM *bptr;

        b64 = BIO_new(BIO_f_base64());
        if (!newline) {
            BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        }
        bmem = BIO_new(BIO_s_mem());
        b64 = BIO_push(b64, bmem);
        BIO_write(b64, buffer, length);
        BIO_flush(b64);
        BIO_get_mem_ptr(b64, &bptr);
        BIO_set_close(b64, BIO_NOCLOSE);

        char buff[bptr->length + 1];
        memcpy(buff, bptr->data, bptr->length);
        buff[bptr->length] = 0;
        BIO_free_all(b64);

        return buff;
    }

    std::string base64_decode(const std::string& str, bool newline) {
        const char* input = str.c_str();
        size_t length = str.size();
        BIO *b64 = NULL;
        BIO *bmem = NULL;
        char buffer[length];
        memset(buffer, 0, length);
        b64 = BIO_new(BIO_f_base64());
        if (!newline) {
            BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        }
        bmem = BIO_new_mem_buf(input, length);
        bmem = BIO_push(b64, bmem);
        BIO_read(bmem, buffer, length);
        BIO_free_all(bmem);

        return buffer;
    }

    std::string sha1(const std::string& str) {
        SHA_CTX ctx;
        SHA1_Init(&ctx);
        SHA1_Update(&ctx, str.c_str(), str.size());
        unsigned char md[SHA_DIGEST_LENGTH];
        SHA1_Final(md, &ctx);
        return std::string((char*) md, SHA_DIGEST_LENGTH);
    }

    std::string bin2hex(const std::string& input) {
        std::string res;
        const char hex[] = "0123456789ABCDEF";
        for (auto sc : input) {
            unsigned char c = static_cast<unsigned char> (sc);
            res += hex[c >> 4];
            res += hex[c & 0xf];
        }

        return res;
    }

    std::string url_encode(const std::string& str) {
        std::string new_str;
        ;
        char c;
        int ic;
        const char* chars = str.c_str();
        char bufHex[10];
        int len = strlen(chars);

        for (int i = 0; i < len; i++) {
            c = chars[i];
            ic = c;
            if (c == ' ') new_str += '+';
            else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') new_str += c;
            else {
                sprintf(bufHex, "%X", c);
                if (ic < 16)
                    new_str += "%0";
                else
                    new_str += "%";
                new_str += bufHex;
            }
        }
        return new_str;
    }

    std::string url_decode(const std::string& str) {
        std::string ret;
        char ch;
        int i, ii, len = str.length();

        for (i = 0; i < len; i++) {
            if (str[i] != '%') {
                if (str[i] == '+')
                    ret += ' ';
                else
                    ret += str[i];
            } else {
                sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
                ch = static_cast<char> (ii);
                ret += ch;
                i = i + 2;
            }
        }
        return ret;
    }

}