#include "config.h"
#include "Efl.h"

/**
 * @brief Sets the size of the Efl_Io_Sizer object.
 *
 * This function attempts to resize the underlying I/O object to the specified
 * size. It is an implementation of the `efl_io_sizer_size_set` Eolian method.
 *
 * @param[in] o The Efl_Io_Sizer object.
 * @param[in] pd Private data for the object (unused in this function).
 * @param[in] size The new desired size for the object in bytes.
 * @return @c EINA_TRUE on success (size was set), @c EINA_FALSE on failure.
 *         Success means efl_io_sizer_resize() returned 0.
 */
EOLIAN static Eina_Bool
_efl_io_sizer_size_set(Eo *o, void *pd EINA_UNUSED, uint64_t size)
{
   return efl_io_sizer_resize(o, size) == 0;
}

#include "interfaces/efl_io_sizer.eo.c"
