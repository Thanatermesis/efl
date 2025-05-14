#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <unistd.h>
#include "Eina.h"
#include "eina_thread_queue.h"
#include "eina_safety_checks.h"
#include "eina_log.h"

#include "eina_private.h"

#ifdef __ATOMIC_RELAXED
#define ATOMIC 1
#endif

// use spinlocks for read/write locks as they lead to more throughput and
// these locks are meant to be held very temporarily, if there is any
// contention at all
#define RW_SPINLOCK 1

/**
 * @internal
 * @def RWLOCK
 * @brief Macro to define the type of Read-Write lock to use.
 * Defaults to Eina_Spinlock if RW_SPINLOCK is defined, otherwise Eina_Lock.
 */

/**
 * @internal
 * @def RWLOCK_NEW
 * @brief Macro to create a new Read-Write lock.
 */

/**
 * @internal
 * @def RWLOCK_FREE
 * @brief Macro to free a Read-Write lock.
 */

/**
 * @internal
 * @def RWLOCK_LOCK
 * @brief Macro to acquire/take a Read-Write lock.
 */

/**
 * @internal
 * @def RWLOCK_UNLOCK
 * @brief Macro to release a Read-Write lock.
 */
#ifdef RW_SPINLOCK
#define RWLOCK           Eina_Spinlock
#define RWLOCK_NEW(x)    eina_spinlock_new(x)
#define RWLOCK_FREE(x)   eina_spinlock_free(x)
#define RWLOCK_LOCK(x)   eina_spinlock_take(x)
#define RWLOCK_UNLOCK(x) eina_spinlock_release(x)
#else
#define RWLOCK           Eina_Lock
#define RWLOCK_NEW(x)    eina_lock_new(x)
#define RWLOCK_FREE(x)   eina_lock_free(x)
#define RWLOCK_LOCK(x)   eina_lock_take(x)
#define RWLOCK_UNLOCK(x) eina_lock_release(x)
#endif

typedef struct _Eina_Thread_Queue_Msg_Block Eina_Thread_Queue_Msg_Block;

/**
 * @internal
 * @struct _Eina_Thread_Queue
 * @brief Internal structure representing a thread queue.
 *
 * This structure holds all the state for a thread queue, including pointers
 * to message blocks, locks, semaphores, and configuration like parent queue
 * or associated file descriptor.
 */
struct _Eina_Thread_Queue
{
   Eina_Thread_Queue_Msg_Block  *data; /**< Pointer to the first message block available for writing. */
   Eina_Thread_Queue_Msg_Block  *last; /**< Pointer to the last message block where new data is appended. */
   Eina_Thread_Queue_Msg_Block  *read; /**< Pointer to the current message block being read from. */
   Eina_Thread_Queue            *parent; /**< Optional parent queue to notify when a message is sent to this queue. */
   RWLOCK                        lock_read; /**< Lock to protect read operations and `read` block manipulation. */
   RWLOCK                        lock_write; /**< Lock to protect write operations and `data`/`last` block manipulation. */
   Eina_Semaphore                sem; /**< Semaphore used for signaling message availability. Incremented on send, decremented on wait. */
#ifndef ATOMIC
   Eina_Spinlock                 lock_pending; /**< Spinlock to protect the `pending` counter if atomic operations are not available. */
#endif
   int                           pending; /**< Count of messages currently pending in the queue. */
   int                           fd; /**< Optional file descriptor to write a byte to when a message is sent. -1 if not set. */
};

/**
 * @internal
 * @union _Eina_Thread_Queue_Msg_Aligned
 * @brief Union to ensure proper alignment for messages within a message block.
 *
 * Messages are always 8-byte aligned. This union helps enforce that by
 * including types that typically require such alignment.
 */
typedef union _Eina_Thread_Queue_Msg_Aligned Eina_Thread_Queue_Msg_Aligned;

