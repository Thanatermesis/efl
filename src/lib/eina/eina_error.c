/* EINA - EFL data type library
 * Copyright (C) 2007-2008 Jorge Luis Zapata Muga, Cedric Bail
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "eina_config.h"
#include "eina_private.h"


/* undefs EINA_ARG_NONULL() so NULL checks are not compiled out! */
#include "eina_safety_checks.h"
#include "eina_error.h"
#include "eina_stringshare.h"
#include "eina_lock.h"
#include "eina_str.h"
#ifdef EINA_HAVE_THREADS
#include "eina_hash.h"
#endif

/* TODO
 * + add a wrapper for assert?
 * + add common error numbers, messages
 * + add a calltrace of errors, not only store the last error but a list of them
 * and also store the function that set it
 */

/*============================================================================*
*                                  Local                                     *
*============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @internal
 * @brief Structure to hold a registered error message.
 *
 * This structure stores the error message string and a flag indicating
 * whether the string was dynamically allocated (and thus needs to be freed)
 * or if it's a static string.
 */
typedef struct _Eina_Error_Message Eina_Error_Message;
struct _Eina_Error_Message
{
   Eina_Bool string_allocated; /**< EINA_TRUE if 'string' was allocated and needs eina_stringshare_del(), EINA_FALSE otherwise. */
   const char *string;         /**< The error message string. This might be a stringshared string or a static string. */
};

#ifdef EINA_HAVE_THREADS
static Eina_Spinlock _eina_errno_msgs_lock; /**< @internal Spinlock to protect access to _eina_errno_msgs hash table. */
static Eina_Hash *_eina_errno_msgs = NULL; /**< @internal Hash table to cache stringshared versions of strerror() messages. Key: (int)errno, Value: (const char *)stringshared_message. Used because strerror() is not always thread-safe or might return pointers to static buffers. */
#endif
static Eina_Error_Message *_eina_errors = NULL; /**< @internal Dynamically allocated array of registered error messages. */
static size_t _eina_errors_count = 0; /**< @internal Current number of error messages stored in _eina_errors. Also used to generate new error IDs. */
static size_t _eina_errors_allocated = 0; /**< @internal Current allocated capacity of the _eina_errors array. */

/* used to differentiate registered errors from errno.h */
/** @internal Bit flag used to distinguish Eina-registered error codes from system errno codes. */
#define EINA_ERROR_REGISTERED_BIT (1 << 30)
/** @internal Checks if an error code is an Eina-registered error. */
#define EINA_ERROR_REGISTERED_CHECK(err) ((err) & EINA_ERROR_REGISTERED_BIT)

/** @internal Converts an internal array index to an Eina-registered error code. */
#define EINA_ERROR_FROM_INDEX(idx) ((idx) | EINA_ERROR_REGISTERED_BIT)
/** @internal Converts an Eina-registered error code back to an internal array index. */
#define EINA_ERROR_TO_INDEX(err) ((err) & (~EINA_ERROR_REGISTERED_BIT))

static Eina_Error _eina_last_error; /**< @internal Stores the last error code when not using TLS (e.g., outside main loop or if TLS init failed). */
static Eina_TLS _eina_last_key;     /**< @internal Thread-Local Storage key for storing the last error code per thread. */

/**
 * @internal
 * @brief Allocates space for a new Eina_Error_Message in the internal array.
 *
 * This function handles the dynamic resizing of the `_eina_errors` array
 * if it's full. It increments `_eina_errors_count`.
 * The initial allocation size is 24 messages, and it grows by 8 messages
 * each time it needs to reallocate.
 *
 * @return A pointer to the newly allocated Eina_Error_Message slot,
 *         or NULL if reallocation fails. The caller is responsible for
 *         filling the members of the returned struct.
 */
