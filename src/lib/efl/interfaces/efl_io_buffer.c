/**
 * @file
 * @brief This file implements the Efl.Io.Buffer interface, providing an in-memory I/O buffer.
 */

#define EFL_IO_READER_PROTECTED 1
#define EFL_IO_WRITER_PROTECTED 1

#include "config.h"
#include "Efl.h"

#define MY_CLASS EFL_IO_BUFFER_CLASS

/**
 * @brief Private data structure for Efl_Io_Buffer.
 */
typedef struct _Efl_Io_Buffer_Data
{
   uint8_t *bytes;            /**< Pointer to the allocated memory buffer. */
   size_t allocated;          /**< Current allocated size of the buffer in bytes. */
   size_t used;               /**< Current used size of the buffer in bytes (Efl.Io.Sizer.size). */
   size_t limit;              /**< Maximum allowed size for the buffer (0 for no limit). */
   size_t position_read;      /**< Current read position in the buffer. */
   size_t position_write;     /**< Current write position in the buffer. */
   Eina_Bool closed;          /**< Flag indicating if the buffer is closed (Efl.Io.Closer.closed). */
   Eina_Bool can_read;        /**< Flag indicating if the buffer can be read from (Efl.Io.Reader.can_read). */
   Eina_Bool can_write;       /**< Flag indicating if the buffer can be written to (Efl.Io.Writer.can_write). */
   Eina_Bool readonly;        /**< Flag indicating if the buffer is in read-only mode. */
} Efl_Io_Buffer_Data;

/**
 * @internal
 * @brief Reallocates the internal buffer to the specified size.
 *
 * This function handles the actual memory reallocation. It considers the
 * buffer's limit and updates positions if the new size is smaller than
 * the current used size.
 *
 * @param o The Efl_Io_Buffer object.
 * @param pd The private data of the Efl_Io_Buffer object.
 * @param size The new desired size for the buffer.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure or if no reallocation was needed.
 */
static Eina_Bool
_efl_io_buffer_realloc(Eo *o, Efl_Io_Buffer_Data *pd, size_t size)
{
   void *tmp;
   size_t limit = efl_io_buffer_limit_get(o);

   EINA_SAFETY_ON_TRUE_RETURN_VAL(pd->readonly, EINA_FALSE);

   if ((limit > 0) && (size > limit))
     size = limit;

   if (pd->allocated == size) return EINA_FALSE;

   if (efl_io_sizer_size_get(o) > size)
     {
        if (efl_io_buffer_position_read_get(o) > size)
          efl_io_buffer_position_read_set(o, size);
        if (efl_io_buffer_position_write_get(o) > size)
          efl_io_buffer_position_write_set(o, size);

        /* no efl_io_sizer_size_set() since it could recurse! */
        pd->used = size;
        efl_event_callback_call(o, EFL_IO_SIZER_EVENT_SIZE_CHANGED, NULL);
     }

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
   efl_event_callback_call(o, EFL_IO_BUFFER_EVENT_REALLOCATED, NULL);
   return EINA_TRUE;
   efl_event_callback_call(o, EFL_IO_BUFFER_EVENT_REALLOCATED, NULL);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Reallocates the internal buffer to a rounded-up size.
 *
 * This function calculates a new buffer size by rounding up the requested
 * 'size' to predefined steps (32, 128, 1024, 4096 bytes). This can help
 * reduce the frequency of reallocations by allocating slightly more memory
 * than immediately needed.
 *
 * @param o The Efl_Io_Buffer object.
 * @param pd The private data of the Efl_Io_Buffer object.
 * @param size The desired minimum size for the buffer.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure or if no reallocation was needed.
 */
static Eina_Bool
_efl_io_buffer_realloc_rounded(Eo *o, Efl_Io_Buffer_Data *pd, size_t size)
{
   if ((size > 0) && (size < 128))
     size = ((size / 32) + 1) * 32;
   else if (size < 1024)
     size = ((size / 128) + 1) * 128;
   else if (size < 8192)
     size = ((size / 1024) + 1) * 1024;
   else
     size = ((size / 4096) + 1) * 4096;

   return _efl_io_buffer_realloc(o, pd, size);
}

EOLIAN static void
_efl_io_buffer_preallocate(Eo *o, Efl_Io_Buffer_Data *pd, size_t size)
{
   /**
    * @brief Ensures the buffer has at least 'size' bytes allocated.
    * If the current allocated size is less than 'size', it reallocates
    * the buffer using a rounded-up size to potentially optimize future
    * allocations. This operation is ignored if the buffer is read-only
    * or closed.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @param size The minimum desired allocated capacity in bytes.
    */
   EINA_SAFETY_ON_TRUE_RETURN(pd->readonly);
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o));
   if (pd->allocated < size)
     _efl_io_buffer_realloc_rounded(o, pd, size);
}

