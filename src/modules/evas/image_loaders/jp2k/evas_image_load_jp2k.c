#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include <openjpeg.h>

#include "Evas_Loader.h"

static int _evas_loader_jp2k_log_dom = -1;

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_loader_jp2k_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_evas_loader_jp2k_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_loader_jp2k_log_dom, __VA_ARGS__)

#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"
#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"

/**
 * @brief Structure to manage reading from a memory-mapped region.
 * Used as user data for OpenJPEG stream callbacks.
 */
typedef struct
{
   unsigned char *base; /**< Pointer to the start of the mapped memory region. */
   size_t length; /**< Total size of the mapped memory region. */
   size_t idx;    /**< Current read position within the mapped region. */
} Map_St;

/**
 * @brief Internal data structure for the Evas JP2K loader instance.
 */
typedef struct _Evas_Loader_Internal Evas_Loader_Internal;
struct _Evas_Loader_Internal
{
   Eina_File *f; /**< Eina file handle for the image file. */
   Evas_Image_Load_Opts *opts; /**< Image loading options (e.g., region, scale). */
};

/**
 * @brief OpenJPEG callback function to suppress log messages.
 * This function is registered with OpenJPEG to prevent it from printing
 * informational, warning, or error messages to the console.
 * @param msg The message string from OpenJPEG (unused).
 * @param client_data User data pointer (unused).
 */
static void
_jp2k_quiet_callback(const char *msg, void *client_data)
{
   (void)msg;
   (void)client_data;
}

/**
 * @brief OpenJPEG stream read callback function.
 * Reads data from the memory-mapped region managed by Map_St.
 * @param buf Buffer to read data into.
 * @param size Number of bytes to read.
 * @param data User data pointer (expected to be a Map_St*).
 * @return The number of bytes actually read, or (OPJ_SIZE_T)-1 on error (EOF).
 */
static OPJ_SIZE_T
_jp2k_read_fn(void *buf, OPJ_SIZE_T size, void *data)
{
   Map_St *map = data;
   OPJ_SIZE_T offset;

   offset = map->length - map->idx;
   if (offset == 0)
     return (OPJ_SIZE_T)-1;
   if (offset > size)
     offset = size;
   memcpy(buf, map->base + map->idx, offset);
   map->idx += offset;

   return offset;
}

/**
 * @brief OpenJPEG stream skip (seek relative) callback function.
 * Advances the read position within the memory-mapped region.
 * @param size Number of bytes to skip (can be negative).
 * @param data User data pointer (expected to be a Map_St*).
 * @return The new offset from the beginning of the stream after skipping.
 */
static OPJ_OFF_T
_jp2k_seek_cur_fn(OPJ_OFF_T size, void *data)
{
   Map_St *map = data;

   if (size > (OPJ_OFF_T)(map->length - map->idx))
     size = (OPJ_OFF_T)(map->length - map->idx);

   map->idx += size;

   return map->idx;
}

/**
 * @brief OpenJPEG stream seek (absolute) callback function.
 * Sets the read position within the memory-mapped region.
 * @param size The absolute offset to seek to from the beginning of the stream.
 * @param data User data pointer (expected to be a Map_St*).
 * @return OPJ_TRUE on success, OPJ_FALSE on failure (e.g., seeking past EOF).
 */
static OPJ_BOOL
_jp2k_seek_set_fn(OPJ_OFF_T size, void *data)
{
   Map_St *map = data;

   if (size > (OPJ_OFF_T)map->length)
     return OPJ_FALSE;

   map->idx = size;

   return OPJ_TRUE;
}

