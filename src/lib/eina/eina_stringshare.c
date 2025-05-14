/* EINA - EFL data type library
 * Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2010
 *			   Carsten Haitzler,
 *                         Jorge Luis Zapata Muga,
 *                         Cedric Bail,
 *                         Gustavo Sverzut Barbieri
 *                         Tom Hacohen
 *                         Brett Nash
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "eina_config.h"
#include "eina_private.h"
#include "eina_alloca.h"
#include "eina_log.h"
#include "eina_lock.h"
#include "eina_share_common.h"

/* undefs EINA_ARG_NONULL() so NULL checks are not compiled out! */
#include "eina_safety_checks.h"
#include "eina_stringshare.h"


#ifdef CRI
#undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_eina_share_stringshare_log_dom, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_eina_share_stringshare_log_dom, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_eina_share_stringshare_log_dom, __VA_ARGS__)

int _eina_share_stringshare_log_dom = -1;

/* The actual share */
static Eina_Share *stringshare_share;
static const char EINA_MAGIC_STRINGSHARE_NODE_STR[] = "Eina Stringshare Node";

extern Eina_Bool _share_common_threads_activated;
static Eina_Spinlock _mutex_small;

/* Stringshare optimizations */
/**
 * @internal
 * @brief A pre-allocated table for all possible single-character strings.
 *
 * This table is used as an optimization for stringshares of length 1.
 * It contains all 256 possible characters, each followed by a null
 * terminator, effectively creating 256 read-only C strings. This
 * avoids allocations and lookups for very common single-character strings.
 * The structure is `[char_0, '\0', char_1, '\0', ..., char_255, '\0']`.
 */
static const unsigned char _eina_stringshare_single[512] = {
   0,0,1,0,2,0,3,0,4,0,5,0,6,0,7,0,8,0,9,0,10,0,11,0,12,0,13,0,14,0,15,0,
   16,0,17,0,18,0,19,0,20,0,21,0,22,0,23,0,24,0,25,0,26,0,27,0,28,0,29,0,30,0,
   31,0,32,0,33,0,34,0,35,0,36,0,37,0,38,0,39,0,40,0,41,0,42,0,43,0,44,0,45,0,
   46,0,47,0,48,0,49,0,50,0,51,0,52,0,53,0,54,0,55,0,56,0,57,0,58,0,59,0,60,0,
   61,0,62,0,63,0,64,0,65,0,66,0,67,0,68,0,69,0,70,0,71,0,72,0,73,0,74,0,75,0,
   76,0,77,0,78,0,79,0,80,0,81,0,82,0,83,0,84,0,85,0,86,0,87,0,88,0,89,0,90,0,
   91,0,92,0,93,0,94,0,95,0,96,0,97,0,98,0,99,0,100,0,101,0,102,0,103,0,104,0,
   105,0,
   106,0,107,0,108,0,109,0,110,0,111,0,112,0,113,0,114,0,115,0,116,0,117,0,118,
   0,119,0,120,0,
   121,0,122,0,123,0,124,0,125,0,126,0,127,0,128,0,129,0,130,0,131,0,132,0,133,
   0,134,0,135,0,
   136,0,137,0,138,0,139,0,140,0,141,0,142,0,143,0,144,0,145,0,146,0,147,0,148,
   0,149,0,150,0,
   151,0,152,0,153,0,154,0,155,0,156,0,157,0,158,0,159,0,160,0,161,0,162,0,163,
   0,164,0,165,0,
   166,0,167,0,168,0,169,0,170,0,171,0,172,0,173,0,174,0,175,0,176,0,177,0,178,
   0,179,0,180,0,
   181,0,182,0,183,0,184,0,185,0,186,0,187,0,188,0,189,0,190,0,191,0,192,0,193,
   0,194,0,195,0,
   196,0,197,0,198,0,199,0,200,0,201,0,202,0,203,0,204,0,205,0,206,0,207,0,208,
   0,209,0,210,0,
   211,0,212,0,213,0,214,0,215,0,216,0,217,0,218,0,219,0,220,0,221,0,222,0,223,
   0,224,0,225,0,
   226,0,227,0,228,0,229,0,230,0,231,0,232,0,233,0,234,0,235,0,236,0,237,0,238,
   0,239,0,240,0,
   241,0,242,0,243,0,244,0,245,0,246,0,247,0,248,0,249,0,250,0,251,0,252,0,253,
   0,254,0,255,0
};

