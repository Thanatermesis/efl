#define EFL_IO_READER_PROTECTED 1
#define EFL_IO_WRITER_PROTECTED 1

#include "config.h"
#include "Efl.h"

#define MY_CLASS EFL_IO_QUEUE_CLASS

/*
 * This queue is simple and based on a single buffer that is
 * reallocated as needed up to some limit, keeping some pre-allocated
 * amount of bytes.
 *
 * Writes appends to the buffer. Reads consume and remove data from
 * buffer head.
 *
 * To avoid too much memmove(), reads won't immediately remove data,
 * instead will only increment position_read and allow some
 * slack. When the slack limit is reached or the buffer needs more
 * memory for write, then the memmove() happens.
 *
 * A more complex and possibly efficient version of this would be to
 * keep a list of internal buffers of fixed size. Writing would result
 * into segment and write into these chunks, creating new if
 * needed. Reading would consume from multiple chunks and if they're
 * all used, would be freed.
 */

/**
 * @brief Private data structure for Efl_Io_Queue.
 */
typedef struct _Efl_Io_Queue_Data
{
   uint8_t *bytes;          /**< The internal buffer holding the queue data. */
   size_t allocated;        /**< Current allocated size of the buffer. */
   size_t preallocated;     /**< Minimum amount of memory to keep preallocated. */
   size_t limit;            /**< Maximum size the buffer can grow to. 0 means no limit. */
   size_t position_read;    /**< Current read offset in the buffer. Used to implement slack. */
   size_t position_write;   /**< Current write offset in the buffer. Data is [position_read, position_write). */
   Eina_Bool pending_eos;   /**< EINA_TRUE if EOS is marked but data still needs to be read. */
   Eina_Bool eos;           /**< EINA_TRUE if End-Of-Stream has been reached and signaled. */
   Eina_Bool closed;        /**< EINA_TRUE if the queue has been closed. */
   Eina_Bool can_read;      /**< Cached value of efl_io_reader_can_read_get(). */
   Eina_Bool can_write;     /**< Cached value of efl_io_writer_can_write_get(). */
} Efl_Io_Queue_Data;

/**
 * @brief Reallocates the internal buffer to the specified size.
 *
 * @param o The Efl_Io_Queue object.
 * @param pd The private data of the Efl_Io_Queue object.
 * @param size The new desired size for the buffer.
 * @return EINA_TRUE if reallocation occurred, EINA_FALSE otherwise (e.g., size is same as current).
 */
static Eina_Bool
_efl_io_queue_realloc(Eo *o, Efl_Io_Queue_Data *pd, size_t size)
{
   void *tmp;
   size_t limit = efl_io_queue_limit_get(o);

   if ((limit > 0) && (size > limit))
     size = limit;

   if (pd->allocated == size) return EINA_FALSE;

   if (size == 0)
     {
        free(pd->bytes);
        tmp = NULL;
     }
   else
     {
        tmp = realloc(pd->bytes, size);
        EINA_SAFETY_ON_NULL_RETURN_VAL(tmp, EINA_FALSE);
     }

   pd->bytes = tmp;
   pd->allocated = size;
   return EINA_TRUE;
}

/**
 * @brief Calculates the slack amount based on current buffer usage.
 *
 * Slack is the amount of "wasted" space at the beginning of the buffer
 * (due to reads) that is tolerated before a memmove() is triggered to
 * reclaim it. This helps to avoid frequent memmove() operations.
 *
 * @param pd The private data of the Efl_Io_Queue object.
 * @return The calculated slack size.
 */
static size_t
_efl_io_queue_slack_get(const Efl_Io_Queue_Data *pd)
{
   const size_t used = pd->position_write - pd->position_read;

   if (used >= 4096) return 4096;
   else if (used >= 1024) return 1024;
   else if (used >= 128) return 128;
   else return 32;
}

