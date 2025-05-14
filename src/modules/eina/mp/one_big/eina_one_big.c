/* EINA - EFL data type library
 * Copyright (C) 2010 Cedric BAIL, Vincent Torri
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
#include <string.h>

#include <assert.h>

#include "eina_config.h"
#include "eina_mempool.h"
#include "eina_trash.h"
#include "eina_inlist.h"
#include "eina_log.h"
#include "eina_lock.h"
#include "eina_thread.h"
#include "eina_cpu.h"

#ifndef NVALGRIND
# include <memcheck.h>
#endif

#include "eina_private.h"

#ifdef INF
#undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_eina_mempool_log_dom, __VA_ARGS__)

#ifdef WRN
#undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_eina_one_big_mp_log_dom, __VA_ARGS__)

#define OVER_MEM_TO_LIST(_pool, _over_mem)                      \
  ((Eina_Inlist *)(((char *)_over_mem) + (_pool)->offset_to_item_inlist))

#define OVER_MEM_FROM_LIST(_pool, _node)        \
  ((void *)(((char *)_node) - (_pool)->offset_to_item_inlist))

/**
 * @internal
 * @brief Log domain for the one_big mempool.
 * Initialized to -1 and registered in one_big_init().
 */
static int _eina_one_big_mp_log_dom = -1;

/**
 * @internal
 * @brief Structure representing a "one_big" memory pool.
 *
 * This mempool strategy allocates a large contiguous block of memory upfront
 * (the "base") for a fixed number of items of a specific size. If this initial
 * block is exhausted, subsequent allocations are handled individually (these
 * are "over" allocations) and tracked in a linked list.
 */
typedef struct _One_Big One_Big;
struct _One_Big
{
   const char *name; /**< Name of the memory pool, for debugging/logging. */

   unsigned int item_size; /**< Size of each item in the pool (aligned). */
   int offset_to_item_inlist; /**< Offset from the start of an "over" allocated memory block to its Eina_Inlist node. This is used when items are larger than the space needed for the Eina_Inlist struct itself, ensuring the payload is properly aligned. */

   int usage; /**< Number of items currently allocated from the main 'base' block or the 'empty' list. */
   int over;  /**< Number of items allocated individually (not from 'base'). */

   unsigned int served; /**< Number of items served from the 'base' block so far. */
   unsigned int max;    /**< Maximum number of items the 'base' block can hold. */
   unsigned char *base; /**< Pointer to the large contiguous block of memory. NULL if not yet allocated or allocation failed. */

   Eina_Trash *empty;   /**< A stack (LIFO) of freed items from the 'base' block, available for reuse. */
   Eina_Inlist *over_list; /**< Linked list of items allocated individually after 'base' was exhausted. Each node in this list points to the start of an Eina_Inlist struct, which is followed by the actual item data. */

#ifdef EINA_HAVE_DEBUG_THREADS
   Eina_Thread self; /**< Thread ID that created the pool, for debugging thread safety. */
#endif
   Eina_Lock mutex; /**< Mutex to protect concurrent access to the pool. */
};

/**
 * @internal
 * @brief Allocates memory from the One_Big pool.
 *
 * This function attempts to allocate an item of `pool->item_size`.
 * It first checks the `empty` list for reusable items from the `base` block.
 * If `empty` is empty, it tries to serve from the `base` block if space is available.
 * If `base` is full or not yet allocated, it attempts to allocate the `base` block.
 * If `base` allocation fails or `base` is full, it falls back to a regular `malloc`
 * for the item (an "over" allocation), adding it to the `over_list`.
 *
 * @param data Pointer to the One_Big pool structure.
 * @param size The requested size (unused, as item_size is fixed for the pool).
 * @return Pointer to the allocated memory, or NULL on failure.
 */