union _Eina_Thread_Queue_Msg_Aligned
{
   Eina_Thread_Queue_Msg         msg; /**< The actual message header. */
   void*                         alignment_for_ptr; /**< Pointer type for alignment. */
   long long                     alignment_for_integer; /**< Long long for integer alignment. */
   double                        alignment_for_floating_point_1; /**< Double for floating point alignment. */
   long double                   alignment_for_floating_point_2; /**< Long double for further floating point alignment. */
};

/**
 * @internal
 * @struct _Eina_Thread_Queue_Msg_Block
 * @brief Internal structure representing a block of memory for storing messages.
 *
 * Thread queues manage messages in contiguous blocks of memory. This structure
 * defines such a block, which can be part of a linked list of blocks.
 */
struct _Eina_Thread_Queue_Msg_Block
{
   Eina_Thread_Queue_Msg_Block  *next; /**< Pointer to the next message block in the chain, if any. */
   Eina_Lock                     lock_non_0_ref; /**< Lock used to ensure safe deallocation when ref count transitions to/from zero. */
#ifndef ATOMIC
   Eina_Spinlock                 lock_ref; /**< Spinlock to protect the `ref` counter if atomic operations are not available. */
   Eina_Spinlock                 lock_first; /**< Spinlock to protect the `first` offset if atomic operations are not available. */
#endif
   int                           ref; /**< Reference count for this block (number of active readers/writers). */
   int                           size; /**< Total allocated size of the `data` field in bytes. */
   int                           first; /**< Byte offset of the first message to be read in this block. */
   int                           last; /**< Byte offset indicating the end of the last written message in this block. */
   Eina_Bool                     full : 1; /**< Flag indicating if this block is completely full. */
   Eina_Thread_Queue_Msg_Aligned data[1]; /**< Flexible array member holding the actual message data. Messages are stored contiguously here. */
};

/**
 * @internal
 * @def MIN_SIZE
 * @brief The minimum size for a new message block.
 * This is typically page-aligned minus the header size to optimize memory use.
 * It's set to 4096 bytes minus the size of `Eina_Thread_Queue_Msg_Block`
 * (excluding the flexible array member `data`) plus one `Eina_Thread_Queue_Msg_Aligned` unit.
 */
// the minimum size of any message block holding 1 or more messages
#define MIN_SIZE ((int)(4096 - sizeof(Eina_Thread_Queue_Msg_Block) + sizeof(Eina_Thread_Queue_Msg_Aligned)))

/** @internal @brief Logging domain for eina_thread_queue. */
static int _eina_thread_queue_log_dom = -1;
/** @internal @brief Count of blocks currently in the spare block pool. */
static int _eina_thread_queue_block_pool_count = 0;
/** @internal @brief Spinlock to protect access to the spare block pool. */
static Eina_Spinlock _eina_thread_queue_block_pool_lock;
/** @internal @brief Pool of pre-allocated spare message blocks of MIN_SIZE. */
static Eina_Thread_Queue_Msg_Block *_eina_thread_queue_block_pool = NULL;

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_eina_thread_queue_log_dom, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_eina_thread_queue_log_dom, __VA_ARGS__)

// api's to get message blocks from the pool or put them back in

/**
 * @internal
 * @brief Allocates or retrieves a message block of at least the specified size.
 *
 * Attempts to reuse a block from `_eina_thread_queue_block_pool` if a suitable one
 * (size >= requested size) is available. Otherwise, allocates a new block.
 * Initializes the block's metadata (first, last, ref, full).
 *
 * @param size The minimum required size for the data area of the block.
 * @return A pointer to an initialized Eina_Thread_Queue_Msg_Block, or NULL on allocation failure.
 */