static Eina_Error_Message *
_eina_error_msg_alloc(void)
{
   size_t idx;

   if (_eina_errors_count == _eina_errors_allocated)
     {
        void *tmp;
        size_t size;

        if (EINA_UNLIKELY(_eina_errors_allocated == 0))
           size = 24;
        else
           size = _eina_errors_allocated + 8;

        tmp = realloc(_eina_errors, sizeof(Eina_Error_Message) * size);
        if (!tmp)
           return NULL;

        _eina_errors = tmp;
        _eina_errors_allocated = size;
     }

   idx = _eina_errors_count;
   _eina_errors_count++;
   return _eina_errors + idx;
}

#ifdef _WIN32
# define HAVE_STRERROR_R
# ifdef STRERROR_R_CHAR_P
#  undef STRERROR_R_CHAR_P
# endif
/**
 * @internal
 * @brief Windows-specific wrapper for strerror_s to behave like POSIX strerror_r.
 *
 * Windows provides `strerror_s` which has a similar purpose to the XSI-compliant
 * `strerror_r`. This function wraps `strerror_s` to provide a consistent
 * interface. If `strerror_s` returns "Unknown error", this function attempts
 * to provide a more informative message including the error number.
 *
 * @param errnum The error number.
 * @param buf Buffer to store the error message.
 * @param buflen Size of the buffer.
 * @return 0 on success, or an error code on failure (consistent with strerror_r).
 */
static inline int strerror_r(int errnum, char *buf, size_t buflen)
{
   int ret;

   ret = strerror_s(buf, buflen, errnum);
   if (strcmp(buf, "Unknown error") == 0)
     snprintf(buf, buflen, "Unknown error %d", errnum);

   return ret;
}
#endif

/**
 * @endcond
 */


/*============================================================================*
*                                 Global                                     *
*============================================================================*/

/**
 * @cond LOCAL
 */

EINA_API Eina_Error EINA_ERROR_OUT_OF_MEMORY = ENOMEM;

/**
 * @endcond
 */

/**
 * @internal
 * @brief Initialize the error module.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function sets up the Eina error system. It is called by eina_init().
 * Its main tasks are:
 * - Creating a Thread-Local Storage (TLS) key (`_eina_last_key`) to store
 *   per-thread error codes. This is fundamental for thread-safe error
 *   handling with eina_error_get()/set().
 * - If `EINA_HAVE_THREADS` is defined, it also initializes a spinlock and
 *   a hash table (`_eina_errno_msgs`) to cache system `strerror` messages
 *   in a thread-safe way.
 *
 * @see eina_init()
 * @see eina_error_shutdown()
 */
Eina_Bool
eina_error_init(void)
{
   if (!eina_tls_new(&_eina_last_key))
     return EINA_FALSE;

#ifdef EINA_HAVE_THREADS
   if (!eina_spinlock_new(&_eina_errno_msgs_lock)) goto failed_lock;
   _eina_errno_msgs = eina_hash_int32_new(EINA_FREE_CB(eina_stringshare_del));
   if (!_eina_errno_msgs) goto failed_hash;
#endif

   return EINA_TRUE;

#ifdef EINA_HAVE_THREADS
 failed_hash:
   eina_spinlock_free(&_eina_errno_msgs_lock);
 failed_lock:
   eina_tls_free(_eina_last_key);
   _eina_last_error = 0;
   return EINA_FALSE;
#endif
}

/**
 * @internal
 * @brief Shut down the error module.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function tears down the Eina error system and frees all associated
 * resources. It is called by eina_shutdown(). Its tasks include:
 * - Iterating through all registered custom error messages (`_eina_errors`)
 *   and deleting the stringshared messages (`eem->string_allocated` is true).
 * - Freeing the array of error messages itself.
 * - If `EINA_HAVE_THREADS` is defined, it frees the `strerror` cache hash
 *   and destroys the spinlock.
 * - Freeing the TLS key used for per-thread error codes.
 *
 * @see eina_shutdown()
 * @see eina_error_init()
 */
