#include "evas_image_private.h"
#include "efl_canvas_image.eo.h"

/**
 * @file
 * @brief These routines are used for managing Efl_Canvas_Image objects.
 */

#define MY_CLASS EFL_CANVAS_IMAGE_CLASS
#define MY_CLASS_NAME efl_class_name_get(MY_CLASS)

/**
 * @internal
 * @brief Unloads the image file data.
 *
 * This function releases resources associated with the loaded image file,
 * effectively clearing the image data from memory. It resets the image state
 * and marks it as not having buffer data set.
 *
 * @param[in] eo_obj The Evas_Object (image) to unload.
 */
void
_evas_image_file_unload(Eo *eo_obj)
{
   Evas_Object_Protected_Data *obj;
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (!o->cur->f) return;

   obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   evas_object_async_block(obj);
   _evas_image_init_set(NULL, NULL, eo_obj, obj, o, NULL);
   o->buffer_data_set = EINA_FALSE;
   _evas_image_done_set(eo_obj, obj, o);
   o->load_error = EFL_GFX_IMAGE_LOAD_ERROR_NONE;
}

/**
 * @internal
 * @brief Updates the image data with a preloaded file.
 *
 * If the image doesn't currently have a file associated (o->cur->f is NULL),
 * this function sets the provided Eina_File as the image's current file.
 * This is typically used when an image is preloaded and its file data
 * needs to be associated with the image object.
 *
 * @param[in] eo_obj The Evas_Object (image) to update.
 * @param[in] f The Eina_File to associate with the image.
 */
void
_evas_image_preload_update(Eo *eo_obj, Eina_File *f)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   if (o->cur->f) return;
   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, cur)
   {
      cur->f = eina_file_dup(f);
   }
   EINA_COW_IMAGE_STATE_WRITE_END(o, cur)
}

/**
 * @internal
 * @brief Loads image data from a file.
 *
 * This function handles the actual loading of the image data from the
 * specified file (or mmaped file) and key. It interacts with the underlying
 * Evas engine to perform the load operation. It also handles animated images
 * and updates the image object's state accordingly.
 *
 * @param[in] eo_obj The Evas_Object (image) to load data into.
 * @param[in,out] o The internal image data structure.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
Eina_Bool
_evas_image_file_load(Eo *eo_obj, Evas_Image_Data *o)
{
   Evas_Object_Protected_Data *obj;
   Evas_Image_Load_Opts lo;
   const Eina_File *f = efl_file_mmap_get(eo_obj);
   const char *key = efl_file_key_get(eo_obj);
   int load_error;
   int frame_index;

   if (!o->skip_head)
     EINA_SAFETY_ON_NULL_RETURN_VAL(f, EINA_FALSE);

   if (f && (o->cur->f == f))
     {
        if ((!o->cur->key) && (!key))
          return EINA_TRUE;
        if ((o->cur->key) && (key) && (!strcmp(o->cur->key, key)))
          return EINA_TRUE;
     }

   obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   evas_object_async_block(obj);
   _evas_image_init_set(f, key, eo_obj, obj, o, &lo);
   if (f)
     o->engine_data = ENFN->image_mmap(ENC, o->cur->f, o->cur->key, &load_error, &lo);
   else
     o->engine_data = ENFN->image_load(ENC, efl_file_get(eo_obj), o->cur->key, &load_error, &lo);

   if (_evas_image_animated_get(eo_obj))
     {
        frame_index = ENFN->image_animated_frame_get(ENC, o->engine_data);
        _evas_image_animated_frame_set(eo_obj, frame_index);
     }

   o->load_error = _evas_load_error_to_efl_gfx_image_load_error(load_error);
   o->buffer_data_set = EINA_FALSE;
   _evas_image_done_set(eo_obj, obj, o);
   o->file_size.w = o->cur->image.w;
   o->file_size.h = o->cur->image.h;

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_canvas_image_efl_file_loaded_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   // If skip_head is true, it means we are managing the file loading internally,
   // so check our internal state (o->cur->f). Otherwise, delegate to parent.
   if (!o->skip_head)
     return efl_file_loaded_get(efl_super(eo_obj, MY_CLASS));
   return !!o->cur->f;
}

EOLIAN static const Eina_File *
_efl_canvas_image_efl_file_mmap_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   // If skip_head is true, return our internally managed mmaped file.
   // Otherwise, delegate to parent.
   if (!o->skip_head)
     return efl_file_mmap_get(efl_super(eo_obj, MY_CLASS));
   return o->cur->f;
}

EOLIAN static Eina_Error
_efl_canvas_image_efl_file_load(Eo *eo_obj, void *_pd EINA_UNUSED)
{
   // If already loaded, nothing to do.
   if (efl_file_loaded_get(eo_obj)) return 0;
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   Eina_Error err = 0;

   // If not skipping header (i.e., standard file loading), call parent's load.
   if (!o->skip_head)
     err = efl_file_load(efl_super(eo_obj, MY_CLASS));
   if (err) return err; // Propagate error from parent.
   // Attempt to load the image file internally.
   if (_evas_image_file_load(eo_obj, o))
     return 0; // Success
   return EFL_GFX_IMAGE_LOAD_ERROR_DOES_NOT_EXIST; // Default error if internal load fails.
}

EOLIAN static void
_efl_canvas_image_efl_file_unload(Eo *eo_obj, void *_pd EINA_UNUSED)
{
   // Always call parent's unload.
   efl_file_unload(efl_super(eo_obj, MY_CLASS));
   // Perform internal image file unload.
   _evas_image_file_unload(eo_obj);
}

/**
 * @internal
 * @brief Gets the mmaped file associated with the image.
 * @param[in] eo_obj The Evas_Object (image).
 * @return The mmaped Eina_File, or @c NULL if not set.
 */
