/**
 * @file
 * @brief Eina Chained Mempool implementation
 *
 * This file implements a memory pool that allocates memory in chained blocks.
 * It is designed to reduce fragmentation and improve allocation speed for
 * fixed-size objects.
 */

/* EINA - EFL data type library
 * Copyright (C) 2008-2010 Cedric BAIL, Vincent Torri
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

#ifdef EINA_HAVE_DEBUG_THREADS
# include <assert.h>
#endif

#ifdef EINA_DEBUG_MALLOC
# ifdef __linux__
#  include <malloc.h>
# endif
# ifdef __FreeBSD__
#  include <malloc_np.h>
# endif
#endif

#include "eina_config.h"
#include "eina_inlist.h"
#include "eina_module.h"
#include "eina_mempool.h"
#include "eina_trash.h"
#include "eina_rbtree.h"
#include "eina_lock.h"
#include "eina_thread.h"
#include "eina_cpu.h"

#include "eina_private.h"

#ifndef NVALGRIND
# include <memcheck.h>
#endif

#if defined DEBUG || defined EINA_DEBUG_MALLOC
#include <assert.h>
#include "eina_log.h"

static int _eina_chained_mp_log_dom = -1;

#ifdef INF
#undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_eina_chained_mp_log_dom, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_eina_chained_mp_log_dom, __VA_ARGS__)

#endif

static int aligned_chained_pool = 0;
static int page_size = 0;

/**
 * @brief Represents a single pool (block) in the chained mempool.
 *
 * Each Chained_Pool holds a contiguous block of memory from which items
 * are allocated. Pools are linked in an inlist and also managed in an
 * rbtree for efficient searching.
 */
typedef struct _Chained_Pool Chained_Pool;
struct _Chained_Pool
{
   EINA_INLIST; /**< Macro for inlist node integration. */
   EINA_RBTREE; /**< Macro for rbtree node integration. */
   Eina_Trash *base; /**< Pointer to a list of freed items within this pool (for recycling). */
   unsigned int usage; /**< Number of currently allocated items in this pool. */

   unsigned char *last; /**< Pointer to the next available memory slot for a new allocation. NULL if no space left for new allocations (only recycled ones). */
   unsigned char *limit; /**< Pointer to the end of the allocatable memory in this pool. */
};

/**
 * @brief Represents the entire chained memory pool manager.
 *
 * This structure holds all the metadata for a chained mempool, including
 * lists of individual pools, allocation sizes, and statistics.
 */
typedef struct _Chained_Mempool Chained_Mempool;
struct _Chained_Mempool
{
   Eina_Inlist *first; /**< Inlist of all Chained_Pool instances, ordered by recent usage or availability. */
   Eina_Rbtree *root; /**< Rbtree of all Chained_Pool instances, for fast address-based lookups. */
   const char *name; /**< Name of the mempool, for debugging and identification. */
   unsigned int item_alloc; /**< Size of each item to be allocated, including alignment. */
   unsigned int pool_size; /**< Number of items that can be allocated in a single Chained_Pool. */
   unsigned int alloc_size; /**< Total size of a Chained_Pool structure plus its item data area. */
   unsigned int group_size; /**< Total size of the item data area in a Chained_Pool (item_alloc * pool_size). */
   unsigned int usage; /**< Total number of currently allocated items across all pools. */
   Chained_Pool* first_fill; /**< Optimization: Pointer to the pool currently preferred for allocations. All allocations will happen in this chain, unless it is filled. */
#ifdef EINA_DEBUG_MALLOC
   int minimal_size; /**< Minimal expected size of a pool, for debugging memory overhead. */
#endif
#ifdef EINA_HAVE_DEBUG_THREADS
   Eina_Thread self;
#endif
   Eina_Spinlock mutex;
};

/**
 * @brief Compares two Chained_Pool instances for rbtree ordering.
 * @param left The left Chained_Pool (as Eina_Rbtree node).
 * @param right The right Chained_Pool (as Eina_Rbtree node).
 * @param data User data (unused).
 * @return EINA_RBTREE_LEFT if left < right, EINA_RBTREE_RIGHT otherwise.
 *
 * Comparison is based on memory addresses of the pool structures.
 */
