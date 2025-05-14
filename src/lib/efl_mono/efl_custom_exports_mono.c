/**
 * @file
 * @brief This file provides a set of C functions exported for use by the Mono/.NET bindings
 *        for the Enlightenment Foundation Libraries (EFL). It includes wrappers for managing
 *        Eo objects, handling thread safety, interacting with Eina data structures (lists,
 *        arrays, hashes, values), and other utility functions necessary for the C# bindings
 *        to communicate with the native EFL code.
 */
/*
 * Copyright 2019 by its authors. See AUTHORS.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef _WIN32
# include "evil_private.h" /* setenv */
#endif

#include "Eo.h"
#include "Eina.h"
#include "Ecore.h"

#include <stdlib.h>
#include <string.h>

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
#  define EAPI __declspec(dllexport)
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif /* ! _WIN32 */

/**
 * @brief Gets the key used to store the C# wrapper supervisor on an Eo object.
 *
 * This key is used with efl_key_data_set() and efl_key_data_get() to associate
 * a C# object (the supervisor) with its native Eo counterpart.
 *
 * @return The constant string key "__c#_wrapper_supervisor".
 */
EAPI const char *efl_mono_wrapper_supervisor_key_get()
{
   return "__c#_wrapper_supervisor";
}

/**
 * @brief Retrieves the C# wrapper supervisor associated with an Eo object.
 *
 * @param eo The Eo object from which to retrieve the supervisor.
 * @return A pointer to the C# wrapper supervisor, or NULL if not set.
 */
EAPI void *efl_mono_wrapper_supervisor_get(Eo *eo)
{
   return efl_key_data_get(eo, efl_mono_wrapper_supervisor_key_get());
}

/**
 * @brief Sets the C# wrapper supervisor for an Eo object.
 *
 * @param eo The Eo object to associate the supervisor with.
 * @param ws A pointer to the C# wrapper supervisor.
 */
EAPI void efl_mono_wrapper_supervisor_set(Eo *eo, void *ws)
{
   efl_key_data_set(eo, efl_mono_wrapper_supervisor_key_get(), ws);
}

/**
 * @brief Callback function type for freeing a C# wrapper supervisor.
 * @param obj The Eo object whose C# wrapper supervisor needs to be freed.
 */
typedef void (*Efl_Mono_Free_Wrapper_Supervisor_Cb)(Eo *obj);

/** @internal Global callback pointer for freeing wrapper supervisors. */
static Efl_Mono_Free_Wrapper_Supervisor_Cb _efl_mono_free_wrapper_supervisor_call = NULL;

/**
 * @brief Sets the callback function used to free C# wrapper supervisors.
 *
 * This function is called from the C# side to register the method that
 * should be invoked when a native object's associated C# wrapper needs
 * to be disposed.
 *
 * @param free_wrapper_supervisor_cb The callback function to be used.
 */
EAPI void efl_mono_wrapper_supervisor_callbacks_set(Efl_Mono_Free_Wrapper_Supervisor_Cb free_wrapper_supervisor_cb)
{
   _efl_mono_free_wrapper_supervisor_call = free_wrapper_supervisor_cb;
}

/**
 * @brief Disposes of the C# wrapper supervisor associated with an Eo object.
 *
 * This function directly calls the registered supervisor freeing callback.
 * It is intended to be called when the native Eo object is being finalized
 * and its managed counterpart needs to be released.
 *
 * @param obj The Eo object whose C# wrapper supervisor should be disposed.
 */
EAPI void efl_mono_native_dispose(Eo *obj)
{
   _efl_mono_free_wrapper_supervisor_call(obj);
}

/**
 * @brief Thread-safely disposes of the C# wrapper supervisor associated with an Eo object.
 *
 * This function schedules the disposal of the C# wrapper supervisor on the main loop,
 * ensuring thread safety.
 *
 * @param obj The Eo object whose C# wrapper supervisor should be disposed.
 */
EAPI void efl_mono_thread_safe_native_dispose(Eo *obj)
{
   ecore_main_loop_thread_safe_call_async((Ecore_Cb)efl_mono_native_dispose, obj);
}

/** @internal Callback for efl_unref to be used with ecore_main_loop_thread_safe_call_async. */
static void _efl_mono_unref_cb(void *obj)
{
   efl_unref(obj);
}

/**
 * @brief Thread-safely decrements the reference count of an Eo object.
 *
 * Schedules an efl_unref() call on the main loop.
 *
 * @param obj The Eo object to unreference.
 */
EAPI void efl_mono_thread_safe_efl_unref(Eo* obj)
{
   ecore_main_loop_thread_safe_call_async(_efl_mono_unref_cb, obj);
}

/**
 * @brief Thread-safely executes an Eina_Free_Cb callback.
 *
 * Schedules the provided free callback with its data on the main loop.
 * This is useful for freeing resources that must be freed on the main thread.
 *
 * @param free_cb The Eina_Free_Cb callback to execute.
 * @param cb_data The data to pass to the callback.
 */
EAPI void efl_mono_thread_safe_free_cb_exec(Eina_Free_Cb free_cb, void* cb_data)
{
   ecore_main_loop_thread_safe_call_async(free_cb, cb_data);
}

/** @internal Callback for eina_list_free to be used with ecore_main_loop_thread_safe_call_async. */
static void _efl_mono_list_free_cb(void *l)
{
   eina_list_free(l);
}

/**
 * @brief Thread-safely frees an Eina_List.
 *
 * Schedules an eina_list_free() call on the main loop.
 *
 * @param list The Eina_List to free.
 */
EAPI void efl_mono_thread_safe_eina_list_free(Eina_List* list)
{
   ecore_main_loop_thread_safe_call_async(_efl_mono_list_free_cb, list);
}

/**
 * @internal
 * @brief Structure to hold data for thread-safe promise rejection.
 */
typedef struct _Efl_Mono_Promise_Reject_Data
{
   Eina_Promise *promise; /**< The promise to reject. */
   Eina_Error err;      /**< The error code for rejection. */
} Efl_Mono_Promise_Reject_Data;

/** @internal Callback for eina_promise_reject to be used with ecore_main_loop_thread_safe_call_async. */
static void _efl_mono_promise_reject_cb(void *data)
{
   Efl_Mono_Promise_Reject_Data *d = data;
   eina_promise_reject(d->promise, d->err);
   free(d);
}

