
#include "evas_image_private.h"
#include "evas_image_eo.h"

#define EVAS_IMAGE_API(_o, ...) do { \
   if (EINA_UNLIKELY(!efl_isa(_o, EFL_CANVAS_IMAGE_INTERNAL_CLASS))) { \
      EINA_SAFETY_ERROR("object is not an image!"); \
      return __VA_ARGS__; \
   } } while (0)

#define EVAS_IMAGE_LEGACY_API(_o, ...) do { \
   EVAS_OBJECT_LEGACY_API(_o, __VA_ARGS__); \
   EVAS_IMAGE_API(_o, __VA_ARGS__); \
   } while (0)

typedef struct _Evas_Image_Legacy_Pixels_Entry Evas_Image_Legacy_Pixels_Entry;

/**
 * @internal
 * @brief Structure to hold an Evas image object and its associated image data
 *        for deferred freeing when pixels are obtained via evas_object_image_data_get.
 */
struct _Evas_Image_Legacy_Pixels_Entry
{
   Eo    *object; /**< The Evas image object. */
   void  *image;  /**< The image data pointer that needs to be freed. */
};

/**
 * @brief Adds a new image object to the given Evas canvas.
 *
 * This function creates a new image object. Initially, the image will be
 * empty and have no source file or data. Its fill_auto property will be
 * set to EINA_FALSE, meaning it won't automatically fill the object's
 * geometry.
 *
 * @param eo_e The Evas canvas to add the new image to.
 * @return A handle to the new image object, or @c NULL on failure.
 * @see evas_object_image_file_set()
 * @see evas_object_image_data_set()
 * @see evas_object_image_filled_set()
 */
EVAS_API Evas_Object *
evas_object_image_add(Evas *eo_e)
{
   eo_e = evas_find(eo_e);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(eo_e, EVAS_CANVAS_CLASS), NULL);
   return efl_add(EVAS_IMAGE_CLASS, eo_e,
                 efl_gfx_fill_auto_set(efl_added, EINA_FALSE),
                 efl_canvas_object_legacy_ctor(efl_added));
}

/**
 * @brief Adds a new image object to the given Evas canvas, with fill_auto enabled.
 *
 * This function creates a new image object. Initially, the image will be
 * empty and have no source file or data. Its fill_auto property will be
 * set to EINA_TRUE by default, meaning it will automatically fill the
 * object's geometry.
 *
 * @param eo_e The Evas canvas to add the new image to.
 * @return A handle to the new image object, or @c NULL on failure.
 * @see evas_object_image_file_set()
 * @see evas_object_image_data_set()
 * @see evas_object_image_filled_get()
 */
EVAS_API Evas_Object *
evas_object_image_filled_add(Evas *eo_e)
{
   eo_e = evas_find(eo_e);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(eo_e, EVAS_CANVAS_CLASS), NULL);
   return efl_add(EVAS_IMAGE_CLASS, eo_e,
                 efl_canvas_object_legacy_ctor(efl_added));
}

/**
 * @brief Sets the source of an image object from in-memory data.
 *
 * This function loads an image from a block of memory. The Evas library
 * will try to determine the image format from the data itself.
 *
 * @param eo_obj The image object.
 * @param data A pointer to the memory containing the image file data.
 * @param size The size of the data in bytes.
 * @param format The format of the image data (e.g., "png", "jpeg"). This parameter is currently unused.
 * @param key An optional key for caching. If @c NULL, no key is used.
 */
EVAS_API void
evas_object_image_memfile_set(Evas_Object *eo_obj, void *data, int size, char *format EINA_UNUSED, char *key)
{
   Eina_File *f;

   EVAS_IMAGE_API(eo_obj);

   f = eina_file_virtualize(NULL, data, size, EINA_TRUE);
   if (!f) return ;
   efl_file_simple_mmap_load(eo_obj, f, key);
   eina_file_close(f); // close matching open OK
}

/**
 * @brief Sets the fill region for an image object.
 *
 * This function defines how an image is displayed within its object's
 * boundaries if it's not set to fill the entire object (see
 * evas_object_image_filled_set()). The fill parameters (x, y, w, h)
 * are relative to the top-left corner of the image itself, not the object.
 * These values can be larger or smaller than the actual image dimensions.
 *
 * @param obj The image object.
 * @param x The horizontal offset of the fill region.
 * @param y The vertical offset of the fill region.
 * @param w The width of the fill region.
 * @param h The height of the fill region.
 */
EVAS_API void
evas_object_image_fill_set(Evas_Object *obj,
                           Evas_Coord x, Evas_Coord y,
                           Evas_Coord w, Evas_Coord h)
{
   EVAS_IMAGE_API(obj);
   _evas_image_fill_set(obj, efl_data_scope_get(obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS), x, y, w, h);
}

/**
 * @brief Manages asynchronous preloading of an image object's data.
 *
 * If @p cancel is @c EINA_FALSE, this function initiates asynchronous loading
 * of the image data. This is useful for loading images in the background
 * without blocking the main loop.
 * If @p cancel is @c EINA_TRUE, any ongoing asynchronous load for this image
 * object is cancelled.
 *
 * @param eo_obj The image object.
 * @param cancel If @c EINA_TRUE, cancel preloading. If @c EINA_FALSE, start preloading.
 */
EVAS_API void
evas_object_image_preload(Evas_Object *eo_obj, Eina_Bool cancel)
{
   EVAS_IMAGE_API(eo_obj);
   if (cancel) _evas_image_load_async_cancel(eo_obj);
   else _evas_image_load_async_start(eo_obj);
}

/**
 * @brief Gets whether the image object is set to auto-fill its area.
 *
 * If auto-fill is enabled, the image will be stretched or shrunk to fit
 * the object's geometry. If disabled, the fill region set by
 * evas_object_image_fill_set() is used.
 *
 * @param eo_obj The image object.
 * @return @c EINA_TRUE if auto-fill is enabled, @c EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_object_image_filled_get(const Evas_Object *eo_obj)
{
   EVAS_IMAGE_API(eo_obj, EINA_FALSE);
   return efl_gfx_fill_auto_get(eo_obj);
}

/**
 * @brief Sets whether the image object should auto-fill its area.
 *
 * @param eo_obj The image object.
 * @param value @c EINA_TRUE to enable auto-fill, @c EINA_FALSE to disable.
 * @see evas_object_image_filled_get()
 * @see evas_object_image_fill_set()
 */
EVAS_API void
evas_object_image_filled_set(Evas_Object *eo_obj, Eina_Bool value)
{
   EVAS_IMAGE_API(eo_obj);
   efl_gfx_fill_auto_set(eo_obj, value);
}

/**
 * @brief Gets the fill region for an image object.
 *
 * This retrieves the fill parameters previously set by
 * evas_object_image_fill_set().
 *
 * @param obj The image object.
 * @param x Pointer to store the horizontal offset of the fill region. Can be @c NULL.
 * @param y Pointer to store the vertical offset of the fill region. Can be @c NULL.
 * @param w Pointer to store the width of the fill region. Can be @c NULL.
 * @param h Pointer to store the height of the fill region. Can be @c NULL.
 * @see evas_object_image_fill_set()
 */
EVAS_API void
evas_object_image_fill_get(const Evas_Object *obj,
                           Evas_Coord *x, Evas_Coord *y,
                           Evas_Coord *w, Evas_Coord *h)
{
   Eina_Rect r;

   EVAS_IMAGE_API(obj);
   r = efl_gfx_fill_get(obj);
   if (x) *x = r.x;
   if (y) *y = r.y;
   if (w) *w = r.w;
   if (h) *h = r.h;
}

/**
 * @brief Sets whether the image object has alpha channel data.
 *
 * This function informs Evas whether the loaded image data contains an
 * alpha channel. This affects how the image is blended and rendered.
 *
 * @param obj The image object.
 * @param alpha @c EINA_TRUE if the image has alpha, @c EINA_FALSE otherwise.
 */
EVAS_API void
evas_object_image_alpha_set(Evas_Object *obj, Eina_Bool alpha)
{
   EVAS_IMAGE_API(obj);
   efl_gfx_buffer_alpha_set(obj, alpha);
}

/**
 * @brief Gets whether the image object has alpha channel data.
 *
 * @param obj The image object.
 * @return @c EINA_TRUE if the image has alpha, @c EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_object_image_alpha_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, EINA_FALSE);
   return efl_gfx_buffer_alpha_get(obj);
}

/**
 * @brief Sets the border region of an image object.
 *
 * The border defines parts of the image that are not scaled when the
 * image is resized. This is useful for creating scalable frames or
 * backgrounds where corners and edges should maintain their appearance.
 * The values are in pixels from the respective edges of the image.
 *
 * @param obj The image object.
 * @param l Left border size.
 * @param r Right border size.
 * @param t Top border size.
 * @param b Bottom border size.
 */
EVAS_API void
evas_object_image_border_set(Evas_Object *obj, int l, int r, int t, int b)
{
   EVAS_IMAGE_API(obj);
   efl_gfx_image_border_insets_set(obj, l, r, t, b);
}

/**
 * @brief Gets the border region of an image object.
 *
 * @param obj The image object.
 * @param l Pointer to store the left border size. Can be @c NULL.
 * @param r Pointer to store the right border size. Can be @c NULL.
 * @param t Pointer to store the top border size. Can be @c NULL.
 * @param b Pointer to store the bottom border size. Can be @c NULL.
 */
