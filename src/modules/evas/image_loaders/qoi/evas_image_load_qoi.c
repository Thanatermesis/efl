#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "evas_common_private.h"

/**
 * @file
 * @brief Evas image loader module for the QOI (Quite OK Image) format.
 *
 * This file implements the Evas image loader interface for loading images
 * stored in the QOI format. It decodes the QOI header and pixel data.
 *
 * Based on the original qoi.h code (MIT license):
 * https://github.com/phoboslab/qoi/blob/master/qoi.h
 * date: 2023 march the 14th
 */

/*
 * code based on original qoi.h code (MIT license):
 * https://github.com/phoboslab/qoi/blob/master/qoi.h
 * date: 2023 march the 14th
 */

#define QOI_ZEROARR(a) memset((a),0,sizeof(a)) /**< Macro to zero out an array. */

/* QOI operation codes */
#define QOI_OP_INDEX  0x00 /**< 00xxxxxx - Index into the previous pixel array */
#define QOI_OP_DIFF   0x40 /**< 01xxxxxx - Difference from the previous pixel (small) */
#define QOI_OP_LUMA   0x80 /**< 10xxxxxx - Difference from the previous pixel (large, luma-based) */
#define QOI_OP_RUN    0xc0 /**< 11xxxxxx - Run-length encoding */
#define QOI_OP_RGB    0xfe /**< 11111110 - Full RGB value */
#define QOI_OP_RGBA   0xff /**< 11111111 - Full RGBA value */

#define QOI_MASK_2    0xc0 /**< 11000000 - Mask to extract the 2 high bits of a QOI op byte */

/**
 * @brief Calculates a hash value for a given RGBA color.
 * Used for indexing into the recent pixel array.
 * @param C A qoi_rgba_t color structure.
 * @return An integer hash value based on the RGBA components.
 */
#define QOI_COLOR_HASH(C) (C.rgba.r*3 + C.rgba.g*5 + C.rgba.b*7 + C.rgba.a*11)

/**
 * @brief The magic bytes identifying a QOI file ("qoif").
 */
#define QOI_MAGIC \
	(((unsigned int)'q') << 24 | ((unsigned int)'o') << 16 | \
	 ((unsigned int)'i') <<  8 | ((unsigned int)'f'))

#define QOI_HEADER_SIZE 14 /**< Size of the QOI header in bytes. */

/**
 * @brief Maximum number of pixels allowed in a QOI image (width * height).
 * Used to prevent excessive memory allocation.
 */
#define QOI_PIXELS_MAX ((unsigned int)400000000)

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_loader_qoi_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_evas_loader_qoi_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_loader_qoi_log_dom, __VA_ARGS__)

/**
 * @brief Internal state structure for the QOI loader instance.
 */
typedef struct _Evas_Loader_Internal Evas_Loader_Internal;
struct _Evas_Loader_Internal
{
   Eina_File *f;                 /**< The opened Eina_File handle. */
   Evas_Image_Load_Opts *opts;   /**< Image loading options. */
   Evas_Image_Animated *animated;/**< Animated image properties (unused for QOI). */
};

/**
 * @brief Represents an RGBA color value.
 * Can be accessed as individual components (r, g, b, a) or as a single 32-bit integer (v).
 */
typedef union {
	struct { unsigned char r, g, b, a; } rgba; /**< Access as individual RGBA components. */
	unsigned int v; /**< Access as a single 32-bit integer. */
} qoi_rgba_t;

static int _evas_loader_qoi_log_dom = -1; /**< Log domain for the QOI loader module. */

/**
 * @brief Padding bytes expected at the end of a QOI file stream.
 * The QOI specification requires the stream to end with 7 `0x00` bytes followed by one `0x01` byte.
 * Example: `[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01]`
 */
static const unsigned char qoi_padding[8] = {0,0,0,0,0,0,0,1};

/**
 * @brief Reads a 32-bit unsigned integer from a byte buffer in big-endian format.
 *
 * @param data The byte buffer to read from.
 * @param p Pointer to the current read position within the buffer. This value is incremented by 4.
 * @return The 32-bit unsigned integer value read from the buffer.
 */