EOLIAN static void
_efl_io_buffer_limit_set(Eo *o, Efl_Io_Buffer_Data *pd, size_t limit)
{
   /**
    * @brief Sets the maximum size (limit) for the buffer.
    * If the new limit is smaller than the currently allocated size, the buffer
    * is reallocated to match the new limit. A limit of 0 means no limit.
    * This also updates the `can_write` status based on the new limit and
    * current write position.
    * This operation is ignored if the buffer is read-only or closed.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @param limit The new maximum size for the buffer in bytes. 0 for unlimited.
    */
   EINA_SAFETY_ON_TRUE_RETURN(pd->readonly);
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o));

   if (pd->limit == limit) return;
   pd->limit = limit;

   if ((limit > 0) && (pd->allocated > limit))
     _efl_io_buffer_realloc(o, pd, limit);

   if (pd->closed) return;

   efl_io_reader_can_read_set(o, efl_io_buffer_position_read_get(o) < efl_io_sizer_size_get(o));
   if (pd->closed) return;
   efl_io_writer_can_write_set(o, (limit == 0) ||
                               (efl_io_buffer_position_write_get(o) < limit));
}

EOLIAN static size_t
_efl_io_buffer_limit_get(const Eo *o EINA_UNUSED, Efl_Io_Buffer_Data *pd)
{
   /**
    * @brief Gets the maximum size (limit) for the buffer.
    *
    * @param o The Efl_Io_Buffer object (unused).
    * @param pd The private data of the Efl_Io_Buffer object.
    * @return The current maximum size for the buffer in bytes. 0 if unlimited.
    */
   return pd->limit;
}

EOLIAN static Eina_Slice
_efl_io_buffer_slice_get(const Eo *o, Efl_Io_Buffer_Data *pd)
{
   /**
    * @brief Gets a direct, read-only slice of the buffer's used memory.
    * The slice represents the current content of the buffer up to its
    * `used` size.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @return An Eina_Slice representing the buffer's content.
    *         The slice will have `len = 0` and `mem = NULL` if the buffer is closed.
    *         Example: `{ .mem = "data", .len = 4 }`
    */
   Eina_Slice slice = { };

   if (!efl_io_closer_closed_get(o))
     {
        slice.mem = pd->bytes;
        slice.len = efl_io_sizer_size_get(o);
     }

   return slice;
}

EOLIAN static Eina_Binbuf *
_efl_io_buffer_binbuf_steal(Eo *o, Efl_Io_Buffer_Data *pd)
{
   /**
    * @brief Transfers ownership of the internal buffer to a new Eina_Binbuf.
    * After this operation, the Efl_Io_Buffer object becomes empty (size 0,
    * no allocated memory). The caller is responsible for freeing the returned
    * Eina_Binbuf. This operation is not allowed if the buffer is read-only or closed.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @return A new Eina_Binbuf containing the stolen buffer data, or @c NULL on failure.
    */
   Eina_Binbuf *ret;
   EINA_SAFETY_ON_TRUE_RETURN_VAL(pd->readonly, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_io_closer_closed_get(o), NULL);

   ret = eina_binbuf_manage_new(pd->bytes, efl_io_sizer_size_get(o), EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ret, NULL);

   pd->bytes = NULL;
   pd->allocated = 0;
   efl_event_callback_call(o, EFL_IO_BUFFER_EVENT_REALLOCATED, NULL);
   efl_io_sizer_resize(o, 0);

   return ret;
}

