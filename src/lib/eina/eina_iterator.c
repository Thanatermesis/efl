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
#include "eina_iterator.h"

#include "eina_list.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/** @internal String identifier for Eina_Iterator magic number. Used for debugging. */
static const char EINA_MAGIC_ITERATOR_STR[] = "Eina Iterator";

/** @internal Macro to check the magic number of an Eina_Iterator. */
#define EINA_MAGIC_CHECK_ITERATOR(d)                            \
   do {                                                          \
        if (!EINA_MAGIC_CHECK(d, EINA_MAGIC_ITERATOR)) {              \
             EINA_MAGIC_FAIL(d, EINA_MAGIC_ITERATOR); }                  \
   } while(0)

/**
 * @endcond
 */


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/**
 * @internal
 * @brief Initialize the iterator module.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function sets up the iterator module of Eina. It is called by
 * eina_init().
 *
 * @see eina_init()
 */
Eina_Bool
eina_iterator_init(void)
{
   return eina_magic_string_set(EINA_MAGIC_ITERATOR, EINA_MAGIC_ITERATOR_STR);
}

/**
 * @internal
 * @brief Shut down the iterator module.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function shuts down the iterator module set up by
 * eina_iterator_init(). It is called by eina_shutdown().
 *
 * @see eina_shutdown()
 */
