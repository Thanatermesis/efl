#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Emile.h>

#include "Evas_Loader.h"

/**
 * @internal
 * @brief Internal data structure for the JPEG loader.
 *
 * This structure holds the state required for loading a JPEG image,
 * including the Emile image handle and any specified loading region.
 */
typedef struct _Evas_Loader_Internal Evas_Loader_Internal;
struct _Evas_Loader_Internal
{
   Emile_Image *image; /**< Handle to the opened Emile image object. */

   Eina_Rectangle region; /**< The specific region to load, if requested. */
};

/**
 * @internal
 * @brief Opens a JPEG image file for loading.
 *
 * This function is called by Evas to open a JPEG image file. It uses the
 * Emile library to handle the actual file opening and prepares an internal
 * loader structure.
 *
 * @param f The Eina_File handle representing the opened file.
 * @param key Optional string key associated with the image file (unused).
 * @param opts Optional loading options, including region and scale down.
 * @param animated Unused parameter for animated images.
 * @param error Pointer to an integer where the error code will be stored
 *              in case of failure. See Evas_Load_Error codes.
 * @return A handle (void *) to the internal loader structure on success,
 *         NULL on failure.
 */
static void *
evas_image_load_file_open_jpeg(Eina_File *f, Eina_Stringshare *key EINA_UNUSED,
                              Evas_Image_Load_Opts *opts,
                              Evas_Image_Animated *animated EINA_UNUSED,
                              int *error)
{
   Evas_Loader_Internal *loader;
   Emile_Image *image;
   Emile_Image_Load_Error image_error;

   image = emile_image_jpeg_file_open(f, opts ? &(opts->emile) : NULL,
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
 * @brief Closes the JPEG image file and cleans up resources.
 *
 * This function is called by Evas when the image loading process is
 * finished or aborted. It closes the Emile image handle and frees the
 * internal loader structure.
 *
 * @param loader_data The handle returned by evas_image_load_file_open_jpeg().
 */
static void
evas_image_load_file_close_jpeg(void *loader_data)
{
   Evas_Loader_Internal *loader = loader_data;

   emile_image_close(loader->image);
   free(loader);
}

/**
 * @internal
 * @brief Reads the header information of the JPEG image.
 *
 * This function is called by Evas to get the properties of the image
 * (like dimensions, format, etc.) without loading the pixel data.
 * It uses the Emile library to retrieve this information.
 *
 * @param loader_data The handle returned by evas_image_load_file_open_jpeg().
 * @param prop Pointer to an Emile_Image_Property structure to be filled
 *             with image properties.
 * @param error Pointer to an integer where the error code will be stored
 *              in case of failure. See Evas_Load_Error codes.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_head_jpeg(void *loader_data,
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
 * @brief Callback function to check if the image loading task was cancelled.
 *
 * This function is registered with Emile and called periodically during
 * the image data loading process. It checks if Evas has requested the
 * loading task to be cancelled.
 *
 * @param data User data associated with the callback (unused).
 * @param image The Emile_Image being processed (unused).
 * @param action The action being performed (unused).
 * @return EINA_TRUE if the task should be cancelled, EINA_FALSE otherwise.
 */
static Eina_Bool
_evas_image_load_jpeg_cancelled(void *data EINA_UNUSED,
                                Emile_Image *image EINA_UNUSED,
                                Emile_Action action EINA_UNUSED)
{
   return evas_module_task_cancelled();
}

/**
 * @internal
 * @brief Loads the actual pixel data of the JPEG image.
 *
 * This function is called by Evas to load the image pixel data into the
 * provided buffer. It uses the Emile library to perform the decoding.
 * It also sets up a cancellation callback.
 *
 * @param loader_data The handle returned by evas_image_load_file_open_jpeg().
 * @param prop Pointer to an Emile_Image_Property structure containing image
 *             properties (used by Emile, potentially updated).
 * @param pixels Pointer to the memory buffer where the decoded pixel data
 *               should be stored. The buffer must be pre-allocated by Evas
 *               based on the properties retrieved by evas_image_load_file_head_jpeg().
 * @param error Pointer to an integer where the error code will be stored
 *              in case of failure. See Evas_Load_Error codes.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
evas_image_load_file_data_jpeg(void *loader_data,
                              Emile_Image_Property *prop,
                              void *pixels,
                              int *error)
{
   Evas_Loader_Internal *loader = loader_data;
   Emile_Image_Load_Error image_error;
   Eina_Bool ret;

   emile_image_callback_set(loader->image,
                            _evas_image_load_jpeg_cancelled,
                            EMILE_ACTION_CANCELLED, NULL);
   ret = emile_image_data(loader->image,
                          prop, sizeof (*prop),
                          pixels,
                          &image_error);
   *error = image_error;
   return ret;
}

/**
 * @internal
 * @brief Structure defining the Evas image loader functions for JPEG.
 *
 * This structure maps the generic Evas image loader API functions
 * (open, close, head, data) to the specific implementations provided
 * in this file for the JPEG format.
 */
Evas_Image_Load_Func evas_image_load_jpeg_func =
{
  EVAS_IMAGE_LOAD_VERSION, /**< Loader API version. */
  evas_image_load_file_open_jpeg, /**< Function to open the image file. */
  evas_image_load_file_close_jpeg, /**< Function to close the image file. */
  (void*) evas_image_load_file_head_jpeg, /**< Function to read image header. */
  NULL, /**< Function to read animated image frame info (unused). */
  (void*) evas_image_load_file_data_jpeg, /**< Function to read image pixel data. */
  NULL, /**< Function to load image region (unused, handled via opts). */
  EINA_TRUE, /**< Indicates thread safety. */
  EINA_TRUE /**< Indicates asynchronous operation support (via cancellation check). */
};

/**
 * @internal
 * @brief Evas module initialization function.
 *
 * Called by Evas when the module is loaded. It registers the loader
 * functions defined in evas_image_load_jpeg_func.
 *
 * @param em The Evas_Module handle.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_image_load_jpeg_func);
   return 1;
}

/**
 * @internal
 * @brief Evas module shutdown function.
 *
 * Called by Evas when the module is unloaded. Currently does nothing.
 *
 * @param em The Evas_Module handle (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Evas module API structure.
 *
 * Defines the module's type, name, and entry points (open/close).
 */
static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION, /**< Evas API version. */
   "jpeg", /**< Module name. */
   "none", /**< Module license. */
   {
     module_open, /**< Module open function. */
     module_close /**< Module close function. */
   }
};

/**
 * @internal
 * @brief Macro to define the Evas module metadata.
 *
 * This macro registers the module with Evas, specifying its type
 * (image loader) and name (jpeg).
 */
EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, jpeg);

#ifndef EVAS_STATIC_BUILD_JPEG
/**
 * @internal
 * @brief Macro to define the Eina module entry point for dynamic loading.
 *
 * This macro is used when building Evas with dynamic module loading support.
 * It defines the necessary symbols for the dynamic loader to find and load
 * this JPEG image loader module.
 */
EVAS_EINA_MODULE_DEFINE(image_loader, jpeg);
#endif