/**
 * @brief Thread-safely rejects an Eina_Promise.
 *
 * Schedules an eina_promise_reject() call on the main loop.
 *
 * @param p The Eina_Promise to reject.
 * @param err The Eina_Error code to reject the promise with.
 */
EAPI void efl_mono_thread_safe_promise_reject(Eina_Promise *p, Eina_Error err)
{
   Efl_Mono_Promise_Reject_Data *d = malloc(sizeof(Efl_Mono_Promise_Reject_Data));
   d->promise = p;
   d->err = err;
   ecore_main_loop_thread_safe_call_async(_efl_mono_promise_reject_cb, d);
}

/**
 * @brief Allocates a block of memory of the specified size.
 *
 * Wrapper for `malloc`.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
EAPI void *efl_mono_native_alloc(unsigned int size)
{
   return malloc(size);
}

/**
 * @brief Fills a block of memory with a specified byte value.
 *
 * Wrapper for `memset`.
 *
 * @param ptr Pointer to the block of memory to fill.
 * @param fill The value to be set. The value is passed as an int, but the function fills
 *             the block of memory using the unsigned char conversion of this value.
 * @param count The number of bytes to be set to the value.
 */
EAPI void efl_mono_native_memset(void *ptr, unsigned int fill, unsigned int count)
{
   memset(ptr, fill, count);
}

/**
 * @brief Frees a previously allocated block of memory.
 *
 * Wrapper for `free`.
 *
 * @param ptr Pointer to the memory block to be deallocated.
 */
EAPI void efl_mono_native_free(void *ptr)
{
   free(ptr);
}

/**
 * @brief Frees a memory block pointed to by a pointer reference.
 *
 * This function takes a pointer to a pointer (`void **ptr`). It frees the memory
 * block that `*ptr` points to. This is useful when the pointer itself is managed
 * by C# and needs to be deallocated from native code.
 *
 * @param ptr A pointer to the pointer that holds the address of the memory to be freed.
 *            If `ptr` is NULL, the function does nothing.
 */
EAPI void efl_mono_native_free_ref(void **ptr)
{
   if (!ptr) return;
   free(*ptr);
}

/**
 * @brief Deletes a reference to an Eina_Stringshare string.
 *
 * This function takes a pointer to a pointer (`void **str`). It calls `eina_stringshare_del`
 * on the string that `*str` points to.
 *
 * @param str A pointer to the pointer that holds the address of the Eina_Stringshare to be deleted.
 *            If `str` is NULL, the function does nothing.
 */
EAPI void efl_mono_native_stringshare_del_ref(void **str)
{
   if (!str) return;
   eina_stringshare_del(*str);
}

/**
 * @brief Allocates memory and copies data into it.
 *
 * Allocates `size` bytes of memory and copies `size` bytes from `val` into the new block.
 *
 * @param val Pointer to the data to be copied. If NULL, returns NULL.
 * @param size The number of bytes to allocate and copy.
 * @return A pointer to the newly allocated and copied memory block, or NULL on failure or if `val` is NULL.
 */
EAPI void *efl_mono_native_alloc_copy(const void *val, unsigned int size)
{
    if (!val) return NULL;
    void *r = malloc(size);
    memcpy(r, val, size);
    return r;
}

/**
 * @brief Duplicates a string.
 *
 * Wrapper for `strdup`.
 *
 * @param str The string to duplicate. If NULL, returns NULL.
 * @return A pointer to the newly allocated duplicated string, or NULL on failure or if `str` is NULL.
 *         The returned string must be freed by the caller.
 */
EAPI const char *efl_mono_native_strdup(const char *str)
{
    if (!str) return NULL;
    return strdup(str);
}

/**
 * @brief Compares two pointers by their memory addresses.
 *
 * @param ptr1 The first pointer.
 * @param ptr2 The second pointer.
 * @return An integer less than, equal to, or greater than zero if `ptr1` is found,
 *         respectively, to be less than, to match, or be greater than `ptr2`.
 */
EAPI int efl_mono_native_ptr_compare(const void *ptr1, const void *ptr2)
{
    uintptr_t addr1 = (uintptr_t)ptr1;
    uintptr_t addr2 = (uintptr_t)ptr2;
    return (addr1 > addr2) - (addr1 < addr2);
}

/**
 * @brief Gets the address of the native pointer comparison function.
 *
 * @return A function pointer to `efl_mono_native_ptr_compare`.
 */
EAPI Eina_Compare_Cb efl_mono_native_ptr_compare_addr_get()
{
    return efl_mono_native_ptr_compare;
}

/**
 * @brief Gets the address of the standard C string comparison function (`strcmp`).
 *
 * @return A function pointer to `strcmp`, cast to `Eina_Compare_Cb`.
 */
EAPI Eina_Compare_Cb efl_mono_native_str_compare_addr_get()
{
    return (Eina_Compare_Cb)strcmp;
}

/**
 * @brief Gets the address of the standard C memory free function (`free`).
 *
 * @return A function pointer to `free`, cast to `Eina_Free_Cb`.
 */
EAPI Eina_Free_Cb efl_mono_native_free_addr_get()
{
    return (Eina_Free_Cb)free;
}

/**
 * @brief Gets the address of the `eina_stringshare_del` function.
 *
 * @return A function pointer to `eina_stringshare_del`, cast to `Eina_Free_Cb`.
 */
EAPI Eina_Free_Cb efl_mono_native_stringshare_del_addr_get()
{
    return (Eina_Free_Cb)eina_stringshare_del;
}

/**
 * @brief Gets the address of the thread-safe `efl_unref` wrapper function.
 *
 * @return A function pointer to `efl_mono_thread_safe_efl_unref`, cast to `Eina_Free_Cb`.
 */
EAPI Eina_Free_Cb efl_mono_native_efl_unref_addr_get()
{
    return (Eina_Free_Cb)efl_mono_thread_safe_efl_unref;
}

/**
 * @internal
 * @brief Callback used as a substitute constructor to avoid issues with top-level C# constructors.
 *
 * This function calls the super constructor of the object, effectively bypassing
 * the immediate class's constructor logic that might be problematic when invoked
 * directly from C# during object instantiation.
 *
 * @param data User data (unused).
 * @param obj The Eo object being constructed.
 * @return The constructed Eo object.
 */
