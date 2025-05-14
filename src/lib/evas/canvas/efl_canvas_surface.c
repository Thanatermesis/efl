#include "efl_canvas_surface.h"

#define MY_CLASS EFL_CANVAS_SURFACE_CLASS

/**
 * @brief Constructor for the Efl_Canvas_Surface object.
 *
 * Initializes the Efl_Canvas_Surface object by calling the parent constructor
 * and setting the version for the native surface.
 *
 * @param[in] eo The Efl object to construct.
 * @param[in,out] pd The private data for the Efl_Canvas_Surface object.
 * @return The constructed Efl object.
 */
EOLIAN static Eo *
_efl_canvas_surface_efl_object_constructor(Eo *eo, Efl_Canvas_Surface_Data *pd)
{
   eo = efl_constructor(efl_super(eo, MY_CLASS));
   pd->surf.version = EVAS_NATIVE_SURFACE_VERSION;
   return eo;
}

/**
 * @brief Retrieves the native buffer associated with the Efl_Canvas_Surface.
 *
 * This function returns a pointer to the raw buffer data that the surface uses.
 * The nature and interpretation of this buffer depend on the specific type of
 * native surface being used (e.g., shared memory, TBM, etc.).
 *
 * @param[in] obj The Efl_Canvas_Surface object (unused).
 * @param[in] pd The private data for the Efl_Canvas_Surface object.
 * @return A pointer to the native buffer data, or NULL if not applicable.
 *         For example, this could be a direct pointer to pixel data or a handle.
 */
EOLIAN static void *
_efl_canvas_surface_native_buffer_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Surface_Data *pd)
{
   return pd->buffer;
}

#include "efl_canvas_surface.eo.c"
