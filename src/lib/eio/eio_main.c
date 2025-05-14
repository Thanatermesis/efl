/* EIO - EFL data type library
 * Copyright (C) 2010 Enlightenment Developers:
 *           Cedric Bail <cedric.bail@free.fr>
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

/**
 * @file
 * @brief Main implementation file for the EIO library.
 *
 * This file contains the core initialization, shutdown, memory management,
 * and event handling logic for EIO. It manages pools for various
 * data structures to optimize memory allocation and deallocation.
 */

#include <Efreet_Mime.h>
#include "eio_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static Eio_Version _version = { VMAJ, VMIN, VMIC, VREV };
EIO_API Eio_Version *eio_version = &_version;

/**
 * @cond LOCAL
 */

/* Progress pool */
/**
 * @brief Structure representing a memory allocation pool.
 *
 * This structure is used to manage a pool of pre-allocated memory blocks
 * of a fixed size. It helps in reducing the overhead of frequent
 * malloc/free calls for commonly used small objects.
 */
typedef struct _Eio_Alloc_Pool Eio_Alloc_Pool;

struct _Eio_Alloc_Pool
{
   Eina_Lock lock; /**< Lock to ensure thread-safe access to the pool. */

   Eina_Trash *trash; /**< A trash stack to store freed objects for reuse. */
   size_t mem_size;   /**< The size of each memory block in this pool. */
   int count;         /**< The number of available objects in the trash. */
};

static int _eio_init_count = 0; /**< Reference counter for eio_init() and eio_shutdown(). */
int _eio_log_dom_global = -1;

static Eio_Alloc_Pool progress_pool;
static Eio_Alloc_Pool direct_info_pool;
static Eio_Alloc_Pool char_pool; /**< Memory pool for Eio_File_Char objects. */
static Eio_Alloc_Pool associate_pool; /**< Memory pool for Eio_File_Associate objects. */

static size_t memory_pool_limit = -1; /**< Maximum memory usage allowed for all pools combined. -1 means no limit. */
static size_t memory_pool_usage = 0; /**< Current total memory usage by all pools. */
static Eina_Spinlock memory_pool_lock; /**< Spinlock for protecting access to memory_pool_usage. */
static Eina_Lock memory_pool_mutex; /**< Mutex for memory_pool_cond. */
static Eina_Condition memory_pool_cond; /**< Condition variable to signal when memory usage drops below the limit. */
static Eina_Bool memory_pool_suspended = 1; /**< Flag indicating if memory allocation is currently suspended due to exceeding the limit. */
static Efl_Io_Manager *io_manager = NULL; /**< Global EFL IO Manager instance. */

/**
 * @brief Allocates memory from a specified pool.
 *
 * Attempts to retrieve an object from the pool's trash. If the trash is empty,
 * it allocates new memory. It also tracks the total memory usage.
 *
 * @param pool The allocation pool to use.
 * @return A pointer to the allocated memory block, or NULL on failure.
 */
static void *
_eio_pool_malloc(Eio_Alloc_Pool *pool)
{
   void *result = NULL;

   if (pool->count)
     {
        eina_lock_take(&(pool->lock));
        result = eina_trash_pop(&pool->trash);
        if (result) pool->count--;
        eina_lock_release(&(pool->lock));
     }

   if (!result)
     {
        result = malloc(pool->mem_size);
        eina_spinlock_take(&memory_pool_lock);
        if (result) memory_pool_usage += pool->mem_size;
        eina_spinlock_release(&memory_pool_lock);
     }
   return result;
}

/**
 * @brief Frees memory back to a specified pool or to the system.
 *
 * If the pool's trash has space (below EIO_PROGRESS_LIMIT), the object is
 * added to the trash for reuse. Otherwise, the memory is freed directly
 * to the system. It updates total memory usage and signals if usage
 * drops below the limit, potentially resuming suspended allocations.
 *
 * @param pool The allocation pool the memory belongs to.
 * @param data Pointer to the memory block to free.
 */
static void
_eio_pool_free(Eio_Alloc_Pool *pool, void *data)
{
   if (pool->count >= EIO_PROGRESS_LIMIT)
     {
        eina_spinlock_take(&memory_pool_lock);
        memory_pool_usage -= pool->mem_size;
        eina_spinlock_release(&memory_pool_lock);
        free(data);

        if (memory_pool_limit > 0 &&
            memory_pool_usage < memory_pool_limit)
          {
             eina_lock_take(&(memory_pool_mutex));
             if (memory_pool_suspended)
               eina_condition_broadcast(&(memory_pool_cond));
             eina_lock_release(&(memory_pool_mutex));
          }
     }
   else
     {
        eina_lock_take(&(pool->lock));
        eina_trash_push(&pool->trash, data);
        pool->count++;
        eina_lock_release(&(pool->lock));
     }
}