static Eo *_efl_mono_avoid_top_level_constructor_cb(void *data EINA_UNUSED, Eo *obj)
{
   return efl_constructor(efl_super(obj, efl_class_get(obj)));
}

/**
 * @brief Gets the address of the callback function that avoids top-level C# constructors.
 *
 * This is used by the C# bindings to handle specific constructor scenarios.
 *
 * @return A function pointer to `_efl_mono_avoid_top_level_constructor_cb`.
 */
EAPI Efl_Substitute_Ctor_Cb efl_mono_avoid_top_level_constructor_callback_addr_get()
{
   return &_efl_mono_avoid_top_level_constructor_cb;
}

// Environment wrappers //
/**
 * @brief Gets the value of an environment variable.
 *
 * Wrapper for `getenv`.
 *
 * @param name The name of the environment variable.
 * @return A pointer to the value of the environment variable, or NULL if not found.
 *         The returned string should not be modified or freed by the caller.
 */
EAPI const char *efl_mono_native_getenv(const char *name)
{
   return getenv(name);
}

/**
 * @brief Sets the value of an environment variable.
 *
 * Wrapper for `setenv`.
 *
 * @param name The name of the environment variable.
 * @param value The value to set for the environment variable.
 * @param overwrite If non-zero, an existing variable with the same name will be overwritten.
 *                  If zero, and the variable exists, its value is not changed.
 * @return 0 on success, or an Eina_Error code on failure.
 */
EAPI Eina_Error efl_mono_native_setenv(const char *name, const char *value, int overwrite)
{
   return setenv(name, value, overwrite);
}

// Iterator Wrapper //

/**
 * @internal
 * @brief Wrapper structure for Eina_Iterator to be used by Mono.
 *
 * This structure holds the original Eina_Iterator and provides a compatible
 * Eina_Iterator interface for C# to interact with, potentially with custom
 * next callbacks.
 */
typedef struct _Eina_Iterator_Wrapper_Mono
{
   Eina_Iterator iterator; /**< The public iterator interface. Must be the first member. */
   Eina_Iterator *internal; /**< The actual internal Eina_Iterator being wrapped. */
} Eina_Iterator_Wrapper_Mono;

/** @internal Retrieves the container of the wrapped internal iterator. */
static void *eina_iterator_wrapper_get_container_mono(Eina_Iterator_Wrapper_Mono *it)
{
   return eina_iterator_container_get(it->internal);
}

/** @internal Frees the wrapper and the internal iterator. */
static void eina_iterator_wrapper_free_mono(Eina_Iterator_Wrapper_Mono *it)
{
   eina_iterator_free(it->internal);
   free(it);
}

/**
 * @internal
 * @brief Creates a new Eina_Iterator wrapper.
 *
 * This function wraps an existing `internal` Eina_Iterator with a new
 * Eina_Iterator_Wrapper_Mono structure. It allows specifying a custom `next_cb`
 * function, which is useful when the data extraction logic for Mono differs
 * from the standard iterator behavior (e.g., for inlists or hash iterators
 * where an extra dereference might be needed).
 *
 * @param internal The Eina_Iterator to wrap.
 * @param next_cb The custom callback function for `iterator.next`.
 * @return A pointer to the new Eina_Iterator (the `iterator` field of the wrapper),
 *         or NULL on failure or if `internal` is NULL.
 */
static Eina_Iterator *eina_iterator_wrapper_new_mono(Eina_Iterator *internal, Eina_Iterator_Next_Callback next_cb)
{
   if (!internal) return NULL;

   Eina_Iterator_Wrapper_Mono *it = calloc(1, sizeof(Eina_Iterator_Wrapper_Mono));
   if (!it)
     {
        eina_iterator_free(internal);
        return NULL;
     }

   it->internal = internal;

   it->iterator.next = next_cb;

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(eina_iterator_wrapper_get_container_mono);
   it->iterator.free = FUNC_ITERATOR_FREE(eina_iterator_wrapper_free_mono);

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   return &it->iterator;
}

// Array //

/**
 * @brief Clears an Eina_Array, removing all its elements.
 * @param array The array to clean.
 * @see eina_array_clean
 */
EAPI void eina_array_clean_custom_export_mono(Eina_Array *array) EINA_ARG_NONNULL(1)
{
   eina_array_clean(array);
}

/**
 * @brief Pushes data onto the end of an Eina_Array.
 * @param array The array to push data onto.
 * @param data The data to push.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @see eina_array_push
 * @example
 * // Assuming `my_array` is an Eina_Array* for integers:
 * // int val = 10;
 * // eina_array_push_custom_export_mono(my_array, &val);
 */
EAPI Eina_Bool eina_array_push_custom_export_mono(Eina_Array *array, const void *data) EINA_ARG_NONNULL(1, 2)
{
   return eina_array_push(array, data);
}

/**
 * @brief Pops data from the end of an Eina_Array.
 * @param array The array to pop data from.
 * @return The data popped from the array, or NULL if the array is empty or on error.
 * @see eina_array_pop
 */
EAPI void *eina_array_pop_custom_export_mono(Eina_Array *array) EINA_ARG_NONNULL(1)
{
   return eina_array_pop(array);
}

/**
 * @brief Gets data from an Eina_Array at a specific index.
 * @param array The array to get data from.
 * @param idx The index of the data to retrieve.
 * @return The data at the specified index, or NULL if the index is out of bounds or on error.
 * @see eina_array_data_get
 */
EAPI void *eina_array_data_get_custom_export_mono(const Eina_Array *array, unsigned int idx) EINA_ARG_NONNULL(1)
{
   return eina_array_data_get(array, idx);
}

/**
 * @brief Sets data in an Eina_Array at a specific index.
 * @param array The array to set data in.
 * @param idx The index at which to set the data.
 * @param data The data to set.
 * @see eina_array_data_set
 * @example
 * // Assuming `my_array` is an Eina_Array* for integers and index 0 exists:
 * // int new_val = 20;
 * // eina_array_data_set_custom_export_mono(my_array, 0, &new_val);
 */
