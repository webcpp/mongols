#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include <ctime>
#include <memory>
#include <string>
#include <unordered_map>

namespace mongols
{

    class openssl
    {
    public:
        class ssl
        {
        public:
            ssl() = delete;
            ssl(SSL_CTX *);
            virtual ~ssl();
            SSL *get_ssl();
            BIO *get_rbio();
            BIO *get_wbio();
            void set_rbio(BIO *);
            void set_wbio(BIO *);
            void set_rbio();
            void set_wbio();
            void set_bio(BIO *);
            void set_bio();
            void bind_bio();

        private:
            SSL *data;
            BIO *rbio, *wbio;
        };

        enum version_t
        {
            SSLv23 = 0,
            TLSv12 = 1,
            TLSv13 = 2,
            DTLS = 3
        };

    public:
        static openssl::version_t version;
        static std::string ciphers;
        static long flags;
        static bool enable_verify, enable_cache;

    private:
        static const int ssl_session_ctx_id;

    public:
        openssl() = delete;
        virtual ~openssl();

        openssl(const std::string &, const std::string &, openssl::version_t = openssl::version_t::TLSv12, const std::string &ciphers = openssl::ciphers, long flags = openssl::flags);
        bool set_socket_and_accept(SSL *, int);
        int read(SSL *, char *, size_t);
        int write(SSL *, const std::string &);
        bool is_ok() const;
        SSL_CTX *get_ctx();
        mongols::openssl::version_t get_version() const;

    private:
        bool ok;
        std::string crt_file, key_file;
        SSL_CTX *ctx;
        version_t v;
    };
} // namespace mongols