EVAS_API void
evas_object_image_border_get(const Evas_Object *obj, int *l, int *r, int *t, int *b)
{
   EVAS_IMAGE_API(obj);
   efl_gfx_image_border_insets_get(obj, l, r, t, b);
}

/**
 * @brief Sets the scaling factor for the image border.
 *
 * This value multiplies the border insets defined by evas_object_image_border_set().
 * A scale of 1.0 means the border insets are used as is.
 *
 * @param obj The image object.
 * @param scale The scaling factor.
 */
EVAS_API void
evas_object_image_border_scale_set(Evas_Object *obj, double scale)
{
   EVAS_IMAGE_API(obj);
   efl_gfx_image_border_insets_scale_set(obj, scale);
}

/**
 * @brief Gets the scaling factor for the image border.
 *
 * @param obj The image object.
 * @return The scaling factor.
 */
EVAS_API double
evas_object_image_border_scale_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, 0.0);
   return efl_gfx_image_border_insets_scale_get(obj);
}

/**
 * @brief Sets the fill mode for the center part of a bordered image.
 *
 * When an image has a border set (see evas_object_image_border_set()),
 * this function determines how the central area (the part not covered by
 * the border) is filled when the image is scaled.
 *
 * @param obj The image object.
 * @param fill The border fill mode.
 *             Example: @c EVAS_BORDER_FILL_DEFAULT, @c EVAS_BORDER_FILL_SOLID.
 */
EVAS_API void
evas_object_image_border_center_fill_set(Evas_Object *obj, Evas_Border_Fill_Mode fill)
{
   EVAS_IMAGE_API(obj);
   efl_gfx_image_center_fill_mode_set(obj, (Efl_Gfx_Center_Fill_Mode) fill);
}

/**
 * @brief Gets the fill mode for the center part of a bordered image.
 *
 * @param obj The image object.
 * @return The border fill mode.
 */
EVAS_API Evas_Border_Fill_Mode
evas_object_image_border_center_fill_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, EVAS_BORDER_FILL_NONE);
   return (Evas_Border_Fill_Mode) efl_gfx_image_center_fill_mode_get(obj);
}

/**
 * @brief Gets the original (unscaled) size of the image data.
 *
 * This function retrieves the dimensions of the image as it was loaded,
 * before any scaling or filling is applied by the object's geometry.
 *
 * @param obj The image object.
 * @param w Pointer to store the width of the image. Can be @c NULL.
 * @param h Pointer to store the height of the image. Can be @c NULL.
 */
EVAS_API void
evas_object_image_size_get(const Evas_Object *obj, int *w, int *h)
{
   Eina_Size2D sz;
   EVAS_IMAGE_API(obj);
   sz = efl_gfx_view_size_get(obj);
   if (w) *w = sz.w;
   if (h) *h = sz.h;
}

/**
 * @brief Gets the colorspace of the image data.
 *
 * @param obj The image object.
 * @return The colorspace of the image.
 *         Example: @c EVAS_COLORSPACE_ARGB8888, @c EVAS_COLORSPACE_YCBCR422P601_PL.
 */
EVAS_API Evas_Colorspace
evas_object_image_colorspace_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, EVAS_COLORSPACE_ARGB8888);
   return (Evas_Colorspace) efl_gfx_buffer_colorspace_get(obj);
}

/**
 * @brief Gets the stride (row length in bytes) of the image data.
 *
 * The stride is the number of bytes from the start of one row of pixels
 * to the start of the next row. This may be larger than width * bytes_per_pixel
 * due to padding.
 *
 * @param obj The image object.
 * @return The stride of the image data in bytes.
 */
EVAS_API int
evas_object_image_stride_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, 0);
   Evas_Image_Data *o = efl_data_scope_get(obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   return o->cur->image.stride;
}

/**
 * @brief Marks a region of the image data as updated.
 *
 * This function informs Evas that a portion of the image's pixel data
 * (obtained via evas_object_image_data_get() for writing) has been modified.
 * Evas will then re-render that part of the image.
 * The coordinates are relative to the image itself.
 *
 * @param obj The image object.
 * @param x The horizontal offset of the updated region.
 * @param y The vertical offset of the updated region.
 * @param w The width of the updated region.
 * @param h The height of the updated region.
 */
EVAS_API void
evas_object_image_data_update_add(Evas_Object *obj, int x, int y, int w, int h)
{
   Eina_Rect r;

   EVAS_IMAGE_API(obj);
   r = EINA_RECT(x, y, w, h);
   efl_gfx_buffer_update_add(obj, &r);
}

/**
 * @brief Sets the source file for an image object.
 *
 * This function loads an image from the specified file path.
 *
 * @param obj The image object.
 * @param file The path to the image file.
 * @param key An optional key for caching. If @c NULL, the file path is used as the key.
 */
EVAS_API void
evas_object_image_file_set(Evas_Object *obj, const char *file, const char *key)
{
   EVAS_IMAGE_API(obj);
   efl_file_simple_load(obj, file, key);
}

/**
 * @brief Gets the source file and key for an image object.
 *
 * @param obj The image object.
 * @param file Pointer to store the image file path. Can be @c NULL.
 * @param key Pointer to store the image key. Can be @c NULL.
 */
EVAS_API void
evas_object_image_file_get(const Evas_Object *obj, const char **file, const char **key)
{
   EVAS_IMAGE_API(obj);
   efl_file_simple_get(obj, file, key);
}

/**
 * @brief Sets the source of an image object from a memory-mapped file.
 *
 * This function loads an image from an Eina_File that has been memory-mapped.
 *
 * @param obj The image object.
 * @param f The memory-mapped Eina_File.
 * @param key An optional key for caching.
 */
EVAS_API void
evas_object_image_mmap_set(Evas_Object *obj, const Eina_File *f, const char *key)
{
   EVAS_IMAGE_API(obj);
   efl_file_simple_mmap_load(obj, f, key);
}

/**
 * @brief Gets the memory-mapped file and key for an image object.
 *
 * @param obj The image object.
 * @param f Pointer to store the Eina_File. Can be @c NULL.
 * @param key Pointer to store the image key. Can be @c NULL.
 */
EVAS_API void
evas_object_image_mmap_get(const Evas_Object *obj, const Eina_File **f, const char **key)
{
   EVAS_IMAGE_API(obj);
   efl_file_simple_mmap_get(obj, f, key);
}

/**
 * @brief Saves the image object's pixel data to a file.
 *
 * @param obj The image object.
 * @param file The path to the file where the image will be saved.
 * @param key The key for the image format (e.g., "png", "jpeg"). If @c NULL,
 *            Evas attempts to guess from the filename extension.
 * @param flags Optional flags for saving, specific to the image format.
 *              Example: "quality=80 compress=9 encoding=lossless"
 *              - "quality=VALUE": For JPEG, sets quality (0-100). Default 80.
 *              - "compress=VALUE": For PNG, sets compression (0-9). Default 9.
 *              - "encoding=VALUE": For WEBP, "lossy", "lossless".
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EVAS_API Eina_Bool
evas_object_image_save(const Evas_Object *obj, const char *file, const char *key, const char *flags)
{
   char *encoding = NULL;
   Efl_File_Save_Info info;
   Eina_Error ret;

   EVAS_IMAGE_API(obj, EINA_FALSE);

   if (flags)
     {
        char *p, *pp;
        char *tflags;
        int quality = 80, compress = 9;

        tflags = alloca(strlen(flags) + 1);
        strcpy(tflags, flags);
        p = tflags;
        while (p)
          {
             pp = strchr(p, ' ');
             if (pp) *pp = 0;
             sscanf(p, "quality=%4i", &quality);
             sscanf(p, "compress=%4i", &compress);
             sscanf(p, "encoding=%ms", &encoding);
             if (pp) p = pp + 1;
             else break;
          }
        info.quality = quality;
        info.compression = compress;
        info.encoding = encoding;

     }
   ret = efl_file_save(obj, file, key, flags ? &info : NULL);
   free(encoding);
   return ret;
}

/**
 * @brief Gets whether an image object is animated.
 *
 * @param obj The image object.
 * @return @c EINA_TRUE if the image is animated (e.g., GIF), @c EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_object_image_animated_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, EINA_FALSE);
   return _evas_image_animated_get(obj);
}

/**
 * @brief Sets the current frame of an animated image.
 *
 * For animated images (like GIFs), this function sets the frame to be displayed.
 * Frame indexing starts from 0.
 *
 * @param obj The image object.
 * @param frame_index The index of the frame to display.
 */
EVAS_API void
evas_object_image_animated_frame_set(Evas_Object *obj, int frame_index)
{
   EVAS_IMAGE_API(obj);
   _evas_image_animated_frame_set(obj, frame_index);
}

/**
 * @brief Gets the current frame index of an animated image.
 *
 * @param obj The image object.
 * @return The current frame index.
 */
EVAS_API int
evas_object_image_animated_frame_get(Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, 0);
   return _evas_image_animated_frame_get(obj);
}

/**
 * @brief Gets the total number of frames in an animated image.
 *
 * @param obj The image object.
 * @return The total number of frames.
 */
EVAS_API int
evas_object_image_animated_frame_count_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, 0);
   return _evas_image_animated_frame_count_get(obj);
}