/**
 * @endcond
 */

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @brief Allocates an Eio_Progress object from its dedicated pool.
 * @return A pointer to an Eio_Progress object, or NULL on allocation failure.
 */
Eio_Progress *
eio_progress_malloc(void)
{
   return _eio_pool_malloc(&progress_pool);
}

/**
 * @brief Frees an Eio_Progress object back to its pool.
 *
 * Also releases the stringshare references for source and dest paths.
 * @param data Pointer to the Eio_Progress object to free.
 */
void
eio_progress_free(Eio_Progress *data)
{
   eina_stringshare_del(data->source);
   eina_stringshare_del(data->dest);

   _eio_pool_free(&progress_pool, data);
}

/**
 * @brief Sends progress information via an Ecore_Thread.
 *
 * If the operation has a progress callback, this function allocates an
 * Eio_Progress structure, populates it with current progress data,
 * and sends it as feedback through the specified Ecore_Thread.
 *
 * @param thread The Ecore_Thread to send feedback to.
 * @param op The Eio_File_Progress operation providing context (like source/dest paths).
 * @param current The current progress value (e.g., bytes transferred).
 * @param max The maximum progress value (e.g., total file size).
 */
void
eio_progress_send(Ecore_Thread *thread, Eio_File_Progress *op, long long current, long long max)
{
   Eio_Progress *progress;

   if (op->progress_cb == NULL)
     return;

   progress = eio_progress_malloc();
   if (!progress) return;

   progress->op = op->op;
   progress->current = current;
   progress->max = max;
   progress->percent = max ? (float) current * 100.0 / (float) max : 100;
   progress->source = eina_stringshare_ref(op->source);
   progress->dest = eina_stringshare_ref(op->dest);

   ecore_thread_feedback(thread, progress);
}

/**
 * @brief Allocates an Eio_File_Direct_Info object from its dedicated pool.
 * @return A pointer to an Eio_File_Direct_Info object, or NULL on allocation failure.
 */
Eio_File_Direct_Info *
eio_direct_info_malloc(void)
{
   return _eio_pool_malloc(&direct_info_pool);
}

/**
 * @brief Frees an Eio_File_Direct_Info object back to its pool.
 * @param data Pointer to the Eio_File_Direct_Info object to free.
 */
void
eio_direct_info_free(Eio_File_Direct_Info *data)
{
   _eio_pool_free(&direct_info_pool, data);
}

/**
 * @brief Allocates an Eio_File_Char object from its dedicated pool.
 * @return A pointer to an Eio_File_Char object, or NULL on allocation failure.
 */
Eio_File_Char *
eio_char_malloc(void)
{
  return _eio_pool_malloc(&char_pool);
}

/**
 * @brief Frees an Eio_File_Char object back to its pool.
 * @param data Pointer to the Eio_File_Char object to free.
 */
void
eio_char_free(Eio_File_Char *data)
{
  _eio_pool_free(&char_pool, data);
}

/**
 * @brief Allocates an Eio_File_Associate object from its dedicated pool.
 *
 * This object is used to associate arbitrary data with an EIO operation,
 * along with a callback to free that data.
 *
 * @param data Pointer to the custom data to associate.
 * @param free_cb Callback function to free the custom data when the Eio_File_Associate object is freed.
 * @return A pointer to an Eio_File_Associate object, or NULL on allocation failure.
 */
Eio_File_Associate *
eio_associate_malloc(const void *data, Eina_Free_Cb free_cb)
{
  Eio_File_Associate *tmp;

  tmp = _eio_pool_malloc(&associate_pool);
  if (!tmp) return tmp;

  tmp->data = (void*) data;
  tmp->free_cb = free_cb;

  return tmp;
}

/**
 * @brief Frees an Eio_File_Associate object and its associated custom data.
 *
 * If a `free_cb` was provided during allocation, it is called with `tmp->data`.
 * The Eio_File_Associate object itself is then returned to its pool.
 *
 * @param data Pointer to the Eio_File_Associate object to free.
 */
void
eio_associate_free(void *data)
{
  Eio_File_Associate *tmp;

  if (!data) return;

  tmp = data;
  if (tmp->free_cb)
    tmp->free_cb(tmp->data);
  _eio_pool_free(&associate_pool, tmp);
}

