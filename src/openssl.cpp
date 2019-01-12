
#include "openssl.hpp"



namespace mongols {

    std::string openssl::ciphers = "AES128-GCM-SHA256";

    long openssl::flags = SSL_OP_NO_COMPRESSION;

    openssl::openssl(const std::string& crt_file, const std::string& key_file, openssl::version v)
    : ok(false), crt_file(crt_file), key_file(key_file)
    , ctx(0) {
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();
        switch (v) {
            case openssl::version::SSLv23:
                this->ctx = SSL_CTX_new(SSLv23_server_method());
                break;
            case openssl::version::TLSv12:
                this->ctx = SSL_CTX_new(TLSv1_2_server_method());
                break;
            case openssl::version::TLSv13:
                this->ctx = SSL_CTX_new(TLSv1_2_server_method());
                break;
            default:
                this->ctx = SSL_CTX_new(TLSv1_2_server_method());
                break;
        }

        if (this->ctx) {
            SSL_CTX_set_ecdh_auto(this->ctx, 1);
            SSL_CTX_set_options(this->ctx, openssl::flags);
            SSL_CTX_set_cipher_list(this->ctx, openssl::ciphers.c_str());

            if (SSL_CTX_use_certificate_file(this->ctx, this->crt_file.c_str(), SSL_FILETYPE_PEM) > 0) {
                if (SSL_CTX_use_PrivateKey_file(this->ctx, this->key_file.c_str(), SSL_FILETYPE_PEM) > 0) {
                    if (SSL_CTX_check_private_key(ctx) > 0) {
                        //                        SSL_CTX_set_verify(this->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT |
                        //                                SSL_VERIFY_CLIENT_ONCE, 0);
                        this->ok = true;
                    }
                }
            }

        }
    }

    openssl::~openssl() {
        if (this->ctx) {
            SSL_CTX_free(this->ctx);
        }
        EVP_cleanup();
    }

    bool openssl::is_ok() const {
        return this->ok;
    }

    SSL_CTX* openssl::get_ctx() {
        return this->ctx;
    }

    bool openssl::set_socket_and_accept(SSL* ssl, int fd) {
        if (SSL_set_fd(ssl, fd)) {
            SSL_set_accept_state(ssl);
            if (SSL_accept(ssl) > 0 && SSL_do_handshake(ssl) > 0) {
                return true;
            }
        }
        return false;
    }

    int openssl::read(SSL* ssl, char* buffer, size_t len) {
        ERR_clear_error();
        return SSL_read(ssl, buffer, len);
    }

    int openssl::write(SSL* ssl, const std::string& output) {
        ERR_clear_error();
        return SSL_write(ssl, output.c_str(), output.size());
    }

    openssl::ssl::ssl(SSL_CTX* ctx) : data(0), t(time(0)) {
        this->data = SSL_new(ctx);
    }

    openssl::ssl::~ssl() {
        if (this->data) {
            SSL_shutdown(this->data);
            SSL_free(this->data);
        }
    }

    SSL* openssl::ssl::get_ssl() {
        return this->data;
    }

    const time_t& openssl::ssl::get_time() const {
        return this->t;
    }











}
