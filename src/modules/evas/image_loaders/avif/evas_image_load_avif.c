#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include <avif/avif.h>

#include "Evas_Loader.h"
#include "evas_common_private.h"

typedef struct _Evas_Loader_Internal Evas_Loader_Internal;
struct _Evas_Loader_Internal
{
   Eina_File *f;
   Evas_Image_Load_Opts *opts;
   Evas_Image_Animated *animated;
   avifDecoder *decoder;
   double duration;
};

static int _evas_loader_avif_log_dom = -1; /**< Log domain for the AVIF loader module. */

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_loader_avif_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_evas_loader_avif_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_loader_avif_log_dom, __VA_ARGS__)

/**
 * @brief Internal function to read the header of an AVIF image file.
 * Parses the AVIF header from memory-mapped data to extract image properties
 * like dimensions, alpha channel presence, and animation details.
 *
 * @param loader The internal loader structure.
 * @param prop Pointer to the structure to store image properties.
 * @param map Pointer to the memory-mapped file data.
 * @param length Size of the memory-mapped data.
 * @param error Pointer to an integer to store the error code on failure.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
evas_image_load_file_head_avif_internal(Evas_Loader_Internal *loader,
                                        Emile_Image_Property *prop,
                                        void *map, size_t length,
                                        int *error)
{
   Evas_Image_Animated *animated;
   avifDecoder *decoder;
   const char *codec_name;
   avifResult res;
   Eina_Bool ret;

   animated = loader->animated;

   ret = EINA_FALSE;
   prop->w = 0;
   prop->h = 0;
   prop->alpha = EINA_FALSE;

   decoder = avifDecoderCreate();
   if (!decoder)
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return ret;
     }

   codec_name = avifCodecName(decoder->codecChoice, AVIF_CODEC_FLAG_CAN_DECODE);
   if (!codec_name)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto destroy_decoder;
     }

   INF("AV1 codec name (decode): %s", codec_name);

   res = avifDecoderSetIOMemory(decoder, (const uint8_t *)map, length);
   if (res != AVIF_RESULT_OK)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto destroy_decoder;
     }
   res = avifDecoderParse(decoder);
   if (res != AVIF_RESULT_OK)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto destroy_decoder;
     }

   if (decoder->imageCount < 1)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto destroy_decoder;
     }

   res = avifDecoderNextImage(decoder);
   if (res != AVIF_RESULT_OK)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto destroy_decoder;
     }

   prop->w = decoder->image->width;
   prop->h = decoder->image->height;

   /* if size is invalid, we exit */
   if ((prop->w < 1) || (prop->h < 1) ||
       (prop->w > IMG_MAX_SIZE) || (prop->h > IMG_MAX_SIZE) ||
       IMG_TOO_BIG(prop->w, prop->h))
     {
        if (IMG_TOO_BIG(prop->w, prop->h))
          *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        else
          *error= EVAS_LOAD_ERROR_GENERIC;
        goto destroy_decoder;
     }

   prop->alpha = !!decoder->image->alphaPlane;

   if (decoder->imageCount > 1)
     {
        animated->loop_hint = EVAS_IMAGE_ANIMATED_HINT_NONE;
        animated->frame_count = decoder->imageCount;
        animated->loop_count = 1;
        animated->animated = EINA_TRUE;
        loader->duration = decoder->duration / decoder->imageCount;
     }

   *error = EVAS_LOAD_ERROR_NONE;
   ret = EINA_TRUE;

 destroy_decoder:
   avifDecoderDestroy(decoder);

   return ret;
}