EOLIAN static Efl_Object *
_efl_io_buffer_efl_object_finalize(Eo *o, Efl_Io_Buffer_Data *pd EINA_UNUSED)
{
   /**
    * @brief Finalizes the Efl_Io_Buffer object.
    * This is part of the Efl_Object lifecycle. It ensures that the `can_read`
    * and `can_write` states are correctly set after the object is fully
    * constructed.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object (unused here but part of signature).
    * @return The finalized Efl_Object, or @c NULL on failure.
    */
   size_t limit;

   o = efl_finalize(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   efl_io_reader_can_read_set(o, efl_io_buffer_position_read_get(o) < efl_io_sizer_size_get(o));
   if (pd->closed) return o;

   limit = efl_io_buffer_limit_get(o);
   efl_io_writer_can_write_set(o, (limit == 0) ||
                               (efl_io_buffer_position_write_get(o) < limit));

   return o;
}

EOLIAN static void
_efl_io_buffer_efl_object_destructor(Eo *o, Efl_Io_Buffer_Data *pd)
{
   /**
    * @brief Destroys the Efl_Io_Buffer object.
    * This is part of the Efl_Object lifecycle. It closes the buffer if it's
    * not already closed and frees any allocated memory.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    */
   if (!efl_io_closer_closed_get(o))
     {
        efl_event_freeze(o);
        efl_io_closer_close(o);
        efl_event_thaw(o);
     }

   efl_destructor(efl_super(o, MY_CLASS));

   if (pd->bytes)
     {
        if (!pd->readonly) free(pd->bytes);
        pd->bytes = NULL;
        pd->allocated = 0;
        pd->used = 0;
        pd->position_read = 0;
        pd->position_write = 0;
     }
}

EOLIAN static Eina_Error
_efl_io_buffer_efl_io_reader_read(Eo *o, Efl_Io_Buffer_Data *pd, Eina_Rw_Slice *rw_slice)
{
   /**
    * @brief Implements Efl.Io.Reader.read.
    * Reads data from the buffer into the provided `rw_slice`.
    * The amount of data read is limited by the `rw_slice->len` and the
    * available data in the buffer from the current read position.
    * The read position is advanced by the number of bytes read.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @param rw_slice Input/Output slice.
    *                 Input: `rw_slice->mem` is the destination buffer, `rw_slice->len` is the maximum bytes to read.
    *                 Output: `rw_slice->len` is updated to the actual number of bytes read.
    *                 Example (Input): `{ .mem = my_buffer, .len = 10 }`
    *                 Example (Output after reading 5 bytes): `{ .mem = my_buffer, .len = 5 }`
    * @return 0 on success, or an Eina_Error code on failure (e.g., EINVAL if closed, EAGAIN if no data to read).
    */
   Eina_Slice ro_slice;
   size_t used, read_pos, available;

   EINA_SAFETY_ON_NULL_RETURN_VAL(rw_slice, EINVAL);
   EINA_SAFETY_ON_TRUE_GOTO(efl_io_closer_closed_get(o), error);

   used = efl_io_sizer_size_get(o);
   read_pos = efl_io_buffer_position_read_get(o);
   available = used - read_pos;

   if (rw_slice->len > available)
     {
        rw_slice->len = available;
        if (rw_slice->len == 0)
          return EAGAIN;
     }

   ro_slice.len = rw_slice->len;
   ro_slice.mem = pd->bytes + read_pos;

   *rw_slice = eina_rw_slice_copy(*rw_slice, ro_slice);
   efl_io_buffer_position_read_set(o, read_pos + ro_slice.len);

   return 0;

 error:
   rw_slice->len = 0;
   rw_slice->mem = NULL;
   efl_io_reader_can_read_set(o, EINA_FALSE);
   return EINVAL;
}

EOLIAN static Eina_Bool
_efl_io_buffer_efl_io_reader_can_read_get(const Eo *o EINA_UNUSED, Efl_Io_Buffer_Data *pd)
{
   /**
    * @brief Implements Efl.Io.Reader.can_read_get.
    *
    * @param o The Efl_Io_Buffer object (unused).
    * @param pd The private data of the Efl_Io_Buffer object.
    * @return @c EINA_TRUE if the buffer can be read from, @c EINA_FALSE otherwise.
    */
   return pd->can_read;
}

EOLIAN static void
_efl_io_buffer_efl_io_reader_can_read_set(Eo *o, Efl_Io_Buffer_Data *pd, Eina_Bool can_read)
{
   /**
    * @brief Implements Efl.Io.Reader.can_read_set.
    * Sets the `can_read` flag and emits the `can_read,changed` event if the state changes.
    * This is typically managed internally based on buffer state (e.g., data availability, closed status).
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @param can_read The new can_read state.
    */
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o));
   if (pd->can_read == can_read) return;
   pd->can_read = can_read;
   efl_event_callback_call(o, EFL_IO_READER_EVENT_CAN_READ_CHANGED, &can_read);
}

EOLIAN static Eina_Bool
_efl_io_buffer_efl_io_reader_eos_get(const Eo *o, Efl_Io_Buffer_Data *pd EINA_UNUSED)
{
   /**
    * @brief Implements Efl.Io.Reader.eos_get (End Of Stream).
    * Checks if the end of the stream (buffer) has been reached for reading.
    * This is true if the buffer is closed or the read position is at or beyond the used size.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object (unused).
    * @return @c EINA_TRUE if EOS is reached, @c EINA_FALSE otherwise.
    */
   return efl_io_closer_closed_get(o) ||
     efl_io_buffer_position_read_get(o) >= efl_io_sizer_size_get(o);
}

