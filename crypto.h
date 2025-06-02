#ifndef CRYPTO_H
#define CRYPTO_H

#include <sodium.h>

#define CRYPTO_SUCCESS 1
#define CRYPTO_ERROR_FILE -1
#define CRYPTO_ERROR_MEM -2
#define CRYPTO_ERROR_ENC -3
#define CRYPTO_ERROR_DEC -4

int crypto_encrypt_file(const char *src, const char *dest,
                        const char *password);
int crypto_decrypt_file(const char *src, const char *dest,
                        const char *password);
int crypto_derive_key(unsigned char *key, size_t key_len, const char *password,
                      const unsigned char *salt);

#endif
