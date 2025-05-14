#define EFL_IO_POSITIONER_FD_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"

#define MY_CLASS EFL_IO_POSITIONER_FD_CLASS

/**
 * @brief Private data for the Efl_Io_Positioner_Fd class.
 */
typedef struct _Efl_Io_Positioner_Fd_Data
{
   int fd; /**< The file descriptor being positioned. */
} Efl_Io_Positioner_Fd_Data;

/**
 * @brief Sets the file descriptor for this positioner.
 *
 * @param o The Eolian object.
 * @param pd The private data for the object.
 * @param fd The file descriptor to set.
 */
EOLIAN static void
_efl_io_positioner_fd_positioner_fd_set(Eo *o EINA_UNUSED, Efl_Io_Positioner_Fd_Data *pd, int fd)
{
   pd->fd = fd;
}

/**
 * @brief Gets the file descriptor for this positioner.
 *
 * @param o The Eolian object.
 * @param pd The private data for the object.
 * @return The file descriptor.
 */
EOLIAN static int
_efl_io_positioner_fd_positioner_fd_get(const Eo *o EINA_UNUSED, Efl_Io_Positioner_Fd_Data *pd)
{
   return pd->fd;
}

/**
 * @brief Converts Efl_Io_Positioner_Whence to standard C whence values (SEEK_SET, SEEK_CUR, SEEK_END).
 *
 * @param whence The Efl_Io_Positioner_Whence value to convert.
 * @return The corresponding standard C whence value. Defaults to SEEK_SET for unknown values.
 */
static inline int
_efl_io_positioner_whence_convert(Efl_Io_Positioner_Whence whence)
{
   switch (whence)
     {
      case EFL_IO_POSITIONER_WHENCE_START: return SEEK_SET;
      case EFL_IO_POSITIONER_WHENCE_CURRENT: return SEEK_CUR;
      case EFL_IO_POSITIONER_WHENCE_END: return SEEK_END;
     }
   return SEEK_SET;
}

/**
 * @brief Seeks to a position within the file descriptor.
 *
 * This function implements the Efl.Io.Positioner.seek interface.
 * It moves the file offset of the associated file descriptor.
 *
 * @param o The Eolian object.
 * @param pd The private data for the object (unused in this function but part of Eolian signature).
 * @param offset The offset to seek to.
 * @param whence The reference point for the offset (EFL_IO_POSITIONER_WHENCE_START,
 *               EFL_IO_POSITIONER_WHENCE_CURRENT, or EFL_IO_POSITIONER_WHENCE_END).
 * @return 0 on success, or a system error code (errno) on failure.
 */
EOLIAN static Eina_Error
_efl_io_positioner_fd_efl_io_positioner_seek(Eo *o, Efl_Io_Positioner_Fd_Data *pd EINA_UNUSED, int64_t offset, Efl_Io_Positioner_Whence whence)
{
   int fd = efl_io_positioner_fd_get(o);
   if (lseek(fd, (off_t)offset, _efl_io_positioner_whence_convert(whence)) < 0)
     return errno;
   efl_event_callback_call(o, EFL_IO_POSITIONER_EVENT_POSITION_CHANGED, NULL);
   return 0;
}

/**
 * @brief Gets the current position within the file descriptor.
 *
 * This function implements the Efl.Io.Positioner.position_get interface.
 * It returns the current file offset of the associated file descriptor.
 *
 * @param o The Eolian object.
 * @param pd The private data for the object (unused in this function but part of Eolian signature).
 * @return The current file offset as a uint64_t. Returns 0 if the fd is invalid or lseek fails.
 */
EOLIAN static uint64_t
_efl_io_positioner_fd_efl_io_positioner_position_get(const Eo *o, Efl_Io_Positioner_Fd_Data *pd EINA_UNUSED)
{
   int fd = efl_io_positioner_fd_get(o);
   off_t offset;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(fd < 0, 0);

   offset = lseek(fd, 0, SEEK_CUR);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(offset < 0, 0);

   return offset;
}

#include "efl_io_positioner_fd.eo.c"