static inline Eina_Rbtree_Direction
_eina_chained_mp_pool_cmp(const Eina_Rbtree *left, const Eina_Rbtree *right, EINA_UNUSED void *data)
{
   if (left < right) return EINA_RBTREE_LEFT;
   return EINA_RBTREE_RIGHT;
}

/**
 * @brief Compares a Chained_Pool with a key (memory address) for rbtree lookup.
 * @param node The Chained_Pool (as Eina_Rbtree node) to compare.
 * @param key The memory address (pointer) to check.
 * @param length Unused.
 * @param data User data (unused).
 * @return 0 if the key is within the memory range of the pool,
 *         -1 if the key is greater than the pool's limit,
 *         1 if the key is less than the pool's start.
 */
static inline int
_eina_chained_mp_pool_key_cmp(const Eina_Rbtree *node, const void *key,
                              EINA_UNUSED int length, EINA_UNUSED void *data)
{
   const Chained_Pool *r = EINA_RBTREE_CONTAINER_GET(node, const Chained_Pool);

   // The key (a pointer) is being checked if it falls within the memory
   // range managed by this specific pool 'r'.
   // The pool 'r' itself is a struct, and its allocatable memory starts right after it.
   // r->limit points to the end of this allocatable memory.
   if (key > (void *) r->limit) return -1; // Key is beyond this pool's managed memory
   if (key < (void *) r) return 1; // Key is before this pool's structure (and thus its managed memory)
   return 0; // Key is within the address range of this pool's structure or its managed memory.
             // Further checks are needed to confirm it's a valid item from this pool.
}

/**
 * @brief Allocates and initializes a new Chained_Pool.
 * @param pool The parent Chained_Mempool.
 * @return A pointer to the newly allocated Chained_Pool, or NULL on failure.
 */
static inline Chained_Pool *
_eina_chained_mp_pool_new(Chained_Mempool *pool)
{
   Chained_Pool *p;
   unsigned char *ptr;

   p = malloc(pool->alloc_size);
   if (!p) return NULL;

#if defined(EINA_DEBUG_MALLOC) && defined (HAVE_MALLOC_USABLE_SIZE)
   {
      size_t sz;
      sz = malloc_usable_size(p);
      if (sz - pool->minimal_size > 0)
        INF("Just allocated %0.2f%% to much memory in '%s' for one block of size %i that means %lu bytes to much.",
            ((float)(sz - pool->minimal_size) * 100) / (float) (pool->alloc_size),
            pool->name,
            pool->alloc_size,
            (unsigned long) sz - pool->minimal_size);
   }
#endif

   ptr = (unsigned char *)(p + 1);
   p->usage = 0;
   p->base = NULL;

   p->last = ptr;
   p->limit = ptr + pool->item_alloc * pool->pool_size;

#ifndef NVALGRIND
   VALGRIND_MAKE_MEM_NOACCESS(ptr, pool->alloc_size - aligned_chained_pool);
#endif

   return p;
}

/**
 * @brief Frees a Chained_Pool.
 * @param p The Chained_Pool to free.
 */
static inline void
_eina_chained_mp_pool_free(Chained_Pool *p)
{
   free(p);
}

/**
 * @brief Compares two Chained_Pool instances based on their usage for sorting.
 * @param l1 The first Chained_Pool (as Eina_Inlist node).
 * @param l2 The second Chained_Pool (as Eina_Inlist node).
 * @return A positive value if p2 has higher usage than p1 (sorts descending by usage),
 *         a negative value if p1 has higher usage, 0 if equal.
 *
 * This function is used to sort pools so that less used pools can be
 * identified, potentially for repacking or freeing.
 */
static int
_eina_chained_mempool_usage_cmp(const Eina_Inlist *l1, const Eina_Inlist *l2)
{
  const Chained_Pool *p1;
  const Chained_Pool *p2;

  p1 = EINA_INLIST_CONTAINER_GET(l1, const Chained_Pool);
  p2 = EINA_INLIST_CONTAINER_GET(l2, const Chained_Pool);

  // Sorts in descending order of usage (p2->usage - p1->usage)
  // so that pools with more free space (lower usage) come first if sorted ascending,
  // or pools with higher usage come first if sorted descending.
  // The list is typically sorted to find pools with free slots or to repack.
  return p2->usage - p1->usage;
}

