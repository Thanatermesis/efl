#include "evas_gl_private.h"

/**
 * @internal
 * @brief Ensures that the underlying RGBA_Image data is allocated.
 *
 * This function is a wrapper around evas_cache_image_size_set. It is used
 * to make sure that an image has its pixel buffer allocated before it is
 * used, for instance before being modified or rendered.
 *
 * @param im The Evas_GL_Image to allocate.
 */
void
evas_gl_common_image_alloc_ensure(Evas_GL_Image *im)
{
   if (!im->im) return;
   im->im = (RGBA_Image *)evas_cache_image_size_set(&im->im->cache_entry,
                                                    im->w, im->h);
}

/**
 * @internal
 * @brief Unloads all image data from memory and GPU.
 *
 * This function iterates through all images associated with the given
 * GL context and unloads their pixel data from CPU memory via the image
 * cache. It also frees any associated GL textures that are not dynamic.
 * This is useful for freeing resources when the application is minimized
 * or backgrounded.
 *
 * @param gc The Evas GL engine context.
 */
EMODAPI void
evas_gl_common_image_all_unload(Evas_Engine_GL_Context *gc)
{
   Eina_List *l;
   Evas_GL_Image *im;

   EINA_LIST_FOREACH(gc->shared->images, l, im)
     {
        if (im->im)
          evas_cache_image_unload_data(&im->im->cache_entry);
        if (im->tex)
          {
             if (!im->tex->pt->dyn.img)
               {
                  evas_gl_common_texture_free(im->tex, EINA_TRUE);
                  im->tex = NULL;
               }
          }
     }
}

/**
 * @internal
 * @brief Trims the image cache to respect the configured cache size.
 *
 * This function is called when the total size of cached images exceeds the
 * limit set by `evas_common_image_get_cache()`. It iterates through the
 * list of images in reverse (starting from the least recently used) and
 * removes any unreferenced images until the cache size is within the limit.
 *
 * @param gc The Evas GL engine context.
 */
static void
_evas_gl_image_cache_trim(Evas_Engine_GL_Context *gc)
{
   int size = evas_common_image_get_cache();

   while (gc->shared->images_size > size)
     {
        Evas_GL_Image *im2;
        Eina_List *l = NULL;
        Eina_Bool removed = EINA_FALSE;

        EINA_LIST_REVERSE_FOREACH(gc->shared->images, l, im2)
          {
             if (im2->references == 0)
               {
                  im2->cached = 0;
                  im2->gc->shared->images =
                     eina_list_remove_list(im2->gc->shared->images, l);
                  im2->gc->shared->images_size -= (im2->csize);
                  evas_gl_common_image_free(im2);
                  l = NULL;
                  removed = EINA_TRUE;
                  break;
               }
          }

        if (!removed || !gc->shared->images)
          {
             // still have referenced images - need to let others release
             // refs on their own
             break;
          }
     }
}

/**
 * @internal
 * @brief Adds an unreferenced image to the cache or removes a referenced one.
 *
 * This function manages an image's presence in the cache. If an image has no
 * references, it is added to the cache, its size is accounted for, and the
 * cache is trimmed if necessary. If the image has references, it is removed
 * from the cache list.
 *
 * @param im The image to process.
 * @return @c EINA_TRUE if the image was added to the cache, @c EINA_FALSE
 * otherwise.
 */