/**
 * @brief Gets the loop type of an animated image.
 *
 * This indicates how the animation should loop (e.g., loop forever,
 * play once).
 *
 * @param obj The image object.
 * @return The loop type.
 *         Example: @c EVAS_IMAGE_ANIMATED_HINT_LOOP, @c EVAS_IMAGE_ANIMATED_HINT_NONE.
 */
EVAS_API Evas_Image_Animated_Loop_Hint
evas_object_image_animated_loop_type_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, EVAS_IMAGE_ANIMATED_HINT_NONE);
   return (Evas_Image_Animated_Loop_Hint) _evas_image_animated_loop_type_get(obj);
}

/**
 * @brief Gets the loop count for an animated image.
 *
 * If the loop type is set to play a specific number of times, this
 * function returns that count.
 *
 * @param obj The image object.
 * @return The loop count.
 */
EVAS_API int
evas_object_image_animated_loop_count_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, 0);
   return _evas_image_animated_loop_count_get(obj);
}

/**
 * @brief Gets the duration of a specific frame or sequence of frames in an animated image.
 *
 * @param obj The image object.
 * @param start_frame The starting frame index.
 * @param frame_num The number of frames from @p start_frame to consider.
 * @return The total duration in seconds for the specified frame(s).
 */
EVAS_API double
evas_object_image_animated_frame_duration_get(const Evas_Object *obj, int start_frame, int frame_num)
{
   EVAS_IMAGE_API(obj, 0.0);
   return _evas_image_animated_frame_duration_get(obj, start_frame, frame_num);
}

/**
 * @brief Sets the desired load dimensions for an image.
 *
 * This hints to the image loader to load the image at a specific size,
 * potentially saving memory if the original image is much larger.
 * Not all loaders support this.
 *
 * @param obj The image object.
 * @param w The desired width.
 * @param h The desired height.
 */
EVAS_API void
evas_object_image_load_size_set(Evas_Object *obj, int w, int h)
{
   EVAS_IMAGE_API(obj);
   _evas_image_load_size_set(obj, w, h);
}

/**
 * @brief Gets the desired load dimensions for an image.
 *
 * @param obj The image object.
 * @param w Pointer to store the desired width. Can be @c NULL.
 * @param h Pointer to store the desired height. Can be @c NULL.
 */
EVAS_API void
evas_object_image_load_size_get(const Evas_Object *obj, int *w, int *h)
{
   EVAS_IMAGE_API(obj);
   _evas_image_load_size_get(obj, w, h);
}

/**
 * @brief Sets the DPI for loading an image.
 *
 * This hints to the image loader about the dots-per-inch (DPI) at which
 * the image should be loaded. This can affect how some image formats
 * (like SVG) are rasterized.
 *
 * @param obj The image object.
 * @param dpi The desired DPI.
 */
EVAS_API void
evas_object_image_load_dpi_set(Evas_Object *obj, double dpi)
{
   EVAS_IMAGE_API(obj);
   _evas_image_load_dpi_set(obj, dpi);
}

/**
 * @brief Gets the DPI for loading an image.
 *
 * @param obj The image object.
 * @return The DPI value.
 */
EVAS_API double
evas_object_image_load_dpi_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, 0.0);
   return _evas_image_load_dpi_get(obj);
}

/**
 * @brief Sets a specific region of an image to be loaded.
 *
 * This hints to the image loader to only load a sub-rectangle of the
 * image. Not all loaders support this.
 *
 * @param obj The image object.
 * @param x The horizontal offset of the region.
 * @param y The vertical offset of the region.
 * @param w The width of the region.
 * @param h The height of the region.
 */
EVAS_API void
evas_object_image_load_region_set(Evas_Object *obj, int x, int y, int w, int h)
{
   EVAS_IMAGE_API(obj);
   _evas_image_load_region_set(obj, x, y, w, h);
}

/**
 * @brief Gets the region of an image to be loaded.
 *
 * @param obj The image object.
 * @param x Pointer to store the horizontal offset. Can be @c NULL.
 * @param y Pointer to store the vertical offset. Can be @c NULL.
 * @param w Pointer to store the width. Can be @c NULL.
 * @param h Pointer to store the height. Can be @c NULL.
 */
EVAS_API void
evas_object_image_load_region_get(const Evas_Object *obj, int *x, int *y, int *w, int *h)
{
   EVAS_IMAGE_API(obj);
   _evas_image_load_region_get(obj, x, y, w, h);
}

/**
 * @brief Gets whether the image loader supports region loading.
 *
 * @param obj The image object.
 * @return @c EINA_TRUE if region loading is supported, @c EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_object_image_region_support_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, EINA_FALSE);
   return _evas_image_load_region_support_get(obj);
}

/**
 * @brief Sets whether to apply EXIF orientation data when loading an image.
 *
 * If enabled, Evas will attempt to read orientation tags (e.g., from EXIF
 * data in JPEGs) and automatically rotate the image accordingly during load.
 *
 * @param obj The image object.
 * @param enable @c EINA_TRUE to enable orientation handling, @c EINA_FALSE to disable.
 */
EVAS_API void
evas_object_image_load_orientation_set(Evas_Object *obj, Eina_Bool enable)
{
   EVAS_IMAGE_API(obj);
   _evas_image_load_orientation_set(obj, enable);
}

/**
 * @brief Gets whether EXIF orientation data is applied when loading an image.
 *
 * @param obj The image object.
 * @return @c EINA_TRUE if orientation handling is enabled, @c EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_object_image_load_orientation_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, EINA_FALSE);
   return _evas_image_load_orientation_get(obj);
}

/**
 * @brief Sets the scale-down factor for loading an image.
 *
 * This hints to the image loader to load the image at a reduced resolution.
 * For example, a @p scale_down value of 2 will attempt to load the image
 * at half its width and height.
 *
 * @param obj The image object.
 * @param scale_down The scale-down factor (e.g., 1, 2, 4, 8).
 */
EVAS_API void
evas_object_image_load_scale_down_set(Evas_Object *obj, int scale_down)
{
   EVAS_IMAGE_API(obj);
   _evas_image_load_scale_down_set(obj, scale_down);
}

/**
 * @brief Gets the scale-down factor for loading an image.
 *
 * @param obj The image object.
 * @return The scale-down factor. Default is 1.
 */
EVAS_API int
evas_object_image_load_scale_down_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, 1);
   return _evas_image_load_scale_down_get(obj);
}

/**
 * @brief Sets whether to skip loading image header information.
 *
 * If true, the loader might skip reading metadata, potentially speeding up
 * the initial phase of loading for certain formats or use cases.
 * This is an advanced option and its effect depends on the loader.
 *
 * @param obj The image object.
 * @param skip @c EINA_TRUE to skip header loading, @c EINA_FALSE otherwise.
 */
EVAS_API void
evas_object_image_load_head_skip_set(Evas_Object *obj, Eina_Bool skip)
{
   EVAS_IMAGE_API(obj);
   _evas_image_load_head_skip_set(obj, skip);
}

/**
 * @brief Gets whether image header information loading is skipped.
 *
 * @param obj The image object.
 * @return @c EINA_TRUE if header loading is skipped, @c EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_object_image_load_head_skip_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, EINA_FALSE);
   return _evas_image_load_head_skip_get(obj);
}

/**
 * @brief Gets the last error that occurred during image loading.
 *
 * @param obj The image object.
 * @return The load error code.
 *         Example: @c EVAS_LOAD_ERROR_NONE, @c EVAS_LOAD_ERROR_GENERIC.
 */
EVAS_API Evas_Load_Error
evas_object_image_load_error_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, EVAS_LOAD_ERROR_GENERIC);
   return _efl_gfx_image_load_error_to_evas_load_error(efl_gfx_image_load_error_get(obj));
}

/**
 * @brief Sets whether smooth scaling should be used for the image.
 *
 * Smooth scaling generally produces better visual results when an image
 * is scaled up or down, but may be slower.
 *
 * @param obj The image object.
 * @param smooth_scale @c EINA_TRUE to enable smooth scaling, @c EINA_FALSE for rough scaling.
 */
EVAS_API void
evas_object_image_smooth_scale_set(Evas_Object *obj, Eina_Bool smooth_scale)
{
   EVAS_IMAGE_API(obj);
   efl_gfx_image_smooth_scale_set(obj, smooth_scale);
}

/**
 * @brief Gets whether smooth scaling is used for the image.
 *
 * @param obj The image object.
 * @return @c EINA_TRUE if smooth scaling is enabled, @c EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_object_image_smooth_scale_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, EINA_FALSE);
   return efl_gfx_image_smooth_scale_get(obj);
}

/**
 * @brief Sets the orientation of the image.
 *
 * This function allows manual rotation/flipping of the image content
 * without reloading the image. This is different from EXIF orientation
 * which is applied at load time.
 *
 * @param obj The image object.
 * @param orient The desired orientation.
 *               Example: @c EVAS_IMAGE_ORIENT_0, @c EVAS_IMAGE_ORIENT_90.
 */
