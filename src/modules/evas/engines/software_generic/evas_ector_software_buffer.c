#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <software/Ector_Software.h>
#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_ector_software.h"

#define MY_CLASS EVAS_ECTOR_SOFTWARE_BUFFER_CLASS

/**
 * @brief Private data structure for the Evas_Ector_Software_Buffer class.
 *
 * This structure holds the necessary data for managing an Evas software buffer
 * within the Ector framework. It links the Ector buffer representation with
 * the underlying Evas RGBA_Image.
 */
typedef struct {
   Ector_Software_Buffer_Base_Data *base; /**< Pointer to the base Ector software buffer data. */
   RGBA_Image *image; /**< Pointer to the associated Evas RGBA_Image. */
} Evas_Ector_Software_Buffer_Data;

// Note: Don't use ENFN, ENDT here because the GL engine may also use SW buffers
// eg. in the filters.

/**
 * @brief Sets the underlying Evas image for this Ector buffer.
 * @internal
 *
 * This function associates an Evas RGBA_Image with the Ector software buffer.
 * It increments the reference count of the image and updates the buffer's
 * pixel data pointer, dimensions, and colorspace based on the image properties.
 *
 * @param obj The Evas_Ector_Software_Buffer object.
 * @param pd The private data for the object.
 * @param engine The Evas engine instance (unused).
 * @param image A pointer to the RGBA_Image to set. Must not be NULL and must
 *              have valid image data.
 */
EOLIAN static void
_evas_ector_software_buffer_evas_ector_buffer_engine_image_set(Eo *obj,
                                                               Evas_Ector_Software_Buffer_Data *pd,
                                                               void *engine EINA_UNUSED,
                                                               void *image)
{
   RGBA_Image *im = image;

   EINA_SAFETY_ON_NULL_RETURN(image);
   EINA_SAFETY_ON_FALSE_RETURN(!efl_finalized_get(obj));
   EINA_SAFETY_ON_NULL_RETURN(im->image.data);

   evas_cache_image_ref(&im->cache_entry);
   pd->image = im;

   ector_buffer_pixels_set(obj, im->image.data, im->cache_entry.w, im->cache_entry.h, 0, (Efl_Gfx_Colorspace)im->cache_entry.space, EINA_TRUE);
}

/**
 * @brief Gets the drawable Evas image associated with this buffer.
 * @internal
 *
 * Returns the underlying RGBA_Image that can be used for drawing operations.
 * Increments the reference count of the returned image.
 *
 * @param obj The Evas_Ector_Software_Buffer object (unused).
 * @param pd The private data for the object.
 * @return A pointer to the RGBA_Image, with its reference count incremented.
 *         Returns NULL if no image is associated.
 */
EOLIAN static void *
_evas_ector_software_buffer_evas_ector_buffer_drawable_image_get(Eo *obj EINA_UNUSED,
                                                                 Evas_Ector_Software_Buffer_Data *pd)
{
   evas_cache_image_ref(&pd->image->cache_entry);
   return pd->image;
}

/**
 * @brief Gets the renderable Evas image associated with this buffer.
 * @internal
 *
 * Returns the underlying RGBA_Image that can be used for rendering operations.
 * Increments the reference count of the returned image. In this software
 * implementation, this is the same as the drawable image.
 *
 * @param obj The Evas_Ector_Software_Buffer object (unused).
 * @param pd The private data for the object.
 * @return A pointer to the RGBA_Image, with its reference count incremented.
 *         Returns NULL if no image is associated.
 */
EOLIAN static void *
_evas_ector_software_buffer_evas_ector_buffer_render_image_get(Eo *obj EINA_UNUSED,
                                                               Evas_Ector_Software_Buffer_Data *pd)
{
   evas_cache_image_ref(&pd->image->cache_entry);
   return pd->image;
}

/**
 * @brief Releases the reference held by the buffer to the Evas image.
 * @internal
 *
 * Decrements the reference count of the provided RGBA_Image. This should be
 * called when the engine no longer needs the specific image reference obtained
 * previously (e.g., via _drawable_image_get or _render_image_get).
 *
 * @param obj The Evas_Ector_Software_Buffer object (unused).
 * @param pd The private data for the object.
 * @param image The RGBA_Image pointer whose reference should be released.
 *              Must match the image currently held by the buffer (pd->image).
 * @return EINA_TRUE if the image reference was successfully dropped,
 *         EINA_FALSE otherwise (e.g., if image is NULL or doesn't match).
 */
EOLIAN static Eina_Bool
_evas_ector_software_buffer_evas_ector_buffer_engine_image_release(Eo *obj EINA_UNUSED,
                                                                   Evas_Ector_Software_Buffer_Data *pd,
                                                                   void *image)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(image, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(pd->image == image, EINA_FALSE);

   evas_cache_image_drop(&pd->image->cache_entry);
   return EINA_TRUE;
}

/**
 * @brief Constructor for the Evas_Ector_Software_Buffer object.
 * @internal
 *
 * Initializes the Evas_Ector_Software_Buffer object by calling the superclass
 * constructor and obtaining a reference to the base Ector software buffer data.
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the object.
 * @return The constructed Eo object.
 */
EOLIAN static Eo *
_evas_ector_software_buffer_efl_object_constructor(Eo *obj, Evas_Ector_Software_Buffer_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   pd->base = efl_data_xref(obj, ECTOR_SOFTWARE_BUFFER_BASE_MIXIN, obj);
   return obj;
}

/**
 * @brief Finalizer for the Evas_Ector_Software_Buffer object.
 * @internal
 *
 * Completes the initialization of the Evas_Ector_Software_Buffer object.
 * It ensures that the base data and image pointers are valid, marks the
 * buffer as immutable (as it's tied to an existing Evas image), and calls
 * the superclass finalizer.
 *
 * @param obj The Eo object to finalize.
 * @param pd The private data for the object.
 * @return The finalized Eo object, or NULL on failure.
 */
EOLIAN static Eo *
_evas_ector_software_buffer_efl_object_finalize(Eo *obj, Evas_Ector_Software_Buffer_Data *pd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->base, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->image, NULL);
   pd->base->generic->immutable = EINA_TRUE;
   return efl_finalize(efl_super(obj, MY_CLASS));
}

/**
 * @brief Destructor for the Evas_Ector_Software_Buffer object.
 * @internal
 *
 * Cleans up resources associated with the Evas_Ector_Software_Buffer object.
 * It releases the reference to the base Ector software buffer data and drops
 * the reference to the associated Evas RGBA_Image before calling the
 * superclass destructor.
 *
 * @param obj The Eo object being destructed.
 * @param pd The private data for the object.
 */
EOLIAN static void
_evas_ector_software_buffer_efl_object_destructor(Eo *obj, Evas_Ector_Software_Buffer_Data *pd)
{
   efl_data_xunref(obj, pd->base, obj);
   evas_cache_image_drop(&pd->image->cache_entry);
   efl_destructor(efl_super(obj, MY_CLASS));
}

#include "evas_ector_buffer.eo.c"
#include "evas_ector_software_buffer.eo.c"