static Eina_Thread_Queue_Msg_Block *
_eina_thread_queue_msg_block_new(int size)
{
   Eina_Thread_Queue_Msg_Block *blk;

   eina_spinlock_take(&(_eina_thread_queue_block_pool_lock));
   if (_eina_thread_queue_block_pool)
     {
        blk = _eina_thread_queue_block_pool;
        if (blk->size >= size)
          {
             blk->first = 0;
             blk->last = 0;
             blk->ref = 0;
             blk->full = 0;
             _eina_thread_queue_block_pool = blk->next;
             blk->next = NULL;
             _eina_thread_queue_block_pool_count--;
             eina_spinlock_release(&(_eina_thread_queue_block_pool_lock));
             return blk;
          }
        blk = NULL;
     }
   eina_spinlock_release(&(_eina_thread_queue_block_pool_lock));

   blk = malloc(sizeof(Eina_Thread_Queue_Msg_Block) -
                sizeof(Eina_Thread_Queue_Msg_Aligned) +
                size);
   if (!blk)
     {
        ERR("Thread queue block buffer of size %i allocation failed", size);
        return NULL;
     }
   blk->next = NULL;
#ifndef ATOMIC
   eina_spinlock_new(&(blk->lock_ref));
   eina_spinlock_new(&(blk->lock_first));
#endif
   eina_lock_new(&(blk->lock_non_0_ref));
   blk->size = size;
   blk->first = 0;
   blk->last = 0;
   blk->ref = 0;
   blk->full = 0;
   return blk;
}

/**
 * @internal
 * @brief Actually frees the memory associated with a message block.
 *
 * This function handles the final deallocation of a block, including freeing
 * its associated locks. It should be called when a block is no longer needed
 * and not being returned to a pool.
 *
 * @param blk The message block to free.
 */
static void
_eina_thread_queue_msg_block_real_free(Eina_Thread_Queue_Msg_Block *blk)
{
   eina_lock_take(&(blk->lock_non_0_ref));
   eina_lock_release(&(blk->lock_non_0_ref));
   eina_lock_free(&(blk->lock_non_0_ref));
#ifndef ATOMIC
   eina_spinlock_take(&(blk->lock_ref));
   eina_spinlock_release(&(blk->lock_ref));
   eina_spinlock_free(&(blk->lock_ref));
   eina_spinlock_take(&(blk->lock_first));
   eina_spinlock_release(&(blk->lock_first));
   eina_spinlock_free(&(blk->lock_first));
#endif
   free(blk);
}

/**
 * @internal
 * @brief Frees a message block, potentially returning it to a pool.
 *
 * If the block's size is `MIN_SIZE` and the pool is not full, the block is
 * added to `_eina_thread_queue_block_pool` for reuse. Otherwise, the block
 * is freed using `_eina_thread_queue_msg_block_real_free()`.
 *
 * @param blk The message block to free or pool.
 */
static void
_eina_thread_queue_msg_block_free(Eina_Thread_Queue_Msg_Block *blk)
{
   if (blk->size == MIN_SIZE)
     {
        eina_spinlock_take(&(_eina_thread_queue_block_pool_lock));
        if (_eina_thread_queue_block_pool_count < 20)
          {
             _eina_thread_queue_block_pool_count++;
             blk->next = _eina_thread_queue_block_pool;
             _eina_thread_queue_block_pool = blk;
             eina_spinlock_release(&(_eina_thread_queue_block_pool_lock));
          }
        else
          {
             eina_spinlock_release(&(_eina_thread_queue_block_pool_lock));
             _eina_thread_queue_msg_block_real_free(blk);
          }
     }
   else _eina_thread_queue_msg_block_real_free(blk);
}

/**
 * @internal
 * @brief Initializes the message block pool.
 *
 * Specifically, this initializes the spinlock used to protect the pool.
 *
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_eina_thread_queue_msg_block_pool_init(void)
{
   return eina_spinlock_new(&_eina_thread_queue_block_pool_lock);
}

/**
 * @internal
 * @brief Shuts down and cleans up the message block pool.
 *
 * Frees all blocks currently in the pool and then frees the pool's spinlock.
 */
static void
_eina_thread_queue_msg_block_pool_shutdown(void)
{
   eina_spinlock_take(&(_eina_thread_queue_block_pool_lock));
   while (_eina_thread_queue_block_pool)
     {
        Eina_Thread_Queue_Msg_Block *blk, *blknext;

        for (;;)
          {
             blk = _eina_thread_queue_block_pool;
             if (!blk) break;
             blknext = blk->next;
             _eina_thread_queue_msg_block_real_free(blk);
             _eina_thread_queue_block_pool = blknext;
          }
     }
   eina_spinlock_release(&(_eina_thread_queue_block_pool_lock));
   eina_spinlock_free(&_eina_thread_queue_block_pool_lock);
}

