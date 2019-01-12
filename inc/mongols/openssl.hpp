#ifndef OPENSSL_HPP
#define OPENSSL_HPP



#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>


#include <string>
#include <unordered_map>
#include <memory>
#include <ctime>

namespace mongols {

    class openssl {
    public:
        static std::string ciphers;
        static long flags;
    public:

        class ssl {
        public:
            ssl() = delete;
            ssl(SSL_CTX*);
            virtual~ssl();
            SSL* get_ssl();
            const time_t& get_time()const;
        private:
            SSL* data;
            time_t t;
        };

        enum version {
            SSLv23
            , TSLv1
            , TSLv11
            , TLSv12
            , TLSv13
        };
    public:
        openssl() = delete;
        virtual~openssl();

        openssl(const std::string&, const std::string&, openssl::version = openssl::version::TLSv12);
        bool set_socket_and_accept(SSL*, int);
        int read(SSL*, char*, size_t);
        int write(SSL*, const std::string&);
        bool is_ok()const;
        SSL_CTX* get_ctx();
    private:
        bool ok;
        std::string crt_file, key_file;
        SSL_CTX *ctx;
    };
}


#endif /* OPENSSL_HPP */

