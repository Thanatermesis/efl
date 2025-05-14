#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include <jxl/encode.h>
#include <jxl/resizable_parallel_runner.h>

#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @brief Saves an RGBA_Image to a JXL file.
 *
 * This function encodes the image data using the libjxl library and writes
 * the compressed data to the specified file.
 *
 * @param im Pointer to the RGBA_Image structure containing the image data.
 *           The image data is expected in RGBA format.
 * @param file The path to the output JXL file.
 * @param quality The desired quality level for the JXL encoding (0-100).
 *                Higher values mean better quality and potentially larger file size.
 * @return 1 on success, 0 on failure.
 */
static int
save_image_jxl(RGBA_Image *im, const char *file, int quality)
{
   FILE *f;
   JxlParallelRunner *runner;
   JxlEncoder *encoder;
   JxlPixelFormat pixel_format;
   JxlBasicInfo basic_info;
   JxlColorEncoding color_encoding;
   JxlEncoderFrameSettings* frame_settings;
   JxlEncoderStatus st;
   JxlEncoderStatus process_result;
   unsigned char *compressed;
   unsigned char *next;
   void *pixels;
   unsigned long long int *iter_src;
   unsigned long long int *iter_dst;
   size_t size;
   size_t avail;
   size_t sz;
   unsigned int i;
   int ret = 0;

   if (!im || !im->image.data || !file || !*file)
     return ret;

   f = fopen(file, "wb");
   if (!f)
     return ret;

   runner = JxlResizableParallelRunnerCreate(NULL);
   if (!runner)
     goto close_f;

   encoder = JxlEncoderCreate(NULL);
   if (!encoder)
     goto destroy_runner;

   st = JxlEncoderSetParallelRunner(encoder,
                                    JxlResizableParallelRunner,
                                    runner);
   if (st != JXL_ENC_SUCCESS)
     goto destroy_encoder;

   JxlResizableParallelRunnerSetThreads(runner,
                                        JxlResizableParallelRunnerSuggestThreads(im->cache_entry.w, im->cache_entry.h));

   pixel_format.num_channels = 4;
   pixel_format.data_type = JXL_TYPE_UINT8;
#ifdef WORDS_BIGENDIAN
   pixel_format.endianness = JXL_BIG_ENDIAN;
#else
   pixel_format.endianness = JXL_LITTLE_ENDIAN;
#endif
   pixel_format.align = 0;

   JxlEncoderInitBasicInfo(&basic_info);
   basic_info.xsize = im->cache_entry.w;
   basic_info.ysize = im->cache_entry.h;
   basic_info.bits_per_sample = 8;
   basic_info.exponent_bits_per_sample = 0;
   basic_info.uses_original_profile = JXL_FALSE;
   basic_info.num_color_channels = 3;
   basic_info.num_extra_channels =1;
   basic_info.alpha_bits = 8;
   st = JxlEncoderSetBasicInfo(encoder, &basic_info);
   if (st != JXL_ENC_SUCCESS)
     goto destroy_encoder;

   memset(&color_encoding, 0, sizeof(JxlColorEncoding));
   JxlColorEncodingSetToSRGB(&color_encoding,
                             /*is gray ? */
                             pixel_format.num_channels < 3);
   st = JxlEncoderSetColorEncoding(encoder, &color_encoding);
   if (st != JXL_ENC_SUCCESS)
     goto destroy_encoder;

   frame_settings = JxlEncoderFrameSettingsCreate(encoder, NULL);
   if (!frame_settings)
     goto destroy_encoder;

   st = JxlEncoderFrameSettingsSetOption(frame_settings,
                                         JXL_ENC_FRAME_SETTING_EFFORT,
                                         (quality * 7) /100);
   if (st != JXL_ENC_SUCCESS)
     goto destroy_encoder;

   /*
    * JXL encoder expects pixel data in BGRA format for 4 channels.
    * Evas provides data in RGBA format.
    * This loop converts the pixel data from RGBA to BGRA.
    * It processes two pixels (8 bytes) at a time using 64-bit integers
    * for potential performance improvement.
    *
    * Example for one pixel (represented as 32-bit integer 0xAAGGBBRR):
    * Input RGBA pixel:   [RR, GG, BB, AA] (e.g., 0xFF0000FF for opaque red)
    * Output BGRA pixel:  [BB, GG, RR, AA] (e.g., 0x0000FFFF for opaque red)
    *
    * The 64-bit operation processes two adjacent pixels P1 and P2:
    * Input (iter_src): [R1 G1 B1 A1 R2 G2 B2 A2]
    * Output (iter_dst):[B1 G1 R1 A1 B2 G2 R2 A2]
    *
    * Masking and shifting:
    * - Keep A and G:   (*iter_src & 0xff00ff00ff00ff00) -> [00 G1 00 A1 00 G2 00 A2]
    * - Shift R:        ((*iter_src & 0x000000ff000000ff) << 16) -> [00 00 R1 00 00 00 R2 00]
    * - Shift B:        ((*iter_src & 0x00ff000000ff0000) >> 16) -> [B1 00 00 00 B2 00 00 00]
    * - Combine:        OR-ing these results gives the BGRA format.
    */
   pixels = malloc(4 * im->cache_entry.w * im->cache_entry.h);
   if (!pixels)
     goto destroy_encoder;

   iter_src = (unsigned long long int *)im->image.data;
   iter_dst = (unsigned long long int *)pixels;

   for (i = 0; i < ((im->cache_entry.w * im->cache_entry.h) >> 1); i++, iter_src++, iter_dst++)
     {
        *iter_dst =
          /* we keep A and G */
          (*iter_src & 0xff00ff00ff00ff00) |
          /* we shift R */
          ((*iter_src & 0x000000ff000000ff) << 16) |
          /* we shift B */
          ((*iter_src & 0x00ff000000ff0000) >> 16);
     }

   st = JxlEncoderAddImageFrame(frame_settings, &pixel_format,
                                (void*)pixels,
                                sizeof(int) * im->cache_entry.w * im->cache_entry.h);
   if (st != JXL_ENC_SUCCESS)
     goto free_pixels;

   /* Signal that all input frames have been added. */
   JxlEncoderCloseInput(encoder);

   /*
    * Process the encoded output. The JXL encoder might require multiple calls
    * to JxlEncoderProcessOutput to retrieve all compressed data.
    * The output buffer `compressed` is dynamically resized as needed.
    */
   size = 64; /* Initial buffer size */
   compressed = (unsigned char *)malloc(size);
   if (!compressed)
     goto free_pixels;

   next = compressed;
   avail = size - (next - compressed);
   process_result = JXL_ENC_NEED_MORE_OUTPUT;
   while (process_result == JXL_ENC_NEED_MORE_OUTPUT)
     {
        process_result = JxlEncoderProcessOutput(encoder, &next, &avail);
        if (process_result == JXL_ENC_NEED_MORE_OUTPUT)
          {
             size_t offset = next - compressed;
             size *= 2;
             compressed = realloc(compressed, size);
             next = compressed + offset;
             avail = size - offset;
          }
     }
   size = next - compressed;
   compressed = realloc(compressed, size);
   if (process_result != JXL_ENC_SUCCESS)
     goto free_compressed;

   sz = fwrite(compressed, size, 1, f);
   if (sz != 1)
     goto free_compressed;

   ret = 1;

 free_compressed:
   free(compressed);
 free_pixels:
   free(pixels);
 destroy_encoder:
   JxlEncoderDestroy(encoder);
 destroy_runner:
   JxlResizableParallelRunnerDestroy(runner);
 close_f:
   fclose(f);

   return ret;
}