const Eina_File *
_evas_image_mmap_get(const Eo *eo_obj)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   return o->cur->f;
}

/**
 * @internal
 * @brief Gets the key associated with the image file.
 * @param[in] eo_obj The Evas_Object (image).
 * @return The key string, or @c NULL if not set.
 */
const char *
_evas_image_key_get(const Eo *eo_obj)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   return o->cur->key;
}

/**
 * @internal
 * @brief Sets the load error for the image.
 * @param[in] eo_obj The Evas_Object (image).
 * @param[in] err The Eina_Error code representing the load error.
 */
void
_efl_canvas_image_load_error_set(Eo *eo_obj EINA_UNUSED, Eina_Error err)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   o->load_error = err;
}

/**
 * @internal
 * @brief Internal helper to handle image preloading or cancellation.
 *
 * This function manages the state of image preloading. If @p cancel is
 * @c EINA_TRUE, it attempts to cancel an ongoing preload. Otherwise, it
 * initiates a preload if not already preloading.
 *
 * @param[in] eo_obj The Evas_Object (image).
 * @param[in,out] o The internal image data structure.
 * @param[in] cancel If @c EINA_TRUE, cancel preload; otherwise, start preload.
 */
static void
_image_preload_internal(Eo *eo_obj, Evas_Image_Data *o, Eina_Bool cancel)
{
   // If no engine data, mark as preloading and inform.
   if (!o->engine_data)
     {
        o->preload = EVAS_IMAGE_PRELOADING;
        evas_object_inform_call_image_preloaded(eo_obj);
        return;
     }
   // FIXME: if already busy preloading, then dont request again until
   // preload done
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   if (cancel)
     {
        if (o->preload & EVAS_IMAGE_PRELOADING)
          {
             o->preload |= EVAS_IMAGE_PRELOAD_CANCEL;
             ENFN->image_data_preload_cancel(ENC, o->engine_data, eo_obj, EINA_TRUE);
          }
     }
   else
     {
        if (o->preload != EVAS_IMAGE_PRELOADING)
          {
             o->preload = EVAS_IMAGE_PRELOADING;
             ENFN->image_data_preload_request(ENC, o->engine_data, eo_obj);
          }
     }
}

/**
 * @internal
 * @brief Initiates asynchronous loading of the image.
 *
 * This function blocks asynchronous operations on the object and then
 * calls the internal preload function to start loading the image data.
 *
 * @param[in] eo_obj The Evas_Object (image) to load asynchronously.
 */
void
_evas_image_load_async_start(Eo *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   evas_object_async_block(obj);
   _image_preload_internal(eo_obj, o, EINA_FALSE);
}

EOLIAN static void
_efl_canvas_image_efl_gfx_image_load_controller_load_async_start(Eo *eo_obj, void *_pd EINA_UNUSED)
{
   _evas_image_load_async_start(eo_obj);
}

/**
 * @internal
 * @brief Cancels asynchronous loading of the image.
 *
 * This function blocks asynchronous operations on the object and then
 * calls the internal preload function with the cancel flag.
 *
 * @param[in] eo_obj The Evas_Object (image) whose loading is to be canceled.
 */
void
_evas_image_load_async_cancel(Eo *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   evas_object_async_block(obj);
   _image_preload_internal(eo_obj, o, EINA_TRUE);
}

EOLIAN static void
_efl_canvas_image_efl_gfx_image_load_controller_load_async_cancel(Eo *eo_obj, void *_pd EINA_UNUSED)
{
   _evas_image_load_async_cancel(eo_obj);
}

/**
 * @internal
 * @brief Sets the DPI for loading the image.
 *
 * If the DPI changes and a file is currently loaded, the image will be
 * unloaded and reloaded with the new DPI setting.
 *
 * @param[in] eo_obj The Evas_Object (image).
 * @param[in] dpi The dots per inch to use for loading. Example: 75.0.
 */
void
_evas_image_load_dpi_set(Eo *eo_obj, double dpi)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (EINA_DBL_EQ(dpi, o->load_opts->dpi)) return;
   evas_object_async_block(obj);
   EINA_COW_LOAD_OPTS_WRITE_BEGIN(o, low)
     low->dpi = dpi;
   EINA_COW_LOAD_OPTS_WRITE_END(o, low);

   if (o->cur->f)
     {
        _evas_image_unload(eo_obj, obj, 0);
        evas_object_inform_call_image_unloaded(eo_obj);
        _evas_image_load(eo_obj, obj, o);
        o->changed = EINA_TRUE;
        evas_object_change(eo_obj, obj);
     }
}

