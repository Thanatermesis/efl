#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Emile.h>

#include "Evas_Loader.h"

/**
 * @internal
 * @brief Internal data structure for the TGV image loader.
 *
 * This structure holds the state required for loading a TGV image,
 * including the Emile image handle and any specified loading region.
 */
typedef struct _Evas_Loader_Internal Evas_Loader_Internal;
struct _Evas_Loader_Internal
{
   Emile_Image *image; /**< Handle to the opened Emile image object. */

   Eina_Rectangle region; /**< Optional region to load from the image. If width/height are <= 0, the whole image is loaded. */
};

/**
 * @internal
 * @brief Opens a TGV image file for loading.
 *
 * This function is called by Evas to open a TGV image file. It uses the
 * Emile library to handle the actual file opening and TGV decoding setup.
 *
 * @param f An Eina_File handle representing the opened file.
 * @param key Optional key for encrypted files (unused for TGV).
 * @param opts Load options, including potential region loading.
 * @param animated Pointer to store animated properties (unused for TGV).
 * @param error Pointer to store the error code on failure.
 * @return A handle (Evas_Loader_Internal *) to the loader instance on success,
 *         NULL on failure.
 * @see evas_image_load_file_close_tgv()
 * @see emile_image_tgv_file_open()
 */
static void *
evas_image_load_file_open_tgv(Eina_File *f, Eina_Stringshare *key EINA_UNUSED,
                              Evas_Image_Load_Opts *opts,
                              Evas_Image_Animated *animated EINA_UNUSED,
                              int *error)
{
   Evas_Loader_Internal *loader;
   Emile_Image *image;
   Emile_Image_Load_Error image_error;

   image = emile_image_tgv_file_open(f, opts ? &(opts->emile) : NULL,
                                     NULL, &image_error);
   if (!image)
     {
        *error = image_error;
        return NULL;
     }

   loader = calloc(1, sizeof (Evas_Loader_Internal));
   if (!loader)
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return NULL;
     }

   loader->image = image;
   if (opts && (opts->emile.region.w > 0) && (opts->emile.region.h > 0))
     {
        EINA_RECTANGLE_SET(&loader->region,
                           opts->emile.region.x,
                           opts->emile.region.y,
                           opts->emile.region.w,
                           opts->emile.region.h);
     }
   else
     {
        EINA_RECTANGLE_SET(&loader->region,
                           0, 0,
                           -1, -1);
     }

   return loader;
}

/**
 * @internal
 * @brief Closes a previously opened TGV image file handle.
 *
 * This function is called by Evas to release resources associated with
 * an opened TGV image. It closes the Emile image handle and frees the
 * internal loader data.
 *
 * @param loader_data The loader instance handle returned by
 *                    evas_image_load_file_open_tgv().
 * @see evas_image_load_file_open_tgv()
 * @see emile_image_close()
 */
static void
evas_image_load_file_close_tgv(void *loader_data)
{
   Evas_Loader_Internal *loader = loader_data;

   emile_image_close(loader->image);
   free(loader);
}

/**
 * @internal
 * @brief Reads the header information of an opened TGV image.
 *
 * This function retrieves metadata about the image, such as dimensions,
 * format, and alpha channel presence, without loading the pixel data.
 *
 * @param loader_data The loader instance handle.
 * @param prop Pointer to an Emile_Image_Property struct to be filled with
 *             header information. Example structure:
 *             `{ .w = 640, .h = 480, .alpha = EINA_TRUE, ... }`
 * @param error Pointer to store the error code on failure.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @see emile_image_head()
 */
static Eina_Bool
evas_image_load_file_head_tgv(void *loader_data,
                              Emile_Image_Property *prop,
                              int *error)
{
   Evas_Loader_Internal *loader = loader_data;
   Emile_Image_Load_Error image_error;
   Eina_Bool ret;

   ret = emile_image_head(loader->image,
                          prop, sizeof (*prop),
                          &image_error);
   *error = image_error;

   return ret;
}

/**
 * @internal
 * @brief Loads the actual pixel data of an opened TGV image.
 *
 * This function reads the image pixel data into the provided buffer.
 * The header must have been successfully read before calling this.
 *
 * @param loader_data The loader instance handle.
 * @param prop Pointer to an Emile_Image_Property struct containing header
 *             information (used by Emile, usually the same as filled by head).
 * @param pixels Pointer to the memory buffer where the decoded pixel data
 *               (usually ARGB) will be stored. The buffer must be large
 *               enough to hold `prop->w * prop->h * sizeof(int)`.
 * @param error Pointer to store the error code on failure.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @see emile_image_data()
 * @see evas_image_load_file_head_tgv()
 */
Eina_Bool
evas_image_load_file_data_tgv(void *loader_data,
                              Emile_Image_Property *prop,
                              void *pixels,
                              int *error)
{
   Evas_Loader_Internal *loader = loader_data;
   Emile_Image_Load_Error image_error;
   Eina_Bool ret;

   ret = emile_image_data(loader->image,
                          prop, sizeof (*prop),
                          pixels,
                          &image_error);
   *error = image_error;
   return ret;
}

/**
 * @brief Structure defining the Evas image loader functions for TGV format.
 *
 * This structure maps the internal TGV loading functions to the standard
 * Evas image loader interface.
 */
Evas_Image_Load_Func evas_image_load_tgv_func =
{
  EVAS_IMAGE_LOAD_VERSION,
  evas_image_load_file_open_tgv, /**< Function to open a TGV file. */
  evas_image_load_file_close_tgv, /**< Function to close a TGV file. */
  (void*) evas_image_load_file_head_tgv, /**< Function to read the TGV header. */
  NULL, /**< Function to read animated properties (unused). */
  (void*) evas_image_load_file_data_tgv, /**< Function to load TGV pixel data. */
  NULL, /**< Function to load frame duration (unused). */
  EINA_TRUE, /**< Indicates this loader supports loading regions. */
  EINA_FALSE /**< Indicates this loader does not support frame-by-frame loading directly (handled by Emile). */
};

/**
 * @internal
 * @brief Evas module initialization function.
 *
 * Called by Evas when loading this image loader module. It registers the
 * TGV loader functions.
 *
 * @param em The Evas module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_image_load_tgv_func);
   return 1;
}

/**
 * @internal
 * @brief Evas module shutdown function.
 *
 * Called by Evas when unloading this image loader module.
 * Currently does nothing.
 *
 * @param em The Evas module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
}

/**
 * @brief Evas module API structure.
 *
 * Defines the module's metadata and entry points (open/close).
 */
static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION, /**< Evas module API version. */
   "tgv", /**< Module name. */
   "none", /**< Module license (placeholder). */
   {
     module_open, /**< Module open function. */
     module_close /**< Module close function. */
   }
};

/**< Macro to define the Evas module metadata. */
/**< Macro to define the Evas module metadata. */
EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, tgv);

#ifndef EVAS_STATIC_BUILD_TGV
/**< Macro to define the Eina module entry point for dynamic loading. */
EVAS_EINA_MODULE_DEFINE(image_loader, tgv);
#endif