/**
 * @brief Reallocates the internal buffer to a rounded-up size.
 *
 * This function rounds the requested size up to certain thresholds
 * (32, 128, 1024, 4096 bytes) to reduce the frequency of reallocations.
 *
 * @param o The Efl_Io_Queue object.
 * @param pd The private data of the Efl_Io_Queue object.
 * @param size The desired minimum size.
 * @return EINA_TRUE if reallocation occurred, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_io_queue_realloc_rounded(Eo *o, Efl_Io_Queue_Data *pd, size_t size)
{
   if ((size > 0) && (size < 128))
     size = ((size / 32) + 1) * 32;
   else if (size < 1024)
     size = ((size / 128) + 1) * 128;
   else if (size < 8192)
     size = ((size / 1024) + 1) * 1024;
   else
     size = ((size / 4096) + 1) * 4096;

   return _efl_io_queue_realloc(o, pd, size);
}

/**
 * @brief Adjusts the buffer by moving data to the beginning.
 *
 * This function performs a memmove() to shift the unread data
 * (from position_read to position_write) to the start of the buffer.
 * It then resets position_read to 0 and updates position_write accordingly.
 * This reclaims space at the beginning of the buffer.
 *
 * @param pd The private data of the Efl_Io_Queue object.
 */
static void
_efl_io_queue_adjust(Efl_Io_Queue_Data *pd)
{
   size_t used = pd->position_write - pd->position_read;
   memmove(pd->bytes, pd->bytes + pd->position_read, used);
   pd->position_write = used;
   pd->position_read = 0;
}

/**
 * @brief Adjusts the buffer and reallocates if spare space is too large or too small.
 *
 * This function first checks if an adjustment (memmove) is needed based on
 * the current slack and limit. Then, it checks if the spare space at the end
 * of the buffer is significantly larger than the slack, and if so, shrinks
 * the buffer (respecting preallocated size).
 *
 * @param o The Efl_Io_Queue object.
 * @param pd The private data of the Efl_Io_Queue object.
 */
static void
_efl_io_queue_adjust_and_realloc_if_needed(Eo *o, Efl_Io_Queue_Data *pd)
{
   const size_t slack = _efl_io_queue_slack_get(pd);
   size_t spare;

   if (pd->limit > 0)
     {
        if (pd->position_write + slack >= pd->limit)
          _efl_io_queue_adjust(pd);
     }
   else if (pd->position_read > slack)
     _efl_io_queue_adjust(pd);

   spare = pd->allocated - pd->position_write;
   if (spare > slack)
     {
        size_t new_size = pd->position_write + slack;

        /*
         * this may result in going over slack again, no
         * problems with that.
         */
        if (new_size < pd->preallocated)
          new_size = pd->preallocated;

        /* use rounded so we avoid too many reallocs */
        _efl_io_queue_realloc_rounded(o, pd, new_size);
     }
}

static void
_efl_io_queue_update_cans(Eo *o, Efl_Io_Queue_Data *pd)
{
   size_t used = pd->position_write - pd->position_read;
   size_t limit;

   efl_io_reader_can_read_set(o, used > 0);
   if (pd->closed) return; /* may be closed from "can_read,changed" */

   limit = efl_io_queue_limit_get(o);
   if (pd->pending_eos)
     efl_io_writer_can_write_set(o, EINA_FALSE);
   else
     efl_io_writer_can_write_set(o, (limit == 0) || (used < limit));
}

EOLIAN static void
_efl_io_queue_preallocate(Eo *o, Efl_Io_Queue_Data *pd, size_t size)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o));
   if (pd->allocated < size)
     _efl_io_queue_realloc_rounded(o, pd, size);
   pd->preallocated = size;
}

EOLIAN static void
_efl_io_queue_limit_set(Eo *o, Efl_Io_Queue_Data *pd, size_t limit)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o));

   if (pd->limit == limit) return;
   pd->limit = limit;
   if (pd->limit == 0) goto end;

   _efl_io_queue_adjust(pd);

   if (pd->allocated > limit)
     _efl_io_queue_realloc(o, pd, limit);

   if (pd->position_write > limit)
     {
        pd->position_write = limit;
        if (pd->position_read > limit) pd->position_read = limit;
     }

   _efl_io_queue_adjust_and_realloc_if_needed(o, pd);
   efl_event_callback_call(o, EFL_IO_QUEUE_EVENT_SLICE_CHANGED, NULL);
   if (pd->closed) return;

 end:
   _efl_io_queue_update_cans(o, pd);
}

