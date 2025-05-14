#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Ector_Software.h"
#include "ector_private.h"
#include "ector_software_private.h"
#include "ector_buffer.h"
#include "draw.h"

#define MY_CLASS ECTOR_SOFTWARE_BUFFER_CLASS

/**
 * @brief Structure to manage a mapped region of a software buffer.
 *
 * This structure holds information about a temporarily mapped pixel data region,
 * including its pointer, dimensions, format, and access mode. It is used
 * internally to track active mappings.
 */
typedef struct _Ector_Software_Buffer_Map
{
   EINA_INLIST; /**< Inlist node for managing multiple maps. */
   uint8_t *ptr; /**< Pointer to the mapped pixel data. */
   unsigned int size, stride; /**< Size (in bytes) and stride (in bytes) of the mapped region. */
   unsigned int x, y, w, h; /**< Coordinates (x, y) and dimensions (w, h) of the mapped sub-region. */
   Efl_Gfx_Colorspace cspace; /**< Colorspace of the mapped data. Can be different from the buffer's native colorspace. */
   Eina_Bool allocated; /**< True if this map involves a new memory allocation (e.g., for conversion or COW). */
   Ector_Buffer_Access_Flag mode; /**< Access mode for this map (read, write, COW). */
} Ector_Software_Buffer_Map;

/* FIXME: Conversion routines don't belong here */

/**
 * @brief Converts a line of pixels from ARGB32 to 8-bit grayscale.
 *
 * Only the alpha channel from the source ARGB32 pixels is used to
 * generate the grayscale value.
 *
 * @param dst Pointer to the destination buffer for grayscale pixels.
 * @param src Pointer to the source buffer of ARGB32 pixels.
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
 * @brief Converts a line of pixels from 8-bit grayscale to ARGB32.
 *
 * The grayscale value is replicated across the R, G, B, and A channels
 * of the destination ARGB32 pixels.
 *
 * @param dst Pointer to the destination buffer for ARGB32 pixels.
 * @param src Pointer to the source buffer of 8-bit grayscale pixels.
 * @param len Number of pixels to convert.
 */
static inline void
_pixels_gry8_to_argb_convert(uint32_t *dst, const uint8_t *src, int len)
{
   int k;
   for (k = 0; k < len; k++)
     {
        uint8_t s = *src++;
        *dst++ = DRAW_ARGB_JOIN(s, s, s, s);
     }
}

/**
 * @internal
 * @brief Clears the pixel data associated with the software buffer.
 *
 * If the buffer owns the pixel data (i.e., it was allocated by the buffer
 * and not set externally with nofree=EINA_TRUE), this function frees the memory.
 * It also resets pixel-related metadata.
 * This function should not be called if there are active memory mappings.
 *
 * @param obj The Ector_Software_Buffer object.
 * @param pd The private data of the Ector_Software_Buffer_Base.
 */
EOLIAN static void
_ector_software_buffer_base_pixels_clear(Eo *obj EINA_UNUSED, Ector_Software_Buffer_Base_Data *pd)
{
   if (!pd->pixels.u8) return;

   if (pd->internal.maps)
     {
        ERR("Can not call pixels_clear when the buffer is mapped.");
        return;
     }

//TODO: Neccessary?
//   efl_event_callback_call(obj, ECTOR_BUFFER_EVENT_DETACHED, pd->pixels.u8);

   if (!pd->nofree) free(pd->pixels.u8);
   pd->pixels.u8 = NULL;
   pd->nofree = EINA_FALSE;
}

/**
 * @internal
 * @brief Retrieves direct access to the buffer's pixel data and its properties.
 *
 * This function provides a pointer to the raw pixel data, along with its
 * dimensions and stride.
 *
 * @param obj The Ector_Buffer object.
 * @param pd The private data of the Ector_Software_Buffer_Base.
 * @param pixels Output parameter for the pointer to the pixel data.
 *               Example: `uint8_t **mypixels;`
 * @param width Output parameter for the width of the buffer in pixels.
 * @param height Output parameter for the height of the buffer in pixels.
 * @param stride Output parameter for the stride of the buffer in bytes.
 * @return EINA_TRUE if the buffer is writable, EINA_FALSE otherwise.
 *         Note: This return value indicates the writability status set
 *         during `ector_buffer_pixels_set`, not necessarily if the current
 *         data pointer itself is writable (e.g. if it points to const data).
 */
EOLIAN static Eina_Bool
_ector_software_buffer_base_ector_buffer_pixels_get(Eo *obj EINA_UNUSED, Ector_Software_Buffer_Base_Data *pd,
                                                    void **pixels, int* width, int* height, int* stride)
{
   if (pixels) *pixels = pd->pixels.u8;
   if (width) *width = pd->generic->w;
   if (height) *height = pd->generic->h;
   if (stride) *stride = pd->stride;
   return pd->writable;
}