EOLIAN static void
_efl_canvas_image_efl_gfx_image_load_controller_load_dpi_set(Eo *eo_obj, void *_pd EINA_UNUSED, double dpi)
{
   _evas_image_load_dpi_set(eo_obj, dpi);
}

/**
 * @internal
 * @brief Gets the DPI used for loading the image.
 * @param[in] eo_obj The Evas_Object (image).
 * @return The current DPI setting. Example: 75.0.
 */
double
_evas_image_load_dpi_get(const Eo *eo_obj)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   return o->load_opts->dpi;
}

EOLIAN static double
_efl_canvas_image_efl_gfx_image_load_controller_load_dpi_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   return _evas_image_load_dpi_get(eo_obj);
}

/**
 * @internal
 * @brief Sets the target load size for the image.
 *
 * If the load size changes and a file is currently loaded, the image will be
 * unloaded and reloaded to attempt to match the new size.
 *
 * @param[in] eo_obj The Evas_Object (image).
 * @param[in] w The target width. Example: 100.
 * @param[in] h The target height. Example: 100.
 */
void
_evas_image_load_size_set(Eo *eo_obj, int w, int h)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if ((o->load_opts->w == w) && (o->load_opts->h == h)) return;
   evas_object_async_block(obj);
   EINA_COW_LOAD_OPTS_WRITE_BEGIN(o, low)
   {
      low->w = w;
      low->h = h;
   }
   EINA_COW_LOAD_OPTS_WRITE_END(o, low);

   if (o->cur->f)
     {
        _evas_image_unload(eo_obj, obj, 0);
        evas_object_inform_call_image_unloaded(eo_obj);
        _evas_image_load(eo_obj, obj, o);
        o->changed = EINA_TRUE;
        evas_object_change(eo_obj, obj);
     }
   o->proxyerror = 0;
}

EOLIAN static void
_efl_canvas_image_efl_gfx_image_load_controller_load_size_set(Eo *eo_obj, void *_pd EINA_UNUSED, Eina_Size2D sz)
{
   _evas_image_load_size_set(eo_obj, sz.w, sz.h);
}

/**
 * @internal
 * @brief Gets the target load size for the image.
 * @param[in] eo_obj The Evas_Object (image).
 * @param[out] w Pointer to store the target width. Can be @c NULL.
 * @param[out] h Pointer to store the target height. Can be @c NULL.
 */
void
_evas_image_load_size_get(const Eo *eo_obj, int *w, int *h)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (w) *w = o->load_opts->w;
   if (h) *h = o->load_opts->h;
}

EOLIAN static Eina_Size2D
_efl_canvas_image_efl_gfx_image_load_controller_load_size_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   Eina_Size2D sz;
   _evas_image_load_size_get(eo_obj, &sz.w, &sz.h);
   return sz; // Example: {w=100, h=100}
}

/**
 * @internal
 * @brief Sets the scale-down factor for loading the image.
 *
 * If the scale-down factor changes and a file is currently loaded,
 * the image will be unloaded and reloaded with the new factor.
 *
 * @param[in] eo_obj The Evas_Object (image).
 * @param[in] scale_down The factor by which to scale down the image during load.
 *                       Example: 2 (halves the dimensions).
 */
void
_evas_image_load_scale_down_set(Eo *eo_obj, int scale_down)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (o->load_opts->scale_down_by == scale_down) return;
   evas_object_async_block(obj);
   EINA_COW_LOAD_OPTS_WRITE_BEGIN(o, low)
     low->scale_down_by = scale_down;
   EINA_COW_LOAD_OPTS_WRITE_END(o, low);

   if (o->cur->f)
     {
        _evas_image_unload(eo_obj, obj, 0);
        evas_object_inform_call_image_unloaded(eo_obj);
        _evas_image_load(eo_obj, obj, o);
        o->changed = EINA_TRUE;
        evas_object_change(eo_obj, obj);
     }
}

EOLIAN static void
_efl_canvas_image_efl_gfx_image_load_controller_load_scale_down_set(Eo *eo_obj, void *_pd EINA_UNUSED, int scale_down)
{
   _evas_image_load_scale_down_set(eo_obj, scale_down);
}

/**
 * @internal
 * @brief Gets the scale-down factor used for loading the image.
 * @param[in] eo_obj The Evas_Object (image).
 * @return The current scale-down factor. Example: 2.
 */
int
_evas_image_load_scale_down_get(const Eo *eo_obj)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   return o->load_opts->scale_down_by;
}

EOLIAN static int
_efl_canvas_image_efl_gfx_image_load_controller_load_scale_down_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   return _evas_image_load_scale_down_get(eo_obj);
}

/**
 * @internal
 * @brief Sets whether to skip header loading (for raw data loading).
 * @param[in] eo_obj The Evas_Object (image).
 * @param[in] skip @c EINA_TRUE to skip header, @c EINA_FALSE otherwise.
 */
void
_evas_image_load_head_skip_set(const Eo *eo_obj, Eina_Bool skip)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   o->skip_head = skip;
}