/**
 * @brief Reads the header of a JP2K image from a memory buffer.
 * This function uses OpenJPEG to parse the header and extract image
 * dimensions and alpha channel presence.
 * @param[out] w Pointer to store the image width.
 * @param[out] h Pointer to store the image height.
 * @param[out] alpha Pointer to store alpha channel presence (1 if present, 0 otherwise).
 * @param map Pointer to the memory buffer containing the JP2K file data.
 * @param length Size of the memory buffer.
 * @param[out] error Pointer to store the Evas load error code.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_head_jp2k_internal(unsigned int *w, unsigned int *h,
					unsigned char *alpha,
                                        void *map, size_t length,
                                        int *error)
{
   Map_St map_st;
   opj_dparameters_t core;
   opj_codec_t *codec;
   opj_stream_t *st;
   opj_image_t* image;
   OPJ_CODEC_FORMAT cfmt;

   map_st.base = map;
   map_st.length = length;
   map_st.idx = 0;

   /* default parameters */
   memset(&core, 0, sizeof(opj_dparameters_t));
   opj_set_default_decoder_parameters(&core);

   /* magic check */
   cfmt = OPJ_CODEC_UNKNOWN;
   if (map_st.length >= 4)
     {
        if (memcmp(map_st.base, J2K_CODESTREAM_MAGIC, 4) == 0)
          cfmt = OPJ_CODEC_J2K;
        else if ((memcmp(map_st.base, JP2_MAGIC, 4) == 0) ||
                 ((map_st.length >= 12) && (memcmp(map_st.base, JP2_RFC3745_MAGIC, 12) == 0)))
          cfmt = OPJ_CODEC_JP2;
     }

   if (cfmt == OPJ_CODEC_UNKNOWN)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        return EINA_FALSE;
     }

   /* codec */
   codec = opj_create_decompress(cfmt);
   if (!codec)
     {
        ERR("can't create codec");
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return EINA_FALSE;
     }
   opj_set_info_handler(codec, _jp2k_quiet_callback, NULL);
   opj_set_warning_handler(codec, _jp2k_quiet_callback, NULL);
   opj_set_error_handler(codec, _jp2k_quiet_callback, NULL);
   if (!opj_setup_decoder(codec, &core))
     {
        ERR("can't setup decoder");
        opj_destroy_codec(codec);
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return EINA_FALSE;
     }
   //opj_codec_set_threads(codec, 0)

   /* stream */
   st = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_TRUE);
   if (!st)
     {
        ERR("can't create stream");
        opj_destroy_codec(codec);
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return EINA_FALSE;
     }

   opj_stream_set_user_data(st, &map_st, NULL);
   opj_stream_set_user_data_length(st, map_st.length);
   opj_stream_set_read_function(st, _jp2k_read_fn);
   opj_stream_set_skip_function(st, _jp2k_seek_cur_fn);
   opj_stream_set_seek_function(st, _jp2k_seek_set_fn);

   opj_read_header(st, codec, &image);
   *w = image->x1 - image->x0;
   *h = image->y1 - image->y0;
   *alpha = ((image->numcomps == 4) || (image->numcomps == 2)) ? 1 : 0;
   *error = EVAS_LOAD_ERROR_NONE;

   opj_image_destroy(image);
   opj_stream_destroy(st);
   opj_destroy_codec(codec);

   return EINA_TRUE;
}