static unsigned int read_32(const unsigned char *data, int *p)
{
   unsigned int a = data[(*p)++];
   unsigned int b = data[(*p)++];
   unsigned int c = data[(*p)++];
   unsigned int d = data[(*p)++];
   return a << 24 | b << 16 | c << 8 | d;
}

/**
 * @brief Reads the header information from a mapped QOI file.
 *
 * This function parses the QOI header to determine image dimensions,
 * channel count, and colorspace, populating the Emile_Image_Property structure.
 * It performs basic validation checks on the header data.
 *
 * @param loader The internal loader state (unused in this function).
 * @param prop Pointer to the image property structure to populate. Output parameter.
 * @param map Pointer to the memory-mapped QOI file data.
 * @param length The total size of the mapped data in bytes.
 * @param error Pointer to an integer where the error code will be stored. Output parameter.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_head_qoi_internal(Evas_Loader_Internal *loader EINA_UNUSED,
                                       Emile_Image_Property *prop,
                                       void *map, size_t length,
                                       int *error)
{
   const unsigned char *bytes;
   unsigned int magic;
   unsigned char channels;
   unsigned char colorspace;
   int p = 0;
   Eina_Bool ret;

   ret = EINA_FALSE;
   prop->w = 0;
   prop->h = 0;
   prop->alpha = EINA_FALSE;

   if (length < QOI_HEADER_SIZE + sizeof(qoi_padding))
     {
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        return ret;
     }

   bytes = (const unsigned char *)map;

   magic = read_32(bytes, &p);
   if (magic != QOI_MAGIC)
     {
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        return ret;
     }

   prop->w = read_32(bytes, &p);
   prop->h = read_32(bytes, &p);
   if ((prop->w < 1) ||
       (prop->h < 1) ||
       (prop->h >= QOI_PIXELS_MAX / prop->w) ||
       (prop->w > IMG_MAX_SIZE) ||
       (prop->h > IMG_MAX_SIZE) ||
       IMG_TOO_BIG(prop->w, prop->h))
     {
        *error= EVAS_LOAD_ERROR_GENERIC;
        return ret;
     }

   channels = bytes[p++];
   colorspace = bytes[p++];

   if ((channels < 3) ||
       (channels > 4) ||
       (colorspace > 1))
     {
        *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
        return ret;
     }

   prop->alpha = channels == 4;

   *error = EVAS_LOAD_ERROR_NONE;
   return EINA_TRUE;
}

/**
 * @brief Decodes the pixel data from a mapped QOI file into an RGBA buffer.
 *
 * This function reads the QOI header again (for validation and context) and then
 * decodes the QOI chunk stream into the provided pixel buffer. It handles
 * all QOI operation codes (INDEX, DIFF, LUMA, RUN, RGB, RGBA).
 * The output format is always 32-bit RGBA, with premultiplied alpha if the
 * source image has an alpha channel.
 *
 * @param loader The internal loader state (unused in this function).
 * @param prop Pointer to the image property structure (used for dimensions and alpha).
 * @param pixels Pointer to the destination buffer where decoded RGBA pixel data will be written. Output parameter.
 * @param map Pointer to the memory-mapped QOI file data.
 * @param length The total size of the mapped data in bytes.
 * @param error Pointer to an integer where the error code will be stored. Output parameter.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_data_qoi_internal(Evas_Loader_Internal *loader EINA_UNUSED,
                                       Emile_Image_Property *prop,
                                       void *pixels,
                                       void *map, size_t length,
                                       int *error)
{
   /**
    * @brief Array storing the last 64 encountered pixel colors for QOI_OP_INDEX.
    * Example element at index `i`: `index[i] = { .rgba = { .r=R, .g=G, .b=B, .a=A } }`
    */
   qoi_rgba_t index[64];
   const unsigned char *bytes;
   qoi_rgba_t px;
   unsigned int *iter;
   unsigned int magic;
   unsigned char channels;
   unsigned char colorspace;
   int p = 0;
   int run = 0;
   int chunks_len;
   size_t px_len;
   size_t px_pos;
   Eina_Bool ret;

   ret = EINA_FALSE;
   prop->w = 0;
   prop->h = 0;
   prop->alpha = EINA_FALSE;

   if (length < QOI_HEADER_SIZE + sizeof(qoi_padding))
     {
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        return ret;
     }

   bytes = (const unsigned char *)map;

   magic = read_32(bytes, &p);
   if (magic != QOI_MAGIC)
     {
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        return ret;
     }

   prop->w = read_32(bytes, &p);
   prop->h = read_32(bytes, &p);
   if ((prop->w < 1) ||
       (prop->h < 1) ||
       (prop->h >= QOI_PIXELS_MAX / prop->w) ||
       (prop->w > IMG_MAX_SIZE) ||
       (prop->h > IMG_MAX_SIZE) ||
       IMG_TOO_BIG(prop->w, prop->h))
     {
        *error= EVAS_LOAD_ERROR_GENERIC;
        return ret;
     }

   channels = bytes[p++];
   colorspace = bytes[p++];

   if ((channels < 3) ||
       (channels > 4) ||
       (colorspace > 1))
     {
        *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
        return ret;
     }

   prop->alpha = channels == 4;

   px_len = prop->w * prop->h * channels;

   QOI_ZEROARR(index);
   px.rgba.r = 0;
   px.rgba.g = 0;
   px.rgba.b = 0;
   px.rgba.a = 255;

   iter = pixels;
   chunks_len = length - (int)sizeof(qoi_padding);
   for (px_pos = 0; px_pos < px_len; px_pos += channels, iter++)
     {
        if (run > 0)
          {
             run--;
          }
        else if (p < chunks_len)
          {
             int b1 = bytes[p++];

             if (b1 == QOI_OP_RGB)
               {
                  px.rgba.r = bytes[p++];
                  px.rgba.g = bytes[p++];
                  px.rgba.b = bytes[p++];
               }
             else if (b1 == QOI_OP_RGBA)
               {
                  px.rgba.r = bytes[p++];
                  px.rgba.g = bytes[p++];
                  px.rgba.b = bytes[p++];
                  px.rgba.a = bytes[p++];
               }
             else if ((b1 & QOI_MASK_2) == QOI_OP_INDEX)
               {
                  px = index[b1];
               }
             else if ((b1 & QOI_MASK_2) == QOI_OP_DIFF)
               {
                  px.rgba.r += ((b1 >> 4) & 0x03) - 2;
                  px.rgba.g += ((b1 >> 2) & 0x03) - 2;
                  px.rgba.b += ( b1       & 0x03) - 2;
               }
             else if ((b1 & QOI_MASK_2) == QOI_OP_LUMA)
               {
                  int b2 = bytes[p++];
                  int vg = (b1 & 0x3f) - 32;
                  px.rgba.r += vg - 8 + ((b2 >> 4) & 0x0f);
                  px.rgba.g += vg;
                  px.rgba.b += vg - 8 +  (b2       & 0x0f);
               }
             else if ((b1 & QOI_MASK_2) == QOI_OP_RUN)
               {
                  run = (b1 & 0x3f);
               }

             index[QOI_COLOR_HASH(px) % 64] = px;
          }

        if (prop->alpha)
          *iter = (px.rgba.a << 24) |
                  (((px.rgba.r * px.rgba.a) / 255) << 16) |
                  (((px.rgba.g * px.rgba.a) / 255) << 8) |
                  (((px.rgba.b * px.rgba.a) / 255));
        else
          *iter = (255 << 24) |
                  (px.rgba.r << 16) |
                  (px.rgba.g << 8) |
                  (px.rgba.b);
     }

   *error = EVAS_LOAD_ERROR_NONE;
   return EINA_TRUE;
}

