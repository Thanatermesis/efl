/*
 * Copyright (C) 2013, 2014 Mike Blumenkrantz
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

#include <time.h>

#ifdef _WIN32
# include <evil_private.h> /* strptime */
#endif

#include "eina_safety_checks.h"
#include "eina_value.h"
#include "eina_stringshare.h"

/**
 * @internal
 * @brief Structure extending Eina_Value_Struct_Desc to include a reference count.
 * This is used to manage the lifecycle of the struct description, especially
 * when shared across multiple Eina_Value instances.
 */
typedef struct _Eina_Value_Util_Struct_Desc
{
   Eina_Value_Struct_Desc base; /**< The base Eina_Value_Struct_Desc structure. */
   int refcount; /**< The reference count for this structure descriptor. */
} Eina_Value_Util_Struct_Desc;

/**
 * @internal
 * @brief Allocates memory for a struct member based on its description.
 *
 * This function is part of the Eina_Value_Struct_Operations. It increments
 * the reference count of the Eina_Value_Util_Struct_Desc.
 *
 * @param ops The struct operations (unused).
 * @param desc The struct descriptor.
 * @return A pointer to the allocated memory for the struct, or @c NULL on failure.
 */
static void *
_ops_malloc(const Eina_Value_Struct_Operations *ops EINA_UNUSED, const Eina_Value_Struct_Desc *desc)
{
   Eina_Value_Util_Struct_Desc *edesc = (Eina_Value_Util_Struct_Desc*)desc;
   edesc->refcount++;
   //DBG("%p refcount=%d", edesc, edesc->refcount);
   return malloc(desc->size);
}

/**
 * @internal
 * @brief Frees memory previously allocated for a struct member.
 *
 * This function is part of the Eina_Value_Struct_Operations. It decrements
 * the reference count of the Eina_Value_Util_Struct_Desc. If the reference
 * count drops to zero, it also frees the struct descriptor itself and its
 * associated member names.
 *
 * @param ops The struct operations (unused).
 * @param desc The struct descriptor.
 * @param memory Pointer to the memory to be freed.
 */
static void
_ops_free(const Eina_Value_Struct_Operations *ops EINA_UNUSED, const Eina_Value_Struct_Desc *desc, void *memory)
{
   Eina_Value_Util_Struct_Desc *edesc = (Eina_Value_Util_Struct_Desc*) desc;
   edesc->refcount--;
   free(memory);
   //DBG("%p refcount=%d", edesc, edesc->refcount);
   if (edesc->refcount <= 0)
     {
        unsigned i;
        for (i = 0; i < edesc->base.member_count; i++)
          eina_stringshare_del((char *)edesc->base.members[i].name);
        free((Eina_Value_Struct_Member *)edesc->base.members);
        free(edesc);
     }
}

/**
 * @internal
 * @brief Defines the operations for managing Eina_Value structs with refcounting.
 *
 * This static structure provides the Eina_Value core with functions to allocate
 * and free memory for struct types that use the Eina_Value_Util_Struct_Desc.
 */
static Eina_Value_Struct_Operations operations =
{
   EINA_VALUE_STRUCT_OPERATIONS_VERSION, /**< Version of the operations structure. */
   _ops_malloc,
   _ops_free,
   NULL,
   NULL,
   NULL
};

EINA_API Eina_Value_Struct_Desc *
eina_value_util_struct_desc_new(void)
{
   Eina_Value_Util_Struct_Desc *st_desc;

   st_desc = calloc(1, sizeof(Eina_Value_Util_Struct_Desc));
   EINA_SAFETY_ON_NULL_RETURN_VAL(st_desc, NULL);
   st_desc->base.version = EINA_VALUE_STRUCT_DESC_VERSION;
   st_desc->base.ops = &operations;
   return (Eina_Value_Struct_Desc*)st_desc;
}

EINA_API Eina_Value *
eina_value_util_time_string_new(const char *timestr)
{
   Eina_Value *v;
   struct tm tm;
   time_t t;

   if (!strptime(timestr, "%Y%m%dT%H:%M:%S", &tm)) return NULL;
   t = mktime(&tm);
   v = eina_value_new(EINA_VALUE_TYPE_TIMESTAMP);
   if (v) eina_value_set(v, t);
   return v;
}