EOLIAN static void
_efl_io_buffer_efl_io_reader_eos_set(Eo *o, Efl_Io_Buffer_Data *pd EINA_UNUSED, Eina_Bool is_eos)
{
   /**
    * @brief Implements Efl.Io.Reader.eos_set.
    * If `is_eos` is true, this function emits the `eos` event.
    * This is typically called internally when the EOS condition is met.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object (unused).
    * @param is_eos If @c EINA_TRUE, signals that EOS has been reached.
    */
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o));
   if (is_eos)
     efl_event_callback_call(o, EFL_IO_READER_EVENT_EOS, NULL);
}

EOLIAN static Eina_Error
_efl_io_buffer_efl_io_writer_write(Eo *o, Efl_Io_Buffer_Data *pd, Eina_Slice *slice, Eina_Slice *remaining)
{
   /**
    * @brief Implements Efl.Io.Writer.write.
    * Writes data from the provided `slice` into the buffer at the current write position.
    * The buffer may be reallocated if there isn't enough space. If a `limit` is set
    * and reached, writing may be partial or fail.
    * The write position is advanced by the number of bytes written.
    * The `used` size of the buffer is updated if new data extends beyond it.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @param slice Input/Output slice containing data to write.
    *              Input: `slice->mem` is the data source, `slice->len` is the number of bytes to write.
    *              Output: `slice->len` is updated to the actual number of bytes written.
    *              Example (Input): `{ .mem = "hello", .len = 5 }`
    *              Example (Output after writing 3 bytes due to limit): `{ .len = 3 }`
    * @param remaining Optional output slice. If not NULL, it will be populated with
    *                  the portion of the input `slice` that was not written.
    *                  Example (if 2 bytes of "hello" remained): `{ .mem = "lo", .len = 2 }`
    * @return 0 on success, or an Eina_Error code on failure (e.g., EPERM if read-only,
    *         EINVAL if closed, ENOSPC if buffer limit reached and no space).
    */
   size_t available, todo, write_pos, limit;
   int err = EINVAL;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(pd->readonly, EPERM);
   EINA_SAFETY_ON_NULL_RETURN_VAL(slice, EINVAL);
   EINA_SAFETY_ON_TRUE_GOTO(efl_io_closer_closed_get(o), error);

   write_pos = efl_io_buffer_position_write_get(o);
   available = pd->allocated - write_pos;
   limit = efl_io_buffer_limit_get(o);

   err = ENOSPC;
   if (available >= slice->len)
     todo = slice->len;
   else if ((limit > 0) && (pd->allocated == limit)) goto error;
   else
     {
        _efl_io_buffer_realloc_rounded(o, pd, write_pos + slice->len);
        if (pd->allocated >= write_pos + slice->len)
          todo = slice->len;
        else
          todo = pd->allocated - write_pos;

        if (todo == 0) goto error;
     }

   memcpy(pd->bytes + write_pos, slice->mem, todo);
   if (remaining)
     {
        remaining->len = slice->len - todo;
        if (remaining->len)
          remaining->mem = slice->bytes + todo;
        else
          remaining->mem = NULL;
     }
   slice->len = todo;

   if (pd->used < write_pos + todo)
     {
        pd->used = write_pos + todo;
        efl_event_callback_call(o, EFL_IO_SIZER_EVENT_SIZE_CHANGED, NULL);
        if (pd->closed) return 0;
        efl_io_reader_can_read_set(o, pd->position_read < pd->used);
        if (pd->closed) return 0;
     }
   efl_io_buffer_position_write_set(o, write_pos + todo);

   return 0;

 error:
   if (remaining) *remaining = *slice;
   slice->len = 0;
   slice->mem = NULL;
   efl_io_writer_can_write_set(o, EINA_FALSE);
   return err;
}

EOLIAN static Eina_Bool
_efl_io_buffer_efl_io_writer_can_write_get(const Eo *o EINA_UNUSED, Efl_Io_Buffer_Data *pd)
{
   /**
    * @brief Implements Efl.Io.Writer.can_write_get.
    *
    * @param o The Efl_Io_Buffer object (unused).
    * @param pd The private data of the Efl_Io_Buffer object.
    * @return @c EINA_TRUE if the buffer can be written to, @c EINA_FALSE otherwise.
    */
   return pd->can_write;
}