/**
 * @brief Opens a QOI file for loading. Evas loader interface function.
 *
 * Allocates and initializes the internal loader state structure.
 *
 * @param f The Eina_File handle representing the opened file.
 * @param key The file key (unused).
 * @param opts Image loading options.
 * @param animated Animated image properties (unused).
 * @param error Pointer to an integer where the error code will be stored. Output parameter.
 * @return A pointer to the allocated Evas_Loader_Internal structure on success, NULL on failure.
 */
static void *
evas_image_load_file_open_qoi(Eina_File *f, Eina_Stringshare *key EINA_UNUSED,
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
 * @brief Closes the QOI file loader instance. Evas loader interface function.
 *
 * Frees the internal loader state structure. The Eina_File handle is managed
 * by the caller.
 *
 * @param loader_data Pointer to the Evas_Loader_Internal structure to free.
 */
static void
evas_image_load_file_close_qoi(void *loader_data)
{
   free(loader_data);
}

/**
 * @brief Reads the header information from a QOI file. Evas loader interface function.
 *
 * This function memory-maps the file, calls the internal header reading function,
 * and then unmaps the file.
 *
 * @param loader_data Pointer to the Evas_Loader_Internal structure.
 * @param prop Pointer to the image property structure to populate. Output parameter.
 * @param error Pointer to an integer where the error code will be stored. Output parameter.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_head_qoi(void *loader_data,
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

   val = evas_image_load_file_head_qoi_internal(loader,
                                                (Emile_Image_Property *)prop,
                                                map, eina_file_size_get(f),
                                                error);

   eina_file_map_free(f, map);

   return val;
}

/**
 * @brief Loads the pixel data from a QOI file. Evas loader interface function.
 *
 * This function memory-maps the file, calls the internal data decoding function
 * to populate the pixel buffer, and then unmaps the file.
 *
 * @param loader_data Pointer to the Evas_Loader_Internal structure.
 * @param prop Pointer to the image property structure (contains dimensions, etc.).
 * @param pixels Pointer to the destination buffer for the decoded RGBA pixel data. Output parameter.
 * @param error Pointer to an integer where the error code will be stored. Output parameter.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_data_qoi(void *loader_data,
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

   val = evas_image_load_file_data_qoi_internal(loader,
                                                (Emile_Image_Property *)prop,
                                                pixels,
                                                map, eina_file_size_get(f),
                                                error);

   eina_file_map_free(f, map);

 on_error:
   return val;
}

/**
 * @brief Structure defining the Evas image loader functions for QOI.
 */
static Evas_Image_Load_Func evas_image_load_qoi_func =
{
   EVAS_IMAGE_LOAD_VERSION,        /**< Loader API version. */
   evas_image_load_file_open_qoi,  /**< Function to open a file. */
   evas_image_load_file_close_qoi, /**< Function to close a file. */
   evas_image_load_file_head_qoi,  /**< Function to read image header. */
   NULL,                           /**< Function to read image header from stringshare (unused). */
   evas_image_load_file_data_qoi,  /**< Function to load image pixel data. */
   NULL,                           /**< Function to load image data from stringshare (unused). */
   EINA_TRUE,                      /**< Supports loading from file. */
   EINA_FALSE                      /**< Does not support loading animated images. */
};

/**
 * @brief Initializes the QOI image loader module. Evas module interface function.
 *
 * Registers the log domain and sets the loader function structure.
 *
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;

   _evas_loader_qoi_log_dom = eina_log_domain_register("evas-qoi", EINA_COLOR_BLUE);
   if (_evas_loader_qoi_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }

   em->functions = (void *)(&evas_image_load_qoi_func);

   return 1;
}

/**
 * @brief Shuts down the QOI image loader module. Evas module interface function.
 *
 * Unregisters the log domain.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_loader_qoi_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_loader_qoi_log_dom);
        _evas_loader_qoi_log_dom = -1;
     }
}

/**
 * @brief Structure defining the Evas module API for the QOI loader.
 */
static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION, /**< Module API version. */
   "qoi",                   /**< Module name. */
   "none",                  /**< Module license. */
   { /**< Module functions. */
     module_open,           /**< Function to open the module. */
     module_close           /**< Function to close the module. */
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, qoi);

#ifndef EVAS_STATIC_BUILD_QOI
EVAS_EINA_MODULE_DEFINE(image_loader, qoi);
#endif