/**
 * @brief Decodes the image data of a JP2K image from a memory buffer.
 * This function uses OpenJPEG to decode the image data and store it
 * in the provided pixel buffer in ARGB32 format.
 * @param pixels Pointer to the destination buffer for decoded pixel data (ARGB32).
 *               The buffer must be pre-allocated with size w * h * 4 bytes.
 *               Pixel format: 0xAARRGGBB (Alpha, Red, Green, Blue).
 * @param map Pointer to the memory buffer containing the JP2K file data.
 * @param length Size of the memory buffer.
 * @param[out] error Pointer to store the Evas load error code.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_data_jp2k_internal(void *pixels,
                                        void *map, size_t length,
                                        int *error)
{
   Map_St map_st;
   opj_dparameters_t core;
   opj_codec_t *codec;
   opj_stream_t *st;
   opj_image_t* image;
   unsigned int *iter;
   OPJ_CODEC_FORMAT cfmt;
   int idx;

   map_st.base = map;
   map_st.length = length;
   map_st.idx = 0;

   /* default parameters */
   memset(&core, 0, sizeof(opj_dparameters_t));
   opj_set_default_decoder_parameters(&core);
   core.flags |= OPJ_DPARAMETERS_IGNORE_PCLR_CMAP_CDEF_FLAG;

   /* magic check */
   cfmt = OPJ_CODEC_UNKNOWN;
   if (map_st.length >= 4)
     {
        if (memcmp(map_st.base, J2K_CODESTREAM_MAGIC, 4) == 0)
          cfmt = OPJ_CODEC_J2K;
        else if ((memcmp(map_st.base, JP2_MAGIC, 4) == 0) ||
                 ((map_st.length >= 12) && (memcmp(map_st.base, JP2_RFC3745_MAGIC, 12) == 0)))
          cfmt = OPJ_CODEC_JP2;
     }

   if (cfmt == OPJ_CODEC_UNKNOWN)
     {
        ERR("jpeg200 file format invalid\n");
        *error = EVAS_LOAD_ERROR_GENERIC;
        return EINA_FALSE;
     }

   /* codec */
   codec = opj_create_decompress(cfmt);
   if (!codec)
     {
        ERR("can't create codec\n");
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return EINA_FALSE;
     }
   opj_set_info_handler(codec, _jp2k_quiet_callback, NULL);
   opj_set_warning_handler(codec, _jp2k_quiet_callback, NULL);
   opj_set_error_handler(codec, _jp2k_quiet_callback, NULL);
   if (!opj_setup_decoder(codec, &core))
     {
        ERR("can't setup decoder\n");
        opj_destroy_codec(codec);
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return EINA_FALSE;
     }
   //opj_codec_set_threads(codec, 0)

   /* stream */
   st = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_TRUE);
   if (!st)
     {
        ERR("can't create stream\n");
        opj_destroy_codec(codec);
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return EINA_FALSE;
     }

   opj_stream_set_user_data(st, &map_st, NULL);
   opj_stream_set_user_data_length(st, map_st.length);
   opj_stream_set_read_function(st, _jp2k_read_fn);
   opj_stream_set_skip_function(st, _jp2k_seek_cur_fn);
   opj_stream_set_seek_function(st, _jp2k_seek_set_fn);

   if (!opj_read_header(st, codec, &image))
     {
        ERR("can not read image header\n");
        opj_stream_destroy(st);
        opj_destroy_codec(codec);
        *error = EVAS_LOAD_ERROR_GENERIC;
        return EINA_FALSE;
     }

   if (!(opj_decode(codec, st, image) && opj_end_decompress(codec, st)))
     {
        ERR("can not decode image\n");
        opj_image_destroy(image);
        opj_stream_destroy(st);
        opj_destroy_codec(codec);
        *error = EVAS_LOAD_ERROR_GENERIC;
        return EINA_FALSE;
     }

   iter = pixels;
   idx = 0;

   /*
    * FIXME:
    * image->numcomps == 4, image->color_space == CLRSPC_SYCC : YUV
    */
   /* BGR(A) */
   if ((image->numcomps >= 3) &&
       (image->comps[0].dx == image->comps[1].dx) &&
       (image->comps[1].dx == image->comps[2].dx) &&
       (image->comps[0].dy == image->comps[1].dy) &&
       (image->comps[1].dy == image->comps[2].dy))
     {
        int a;
        int r;
        int g;
        int b;
        unsigned int i;
        unsigned int j;

        for (j = 0; j < image->comps[0].h; j++)
          {
             for (i = 0; i < image->comps[0].w; i++, idx++, iter++)
               {
                  r = image->comps[0].data[idx];
                  r+= (image->comps[0].sgnd ? 1 << (image->comps[0].prec - 1) : 0);
                  if (r > 255) r = 255;
                  if (r < 0) r = 0;

                  g = image->comps[1].data[idx];
                  g+= (image->comps[1].sgnd ? 1 << (image->comps[1].prec - 1) : 0);
                  if (g > 255) g = 255;
                  if (g < 0) g = 0;

                  b = image->comps[2].data[idx];
                  b+= (image->comps[2].sgnd ? 1 << (image->comps[2].prec - 1) : 0);
                  if (b > 255) b = 255;
                  if (b < 0) b = 0;

                  if (image->numcomps == 4)
                    {
                       a = image->comps[3].data[idx];
                       a+= (image->comps[3].sgnd ? 1 << (image->comps[3].prec - 1) : 0);
                       if (a > 255) a = 255;
                       if (a < 0) a = 0;
                    }
                  else
                    a = 255;

                  *iter = a << 24 | r << 16 | g << 8 | b;
               }
          }
     }
   /* *GRAY(A) */
   else if (((image->numcomps == 1) || (image->numcomps == 2)) &&
            (image->comps[0].dx == image->comps[1].dx) &&
            (image->comps[1].dx == image->comps[2].dx) &&
            (image->comps[0].dy == image->comps[1].dy) &&
            (image->comps[1].dy == image->comps[2].dy))
     {
        int a;
        int g;
        unsigned int i;
        unsigned int j;

        for (j = 0; j < image->comps[0].h; j++)
          {
             for (i = 0; i < image->comps[0].w; i++, idx++, iter++)
               {
                  g = image->comps[0].data[idx];
                  g+= (image->comps[0].sgnd ? 1 << (image->comps[0].prec - 1) : 0);
                  if (g > 255) g = 255;
                  if (g < 0) g = 0;

                  if (image->numcomps == 2)
                    {
                       a = image->comps[1].data[idx];
                       a+= (image->comps[1].sgnd ? 1 << (image->comps[1].prec - 1) : 0);
                       if (a > 255) a = 255;
                       if (a < 0) a = 0;
                    }
                  else
                    a = 255;

                  *iter = a << 24 | g << 16 | g << 8 | g;
               }
          }
     }

   opj_image_destroy(image);
   opj_stream_destroy(st);
   opj_destroy_codec(codec);

   *error = EVAS_LOAD_ERROR_NONE;
   return EINA_TRUE;
}