/**
 * @brief Internal function to load the image data of an AVIF file.
 * Decodes the AVIF image data from memory-mapped content into the provided pixel buffer.
 * Handles both static and animated images (decoding the current frame).
 *
 * @param loader The internal loader structure.
 * @param pixels Pointer to the destination buffer for decoded pixel data (ARGB or BGRA).
 * @param map Pointer to the memory-mapped file data.
 * @param length Size of the memory-mapped data.
 * @param error Pointer to an integer to store the error code on failure.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
evas_image_load_file_data_avif_internal(Evas_Loader_Internal *loader,
                                        void *pixels,
                                        void *map, size_t length,
                                        int *error)
{
   avifRGBImage rgb;
   avifDecoder *decoder;
   avifResult res;
   Evas_Image_Animated *animated;
   Eina_Bool ret;

   ret = EINA_FALSE;

   /* FIXME: create decoder in evas_image_load_file_data_avif instead ? */
   decoder = loader->decoder;
   if (!decoder)
     {
        const char *codec_name;

        decoder = avifDecoderCreate();
        if (!decoder)
          {
             *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
             return EINA_FALSE;
          }

        codec_name = avifCodecName(decoder->codecChoice,
                                   AVIF_CODEC_FLAG_CAN_DECODE);
        if (!codec_name)
          {
             *error = EVAS_LOAD_ERROR_GENERIC;
             goto on_error;
          }

        INF("AV1 codec name (decode): %s", codec_name);

        res = avifDecoderSetIOMemory(decoder, (const uint8_t *)map, length);
        if (res != AVIF_RESULT_OK)
          {
             *error = EVAS_LOAD_ERROR_GENERIC;
             goto on_error;
          }
        res = avifDecoderParse(decoder);
        if (res != AVIF_RESULT_OK)
          {
             *error = EVAS_LOAD_ERROR_GENERIC;
             goto on_error;
          }

        loader->decoder = decoder;
     }

   animated = loader->animated;
   if (animated->animated)
     {
        /* FIXME: next image instead ? */
        res = avifDecoderNthImage(decoder, animated->cur_frame + 1);
        if (res != AVIF_RESULT_OK)
          {
             *error = EVAS_LOAD_ERROR_GENERIC;
             goto on_error;
          }
     }
   else
     {
        res = avifDecoderNextImage(decoder);
        if (res != AVIF_RESULT_OK)
          {
             *error = EVAS_LOAD_ERROR_GENERIC;
             goto on_error;
          }
     }

   avifRGBImageSetDefaults(&rgb, decoder->image);
#ifdef WORDS_BIGENDIAN
   rgb.format = AVIF_RGB_FORMAT_ARGB;
#else
   rgb.format = AVIF_RGB_FORMAT_BGRA;
#endif
   rgb.depth = 8;
   rgb.pixels = pixels;
   rgb.rowBytes = 4 * decoder->image->width;

   res = avifImageYUVToRGB(decoder->image, &rgb);
   if (res != AVIF_RESULT_OK)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto on_error;
     }

   *error = EVAS_LOAD_ERROR_NONE;

   ret = EINA_TRUE;

 on_error:

   return ret;
}

/**
 * @brief Opens an AVIF image file for loading. Evas loader API function.
 * Allocates and initializes the internal loader structure.
 *
 * @param f Eina file handle.
 * @param key Optional key associated with the image. (Unused)
 * @param opts Load options.
 * @param animated Pointer to store animation details if applicable.
 * @param error Pointer to store error code on failure.
 * @return A handle (loader_data) to the internal loader structure on success, NULL otherwise.
 */
static void *
evas_image_load_file_open_avif(Eina_File *f, Eina_Stringshare *key EINA_UNUSED,
			       Evas_Image_Load_Opts *opts,
			       Evas_Image_Animated *animated,
			       int *error)
{
   Evas_Loader_Internal *loader;

   loader = calloc(1, sizeof (Evas_Loader_Internal));
   if (!loader)
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return NULL;
     }

   loader->f = f;
   loader->opts = opts;
   loader->animated = animated;

   return loader;
}

/**
 * @brief Closes an AVIF image file previously opened by evas_image_load_file_open_avif.
 * Evas loader API function.
 * Frees resources associated with the loader, including the decoder instance.
 *
 * @param loader_data The handle returned by evas_image_load_file_open_avif.
 */
static void
evas_image_load_file_close_avif(void *loader_data)
{
   Evas_Loader_Internal *loader;

   loader = loader_data;
   /*
    * in case _head() fails (because the file is not an avif one),
    * loader is not filled and loader->decoder is NULL
    */
   if (loader->decoder)
     avifDecoderDestroy(loader->decoder);
   free(loader_data);
}