static Eina_Bool
_evas_gl_image_cache_add(Evas_GL_Image *im)
{
   if (im->references == 0)
     {
        if (im->cached)
          {
             im->csize = im->w * im->h * 4;
             im->gc->shared->images_size += im->csize;
             _evas_gl_image_cache_trim(im->gc);
             if (!eina_list_data_find(im->gc->shared->images, im))
               { // FIXME for a messed up caching system... this used to be simple
                  im->gc->shared->images = eina_list_prepend(im->gc->shared->images, im);
               }
             return EINA_TRUE;
          }
     }
   else
     {
        im->gc->shared->images = eina_list_remove(im->gc->shared->images, im);
        im->cached = 0;
     }
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Increments the reference count of an Evas_GL_Image.
 *
 * When an image's reference count goes from 0 to 1, it is considered "in use"
 * and is removed from the cache-size accounting, though it may remain in the
 * `images` list.
 *
 * @param im The image to reference.
 */
EMODAPI void
evas_gl_common_image_ref(Evas_GL_Image *im)
{
   if (im->references == 0)
     {
        im->gc->shared->images_size -= (im->csize);
     }
   im->references++;
}

/**
 * @internal
 * @brief Decrements the reference count of an Evas_GL_Image.
 *
 * If the reference count drops to zero, the image is no longer considered
 * "in use" and is added to the cache via `_evas_gl_image_cache_add()`.
 *
 * @param im The image to unreference.
 */
EMODAPI void
evas_gl_common_image_unref(Evas_GL_Image *im)
{
   im->references--;
   if (im->references == 0)
     {
        _evas_gl_image_cache_add(im);
     }
}

/**
 * @internal
 * @brief Populates the list of supported colorspaces for the GL engine.
 *
 * This function checks for hardware support for various texture compression
 * formats (like ETC1, ETC2, S3TC) and populates the `cspaces` list in the
 * shared GL context information. This list is then used to select the most
 * optimal colorspace for loaded images.
 *
 * @param gc The Evas GL engine context.
 */
static void
_evas_gl_cspace_list_fill(Evas_Engine_GL_Context *gc)
{
#define CS_APPEND(cs) gc->shared->info.cspaces = eina_list_append \
   (gc->shared->info.cspaces, (void *) (intptr_t) cs)
   if (gc->shared->info.etc2)
     {
        CS_APPEND(EVAS_COLORSPACE_RGBA8_ETC2_EAC);
        CS_APPEND(EVAS_COLORSPACE_RGB8_ETC2);
        CS_APPEND(EVAS_COLORSPACE_ETC1);
        CS_APPEND(EVAS_COLORSPACE_ETC1_ALPHA);
     }
   else if (gc->shared->info.etc1)
     {
        CS_APPEND(EVAS_COLORSPACE_ETC1);
        CS_APPEND(EVAS_COLORSPACE_ETC1_ALPHA);
     }
   if (gc->shared->info.s3tc)
     {
        CS_APPEND(EVAS_COLORSPACE_RGB_S3TC_DXT1);
        CS_APPEND(EVAS_COLORSPACE_RGBA_S3TC_DXT1);
        CS_APPEND(EVAS_COLORSPACE_RGBA_S3TC_DXT2);
        CS_APPEND(EVAS_COLORSPACE_RGBA_S3TC_DXT3);
        CS_APPEND(EVAS_COLORSPACE_RGBA_S3TC_DXT4);
        CS_APPEND(EVAS_COLORSPACE_RGBA_S3TC_DXT5);
     }
   CS_APPEND(EVAS_COLORSPACE_GRY8);
   CS_APPEND(EVAS_COLORSPACE_AGRY88);
   CS_APPEND(EVAS_COLORSPACE_ARGB8888);
}

/**
 * @internal
 * @brief Callback executed when image data preloading is complete.
 *
 * This function is registered as a callback with the image cache. When the
 * image loader finishes preloading an image header, this function is called
 * to update the Evas_GL_Image properties (like dimensions, alpha, colorspace)
 * from the now-available image data. It selects the best colorspace based
 * on hardware capabilities.
 *
 * @param data A pointer to the Evas_GL_Image.
 */
void
evas_gl_common_image_preload_done(void *data)
{
   Evas_GL_Image *im = data;

   if (im->im)
     {
        Evas_Colorspace cspace = EVAS_COLORSPACE_ARGB8888;

        if (im->im->cache_entry.cspaces)
          {
             Evas_Colorspace cs;
             unsigned int i;
             Eina_List *l2;
             void *ldata;

             cspace = EVAS_COLORSPACE_ARGB8888;
             for (i = 0;
                  im->im->cache_entry.cspaces[i] != EVAS_COLORSPACE_ARGB8888;
                  i++)
               {
                  EINA_LIST_FOREACH(im->gc->shared->info.cspaces, l2, ldata)
                    {
                       cs = (Evas_Colorspace) (intptr_t) ldata;
                       if (cs == im->im->cache_entry.cspaces[i])
                         {
                            cspace = cs;
                            goto found_cspace;
                         }
                    }
               }
found_cspace:
             if (cspace == EVAS_COLORSPACE_ETC1 && im->gc->shared->info.etc2)
               cspace = EVAS_COLORSPACE_RGB8_ETC2;
             im->im->cache_entry.space = cspace;
          }
        im->cs.space = cspace;
        im->orient = EVAS_IMAGE_ORIENT_NONE;
        im->alpha = im->im->cache_entry.flags.alpha;
        im->w = im->im->cache_entry.w;
        im->h = im->im->cache_entry.h;
     }
}

/**
 * @internal
 * @brief Removes the preload callback for a given image.
 *
 * This is a workaround to stop watching for preload completion. It iterates
 * through the cache entry's targets and removes the one that matches this
 * image's preload callback. This is typically done when the image is being
 * freed.
 *
 * @param im The image for which to remove the preload watcher.
 */
//FIXME: This is a hacky way. Need an proper interface...
void
evas_gl_common_image_preload_unwatch(Evas_GL_Image *im)
{
   Eina_Inlist *l2;
   Evas_Cache_Target *tg;

   if (!im->im) return;
   EINA_INLIST_FOREACH_SAFE(im->im->cache_entry.targets, l2, tg)
     {
        if ((tg->preloaded_cb != evas_gl_common_image_preload_done) || (tg->preloaded_data != im))
          continue;
        tg->delete_me = EINA_TRUE;
        break;
     }
}

/**
 * @internal
 * @brief Creates a new Evas_GL_Image from an existing RGBA_Image.
 *
 * This function wraps an RGBA_Image (which is a CPU-side image representation
 * from Evas' cache) in an Evas_GL_Image. It first checks if a GL image for
 * the given RGBA_Image already exists to avoid duplication. If not, it
 * allocates a new Evas_GL_Image, determines the optimal colorspace, and
 * adds it to the list of managed images.
 *
 * @param gc The Evas GL engine context.
 * @param im_im The source RGBA_Image.
 * @param lo The load options used for loading the image.
 * @param[out] error A pointer to an integer where a load error code will be stored.
 * @return A new or existing Evas_GL_Image, or @c NULL on failure.
 */
Evas_GL_Image *
evas_gl_common_image_new_from_rgbaimage(Evas_Engine_GL_Context *gc, RGBA_Image *im_im,
                                        Evas_Image_Load_Opts *lo, int *error)
{
   Evas_GL_Image *im;
   Eina_List     *l;
   Evas_Colorspace cspace = EVAS_COLORSPACE_ARGB8888;

   /* i'd LOVe to do this, but we can't because we load to load header
    * to get image size to know if its too big or not! so this disallows
    * us to know that - photocam thus suffers
   if (((int)im_im->cache_entry.w > gc->shared->info.max_texture_size) ||
       ((int)im_im->cache_entry.h > gc->shared->info.max_texture_size))
     {
        evas_cache_image_drop(&(im_im->cache_entry));
        if (error) *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return NULL;
     }
    */

   if (error) *error = EVAS_LOAD_ERROR_NONE;

   // FIXME: keep unreffed shared images around
   EINA_LIST_FOREACH(gc->shared->images, l, im)
     {
        if (im->im == im_im)
          {
             evas_cache_image_drop(&(im_im->cache_entry));
             gc->shared->images = eina_list_remove_list(gc->shared->images, l);
             gc->shared->images = eina_list_prepend(gc->shared->images, im);
             evas_gl_common_image_ref(im);
             if (error) *error = EVAS_LOAD_ERROR_NONE;
             return im;
          }
     }

   im = calloc(1, sizeof(Evas_GL_Image));
   if (!im)
     {
        evas_cache_image_drop(&(im_im->cache_entry));
        if (error) *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return NULL;
     }
   if (im_im->cache_entry.cspaces)
     {
        Evas_Colorspace cs;
        unsigned int i;
        Eina_List *l2;
        void *ldata;

        if (!gc->shared->info.cspaces)
          _evas_gl_cspace_list_fill(gc);

        cspace = EVAS_COLORSPACE_ARGB8888;
        for (i = 0; im_im->cache_entry.cspaces[i] != EVAS_COLORSPACE_ARGB8888; i++)
          EINA_LIST_FOREACH(gc->shared->info.cspaces, l2, ldata)
            {
               cs = (Evas_Colorspace) (intptr_t) ldata;
               if (cs == im_im->cache_entry.cspaces[i])
                 {
                    cspace = cs;
                    goto found_cspace;
                 }
            }

found_cspace:
        // ETC2 is backwards compatible with ETC1 but we prefer ETC2
        if (cspace == EVAS_COLORSPACE_ETC1 && gc->shared->info.etc2)
          cspace = EVAS_COLORSPACE_RGB8_ETC2;

        im_im->cache_entry.space = cspace;
     }

   im->references = 1;
   im->im = im_im;
   im->gc = gc;
   im->cached = 1;
   im->cs.space = cspace;
   im->orient = EVAS_IMAGE_ORIENT_NONE;
   im->alpha = im->im->cache_entry.flags.alpha;
   im->w = im->im->cache_entry.w;
   im->h = im->im->cache_entry.h;
   if (lo) im->load_opts = *lo;
   gc->shared->images = eina_list_prepend(gc->shared->images, im);
   return im;
}

/**
 * @internal
 * @brief Loads an image from a file and creates an Evas_GL_Image for it.
 *
 * This is a convenience function that first calls the common image loader
 * to get an RGBA_Image from a file, and then calls
 * `evas_gl_common_image_new_from_rgbaimage()` to wrap it in a GL-specific
 * image object.
 *
 * @param gc The Evas GL engine context.
 * @param file The path to the image file.
 * @param key Optional key for image identification in the cache.
 * @param lo Load options for the image.
 * @param[out] error A pointer to an integer where a load error code will be stored.
 * @return A new Evas_GL_Image, or @c NULL on failure.
 */
Evas_GL_Image *
evas_gl_common_image_load(Evas_Engine_GL_Context *gc, const char *file, const char *key, Evas_Image_Load_Opts *lo, int *error)
{
   RGBA_Image *im_im;

   im_im = evas_common_load_image_from_file(file, key, lo, error);
   if (!im_im) return NULL;

   return evas_gl_common_image_new_from_rgbaimage(gc, im_im, lo, error);
}

/**
 * @internal
 * @brief Loads an image from a memory-mapped file and creates an Evas_GL_Image.
 *
 * Similar to `evas_gl_common_image_load`, but loads the image from an
 * Eina_File (which could be a memory map).
 *
 * @param gc The Evas GL engine context.
 * @param f The Eina_File handle.
 * @param key Optional key for image identification in the cache.
 * @param lo Load options for the image.
 * @param[out] error A pointer to an integer where a load error code will be stored.
 * @return A new Evas_GL_Image, or @c NULL on failure.
 */
Evas_GL_Image *
evas_gl_common_image_mmap(Evas_Engine_GL_Context *gc, Eina_File *f, const char *key, Evas_Image_Load_Opts *lo, int *error)
{
   RGBA_Image *im_im;

   im_im = evas_common_load_image_from_mmap(f, key, lo, error);
   if (!im_im) return NULL;

   return evas_gl_common_image_new_from_rgbaimage(gc, im_im, lo, error);
}

/**
 * @internal
 * @brief Creates a new Evas_GL_Image from a raw data buffer.
 *
 * This function creates an image that uses the provided `data` buffer directly,
 * without copying it. The caller is responsible for ensuring the buffer remains
 * valid for the lifetime of the image. It checks for existing images that might
 * already be using the same data buffer.
 *
 * @param gc The Evas GL engine context.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param data Pointer to the pixel data. For YUV formats, this is a pointer
 * to an array of plane pointers. Example for `EVAS_COLORSPACE_YCBCR422P601_PL`:
 * @code
 * unsigned char *planes[3] = { y_plane, u_plane, v_plane };
 * evas_gl_common_image_new_from_data(gc, w, h, (DATA32 *)planes, 0,
 *                                    EVAS_COLORSPACE_YCBCR422P601_PL);
 * @endcode
 * @param alpha Whether the image has an alpha channel.
 * @param cspace The colorspace of the data.
 * @return A new Evas_GL_Image, or @c NULL on failure.
 */
EMODAPI Evas_GL_Image *
evas_gl_common_image_new_from_data(Evas_Engine_GL_Context *gc, unsigned int w, unsigned int h, DATA32 *data, int alpha, Evas_Colorspace cspace)
{
   Evas_GL_Image *im;
   Eina_List *l;

   if (((int)w > gc->shared->info.max_texture_size) ||
       ((int)h > gc->shared->info.max_texture_size))
     return NULL;

   if (data)
     {
        EINA_LIST_FOREACH(gc->shared->images, l, im)
          {
             if (((void *)(im->im->image.data) == (void *)data) &&
                 (im->im->cache_entry.w == w) &&
                 (im->im->cache_entry.h == h))
               {
                  gc->shared->images = eina_list_remove_list(gc->shared->images, l);
                  gc->shared->images = eina_list_prepend(gc->shared->images, im);
                  evas_gl_common_image_ref(im);
                  return im;
               }
          }
     }
   im = calloc(1, sizeof(Evas_GL_Image));
   if (!im) return NULL;
   im->references = 1;
   im->im = (RGBA_Image *) evas_cache_image_data(evas_common_image_cache_get(),
                                                 w, h, data, alpha, cspace);
   if (!im->im)
     {
	free(im);
	return NULL;
     }
   im->gc = gc;
   im->cs.space = cspace;
   im->alpha = im->im->cache_entry.flags.alpha;
   im->w = im->im->cache_entry.w;
   im->h = im->im->cache_entry.h;
   switch (cspace)
     {
      case EVAS_COLORSPACE_ARGB8888:
      case EVAS_COLORSPACE_GRY8:
      case EVAS_COLORSPACE_AGRY88:
        break;
      case EVAS_COLORSPACE_ETC1:
      case EVAS_COLORSPACE_ETC1_ALPHA:
        if (gc->shared->info.etc1 && !gc->shared->info.etc2) break;
        ERR("We don't know what to do with ETC1 on this hardware. You need to add a software converter here.");
        break;
      case EVAS_COLORSPACE_RGB8_ETC2:
      case EVAS_COLORSPACE_RGBA8_ETC2_EAC:
        if (gc->shared->info.etc2) break;
        ERR("We don't know what to do with ETC2 on this hardware. You need to add a software converter here.");
        break;
      case EVAS_COLORSPACE_YCBCR422P601_PL:
      case EVAS_COLORSPACE_YCBCR422P709_PL:
	im->cs.data = data;
	im->cs.no_free = 1;
	break;
      default:
        ERR("color space not supported: %d", cspace);
	break;
     }
   return im;
}

/**
 * @internal
 * @brief Creates a new Evas_GL_Image by copying from a raw data buffer.
 *
 * This function creates an image by allocating new memory and copying the
 * provided pixel data into it. This is safer than `evas_gl_common_image_new_from_data`
 * as the caller does not need to maintain the original data buffer.
 *
 * @param gc The Evas GL engine context.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param data Pointer to the source pixel data. For YUV formats, this is a pointer
 * to an array of plane pointers, similar to `evas_gl_common_image_new_from_data`.
 * @param alpha Whether the image has an alpha channel.
 * @param cspace The colorspace of the data.
 * @return A new Evas_GL_Image, or @c NULL on failure.
 */
Evas_GL_Image *
evas_gl_common_image_new_from_copied_data(Evas_Engine_GL_Context *gc, unsigned int w, unsigned int h, DATA32 *data, int alpha, Evas_Colorspace cspace)
{
   Evas_GL_Image *im;

   if (((int)w > gc->shared->info.max_texture_size) ||
       ((int)h > gc->shared->info.max_texture_size))
     return NULL;

   im = calloc(1, sizeof(Evas_GL_Image));
   if (!im) return NULL;
   im->references = 1;
   im->im = (RGBA_Image *) evas_cache_image_copied_data(evas_common_image_cache_get(),
                                                        w, h, data, alpha, cspace);
   if (!im->im)
     {
	free(im);
	return NULL;
     }
   im->gc = gc;
   im->cs.space = cspace;
   im->alpha = im->im->cache_entry.flags.alpha;
   im->w = im->im->cache_entry.w;
   im->h = im->im->cache_entry.h;
   switch (cspace)
     {
      case EVAS_COLORSPACE_ARGB8888:
      case EVAS_COLORSPACE_GRY8:
      case EVAS_COLORSPACE_AGRY88:
        break;
      case EVAS_COLORSPACE_ETC1:
      case EVAS_COLORSPACE_ETC1_ALPHA:
        if (gc->shared->info.etc1 && !gc->shared->info.etc2) break;
        ERR("We don't know what to do with ETC1 on this hardware. You need to add a software converter here.");
        break;
      case EVAS_COLORSPACE_RGB8_ETC2:
      case EVAS_COLORSPACE_RGBA8_ETC2_EAC:
        if (gc->shared->info.etc2) break;
        ERR("We don't know what to do with ETC2 on this hardware. You need to add a software converter here.");
        break;
      case EVAS_COLORSPACE_YCBCR422P601_PL:
      case EVAS_COLORSPACE_YCBCR422P709_PL:
        if (im->im->cache_entry.h > 0)
          im->cs.data = calloc(1, im->im->cache_entry.h * sizeof(unsigned char *) * 2);
        if ((data) && (im->cs.data))
          memcpy(im->cs.data, data, im->im->cache_entry.h * sizeof(unsigned char *) * 2);
        break;
      default:
        ERR("color space not supported: %d", cspace);
        break;
     }
   return im;
}

/**
 * @internal
 * @brief Creates a new, empty (blank) Evas_GL_Image.
 *
 * This function allocates an Evas_GL_Image of the specified dimensions and
 * colorspace, but does not initialize its pixel data (it will be cleared to
 * black/transparent). The underlying image buffer is managed by the Evas
 * image cache.
 *
 * @param gc The Evas GL engine context.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param alpha Whether the image should have an alpha channel.
 * @param cspace The colorspace of the image.
 * @return A new Evas_GL_Image, or @c NULL on failure.
 */
Evas_GL_Image *
evas_gl_common_image_new(Evas_Engine_GL_Context *gc, unsigned int w, unsigned int h, int alpha, Evas_Colorspace cspace)
{
   Evas_GL_Image *im;

   if (((int)w > gc->shared->info.max_texture_size) ||
       ((int)h > gc->shared->info.max_texture_size))
     return NULL;

   im = calloc(1, sizeof(Evas_GL_Image));
   if (!im) return NULL;
   im->references = 1;
   im->im = (RGBA_Image *)evas_cache_image_empty(evas_common_image_cache_get());
   if (!im->im)
     {
	free(im);
	return NULL;
     }
   im->gc = gc;
   im->im->cache_entry.flags.alpha = alpha ? 1 : 0;
   im->cs.space = cspace;
   im->alpha = im->im->cache_entry.flags.alpha;
   im->im->cache_entry.w = w;
   im->im->cache_entry.h = h;
   im->w = im->im->cache_entry.w;
   im->h = im->im->cache_entry.h;
   evas_cache_image_colorspace(&im->im->cache_entry, cspace);
   im->im = (RGBA_Image *)evas_cache_image_size_set(&im->im->cache_entry, w, h);
   switch (cspace)
     {
      case EVAS_COLORSPACE_ARGB8888:
      case EVAS_COLORSPACE_GRY8:
      case EVAS_COLORSPACE_AGRY88:
         break;
      case EVAS_COLORSPACE_ETC1:
      case EVAS_COLORSPACE_ETC1_ALPHA:
        if (gc->shared->info.etc1 && !gc->shared->info.etc2) break;
        ERR("We don't know what to do with ETC1 on this hardware. You need to add a software converter here.");
        break;
      case EVAS_COLORSPACE_RGB8_ETC2:
      case EVAS_COLORSPACE_RGBA8_ETC2_EAC:
        if (gc->shared->info.etc2) break;
        ERR("We don't know what to do with ETC2 on this hardware. You need to add a software converter here.");
        break;
      case EVAS_COLORSPACE_YCBCR422P601_PL:
      case EVAS_COLORSPACE_YCBCR422P709_PL:
      case EVAS_COLORSPACE_YCBCR422601_PL:
      case EVAS_COLORSPACE_YCBCR420NV12601_PL:
      case EVAS_COLORSPACE_YCBCR420TM12601_PL:
//        if (im->tex) evas_gl_common_texture_free(im->tex);
	im->tex = NULL;
	im->cs.no_free = 0;
        if (im->im->cache_entry.h > 0)
          im->cs.data = calloc(1, im->im->cache_entry.h * sizeof(unsigned char *) * 2);
	break;
      default:
	abort();
	break;
     }
   return im;
}

/**
 * @internal
 * @brief Sets the alpha channel flag for an image.
 *
 * This function modifies the alpha flag of an image. If the flag changes, it
 * may trigger a re-creation and re-upload of the associated GL texture to
 * ensure it has the correct format (e.g., RGB vs. RGBA).
 *
 * @param im The image to modify.
 * @param alpha @c 1 to enable alpha, @c 0 to disable.
 * @return The modified Evas_GL_Image, or @c NULL on failure.
 */
Evas_GL_Image *
evas_gl_common_image_alpha_set(Evas_GL_Image *im, int alpha)
{
   if (!im) return NULL;
   if (im->alpha == alpha) return im;
   im->alpha = alpha;
   if (!im->im) return im;
   evas_gl_common_image_alloc_ensure(im);
   evas_cache_image_load_data(&im->im->cache_entry);
   im->im->cache_entry.flags.alpha = alpha ? 1 : 0;

   if (im->tex) evas_gl_common_texture_free(im->tex, EINA_TRUE);
   if (im->tex_only)
     {
        im->tex = evas_gl_common_texture_native_new(im->gc, im->w, im->h,
                                                    im->alpha, im);
     }
   else
     {
        im->tex = evas_gl_common_texture_new(im->gc, im->im, EINA_FALSE);
        if (im->tex) evas_gl_common_texture_update(im->tex, im->im);
     }
   return im;
}

/**
 * @internal
 * @brief Converts an image to use a native texture surface.
 *
 * This function transitions an Evas_GL_Image from being backed by CPU-side
 * pixel data (in `im->im`) to being backed by a native texture. This is
 * often used for zero-copy video playback where the texture is managed by an
 * external entity (e.g., a hardware decoder). It frees any CPU-side buffers
 * and creates a new native GL texture.
 *
 * @param im The image to convert.
 */
EMODAPI void
evas_gl_common_image_native_enable(Evas_GL_Image *im)
{
   if (im->cs.data)
     {
	if (!im->cs.no_free) free(im->cs.data);
        im->cs.data = NULL;
     }
   im->cs.no_free = 0;
   if (im->cached)
     {
        if (im->references == 0)
           im->gc->shared->images_size -= (im->csize);
        im->gc->shared->images = eina_list_remove(im->gc->shared->images, im);
        im->cached = 0;
     }
   if (im->im)
     {
        evas_cache_image_drop(&im->im->cache_entry);
        im->im = NULL;
     }
   if (im->tex)
     {
        evas_gl_common_texture_free(im->tex, EINA_TRUE);
        im->tex = NULL;
     }

   im->cs.space = EVAS_COLORSPACE_ARGB8888;
   im->tex = evas_gl_common_texture_native_new(im->gc, im->w, im->h, im->alpha, im);
   im->tex_only = 1;
}

/**
 * @internal
 * @brief Converts an image from using a native texture back to a standard image.
 *
 * This function reverses the effect of `evas_gl_common_image_native_enable`.
 * It frees the native texture and replaces it with a standard, empty,
 * CPU-backed Evas image cache entry. The image content is lost in this
 * process.
 *
 * @param im The image to convert.
 */
EMODAPI void
evas_gl_common_image_native_disable(Evas_GL_Image *im)
{
   if (im->im)
     {
        evas_cache_image_drop(&im->im->cache_entry);
        im->im = NULL;
     }
   if (im->tex)
     {
        evas_gl_common_texture_free(im->tex, EINA_TRUE);
        im->tex = NULL;
     }
   im->tex_only = 0;
   im->im = (RGBA_Image *)evas_cache_image_empty(evas_common_image_cache_get());
   im->im->cache_entry.flags.alpha = im->alpha;
   im->cs.space = EVAS_COLORSPACE_ARGB8888;
   evas_cache_image_colorspace(&im->im->cache_entry, im->cs.space);
/*
   im->im = (RGBA_Image *)evas_cache_image_size_set(&im->im->cache_entry, im->w, im->h);
   if (!im->tex)
     im->tex = evas_gl_common_texture_new(im->gc, im->im);
 */
}

/**
 * @internal
 * @brief Sets a hint about the scaling behavior of an image.
 *
 * This hint can be used by the engine to optimize texture memory usage. For
 * example, if an image is always scaled down, the engine could potentially
 * create a smaller texture to save memory.
 *
 * @param im The image to modify.
 * @param hint The scaling hint (e.g., EVAS_IMAGE_SCALE_HINT_STATIC).
 */
void
evas_gl_common_image_scale_hint_set(Evas_GL_Image *im, int hint)
{
   im->scale_hint = hint;
   // FIXME: take advantage of this even in gl (eg if image is
   // 1600x1200 but we always use it at 800x600 or even less - drop
   // the texture res down for "non dynamic" stuff to save memory)
}

/**
 * @internal
 * @brief Sets a hint about the content of an image.
 *
 * This hint indicates whether the image content is expected to change
 * frequently (dynamic) or not (static). For dynamic content, the engine
 * can switch to using special texture types (like TBM or EGLImage) that
 * allow for more efficient updates, especially in zero-copy scenarios.
 *
 * @param im The image to modify.
 * @param hint The content hint (e.g., EVAS_IMAGE_CONTENT_HINT_DYNAMIC).
 */
void
evas_gl_common_image_content_hint_set(Evas_GL_Image *im, int hint)
{
   if (im->content_hint == hint) return;
   if ((im->gc) &&
       ((!im->gc->shared->info.sec_image_map) &&
        ((!im->gc->shared->info.sec_tbm_surface) ||
         (!im->gc->shared->info.egl_tbm_ext))))
     return;
   im->content_hint = hint;
   if (!im->gc) return;
   if (!im->gc->shared->info.bgra) return;
   // does not handle yuv yet.
   // TODO: Check this list of cspaces
   switch (im->cs.space)
     {
      case EVAS_COLORSPACE_YCBCR422P601_PL:
      case EVAS_COLORSPACE_YCBCR422P709_PL:
      case EVAS_COLORSPACE_RGB565_A5P:
      case EVAS_COLORSPACE_YCBCR422601_PL:
      case EVAS_COLORSPACE_YCBCR420NV12601_PL:
      case EVAS_COLORSPACE_YCBCR420TM12601_PL:
      case EVAS_COLORSPACE_ETC1_ALPHA:
        return;
      default: break;
     }
   if (im->content_hint == EVAS_IMAGE_CONTENT_HINT_DYNAMIC)
     {
        if ((!im->gc->shared->info.sec_image_map) &&
            ((!im->gc->shared->info.sec_tbm_surface) || (!im->gc->shared->info.egl_tbm_ext))) return;
        if (im->cs.data)
          {
             if (!im->cs.no_free) free(im->cs.data);
             im->cs.data = NULL;
          }
        im->cs.no_free = 0;
        if (im->cached)
          {
             if (im->references == 0)
                im->gc->shared->images_size -= im->csize;
             im->gc->shared->images = eina_list_remove(im->gc->shared->images, im);
             im->cached = 0;
          }
        if (im->im)
          {
             evas_cache_image_drop(&im->im->cache_entry);
             im->im = NULL;
          }
        if (im->tex)
          {
             evas_gl_common_texture_free(im->tex, EINA_TRUE);
             im->tex = NULL;
          }
        im->tex = evas_gl_common_texture_dynamic_new(im->gc, im);
        im->tex_only = 1;
     }
   else
     {
        if (im->im)
          {
             evas_cache_image_drop(&im->im->cache_entry);
             im->im = NULL;
          }
        if (im->tex)
          {
             evas_gl_common_texture_free(im->tex, EINA_TRUE);
             im->tex = NULL;
          }
        im->tex_only = 0;

        im->im = (RGBA_Image *)evas_cache_image_empty(evas_common_image_cache_get());
        im->im->cache_entry.flags.alpha = im->alpha;
        im->im->cache_entry.space = im->cs.space;
        evas_cache_image_colorspace(&im->im->cache_entry, im->cs.space);
        im->im = (RGBA_Image *)evas_cache_image_size_set(&im->im->cache_entry, im->w, im->h);
        if (!im->tex)
           im->tex = evas_gl_common_texture_new(im->gc, im->im, EINA_FALSE);
     }
}

/**
 * @internal
 * @brief Flushes the image cache.
 *
 * This function attempts to trim the image cache by freeing unreferenced
 * images.
 *
 * @param gc The Evas GL engine context.
 */
void
evas_gl_common_image_cache_flush(Evas_Engine_GL_Context *gc)
{
   _evas_gl_image_cache_trim(gc);
}

/**
 * @internal
 * @brief Frees an Evas_GL_Image, possibly caching it.
 *
 * This function decrements the image's reference count. If the count becomes
 * greater than zero, the function does nothing else. If it drops to zero,
 * the image is either freed or added to a cache of unused images for later
 * reuse. It handles freeing all associated resources like GL textures and
 * CPU-side data.
 *
 * @param im The image to free.
 */
EMODAPI void
evas_gl_common_image_free(Evas_GL_Image *im)
{
   if (!im) return;

   im->references--;
   if (im->references > 0) return;

   if (im->gc && (im->gc->pipe[0].shader.surface == im))
     evas_gl_common_context_target_surface_set(im->gc, im->gc->def_surface);

   if (im->fglyph)
     {
        if (im->gc)
          im->gc->font_glyph_images = eina_list_remove(im->gc->font_glyph_images, im);
        im->fglyph->ext_dat = NULL;
        im->fglyph->ext_dat_free = NULL;
     }
   else if ((im->gc) && (im->gc->shared))
     {
        im->gc->shared->images = eina_list_remove(im->gc->shared->images, im);
     }

   if (im->gc)
     evas_gl_common_context_flush(im->gc);

   evas_gl_common_image_preload_unwatch(im);

   if (im->scaled.origin)
     {
        evas_gl_common_image_free(im->scaled.origin);
        im->scaled.origin = NULL;
     }

   if (im->native.func.free)
     im->native.func.free(im);

   if (im->cs.data)
     {
        if (!im->cs.no_free) free(im->cs.data);
     }
   if (im->cached && im->gc)
     {
        if (_evas_gl_image_cache_add(im)) return;
     }
   if (im->tex)
     {
        if (!evas_gl_common_texture_free(im->tex, EINA_TRUE))
          {
             /* if texture is not freed, we need to assign im to NULL
                because after this point im will be freed */
             im->tex->im = NULL;
          }
     }
   if (im->im)
     evas_cache_image_drop(&im->im->cache_entry);

   free(im);
}

/**
 * @internal
 * @brief Creates a new Evas_GL_Image that is a render target (surface).
 *
 * This function creates an image that is not backed by a CPU buffer but
 * directly by a renderable GL texture (via a Framebuffer Object). This is
 * used for rendering to a texture.
 *
 * @param gc The Evas GL engine context.
 * @param w The width of the surface.
 * @param h The height of the surface.
 * @param alpha Whether the surface should have an alpha channel.
 * @param stencil Whether the surface should have a stencil buffer.
 * @return A new surface Evas_GL_Image, or @c NULL on failure.
 */
Evas_GL_Image *
evas_gl_common_image_surface_new(Evas_Engine_GL_Context *gc, unsigned int w, unsigned int h, int alpha, int stencil)
{
   Evas_GL_Image *im;

   if (((int)w > gc->shared->info.max_texture_size) ||
       ((int)h > gc->shared->info.max_texture_size))
     return NULL;

   im = calloc(1, sizeof(Evas_GL_Image));
   if (!im) return NULL;
   im->references = 1;
   im->gc = gc;
   im->cs.space = EVAS_COLORSPACE_ARGB8888;
   im->alpha = alpha;
   im->w = w;
   im->h = h;
   im->tex = evas_gl_common_texture_render_new(gc, w, h, alpha, stencil);
   im->tex_only = 1;
   return im;
}

/**
 * @internal
 * @brief Creates a new Evas_GL_Image surface that is not scaled.
 *
 * This is similar to `evas_gl_common_image_surface_new`, but creates a
 * texture that is not meant to be scaled by the hardware. This can be an
 * optimization for surfaces that are always rendered 1:1.
 *
 * @param gc The Evas GL engine context.
 * @param w The width of the surface.
 * @param h The height of the surface.
 * @param alpha Whether the surface should have an alpha channel.
 * @return A new surface Evas_GL_Image, or @c NULL on failure.
 */
Evas_GL_Image *
evas_gl_common_image_surface_noscale_new(Evas_Engine_GL_Context *gc, unsigned int w, unsigned int h, int alpha)
{
   Evas_GL_Image *im;

   if (((int)w > gc->shared->info.max_texture_size) ||
       ((int)h > gc->shared->info.max_texture_size))
     return NULL;

   im = calloc(1, sizeof(Evas_GL_Image));
   if (!im) return NULL;
   im->references = 1;
   im->gc = gc;
   im->cs.space = EVAS_COLORSPACE_ARGB8888;
   im->alpha = alpha;
   im->w = w;
   im->h = h;
   im->tex = evas_gl_common_texture_render_noscale_new(gc, w, h, alpha);
   im->tex_only = 1;
   return im;
}

/**
 * @internal
 * @brief Marks a region of an image as dirty.
 *
 * This function flags that a portion of the image's CPU-side data has
 * changed. This will cause the corresponding texture region to be updated
 * on the GPU during the next rendering pass (`evas_gl_common_image_update`).
 *
 * @param im The image to mark as dirty.
 * @param x The x-coordinate of the dirty rectangle.
 * @param y The y-coordinate of the dirty rectangle.
 * @param w The width of the dirty rectangle.
 * @param h The height of the dirty rectangle.
 */
void
evas_gl_common_image_dirty(Evas_GL_Image *im, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
   if ((w == 0) && (h == 0) && (x == 0) && (y == 0))
     {
        w = im->w;
        h = im->h;
     }
   if (im->im)
     {
        evas_gl_common_image_alloc_ensure(im);
        im->im = (RGBA_Image *)evas_cache_image_dirty(&im->im->cache_entry, x, y, w, h);
     }
   im->dirty = 1;
}

/**
 * @internal
 * @brief Updates the GL texture for an image if necessary.
 *
 * This function is central to synchronizing CPU-side image data with the
 * GPU. If the image has no GL texture yet, it creates one. If the image has
 * been marked as dirty (or is animated), it uploads the modified data to the
 * existing GL texture. It handles various colorspaces and formats, including
 * YUV planes.
 *
 * @param gc The Evas GL engine context.
 * @param im The image to update.
 */
void
evas_gl_common_image_update(Evas_Engine_GL_Context *gc, Evas_GL_Image *im)
{
   Image_Entry *ie;
   if (!im->im) return;

   ie = &im->im->cache_entry;
   if (!im->tex && ie->preload) return;

   evas_gl_common_image_alloc_ensure(im);
   // alloc ensure can change im->im, so only get the local variable later.
   ie = &im->im->cache_entry;
/*
   if ((im->cs.space == EVAS_COLORSPACE_YCBCR422P601_PL) ||
       (im->cs.space == EVAS_COLORSPACE_YCBCR422P709_PL))
     {
        // SOFTWARE convert. do multi texture later
        if ((im->cs.data) && (*((unsigned char **)im->cs.data)))
          {
             if (im->dirty || !im->im->image.data)
               {
                  free(im->im->image.data);
                  im->im->image.data = malloc(im->im->cache_entry.w * im->im->cache_entry.h * sizeof(DATA32));
                  if (im->im->image.data)
                    evas_common_convert_yuv_422p_601_rgba(im->cs.data,
                                                          (void *)im->im->image.data,
                                                          im->im->cache_entry.w, im->im->cache_entry.h);
               }
          }
        space = EVAS_COLORSPACE_ARGB8888;
     }
   else
 */
   switch (im->cs.space)
     {
      case EVAS_COLORSPACE_ARGB8888:
      case EVAS_COLORSPACE_GRY8:
      case EVAS_COLORSPACE_AGRY88:
      case EVAS_COLORSPACE_ETC1:
      case EVAS_COLORSPACE_RGB8_ETC2:
      case EVAS_COLORSPACE_RGBA8_ETC2_EAC:
      case EVAS_COLORSPACE_RGB_S3TC_DXT1:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT1:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT2:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT3:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT4:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT5:
         if ((im->tex) &&
             ((im->dirty) || (ie->animated.animated) || (ie->flags.updated_data)))
          {
             ie->load_error = evas_cache_image_load_data(ie);
             evas_gl_common_texture_update(im->tex, im->im);
             evas_cache_image_unload_data(ie);
          }
        else if (!im->tex &&
                 ((ie->load_error == EVAS_LOAD_ERROR_NONE) ||
                  (ie->load_error == EVAS_LOAD_ERROR_CANCELLED)))
          {
             ie->load_error = evas_cache_image_load_data(ie);
             im->tex = evas_gl_common_texture_new(gc, im->im, im->disable_atlas);
             evas_cache_image_unload_data(ie);
          }
        ie->flags.updated_data = EINA_FALSE;
        im->dirty = 0;
        break;
      case EVAS_COLORSPACE_ETC1_ALPHA:
        if ((im->tex) && (im->dirty))
          {
             ie->load_error = evas_cache_image_load_data(ie);
             evas_gl_common_texture_rgb_a_pair_update(im->tex, im->im);
             evas_cache_image_unload_data(ie);
          }
        else if (!im->tex &&
                 ((ie->load_error == EVAS_LOAD_ERROR_NONE) ||
                  (ie->load_error == EVAS_LOAD_ERROR_CANCELLED)))
          {
             ie->load_error = evas_cache_image_load_data(ie);
             im->tex = evas_gl_common_texture_rgb_a_pair_new(gc, im->im);
             evas_cache_image_unload_data(ie);
          }
        im->dirty = 0;
        break;
      case EVAS_COLORSPACE_YCBCR422P601_PL:
      case EVAS_COLORSPACE_YCBCR422P709_PL:
        if ((im->tex) && (im->dirty))
          {
             evas_gl_common_texture_yuv_update(im->tex, im->cs.data, ie->w, ie->h);
             im->dirty = 0;
          }
        if ((!im->tex) && (im->cs.data) && (*((unsigned char **)im->cs.data)))
          {
             im->tex = evas_gl_common_texture_yuv_new(gc, im->cs.data, ie->w, ie->h);
             im->dirty = 0;
          }
        break;
      case EVAS_COLORSPACE_YCBCR422601_PL:
        if ((im->tex) && (im->dirty))
          {
             evas_gl_common_texture_yuy2_update(im->tex, im->cs.data, ie->w, ie->h);
             im->dirty = 0;
          }
        if ((!im->tex) && (im->cs.data) && (*((unsigned char **)im->cs.data)))
          {
             im->tex = evas_gl_common_texture_yuy2_new(gc, im->cs.data, ie->w, ie->h);
             im->dirty = 0;
          }
        break;
      case EVAS_COLORSPACE_YCBCR420NV12601_PL:
        if ((im->tex) && (im->dirty))
          {
             evas_gl_common_texture_nv12_update(im->tex, im->cs.data, ie->w, ie->h);
             im->dirty = 0;
          }
        if ((!im->tex) && (im->cs.data) && (*((unsigned char **)im->cs.data)))
          {
             im->tex = evas_gl_common_texture_nv12_new(gc, im->cs.data, ie->w, ie->h);
             im->dirty = 0;
          }
        break;
      case EVAS_COLORSPACE_YCBCR420TM12601_PL:
        if ((im->tex) && (im->dirty))
          {
             evas_gl_common_texture_nv12tiled_update(im->tex, im->cs.data, ie->w, ie->h);
             im->dirty = 0;
          }
        if ((!im->tex) && (im->cs.data) && (*((unsigned char **)im->cs.data)))
          {
             im->tex = evas_gl_common_texture_nv12tiled_new(gc, im->cs.data, ie->w, ie->h);
             im->dirty = 0;
          }
        break;
      default:
        ERR("unhandled img format colorspace=%d", im->cs.space);
        break;
    }
}

/**
 * @internal
 * @brief Updates a GL surface with pixel data from a CPU-backed image.
 *
 * This function is used to upload pixel data into an image that is a render
 * surface. It effectively converts a CPU-data image into a GPU-only surface,
 * by creating a new surface, copying the pixels, and freeing the original
 * image.
 *
 * @param im The source Evas_GL_Image with CPU pixel data.
 * @return The new surface-based Evas_GL_Image, or @c NULL on failure.
 *         The original `im` is freed on success.
 */
Evas_GL_Image *
evas_gl_common_image_surface_update(Evas_GL_Image *im)
{
   Evas_Engine_GL_Context *gc;
   Evas_GL_Image *glim = NULL;
   Eina_Bool alpha;
   int w, h;

   if (!im || !im->gc || !im->im || !im->im->image.data)
     goto fail;

   if (im->im->cache_entry.space == (Evas_Colorspace)EFL_GFX_COLORSPACE_ARGB8888)
     alpha = EINA_FALSE;
   else if (im->im->cache_entry.space == (Evas_Colorspace)EFL_GFX_COLORSPACE_GRY8)
     alpha = EINA_TRUE;
   else goto fail;

   gc = im->gc;
   w = im->im->cache_entry.w;
   h = im->im->cache_entry.h;
   glim = evas_gl_common_image_surface_new(gc, w, h, EINA_TRUE, EINA_FALSE);
   if (!glim) goto fail;

   if (alpha)
     {
        RGBA_Image *image;
        uint32_t *rgba;
        uint8_t *gry8;
        int k;

        image = evas_common_image_new(w, h, EINA_TRUE);
        if (!image) goto fail;

        rgba = image->image.data;
        gry8 = im->im->image.data8;
        for (k = 0; k < (w * h); k++)
          {
             const int c = *gry8++;
             *rgba++ = ARGB_JOIN(c, c, c, c);
          }

        glim->im = image;
     }
   else
     {
        evas_cache_image_ref(&im->im->cache_entry);
        glim->im = im->im;
     }

   glim->dirty = EINA_TRUE;
   evas_gl_common_image_update(gc, glim);
   evas_gl_common_image_free(im);

   return glim;

fail:
   ERR("Failed to update surface pixels!");
   if (glim) evas_gl_common_image_free(glim);
   return NULL;
}

/**
 * @internal
 * @brief Detaches the CPU-side RGBA_Image from a surface.
 *
 * After pixels have been uploaded to a surface texture, the CPU-side buffer
 * may no longer be needed. This function drops the reference to the cache
 * entry, allowing the memory to be freed.
 *
 * @param im The surface image.
 * @return The modified surface image.
 */
Evas_GL_Image *
evas_gl_common_image_surface_detach(Evas_GL_Image *im)
{
   if (!im || !im->im) return im;

   evas_cache_image_drop(&im->im->cache_entry);
   im->im = NULL;

   return im;
}

/**
 * @internal
 * @brief Creates or updates a "virtual" scaled image.
 *
 * This function provides a memory-efficient way to handle scaled images,
 * particularly for masks. It doesn't create a new scaled texture. Instead, it
 * creates a new `Evas_GL_Image` that re-uses the texture of the original
 * `image` but stores different dimensions (`dst_w`, `dst_h`). The actual
 * scaling is handled by the GPU using texture coordinates at draw time.
 *
 * This is an optimization to avoid allocating large temporary surfaces for
 * operations like masked rendering where the mask is scaled.
 *
 * @param scaled A previously returned scaled image to be updated, or @c NULL.
 * @param image The original source image.
 * @param dst_w The desired width of the virtual scaled image.
 * @param dst_h The desired height of the virtual scaled image.
 * @param smooth Whether scaling should be smooth (bilinear filtering).
 * @return A new or updated `Evas_GL_Image` representing the scaled version.
 */
Evas_GL_Image *
evas_gl_common_image_virtual_scaled_get(Evas_GL_Image *scaled, Evas_GL_Image *image,
                                        int dst_w, int dst_h, Eina_Bool smooth)
{
   Evas_GL_Image *dst = scaled, *newdst;
   Evas_GL_Image *src = image;
   Evas_Engine_GL_Context *gc;
   Eina_Bool reffed = EINA_FALSE;

   if (!src) return NULL;

   // masking will work only with single texture images
   switch (src->cs.space)
     {
      case EVAS_COLORSPACE_AGRY88:
      case EVAS_COLORSPACE_ARGB8888:
      case EVAS_COLORSPACE_GRY8:
      case EVAS_COLORSPACE_RGBA8_ETC2_EAC:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT1:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT2:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT3:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT4:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT5:
        break;
      default:
        DBG("cspace %d can't be used for masking's fast path", src->cs.space);
        return NULL;
     }

   gc = src->gc;
   if (dst && (dst->scaled.origin == src) &&
       (dst->w == dst_w) && (dst->h == dst_h))
     return dst;

   evas_gl_common_image_update(gc, src);
   if (!src->tex)
     {
        ERR("No source texture.");
        return NULL;
     }

   newdst = calloc(1, sizeof(Evas_GL_Image));
   if (!newdst) return NULL;

   if (dst)
     {
        if (dst->scaled.origin == src)
          {
             if (dst->references == 1)
               {
                  dst->w = dst_w;
                  dst->h = dst_h;
                  dst->scaled.smooth = smooth;
                  free(newdst);
                  return dst;
               }
             src->references++;
             reffed = EINA_TRUE;
          }
        evas_gl_common_image_free(dst);
     }

   newdst->references = 1;
   newdst->gc = gc;
   newdst->cs.space = src->cs.space;
   newdst->alpha = src->alpha;
   newdst->w = dst_w;
   newdst->h = dst_h;
   newdst->tex = src->tex;
   newdst->tex->references++;
   newdst->tex_only = 1;

   if (!reffed) src->references++;
   newdst->scaled.origin = src;
   newdst->scaled.smooth = smooth;

   return newdst;
}

/**
 * @internal
 * @brief Draws an image mapped onto a 3D-transformed quad.
 *
 * This function draws an image (or parts of it) with perspective
 * transformation. It takes an array of map points, each defining a vertex
 * with 3D coordinates (x, y, z), texture coordinates (u, v), and color.
 * The function pushes these quads to the GL context for rendering.
 *
 * @param gc The Evas GL engine context.
 * @param im The source image to draw.
 * @param npoints The number of points in the map (must be a multiple of 4).
 * @param p An array of `RGBA_Map_Point` defining the vertices.
 *        Example of a single quad's points:
 *        @code
 *        RGBA_Map_Point p[4];
 *        p[0].x, p[0].y, p[0].z; // 3D coordinates
 *        p[0].u, p[0].v;       // Texture coordinates
 *        p[0].col;             // Vertex color
 *        // ... and so on for p[1], p[2], p[3]
 *        @endcode
 * @param smooth Enables smooth (bilinear) filtering for the texture.
 * @param level Unused.
 */
void
evas_gl_common_image_map_draw(Evas_Engine_GL_Context *gc, Evas_GL_Image *im,
                              int npoints, RGBA_Map_Point *p, int smooth, int level EINA_UNUSED)
{
   int mx = 0, my = 0, mw = 0, mh = 0;
   RGBA_Draw_Context *dc = gc->dc;
   Eina_Bool mask_smooth = EINA_FALSE;
   Evas_GL_Image *mask = dc->clip.mask;
   Evas_GL_Texture *mtex = NULL;
   Eina_Bool mask_color = EINA_FALSE;
   int r, g, b, a;
   int c, cx, cy, cw, ch;
   int offset = 0;

   if (dc->mul.use)
     {
        a = (dc->mul.col >> 24) & 0xff;
        r = (dc->mul.col >> 16) & 0xff;
        g = (dc->mul.col >> 8 ) & 0xff;
        b = (dc->mul.col      ) & 0xff;
     }
   else
     {
        r = g = b = a = 255;
     }

   evas_gl_common_image_update(gc, im);

   c = dc->clip.use;
   cx = dc->clip.x;   cy = dc->clip.y;
   cw = dc->clip.w;   ch = dc->clip.h;
   if (!im->tex) return;
   im->tex->im = im;

   if (mask)
     {
        evas_gl_common_image_update(gc, mask);
        mtex = mask->tex;
        if (mtex && mtex->pt && mtex->pt->w && mtex->pt->h)
          {
             // canvas coords
             mx = gc->dc->clip.mask_x;
             my = gc->dc->clip.mask_y;
             mw = mask->w;
             mh = mask->h;
             mask_smooth = mask->scaled.smooth;
             mask_color = dc->clip.mask_color;
          }
        else mtex = NULL;
     }

   while (npoints >= 4)
     {
        evas_gl_common_context_image_map_push(gc, im->tex, npoints, &p[offset],
                                              c, cx, cy, cw, ch,
                                              mtex, mx, my, mw, mh, mask_smooth, mask_color,
                                              r, g, b, a, smooth, im->tex_only,
                                              im->cs.space);
        offset += 4;
        npoints -= 4;
     }
}

/**
 * @internal
 * @brief Pushes a clipped and masked image draw operation to the GL pipeline.
 *
 * This is the core helper function for drawing scaled/clipped images. It takes
 * source and destination rectangles, a clip rectangle, and a mask, and calculates
 * the final geometry and texture coordinates. It then calls the appropriate
 * context push function based on the image's colorspace (e.g., RGBA, YUV, etc.).
 *
 * @param gc The Evas GL engine context.
 * @param im The source image.
 * @param dx,dy,dw,dh Destination rectangle in canvas coordinates.
 * @param sx,sy,sw,sh Source rectangle in image coordinates.
 * @param cx,cy,cw,ch Clip rectangle in canvas coordinates.
 * @param r,g,b,a Color multiplier.
 * @param mask Optional mask image.
 * @param smooth Enable smooth scaling.
 * @param yuv,yuv_709,yuy2,nv12,rgb_a_pair Flags indicating special colorspaces.
 */
static void
_evas_gl_common_image_push(Evas_Engine_GL_Context *gc, Evas_GL_Image *im,
                           int dx, int dy, int dw, int dh,
                           int sx, int sy, int sw, int sh,
                           int cx, int cy, int cw, int ch,
                           int r, int g, int b, int a,
                           Evas_GL_Image *mask,
                           Eina_Bool smooth,
                           Eina_Bool yuv, Eina_Bool yuv_709,
                           Eina_Bool yuy2, Eina_Bool nv12,
                           Eina_Bool rgb_a_pair)
{
   int mx = 0, my = 0, mw = 0, mh = 0;
   double ssx, ssy, ssw, ssh;
   Evas_GL_Texture *mtex = NULL;
   Eina_Bool mask_smooth = EINA_FALSE;
   Eina_Bool mask_color = EINA_FALSE;
   int nx, ny, nw, nh;

   nx = dx; ny = dy; nw = dw; nh = dh;
   RECTS_CLIP_TO_RECT(nx, ny, nw, nh,
                      cx, cy, cw, ch);
   if ((nw < 1) || (nh < 1)) return;
   if (!im->tex) return;

   if (mask)
     {
        evas_gl_common_image_update(gc, mask);
        mtex = mask->tex;
        if (mtex && mtex->pt && mtex->pt->w && mtex->pt->h)
          {
             // canvas coords
             mx = gc->dc->clip.mask_x;
             my = gc->dc->clip.mask_y;
             mw = mask->w;
             mh = mask->h;
             mask_smooth = mask->scaled.smooth;
             mask_color = gc->dc->clip.mask_color;
          }
        else mtex = NULL;
     }

   if ((nx == dx) && (ny == dy) && (nw == dw) && (nh == dh))
     {
        if (yuv)
          evas_gl_common_context_yuv_push(gc,
                                          im->tex,
                                          sx, sy, sw, sh,
                                          dx, dy, dw, dh,
                                          mtex, mx, my, mw, mh, mask_smooth, mask_color,
                                          r, g, b, a,
                                          smooth);
        else if (yuv_709)
          evas_gl_common_context_yuv_709_push(gc,
                                              im->tex,
                                              sx, sy, sw, sh,
                                              dx, dy, dw, dh,
                                              mtex, mx, my, mw, mh, mask_smooth, mask_color,
                                              r, g, b, a,
                                              smooth);
        else if (yuy2)
          evas_gl_common_context_yuy2_push(gc,
                                           im->tex,
                                           sx, sy, sw, sh,
                                           dx, dy, dw, dh,
                                           mtex, mx, my, mw, mh, mask_smooth, mask_color,
                                           r, g, b, a,
                                           smooth);
        else if (nv12)
          evas_gl_common_context_nv12_push(gc,
                                           im->tex,
                                           sx, sy, sw, sh,
                                           dx, dy, dw, dh,
                                           mtex, mx, my, mw, mh, mask_smooth, mask_color,
                                           r, g, b, a,
                                           smooth);
        else if (rgb_a_pair)
          evas_gl_common_context_rgb_a_pair_push(gc,
                                                 im->tex,
                                                 sx, sy, sw, sh,
                                                 dx, dy, dw, dh,
                                                 mtex, mx, my, mw, mh, mask_smooth, mask_color,
                                                 r, g, b, a,
                                                 smooth);
        else
          evas_gl_common_context_image_push(gc,
                                            im->tex,
                                            sx, sy, sw, sh,
                                            dx, dy, dw, dh,
                                            mtex, mx, my, mw, mh, mask_smooth, mask_color,
                                            r, g, b, a,
                                            smooth, im->tex_only, EINA_FALSE);
        return;
     }

   ssx = (double)sx + ((double)(sw * (nx - dx)) / (double)(dw));
   ssy = (double)sy + ((double)(sh * (ny - dy)) / (double)(dh));
   ssw = ((double)sw * (double)(nw)) / (double)(dw);
   ssh = ((double)sh * (double)(nh)) / (double)(dh);

   if (yuv)
     evas_gl_common_context_yuv_push(gc,
                                     im->tex,
                                     ssx, ssy, ssw, ssh,
                                     nx, ny, nw, nh,
                                     mtex, mx, my, mw, mh, mask_smooth, mask_color,
                                     r, g, b, a,
                                     smooth);
   else if (yuv_709)
     evas_gl_common_context_yuv_709_push(gc,
                                         im->tex,
                                         ssx, ssy, ssw, ssh,
                                         nx, ny, nw, nh,
                                         mtex, mx, my, mw, mh, mask_smooth, mask_color,
                                         r, g, b, a,
                                         smooth);
   else if (yuy2)
     evas_gl_common_context_yuy2_push(gc,
                                      im->tex,
                                      ssx, ssy, ssw, ssh,
                                      nx, ny, nw, nh,
                                      mtex, mx, my, mw, mh, mask_smooth, mask_color,
                                      r, g, b, a,
                                      smooth);
   else if (nv12)
     evas_gl_common_context_nv12_push(gc,
                                      im->tex,
                                      ssx, ssy, ssw, ssh,
                                      nx, ny, nw, nh,
                                      mtex, mx, my, mw, mh, mask_smooth, mask_color,
                                      r, g, b, a,
                                      smooth);
   else if (rgb_a_pair)
     evas_gl_common_context_rgb_a_pair_push(gc,
                                            im->tex,
                                            ssx, ssy, ssw, ssh,
                                            nx, ny, nw, nh,
                                            mtex, mx, my, mw, mh, mask_smooth, mask_color,
                                            r, g, b, a,
                                            smooth);
   else
     evas_gl_common_context_image_push(gc,
                                       im->tex,
                                       ssx, ssy, ssw, ssh,
                                       nx, ny, nw, nh,
                                       mtex, mx, my, mw, mh, mask_smooth, mask_color,
                                       r, g, b, a,
                                       smooth, im->tex_only, EINA_FALSE);
}

/**
 * @internal
 * @brief Draws a scaled portion of an image to the canvas.
 *
 * This is the high-level function for drawing an image. It handles updating
 * the image texture if needed, applying color multipliers, and handling
 * cutouts (rendering the image in multiple pieces to avoid drawing over
 * occluding objects). It ultimately uses `_evas_gl_common_image_push` to
 * perform the actual drawing operations.
 *
 * @param gc The Evas GL engine context.
 * @param im The source image.
 * @param sx,sy,sw,sh The source rectangle within the image.
 * @param dx,dy,dw,dh The destination rectangle on the canvas.
 * @param smooth Enables smooth (bilinear) filtering if the image is scaled.
 */
void
evas_gl_common_image_draw(Evas_Engine_GL_Context *gc, Evas_GL_Image *im,
                          int sx, int sy, int sw, int sh,
                          int dx, int dy, int dw, int dh,
                          int smooth)
{
   RGBA_Draw_Context *dc;
   int r, g, b, a;
   Cutout_Rect  *rct;
   int c, cx, cy, cw, ch;
   int i;
   Eina_Bool yuv = EINA_FALSE;
   Eina_Bool yuv_709 = EINA_FALSE;
   Eina_Bool yuy2 = EINA_FALSE;
   Eina_Bool nv12 = EINA_FALSE;
   Eina_Bool rgb_a_pair = EINA_FALSE;
   Evas_GL_Image *mask;

   if (sw < 1) sw = 1;
   if (sh < 1) sh = 1;
   dc = gc->dc;
   if (dc->mul.use)
     {
	a = (dc->mul.col >> 24) & 0xff;
        r = (dc->mul.col >> 16) & 0xff;
        g = (dc->mul.col >> 8 ) & 0xff;
        b = (dc->mul.col      ) & 0xff;
     }
   else
     {
	r = g = b = a = 255;
     }

   evas_gl_common_image_update(gc, im);
   if (!im->tex)
     {
        evas_gl_common_rect_draw(gc, dx, dy, dw, dh);
        return;
     }

   mask = gc->dc->clip.mask;

   switch (im->cs.space)
     {
      case EVAS_COLORSPACE_YCBCR422P601_PL:
        yuv = EINA_TRUE;
        break;
      case EVAS_COLORSPACE_YCBCR422P709_PL:
        yuv_709 = EINA_TRUE;
        break;
      case EVAS_COLORSPACE_YCBCR422601_PL:
        yuy2 = EINA_TRUE;
        break;
      case EVAS_COLORSPACE_YCBCR420NV12601_PL:
      case EVAS_COLORSPACE_YCBCR420TM12601_PL:
        nv12 = EINA_TRUE;
        break;
      case EVAS_COLORSPACE_ETC1_ALPHA:
        rgb_a_pair = EINA_TRUE;
        break;
      default: break;
     }

   if ((sw == dw) && (sh == dh)) smooth = 0;

   im->tex->im = im;
   if ((!gc->dc->cutout.rects) ||
       ((gc->shared->info.tune.cutout.max > 0) &&
           (gc->dc->cutout.active > gc->shared->info.tune.cutout.max)))
     {
        if (mask || gc->dc->clip.use)
          {
             _evas_gl_common_image_push(gc, im,
                                        dx, dy, dw, dh,
                                        sx, sy, sw, sh,
                                        gc->dc->clip.x, gc->dc->clip.y,
                                        gc->dc->clip.w, gc->dc->clip.h,
                                        r, g, b, a,
                                        mask,
                                        smooth,
                                        yuv, yuv_709, yuy2, nv12, rgb_a_pair);
          }
        else
          {
             _evas_gl_common_image_push(gc, im,
                                        dx, dy, dw, dh,
                                        sx, sy, sw, sh,
                                        dx, dy, dw, dh,
                                        r, g, b, a,
                                        mask,
                                        smooth,
                                        yuv, yuv_709, yuy2, nv12, rgb_a_pair);
          }
        return;
     }

   /* save out clip info */
   c = gc->dc->clip.use; cx = gc->dc->clip.x; cy = gc->dc->clip.y; cw = gc->dc->clip.w; ch = gc->dc->clip.h;
   evas_common_draw_context_clip_clip(gc->dc, 0, 0, gc->shared->w, gc->shared->h);
   evas_common_draw_context_clip_clip(gc->dc, dx, dy, dw, dh);
   /* our clip is 0 size.. abort */
   if ((gc->dc->clip.w <= 0) || (gc->dc->clip.h <= 0))
     {
        gc->dc->clip.use = c; gc->dc->clip.x = cx; gc->dc->clip.y = cy; gc->dc->clip.w = cw; gc->dc->clip.h = ch;
        return;
     }
   _evas_gl_common_cutout_rects = evas_common_draw_context_apply_cutouts(dc, _evas_gl_common_cutout_rects);
   for (i = 0; i < _evas_gl_common_cutout_rects->active; ++i)
     {
        rct = _evas_gl_common_cutout_rects->rects + i;

        _evas_gl_common_image_push(gc, im,
                                   dx, dy, dw, dh,
                                   sx, sy, sw, sh,
                                   rct->x, rct->y, rct->w, rct->h,
                                   r, g, b, a,
                                   mask,
                                   smooth,
                                   yuv, yuv_709, yuy2, nv12, rgb_a_pair);
     }
   evas_common_draw_context_cutouts_free(_evas_gl_common_cutout_rects);
   /* restore clip info */
   gc->dc->clip.use = c; gc->dc->clip.x = cx; gc->dc->clip.y = cy; gc->dc->clip.w = cw; gc->dc->clip.h = ch;
}
