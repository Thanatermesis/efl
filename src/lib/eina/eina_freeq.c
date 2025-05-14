#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Eina.h"
#include "eina_private.h"

#ifdef HAVE_VALGRIND
# include <valgrind.h>
# include <memcheck.h>
#endif

// ========================================================================= //

#define ITEM_FILLPAT_MAX 0
#define ITEM_TOTAL_MAX   ( 256 * 1024)
#define ITEM_MEM_MAX     (  32 * 1024 * 1024)
#if __WORDSIZE == 32
// 4kb blocks
//# define ITEM_BLOCK_COUNT 340
// 64k blocks
# define ITEM_BLOCK_COUNT 5459
// 256k blocks
//# define ITEM_BLOCK_COUNT 21844
#else // __WORDSIZE == 64
// 4kb blocks
//# define ITEM_BLOCK_COUNT 170
// 64k blocks
# define ITEM_BLOCK_COUNT 2730
// 256k blocks
//# define ITEM_BLOCK_COUNT 10922
#endif

// ========================================================================= //

/**
 * @struct _Eina_FreeQ_Item
 * @brief Represents an item to be freed within the Eina_FreeQ.
 *
 * This structure holds the pointer to the memory, the function to free it,
 * and its size. This information is used when the item is processed from
 * the queue.
 */
typedef struct _Eina_FreeQ_Item Eina_FreeQ_Item;
/**
 * @struct _Eina_FreeQ_Block
 * @brief A block of Eina_FreeQ_Item structures.
 *
 * Free queue items are stored in blocks to manage memory efficiently.
 * Each block contains an array of items and pointers to manage the
 * active range of items within the block. Blocks are linked together
 * to form the queue.
 */
typedef struct _Eina_FreeQ_Block Eina_FreeQ_Block;

// ========================================================================= //