static void *
eina_one_big_malloc(void *data, EINA_UNUSED unsigned int size)
{
   One_Big *pool = data;
   unsigned char *mem = NULL;

   if (!eina_lock_take(&pool->mutex))
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(eina_thread_equal(pool->self, eina_thread_self()));
#endif
     }

   if (pool->empty)
     {
#ifndef NVALGRIND
        VALGRIND_MAKE_MEM_DEFINED(pool->empty, pool->item_size);
#endif
        mem = eina_trash_pop(&pool->empty);
        pool->usage++;
        goto on_exit;
     }

   if (!pool->base)
     {
	pool->base = malloc(pool->item_size * pool->max);
	if (!pool->base) goto retry_smaller;
#ifndef NVALGRIND
        VALGRIND_MAKE_MEM_NOACCESS(pool->base, pool->item_size * pool->max);
#endif
     }

   if (pool->served < pool->max)
     {
        mem = pool->base + (pool->served++ *pool->item_size);
        pool->usage++;
        goto on_exit;
     }

 retry_smaller:
   mem = malloc(sizeof(Eina_Inlist) + pool->offset_to_item_inlist);
   if (mem)
     {
        Eina_Inlist *node = OVER_MEM_TO_LIST(pool, mem);
        pool->over++;
        /* Only need to zero list elements and not the payload here */
        memset(node, 0, sizeof(Eina_Inlist));
        pool->over_list = eina_inlist_append(pool->over_list, node);
     }
#ifndef NVALGRIND
   VALGRIND_MAKE_MEM_NOACCESS(mem, pool->item_size);
#endif

on_exit:
   eina_lock_release(&pool->mutex);

#ifndef NVALGRIND
   VALGRIND_MEMPOOL_ALLOC(pool, mem, pool->item_size);
#endif
   return mem;
}

/**
 * @internal
 * @brief Frees memory previously allocated from the One_Big pool.
 *
 * If the pointer `ptr` belongs to the main `base` block, it's pushed onto
 * the `empty` list for reuse.
 * If `ptr` belongs to an "over" allocation, it's removed from the `over_list`
 * and freed using `free()`.
 *
 * @param data Pointer to the One_Big pool structure.
 * @param ptr Pointer to the memory to be freed.
 */
static void
eina_one_big_free(void *data, void *ptr)
{
   One_Big *pool = data;

   if (!eina_lock_take(&pool->mutex))
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(eina_thread_equal(pool->self, eina_thread_self()));
#endif
     }

   if ((void *)pool->base <= ptr
       && ptr < (void *)(pool->base + (pool->max * pool->item_size)))
     {
        eina_trash_push(&pool->empty, ptr);
        pool->usage--;

#ifndef NVALGRIND
        VALGRIND_MAKE_MEM_NOACCESS(ptr, pool->item_size);
#endif
     }
   else
     {
#ifndef NDEBUG
        Eina_Inlist *it;
#endif
        Eina_Inlist *il;

        il = OVER_MEM_TO_LIST(pool, ptr);

#ifndef NDEBUG
        for (it = pool->over_list; it != NULL; it = it->next)
          if (it == il) break;

        assert(it != NULL);
#endif

        pool->over_list = eina_inlist_remove(pool->over_list, il);

#ifndef NVALGRIND
        VALGRIND_MEMPOOL_FREE(pool, ptr);
#endif
        free(ptr);
        pool->over--;
     }

   eina_lock_release(&pool->mutex);
}

/**
 * @internal
 * @brief Checks if a given pointer was allocated from this One_Big pool.
 *
 * This function verifies if `ptr` is a valid memory address managed by the pool.
 * It checks if `ptr` falls within the `base` block and is not on the `empty` list (i.e., not already freed).
 * It also checks if `ptr` is part of the `over_list`.
 *
 * @param data Pointer to the One_Big pool structure.
 * @param ptr Pointer to the memory to check.
 * @return EINA_TRUE if the pointer is from this pool and currently allocated, EINA_FALSE otherwise.
 */