/**
 * @internal
 * @brief Sets or replaces the pixel data of the software buffer.
 *
 * This function allows associating an external memory region as the buffer's
 * pixel data, or allocating a new one if `pixels` is NULL.
 * It's not allowed if there are active memory mappings.
 *
 * @param obj The Ector_Buffer object.
 * @param pd The private data of the Ector_Software_Buffer_Base.
 * @param pixels Pointer to the pixel data to use. If NULL, new memory is allocated.
 *               Example: `uint8_t *my_pixel_data;`
 * @param width Width of the buffer in pixels.
 * @param height Height of the buffer in pixels.
 * @param stride Stride of the buffer in bytes. If 0, it's calculated as `width * pixel_size`.
 * @param cspace Colorspace of the pixel data.
 *               Supported: EFL_GFX_COLORSPACE_ARGB8888, EFL_GFX_COLORSPACE_GRY8.
 * @param writable If EINA_TRUE, the buffer is marked as writable.
 *                 If `pixels` is provided, this flag indicates if the provided
 *                 memory can be written to by Ector.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid parameters,
 *         immutable buffer, active maps).
 */
EOLIAN static Eina_Bool
_ector_software_buffer_base_ector_buffer_pixels_set(Eo *obj, Ector_Software_Buffer_Base_Data *pd,
                                                    void *pixels, int width, int height, int stride,
                                                    Efl_Gfx_Colorspace cspace, Eina_Bool writable)
{
   unsigned int pxs;

#if 0
   if (pd->generic->immutable)
     {
        ERR("This buffer is immutable.");
        return EINA_FALSE;
     }
#endif

   if (pd->internal.maps) return EINA_FALSE;

   if (cspace == EFL_GFX_COLORSPACE_ARGB8888) pxs = 4;
   else if (cspace == EFL_GFX_COLORSPACE_GRY8) pxs = 1;
   else return EINA_FALSE;

   if (stride == 0) stride = width * pxs;
   else if (stride < (int)(width * pxs)) return EINA_FALSE;

   if (pd->pixels.u8 && (pd->pixels.u8 != pixels))
     _ector_software_buffer_base_pixels_clear(obj, pd);

   if (pixels)
     {
        pd->pixels.u8 = pixels;
        pd->nofree = EINA_TRUE;
        pd->writable = !!writable;
     }
   else if (stride > 0 && height > 0)
     {
        pd->pixels.u8 = calloc(stride * height, 1);
        pd->nofree = EINA_FALSE;
        pd->writable = EINA_TRUE;
     }
   pd->generic->w = width;
   pd->generic->h = height;
   pd->generic->cspace = cspace;
   pd->stride = stride;
   pd->pixel_size = pxs;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Maps a region of the buffer's pixel data for direct access.
 *
 * This function provides a pointer to a region of the buffer's pixel data.
 * It handles colorspace conversion and copy-on-write (COW) semantics if requested
 * or necessary.
 *
 * @param obj The Ector_Buffer object.
 * @param pd The private data of the Ector_Software_Buffer_Base.
 * @param length Output parameter for the size of the mapped region in bytes.
 * @param mode Access flags for the mapping (read, write, COW).
 *             Example: `ECTOR_BUFFER_ACCESS_FLAG_READ | ECTOR_BUFFER_ACCESS_FLAG_WRITE`
 * @param x The starting X coordinate of the region to map.
 * @param y The starting Y coordinate of the region to map.
 * @param w The width of the region to map. If 0, maps full width from x.
 * @param h The height of the region to map. If 0, maps full height from y.
 * @param cspace The desired colorspace for the mapped data. If different from
 *               the buffer's native colorspace, conversion will occur.
 *               Supported: EFL_GFX_COLORSPACE_ARGB8888, EFL_GFX_COLORSPACE_GRY8.
 * @param stride Output parameter for the stride of the mapped region in bytes.
 * @return Pointer to the mapped pixel data, or NULL on failure.
 *         The returned pointer must be released using `ector_buffer_unmap`.
 *         Example of returned data structure for ARGB8888:
 *         `uint8_t mapped_pixels[] = {B0, G0, R0, A0, B1, G1, R1, A1, ...};`
 *         For GRY8:
 *         `uint8_t mapped_pixels[] = {GRAY0, GRAY1, ...};`
 */
EOLIAN static void *
_ector_software_buffer_base_ector_buffer_map(Eo *obj EINA_UNUSED, Ector_Software_Buffer_Base_Data *pd,
                                             unsigned int *length, Ector_Buffer_Access_Flag mode,
                                             unsigned int x, unsigned int y, unsigned int w, unsigned int h,
                                             Efl_Gfx_Colorspace cspace EINA_UNUSED, unsigned int *stride)
{
   Ector_Software_Buffer_Map *map = NULL;
   Eina_Bool need_cow = EINA_FALSE;
   unsigned int off, k, dst_stride, pxs, pxs_dest;

   if (!w) w = pd->generic->w;
   if (!h) h = pd->generic->h;

   if (!pd->pixels.u8 || !pd->stride) goto on_fail;

   if (((x + w) > pd->generic->w) || (y + h > pd->generic->h))
     {
        ERR("Invalid region requested: wanted %u,%u %ux%u but image is %ux%u",
            x, y, w, h, pd->generic->w, pd->generic->h);
        goto on_fail;
     }
   if ((mode & ECTOR_BUFFER_ACCESS_FLAG_WRITE) && !pd->writable)
     {
        ERR("Can not map a read-only buffer for writing");
        goto on_fail;
     }

   pxs = (pd->generic->cspace == EFL_GFX_COLORSPACE_ARGB8888) ? 4 : 1;
   if (cspace == EFL_GFX_COLORSPACE_ARGB8888) pxs_dest = 4;
   else if (cspace == EFL_GFX_COLORSPACE_GRY8) pxs_dest = 1;
   else
     {
        ERR("Unsupported colorspace: %u", cspace);
        goto on_fail;
     }

   if ((mode & ECTOR_BUFFER_ACCESS_FLAG_WRITE) &&
       (mode & ECTOR_BUFFER_ACCESS_FLAG_COW))
     {
        EINA_INLIST_FOREACH(pd->internal.maps, map)
          if (map->mode == ECTOR_BUFFER_ACCESS_FLAG_READ)
            {
               need_cow = EINA_TRUE;
               break;
            }
     }

   map = calloc(1, sizeof(Ector_Software_Buffer_Map));
   if (!map) goto on_fail;

   off = (pxs * x) + (pd->stride * y);
   dst_stride = w * pxs_dest;

   map->mode = mode;
   map->cspace = cspace;
   map->stride = dst_stride;
   map->x = x;
   map->y = y;
   map->w = w;
   map->h = h;

   if (cspace != pd->generic->cspace)
     {
        // convert on the fly
        map->size = w * h * pxs_dest;
        map->allocated = EINA_TRUE;
        map->ptr = malloc(map->size);
        if (!map->ptr) goto on_fail;

        if (cspace == EFL_GFX_COLORSPACE_ARGB8888)
          {
             for (k = 0; k < h; k++)
               _pixels_gry8_to_argb_convert((uint32_t *) map->ptr + (k * w), pd->pixels.u8 + off + (k * pd->stride), w);
          }
        else
          {
             for (k = 0; k < h; k++)
               _pixels_argb_to_gry8_convert(map->ptr + (k * w), (uint32_t *) (pd->pixels.u8 + off + (k * pd->stride)), w);
          }
     }
   else if (need_cow)
     {
        // copy-on-write access
        map->size = w * h * pxs_dest;
        map->allocated = EINA_TRUE;
        map->ptr = malloc(map->size);

        if (!map->ptr) goto on_fail;

        for (k = 0; k < h; k++)
          memcpy(map->ptr + k * dst_stride, pd->pixels.u8 + x + (k + y) * pd->stride, dst_stride);
     }
   else
     {
        // direct access, zero-copy
        map->size = (pd->stride * h) - off;
        map->ptr = pd->pixels.u8 + off;
        dst_stride = pd->stride;
     }

   pd->internal.maps = eina_inlist_prepend(pd->internal.maps, EINA_INLIST_GET(map));
   if (length) *length = map->size;
   if (stride) *stride = dst_stride;
   return map->ptr;

on_fail:
   free(map);
   if (length) *length = 0;
   if (stride) *stride = 0;
   return NULL;
}

/**
 * @internal
 * @brief Unmaps a previously mapped region of the buffer's pixel data.
 *
 * This function releases resources associated with a mapping created by
 * `ector_buffer_map`. If the mapping involved an allocation (e.g., for
 * colorspace conversion or COW) and was writable, the changes are written
 * back to the main buffer.
 *
 * @param obj The Ector_Buffer object.
 * @param pd The private data of the Ector_Software_Buffer_Base.
 * @param data Pointer to the mapped data, as returned by `ector_buffer_map`.
 * @param length Length of the mapped data, as returned by `ector_buffer_map`.
 *               Can be (unsigned int) -1 to ignore length check, but this is risky.
 */
EOLIAN static void
_ector_software_buffer_base_ector_buffer_unmap(Eo *obj EINA_UNUSED, Ector_Software_Buffer_Base_Data *pd,
                                                       void *data, unsigned int length)
{
   Ector_Software_Buffer_Map *map;
   if (!data) return;

   EINA_INLIST_FOREACH(pd->internal.maps, map)
     {
        if ((map->ptr == data) && ((map->size == length) || (length == (unsigned int) -1)))
          {
             pd->internal.maps = eina_inlist_remove(pd->internal.maps, EINA_INLIST_GET(map));
             if (map->allocated)
               {
                  if (map->mode & ECTOR_BUFFER_ACCESS_FLAG_WRITE)
                    {
                       unsigned k;

                       if (map->cspace != pd->generic->cspace)
                         {
                            if (pd->generic->cspace == EFL_GFX_COLORSPACE_ARGB8888)
                              {
                                 for (k = 0; k < map->h; k++)
                                   _pixels_gry8_to_argb_convert((uint32_t *) (pd->pixels.u8 + (k + map->y) * pd->stride),
                                                                map->ptr + (k * map->w),
                                                                map->w);
                              }
                            else
                              {
                                 for (k = 0; k < map->h; k++)
                                   _pixels_argb_to_gry8_convert(pd->pixels.u8 + (k + map->y) * pd->stride,
                                                                (uint32_t *) map->ptr + (k * map->w),
                                                                map->w);
                              }
                         }
                       else
                         {
                            for (k = 0; k < map->h; k++)
                              {
                                 memcpy(pd->pixels.u8 + map->x + (k + map->y) * pd->stride,
                                        map->ptr + k * map->stride, map->stride);
                              }
                         }
                    }
                  free(map->ptr);
               }
             free(map);
             return;
          }
     }

   CRI("Tried to unmap a non-mapped region!");
}

/**
 * @internal
 * @brief Gets the capability flags of the software buffer.
 *
 * These flags indicate properties like readability, writability, and
 * whether the buffer can be used as a rendering target.
 *
 * @param obj The Ector_Buffer object.
 * @param pd The private data of the Ector_Software_Buffer_Base.
 * @return A bitmask of Ector_Buffer_Flag values.
 *         Example: `ECTOR_BUFFER_FLAG_CPU_READABLE | ECTOR_BUFFER_FLAG_CPU_WRITABLE`
 */
EOLIAN static Ector_Buffer_Flag
_ector_software_buffer_base_ector_buffer_flags_get(const Eo *obj EINA_UNUSED, Ector_Software_Buffer_Base_Data *pd)
{
   return ECTOR_BUFFER_FLAG_CPU_READABLE |
         ECTOR_BUFFER_FLAG_DRAWABLE |
         ECTOR_BUFFER_FLAG_CPU_READABLE_FAST |
         (pd->writable ? (ECTOR_BUFFER_FLAG_CPU_WRITABLE |
                          ECTOR_BUFFER_FLAG_RENDERABLE |
                          ECTOR_BUFFER_FLAG_CPU_WRITABLE_FAST)
                       : 0);
}

/**
 * @internal
 * @brief Constructor for the Ector_Software_Buffer object.
 *
 * Initializes the Ector_Software_Buffer_Base_Data and links it with the
 * ECTOR_BUFFER_MIXIN data.
 *
 * @param obj The Efl_Object being constructed.
 * @param data Private data for this class (unused here).
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_ector_software_buffer_efl_object_constructor(Eo *obj, void *data EINA_UNUSED)
{
   Ector_Software_Buffer_Base_Data *pd;
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   pd = efl_data_scope_get(obj, ECTOR_SOFTWARE_BUFFER_BASE_MIXIN);
   pd->generic = efl_data_ref(obj, ECTOR_BUFFER_MIXIN);
   pd->generic->eo = obj;
   return obj;
}

/**
 * @internal
 * @brief Destructor for the Ector_Software_Buffer object.
 *
 * Cleans up resources, including freeing pixel data and unreferencing
 * associated mixin data. It also checks for any lingering mappings,
 * which would indicate a programming error.
 *
 * @param obj The Efl_Object being destructed.
 * @param data Private data for this class (unused here).
 */
EOLIAN static void
_ector_software_buffer_efl_object_destructor(Eo *obj, void *data EINA_UNUSED)
{
   Ector_Software_Buffer_Base_Data *pd = efl_data_scope_get(obj, ECTOR_SOFTWARE_BUFFER_BASE_MIXIN);
   _ector_software_buffer_base_pixels_clear(obj, pd);
   efl_data_unref(obj, pd->generic);
   efl_destructor(efl_super(obj, MY_CLASS));
   if (pd->internal.maps)
     ERR("Pixel data is still mapped during destroy!");
}

#include "ector_software_buffer.eo.c"
#include "ector_software_buffer_base.eo.c"