// these items are highly compressable. here is a dump of F: ptr freefunc size
// ...
// F: 0xaaab0454dd00 0xffff8b83b628 0x10
// F: 0xaaab0454bd00 0xffff8b83b648 0x20
// F: 0xaaab0454dd10 0xffff8b83b628 0x10
// F: 0xaaab0454bd20 0xffff8b83b648 0x20
// F: 0xaaab0454dd20 0xffff8b83b628 0x10
// F: 0xaaab0454bd40 0xffff8b83b648 0x20
// F: 0xaaab0454dd30 0xffff8b83b628 0x10
// F: 0xaaab0454bd60 0xffff8b83b648 0x20
// F: 0xaaab0454bda0 0xffff8b83b648 0x20
// F: 0xaaab0454bdc0 0xffff8b83b648 0x20
// ...
// F: 0xaaab049176d0 0xffff8b83b648 0x20
// F: 0xaaab04917750 0xffff8b83b648 0x20
// F: 0xaaab04917770 0xffff8b83b648 0x20
// F: 0xaaab04917330 0xffff8b83b648 0x20
// F: 0xaaab0481b3c0 0xffff8b83b628 0x10
// F: 0xaaab049177f0 0xffff8b83b648 0x20
// F: 0xaaab049259a0 0xffff8af1c638 0x38
// F: 0xaaab049172d0 0xffff8b83b648 0x20
// F: 0xaaab049172f0 0xffff8b83b648 0x20
// F: 0xaaab04925c00 0xffff8af1c638 0x38
// F: 0xaaab04925c40 0xffff8af1c638 0x38
// F: 0xaaab0481b3b0 0xffff8b83b628 0x10
// F: 0xaaab04917310 0xffff8b83b648 0x20
// F: 0xaaab04925bc0 0xffff8af1c638 0x38
// F: 0xaaab0491d240 0xffff8af1c638 0x40
// F: 0xaaab0492b3a0 0xffff8af1c638 0x38
// F: 0xaaab049225c0 0xffff8af1c638 (nil)
// F: 0xaaab04917730 0xffff8b83b648 0x20
// F: 0xaaab04917790 0xffff8b83b648 0x20
// F: 0xaaab049177b0 0xffff8b83b648 0x20
// F: 0xaaab0492b710 0xffff8af1c638 0x38
// F: 0xaaab0492b750 0xffff8af1c638 0x38
// F: 0xaaab0481b420 0xffff8b83b628 0x10
// F: 0xaaab049177d0 0xffff8b83b648 0x20
// F: 0xaaab0492b2d0 0xffff8af1c638 0x38
// F: 0xaaab0481b410 0xffff8b83b628 0x10
// F: 0xaaab04917710 0xffff8b83b648 0x20
// F: 0xaaab0492b280 0xffff8af1c638 0x40
// F: 0xaaab0481b430 0xffff8b83b628 0x10
// F: 0xaaab04917810 0xffff8b83b648 0x20
// F: 0xaaab0492b6d0 0xffff8af1c638 0x38
// F: 0xaaab0491ca80 0xffff8af1c638 (nil)
// F: 0xaaab0492b350 0xffff8af1c638 0x40
// F: 0xaaab0490fef0 0xffff8af1c638 0x38
// F: 0xaaab0481b320 0xffff8b83b628 0x10
// F: 0xaaab04916ff0 0xffff8b83b648 0x20
// F: 0xaaab0481b330 0xffff8b83b628 0x10
// F: 0xaaab04917030 0xffff8b83b648 0x20
// F: 0xaaab04920560 0xffff8af1c638 0x38
// F: 0xaaab0481b350 0xffff8b83b628 0x10
// F: 0xaaab049170f0 0xffff8b83b648 0x20
// F: 0xaaab04917050 0xffff8b83b648 0x20
// F: 0xaaab04920510 0xffff8af1c638 0x40
// F: 0xaaab04920850 0xffff8af1c638 0x38
// F: 0xaaab04917070 0xffff8b83b648 0x20
// F: 0xaaab04920800 0xffff8af1c638 0x40
// F: 0xaaab04920c30 0xffff8af1c638 0x38
// F: 0xaaab04917090 0xffff8b83b648 0x20
// F: 0xaaab04920be0 0xffff8af1c638 0x40
// F: 0xaaab04920e60 0xffff8af1c638 0x38
// F: 0xaaab04920e10 0xffff8af1c638 0x40
// F: 0xaaab049212b0 0xffff8af1c638 0x38
// F: 0xaaab0481b340 0xffff8b83b628 0x10
// F: 0xaaab049170d0 0xffff8b83b648 0x20
// F: 0xaaab04921260 0xffff8af1c638 0x40
// F: 0xaaab0481b1f0 0xffff8b83b628 0x10
// F: 0xaaab04875d50 0xffff8b83b648 0x20
// F: 0xaaab0490fea0 0xffff8af1c638 0x40
// F: 0xaaab049102e0 0xffff8af1c638 0x38
// F: 0xaaab04917010 0xffff8b83b648 0x20
// F: 0xaaab0490b4f0 0xffff8af1c638 (nil)
// F: 0xaaab04910290 0xffff8af1c638 0x40
// F: 0xaaab0481b460 0xffff8b83b628 0x10
// F: 0xaaab049178b0 0xffff8b83b648 0x20
// F: 0xaaab0481b450 0xffff8b83b628 0x10
// F: 0xaaab04917870 0xffff8b83b648 0x20
// F: 0xaaab0481b490 0xffff8b83b628 0x10
// F: 0xaaab049e56f0 0xffff8b83b648 0x20
// F: 0xaaab0481b4a0 0xffff8b83b628 0x10
// F: 0xaaab049e5710 0xffff8b83b648 0x20
// F: 0xaaab0481b4b0 0xffff8b83b628 0x10
// F: 0xaaab049e5730 0xffff8b83b648 0x20
// F: 0xaaab0481b4c0 0xffff8b83b628 0x10
// F: 0xaaab049e5750 0xffff8b83b648 0x20
// F: 0xaaab0481b4f0 0xffff8b83b628 0x10
// F: 0xaaab049e57d0 0xffff8b83b648 0x20
// F: 0xaaab049e5990 0xffff8b83b648 0x20
// F: 0xaaab049e5770 0xffff8b83b648 0x20
// F: 0xaaab0481b4d0 0xffff8b83b628 0x10
// F: 0xaaab049e5790 0xffff8b83b648 0x20
// F: 0xaaab049e5a50 0xffff8b83b648 0x20
// F: 0xaaab049e57b0 0xffff8b83b648 0x20
// ...
// F: 0xaaab04d9f330 0xffff8b83b648 0x20
// F: 0xaaab04b18920 0xffff8b83b628 0x10
// F: 0xaaab04d9f350 0xffff8b83b648 0x20
// F: 0xaaab04d4d000 0xffff8b83b648 0x20
// F: 0xaaab04d9f370 0xffff8b83b648 0x20
// F: 0xaaab04d9f390 0xffff8b83b648 0x20
// F: 0xaaab04d9f3b0 0xffff8b83b648 0x20
// F: 0xaaab04b18930 0xffff8b83b628 0x10
// F: 0xaaab04d9f3d0 0xffff8b83b648 0x20
// F: 0xaaab04d9f3f0 0xffff8b83b648 0x20
// F: 0xaaab04d9f410 0xffff8b83b648 0x20
// F: 0xaaab04d9f430 0xffff8b83b648 0x20
// F: 0xaaab04b18940 0xffff8b83b628 0x10
// F: 0xaaab04d9f450 0xffff8b83b648 0x20
// F: 0xaaab04d4d020 0xffff8b83b648 0x20
// F: 0xaaab04d9f470 0xffff8b83b648 0x20
// F: 0xaaab04d9f490 0xffff8b83b648 0x20
// F: 0xaaab04d9f4b0 0xffff8b83b648 0x20
// F: 0xaaab04b18950 0xffff8b83b628 0x10
// F: 0xaaab04d9f4d0 0xffff8b83b648 0x20
// F: 0xaaab04d9f4f0 0xffff8b83b648 0x20
// F: 0xaaab04d9f510 0xffff8b83b648 0x20
// F: 0xaaab04d9f530 0xffff8b83b648 0x20
// F: 0xaaab04b18960 0xffff8b83b628 0x10
// F: 0xaaab04d9f550 0xffff8b83b648 0x20
// F: 0xaaab04640ce0 0xffff8b83b628 0x10
// F: 0xaaab04d9ee10 0xffff8b83b648 0x20
// F: 0xaaab04577e50 0xffff8af1c638 (nil)
// F: 0xaaab04571570 0xffff8af1c638 (nil)
// F: 0xaaab04577ee0 0xffff8af1c638 0x40
// F: 0xaaab0457af50 0xffff8af1c638 (nil)
// F: 0xaaab04571590 0xffff8af1c638 (nil)
// F: 0xaaab0457afe0 0xffff8af1c638 0x40
// F: 0xaaab0457e0c0 0xffff8af1c638 (nil)
// F: 0xaaab0457b360 0xffff8af1c638 (nil)
// F: 0xaaab0457e150 0xffff8af1c638 0x40
// F: 0xaaab04581860 0xffff8af1c638 (nil)
// F: 0xaaab04581330 0xffff8af1c638 (nil)
// F: 0xaaab045818f0 0xffff8af1c638 0x40
// F: 0xaaab0490ed00 0xffff8af1c638 0xc0
// F: 0xaaab0490f090 0xffff8af1c638 0xc0
// F: 0xaaab04922760 0xffff8af1c638 0xc0
// F: 0xaaab04922880 0xffff8af1c638 0xc0
// F: 0xaaab04922a20 0xffff8af1c638 0xc0
// F: 0xaaab0491cb80 0xffff8af1c638 0xc0
// F: 0xaaab0492b8e0 0xffff8af1c638 0xc0
// F: 0xaaab0492c4b0 0xffff8af1c638 0xc0
// F: 0xaaab0492c5c0 0xffff8af1c638 0xc0
// F: 0xaaab0492c750 0xffff8af1c638 0xc0
// F: 0xaaab04926230 0xffff8af1c638 0xc0
// F: 0xaaab04920d00 0xffff8af1c638 0xc0
// F: 0xaaab04920aa0 0xffff8af1c638 0xc0
// F: 0xaaab04a05280 0xffff8af1c638 0xc0
// F: 0xaaab04a053d0 0xffff8af1c638 0xc0
// F: 0xaaab04a05520 0xffff8af1c638 0xc0
// F: 0xaaab049f3860 0xffff8af1c638 0xc0
// F: 0xaaab049f3a30 0xffff8af1c638 0xc0
// F: 0xaaab04a06e60 0xffff8af1c638 0xc0
// F: 0xaaab04a25490 0xffff8af1c638 0xc0
// F: 0xaaab04a55170 0xffff8af1c638 0xc0
// F: 0xaaab04a55ca0 0xffff8af1c638 0xc0
// ...
// so in future maybe create delta compression. keep a "start value" in the
// Eina_FreeQ_Block block for each to begin from (and update these as we
// march blcok->start forward (or at least update them when we finish a run
// of processing items at the end of the processing.
//
// we can store things as DELTAS from the preview value. ptr, func, size all
// are ptr sized values so we can compress them with deltas and thus encode
// them in variable runs of bytes depending on the size of the delta. e.g.
// use LEB128 maybe or PrefixVariant.
//
// after some playng leb128 seems to be the best from simplicity (so fast
// encode which matters and decode needs to be good too) and size. i saw
// a reduction to 24% of the original data size this way based on the sample
// data i collected like above. is it worth the extra cycles? don't know.
//
// when looking at the deltas i noticed that func and sie delats are very
// often 0 for long runs. this means we can probably use RLE effectively
// if we split this into 3 streams wahc delta compressed then RLE compressed
// per stream. walking is more complex and filling the block means taking
// a guess at pre-allocating offsets per stream so it may not fill the blocks
// as affectively then. again - is it worth it? need to measure if RLE helps
// a lot or not in keeping size down in addition to delta + leb128.
struct _Eina_FreeQ_Item
{
   void    *ptr; /**< Pointer to the memory to be freed. */
   void   (*free_func) (void *ptr); /**< Custom free function for the pointer. If NULL, libc free() is used. */
   size_t  size; /**< Size of the memory pointed to by ptr. Used for debugging and memory limit calculations. */
};

