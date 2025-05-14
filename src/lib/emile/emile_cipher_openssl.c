/**
 * @file
 * @brief Emile cipher functions implementation using OpenSSL.
 *
 * This file provides the implementation for various cryptographic operations
 * such as hashing, HMAC, encryption, decryption, and SSL/TLS functionalities
 * using the OpenSSL library.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/dh.h>

#include <Eina.h>

#include "Emile.h"

#include "emile_private.h"

#define MAX_KEY_LEN   EVP_MAX_KEY_LENGTH
#define MAX_IV_LEN    EVP_MAX_IV_LENGTH

struct _Emile_SSL
{
   Emile_SSL *parent;
   SSL_CTX *ssl_ctx;
   SSL *ssl;

   const char *last_error;
   const char *verify_name;

   int ssl_err;
   Emile_SSL_State ssl_state;
   Emile_Want_Type ssl_want;

   Eina_Bool server : 1;
   Eina_Bool listen : 1;
   Eina_Bool connecting : 1;
   Eina_Bool handshaking : 1;
   Eina_Bool upgrade : 1;
   Eina_Bool crl_flag : 1;
   Eina_Bool verify : 1;
   Eina_Bool verify_basic : 1; /**< EINA_TRUE if basic certificate verification is enabled. */
};

/**
 * @brief Initializes the OpenSSL library.
 *
 * This function loads error strings and initializes algorithms for OpenSSL.
 * It's typically called before any other OpenSSL operations.
 * This is conditionally compiled for older OpenSSL/LibreSSL versions.
 *
 * @return EINA_TRUE on success, EINA_FALSE otherwise (though currently always returns EINA_TRUE).
 */
Eina_Bool
_emile_cipher_init(void)
{
#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (LIBRESSL_VERSION_NUMBER < 0x3050000fL)
   ERR_load_crypto_strings();
   SSL_library_init();
   SSL_load_error_strings();
   OpenSSL_add_all_algorithms();
#endif /* if OPENSSL_VERSION_NUMBER < 0x10100000L || LIBRESSL_VERSION_NUMBER < 0x3050000fL */

   return EINA_TRUE;
}

/**
 * @brief Computes the HMAC-SHA1 of a binary buffer.
 *
 * @param key The secret key for HMAC.
 * @param key_len The length of the key.
 * @param data The binary buffer to hash.
 * @param digest Output buffer for the 20-byte SHA1 HMAC digest.
 *               Example: unsigned char digest_output[20];
 * @return EINA_TRUE on success (always returns EINA_TRUE in current implementation).
 */
EAPI Eina_Bool
emile_binbuf_hmac_sha1(const char *key,
                       unsigned int key_len,
                       const Eina_Binbuf *data,
                       unsigned char digest[20])
{
   HMAC(EVP_sha1(),
        key, key_len,
        eina_binbuf_string_get(data), eina_binbuf_length_get(data),
        digest, NULL);
   return EINA_TRUE;
}

/**
 * @brief Computes the SHA1 hash of a binary buffer.
 *
 * @param data The binary buffer to hash.
 * @param digest Output buffer for the 20-byte SHA1 digest.
 *               Example: unsigned char digest_output[20];
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., context creation failure).
 */
EAPI Eina_Bool
emile_binbuf_sha1(const Eina_Binbuf * data, unsigned char digest[20])
{
   const EVP_MD *md = EVP_sha1();
   Eina_Slice slice = eina_binbuf_slice_get(data);
#if (LIBRESSL_VERSION_NUMBER >= 0x3050000fL) || ((OPENSSL_VERSION_NUMBER >= 0x10100000L) && !defined(LIBRESSL_VERSION_NUMBER))
   EVP_MD_CTX *ctx = EVP_MD_CTX_new();
   if (!ctx) return EINA_FALSE;

   EVP_DigestInit_ex(ctx, md, NULL);

   if (!EVP_DigestUpdate(ctx, slice.mem, slice.len))
     {
        EVP_MD_CTX_free(ctx);
        return EINA_FALSE;
     }

   EVP_DigestFinal_ex(ctx, digest, NULL);

   EVP_MD_CTX_free(ctx);
#else
   EVP_MD_CTX ctx;

   EVP_MD_CTX_init(&ctx);
   EVP_DigestInit_ex(&ctx, md, NULL);

   EVP_DigestUpdate(&ctx, slice.mem, slice.len);
   EVP_DigestFinal_ex(&ctx, digest, NULL);

   EVP_MD_CTX_cleanup(&ctx);
#endif
   return EINA_TRUE;
}

/**
 * @brief Encrypts a binary buffer using a specified algorithm (currently only AES256-CBC).
 *
 * The encryption process involves:
 * 1. Generating a random salt.
 * 2. Deriving a key and IV from the provided key and salt using PBKDF2-SHA1.
 * 3. Prepending the salt and the original data length to the output buffer.
 * 4. Encrypting the data along with its original length.
 *
 * The output format is: [salt (4 bytes)][encrypted_original_length (4 bytes)][encrypted_data]
 *
 * @param algo The encryption algorithm to use. Currently, only EMILE_AES256_CBC is supported.
 * @param data The binary buffer to encrypt.
 * @param key The encryption key.
 * @param length The length of the encryption key.
 * @return A new Eina_Binbuf containing the encrypted data (including salt and original length),
 *         or NULL on failure. The caller is responsible for freeing the returned buffer.
 *         Example of returned structure (conceptual):
 *         Eina_Binbuf {
 *           unsigned char salt[4];
 *           unsigned int encrypted_original_data_length; // network byte order
 *           unsigned char encrypted_data_payload[];
 *         }
 */