/**
 * @brief Allocates an item from a specific Chained_Pool.
 * @param pool The parent Chained_Mempool.
 * @param p The Chained_Pool to allocate from.
 * @return A pointer to the allocated memory item, or NULL if the pool is full
 *         and has no recycled items.
 *
 * This function first tries to recycle a previously freed item from the pool's
 * trash list. If no recycled items are available, it allocates a new item
 * from the pool's contiguous memory block.
 */
static void *
_eina_chained_mempool_alloc_in(Chained_Mempool *pool, Chained_Pool *p)
{
  void *mem = NULL;

  // Let's try to first recycle memory
  if (p->base)
    {
#ifndef NVALGRIND
      VALGRIND_MAKE_MEM_DEFINED(p->base, pool->item_alloc);
#endif
      // Request a free pointer
      mem = eina_trash_pop(&p->base);
    }
  else if (p->last)
    {
      mem = p->last;
      p->last += pool->item_alloc;
      if (p->last >= p->limit)
        p->last = NULL;
    }

  // move to end - it just filled up
  if (!p->base && !p->last)
    pool->first = eina_inlist_demote(pool->first, EINA_INLIST_GET(p));

  p->usage++;
  pool->usage++;

#ifndef NVALGRIND
   VALGRIND_MEMPOOL_ALLOC(pool, mem, pool->item_alloc);
#endif

  return mem;
}

/**
 * @brief Frees an item within a specific Chained_Pool.
 * @param pool The parent Chained_Mempool.
 * @param p The Chained_Pool from which the item was allocated.
 * @param ptr The memory item to free.
 * @return EINA_TRUE if the Chained_Pool `p` became empty and was freed,
 *         EINA_FALSE otherwise.
 *
 * The freed item is added to the pool's trash list for recycling.
 * If the pool becomes completely empty (all items freed), the pool itself
 * is deallocated and removed from the mempool's management.
 */
static Eina_Bool
_eina_chained_mempool_free_in(Chained_Mempool *pool, Chained_Pool *p, void *ptr)
{
#ifdef DEBUG
   void *pmem;

   // pool mem base
   pmem = (void *)(((unsigned char *)p) + sizeof(Chained_Pool));

   // is it in pool mem?
   if (ptr < pmem)
     {
        ERR("%p is inside the private part of %p pool from %p '%s' Chained_Mempool (could be the sign of a buffer underrun).", ptr, p, pool, pool->name);
        return EINA_FALSE;
     }

   // is it really a pointer returned by malloc
   if ((((unsigned char *)ptr) - (unsigned char *)(p + 1)) % pool->item_alloc)
     {
        ERR("%p is %lu bytes inside a pointer served by %p '%s' Chained_Mempool (You are freeing the wrong pointer man !).",
            ptr, ((((unsigned char *)ptr) - (unsigned char *)(p + 1)) % pool->item_alloc), pool, pool->name);
        return EINA_FALSE;
     }
#endif

   // freed node points to prev free node
   eina_trash_push(&p->base, ptr);
   // next free node is now the one we freed
   p->usage--;
   pool->usage--;
   if (p->usage == 0)
     {
        // free bucket
        pool->first = eina_inlist_remove(pool->first, EINA_INLIST_GET(p));
        pool->root = eina_rbtree_inline_remove(pool->root, EINA_RBTREE_GET(p),
                                               _eina_chained_mp_pool_cmp, NULL);
        if (pool->first_fill == p)
          {
             pool->first_fill = NULL;
             pool->first_fill = EINA_INLIST_CONTAINER_GET(pool->first, Chained_Pool);
          }
        _eina_chained_mp_pool_free(p);

       return EINA_TRUE;
     }
   else
     {
        // move to front
        pool->first = eina_inlist_promote(pool->first, EINA_INLIST_GET(p));
     }

   return EINA_FALSE;
}