EAPI void eina_array_data_set_custom_export_mono(const Eina_Array *array, unsigned int idx, const void *data) EINA_ARG_NONNULL(1)
{
   eina_array_data_set(array, idx, data);
}

/**
 * @brief Gets the number of elements in an Eina_Array.
 * @param array The array to count elements from.
 * @return The number of elements in the array.
 * @see eina_array_count
 */
EAPI unsigned int eina_array_count_custom_export_mono(const Eina_Array *array) EINA_ARG_NONNULL(1)
{
   return eina_array_count(array);
}

/**
 * @brief Iterates over an Eina_Array and calls a callback for each element.
 * @param array The array to iterate over.
 * @param cb The callback function to call for each element.
 * @param fdata User data to pass to the callback function.
 * @return EINA_TRUE if the iteration completed, EINA_FALSE if it was aborted by the callback.
 * @see eina_array_foreach
 */
EAPI Eina_Bool eina_array_foreach_custom_export_mono(Eina_Array *array, Eina_Each_Cb cb, void *fdata)
{
   return eina_array_foreach(array, cb, fdata);
}

/**
 * @brief Inserts data into an Eina_Array at a specific index.
 *
 * This function pushes the data to the end of the array and then shifts elements
 * to make space at the desired index. This is not the most efficient way to insert
 * but provides the required functionality.
 *
 * @param array The array to insert data into.
 * @param index The index at which to insert the data.
 * @param data The data to insert.
 * @example
 * // To insert `new_element` at `my_array[2]`:
 * // eina_array_insert_at_custom_export_mono(my_array, 2, new_element_ptr);
 * // Elements originally at index 2 and above will be shifted to the right.
 */
EAPI void eina_array_insert_at_custom_export_mono(Eina_Array* array, unsigned int index, void* const data)
{
   eina_array_push(array, data); // Push to end first to ensure space and correct count
   // Shift elements from the new end towards the insertion point
   for (unsigned int i = eina_array_count(array) - 1; i > index; --i)
     {
        void* tmp = eina_array_data_get(array, i);
        eina_array_data_set(array, i, eina_array_data_get(array, i - 1));
        eina_array_data_set(array, i - 1, tmp);
     }
}

// List //

/**
 * @brief Gets the last element in an Eina_List.
 * @param list The list.
 * @return The last Eina_List node, or NULL if the list is empty.
 * @see eina_list_last
 */
EAPI Eina_List *eina_list_last_custom_export_mono(const Eina_List *list)
{
   return eina_list_last(list);
}

/**
 * @brief Gets the next element in an Eina_List.
 * @param list The current list node.
 * @return The next Eina_List node, or NULL if this is the last node.
 * @see eina_list_next
 */
EAPI Eina_List *eina_list_next_custom_export_mono(const Eina_List *list)
{
   return eina_list_next(list);
}

/**
 * @brief Gets the previous element in an Eina_List.
 * @param list The current list node.
 * @return The previous Eina_List node, or NULL if this is the first node.
 * @see eina_list_prev
 */
EAPI Eina_List *eina_list_prev_custom_export_mono(const Eina_List *list)
{
   return eina_list_prev(list);
}

/**
 * @brief Gets the data stored in an Eina_List node.
 * @param list The list node.
 * @return The data stored in the node.
 * @see eina_list_data_get
 */
EAPI void *eina_list_data_get_custom_export_mono(const Eina_List *list)
{
   return eina_list_data_get(list);
}

/**
 * @brief Sets the data stored in an Eina_List node.
 * @param list The list node.
 * @param data The new data to store.
 * @return The old data that was stored in the node.
 * @see eina_list_data_set
 */
EAPI void *eina_list_data_set_custom_export_mono(Eina_List *list, const void *data)
{
   return eina_list_data_set(list, data);
}

/**
 * @brief Gets the number of elements in an Eina_List.
 * @param list The list.
 * @return The number of elements in the list.
 * @see eina_list_count
 */
EAPI unsigned int eina_list_count_custom_export_mono(const Eina_List *list)
{
   return eina_list_count(list);
}

/**
 * @brief Gets the data from the last element of an Eina_List.
 * @param list The list.
 * @return The data of the last element, or NULL if the list is empty.
 * @see eina_list_last_data_get
 */
EAPI void *eina_list_last_data_get_custom_export_mono(const Eina_List *list)
{
   return eina_list_last_data_get(list);
}

// Inlist //

/**
 * @internal
 * @brief Structure for an Eina_Inlist node suitable for Mono.
 *
 * The `mem_start` field is a placeholder to get a pointer to the actual data
 * embedded within the inlist node, as `EINA_INLIST_CONTAINER_GET` would typically be used.
 * The iterator wrapper for inlists will return a pointer to `mem_start`.
 */
typedef struct _Inlist_Node_Mono
{
   EINA_INLIST;      /**< Eina_Inlist macro, must be first. */
   char mem_start;   /**< Placeholder for the start of the user data within the node. */
} Inlist_Node_Mono;

/**
 * @internal
 * @brief Custom 'next' function for an Eina_Inlist iterator wrapper.
 *
 * This function advances the internal inlist iterator and retrieves the data.
 * For inlists, the data is part of the node structure itself. This function
 * extracts a pointer to the data portion (`mem_start`) of the `Inlist_Node_Mono`.
 *
 * @param it The iterator wrapper.
 * @param data Pointer to store the retrieved data item.
 * @return EINA_TRUE if an item was retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool eina_inlist_iterator_wrapper_next_mono(Eina_Iterator_Wrapper_Mono *it, void **data)
{
   Inlist_Node_Mono *node = NULL;

   if (!eina_iterator_next(it->internal, (void**)&node)) // node will point to the EINA_INLIST part
     return EINA_FALSE;

   if (data) // If a node was retrieved
     *data = &node->mem_start; // Return pointer to the data part of the node

   return EINA_TRUE;
}

/**
 * @brief Creates a new Eina_Iterator wrapper for an Eina_Inlist.
 *
 * This uses a custom 'next' callback (`eina_inlist_iterator_wrapper_next_mono`)
 * to correctly extract data from inlist nodes for Mono.
 *
 * @param in_list The Eina_Inlist to iterate over.
 * @return A new Eina_Iterator (wrapper) for the inlist.
 */
