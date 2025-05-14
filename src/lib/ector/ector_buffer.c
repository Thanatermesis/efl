#ifdef HAVE_CONFIG_H
# include "config.h"
#else
# define EFL_BETA_API_SUPPORT
#endif

#include <Eo.h>
#include "ector_private.h"
#include "ector_buffer.eo.h"

/**
 * @internal
 * Implements efl_gfx_buffer_cspace_get for Ector_Buffer.
 * Retrieves the colorspace of the buffer.
 *
 * @param obj The Ector_Buffer object.
 * @param pd The private data of the Ector_Buffer object.
 * @return The colorspace of the buffer.
 */
EOLIAN static Efl_Gfx_Colorspace
_ector_buffer_cspace_get(const Eo *obj EINA_UNUSED, Ector_Buffer_Data *pd)
{
   return pd->cspace;
}

/**
 * @internal
 * Implements efl_gfx_buffer_size_get for Ector_Buffer.
 * Retrieves the dimensions (width and height) of the buffer.
 *
 * @param obj The Ector_Buffer object.
 * @param pd The private data of the Ector_Buffer object.
 * @param w Pointer to store the width of the buffer. Can be NULL.
 * @param h Pointer to store the height of the buffer. Can be NULL.
 */
EOLIAN static void
_ector_buffer_size_get(const Eo *obj EINA_UNUSED, Ector_Buffer_Data *pd, int *w, int *h)
{
   if (w) *w = pd->w;
   if (h) *h = pd->h;
}

/**
 * @internal
 * Implements ector_buffer_flags_get for Ector_Buffer.
 * Retrieves the flags associated with the buffer.
 * Currently, this base implementation returns no flags.
 * Derived classes may override this to provide specific flags.
 *
 * @param obj The Ector_Buffer object.
 * @param pd The private data of the Ector_Buffer object.
 * @return The flags of the buffer. See #Ector_Buffer_Flag.
 */
EOLIAN static Ector_Buffer_Flag
_ector_buffer_flags_get(const Eo *obj EINA_UNUSED, Ector_Buffer_Data *pd EINA_UNUSED)
{
   return ECTOR_BUFFER_FLAG_NONE;
}

#include "ector_buffer.eo.c"
#include "ector_surface.eo.c"