// utility functions for waiting/waking threads

/**
 * @internal
 * @brief Waits on the thread queue's semaphore.
 *
 * This function is called when a thread needs to wait for a message to become
 * available in the queue. It blocks until the semaphore is released.
 *
 * @param thq The thread queue to wait on.
 */
static void
_eina_thread_queue_wait(Eina_Thread_Queue *thq)
{
   if (!eina_semaphore_lock(&(thq->sem)))
     ERR("Thread queue semaphore lock/wait failed - bad things will happen");
}

/**
 * @internal
 * @brief Wakes up a thread waiting on the queue's semaphore.
 *
 * This function is called after a message has been successfully sent to the
 * queue, to signal any waiting consumer.
 *
 * @param thq The thread queue whose semaphore should be released.
 */
static void
_eina_thread_queue_wake(Eina_Thread_Queue *thq)
{
   if (!eina_semaphore_release(&(thq->sem), 1))
     ERR("Thread queue semaphore release/wakeup faile - bad things will happen");
}

// how to allocate or release memory within one of the message blocks for
// an arbitrary sized bit of message data. the size always includes the
// message header which tells you the size of that message

/**
 * @internal
 * @brief Allocates space for a new message within a message block.
 *
 * This function finds or creates a suitable message block (`Eina_Thread_Queue_Msg_Block`)
 * and reserves `size` bytes within it for a new message. The `size` is
 * rounded up to the nearest 8-byte boundary. If the current `last` block
 * is full or doesn't have enough space, a new block is allocated and
 * appended.
 * The reference count of the block (`blk->ref`) is incremented.
 *
 * @param thq The thread queue.
 * @param size The requested size for the message (including its header).
 * @param[out] blkret Pointer to store the message block where the message was allocated.
 *             This is used as the `allocref` in the public API.
 * @return A pointer to the allocated message space (Eina_Thread_Queue_Msg *),
 *         or NULL if allocation fails (though current implementation might not return NULL directly here,
 *         relying on _eina_thread_queue_msg_block_new to handle fatal errors).
 */
static Eina_Thread_Queue_Msg *
_eina_thread_queue_msg_alloc(Eina_Thread_Queue *thq, int size, Eina_Thread_Queue_Msg_Block **blkret)
{
   Eina_Thread_Queue_Msg_Block *blk;
   Eina_Thread_Queue_Msg *msg = NULL;
   int ref;

   // round up to nearest 8
   size = ((size + 7) >> 3) << 3;
   if (!thq->data)
     {
        if (size < MIN_SIZE)
          thq->data = _eina_thread_queue_msg_block_new(MIN_SIZE);
        else
          thq->data = _eina_thread_queue_msg_block_new(size);
        thq->last = thq->data;
     }
   blk = thq->last;
   if (blk->full)
     {
        if (size < MIN_SIZE)
          blk->next = _eina_thread_queue_msg_block_new(MIN_SIZE);
        else
          blk->next = _eina_thread_queue_msg_block_new(size);
        blk = blk->next;
        thq->last = blk;
     }
   if ((blk->size - blk->last) >= size)
     {
        blk->last += size;
        if (blk->last == blk->size) blk->full = 1;
        msg = (Eina_Thread_Queue_Msg *)((char *)(&(blk->data[0])) + (blk->last - size));
     }
   else
     {
        if (size < MIN_SIZE)
          blk->next = _eina_thread_queue_msg_block_new(MIN_SIZE);
        else
          blk->next = _eina_thread_queue_msg_block_new(size);
        blk = blk->next;
        thq->last = blk;
        blk->last += size;
        if (blk->last == blk->size) blk->full = 1;
        msg = (Eina_Thread_Queue_Msg *)(&(blk->data[0]));
     }
   msg->size = size;
#ifdef ATOMIC
   ref = __atomic_add_fetch(&(blk->ref), 1, __ATOMIC_RELAXED);
#else
   eina_spinlock_take(&(blk->lock_ref));
   blk->ref++;
   ref = blk->ref;
   eina_spinlock_release(&(blk->lock_ref));
#endif
   if (ref == 1) eina_lock_take(&(blk->lock_non_0_ref));
   *blkret = blk;
   return msg;
}

