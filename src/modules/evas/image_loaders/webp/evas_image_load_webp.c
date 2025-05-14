#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <webp/decode.h>
#include <webp/demux.h>

#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @brief Structure to hold loader-specific information during WebP image loading.
 *
 * This structure maintains the state and data required throughout the loading
 * process of a WebP image, including file handles, animation data, and decoded frames.
 */
typedef struct _Loader_Info
{
   Eina_File *f; /**< Pointer to the Eina_File object for the image file. */
   Evas_Image_Load_Opts *opts; /**< Pointer to the image loading options. */
   Evas_Image_Animated *animated; /**< Pointer to the animated image properties. */
   WebPAnimDecoder *dec; /**< Pointer to the WebP animation decoder. */
   void *map; /**< Memory-mapped region of the image file. */
   Eina_Array *frames; /**< Array of decoded animation frames (_Image_Frame). Example: [frame1, frame2, ...] */
}Loader_Info;

// WebP Frame Information
/**
 * @brief Structure to store information about a single WebP animation frame.
 *
 * This includes the frame's index, timestamp, display delay, and pixel data.
 */
typedef struct _Image_Frame
{
   int index; /**< 1-based index of the frame in the animation sequence. */
   int timestamp; /**< Timestamp of the frame in milliseconds. */
   double delay; /**< Delay in seconds before displaying the next frame. */
   uint8_t *data; /**< Raw pixel data for the frame (BGRA format). */
}Image_Frame;

/**
 * @brief Checks if the given file is a valid WebP image and retrieves its properties.
 *
 * This function reads the header of the WebP file to determine its dimensions,
 * and whether it has an alpha channel.
 *
 * @param f Pointer to the Eina_File object.
 * @param map Pointer to the memory-mapped file data.
 * @param w Pointer to store the width of the image.
 * @param h Pointer to store the height of the image.
 * @param alpha Pointer to store whether the image has an alpha channel.
 * @param error Pointer to an integer to store the error code if any.
 * @return EINA_TRUE if the file is a valid WebP image, EINA_FALSE otherwise.
 */
static Eina_Bool
evas_image_load_file_check(Eina_File *f, void *map,
			   unsigned int *w, unsigned int *h, Eina_Bool *alpha,
			   int *error)
{
   WebPDecoderConfig config;

   if (eina_file_size_get(f) < 30) return EINA_FALSE;

   if (!WebPInitDecoderConfig(&config))
   {
      *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
      return EINA_FALSE;
   }
   if (WebPGetFeatures(map, 30, &config.input) != VP8_STATUS_OK)
   {
      *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
      return EINA_FALSE;
   }

   *w = config.input.width;
   *h = config.input.height;
   *alpha = config.input.has_alpha;

   return EINA_TRUE;
}

/**
 * @brief Opens a WebP image file and initializes the loader.
 *
 * This function allocates and initializes a Loader_Info structure for
 * managing the WebP image loading process.
 *
 * @param f Pointer to the Eina_File object.
 * @param key The key associated with the image file (unused).
 * @param opts Pointer to the image loading options.
 * @param animated Pointer to the animated image properties.
 * @param error Pointer to an integer to store the error code if any.
 * @return A pointer to the initialized Loader_Info structure, or NULL on failure.
 */
static void *
evas_image_load_file_open_webp(Eina_File *f, Eina_Stringshare *key EINA_UNUSED,
			       Evas_Image_Load_Opts *opts,
			       Evas_Image_Animated *animated,
			       int *error)
{
   Loader_Info *loader = calloc(1, sizeof (Loader_Info));
   if (!loader)
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return NULL;
     }
   loader->f = eina_file_dup(f);
   loader->opts = opts;
   loader->animated = animated;
   return loader;
}

/**
 * @brief Frees all allocated frame data within the loader.
 *
 * Iterates through the `frames` array in the Loader_Info structure and
 * deallocates the pixel data and the Image_Frame structure itself for each frame.
 *
 * @param loader Pointer to the Loader_Info structure.
 */
static void
_free_all_frame(Loader_Info *loader)
{
   Image_Frame *frame;

   if (!loader->frames) return;

   for (unsigned int i = 0; i < eina_array_count(loader->frames); ++i)
     {
        frame = eina_array_data_get(loader->frames, i);
        if (frame->data)
          {
             free(frame->data);
             frame->data = NULL;
          }
        free(frame);
     }
}


/**
 * @brief Closes the WebP image file and frees associated resources.
 *
 * This function deallocates all resources used by the WebP loader,
 * including frame data, the animation decoder, and file handles.
 *
 * @param loader_data Pointer to the Loader_Info structure.
 */
