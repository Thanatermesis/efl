#define EFL_IO_WRITER_FD_PROTECTED 1
#define EFL_IO_WRITER_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"

#define MY_CLASS EFL_IO_STDOUT_CLASS

/**
 * @brief Event callback for when the FD is writable.
 *
 * This function is called when the underlying file descriptor associated with
 * the Efl.Io.Stdout object becomes writable. It updates the 'can_write'
 * property of the writer.
 *
 * @param data User data, unused in this callback.
 * @param event The event information.
 */
static void
_efl_io_stdout_event_write(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_io_writer_can_write_set(event->object, EINA_TRUE);
}

/**
 * @brief Event callback for an error on the FD.
 *
 * This function is called when an error occurs on the underlying file
 * descriptor associated with the Efl.Io.Stdout object. It updates the
 * 'can_write' property of the writer to EINA_FALSE.
 *
 * @param data User data, unused in this callback.
 * @param event The event information.
 */
static void
_efl_io_stdout_event_error(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_io_writer_can_write_set(event->object, EINA_FALSE);
}

/**
 * @brief Sets the file descriptor for the Efl.Io.Stdout object.
 *
 * This function sets the underlying file descriptor for this stdout object.
 * It also propagates this fd to the parent Efl.Loop.Fd and the Efl.Io.Writer.
 *
 * @param o The Efl.Io.Stdout object.
 * @param pd Private data, unused.
 * @param fd The file descriptor to set.
 */
EOLIAN static void
_efl_io_stdout_efl_loop_fd_fd_set(Eo *o, void *pd EINA_UNUSED, int fd)
{
   efl_loop_fd_file_set(efl_super(o, MY_CLASS), fd);
   efl_io_writer_fd_set(o, fd);
}

/**
 * @brief Finalizes the Efl.Io.Stdout object.
 *
 * This function is called when the object is being finalized.
 * It ensures that a valid file descriptor (STDOUT_FILENO by default) is set
 * if none was provided. It then finalizes the parent class and sets up
 * event callbacks for FD write and error events.
 *
 * @param o The Efl.Io.Stdout object.
 * @param pd Private data, unused.
 * @return The finalized Efl_Object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_io_stdout_efl_object_finalize(Eo *o, void *pd EINA_UNUSED)
{
   int fd = efl_loop_fd_get(o);
   if (fd < 0) efl_loop_fd_set(o, STDOUT_FILENO);

   o = efl_finalize(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   efl_event_callback_add(o, EFL_LOOP_FD_EVENT_WRITE, _efl_io_stdout_event_write, NULL);
   efl_event_callback_add(o, EFL_LOOP_FD_EVENT_ERROR, _efl_io_stdout_event_error, NULL);
   return o;
}

/**
 * @brief Writes data to the Efl.Io.Stdout object.
 *
 * This function attempts to write the data provided in @p ro_slice to the
 * underlying file descriptor. If the write is partial or cannot be completed
 * immediately (e.g., due to the FD not being ready), it sets the 'can_write'
 * property to EINA_FALSE and relies on the Efl.Loop.Fd "write" event to
 * signal when writing can resume.
 *
 * @param o The Efl.Io.Stdout object.
 * @param pd Private data, unused.
 * @param ro_slice The slice of data to write.
 * @param remaining A slice that will be updated to indicate any data that
 *                  could not be written. Can be NULL if not needed.
 * @return 0 on success, or an Eina_Error code on failure.
 */
EOLIAN static Eina_Error
_efl_io_stdout_efl_io_writer_write(Eo *o, void *pd EINA_UNUSED, Eina_Slice *ro_slice, Eina_Slice *remaining)
{
   Eina_Error ret;

   ret = efl_io_writer_write(efl_super(o, MY_CLASS), ro_slice, remaining);
   if (ro_slice && ro_slice->len > 0)
     efl_io_writer_can_write_set(o, EINA_FALSE); /* wait Efl.Loop.Fd "write" */

   return ret;
}

/**
 * @brief Sets the 'can_write' property for the Efl.Io.Stdout object.
 *
 * This function updates the 'can_write' state of the writer.
 * If 'can_write' becomes EINA_TRUE, it means the user can attempt to write data,
 * so monitoring for the FD write event is stopped.
 * If 'can_write' becomes EINA_FALSE, it means a previous write was incomplete
 * or the FD is not ready, so monitoring for the FD write event is resumed.
 *
 * @param o The Efl.Io.Stdout object.
 * @param pd Private data, unused.
 * @param value The new boolean value for 'can_write'.
 */
EOLIAN static void
_efl_io_stdout_efl_io_writer_can_write_set(Eo *o, void *pd EINA_UNUSED, Eina_Bool value)
{
   Eina_Bool old = efl_io_writer_can_write_get(o);
   if (old == value) return;

   efl_io_writer_can_write_set(efl_super(o, MY_CLASS), value);

   if (value)
     {
        /* stop monitoring the FD, we need to wait the user to write and clear the kernel flag */
        efl_event_callback_del(o, EFL_LOOP_FD_EVENT_WRITE, _efl_io_stdout_event_write, NULL);
     }
   else
     {
        /* kernel flag is clear, resume monitoring the FD */
        efl_event_callback_add(o, EFL_LOOP_FD_EVENT_WRITE, _efl_io_stdout_event_write, NULL);
     }
}

#include "efl_io_stdout.eo.c"