/**
 * @brief Gets the configured limit for the queue size.
 * @param o The Efl_Io_Queue object (unused).
 * @param pd The private data of the Efl_Io_Queue object.
 * @return The size limit in bytes. 0 means no limit.
 */
EOLIAN static size_t
_efl_io_queue_limit_get(const Eo *o EINA_UNUSED, Efl_Io_Queue_Data *pd)
{
   return pd->limit;
}

/**
 * @brief Gets the current number of bytes used in the queue.
 * @param o The Efl_Io_Queue object (unused).
 * @param pd The private data of the Efl_Io_Queue object.
 * @return The number of bytes currently stored and readable.
 */
EOLIAN static size_t
_efl_io_queue_usage_get(const Eo *o EINA_UNUSED, Efl_Io_Queue_Data *pd)
{
   return pd->position_write - pd->position_read;
}

/**
 * @brief Gets a slice representing the currently readable data in the queue.
 * @param o The Efl_Io_Queue object.
 * @param pd The private data of the Efl_Io_Queue object.
 * @return An Eina_Slice pointing to the readable data. The slice is empty if closed or no data.
 */
EOLIAN static Eina_Slice
_efl_io_queue_slice_get(const Eo *o, Efl_Io_Queue_Data *pd)
{
   Eina_Slice slice = { };

   if (!efl_io_closer_closed_get(o))
     {
        slice.mem = pd->bytes + pd->position_read;
        slice.len = efl_io_queue_usage_get(o);
     }

   return slice;
}

/**
 * @brief Clears all data from the queue.
 *
 * Resets read and write positions. If EOS was pending, it's now signaled.
 * Updates can_read status and emits SLICE_CHANGED event.
 *
 * @param o The Efl_Io_Queue object.
 * @param pd The private data of the Efl_Io_Queue object.
 */
EOLIAN static void
_efl_io_queue_clear(Eo *o, Efl_Io_Queue_Data *pd)
{
   pd->position_read = 0;
   pd->position_write = 0;
   efl_io_reader_can_read_set(o, EINA_FALSE);
   efl_event_callback_call(o, EFL_IO_QUEUE_EVENT_SLICE_CHANGED, NULL);
   if (pd->closed) return;
   if (pd->pending_eos)
     efl_io_reader_eos_set(o, EINA_TRUE);
}

/**
 * @brief Marks the queue for End-Of-Stream (EOS).
 *
 * If there's no data currently in the queue, EOS is signaled immediately.
 * Otherwise, EOS is marked as pending and will be signaled once all
 * existing data is read.
 *
 * @param o The Efl_Io_Queue object.
 * @param pd The private data of the Efl_Io_Queue object.
 */
EOLIAN static void
_efl_io_queue_eos_mark(Eo *o, Efl_Io_Queue_Data *pd)
{
   if (pd->eos) return;

   if (efl_io_queue_usage_get(o) > 0)
     pd->pending_eos = EINA_TRUE;
   else
     efl_io_reader_eos_set(o, EINA_TRUE);
}

/**
 * @brief Finalizes the Efl_Io_Queue object.
 *
 * Calls the parent's finalize method and then updates the can_read/can_write status.
 *
 * @param o The Efl_Io_Queue object.
 * @param pd The private data of the Efl_Io_Queue object.
 * @return The finalized Efl_Object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_io_queue_efl_object_finalize(Eo *o, Efl_Io_Queue_Data *pd EINA_UNUSED)
{
   o = efl_finalize(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   _efl_io_queue_update_cans(o, pd);

   return o;
}

/**
 * @brief Destructor for the Efl_Io_Queue object.
 *
 * Ensures the queue is closed, calls the parent's destructor, and frees
 * the internal buffer.
 *
 * @param o The Efl_Io_Queue object.
 * @param pd The private data of the Efl_Io_Queue object.
 */