/**
 * @brief Allocates an item from the chained mempool.
 * @param data The Chained_Mempool instance.
 * @param size The size of the item to allocate (unused, as item size is fixed per pool).
 * @return A pointer to the allocated memory item, or NULL on failure.
 *
 * This function implements the malloc behavior for the mempool. It tries to
 * allocate from the `first_fill` pool if available and has space. If not,
 * it searches for other pools with free space or creates a new pool.
 */
static void *
eina_chained_mempool_malloc(void *data, EINA_UNUSED unsigned int size)
{
   Chained_Mempool *pool = data;
   Chained_Pool *p = NULL;
   void *mem = NULL;

   if (!eina_spinlock_take(&pool->mutex))
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(eina_thread_equal(pool->self, eina_thread_self()));
#endif
     }

   //we have some free space in first fill chain
   if (pool->first_fill) p = pool->first_fill;

   // base is not NULL - has a free slot
   if (p && !p->base && !p->last)
     {
       //Current pointed chain is filled , so point it to first one
       pool->first_fill = EINA_INLIST_CONTAINER_GET(pool->first, Chained_Pool);
       //Either first one has some free space or every chain is filled
       if (pool->first_fill && !pool->first_fill->base && !pool->first_fill->last)
        {
           p = NULL;
           pool->first_fill = NULL;
        }
     }

#ifdef DEBUG
   if (p == NULL)
     EINA_INLIST_FOREACH(pool->first, p)
       assert(!p->base && !p->last);
#endif

   // we have reached the end of the list - no free pools
   if (!p)
      {
       //new chain created ,point it to be the first_fill chain
        pool->first_fill = _eina_chained_mp_pool_new(pool);
        if (!pool->first_fill)
          {
             eina_spinlock_release(&pool->mutex);
             return NULL;
          }

        pool->first = eina_inlist_prepend(pool->first, EINA_INLIST_GET(pool->first_fill));
        pool->root = eina_rbtree_inline_insert(pool->root, EINA_RBTREE_GET(pool->first_fill),
                                               _eina_chained_mp_pool_cmp, NULL);
     }

   if (pool->first_fill)
     mem = _eina_chained_mempool_alloc_in(pool, pool->first_fill);

   eina_spinlock_release(&pool->mutex);
   return mem;
}

/**
 * @brief Frees an item allocated from the chained mempool.
 * @param data The Chained_Mempool instance.
 * @param ptr The memory item to free.
 *
 * This function implements the free behavior for the mempool. It locates
 * the Chained_Pool to which the item belongs and then calls
 * `_eina_chained_mempool_free_in` to perform the actual free operation.
 */
static void
eina_chained_mempool_free(void *data, void *ptr)
{
   Chained_Mempool *pool = data;
   Eina_Rbtree *r;
   Chained_Pool *p;

   // look 4 pool
   if (!eina_spinlock_take(&pool->mutex))
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(eina_thread_equal(pool->self, eina_thread_self()));
#endif
     }

   // searching for the right mempool
   r = eina_rbtree_inline_lookup(pool->root, ptr, 0, _eina_chained_mp_pool_key_cmp, NULL);

   // related mempool not found
   if (!r)
     {
#ifdef DEBUG
        ERR("%p is not the property of %p Chained_Mempool", ptr, pool);
#endif
        goto on_error;
     }

   p = EINA_RBTREE_CONTAINER_GET(r, Chained_Pool);

   _eina_chained_mempool_free_in(pool, p, ptr);

 on_error:
#ifndef NVALGRIND
   if (ptr)
     {
        VALGRIND_MEMPOOL_FREE(pool, ptr);
     }
#endif

   eina_spinlock_release(&pool->mutex);
   return;
}

/**
 * @brief Allocates an item from the chained mempool, trying to allocate
 *        near a given existing allocation.
 * @param data The Chained_Mempool instance.
 * @param after A pointer to an existing allocation after which the new
 *              allocation should ideally be placed.
 * @param before A pointer to an existing allocation before which the new
 *               allocation should ideally be placed.
 * @param size The size of the item to allocate (unused).
 * @return A pointer to the allocated memory item, or NULL on failure.
 *
 * This function attempts to allocate memory from the same Chained_Pool
 * as the `after` or `before` pointers, if possible. If not, it falls
 * back to the standard `eina_chained_mempool_malloc`. This can be useful
 * for improving data locality.
 */
