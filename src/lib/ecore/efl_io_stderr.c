#define EFL_IO_WRITER_FD_PROTECTED 1
#define EFL_IO_WRITER_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"

#define MY_CLASS EFL_IO_STDERR_CLASS

/**
 * @brief Event handler for the EFL_LOOP_FD_EVENT_WRITE event.
 *
 * This function is called when the file descriptor associated with the Efl.Io.Stderr
 * object becomes writable. It updates the 'can_write' property to EINA_TRUE.
 *
 * @param data User data, unused in this handler.
 * @param event The event information.
 */
static void
_efl_io_stderr_event_write(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_io_writer_can_write_set(event->object, EINA_TRUE);
}

/**
 * @brief Event handler for the EFL_LOOP_FD_EVENT_ERROR event.
 *
 * This function is called when an error occurs on the file descriptor
 * associated with the Efl.Io.Stderr object. It updates the 'can_write'
 * property to EINA_FALSE.
 *
 * @param data User data, unused in this handler.
 * @param event The event information.
 */
static void
_efl_io_stderr_event_error(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_io_writer_can_write_set(event->object, EINA_FALSE);
}

/**
 * @brief Sets the file descriptor for the Efl.Io.Stderr object.
 *
 * This function sets the file descriptor for both the underlying Efl.Loop.Fd
 * and the Efl.Io.Writer interfaces.
 *
 * @param o The Efl.Io.Stderr object.
 * @param pd Private data, unused.
 * @param fd The file descriptor to set.
 */
EOLIAN static void
_efl_io_stderr_efl_loop_fd_fd_set(Eo *o, void *pd EINA_UNUSED, int fd)
{
   efl_loop_fd_file_set(efl_super(o, MY_CLASS), fd);
   efl_io_writer_fd_set(o, fd);
}

/**
 * @brief Finalizes the Efl.Io.Stderr object.
 *
 * This function performs final setup for the Efl.Io.Stderr object.
 * If no file descriptor has been set, it defaults to STDIN_FILENO (which is
 * incorrect for stderr, but likely a placeholder or error in the original logic,
 * as stderr should be STDOUT_FILENO or STDERR_FILENO. For Efl.Io.Stderr, it should be STDERR_FILENO).
 * It then finalizes the superclass and registers event callbacks for
 * write and error events on the file descriptor.
 *
 * @param o The Efl.Io.Stderr object to finalize.
 * @param pd Private data, unused.
 * @return The finalized Efl.Io.Stderr object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_io_stderr_efl_object_finalize(Eo *o, void *pd EINA_UNUSED)
{
   int fd = efl_loop_fd_get(o);
   // FIXME: Defaulting to STDIN_FILENO for stderr seems incorrect.
   // It should likely be STDERR_FILENO.
   if (fd < 0) efl_loop_fd_set(o, STDIN_FILENO);

   o = efl_finalize(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   efl_event_callback_add(o, EFL_LOOP_FD_EVENT_WRITE, _efl_io_stderr_event_write, NULL);
   efl_event_callback_add(o, EFL_LOOP_FD_EVENT_ERROR, _efl_io_stderr_event_error, NULL);

   return o;
}

/**
 * @brief Writes data to the Efl.Io.Stderr object.
 *
 * This function attempts to write the data in @p ro_slice to the underlying
 * file descriptor. If the write is partial or would block, the 'can_write'
 * property is set to EINA_FALSE, and the caller should wait for the
 * EFL_LOOP_FD_EVENT_WRITE event before attempting to write again.
 *
 * @param o The Efl.Io.Stderr object.
 * @param pd Private data, unused.
 * @param ro_slice The slice of data to write.
 * @param remaining A slice that will be updated to reflect any data that
 *                  could not be written. Can be NULL if not needed.
 * @return 0 on success, or an Eina_Error code on failure.
 */
EOLIAN static Eina_Error
_efl_io_stderr_efl_io_writer_write(Eo *o, void *pd EINA_UNUSED, Eina_Slice *ro_slice, Eina_Slice *remaining)
{
   Eina_Error ret;

   ret = efl_io_writer_write(efl_super(o, MY_CLASS), ro_slice, remaining);
   if (ro_slice && ro_slice->len > 0)
     efl_io_writer_can_write_set(o, EINA_FALSE); /* wait Efl.Loop.Fd "write" */

   return ret;
}

/**
 * @brief Sets the 'can_write' property for the Efl.Io.Stderr object.
 *
 * This function updates the 'can_write' property and manages monitoring
 * of the EFL_LOOP_FD_EVENT_WRITE event.
 * If 'can_write' is set to EINA_TRUE (meaning the fd is ready for writing),
 * monitoring for the write event is stopped because the user is expected to
 * perform a write, which will clear the kernel's ready-to-write flag.
 * If 'can_write' is set to EINA_FALSE (meaning a write would block or failed),
 * monitoring for the write event is started (or resumed) to be notified
 * when the fd becomes writable again.
 *
 * @param o The Efl.Io.Stderr object.
 * @param pd Private data, unused.
 * @param value The new value for the 'can_write' property.
 */
EOLIAN static void
_efl_io_stderr_efl_io_writer_can_write_set(Eo *o, void *pd EINA_UNUSED, Eina_Bool value)
{
   Eina_Bool old = efl_io_writer_can_write_get(o);
   if (old == value) return;

   efl_io_writer_can_write_set(efl_super(o, MY_CLASS), value);

   if (value)
     {
        /* stop monitoring the FD, we need to wait the user to write and clear the kernel flag */
        efl_event_callback_del(o, EFL_LOOP_FD_EVENT_WRITE, _efl_io_stderr_event_write, NULL);
     }
   else
     {
        /* kernel flag is clear, resume monitoring the FD */
        efl_event_callback_add(o, EFL_LOOP_FD_EVENT_WRITE, _efl_io_stderr_event_write, NULL);
     }
}

#include "efl_io_stderr.eo.c"
