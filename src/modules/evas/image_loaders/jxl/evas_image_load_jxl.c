#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include <jxl/decode.h>
#include <jxl/resizable_parallel_runner.h>

#include <Ecore.h>
#include "Evas_Loader.h"
#include "evas_common_private.h"

typedef struct _Evas_Loader_Internal Evas_Loader_Internal;
struct _Evas_Loader_Internal
{
   Eina_File *f;
   Evas_Image_Load_Opts *opts;
   Evas_Image_Animated *animated;
   JxlParallelRunner *runner;
   JxlDecoder *decoder;
   double duration;
};

static int _evas_loader_jxl_log_dom = -1;

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_loader_jxl_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_evas_loader_jxl_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_loader_jxl_log_dom, __VA_ARGS__)

/**
 * @brief Converts an RGBA pixel buffer to BGRA in-place.
 *
 * This function iterates through the pixel data and swaps the R and B channels.
 * It processes two pixels at a time (as unsigned long long int) for efficiency.
 *
 * @param pixels Pointer to the pixel data.
 * @param size Total number of pixels (not bytes).
 */
void _rgba_to_bgra(void *pixels, int size /* in pixels */)
{
   unsigned long long int *iter = pixels;
   int i;

   for (i = 0; i < (size >> 1); i++, iter++)
     {
        *iter =
          /* we keep A and G */
          (*iter & 0xff00ff00ff00ff00) |
          /* we shift R */
          ((*iter & 0x000000ff000000ff) << 16) |
          /* we shift B */
          ((*iter & 0x00ff000000ff0000) >> 16);
     }
}

/**
 * @brief Reads the header of a JXL image file to get properties.
 *
 * This function decodes the JXL image header to extract information like
 * width, height, alpha presence, and animation details.
 *
 * @param loader Pointer to the internal loader structure.
 * @param prop Pointer to store the image properties.
 * @param map Memory-mapped content of the JXL file.
 * @param length Size of the memory-mapped file.
 * @param error Pointer to an integer to store error codes.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_head_jxl_internal(Evas_Loader_Internal *loader,
                                       Emile_Image_Property *prop,
                                       void *map, size_t length,
                                       int *error)
{
   Evas_Image_Animated *animated;
   JxlBasicInfo basic_info;
   JxlFrameHeader frame_header;
   JxlDecoder *decoder;
   JxlDecoderStatus s;
   JxlDecoderStatus st;
   uint32_t frame_count = 0;
   Eina_Bool ret;

   animated = loader->animated;

   ret = EINA_FALSE;
   prop->w = 0;
   prop->h = 0;
   prop->alpha = EINA_FALSE;

   decoder = JxlDecoderCreate(NULL);
   if (!decoder)
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return ret;
     }

   JxlDecoderSetKeepOrientation(decoder, JXL_TRUE);

   st = JxlDecoderSubscribeEvents(decoder,
                                  JXL_DEC_BASIC_INFO |
                                  JXL_DEC_FRAME);
   if (st != JXL_DEC_SUCCESS)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto destroy_decoder;
     }

   st = JxlDecoderSetInput(decoder, map, length);
   if (st != JXL_DEC_SUCCESS)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto destroy_decoder;
     }

   JxlDecoderCloseInput(decoder);

   /* First, JXL_DEC_BASIC_INFO event */
   st = JxlDecoderProcessInput(decoder);
   if (st != JXL_DEC_BASIC_INFO)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto release_input;
     }

   s = JxlDecoderGetBasicInfo(decoder, &basic_info);
   if (s != JXL_DEC_SUCCESS)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto release_input;
     }

   prop->w = basic_info.xsize;
   prop->h = basic_info.ysize;
   /* if size is invalid, we exit */
   if ((prop->w < 1) ||
       (prop->h < 1) ||
       (prop->w > IMG_MAX_SIZE) ||
       (prop->h > IMG_MAX_SIZE) ||
       IMG_TOO_BIG(prop->w, prop->h))
     {
        if (IMG_TOO_BIG(prop->w, prop->h))
          *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        else
          *error= EVAS_LOAD_ERROR_GENERIC;
        goto release_input;
     }

   prop->alpha = (basic_info.alpha_bits != 0);

   /* Then, JXL_DEC_FRAME event */

   if (basic_info.have_animation)
     {
        frame_count = 0;
     }
   for (;;)
     {
       st = JxlDecoderProcessInput(decoder);
       if (st == JXL_DEC_FRAME)
         {
           JxlDecoderGetFrameHeader(decoder, &frame_header);
           frame_count++;
           if (frame_header.is_last)
             break;
         }
     }

   /* Finally, JXL_DEC_SUCCESS event */
   st = JxlDecoderProcessInput(decoder);
   if (st != JXL_DEC_SUCCESS)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto release_input;
     }

   if (basic_info.have_animation)
     {
        animated->loop_hint = basic_info.animation.num_loops ? EVAS_IMAGE_ANIMATED_HINT_NONE : EVAS_IMAGE_ANIMATED_HINT_LOOP;
        animated->frame_count = frame_count;
        animated->loop_count = basic_info.animation.num_loops;
        animated->animated = EINA_TRUE;
        loader->duration = ((double)frame_header.duration * (double)basic_info.animation.tps_denominator) / (double)basic_info.animation.tps_numerator;
     }

   *error = EVAS_LOAD_ERROR_NONE;
   ret = EINA_TRUE;

 release_input:
   JxlDecoderReleaseInput(decoder);
 destroy_decoder:
   JxlDecoderDestroy(decoder);

   return ret;
}

