#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* this one is read-only buffer wrapping an existing evas_gl_image */

#define ECTOR_GL_BUFFER_BASE_PROTECTED

#include "evas_common_private.h"
#include "evas_gl_private.h"

#include <software/Ector_Software.h>
#include <gl/Ector_GL.h>
#include "Evas_Engine_GL_Generic.h"
#include "evas_ector_gl.h"
#include "evas_gl_private.h"

#define MY_CLASS EVAS_ECTOR_GL_IMAGE_BUFFER_CLASS

typedef struct _Ector_GL_Buffer_Map Ector_GL_Buffer_Map;
typedef struct _Evas_Ector_GL_Image_Buffer_Data Evas_Ector_GL_Image_Buffer_Data;

/**
 * @brief Structure to hold information about a mapped region of an Ector GL buffer.
 *
 * This structure is used internally to manage memory mappings of the Evas_GL_Image.
 * It keeps track of the mapped pointer, dimensions, colorspace, and other relevant
 * details for a specific mapping operation.
 */
struct _Ector_GL_Buffer_Map
{
   EINA_INLIST; /**< Inlist node for managing multiple maps. */
   void *ptr; /**< Pointer to the mapped region, adjusted for x, y offset. */
   unsigned int base_size; /**< Total size in bytes of the base data (e.g., W * H * pixel_size). */
   unsigned int x, y, w, h; /**< Coordinates and dimensions of the mapped sub-region. */
   void *image_data; /**< Pointer to the raw image data obtained from the engine. */
   void *base_data; /**< Pointer to the base of the (potentially converted) pixel data. */
   size_t length; /**< Length of the mapped region in bytes (e.g., (W * h + w - W) * pixel_size for a sub-region). */
   Efl_Gfx_Colorspace cspace; /**< Colorspace of the mapped data. */
   Evas_GL_Image *im; /**< The Evas_GL_Image associated with this map. */
   Eina_Bool allocated; /**< Flag indicating if base_data was allocated (e.g., for colorspace conversion). */
   Eina_Bool free_image; /**< Flag indicating if the Evas_GL_Image data should be freed on unmap. */
   Ector_Buffer_Access_Flag mode; /**< Access mode for this map (e.g., read-only). */
};

/**
 * @brief Private data structure for Evas_Ector_GL_Image_Buffer.
 *
 * This structure holds the Evas GL rendering engine context and the
 * Evas_GL_Image that this buffer wraps. It also maintains a list of
 * active memory mappings.
 */
struct _Evas_Ector_GL_Image_Buffer_Data
{
   Render_Output_GL_Generic *re; /**< Pointer to the Evas GL generic rendering engine. */
   Evas_GL_Image *glim; /**< The Evas_GL_Image being wrapped by this buffer. */
   Ector_GL_Buffer_Map *maps; /**< Inlist of active memory maps (Ector_GL_Buffer_Map). */
};

#undef ENFN
#undef ENDT
#undef ENC

#define ENC  pd->re

void *eng_image_data_put(void *data, void *image, DATA32 *image_data);
void *eng_image_data_get(void *data, void *image, int to_write, DATA32 **image_data, int *err, Eina_Bool *tofree);
void eng_image_free(void *data, void *image);

// testing out some macros to maybe add to eina
#define EINA_INLIST_REMOVE(l,i) do { l = (__typeof__(l)) eina_inlist_remove(EINA_INLIST_GET(l), EINA_INLIST_GET(i)); } while (0)
#define EINA_INLIST_APPEND(l,i) do { l = (__typeof__(l)) eina_inlist_append(EINA_INLIST_GET(l), EINA_INLIST_GET(i)); } while (0)

#define fail(fmt, ...) do { ERR(fmt, ##__VA_ARGS__); goto on_fail; } while (0)

/* FIXME: Conversion routines don't belong here */
/**
 * @brief Converts a block of pixels from ARGB32 format to 8-bit grayscale.
 *
 * This function takes an array of ARGB32 pixels and converts them to
 * an array of 8-bit grayscale pixels. The conversion uses the alpha channel
 * of the source ARGB data as the grayscale value.
 *
 * @param[out] dst Pointer to the destination buffer for GRY8 pixels.
 * @param[in] src Pointer to the source buffer of ARGB32 pixels.
 * @param[in] len Number of pixels to convert.
 */
static inline void
_pixels_argb_to_gry8_convert(uint8_t *dst, const uint32_t *src, int len)
{
   int k;
   for (k = 0; k < len; k++)
     {
        const uint32_t *s = src++;
        *dst++ = A_VAL(s);
     }
}