EOLIAN static void
_efl_canvas_image_efl_gfx_image_load_controller_load_skip_header_set(Eo *eo_obj, void *_pd EINA_UNUSED, Eina_Bool skip)
{
   _evas_image_load_head_skip_set(eo_obj, skip);
}

/**
 * @internal
 * @brief Gets whether header loading is skipped.
 * @param[in] eo_obj The Evas_Object (image).
 * @return @c EINA_TRUE if header skipping is enabled, @c EINA_FALSE otherwise.
 */
Eina_Bool
_evas_image_load_head_skip_get(const Eo *eo_obj)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   return o->skip_head;
}

EOLIAN static Eina_Bool
_efl_canvas_image_efl_gfx_image_load_controller_load_skip_header_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   return _evas_image_load_head_skip_get(eo_obj);
}

/**
 * @internal
 * @brief Sets the region of the image to load.
 *
 * If the region changes and a file is currently loaded, the image will be
 * unloaded and reloaded to load only the specified region.
 *
 * @param[in] eo_obj The Evas_Object (image).
 * @param[in] x The X offset of the region. Example: 10.
 * @param[in] y The Y offset of the region. Example: 10.
 * @param[in] w The width of the region. Example: 50.
 * @param[in] h The height of the region. Example: 50.
 */
void
_evas_image_load_region_set(Eo *eo_obj, int x, int y, int w, int h)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if ((o->load_opts->region.x == x) && (o->load_opts->region.y == y) &&
       (o->load_opts->region.w == w) && (o->load_opts->region.h == h)) return;
   evas_object_async_block(obj);
   EINA_COW_LOAD_OPTS_WRITE_BEGIN(o, low)
   {
      low->region.x = x;
      low->region.y = y;
      low->region.w = w;
      low->region.h = h;
   }
   EINA_COW_LOAD_OPTS_WRITE_END(o, low);

   if (o->cur->f)
     {
        _evas_image_unload(eo_obj, obj, 0);
        evas_object_inform_call_image_unloaded(eo_obj);
        _evas_image_load(eo_obj, obj, o);
        o->changed = EINA_TRUE;
        evas_object_change(eo_obj, obj);
     }
}

EOLIAN static void
_efl_canvas_image_efl_gfx_image_load_controller_load_region_set(Eo *eo_obj, void *_pd EINA_UNUSED, Eina_Rect region)
{
   _evas_image_load_region_set(eo_obj, region.x, region.y, region.w, region.h);
}

/**
 * @internal
 * @brief Gets the region of the image to load.
 * @param[in] eo_obj The Evas_Object (image).
 * @param[out] x Pointer to store the X offset. Can be @c NULL.
 * @param[out] y Pointer to store the Y offset. Can be @c NULL.
 * @param[out] w Pointer to store the width. Can be @c NULL.
 * @param[out] h Pointer to store the height. Can be @c NULL.
 */
void
_evas_image_load_region_get(const Eo *eo_obj, int *x, int *y, int *w, int *h)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (x) *x = o->load_opts->region.x;
   if (y) *y = o->load_opts->region.y;
   if (w) *w = o->load_opts->region.w;
   if (h) *h = o->load_opts->region.h;
}

EOLIAN static Eina_Rect
_efl_canvas_image_efl_gfx_image_load_controller_load_region_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   Eina_Rect r;
   _evas_image_load_region_get(eo_obj, &r.x, &r.y, &r.w, &r.h);
   return r; // Example: {x=10, y=10, w=50, h=50}
}

/**
 * @internal
 * @brief Sets whether to apply EXIF orientation data during load.
 * @param[in] eo_obj The Evas_Object (image).
 * @param[in] enable @c EINA_TRUE to apply orientation, @c EINA_FALSE otherwise.
 */
void
_evas_image_load_orientation_set(Eo *eo_obj, Eina_Bool enable)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (o->load_opts->orientation == !!enable) return;
   evas_object_async_block(obj);

   EINA_COW_LOAD_OPTS_WRITE_BEGIN(o, low)
         low->orientation = !!enable;
   EINA_COW_LOAD_OPTS_WRITE_END(o, low);
}

EOLIAN static void
_efl_canvas_image_efl_gfx_image_load_controller_load_orientation_set(Eo *eo_obj, void *_pd EINA_UNUSED, Eina_Bool enable)
{
   _evas_image_load_orientation_set(eo_obj, enable);
}

/**
 * @internal
 * @brief Gets whether EXIF orientation data is applied during load.
 * @param[in] eo_obj The Evas_Object (image).
 * @return @c EINA_TRUE if orientation is applied, @c EINA_FALSE otherwise.
 */
Eina_Bool
_evas_image_load_orientation_get(const Eo *eo_obj)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   return o->load_opts->orientation;
}

EOLIAN static Eina_Bool
_efl_canvas_image_efl_gfx_image_load_controller_load_orientation_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   return _evas_image_load_orientation_get(eo_obj);
}

/**
 * @internal
 * @brief Checks if the current image loader supports region loading.
 * @param[in] eo_obj The Evas_Object (image).
 * @return @c EINA_TRUE if region loading is supported, @c EINA_FALSE otherwise.
 */