struct _Eina_FreeQ_Block
{
   int start; /**< Index of the first valid item in this block. Items from 0 to start-1 have been processed. */
   int end; /**< Index of the next available slot to store a new item. Items from start to end-1 are pending. */
   Eina_FreeQ_Block *next; /**< Pointer to the next block in the queue. NULL if this is the last block. */
   Eina_FreeQ_Item items[ITEM_BLOCK_COUNT]; /**< Array of items stored in this block. ITEM_BLOCK_COUNT is the capacity. */
};

/**
 * @struct _Eina_FreeQ
 * @brief Represents a free queue.
 *
 * This structure holds all the state for a free queue, including locks for
 * thread-safety, item counts, memory usage, pointers to the blocks of items,
 * and configuration flags like bypass or postponed behavior.
 */
struct _Eina_FreeQ
{
   Eina_Lock lock; /**< Recursive lock for thread-safe operations. Not used if 'unlocked' is EINA_TRUE. */
   int count; /**< Current number of items in the queue across all blocks. */
   int count_max; /**< Maximum number of items allowed in the queue. -1 for no limit. */
   size_t mem_max; /**< Maximum total memory (in bytes) allowed for items in the queue. 0 for no limit (for default queues, postponed queues have no limit by default). */
   size_t mem_total; /**< Current total memory (in bytes) occupied by items in the queue. */
   Eina_FreeQ_Block *blocks; /**< Pointer to the first block in the linked list of blocks. This is where items are processed from. */
   Eina_FreeQ_Block *block_last; /**< Pointer to the last block in the linked list. New items are added here. */
   Eina_Bool bypass; /**< If EINA_TRUE, items are freed immediately instead of being queued. */
   Eina_Bool postponed; /**< If EINA_TRUE, this queue is of EINA_FREEQ_POSTPONED type. */
   Eina_Bool unlocked; /**< If EINA_TRUE, lock operations are skipped (queue is considered thread-local). */
};

