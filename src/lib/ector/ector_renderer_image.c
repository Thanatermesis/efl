#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>

#include "ector_private.h"

#define MY_CLASS ECTOR_RENDERER_IMAGE_MIXIN

/**
 * @internal
 * @brief Sets the Ector_Buffer for the image renderer.
 *
 * This function assigns the provided Ector_Buffer to the
 * Ector_Renderer_Image_Data structure. This buffer will typically
 * contain the pixel data that the renderer will operate on or output to.
 *
 * @param obj The Ector_Renderer_Image object (unused in this function).
 * @param pd Pointer to the private data of the Ector_Renderer_Image object.
 * @param buffer The Ector_Buffer to be associated with this renderer.
 *               This can be NULL if no buffer is currently set.
 */
static void
_ector_renderer_image_buffer_set(Eo *obj EINA_UNUSED,
                                 Ector_Renderer_Image_Data *pd,
                                 Ector_Buffer *buffer)
{
   pd->buffer = buffer;
}


#include "ector_renderer_image.eo.c"