EOLIAN static void
_efl_io_queue_efl_object_destructor(Eo *o, Efl_Io_Queue_Data *pd)
{
   if (!efl_io_closer_closed_get(o))
     {
        efl_event_freeze(o);
        efl_io_closer_close(o);
        efl_event_thaw(o);
     }

   efl_destructor(efl_super(o, MY_CLASS));

   if (pd->bytes)
     {
        free(pd->bytes);
        pd->bytes = NULL;
        pd->allocated = 0;
        pd->position_read = 0;
        pd->position_write = 0;
     }
}

EOLIAN static Eina_Error
_efl_io_queue_efl_io_reader_read(Eo *o, Efl_Io_Queue_Data *pd, Eina_Rw_Slice *rw_slice)
{
   Eina_Slice ro_slice;
   size_t available;

   EINA_SAFETY_ON_NULL_RETURN_VAL(rw_slice, EINVAL);
   EINA_SAFETY_ON_TRUE_GOTO(efl_io_closer_closed_get(o), error);

   available = pd->position_write - pd->position_read;
   if (rw_slice->len > available)
     {
        rw_slice->len = available;
        if (rw_slice->len == 0)
          return EAGAIN;
     }

   ro_slice.len = rw_slice->len;
   ro_slice.mem = pd->bytes + pd->position_read;

   *rw_slice = eina_rw_slice_copy(*rw_slice, ro_slice);
   pd->position_read += ro_slice.len;

   efl_io_reader_can_read_set(o, pd->position_read < pd->position_write);
   efl_event_callback_call(o, EFL_IO_QUEUE_EVENT_SLICE_CHANGED, NULL);
   if (pd->closed) return 0;

   if ((pd->pending_eos) && (efl_io_queue_usage_get(o) == 0))
     efl_io_reader_eos_set(o, EINA_TRUE);

   return 0;

 error:
   rw_slice->len = 0;
   rw_slice->mem = NULL;
   efl_io_reader_can_read_set(o, EINA_FALSE);
   return EINVAL;
}

/**
 * @brief Discards a specified amount of data from the read end of the queue.
 *
 * This is similar to reading, but the data is simply dropped instead of copied.
 *
 * @param o The Efl_Io_Queue object.
 * @param pd The private data of the Efl_Io_Queue object.
 * @param amount The number of bytes to discard. If greater than available data, all available data is discarded.
 */
EOLIAN static void
_efl_io_queue_discard(Eo *o, Efl_Io_Queue_Data *pd, size_t amount)
{
   size_t available;

   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o));

   available = pd->position_write - pd->position_read;
   if (amount > available)
     {
        amount = available;
        if (amount == 0)
          return;
     }

   pd->position_read += amount;

   efl_io_reader_can_read_set(o, pd->position_read < pd->position_write);
   efl_event_callback_call(o, EFL_IO_QUEUE_EVENT_SLICE_CHANGED, NULL);
   if (pd->closed) return;

   if ((pd->pending_eos) && (efl_io_queue_usage_get(o) == 0))
     efl_io_reader_eos_set(o, EINA_TRUE);
}

/**
 * @brief Gets the cached can_read status.
 * @param o The Efl_Io_Queue object (unused).
 * @param pd The private data of the Efl_Io_Queue object.
 * @return EINA_TRUE if the queue might have data to read, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_io_queue_efl_io_reader_can_read_get(const Eo *o EINA_UNUSED, Efl_Io_Queue_Data *pd)
{
   return pd->can_read;
}

/**
 * @brief Sets the can_read status and emits an event if it changed.
 * @param o The Efl_Io_Queue object.
 * @param pd The private data of the Efl_Io_Queue object.
 * @param can_read The new can_read status.
 */
EOLIAN static void
_efl_io_queue_efl_io_reader_can_read_set(Eo *o, Efl_Io_Queue_Data *pd, Eina_Bool can_read)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o));
   if (pd->can_read == can_read) return;
   pd->can_read = can_read;
   efl_event_callback_call(o, EFL_IO_READER_EVENT_CAN_READ_CHANGED, &can_read);
}