// ========================================================================= //

/** @internal @brief The global main free queue, typically processed by the main loop. */
static Eina_FreeQ    *_eina_freeq_main              = NULL;
/** @internal @brief Global bypass flag, initialized from EINA_FREEQ_BYPASS env var. -1 means not initialized. */
static int            _eina_freeq_bypass            = -1;
/** @internal @brief Max item size for memory filling, from EINA_FREEQ_FILL_MAX env var. */
static unsigned int   _eina_freeq_fillpat_max       = ITEM_FILLPAT_MAX;
/** @internal @brief Byte value for filling memory on add, from EINA_FREEQ_FILL env var. Default 0x55. */
static unsigned char  _eina_freeq_fillpat_val       = 0x55;
/** @internal @brief Byte value for filling memory on free, from EINA_FREEQ_FILL_FREED env var. Default 0x77. */
static unsigned char  _eina_freeq_fillpat_freed_val = 0x77;
/** @internal @brief Default max items for new queues, from EINA_FREEQ_TOTAL_MAX env var. */
static int            _eina_freeq_total_max         = ITEM_TOTAL_MAX;
/** @internal @brief Default max memory (in Kb, converted to bytes) for new queues, from EINA_FREEQ_MEM_MAX env var. */
static size_t         _eina_freeq_mem_max           = ITEM_MEM_MAX;