static void *
eina_chained_mempool_malloc_near(void *data,
                                 void *after, void *before,
                                 unsigned int size EINA_UNUSED)
{
   Chained_Mempool *pool = data;
   Chained_Pool *p = NULL;
   void *mem = NULL;

   if (!eina_spinlock_take(&pool->mutex))
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(eina_thread_equal(pool->self, eina_thread_self()));
#endif
     }

   if (after)
     {
        Eina_Rbtree *r = eina_rbtree_inline_lookup(pool->root, after,
                                                   0, _eina_chained_mp_pool_key_cmp, NULL);

        if (r)
          {
             p = EINA_RBTREE_CONTAINER_GET(r, Chained_Pool);

             if (!p->base && !p->last)
               p = NULL;
          }
     }

   if (before && p == NULL)
     {
        Eina_Rbtree *r = eina_rbtree_inline_lookup(pool->root, before,
                                                   0, _eina_chained_mp_pool_key_cmp, NULL);
        if (r)
          {
             p = EINA_RBTREE_CONTAINER_GET(r, Chained_Pool);
             if (!p->base && !p->last)
               p = NULL;
          }
     }

   if (p) mem = _eina_chained_mempool_alloc_in(pool, p);

   eina_spinlock_release(&pool->mutex);

   if (!mem) return eina_chained_mempool_malloc(pool, size);
   return mem;
}

/**
 * @brief Checks if a given pointer was allocated from this mempool.
 * @param data The Chained_Mempool instance.
 * @param ptr The pointer to check.
 * @return EINA_TRUE if the pointer was allocated from this mempool and is
 *         currently considered live (not freed), EINA_FALSE otherwise.
 *
 * This function verifies if the pointer falls within the memory range of
 * any Chained_Pool managed by this mempool, if it's correctly aligned,
 * and if it's not currently in the trash list of that pool.
 */
static Eina_Bool
eina_chained_mempool_from(void *data, void *ptr)
{
   Chained_Mempool *pool = data;
   Eina_Rbtree *r;
   Chained_Pool *p;
   Eina_Trash *t;
#ifndef NVALGRIND
   Eina_Trash *last = NULL;
#endif
   void *pmem;
   Eina_Bool ret = EINA_FALSE;

   // look 4 pool
   if (!eina_spinlock_take(&pool->mutex))
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(eina_thread_equal(pool->self, eina_thread_self()));
#endif
     }

   // searching for the right mempool
   r = eina_rbtree_inline_lookup(pool->root, ptr, 0, _eina_chained_mp_pool_key_cmp, NULL);

   // related mempool not found
   if (!r) goto end;

   p = EINA_RBTREE_CONTAINER_GET(r, Chained_Pool);

   // pool mem base
   pmem = (void *)(((unsigned char *)p) + sizeof(Chained_Pool));

   // is it in pool mem?
   if (ptr < pmem)
     {
#ifdef DEBUG
        ERR("%p is inside the private part of %p pool from %p '%s' Chained_Mempool (could be the sign of a buffer underrun).", ptr, p, pool, pool->name);
#endif
        goto end;
     }

   // is the pointer in the allocated zone of the mempool
   if (p->last != NULL && ((unsigned char *)ptr >= p->last))
     {
#ifdef DEBUG
        ERR("%p has not been allocated yet from %p pool of %p '%s' Chained_Mempool.", ptr, p, pool, pool->name);
#endif
        goto end;
     }

   // is it really a pointer returned by malloc
   if ((((unsigned char *)ptr) - (unsigned char *)(p + 1)) % pool->item_alloc)
     {
#ifdef DEBUG
        ERR("%p is %lu bytes inside a pointer served by %p '%s' Chained_Mempool (You are freeing the wrong pointer man !).",
            ptr, ((((unsigned char *)ptr) - (unsigned char *)(p + 1)) % pool->item_alloc), pool, pool->name);
#endif
        goto end;
     }

   // Check if the pointer was freed
   for (t = p->base; t != NULL; t = t->next)
     {
#ifndef NVALGRIND
        VALGRIND_MAKE_MEM_DEFINED(t, pool->item_alloc);
        if (last) VALGRIND_MAKE_MEM_NOACCESS(last, pool->item_alloc);
        last = t;
#endif

        if (t == ptr) goto end;
     }