EVAS_API void
evas_object_image_orient_set(Evas_Object *obj, Evas_Image_Orient orient)
{
   EVAS_IMAGE_API(obj);

   Evas_Image_Data *o = efl_data_scope_get(obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   _evas_image_orientation_set(obj, o, orient);
}

/**
 * @brief Gets the current orientation of the image.
 *
 * @param obj The image object.
 * @return The current image orientation.
 */
EVAS_API Evas_Image_Orient
evas_object_image_orient_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, EVAS_IMAGE_ORIENT_NONE);

   Evas_Image_Data *o = efl_data_scope_get(obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   return o->cur->orient;
}

/**
 * @brief Sets whether the image object is a snapshot image.
 *
 * A snapshot image is typically a render of another part of the Evas scene.
 * This property influences how it's handled internally.
 *
 * @param eo The image object.
 * @param s @c EINA_TRUE if it's a snapshot, @c EINA_FALSE otherwise.
 */
EVAS_API void
evas_object_image_snapshot_set(Evas_Object *eo, Eina_Bool s)
{
   EVAS_IMAGE_API(eo);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo, EFL_CANVAS_OBJECT_CLASS);

   if (obj->cur->snapshot == s) return;

   EINA_COW_STATE_WRITE_BEGIN(obj, state_write, cur)
     state_write->snapshot = !!s;
   EINA_COW_STATE_WRITE_END(obj, state_write, cur);
}

/**
 * @brief Gets whether the image object is a snapshot image.
 *
 * @param eo The image object.
 * @return @c EINA_TRUE if it's a snapshot, @c EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_object_image_snapshot_get(const Evas_Object *eo)
{
   EVAS_IMAGE_API(eo, EINA_FALSE);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo, EFL_CANVAS_OBJECT_CLASS);
   return obj->cur->snapshot;
}

/**
 * @brief Sets another Evas object as the source for this image object (proxy).
 *
 * This turns the image object into a proxy, displaying the content of the
 * @p src object. Changes to the source object will be reflected in the proxy.
 *
 * @param eo The image object (proxy).
 * @param src The Evas object to use as the source. Pass @c NULL to unset.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EVAS_API Eina_Bool
evas_object_image_source_set(Evas_Object *eo, Evas_Object *src)
{
   EVAS_IMAGE_API(eo, EINA_FALSE);
   return _evas_image_proxy_source_set(eo, src);
}

/**
 * @brief Gets the source object for a proxy image.
 *
 * @param eo The image object (proxy).
 * @return The source Evas object, or @c NULL if not a proxy or no source is set.
 */
EVAS_API Evas_Object *
evas_object_image_source_get(const Evas_Object *eo)
{
   EVAS_IMAGE_API(eo, NULL);
   return _evas_image_proxy_source_get(eo);
}

/**
 * @brief Unsets the source object for a proxy image.
 *
 * This is equivalent to calling evas_object_image_source_set() with @c NULL
 * as the source.
 *
 * @param eo_obj The image object (proxy).
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EVAS_API Eina_Bool
evas_object_image_source_unset(Evas_Object *eo_obj)
{
   EVAS_IMAGE_API(eo_obj, EINA_FALSE);
   return _evas_image_proxy_source_set(eo_obj, NULL);
}

/**
 * @brief Sets whether a proxy image should clip its content to the source object's geometry.
 *
 * @param eo The image object (proxy).
 * @param source_clip @c EINA_TRUE to enable source clipping, @c EINA_FALSE otherwise.
 */
EVAS_API void
evas_object_image_source_clip_set(Evas_Object *eo, Eina_Bool source_clip)
{
   EVAS_IMAGE_API(eo);
   _evas_image_proxy_source_clip_set(eo, source_clip);
}

/**
 * @brief Gets whether a proxy image clips its content to the source object's geometry.
 *
 * @param eo The image object (proxy).
 * @return @c EINA_TRUE if source clipping is enabled, @c EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_object_image_source_clip_get(const Evas_Object *eo)
{
   EVAS_IMAGE_API(eo, EINA_FALSE);
   return _evas_image_proxy_source_clip_get(eo);
}

/**
 * @brief Sets whether events on a proxy image should be repeated to its source object.
 *
 * @param eo The image object (proxy).
 * @param repeat @c EINA_TRUE to repeat events, @c EINA_FALSE otherwise.
 */
EVAS_API void
evas_object_image_source_events_set(Evas_Object *eo, Eina_Bool repeat)
{
   EVAS_IMAGE_API(eo);
   _evas_image_proxy_source_events_set(eo, repeat);
}

/**
 * @brief Gets whether events on a proxy image are repeated to its source object.
 *
 * @param eo The image object (proxy).
 * @return @c EINA_TRUE if events are repeated, @c EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_object_image_source_events_get(const Evas_Object *eo)
{
   EVAS_IMAGE_API(eo, EINA_FALSE);
   return _evas_image_proxy_source_events_get(eo);
}

/**
 * @brief Sets a hint about the content of the image.
 *
 * This hint can be used by Evas or underlying engines to optimize rendering
 * or caching strategies. For example, hinting that an image is dynamic
 * might change how it's cached.
 *
 * @param obj The image object.
 * @param hint The content hint.
 *             Example: @c EVAS_IMAGE_CONTENT_HINT_DYNAMIC, @c EVAS_IMAGE_CONTENT_HINT_STATIC.
 */
EVAS_API void
evas_object_image_content_hint_set(Evas_Object *obj, Evas_Image_Content_Hint hint)
{
   EVAS_IMAGE_API(obj);
   efl_gfx_image_content_hint_set(obj, (Efl_Gfx_Image_Content_Hint)hint);
}

/**
 * @brief Gets the content hint for the image.
 *
 * @param obj The image object.
 * @return The content hint.
 */
EVAS_API Evas_Image_Content_Hint
evas_object_image_content_hint_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, EVAS_IMAGE_CONTENT_HINT_NONE);
   return (Evas_Image_Content_Hint)efl_gfx_image_content_hint_get(obj);
}

/**
 * @brief Sets a hint about how the image should be scaled.
 *
 * This hint can influence the scaling algorithm or quality.
 *
 * @param obj The image object.
 * @param hint The scale hint.
 *             Example: @c EVAS_IMAGE_SCALE_HINT_STATIC, @c EVAS_IMAGE_SCALE_HINT_DYNAMIC.
 */
EVAS_API void
evas_object_image_scale_hint_set(Evas_Object *obj, Evas_Image_Scale_Hint hint)
{
   EVAS_IMAGE_API(obj);
   return efl_gfx_image_scale_hint_set(obj, (Efl_Gfx_Image_Scale_Hint) hint);
}

/**
 * @brief Gets the scale hint for the image.
 *
 * @param obj The image object.
 * @return The scale hint.
 */
EVAS_API Evas_Image_Scale_Hint
evas_object_image_scale_hint_get(const Evas_Object *obj)
{
   EVAS_IMAGE_API(obj, EVAS_IMAGE_SCALE_HINT_NONE);
   return (Evas_Image_Scale_Hint) efl_gfx_image_scale_hint_get(obj);
}

/**
 * @brief Sets a native surface for the image object.
 *
 * This allows an Evas image object to display content from an external,
 * platform-specific surface (e.g., a video buffer, a hardware-decoded image).
 * The exact nature of Evas_Native_Surface depends on the Evas engine and platform.
 *
 * @param eo_obj The image object.
 * @param surf Pointer to the native surface structure. Pass @c NULL to unset.
 */
EVAS_API void
evas_object_image_native_surface_set(Evas_Object *eo_obj, Evas_Native_Surface *surf)
{
   EVAS_IMAGE_API(eo_obj);

   Eina_Bool ret;

   ret = _evas_image_native_surface_set(eo_obj, surf);

   if (surf && !ret)
     {
        Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

        o->load_error = EFL_GFX_IMAGE_LOAD_ERROR_GENERIC;
     }
}

/**
 * @brief Gets the native surface associated with an image object.
 *
 * @param eo_obj The image object.
 * @return Pointer to the native surface structure, or @c NULL if none is set.
 */
EVAS_API Evas_Native_Surface *
evas_object_image_native_surface_get(const Evas_Object *eo_obj)
{
   EVAS_IMAGE_API(eo_obj, NULL);
   return _evas_image_native_surface_get(eo_obj);
}

/**
 * @brief Sets a callback function to be invoked when image pixels are needed.
 *
 * This allows for "on-demand" pixel data provision. When Evas needs to
 * render the image and its pixel data is not readily available (or marked dirty),
 * this callback will be called. The callback is responsible for populating
 * the image data.
 *
 * @param eo_obj The image object.
 * @param func The callback function.
 * @param data User data to be passed to the callback function.
 */
EVAS_API void
evas_object_image_pixels_get_callback_set(Eo *eo_obj, Evas_Object_Image_Pixels_Get_Cb func, void *data)
{
   EVAS_IMAGE_API(eo_obj);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   evas_object_async_block(obj);
   EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
     {
        pixi_write->func.get_pixels = func;
        pixi_write->func.get_pixels_data = data;
     }
   EINA_COW_PIXEL_WRITE_END(o, pixi_write);
}

/**
 * @brief Marks the image object's pixels as dirty or not.
 *
 * If set to dirty (@c EINA_TRUE), Evas knows that the pixel data has changed
 * (or needs to be fetched via callback) and will trigger a refresh/redraw.
 * Setting to @c EINA_FALSE can be used if an external update mechanism has
 * already updated the pixels and Evas just needs to be aware.
 *
 * @param eo_obj The image object.
 * @param dirty @c EINA_TRUE to mark pixels as dirty, @c EINA_FALSE otherwise.
 */