static void
evas_image_load_file_close_webp(void *loader_data)
{
   // Free Allocated Data
   Loader_Info *loader = loader_data;
   _free_all_frame(loader);
   eina_array_free(loader->frames);
   if (loader->dec) WebPAnimDecoderDelete(loader->dec);
   if ((loader->map) && (loader->f))
     eina_file_map_free(loader->f, loader->map);
   if (loader->f) eina_file_close(loader->f);
   free(loader);
}


/**
 * @brief Creates a new Image_Frame and adds it to the loader's frame array.
 *
 * This function allocates memory for a new frame, copies the pixel data,
 * calculates the frame delay, and appends it to the `frames` array in Loader_Info.
 *
 * @param loader Pointer to the Loader_Info structure.
 * @param data Pointer to the raw pixel data (BGRA) for the frame.
 * @param width Width of the frame.
 * @param height Height of the frame.
 * @param index 1-based index of the frame.
 * @param pre_timestamp Timestamp of the previous frame in milliseconds.
 * @param cur_timestamp Timestamp of the current frame in milliseconds.
 */
static void
_new_frame(Loader_Info *loader, uint8_t *data, int width, int height, int index,
           int pre_timestamp, int cur_timestamp)
{
   // Allocate Frame Data
   Image_Frame *frame;

   frame = calloc(1, sizeof(Image_Frame));
   if (!frame) return;

   frame->data = calloc(width * height * 4, sizeof(uint8_t));
   if (!frame->data)
     {
        free(frame);
        return;
     }

   frame->index = index;
   frame->timestamp = cur_timestamp;
   frame->delay = ((double)(cur_timestamp - pre_timestamp)/1000.0);
   memcpy(frame->data, data, width * height * 4);

   eina_array_push(loader->frames, frame);
}

/**
 * @brief Finds a specific frame by its index in the loader's frame array.
 *
 * @param loader Pointer to the Loader_Info structure.
 * @param index 1-based index of the frame to find.
 * @return Pointer to the Image_Frame if found, NULL otherwise.
 *         Example: If `loader->frames` contains [frame_at_idx_0, frame_at_idx_1, ...],
 *         and `index` is 1, this returns `frame_at_idx_0`.
 *         If `index` is 2, this returns `frame_at_idx_1`.
 */
static Image_Frame *
_find_frame(Loader_Info *loader, int index)
{
   // Find Frame
   Image_Frame *frame;

   if (!loader->frames) return NULL;

   frame = eina_array_data_get(loader->frames, index - 1);
   if (frame->index == index)
     return frame;

   return NULL;
}