EOLIAN static void
_efl_io_buffer_efl_io_writer_can_write_set(Eo *o, Efl_Io_Buffer_Data *pd, Eina_Bool can_write)
{
   /**
    * @brief Implements Efl.Io.Writer.can_write_set.
    * Sets the `can_write` flag and emits the `can_write,changed` event if the state changes.
    * This is typically managed internally based on buffer state (e.g., buffer limit, closed status, read-only status).
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @param can_write The new can_write state.
    */
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o));
   if (pd->can_write == can_write) return;
   pd->can_write = can_write;
   efl_event_callback_call(o, EFL_IO_WRITER_EVENT_CAN_WRITE_CHANGED, &can_write);
}

EOLIAN static Eina_Error
_efl_io_buffer_efl_io_closer_close(Eo *o, Efl_Io_Buffer_Data *pd)
{
   /**
    * @brief Implements Efl.Io.Closer.close.
    * Closes the buffer. This typically means resizing the used data to 0
    * and marking the buffer as closed, preventing further reads/writes.
    * Emits the `closed` event.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @return 0 on success, or EINVAL if already closed.
    */
   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_io_closer_closed_get(o), EINVAL);
   efl_io_sizer_resize(o, 0);
   pd->closed = EINA_TRUE;
   efl_event_callback_call(o, EFL_IO_CLOSER_EVENT_CLOSED, NULL);
   return 0;
}

EOLIAN static Eina_Bool
_efl_io_buffer_efl_io_closer_closed_get(const Eo *o EINA_UNUSED, Efl_Io_Buffer_Data *pd)
{
   /**
    * @brief Implements Efl.Io.Closer.closed_get.
    *
    * @param o The Efl_Io_Buffer object (unused).
    * @param pd The private data of the Efl_Io_Buffer object.
    * @return @c EINA_TRUE if the buffer is closed, @c EINA_FALSE otherwise.
    */
   return pd->closed;
}

EOLIAN static Eina_Bool
_efl_io_buffer_efl_io_closer_close_on_exec_set(Eo *o EINA_UNUSED, Efl_Io_Buffer_Data *pd EINA_UNUSED, Eina_Bool close_on_exec)
{
   /**
    * @brief Implements Efl.Io.Closer.close_on_exec_set.
    * For an in-memory buffer, this concept doesn't directly apply as it does for file descriptors.
    * This implementation effectively treats it as always true if `close_on_exec` is true,
    * and returns false if an attempt is made to set it to false, indicating it cannot be disabled.
    *
    * @param o The Efl_Io_Buffer object (unused).
    * @param pd The private data of the Efl_Io_Buffer object (unused).
    * @param close_on_exec The desired state.
    * @return @c EINA_TRUE if `close_on_exec` is true, @c EINA_FALSE if `close_on_exec` is false (as it cannot be unset).
    */
   if (!close_on_exec) return EINA_FALSE;

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_io_buffer_efl_io_closer_close_on_exec_get(const Eo *o EINA_UNUSED, Efl_Io_Buffer_Data *pd EINA_UNUSED)
{
   /**
    * @brief Implements Efl.Io.Closer.close_on_exec_get.
    * For an in-memory buffer, this is always considered true.
    *
    * @param o The Efl_Io_Buffer object (unused).
    * @param pd The private data of the Efl_Io_Buffer object (unused).
    * @return Always @c EINA_TRUE.
    */
   return EINA_TRUE;
}

EOLIAN static void
_efl_io_buffer_efl_io_closer_close_on_invalidate_set(Eo *o EINA_UNUSED, Efl_Io_Buffer_Data *pd EINA_UNUSED, Eina_Bool close_on_invalidate EINA_UNUSED)
{
   /**
    * @brief Implements Efl.Io.Closer.close_on_invalidate_set.
    * This is a no-op for Efl_Io_Buffer as its lifecycle is tied to the object itself.
    *
    * @param o The Efl_Io_Buffer object (unused).
    * @param pd The private data of the Efl_Io_Buffer object (unused).
    * @param close_on_invalidate The desired state (unused).
    */
}

EOLIAN static Eina_Bool
_efl_io_buffer_efl_io_closer_close_on_invalidate_get(const Eo *o EINA_UNUSED, Efl_Io_Buffer_Data *pd EINA_UNUSED)
{
   /**
    * @brief Implements Efl.Io.Closer.close_on_invalidate_get.
    * Always returns @c EINA_TRUE, indicating the buffer effectively "closes" when invalidated.
    *
    * @param o The Efl_Io_Buffer object (unused).
    * @param pd The private data of the Efl_Io_Buffer object (unused).
    * @return Always @c EINA_TRUE.
    */
   return EINA_TRUE;
}

EOLIAN static Eina_Error
_efl_io_buffer_efl_io_sizer_resize(Eo *o, Efl_Io_Buffer_Data *pd, uint64_t size)
{
   /**
    * @brief Implements Efl.Io.Sizer.resize.
    * Changes the `used` size of the buffer.
    * If the new size is larger than the current allocated capacity, the buffer
    * is reallocated (using rounded-up allocation). If reallocation fails to
    * meet the requested size (e.g., due to a `limit`), the `used` size is set
    * to the new allocated capacity, and ENOSPC is returned.
    * If the new size is larger than the old `used` size, the new area is zeroed.
    * Read and write positions are adjusted if they fall outside the new `used` size.
    * `can_read` and `can_write` states are updated accordingly.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @param size The new desired `used` size for the buffer in bytes.
    * @return 0 on success, EINVAL if closed, EPERM if read-only and trying to expand,
    *         or ENOSPC if the buffer could not be grown to the requested size (e.g. due to limit).
    */
   Eina_Error ret = 0;
   Eina_Bool reallocated = EINA_FALSE;
   size_t old_size, pos_read, pos_write;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_io_closer_closed_get(o), EINVAL);

   if (efl_io_sizer_size_get(o) == size) return 0;

   if (pd->readonly)
     {
        EINA_SAFETY_ON_TRUE_RETURN_VAL(size > pd->used, EPERM);
        pd->used = size;
        goto end;
     }

   old_size = pd->used;
   pd->used = size;

   efl_event_freeze(o);
   reallocated = _efl_io_buffer_realloc_rounded(o, pd, size);
   efl_event_thaw(o);

   if (size > pd->allocated)
     {
        pd->used = size = pd->allocated;
        ret = ENOSPC;
     }

   if (old_size < size)
     memset(pd->bytes + old_size, 0, size - old_size);

 end:
   pos_read = efl_io_buffer_position_read_get(o);
   if (pos_read > size)
     efl_io_buffer_position_read_set(o, size);
   else
     efl_io_reader_can_read_set(o, pos_read < size);

   if (pd->closed) return 0;

   pos_write = efl_io_buffer_position_write_get(o);
   if (pos_write > size)
     efl_io_buffer_position_write_set(o, size);
   else
     {
        size_t limit = efl_io_buffer_limit_get(o);
        efl_io_writer_can_write_set(o, (limit == 0) || (pos_write < limit));
        if (pd->closed) return 0;
     }

   efl_event_callback_call(o, EFL_IO_SIZER_EVENT_SIZE_CHANGED, NULL);
   if (reallocated)
     efl_event_callback_call(o, EFL_IO_BUFFER_EVENT_REALLOCATED, NULL);

   return ret;
}

