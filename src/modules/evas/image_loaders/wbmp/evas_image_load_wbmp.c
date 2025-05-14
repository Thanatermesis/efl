#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @brief Reads a multi-byte integer from a memory map.
 *
 * This function decodes a variable-length integer format where the most
 * significant bit of each byte indicates if more bytes follow.
 *
 * @param data Pointer to store the decoded unsigned integer.
 * @param map Pointer to the memory-mapped file data.
 * @param length Total length of the memory-mapped data.
 * @param position Pointer to the current read position within the map;
 *                 it will be updated after reading.
 * @return 0 on success, -1 on error (e.g., read past end of data, or
 *         integer too long).
 */
static int
read_mb(unsigned int *data, void *map, size_t length, size_t *position)
{
   int ac = 0, ct;
   unsigned char buf;

   for (ct = 0;;)
     {
        if ((ct++) == 5) return -1;
	if (*position > length) return -1;
	buf = ((unsigned char *) map)[(*position)++];
        ac = (ac << 7) | (buf & 0x7f);
        if ((buf & 0x80) == 0) break;
     }
   *data = ac;
   return 0;
}

/**
 * @brief Opens a WBMP image file for loading.
 *
 * This function is part of the Evas image loader interface. For WBMP,
 * it simply returns the Eina_File handle as the loader_data.
 *
 * @param f The Eina_File handle for the image file.
 * @param key Unused for WBMP.
 * @param opts Unused for WBMP.
 * @param animated Unused for WBMP.
 * @param error Unused for WBMP.
 * @return The Eina_File handle as a void pointer, to be used as loader_data.
 */
static void *
evas_image_load_file_open_wbmp(Eina_File *f, Eina_Stringshare *key EINA_UNUSED,
			       Evas_Image_Load_Opts *opts EINA_UNUSED,
			       Evas_Image_Animated *animated EINA_UNUSED,
			       int *error EINA_UNUSED)
{
   return f;
}

/**
 * @brief Closes a WBMP image file after loading.
 *
 * This function is part of the Evas image loader interface. For WBMP,
 * it's a no-op as the Eina_File is managed externally or by other
 * loader functions.
 *
 * @param loader_data Unused for WBMP.
 */
static void
evas_image_load_file_close_wbmp(void *loader_data EINA_UNUSED)
{
}

/**
 * @brief Reads the header of a WBMP image file.
 *
 * This function parses the WBMP header to determine image properties like
 * width and height. It performs basic validation of the WBMP format.
 *
 * @param loader_data The Eina_File handle cast to void*.
 * @param prop Pointer to an Emile_Image_Property struct to store image
 *             dimensions and other properties.
 * @param error Pointer to an integer to store an Evas_Load_Error code.
 * @return EINA_TRUE on successful header parsing, EINA_FALSE otherwise.
 */
static Eina_Bool
evas_image_load_file_head_wbmp(void *loader_data,
			       Emile_Image_Property *prop,
			       int *error)
{
   Eina_File *f = loader_data;
   void *map = NULL;
   size_t position = 0;
   size_t length;
   unsigned int type, w, h;
   Eina_Bool r = EINA_FALSE;

   *error = EVAS_LOAD_ERROR_GENERIC;
   length = eina_file_size_get(f);
   if (length <= 4) goto bail;

   map = eina_file_map_all(f, EINA_FILE_RANDOM);
   if (!map) goto bail;

   if (read_mb(&type, map, length, &position) < 0) goto bail;

   if (type != 0)
     {
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        goto bail;
     }

   position++; /* skipping one byte */
   if (read_mb(&w, map, length, &position) < 0) goto bail;
   if (read_mb(&h, map, length, &position) < 0) goto bail;

   /* Wbmp header identifier is too weak....
      Here checks size validation whether it's acutal wbmp or not. */
   if ((((w + 7) >> 3) * h) + position != length)
     {
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        goto bail;
     }

   if ((w < 1) || (h < 1) || (w > IMG_MAX_SIZE) || (h > IMG_MAX_SIZE) ||
       IMG_TOO_BIG(w, h))
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        goto bail;
     }

   prop->w = w;
   prop->h = h;

   *error = EVAS_LOAD_ERROR_NONE;
   r = EINA_TRUE;

 bail:
   if (map) eina_file_map_free(f, map);
   return r;
}