typedef struct _Eina_Stringshare_Small Eina_Stringshare_Small;
typedef struct _Eina_Stringshare_Small_Bucket Eina_Stringshare_Small_Bucket;

/**
 * @internal
 * @struct _Eina_Stringshare_Small_Bucket
 * @brief A bucket for storing small shared strings.
 *
 * This structure holds sorted arrays of small strings that share the same
 * first character. Using separate arrays for strings, lengths, and reference
 * counts is a form of struct-of-arrays, which can improve cache performance
 * during lookups.
 */
struct _Eina_Stringshare_Small_Bucket
{
   /* separate arrays for faster lookups */
   const char **strings; /**< Array of pointers to the string data */
   unsigned char *lengths; /**< Array of string lengths */
   unsigned int *references; /**< Array of reference counts for each string */
   int count; /**< Number of strings currently in the bucket */
   int size; /**< Allocated size of the arrays */
};

/**
 * @internal
 * @struct _Eina_Stringshare_Small
 * @brief Main structure for managing small stringshares.
 *
 * This structure contains an array of 256 pointers to buckets. Each bucket
 * corresponds to a possible starting character of a string. This allows for
 * quick dispatch to the correct bucket for a given string.
 */
struct _Eina_Stringshare_Small
{
   Eina_Stringshare_Small_Bucket *buckets[256]; /**< One bucket for each possible first character. */
};

#define EINA_STRINGSHARE_SMALL_BUCKET_STEP 8
static Eina_Stringshare_Small _eina_small_share;

/**
 * @internal
 * @brief Compares two "small" strings for sorting and searching.
 * @param bucket The bucket where the string to compare against is stored.
 * @param i The index of the string to compare against in the bucket.
 * @param pstr The other string to compare. This string has its first character skipped.
 * @param plength The length of the other string, minus one.
 * @return < 0 if the bucket string is less than pstr, 0 if they are equal, > 0 otherwise.
 *
 * This function is an optimized comparison for strings of length 2 or 3.
 * It assumes the first character of both strings are identical and thus
 * starts comparison from the second character.
 */
static inline int
_eina_stringshare_small_cmp(const Eina_Stringshare_Small_Bucket *bucket,
                            int i,
                            const char *pstr,
                            unsigned char plength)
{
   /* pstr and plength are from second char and on, since the first is
    * always the same.
    *
    * First string being always the same, size being between 2 and 3
    * characters (there is a check for special case length==1 and then
    * small stringshare is applied to strings < 4), we just need to
    * compare 2 characters of both strings.
    */
   const unsigned char cur_plength = bucket->lengths[i] - 1;
   const char *cur_pstr;

   if (cur_plength > plength)
     return 1;
   else if (cur_plength < plength)
     return -1;

   cur_pstr = bucket->strings[i] + 1;

   if (cur_pstr[0] > pstr[0])
     return 1;
   else if (cur_pstr[0] < pstr[0])
     return -1;

   if (plength == 1)
     return 0;

   if (cur_pstr[1] > pstr[1])
     return 1;
   else if (cur_pstr[1] < pstr[1])
     return -1;

   return 0;
}

/**
 * @internal
 * @brief Finds a small string within a bucket using binary search.
 * @param bucket The bucket to search in.
 * @param str The string to find.
 * @param length The length of the string.
 * @param idx Pointer to an integer where the index of the string will be
 *        stored if found, or the index where it should be inserted.
 * @return The found shared string, or NULL if not found.
 *
 * This function performs a binary search in the given bucket for the
 * provided string. The search is optimized by skipping the first character,
 * which is common to all strings in the bucket.
 */
static const char *
_eina_stringshare_small_bucket_find(const Eina_Stringshare_Small_Bucket *bucket,
                                    const char *str,
                                    unsigned char length,
                                    int *idx)
{
   const char *pstr = str + 1; /* skip first letter, it's always the same */
   unsigned char plength = length - 1;
   int i, low, high;

   if (bucket->count == 0)
     {
        *idx = 0;
        return NULL;
     }

   low = 0;
   high = bucket->count;

   while (low < high)
     {
        int r;

        i = (low + high - 1) / 2;

        r = _eina_stringshare_small_cmp(bucket, i, pstr, plength);
        if (r > 0)
          high = i;
        else if (r < 0)
          low = i + 1;
        else
          {
             *idx = i;
             return bucket->strings[i];
          }
     }

   *idx = low;
   return NULL;
}

