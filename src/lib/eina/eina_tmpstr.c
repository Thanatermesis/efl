/* EINA - EFL data type library
 * Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2010
 *                         Carsten Haitzler,
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "eina_config.h"
#include "eina_private.h"
#include "eina_log.h"
#include "eina_lock.h"
#include "eina_share_common.h"

/* undefs EINA_ARG_NONULL() so NULL checks are not compiled out! */
#include "eina_safety_checks.h"
#include "eina_tmpstr.h"

typedef struct _Str Str;

/**
 * @internal
 * @brief Structure for a temporary string entry in a linked list.
 *
 * This structure is used internally to manage temporary strings. It is
 * part of a singly linked list.
 */
struct _Str
{
   size_t length; /**< The length of the string, not including the NUL terminator. */
   Str *next; /**< Pointer to the next temporary string in the list. */
   char *str; /**< The string itself. */
   /**
    * @brief Flag indicating if the string memory is managed separately.
    *
    * If EINA_TRUE, 'str' was allocated separately and must be freed when
    * the tmpstr is deleted.
    * If EINA_FALSE, 'str' points to memory allocated along with the 'Str'
    * struct itself, and should not be freed separately.
    */
   Eina_Bool ma : 1;
};

static Eina_Lock _mutex; /**< Mutex for thread-safe access to the 'strs' list. */
static Str *strs = NULL; /**< Head of the global linked list of temporary strings. */

/*
 * @internal
 * @brief Initializes the tmpstr subsystem.
 *
 * This must be called before any other eina_tmpstr function. It sets up
 * the mutex for thread safety. It is called by eina_init().
 *
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool
eina_tmpstr_init(void)
{
   if (!eina_lock_new(&_mutex)) return EINA_FALSE;
   return EINA_TRUE;
}

/*
 * @internal
 * @brief Shuts down the tmpstr subsystem.
 *
 * This should be called when eina is shut down. It cleans up resources
 * used by the tmpstr subsystem (the mutex). It is called by eina_shutdown().
 *
 * @return EINA_TRUE on success.
 */
Eina_Bool
eina_tmpstr_shutdown(void)
{
   eina_lock_free(&_mutex);
   return EINA_TRUE;
}

EINA_API Eina_Tmpstr *
eina_tmpstr_add_length(const char *str, size_t length)
{
   Str *s;

   if (!str || !length) return NULL;
   /* Allocate space for the Str struct and the string data in one block. */
   s = malloc(sizeof(Str) + length + 1);
   if (!s) return NULL;
   s->length = length;
   /* The string buffer starts immediately after the Str struct. */
   s->str = ((char *)s) + sizeof(Str);
   strncpy(s->str, str, length);
   s->str[length] = '\0';
   /* String is not separately allocated, so 'ma' is false. */
   s->ma = EINA_FALSE;
   eina_lock_take(&_mutex);
   s->next = strs;
   strs = s;
   eina_lock_release(&_mutex);
   return s->str;
}

EINA_API Eina_Tmpstr *
eina_tmpstr_manage_new_length(char *str, size_t length)
{
   Str *s;

   if (!str || !length) return NULL;
   s = calloc(1, sizeof(Str));
   if (!s) return NULL;
   s->length = length;
   s->str = str;
   /* The provided string is from a separate allocation, so mark it for freeing. */
   s->ma = EINA_TRUE;
   eina_lock_take(&_mutex);
   s->next = strs;
   strs = s;
   eina_lock_release(&_mutex);
   return s->str;
}

EINA_API Eina_Tmpstr *
eina_tmpstr_manage_new(char *str)
{
   size_t len;

   if (!str) return NULL;
   len = strlen(str);
   return eina_tmpstr_manage_new_length(str, len);
}

EINA_API Eina_Tmpstr *
eina_tmpstr_add(const char *str)
{
   size_t len;

   if (!str) return NULL;
   len = strlen(str);
   return eina_tmpstr_add_length(str, len);
}

EINA_API void
eina_tmpstr_del(Eina_Tmpstr *tmpstr)
{
   Str *s, *sp;

   if ((!strs) || (!tmpstr)) return;
   eina_lock_take(&_mutex);
   for (sp = NULL, s = strs; s; sp = s, s = s->next)
     {
        /* We can compare pointers because tmpstr must be the exact pointer
         * returned by an add/manage function. */
        if (s->str == tmpstr)
          {
             if (sp) sp->next = s->next;
             else strs = s->next;
             /* If the string was separately allocated, free it. */
             if (s->ma) free(s->str);
             /* Free the list node structure itself. */
             free(s);
             break;
          }
     }
   eina_lock_release(&_mutex);
}

EINA_API size_t
eina_tmpstr_strlen(Eina_Tmpstr *tmpstr)
{
   if (!tmpstr) return 0;
   return eina_tmpstr_len(tmpstr) + 1;
}

EINA_API size_t
eina_tmpstr_len(Eina_Tmpstr *tmpstr)
{
   Str *s;

   if (!tmpstr) return 0;
   if (!strs) return strlen(tmpstr);
   eina_lock_take(&_mutex);
   /* Find the string in the list of tmpstrs to get its cached length. */
   for (s = strs; s; s = s->next)
     {
        if (s->str == tmpstr)
	  {
             size_t ret = s->length;
             eina_lock_release(&_mutex);
             return ret;
	  }
     }
   eina_lock_release(&_mutex);

   /*
    * If the string is not in our list, it's not a tmpstr we manage.
    * Fallback to strlen() to handle regular C strings safely.
    */
   return strlen(tmpstr);
}