EAPI Eina_Binbuf *
emile_binbuf_cipher(Emile_Cipher_Algorithm algo,
                    const Eina_Binbuf *data,
                    const char *key,
                    unsigned int length)
{
   /* Cipher declarations */
   Eina_Binbuf *result;
   unsigned char *pointer;
   unsigned char iv[MAX_IV_LEN];
   unsigned char ik[MAX_KEY_LEN];
   unsigned char key_material[MAX_IV_LEN + MAX_KEY_LEN];
   unsigned int salt;
   unsigned int tmp = 0;
   unsigned int crypted_length;
   /* Openssl declarations*/
   EVP_CIPHER_CTX *ctx = NULL;
   unsigned int *buffer = NULL;
   int tmp_len;

   if (algo != EMILE_AES256_CBC) return NULL;
   if (!emile_cipher_init()) return NULL;

   /* Openssl salt generation */
   if (!RAND_bytes((unsigned char *)&salt, sizeof (unsigned int)))
     return NULL;

   result = eina_binbuf_new();
   if (!result) return NULL;

   emile_pbkdf2_sha1(key,
                     length,
                     (unsigned char *)&salt,
                     sizeof(unsigned int),
                     2048,
                     key_material,
                     MAX_KEY_LEN + MAX_IV_LEN);

   memcpy(iv, key_material, MAX_IV_LEN);
   memcpy(ik, key_material + MAX_IV_LEN, MAX_KEY_LEN);

   memset(key_material, 0, sizeof (key_material));

   crypted_length = ((((eina_binbuf_length_get(data) + sizeof (unsigned int)) >> 5) + 1) << 5)
     + sizeof (unsigned int);

   eina_binbuf_append_length(result, (unsigned char*) &salt, sizeof (salt));
   memset(&salt, 0, sizeof (salt));

   tmp = eina_htonl(eina_binbuf_length_get(data));
   buffer = malloc(crypted_length - sizeof (int));
   if (!buffer) goto on_error;
   *buffer = tmp;

   eina_binbuf_append_length(result,
                             (unsigned char *) buffer,
                             crypted_length - sizeof (int));
   memcpy(buffer + 1,
          eina_binbuf_string_get(data),
          eina_binbuf_length_get(data));

   /* Openssl create the corresponding cipher
      AES with a 256 bit key, Cipher Block Chaining mode */
   ctx = EVP_CIPHER_CTX_new();
   if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, ik, iv))
     goto on_error;

   memset(iv, 0, sizeof (iv));
   memset(ik, 0, sizeof (ik));

   pointer = (unsigned char*) eina_binbuf_string_get(result);

   /* Openssl encrypt */
   if (!EVP_EncryptUpdate(ctx, pointer + sizeof (int), &tmp_len,
                          (unsigned char *)buffer,
                          eina_binbuf_length_get(data) + sizeof(unsigned int)))
     goto on_error;

   /* Openssl close the cipher */
   if (!EVP_EncryptFinal_ex(ctx, pointer + sizeof (int) + tmp_len,
                            &tmp_len))
     goto on_error;

   EVP_CIPHER_CTX_free(ctx);
   ctx = NULL;
   free(buffer);

   return result;

on_error:
   memset(iv, 0, sizeof (iv));
   memset(ik, 0, sizeof (ik));

   /* Openssl error */
#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (LIBRESSL_VERSION_NUMBER < 0x3050000fL)
   if (ctx)
     EVP_CIPHER_CTX_cleanup(ctx);
#else
   if (ctx) {
     EVP_CIPHER_CTX_cleanup(ctx);
     EVP_CIPHER_CTX_free(ctx);
   }
#endif /* if OPENSSL_VERSION_NUMBER < 0x10100000L || LIBRESSL_VERSION_NUMBER < 0x3050000fL */


   free(buffer);

   /* General error */
   eina_binbuf_free(result);

   return NULL;
}

/**
 * @brief Decrypts a binary buffer encrypted with emile_binbuf_cipher.
 *
 * The decryption process assumes the input data was encrypted by `emile_binbuf_cipher`
 * and thus expects the format: [salt (4 bytes)][encrypted_original_length (4 bytes)][encrypted_data].
 * It will:
 * 1. Extract the salt from the beginning of the data.
 * 2. Derive the key and IV using the provided key and the extracted salt via PBKDF2-SHA1.
 * 3. Decrypt the payload.
 * 4. Extract the original data length from the decrypted payload.
 * 5. Return the original plaintext data.
 *
 * @param algo The decryption algorithm to use. Currently, only EMILE_AES256_CBC is supported.
 * @param data The binary buffer to decrypt. Expected format:
 *         Eina_Binbuf {
 *           unsigned char salt[4];
 *           unsigned int encrypted_original_data_length; // network byte order
 *           unsigned char encrypted_data_payload[];
 *         }
 * @param key The decryption key.
 * @param length The length of the decryption key.
 * @return A new Eina_Binbuf containing the decrypted data, or NULL on failure.
 *         The caller is responsible for freeing the returned buffer.
 */
EAPI Eina_Binbuf *
emile_binbuf_decipher(Emile_Cipher_Algorithm algo,
                      const Eina_Binbuf *data,
                      const char *key,
                      unsigned int length)
{
   Eina_Binbuf *result = NULL;
   unsigned int *over;
   EVP_CIPHER_CTX *ctx = NULL;
   unsigned char ik[MAX_KEY_LEN];
   unsigned char iv[MAX_IV_LEN];
   unsigned char key_material[MAX_KEY_LEN + MAX_IV_LEN];
   unsigned int salt;
   unsigned int size;
   int tmp_len;
   int tmp = 0;

   if (algo != EMILE_AES256_CBC) return NULL;
   if (!emile_cipher_init()) return NULL;

   over = (unsigned int*) eina_binbuf_string_get(data);
   size = eina_binbuf_length_get(data);

   /* At least the salt and an AES block */
   if (size < sizeof(unsigned int) + 16)
     return NULL;

   /* Get the salt */
   salt = *over;

   /* Generate the iv and the key with the salt */
   emile_pbkdf2_sha1(key, length, (unsigned char *)&salt,
                     sizeof(unsigned int), 2048, key_material,
                     MAX_KEY_LEN + MAX_IV_LEN);

   memcpy(iv, key_material, MAX_IV_LEN);
   memcpy(ik, key_material + MAX_IV_LEN, MAX_KEY_LEN);

   memset(key_material, 0, sizeof (key_material));
   memset(&salt, 0, sizeof (salt));

   /* Align to AES block size if size is not align */
   tmp_len = size - sizeof (unsigned int);
   if ((tmp_len & 0x1F) != 0) goto on_error;

   result = eina_binbuf_new();
   if (!result) goto on_error;

   eina_binbuf_append_length(result, (unsigned char*) (over + 1), tmp_len);

   /* Openssl create the corresponding cipher */
   ctx = EVP_CIPHER_CTX_new();

   if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, ik, iv))
     goto on_error;

   memset(iv, 0, sizeof (iv));
   memset(ik, 0, sizeof (ik));

   /* Openssl decrypt */
   if (!EVP_DecryptUpdate(ctx,
                          (void*) eina_binbuf_string_get(result), &tmp,
                          (void*) (over + 1), tmp_len))
     goto on_error;

   /* Openssl close the cipher*/
   EVP_CIPHER_CTX_free(ctx);
   ctx = NULL;

   /* Get the decrypted data size */
   tmp = *(unsigned int*)(eina_binbuf_string_get(result));
   tmp = eina_ntohl(tmp);
   if (tmp > tmp_len || tmp <= 0)
     goto on_error;

   /* Remove header and padding  */
   eina_binbuf_remove(result, 0, sizeof (unsigned int));
   eina_binbuf_remove(result, tmp, eina_binbuf_length_get(result));

   return result;

