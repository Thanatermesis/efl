#include "config.h"
#include "Efl.h"

/**
 * @brief Sets the current position in the I/O object.
 *
 * This function sets the current read/write position to the given @p position.
 * It is equivalent to calling efl_io_positioner_seek() with
 * EFL_IO_POSITIONER_WHENCE_START as the whence parameter.
 *
 * @param[in] o The Efl object.
 * @param[in] pd Private data, unused in this function.
 * @param[in] position The new position (offset from the beginning of the stream).
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_io_positioner_position_set(Eo *o, void *pd EINA_UNUSED, uint64_t position)
{
   return efl_io_positioner_seek(o, position, EFL_IO_POSITIONER_WHENCE_START) == 0;
}

#include "interfaces/efl_io_positioner.eo.c"