/**
 * @brief Reads the header information of an opened AVIF file. Evas loader API function.
 * Maps the file to memory and calls the internal header reading function.
 *
 * @param loader_data The handle returned by evas_image_load_file_open_avif.
 * @param prop Pointer to the structure to store image properties.
 * @param error Pointer to store error code on failure.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
evas_image_load_file_head_avif(void *loader_data,
                               Evas_Image_Property *prop,
                               int *error)
{
   Evas_Loader_Internal *loader = loader_data;
   Eina_File *f;
   void *map;
   Eina_Bool val;

   f = loader->f;

   map = eina_file_map_all(f, EINA_FILE_RANDOM);
   if (!map)
     {
	*error = EVAS_LOAD_ERROR_DOES_NOT_EXIST;
        return EINA_FALSE;
     }

   val = evas_image_load_file_head_avif_internal(loader,
                                                 (Emile_Image_Property *)prop,
                                                 map, eina_file_size_get(f),
                                                 error);

   eina_file_map_free(f, map);

   return val;
}

/**
 * @brief Loads the image data from an opened AVIF file into a pixel buffer.
 * Evas loader API function.
 * Maps the file to memory and calls the internal data loading function.
 *
 * @param loader_data The handle returned by evas_image_load_file_open_avif.
 * @param prop Image properties (unused in this function).
 * @param pixels Pointer to the destination buffer for decoded pixel data.
 * @param error Pointer to store error code on failure.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
evas_image_load_file_data_avif(void *loader_data,
                               Evas_Image_Property *prop EINA_UNUSED,
			       void *pixels,
			       int *error)
{
   Evas_Loader_Internal *loader;
   Eina_File *f;
   void *map;
   Eina_Bool val = EINA_FALSE;

   loader = (Evas_Loader_Internal *)loader_data;
   f = loader->f;

   map = eina_file_map_all(f, EINA_FILE_WILLNEED);
   if (!map)
     {
        *error = EVAS_LOAD_ERROR_DOES_NOT_EXIST;
        goto on_error;
     }

   val = evas_image_load_file_data_avif_internal(loader,
                                                 pixels,
                                                 map, eina_file_size_get(f),
                                                 error);

   eina_file_map_free(f, map);

 on_error:
   return val;
}

/**
 * @brief Gets the duration for a specific frame or sequence of frames in an animated AVIF.
 * Evas loader API function.
 * Currently returns a fixed duration per frame calculated during header loading.
 *
 * @param loader_data The handle returned by evas_image_load_file_open_avif.
 * @param start_frame The starting frame index (currently ignored, assumes uniform duration).
 * @param frame_num The frame number relative to start_frame (used for validation).
 * @return The duration of the frame in seconds, or -1.0 on error or if not animated.
 */
static double
evas_image_load_frame_duration_avif(void *loader_data,
                                    int start_frame,
                                    int frame_num)
{
   Evas_Loader_Internal *loader;
   Evas_Image_Animated *animated;

   loader = (Evas_Loader_Internal *)loader_data;
   animated = loader->animated;

   if (!animated->animated)
     return -1.0;

   if (frame_num < 0)
     return -1.0;

   if ((start_frame + frame_num) > animated->frame_count)
     return -1.0;

   if (frame_num < 1)
     frame_num = 1;

   return loader->duration;
}

/**
 * @brief Structure defining the Evas image loader functions for AVIF.
 */
static Evas_Image_Load_Func evas_image_load_avif_func =
{
   EVAS_IMAGE_LOAD_VERSION,
   evas_image_load_file_open_avif,
   evas_image_load_file_close_avif,
   evas_image_load_file_head_avif,
   NULL,
   evas_image_load_file_data_avif,
   evas_image_load_frame_duration_avif,
   EINA_TRUE,
   EINA_FALSE /**< Does not provide slice loading capabilities. */
};

/**
 * @brief Initializes the AVIF image loader module. Evas module API function.
 * Registers the log domain and sets the loader functions.
 *
 * @param em The Evas module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;

   _evas_loader_avif_log_dom = eina_log_domain_register("evas-avif", EINA_COLOR_BLUE);
   if (_evas_loader_avif_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }

   em->functions = (void *)(&evas_image_load_avif_func);

   return 1;
}

/**
 * @brief Shuts down the AVIF image loader module. Evas module API function.
 * Unregisters the log domain.
 *
 * @param em The Evas module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_loader_avif_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_loader_avif_log_dom);
        _evas_loader_avif_log_dom = -1;
     }
}

/**
 * @brief Module API structure definition for the AVIF loader.
 */
static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "avif",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, avif);

#ifndef EVAS_STATIC_BUILD_AVIF
EVAS_EINA_MODULE_DEFINE(image_loader, avif);
#endif