Eina_Bool
_evas_image_load_region_support_get(const Eo *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   return ENFN->image_can_region_get(ENC, o->engine_data);
}

EOLIAN static Eina_Bool
_efl_canvas_image_efl_gfx_image_load_controller_load_region_support_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   return _evas_image_load_region_support_get(eo_obj);
}

/* animated feature */

/**
 * @internal
 * @brief Checks if the image is animated.
 * @param[in] eo_obj The Evas_Object (image).
 * @return @c EINA_TRUE if the image is animated, @c EINA_FALSE otherwise.
 */
Eina_Bool
_evas_image_animated_get(const Eo *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (!ENFN->image_animated_get)
     return EINA_FALSE;

   return ENFN->image_animated_get(ENC, o->engine_data);
}

EOLIAN static Eina_Bool
_efl_canvas_image_efl_gfx_frame_controller_animated_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   return _evas_image_animated_get(eo_obj);
}

/**
 * @internal
 * @brief Gets the total number of frames in an animated image.
 * @param[in] eo_obj The Evas_Object (image).
 * @return The number of frames, or -1 if not applicable or on error.
 */
int
_evas_image_animated_frame_count_get(const Eo *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (!ENFN->image_animated_frame_count_get ||
       !evas_object_image_animated_get(eo_obj))
     return -1;

   obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   return ENFN->image_animated_frame_count_get(ENC, o->engine_data);
}

EOLIAN static int
_efl_canvas_image_efl_gfx_frame_controller_frame_count_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   return _evas_image_animated_frame_count_get(eo_obj);
}

/**
 * @internal
 * @brief Gets the loop type of an animated image.
 * @param[in] eo_obj The Evas_Object (image).
 * @return The loop type hint (e.g., loop, ping-pong).
 *         Returns EFL_GFX_FRAME_CONTROLLER_LOOP_HINT_NONE if not applicable or on error.
 */
Efl_Gfx_Frame_Controller_Loop_Hint
_evas_image_animated_loop_type_get(const Eo *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (!ENFN->image_animated_loop_type_get ||
       !evas_object_image_animated_get(eo_obj))
     return EFL_GFX_FRAME_CONTROLLER_LOOP_HINT_NONE;

   return (Efl_Gfx_Frame_Controller_Loop_Hint) ENFN->image_animated_loop_type_get(ENC, o->engine_data);
}

EOLIAN static Efl_Gfx_Frame_Controller_Loop_Hint
_efl_canvas_image_efl_gfx_frame_controller_loop_type_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   return _evas_image_animated_loop_type_get(eo_obj);
}

/**
 * @internal
 * @brief Gets the loop count for an animated image.
 * @param[in] eo_obj The Evas_Object (image).
 * @return The number of times the animation will loop, or -1 if not applicable or on error.
 *         0 means loop forever.
 */
int
_evas_image_animated_loop_count_get(const Eo *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (!ENFN->image_animated_loop_count_get ||
       !evas_object_image_animated_get(eo_obj))
     return -1;

   return ENFN->image_animated_loop_count_get(ENC, o->engine_data);
}

EOLIAN static int
_efl_canvas_image_efl_gfx_frame_controller_loop_count_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   return _evas_image_animated_loop_count_get(eo_obj);
}

/**
 * @internal
 * @brief Gets the duration of a specific frame or a sequence of frames in an animated image.
 * @param[in] eo_obj The Evas_Object (image).
 * @param[in] start_frame The starting frame index. Example: 0.
 * @param[in] frame_num The number of frames from start_frame to consider. Example: 1 for a single frame.
 * @return The duration in seconds, or -1.0 on error or if not applicable.
 */
double
_evas_image_animated_frame_duration_get(const Eo *eo_obj, int start_frame, int frame_num)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   int frame_count = 0;

   if (!ENFN->image_animated_frame_count_get ||
       !ENFN->image_animated_frame_duration_get)
     return -1.0;

   frame_count = ENFN->image_animated_frame_count_get(ENC, o->engine_data);
   if ((start_frame + frame_num) > frame_count)
     return -1.0;

   return ENFN->image_animated_frame_duration_get(ENC, o->engine_data, start_frame, frame_num);
}

EOLIAN static double
_efl_canvas_image_efl_gfx_frame_controller_frame_duration_get(const Eo *eo_obj, void *_pd EINA_UNUSED, int start_frame, int frame_num)
{
   return _evas_image_animated_frame_duration_get(eo_obj, start_frame, frame_num);
}