#ifndef NVALGRIND
     if (last) VALGRIND_MAKE_MEM_NOACCESS(last, pool->item_alloc);
#endif

   // Seems like we have a valid pointer actually
   ret = EINA_TRUE;

 end:
   eina_spinlock_release(&pool->mutex);
   return ret;
}

typedef struct _Eina_Iterator_Chained_Mempool Eina_Iterator_Chained_Mempool;
struct _Eina_Iterator_Chained_Mempool
{
   Eina_Iterator iterator; /**< The Eina_Iterator interface. */

   Eina_Iterator *walker; /**< An iterator for the list of Chained_Pools. */
   Chained_Pool *current; /**< The current Chained_Pool being iterated. */
   Chained_Mempool *pool; /**< The Chained_Mempool being iterated. */

   unsigned int offset; /**< Offset within the current Chained_Pool's data block. */
};

/**
 * @brief Advances the mempool iterator to the next live allocated item.
 * @param it The mempool iterator.
 * @param data Pointer to store the next item.
 * @return EINA_TRUE if an item was found, EINA_FALSE otherwise.
 */
static Eina_Bool
eina_mempool_iterator_next(Eina_Iterator_Chained_Mempool *it, void **data)
{
   if (!it->current)
     {
        if (!eina_iterator_next(it->walker, (void**) &it->current))
          return EINA_FALSE;
        if (!it->current) return EINA_FALSE;
     }

 retry:
   if (it->offset < it->pool->group_size)
     {
        unsigned char *ptr = (unsigned char *) (it->current + 1);

        ptr += it->offset;
        it->offset += it->pool->item_alloc;

        if (!eina_chained_mempool_from(it->pool, ptr)) goto retry;

        if (data) *data = (void *) ptr;
        return EINA_TRUE;
     }

   if (!eina_iterator_next(it->walker, (void**) &it->current))
     return EINA_FALSE;

   it->offset = 0;
   goto retry;
}

/**
 * @brief Gets the container (Chained_Mempool) of the iterator.
 * @param it The mempool iterator.
 * @return The Chained_Mempool instance.
 */
static Chained_Mempool *
eina_mempool_iterator_get_container(Eina_Iterator_Chained_Mempool *it)
{
   return it->pool;
}

/**
 * @brief Frees the mempool iterator.
 * @param it The mempool iterator to free.
 */
static void
eina_mempool_iterator_free(Eina_Iterator_Chained_Mempool *it)
{
   eina_iterator_free(it->walker);
   free(it);
}

/**
 * @brief Creates a new iterator for the chained mempool.
 * @param data The Chained_Mempool instance.
 * @return A new Eina_Iterator for the live items in the mempool, or NULL on failure.
 *
 * The iterator will walk through all live (currently allocated and not freed)
 * items in the mempool.
 */