/**
 * @internal
 * @brief Finalizes a message allocation operation on a block.
 *
 * This function decrements the reference count of the given message block.
 * It's called after a message has been written and is ready to be sent,
 * effectively "releasing" the writer's claim on the block for that specific
 * allocation operation. If the reference count drops to zero, it means there are
 * no more active operations (allocations or fetches) on this block, and if it's
 * also fully read, it can be freed.
 *
 * @param blk The message block on which an allocation was completed. This corresponds
 *            to the `allocref` from `eina_thread_queue_send()`.
 */
static void
_eina_thread_queue_msg_alloc_done(Eina_Thread_Queue_Msg_Block *blk)
{
   int ref;
#ifdef ATOMIC
   ref = __atomic_sub_fetch(&(blk->ref), 1, __ATOMIC_RELAXED);
#else
   eina_spinlock_take(&(blk->lock_ref));
   blk->ref--;
   ref = blk->ref;
   eina_spinlock_release(&(blk->lock_ref));
#endif
   if (ref == 0) eina_lock_release(&(blk->lock_non_0_ref));
}

/**
 * @internal
 * @brief Fetches the next available message from the thread queue.
 *
 * This function retrieves the next message from the `read` block of the queue.
 * If the current `read` block is NULL or exhausted, it attempts to acquire the
 * next block from the `data` chain (protected by `lock_write`).
 * The `first` pointer in the block is advanced past the fetched message.
 * The reference count of the block (`blk->ref`) is incremented.
 *
 * @param thq The thread queue.
 * @param[out] blkret Pointer to store the message block from which the message was fetched.
 *             This is used as the `allocref` in the public API.
 * @return A pointer to the fetched message (Eina_Thread_Queue_Msg *), or NULL if
 *         no message is available and no block could be prepared.
 */
static Eina_Thread_Queue_Msg *
_eina_thread_queue_msg_fetch(Eina_Thread_Queue *thq, Eina_Thread_Queue_Msg_Block **blkret)
{
   Eina_Thread_Queue_Msg_Block *blk;
   Eina_Thread_Queue_Msg *msg;
   int ref, first;

   if (!thq->read)
     {
        RWLOCK_LOCK(&(thq->lock_write));
        blk = thq->data;
        if (!blk)
          {
             RWLOCK_UNLOCK(&(thq->lock_write));
             return NULL;
          }
#ifdef ATOMIC
        __atomic_load(&(blk->ref), &ref, __ATOMIC_RELAXED);
#else
        eina_spinlock_take(&(blk->lock_ref));
        ref = blk->ref;
        eina_spinlock_release(&(blk->lock_ref));
#endif
        if (ref > 0) eina_lock_take(&(blk->lock_non_0_ref));
        thq->read = blk;
        if (thq->last == blk) thq->last = blk->next;
        thq->data = blk->next;
        blk->next = NULL;
        if (ref > 0) eina_lock_release(&(blk->lock_non_0_ref));
        RWLOCK_UNLOCK(&(thq->lock_write));
     }
   blk = thq->read;
#ifdef ATOMIC
   __atomic_load(&blk->first, &first, __ATOMIC_RELAXED);
   msg = (Eina_Thread_Queue_Msg *)((char *)(&(blk->data[0])) + first);
   first = __atomic_add_fetch(&(blk->first), msg->size, __ATOMIC_RELAXED);
#else
   eina_spinlock_take(&blk->lock_first);
   msg = (Eina_Thread_Queue_Msg *)((char *)(&(blk->data[0])) + blk->first);
   first = blk->first += msg->size;
   eina_spinlock_release(&blk->lock_first);
#endif
   if (first >= blk->last) thq->read = NULL;
   *blkret = blk;
#ifdef ATOMIC
   __atomic_add_fetch(&(blk->ref), 1, __ATOMIC_RELAXED);
#else
   eina_spinlock_take(&(blk->lock_ref));
   blk->ref++;
   eina_spinlock_release(&(blk->lock_ref));
#endif
   return msg;
}

