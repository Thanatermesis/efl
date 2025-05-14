/* EINA - EFL data type library
 * Copyright (C) 2002-2008 Cedric Bail
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

#include "eina_config.h"
#include "eina_private.h"

/* undefs EINA_ARG_NONULL() so NULL checks are not compiled out! */
#include "eina_safety_checks.h"
#include "eina_accessor.h"

/*============================================================================*
*                                  Local                                     *
*============================================================================*/

/**
 * @cond LOCAL
 */

static const char EINA_MAGIC_ACCESSOR_STR[] = "Eina Accessor";

#define EINA_MAGIC_CHECK_ACCESSOR(d)                            \
   do {                                                          \
        if (!EINA_MAGIC_CHECK(d, EINA_MAGIC_ACCESSOR)) {              \
             EINA_MAGIC_FAIL(d, EINA_MAGIC_ACCESSOR); }                  \
     } while(0)

/**
 * @endcond
 */

/*============================================================================*
*                                 Global                                     *
*============================================================================*/

/**
 * @internal
 * @brief Initialize the accessor module.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function sets up the accessor module of Eina. It is called by
 * eina_init().
 *
 * @see eina_init()
 */
Eina_Bool
eina_accessor_init(void)
{
   return eina_magic_string_set(EINA_MAGIC_ACCESSOR, EINA_MAGIC_ACCESSOR_STR);
}

/**
 * @internal
 * @brief Shut down the accessor module.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function shuts down the accessor module set up by
 * eina_accessor_init(). It is called by eina_shutdown().
 *
 * @see eina_shutdown()
 */
Eina_Bool
eina_accessor_shutdown(void)
{
   return EINA_TRUE;
}

/*============================================================================*
*                                   API                                      *
*============================================================================*/


EINA_API void
eina_accessor_free(Eina_Accessor *accessor)
{
   if (!accessor)
     return;

   EINA_MAGIC_CHECK_ACCESSOR(accessor);
   EINA_SAFETY_ON_NULL_RETURN(accessor->free);
   accessor->free(accessor);
}