static Eina_Iterator *
eina_chained_mempool_iterator_new(void *data)
{
   Eina_Iterator_Chained_Mempool *it;
   Chained_Mempool *pool = data;

   it = calloc(1, sizeof (Eina_Iterator_Chained_Mempool));
   if (!it) return NULL;

   it->walker = eina_inlist_iterator_new(pool->first);
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
 * @brief Repacks the mempool to consolidate allocations and free empty pools.
 * @param data The Chained_Mempool instance.
 * @param cb Callback function to notify about moved items.
 *           `cb(new_pointer, old_pointer, callback_data)`
 * @param cb_data User data for the callback.
 *
 * This function attempts to move allocations from sparsely populated pools
 * to more densely populated ones, potentially freeing up entire pool blocks.
 * The callback `cb` is invoked for each item that is moved, allowing the
 * application to update any references to the old item pointer.
 */
static void
eina_chained_mempool_repack(void *data,
			    Eina_Mempool_Repack_Cb cb,
			    void *cb_data)
{
  Chained_Mempool *pool = data;
  Chained_Pool *start;
  Chained_Pool *tail;

  /* FIXME: Improvement - per Chained_Pool lock */
   if (!eina_spinlock_take(&pool->mutex))
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(eina_thread_equal(pool->self, eina_thread_self()));
#endif
     }

   pool->first = eina_inlist_sort(pool->first,
				  (Eina_Compare_Cb) _eina_chained_mempool_usage_cmp);

   /*
     idea : remove the almost empty pool at the beginning of the list by
     moving data in the last pool with empty slot
    */
   tail = EINA_INLIST_CONTAINER_GET(pool->first->last, Chained_Pool);
   while (tail && tail->usage == pool->pool_size)
     tail = EINA_INLIST_CONTAINER_GET((EINA_INLIST_GET(tail)->prev), Chained_Pool);

   while (tail)
     {
       unsigned char *src;
       unsigned char *dst;

       start = EINA_INLIST_CONTAINER_GET(pool->first, Chained_Pool);

       if (start == tail || start->usage == pool->pool_size)
	 break;

       for (src = start->limit - pool->group_size;
	    src != start->limit;
	    src += pool->item_alloc)
	 {
	   Eina_Bool is_free = EINA_FALSE;
	   Eina_Bool is_dead;

	   /* Do we have something inside that piece of memory */
	   if (start->last != NULL && src >= start->last)
	     {
	       is_free = EINA_TRUE;
	     }
	   else
	     {
	       Eina_Trash *over = start->base;

	       while (over != NULL && (unsigned char*) over != src)
		 over = over->next;

	       if (over == NULL)
		 is_free = EINA_TRUE;
	     }

	   if (is_free) continue ;

	   /* get a new memory pointer from the latest most occuped pool */
	   dst = _eina_chained_mempool_alloc_in(pool, tail);
	   /* move data from one to another */
	   memcpy(dst, src, pool->item_alloc);
	   /* notify caller */
	   cb(dst, src, cb_data);
	   /* destroy old pointer */
	   is_dead = _eina_chained_mempool_free_in(pool, start, src);

	   /* search last tail with empty slot */
	   while (tail && tail->usage == pool->pool_size)
	     tail = EINA_INLIST_CONTAINER_GET((EINA_INLIST_GET(tail)->prev),
					      Chained_Pool);
	   /* no more free space */
	   if (!tail || tail == start) break;
	   if (is_dead) break;
	 }
     }

   /* FIXME: improvement - reorder pool so that the most used one get in front */
   eina_spinlock_release(&pool->mutex);
}

/**
 * @brief Reallocates an item from the chained mempool.
 * @param data The Chained_Mempool instance.
 * @param element The existing memory item to reallocate.
 * @param size The new size for the item.
 * @return This implementation currently does not support realloc and always returns NULL.
 *
 * @note Realloc is not naturally supported by fixed-size mempools.
 */
static void *
eina_chained_mempool_realloc(EINA_UNUSED void *data,
                             EINA_UNUSED void *element,
                             EINA_UNUSED unsigned int size)
{
   // Fixed-size mempools typically don't support realloc in the traditional sense.
   // If an item needs to change size, it usually means freeing the old one
   // and allocating a new one from a different pool (if sizes differ) or the same pool.
   return NULL;
}

/**
 * @brief Initializes a new chained mempool.
 * @param context Name for the mempool (e.g., "my_object_pool").
 * @param option Options string (unused in this implementation).
 * @param args Variable arguments:
 *             - int: item_size (size of each element to be stored)
 *             - int: pool_size (number of items per Chained_Pool block)
 * @return A pointer to the newly initialized Chained_Mempool, or NULL on failure.
 */
