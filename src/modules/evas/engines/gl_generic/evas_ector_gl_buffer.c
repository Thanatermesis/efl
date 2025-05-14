/**
 * @file
 * @brief Evas Ector GL Buffer implementation
 *
 * This file provides the Evas Ector GL Buffer, which allows for the creation
 * and manipulation of graphical buffers using OpenGL. It integrates with
 * Ector for rendering operations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define ECTOR_GL_BUFFER_BASE_PROTECTED

#include "evas_common_private.h"
#include "evas_gl_private.h"

#include <software/Ector_Software.h>
#include <gl/Ector_GL.h>
#include "Evas_Engine_GL_Generic.h"
#include "evas_ector_gl.h"
#include "evas_gl_private.h"

#define MY_CLASS EVAS_ECTOR_GL_BUFFER_CLASS

typedef struct _Ector_GL_Buffer_Map Ector_GL_Buffer_Map;
typedef struct _Evas_Ector_GL_Buffer_Data Evas_Ector_GL_Buffer_Data;

static int _map_id = 0; /**< Counter for unique map identifiers. */

/**
 * @brief Structure representing a mapped region of a GL buffer.
 *
 * This structure holds information about a memory-mapped region of an
 * Evas_GL_Image, allowing direct CPU access to pixel data.
 */
struct _Ector_GL_Buffer_Map
{
   EINA_INLIST; /**< Macro for Eina_Inlist node integration. */
   void *ptr; /**< Pointer to the mapped pixel data, adjusted for sub-region. */
   unsigned int base_size; /**< Total size of the base_data in bytes. */
   unsigned int x, y, w, h; /**< Coordinates and dimensions of the mapped sub-region. */
   void *image_data; /**< Pointer to the raw image data from eng_image_data_get(). */
   void *base_data; /**< Pointer to the base of the (potentially converted) pixel data.
                       * If cspace is GRY8, this points to a converted buffer.
                       * Otherwise, it's the same as image_data. */
   int map_id; /**< Unique identifier for this map operation. */
   size_t length; /**< Length of the mapped region in bytes (w * h * pixel_size). */
   Efl_Gfx_Colorspace cspace; /**< Colorspace of the mapped data (ptr). */
   Evas_GL_Image *im; /**< Evas_GL_Image associated with this map, if it needs to be freed separately.
                          * This is typically the case when a new image is created for the map
                          * (e.g., when detaching from an FBO for writing). */
   Eina_Bool allocated; /**< True if base_data was allocated for colorspace conversion (e.g., to GRY8). */
   Ector_Buffer_Access_Flag mode; /**< Access mode for the map (read, write, etc.). */
};

/**
 * @brief Private data for the Evas_Ector_GL_Buffer object.
 *
 * This structure holds all the internal state for an Evas Ector GL buffer,
 * including the GL image, rendering engine context, and active memory maps.
 */
struct _Evas_Ector_GL_Buffer_Data
{
   Render_Engine_GL_Generic *re; /**< Pointer to the generic GL rendering engine. */
   Evas_GL_Image *glim; /**< The core Evas_GL_Image used by this buffer. */
   Eina_Bool alpha_only; /**< True if the buffer is for alpha-only (GRY8) data. */
   Eina_Bool was_render; /**< True if the buffer was last used for rendering (FBO). */
   Ector_GL_Buffer_Map *maps; /**< Inlist of active Ector_GL_Buffer_Map structures. */
};

void *eng_image_data_put(void *data, void *image, DATA32 *image_data);
void *eng_image_data_get(void *data, void *image, int to_write, DATA32 **image_data, int *err, Eina_Bool *tofree);
void eng_image_free(void *data, void *image);

#undef ENFN
#undef ENDT
#undef ENC

#define ENC pd->re

// testing out some macros to maybe add to eina
#define EINA_INLIST_REMOVE(l,i) do { l = (__typeof__(l)) eina_inlist_remove(EINA_INLIST_GET(l), EINA_INLIST_GET(i)); } while (0)
#define EINA_INLIST_APPEND(l,i) do { l = (__typeof__(l)) eina_inlist_append(EINA_INLIST_GET(l), EINA_INLIST_GET(i)); } while (0)
#define EINA_INLIST_PREPEND(l,i) do { l = (__typeof__(l)) eina_inlist_prepend(EINA_INLIST_GET(l), EINA_INLIST_GET(i)); } while (0)