on_error:
   memset(iv, 0, sizeof (iv));
   memset(ik, 0, sizeof (ik));

   if (ctx)
     EVP_CIPHER_CTX_free(ctx);

   eina_binbuf_free(result);

   return NULL;
}

/**
 * @brief Creates and configures an Emile_SSL context for a listening server.
 *
 * This function initializes an SSL_CTX for server-side operations,
 * sets up appropriate SSL/TLS versions (SSLv23 or TLSv1), configures options
 * like disabling SSLv2 and enabling single DH use, generates temporary DH parameters,
 * and sets a default cipher list.
 *
 * @param t The type of SSL/TLS protocol to use (e.g., EMILE_SSLv23, EMILE_TLSv1).
 * @return A pointer to an initialized Emile_SSL structure, or NULL on failure.
 *         The caller is responsible for freeing this structure using emile_cipher_free().
 */
EAPI Emile_SSL *
emile_cipher_server_listen(Emile_Cipher_Type t)
{
   Emile_SSL *r;
   int options;
   int dh = 0;

   if (!emile_cipher_init()) return NULL;

   r = calloc(1, sizeof (Emile_SSL));
   if (!r) return NULL;

   switch (t)
     {
      case EMILE_SSLv23:
         r->ssl_ctx = SSL_CTX_new(SSLv23_server_method());
         if (!r->ssl_ctx) goto on_error;
         options = SSL_CTX_get_options(r->ssl_ctx);
         SSL_CTX_set_options(r->ssl_ctx,
                             options | SSL_OP_NO_SSLv2 | SSL_OP_SINGLE_DH_USE);
         break;
#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (LIBRESSL_VERSION_NUMBER < 0x3050000fL)
      case EMILE_TLSv1:
         r->ssl_ctx = SSL_CTX_new(TLS_server_method());
         break;
#endif
      default:
         free(r);
         return NULL;
     }

   if (!r->ssl_ctx) goto on_error;


   do
     {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
       EVP_PKEY *params = NULL;
       EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_DH, NULL);
       if (!pctx)
         goto on_error;
       if (EVP_PKEY_paramgen_init(pctx) <= 0)
         goto on_error2;
       if (EVP_PKEY_CTX_set_dh_paramgen_prime_len(pctx, 1024) <= 0)
         goto on_error2;
       if (EVP_PKEY_CTX_set_dh_paramgen_generator(pctx, 5) <= 0)
         goto on_error2;
       if (EVP_PKEY_paramgen(pctx, &params) <= 0)
         goto on_error2;
       if (SSL_CTX_set0_tmp_dh_pkey(r->ssl_ctx, params) <= 0)
         goto on_error2;
on_error2:
       if (params) EVP_PKEY_free(params);
       EVP_PKEY_CTX_free(pctx);
       if (!params) goto on_error;
#else
       DH *dh_params = DH_new();
       if (!dh_params) goto on_error;
       if (!DH_generate_parameters_ex(dh_params, 1024, DH_GENERATOR_5, NULL))
         goto on_error;
       if (!DH_check(dh_params, &dh))
         goto on_error;
       if ((dh & DH_CHECK_P_NOT_PRIME) || (dh & DH_CHECK_P_NOT_SAFE_PRIME))
         goto on_error;
       if (!DH_generate_key(dh_params))
         goto on_error;
       if (!SSL_CTX_set_tmp_dh(r->ssl_ctx, dh_params))
         goto on_error;
       DH_free(dh_params);
#endif
     }
   while (0);

   INF("DH params successfully generated and applied!");

   if (!SSL_CTX_set_cipher_list(r->ssl_ctx,
                                "aRSA+HIGH:+kEDH:+kRSA:!kSRP:!kPSK:+3DES:!MD5"))
     goto on_error;

   return r;

 on_error:
   if (dh)
     {
        if (dh & DH_CHECK_P_NOT_PRIME)
          ERR("openssl error: dh_params could not generate a prime!");
        else
          ERR("openssl error: dh_params could not generate a safe prime!");
     }
   else
     {
        ERR("openssl error: %s.", ERR_reason_error_string(ERR_get_error()));
     }
   emile_cipher_free(r);
   return NULL;
}

/**
 * @internal
 * @brief Prints a human-readable string for an X509 verification error code.
 *
 * This function maps OpenSSL's X509_V_ERR_* constants to their string
 * representations and logs them as errors.
 *
 * @param error The X509 verification error code (e.g., X509_V_OK, X509_V_ERR_CERT_HAS_EXPIRED).
 */