/**
 * @brief Evas image loader 'open' function for JP2K files.
 * Called by Evas to open a JP2K image file. Allocates and initializes
 * the internal loader data structure.
 * @param f Eina file handle.
 * @param key Optional key (unused for JP2K).
 * @param opts Load options.
 * @param animated Pointer to store animated properties (unused for JP2K).
 * @param[out] error Pointer to store the Evas load error code.
 * @return A pointer to the allocated loader data structure on success, NULL on failure.
 */
static void *
evas_image_load_file_open_jp2k(Eina_File *f, Eina_Stringshare *key EINA_UNUSED,
			       Evas_Image_Load_Opts *opts,
			       Evas_Image_Animated *animated EINA_UNUSED,
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

   return loader;
}

/**
 * @brief Evas image loader 'close' function for JP2K files.
 * Called by Evas to close a previously opened JP2K image file handle.
 * Frees the internal loader data structure.
 * @param loader_data Pointer to the loader data structure returned by open.
 */
static void
evas_image_load_file_close_jp2k(void *loader_data)
{
   free(loader_data);
}

/**
 * @brief Evas image loader 'head' function for JP2K files.
 * Called by Evas to read the header information (dimensions, alpha) of the image.
 * Maps the file to memory and calls the internal header reading function.
 * @param loader_data Pointer to the loader data structure.
 * @param[out] prop Pointer to store the image properties (width, height, alpha).
 * @param[out] error Pointer to store the Evas load error code.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_head_jp2k(void *loader_data,
                               Emile_Image_Property *prop,
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

   val = evas_image_load_file_head_jp2k_internal(&prop->w, &prop->h,
						 &prop->alpha,
                                                 map, eina_file_size_get(f),
                                                 error);

   eina_file_map_free(f, map);

   return val;
}

/**
 * @brief Evas image loader 'data' function for JP2K files.
 * Called by Evas to decode and load the actual image pixel data.
 * Maps the file to memory and calls the internal data loading function.
 * @param loader_data Pointer to the loader data structure.
 * @param prop Image properties (unused in this function but part of the API).
 * @param pixels Pointer to the destination buffer for decoded pixel data (ARGB32).
 * @param[out] error Pointer to store the Evas load error code.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_data_jp2k(void *loader_data,
                               Emile_Image_Property *prop EINA_UNUSED,
			       void *pixels,
			       int *error)
{
   Evas_Loader_Internal *loader = loader_data;
   Eina_File *f;
   void *map;
   Eina_Bool val = EINA_FALSE;

   f = loader->f;

   map = eina_file_map_all(f, EINA_FILE_WILLNEED);
   if (!map)
     {
        *error = EVAS_LOAD_ERROR_DOES_NOT_EXIST;
        goto on_error;
     }

   val = evas_image_load_file_data_jp2k_internal(pixels,
                                                 map, eina_file_size_get(f),
                                                 error);

   eina_file_map_free(f, map);

 on_error:
   return val;
}

/**
 * @brief Structure defining the Evas image loader functions for JP2K.
 * This structure is registered with Evas to provide the necessary callbacks
 * for opening, closing, reading header, and reading data for JP2K images.
 */
static Evas_Image_Load_Func evas_image_load_jp2k_func =
{
  EVAS_IMAGE_LOAD_VERSION, /**< Loader API version. */
  evas_image_load_file_open_jp2k,
  evas_image_load_file_close_jp2k,
  (void*) evas_image_load_file_head_jp2k,
  NULL,
  (void*) evas_image_load_file_data_jp2k,
  NULL,
  EINA_TRUE,
  EINA_TRUE /**< Supports loading directly into provided pixel buffer. */
};

/**
 * @brief Evas module initialization function.
 * Called when the Evas JP2K loader module is loaded. Registers the log domain
 * and sets the loader functions.
 * @param em The Evas module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;

   _evas_loader_jp2k_log_dom = eina_log_domain_register("evas-jp2k", EINA_COLOR_BLUE);
   if (_evas_loader_jp2k_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }

   em->functions = (void *)(&evas_image_load_jp2k_func);

   return 1;
}

/**
 * @brief Evas module shutdown function.
 * Called when the Evas JP2K loader module is unloaded. Unregisters the log domain.
 * @param em The Evas module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_loader_jp2k_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_loader_jp2k_log_dom);
        _evas_loader_jp2k_log_dom = -1;
     }
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "jp2k",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, jp2k);

#ifndef EVAS_STATIC_BUILD_JP2K
EVAS_EINA_MODULE_DEFINE(image_loader, jp2k);
#endif