#define fail(fmt, ...) do { ERR(fmt, ##__VA_ARGS__); goto on_fail; } while (0)

#if 0
static inline void
_mapped_image_dump(Eo *buf, Evas_GL_Image *im, const char *fmt, int id)
{
   if (!im || !im->im) return;
   evas_common_save_image_to_file(im->im, eina_slstr_printf("/tmp/dump/%s_%02d_buf_%p_im_%p.png", fmt, id, buf, im),
                                  NULL, 100, 9, NULL);
}

#define MAP_DUMP(_im, _fmt) _mapped_image_dump(obj, _im, _fmt, map->map_id)
#else
#define MAP_DUMP(...)
#endif

/* FIXME: Conversion routines don't belong here */
/**
 * @brief Converts ARGB8888 pixel data to GRY8 (alpha channel).
 * @param dst Pointer to the destination buffer (GRY8).
 * @param src Pointer to the source buffer (ARGB8888).
 * @param len Number of pixels to convert.
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
 * @brief Converts GRY8 (alpha channel) pixel data to ARGB8888.
 * @param dst Pointer to the destination buffer (ARGB8888).
 * @param src Pointer to the source buffer (GRY8).
 * @param len Number of pixels to convert.
 * @note The R, G, B components are filled with the alpha value.
 */
static inline void
_pixels_gry8_to_argb_convert(uint32_t *dst, const uint8_t *src, int len)
{
   int k;
   for (k = 0; k < len; k++)
     {
        const uint8_t s = *src++;
        *dst++ = ARGB_JOIN(s, s, s, s);
     }
}

/**
 * @brief Checks if an Evas_GL_Image is backed by an FBO.
 * @param glim The Evas_GL_Image to check.
 * @return EINA_TRUE if the image is FBO-backed, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_evas_gl_image_is_fbo(Evas_GL_Image *glim)
{
   return glim && glim->tex && glim->tex->pt && glim->tex->pt->fb;
}

/**
 * @brief Prepares the GL buffer for use.
 * @param obj The Evas_Ector_GL_Buffer object.
 * @param pd Private data of the Evas_Ector_GL_Buffer.
 * @param engine Pointer to the Render_Engine_GL_Generic.
 * @param w Width of the buffer.
 * @param h Height of the buffer.
 * @param cspace Colorspace of the buffer.
 * @param flags Buffer flags (currently unused).
 *
 * This function initializes the internal Evas_GL_Image based on the provided
 * dimensions and colorspace. It is intended to be called once during setup.
 */
EOLIAN static void
_evas_ector_gl_buffer_gl_buffer_prepare(Eo *obj, Evas_Ector_GL_Buffer_Data *pd,
                                        void *engine,
                                        int w, int h, Efl_Gfx_Colorspace cspace,
                                        Ector_Buffer_Flag flags EINA_UNUSED)
{
   Render_Engine_GL_Generic *re = engine;
   Evas_Engine_GL_Context *gc;
   Evas_GL_Image *im;

   // this is meant to be called only once
   EINA_SAFETY_ON_FALSE_GOTO(!pd->re, on_fail);
   EINA_SAFETY_ON_FALSE_GOTO(!efl_finalized_get(obj), on_fail);

   if (cspace == EFL_GFX_COLORSPACE_ARGB8888)
     pd->alpha_only = EINA_FALSE;
   else if (cspace == EFL_GFX_COLORSPACE_GRY8)
     pd->alpha_only = EINA_TRUE;
   else
     fail("Unsupported colorspace: %u", cspace);

   pd->re = re;
   gc = gl_generic_context_find(re, 1);
   im = evas_gl_common_image_surface_new(gc, w, h, EINA_TRUE, EINA_FALSE);
   if (!im) fail("Failed to create GL surface!");

   pd->glim = im;
   return;

on_fail:
   evas_gl_common_image_free(pd->glim);
   pd->glim = NULL;
}