static Eina_Bool
eina_one_big_from(void *data, void *ptr)
{
   One_Big *pool = data;
   Eina_Bool r = EINA_FALSE;

   if (!eina_lock_take(&pool->mutex))
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(eina_thread_equal(pool->self, eina_thread_self()));
#endif
     }

   if ((void *)pool->base <= ptr
       && ptr < (void *)(pool->base + (pool->max * pool->item_size)))
     {
        Eina_Trash *t;
#ifndef NVALGRIND
        Eina_Trash *last = NULL;
#endif
        // Part of the bigger area

        // Check if it is a properly aligned element
        if (((unsigned char *)ptr - (unsigned char *) pool->base) % pool->item_size)
          {
#ifdef DEBUG
             ERR("%p is %lu bytes inside a pointer served by %p '%s' One_Big_Mempool (You are freeing the wrong pointer man !).",
                 ptr, ((unsigned char *)ptr - (unsigned char *) pool->base) % pool->item_size, pool, pool->name);
#endif
             goto end;
          }

        // Check if the pointer was freed
        for (t = pool->empty; t != NULL; t = t->next)
          {
#ifndef NVALGRIND
             VALGRIND_MAKE_MEM_DEFINED(t, pool->item_size);
             if (last) VALGRIND_MAKE_MEM_NOACCESS(last, pool->item_size);
             last = t;
#endif
             if (t == ptr) goto end;
          }
#ifndef NVALGRIND
        if (last) VALGRIND_MAKE_MEM_NOACCESS(last, pool->item_size);
#endif

        // Everything seems correct
        r = EINA_TRUE;
     }
   else
     {
        Eina_Inlist *it, *il;
        // Part of the smaller items inlist

        il = OVER_MEM_TO_LIST(pool, ptr);

        for (it = pool->over_list; it != NULL; it = it->next)
          if (it == il)
            {
               r = EINA_TRUE;
               break;
            }
     }

 end:
   eina_lock_release(&pool->mutex);
   return r;
}

/**
 * @internal
 * @brief Structure for iterating over allocated items in a One_Big mempool.
 */
typedef struct _Eina_Iterator_One_Big_Mempool Eina_Iterator_One_Big_Mempool;
struct _Eina_Iterator_One_Big_Mempool
{
   Eina_Iterator iterator; /**< Base Eina_Iterator structure. */

   Eina_Iterator *walker;  /**< Iterator for the `over_list` of the pool. */
   One_Big *pool;          /**< Pointer to the One_Big pool being iterated. */

   unsigned int offset;    /**< Current byte offset within the `pool->base` block. Used to iterate items from the main block. */
};

/**
 * @internal
 * @brief Advances the iterator to the next allocated item in the One_Big pool.
 *
 * It first iterates through the `base` block. For each potential item slot,
 * it uses `eina_one_big_from()` to check if the item is currently allocated
 * (i.e., not on the `empty` list).
 * After exhausting the `base` block, it iterates through the `over_list`
 * using the `walker` iterator.
 *
 * @param it Pointer to the Eina_Iterator_One_Big_Mempool structure.
 * @param data Pointer to a void pointer where the address of the next item will be stored.
 * @return EINA_TRUE if an item was found and `*data` is set, EINA_FALSE if iteration is complete.
 */
