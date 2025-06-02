#include "crypto.h"
#include <stdio.h>
#include <string.h>

#define CHUNK_SIZE 4096
#define AAD_STRING (const unsigned char *)"ZmlsZWNyeXB0aW9u"
#define AAD_STRING_LEN 16

int crypto_derive_key(unsigned char *key, size_t key_len, const char *password,
                      const unsigned char *salt) {
  return crypto_pwhash(key, key_len, password, strlen(password), salt,
                       crypto_pwhash_OPSLIMIT_MODERATE,
                       crypto_pwhash_MEMLIMIT_MODERATE,
                       crypto_pwhash_ALG_DEFAULT);
}

int crypto_encrypt_file(const char *src, const char *dest,
                        const char *password) {
  FILE *src_file = fopen(src, "rb");
  if (!src_file) {
    // perror("Error opening source file for encryption");
    return CRYPTO_ERROR_FILE;
  }

  FILE *dest_file = fopen(dest, "wb");
  if (!dest_file) {
    fclose(src_file);
    // perror("Error opening destination file for encryption");
    return CRYPTO_ERROR_FILE;
  }

  unsigned char salt[crypto_pwhash_SALTBYTES];
  randombytes_buf(salt, sizeof(salt));

  unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
  if (crypto_derive_key(key, sizeof(key), password, salt) != 0) {
    fclose(dest_file);
    fclose(src_file);
    // perror("Unable to derive key");
    return CRYPTO_ERROR_ENC; // unable to derive key
  }

  fwrite(salt, 1, sizeof(salt), dest_file);

  crypto_secretstream_xchacha20poly1305_state state;
  unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
  crypto_secretstream_xchacha20poly1305_init_push(&state, header, key);
  fwrite(header, 1, sizeof(header), dest_file);

  unsigned char input_buffer[CHUNK_SIZE];
  unsigned char
      output_buffer[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
  unsigned long long encrypted_chunk_len;
  size_t bytes_read;
  int eof;
  unsigned char tag; // tag to mark the chunks
  do {
    bytes_read = fread(input_buffer, 1, sizeof(input_buffer), src_file);
    eof = feof(src_file);

    tag = eof ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;

    crypto_secretstream_xchacha20poly1305_push(
        &state, output_buffer, &encrypted_chunk_len, input_buffer, bytes_read,
        AAD_STRING, AAD_STRING_LEN, tag);

    fwrite(output_buffer, 1, (size_t)encrypted_chunk_len, dest_file);
  } while (!eof);

  fclose(src_file);
  fclose(dest_file);

  sodium_memzero(key, sizeof(key));
  return CRYPTO_SUCCESS;
}

int crypto_decrypt_file(const char *src, const char *dest,
                        const char *password) {
  FILE *src_file = fopen(src, "rb");
  if (!src_file) {
    // perror("Error opening source file for decryption");
    return CRYPTO_ERROR_FILE;
  }

  FILE *dest_file = fopen(dest, "wb");
  if (!dest_file) {
    fclose(src_file);
    // perror("Error opening destination file for decryption");
    return CRYPTO_ERROR_FILE;
  }

  unsigned char salt[crypto_pwhash_SALTBYTES];
  if (fread(salt, 1, sizeof(salt), src_file) != sizeof(salt)) {
    fclose(src_file);
    fclose(dest_file);
    // perror("Incomplete salt in source file, corruption");
    return CRYPTO_ERROR_DEC; // incomplete salt
  }

  unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
  if (crypto_derive_key(key, sizeof(key), password, salt) != 0) {
    fclose(dest_file);
    fclose(src_file);
    // perror("Unable to derive key");
    return CRYPTO_ERROR_DEC; // unable to derive key
  }

  unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
  if (fread(header, 1, sizeof(header), src_file) != sizeof(header)) {
    fclose(src_file);
    fclose(dest_file);
    // perror("Incomplete header in source file, corruption");
    return CRYPTO_ERROR_DEC; // incomplete header
  }

  crypto_secretstream_xchacha20poly1305_state state;
  if (crypto_secretstream_xchacha20poly1305_init_pull(&state, header, key) !=
      0) {
    fclose(src_file);
    fclose(dest_file);
    // perror("Corrupted header in source file");
    return CRYPTO_ERROR_DEC; // corrupted header
  }

  unsigned char
      input_buffer[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
  unsigned char output_buffer[CHUNK_SIZE];

  unsigned long long decrypted_chunk_len;
  size_t bytes_read;
  int eof;
  unsigned char tag;

  do {
    bytes_read = fread(input_buffer, 1, sizeof(input_buffer), src_file);
    eof = feof(src_file);

    tag = eof ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;
    if (crypto_secretstream_xchacha20poly1305_pull(
            &state, output_buffer, &decrypted_chunk_len, &tag, input_buffer,
            bytes_read, AAD_STRING, AAD_STRING_LEN) != 0) {
      fclose(src_file);
      fclose(dest_file);
      // perror("Corrupted chunk in file");
      return CRYPTO_ERROR_DEC; // corrupted chunk
    }

    if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL && !eof) {
      fclose(src_file);
      fclose(dest_file);
      // perror("End of stream reached before end of file");
      return CRYPTO_ERROR_DEC; // end of stream before the end of the file
    } else if (tag != crypto_secretstream_xchacha20poly1305_TAG_FINAL && eof) {
      fclose(src_file);
      fclose(dest_file);
      // perror("End of file reached before end of stream");
      return CRYPTO_ERROR_DEC; // end of file before the end of the stream
    }

    fwrite(output_buffer, 1, (size_t)decrypted_chunk_len, dest_file);
  } while (!eof);

  fclose(src_file);
  fclose(dest_file);

  sodium_memzero(key, sizeof(key));
  return CRYPTO_SUCCESS;
}