Eina_Bool
eina_iterator_shutdown(void)
{
   return EINA_TRUE;
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

EINA_API void
eina_iterator_free(Eina_Iterator *iterator)
{
   if (!iterator)
     return;

   EINA_MAGIC_CHECK_ITERATOR(iterator);
   EINA_SAFETY_ON_NULL_RETURN(iterator->free);
   iterator->free(iterator);
}

EINA_API void *
eina_iterator_container_get(Eina_Iterator *iterator)
{
   EINA_MAGIC_CHECK_ITERATOR(iterator);
   EINA_SAFETY_ON_NULL_RETURN_VAL(iterator,                NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(iterator->get_container, NULL);
   return iterator->get_container(iterator);
}

EINA_API Eina_Bool
eina_iterator_next(Eina_Iterator *iterator, void **data)
{
   if (!iterator)
     return EINA_FALSE;

   EINA_MAGIC_CHECK_ITERATOR(iterator);
   EINA_SAFETY_ON_NULL_RETURN_VAL(iterator->next, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(data,           EINA_FALSE);
   return iterator->next(iterator, data);
}

EINA_API void
eina_iterator_foreach(Eina_Iterator *iterator,
                      Eina_Each_Cb cb,
                      const void *fdata)
{
   const void *container;
   void *data;

   if (!iterator)
     return;

   EINA_MAGIC_CHECK_ITERATOR(iterator);
   EINA_SAFETY_ON_NULL_RETURN(iterator->get_container);
   EINA_SAFETY_ON_NULL_RETURN(iterator->next);
   EINA_SAFETY_ON_NULL_RETURN(cb);

   if (!eina_iterator_lock(iterator)) return;

   container = iterator->get_container(iterator);
   while (iterator->next(iterator, &data) == EINA_TRUE)
     {
        if (cb(container, data, (void *)fdata) != EINA_TRUE)
          goto on_exit;
     }

on_exit:
   (void) eina_iterator_unlock(iterator);
}

EINA_API Eina_Bool
eina_iterator_lock(Eina_Iterator *iterator)
{
   EINA_MAGIC_CHECK_ITERATOR(iterator);
   EINA_SAFETY_ON_NULL_RETURN_VAL(iterator, EINA_FALSE);

   if (iterator->lock)
     return iterator->lock(iterator);
   return EINA_TRUE;
}

EINA_API Eina_Bool
eina_iterator_unlock(Eina_Iterator *iterator)
{
   EINA_MAGIC_CHECK_ITERATOR(iterator);
   EINA_SAFETY_ON_NULL_RETURN_VAL(iterator, EINA_FALSE);

   if (iterator->unlock)
     return iterator->unlock(iterator);
   return EINA_TRUE;
}

/**
 * @internal
 * @struct _Eina_Iterator_CArray
 * @brief Structure for an iterator over a NUL-terminated C array of pointers.
 */
typedef struct _Eina_Iterator_CArray Eina_Iterator_CArray;

struct _Eina_Iterator_CArray
{
  Eina_Iterator iterator; /**< The base Eina_Iterator structure. Must be the first member. */

  void** array;   /**< Pointer to the start of the NUL-terminated C array of pointers. */
  void** current; /**< Pointer to the current element in the @p array. */
};

/**
 * @internal
 * @brief Advances the C array iterator to the next element.
 * @param it The C array iterator.
 * @param data Pointer to store the data of the next element.
 * @return #EINA_TRUE if a next element is found, #EINA_FALSE otherwise.
 */
static Eina_Bool
eina_carray_iterator_next(Eina_Iterator_CArray *it, void **data)
{
   if (!it->current || !*it->current)
     return EINA_FALSE;

   *data = *it->current++;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the container (the C array itself) of the C array iterator.
 * @param it The C array iterator.
 * @return Pointer to the C array.
 */
static void**
eina_carray_iterator_get_container(Eina_Iterator_CArray *it)
{
   return it->array;
}

/**
 * @internal
 * @brief Frees the C array iterator structure.
 * @param it The C array iterator to free.
 */
static void
eina_carray_iterator_free(Eina_Iterator_CArray *it)
{
  free(it);
}

EINA_API Eina_Iterator*
eina_carray_iterator_new(void** array)
{
   Eina_Iterator_CArray *it;

   it = calloc(1, sizeof (Eina_Iterator_CArray));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->array = it->current = array;

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(eina_carray_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(
      eina_carray_iterator_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(eina_carray_iterator_free);

   return &it->iterator;
}

/**
 * @internal
 * @struct _Eina_Iterator_CArray_Length
 * @brief Structure for an iterator over a C array with a specified length and element step.
 */
typedef struct _Eina_Iterator_CArray_Length Eina_Iterator_CArray_Length;

struct _Eina_Iterator_CArray_Length
{
   Eina_Iterator iterator; /**< The base Eina_Iterator structure. Must be the first member. */

   void** array;          /**< Pointer to the start of the C array. */
   uintptr_t current;     /**< Byte offset of the current element from the start of @p array. */

   uintptr_t end;         /**< Byte offset of the position immediately after the last element. */
   unsigned int step;     /**< Size of each element in bytes. */
};

/**
 * @internal
 * @brief Advances the C array (with length) iterator to the next element.
 * @param it The C array (with length) iterator.
 * @param data Pointer to store the data of the next element. The data is copied.
 * @return #EINA_TRUE if a next element is found, #EINA_FALSE otherwise.
 */
static Eina_Bool
eina_carray_length_iterator_next(Eina_Iterator_CArray_Length *it, void **data)
{
   if (it->current >= it->end)
     return EINA_FALSE;

   memcpy(data, (void*) it->current, it->step);
   it->current += it->step;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the container (the C array itself) of the C array (with length) iterator.
 * @param it The C array (with length) iterator.
 * @return Pointer to the C array.
 */
static void**
eina_carray_length_iterator_get_container(Eina_Iterator_CArray_Length *it)
{
   return it->array;
}

/**
 * @internal
 * @brief Frees the C array (with length) iterator structure.
 * @param it The C array (with length) iterator to free.
 */
static void
eina_carray_length_iterator_free(Eina_Iterator_CArray_Length *it)
{
   free(it);
}

EINA_API Eina_Iterator *
eina_carray_length_iterator_new(void** array, unsigned int step, unsigned int length)
{
   Eina_Iterator_CArray_Length *it;

   it = calloc(1, sizeof (Eina_Iterator_CArray_Length));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->array = array;
   it->current = (uintptr_t) it->array;
   it->end = it->current + length * step;
   it->step = step;

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(eina_carray_length_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(
      eina_carray_length_iterator_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(eina_carray_length_iterator_free);

   return &it->iterator;
}

/**
 * @internal
 * @struct _Eina_Iterator_Multi
 * @brief Structure for an iterator that chains multiple iterators together.
 * @details This is aliased as Eina_Multi_Iterator for brevity in function signatures.
 */
typedef struct _Eina_Iterator_Multi Eina_Multi_Iterator;

struct _Eina_Iterator_Multi
{
   Eina_Iterator iterator; /**< The base Eina_Iterator structure. Must be the first member. */

   Eina_List *iterators;   /**< A list (`Eina_List`) of `Eina_Iterator *` to iterate over sequentially. */
};

/**
 * @internal
 * @brief Advances the multi-iterator to the next element.
 * @details It tries to get the next element from the current iterator in its list.
 *          If the current iterator is exhausted, it is freed, removed from the list,
 *          and the multi-iterator moves to the next iterator in the list.
 * @param it The multi-iterator.
 * @param data Pointer to store the data of the next element.
 * @return #EINA_TRUE if a next element is found from any of the chained iterators, #EINA_FALSE otherwise.
 */
static Eina_Bool
eina_multi_iterator_next(Eina_Multi_Iterator *it, void **data)
{
   if (!it->iterators)
     return EINA_FALSE;

   // Search for an iterator that do have some data
   while (!eina_iterator_next(eina_list_data_get(it->iterators), data))
     {
        eina_iterator_free(eina_list_data_get(it->iterators));
        it->iterators = eina_list_remove_list(it->iterators, it->iterators);

        if (!it->iterators) return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the container of the currently active sub-iterator in the multi-iterator.
 * @param it The multi-iterator.
 * @return Pointer to the container of the current sub-iterator, or @c NULL if no active sub-iterator.
 */
static void**
eina_multi_iterator_get_container(Eina_Multi_Iterator *it)
{
   if (!it->iterators) return NULL;
   return eina_iterator_container_get(eina_list_data_get(it->iterators));
}

/**
 * @internal
 * @brief Frees the multi-iterator structure and all remaining sub-iterators it owns.
 * @param it The multi-iterator to free.
 */
static void
eina_multi_iterator_free(Eina_Multi_Iterator *it)
{
   Eina_Iterator *itc;

   EINA_LIST_FREE(it->iterators, itc)
     eina_iterator_free(itc);
   free(it);
}

EINA_API Eina_Iterator *
eina_multi_iterator_internal_new(Eina_Iterator *itc, ...)
{
   Eina_Multi_Iterator *it;
   va_list args;

   it = calloc(1, sizeof (Eina_Multi_Iterator));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->iterators = eina_list_append(it->iterators, itc);

   va_start(args, itc);

   while ((itc = (Eina_Iterator *) va_arg(args, Eina_Iterator *)))
     {
        it->iterators = eina_list_append(it->iterators, itc);
     }

   va_end(args);

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(eina_multi_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(
      eina_multi_iterator_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(eina_multi_iterator_free);

   return &it->iterator;
}

/**
 * @internal
 * @struct Eina_Iterator_Filter
 * @brief Structure for an iterator that filters elements from an original iterator.
 */
typedef struct {
   Eina_Iterator iterator;    /**< The base Eina_Iterator structure. Must be the first member. */

   void *data;              /**< User data passed to the filter callback @p cb and free callback @p free. */
   Eina_Iterator *original; /**< The original iterator being filtered. Owned by this filter iterator. */
   Eina_Each_Cb cb;         /**< The filter callback function. */
   Eina_Free_Cb free;       /**< Callback to free @p data when this filter iterator is freed. Can be @c NULL. */
} Eina_Iterator_Filter;

/**
 * @internal
 * @brief Advances the filter iterator to the next element that matches the filter criteria.
 * @details Iterates through the @p original iterator, applying the filter callback @p cb
 *          to each element. Returns the first element for which @p cb returns #EINA_TRUE.
 * @param it The filter iterator.
 * @param data Pointer to store the data of the next matching element.
 * @return #EINA_TRUE if a matching element is found, #EINA_FALSE otherwise.
 */
static Eina_Bool
eina_iterator_filter_next(Eina_Iterator_Filter *it, void **data)
{
   do
     {
        if (!eina_iterator_next(it->original , data))
          return EINA_FALSE;
     }
   while (!it->cb(it->original, *data, it->data));

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the container of the filter iterator, which is the original iterator itself.
 * @param it The filter iterator.
 * @return Pointer to the original iterator (considered the "container" in this context).
 */
static void*
eina_iterator_filter_get_container(Eina_Iterator_Filter *it)
{
   return it->original;
}

/**
 * @internal
 * @brief Frees the filter iterator structure.
 * @details Calls the @p free callback for its user @p data (if provided),
 *          frees the @p original iterator it owns, and then frees the
 *          Eina_Iterator_Filter structure itself.
 * @param it The filter iterator to free.
 */
static void
eina_iterator_filter_free(Eina_Iterator_Filter *it)
{
   if (it->free)
     it->free(it->data);
   eina_iterator_free(it->original);
   free(it);
}

EINA_API Eina_Iterator*
eina_iterator_filter_new(Eina_Iterator *iterator, Eina_Each_Cb filter, Eina_Free_Cb free_cb, void *data)
{
   Eina_Iterator_Filter *it;

   EINA_SAFETY_ON_NULL_RETURN_VAL(iterator, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(filter, NULL);

   it = calloc(1, sizeof(Eina_Iterator_Filter));

   it->original = iterator;
   it->data = data;
   it->cb = filter;
   it->free = free_cb;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(eina_iterator_filter_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(
      eina_iterator_filter_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(eina_iterator_filter_free);

   return &it->iterator;
}

/**
 * @internal
 * @struct Eina_Iterator_Processor
 * @brief Structure for an iterator that processes/transforms elements from an original iterator.
 */
typedef struct {
   Eina_Iterator iterator;    /**< The base Eina_Iterator structure. Must be the first member. */

   void *data;              /**< User data passed to the process callback @p cb and free callback @p free. */
   Eina_Iterator *original; /**< The original iterator whose elements are being processed. Owned by this processor iterator. */
   Eina_Process_Cb cb;      /**< The processing callback function. */
   Eina_Free_Cb free;       /**< Callback to free @p data when this processor iterator is freed. Can be @c NULL. */
} Eina_Iterator_Processor;

/**
 * @internal
 * @brief Advances the processor iterator to the next processed element.
 * @details Gets the next element from the @p original iterator, applies the
 *          processing callback @p cb to transform it, and returns the transformed element.
 * @param it The processor iterator.
 * @param data Pointer to store the data of the next processed element.
 * @return #EINA_TRUE if an element was successfully fetched and processed, #EINA_FALSE if the original iterator is exhausted.
 */
static Eina_Bool
eina_iterator_process_next(Eina_Iterator_Processor *it, void **data)
{
   if (!eina_iterator_next(it->original, data))
     return EINA_FALSE;

   *data = it->cb(it->original, *data, it->data);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the container of the processor iterator, which is the original iterator itself.
 * @param it The processor iterator.
 * @return Pointer to the original iterator (considered the "container" in this context).
 */
static void*
eina_iterator_process_get_container(Eina_Iterator_Processor *it)
{
   return it->original;
}

/**
 * @internal
 * @brief Frees the processor iterator structure.
 * @details Calls the @p free callback for its user @p data (if provided),
 *          frees the @p original iterator it owns, and then frees the
 *          Eina_Iterator_Processor structure itself.
 * @param it The processor iterator to free.
 */
static void
eina_iterator_process_free(Eina_Iterator_Processor *it)
{
   if (it->free)
     it->free(it->data);
   eina_iterator_free(it->original);
   free(it);
}

EINA_API Eina_Iterator*
eina_iterator_processed_new(Eina_Iterator *iterator, Eina_Process_Cb process, Eina_Free_Cb free_cb, void *data)
{
   Eina_Iterator_Processor *it;

   EINA_SAFETY_ON_NULL_RETURN_VAL(iterator, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(process, NULL);

   it = calloc(1, sizeof(Eina_Iterator_Processor));
   it->data = data;
   it->cb = process;
   it->free = free_cb;
   it->original = iterator;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(eina_iterator_process_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(eina_iterator_process_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(eina_iterator_process_free);

   return &it->iterator;
}