/**
 * @internal
 * @brief Resizes the arrays within a small string bucket.
 * @param bucket The bucket to resize.
 * @param size The new size for the bucket arrays.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * This function reallocates the 'strings', 'lengths', and 'references'
 * arrays in the bucket to the new specified size.
 */
static Eina_Bool
_eina_stringshare_small_bucket_resize(Eina_Stringshare_Small_Bucket *bucket,
                                      int size)
{
   void *tmp;

   tmp = realloc((void *)bucket->strings, size * sizeof(bucket->strings[0]));
   if (!tmp) return 0;
   bucket->strings = tmp;

   tmp = realloc(bucket->lengths, size * sizeof(bucket->lengths[0]));
   if (!tmp) return 0;
   bucket->lengths = tmp;

   tmp = realloc(bucket->references, size * sizeof(bucket->references[0]));
   if (!tmp) return 0;
   bucket->references = tmp;

   bucket->size = size;
   return 1;
}

/**
 * @internal
 * @brief Inserts a new small string into a bucket at a given index.
 * @param p_bucket Pointer to the bucket pointer. The bucket can be created if it's NULL.
 * @param str The string to insert.
 * @param length The length of the string.
 * @param idx The index at which to insert the string.
 * @return The newly allocated and inserted shared string, or NULL on failure.
 *
 * This function handles memory allocation for the new string and resizing
 * the bucket if necessary. It also initializes the reference count to 1.
 */
static const char *
_eina_stringshare_small_bucket_insert_at(
   Eina_Stringshare_Small_Bucket **p_bucket,
   const char *str,
   unsigned char length,
   int idx)
{
   Eina_Stringshare_Small_Bucket *bucket = *p_bucket;
   int todo, off;
   char *snew;

   if (!bucket)
     {
        *p_bucket = bucket = calloc(1, sizeof(*bucket));
        if (!bucket) return NULL;
     }

   if (bucket->count + 1 >= bucket->size)
     {
        int size = bucket->size + EINA_STRINGSHARE_SMALL_BUCKET_STEP;
        if (!_eina_stringshare_small_bucket_resize(bucket, size))
          return NULL;
     }

   snew = malloc(length + 1);
   if (!snew) return NULL;

   memcpy(snew, str, length);
   snew[length] = '\0';

   off = idx + 1;
   todo = bucket->count - idx;
   if (todo > 0)
     {
        memmove((void *)(bucket->strings + off), bucket->strings + idx,
                todo * sizeof(bucket->strings[0]));
        memmove(bucket->lengths + off,           bucket->lengths + idx,
                todo * sizeof(bucket->lengths[0]));
        memmove(bucket->references + off,        bucket->references + idx,
                todo * sizeof(bucket->references[0]));
     }

   bucket->strings[idx] = snew;
   bucket->lengths[idx] = length;
   bucket->references[idx] = 1;
   bucket->count++;

   return snew;
}

/**
 * @internal
 * @brief Removes a string from a bucket at a given index.
 * @param p_bucket Pointer to the bucket pointer.
 * @param idx The index of the string to remove.
 *
 * This function decrements the reference count of the string. If the
 * reference count reaches zero, the string is freed and removed from the
 * bucket's arrays. The bucket itself is freed if it becomes empty.
 */