/**
 * @brief Sends a pack of events or defers sending based on time and memory pressure.
 *
 * This function is used to batch events (e.g., file listing results) to avoid
 * overwhelming the main loop. It sends the pack if enough time (EIO_PACKED_TIME)
 * has passed since the last send. It also checks memory pool limits and may
 * suspend the calling thread if memory usage is too high, waiting for it to drop.
 *
 * @param thread The Ecore_Thread to send the pack to.
 * @param pack The Eina_List of items to send.
 * @param start Pointer to a double storing the timestamp of the last send. This is updated by the function.
 * @return NULL if the pack was sent (or an error occurred), otherwise returns the original `pack` (if sending was deferred).
 */
Eina_List *
eio_pack_send(Ecore_Thread *thread, Eina_List *pack, double *start)
{
   double current;

   current = ecore_time_get();
   if (current - *start > EIO_PACKED_TIME)
     {
        *start = current;
        ecore_thread_feedback(thread, pack);
        return NULL;
     }

   if (memory_pool_limit > 0 &&
       memory_pool_usage > memory_pool_limit)
     {
        eina_lock_take(&(memory_pool_mutex));
        memory_pool_suspended = EINA_TRUE;
        eina_condition_wait(&(memory_pool_cond));
        memory_pool_suspended = EINA_FALSE;
        eina_lock_release(&(memory_pool_mutex));
     }

   return pack;
}

/**
 * @brief Allocates memory for a generic Eio_File structure.
 *
 * This is a simple wrapper around calloc.
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated and zeroed memory, or NULL on failure.
 */
void *
eio_common_alloc(size_t size)
{
   return calloc(1, size);
}

/**
 * @brief Frees memory allocated for an Eio_File structure.
 *
 * This is a simple wrapper around free.
 * @param common Pointer to the Eio_File structure to free.
 */
void
eio_common_free(Eio_File *common)
{
   free(common);
}

/**
 * @brief List of currently active EIO file operation threads.
 * This list is used to track and manage ongoing EIO operations,
 * particularly for cancellation during shutdown.
 */
// For now use a list for simplicity and we should not have that many
// pending request
static Eina_List *tracked_thread = NULL;

/**
 * @brief Registers an Eio_File operation (and its associated thread) in the tracked list.
 * @param common Pointer to the Eio_File structure representing the operation.
 */
void
eio_file_register(Eio_File *common)
{
   tracked_thread = eina_list_append(tracked_thread, common);
}

/**
 * @brief Unregisters an Eio_File operation from the tracked list.
 * Also sets the thread member of the Eio_File structure to NULL.
 * @param common Pointer to the Eio_File structure representing the operation.
 */
void
eio_file_unregister(Eio_File *common)
{
   tracked_thread = eina_list_remove(tracked_thread, common);
   common->thread = NULL;
}

/**
 * @endcond
 */


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @brief Initializes the EIO library.
 *
 * This function sets up Eina, Ecore, logging, memory pools,
 * the EFL IO Manager, and other necessary components for EIO.
 * It uses a reference counter (`_eio_init_count`) to allow multiple
 * init calls, but only performs actual initialization on the first call.
 *
 * @return The current initialization count. Returns 0 or a negative value on failure.
 * @see eio_shutdown()
 */
EIO_API int
eio_init(void)
{
   if (++_eio_init_count != 1)
     return _eio_init_count;

   if (!eina_init())
     {
        fprintf(stderr, "Eio can not initialize Eina\n");
        return --_eio_init_count;
     }

   _eio_log_dom_global = eina_log_domain_register("eio", EIO_DEFAULT_LOG_COLOR);
   if (_eio_log_dom_global < 0)
     {
        EINA_LOG_ERR("Eio can not create a general log domain.");
        goto shutdown_eina;
     }

   if (!ecore_init())
     {
        ERR("Can not initialize Ecore\n");
        goto unregister_log_domain;
     }

   memset(&progress_pool, 0, sizeof(progress_pool));
   memset(&direct_info_pool, 0, sizeof(direct_info_pool));
   memset(&char_pool, 0, sizeof(char_pool));
   memset(&associate_pool, 0, sizeof(associate_pool));

   eina_lock_new(&(progress_pool.lock));
   progress_pool.mem_size = sizeof (Eio_Progress);
   eina_lock_new(&(direct_info_pool.lock));
   direct_info_pool.mem_size = sizeof (Eio_File_Direct_Info);
   eina_lock_new(&(char_pool.lock));
   char_pool.mem_size = sizeof (Eio_File_Char);
   eina_lock_new(&(associate_pool.lock));
   associate_pool.mem_size = sizeof (Eio_File_Associate);

   eina_spinlock_new(&(memory_pool_lock));
   eina_lock_new(&(memory_pool_mutex));
   eina_condition_new(&(memory_pool_cond), &(memory_pool_mutex));

   eio_monitor_init();

   efreet_mime_init();

   io_manager = efl_add(EFL_IO_MANAGER_CLASS, efl_main_loop_get());
   efl_provider_register(efl_main_loop_get(), EFL_IO_MANAGER_CLASS, io_manager);

   eina_log_timing(_eio_log_dom_global,
                   EINA_LOG_STATE_STOP,
                   EINA_LOG_STATE_INIT);

   return _eio_init_count;

unregister_log_domain:
   eina_log_domain_unregister(_eio_log_dom_global);
   _eio_log_dom_global = -1;
shutdown_eina:
   eina_shutdown();
   return --_eio_init_count;
}