Eina_Bool _efl_canvas_image_efl_gfx_frame_controller_sector_set(Eo *obj EINA_UNUSED,
                                                                    void *_pd EINA_UNUSED,
                                                                    const char *name EINA_UNUSED,
                                                                    int startframe EINA_UNUSED,
                                                                    int endframe EINA_UNUSED)
{
   // TODO: We need to implement the feature to section playback of image animation.
   ERR("efl_gfx_frame_controller_sector_set not implemented for efl_canvas_image yet.");
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Retrieves a named animation sector's start and end frames.
 * @param obj The Evas_Object (image).
 * @param _pd Private data (unused).
 * @param name The name of the sector.
 * @param startframe Pointer to store the start frame index.
 * @param endframe Pointer to store the end frame index.
 * @return EINA_TRUE on success, EINA_FALSE otherwise (e.g., sector not found).
 * @note This feature is not yet implemented.
 */
Eina_Bool _efl_canvas_image_efl_gfx_frame_controller_sector_get(const Eo *obj EINA_UNUSED,
                                                                      void *_pd EINA_UNUSED,
                                                                      const char *name EINA_UNUSED,
                                                                      int *startframe EINA_UNUSED,
                                                                      int *endframe EINA_UNUSED)
{
   // TODO: We need to implement the feature to section playback of image animation.
   ERR("efl_gfx_frame_controller_sector_get not implemented for efl_canvas_image yet.");
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Sets the current frame of an animated image.
 * @param[in] eo_obj The Evas_Object (image).
 * @param[in] frame_index The index of the frame to display. Example: 0.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise (e.g., invalid index).
 */
Eina_Bool
_evas_image_animated_frame_set(Eo *eo_obj, int frame_index)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   int frame_count = 0;

   if (!o->cur->f) return EINA_FALSE;
   if (o->cur->frame == frame_index) return EINA_TRUE;

   if (!evas_object_image_animated_get(eo_obj)) return EINA_FALSE;
   evas_object_async_block(obj);
   frame_count = evas_object_image_animated_frame_count_get(eo_obj);

   if ((frame_count < 0) || (frame_index > frame_count))
     return EINA_FALSE;

   if (!ENFN->image_animated_frame_set) return EINA_FALSE;
   ENFN->image_animated_frame_set(ENC, o->engine_data, frame_index);
   //   if (!ENFN->image_animated_frame_set(ENC, o->engine_data, frame_index)) return;

   EINA_COW_WRITE_BEGIN(evas_object_image_state_cow, o->prev, Evas_Object_Image_State, prev_write)
     prev_write->frame = o->cur->frame;
   EINA_COW_WRITE_END(evas_object_image_state_cow, o->prev, prev_write);

   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
     state_write->frame = frame_index;
   EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);

   o->changed = EINA_TRUE;
   evas_object_change(eo_obj, obj);

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_canvas_image_efl_gfx_frame_controller_frame_set(Eo *eo_obj, void *_pd EINA_UNUSED, int frame_index)
{
   return _evas_image_animated_frame_set(eo_obj, frame_index);
}

/**
 * @internal
 * @brief Gets the current frame index of an animated image.
 * @param[in] eo_obj The Evas_Object (image).
 * @return The current frame index, or EINA_FALSE (0) if not applicable or on error.
 */
int
_evas_image_animated_frame_get(const Eo *eo_obj)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (!o->cur->f) return EINA_FALSE; // No file loaded
   if (!evas_object_image_animated_get(eo_obj)) return EINA_FALSE; // Not animated
   return o->cur->frame;
}

EOLIAN static int
_efl_canvas_image_efl_gfx_frame_controller_frame_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   return _evas_image_animated_frame_get(eo_obj);
}

EOLIAN static Eina_Size2D
_efl_canvas_image_efl_gfx_buffer_buffer_size_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   // Returns the current pixel dimensions of the image data.
   // Example: {w=640, h=480}
   return EINA_SIZE2D(o->cur->image.w, o->cur->image.h);
}