static void
_eina_stringshare_small_bucket_remove_at(
   Eina_Stringshare_Small_Bucket **p_bucket,
   int idx)
{
   Eina_Stringshare_Small_Bucket *bucket = *p_bucket;
   int todo, off;

   if (bucket->references[idx] > 1)
     {
        bucket->references[idx]--;
        return;
     }

   free((char *)bucket->strings[idx]);

   if (bucket->count == 1)
     {
        free((void *)bucket->strings);
        free(bucket->lengths);
        free(bucket->references);
        free(bucket);
        *p_bucket = NULL;
        return;
     }

   bucket->count--;
   if (idx == bucket->count)
     goto end;

   off = idx + 1;
   todo = bucket->count - idx;

   memmove((void *)(bucket->strings + idx), bucket->strings + off,
           todo * sizeof(bucket->strings[0]));
   memmove(bucket->lengths + idx,           bucket->lengths + off,
           todo * sizeof(bucket->lengths[0]));
   memmove(bucket->references + idx,        bucket->references + off,
           todo * sizeof(bucket->references[0]));

end:
   if (bucket->count + EINA_STRINGSHARE_SMALL_BUCKET_STEP < bucket->size)
     {
        int size = bucket->size - EINA_STRINGSHARE_SMALL_BUCKET_STEP;
        _eina_stringshare_small_bucket_resize(bucket, size);
     }
}

/**
 * @internal
 * @brief Adds a "small" string (length 2 or 3) to the small string share.
 * @param str The string to add.
 * @param length The length of the string.
 * @return The shared string instance.
 *
 * This function manages small strings to avoid the overhead of the main
 * stringshare hash. It finds the appropriate bucket (based on the first
 * character of the string) and then adds the string to that bucket.
 * If the string already exists, its reference count is incremented.
 */
static const char *
_eina_stringshare_small_add(const char *str, unsigned char length)
{
   Eina_Stringshare_Small_Bucket **bucket;
   int i;

   bucket = _eina_small_share.buckets + (unsigned char)str[0];
   if (!*bucket)
     i = 0;
   else
     {
        const char *ret;
        ret = _eina_stringshare_small_bucket_find(*bucket, str, length, &i);
        if (ret)
          {
             (*bucket)->references[i]++;
             return ret;
          }
     }

   return _eina_stringshare_small_bucket_insert_at(bucket, str, length, i);
}

/**
 * @internal
 * @brief Deletes a "small" string from the small string share.
 * @param str The string to delete.
 * @param length The length of the string.
 *
 * This function finds the string in the appropriate bucket and decreases
 * its reference count. If the reference count drops to zero, the string
 * is removed. It logs an error if a non-shared string is passed.
 */
static void
_eina_stringshare_small_del(const char *str, unsigned char length)
{
   Eina_Stringshare_Small_Bucket **bucket;
   const char *ret;
   int i;

   bucket = _eina_small_share.buckets + (unsigned char)str[0];
   if (!*bucket)
     goto error;

   ret = _eina_stringshare_small_bucket_find(*bucket, str, length, &i);
   if (!ret)
     goto error;

   _eina_stringshare_small_bucket_remove_at(bucket, i);
   return;

error:
   CRI("EEEK trying to del non-shared stringshare \"%s\"", str);
}

/**
 * @internal
 * @brief Initializes the small string sharing mechanism.
 *
 * This function sets up the spinlock and initializes the data structure
 * for managing shared strings of small length (2 or 3 characters). This
 * is an optimization to handle frequent, small strings more efficiently
 * than the generic stringsharing mechanism.
 */
static void
_eina_stringshare_small_init(void)
{
   eina_spinlock_new(&_mutex_small);
   memset(&_eina_small_share, 0, sizeof(_eina_small_share));
}

/**
 * @internal
 * @brief Shuts down the small string sharing mechanism.
 *
 * This function frees all resources used by the small string share,
 * including all the buckets and the strings they contain. It also
 * destroys the spinlock.
 */
static void
_eina_stringshare_small_shutdown(void)
{
   Eina_Stringshare_Small_Bucket **p_bucket, **p_bucket_end;

   p_bucket = _eina_small_share.buckets;
   p_bucket_end = p_bucket + 256;

   for (; p_bucket < p_bucket_end; p_bucket++)
     {
        Eina_Stringshare_Small_Bucket *bucket = *p_bucket;
        char **s, **s_end;

        if (!bucket)
          continue;

        s = (char **)bucket->strings;
        s_end = s + bucket->count;
        for (; s < s_end; s++)
          free(*s);

        free((void *)bucket->strings);
        free(bucket->lengths);
        free(bucket->references);
        free(bucket);
        *p_bucket = NULL;
     }

   eina_spinlock_free(&_mutex_small);
}

/**
 * @internal
 * @brief Dumps statistics for a single small string bucket.
 * @param bucket The bucket to dump.
 * @param di A struct to accumulate dump information.
 *
 * This function iterates through a bucket and prints information about
 * each shared string (length, refcount, value) and updates the global
 * dump statistics.
 */