/**
 * @brief Gets the EOS status.
 * @param o The Efl_Io_Queue object (unused).
 * @param pd The private data of the Efl_Io_Queue object.
 * @return EINA_TRUE if EOS has been signaled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_io_queue_efl_io_reader_eos_get(const Eo *o EINA_UNUSED, Efl_Io_Queue_Data *pd EINA_UNUSED)
{
   return pd->eos;
}

/**
 * @brief Sets the EOS status and emits an event if it's being set to true.
 * @param o The Efl_Io_Queue object.
 * @param pd The private data of the Efl_Io_Queue object.
 * @param is_eos The new EOS status.
 */
EOLIAN static void
_efl_io_queue_efl_io_reader_eos_set(Eo *o, Efl_Io_Queue_Data *pd EINA_UNUSED, Eina_Bool is_eos)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o));
   if (pd->eos == is_eos) return;
   pd->eos = is_eos;
   if (is_eos)
     {
        pd->pending_eos = EINA_FALSE;
        efl_event_callback_call(o, EFL_IO_READER_EVENT_EOS, NULL);
     }
}

EOLIAN static Eina_Error
_efl_io_queue_efl_io_writer_write(Eo *o, Efl_Io_Queue_Data *pd, Eina_Slice *slice, Eina_Slice *remaining)
{
   size_t available_write, available_total, todo, limit;
   int err = EINVAL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(slice, EINVAL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(slice->mem, EINVAL);
   EINA_SAFETY_ON_TRUE_GOTO(efl_io_closer_closed_get(o), error);

   err = EBADF;
   EINA_SAFETY_ON_TRUE_GOTO(pd->pending_eos, error);

   available_write = pd->allocated - pd->position_write;
   available_total = available_write + pd->position_read;
   limit = efl_io_queue_limit_get(o);

   err = ENOSPC;
   if (available_write >= slice->len)
     {
        todo = slice->len;
     }
   else if (available_total >= slice->len)
     {
        _efl_io_queue_adjust(pd);
        todo = slice->len;
     }
   else if ((limit > 0) && (pd->allocated == limit)) goto error;
   else
     {
        _efl_io_queue_adjust(pd);
        _efl_io_queue_realloc_rounded(o, pd, pd->position_write + slice->len);
        if (pd->allocated >= pd->position_write + slice->len)
          todo = slice->len;
        else
          todo = pd->allocated - pd->position_write;

        if (todo == 0) goto error;
     }

   memcpy(pd->bytes + pd->position_write, slice->mem, todo);
   if (remaining)
     {
        remaining->len = slice->len - todo;
        if (remaining->len)
          remaining->mem = slice->bytes + todo;
        else
          remaining->mem = NULL;
     }
   slice->len = todo;

   pd->position_write += todo;

   _efl_io_queue_adjust_and_realloc_if_needed(o, pd);
   efl_event_callback_call(o, EFL_IO_QUEUE_EVENT_SLICE_CHANGED, NULL);
   if (pd->closed) return 0;
   _efl_io_queue_update_cans(o, pd);

   return 0;

 error:
   if (remaining) *remaining = *slice;
   slice->len = 0;
   slice->mem = NULL;
   efl_io_writer_can_write_set(o, EINA_FALSE);
   return err;
}

/**
 * @brief Gets the cached can_write status.
 * @param o The Efl_Io_Queue object (unused).
 * @param pd The private data of the Efl_Io_Queue object.
 * @return EINA_TRUE if the queue can accept more data, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_io_queue_efl_io_writer_can_write_get(const Eo *o EINA_UNUSED, Efl_Io_Queue_Data *pd)
{
   return pd->can_write;
}

/**
 * @brief Sets the can_write status and emits an event if it changed.
 * @param o The Efl_Io_Queue object.
 * @param pd The private data of the Efl_Io_Queue object.
 * @param can_write The new can_write status.
 */
EOLIAN static void
_efl_io_queue_efl_io_writer_can_write_set(Eo *o, Efl_Io_Queue_Data *pd, Eina_Bool can_write)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o));
   if (pd->can_write == can_write) return;
   pd->can_write = can_write;
   efl_event_callback_call(o, EFL_IO_WRITER_EVENT_CAN_WRITE_CHANGED, &can_write);
}