EINA_API void *
eina_accessor_container_get(Eina_Accessor *accessor)
{
   EINA_MAGIC_CHECK_ACCESSOR(accessor);
   EINA_SAFETY_ON_NULL_RETURN_VAL(accessor,                NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(accessor->get_container, NULL);
   return accessor->get_container(accessor);
}

EINA_API Eina_Bool
eina_accessor_data_get(Eina_Accessor *accessor,
                       unsigned int position,
                       void **data)
{
   EINA_MAGIC_CHECK_ACCESSOR(accessor);
   EINA_SAFETY_ON_NULL_RETURN_VAL(accessor,         EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(accessor->get_at, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(data,             EINA_FALSE);
   return accessor->get_at(accessor, position, data);
}

EINA_API void
eina_accessor_over(Eina_Accessor *accessor,
                   Eina_Each_Cb cb,
                   unsigned int start,
                   unsigned int end,
                   const void *fdata)
{
   const void *container;
   void *data;
   unsigned int i;

   if (!accessor) return;

   EINA_MAGIC_CHECK_ACCESSOR(accessor);
   EINA_SAFETY_ON_NULL_RETURN(accessor->get_container);
   EINA_SAFETY_ON_NULL_RETURN(accessor->get_at);
   EINA_SAFETY_ON_NULL_RETURN(cb);
   EINA_SAFETY_ON_FALSE_RETURN(start < end);

   if (!eina_accessor_lock(accessor))
      return;

   container = accessor->get_container(accessor);
   for (i = start; i < end && accessor->get_at(accessor, i, &data) == EINA_TRUE;
        ++i)
      if (cb(container, data, (void *)fdata) != EINA_TRUE)
	 goto on_exit;

 on_exit:
   (void) eina_accessor_unlock(accessor);
}

EINA_API Eina_Bool
eina_accessor_lock(Eina_Accessor *accessor)
{
   EINA_MAGIC_CHECK_ACCESSOR(accessor);
   EINA_SAFETY_ON_NULL_RETURN_VAL(accessor, EINA_FALSE);

   if (accessor->lock)
      return accessor->lock(accessor);
   return EINA_TRUE;
}

EINA_API Eina_Accessor*
eina_accessor_clone(Eina_Accessor *accessor)
{
   EINA_MAGIC_CHECK_ACCESSOR(accessor);
   EINA_SAFETY_ON_NULL_RETURN_VAL(accessor, NULL);

   if (accessor->clone)
      return accessor->clone(accessor);

   return NULL;
}

EINA_API Eina_Bool
eina_accessor_unlock(Eina_Accessor *accessor)
{
   EINA_MAGIC_CHECK_ACCESSOR(accessor);
   EINA_SAFETY_ON_NULL_RETURN_VAL(accessor, EINA_FALSE);

   if (accessor->unlock)
      return accessor->unlock(accessor);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Internal structure for C array accessors.
 * This structure holds the necessary information to access elements
 * in a C array, such as the start and end pointers, and element size.
 * It is used by both value-copying and pointer-providing C array accessors.
 */
typedef struct _Eina_Accessor_CArray_Length Eina_Accessor_CArray_Length;

struct _Eina_Accessor_CArray_Length
{
   Eina_Accessor accessor; /**< The base Eina_Accessor structure. Must be the first member. */

   void** array; /**< Stores the pointer to the start of the C array's data.
                  *   Although typed as `void**`, this is treated as a base memory address
                  *   (effectively like `void*` or `char*`) for pointer arithmetic.
                  *   This matches the type of the `array` parameter passed to the _new functions. */

   void** end;   /**< Stores a pointer to the memory location immediately after the end of the C array data.
                  *   Calculated as `array + (length * step)`. Used for bounds checking.
                  *   Like `array`, this is treated as a base memory address. */
   unsigned int step; /**< Size of a single element in the array, in bytes (e.g., `sizeof(int)`). */
};

/**
 * @internal
 * @brief Get_at callback for C array accessors that copy element values.
 * @param accessor The C array accessor instance.
 * @param idx The zero-based index of the element to retrieve.
 * @param data Output parameter; a pointer to a memory location where the
 *             element's value (accessor->step bytes) will be copied.
 *             When used with EINA_ACCESSOR_FOREACH(acc, i, var), 'data'
 *             will effectively be `(void**)&var`.
 * @return EINA_TRUE if the element is successfully copied, EINA_FALSE if 'idx' is out of bounds.
 *
 * This function calculates the address of the element at 'idx' within the C array
 * and copies 'accessor->step' bytes from that address into the location pointed to by 'data'.
 */
static Eina_Bool
eina_carray_length_accessor_get_at(Eina_Accessor_CArray_Length *accessor, unsigned int idx, void **data)
{
   if ((char*)accessor->array + idx*accessor->step >= (char*)accessor->end)
     return EINA_FALSE;

   memcpy(data, (char*) accessor->array + idx*accessor->step, accessor->step);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Get_at callback for C array accessors that provide pointers to elements.
 * @param accessor The C array accessor instance.
 * @param idx The zero-based index of the element to retrieve a pointer to.
 * @param data Output parameter; a pointer to a `void*` variable. This `void*`
 *             variable will be set to point directly to the element at 'idx'
 *             within the original C array.
 *             When used with EINA_ACCESSOR_FOREACH(acc, i, ptr_var), 'data'
 *             will effectively be `(void**)&ptr_var`.
 * @return EINA_TRUE if the pointer is successfully set, EINA_FALSE if 'idx' is out of bounds.
 *
 * This function calculates the address of the element at 'idx' and stores this
 * address in the `void*` variable pointed to by 'data'.
 */
static Eina_Bool
eina_carray_length_accessor_ptr_get_at(Eina_Accessor_CArray_Length *accessor, unsigned int idx, void **data)
{
   // Calculate the address of the target element.
   char *element_ptr = (char*)accessor->array + (idx * accessor->step);

   // Bounds check: ensure the element is within the array.
   if (element_ptr >= (char*)accessor->end)
     return EINA_FALSE;

   // 'data' is a pointer to a void* variable (e.g., &my_ptr_var from EINA_ACCESSOR_FOREACH, cast to void**).
   // This sets that void* variable (*data) to point directly to the element within the C array.
   *data = (void*)element_ptr;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Get_container callback for C array accessors.
 * @param accessor The C array accessor instance.
 * @return Returns the `void**` pointer that was originally passed as the C array
 *         during the accessor's creation. This typically represents the base
 *         address of the C array.
 */
static void**
eina_carray_length_accessor_get_container(Eina_Accessor_CArray_Length *accessor)
{
   return accessor->array;
}

/**
 * @internal
 * @brief Free callback for C array accessors.
 * @param accessor The C array accessor instance to be freed.
 *
 * This function releases the memory allocated for the Eina_Accessor_CArray_Length structure.
 * It does not free the C array itself, as the accessor does not own the array data.
 */
static void
eina_carray_length_accessor_free(Eina_Accessor_CArray_Length *accessor)
{
   // The Eina_Accessor_CArray_Length structure itself was allocated with calloc.
   free(accessor);
}

EINA_API Eina_Accessor *
eina_carray_length_accessor_new(void** array, unsigned int step, unsigned int length)
{
   Eina_Accessor_CArray_Length *accessor;

   accessor = calloc(1, sizeof (Eina_Accessor_CArray_Length));
   if (!accessor) return NULL;

   EINA_MAGIC_SET(&accessor->accessor, EINA_MAGIC_ACCESSOR);

   accessor->array = array;
   // Calculate the end pointer: base address + (number of elements * size of each element).
   // This pointer points one position *past* the last valid element's data block.
   // It's used for bounds checking in the get_at functions.
   // The casts to (char*) ensure byte-level pointer arithmetic. The result is cast back
   // to void** to match the type of accessor->end.
   accessor->end = (void**)((char*)array + length * step);
   accessor->step = step;

   accessor->accessor.version = EINA_ACCESSOR_VERSION;
   accessor->accessor.get_at = FUNC_ACCESSOR_GET_AT(eina_carray_length_accessor_get_at);
   accessor->accessor.get_container = FUNC_ACCESSOR_GET_CONTAINER(
      eina_carray_length_accessor_get_container);
   accessor->accessor.free = FUNC_ACCESSOR_FREE(eina_carray_length_accessor_free);

   return &accessor->accessor;
}

EINA_API Eina_Accessor *
eina_carray_length_ptr_accessor_new(void** array, unsigned int step, unsigned int length)
{
   Eina_Accessor_CArray_Length *accessor;

   accessor = calloc(1, sizeof (Eina_Accessor_CArray_Length));
   if (!accessor) return NULL;

   EINA_MAGIC_SET(&accessor->accessor, EINA_MAGIC_ACCESSOR);

   accessor->array = array;
   // Calculate the end pointer, similar to eina_carray_length_accessor_new.
   // This pointer points one position *past* the last valid element's data block
   // and is used for bounds checking.
   accessor->end = (void**)((char*)array + length * step);
   accessor->step = step;

   accessor->accessor.version = EINA_ACCESSOR_VERSION;
   accessor->accessor.get_at = FUNC_ACCESSOR_GET_AT(eina_carray_length_accessor_ptr_get_at);
   accessor->accessor.get_container = FUNC_ACCESSOR_GET_CONTAINER(
      eina_carray_length_accessor_get_container);
   accessor->accessor.free = FUNC_ACCESSOR_FREE(eina_carray_length_accessor_free);

   return &accessor->accessor;
}