static void
_eina_stringshare_small_bucket_dump(Eina_Stringshare_Small_Bucket *bucket,
                                    struct dumpinfo *di)
{
   const char **s = bucket->strings;
   unsigned char *l = bucket->lengths;
   unsigned int *r = bucket->references;
   int i;

   di->used += sizeof(*bucket);
   di->used += bucket->count * sizeof(*s);
   di->used += bucket->count * sizeof(*l);
   di->used += bucket->count * sizeof(*r);
   di->unique += bucket->count;

   for (i = 0; i < bucket->count; i++, s++, l++, r++)
     {
        int dups;

        printf("DDD: %5hhu %5u '%s'\n", *l, *r, *s);

        dups = (*r - 1);

        di->used += *l;
        di->saved += *l * dups;
        di->dups += dups;
     }
}

/**
 * @internal
 * @brief Dumps statistics for all small string buckets.
 * @param di A struct to accumulate dump information.
 *
 * This function is a callback used by eina_share_common_dump(). It iterates
 * over all possible buckets and calls the bucket-specific dump function
 * for each one that is active.
 */
static void
_eina_stringshare_small_dump(struct dumpinfo *di)
{
   Eina_Stringshare_Small_Bucket **p_bucket, **p_bucket_end;

   p_bucket = _eina_small_share.buckets;
   p_bucket_end = p_bucket + 256;

   for (; p_bucket < p_bucket_end; p_bucket++)
     {
        Eina_Stringshare_Small_Bucket *bucket = *p_bucket;

        if (!bucket)
          continue;

        _eina_stringshare_small_bucket_dump(bucket, di);
     }
}


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/**
 * @internal
 * @brief Initialize the share_common module.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function sets up the share_common module of Eina. It is called by
 * eina_init().
 *
 * @see eina_init()
 */
Eina_Bool
eina_stringshare_init(void)
{
   Eina_Bool ret;

   if (_eina_share_stringshare_log_dom < 0)
     {
        _eina_share_stringshare_log_dom = eina_log_domain_register
           ("eina_stringshare", EINA_LOG_COLOR_DEFAULT);

        if (_eina_share_stringshare_log_dom < 0)
          {
             EINA_LOG_ERR("Could not register log domain: eina_stringshare");
             return EINA_FALSE;
          }
     }

   ret = eina_share_common_init(&stringshare_share,
                                EINA_MAGIC_STRINGSHARE_NODE,
                                EINA_MAGIC_STRINGSHARE_NODE_STR);
   if (ret)
     _eina_stringshare_small_init();
   else
     {
        eina_log_domain_unregister(_eina_share_stringshare_log_dom);
        _eina_share_stringshare_log_dom = -1;
     }

   return ret;
}

/**
 * @internal
 * @brief Shut down the share_common module.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function shuts down the share_common module set up by
 * eina_share_common_init(). It is called by eina_shutdown().
 *
 * @see eina_shutdown()
 */
