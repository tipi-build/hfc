#include <openssl/ssl.h>
#include <openssl/err.h>
#include <iostream>

int main() {
    SSL_library_init();
    SSL_load_error_strings();
    
    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    
    if (ctx == nullptr) {
        ERR_print_errors_fp(stderr);
        return 1;
    }
    
    std::cout << "OpenSSL initialized successfully" << std::endl;
    std::cout << "OpenSSL version: " << OpenSSL_version(OPENSSL_VERSION) << std::endl;
    
    SSL_CTX_free(ctx);
    return 0;
}