static Eina_Bool
eina_mempool_iterator_next(Eina_Iterator_One_Big_Mempool *it, void **data)
{
   Eina_Inlist *il = NULL;

 retry:
   if (it->offset < (it->pool->max * it->pool->item_size))
     {
        unsigned char *ptr = (unsigned char *) (it->pool->base);

        ptr += it->offset;
        it->offset += it->pool->item_size;

        if (!eina_one_big_from(it->pool, ptr)) goto retry;

        if (data) *data = (void *) ptr;
        return EINA_TRUE;
     }

   if (!eina_iterator_next(it->walker, (void **) &il))
     return EINA_FALSE;

   if (data) *data = OVER_MEM_FROM_LIST(it->pool, il);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the container (the One_Big pool) of the iterator.
 *
 * @param it Pointer to the Eina_Iterator_One_Big_Mempool structure.
 * @return Pointer to the One_Big pool being iterated.
 */
static One_Big *
eina_mempool_iterator_get_container(Eina_Iterator_One_Big_Mempool *it)
{
   return it->pool;
}

/**
 * @internal
 * @brief Frees the One_Big mempool iterator.
 *
 * @param it Pointer to the Eina_Iterator_One_Big_Mempool structure to be freed.
 */
static void
eina_mempool_iterator_free(Eina_Iterator_One_Big_Mempool *it)
{
   eina_iterator_free(it->walker);
   free(it);
}

/**
 * @internal
 * @brief Creates a new iterator for a One_Big mempool.
 *
 * This iterator will traverse all currently allocated items in the pool,
 * both from the `base` block and the `over_list`.
 *
 * @param data Pointer to the One_Big pool structure.
 * @return A new Eina_Iterator, or NULL on allocation failure.
 */
static Eina_Iterator *
eina_one_big_iterator_new(void *data)
{
   Eina_Iterator_One_Big_Mempool *it;
   One_Big *pool = data;

   it = calloc(1, sizeof (Eina_Iterator_One_Big_Mempool));
   if (!it) return NULL;

    it->walker = eina_inlist_iterator_new(pool->over_list);
    it->pool = pool;

    it->iterator.version = EINA_ITERATOR_VERSION;
    it->iterator.next = FUNC_ITERATOR_NEXT(eina_mempool_iterator_next);
    it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(
                                                             eina_mempool_iterator_get_container);
    it->iterator.free = FUNC_ITERATOR_FREE(eina_mempool_iterator_free);

    EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

    return &it->iterator;
}

/**
 * @internal
 * @brief Reallocates memory for an element from the One_Big pool.
 * @warning This operation is not supported by the One_Big mempool.
 *
 * @param data Unused.
 * @param element Unused.
 * @param size Unused.
 * @return Always NULL, as realloc is not supported.
 */
static void *
eina_one_big_realloc(EINA_UNUSED void *data,
                     EINA_UNUSED void *element,
                     EINA_UNUSED unsigned int size)
{
   return NULL;
}

/**
 * @internal
 * @brief Initializes a new One_Big memory pool.
 *
 * This function is called by the generic eina_mempool_new() when the "one_big"
 * type is specified. It allocates and sets up the One_Big structure.
 * The actual large memory block (`pool->base`) is not allocated here but
 * on the first call to `eina_one_big_malloc`.
 *
 * @param context Name for the pool (e.g., "my_object_pool").
 * @param option Unused options string.
 * @param args Variable argument list, expected to contain:
 *             - int: item_size (size of each element in the pool).
 *             - int: max_items (maximum number of items to pre-allocate in the 'base' block).
 * @return Pointer to the initialized One_Big pool structure, or NULL on failure.
 *
 * @par Example Usage from eina_mempool_new():
 * @code
 * Eina_Mempool *mp = eina_mempool_new(EINA_MEMPOOL_ONE_BIG("my_struct_pool", sizeof(MyStruct), 100));
 * // This would call eina_one_big_init with:
 * // context = "my_struct_pool"
 * // args containing: sizeof(MyStruct), 100
 * @endcode
 */
static void *
eina_one_big_init(const char *context,
                  EINA_UNUSED const char *option,
                  va_list args)
{
   One_Big *pool;
   int item_size;
   size_t length;

   length = context ? strlen(context) + 1 : 0;

   pool = calloc(1, sizeof (One_Big) + length);
   if (!pool)
      return NULL;

   item_size = va_arg(args, int);
   if (item_size < 1) item_size = 1;

   pool->item_size = MAX(eina_mempool_alignof(item_size), sizeof(void*));
   pool->max = va_arg(args, int);
   if (pool->max < 1) pool->max = 1;

   pool->offset_to_item_inlist = pool->item_size;
   if (pool->offset_to_item_inlist % (int)sizeof(void *) != 0)
     {
        pool->offset_to_item_inlist =
          (((pool->offset_to_item_inlist / (int)sizeof(void *)) + 1) *
           (int)sizeof(void *));
     }

   if (length)
     {
        pool->name = (const char *)(pool + 1);
        memcpy((char *)pool->name, context, length);
     }

#ifdef EINA_HAVE_DEBUG_THREADS
   pool->self = eina_thread_self();
#endif
   eina_lock_new(&pool->mutex);

#ifndef NVALGRIND
   VALGRIND_CREATE_MEMPOOL(pool, 0, 1);
#endif

   return pool;
}

/**
 * @internal
 * @brief Shuts down and cleans up a One_Big memory pool.
 *
 * This function is called by eina_mempool_del() for "one_big" pools.
 * It frees all "over" allocated items, the main `base` block,
 * and the pool structure itself. It also handles mutex destruction.
 *
 * @param data Pointer to the One_Big pool structure to be shut down.
 */
static void
eina_one_big_shutdown(void *data)
{
   One_Big *pool = data;

   if (!pool) return;
   if (!eina_lock_take(&pool->mutex))
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(eina_thread_equal(pool->self, eina_thread_self()));
#endif
     }

   if (pool->over > 0)
     {
// FIXME: should we warn here? one_big mempool exceeded its alloc and now
// mempool is cleaning up the mess created. be quiet for now as we were before
// but edje seems to be a big offender at the moment! bad cedric! :)
//        WRN(
//            "Pool [%s] over by %i. cleaning up for you",
//            pool->name, pool->over);
        while (pool->over_list)
          {
             Eina_Inlist *il = pool->over_list;
             void *ptr = OVER_MEM_FROM_LIST(pool, il);
             pool->over_list = eina_inlist_remove(pool->over_list, il);
             free(ptr);
             pool->over--;
          }
     }
   if (pool->over > 0)
     {
        WRN(
            "Pool [%s] still over by %i\n",
            pool->name, pool->over);
     }

#ifndef NVALGRIND
   VALGRIND_DESTROY_MEMPOOL(pool);
#endif

   if (pool->base) free(pool->base);

   eina_lock_release(&pool->mutex);
   eina_lock_free(&pool->mutex);
   free(pool);
}

