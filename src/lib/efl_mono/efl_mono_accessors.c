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



#include "Eina.h"

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

// This just a wrapper around carray acessors for pinned managed data
// It uses the free callback to unpin the managed data so it can be
// reclaimed by the GC back in C# world.
/**
 * @internal
 * @brief Structure that wraps an Eina_Accessor for data owned by the Mono runtime.
 *
 * This structure holds an underlying Eina_CArray_Accessor and manages the lifetime
 * of pinned managed data. When the accessor is freed, a callback is invoked to
 * unpin the data in the C# world, allowing the GC to reclaim it.
 */
struct _Eina_Mono_Owned_Accessor
{
   Eina_Accessor accessor; /**< The public Eina_Accessor interface. */

   Eina_Accessor *carray_acc; /**< The underlying C array accessor. */
   void *free_data; /**< Opaque handle to the pinned managed data. */
   Eina_Free_Cb free_cb; /**< Callback function to unpin the managed data. */
};

/**
 * @internal
 * @brief Typedef for the internal Eina_Mono_Owned_Accessor structure.
 */
typedef struct _Eina_Mono_Owned_Accessor Eina_Mono_Owned_Accessor;

/**
 * @internal
 * @brief Retrieves data from the accessor at a given index.
 *
 * This function is a wrapper around `eina_accessor_data_get` for the
 * underlying C array accessor.
 *
 * @param accessor The Eina_Mono_Owned_Accessor instance.
 * @param idx The index of the data to retrieve.
 * @param data Pointer to store the retrieved data.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool eina_mono_owned_carray_get_at(Eina_Mono_Owned_Accessor *accessor, unsigned int idx, void **data)
{
   return eina_accessor_data_get(accessor->carray_acc, idx, data);
}

/**
 * @internal
 * @brief Gets the container of the accessor.
 *
 * In this context, the "container" is considered to be the underlying
 * C array accessor.
 *
 * @param accessor The Eina_Mono_Owned_Accessor instance.
 * @return A pointer to the underlying C array accessor, cast to void**.
 */
static void** eina_mono_owned_carray_get_container(Eina_Mono_Owned_Accessor *accessor)
{
  // Is another accessor a valid container?
  return (void**)&accessor->carray_acc;
}

/**
 * @internal
 * @brief Frees the Eina_Mono_Owned_Accessor and its associated resources.
 *
 * This function invokes the `free_cb` to unpin the managed data,
 * then frees the underlying C array accessor and the Eina_Mono_Owned_Accessor itself.
 *
 * @param accessor The Eina_Mono_Owned_Accessor instance to free.
 */
static void eina_mono_owned_carray_free_cb(Eina_Mono_Owned_Accessor* accessor)
{
   accessor->free_cb(accessor->free_data);

   free(accessor->carray_acc); // From Eina_CArray_Length_Accessor implementation...

   free(accessor);
}

/**
 * @brief Creates a new Eina_Accessor for a C array with a specified length,
 *        where the underlying data is owned and managed by the Mono runtime.
 *
 * This accessor is designed to work with data that has been pinned in C# memory.
 * When the accessor is no longer needed and freed, the provided `free_cb` callback
 * is invoked with the `handle` parameter. This callback is intended to unpin
 * the data on the C# side, allowing the .NET Garbage Collector to reclaim it.
 *
 * @param array Pointer to the start of the C array.
 *              Example: `MyObjectType** my_array = ...;`
 *                       `eina_mono_owned_carray_length_accessor_new((void**)my_array, ...)`
 * @param step The size of each element in the array (in bytes).
 *             Typically `sizeof(MyObjectType*)` or `sizeof(void*)`.
 * @param length The number of elements in the array.
 * @param free_cb A callback function that will be called when the accessor is freed.
 *                This function is responsible for unpinning the managed data.
 *                It receives the `handle` as its argument.
 * @param handle An opaque pointer (handle) to the pinned managed data. This handle
 *               is passed to `free_cb` upon accessor destruction.
 * @return A new Eina_Accessor instance on success, or @c NULL on failure (e.g., memory allocation failed).
 *         The returned accessor should be freed using eina_accessor_free() when no longer needed.
 */
EAPI Eina_Accessor *eina_mono_owned_carray_length_accessor_new(void** array, unsigned int step, unsigned int length, Eina_Free_Cb free_cb, void *handle)
{
   Eina_Mono_Owned_Accessor *accessor = calloc(1, sizeof(Eina_Mono_Owned_Accessor));
   if (!accessor) return NULL;

   EINA_MAGIC_SET(&accessor->accessor, EINA_MAGIC_ACCESSOR);

   accessor->carray_acc = eina_carray_length_accessor_new(array, step, length);

   accessor->accessor.version = EINA_ACCESSOR_VERSION;
   accessor->accessor.get_at = FUNC_ACCESSOR_GET_AT(eina_mono_owned_carray_get_at);
   accessor->accessor.get_container = FUNC_ACCESSOR_GET_CONTAINER(eina_mono_owned_carray_get_container);
   accessor->accessor.free = FUNC_ACCESSOR_FREE(eina_mono_owned_carray_free_cb);

   // The managed callback to be called with the pinned data.
   accessor->free_cb = free_cb;
   // The managed pinned data to be unpinned.
   accessor->free_data = handle;

   return &accessor->accessor;
}