EOLIAN static uint64_t
_efl_io_buffer_efl_io_sizer_size_get(const Eo *o EINA_UNUSED, Efl_Io_Buffer_Data *pd)
{
   /**
    * @brief Implements Efl.Io.Sizer.size_get.
    * Gets the current `used` size of the buffer.
    *
    * @param o The Efl_Io_Buffer object (unused).
    * @param pd The private data of the Efl_Io_Buffer object.
    * @return The current `used` size of the buffer in bytes.
    */
   return pd->used;
}

EOLIAN static Eina_Error
_efl_io_buffer_efl_io_positioner_seek(Eo *o, Efl_Io_Buffer_Data *pd EINA_UNUSED, int64_t offset, Efl_Io_Positioner_Whence whence)
{
   /**
    * @brief Implements Efl.Io.Positioner.seek.
    * Sets both the read and write positions within the buffer.
    * The `offset` is interpreted based on the `whence` parameter:
    * - `EFL_IO_POSITIONER_WHENCE_START`: Seek from the beginning of the buffer.
    * - `EFL_IO_POSITIONER_WHENCE_CURRENT`: Seek from the current position (maximum of read/write positions).
    * - `EFL_IO_POSITIONER_WHENCE_END`: Seek from the end of the used data in the buffer.
    * The final position must be within the bounds of the current `used` size of the buffer.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object (unused).
    * @param offset The offset to seek to.
    * @param whence The reference point for the seek (EFL_IO_POSITIONER_WHENCE_START,
    *               EFL_IO_POSITIONER_WHENCE_CURRENT, or EFL_IO_POSITIONER_WHENCE_END).
    * @return 0 on success, or EINVAL if closed, whence is invalid, or the target offset is out of bounds.
    */
   size_t size;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_io_closer_closed_get(o), EINVAL);

   size = efl_io_sizer_size_get(o);

   if (whence == EFL_IO_POSITIONER_WHENCE_CURRENT)
     {
        whence = EFL_IO_POSITIONER_WHENCE_START;
        offset += efl_io_positioner_position_get(o);
     }
   else if (whence == EFL_IO_POSITIONER_WHENCE_END)
     {
        whence = EFL_IO_POSITIONER_WHENCE_START;
        offset += size;
     }

   EINA_SAFETY_ON_TRUE_RETURN_VAL(whence != EFL_IO_POSITIONER_WHENCE_START, EINVAL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(offset < 0, EINVAL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((size_t)offset > size, EINVAL);

   efl_io_buffer_position_read_set(o, offset);
   efl_io_buffer_position_write_set(o, offset);

   return 0;
}

EOLIAN static uint64_t
_efl_io_buffer_efl_io_positioner_position_get(const Eo *o, Efl_Io_Buffer_Data *pd EINA_UNUSED)
{
   /**
    * @brief Implements Efl.Io.Positioner.position_get.
    * Gets the "current" position, defined as the greater of the read and write positions.
    * This behavior is chosen because `seek` sets both positions, but read/write operations
    * only affect their respective individual positions. This ensures that `position_get`
    * reflects a meaningful overall position, especially when the buffer is used primarily
    * for either reading or writing after a seek.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object (unused).
    * @return The current position in the buffer (max of read and write positions).
    */
   uint64_t r = efl_io_buffer_position_read_get(o);
   uint64_t w = efl_io_buffer_position_write_get(o);
   /* if using Efl.Io.Positioner.position, on set it will do both
    * read/write to the same offset, however on Efl.Io.Reader.read it
    * will only update position_read (and similarly for
    * Efl.Io.Writer), thus on the next position.get we want the
    * greatest position.
    *
    * This allows the buffer to be used solely as reader or writer
    * without the need to know it have two internal offsets.
    */
   if (r >= w)
     return r;
   return w;
}

EOLIAN static Eina_Bool
_efl_io_buffer_position_read_set(Eo *o, Efl_Io_Buffer_Data *pd, uint64_t position)
{
   /**
    * @brief Sets the read position within the buffer.
    * The position must be within the current `used` size of the buffer.
    * Updates `can_read` and `eos` status accordingly. Emits `position_read,changed`
    * and `Efl.Io.Positioner.position,changed` events if applicable.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @param position The new read position.
    * @return @c EINA_TRUE on success, @c EINA_FALSE if closed or position is out of bounds.
    */
   size_t size;
   Eina_Bool changed;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_io_closer_closed_get(o), EINA_FALSE);

   size = efl_io_sizer_size_get(o);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(position > size, EINA_FALSE);

   if (pd->position_read == position) return EINA_TRUE;

   changed = efl_io_positioner_position_get(o) != position;

   pd->position_read = position;
   efl_event_callback_call(o, EFL_IO_BUFFER_EVENT_POSITION_READ_CHANGED, NULL);
   if (changed)
     efl_event_callback_call(o, EFL_IO_POSITIONER_EVENT_POSITION_CHANGED, NULL);

   efl_io_reader_can_read_set(o, position < size);
   if (pd->closed) return EINA_TRUE;
   efl_io_reader_eos_set(o, position >= size);
   return EINA_TRUE;
}