/**
 * @internal
 * @brief Finalizes a message fetch operation on a block.
 *
 * This function decrements the reference count of the given message block.
 * It's called after a fetched message has been processed. If the reference
 * count drops to zero and all messages in the block have been read
 * (`first >= last`), the block is freed via `_eina_thread_queue_msg_block_free()`.
 *
 * @param blk The message block from which a message was fetched and processed.
 *            This corresponds to the `allocref` from `eina_thread_queue_wait()`
 *            or `eina_thread_queue_poll()`.
 */
static void
_eina_thread_queue_msg_fetch_done(Eina_Thread_Queue_Msg_Block *blk)
{
   int ref, first;

#ifdef ATOMIC
   ref = __atomic_sub_fetch(&(blk->ref), 1, __ATOMIC_RELAXED);
   __atomic_load(&blk->first, &first, __ATOMIC_RELAXED);
#else
   eina_spinlock_take(&(blk->lock_ref));
   blk->ref--;
   ref = blk->ref;
   eina_spinlock_release(&(blk->lock_ref));
   eina_spinlock_take(&blk->lock_first);
   first = blk->first;
   eina_spinlock_release(&blk->lock_first);
#endif
   if ((first >= blk->last) && (ref == 0))
     _eina_thread_queue_msg_block_free(blk);
}


