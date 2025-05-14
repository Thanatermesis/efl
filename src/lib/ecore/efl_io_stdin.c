#define EFL_IO_READER_PROTECTED 1
#define EFL_IO_READER_FD_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"

#define MY_CLASS EFL_IO_STDIN_CLASS

/**
 * @internal
 * @brief Event callback for when the STDIN file descriptor has data to read.
 *
 * This function is called when the event EFL_LOOP_FD_EVENT_READ is triggered.
 * It sets the 'can_read' property to EINA_TRUE, indicating that data is
 * available for reading, and 'eos' to EINA_FALSE.
 *
 * @param data User data, unused in this callback.
 * @param event The event information.
 */
static void
_efl_io_stdin_event_read(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_io_reader_can_read_set(event->object, EINA_TRUE);
   efl_io_reader_eos_set(event->object, EINA_FALSE);
}

/**
 * @internal
 * @brief Event callback for when an error occurs on the STDIN file descriptor.
 *
 * This function is called when the event EFL_LOOP_FD_EVENT_ERROR is triggered.
 * It sets the 'can_read' property to EINA_FALSE and 'eos' (end-of-stream)
 * to EINA_TRUE, indicating an error state or that the stream has ended.
 *
 * @param data User data, unused in this callback.
 * @param event The event information.
 */
static void
_efl_io_stdin_event_error(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_io_reader_can_read_set(event->object, EINA_FALSE);
   efl_io_reader_eos_set(event->object, EINA_TRUE);
}

/**
 * @internal
 * @brief Sets the file descriptor for the Efl.Io.Stdin object.
 *
 * This function overrides the efl_loop_fd_set method. It sets the fd
 * for the underlying loop handler and also for the reader interface.
 *
 * @param o The Efl_Io_Stdin object.
 * @param pd Private data, unused.
 * @param fd The file descriptor to set. Typically STDIN_FILENO.
 */
EOLIAN static void
_efl_io_stdin_efl_loop_fd_fd_set(Eo *o, void *pd EINA_UNUSED, int fd)
{
   efl_loop_fd_file_set(efl_super(o, MY_CLASS), fd);
   efl_io_reader_fd_set(o, fd);
}

/**
 * @internal
 * @brief Finalizes the Efl_Io_Stdin object.
 *
 * This function is part of the Efl.Object lifecycle. It ensures that
 * a valid file descriptor (STDIN_FILENO by default) is set if none was
 * provided. It then calls the parent's finalize method and sets up
 * event callbacks for read and error events on the file descriptor.
 *
 * @param o The Efl_Io_Stdin object to finalize.
 * @param pd Private data, unused.
 * @return The finalized Efl_Object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_io_stdin_efl_object_finalize(Eo *o, void *pd EINA_UNUSED)
{
   int fd = efl_loop_fd_get(o);
   if (fd < 0) efl_loop_fd_set(o, STDIN_FILENO);

   o = efl_finalize(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   efl_event_callback_add(o, EFL_LOOP_FD_EVENT_READ, _efl_io_stdin_event_read, NULL);
   efl_event_callback_add(o, EFL_LOOP_FD_EVENT_ERROR, _efl_io_stdin_event_error, NULL);
   return o;
}

/**
 * @internal
 * @brief Reads data from the STDIN file descriptor.
 *
 * This function overrides the efl_io_reader_read method. It performs the
 * actual read operation using the parent class's implementation. If data
 * is successfully read (rw_slice->len > 0), it sets 'can_read' to EINA_FALSE
 * to indicate that the current data has been consumed and it should wait for
 * the EFL_LOOP_FD_EVENT_READ event before attempting another read.
 *
 * @param o The Efl_Io_Stdin object.
 * @param pd Private data, unused.
 * @param rw_slice A pointer to an Eina_Rw_Slice to store the read data.
 *                 The 'len' field will be updated with the number of bytes read.
 *                 Example:
 *                 Eina_Rw_Slice slice = { .mem = buffer, .len = sizeof(buffer) };
 *                 efl_io_reader_read(stdin_obj, &slice);
 *                 // slice.len now contains the number of bytes read into buffer.
 * @return EINA_ERROR_NONE on success, or an Eina_Error code on failure.
 */
EOLIAN static Eina_Error
_efl_io_stdin_efl_io_reader_read(Eo *o, void *pd EINA_UNUSED, Eina_Rw_Slice *rw_slice)
{
   Eina_Error ret;

   ret = efl_io_reader_read(efl_super(o, MY_CLASS), rw_slice);
   if (rw_slice && rw_slice->len > 0)
     efl_io_reader_can_read_set(o, EINA_FALSE); /* wait Efl.Loop.Fd "read" */

   return ret;
}

/**
 * @internal
 * @brief Sets the 'can_read' property and manages FD event monitoring.
 *
 * This function overrides the efl_io_reader_can_read_set method.
 * When 'can_read' is set to EINA_TRUE (meaning data is available or an event
 * indicated readiness), it stops monitoring the EFL_LOOP_FD_EVENT_READ event.
 * This is because the user is expected to call efl_io_reader_read() which will
 * consume the available data.
 * When 'can_read' is set to EINA_FALSE (typically after a read operation or
 * if no data is available), it resumes monitoring the EFL_LOOP_FD_EVENT_READ
 * event to be notified when new data arrives.
 *
 * @param o The Efl_Io_Stdin object.
 * @param pd Private data, unused.
 * @param value The boolean value to set for 'can_read'. EINA_TRUE if data can be read,
 *              EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_io_stdin_efl_io_reader_can_read_set(Eo *o, void *pd EINA_UNUSED, Eina_Bool value)
{
   Eina_Bool old = efl_io_reader_can_read_get(o);
   if (old == value) return;

   efl_io_reader_can_read_set(efl_super(o, MY_CLASS), value);

   if (value)
     {
        /* stop monitoring the FD, we need to wait the user to read and clear the kernel flag */
        efl_event_callback_del(o, EFL_LOOP_FD_EVENT_READ, _efl_io_stdin_event_read, NULL);
     }
   else
     {
        /* kernel flag is clear, resume monitoring the FD */
        efl_event_callback_add(o, EFL_LOOP_FD_EVENT_READ, _efl_io_stdin_event_read, NULL);
     }
}

#include "efl_io_stdin.eo.c"