/**
 * @brief Loads the image data from a JXL file.
 *
 * This function decodes the JXL image data into the provided pixel buffer.
 * It handles both static and animated JXL images. For animated images,
 * it loads the current frame specified in loader->animated->cur_frame.
 *
 * @param loader Pointer to the internal loader structure.
 * @param prop Pointer to the image properties.
 * @param pixels Buffer to store the decoded pixel data (BGRA format).
 * @param map Memory-mapped content of the JXL file.
 * @param length Size of the memory-mapped file.
 * @param error Pointer to an integer to store error codes.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_data_jxl_internal(Evas_Loader_Internal *loader,
                                       Emile_Image_Property *prop,
                                       void *pixels,
                                       void *map, size_t length,
                                       int *error)
{
   Evas_Image_Animated *animated;
   JxlParallelRunner *runner;
   JxlDecoder *decoder;
   JxlPixelFormat pixel_format;
   JxlDecoderStatus st;
   size_t buffer_size;
   Eina_Bool ret = EINA_FALSE;

   animated = loader->animated;

   runner = loader->runner;
   decoder = loader->decoder;
   if (!runner || !decoder)
     {
        runner = JxlResizableParallelRunnerCreate(NULL);
        if (!runner)
          {
             *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
             goto on_error;
          }

        decoder = JxlDecoderCreate(NULL);
        if (!decoder)
          {
             *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
             goto on_error;
          }

        st = JxlDecoderSetParallelRunner(decoder,
                                         JxlResizableParallelRunner,
                                         runner);
        if (st != JXL_DEC_SUCCESS)
          {
             *error = EVAS_LOAD_ERROR_GENERIC;
             goto on_error;
          }

        JxlResizableParallelRunnerSetThreads(runner,
                                             JxlResizableParallelRunnerSuggestThreads(prop->w, prop->h));

        JxlDecoderSetKeepOrientation(decoder, JXL_TRUE);

        st = JxlDecoderSetInput(decoder, map, length);
        if (st != JXL_DEC_SUCCESS)
          {
             *error = EVAS_LOAD_ERROR_GENERIC;
             goto on_error;
          }

        JxlDecoderCloseInput(decoder);

        st = JxlDecoderSubscribeEvents(decoder,
                                       JXL_DEC_FULL_IMAGE);
        if (st != JXL_DEC_SUCCESS)
          {
             *error = EVAS_LOAD_ERROR_GENERIC;
             goto on_error;
          }
     }

   pixel_format.num_channels = 4;
   pixel_format.data_type = JXL_TYPE_UINT8;
#ifdef WORDS_BIGENDIAN
   pixel_format.endianness = JXL_BIG_ENDIAN;
#else
   pixel_format.endianness = JXL_LITTLE_ENDIAN;
#endif
   pixel_format.align = 0;

   if (animated->animated)
     {
       /*
        * According to the libjxl devsn there is a better way than
        * JxlDecoderSkipFrames(), but i can't...
        */
        JxlDecoderSkipFrames(decoder, animated->cur_frame);
     }

   st = JxlDecoderProcessInput(decoder);
   if (animated->animated)
     {
        if (st == JXL_DEC_SUCCESS)
          goto on_success;
     }

   if (st != JXL_DEC_NEED_IMAGE_OUT_BUFFER)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto on_error;
     }

   st = JxlDecoderImageOutBufferSize(decoder,
                                     &pixel_format,
                                     &buffer_size);
   if (st != JXL_DEC_SUCCESS)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto on_error;
     }

   if (buffer_size != (size_t)(prop->w * prop->h * 4))
     {
        ERR("buffer size does not match image size");
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto on_error;
     }

   st = JxlDecoderSetImageOutBuffer(decoder,
                                    &pixel_format,
                                    pixels,
                                    buffer_size);
   if (st != JXL_DEC_SUCCESS)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto on_error;
     }

   st = JxlDecoderProcessInput(decoder);
   if (st != JXL_DEC_FULL_IMAGE)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto on_error;
     }

   _rgba_to_bgra(pixels, prop->w * prop->h);

 on_success:
   *error = EVAS_LOAD_ERROR_NONE;
   ret = EINA_TRUE;

 on_error:
   return ret;
}