EAPI Eina_Iterator *eina_inlist_iterator_wrapper_new_custom_export_mono(const Eina_Inlist *in_list)
{
   return eina_iterator_wrapper_new_mono(eina_inlist_iterator_new(in_list), FUNC_ITERATOR_NEXT(eina_inlist_iterator_wrapper_next_mono));
}

/**
 * @brief Gets the first node of an Eina_Inlist.
 * @param list The inlist.
 * @return The first node, or NULL if the list is empty.
 * @see eina_inlist_first
 */
EAPI Eina_Inlist *eina_inlist_first_custom_export_mono(const Eina_Inlist *list)
{
   return eina_inlist_first(list);
}

/**
 * @brief Gets the last node of an Eina_Inlist.
 * @param list The inlist.
 * @return The last node, or NULL if the list is empty.
 * @see eina_inlist_last
 */
EAPI Eina_Inlist *eina_inlist_last_custom_export_mono(const Eina_Inlist *list)
{
   return eina_inlist_last(list);
}

/**
 * @brief Gets the next node in an Eina_Inlist.
 * @param list The current inlist node.
 * @return The next node, or NULL if this is the last node.
 * @see EINA_INLIST_GET next field
 */
EAPI Eina_Inlist *eina_inlist_next_custom_export_mono(const Eina_Inlist *list)
{
   if (list)
     return list->next;
   return NULL;
}

/**
 * @brief Gets the previous node in an Eina_Inlist.
 * @param list The current inlist node.
 * @return The previous node, or NULL if this is the first node.
 * @see EINA_INLIST_GET prev field
 */
EAPI Eina_Inlist *eina_inlist_prev_custom_export_mono(const Eina_Inlist *list)
{
   if (list)
     return list->prev;
   return NULL;
}

// Hash //

/**
 * @internal
 * @brief Custom 'next' function for an Eina_Hash key iterator wrapper (for pointer keys).
 *
 * This function advances the internal hash key iterator. Since hash key iterators
 * for pointer keys return a `void **` (a pointer to the key), this function
 * dereferences it to get the actual key `void *`.
 *
 * @param it The iterator wrapper.
 * @param data Pointer to store the retrieved key.
 * @return EINA_TRUE if a key was retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool eina_hash_iterator_ptr_key_wrapper_next_mono(Eina_Iterator_Wrapper_Mono *it, void **data)
{
   void **ptr = NULL; // eina_hash_iterator_key_new returns void** for pointer keys

   if (!eina_iterator_next(it->internal, (void**)&ptr))
     return EINA_FALSE;

   if (data)
     *data = *ptr; // Dereference to get the actual key

   return EINA_TRUE;
}

/**
 * @brief Creates a new Eina_Iterator wrapper for the keys of an Eina_Hash (pointer keys).
 *
 * This uses a custom 'next' callback (`eina_hash_iterator_ptr_key_wrapper_next_mono`)
 * to correctly extract pointer keys for Mono.
 *
 * @param hash The Eina_Hash to iterate over its keys.
 * @return A new Eina_Iterator (wrapper) for the hash keys.
 */
EAPI Eina_Iterator *eina_hash_iterator_ptr_key_wrapper_new_custom_export_mono(const Eina_Hash *hash)
{
   return eina_iterator_wrapper_new_mono(eina_hash_iterator_key_new(hash), FUNC_ITERATOR_NEXT(eina_hash_iterator_ptr_key_wrapper_next_mono));
}

// Eina Value //
/** @brief Returns the EINA_VALUE_TYPE_UCHAR type constant. */
EAPI const Eina_Value_Type *type_byte() {
   return EINA_VALUE_TYPE_UCHAR;
}
/** @brief Returns the EINA_VALUE_TYPE_CHAR type constant. */
EAPI const Eina_Value_Type *type_sbyte() {
   return EINA_VALUE_TYPE_CHAR;
}
/** @brief Returns the EINA_VALUE_TYPE_SHORT type constant. */
EAPI const Eina_Value_Type *type_short() {
   return EINA_VALUE_TYPE_SHORT;
}
/** @brief Returns the EINA_VALUE_TYPE_USHORT type constant. */
EAPI const Eina_Value_Type *type_ushort() {
   return EINA_VALUE_TYPE_USHORT;
}
/** @brief Returns the EINA_VALUE_TYPE_INT type constant. */
EAPI const Eina_Value_Type *type_int32() {
   return EINA_VALUE_TYPE_INT;
}
/** @brief Returns the EINA_VALUE_TYPE_UINT type constant. */
EAPI const Eina_Value_Type *type_uint32() {
   return EINA_VALUE_TYPE_UINT;
}
/** @brief Returns the EINA_VALUE_TYPE_LONG type constant. */
EAPI const Eina_Value_Type *type_long() {
   return EINA_VALUE_TYPE_LONG;
}
/** @brief Returns the EINA_VALUE_TYPE_ULONG type constant. */
EAPI const Eina_Value_Type *type_ulong() {
   return EINA_VALUE_TYPE_ULONG;
}
/** @brief Returns the EINA_VALUE_TYPE_INT64 type constant. */
EAPI const Eina_Value_Type *type_int64() {
   return EINA_VALUE_TYPE_INT64;
}
/** @brief Returns the EINA_VALUE_TYPE_UINT64 type constant. */
EAPI const Eina_Value_Type *type_uint64() {
   return EINA_VALUE_TYPE_UINT64;
}
/** @brief Returns the EINA_VALUE_TYPE_STRING type constant. */
EAPI const Eina_Value_Type *type_string() {
   return EINA_VALUE_TYPE_STRING;
}
/** @brief Returns the EINA_VALUE_TYPE_FLOAT type constant. */
EAPI const Eina_Value_Type *type_float() {
   return EINA_VALUE_TYPE_FLOAT;
}
/** @brief Returns the EINA_VALUE_TYPE_DOUBLE type constant. */
EAPI const Eina_Value_Type *type_double() {
   return EINA_VALUE_TYPE_DOUBLE;
}
/** @brief Returns the EINA_VALUE_TYPE_ARRAY type constant. */
EAPI const Eina_Value_Type *type_array() {
   return EINA_VALUE_TYPE_ARRAY;
}
/** @brief Returns the EINA_VALUE_TYPE_LIST type constant. */
EAPI const Eina_Value_Type *type_list() {
   return EINA_VALUE_TYPE_LIST;
}
/** @brief Returns the EINA_VALUE_TYPE_HASH type constant. */
EAPI const Eina_Value_Type *type_hash() {
   return EINA_VALUE_TYPE_HASH;
}
/** @brief Returns the EINA_VALUE_TYPE_ERROR type constant. */
EAPI const Eina_Value_Type *type_error() {
   return EINA_VALUE_TYPE_ERROR;
}
/** @brief Returns the EINA_VALUE_TYPE_OBJECT type constant. */
EAPI const Eina_Value_Type *type_object() {
   return EINA_VALUE_TYPE_OBJECT;
}