// debgging/tuning info to enable in future when gathering stats
#if 0
static int            _max_seen = 0;
# define FQMAX(fq) \
   if (fq == _eina_freeq_main) { \
      if (fq->count > _max_seen) { \
         _max_seen = fq->count; \
         printf("FQ max: %i\n", _max_seen); \
      } \
   }
#else
# define FQMAX(fq)
#endif

// ========================================================================= //

#define LOCK_FQ(fq); do { \
   if (!fq->unlocked) eina_lock_take(&(fq->lock)); } while(0)
#define UNLOCK_FQ(fq); do { \
   if (!fq->unlocked) eina_lock_release(&(fq->lock)); } while(0)

// ========================================================================= //

/**
 * @internal
 * @brief Fills a memory region with the debug pattern `_eina_freeq_fillpat_val`.
 * This is typically done when an item is added to the free queue to help
 * detect use-after-free errors.
 * @param ptr The memory region to fill.
 * @param size The size of the memory region.
 */
static inline void
_eina_freeq_fill_do(void *ptr, size_t size)
{
   if (ptr) memset(ptr, _eina_freeq_fillpat_val, size);
}

/**
 * @internal
 * @brief Fills a memory region with the debug pattern `_eina_freeq_fillpat_freed_val`.
 * This is done just before the memory is actually freed, to further help
 * detect use-after-free or double-free issues.
 * @param ptr The memory region to fill.
 * @param size The size of the memory region.
 */
static inline void
_eina_freeq_freed_fill_do(void *ptr, size_t size)
{
   if (_eina_freeq_fillpat_freed_val == 0) return; // Optimization: if fill pattern is 0, skip.
   if (ptr) memset(ptr, _eina_freeq_fillpat_freed_val, size);
}

/**
 * @internal
 * @brief Checks if a memory region is still filled with `_eina_freeq_fillpat_val`.
 * This function is called before freeing memory to ensure it hasn't been
 * modified after being added to the free queue, which could indicate a
 * use-after-free bug.
 * @param ptr The memory region to check.
 * @param free_func The free function associated with this pointer (for logging).
 * @param size The size of the memory region.
 */