static void *
eina_chained_mempool_init(const char *context,
                          EINA_UNUSED const char *option,
                          va_list args)
{
   Chained_Mempool *mp;
   int item_size;
   size_t length;

   length = context ? strlen(context) + 1 : 0;

   mp = calloc(1, sizeof(Chained_Mempool) + length);
   if (!mp)
      return NULL;

   item_size = va_arg(args, int);
   mp->pool_size = va_arg(args, int);

   if (length)
     {
        mp->name = (const char *)(mp + 1);
        memcpy((char *)mp->name, context, length);
     }

   mp->item_alloc = MAX(eina_mempool_alignof(item_size), sizeof(void *));

   mp->pool_size = (((((mp->item_alloc * mp->pool_size + aligned_chained_pool) / page_size)
		      + 1) * page_size)
		    - aligned_chained_pool) / mp->item_alloc;

#ifdef EINA_DEBUG_MALLOC
   mp->minimal_size = item_size * mp->pool_size + sizeof(Chained_Pool);
#endif

   mp->group_size = mp->item_alloc * mp->pool_size;
   mp->alloc_size = mp->group_size + aligned_chained_pool;

#ifndef NVALGRIND
   VALGRIND_CREATE_MEMPOOL(mp, 0, 1);
#endif

#ifdef EINA_HAVE_DEBUG_THREADS
   mp->self = eina_thread_self();
#endif
   mp->first_fill = NULL;
   eina_spinlock_new(&mp->mutex);

   return mp;
}

/**
 * @brief Shuts down and frees a chained mempool.
 * @param data The Chained_Mempool instance to shut down.
 *
 * This function frees all Chained_Pool blocks and the Chained_Mempool
 * structure itself. It will log errors if the mempool is not empty at shutdown
 * (i.e., if there are memory leaks).
 */
static void
eina_chained_mempool_shutdown(void *data)
{
   Chained_Mempool *mp;

   mp = (Chained_Mempool *)data;

   while (mp->first)
     {
        Chained_Pool *p = (Chained_Pool *)mp->first;

#ifdef DEBUG
        if (p->usage > 0)
           INF("Bad news we are destroying a non-empty mempool [%s]\n",
               mp->name);

#endif

        mp->first = eina_inlist_remove(mp->first, mp->first);
        mp->root = eina_rbtree_inline_remove(mp->root, EINA_RBTREE_GET(p),
                                             _eina_chained_mp_pool_cmp, NULL);
        _eina_chained_mp_pool_free(p);
     }

#ifdef DEBUG
   if (mp->root)
     ERR("Bad news, list of pool and rbtree are out of sync for %p !", mp);
#endif

#ifndef NVALGRIND
   VALGRIND_DESTROY_MEMPOOL(mp);
#endif

   eina_spinlock_free(&mp->mutex);

   free(mp);
}

static Eina_Mempool_Backend _eina_chained_mp_backend = {
   "chained_mempool",
   &eina_chained_mempool_init,
   &eina_chained_mempool_free,
   &eina_chained_mempool_malloc,
   &eina_chained_mempool_realloc,
   NULL,
   NULL,
   &eina_chained_mempool_shutdown,
   &eina_chained_mempool_repack,
   &eina_chained_mempool_from,
   &eina_chained_mempool_iterator_new,
   &eina_chained_mempool_malloc_near /**< Function to allocate memory near another block. */
};

/**
 * @brief Initializes the chained mempool module.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * Registers the chained mempool backend with Eina's mempool system.
 * Also initializes logging domain and retrieves system page size.
 */
Eina_Bool chained_init(void)
{
#if defined DEBUG || defined EINA_DEBUG_MALLOC
   _eina_chained_mp_log_dom = eina_log_domain_register("eina_mempool",
                                                       EINA_LOG_COLOR_DEFAULT);
   if (_eina_chained_mp_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: eina_mempool");
        return EINA_FALSE;
     }

#endif
   aligned_chained_pool = eina_mempool_alignof(sizeof(Chained_Pool));
   page_size = eina_cpu_page_size();

   return eina_mempool_register(&_eina_chained_mp_backend);
}

/**
 * @brief Shuts down the chained mempool module.
 *
 * Unregisters the chained mempool backend from Eina's mempool system
 * and unregisters the logging domain.
 */
void chained_shutdown(void)
{
   eina_mempool_unregister(&_eina_chained_mp_backend);
#if defined DEBUG || defined EINA_DEBUG_MALLOC
   eina_log_domain_unregister(_eina_chained_mp_log_dom);
   _eina_chained_mp_log_dom = -1;
#endif
}

#ifndef EINA_STATIC_BUILD_CHAINED_POOL

EINA_MODULE_INIT(chained_init);
EINA_MODULE_SHUTDOWN(chained_shutdown);

#endif /* ! EINA_STATIC_BUILD_CHAINED_POOL */