Eina_Bool
eina_error_shutdown(void)
{
   Eina_Error_Message *eem, *eem_end;

   eem = _eina_errors;
   eem_end = eem + _eina_errors_count;

   for (; eem < eem_end; eem++)
      if (eem->string_allocated)
         eina_stringshare_del(eem->string);

   free(_eina_errors);
   _eina_errors = NULL;
   _eina_errors_count = 0;
   _eina_errors_allocated = 0;

#ifdef EINA_HAVE_THREADS
   eina_hash_free(_eina_errno_msgs);
   _eina_errno_msgs = NULL;
   eina_spinlock_free(&_eina_errno_msgs_lock);
#endif

   eina_tls_free(_eina_last_key);
   _eina_last_error = 0;

   return EINA_TRUE;
}

/*============================================================================*
*                                   API                                      *
*============================================================================*/

/**
 * @brief Registers a new, dynamically allocated error message.
 *
 * @param msg The error message string to register. This string is copied
 *            using eina_stringshare_add(), so the original can be freed.
 * @return A new #Eina_Error code on success, or 0 on failure.
 *
 * This function allocates space for a new error message and registers it.
 * The returned error code is a unique identifier that can be used with
 * eina_error_msg_get() to retrieve the message. The error code is constructed
 * by setting a specific bit (#EINA_ERROR_REGISTERED_BIT) to distinguish it
 * from system errno values.
 *
 * The internal machinery uses a dynamically growing array to store messages.
 * If memory allocation for the stringshare copy fails, the allocated slot
 * for the error message is rolled back.
 */
EINA_API Eina_Error
eina_error_msg_register(const char *msg)
{
   Eina_Error_Message *eem;

   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, 0);

   eem = _eina_error_msg_alloc();
   if (!eem)
      return 0;

   eem->string_allocated = EINA_TRUE;
   eem->string = eina_stringshare_add(msg);
   if (!eem->string)
     {
        _eina_errors_count--;
        return 0;
     }

   return EINA_ERROR_FROM_INDEX(_eina_errors_count); /* identifier = index + 1 (== _count). */
}

/**
 * @brief Registers a new error message from a static string.
 *
 * @param msg The error message string to register. This string is NOT copied,
 *            so it must be a static literal or have a lifetime that exceeds
 *            the use of eina_error.
 * @return A new #Eina_Error code on success, or 0 on failure.
 *
 * This function is similar to eina_error_msg_register(), but it avoids
 * a string copy by storing the pointer directly. This is more efficient
 * for constant string literals. The `string_allocated` flag in the internal
 * #Eina_Error_Message struct is set to #EINA_FALSE to prevent a double-free
 * during shutdown.
 *
 * @see eina_error_msg_register()
 */
EINA_API Eina_Error
eina_error_msg_static_register(const char *msg)
{
   Eina_Error_Message *eem;

   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, 0);

   eem = _eina_error_msg_alloc();
   if (!eem)
      return 0;

   eem->string_allocated = EINA_FALSE;
   eem->string = msg;
   return EINA_ERROR_FROM_INDEX(_eina_errors_count); /* identifier = index + 1 (== _count). */
}

/**
 * @brief Modifies the message for an already registered error.
 *
 * @param error The #Eina_Error code to modify. Must be a registered error,
 *              not a system errno.
 * @param msg The new message string.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function allows changing the string associated with an error code
 * that was previously created with eina_error_msg_register() or
 * eina_error_msg_static_register().
 *
 * A key detail is how it handles memory:
 * - If the original message was dynamically allocated (via eina_error_msg_register()),
 *   this function will stringshare the new message and free the old one.
 * - If the original message was static (via eina_error_msg_static_register()),
 *   this function simply replaces the pointer, assuming the new message is
 *   also static. The `string_allocated` flag remains #EINA_FALSE.
 *
 * It is not possible to modify system errno messages.
 */
EINA_API Eina_Bool
eina_error_msg_modify(Eina_Error error, const char *msg)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(EINA_ERROR_REGISTERED_CHECK(error), EINA_FALSE);
   error = EINA_ERROR_TO_INDEX(error);
   if (error < 1)
      return EINA_FALSE;

   if ((size_t)error > _eina_errors_count)
      return EINA_FALSE;

   if (_eina_errors[error - 1].string_allocated)
     {
        const char *tmp;

        if (!(tmp = eina_stringshare_add(msg)))
           return EINA_FALSE;

        eina_stringshare_del(_eina_errors[error - 1].string);
        _eina_errors[error - 1].string = tmp;
        return EINA_TRUE;
     }

   _eina_errors[error - 1].string = msg;
   return EINA_TRUE;
}