static void
_eina_freeq_fill_check(void *ptr, void (*free_func) (void *ptr), size_t size)
{
   unsigned char *p0 = ptr, *p = p0, *pe = p + size;
   for (; p < pe; p++)
     {
        if (*p != _eina_freeq_fillpat_val) goto err;
     }
   return;
err:
   EINA_LOG_ERR("Pointer %p size %lu freed by %p has fill error %x != %x @ %lu",
                p0, (unsigned long)size, free_func,
                (unsigned int)*p, (unsigned int)_eina_freeq_fillpat_val,
                (unsigned long)(p - p0));
}

/**
 * @internal
 * @brief Performs the actual freeing of a pointer.
 * This includes optional fill checks and filling with a "freed" pattern
 * before calling the actual free function.
 * @param ptr The pointer to free.
 * @param free_func The function to use for freeing (e.g., libc free).
 * @param size The size of the memory associated with ptr.
 */
static void
_eina_freeq_free_do(void *ptr,
                    void (*free_func) (void *ptr),
                    size_t size)
{
   // Only perform fill checks and freed_fill if size is within debuggable range.
   if (EINA_LIKELY((size > 0) && (size < _eina_freeq_fillpat_max)))
     {
        _eina_freeq_fill_check(ptr, free_func, size);
        _eina_freeq_freed_fill_do(ptr, size);
     }
   free_func(ptr);
   return;
}

/**
 * @internal
 * @brief Appends a new, empty Eina_FreeQ_Block to the given free queue.
 * This is called when the current last block is full and a new item needs
 * to be added.
 * @param fq The free queue to append a new block to.
 * @return EINA_TRUE on success, EINA_FALSE if memory allocation fails.
 */