EVAS_API void
evas_object_image_pixels_dirty_set(Eo *eo_obj, Eina_Bool dirty)
{
   EVAS_IMAGE_API(eo_obj);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   evas_object_async_block(obj);
   if (dirty)
     {
        o->dirty_pixels = EINA_TRUE;
        o->changed = EINA_TRUE;
     }
   else o->dirty_pixels = EINA_FALSE;

   evas_object_change(eo_obj, obj);
}

/**
 * @brief Gets the dirty state of the image object's pixels.
 *
 * @param eo_obj The image object.
 * @return @c EINA_TRUE if pixels are marked dirty, @c EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_object_image_pixels_dirty_get(const Eo *eo_obj)
{
   EVAS_IMAGE_API(eo_obj, EINA_FALSE);

   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   return (o->dirty_pixels ? 1 : 0);
}

/**
 * @brief Sets the raw pixel data for an image object.
 *
 * This function directly provides pixel data to the image object. The data
 * is interpreted according to the image's current size, colorspace, and
 * alpha settings.
 *
 * If @p data is @c NULL, any existing engine-side image data is freed, and
 * the image dimensions are effectively set to 0x0 (though a resize event
 * might be triggered).
 *
 * The ownership of the @p data memory depends on the engine and how it
 * handles it. It might be copied, or the engine might take ownership.
 *
 * @param eo_obj The image object.
 * @param data Pointer to the raw pixel data. The format should match the
 *             image's colorspace (e.g., ARGB32 for EVAS_COLORSPACE_ARGB8888).
 *             Pass @c NULL to clear the image data.
 */
EVAS_API void
evas_object_image_data_set(Eo *eo_obj, void *data)
{
   EVAS_IMAGE_API(eo_obj);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   void *p_data, *pixels;
   Eina_Bool resize_call = EINA_FALSE;


   evas_object_async_block(obj);
   evas_render_rendering_wait(obj->layer->evas);

   _evas_image_cleanup(eo_obj, obj, o);
   p_data = o->engine_data;
   if (data)
     {
        // r/o FBO data_get: only free the image, don't update pixels
        if ((pixels = eina_hash_find(o->pixels->images_to_free, data)) != NULL)
          {
             eina_hash_del(o->pixels->images_to_free, data, pixels);
             return;
          }

        if (o->engine_data)
          {
             o->engine_data = ENFN->image_data_put(ENC, o->engine_data, data);
          }
        else
          {
             o->engine_data = ENFN->image_new_from_data(ENC,
                                                        o->cur->image.w,
                                                        o->cur->image.h,
                                                        data,
                                                        o->cur->has_alpha,
                                                        o->cur->cspace);
          }
        if (o->engine_data)
          {
             int stride = 0;

             if (ENFN->image_scale_hint_set)
               ENFN->image_scale_hint_set(ENC, o->engine_data, o->scale_hint);

             if (ENFN->image_content_hint_set)
               ENFN->image_content_hint_set(ENC, o->engine_data, o->content_hint);

             if (ENFN->image_stride_get)
               ENFN->image_stride_get(ENC, o->engine_data, &stride);
             else
               stride = o->cur->image.w * 4;

             if (o->cur->image.stride != stride)
               {
                  EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
                    state_write->image.stride = stride;
                  EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
               }
         }
       o->written = EINA_TRUE;
     }
   else
     {
        if (o->engine_data)
          {
             ENFN->image_free(ENC, o->engine_data);
             o->engine_data = NULL;
             o->changed = EINA_TRUE;
             evas_object_change(eo_obj, obj);
          }
        o->load_error = EFL_GFX_IMAGE_LOAD_ERROR_NONE;
        if ((o->cur->image.w != 0) || (o->cur->image.h != 0))
          resize_call = EINA_TRUE;

        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
          {
             state_write->image.w = 0;
             state_write->image.h = 0;
             state_write->image.stride = 0;
          }
        EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
     }
/* FIXME - in engine call above
   if (o->engine_data)
     o->engine_data = ENFN->image_alpha_set(ENC, o->engine_data, o->cur->has_alpha);
*/
   if (o->pixels_checked_out > 0) o->pixels_checked_out--;
   if (p_data != o->engine_data)
     {
        o->pixels_checked_out = 0;
     }
   if (resize_call) evas_object_inform_call_image_resize(eo_obj);
}

/**
 * @internal
 * @brief Callback function to free an Evas_Image_Legacy_Pixels_Entry.
 *
 * This function is used by an Eina_Hash to clean up entries when
 * pixel data obtained via evas_object_image_data_get (with to_free=true)
 * is no longer needed or the hash is destroyed.
 * It frees the engine-specific image data and the entry structure itself.
 *
 * @param data A pointer to an Evas_Image_Legacy_Pixels_Entry.
 */