/**
 * @brief Sets the Evas GL engine and image for this buffer.
 * @internal
 *
 * This Eolian method is called to associate an Evas GL engine instance
 * and an Evas_GL_Image with this buffer. It ensures the image has a
 * texture and references the image. This is typically called during
 * initialization of the buffer.
 *
 * @param obj The Evas_Ector_GL_Image_Buffer object.
 * @param pd Private data for the Evas_Ector_GL_Image_Buffer.
 * @param engine Pointer to the Evas GL generic rendering engine.
 * @param image Pointer to the Evas_GL_Image to be wrapped.
 */
EOLIAN static void
_evas_ector_gl_image_buffer_evas_ector_buffer_engine_image_set(Eo *obj EINA_UNUSED,
                                                               Evas_Ector_GL_Image_Buffer_Data *pd,
                                                               void *engine, void *image)
{
   Render_Output_GL_Generic *re = engine;
   Evas_GL_Image *im = image;

   EINA_SAFETY_ON_FALSE_RETURN(!pd->glim);
   EINA_SAFETY_ON_NULL_RETURN(im);

   if (!im->tex)
     {
        Evas_Engine_GL_Context *gc;

        gc = re->window_gl_context_get(re->software.ob);
        evas_gl_common_image_update(gc, im);

        if (!im->tex)
          fail("Image has no texture!");
     }

   pd->re = re;
   evas_gl_common_image_ref(im);
   pd->glim = im;

 on_fail:
   return;
}

/**
 * @brief Retrieves the drawable Evas_GL_Image from the buffer.
 * @internal
 *
 * This Eolian method returns the underlying Evas_GL_Image that this buffer
 * wraps. It ensures the image has a texture and increments its reference count.
 * The caller is responsible for freeing the reference when done.
 *
 * @param obj The Evas_Ector_GL_Image_Buffer object.
 * @param pd Private data for the Evas_Ector_GL_Image_Buffer.
 * @return Pointer to the Evas_GL_Image, or NULL on failure (e.g., no texture).
 *         The returned image is ref-counted.
 */
EOLIAN static void *
_evas_ector_gl_image_buffer_evas_ector_buffer_drawable_image_get(Eo *obj EINA_UNUSED,
                                                                 Evas_Ector_GL_Image_Buffer_Data *pd)
{
   if (!pd->glim->tex)
     fail("Image has no texture!");

   evas_gl_common_image_ref(pd->glim);
   return pd->glim;

on_fail:
   return NULL;
}

/**
 * @brief Releases the Evas_GL_Image previously obtained via drawable_image_get.
 * @internal
 *
 * This Eolian method decrements the reference count of the provided Evas_GL_Image.
 * It's intended to be used with images obtained from
 * _evas_ector_gl_image_buffer_evas_ector_buffer_drawable_image_get.
 *
 * @param obj The Evas_Ector_GL_Image_Buffer object.
 * @param pd Private data for the Evas_Ector_GL_Image_Buffer.
 * @param image Pointer to the Evas_GL_Image to be released.
 * @return EINA_TRUE if the image was successfully released (i.e., matched the
 *         buffer's internal image and was not NULL), EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_evas_ector_gl_image_buffer_evas_ector_buffer_engine_image_release(Eo *obj EINA_UNUSED,
                                                                   Evas_Ector_GL_Image_Buffer_Data *pd,
                                                                   void *image)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(image, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(pd->glim == image, EINA_FALSE);

   evas_gl_common_image_free(pd->glim);
   return EINA_TRUE;
}

/**
 * @brief Gets the flags for this buffer.
 * @internal
 *
 * This Eolian method returns the capability flags for the buffer.
 * For Evas_Ector_GL_Image_Buffer, it indicates that the buffer is CPU readable
 * and can be used as a drawable source.
 *
 * @param obj The Evas_Ector_GL_Image_Buffer object.
 * @param pd Private data for the Evas_Ector_GL_Image_Buffer.
 * @return A bitmask of Ector_Buffer_Flag values.
 *         Example: ECTOR_BUFFER_FLAG_CPU_READABLE | ECTOR_BUFFER_FLAG_DRAWABLE
 */
EOLIAN static Ector_Buffer_Flag
_evas_ector_gl_image_buffer_ector_buffer_flags_get(const Eo *obj EINA_UNUSED,
                                                   Evas_Ector_GL_Image_Buffer_Data *pd EINA_UNUSED)
{
   return ECTOR_BUFFER_FLAG_CPU_READABLE | ECTOR_BUFFER_FLAG_DRAWABLE;
}

/**
 * @brief Gets the dimensions (width and height) of the buffer.
 * @internal
 *
 * This Eolian method retrieves the width and height of the underlying Evas_GL_Image.
 *
 * @param obj The Evas_Ector_GL_Image_Buffer object.
 * @param pd Private data for the Evas_Ector_GL_Image_Buffer.
 * @param[out] w Pointer to store the width of the buffer. Can be NULL.
 * @param[out] h Pointer to store the height of the buffer. Can be NULL.
 */