static Eina_Bool
_eina_freeq_block_append(Eina_FreeQ *fq)
{
   Eina_FreeQ_Block *fb = malloc(sizeof(Eina_FreeQ_Block));
   if (!fb) return EINA_FALSE;
   fb->start = 0;
   fb->end = 0;
   fb->next = NULL;
   if (!fq->blocks) fq->blocks = fb;
   else fq->block_last->next = fb;
   fq->block_last = fb;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Processes (frees) the oldest item from the free queue.
 * It takes the item from the `start` of the first block, calls
 * `_eina_freeq_free_do` on it, updates queue statistics, and
 * frees the block if it becomes empty.
 * @param fq The free queue to process an item from.
 */
static void
_eina_freeq_process(Eina_FreeQ *fq)
{
   Eina_FreeQ_Block *fb = fq->blocks;
   if (!fb) return;
   _eina_freeq_free_do(fb->items[fb->start].ptr,
                       fb->items[fb->start].free_func,
                       fb->items[fb->start].size);
   fq->mem_total -= fb->items[fb->start].size;
   fb->start++;
   fq->count--;
   if (fb->start == fb->end)
     {
        fq->blocks = fb->next;
        if (!fq->blocks) fq->block_last = NULL;
        free(fb);
     }
}

/**
 * @internal
 * @brief Flushes items from the free queue until count and memory limits are met.
 * This function does not acquire the queue's lock, assuming the caller
 * handles locking. It repeatedly calls `_eina_freeq_process` if the queue
 * exceeds its configured `count_max` or `mem_max`.
 * This function does nothing for postponed queues as they have different flushing logic.
 * @param fq The free queue to flush.
 */
static void
_eina_freeq_flush_nolock(Eina_FreeQ *fq)
{
   // Postponed queues are flushed differently (typically all at once at a specific point).
   if (fq->postponed) return;

   FQMAX(fq); // Debug macro, likely for tracking max queue size.
   while ((fq->count > fq->count_max) || (fq->mem_total > fq->mem_max))
     _eina_freeq_process(fq);
}

// ========================================================================= //

/**
 * @internal
 * @brief Creates and initializes a new free queue of the EINA_FREEQ_DEFAULT type.
 * This involves allocating the Eina_FreeQ structure, initializing its lock,
 * setting default limits (count_max, mem_max) based on global settings
 * (which can be influenced by environment variables), and determining
 * the initial bypass state.
 * Environment variables like EINA_FREEQ_BYPASS, EINA_FREEQ_FILL_MAX, etc.,
 * are read here if not already cached.
 * @return A pointer to the newly created Eina_FreeQ, or NULL on allocation failure.
 */
static Eina_FreeQ *
_eina_freeq_new_default(void)
{
   Eina_FreeQ *fq;

   // Initialize global settings from environment variables if this is the first time.
   if (EINA_UNLIKELY(_eina_freeq_bypass == -1))
     {
        const char *s;
        int v;

        s = getenv("EINA_FREEQ_BYPASS");
        if (s)
          {
             v = atoi(s);
             if (v == 0) _eina_freeq_bypass = 0;
             else _eina_freeq_bypass = 1;
          }
        if (_eina_freeq_bypass == -1)
          {
#ifdef HAVE_VALGRIND
             if (RUNNING_ON_VALGRIND) _eina_freeq_bypass = 1;
             else
#endif
               _eina_freeq_bypass = 0;
          }
        s = getenv("EINA_FREEQ_FILL_MAX");
        if (s) _eina_freeq_fillpat_max = atoi(s);
        s = getenv("EINA_FREEQ_TOTAL_MAX");
        if (s) _eina_freeq_total_max = atoi(s);
        s = getenv("EINA_FREEQ_MEM_MAX");
        if (s) _eina_freeq_mem_max = atoi(s) * 1024;
        s = getenv("EINA_FREEQ_FILL");
        if (s) _eina_freeq_fillpat_val = atoi(s);
        s = getenv("EINA_FREEQ_FILL_FREED");
        if (s) _eina_freeq_fillpat_freed_val = atoi(s);
     }
   fq = calloc(1, sizeof(Eina_FreeQ));
   if (!fq) return NULL;
   eina_lock_recursive_new(&(fq->lock));
   fq->count_max = _eina_freeq_total_max;
   fq->mem_max = _eina_freeq_mem_max;
   fq->bypass = _eina_freeq_bypass;
   return fq;
}

/**
 * @internal
 * @brief Creates and initializes a new free queue of the EINA_FREEQ_POSTPONED type.
 * Postponed queues are simpler: they typically have no memory or count limits
 * by default, are not bypassed, and are considered thread-local (unlocked).
 * @return A pointer to the newly created Eina_FreeQ, or NULL on allocation failure.
 */
static Eina_FreeQ *
_eina_freeq_new_postponed(void)
{
   Eina_FreeQ *fq;

   fq = calloc(1, sizeof(*fq));
   if (!fq) return NULL;
   fq->mem_max = 0;
   fq->count_max = -1;
   fq->postponed = EINA_TRUE;
   fq->unlocked = EINA_TRUE;
   return fq;
}

EINA_API Eina_FreeQ *
eina_freeq_new(Eina_FreeQ_Type type)
{
   switch (type)
     {
      case EINA_FREEQ_DEFAULT:
        return _eina_freeq_new_default();
      case EINA_FREEQ_POSTPONED:
        return _eina_freeq_new_postponed();
      default:
        return NULL;
     }
}

EINA_API void
eina_freeq_free(Eina_FreeQ *fq)
{
   if (!fq) return;
   if (fq == _eina_freeq_main) _eina_freeq_main = NULL;
   eina_freeq_clear(fq);
   if (!fq->unlocked) eina_lock_free(&(fq->lock));
   free(fq);
}

EINA_API Eina_FreeQ_Type
eina_freeq_type_get(Eina_FreeQ *fq)
{
   if (fq && fq->postponed)
     return EINA_FREEQ_POSTPONED;
   return EINA_FREEQ_DEFAULT;
}

/**
 * @internal
 * @brief Sets the global main free queue.
 * This function is not part of the public EINA_API but is used internally,
 * for example, during Eina initialization to set up the default main free queue.
 * @param fq The Eina_FreeQ instance to be set as the main free queue.
 */
void
eina_freeq_main_set(Eina_FreeQ *fq)
{
   if (!fq) return;
   _eina_freeq_main = fq;
}

EINA_API Eina_FreeQ *
eina_freeq_main_get(void)
{
   return _eina_freeq_main;
}

EINA_API void
eina_freeq_count_max_set(Eina_FreeQ *fq, int count)
{
   if (!fq) return;
   if (fq->postponed) return;
   if (count < 0) count = -1;
   LOCK_FQ(fq);
   fq->bypass = 0;
   fq->count_max = count;
   _eina_freeq_flush_nolock(fq);
   UNLOCK_FQ(fq);
}

EINA_API int
eina_freeq_count_max_get(Eina_FreeQ *fq)
{
   int count;

   if (!fq) return 0;
   LOCK_FQ(fq);
   if (fq->bypass) count = 0;
   else count = fq->count_max;
   UNLOCK_FQ(fq);
   return count;
}

EINA_API void
eina_freeq_mem_max_set(Eina_FreeQ *fq, size_t mem)
{
   if (!fq) return;
   if (fq->postponed) return;
   LOCK_FQ(fq);
   fq->bypass = 0;
   fq->mem_max = mem;
   _eina_freeq_flush_nolock(fq);
   UNLOCK_FQ(fq);
}

EINA_API size_t
eina_freeq_mem_max_get(Eina_FreeQ *fq)
{
   size_t mem;

   if (!fq) return 0;
   LOCK_FQ(fq);
   if (fq->bypass) mem = 0;
   else mem = fq->mem_max;
   UNLOCK_FQ(fq);
   return mem;
}

EINA_API void
eina_freeq_clear(Eina_FreeQ *fq)
{
   if (!fq) return;
   LOCK_FQ(fq);
   FQMAX(fq);
   while (fq->count > 0) _eina_freeq_process(fq);
   UNLOCK_FQ(fq);
}

EINA_API void
eina_freeq_reduce(Eina_FreeQ *fq, int count)
{
   if (!fq) return;
   LOCK_FQ(fq);
   FQMAX(fq);
   while ((fq->count > 0) && (count > 0))
     {
        _eina_freeq_process(fq);
        count--;
     }
   UNLOCK_FQ(fq);
}

EINA_API Eina_Bool
eina_freeq_ptr_pending(Eina_FreeQ *fq)
{
   Eina_Bool pending;

   if (!fq) return EINA_FALSE;
   LOCK_FQ(fq);
   if (fq->blocks) pending = EINA_TRUE;
   else pending = EINA_FALSE;
   UNLOCK_FQ(fq);
   return pending;
}

EINA_API void
eina_freeq_ptr_add(Eina_FreeQ *fq,
                   void *ptr,
                   void (*free_func) (void *ptr),
                   size_t size)
{
   Eina_FreeQ_Block *fb;

   if (!ptr) return;
   if (!free_func) free_func = free;

   if (((fq->count_max <= 0) || (fq->mem_max <= 0)) &&
       (!fq->postponed))
     {
        free_func(ptr);
        return;
     }

   if ((((fq) && !fq->postponed) || (!fq)) &&
       (size < _eina_freeq_fillpat_max) && (size > 0))
     _eina_freeq_fill_do(ptr, size);

   if (!fq || fq->bypass)
     {
        _eina_freeq_free_do(ptr, free_func, size);
        return;
     }

   LOCK_FQ(fq);
   if ((!fq->block_last) || (fq->block_last->end == ITEM_BLOCK_COUNT))
     {
        if (!_eina_freeq_block_append(fq))
          {
             UNLOCK_FQ(fq);
             if (!fq->postponed)
               _eina_freeq_free_do(ptr, free_func, size);
             else
               EINA_LOG_ERR("Could not add a pointer to the free queue! This "
                            "program will leak resources!");
             return;
          }
     }

   fb = fq->block_last;
   fb->items[fb->end].ptr = ptr;
   fb->items[fb->end].free_func = free_func;
   fb->items[fb->end].size = size;
   fb->end++;
   fq->count++;
   fq->mem_total += size;
   _eina_freeq_flush_nolock(fq);
   UNLOCK_FQ(fq);
}