static void
_image_to_free_del_cb(void *data)
{
   Evas_Image_Legacy_Pixels_Entry *px_entry = data;
   Evas_Object_Protected_Data *obj;

   obj = efl_data_scope_safe_get(px_entry->object, EFL_CANVAS_OBJECT_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(obj);
   ENFN->image_free(ENC, px_entry->image);
   free(px_entry);
}

/**
 * @brief Gets a direct pointer to the image object's pixel data.
 *
 * This function provides access to the raw pixel data of an image.
 *
 * If @p for_writing is @c EINA_TRUE:
 * - The data is being requested for modification.
 * - Evas may perform synchronization or copy-on-write operations.
 * - After modifying the data, evas_object_image_data_update_add() must be
 *   called to inform Evas of the changed region.
 * - The pointer returned should be considered valid only until the next Evas
 *   API call that might modify the image or its data (like setting a new file,
 *   resizing, etc.).
 *
 * If @p for_writing is @c EINA_FALSE:
 * - The data is requested for reading only.
 * - The pointer should not be used to modify the data.
 *
 * The engine might return a temporary copy of the data (indicated by `tofree`
 * internally). If so, this data is managed by a hash table and freed later.
 * If the engine provides direct access, `pixels_checked_out` is incremented.
 *
 * @param eo_obj The image object.
 * @param for_writing @c EINA_TRUE if the data will be modified,
 *                    @c EINA_FALSE for read-only access.
 * @return A pointer to the pixel data, or @c NULL on failure or if the image
 *         has no data. The format of the data depends on the image's
 *         colorspace (e.g., ARGB32 for EVAS_COLORSPACE_ARGB8888).
 * @see evas_object_image_data_set()
 * @see evas_object_image_data_update_add()
 * @see evas_object_image_stride_get()
 */
EVAS_API void*
evas_object_image_data_get(const Eo *eo_obj, Eina_Bool for_writing)
{
   EVAS_IMAGE_API(eo_obj, NULL);

   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   Evas_Image_Legacy_Pixels_Entry *px_entry = NULL;
   Eina_Bool tofree = 0;
   void *pixels = NULL;
   int stride = 0;
   DATA32 *data;
   int load_error;

   if (!o->engine_data) return NULL;

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   if (for_writing) evas_object_async_block(obj);
   if (for_writing) evas_render_rendering_wait(obj->layer->evas);

   data = NULL;
   if (ENFN->image_scale_hint_set)
     ENFN->image_scale_hint_set(ENC, o->engine_data, o->scale_hint);
   if (ENFN->image_content_hint_set)
     ENFN->image_content_hint_set(ENC, o->engine_data, o->content_hint);
   pixels = ENFN->image_data_get(ENC, o->engine_data, for_writing, &data, &load_error, &tofree);
   o->load_error = _evas_load_error_to_efl_gfx_image_load_error(load_error);

   /* if we fail to get engine_data, we have to return NULL */
   if (!pixels || !data) goto error;

   if (!tofree)
     {
        o->engine_data = pixels;
        if (ENFN->image_stride_get)
          ENFN->image_stride_get(ENC, o->engine_data, &stride);
        else
           stride = o->cur->image.w * 4;

        if (o->cur->image.stride != stride)
          {
             EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
                   state_write->image.stride = stride;
             EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
          }

        o->pixels_checked_out++;
     }
   else
     {
        Eina_Hash *hash = o->pixels->images_to_free;

        if (!hash)
          {
             hash = eina_hash_pointer_new(_image_to_free_del_cb);
             if (!hash) goto error;
             EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
               pixi_write->images_to_free = hash;
             EINA_COW_PIXEL_WRITE_END(o, pixi_write);
          }

        px_entry = calloc(1, sizeof(*px_entry));
        px_entry->object = (Eo *) eo_obj;
        px_entry->image = pixels;
        if (!eina_hash_add(hash, data, px_entry))
          goto error;
     }

   if (for_writing)
     {
        o->written = EINA_TRUE;
     }

   return data;

error:
   free(px_entry);
   if (tofree && pixels)
     ENFN->image_free(ENC, pixels);
   return NULL;
}

/**
 * @brief Sets the image object's pixel data by copying from a provided buffer.
 *
 * This function is similar to evas_object_image_data_set(), but it explicitly
 * copies the pixel data from the @p data buffer into the image object's
 * internal storage. The provided @p data buffer can be freed or reused by
 * the caller immediately after this function returns.
 *
 * The image must have its size (width and height) and colorspace set
 * appropriately before calling this function, as the copy operation will
 * use these properties.
 *
 * @param eo_obj The image object.
 * @param data Pointer to the raw pixel data to copy. The format should match
 *             the image's colorspace. Must not be @c NULL.
 * @see evas_object_image_size_set()
 * @see evas_object_image_colorspace_set()
 */
EVAS_API void
evas_object_image_data_copy_set(Eo *eo_obj, void *data)
{
   EVAS_IMAGE_API(eo_obj);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (!data) return;
   evas_object_async_block(obj);
   _evas_image_cleanup(eo_obj, obj, o);
   if ((o->cur->image.w <= 0) ||
       (o->cur->image.h <= 0)) return;
   if (o->engine_data)
     ENFN->image_free(ENC, o->engine_data);
   o->engine_data = ENFN->image_new_from_copied_data(ENC,
                                                     o->cur->image.w,
                                                     o->cur->image.h,
                                                     data,
                                                     o->cur->has_alpha,
                                                     o->cur->cspace);
   if (o->engine_data)
     {
        int stride = 0;

        o->engine_data =
          ENFN->image_alpha_set(ENC, o->engine_data, o->cur->has_alpha);
        if (ENFN->image_scale_hint_set)
          ENFN->image_scale_hint_set(ENC, o->engine_data, o->scale_hint);
        if (ENFN->image_content_hint_set)
          ENFN->image_content_hint_set(ENC, o->engine_data, o->content_hint);
        if (ENFN->image_stride_get)
          ENFN->image_stride_get(ENC, o->engine_data, &stride);
        else
          stride = o->cur->image.w * 4;

        if (o->cur->image.stride != stride)
          {
             EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
               state_write->image.stride = stride;
             EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
          }
        o->written = EINA_TRUE;
     }
   o->pixels_checked_out = 0;
}

/**
 * @brief Sets the dimensions (width and height) of an image object's pixel data.
 *
 * This function resizes the internal pixel buffer of the image object.
 * If the image already had pixel data, it might be reallocated. The content
 * of the existing data after resize is undefined (it might be preserved,
 * cleared, or garbage).
 * If the image had no data, this allocates a new buffer of the specified size.
 *
 * This is often used to prepare an image object to receive raw pixel data via
 * evas_object_image_data_set() or evas_object_image_data_copy_set(), or before
 * getting a writable buffer with evas_object_image_data_get().
 *
 * Minimum width and height are 1. Maximum is 32767.
 *
 * @param eo_obj The image object.
 * @param w The new width for the image data.
 * @param h The new height for the image data.
 */
EVAS_API void
evas_object_image_size_set(Evas_Object *eo_obj, int w, int h)
{
   EVAS_IMAGE_API(eo_obj);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   int stride = 0;

   evas_object_async_block(obj);
   _evas_image_cleanup(eo_obj, obj, o);
   if (w < 1) w = 1;
   if (h < 1) h = 1;
   if (w >= 32768) return;
   if (h >= 32768) return;
   if ((w == o->cur->image.w) &&
       (h == o->cur->image.h)) return;

   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
     {
        state_write->image.w = w;
        state_write->image.h = h;
     }
   EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);

   if (o->engine_data)
      o->engine_data = ENFN->image_size_set(ENC, o->engine_data, w, h);
   else
      o->engine_data = ENFN->image_new_from_copied_data
        (ENC, w, h, NULL, o->cur->has_alpha, o->cur->cspace);

   if (o->engine_data)
     {
        if (ENFN->image_scale_hint_set)
           ENFN->image_scale_hint_set(ENC, o->engine_data, o->scale_hint);
        if (ENFN->image_content_hint_set)
           ENFN->image_content_hint_set(ENC, o->engine_data, o->content_hint);
        if (ENFN->image_stride_get)
           ENFN->image_stride_get(ENC, o->engine_data, &stride);
        else
           stride = w * 4;
     }
   else
      stride = w * 4;
   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, cur_write)
     {
        cur_write->image.stride = stride;

/* FIXME - in engine call above
   if (o->engine_data)
     o->engine_data = ENFN->image_alpha_set(ENC, o->engine_data, o->cur->has_alpha);
*/
        EINA_COW_WRITE_BEGIN(evas_object_image_state_cow, o->prev, Evas_Object_Image_State, prev_write)
          EVAS_OBJECT_IMAGE_FREE_FILE_AND_KEY(cur_write, prev_write);
        EINA_COW_WRITE_END(evas_object_image_state_cow, o->prev, prev_write);
     }
   EINA_COW_IMAGE_STATE_WRITE_END(o, cur_write);

   o->written = EINA_TRUE;
   o->changed = EINA_TRUE;
   evas_object_inform_call_image_resize(eo_obj);
   evas_object_change(eo_obj, obj);
}

/**
 * @brief Sets the colorspace for an image object's pixel data.
 *
 * This function defines how the pixel data (set via evas_object_image_data_set(),
 * evas_object_image_data_copy_set(), or obtained via evas_object_image_data_get())
 * should be interpreted.
 *
 * Changing the colorspace of an image that already has pixel data might lead
 * to misinterpretation of that data unless the data itself is also converted
 * or replaced.
 *
 * @param eo_obj The image object.
 * @param cspace The new colorspace for the image data.
 *               Example: @c EVAS_COLORSPACE_ARGB8888, @c EVAS_COLORSPACE_YCBCR422P601_PL.
 */
EVAS_API void
evas_object_image_colorspace_set(Evas_Object *eo_obj, Evas_Colorspace cspace)
{
   EVAS_IMAGE_API(eo_obj);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   evas_object_async_block(obj);
   _evas_image_cleanup(eo_obj, obj, o);

   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
     state_write->cspace = cspace;
   EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);

   if (o->engine_data)
     ENFN->image_colorspace_set(ENC, o->engine_data, cspace);
}

/* old video surfaces */

/**
 * @brief (Legacy) Sets a video surface for the image object.
 * @deprecated This API is part of an older video integration mechanism.
 *             Prefer newer multimedia solutions if available.
 *
 * This function associates an Evas_Video_Surface with the image object,
 * allowing it to display video frames. The Evas_Video_Surface struct
 * contains callbacks that the video playback system uses to provide
 * pixel data and control the surface.
 *
 * @param eo_obj The image object.
 * @param surf Pointer to an Evas_Video_Surface structure, or @c NULL to unset.
 *             The structure must be filled with valid function pointers and data
 *             if not @c NULL.
 */
EVAS_API void
evas_object_image_video_surface_set(Evas_Object *eo_obj, Evas_Video_Surface *surf)
{
   EVAS_IMAGE_LEGACY_API(eo_obj);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   evas_object_async_block(obj);

   _evas_image_cleanup(eo_obj, obj, o);
   if (o->video_surface)
     {
        o->video_surface = EINA_FALSE;
        obj->layer->evas->video_objects = eina_list_remove(obj->layer->evas->video_objects, eo_obj);
     }

   if (surf)
     {
        if (surf->version != EVAS_VIDEO_SURFACE_VERSION) return;

        if (!surf->update_pixels ||
            !surf->move ||
            !surf->resize ||
            !surf->hide ||
            !surf->show)
          return;

        o->created = EINA_TRUE;
        o->video_surface = EINA_TRUE;

        EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
          pixi_write->video = *surf;
        EINA_COW_PIXEL_WRITE_END(o, pixi_write)

        obj->layer->evas->video_objects = eina_list_append(obj->layer->evas->video_objects, eo_obj);
     }
   else
     {
        if (!o->video_surface &&
            !o->pixels->video.update_pixels &&
            !o->pixels->video.move &&
            !o->pixels->video.resize &&
            !o->pixels->video.hide &&
            !o->pixels->video.show &&
            !o->pixels->video.data)
          return;

        o->video_surface = EINA_FALSE;
        EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
          {
             pixi_write->video.update_pixels = NULL;
             pixi_write->video.move = NULL;
             pixi_write->video.resize = NULL;
             pixi_write->video.hide = NULL;
             pixi_write->video.show = NULL;
             pixi_write->video.data = NULL;
          }
        EINA_COW_PIXEL_WRITE_END(o, pixi_write)
     }
}

/**
 * @brief (Legacy) Gets the video surface associated with the image object.
 * @deprecated This API is part of an older video integration mechanism.
 *
 * @param eo_obj The image object.
 * @return A const pointer to the Evas_Video_Surface structure if one is set,
 *         otherwise @c NULL.
 */
EVAS_API const Evas_Video_Surface*
evas_object_image_video_surface_get(const Evas_Object *eo_obj)
{
   EVAS_IMAGE_LEGACY_API(eo_obj, NULL);

   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   return (!o->video_surface ? NULL : &o->pixels->video);
}

/**
 * @brief (Legacy) Sets capabilities for the video surface.
 * @deprecated This API is part of an older video integration mechanism.
 *
 * This function allows specifying capabilities or properties of the video
 * surface, such as how it interacts with hardware planes or stacking.
 *
 * @param eo_obj The image object.
 * @param caps A bitmask of capability flags (e.g., @c EVAS_VIDEO_SURFACE_STACKING_CHECK).
 */