/**
 * @brief Loads the header information of a WebP image file.
 *
 * This function maps the file into memory, checks its validity,
 * initializes the WebP animation decoder, decodes all frames,
 * and populates the image properties and animation information.
 *
 * @param loader_data Pointer to the Loader_Info structure.
 * @param prop Pointer to an Emile_Image_Property structure to store image properties.
 * @param error Pointer to an integer to store the error code if any.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_head_webp(void *loader_data,
			       Emile_Image_Property *prop,
			       int *error)
{
   Loader_Info *loader = loader_data;
   Evas_Image_Animated *animated = loader->animated;
   Eina_File *f = loader->f;
   void *data;

   *error = EVAS_LOAD_ERROR_NONE;

   data = eina_file_map_all(f, EINA_FILE_RANDOM);
   loader->map = data;

   if (!evas_image_load_file_check(f, data,
				  &prop->w, &prop->h, &prop->alpha,
				  error))
     {
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        return EINA_FALSE;
     }

   // Init WebP Data
   WebPData webp_data;
   WebPDataInit(&webp_data);

   // Assign Data
   webp_data.bytes = data;
   webp_data.size = eina_file_size_get(f);

   // Set Decode Option
   WebPAnimDecoderOptions dec_options;
   WebPAnimDecoderOptionsInit(&dec_options);
   dec_options.color_mode = MODE_BGRA;

   // Create WebPAnimation Decoder
   WebPAnimDecoder *dec = WebPAnimDecoderNew(&webp_data, &dec_options);
   if (!dec)
     {
        ERR("WebP Decoder Creation failed");
        *error = EVAS_LOAD_ERROR_GENERIC;
        return EINA_FALSE;
     }
   loader->dec = dec;

   // Get WebP Animation Info
   WebPAnimInfo anim_info;
   if (!WebPAnimDecoderGetInfo(dec, &anim_info))
     {
        ERR("Getting WebP Information failed");
        *error = EVAS_LOAD_ERROR_GENERIC;
        return EINA_FALSE;
     }

   uint8_t* buf;
   int pre_timestamp = 0;
   int cur_timestamp = 0;
   int index = 1;

   // Set Frame Array
   loader->frames = eina_array_new(anim_info.frame_count);
   if (!loader->frames)
     {
        ERR("Frame Array Allocation failed");
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return EINA_FALSE;
     }

   // Decode Frames
   while (WebPAnimDecoderHasMoreFrames(dec))
     {
        if (!WebPAnimDecoderGetNext(dec, &buf, &cur_timestamp))
          {
             ERR("WebP Decoded Frame Get failed");
             *error = EVAS_LOAD_ERROR_GENERIC;
             return EINA_FALSE;
          }
        _new_frame(loader, buf, anim_info.canvas_width, anim_info.canvas_height, index,
                   pre_timestamp, cur_timestamp);
        pre_timestamp = cur_timestamp;
        index++;
     }

   // Set Animation Info
   if (anim_info.frame_count > 1)
     {
        animated->animated = 1;
        animated->loop_count = anim_info.loop_count;
        animated->loop_hint = EVAS_IMAGE_ANIMATED_HINT_LOOP;
        animated->frame_count = anim_info.frame_count;
     }

   return EINA_TRUE;
}

/**
 * @brief Loads the pixel data for the current frame of a WebP image.
 *
 * This function retrieves the pre-decoded pixel data for the requested frame
 * (specified by `animated->cur_frame`) and copies it into the provided pixel buffer.
 *
 * @param loader_data Pointer to the Loader_Info structure.
 * @param prop Pointer to an Emile_Image_Property structure (used to set premul).
 * @param pixels Pointer to the buffer where the pixel data will be copied.
 * @param error Pointer to an integer to store the error code if any.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_data_webp(void *loader_data,
			       Emile_Image_Property *prop,
			       void *pixels,
			       int *error)
{
   Loader_Info *loader = loader_data;
   Evas_Image_Animated *animated = loader->animated;

   *error = EVAS_LOAD_ERROR_NONE;

   void *surface = NULL;
   int width, height;
   int index = 0;

   index = animated->cur_frame;

   // Find Cur Frame
   if (index == 0)
     index = 1;
   Image_Frame *frame = _find_frame(loader, index);
   if (frame == NULL) return EINA_FALSE;

   WebPAnimInfo anim_info;
   WebPAnimDecoderGetInfo(loader->dec, &anim_info);
   width = anim_info.canvas_width;
   height = anim_info.canvas_height;

   // Render Frame
   surface = pixels;
   memcpy(surface, frame->data, width * height * 4);
   prop->premul = EINA_TRUE;

   return EINA_TRUE;
}

/**
 * @brief Gets the duration of a specific frame in an animated WebP image.
 *
 * @param loader_data Pointer to the Loader_Info structure.
 * @param start_frame The 1-based index of the frame for which to get the duration.
 * @param frame_num The number of the frame (unused in this implementation, seems redundant with start_frame).
 * @return The duration of the frame in seconds, or -1.0 on error or if not animated.
 *         Example: If frame `start_frame` has a delay of 50ms, returns 0.05.
 */
static double
evas_image_load_frame_duration_webp(void *loader_data,
                                    int start_frame,
                                    int frame_num)
{
   Loader_Info *loader = loader_data;
   Evas_Image_Animated *animated = loader->animated;

   if (!animated->animated) return -1.0;
   if (frame_num < 0) return -1.0;
   if (start_frame < 1) return -1.0;

   // Calculate Duration of Current Frame
   Image_Frame *frame = _find_frame(loader, start_frame);
   if (frame == NULL) return -1.0;

   return frame->delay;
}

/**
 * @brief Structure defining the Evas image loader functions for WebP.
 *
 * This structure maps the generic Evas image loading API calls to the
 * WebP-specific implementation functions.
 */
static Evas_Image_Load_Func evas_image_load_webp_func =
{
  EVAS_IMAGE_LOAD_VERSION, /**< Loader API version. */
  evas_image_load_file_open_webp, /**< Function to open a WebP file. */
  evas_image_load_file_close_webp, /**< Function to close a WebP file. */
  (void*) evas_image_load_file_head_webp, /**< Function to load WebP header. */
  NULL, /**< Function to load WebP header and data (not used when head/data separate). */
  (void*) evas_image_load_file_data_webp, /**< Function to load WebP pixel data. */
  evas_image_load_frame_duration_webp, /**< Function to get frame duration. */
  EINA_TRUE, /**< Indicates that the loader supports loading from Eina_File. */
  EINA_FALSE /**< Indicates that the loader does not support progressive loading. */
};

/**
 * @brief Evas module initialization function.
 *
 * Called when the Evas module is loaded. It registers the WebP image
 * loader functions with Evas.
 *
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_image_load_webp_func);
   return 1;
}

/**
 * @brief Evas module shutdown function.
 *
 * Called when the Evas module is unloaded. Currently does nothing.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
}

/**
 * @brief Evas module API structure.
 *
 * Defines the API for this Evas image loader module.
 */
static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION, /**< Evas module API version. */
   "webp",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, webp);

#ifndef EVAS_STATIC_BUILD_WEBP
EVAS_EINA_MODULE_DEFINE(image_loader, webp);
#endif
