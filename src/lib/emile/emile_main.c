/**
 * @file
 * @brief Main file for the Emile library, providing initialization, shutdown,
 *        and core cryptographic operations.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#ifdef HAVE_OPENSSL
# include <openssl/ssl.h>
# include <openssl/err.h>
# include <openssl/evp.h>
#endif /* ifdef HAVE_OPENSSL */

#include <Eina.h>

#include "Emile.h"
#include "emile_private.h"

static Eina_Bool _emile_cipher_inited = EINA_FALSE;
static unsigned int _emile_init_count = 0;
int _emile_log_dom_global = -1; /**< Global log domain for Emile. */

/**
 * @brief Initializes the cipher subsystem.
 *
 * This function initializes the underlying cipher library (e.g., OpenSSL)
 * if it hasn't been initialized already. It's safe to call this function
 * multiple times.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
emile_cipher_init(void)
{
   if (_emile_cipher_inited)
     return EINA_TRUE;

   if (!_emile_cipher_init())
     return EINA_FALSE;

   _emile_cipher_inited = EINA_TRUE;

   return EINA_TRUE;
}

/**
 * @brief Gets the currently used cipher backend module.
 *
 * This function returns which cryptographic backend is being used by Emile.
 *
 * @return The active Emile_Cipher_Backend.
 * @see Emile_Cipher_Backend
 */
EAPI Emile_Cipher_Backend
emile_cipher_module_get(void)
{
#ifdef HAVE_OPENSSL
   return EMILE_OPENSSL;
#else
   return EMILE_NONE;
#endif
}

/**
 * @brief Initializes the Emile library.
 *
 * This function initializes Emile, including its dependency on Eina.
 * It sets up logging and increments an initialization counter.
 * It's safe to call this function multiple times; it will only fully
 * initialize on the first call.
 *
 * @return The current initialization count. A count of 1 means the library
 *         was just initialized. A count greater than 1 means it was already
 *         initialized. A count of 0 or less indicates an error during
 *         initialization.
 */
EAPI int
emile_init(void)
{
   if (++_emile_init_count != 1)
     return _emile_init_count;

   if (!eina_init())
     return --_emile_init_count;

   _emile_log_dom_global = eina_log_domain_register("emile", EINA_COLOR_CYAN);
   if (_emile_log_dom_global < 0)
     {
        EINA_LOG_ERR("Emile can not create a general log domain.");
        goto shutdown_eina;
     }

   eina_log_timing(_emile_log_dom_global, EINA_LOG_STATE_STOP, EINA_LOG_STATE_INIT);

   return _emile_init_count;

shutdown_eina:
   eina_shutdown();

   return --_emile_init_count;
}

/**
 * @brief Shuts down the Emile library.
 *
 * This function shuts down Emile and its dependencies, such as Eina and
 * the cipher subsystem (if initialized). It decrements an initialization
 * counter. The library is fully shut down when the counter reaches 0.
 *
 * @return The current initialization count. A count of 0 means the library
 *         was just shut down. A count greater than 0 means it's still in use.
 */
EAPI int
emile_shutdown(void)
{
   if (--_emile_init_count != 0)
     return _emile_init_count;

   eina_log_timing(_emile_log_dom_global, EINA_LOG_STATE_START, EINA_LOG_STATE_SHUTDOWN);

   if (_emile_cipher_inited)
     {
#if defined(HAVE_OPENSSL) && (OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER))
        EVP_cleanup();
        ERR_free_strings();
#endif /* if defined(HAVE_OPENSSL) && (OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)) */
     }

   eina_log_domain_unregister(_emile_log_dom_global);
   _emile_log_dom_global = -1;

   eina_shutdown();

   return _emile_init_count;
}

/* For the moment, we have just one function shared accross both cipher
 * backend, so here it is. */

/**
 * @brief Derives a key using PBKDF2 with HMAC-SHA1.
 *
 * This function implements the PBKDF2 (Password-Based Key Derivation Function 2)
 * algorithm using HMAC-SHA1 as the pseudorandom function. It is used to
 * derive a cryptographic key from a password or passphrase.
 *
 * @param key The password or master key.
 * @param key_len The length of the key in bytes.
 * @param salt The salt value.
 *             Example: `(const unsigned char *)"mysalt123"`
 * @param salt_len The length of the salt in bytes.
 * @param iter The number of iterations to perform. Higher numbers increase
 *             security but also computation time.
 *             Example: `10000`
 * @param[out] res Buffer to store the derived key.
 *                 The derived key will be written here.
 *                 Example: `unsigned char derived_key[32];`
 * @param res_len The desired length of the derived key in bytes.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., memory allocation issues).
 */
Eina_Bool
emile_pbkdf2_sha1(const char *key, unsigned int key_len, const unsigned char *salt, unsigned int salt_len, unsigned int iter, unsigned char *res, unsigned int res_len)
{
   Eina_Binbuf *step1, *step2;
   unsigned char *buf;
   unsigned char *p = res;
   unsigned char digest[20];
   unsigned char tab[4];
   unsigned int len = res_len;
   unsigned int tmp_len;
   unsigned int i, j, k;

// this warning is wrong here so disable it
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
   buf = alloca(salt_len + 4);
   step1 = eina_binbuf_manage_new(buf, salt_len + 4, EINA_TRUE);
#pragma GCC diagnostic pop

   if (!step1)
     return EINA_FALSE;
   step2 = eina_binbuf_manage_new(digest, 20, EINA_TRUE);
   if (!step2)
     {
        eina_binbuf_free(step1);
        return EINA_FALSE;
     }

   for (i = 1; len; len -= tmp_len, p += tmp_len, i++)
     {
        tmp_len = (len > 20) ? 20 : len;

        tab[0] = (unsigned char)(i & 0xff000000) >> 24;
        tab[1] = (unsigned char)(i & 0x00ff0000) >> 16;
        tab[2] = (unsigned char)(i & 0x0000ff00) >> 8;
        tab[3] = (unsigned char)(i & 0x000000ff) >> 0;

        memcpy(buf, salt, salt_len);
        memcpy(buf + salt_len, tab, 4);

        if (!emile_binbuf_hmac_sha1(key, key_len, step1, digest))
          return EINA_FALSE;

        memcpy(p, digest, tmp_len);

        for (j = 1; j < iter; j++)
          {
             if (!emile_binbuf_hmac_sha1(key, key_len, step2, digest))
               return EINA_FALSE;
             for (k = 0; k < tmp_len; k++)
               p[k] ^= digest[k];
          }
     }

   eina_binbuf_free(step1);
   eina_binbuf_free(step2);

   return EINA_TRUE;
}

