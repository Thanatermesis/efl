/**
 * @file eet_utils.c
 * @brief Utility functions for the Eet library.
 *
 * This file contains helper functions, primarily for hashing,
 * used internally by the Eet library.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <stdio.h>
#include <math.h>

#include "Eet.h"
#include "Eet_private.h"

/**
 * @brief Generates a hash value for a given string key and returns its length.
 *
 * This function computes a hash value for the input string @p key. The size of
 * the hash table (and thus the range of the hash value) is determined by
 * @p hash_size (e.g., a @p hash_size of 8 means a 2^8 = 256 entry hash table).
 * The length of the input string @p key is stored in @p len_ret.
 *
 * @param key The input string to hash. If NULL, the hash is 0 and length is 0.
 * @param hash_size The number of bits for the hash value (e.g., 8 for a 256-entry table).
 *                  The resulting hash will be masked by (1 << hash_size) - 1.
 * @param[out] len_ret Pointer to an integer where the length of the @p key will be stored.
 * @return The calculated hash value, masked to fit within the hash table size.
 *         Returns 0 if @p key is NULL.
 */
int
_eet_hash_gen_len(const char *key,
                  int         hash_size,
                  int         *len_ret)
{
   int hash_num = 0;
   int value, i;
   int mask;
   unsigned char *ptr;

   /* no string - index 0 */
   if (!key)
     {
        *len_ret = 0;
        return 0;
     }

   /* calc hash num */
   for (i = 0, ptr = (unsigned char *)key, value = (int)(*ptr);
        value;
        ptr++, i++, value = (int)(*ptr))
     hash_num ^= (value | (value << 8)) >> (i & 0x7);
   *len_ret = i;

   /* mask it */
   mask = (1 << hash_size) - 1;
   hash_num &= mask;
   /* return it */
   return hash_num;
}

/**
 * @brief Generates a hash value for a given string key.
 *
 * This function is a wrapper around _eet_hash_gen_len(), but it discards
 * the length of the key. It computes a hash value for the input string @p key.
 * The size of the hash table is determined by @p hash_size.
 *
 * @param key The input string to hash.
 * @param hash_size The number of bits for the hash value (e.g., 8 for a 256-entry table).
 * @return The calculated hash value.
 * @see _eet_hash_gen_len()
 */
int
_eet_hash_gen(const char *key,
              int         hash_size)
{
   int len;
   return _eet_hash_gen_len(key, hash_size, &len);
}