/**
 * @brief Retrieves the internal Evas_GL_Image.
 * @param pd Private data of the Evas_Ector_GL_Buffer.
 * @param render EINA_TRUE if the image is being retrieved for rendering (FBO usage),
 *               EINA_FALSE otherwise.
 * @return Pointer to the Evas_GL_Image on success, NULL on failure.
 *
 * This helper function ensures the image is not currently mapped and handles
 * FBO-specific logic if @p render is true. It also increments the reference
 * count of the returned image.
 */
static inline void *
_image_get(Evas_Ector_GL_Buffer_Data *pd, Eina_Bool render)
{
   if (pd->maps != NULL)
     fail("Image is currently mapped!");

   if (!pd->glim || !pd->glim->tex || !pd->glim->tex->pt)
     fail("Image has no texture!");

   evas_gl_common_image_ref(pd->glim);
   if (render)
     {
        if (!pd->glim->tex->pt->fb)
          fail("Image has no FBO!");
        pd->was_render = EINA_TRUE;
     }
   return pd->glim;

on_fail:
   return NULL;
}

/**
 * @brief Gets the Evas_GL_Image for drawing operations (not necessarily FBO).
 * @param obj The Evas_Ector_GL_Buffer object (unused).
 * @param pd Private data of the Evas_Ector_GL_Buffer.
 * @return Pointer to the Evas_GL_Image.
 * @see _image_get
 */
EOLIAN static void *
_evas_ector_gl_buffer_evas_ector_buffer_drawable_image_get(Eo *obj EINA_UNUSED,
                                                           Evas_Ector_GL_Buffer_Data *pd)
{
   return _image_get(pd, EINA_FALSE);
}

/**
 * @brief Gets the Evas_GL_Image for rendering operations (FBO).
 * @param obj The Evas_Ector_GL_Buffer object (unused).
 * @param pd Private data of the Evas_Ector_GL_Buffer.
 * @return Pointer to the Evas_GL_Image (FBO).
 * @see _image_get
 */
EOLIAN static void *
_evas_ector_gl_buffer_evas_ector_buffer_render_image_get(Eo *obj EINA_UNUSED,
                                                         Evas_Ector_GL_Buffer_Data *pd)
{
   return _image_get(pd, EINA_TRUE);
}

/**
 * @brief Releases an Evas_GL_Image previously obtained via _drawable_image_get or _render_image_get.
 * @param obj The Evas_Ector_GL_Buffer object (unused).
 * @param pd Private data of the Evas_Ector_GL_Buffer.
 * @param image The Evas_GL_Image to release.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * This function decrements the reference count of the image. If the image was
 * used for rendering (FBO), it might detach the surface.
 */
EOLIAN static Eina_Bool
_evas_ector_gl_buffer_evas_ector_buffer_engine_image_release(Eo *obj EINA_UNUSED,
                                                             Evas_Ector_GL_Buffer_Data *pd,
                                                             void *image)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(image, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(pd->glim == image, EINA_FALSE);

   if (pd->was_render)
     pd->glim = evas_gl_common_image_surface_detach(pd->glim);

   evas_gl_common_image_free(pd->glim);

   return EINA_TRUE;
}

/**
 * @brief Gets the dimensions of the buffer.
 * @param obj The Evas_Ector_GL_Buffer object (unused).
 * @param pd Private data of the Evas_Ector_GL_Buffer.
 * @param[out] w Pointer to store the width.
 * @param[out] h Pointer to store the height.
 */
EOLIAN static void
_evas_ector_gl_buffer_ector_buffer_size_get(const Eo *obj EINA_UNUSED,
                                            Evas_Ector_GL_Buffer_Data *pd,
                                            int *w, int *h)
{
   if (w) *w = pd->glim->w;
   if (h) *h = pd->glim->h;
}

/**
 * @brief Gets the native colorspace of the buffer.
 * @param obj The Evas_Ector_GL_Buffer object (unused).
 * @param pd Private data of the Evas_Ector_GL_Buffer.
 * @return The native Efl_Gfx_Colorspace of the buffer (EFL_GFX_COLORSPACE_GRY8 or EFL_GFX_COLORSPACE_ARGB8888).
 */