static void
_emile_cipher_print_verify_error(int error)
{
   switch (error)
     {
#define ERROR_OPENSSL(X) \
case (X):        \
  ERR("%s", #X); \
  break
#ifdef X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT
        ERROR_OPENSSL(X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT);
#endif
#ifdef X509_V_ERR_UNABLE_TO_GET_CRL
        ERROR_OPENSSL(X509_V_ERR_UNABLE_TO_GET_CRL);
#endif
#ifdef X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE
        ERROR_OPENSSL(X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE);
#endif
#ifdef X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE
        ERROR_OPENSSL(X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE);
#endif
#ifdef X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY
        ERROR_OPENSSL(X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY);
#endif
#ifdef X509_V_ERR_CERT_SIGNATURE_FAILURE
        ERROR_OPENSSL(X509_V_ERR_CERT_SIGNATURE_FAILURE);
#endif
#ifdef X509_V_ERR_CRL_SIGNATURE_FAILURE
        ERROR_OPENSSL(X509_V_ERR_CRL_SIGNATURE_FAILURE);
#endif
#ifdef X509_V_ERR_CERT_NOT_YET_VALID
        ERROR_OPENSSL(X509_V_ERR_CERT_NOT_YET_VALID);
#endif
#ifdef X509_V_ERR_CERT_HAS_EXPIRED
        ERROR_OPENSSL(X509_V_ERR_CERT_HAS_EXPIRED);
#endif
#ifdef X509_V_ERR_CRL_NOT_YET_VALID
        ERROR_OPENSSL(X509_V_ERR_CRL_NOT_YET_VALID);
#endif
#ifdef X509_V_ERR_CRL_HAS_EXPIRED
        ERROR_OPENSSL(X509_V_ERR_CRL_HAS_EXPIRED);
#endif
#ifdef X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD
        ERROR_OPENSSL(X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD);
#endif
#ifdef X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD
        ERROR_OPENSSL(X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD);
#endif
#ifdef X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD
        ERROR_OPENSSL(X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD);
#endif
#ifdef X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD
        ERROR_OPENSSL(X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD);
#endif
#ifdef X509_V_ERR_OUT_OF_MEM
        ERROR_OPENSSL(X509_V_ERR_OUT_OF_MEM);
#endif
#ifdef X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT
        ERROR_OPENSSL(X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT);
#endif
#ifdef X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN
        ERROR_OPENSSL(X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN);
#endif
#ifdef X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY
        ERROR_OPENSSL(X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY);
#endif
#ifdef X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE
        ERROR_OPENSSL(X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE);
#endif
#ifdef X509_V_ERR_CERT_CHAIN_TOO_LONG
        ERROR_OPENSSL(X509_V_ERR_CERT_CHAIN_TOO_LONG);
#endif
#ifdef X509_V_ERR_CERT_REVOKED
        ERROR_OPENSSL(X509_V_ERR_CERT_REVOKED);
#endif
#ifdef X509_V_ERR_INVALID_CA
        ERROR_OPENSSL(X509_V_ERR_INVALID_CA);
#endif
#ifdef X509_V_ERR_PATH_LENGTH_EXCEEDED
        ERROR_OPENSSL(X509_V_ERR_PATH_LENGTH_EXCEEDED);
#endif
#ifdef X509_V_ERR_INVALID_PURPOSE
        ERROR_OPENSSL(X509_V_ERR_INVALID_PURPOSE);
#endif
#ifdef X509_V_ERR_CERT_UNTRUSTED
        ERROR_OPENSSL(X509_V_ERR_CERT_UNTRUSTED);
#endif
#ifdef X509_V_ERR_CERT_REJECTED
        ERROR_OPENSSL(X509_V_ERR_CERT_REJECTED);
#endif
        /* These are 'informational' when looking for issuer cert */
#ifdef X509_V_ERR_SUBJECT_ISSUER_MISMATCH
        ERROR_OPENSSL(X509_V_ERR_SUBJECT_ISSUER_MISMATCH);
#endif
#ifdef X509_V_ERR_AKID_SKID_MISMATCH
        ERROR_OPENSSL(X509_V_ERR_AKID_SKID_MISMATCH);
#endif
#ifdef X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH
        ERROR_OPENSSL(X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH);
#endif
#ifdef X509_V_ERR_KEYUSAGE_NO_CERTSIGN
        ERROR_OPENSSL(X509_V_ERR_KEYUSAGE_NO_CERTSIGN);
#endif

#ifdef X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER
        ERROR_OPENSSL(X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER);
#endif
#ifdef X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION
        ERROR_OPENSSL(X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION);
#endif
#ifdef X509_V_ERR_KEYUSAGE_NO_CRL_SIGN
        ERROR_OPENSSL(X509_V_ERR_KEYUSAGE_NO_CRL_SIGN);
#endif
#ifdef X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION
        ERROR_OPENSSL(X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION);
#endif
#ifdef X509_V_ERR_INVALID_NON_CA
        ERROR_OPENSSL(X509_V_ERR_INVALID_NON_CA);
#endif
#ifdef X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED
        ERROR_OPENSSL(X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED);
#endif
#ifdef X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE
        ERROR_OPENSSL(X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE);
#endif
#ifdef X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED
        ERROR_OPENSSL(X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED);
#endif

#ifdef X509_V_ERR_INVALID_EXTENSION
        ERROR_OPENSSL(X509_V_ERR_INVALID_EXTENSION);
#endif
#ifdef X509_V_ERR_INVALID_POLICY_EXTENSION
        ERROR_OPENSSL(X509_V_ERR_INVALID_POLICY_EXTENSION);
#endif
#ifdef X509_V_ERR_NO_EXPLICIT_POLICY
        ERROR_OPENSSL(X509_V_ERR_NO_EXPLICIT_POLICY);
#endif
#ifdef X509_V_ERR_DIFFERENT_CRL_SCOPE
        ERROR_OPENSSL(X509_V_ERR_DIFFERENT_CRL_SCOPE);
#endif
#ifdef X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE
        ERROR_OPENSSL(X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE);
#endif

#ifdef X509_V_ERR_UNNESTED_RESOURCE
        ERROR_OPENSSL(X509_V_ERR_UNNESTED_RESOURCE);
#endif

#ifdef X509_V_ERR_PERMITTED_VIOLATION
        ERROR_OPENSSL(X509_V_ERR_PERMITTED_VIOLATION);
#endif
#ifdef X509_V_ERR_EXCLUDED_VIOLATION
        ERROR_OPENSSL(X509_V_ERR_EXCLUDED_VIOLATION);
#endif
#ifdef X509_V_ERR_SUBTREE_MINMAX
        ERROR_OPENSSL(X509_V_ERR_SUBTREE_MINMAX);
#endif
#ifdef X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE
        ERROR_OPENSSL(X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE);
#endif
#ifdef X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX
        ERROR_OPENSSL(X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX);
#endif
#ifdef X509_V_ERR_UNSUPPORTED_NAME_SYNTAX
        ERROR_OPENSSL(X509_V_ERR_UNSUPPORTED_NAME_SYNTAX);
#endif
#ifdef X509_V_ERR_CRL_PATH_VALIDATION_ERROR
        ERROR_OPENSSL(X509_V_ERR_CRL_PATH_VALIDATION_ERROR);
#endif

        /* The application is not happy */
#ifdef X509_V_ERR_APPLICATION_VERIFICATION
        ERROR_OPENSSL(X509_V_ERR_APPLICATION_VERIFICATION);
#endif
     }
#undef ERROR_OPENSSL
}

/**
 * @internal
 * @brief Prints detailed information about an SSL session for debugging.
 *
 * This function logs the peer certificate chain and SSL session details
 * if the Emile log domain level is set to EINA_LOG_LEVEL_DBG or higher.
 *
 * @param ssl The SSL connection object.
 */
static void
_emile_cipher_session_print(SSL *ssl)
{
   Eina_Strbuf *str;
   SSL_SESSION *s;
   STACK_OF(X509) *sk;
   BIO *b;
   BUF_MEM *bptr;
   char log[4096];

   if (!eina_log_domain_level_check(_emile_log_dom_global, EINA_LOG_LEVEL_DBG))
     return ;

   str = eina_strbuf_new();
   if (!str) return ;

   log[0] = '\0';
   b = BIO_new(BIO_s_mem());
   sk = SSL_get_peer_cert_chain(ssl);
   if (sk)
     {
        int i;

        DBG("CERTIFICATES:");
        for (i = 0; i < sk_X509_num(sk); i++)
          {
             char *p;

             p = X509_NAME_oneline(X509_get_subject_name(sk_X509_value(sk, i)),
                                   log, sizeof (log));
             DBG("%2d s:%s", i, p);
             p = X509_NAME_oneline(X509_get_issuer_name(sk_X509_value(sk, i)),
                                   log, sizeof(log));
             DBG("   i:%s", p);

             PEM_write_bio_X509(b, sk_X509_value(sk, i));
             BIO_get_mem_ptr(b, &bptr);
             eina_strbuf_append_length(str, bptr->data, bptr->length);
             DBG("%s", eina_strbuf_string_get(str));
             eina_strbuf_reset(str);
          }
     }

   s = SSL_get_session(ssl);
   SSL_SESSION_print(b, s);
   BIO_get_mem_ptr(b, &bptr);
   eina_strbuf_append_length(str, bptr->data, bptr->length);
   DBG("%s", eina_strbuf_string_get(str));
   eina_strbuf_free(str);
   BIO_free(b);
}

/**
 * @internal
 * @brief Manages the SSL/TLS handshake process for a client connection on the server side.
 *
 * This function performs the SSL handshake steps. If verification is enabled
 * (client->parent->verify or client->parent->verify_basic), it also
 * verifies the peer's certificate against configured CAs and optionally
 * checks the certificate's common name or subject alternative name against
 * client->parent->verify_name.
 *
 * It handles SSL_ERROR_WANT_READ/WRITE by setting the appropriate
 * client->ssl_want state for non-blocking operations.
 *
 * @param client The Emile_SSL structure representing the client connection.
 *               Its ssl_state will be updated based on the handshake progress.
 */
static void
_emile_cipher_client_handshake(Emile_SSL *client)
{
   X509 *cert;
   int ret = -1;

   if (!client) return ;

   switch (client->ssl_state)
     {
      case EMILE_SSL_STATE_INIT:
         client->ssl_state = EMILE_SSL_STATE_HANDSHAKING;
         client->handshaking = EINA_TRUE;
         EINA_FALLTHROUGH;

      case EMILE_SSL_STATE_HANDSHAKING:
         if (!client->ssl) goto on_error;

         ret = SSL_do_handshake(client->ssl);
         client->ssl_err = SSL_get_error(client->ssl, ret);

         if ((client->ssl_err == SSL_ERROR_SYSCALL) ||
             (client->ssl_err == SSL_ERROR_SSL))
           goto on_error;

         if (ret != 1)
           {
              if (client->ssl_err == SSL_ERROR_WANT_READ)
                client->ssl_want = EMILE_WANT_READ;
              else if (client->ssl_err == SSL_ERROR_WANT_WRITE)
                client->ssl_want = EMILE_WANT_WRITE;

              return ;
           }

         client->handshaking = EINA_FALSE;
         client->ssl_state = EMILE_SSL_STATE_DONE;
         EINA_FALLTHROUGH;
      case EMILE_SSL_STATE_DONE:
         break;
      case EMILE_SSL_STATE_ERROR:
         goto on_error;
     }

   _emile_cipher_session_print(client->ssl);
   if (!client->parent->verify &&
       !client->parent->verify_basic)
     return ;

   SSL_set_verify(client->ssl, SSL_VERIFY_PEER, NULL);
   /* use CRL/CA lists to verify */
   cert = SSL_get_peer_certificate(client->ssl);
   if (cert)
     {
        const char *verify_name;
        char *cert_name;
        char *s;
        int clen;
        int err;
        int name = 0;

        if (client->parent->verify)
          {
             err = SSL_get_verify_result(client->ssl);
             _emile_cipher_print_verify_error(err);
             if (err) goto on_error;
          }

        clen = X509_NAME_get_text_by_NID(X509_get_subject_name(cert),
                                         NID_subject_alt_name,
                                         NULL, 0);
        if (clen > 0)
          {
             name = NID_subject_alt_name;
          }
        else
          {
             clen = X509_NAME_get_text_by_NID(X509_get_subject_name(cert),
                                              NID_commonName,
                                              NULL, 0);
             if (clen <= 0) goto on_error;
             name = NID_commonName;
          }

        cert_name = alloca(++clen);
        X509_NAME_get_text_by_NID(X509_get_subject_name(cert),
                                  name, cert_name, clen);
        verify_name = client->parent->verify_name;

        INF("Cert name: '%s' vs verify name: '%s'.", cert_name, verify_name);

        if (!verify_name) goto on_error;
        if (strcasecmp(cert_name, verify_name)) goto on_error;
        if (verify_name[0] != '*') goto on_error;

        /* verify that their is only one wildcard in the client cert name */
        if (strchr(cert_name + 1, '*')) goto on_error;
        /* verify that we have a domain of at least *.X.TLD and not *.TLD */
        if (!strchr(cert_name + 2, '.')) goto on_error;
        s = strchr(verify_name, '.');
        if (!s) goto on_error;
        /* same as above for the stored name */
        if (!strchr(s + 1, '.')) goto on_error;
        if (strcasecmp(s, verify_name + 1)) goto on_error;

        DBG("Successfully verified certificate.");
     }

   return ;

 on_error:
   DBG("Failed to finish handshake.");
   client->ssl_state = EMILE_SSL_STATE_ERROR;
   return ;
}

/**
 * @brief Accepts a new client connection on a listening server.
 *
 * This function creates a new Emile_SSL structure for an incoming client connection,
 * associates it with the server's SSL context, sets the file descriptor for the
 * connection, and initiates the SSL handshake using _emile_cipher_client_handshake().
 * This is typically used by a server after an accept() call.
 *
 * @param server The Emile_SSL structure of the listening server, obtained from
 *               emile_cipher_server_listen().
 * @param fd The file descriptor for the accepted client socket.
 * @return A new Emile_SSL structure for the client connection, or NULL on failure.
 *         The caller is responsible for freeing this structure using emile_cipher_free().
 */
EAPI Emile_SSL *
emile_cipher_client_connect(Emile_SSL *server, int fd)
{
   Emile_SSL *r;

   if (!server) return NULL;

   r = calloc(1, sizeof (Emile_SSL));
   if (!r) return NULL;

   r->parent = server;
   r->ssl = SSL_new(r->parent->ssl_ctx);
   if (!r->ssl) goto on_error;

   if (!SSL_set_fd(r->ssl, fd))
     goto on_error;

   SSL_set_accept_state(r->ssl);

   _emile_cipher_client_handshake(r);

   if (r->ssl_state == EMILE_SSL_STATE_ERROR) goto on_error;

   return r;

 on_error:
   emile_cipher_free(r);
   return NULL;
}

/**
 * @brief Creates and configures an Emile_SSL context for an outgoing client connection.
 *
 * This function initializes an SSL_CTX for client-side operations,
 * sets up appropriate SSL/TLS versions (SSLv23 or TLSv1), configures options
 * like disabling SSLv2, and sets a default cipher list.
 * This is used when this application intends to connect to a remote SSL/TLS server.
 *
 * @param t The type of SSL/TLS protocol to use (e.g., EMILE_SSLv23, EMILE_TLSv1).
 * @return A pointer to an initialized Emile_SSL structure, or NULL on failure.
 *         The caller is responsible for freeing this structure using emile_cipher_free().
 */
EAPI Emile_SSL *
emile_cipher_server_connect(Emile_Cipher_Type t)
{
   Emile_SSL *r;
   const char *msg;
   int options;

   if (!emile_cipher_init()) return NULL;

   r = calloc(1, sizeof (Emile_SSL));
   if (!r) return NULL;

   switch (t)
     {
      case EMILE_SSLv23:
         r->ssl_ctx = SSL_CTX_new(SSLv23_client_method());
         if (!r->ssl_ctx) goto on_error;
         options = SSL_CTX_get_options(r->ssl_ctx);
         SSL_CTX_set_options(r->ssl_ctx,
                             options | SSL_OP_NO_SSLv2 | SSL_OP_SINGLE_DH_USE);
         break;
      case EMILE_TLSv1:
#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (LIBRESSL_VERSION_NUMBER < 0x3050000fL)
         r->ssl_ctx = SSL_CTX_new(TLS_client_method());
         break;
#endif
      default:
         free(r);
         return NULL;
     }

   if (!SSL_CTX_set_cipher_list(r->ssl_ctx,
                                "aRSA+HIGH:+kEDH:+kRSA:!kSRP:!kPSK:+3DES:!MD5"))
     goto on_error;

   return r;

 on_error:
   msg = ERR_reason_error_string(ERR_get_error());
   ERR("OpenSSL error: '%s'.", msg);
   emile_cipher_free(r);
   return NULL;
}

/**
 * @brief Frees an Emile_SSL structure and its associated resources.
 *
 * This function releases all resources associated with an Emile_SSL object,
 * including shutting down the SSL connection, freeing the SSL and SSL_CTX objects,
 * and deallocating the Emile_SSL structure itself.
 *
 * @param emile The Emile_SSL structure to free.
 * @return EINA_TRUE on success, EINA_FALSE if emile is NULL.
 */
EAPI Eina_Bool
emile_cipher_free(Emile_SSL *emile)
{
   if (!emile) return EINA_FALSE;

   eina_stringshare_del(emile->last_error);
   emile->last_error = NULL;

   eina_stringshare_del(emile->verify_name);
   emile->verify_name = NULL;

   if (emile->ssl)
     {
        if (!SSL_shutdown(emile->ssl))
          SSL_shutdown(emile->ssl);

        SSL_free(emile->ssl);
     }
   emile->ssl = NULL;

   if (emile->ssl_ctx)
     SSL_CTX_free(emile->ssl_ctx);
   emile->ssl_ctx = NULL;

   free(emile);
   return EINA_TRUE;
}

/**
 * @brief Adds a CA certificate file or directory for peer verification.
 *
 * This function loads CA certificates from the specified file or all
 * certificates from the specified directory into the SSL context's trust store.
 * These CAs are used to verify the peer's certificate during the SSL handshake.
 *
 * @param emile The Emile_SSL context to which the CA(s) will be added.
 *              This should be a context created by emile_cipher_server_listen()
 *              or emile_cipher_server_connect().
 * @param file Path to a PEM-encoded CA certificate file or a directory
 *             containing PEM-encoded CA certificate files (hashed with c_rehash).
 *             Example: "/etc/ssl/certs/ca-certificates.crt" or "/etc/ssl/certs/"
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., file not found, OpenSSL error).
 *         On failure, emile->last_error may be set.
 */
EAPI Eina_Bool
emile_cipher_cafile_add(Emile_SSL *emile, const char *file)
{
   struct stat st;
   unsigned long err;

   if (stat(file, &st)) return EINA_FALSE;
   if (S_ISDIR(st.st_mode))
     {
        if (!SSL_CTX_load_verify_locations(emile->ssl_ctx, NULL, file))
          goto on_error;
     }
   else
     {
        if (!SSL_CTX_load_verify_locations(emile->ssl_ctx, file, NULL))
          goto on_error;
     }

   return EINA_TRUE;

 on_error:
   err = ERR_peek_last_error();
   if (!err) return EINA_FALSE;

   DBG("OpenSSL error: '%s'.", ERR_reason_error_string(err));
   eina_stringshare_replace(&emile->last_error, ERR_reason_error_string(err));
   return EINA_FALSE;
}

/**
 * @brief Adds a certificate to the Emile_SSL context.
 *
 * This function loads a PEM-encoded certificate from the specified file and
 * associates it with the SSL context. This is typically the server's own
 * certificate.
 *
 * @param emile The Emile_SSL context to which the certificate will be added.
 * @param file Path to the PEM-encoded certificate file.
 *             Example: "/path/to/server.crt"
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., file not found, invalid format, OpenSSL error).
 *         On failure, emile->last_error may be set.
 */
EAPI Eina_Bool
emile_cipher_cert_add(Emile_SSL *emile, const char *file)
{
   Eina_File *f;
   void *m;
   X509 *cert = NULL;
   BIO *bio = NULL;
   int err;

   f = eina_file_open(file, EINA_FALSE);
   if (!f) return EINA_FALSE;

   m = eina_file_map_all(f, EINA_FILE_WILLNEED);
   if (!m) goto on_error;

   bio = BIO_new_mem_buf(m, eina_file_size_get(f));
   if (!bio) goto on_error;

   cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
   if (!cert) goto on_error;

   if (SSL_CTX_use_certificate(emile->ssl_ctx, cert) < 1)
     goto on_error;

   eina_file_map_free(f, m);
   eina_file_close(f);
   BIO_free(bio);

   return EINA_TRUE;

 on_error:
   err = ERR_peek_last_error();

   if (m) eina_file_map_free(f, m);
   if (f) eina_file_close(f);
   if (bio) BIO_free(bio);

   if (!err) return EINA_FALSE;

   DBG("OpenSSL error: '%s'.", ERR_reason_error_string(err));
   eina_stringshare_replace(&emile->last_error, ERR_reason_error_string(err));
   return EINA_FALSE;
}

/**
 * @brief Adds a private key to the Emile_SSL context.
 *
 * This function loads a PEM-encoded private key from the specified file and
 * associates it with the SSL context. This key should correspond to the
 * certificate added via emile_cipher_cert_add().
 * After loading, it also checks if the private key matches the certificate.
 *
 * @param emile The Emile_SSL context to which the private key will be added.
 * @param file Path to the PEM-encoded private key file.
 *             Example: "/path/to/server.key"
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., file not found, key mismatch, OpenSSL error).
 *         On failure, emile->last_error may be set.
 */
EAPI Eina_Bool
emile_cipher_privkey_add(Emile_SSL *emile, const char *file)
{
   Eina_File *f;
   void *m;
   EVP_PKEY *privkey = NULL;
   BIO *bio = NULL;
   int err;

   f = eina_file_open(file, EINA_FALSE);
   if (!f) return EINA_FALSE;

   m = eina_file_map_all(f, EINA_FILE_WILLNEED);
   if (!m) goto on_error;

   bio = BIO_new_mem_buf(m, eina_file_size_get(f));
   if (!bio) goto on_error;

   privkey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
   if (!privkey) goto on_error;

   eina_file_map_free(f, m);
   m = NULL;

   eina_file_close(f);
   f = NULL;

   if (SSL_CTX_use_PrivateKey(emile->ssl_ctx, privkey) < 1)
     goto on_error;

   if (SSL_CTX_check_private_key(emile->ssl_ctx) < 1)
     goto on_error;

   BIO_free(bio);

   return EINA_TRUE;

 on_error:
   err = ERR_peek_last_error();

   if (m) eina_file_map_free(f, m);
   if (f) eina_file_close(f);
   if (bio) BIO_free(bio);

   if (!err) return EINA_FALSE;

   DBG("OpenSSL error: '%s'.", ERR_reason_error_string(err));
   eina_stringshare_replace(&emile->last_error, ERR_reason_error_string(err));
   return EINA_FALSE;
}

/**
 * @brief Adds a Certificate Revocation List (CRL) to the Emile_SSL context.
 *
 * This function loads a PEM-encoded CRL from the specified file into the
 * SSL context's certificate store. If this is the first CRL added, it also
 * enables CRL checking flags (X509_V_FLAG_CRL_CHECK and X509_V_FLAG_CRL_CHECK_ALL).
 *
 * @param emile The Emile_SSL context to which the CRL will be added.
 * @param file Path to the PEM-encoded CRL file.
 *             Example: "/path/to/crl.pem"
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., file not found, OpenSSL error).
 *         On failure, emile->last_error may be set.
 */
EAPI Eina_Bool
emile_cipher_crl_add(Emile_SSL *emile, const char *file)
{
   X509_LOOKUP *lu;
   X509_STORE *st;
   int err;

   st = SSL_CTX_get_cert_store(emile->ssl_ctx);
   if (!st) goto on_error;

   lu = X509_STORE_add_lookup(st, X509_LOOKUP_file());
   if (!lu) goto on_error;

   if (X509_load_crl_file(lu, file, X509_FILETYPE_PEM) < 1)
     goto on_error;

   if (!emile->crl_flag)
     {
        X509_STORE_set_flags(st,
                             X509_V_FLAG_CRL_CHECK |
                             X509_V_FLAG_CRL_CHECK_ALL);
        emile->crl_flag = EINA_TRUE;
     }

   return EINA_TRUE;

 on_error:
   err = ERR_peek_last_error();
   if (!err) return EINA_FALSE;

   DBG("OpenSSL error: '%s'.", ERR_reason_error_string(err));
   eina_stringshare_replace(&emile->last_error, ERR_reason_error_string(err));
   return EINA_FALSE;
}

/**
 * @brief Reads data from an SSL/TLS connection.
 *
 * Attempts to read up to `eina_binbuf_length_get(buffer)` bytes from the SSL connection
 * into the provided buffer.
 * If the SSL handshake is not yet complete (emile->ssl_state == EMILE_SSL_STATE_HANDSHAKING),
 * it will first attempt to continue the handshake via _emile_cipher_client_handshake().
 *
 * Handles non-blocking I/O by setting emile->ssl_want if SSL_read returns
 * SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE.
 *
 * @param emile The Emile_SSL connection object.
 * @param buffer The Eina_Binbuf to store the read data. The length of the binbuf
 *               determines the maximum number of bytes to read. The actual data
 *               will be written into `eina_binbuf_string_get(buffer)`.
 * @return The number of bytes read on success.
 *         0 if the connection was cleanly closed by the peer (SSL_ERROR_ZERO_RETURN),
 *         or if SSL_ERROR_WANT_READ/WRITE occurred (indicating to try again later).
 *         -1 on error (e.g., handshake error, SSL error, system call error).
 *         On error or SSL_ERROR_ZERO_RETURN, emile->last_error may be set.
 */
EAPI int
emile_cipher_read(Emile_SSL *emile, Eina_Binbuf *buffer)
{
   int err;
   int num;

   if (!emile->ssl) return -1;
   if (eina_binbuf_length_get(buffer) <= 0) return 0;

   if (emile->ssl_state == EMILE_SSL_STATE_HANDSHAKING)
     _emile_cipher_client_handshake(emile);
   if (emile->ssl_state == EMILE_SSL_STATE_ERROR)
     return -1;
   else if (emile->ssl_state == EMILE_SSL_STATE_HANDSHAKING)
     return 0;

   num = SSL_read(emile->ssl,
                  (void*) eina_binbuf_string_get(buffer),
                  eina_binbuf_length_get(buffer));
   emile->ssl_err = SSL_get_error(emile->ssl, num);

   switch (emile->ssl_err)
     {
      case SSL_ERROR_WANT_READ: emile->ssl_want = EMILE_WANT_READ; break;
      case SSL_ERROR_WANT_WRITE: emile->ssl_want = EMILE_WANT_WRITE; break;
      case SSL_ERROR_ZERO_RETURN:
      case SSL_ERROR_SYSCALL:
      case SSL_ERROR_SSL:
         err = ERR_peek_last_error();
         if (!err) return -1;

         DBG("OpenSSL error: '%s'.", ERR_reason_error_string(err));
         eina_stringshare_replace(&emile->last_error, ERR_reason_error_string(err));
         return -1;

      default:
         emile->ssl_want = EMILE_WANT_NOTHING;
         break;
     }

   return num < 0 ? 0 : num;
}

/**
 * @brief Writes data to an SSL/TLS connection.
 *
 * Attempts to write `eina_binbuf_length_get(buffer)` bytes from the provided buffer
 * to the SSL connection.
 * If the SSL handshake is not yet complete (emile->ssl_state == EMILE_SSL_STATE_HANDSHAKING),
 * it will first attempt to continue the handshake via _emile_cipher_client_handshake().
 *
 * Handles non-blocking I/O by setting emile->ssl_want if SSL_write returns
 * SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE.
 *
 * @param emile The Emile_SSL connection object.
 * @param buffer The Eina_Binbuf containing the data to write.
 * @return The number of bytes written on success.
 *         0 if no data was provided or if SSL_ERROR_WANT_READ/WRITE occurred
 *         (indicating to try again later).
 *         -1 on error (e.g., handshake error, SSL error, system call error).
 *         On error or SSL_ERROR_ZERO_RETURN, emile->last_error may be set.
 */
EAPI int
emile_cipher_write(Emile_SSL *emile, const Eina_Binbuf *buffer)
{
   int num;
   int err;

   if (!emile->ssl) return -1;
   if (!buffer || eina_binbuf_length_get(buffer) <= 0) return 0;

   if (emile->ssl_state == EMILE_SSL_STATE_HANDSHAKING)
     _emile_cipher_client_handshake(emile);
   if (emile->ssl_state == EMILE_SSL_STATE_ERROR)
     return -1;
   else if (emile->ssl_state == EMILE_SSL_STATE_HANDSHAKING)
     return 0;

   num = SSL_write(emile->ssl,
                   (void*) eina_binbuf_string_get(buffer),
                   eina_binbuf_length_get(buffer));
   emile->ssl_err = SSL_get_error(emile->ssl, num);

   switch (emile->ssl_err)
     {
      case SSL_ERROR_WANT_READ: emile->ssl_want = EMILE_WANT_READ; break;
      case SSL_ERROR_WANT_WRITE: emile->ssl_want = EMILE_WANT_WRITE; break;
      case SSL_ERROR_ZERO_RETURN:
      case SSL_ERROR_SYSCALL:
      case SSL_ERROR_SSL:
         err = ERR_peek_last_error();
         if (!err) return -1;

         DBG("OpenSSL error: '%s'.", ERR_reason_error_string(err));
         eina_stringshare_replace(&emile->last_error, ERR_reason_error_string(err));
         return -1;

      default:
         emile->ssl_want = EMILE_WANT_NOTHING;
         break;
     }

   return num < 0 ? 0 : num;
}

/**
 * @brief Gets the last error message string for an Emile_SSL context.
 *
 * @param emile The Emile_SSL context.
 * @return A pointer to a stringshared C-string containing the last error message,
 *         or NULL if no error message is set. The string is owned by the Emile_SSL
 *         context and should not be freed by the caller.
 */
EAPI const char *
emile_cipher_error_get(const Emile_SSL *emile)
{
   return emile->last_error;
}

/**
 * @brief Sets the hostname to verify against in the peer's certificate.
 *
 * This name is used during certificate verification if `emile->verify` is true.
 * It's typically compared against the certificate's Common Name (CN) or
 * Subject Alternative Name (SAN). Wildcards like "*.example.com" are supported
 * with specific validation rules (see _emile_cipher_client_handshake).
 *
 * @param emile The Emile_SSL context.
 * @param name The hostname to verify (e.g., "example.com", "*.example.com").
 *             The string is stringshared.
 * @return EINA_TRUE on success, EINA_FALSE on memory allocation failure for stringshare.
 */
EAPI Eina_Bool
emile_cipher_verify_name_set(Emile_SSL *emile, const char *name)
{
   return eina_stringshare_replace(&emile->verify_name, name);
}

/**
 * @brief Gets the hostname used for certificate verification.
 *
 * @param emile The Emile_SSL context.
 * @return A pointer to the stringshared hostname, or NULL if not set.
 *         The string is owned by the Emile_SSL context.
 */
EAPI const char *
emile_cipher_verify_name_get(const Emile_SSL *emile)
{
   return emile->verify_name;
}

/**
 * @brief Enables or disables full peer certificate verification.
 *
 * If enabled, this implies:
 * 1. The peer's certificate chain is validated against the trusted CAs.
 * 2. CRLs are checked if loaded.
 * 3. The certificate's hostname (CN/SAN) is matched against the name set by
 *    emile_cipher_verify_name_set().
 *
 * @param emile The Emile_SSL context.
 * @param verify EINA_TRUE to enable full verification, EINA_FALSE to disable.
 */
EAPI void
emile_cipher_verify_set(Emile_SSL *emile, Eina_Bool verify)
{
   emile->verify = verify;
}

/**
 * @brief Enables or disables basic peer certificate verification.
 *
 * If enabled, this implies that the peer's certificate chain is validated
 * against the trusted CAs and CRLs are checked if loaded.
 * It does *not* perform hostname verification.
 * This is typically less strict than full verification set by emile_cipher_verify_set().
 *
 * @param emile The Emile_SSL context.
 * @param verify_basic EINA_TRUE to enable basic verification, EINA_FALSE to disable.
 */
EAPI void
emile_cipher_verify_basic_set(Emile_SSL *emile, Eina_Bool verify_basic)
{
   emile->verify_basic = verify_basic;
}

/**
 * @brief Gets the status of full peer certificate verification.
 *
 * @param emile The Emile_SSL context.
 * @return EINA_TRUE if full verification is enabled, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
emile_cipher_verify_get(const Emile_SSL *emile)
{
   return emile->verify;
}

/**
 * @brief Gets the status of basic peer certificate verification.
 *
 * @param emile The Emile_SSL context.
 * @return EINA_TRUE if basic verification is enabled, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
emile_cipher_verify_basic_get(const Emile_SSL *emile)
{
   return emile->verify_basic;
}
