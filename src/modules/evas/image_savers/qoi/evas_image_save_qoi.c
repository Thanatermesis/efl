#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @file
 * @brief This file implements the QOI image saving functionality for Evas.
 * It is based on the original qoi.h code.
 */

/*
 * code based on original qoi.h code (MIT license):
 * https://github.com/phoboslab/qoi/blob/master/qoi.h
 * date: 2023 march the 14th
 */

#define QOI_SRGB   0 /**< QOI colorspace sRGB. */

#define QOI_ZEROARR(a) memset((a),0,sizeof(a)) /**< Macro to zero out an array. */

#define QOI_OP_INDEX  0x00 /**< QOI operation: index. 00xxxxxx */
#define QOI_OP_DIFF   0x40 /**< QOI operation: diff. 01xxxxxx */
#define QOI_OP_LUMA   0x80 /**< QOI operation: luma. 10xxxxxx */
#define QOI_OP_RUN    0xc0 /**< QOI operation: run.   11xxxxxx */
#define QOI_OP_RGB    0xfe /**< QOI operation: RGB.   11111110 */
#define QOI_OP_RGBA   0xff /**< QOI operation: RGBA.  11111111 */

/**
 * @brief Macro to calculate a hash for a QOI RGBA color.
 * @param C The qoi_rgba_t color.
 * @return The hash value.
 */
#define QOI_COLOR_HASH(C) (C.rgba.r*3 + C.rgba.g*5 + C.rgba.b*7 + C.rgba.a*11)

/**
 * @brief Magic number for QOI file format ('qoif').
 */
#define QOI_MAGIC \
	(((unsigned int)'q') << 24 | ((unsigned int)'o') << 16 | \
	 ((unsigned int)'i') <<  8 | ((unsigned int)'f'))

#define QOI_HEADER_SIZE 14 /**< Size of the QOI header in bytes. */

#define QOI_PIXELS_MAX ((unsigned int)400000000) /**< Maximum number of pixels QOI can handle (width * height). */

/**
 * @brief Union representing a QOI RGBA color.
 * Can be accessed as individual r, g, b, a components or as a single unsigned int.
 */
typedef union {
	struct { unsigned char r, g, b, a; } rgba; /**< RGBA components. */
	unsigned int v; /**< Integer representation of the color. */
} qoi_rgba_t;

/**
 * @brief Padding bytes for the end of a QOI file.
 * The QOI format specification requires a specific 7-byte padding followed by a 1.
 */
static const unsigned char qoi_padding[8] = {0,0,0,0,0,0,0,1};

/**
 * @brief Saves an RGBA_Image to a QOI file.
 *
 * This function implements the core QOI encoding logic.
 *
 * @param im Pointer to the RGBA_Image structure to save.
 *           The image data is expected to be in pre-multiplied ARGB format if alpha is present,
 *           or BGRx if no alpha. This function handles conversion to non-premultiplied RGBA.
 * @param file The path to the output QOI file.
 * @param quality Unused parameter, present for API compatibility.
 * @return 1 on success, 0 on failure.
 */