/**
 * @internal
 * @brief Backend function table for the "one_big" mempool type.
 *
 * This structure provides the Eina_Mempool system with the necessary
 * function pointers to manage "one_big" mempools.
 */
static Eina_Mempool_Backend _eina_one_big_mp_backend = {
   "one_big", /**< Name of this mempool backend. */
   &eina_one_big_init, /**< Initialization function. */
   &eina_one_big_free,
   &eina_one_big_malloc,
   &eina_one_big_realloc,
   NULL,
   NULL,
   &eina_one_big_shutdown,
   NULL,
   &eina_one_big_from,
   &eina_one_big_iterator_new,
   NULL /**< Function to get statistics (not implemented). */
};

/**
 * @internal
 * @brief Initializes the "one_big" mempool module.
 *
 * Registers the "one_big" mempool backend with the Eina_Mempool system.
 * Also registers a log domain for debugging if DEBUG is enabled.
 * This function is typically called via EINA_MODULE_INIT.
 *
 * @return EINA_TRUE on successful registration, EINA_FALSE otherwise.
 */
Eina_Bool one_big_init(void)
{
#ifdef DEBUG
   _eina_one_big_mp_log_dom = eina_log_domain_register("eina_one_big_mempool",
                                                       EINA_LOG_COLOR_DEFAULT);
   if (_eina_one_big_mp_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: eina_one_big_mempool");
        return EINA_FALSE;
     }

#endif
   return eina_mempool_register(&_eina_one_big_mp_backend);
}

/**
 * @internal
 * @brief Shuts down the "one_big" mempool module.
 *
 * Unregisters the "one_big" mempool backend from the Eina_Mempool system.
 * Also unregisters the log domain if DEBUG is enabled.
 * This function is typically called via EINA_MODULE_SHUTDOWN.
 */
void one_big_shutdown(void)
{
   eina_mempool_unregister(&_eina_one_big_mp_backend);
#ifdef DEBUG
   eina_log_domain_unregister(_eina_one_big_mp_log_dom);
   _eina_one_big_mp_log_dom = -1;
#endif
}

#ifndef EINA_STATIC_BUILD_ONE_BIG

EINA_MODULE_INIT(one_big_init);
EINA_MODULE_SHUTDOWN(one_big_shutdown);

#endif /* ! EINA_STATIC_BUILD_ONE_BIG */