//////////////////////////////////////////////////////////////////////////////
Eina_Bool
eina_thread_queue_init(void)
{
   _eina_thread_queue_log_dom = eina_log_domain_register("eina_thread_queue",
                                                         EINA_LOG_COLOR_DEFAULT);
   if (_eina_thread_queue_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: eina_thread_queue");
        return EINA_FALSE;
     }
   if (!_eina_thread_queue_msg_block_pool_init())
     {
        ERR("Cannot init thread queue block pool spinlock");
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

Eina_Bool
eina_thread_queue_shutdown(void)
{
   _eina_thread_queue_msg_block_pool_shutdown();
   eina_log_domain_unregister(_eina_thread_queue_log_dom);
   return EINA_TRUE;
}

EINA_API Eina_Thread_Queue *
eina_thread_queue_new(void)
{
   Eina_Thread_Queue *thq;

   thq = calloc(1, sizeof(Eina_Thread_Queue));
   if (!thq)
     {
        ERR("Allocation of Thread queue structure failed");
        return NULL;
     }
   thq->fd = -1;
   if (!eina_semaphore_new(&(thq->sem), 0))
     {
        ERR("Cannot init new semaphore for eina_threadqueue");
        free(thq);
        return NULL;
     }
   RWLOCK_NEW(&(thq->lock_read));
   RWLOCK_NEW(&(thq->lock_write));
#ifndef ATOMIC
   eina_spinlock_new(&(thq->lock_pending));
#endif
   return thq;
}

EINA_API void
eina_thread_queue_free(Eina_Thread_Queue *thq)
{
   if (!thq) return;

#ifndef ATOMIC
   eina_spinlock_free(&(thq->lock_pending));
#endif
   RWLOCK_FREE(&(thq->lock_read));
   RWLOCK_FREE(&(thq->lock_write));
   eina_semaphore_free(&(thq->sem));
   free(thq);
}

EINA_API void *
eina_thread_queue_send(Eina_Thread_Queue *thq, int size, void **allocref)
{
   Eina_Thread_Queue_Msg *msg;
   Eina_Thread_Queue_Msg_Block *blk;

   RWLOCK_LOCK(&(thq->lock_write));
   msg = _eina_thread_queue_msg_alloc(thq, size, &blk);
   RWLOCK_UNLOCK(&(thq->lock_write));
   *allocref = blk;
#ifdef ATOMIC
  __atomic_add_fetch(&(thq->pending), 1, __ATOMIC_RELAXED);
#else
   eina_spinlock_take(&(thq->lock_pending));
   thq->pending++;
   eina_spinlock_release(&(thq->lock_pending));
#endif
   return msg;
}

EINA_API void
eina_thread_queue_send_done(Eina_Thread_Queue *thq, void *allocref)
{
   _eina_thread_queue_msg_alloc_done(allocref);
   _eina_thread_queue_wake(thq);
   if (thq->parent)
     {
        void *ref;
        Eina_Thread_Queue_Msg_Sub *msg;

        msg = eina_thread_queue_send(thq->parent,
                                     sizeof(Eina_Thread_Queue_Msg_Sub), &ref);
        if (msg)
          {
             msg->queue = thq;
             eina_thread_queue_send_done(thq->parent, ref);
          }
     }
   if (thq->fd >= 0)
     {
        char dummy = 0;
        if (write(thq->fd, &dummy, 1) != 1)
          ERR("Eina Threadqueue write to fd %i failed", thq->fd);
     }
}

EINA_API void *
eina_thread_queue_wait(Eina_Thread_Queue *thq, void **allocref)
{
   Eina_Thread_Queue_Msg *msg;
   Eina_Thread_Queue_Msg_Block *blk;

   _eina_thread_queue_wait(thq);
   RWLOCK_LOCK(&(thq->lock_read));
   msg = _eina_thread_queue_msg_fetch(thq, &blk);
   RWLOCK_UNLOCK(&(thq->lock_read));
   *allocref = blk;
#ifdef ATOMIC
  __atomic_sub_fetch(&(thq->pending), 1, __ATOMIC_RELAXED);
#else
   eina_spinlock_take(&(thq->lock_pending));
   thq->pending--;
   eina_spinlock_release(&(thq->lock_pending));
#endif
   return msg;
}

EINA_API void
eina_thread_queue_wait_done(Eina_Thread_Queue *thq EINA_UNUSED, void *allocref)
{
   _eina_thread_queue_msg_fetch_done(allocref);
}

EINA_API void *
eina_thread_queue_poll(Eina_Thread_Queue *thq, void **allocref)
{
   Eina_Thread_Queue_Msg *msg;
   Eina_Thread_Queue_Msg_Block *blk;

   RWLOCK_LOCK(&(thq->lock_read));
   msg = _eina_thread_queue_msg_fetch(thq, &blk);
   RWLOCK_UNLOCK(&(thq->lock_read));
   if (msg)
     {
        _eina_thread_queue_wait(thq);
        *allocref = blk;
#ifdef ATOMIC
        __atomic_sub_fetch(&(thq->pending), 1, __ATOMIC_RELAXED);
#else
        eina_spinlock_take(&(thq->lock_pending));
        thq->pending--;
        eina_spinlock_release(&(thq->lock_pending));
#endif
     }
   return msg;
}

EINA_API int
eina_thread_queue_pending_get(const Eina_Thread_Queue *thq)
{
   int pending;

#ifdef ATOMIC
   __atomic_load(&(thq->pending), &pending, __ATOMIC_RELAXED);
#else
   eina_spinlock_take((Eina_Spinlock *)&(thq->lock_pending));
   pending = thq->pending;
   eina_spinlock_release((Eina_Spinlock *)&(thq->lock_pending));
#endif
   return pending;
}

EINA_API void
eina_thread_queue_parent_set(Eina_Thread_Queue *thq, Eina_Thread_Queue *thq_parent)
{
   thq->parent = thq_parent;
}

EINA_API Eina_Thread_Queue *
eina_thread_queue_parent_get(const Eina_Thread_Queue *thq)
{
   return thq->parent;
}

EINA_API void
eina_thread_queue_fd_set(Eina_Thread_Queue *thq, int fd)
{
   thq->fd = fd;
}

EINA_API int
eina_thread_queue_fd_get(const Eina_Thread_Queue *thq)
{
   return thq->fd;
}