/**
 * @brief Evas image saver function for JXL format.
 *
 * This function acts as a wrapper around save_image_jxl, conforming to the
 * Evas_Image_Save_Func interface.
 *
 * @param im Pointer to the RGBA_Image structure.
 * @param file The output filename.
 * @param key Unused parameter.
 * @param quality Quality setting (0-100).
 * @param compress Unused parameter.
 * @param encoding Unused parameter.
 * @return 1 on success, 0 on failure.
 */
static int evas_image_save_file_jxl(RGBA_Image *im, const char *file, const char *key EINA_UNUSED,
                                     int quality, int compress EINA_UNUSED, const char *encoding EINA_UNUSED)
{
   /* Call the core JXL saving function, passing only relevant parameters. */
   return save_image_jxl(im, file, quality);
}


static Evas_Image_Save_Func evas_image_save_jxl_func =
{
   evas_image_save_file_jxl
};

/**
 * @brief Opens the JXL image saver module.
 *
 * Called by Evas when loading the module. It registers the save function.
 *
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   /* Assign the function pointer structure to the module. */
   em->functions = (void *)(&evas_image_save_jxl_func);
   return 1;
}

/**
 * @brief Closes the JXL image saver module.
 *
 * Called by Evas when unloading the module. Currently does nothing.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   /* Nothing to clean up in this specific module. */
}

/* Module API structure */
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

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_SAVER, image_saver, jxl);

#ifndef EVAS_STATIC_BUILD_JXL
EVAS_EINA_MODULE_DEFINE(image_saver, jxl);
#endif