/** @brief Returns the EINA_VALUE_TYPE_OPTIONAL type constant. */
EAPI const Eina_Value_Type *type_optional() {
   return EINA_VALUE_TYPE_OPTIONAL;
}

/**
 * @brief Gets the size of the Eina_Value structure.
 * @return The size of Eina_Value in bytes.
 */
EAPI size_t eina_value_sizeof()
{
   return sizeof(Eina_Value);
}

/**
 * @def EINA_SET_WRAPPER(N, T)
 * @brief Macro to generate wrapper functions for eina_value_set for various types.
 *
 * This macro creates functions like `eina_value_set_wrapper_int(Eina_Value *value, int new_value)`.
 * These wrappers are provided for convenience for the Mono bindings.
 *
 * @param N Suffix for the function name (e.g., `int`, `string`).
 * @param T The C type of the value being set (e.g., `int`, `const char *`).
 */
#define EINA_SET_WRAPPER(N, T) EAPI Eina_Bool eina_value_set_wrapper_##N(Eina_Value *value, T new_value) \
{ \
    return eina_value_set(value, new_value); \
}

EINA_SET_WRAPPER(char, char)
EINA_SET_WRAPPER(uchar, unsigned char)
EINA_SET_WRAPPER(short, short)
EINA_SET_WRAPPER(ushort, unsigned short)
EINA_SET_WRAPPER(int, int)
EINA_SET_WRAPPER(uint, unsigned int)
EINA_SET_WRAPPER(long, long)
EINA_SET_WRAPPER(ulong, unsigned long)
EINA_SET_WRAPPER(float, float)
EINA_SET_WRAPPER(double, double)
EINA_SET_WRAPPER(string, const char *)
EINA_SET_WRAPPER(ptr, void *)

/**
 * @def EINA_CONTAINER_SET_WRAPPER(N, T)
 * @brief Macro to generate wrapper functions for setting elements in Eina_Value containers (array/list).
 *
 * This macro creates functions like `eina_value_container_set_wrapper_int(Eina_Value *value, int i, int new_value)`.
 * It checks if the Eina_Value is an array or list and calls the appropriate set function.
 *
 * @param N Suffix for the function name.
 * @param T The C type of the value being set.
 */
#define EINA_CONTAINER_SET_WRAPPER(N, T) EAPI Eina_Bool eina_value_container_set_wrapper_##N(Eina_Value *value, int i, T new_value) \
{ \
    const Eina_Value_Type *tp = eina_value_type_get(value); \
    if (tp == EINA_VALUE_TYPE_ARRAY) \
        return eina_value_array_set(value, i, new_value); \
    else if (tp == EINA_VALUE_TYPE_LIST) \
        return eina_value_list_set(value, i, new_value); \
    else \
        return EINA_FALSE; \
}

EINA_CONTAINER_SET_WRAPPER(char, char)
EINA_CONTAINER_SET_WRAPPER(uchar, unsigned char)
EINA_CONTAINER_SET_WRAPPER(short, short)
EINA_CONTAINER_SET_WRAPPER(ushort, unsigned short)
EINA_CONTAINER_SET_WRAPPER(int, int)
EINA_CONTAINER_SET_WRAPPER(uint, unsigned int)
EINA_CONTAINER_SET_WRAPPER(long, long)
EINA_CONTAINER_SET_WRAPPER(ulong, unsigned long)
EINA_CONTAINER_SET_WRAPPER(float, float)
EINA_CONTAINER_SET_WRAPPER(double, double)
EINA_CONTAINER_SET_WRAPPER(string, const char *)
EINA_CONTAINER_SET_WRAPPER(ptr, void *)

/**
 * @def EINA_CONTAINER_APPEND_WRAPPER(N, T)
 * @brief Macro to generate wrapper functions for appending elements to Eina_Value containers (array/list).
 *
 * This macro creates functions like `eina_value_container_append_wrapper_int(Eina_Value *value, int new_value)`.
 * It checks if the Eina_Value is an array or list and calls the appropriate append function.
 *
 * @param N Suffix for the function name.
 * @param T The C type of the value being appended.
 */
#define EINA_CONTAINER_APPEND_WRAPPER(N, T) EAPI Eina_Bool eina_value_container_append_wrapper_##N(Eina_Value *value, T new_value) \
{ \
    const Eina_Value_Type *tp = eina_value_type_get(value); \
    if (tp == EINA_VALUE_TYPE_ARRAY) \
        return eina_value_array_append(value, new_value); \
    else if (tp == EINA_VALUE_TYPE_LIST) \
        return eina_value_list_append(value, new_value); \
    else \
        return EINA_FALSE; \
}

EINA_CONTAINER_APPEND_WRAPPER(char, char)
EINA_CONTAINER_APPEND_WRAPPER(uchar, unsigned char)
EINA_CONTAINER_APPEND_WRAPPER(short, short)
EINA_CONTAINER_APPEND_WRAPPER(ushort, unsigned short)
EINA_CONTAINER_APPEND_WRAPPER(int, int)
EINA_CONTAINER_APPEND_WRAPPER(uint, unsigned int)
EINA_CONTAINER_APPEND_WRAPPER(long, long)
EINA_CONTAINER_APPEND_WRAPPER(ulong, unsigned long)
EINA_CONTAINER_APPEND_WRAPPER(float, float)
EINA_CONTAINER_APPEND_WRAPPER(double, double)
EINA_CONTAINER_APPEND_WRAPPER(string, const char *)
EINA_CONTAINER_APPEND_WRAPPER(ptr, void *)