/**
 * @brief Opens a JXL image file for loading.
 *
 * This function is called by Evas to initialize the loading process for a JXL image.
 * It allocates and initializes an Evas_Loader_Internal structure.
 *
 * @param f Eina_File handle for the image file.
 * @param key Optional key associated with the image file (unused).
 * @param opts Load options for the image.
 * @param animated Pointer to store animated image properties.
 * @param error Pointer to an integer to store error codes.
 * @return Pointer to an Evas_Loader_Internal structure on success, NULL on failure.
 */
static void *
evas_image_load_file_open_jxl(Eina_File *f, Eina_Stringshare *key EINA_UNUSED,
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
 * @brief Closes a JXL image file and cleans up resources.
 *
 * This function is called by Evas when the image loading process is finished
 * or aborted. It frees resources allocated by evas_image_load_file_open_jxl.
 *
 * @param loader_data Pointer to the Evas_Loader_Internal structure.
 */
static void
evas_image_load_file_close_jxl(void *loader_data)
{
   Evas_Loader_Internal *loader;

   loader = loader_data;
   if (loader->decoder)
     {
        JxlDecoderReleaseInput(loader->decoder);
        JxlDecoderDestroy(loader->decoder);
        /* if decoder is valid, runner is necessarly valid */
        JxlResizableParallelRunnerDestroy(loader->runner);
     }
   free(loader_data);
}

/**
 * @brief Reads the header of a JXL image file (Evas plugin interface).
 *
 * This function is part of the Evas image loader plugin interface.
 * It maps the file to memory and calls evas_image_load_file_head_jxl_internal
 * to do the actual header parsing.
 *
 * @param loader_data Pointer to the Evas_Loader_Internal structure.
 * @param prop Pointer to store the image properties.
 * @param error Pointer to an integer to store error codes.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_head_jxl(void *loader_data,
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

   val = evas_image_load_file_head_jxl_internal(loader,
                                                (Emile_Image_Property *)prop,
                                                map, eina_file_size_get(f),
                                                error);

   eina_file_map_free(f, map);

   return val;
}

/**
 * @brief Loads the image data from a JXL file (Evas plugin interface).
 *
 * This function is part of the Evas image loader plugin interface.
 * It maps the file to memory and calls evas_image_load_file_data_jxl_internal
 * to do the actual image data decoding.
 *
 * @param loader_data Pointer to the Evas_Loader_Internal structure.
 * @param prop Pointer to the image properties.
 * @param pixels Buffer to store the decoded pixel data.
 * @param error Pointer to an integer to store error codes.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_data_jxl(void *loader_data,
                              Evas_Image_Property *prop,
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

   val = evas_image_load_file_data_jxl_internal(loader,
                                                (Emile_Image_Property *)prop,
                                                pixels,
                                                map, eina_file_size_get(f),
                                                error);

   eina_file_map_free(f, map);

 on_error:
   return val;
}

/**
 * @brief Gets the duration of a specific frame or sequence of frames in an animated JXL.
 *
 * This function is part of the Evas image loader plugin interface.
 * It returns the duration for a given frame number. In the current JXL
 * implementation, it seems all frames have the same duration as reported
 * by the header.
 *
 * @param loader_data Pointer to the Evas_Loader_Internal structure.
 * @param start_frame The starting frame index (currently unused by this loader).
 * @param frame_num The frame number for which to get the duration. If < 1, it's treated as 1.
 * @return The duration of the frame in seconds, or -1.0 on error or if not animated.
 */
static double
evas_image_load_frame_duration_jxl(void *loader_data,
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

static Evas_Image_Load_Func evas_image_load_jxl_func =
{
   EVAS_IMAGE_LOAD_VERSION,
   evas_image_load_file_open_jxl,
   evas_image_load_file_close_jxl,
   evas_image_load_file_head_jxl,
   NULL,
   evas_image_load_file_data_jxl,
   evas_image_load_frame_duration_jxl,
   EINA_TRUE,
   EINA_FALSE
};

/**
 * @brief Opens the Evas JXL image loader module.
 *
 * This function is called when the Evas module is loaded.
 * It registers a log domain and sets up the module functions.
 *
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;

   _evas_loader_jxl_log_dom = eina_log_domain_register("evas-jxl", EINA_COLOR_BLUE);
   if (_evas_loader_jxl_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }

   em->functions = (void *)(&evas_image_load_jxl_func);

   return 1;
}

/**
 * @brief Closes the Evas JXL image loader module.
 *
 * This function is called when the Evas module is unloaded.
 * It unregisters the log domain.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_loader_jxl_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_loader_jxl_log_dom);
        _evas_loader_jxl_log_dom = -1;
     }
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "jxl",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, jxl);

#ifndef EVAS_STATIC_BUILD_JXL
EVAS_EINA_MODULE_DEFINE(image_loader, jxl);
#endif