EOLIAN static void
_evas_ector_gl_image_buffer_ector_buffer_size_get(const Eo *obj EINA_UNUSED,
                                                  Evas_Ector_GL_Image_Buffer_Data *pd,
                                                  int *w, int *h)
{
   if (w) *w = pd->glim->w;
   if (h) *h = pd->glim->h;
}

/**
 * @brief Gets the native colorspace of the buffer.
 * @internal
 *
 * This Eolian method returns the colorspace of the underlying Evas_GL_Image.
 * For this implementation, it is always ARGB8888.
 *
 * @param obj The Evas_Ector_GL_Image_Buffer object.
 * @param pd Private data for the Evas_Ector_GL_Image_Buffer.
 * @return The colorspace, EFL_GFX_COLORSPACE_ARGB8888.
 */
EOLIAN static Efl_Gfx_Colorspace
_evas_ector_gl_image_buffer_ector_buffer_cspace_get(const Eo *obj EINA_UNUSED,
                                                    Evas_Ector_GL_Image_Buffer_Data *pd EINA_UNUSED)
{
   return EFL_GFX_COLORSPACE_ARGB8888;
}

/**
 * @brief Maps a region of the buffer for CPU access.
 * @internal
 *
 * This Eolian method provides a pointer to the pixel data of the Evas_GL_Image.
 * It supports read-only access and can perform colorspace conversion to GRY8
 * or provide direct access to ARGB8888 data.
 * The returned pointer corresponds to the top-left of the requested sub-region (x, y).
 *
 * @param obj The Evas_Ector_GL_Image_Buffer object.
 * @param pd Private data for the Evas_Ector_GL_Image_Buffer.
 * @param[out] length Pointer to store the size of the mapped region in bytes. Can be NULL.
 *                    This length corresponds to the actual data accessible from the returned pointer
 *                    for the requested w, h region, considering potential non-contiguous memory.
 *                    Example: For a full map of a WxH image, length = W * H * pixel_size.
 *                             For a sub-region, it might be (W_image * h_subregion + w_subregion - W_image) * pixel_size
 *                             if the memory is contiguous, or more complex if not. Here it's calculated for a contiguous block.
 * @param mode Access flags for the mapping (e.g., ECTOR_BUFFER_ACCESS_FLAG_READ).
 *             Only read access is supported.
 * @param x The starting X coordinate of the region to map.
 * @param y The starting Y coordinate of the region to map.
 * @param w Width of the region to map. If 0, maps from x to the image width.
 * @param h Height of the region to map. If 0, maps from y to the image height.
 * @param cspace The desired colorspace for the mapped data (EFL_GFX_COLORSPACE_ARGB8888 or EFL_GFX_COLORSPACE_GRY8).
 * @param[out] stride Pointer to store the stride (bytes per row) of the mapped data. Can be NULL.
 *                    Example: For ARGB8888, stride = image_width * 4. For GRY8, stride = image_width * 1.
 * @return Pointer to the mapped pixel data, or NULL on failure.
 *         The pointer is to the (x,y) offset within the (potentially colorspace converted) image data.
 */
EOLIAN static void *
_evas_ector_gl_image_buffer_ector_buffer_map(Eo *obj EINA_UNUSED, Evas_Ector_GL_Image_Buffer_Data *pd, unsigned int *length,
                                             Ector_Buffer_Access_Flag mode,
                                             unsigned int x, unsigned int y, unsigned int w, unsigned int h,
                                             Efl_Gfx_Colorspace cspace, unsigned int *stride)
{
   Ector_GL_Buffer_Map *map = NULL;
   Eina_Bool tofree = EINA_FALSE;
   Evas_GL_Image *im = NULL;
   unsigned int W, H;
   int len, err;
   uint32_t *data;
   int pxs;

   if ((cspace != EFL_GFX_COLORSPACE_GRY8) && (cspace != EFL_GFX_COLORSPACE_ARGB8888))
     {
        ERR("Unsupported colorspace for map: %d", (int) cspace);
        return NULL;
     }

   if (!mode)
     {
        ERR("Invalid access mode for map (none)");
        return NULL;
     }

   if (mode & EFL_GFX_BUFFER_ACCESS_MODE_WRITE)
     {
        ERR("%s does not support write access for map", efl_class_name_get(MY_CLASS));
        return NULL;
     }

   W = pd->glim->w;
   H = pd->glim->h;
   if (!w) w = W - x;
   if (!h) h = H - y;
   if ((x + w > W) || (y + h > H)) return NULL;

   im = eng_image_data_get(ENC, pd->glim, EINA_FALSE, &data, &err, &tofree);
   if (!im) return NULL;

   map = calloc(1, sizeof(*map));
   map->mode = mode;
   map->cspace = cspace;
   map->x = x;
   map->y = y;
   map->w = w;
   map->h = h;
   map->image_data = data;
   map->im = im;
   map->free_image = tofree;

   len = W * H;
   if (cspace == EFL_GFX_COLORSPACE_GRY8)
     {
        uint8_t *data8 = malloc(len);

        if (!data8) goto fail;
        _pixels_argb_to_gry8_convert(data8, data, len);
        map->allocated = EINA_TRUE;
        map->base_data = data8;
        map->ptr = data8 + x + (y * W);
        pxs = 1;
     }
   else
     {
        map->allocated = EINA_FALSE;
        map->base_data = data;
        map->ptr = data + x + (y * W);
        pxs = 4;
     }

   map->base_size = len * pxs;
   map->length = (W * h + w - W) * pxs;
   if (stride) *stride = W * pxs;
   if (length) *length = map->length;

   if (!tofree)
     pd->glim = im;

   EINA_INLIST_APPEND(pd->maps, map);
   return map->ptr;

fail:
   free(map);
   return NULL;
}

