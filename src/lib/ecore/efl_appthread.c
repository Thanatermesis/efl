#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define EFL_IO_READER_PROTECTED 1
#define EFL_IO_WRITER_PROTECTED 1
#define EFL_IO_CLOSER_PROTECTED 1

#include <Ecore.h>

#include "ecore_private.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define MY_CLASS EFL_APPTHREAD_CLASS

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

/**
 * @internal
 * @brief Constructor for the Efl_Appthread object.
 *
 * Initializes the file descriptors and control pipe descriptors to -1,
 * and sets the initial can_write state to EINA_TRUE.
 *
 * @param obj The Efl_Appthread object.
 * @param pd The private data for the Efl_Appthread object.
 * @return The constructed Efl_Appthread object.
 */
EOLIAN static Efl_Object *
_efl_appthread_efl_object_constructor(Eo *obj, Efl_Appthread_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   pd->fd.in = -1;
   pd->fd.out = -1;
   pd->fd.can_write = EINA_TRUE;
   pd->ctrl.in = -1;
   pd->ctrl.out = -1;
   return obj;
}

/**
 * @internal
 * @brief Destructor for the Efl_Appthread object.
 *
 * Closes all open file descriptors and control pipe descriptors.
 * It also nullifies the handlers associated with these descriptors.
 *
 * @param obj The Efl_Appthread object.
 * @param pd The private data for the Efl_Appthread object.
 */
EOLIAN static void
_efl_appthread_efl_object_destructor(Eo *obj, Efl_Appthread_Data *pd)
{
   if (pd->fd.in >= 0)
     {
//        efl_del(pd->fd.in_handler);
//        efl_del(pd->fd.out_handler);
//        efl_del(pd->ctrl.in_handler);
//        efl_del(pd->ctrl.out_handler);
        close(pd->fd.in);
        close(pd->fd.out);
        close(pd->ctrl.in);
        close(pd->ctrl.out);
        pd->fd.in_handler = NULL;
        pd->fd.out_handler = NULL;
        pd->ctrl.in_handler = NULL;
        pd->ctrl.out_handler = NULL;
        pd->fd.in = -1;
        pd->fd.out = -1;
        pd->ctrl.in = -1;
        pd->ctrl.out = -1;
     }
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Implements Efl.Io.Closer.close.
 *
 * Closes the I/O streams associated with the appthread.
 * Sets can_write and can_read to EINA_FALSE, and eos to EINA_TRUE.
 * Closes the input and output file descriptors and deletes their handlers.
 *
 * @param obj The Efl_Appthread object.
 * @param pd The private data for the Efl_Appthread object.
 * @return 0 on success, or an error code on failure. EBADF if already closed.
 */
EOLIAN static Eina_Error
_efl_appthread_efl_io_closer_close(Eo *obj, Efl_Appthread_Data *pd)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_io_closer_closed_get(obj), EBADF);
   efl_io_writer_can_write_set(obj, EINA_FALSE);
   efl_io_reader_can_read_set(obj, EINA_FALSE);
   efl_io_reader_eos_set(obj, EINA_TRUE);
   if (pd->fd.in >= 0) close(pd->fd.in);
   if (pd->fd.out >= 0) close(pd->fd.out);
   if (pd->fd.in_handler) efl_del(pd->fd.in_handler);
   if (pd->fd.out_handler) efl_del(pd->fd.out_handler);
   pd->fd.in = -1;
   pd->fd.out = -1;
   pd->fd.in_handler = NULL;
   pd->fd.out_handler = NULL;
   return 0;
}

/**
 * @internal
 * @brief Implements Efl.Io.Closer.closed_get.
 *
 * Checks if the I/O streams are closed.
 *
 * @param obj The Efl_Appthread object (unused).
 * @param pd The private data for the Efl_Appthread object.
 * @return EINA_TRUE if both input and output file descriptors are -1, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_appthread_efl_io_closer_closed_get(const Eo *obj EINA_UNUSED, Efl_Appthread_Data *pd)
{
   if ((pd->fd.in == -1) && (pd->fd.out == -1)) return EINA_TRUE;
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Implements Efl.Io.Reader.read.
 *
 * Reads data from the appthread's output file descriptor.
 * If the read operation results in 0 bytes read (EOF), it sets can_read to EINA_FALSE,
 * eos to EINA_TRUE, closes the output fd, and cleans up its handler.
 *
 * @param obj The Efl_Appthread object.
 * @param pd The private data for the Efl_Appthread object.
 * @param rw_slice The slice to read data into. Its len field will be updated with the number of bytes read.
 * @return 0 on success. EPIPE if EOF is reached. EINVAL on other errors or if fd is invalid.
 */