/**
 * @def EINA_CONTAINER_INSERT_WRAPPER(N, T)
 * @brief Macro to generate wrapper functions for inserting elements into Eina_Value containers (array/list).
 *
 * This macro creates functions like `eina_value_container_insert_wrapper_int(Eina_Value *value, unsigned int pos, int new_value)`.
 * It checks if the Eina_Value is an array or list and calls the appropriate insert function.
 *
 * @param N Suffix for the function name.
 * @param T The C type of the value being inserted.
 */
#define EINA_CONTAINER_INSERT_WRAPPER(N, T) EAPI Eina_Bool eina_value_container_insert_wrapper_##N(Eina_Value *value, unsigned int position, T new_value) \
{ \
    const Eina_Value_Type *tp = eina_value_type_get(value); \
    if (tp == EINA_VALUE_TYPE_ARRAY) \
        return eina_value_array_insert(value, position, new_value); \
    else if (tp == EINA_VALUE_TYPE_LIST) \
        return eina_value_list_insert(value, position, new_value); \
    else \
        return EINA_FALSE; \
}

EINA_CONTAINER_INSERT_WRAPPER(char, char)
EINA_CONTAINER_INSERT_WRAPPER(uchar, unsigned char)
EINA_CONTAINER_INSERT_WRAPPER(short, short)
EINA_CONTAINER_INSERT_WRAPPER(ushort, unsigned short)
EINA_CONTAINER_INSERT_WRAPPER(int, int)
EINA_CONTAINER_INSERT_WRAPPER(uint, unsigned int)
EINA_CONTAINER_INSERT_WRAPPER(long, long)
EINA_CONTAINER_INSERT_WRAPPER(ulong, unsigned long)
EINA_CONTAINER_INSERT_WRAPPER(float, float)
EINA_CONTAINER_INSERT_WRAPPER(double, double)
EINA_CONTAINER_INSERT_WRAPPER(string, const char *)
EINA_CONTAINER_INSERT_WRAPPER(ptr, void *)

/**
 * @brief Gets an element from an Eina_Value container (array or list) by index.
 *
 * This function determines if the Eina_Value is an array or list and calls the
 * appropriate get function.
 *
 * @param value The Eina_Value container (must be an array or list).
 * @param i The index of the element to retrieve.
 * @param output Pointer to store the retrieved element. The type of `output` must match
 *               the subtype of the container.
 */
EAPI void eina_value_container_get_wrapper(const Eina_Value *value, int i, void *output)
{
    const Eina_Value_Type *tp = eina_value_type_get(value);
    if (tp == EINA_VALUE_TYPE_ARRAY)
        eina_value_array_get(value, i, output);
    else if (tp == EINA_VALUE_TYPE_LIST)
        eina_value_list_get(value, i, output);
}

/**
 * @brief Sets up an Eina_Value with a specific type.
 *
 * Wrapper for `eina_value_setup`.
 *
 * @param value The Eina_Value to set up.
 * @param type The Eina_Value_Type to set for the value.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool eina_value_setup_wrapper(Eina_Value *value,
                                   const Eina_Value_Type *type)
{
   return eina_value_setup(value, type);
}

/**
 * @brief Flushes an Eina_Value, releasing its held resources and resetting its type.
 *
 * Wrapper for `eina_value_flush`.
 *
 * @param value The Eina_Value to flush.
 */
EAPI void eina_value_flush_wrapper(Eina_Value *value)
{
   eina_value_flush(value);
}

/**
 * @brief Gets the type of an Eina_Value.
 *
 * Wrapper for `eina_value_type_get`. Handles the case where the Eina_Value
 * might be "empty" (value->type is NULL) by returning NULL, as eina_value_type_get
 * expects a non-NULL value->type.
 *
 * @param value The Eina_Value.
 * @return The Eina_Value_Type of the value, or NULL if the value is NULL or empty.
 */
EAPI const Eina_Value_Type *eina_value_type_get_wrapper(const Eina_Value *value)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(value, NULL);

   // Can't pass null value type (for Empty values) to value_type_get.
   if (value->type == NULL) // Check if the Eina_Value itself indicates an "empty" or uninitialized state
     return NULL;
   return eina_value_type_get(value);
}

/**
 * @brief Gets the content of an Eina_Value.
 *
 * Wrapper for `eina_value_get`.
 *
 * @param value The Eina_Value.
 * @param output Pointer to store the retrieved content. The type of `output` must match
 *               the type of the Eina_Value.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool eina_value_get_wrapper(const Eina_Value *value, void *output)
{
   return eina_value_get(value, output);
}

/**
 * @brief Compares two Eina_Value instances.
 *
 * Wrapper for `eina_value_compare`.
 *
 * @param this The first Eina_Value.
 * @param other The second Eina_Value.
 * @return An integer less than, equal to, or greater than zero if `this` is found,
 *         respectively, to be less than, to match, or be greater than `other`.
 *         Returns -1 if types are not comparable or an error occurs.
 */
EAPI int eina_value_compare_wrapper(const Eina_Value *this, const Eina_Value *other)
{
   return eina_value_compare(this, other);
}