static int
save_image_qoi(RGBA_Image *im, const char *file, int quality EINA_UNUSED)
{
   qoi_rgba_t index[64];
   qoi_rgba_t px;
   qoi_rgba_t px_prev;
   FILE *f;
   unsigned char *iter;
   unsigned int channels;
   unsigned int colorspace;
   unsigned int v;
   int run;
   int px_len;
   int px_end;
   int px_pos;
   int i;
   int ret = 0;

   if (!im || !im->image.data || !file || !*file)
     return ret;

   if (im->cache_entry.h >= QOI_PIXELS_MAX / im->cache_entry.w)
     return ret;

   f = fopen(file, "wb");
   if (!f)
     return ret;

   v = (0xff000000 & QOI_MAGIC) >> 24;
   if (fwrite(&v, 1, 1, f) != 1) goto close_f;
   v = (0x00ff0000 & QOI_MAGIC) >> 16;
   if (fwrite(&v, 1, 1, f) != 1) goto close_f;
   v = (0x0000ff00 & QOI_MAGIC) >> 8;
   if (fwrite(&v, 1, 1, f) != 1) goto close_f;
   v = (0x000000ff & QOI_MAGIC);
   if (fwrite(&v, 1, 1, f) != 1) goto close_f;

   v = (0xff000000 & im->cache_entry.w) >> 24;
   if (fwrite(&v, 1, 1, f) != 1) goto close_f;
   v = (0x00ff0000 & im->cache_entry.w) >> 16;
   if (fwrite(&v, 1, 1, f) != 1) goto close_f;
   v = (0x0000ff00 & im->cache_entry.w) >> 8;
   if (fwrite(&v, 1, 1, f) != 1) goto close_f;
   v = (0x000000ff & im->cache_entry.w);
   if (fwrite(&v, 1, 1, f) != 1) goto close_f;

   v = (0xff000000 & im->cache_entry.h) >> 24;
   if (fwrite(&v, 1, 1, f) != 1) goto close_f;
   v = (0x00ff0000 & im->cache_entry.h) >> 16;
   if (fwrite(&v, 1, 1, f) != 1) goto close_f;
   v = (0x0000ff00 & im->cache_entry.h) >> 8;
   if (fwrite(&v, 1, 1, f) != 1) goto close_f;
   v = (0x000000ff & im->cache_entry.h);
   if (fwrite(&v, 1, 1, f) != 1) goto close_f;

   channels = 4;
   if (fwrite(&channels, 1, 1, f) != 1) goto close_f;

   colorspace = QOI_SRGB;
   if (fwrite(&colorspace, 1, 1, f) != 1) goto close_f;

   QOI_ZEROARR(index);

   run = 0;
   px_prev.rgba.r = 0;
   px_prev.rgba.g = 0;
   px_prev.rgba.b = 0;
   px_prev.rgba.a = 255;
   px = px_prev;

   px_len = im->cache_entry.w * im->cache_entry.h * channels;
   px_end = px_len - channels;

   iter = (unsigned char *)im->image.data;
   for (px_pos = 0; px_pos < px_len; px_pos += channels, iter +=4)
     {
        px.rgba.a = *(iter + 3);
        if (px.rgba.a == 0)
          {
             px.rgba.b = *(iter + 0);
             px.rgba.g = *(iter + 1);
             px.rgba.r = *(iter + 2);
          }
        else
          {
             px.rgba.b = *(iter + 0) * 255 / px.rgba.a;
             px.rgba.g = *(iter + 1) * 255 / px.rgba.a;
             px.rgba.r = *(iter + 2) * 255 / px.rgba.a;
          }

        if (px.v == px_prev.v)
          {
             run++;
             if (run == 62 || px_pos == px_end)
               {
                  unsigned char val = QOI_OP_RUN | (run - 1);
                  if (fwrite(&val, 1, 1, f) != 1) goto close_f;
                  run = 0;
               }
          }
        else
          {
             int index_pos;

             if (run > 0)
               {
                  unsigned char val = QOI_OP_RUN | (run - 1);
                  if (fwrite(&val, 1, 1, f) != 1) goto close_f;
                  run = 0;
               }

             index_pos = QOI_COLOR_HASH(px) % 64;

             if (index[index_pos].v == px.v)
               {
                  unsigned char val = QOI_OP_INDEX | index_pos;
                  if (fwrite(&val, 1, 1, f) != 1) goto close_f;
               }
             else
               {
                  index[index_pos] = px;

                  if (px.rgba.a == px_prev.rgba.a)
                    {
                       signed char vr = px.rgba.r - px_prev.rgba.r;
                       signed char vg = px.rgba.g - px_prev.rgba.g;
                       signed char vb = px.rgba.b - px_prev.rgba.b;

                       signed char vg_r = vr - vg;
                       signed char vg_b = vb - vg;

                       if (vr > -3 && vr < 2 &&
                           vg > -3 && vg < 2 &&
                           vb > -3 && vb < 2)
                         {
                            unsigned char val = QOI_OP_DIFF | (vr + 2) << 4 | (vg + 2) << 2 | (vb + 2);
                            if (fwrite(&val, 1, 1, f) != 1) goto close_f;
                         }
                       else if (vg_r >  -9 && vg_r <  8 &&
                                vg   > -33 && vg   < 32 &&
                                vg_b >  -9 && vg_b <  8)
                         {
                            unsigned char val;

                            val = QOI_OP_LUMA | (vg   + 32);
                            if (fwrite(&val, 1, 1, f) != 1) goto close_f;

                            val = (vg_r + 8) << 4 | (vg_b +  8);
                            if (fwrite(&val, 1, 1, f) != 1) goto close_f;
                         }
                       else
                         {
                            unsigned char val;

                            val = QOI_OP_RGB;
                            if (fwrite(&val, 1, 1, f) != 1) goto close_f;
                            if (fwrite(&px.rgba.r, 1, 1, f) != 1) goto close_f;
                            if (fwrite(&px.rgba.g, 1, 1, f) != 1) goto close_f;
                            if (fwrite(&px.rgba.b, 1, 1, f) != 1) goto close_f;
                         }
                    }
                  else
                    {
                       unsigned char val;

                       val = QOI_OP_RGBA;
                       if (fwrite(&val, 1, 1, f) != 1) goto close_f;
                       if (fwrite(&px.rgba.r, 1, 1, f) != 1) goto close_f;
                       if (fwrite(&px.rgba.g, 1, 1, f) != 1) goto close_f;
                       if (fwrite(&px.rgba.b, 1, 1, f) != 1) goto close_f;
                       if (fwrite(&px.rgba.a, 1, 1, f) != 1) goto close_f;
                    }
               }
          }
        px_prev = px;
     }

   for (i = 0; i < (int)sizeof(qoi_padding); i++)
     {
        if (fwrite(&qoi_padding[i], 1, 1, f) != 1) goto close_f;
     }

   ret = 1;

 close_f:
   fclose(f);

   return ret;
}

/**
 * @brief Wrapper function to save an RGBA_Image to a QOI file.
 *
 * This function conforms to the Evas_Image_Save_Func interface.
 *
 * @param im Pointer to the RGBA_Image structure to save.
 * @param file The path to the output QOI file.
 * @param key Unused parameter.
 * @param quality Quality setting for the save operation (unused for QOI).
 * @param compress Compression level (unused for QOI).
 * @param encoding Encoding type (unused for QOI).
 * @return 1 on success, 0 on failure.
 */
static int evas_image_save_file_qoi(RGBA_Image *im, const char *file, const char *key EINA_UNUSED,
                                     int quality, int compress EINA_UNUSED, const char *encoding EINA_UNUSED)
{
   return save_image_qoi(im, file, quality);
}

/**
 * @brief Structure holding the QOI image save function.
 */
static Evas_Image_Save_Func evas_image_save_qoi_func =
{
   evas_image_save_file_qoi
};

/**
 * @brief Opens the QOI image saver module.
 *
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_image_save_qoi_func);
   return 1;
}

/**
 * @brief Closes the QOI image saver module.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
}

/**
 * @brief API structure for the QOI image saver module.
 */
static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "qoi",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_SAVER, image_saver, qoi);

#ifndef EVAS_STATIC_BUILD_QOI
EVAS_EINA_MODULE_DEFINE(image_saver, qoi);
#endif