/**
 * @brief Shuts down the EIO library.
 *
 * This function cleans up resources allocated by eio_init().
 * It cancels any pending EIO operations, frees memory pools,
 * unregisters the EFL IO Manager, and shuts down Ecore and Eina
 * components used by EIO. It uses a reference counter and only
 * performs actual shutdown when the count reaches zero.
 *
 * @return The current initialization count (0 after successful shutdown of the last reference).
 *         Returns a value greater than 0 if there are still active references.
 *         Returns a negative value or logs an error if shutdown is called without prior init.
 * @see eio_init()
 */
EIO_API int
eio_shutdown(void)
{
   Eio_File_Direct_Info *info;
   Eio_File_Char *cin;
   Eio_Progress *pg;
   Eio_File_Associate *asso;
   Eio_File *f;
   Eina_List *l;

   if (_eio_init_count <= 0)
     {
        ERR("Init count not greater than 0 in shutdown.");
        return 0;
     }
   if (--_eio_init_count != 0)
     return _eio_init_count;

   eina_log_timing(_eio_log_dom_global,
                   EINA_LOG_STATE_START,
                   EINA_LOG_STATE_SHUTDOWN);

   efl_provider_unregister(efl_main_loop_get(), EFL_IO_MANAGER_CLASS, io_manager);
   efl_del(io_manager);
   io_manager = NULL;

   EINA_LIST_FOREACH(tracked_thread, l, f)
     ecore_thread_cancel(f->thread);

   EINA_LIST_FREE(tracked_thread, f)
     {
        if (!ecore_thread_wait(f->thread, 0.5))
          CRI("We couldn't terminate in less than 30s some pending IO. This can led to some crash.");
     }

   efreet_mime_shutdown();

   eio_monitor_shutdown();

   eina_condition_free(&(memory_pool_cond));
   eina_lock_free(&(memory_pool_mutex));
   eina_spinlock_free(&(memory_pool_lock));

   eina_lock_free(&(direct_info_pool.lock));
   eina_lock_free(&(progress_pool.lock));
   eina_lock_free(&(char_pool.lock));
   eina_lock_free(&(associate_pool.lock));

   /* Cleanup pool */
   EINA_TRASH_CLEAN(&progress_pool.trash, pg)
     free(pg);
   progress_pool.count = 0;

   EINA_TRASH_CLEAN(&direct_info_pool.trash, info)
     free(info);
   direct_info_pool.count = 0;

   EINA_TRASH_CLEAN(&char_pool.trash, cin)
     free(cin);
   char_pool.count = 0;

   EINA_TRASH_CLEAN(&associate_pool.trash, asso)
     free(asso);
   associate_pool.count = 0;

   ecore_shutdown();
   eina_log_domain_unregister(_eio_log_dom_global);
   _eio_log_dom_global = -1;
   eina_shutdown();

   return _eio_init_count;
}

/**
 * @brief Sets the memory burst limit for EIO's internal pools.
 *
 * This limit controls the maximum amount of memory EIO's object pools
 * (for progress, direct_info, etc.) can consume collectively.
 * When this limit is reached, new allocations from these pools might
 * be temporarily suspended until usage drops.
 * Setting a limit can help prevent EIO from consuming excessive memory
 * during bursts of activity (e.g., listing a directory with many files).
 *
 * @param limit The maximum memory size in bytes. A value of (size_t)-1 means no limit.
 */
EIO_API void
eio_memory_burst_limit_set(size_t limit)
{
   eina_lock_take(&(memory_pool_mutex));
   memory_pool_limit = limit;
   if (memory_pool_suspended)
     {
        if (memory_pool_usage < memory_pool_limit)
          eina_condition_broadcast(&(memory_pool_cond));
     }
   eina_lock_release(&(memory_pool_mutex));
}

/**
 * @brief Gets the current memory burst limit for EIO's internal pools.
 *
 * @return The current maximum memory size in bytes. (size_t)-1 indicates no limit.
 * @see eio_memory_burst_limit_set()
 */
EIO_API size_t
eio_memory_burst_limit_get(void)
{
   return memory_pool_limit;
}