/**
 * @brief Closes the Efl_Io_Queue.
 *
 * Marks EOS, clears any remaining data, sets the closed flag, and emits the CLOSED event.
 * After closing, no more reads or writes are possible.
 *
 * @param o The Efl_Io_Queue object.
 * @param pd The private data of the Efl_Io_Queue object.
 * @return 0 on success, or an error code (EINVAL if already closed).
 */
EOLIAN static Eina_Error
_efl_io_queue_efl_io_closer_close(Eo *o, Efl_Io_Queue_Data *pd)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_io_closer_closed_get(o), EINVAL);
   efl_io_queue_eos_mark(o);
   efl_io_queue_clear(o);
   pd->closed = EINA_TRUE;
   efl_event_callback_call(o, EFL_IO_CLOSER_EVENT_CLOSED, NULL);
   return 0;
}

/**
 * @brief Gets the closed status of the queue.
 * @param o The Efl_Io_Queue object (unused).
 * @param pd The private data of the Efl_Io_Queue object.
 * @return EINA_TRUE if closed, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_io_queue_efl_io_closer_closed_get(const Eo *o EINA_UNUSED, Efl_Io_Queue_Data *pd)
{
   return pd->closed;
}

/**
 * @brief Sets the close_on_exec property.
 * For Efl_Io_Queue, this is a no-op that always effectively behaves as true,
 * as it's an in-memory queue not tied to a system file descriptor.
 * @param o The Efl_Io_Queue object (unused).
 * @param pd The private data of the Efl_Io_Queue object (unused).
 * @param close_on_exec If EINA_TRUE, it's a request to close on exec.
 * @return EINA_TRUE if close_on_exec is EINA_TRUE, EINA_FALSE otherwise.
 *         Effectively, it can only be "set" to true.
 */
EOLIAN static Eina_Bool
_efl_io_queue_efl_io_closer_close_on_exec_set(Eo *o EINA_UNUSED, Efl_Io_Queue_Data *pd EINA_UNUSED, Eina_Bool close_on_exec)
{
   if (!close_on_exec) return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @brief Gets the close_on_exec property.
 * For Efl_Io_Queue, this always returns EINA_TRUE.
 * @param o The Efl_Io_Queue object (unused).
 * @param pd The private data of the Efl_Io_Queue object (unused).
 * @return Always EINA_TRUE.
 */
EOLIAN static Eina_Bool
_efl_io_queue_efl_io_closer_close_on_exec_get(const Eo *o EINA_UNUSED, Efl_Io_Queue_Data *pd EINA_UNUSED)
{
   return EINA_TRUE;
}

/**
 * @brief Sets the close_on_invalidate property.
 * For Efl_Io_Queue, this is a no-op as it's always closed on invalidate (destruction).
 * @param o The Efl_Io_Queue object (unused).
 * @param pd The private data of the Efl_Io_Queue object (unused).
 * @param close_on_invalidate Unused.
 */
EOLIAN static void
_efl_io_queue_efl_io_closer_close_on_invalidate_set(Eo *o EINA_UNUSED, Efl_Io_Queue_Data *pd EINA_UNUSED, Eina_Bool close_on_invalidate EINA_UNUSED)
{
}

/**
 * @brief Gets the close_on_invalidate property.
 * For Efl_Io_Queue, this always returns EINA_TRUE.
 * @param o The Efl_Io_Queue object (unused).
 * @param pd The private data of the Efl_Io_Queue object (unused).
 * @return Always EINA_TRUE.
 */
EOLIAN static Eina_Bool
_efl_io_queue_efl_io_closer_close_on_invalidate_get(const Eo *o EINA_UNUSED, Efl_Io_Queue_Data *pd EINA_UNUSED)
{
   return EINA_TRUE;
}

#include "interfaces/efl_io_queue.eo.c"