/**
 * @internal
 * @brief Core logic for setting pixel data for an image.
 *
 * This function handles setting new pixel data, potentially resizing the image,
 * and updating its internal state. It can either take ownership of the provided
 * slice memory (if @p copy is @c EINA_FALSE) or make a copy.
 *
 * @param[in] obj The protected data of the Evas_Object (image).
 * @param[in,out] o The internal image data structure.
 * @param[in] slice The Eina_Slice containing the pixel data.
 *                  Example for ARGB8888: slice.mem points to an array of uint32_t,
 *                  where each uint32_t is 0xAARRGGBB.
 *                  slice.len is (stride * h).
 * @param[in] w Width of the new pixel data. Example: 640.
 * @param[in] h Height of the new pixel data. Example: 480.
 * @param[in] stride Stride (bytes per row) of the new pixel data. Example: 640*4 for ARGB8888.
 * @param[in] cspace Colorspace of the new pixel data. Example: EFL_GFX_COLORSPACE_ARGB8888.
 * @param[in] plane The plane index if this is planar data (e.g., YUV). Example: 0 for Y plane.
 * @param[in] copy If @c EINA_TRUE, data is copied. If @c EINA_FALSE, data is managed (slice memory is used directly).
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_image_pixels_set(Evas_Object_Protected_Data *obj,
                  Evas_Image_Data *o, const Eina_Slice *slice,
                  int w, int h, int stride, Efl_Gfx_Colorspace cspace, int plane,
                  Eina_Bool copy)
{
   Eina_Bool resized = EINA_FALSE, ret = EINA_FALSE;
   int int_stride = 0;

   // FIXME: buffer border support is not implemented

   if (ENFN->image_data_maps_get)
     {
        if (ENFN->image_data_maps_get(ENC, o->engine_data, NULL) > 0)
          {
             ERR("can not set pixels when there are open memory maps");
             return EINA_FALSE;
          }
     }

   if (o->pixels_checked_out)
     {
        // is there anything to do?
        ERR("Calling efl_gfx_buffer_%s_set after evas_object_image_data_get is "
            "not valid.", copy ? "copy" : "managed");
        return EINA_FALSE;
     }

   if (o->engine_data)
     {
        Evas_Colorspace ics;
        int iw = 0, ih = 0;
        Eina_Bool alpha;

        ENFN->image_size_get(ENC, o->engine_data, &iw, &ih);
        ics = ENFN->image_colorspace_get(ENC, o->engine_data);
        alpha = ENFN->image_alpha_get(ENC, o->engine_data);
        if ((w != iw) || (h != ih) || (ics != (Evas_Colorspace)cspace) || (alpha != o->cur->has_alpha))
          {
             ENFN->image_free(ENC, o->engine_data);
             o->engine_data = NULL;
          }
     }

   if (!slice || !slice->mem)
     {
        // note: we release all planes at once
        if (o->engine_data)
          ENFN->image_free(ENC, o->engine_data);
        o->engine_data = ENFN->image_new_from_copied_data(ENC, w, h, NULL, o->cur->has_alpha, (Evas_Colorspace)cspace);
     }
   else
     {
        o->buffer_data_set = EINA_TRUE;
        o->engine_data = ENFN->image_data_slice_add(ENC, o->engine_data,
                                                    slice, copy, w, h, stride,
                                                    (Evas_Colorspace)cspace, plane, o->cur->has_alpha);
     }

   if (!o->engine_data)
     {
        ERR("Failed to create internal image");
        goto end;
     }

   if ((o->cur->image.w != w) || (o->cur->image.h != h))
     resized = EINA_TRUE;

   if (ENFN->image_scale_hint_set)
     ENFN->image_scale_hint_set(ENC, o->engine_data, o->scale_hint);

   if (ENFN->image_content_hint_set)
     ENFN->image_content_hint_set(ENC, o->engine_data, o->content_hint);

   if (ENFN->image_stride_get)
     ENFN->image_stride_get(ENC, o->engine_data, &int_stride);

   if (resized || o->cur->f || o->cur->key ||
       (o->cur->image.stride != int_stride) || (cspace != (Efl_Gfx_Colorspace)o->cur->cspace))
     {
        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, cur)
        {
           cur->f = NULL;
           cur->key = NULL;
           cur->cspace = (Evas_Colorspace)cspace;
           cur->image.w = w;
           cur->image.h = h;
           cur->image.stride = int_stride;
        }
        EINA_COW_IMAGE_STATE_WRITE_END(o, cur)
     }

   ret = EINA_TRUE;

end:
   o->written = EINA_TRUE;
   if (resized)
     evas_object_inform_call_image_resize(obj->object);

   efl_gfx_buffer_update_add(obj->object, NULL);
   return ret;
}

EOLIAN static Eina_Bool
_efl_canvas_image_efl_gfx_buffer_buffer_managed_set(Eo *eo_obj, void *_pd EINA_UNUSED,
                                                    const Eina_Slice *slice,
                                                    Eina_Size2D size, int stride,
                                                    Efl_Gfx_Colorspace cspace,
                                                    int plane)
{
   // Sets pixel data that Evas will manage (not copy).
   // slice: Pixel data. Example: {mem=(const uint8_t*)pixels, len=width*height*4} for ARGB8888.
   // size: Dimensions. Example: {w=100, h=100}.
   // stride: Bytes per row. Example: 100*4.
   // cspace: Colorspace. Example: EFL_GFX_COLORSPACE_ARGB8888.
   // plane: Data plane index. Example: 0.
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   return _image_pixels_set(obj, o, slice, size.w, size.h, stride, cspace, plane, EINA_FALSE);
}

EOLIAN static Eina_Bool
_efl_canvas_image_efl_gfx_buffer_buffer_copy_set(Eo *eo_obj, void *_pd EINA_UNUSED,
                                                 const Eina_Slice *slice, Eina_Size2D size, int stride,
                                                 Efl_Gfx_Colorspace cspace, int plane)
{
   // Sets pixel data by copying it into Evas' internal buffers.
   // Parameters are similar to _efl_canvas_image_efl_gfx_buffer_buffer_managed_set.
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   return _image_pixels_set(obj, o, slice, size.w, size.h, stride, cspace, plane, EINA_TRUE);
}

EOLIAN static Eina_Slice
_efl_canvas_image_efl_gfx_buffer_buffer_managed_get(Eo *eo_obj, void *_pd EINA_UNUSED,
                                                    int plane)
{
   // Retrieves a slice to the internally managed pixel data for a specific plane.
   // This is only valid if data was set via buffer_managed_set or if the loader
   // provides direct access. The memory is managed by Evas.
   // plane: Data plane index. Example: 0.
   // Returns: Eina_Slice to the data. Example: {mem=(const uint8_t*)pixels, len=width*height*4}.
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   Evas_Colorspace cspace = EVAS_COLORSPACE_ARGB8888;
   Eina_Slice slice = {};

   if (!o->buffer_data_set || !o->engine_data || !ENFN->image_data_direct_get)
     return slice;

   ENFN->image_data_direct_get(ENC, o->engine_data, plane, &slice, &cspace, EINA_FALSE, NULL);

   return slice;
}

EOLIAN static Eina_Rw_Slice
_efl_canvas_image_efl_gfx_buffer_buffer_map(Eo *eo_obj, void *_pd EINA_UNUSED,
                                            Efl_Gfx_Buffer_Access_Mode mode,
                                            const Eina_Rect *region,
                                            Efl_Gfx_Colorspace cspace,
                                            int plane, int *stride)
{
   // Maps a region of the image's pixel buffer for direct access.
   // mode: Access mode (read, write, read/write). Example: EFL_GFX_BUFFER_ACCESS_MODE_READ.
   // region: The rectangular area to map. Example: {x=0, y=0, w=100, h=100}. If NULL, maps whole image.
   // cspace: Desired colorspace for the mapped buffer. Example: EFL_GFX_COLORSPACE_ARGB8888.
   // plane: Data plane index. Example: 0.
   // stride: Output for the stride of the mapped buffer. Example: *stride = 100*4.
   // Returns: A writable/readable slice to the mapped memory.
   //          Example: {mem=(uint8_t*)pixels, len=region_w*region_h*bytes_per_pixel, cap=..., rw=EINA_TRUE}.
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   int s = 0, width = 0, height = 0;
   Eina_Rw_Slice slice = {};
   int x, y, w, h;

   if (!ENFN->image_data_map)
     goto end; // not implemented

   if (o->engine_data)
     ENFN->image_size_get(ENC, o->engine_data, &width, &height);

   if (!o->engine_data || !width || !height)
     {
        // TODO: Create a map_surface and draw there. Maybe. This could
        // depend on the flags (eg. add a "force render" flag).
        WRN("This image image has no data available");
        goto end;
     }

   if (region)
     {
        x = region->x;
        y = region->y;
        w = region->w;
        h = region->h;
     }
   else
     {
        x = y = 0;
        w = width;
        h = height;
     }

   if ((x < 0) || (y < 0) || (w <= 0) || (h <= 0) || ((x + w) > width) || ((y + h) > height))
     {
        ERR("Invalid map dimensions: %dx%d +%d,%d. Image is %dx%d.",
            w, h, x, y, width, height);
        goto end;
     }

   if (ENFN->image_data_map(ENC, &o->engine_data, &slice, &s, x, y, w, h, (Evas_Colorspace)cspace, mode, plane))
     {
        DBG("map(%p, %d,%d %dx%d plane:%d) -> " EINA_SLICE_FMT,
            eo_obj, x, y, w, h, plane, EINA_SLICE_PRINT(slice));
     }
   else DBG("map(%p, %d,%d %dx%d plane:%d) -> (null)", eo_obj, x, y, w, h, plane);

end:
   if (stride) *stride = s;
   return slice;
}

EOLIAN static Eina_Bool
_efl_canvas_image_efl_gfx_buffer_buffer_unmap(Eo *eo_obj, void *_pd EINA_UNUSED,
                                              Eina_Rw_Slice slice)
{
   // Unmaps a previously mapped pixel buffer region.
   // slice: The Eina_Rw_Slice that was returned by buffer_map.
   //        Example: The slice obtained from a previous buffer_map call.
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (!slice.mem || !ENFN->image_data_unmap || !o->engine_data)
     return EINA_FALSE;

   if (!ENFN->image_data_unmap(ENC, o->engine_data, &slice))
     return EINA_FALSE;

   return EINA_TRUE;
}

EOLIAN static void
_efl_canvas_image_efl_object_dbg_info_get(Eo *obj, void *pd EINA_UNUSED, Efl_Dbg_Info *root)
{
   efl_dbg_info_get(efl_super(obj, MY_CLASS), root);

   // If there's a load error, add it to the debug information.
   if ((efl_gfx_image_load_error_get(obj) != EFL_GFX_IMAGE_LOAD_ERROR_NONE) &&
       (root))
     {
        Efl_Dbg_Info *group = EFL_DBG_INFO_LIST_APPEND(root, MY_CLASS_NAME);
        Eina_Error error = EFL_GFX_IMAGE_LOAD_ERROR_GENERIC;

        error = efl_gfx_image_load_error_get(obj);
        // Append the human-readable error message.
        // Example for root:
        // root (Efl_Dbg_Info)
        //  |- group (Efl_Dbg_Info, name="Efl.Canvas.Image")
        //     |- "Load Error": "Generic error" (EINA_VALUE_TYPE_STRING)
        EFL_DBG_INFO_APPEND(group, "Load Error", EINA_VALUE_TYPE_STRING,
                            eina_error_msg_get(error));
     }
}

#define EFL_CANVAS_IMAGE_EXTRA_OPS \
   EFL_OBJECT_OP_FUNC(efl_dbg_info_get, _efl_canvas_image_efl_object_dbg_info_get)

#include "efl_canvas_image.eo.c"