/**
 * @brief Loads the image data from a WBMP file.
 *
 * This function reads the pixel data from the WBMP file and converts it
 * into a 32-bit ARGB format (though WBMP is monochrome, so alpha is full
 * and R, G, B are either 0 or 255).
 *
 * @param loader_data The Eina_File handle cast to void*.
 * @param prop Pointer to an Emile_Image_Property struct containing expected
 *             image dimensions (already filled by head_wbmp).
 * @param pixels Pointer to the destination buffer where the decoded image
 *               data (in DATA32 format) will be stored.
 * @param error Pointer to an integer to store an Evas_Load_Error code.
 * @return EINA_TRUE on successful image data loading, EINA_FALSE otherwise.
 */
static Eina_Bool
evas_image_load_file_data_wbmp(void *loader_data,
			       Emile_Image_Property *prop,
			       void *pixels,
			       int *error)
{
   Eina_File *f = loader_data;
   void *map = NULL;
   size_t position = 0;
   size_t length;
   unsigned int type, w, h;
   unsigned int line_length;
   unsigned char *line = NULL;
   int cur = 0, x, y;
   DATA32 *dst_data;
   Eina_Bool r = EINA_FALSE;

   *error = EVAS_LOAD_ERROR_GENERIC;
   length = eina_file_size_get(f);
   if (length <= 4) goto bail;

   map = eina_file_map_all(f, EINA_FILE_SEQUENTIAL);
   if (!map) goto bail;

   if (read_mb(&type, map, length, &position) < 0) goto bail;

   if (type != 0)
     {
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        goto bail;
     }

   position++; /* skipping one byte */
   if (read_mb(&w, map, length, &position) < 0) goto bail;
   if (read_mb(&h, map, length, &position) < 0) goto bail;

   /* Wbmp header identifier is too weak....
      Here checks size validation whether it's acutal wbmp or not. */
   if ((((w + 7) >> 3) * h) + position != length)
     {
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        goto bail;
     }

   if ((w < 1) || (h < 1) || (w > IMG_MAX_SIZE) || (h > IMG_MAX_SIZE) ||
       IMG_TOO_BIG(w, h))
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        goto bail;
     }

   if (prop->w != w || prop->h != h)
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        goto bail;
     }

   dst_data = pixels;

   line_length = (prop->w + 7) >> 3;

   for (y = 0; y < (int)prop->h; y++)
     {
        line = ((unsigned char*) map) + position;
        position += line_length;
        for (x = 0; x < (int)prop->w; x++)
          {
             int idx = x >> 3;
             int offset = 1 << (0x07 - (x & 0x07));
             if (line[idx] & offset) dst_data[cur] = 0xffffffff;
             else dst_data[cur] = 0xff000000;
             cur++;
          }
     }

   *error = EVAS_LOAD_ERROR_NONE;
   r = EINA_TRUE;

 bail:
   if (map) eina_file_map_free(f, map);
   return r;
}

static Evas_Image_Load_Func evas_image_load_wbmp_func =
{
   EVAS_IMAGE_LOAD_VERSION,
   evas_image_load_file_open_wbmp,
   evas_image_load_file_close_wbmp,
   (void*) evas_image_load_file_head_wbmp,
   NULL,
   (void*) evas_image_load_file_data_wbmp,
   NULL,
   EINA_TRUE,
   EINA_FALSE
};

/**
 * @brief Initializes the WBMP image loader module.
 *
 * This function is called when Evas loads the WBMP image loader module.
 * It registers the loader functions with the Evas module system.
 *
 * @param em Pointer to the Evas_Module structure for this loader.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_image_load_wbmp_func);
   return 1;
}

/**
 * @brief Shuts down the WBMP image loader module.
 *
 * This function is called when Evas unloads the WBMP image loader module.
 * For this loader, it's a no-op.
 *
 * @param em Unused.
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "wbmp",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, wbmp);

#ifndef EVAS_STATIC_BUILD_WBMP
EVAS_EINA_MODULE_DEFINE(image_loader, wbmp);
#endif