/**
 * @brief Sets up an Eina_Value as an array.
 *
 * Wrapper for `eina_value_array_setup`.
 *
 * @param array The Eina_Value to set up as an array.
 * @param subtype The Eina_Value_Type for the elements of the array.
 * @param step The allocation step size for the array.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool eina_value_array_setup_wrapper(Eina_Value *array, const Eina_Value_Type *subtype, unsigned int step)
{
   return eina_value_array_setup(array, subtype, step);
}

/**
 * @brief Sets up an Eina_Value as a list.
 *
 * Wrapper for `eina_value_list_setup`.
 *
 * @param list The Eina_Value to set up as a list.
 * @param subtype The Eina_Value_Type for the elements of the list.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool eina_value_list_setup_wrapper(Eina_Value *list, const Eina_Value_Type *subtype)
{
   return eina_value_list_setup(list, subtype);
}

/**
 * @brief Appends a value to an Eina_Value array using a va_list.
 *
 * Wrapper for `eina_value_array_append` that takes a `va_list`.
 * This is typically used when the value to append is passed through varargs.
 * The C# side needs to marshal the value appropriately into a `va_list`
 * representation if calling this directly, or more likely, use type-specific
 * append wrappers.
 *
 * @param array The Eina_Value array.
 * @param argp A `va_list` containing the value to append.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool eina_value_array_append_wrapper(Eina_Value *array, va_list argp)
{
   return eina_value_array_append(array, argp);
}

/**
 * @brief Appends a value to an Eina_Value list using a va_list.
 *
 * Wrapper for `eina_value_list_append` that takes a `va_list`.
 * Similar to `eina_value_array_append_wrapper`, this is for varargs scenarios.
 *
 * @param list The Eina_Value list.
 * @param argp A `va_list` containing the value to append.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool eina_value_list_append_wrapper(Eina_Value *list, va_list argp)
{
   return eina_value_list_append(list, argp);
}

/**
 * @brief Gets an element from an Eina_Value array by index.
 *
 * Wrapper for `eina_value_array_get`.
 *
 * @param array The Eina_Value array.
 * @param i The index of the element to retrieve.
 * @param output Pointer to store the retrieved element.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool eina_value_array_get_wrapper(const Eina_Value *array, int i, void *output)
{
   return eina_value_array_get(array, i, output);
}

/**
 * @brief Gets an element from an Eina_Value list by index.
 *
 * Wrapper for `eina_value_list_get`.
 *
 * @param list The Eina_Value list.
 * @param i The index of the element to retrieve.
 * @param output Pointer to store the retrieved element.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool eina_value_list_get_wrapper(const Eina_Value *list, int i, void *output)
{
   return eina_value_list_get(list, i, output);
}

/**
 * @brief Sets an element in an Eina_Value array at a specific index.
 *
 * Wrapper for `eina_value_array_set`. The `value` parameter is a pointer
 * to the data to be set.
 *
 * @param array The Eina_Value array.
 * @param i The index at which to set the element.
 * @param value Pointer to the new value for the element.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool eina_value_array_set_wrapper(Eina_Value *array, int i, void *value)
{
   return eina_value_array_set(array, i, value);
}

/**
 * @brief Sets an element in an Eina_Value list at a specific index.
 *
 * Wrapper for `eina_value_list_set`. The `value` parameter is a pointer
 * to the data to be set.
 *
 * @param list The Eina_Value list.
 * @param i The index at which to set the element.
 * @param value Pointer to the new value for the element.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool eina_value_list_set_wrapper(Eina_Value *list, int i, void *value)
{
   return eina_value_list_set(list, i, value);
}

/**
 * @brief Gets the subtype of an Eina_Value array.
 *
 * This function retrieves the Eina_Value_Type of the elements stored within the array.
 * It is not a direct wrapper but provides access to this information.
 *
 * @param array The Eina_Value array.
 * @return The Eina_Value_Type of the array's elements, or NULL on error.
 */
// Not actually a wrapper, but keeping the naming convention for functions on this file.
EAPI const Eina_Value_Type* eina_value_array_subtype_get_wrapper(const Eina_Value *array)
{
   Eina_Value_Array array_value;
   if (!eina_value_get(array, &array_value)) return NULL; // Ensure value is valid and get its content
   return array_value.subtype;
}

/**
 * @brief Gets the subtype of an Eina_Value list.
 *
 * This function retrieves the Eina_Value_Type of the elements stored within the list.
 * It is not a direct wrapper but provides access to this information.
 *
 * @param list The Eina_Value list.
 * @return The Eina_Value_Type of the list's elements, or NULL on error.
 */
EAPI const Eina_Value_Type* eina_value_list_subtype_get_wrapper(const Eina_Value *list)
{
   Eina_Value_List list_value;
   if (!eina_value_get(list, &list_value)) return NULL; // Ensure value is valid and get its content
   return list_value.subtype;
}

/**
 * @brief Gets the number of elements in an Eina_Value array.
 *
 * Wrapper for `eina_value_array_count`.
 *
 * @param array The Eina_Value array.
 * @return The number of elements in the array.
 */
EAPI unsigned int eina_value_array_count_wrapper(const Eina_Value *array)
{
   return eina_value_array_count(array);
}

/**
 * @brief Gets the number of elements in an Eina_Value list.
 *
 * Wrapper for `eina_value_list_count`.
 *
 * @param list The Eina_Value list.
 * @return The number of elements in the list.
 */
EAPI unsigned int eina_value_list_count_wrapper(const Eina_Value *list)
{
   return eina_value_list_count(list);
}

/**
 * @brief Checks if an Eina_Value of optional type is empty (has no value).
 *
 * Wrapper for `eina_value_optional_empty_is`.
 *
 * @param value The Eina_Value (must be of optional type).
 * @param[out] empty Pointer to a boolean that will be set to EINA_TRUE if the optional is empty,
 *                   EINA_FALSE otherwise.
 * @return EINA_TRUE on success, EINA_FALSE if `value` is not an optional type or on other errors.
 */
EAPI Eina_Bool eina_value_optional_empty_is_wrapper(const Eina_Value *value, Eina_Bool *empty)
{
   return eina_value_optional_empty_is(value, empty);
}

/**
 * @brief Gets the underlying type of an Eina_Value of optional type.
 *
 * Wrapper for `eina_value_optional_type_get`.
 *
 * @param value The Eina_Value (must be of optional type).
 * @return The Eina_Value_Type of the value contained within the optional,
 *         or NULL if the optional is empty or `value` is not an optional type.
 */
EAPI const Eina_Value_Type *eina_value_optional_type_get_wrapper(Eina_Value *value)
{
   return eina_value_optional_type_get(value);
}

/**
 * @brief Sets an Eina_Value to hold a pointer.
 *
 * This is a convenience wrapper for `eina_value_set` when the type is known to be a pointer type,
 * or when setting an Eina_Value of type EINA_VALUE_TYPE_OBJECT or a custom pointer type.
 * It effectively calls `eina_value_set(value, ptr)`.
 *
 * @param value The Eina_Value to set.
 * @param ptr The pointer to store in the Eina_Value.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @see eina_value_set
 * @see eina_value_type_get() to check the type before calling.
 * @note This function is similar to `eina_value_set_wrapper_ptr`. `eina_value_pset` is the
 *       original EFL function name for setting pointer-like types.
 */
EAPI Eina_Bool eina_value_pset_wrapper(Eina_Value *value, void *ptr)
{
   return eina_value_pset(value, ptr);
}
