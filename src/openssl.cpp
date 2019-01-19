
#include "openssl.hpp"
#include "util.hpp"


namespace mongols {

    openssl::version_t openssl::version = openssl::version_t::TLSv12;
    std::string openssl::ciphers = "ECDHE-ECDSA-AES128-SHA256:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:RSA+AES128:!aNULL:!eNULL:!LOW:!ADH:!RC4:!3DES:!MD5:!EXP:!PSK:!SRP:!DSS"; //"AES128-GCM-SHA256";
    long openssl::flags = SSL_OP_NO_COMPRESSION | SSL_OP_NO_TICKET | SSL_OP_CIPHER_SERVER_PREFERENCE | SSL_OP_SINGLE_ECDH_USE | SSL_OP_SINGLE_DH_USE;
    const int openssl::ssl_session_ctx_id = 1;
    bool openssl::enable_verify = false, openssl::enable_cache = true;

    openssl::openssl(const std::string& crt_file, const std::string& key_file
            , openssl::version_t v
            , const std::string& ciphers
            , long flags)
    : ok(false), crt_file(crt_file), key_file(key_file)
    , ctx(0) {
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();
        switch (v) {
            case openssl::version_t::SSLv23:
                this->ctx = SSL_CTX_new(SSLv23_server_method());
                break;
            case openssl::version_t::TLSv12:
                this->ctx = SSL_CTX_new(TLSv1_2_server_method());
                break;
            case openssl::version_t::TLSv13:
                this->ctx = SSL_CTX_new(TLSv1_2_server_method());
                break;
            default:
                this->ctx = SSL_CTX_new(TLSv1_2_server_method());
                break;
        }

        if (this->ctx) {
            SSL_CTX_set_ecdh_auto(this->ctx, 1024);
            this->ctx->freelist_max_len = 0;
            SSL_CTX_set_mode(this->ctx, SSL_MODE_RELEASE_BUFFERS
                    | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER
                    | SSL_MODE_AUTO_RETRY
                    );
            SSL_CTX_set_options(this->ctx, flags);
            SSL_CTX_set_cipher_list(this->ctx, ciphers.c_str());

            if (openssl::enable_cache) {
                SSL_CTX_set_session_cache_mode(this->ctx, SSL_SESS_CACHE_BOTH | SSL_SESS_CACHE_NO_INTERNAL);
                SSL_CTX_sess_set_cache_size(this->ctx, 1);
                SSL_CTX_set_session_id_context(this->ctx, (const unsigned char *) &openssl::ssl_session_ctx_id, sizeof (openssl::ssl_session_ctx_id));
            } else {
                SSL_CTX_set_session_cache_mode(this->ctx, SSL_SESS_CACHE_OFF);
            }


            if (SSL_CTX_use_certificate_file(this->ctx, this->crt_file.c_str(), SSL_FILETYPE_PEM) > 0) {
                if (SSL_CTX_use_PrivateKey_file(this->ctx, this->key_file.c_str(), SSL_FILETYPE_PEM) > 0) {
                    if (SSL_CTX_check_private_key(ctx) > 0) {
                        if (openssl::enable_verify) {
                            SSL_CTX_set_verify(this->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT |
                                    SSL_VERIFY_CLIENT_ONCE, 0);
                        }
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
            bool reconnectioned = false;
            //            SSL_set_accept_state(ssl);
ssl_accept:
            int ret = SSL_accept(ssl);
            if (ret > 0 /*&& SSL_do_handshake(ssl) > 0*/) {
                return true;
            } else {
                int err = SSL_get_error(ssl, ret);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                    if (!reconnectioned) {
                        reconnectioned = true;
                        goto ssl_accept;
                    }
                }
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

    openssl::ssl::ssl(SSL_CTX* ctx) : data(0) {
        this->data = SSL_new(ctx);
        if (this->data) {
            SSL_set_mode(this->data, SSL_MODE_RELEASE_BUFFERS
                    | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER
                    );
        }

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













}