/**
 * @brief Unmaps a previously mapped region of the buffer.
 * @internal
 *
 * This Eolian method releases resources associated with a previous call to
 * _evas_ector_gl_image_buffer_ector_buffer_map. It finds the corresponding
 * map entry using the data pointer and length, frees any allocated memory
 * (e.g., for colorspace conversion), and updates engine image data if necessary.
 *
 * @param obj The Evas_Ector_GL_Image_Buffer object.
 * @param pd Private data for the Evas_Ector_GL_Image_Buffer.
 * @param data Pointer to the mapped data, as returned by the map function.
 *             This should be the `base_data` of the map, not the offset `ptr`.
 * @param length Length of the mapped data, as returned by the map function.
 */
EOLIAN static void
_evas_ector_gl_image_buffer_ector_buffer_unmap(Eo *obj EINA_UNUSED,
                                               Evas_Ector_GL_Image_Buffer_Data *pd,
                                               void *data, unsigned int length)
{
   Ector_GL_Buffer_Map *map;
   if (!data) return;

   EINA_INLIST_FOREACH(pd->maps, map)
     {
        if ((map->base_data == data) && (map->length == length))
          {
             EINA_INLIST_REMOVE(pd->maps, map);
             if (map->free_image)
               eng_image_free(ENC, map->im);
             else
               map->im = eng_image_data_put(ENC, map->im, map->image_data);
             if (map->allocated)
               free(map->base_data);
             free(map);
             return;
          }
     }

   ERR("Tried to unmap a non-mapped region: %p +%u", data, length);
}

/**
 * @brief Finalizes the Evas_Ector_GL_Image_Buffer object.
 * @internal
 *
 * This Eolian method is part of the Efl object lifecycle. It checks if the
 * buffer was initialized correctly (i.e., has an associated Evas_GL_Image)
 * before calling the superclass finalize method.
 *
 * @param obj The Evas_Ector_GL_Image_Buffer object to finalize.
 * @param pd Private data for the Evas_Ector_GL_Image_Buffer.
 * @return The finalized Efl_Object, or NULL if initialization failed.
 */
EOLIAN static Efl_Object *
_evas_ector_gl_image_buffer_efl_object_finalize(Eo *obj, Evas_Ector_GL_Image_Buffer_Data *pd)
{
   if (!pd->glim)
     {
        ERR("Buffer was not initialized properly!");
        return NULL;
     }
   return efl_finalize(efl_super(obj, MY_CLASS));
}

/**
 * @brief Destructor for the Evas_Ector_GL_Image_Buffer object.
 * @internal
 *
 * This Eolian method is part of the Efl object lifecycle. It frees the
 * associated Evas_GL_Image and then calls the superclass destructor.
 * Any active maps should ideally be unmapped before destruction, though
 * this function does not explicitly handle unmapping them.
 *
 * @param obj The Evas_Ector_GL_Image_Buffer object to destruct.
 * @param pd Private data for the Evas_Ector_GL_Image_Buffer.
 */
EOLIAN static void
_evas_ector_gl_image_buffer_efl_object_destructor(Eo *obj, Evas_Ector_GL_Image_Buffer_Data *pd)
{
   // Note: Active maps in pd->maps are not explicitly cleaned here.
   // This could lead to a resource leak if maps are not unmapped before object destruction.
   // However, the Evas_GL_Image itself is freed.
   evas_gl_common_image_free(pd->glim);
   efl_destructor(efl_super(obj, MY_CLASS));
}

#include "evas_ector_gl_image_buffer.eo.c"