EOLIAN static Efl_Gfx_Colorspace
_evas_ector_gl_buffer_ector_buffer_cspace_get(const Eo *obj EINA_UNUSED,
                                              Evas_Ector_GL_Buffer_Data *pd)
{
   if (pd->alpha_only)
     return EFL_GFX_COLORSPACE_GRY8;
   else
     return EFL_GFX_COLORSPACE_ARGB8888;
}

/**
 * @brief Gets the flags of the buffer.
 * @param obj The Evas_Ector_GL_Buffer object (unused).
 * @param pd Private data of the Evas_Ector_GL_Buffer (unused).
 * @return A combination of Ector_Buffer_Flag values indicating buffer capabilities.
 */
EOLIAN static Ector_Buffer_Flag
_evas_ector_gl_buffer_ector_buffer_flags_get(const Eo *obj EINA_UNUSED,
                                             Evas_Ector_GL_Buffer_Data *pd EINA_UNUSED)
{
   return ECTOR_BUFFER_FLAG_CPU_READABLE | ECTOR_BUFFER_FLAG_DRAWABLE |
         ECTOR_BUFFER_FLAG_CPU_WRITABLE | ECTOR_BUFFER_FLAG_RENDERABLE;
}

/**
 * @brief Maps a region of the buffer for direct CPU access.
 * @param obj The Evas_Ector_GL_Buffer object (unused).
 * @param pd Private data of the Evas_Ector_GL_Buffer.
 * @param[out] length Pointer to store the length of the mapped region in bytes.
 * @param mode Access mode flags (e.g., ECTOR_BUFFER_ACCESS_FLAG_READ, ECTOR_BUFFER_ACCESS_FLAG_WRITE).
 * @param x X-coordinate of the region to map.
 * @param y Y-coordinate of the region to map.
 * @param w Width of the region to map (0 for full width from x).
 * @param h Height of the region to map (0 for full height from y).
 * @param cspace Requested colorspace for the mapped data. If different from the buffer's
 *               native colorspace, conversion will occur.
 * @param[out] stride Pointer to store the stride (bytes per row) of the mapped region.
 * @return Pointer to the mapped memory region on success, NULL on failure.
 *
 * This function allows direct access to the buffer's pixel data. If the requested
 * colorspace (@p cspace) is EFL_GFX_COLORSPACE_GRY8 and the buffer is ARGB8888,
 * the data will be converted. If writing to an FBO-backed image, the image data
 * might be detached and copied.
 */