EVAS_API void
evas_object_image_video_surface_caps_set(Evas_Object *eo_obj, unsigned int caps)
{
   EVAS_IMAGE_LEGACY_API(eo_obj);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   evas_object_async_block(obj);

   _evas_image_cleanup(eo_obj, obj, o);

   if (caps == o->pixels->video_caps)
     return;

   EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
     pixi_write->video_caps = caps;
   EINA_COW_PIXEL_WRITE_END(o, pixi_write)
}

/**
 * @brief (Legacy) Gets the capabilities of the video surface.
 * @deprecated This API is part of an older video integration mechanism.
 *
 * @param eo_obj The image object.
 * @return A bitmask of capability flags. If not a video surface, it may
 *         return flags relevant for generic hardware plane checks.
 */
EVAS_API unsigned int
evas_object_image_video_surface_caps_get(const Evas_Object *eo_obj)
{
   EVAS_IMAGE_LEGACY_API(eo_obj, 0);

   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   /* The generic hardware plane code calls this function on
    * non-video surfaces, return stacking check for those to
    * allow them to use common video surface code */
   return (!o->video_surface ? EVAS_VIDEO_SURFACE_STACKING_CHECK : o->pixels->video_caps);
}

/**
 * @brief (Deprecated) Sets the fill spread mode for an image.
 * @deprecated This function is not implemented and warns if used with
 *             spread modes other than @c EFL_GFX_FILL_REPEAT.
 *             The concept of fill spread beyond simple tiling (repeat)
 *             was not fully realized in Evas.
 *
 * @param obj The image object (unused).
 * @param spread The desired fill spread mode.
 */
EVAS_API void
evas_object_image_fill_spread_set(Evas_Object *obj EINA_UNUSED, Evas_Fill_Spread spread)
{
   /* not implemented! */
   if (spread != EFL_GFX_FILL_REPEAT)
     WRN("Fill spread support is not implemented!");
}

/**
 * @brief (Deprecated) Gets the fill spread mode for an image.
 * @deprecated This function always returns @c EFL_GFX_FILL_REPEAT as other
 *             modes were not implemented.
 *
 * @param obj The image object (unused).
 * @return Always returns @c EFL_GFX_FILL_REPEAT.
 */
EVAS_API Evas_Fill_Spread
evas_object_image_fill_spread_get(const Evas_Object *obj EINA_UNUSED)
{
   return EFL_GFX_FILL_REPEAT;
}

/**
 * @brief (Deprecated) Sets the visibility of the source object of a proxy image.
 * @deprecated This feature had complex implications for rendering and event handling
 *             and is generally discouraged. Consider managing source object visibility
 *             directly or using `efl_gfx_entity_visible_set(src_obj, visible)`.
 *
 * This function attempts to control the visibility of the original source object
 * when this image object is acting as its proxy.
 *
 * @param eo The proxy image object.
 * @param visible @c EINA_TRUE to make the source visible, @c EINA_FALSE to hide it
 *                (from the perspective of this proxy relationship).
 */
EVAS_API void
evas_object_image_source_visible_set(Evas_Object *eo, Eina_Bool visible)
{
   /* FIXME: I'd love to remove this feature and replace by no_render.
    * But they are not 100% equivalent: if all proxies are removed, then the
    * source becomes visible again. This has some advantages for some apps but
    * it's complete hell to handle in evas render side.
    * -- jpeg, 2016/03/07
    */

   EVAS_IMAGE_LEGACY_API(eo);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo, EFL_CANVAS_OBJECT_CLASS);
   Evas_Object_Protected_Data *src_obj;
   Evas_Image_Data *o;

   o = efl_data_scope_get(eo, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   if (!o->cur->source) return;

   visible = !!visible;
   src_obj = efl_data_scope_get(o->cur->source, EFL_CANVAS_OBJECT_CLASS);
   if (src_obj->proxy->src_invisible == !visible) return;

   evas_object_async_block(obj);
   EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, src_obj->proxy, Evas_Object_Proxy_Data, proxy_write)
     proxy_write->src_invisible = !visible;
   EINA_COW_WRITE_END(evas_object_proxy_cow, src_obj->proxy, proxy_write);

   src_obj->changed_src_visible = EINA_TRUE;
   evas_object_smart_member_cache_invalidate(o->cur->source, EINA_FALSE,
                                             EINA_FALSE, EINA_TRUE);
   evas_object_change(o->cur->source, src_obj);
   if ((!visible) || (!src_obj->proxy->src_events)) return;
   //FIXME: Feed mouse events here.
}

/**
 * @brief (Deprecated) Gets the visibility state of the source object of a proxy image.
 * @deprecated See evas_object_image_source_visible_set() for deprecation reasons.
 *             Consider `efl_gfx_entity_visible_get(src_obj)`.
 *
 * @param eo The proxy image object.
 * @return @c EINA_TRUE if the source is considered visible in the context of this
 *         proxy, @c EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_object_image_source_visible_get(const Evas_Object *eo)
{
   /* FIXME: see evas_object_image_source_visible_set */

   EVAS_IMAGE_LEGACY_API(eo, EINA_FALSE);

   Evas_Object_Protected_Data *src_obj;
   Evas_Image_Data *o;
   Eina_Bool visible;

   o = efl_data_scope_get(eo, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   if (!o->cur->source) visible = EINA_FALSE;
   src_obj = efl_data_scope_get(o->cur->source, EFL_CANVAS_OBJECT_CLASS);
   if (src_obj) visible = !src_obj->proxy->src_invisible;
   else visible = EINA_FALSE;

   return visible;
}

/**
 * @brief (Deprecated) Converts image data to a specified colorspace.
 * @deprecated This function is deprecated. Modern Evas handles colorspace
 *             conversions internally or through engine capabilities.
 *             Directly manipulating pixel data and converting colorspaces
 *             manually is error-prone.
 *
 * This function attempts to take the current image data, convert it to the
 * @p to_cspace, and return a new buffer with the converted data. The caller
 * is responsible for freeing the returned buffer.
 *
 * @param eo_obj The image object.
 * @param to_cspace The target colorspace to convert to.
 * @return A pointer to a newly allocated buffer containing the converted
 *         pixel data, or @c NULL on failure or if no conversion is needed/possible.
 *         The caller owns this buffer and must free it.
 */
EVAS_API void*
evas_object_image_data_convert(Evas_Object *eo_obj, Evas_Colorspace to_cspace)
{
   EVAS_IMAGE_LEGACY_API(eo_obj, NULL);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o;
   void *engine_data;
   DATA32 *data;
   void* result = NULL;
   int load_error;

   static int warned = 0;
   if (!warned)
     {
        ERR("%s is deprecated and shouldn't be called", __func__);
        warned = 1;
     }

   evas_object_async_block(obj);
   o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   if (!o->engine_data) return NULL;
   if (o->video_surface)
     o->pixels->video.update_pixels(o->pixels->video.data, eo_obj, &o->pixels->video);
   if (o->cur->cspace == to_cspace) return NULL;
   if ((o->preload & EVAS_IMAGE_PRELOADING) && (o->engine_data))
     {
        o->preload = EVAS_IMAGE_PRELOAD_NONE;
        ENFN->image_data_preload_cancel(ENC, o->engine_data, eo_obj, EINA_TRUE);
     }
   data = NULL;
   engine_data = ENFN->image_data_get(ENC, o->engine_data, 0, &data, &load_error, NULL);
   o->load_error = _evas_load_error_to_efl_gfx_image_load_error(load_error);
   result = _evas_image_data_convert_internal(o, data, to_cspace);
   if (engine_data)
     o->engine_data = ENFN->image_data_put(ENC, engine_data, data);

   return result;
}

/**
 * @brief (Deprecated) Reloads an image from its source file.
 * @deprecated The need for manual reload is often a sign of issues elsewhere
 *             in managing image state or caching. Evas typically handles updates
 *             when files change if monitoring is enabled, or by resetting the
 *             file via evas_object_image_file_set().
 *
 * This function forces the image object to unload its current data and
 * reload it from its original file source (if one was set).
 * This is useful if the underlying file has changed on disk and Evas
 * hasn't automatically picked up the change.
 *
 * @param eo_obj The image object.
 */
EVAS_API void
evas_object_image_reload(Evas_Object *eo_obj)
{
   EVAS_IMAGE_LEGACY_API(eo_obj);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o;

   evas_object_async_block(obj);
   o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   if ((!o->cur->f) || (o->pixels_checked_out > 0)) return;
   if ((o->preload & EVAS_IMAGE_PRELOADING) && (o->engine_data))
     {
        o->preload = EVAS_IMAGE_PRELOAD_NONE;
        ENFN->image_data_preload_cancel(ENC, o->engine_data, eo_obj, EINA_TRUE);
     }
   if (o->engine_data)
     o->engine_data = ENFN->image_dirty_region(ENC, o->engine_data, 0, 0, o->cur->image.w, o->cur->image.h);

   eina_file_refresh(o->cur->f);
   o->written = EINA_FALSE;

   _evas_image_unload(eo_obj, obj, 1);
   evas_object_inform_call_image_unloaded(eo_obj);
   _evas_image_load(eo_obj, obj, o);

   EINA_COW_WRITE_BEGIN(evas_object_image_state_cow, o->prev, Evas_Object_Image_State, prev_write)
     {
        prev_write->f = NULL;
        prev_write->key = NULL;
     }
   EINA_COW_WRITE_END(evas_object_image_state_cow, o->prev, prev_write);

   o->changed = EINA_TRUE;
   evas_object_change(eo_obj, obj);
}