EOLIAN static uint64_t
_efl_io_buffer_position_read_get(const Eo *o EINA_UNUSED, Efl_Io_Buffer_Data *pd)
{
   /**
    * @brief Gets the current read position within the buffer.
    *
    * @param o The Efl_Io_Buffer object (unused).
    * @param pd The private data of the Efl_Io_Buffer object.
    * @return The current read position.
    */
   return pd->position_read;
}

EOLIAN static Eina_Bool
_efl_io_buffer_position_write_set(Eo *o, Efl_Io_Buffer_Data *pd, uint64_t position)
{
   /**
    * @brief Sets the write position within the buffer.
    * The position must be within the current `used` size of the buffer.
    * Cannot set write position if buffer is read-only and position is less than current size.
    * Updates `can_write` status accordingly. Emits `position_write,changed`
    * and `Efl.Io.Positioner.position,changed` events if applicable.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @param position The new write position.
    * @return @c EINA_TRUE on success, @c EINA_FALSE if closed, read-only violation, or position is out of bounds.
    */
   size_t size;
   size_t limit;
   Eina_Bool changed;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_io_closer_closed_get(o), EINA_FALSE);

   size = efl_io_sizer_size_get(o);
   if (position < size)
     EINA_SAFETY_ON_TRUE_RETURN_VAL(pd->readonly, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(position > size, EINA_FALSE);

   if (pd->position_write == position) return EINA_TRUE;

   changed = efl_io_positioner_position_get(o) != position;

   pd->position_write = position;
   efl_event_callback_call(o, EFL_IO_BUFFER_EVENT_POSITION_WRITE_CHANGED, NULL);
   if (changed)
     efl_event_callback_call(o, EFL_IO_POSITIONER_EVENT_POSITION_CHANGED, NULL);

   if (pd->closed) return 0;

   limit = efl_io_buffer_limit_get(o);
   efl_io_writer_can_write_set(o, (limit == 0) || (position < limit));
   return EINA_TRUE;
}