EOLIAN static void *
_evas_ector_gl_buffer_ector_buffer_map(Eo *obj EINA_UNUSED, Evas_Ector_GL_Buffer_Data *pd,
                                       unsigned int *length,
                                       Ector_Buffer_Access_Flag mode,
                                       unsigned int x, unsigned int y, unsigned int w, unsigned int h,
                                       Efl_Gfx_Colorspace cspace, unsigned int *stride)
{
   Eina_Bool write = !!(mode & ECTOR_BUFFER_ACCESS_FLAG_WRITE);
   Ector_GL_Buffer_Map *map = NULL;
   Eina_Bool tofree = EINA_FALSE;
   Evas_GL_Image *im = NULL;
   unsigned int W, H;
   int len, err;
   uint32_t *data;
   int pxs;

   W = pd->glim->w;
   H = pd->glim->h;
   if (!w) w = W - x;
   if (!h) h = H - y;
   if ((x + w > W) || (y + h > H)) return NULL;

   if (write && _evas_gl_image_is_fbo(pd->glim))
     {
        // Can not open FBO data to write!
        im = eng_image_data_get(ENC, pd->glim, EINA_FALSE, &data, &err, &tofree);
        if (!im) return NULL;
     }
   else
     {
        im = eng_image_data_get(ENC, pd->glim, write, &data, &err, &tofree);
        if (!im) return NULL;
     }

   map = calloc(1, sizeof(*map));
   map->mode = mode;
   map->cspace = cspace;
   map->x = x;
   map->y = y;
   map->w = w;
   map->h = h;
   map->image_data = data;
   map->im = tofree ? im : NULL;

   len = W * H;
   if (cspace == EFL_GFX_COLORSPACE_GRY8)
     {
        uint8_t *data8 = malloc(len);

        if (!data8) goto on_fail;
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

   map->map_id = ++_map_id;
   map->base_size = len * pxs;
   map->length = (W * h + w - W) * pxs;
   if (stride) *stride = W * pxs;
   if (length) *length = map->length;

   MAP_DUMP(im, "in");

   EINA_INLIST_PREPEND(pd->maps, map);
   return map->ptr;

on_fail:
   free(map);
   return NULL;
}

/**
 * @brief Unmaps a previously mapped region of the buffer.
 * @param obj The Evas_Ector_GL_Buffer object (unused).
 * @param pd Private data of the Evas_Ector_GL_Buffer.
 * @param data Pointer to the mapped memory region (returned by _ector_buffer_map).
 * @param length Length of the mapped region in bytes (returned by _ector_buffer_map).
 *
 * This function releases the mapped memory region. If the region was mapped for
 * writing, the changes are applied to the underlying Evas_GL_Image. This might
 * involve colorspace conversion (e.g., from GRY8 back to ARGB8888) and updating
 * the GL texture.
 */
EOLIAN static void
_evas_ector_gl_buffer_ector_buffer_unmap(Eo *obj EINA_UNUSED, Evas_Ector_GL_Buffer_Data *pd,
                                         void *data, unsigned int length)
{
   Ector_GL_Buffer_Map *map;
   if (!data) return;

   EINA_INLIST_FOREACH(pd->maps, map)
     {
        if ((map->ptr == data) && (map->length == length))
          {
             EINA_INLIST_REMOVE(pd->maps, map);
             if (map->mode & ECTOR_BUFFER_ACCESS_FLAG_WRITE)
               {
                  Evas_GL_Image *old_glim = pd->glim;
                  int W, H;

                  W = pd->glim->w;
                  H = pd->glim->h;

                  if (map->cspace == EFL_GFX_COLORSPACE_GRY8)
                    _pixels_gry8_to_argb_convert(map->image_data, map->base_data, W * H);

                  if (map->im)
                    {
                       MAP_DUMP(map->im, "out_w_free");
                       pd->glim = evas_gl_common_image_surface_update(map->im);
                       evas_gl_common_image_free(old_glim);
                    }
                  else
                    {
                       MAP_DUMP(old_glim, "out_w_nofree");
                       pd->glim = evas_gl_common_image_surface_update(old_glim);
                    }
               }
             else
               {
                  if (map->im)
                    {
                       MAP_DUMP(map->im, "out_ro_free");
                       eng_image_free(ENC, map->im);
                    }
                  else
                    {
                       MAP_DUMP(pd->glim, "out_ro_nofree");
                       pd->glim = eng_image_data_put(ENC, pd->glim, map->image_data);
                    }
               }
             if (map->allocated)
               free(map->base_data);
             free(map);
             return;
          }
     }

   ERR("Tried to unmap a non-mapped region!");
}

/**
 * @brief Finalizes the Evas_Ector_GL_Buffer object.
 * @param obj The Evas_Ector_GL_Buffer object.
 * @param pd Private data of the Evas_Ector_GL_Buffer.
 * @return The finalized Efl_Object.
 *
 * Ensures that the buffer was properly initialized (i.e., pd->glim is not NULL)
 * before calling the superclass finalize method.
 */
EOLIAN static Efl_Object *
_evas_ector_gl_buffer_efl_object_finalize(Eo *obj, Evas_Ector_GL_Buffer_Data *pd)
{
   if (!pd->glim)
     {
        ERR("Buffer was not initialized properly!");
        return NULL;
     }
   return efl_finalize(efl_super(obj, MY_CLASS));
}

/**
 * @brief Destructor for the Evas_Ector_GL_Buffer object.
 * @param obj The Evas_Ector_GL_Buffer object.
 * @param pd Private data of the Evas_Ector_GL_Buffer.
 *
 * Frees the internal Evas_GL_Image and calls the superclass destructor.
 * Any active maps should ideally be unmapped before destruction, though
 * this function does not explicitly handle unmapping.
 */
EOLIAN static void
_evas_ector_gl_buffer_efl_object_destructor(Eo *obj, Evas_Ector_GL_Buffer_Data *pd)
{
   evas_gl_common_image_free(pd->glim);
   efl_destructor(efl_super(obj, MY_CLASS));
}

#include "evas_ector_gl_buffer.eo.c"