/**
 * @brief Retrieves the descriptive string for an error code.
 *
 * @param error The error code. This can be either a system `errno` value
 *              (e.g., `ENOMEM`) or an #Eina_Error code returned by
 *              `eina_error_msg_register()`.
 * @return A read-only string containing the error message, or `NULL` if
 *         the error code is not found, is 0, or corresponds to an
 *         "Unknown error" system message.
 *
 * This function translates an error code into a human-readable string.
 *
 * For Eina-registered errors (identified by #EINA_ERROR_REGISTERED_CHECK), it
 * looks up the message in the internal `_eina_errors` array.
 *
 * For system `errno` values, its behavior is more complex to ensure
 * thread-safety, as `strerror()` is not always thread-safe:
 * - On systems with `strerror_r()`, it uses that to get the message.
 * - To avoid repeated calls and to provide a stable string pointer, the
 *   retrieved system messages are cached in a stringshared hash table
 *   (`_eina_errno_msgs`), protected by a spinlock.
 * - On systems without `strerror_r()` (or on Windows), it uses fallbacks.
 *
 * The function intentionally returns `NULL` for `error == 0` and for generic
 * "Unknown error" messages from the system to maintain backward compatibility.
 */
EINA_API const char *
eina_error_msg_get(Eina_Error error)
{
   if (!EINA_ERROR_REGISTERED_CHECK(error))
     {
        const char unknown_prefix[] = "Unknown error ";
        const char *msg;

        /* original behavior of this function did not return strings
         * for unknown errors, so skip 0 ("Success") and
         * "Unknown error $N".
         */
        if (error == 0) return NULL;

#ifndef EINA_HAVE_THREADS
        msg = strerror(error);
        if (strncmp(msg, unknown_prefix, sizeof(unknown_prefix) -1) == 0)
          msg = NULL;
#else /* EINA_HAVE_THREADS */
        /* strerror() is not thread safe, so use a local buffer with
         * strerror_r() and cache resolved strings in a hash so we can
         * return the stringshared refernece.
         */
        if (eina_spinlock_take(&_eina_errno_msgs_lock) != EINA_LOCK_SUCCEED)
          {
             EINA_SAFETY_ERROR("could not take spinlock for errno messages hash!");
             return NULL;
          }
        msg = eina_hash_find(_eina_errno_msgs, &error);
        eina_spinlock_release(&_eina_errno_msgs_lock);

        if (!msg)
          {
             char buf[256] = "";
             const char *str = NULL;

#ifdef HAVE_STRERROR_R
# ifndef STRERROR_R_CHAR_P
             int ret;

             ret = strerror_r(error, buf, sizeof(buf)); /* XSI */
             if (ret == 0)
               str = buf;
             else if (ret == EINVAL)
               return NULL;
# else /* STRERROR_R_CHAR_P */
             str = strerror_r(error, buf, sizeof(buf)); /* GNU */
# endif /* ! STRERROR_R_CHAR_P */
#else
              /* not so good fallback. Usually strerror(err) will
               * return a const string if a known error (what we use),
               * and will return a pointer to a global modified string
               * formatted with "Unknown error XXXX".. which we just
               * ignore... so while it's not super-correct, this
               * should work well.
               */
             eina_strlcpy(buf, strerror(error), sizeof(buf));
             str = buf;
#endif /* HAVE_STRERROR_R */

             if (!str)
               EINA_SAFETY_ERROR("strerror_r() failed");
             else
               {
                  if (strncmp(str, unknown_prefix, sizeof(unknown_prefix) -1) == 0)
                    msg = NULL;
                  else
                    {
                       msg = eina_stringshare_add(str);
                       if (eina_spinlock_take(&_eina_errno_msgs_lock) != EINA_LOCK_SUCCEED)
                         {
                            EINA_SAFETY_ERROR("could not take spinlock for errno messages hash!");
                            return NULL;
                         }
                       eina_hash_add(_eina_errno_msgs, &error, msg);
                       eina_spinlock_release(&_eina_errno_msgs_lock);
                    }
               }
          }
#endif
        return msg;
     }

   error = EINA_ERROR_TO_INDEX(error);

   if (error < 1)
      return NULL;

   if ((size_t)error > _eina_errors_count)
      return NULL;

   return _eina_errors[error - 1].string;
}