EOLIAN static uint64_t
_efl_io_buffer_position_write_get(const Eo *o EINA_UNUSED, Efl_Io_Buffer_Data *pd)
{
   /**
    * @brief Gets the current write position within the buffer.
    *
    * @param o The Efl_Io_Buffer object (unused).
    * @param pd The private data of the Efl_Io_Buffer object.
    * @return The current write position.
    */
   return pd->position_write;
}

EOLIAN static void
_efl_io_buffer_adopt_readonly(Eo *o, Efl_Io_Buffer_Data *pd, const Eina_Slice slice)
{
   /**
    * @brief Adopts an external, read-only memory slice.
    * The buffer takes ownership of the provided `slice`'s memory region for reading.
    * The buffer becomes read-only. Any previously managed memory is freed (if not already read-only).
    * The `used` size and `allocated` size are set to `slice.len`.
    * `can_write` is set to false. Read/write positions are adjusted if they exceed the new size.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @param slice The read-only slice to adopt. Its memory must remain valid for the lifetime
    *              of its use by the buffer. The buffer does not copy the data.
    *              Example: `{ .bytes = "external_data", .len = 13 }`
    */
   Eina_Bool changed_size;

   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o));

   if (!pd->readonly) free(pd->bytes);
   pd->readonly = EINA_TRUE;
   pd->bytes = (uint8_t *)slice.bytes;
   pd->allocated = slice.len;

   changed_size = (pd->used != slice.len);
   pd->used = slice.len;
   efl_io_writer_can_write_set(o, EINA_FALSE);
   if (pd->closed) return;

   if (efl_io_buffer_position_read_get(o) > slice.len)
     {
        efl_io_buffer_position_read_set(o, slice.len);
        if (pd->closed) return;
     }

   efl_io_buffer_position_write_set(o, slice.len);
   if (pd->closed) return;

   if (changed_size)
     {
        efl_event_callback_call(o, EFL_IO_SIZER_EVENT_SIZE_CHANGED, NULL);
        if (pd->closed) return;
     }

   efl_event_callback_call(o, EFL_IO_BUFFER_EVENT_REALLOCATED, NULL);
}

EOLIAN static void
_efl_io_buffer_adopt_readwrite(Eo *o, Efl_Io_Buffer_Data *pd, Eina_Rw_Slice slice)
{
   /**
    * @brief Adopts an external, read-write memory slice.
    * The buffer takes ownership of the provided `slice`'s memory region for reading and writing.
    * The buffer becomes read-write (if it was read-only, it's changed).
    * Any previously managed memory is freed (if not already read-only).
    * The `used` size and `allocated` size are set to `slice.len`.
    * `can_write` is updated based on the buffer's limit. Read/write positions are adjusted
    * if they exceed the new size.
    *
    * @param o The Efl_Io_Buffer object.
    * @param pd The private data of the Efl_Io_Buffer object.
    * @param slice The read-write slice to adopt. Its memory must remain valid for the lifetime
    *              of its use by the buffer. The buffer does not copy the data initially but may
    *              reallocate it later if writes exceed `slice.len`.
    *              Example: `{ .bytes = my_mutable_buffer, .len = 100 }`
    */
   Eina_Bool changed_size;

   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o));

   if (!pd->readonly) free(pd->bytes);
   pd->readonly = EINA_FALSE;
   pd->bytes = slice.bytes;
   pd->allocated = slice.len;

   changed_size = (pd->used != slice.len);
   pd->used = slice.len;

   efl_io_writer_can_write_set(o, (pd->limit == 0) ||
                               (efl_io_buffer_position_write_get(o) < pd->limit));
   if (pd->closed) return;

   if (efl_io_buffer_position_read_get(o) > slice.len)
     {
        efl_io_buffer_position_read_set(o, slice.len);
        if (pd->closed) return;
     }

   if (efl_io_buffer_position_write_get(o) > slice.len)
     {
        efl_io_buffer_position_write_set(o, slice.len);
        if (pd->closed) return;
     }

   if (changed_size)
     {
        efl_event_callback_call(o, EFL_IO_SIZER_EVENT_SIZE_CHANGED, NULL);
        if (pd->closed) return;
     }

   efl_event_callback_call(o, EFL_IO_BUFFER_EVENT_REALLOCATED, NULL);
}

#include "interfaces/efl_io_buffer.eo.c"