EOLIAN static Eina_Error
_efl_appthread_efl_io_reader_read(Eo *obj, Efl_Appthread_Data *pd, Eina_Rw_Slice *rw_slice)
{
   ssize_t r;

   errno = 0;
   if (pd->fd.out == -1) goto err;

   do
     {
        errno = 0;
        r = read(pd->fd.out, rw_slice->mem, rw_slice->len);
        if (r == -1)
          {
             if (errno == EINTR) continue;
             goto err;
          }
     }
   while (r == -1);

   rw_slice->len = r;
   if (r == 0)
     {
        efl_io_reader_can_read_set(obj, EINA_FALSE);
        efl_io_reader_eos_set(obj, EINA_TRUE);
        close(pd->fd.out);
        pd->fd.out = -1;
        efl_del(pd->fd.out_handler);
        pd->fd.out_handler = NULL;
        return EPIPE;
     }
   return 0;
err:
   if ((pd->fd.out != -1) && (errno != EAGAIN))
     {
        close(pd->fd.out);
        pd->fd.out = -1;
        efl_del(pd->fd.out_handler);
        pd->fd.out_handler = NULL;
     }
   rw_slice->len = 0;
   rw_slice->mem = NULL;
   efl_io_reader_can_read_set(obj, EINA_FALSE);
   return EINVAL;
}

/**
 * @internal
 * @brief Implements Efl.Io.Reader.can_read_set.
 *
 * Sets the can_read status and adjusts the activity of the input handler accordingly.
 * Emits the EFL_IO_READER_EVENT_CAN_READ_CHANGED event if the status changes.
 *
 * @param obj The Efl_Appthread object.
 * @param pd The private data for the Efl_Appthread object.
 * @param can_read The new can_read status.
 */
EOLIAN static void
_efl_appthread_efl_io_reader_can_read_set(Eo *obj, Efl_Appthread_Data *pd, Eina_Bool can_read)
{
   Eina_Bool old = efl_io_reader_can_read_get(obj);
   if (old == can_read) return;
   pd->fd.can_read = can_read;
   if (can_read)
     efl_loop_handler_active_set(pd->fd.in_handler, 0);
   else
     efl_loop_handler_active_set(pd->fd.in_handler,
                                 EFL_LOOP_HANDLER_FLAGS_READ);
   efl_event_callback_call(obj, EFL_IO_READER_EVENT_CAN_READ_CHANGED, &can_read);
}

/**
 * @internal
 * @brief Implements Efl.Io.Reader.can_read_get.
 *
 * Gets the current can_read status.
 *
 * @param obj The Efl_Appthread object (unused).
 * @param pd The private data for the Efl_Appthread object.
 * @return The current can_read status.
 */
EOLIAN static Eina_Bool
_efl_appthread_efl_io_reader_can_read_get(const Eo *obj EINA_UNUSED, Efl_Appthread_Data *pd)
{
   return pd->fd.can_read;
}

/**
 * @internal
 * @brief Implements Efl.Io.Reader.eos_set.
 *
 * Sets the end-of-stream (EOS) status for reading.
 * If EOS is reached, it deactivates the output handler and emits the EFL_IO_READER_EVENT_EOS event.
 *
 * @param obj The Efl_Appthread object.
 * @param pd The private data for the Efl_Appthread object.
 * @param is_eos The new EOS status.
 */
EOLIAN static void
_efl_appthread_efl_io_reader_eos_set(Eo *obj, Efl_Appthread_Data *pd, Eina_Bool is_eos)
{
   Eina_Bool old = efl_io_reader_eos_get(obj);
   if (old == is_eos) return;

   pd->fd.eos_read = is_eos;
   if (!is_eos) return;
   if (pd->fd.out_handler)
     efl_loop_handler_active_set(pd->fd.out_handler, 0);
   efl_event_callback_call(obj, EFL_IO_READER_EVENT_EOS, NULL);
}

/**
 * @internal
 * @brief Implements Efl.Io.Reader.eos_get.
 *
 * Gets the current end-of-stream (EOS) status for reading.
 *
 * @param obj The Efl_Appthread object (unused).
 * @param pd The private data for the Efl_Appthread object.
 * @return The current EOS status for reading.
 */
EOLIAN static Eina_Bool
_efl_appthread_efl_io_reader_eos_get(const Eo *obj EINA_UNUSED, Efl_Appthread_Data *pd)
{
   return pd->fd.eos_read;
}

/**
 * @internal
 * @brief Implements Efl.Io.Writer.write.
 *
 * Writes data to the appthread's input file descriptor.
 * If the write operation results in 0 bytes written (which usually indicates an issue or closed pipe),
 * it closes the input fd and cleans up its handler.
 * If any data is written, can_write is set to EINA_FALSE.
 *
 * @param obj The Efl_Appthread object.
 * @param pd The private data for the Efl_Appthread object.
 * @param slice The slice of data to write. Its len field will be updated with the number of bytes written.
 * @param remaining If not NULL, this slice will be updated to reflect any data that was not written.
 *                  Example: If slice is {mem="abc", len=3} and 1 byte is written,
 *                           slice becomes {mem="abc", len=1} and
 *                           remaining becomes {mem="bc", len=2}.
 * @return 0 on success. EPIPE if the pipe is closed. EINVAL on other errors or if fd is invalid.
 */