/**
 * @brief Retrieves the last error code set for the current context.
 *
 * @return The last #Eina_Error code. Returns 0 (#EINA_ERROR_NO_ERROR)
 *         if no error has been set.
 *
 * This function provides thread-safe error retrieval. Its storage mechanism
 * depends on the execution context:
 * - If running inside the Ecore main loop (`eina_main_loop_is()` is true),
 *   it returns a global `_eina_last_error`. This is a performance
 *   optimization for the common single-threaded main loop case.
 * - Otherwise (e.g., in worker threads), it uses Thread-Local Storage (TLS)
 *   via `eina_tls_get()` to retrieve a per-thread error value.
 *
 * The cast `(Eina_Error)(uintptr_t)` is necessary because TLS stores a `void*`.
 */
EINA_API Eina_Error
eina_error_get(void)
{
   if (eina_main_loop_is())
     return _eina_last_error;

   return (Eina_Error)(uintptr_t) eina_tls_get(_eina_last_key);
}

/**
 * @brief Sets the last error code for the current context.
 *
 * @param err The #Eina_Error code to set. Use 0 (#EINA_ERROR_NO_ERROR)
 *            to clear the error state.
 *
 * This function provides thread-safe error setting. Like eina_error_get(),
 * its storage mechanism depends on the execution context:
 * - If inside the Ecore main loop, it sets a global `_eina_last_error`.
 * - Otherwise, it uses Thread-Local Storage (TLS) via `eina_tls_set()`
 *   to store the error on a per-thread basis.
 *
 * The cast `(void*)(uintptr_t)` is necessary to store the integer error
 * code in the `void*` provided by the TLS API.
 */
EINA_API void
eina_error_set(Eina_Error err)
{
   if (eina_main_loop_is())
     _eina_last_error = err;
   else
     eina_tls_set(_eina_last_key, (void*)(uintptr_t) err);
}

/**
 * @brief Finds a registered #Eina_Error code by its message string.
 *
 * @param msg The error message string to search for. Must not be `NULL`.
 * @return The corresponding #Eina_Error code if found, otherwise 0.
 *
 * This function performs a reverse lookup, searching for an error code
 * that matches the given message string. It iterates through all
 * registered Eina errors.
 *
 * The matching logic is twofold:
 * 1. For stringshared messages (`string_allocated` is true), it first
 *    attempts a fast pointer comparison. This works if `msg` is the
 *    exact stringshared pointer.
 * 2. It then falls back to a full string comparison using `strcmp()` for
 *    all cases.
 *
 * Note: This function only searches through errors registered with
 * eina_error_msg_register() or eina_error_msg_static_register(). It does
 * not search through system `errno` messages.
 */
EINA_API Eina_Error
eina_error_find(const char *msg)
{
   size_t i;

   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, 0);

   for (i = 0; i < _eina_errors_count; i++)
     {
        if (_eina_errors[i].string_allocated)
          {
             if (_eina_errors[i].string == msg)
               return EINA_ERROR_FROM_INDEX(i + 1);
          }
        if (!strcmp(_eina_errors[i].string, msg))
          return EINA_ERROR_FROM_INDEX(i + 1);
     }

   /* not bothering to lookup errno.h as we don't have a "maximum
    * error", thus we'd need to loop up to some arbitrary constant and
    * keep comparing if strerror() returns something meaningful.
    */

   return 0;
}
