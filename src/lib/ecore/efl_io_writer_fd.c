#define EFL_IO_WRITER_PROTECTED 1
#define EFL_IO_WRITER_FD_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"

#define MY_CLASS EFL_IO_WRITER_FD_CLASS

/**
 * @brief Private data for the Efl_Io_Writer_Fd class.
 */
typedef struct _Efl_Io_Writer_Fd_Data
{
   int fd; /**< The file descriptor to write to. */
   Eina_Bool can_write; /**< Flag indicating if writing is possible. @c EINA_TRUE if writing is possible, @c EINA_FALSE otherwise. */
} Efl_Io_Writer_Fd_Data;

/**
 * @brief Sets the file descriptor for this writer.
 *
 * @param o The Efl_Io_Writer_Fd object.
 * @param pd The private data for the Efl_Io_Writer_Fd object.
 * @param fd The file descriptor to set.
 */
EOLIAN static void
_efl_io_writer_fd_writer_fd_set(Eo *o EINA_UNUSED, Efl_Io_Writer_Fd_Data *pd, int fd)
{
   pd->fd = fd;
}

/**
 * @brief Gets the file descriptor for this writer.
 *
 * @param o The Efl_Io_Writer_Fd object.
 * @param pd The private data for the Efl_Io_Writer_Fd object.
 * @return The file descriptor.
 */
EOLIAN static int
_efl_io_writer_fd_writer_fd_get(const Eo *o EINA_UNUSED, Efl_Io_Writer_Fd_Data *pd)
{
   return pd->fd;
}

/**
 * @brief Writes data to the file descriptor.
 *
 * This function attempts to write the data in @p ro_slice to the file
 * descriptor associated with the Efl_Io_Writer_Fd object.
 *
 * If the write operation is interrupted by a signal (EINTR), it will be retried.
 * If any other error occurs during the write, the function will return the
 * error code and set the 'can_write' flag to @c EINA_FALSE.
 *
 * If @p remaining is not @c NULL, it will be updated to reflect the portion
 * of @p ro_slice that was not written.
 *
 * @param o The Efl_Io_Writer_Fd object.
 * @param pd The private data for the Efl_Io_Writer_Fd object.
 * @param ro_slice A pointer to an Eina_Slice containing the data to write.
 *                 The slice's mem and len fields will be updated to reflect
 *                 the data written.
 *                 Example:
 *                 Given ro_slice = { .mem = "ABCDEFG", .len = 7 }
 *                 If 3 bytes are written, ro_slice becomes { .mem = "ABC", .len = 3 }
 * @param remaining A pointer to an Eina_Slice that will be filled with the
 *                  data that could not be written. Can be @c NULL.
 *                  Example:
 *                  Given ro_slice = { .mem = "ABCDEFG", .len = 7 }
 *                  If 3 bytes are written, remaining becomes { .mem = "DEFG", .len = 4 }
 * @return 0 on success, or an errno code on failure. EINVAL if fd is invalid or ro_slice is NULL.
 */
EOLIAN static Eina_Error
_efl_io_writer_fd_efl_io_writer_write(Eo *o, Efl_Io_Writer_Fd_Data *pd EINA_UNUSED, Eina_Slice *ro_slice, Eina_Slice *remaining)
{
   int fd = efl_io_writer_fd_get(o);
   ssize_t r;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ro_slice, EINVAL);
   if (fd < 0) goto error;

   do
     {
        r = write(fd, ro_slice->mem, ro_slice->len);
        if (r < 0)
          {
             if (errno == EINTR) continue;

             if (remaining) *remaining = *ro_slice;
             ro_slice->len = 0;
             ro_slice->mem = NULL;
             efl_io_writer_can_write_set(o, EINA_FALSE);
             return errno;
          }
     }
   while (r < 0);

   if (remaining)
     {
        remaining->len = ro_slice->len - r;
        remaining->bytes = ro_slice->bytes + r;
     }
   ro_slice->len = r;
   if (r == 0) efl_io_writer_can_write_set(o, EINA_FALSE);
   return 0;

 error:
   if (remaining) *remaining = *ro_slice;
   ro_slice->len = 0;
   ro_slice->mem = NULL;
   efl_io_writer_can_write_set(o, EINA_FALSE);
   return EINVAL;

}

/**
 * @brief Gets the 'can_write' status of the writer.
 *
 * This indicates if the writer is currently able to write more data.
 * It is typically set to @c EINA_FALSE after a write error or when a
 * non-blocking write would block.
 *
 * @param o The Efl_Io_Writer_Fd object.
 * @param pd The private data for the Efl_Io_Writer_Fd object.
 * @return @c EINA_TRUE if the writer can accept data, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_io_writer_fd_efl_io_writer_can_write_get(const Eo *o EINA_UNUSED, Efl_Io_Writer_Fd_Data *pd)
{
   return pd->can_write;
}

/**
 * @brief Sets the 'can_write' status of the writer.
 *
 * This function updates the internal 'can_write' flag and emits the
 * "can_write,changed" event if the status actually changes.
 * It prevents setting 'can_write' to @c EINA_TRUE if the file descriptor is invalid.
 *
 * @param o The Efl_Io_Writer_Fd object.
 * @param pd The private data for the Efl_Io_Writer_Fd object.
 * @param can_write The new 'can_write' status.
 */
EOLIAN static void
_efl_io_writer_fd_efl_io_writer_can_write_set(Eo *o, Efl_Io_Writer_Fd_Data *pd, Eina_Bool can_write)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_writer_fd_get(o) < 0 && can_write);
   if (pd->can_write == can_write) return;
   pd->can_write = can_write;
   efl_event_callback_call(o, EFL_IO_WRITER_EVENT_CAN_WRITE_CHANGED, &can_write);
}

#include "efl_io_writer_fd.eo.c"