EOLIAN static Eina_Error
_efl_appthread_efl_io_writer_write(Eo *obj, Efl_Appthread_Data *pd, Eina_Slice *slice, Eina_Slice *remaining)
{
   ssize_t r;

   errno = 0;
   if (pd->fd.in == -1) goto err;
   if (!slice) return EINVAL;

   do
     {
        errno = 0;
        r = write(pd->fd.in, slice->mem, slice->len);
        if (r == -1)
          {
             if (errno == EINTR) continue;
             goto err;
          }
     }
   while (r == -1);

   if (remaining)
     {
        remaining->len = slice->len - r;
        remaining->bytes = slice->bytes + r;
     }
   slice->len = r;

   if (slice->len > 0)
     efl_io_writer_can_write_set(obj, EINA_FALSE);
   if (r == 0)
     {
        close(pd->fd.in);
        pd->fd.in = -1;
        efl_del(pd->fd.in_handler);
        pd->fd.in_handler = NULL;
        return EPIPE;
     }
   return 0;
err:
   if ((pd->fd.in != -1) && (errno != EAGAIN))
     {
        close(pd->fd.in);
        pd->fd.in = -1;
        efl_del(pd->fd.in_handler);
        pd->fd.in_handler = NULL;
     }
   if (remaining) *remaining = *slice;
   slice->len = 0;
   slice->mem = NULL;
   efl_io_writer_can_write_set(obj, EINA_FALSE);
   return EINVAL;
}

/**
 * @internal
 * @brief Implements Efl.Io.Writer.can_write_set.
 *
 * Sets the can_write status and adjusts the activity of the input handler accordingly.
 * Emits the EFL_IO_WRITER_EVENT_CAN_WRITE_CHANGED event if the status changes.
 *
 * @param obj The Efl_Appthread object.
 * @param pd The private data for the Efl_Appthread object.
 * @param can_write The new can_write status.
 */
EOLIAN static void
_efl_appthread_efl_io_writer_can_write_set(Eo *obj, Efl_Appthread_Data *pd, Eina_Bool can_write)
{
   Eina_Bool old = efl_io_writer_can_write_get(obj);
   if (old == can_write) return;
   pd->fd.can_write = can_write;
   if (can_write)
     efl_loop_handler_active_set(pd->fd.in_handler, 0);
   else
     efl_loop_handler_active_set(pd->fd.in_handler,
                                 EFL_LOOP_HANDLER_FLAGS_WRITE);
   efl_event_callback_call(obj, EFL_IO_WRITER_EVENT_CAN_WRITE_CHANGED, &can_write);
}

/**
 * @internal
 * @brief Implements Efl.Io.Writer.can_write_get.
 *
 * Gets the current can_write status.
 *
 * @param obj The Efl_Appthread object (unused).
 * @param pd The private data for the Efl_Appthread object.
 * @return The current can_write status.
 */
EOLIAN static Eina_Bool
_efl_appthread_efl_io_writer_can_write_get(const Eo *obj EINA_UNUSED, Efl_Appthread_Data *pd)
{
   return pd->fd.can_write;
}

/**
 * @internal
 * @brief Forwarding function for asynchronous calls to the appthread's context.
 *
 * This is the Eolian exposed function that wraps the internal implementation.
 *
 * @param obj The Efl_Appthread object.
 * @param pd The private data for the Efl_Appthread object.
 * @param func_data User data to be passed to the callback function.
 * @param func The callback function to execute in the appthread's context.
 *             Example: `void my_callback(void *data, Eo *thread_obj, Efl_Io_Closer *io) { ... }`
 * @param func_free_cb Optional callback to free func_data when the call is completed or cancelled.
 */
void _appthread_threadio_call(Eo *obj, Efl_Appthread_Data *pd, void *func_data, EflThreadIOCall func, Eina_Free_Cb func_free_cb);

EOLIAN static void
_efl_appthread_efl_threadio_call(Eo *obj, Efl_Appthread_Data *pd, void *func_data, EflThreadIOCall func, Eina_Free_Cb func_free_cb)
{
   _appthread_threadio_call(obj, pd, func_data, func, func_free_cb);
}

/**
 * @internal
 * @brief Forwarding function for synchronous calls to the appthread's context.
 *
 * This is the Eolian exposed function that wraps the internal implementation.
 *
 * @param obj The Efl_Appthread object.
 * @param pd The private data for the Efl_Appthread object.
 * @param func_data User data to be passed to the callback function.
 * @param func The callback function to execute in the appthread's context.
 *             Example: `void *my_sync_callback(void *data, Eo *thread_obj, Efl_Io_Closer *io) { return result_data; }`
 * @param func_free_cb Optional callback to free func_data when the call is completed.
 * @return The value returned by the synchronous callback function `func`.
 */
void *_appthread_threadio_call_sync(Eo *obj, Efl_Appthread_Data *pd, void *func_data, EflThreadIOCallSync func, Eina_Free_Cb func_free_cb);

EOLIAN static void *
_efl_appthread_efl_threadio_call_sync(Eo *obj, Efl_Appthread_Data *pd, void *func_data, EflThreadIOCallSync func, Eina_Free_Cb func_free_cb)
{
   return _appthread_threadio_call_sync(obj, pd, func_data, func, func_free_cb);
}

//////////////////////////////////////////////////////////////////////////

#include "efl_appthread.eo.c"