Eina_Bool
eina_stringshare_shutdown(void)
{
   Eina_Bool ret;
   _eina_stringshare_small_shutdown();
   ret = eina_share_common_shutdown(&stringshare_share);

   if (_eina_share_stringshare_log_dom >= 0)
     {
        eina_log_domain_unregister(_eina_share_stringshare_log_dom);
        _eina_share_stringshare_log_dom = -1;
     }

   return ret;
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

EINA_API void
eina_stringshare_del(Eina_Stringshare *str)
{
   int slen;

   if (!str)
     return;

   /* special cases */
   if (str[0] == '\0')
     slen = 0;
   else if (str[1] == '\0')
     slen = 1;
   else if (str[2] == '\0')
     slen = 2;
   else if (str[3] == '\0')
     slen = 3;
   else
     slen = 4;  /* handled later */

   if (slen < 2)
     {
        eina_share_common_population_del(stringshare_share, slen);

        return;
     }
   else if (slen < 4)
     {
        eina_share_common_population_del(stringshare_share, slen);
        eina_spinlock_take(&_mutex_small);
        _eina_stringshare_small_del(str, slen);
        eina_spinlock_release(&_mutex_small);

        return;
     }

   if (!eina_share_common_del(stringshare_share, str))
     CRI("EEEK trying to del non-shared stringshare \"%s\"", str);
}

EINA_API Eina_Stringshare *
eina_stringshare_add_length(const char *str, unsigned int slen)
{
   if (!str)
     return NULL;
   else if (slen == 0)
     {
        eina_share_common_population_add(stringshare_share, slen);

        return "";
     }
   else if (slen == 1)
     {
        eina_share_common_population_add(stringshare_share, slen);

        return (Eina_Stringshare *) _eina_stringshare_single + ((*str) << 1);
     }
   else if (slen < 4)
     {
        const char *s;

        eina_share_common_population_add(stringshare_share, slen);
        eina_spinlock_take(&_mutex_small);
        s = _eina_stringshare_small_add(str, slen);
        eina_spinlock_release(&_mutex_small);

        return s;
     }

   return eina_share_common_add_length(stringshare_share, str, slen *
                                       sizeof(char), sizeof(char));
}

EINA_API Eina_Stringshare *
eina_stringshare_add(const char *str)
{
   if (!str) return NULL;
   return eina_stringshare_add_length(str, strlen(str));
}

EINA_API Eina_Stringshare *
eina_stringshare_printf(const char *fmt, ...)
{
   va_list args;
   char *tmp = NULL;
   const char *ret = "";
   int len;

   if (!fmt)
     return NULL;

   va_start(args, fmt);
   len = vasprintf(&tmp, fmt, args);
   va_end(args);

   if (len < 1) goto on_error;

   ret = eina_stringshare_add_length(tmp, len);

 on_error:
   free(tmp);
   return ret;
}

EINA_API Eina_Stringshare *
eina_stringshare_vprintf(const char *fmt, va_list args)
{
   char *tmp = NULL;
   const char *ret = "";
   int len;

   if (!fmt)
     return NULL;

   len = vasprintf(&tmp, fmt, args);

   if (len < 1) goto on_error;

   ret = eina_stringshare_add_length(tmp, len);

 on_error:
   free(tmp);
   return ret;
}

EINA_API Eina_Stringshare *
eina_stringshare_nprintf(unsigned int len, const char *fmt, ...)
{
   va_list args;
   char *tmp;
   int size;

   if (!fmt)
     return NULL;

   if (len == 0)
     return "";

   tmp = alloca(sizeof(char) * (len + 1));

   va_start(args, fmt);
   size = vsnprintf(tmp, len, fmt, args);
   va_end(args);

   if (size < 1)
     return "";
   if ((unsigned int)size > len)
     size = len;

   return eina_stringshare_add_length(tmp, size);
}

EINA_API Eina_Stringshare *
eina_stringshare_ref(Eina_Stringshare *str)
{
   int slen;

   if (!str)
     return NULL;

   /* special cases */
   if      (str[0] == '\0')
     slen = 0;
   else if (str[1] == '\0')
     slen = 1;
   else if (str[2] == '\0')
     slen = 2;
   else if (str[3] == '\0')
     slen = 3;
   else
     slen = 3 + (int)strlen(str + 3);

   if (slen < 2)
     {
        eina_share_common_population_add(stringshare_share, slen);

        return str;
     }
   else if (slen < 4)
     {
        const char *s;

        eina_share_common_population_add(stringshare_share, slen);
        eina_spinlock_take(&_mutex_small);
        s = _eina_stringshare_small_add(str, slen);
        eina_spinlock_release(&_mutex_small);

        return s;
     }

   return eina_share_common_ref(stringshare_share, str);
}

EINA_API int
eina_stringshare_strlen(Eina_Stringshare *str)
{
   int len;

   if (!str) return 0;

   /* special cases */
   if (str[0] == '\0')
     return 0;

   if (str[1] == '\0')
     return 1;

   if (str[2] == '\0')
     return 2;

   if (str[3] == '\0')
     return 3;

   len = eina_share_common_length(stringshare_share, (Eina_Stringshare *) str);
   len = (len > 0) ? len / (int)sizeof(char) : -1;
   return len;
}

EINA_API void
eina_stringshare_dump(void)
{
   eina_share_common_dump(stringshare_share,
                          _eina_stringshare_small_dump,
                          sizeof(_eina_stringshare_single));
}
