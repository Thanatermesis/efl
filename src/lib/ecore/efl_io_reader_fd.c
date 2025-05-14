#define EFL_IO_READER_PROTECTED 1
#define EFL_IO_READER_FD_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"

#define MY_CLASS EFL_IO_READER_FD_CLASS

/**
 * @brief Private data for the Efl_Io_Reader_Fd class.
 */
typedef struct _Efl_Io_Reader_Fd_Data
{
   int fd; /**< The file descriptor to read from. */
   Eina_Bool can_read; /**< Whether the reader can currently read. Set to EINA_FALSE if an error occurs or fd is invalid. */
   Eina_Bool eos; /**< Whether the end of stream has been reached. */
} Efl_Io_Reader_Fd_Data;

/**
 * @brief Sets the file descriptor for this reader.
 *
 * @param o The Efl_Io_Reader_Fd object.
 * @param pd The private data for the Efl_Io_Reader_Fd object.
 * @param fd The file descriptor to set.
 */
EOLIAN static void
_efl_io_reader_fd_reader_fd_set(Eo *o EINA_UNUSED, Efl_Io_Reader_Fd_Data *pd, int fd)
{
   pd->fd = fd;
}

/**
 * @brief Gets the file descriptor for this reader.
 *
 * @param o The Efl_Io_Reader_Fd object.
 * @param pd The private data for the Efl_Io_Reader_Fd object.
 * @return The file descriptor.
 */
EOLIAN static int
_efl_io_reader_fd_reader_fd_get(const Eo *o EINA_UNUSED, Efl_Io_Reader_Fd_Data *pd)
{
   return pd->fd;
}

/**
 * @brief Reads data from the file descriptor into the provided slice.
 *
 * This function attempts to read up to `rw_slice->len` bytes from the
 * file descriptor `pd->fd` into `rw_slice->mem`.
 *
 * On success, `rw_slice->len` is updated to the number of bytes actually read.
 * If the end of file is reached, `rw_slice->len` will be 0, and the `eos` property
 * will be set to EINA_TRUE.
 * If an error occurs, `rw_slice->len` will be 0, `rw_slice->mem` will be NULL,
 * the `can_read` property will be set to EINA_FALSE, and an appropriate
 * `errno` value will be returned. EINTR is handled internally by retrying the read.
 *
 * @param o The Efl_Io_Reader_Fd object.
 * @param pd The private data for the Efl_Io_Reader_Fd object.
 * @param rw_slice A pointer to an Eina_Rw_Slice structure.
 *                 The `mem` field should point to a buffer where data can be read,
 *                 and the `len` field should specify the maximum number of bytes to read.
 *                 Example:
 *                 ```c
 *                 char buffer[1024];
 *                 Eina_Rw_Slice slice = { .mem = buffer, .len = sizeof(buffer) };
 *                 Eina_Error err = efl_io_reader_read(obj, &slice);
 *                 if (err == 0) {
 *                   // slice.len now contains the number of bytes read
 *                   // slice.mem (still buffer) contains the data
 *                 }
 *                 ```
 * @return 0 on success, or an errno value on failure. EINVAL is returned if the
 *         file descriptor is invalid or `rw_slice` is NULL.
 */
EOLIAN static Eina_Error
_efl_io_reader_fd_efl_io_reader_read(Eo *o, Efl_Io_Reader_Fd_Data *pd EINA_UNUSED, Eina_Rw_Slice *rw_slice)
{
   int fd = efl_io_reader_fd_get(o);
   ssize_t r;

   EINA_SAFETY_ON_NULL_RETURN_VAL(rw_slice, EINVAL);
   if (fd < 0) goto error;
   do
     {
        r = read(fd, rw_slice->mem, rw_slice->len);
        if (r < 0)
          {
             if (errno == EINTR) continue;

             rw_slice->len = 0;
             rw_slice->mem = NULL;
             efl_io_reader_can_read_set(o, EINA_FALSE);
             return errno;
          }
     }
   while (r < 0);

   rw_slice->len = r;
   if (r == 0)
     {
        efl_io_reader_can_read_set(o, EINA_FALSE);
        efl_io_reader_eos_set(o, EINA_TRUE);
     }
   return 0;

 error:
   rw_slice->len = 0;
   rw_slice->mem = NULL;
   efl_io_reader_can_read_set(o, EINA_FALSE);
   return EINVAL;
}

/**
 * @brief Gets the can_read property.
 *
 * @param o The Efl_Io_Reader_Fd object.
 * @param pd The private data for the Efl_Io_Reader_Fd object.
 * @return EINA_TRUE if the reader can read, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_io_reader_fd_efl_io_reader_can_read_get(const Eo *o EINA_UNUSED, Efl_Io_Reader_Fd_Data *pd)
{
   return pd->can_read;
}

/**
 * @brief Sets the can_read property and emits an event if it changed.
 *
 * It is an error to set can_read to EINA_TRUE if the fd is invalid.
 *
 * @param o The Efl_Io_Reader_Fd object.
 * @param pd The private data for the Efl_Io_Reader_Fd object.
 * @param can_read The new value for the can_read property.
 */
EOLIAN static void
_efl_io_reader_fd_efl_io_reader_can_read_set(Eo *o, Efl_Io_Reader_Fd_Data *pd, Eina_Bool can_read)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_reader_fd_get(o) < 0 && can_read);
   if (pd->can_read == can_read) return;
   pd->can_read = can_read;
   efl_event_callback_call(o, EFL_IO_READER_EVENT_CAN_READ_CHANGED, &can_read);
}

/**
 * @brief Gets the eos property.
 *
 * @param o The Efl_Io_Reader_Fd object.
 * @param pd The private data for the Efl_Io_Reader_Fd object.
 * @return EINA_TRUE if end of stream has been reached, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_io_reader_fd_efl_io_reader_eos_get(const Eo *o EINA_UNUSED, Efl_Io_Reader_Fd_Data *pd)
{
   return pd->eos;
}

/**
 * @brief Sets the eos property and emits an event if it changed to EINA_TRUE.
 *
 * It is an error to set eos to EINA_FALSE if the fd is invalid.
 *
 * @param o The Efl_Io_Reader_Fd object.
 * @param pd The private data for the Efl_Io_Reader_Fd object.
 * @param is_eos The new value for the eos property.
 */
EOLIAN static void
_efl_io_reader_fd_efl_io_reader_eos_set(Eo *o, Efl_Io_Reader_Fd_Data *pd, Eina_Bool is_eos)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_reader_fd_get(o) < 0 && !is_eos);
   if (pd->eos == is_eos) return;
   pd->eos = is_eos;
   if (is_eos)
     efl_event_callback_call(o, EFL_IO_READER_EVENT_EOS, NULL);
}

#include "efl_io_reader_fd.eo.c"