/**
 * @brief (Deprecated) Imports pixel data from an Evas_Pixel_Import_Source structure.
 * @deprecated This function is deprecated. Use evas_object_image_data_set() or
 *             evas_object_image_data_copy_set() with appropriate colorspace
 *             settings. The Evas_Pixel_Import_Source structure and specific
 *             pixel format enums like @c EVAS_PIXEL_FORMAT_YUV420P_601 are part
 *             of an older, less flexible system.
 *
 * This function attempts to import pixel data from a source described by
 * the @p pixels structure. It supports a limited set of source formats.
 * The image object must already be sized correctly (w, h) to match the
 * source pixels.
 *
 * @param eo_obj The image object.
 * @param pixels Pointer to an Evas_Pixel_Import_Source structure describing
 *               the source pixel data.
 *               Example for `pixels->rows` (for YUV420P):
 *               `pixels->rows[0]` = Y plane data
 *               `pixels->rows[1]` = U plane data
 *               `pixels->rows[2]` = V plane data
 * @return @c EINA_TRUE on successful import, @c EINA_FALSE on failure (e.g.,
 *         mismatched size, unsupported format).
 */
EVAS_API Eina_Bool
evas_object_image_pixels_import(Evas_Object *eo_obj, Evas_Pixel_Import_Source *pixels)
{
   EVAS_IMAGE_LEGACY_API(eo_obj, EINA_FALSE);

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o;
   int load_error;

   static int warned = 0;
   if (!warned)
     {
        ERR("%s is deprecated and shouldn't be called", __func__);
        warned = 1;
     }

   evas_object_async_block(obj);
   o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   _evas_image_cleanup(eo_obj, obj, o);
   if ((pixels->w != o->cur->image.w) || (pixels->h != o->cur->image.h)) return EINA_FALSE;

   switch (pixels->format)
     {
#if 0
      case EVAS_PIXEL_FORMAT_ARGB32:
          {
             if (o->engine_data)
               {
                  DATA32 *image_pixels = NULL;

                  o->engine_data =
                    ENFN->image_data_get(ENC,
                                         o->engine_data,
                                         1,
                                         &image_pixels,
                                         &load_error);
                  o->load_error = _evas_load_error_to_efl_gfx_image_load_error(load_error);
/* FIXME: need to actualyl support this */
/*		  memcpy(image_pixels, pixels->rows, o->cur->image.w * o->cur->image.h * 4);*/
                  if (o->engine_data)
                    o->engine_data =
                    ENFN->image_data_put(ENC, o->engine_data, image_pixels);
                  if (o->engine_data)
                    o->engine_data =
                    ENFN->image_alpha_set(ENC, o->engine_data, o->cur->has_alpha);
                  o->changed = EINA_TRUE;
                  evas_object_change(eo_obj, obj);
               }
          }
        break;
#endif
      case EVAS_PIXEL_FORMAT_YUV420P_601:
          {
             if (o->engine_data)
               {
                  DATA32 *image_pixels = NULL;

                  o->engine_data = ENFN->image_data_get(ENC, o->engine_data, 1, &image_pixels, &load_error, NULL);
                  o->load_error = _evas_load_error_to_efl_gfx_image_load_error(load_error);
                  if (image_pixels)
                    evas_common_convert_yuv_422p_601_rgba((DATA8 **) pixels->rows, (DATA8 *) image_pixels, o->cur->image.w, o->cur->image.h);
                  if (o->engine_data)
                    o->engine_data = ENFN->image_data_put(ENC, o->engine_data, image_pixels);
                  if (o->engine_data)
                    o->engine_data = ENFN->image_alpha_set(ENC, o->engine_data, o->cur->has_alpha);
                  o->changed = EINA_TRUE;
                  evas_object_change(eo_obj, obj);
               }
          }
        break;
      default:
        return EINA_FALSE;
        break;
     }
   return EINA_TRUE;
}

/**
 * @brief Gets the maximum dimensions (width and height) an image can have on a given Evas canvas.
 *
 * These limits are usually imposed by the underlying graphics hardware or engine.
 * Attempting to load or create an image larger than these dimensions may fail
 * or lead to undefined behavior.
 *
 * @param eo_e The Evas canvas.
 * @param w Pointer to store the maximum width. Can be @c NULL.
 * @param h Pointer to store the maximum height. Can be @c NULL.
 * @return @c EINA_TRUE on success, @c EINA_FALSE if the information cannot be retrieved.
 */
EVAS_API Eina_Bool
evas_image_max_size_get(Eo *eo_e, int *w, int *h)
{
   Eina_Size2D size;
   Eina_Bool ret;

   ret = efl_canvas_scene_image_max_size_get(eo_e, &size);
   if (ret)
     {
        if (w) *w = size.w;
        if (h) *h = size.h;
     }
   return ret;
}

/**
 * @brief (Deprecated) Sets whether an image is an alpha mask.
 * @deprecated This function was never implemented and serves no purpose.
 *             Alpha channels are handled via evas_object_image_alpha_set()
 *             and the image data itself.
 *
 * @param eo_obj The image object (unused).
 * @param ismask Unused.
 */
EVAS_API void
evas_object_image_alpha_mask_set(Evas_Object *eo_obj EINA_UNUSED, Eina_Bool ismask EINA_UNUSED)
{
   WRN("This function is not implemented, has never been and never will be.");
   EVAS_IMAGE_LEGACY_API(eo_obj);
}

/**
 * @internal
 * @brief Implements Efl.File.loaded_get for Evas_Image.
 *
 * This function checks if the image file data has been loaded.
 * It considers the `skip_head` flag: if true, it checks for the presence
 * of `o->cur->f` (the Eina_File handle), otherwise it calls the superclass's
 * implementation.
 *
 * @param eo_obj The Evas image object.
 * @param _pd Private data (unused).
 * @return @c EINA_TRUE if the image is considered loaded, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_evas_image_efl_file_loaded_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   if (!o->skip_head)
     return efl_file_loaded_get(efl_super(eo_obj, EVAS_IMAGE_CLASS));
   return !!o->cur->f;
}

/**
 * @internal
 * @brief Implements Efl.File.mmap_get for Evas_Image.
 *
 * This function retrieves the memory-mapped file (Eina_File) associated with
 * the image. It considers the `skip_head` flag: if true, it returns `o->cur->f`,
 * otherwise it calls the superclass's implementation.
 *
 * @param eo_obj The Evas image object.
 * @param _pd Private data (unused).
 * @return A const pointer to the Eina_File, or @c NULL if not mmapped or not applicable.
 */
EOLIAN static const Eina_File *
_evas_image_efl_file_mmap_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   if (!o->skip_head)
     return efl_file_mmap_get(efl_super(eo_obj, EVAS_IMAGE_CLASS));
   return o->cur->f;
}

/**
 * @internal
 * @brief Implements Efl.File.load for Evas_Image.
 *
 * This function triggers the loading of the image data from its source
 * (file or mmap). It handles errors and updates the image's load error state.
 * It respects the `skip_head` flag, potentially bypassing the superclass's
 * load if `skip_head` is true and directly calling internal Evas image loading logic.
 *
 * @param obj The Evas image object.
 * @param pd Private data (unused).
 * @return 0 on success, or an Eina_Error code on failure.
 *         Common errors include ENOENT, ENOMEM, EPERM, EACCES.
 */
EOLIAN static Eina_Error
_evas_image_efl_file_load(Eo *obj, void *pd EINA_UNUSED)
{
   EVAS_IMAGE_API(obj, EINA_FALSE);
   if (efl_file_loaded_get(obj)) return 0;
   Evas_Image_Data *o = efl_data_scope_get(obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   Eina_Error err = 0;

   if (!o->skip_head)
     err = efl_file_load(efl_super(obj, EVAS_IMAGE_CLASS));
   if (err)
     {
        if (err == ENOENT)
          _efl_canvas_image_load_error_set(obj, EFL_GFX_IMAGE_LOAD_ERROR_DOES_NOT_EXIST);
        else if (err == ENOMEM)
          _efl_canvas_image_load_error_set(obj, EFL_GFX_IMAGE_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED);
        else if ((err == EPERM) || (err == EACCES))
          _efl_canvas_image_load_error_set(obj, EFL_GFX_IMAGE_LOAD_ERROR_PERMISSION_DENIED);
        else
          _efl_canvas_image_load_error_set(obj, EFL_GFX_IMAGE_LOAD_ERROR_GENERIC);
        return err;
     }
   if (_evas_image_file_load(obj, o))
     return 0;
   return EFL_GFX_IMAGE_LOAD_ERROR_DOES_NOT_EXIST;
}

/**
 * @internal
 * @brief Implements Efl.File.unload for Evas_Image.
 *
 * This function unloads the image data, freeing associated resources.
 * It calls the superclass's unload implementation and then performs
 * Evas-specific image unloading.
 *
 * @param obj The Evas image object.
 * @param pd Private data (unused).
 */
EOLIAN static void
_evas_image_efl_file_unload(Eo *obj, void *pd EINA_UNUSED)
{
   EVAS_IMAGE_API(obj);
   efl_file_unload(efl_super(obj, EVAS_IMAGE_CLASS));
   _evas_image_file_unload(obj);
}

#include "canvas/evas_image_eo.c"
