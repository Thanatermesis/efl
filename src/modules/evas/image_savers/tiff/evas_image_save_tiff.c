#include "evas_common_private.h"
#include "evas_private.h"

#include <tiffio.h>

static int evas_image_save_file_tiff(RGBA_Image *im, const char *file, const char *key, int quality, int compress, const char *encoding);

static Evas_Image_Save_Func evas_image_save_tiff_func =
{
   evas_image_save_file_tiff
};

/**
 * @brief Saves an RGBA_Image to a TIFF file.
 *
 * This function handles the core logic of writing image data to a TIFF file,
 * including setting necessary TIFF tags and handling pixel data conversion.
 *
 * @param im The RGBA_Image structure containing the image data and properties.
 *           im->image.data contains the pixel data in 32-bit format (ARGB).
 *           im->cache_entry.w holds the image width.
 *           im->cache_entry.h holds the image height.
 *           im->cache_entry.flags.alpha indicates if the image has an alpha channel.
 * @param file The path to the output TIFF file. Example: "/path/to/output.tiff"
 * @param compress Unused parameter (compression is fixed to DEFLATE).
 * @param interlace Unused parameter.
 * @return 1 on success, 0 on failure.
 */
static int
save_image_tiff(RGBA_Image *im, const char *file, int compress EINA_UNUSED, int interlace EINA_UNUSED)
{
   TIFF               *tif = NULL;
   uint8_t            *buf = NULL;
   DATA32              pixel;
   DATA32             *data;
   uint32_t            x, y;
   uint8_t             r, g, b, a = 0;
   int                 i = 0;
   int                 has_alpha;

   if (!im || !im->image.data || !file)
      return 0;

   has_alpha = im->cache_entry.flags.alpha;
   data = im->image.data;

   tif = TIFFOpen(file, "w");
   if (!tif)
      return 0;

   /* None of the TIFFSetFields are checked for errors, but since they */
   /* shouldn't fail, this shouldn't be a problem */

   TIFFSetField(tif, TIFFTAG_IMAGELENGTH, im->cache_entry.h); // Image height
   TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, im->cache_entry.w); // Image width
   TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB); // Color space
   TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG); // Pixel data layout (chunky)
   TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT); // Origin is top-left
   TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE); // No specific resolution unit

   /* By default uses patent-free use COMPRESSION_DEFLATE,
    * another lossless compression technique */
   TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE); // Use zlib compression

   /* Configure samples per pixel based on alpha channel presence */
   if (has_alpha)
     {
        uint16_t extras[] = { EXTRASAMPLE_ASSOCALPHA };
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 4);
        TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, extras);
     }
   else
     {
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
     }

   TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
   TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, 0));

   buf = (uint8_t *) _TIFFmalloc(TIFFScanlineSize(tif));
   if (!buf)
     {
        TIFFClose(tif);
        return 0;
     }

   /* Iterate over each row and pixel to fill the TIFF scanline buffer */
   for (y = 0; y < im->cache_entry.h; y++)
     {
        i = 0; // Reset buffer index for each row
        for (x = 0; x < im->cache_entry.w; x++)
          {
             pixel = data[(y * im->cache_entry.w) + x];

             r = (pixel >> 16) & 0xff;
             g = (pixel >> 8) & 0xff;
             b = pixel & 0xff;
             if (has_alpha)
                a = (pixel >> 24) & 0xff; // Extract alpha component

             /* This might be endian dependent */
             /* Fill the buffer in R, G, B[, A] order */
             /* Example pixel (ARGB): 0xAARRGGBB */
             /* buf layout for RGB:   [R, G, B, R, G, B, ...] */
             /* buf layout for RGBA:  [R, G, B, A, R, G, B, A, ...] */
             buf[i++] = r; // Red component
             buf[i++] = g; // Green component
             buf[i++] = b; // Blue component
             if (has_alpha)
                buf[i++] = a; // Alpha component
          }

        if (!TIFFWriteScanline(tif, buf, y, 0))
          {
             _TIFFfree(buf);
             TIFFClose(tif);
             return 0;
          }
     }

   _TIFFfree(buf);
   TIFFClose(tif);

   return 1;
}

/**
 * @brief Evas image saver function for TIFF format.
 *
 * This function acts as the entry point for the Evas image saving mechanism
 * for the TIFF format. It calls the internal save_image_tiff function.
 *
 * @param im The RGBA_Image to save.
 * @param file The destination file path.
 * @param key Unused parameter.
 * @param quality Unused parameter.
 * @param compress Compression flag (passed to save_image_tiff, currently unused there).
 * @param encoding Unused parameter.
 * @return 1 on success, 0 on failure, as returned by save_image_tiff.
 */
static int evas_image_save_file_tiff(RGBA_Image *im, const char *file, const char *key EINA_UNUSED,
                                     int quality EINA_UNUSED, int compress, const char *encoding EINA_UNUSED)
{
   return save_image_tiff(im, file, compress, 0);
}

/**
 * @brief Opens the TIFF image saver module.
 *
 * Called by Evas when loading the module. It assigns the save function
 * implementation (evas_image_save_tiff_func) to the module structure.
 *
 * @param em The Evas_Module structure to initialize.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_image_save_tiff_func);
   return 1;
}

/**
 * @brief Closes the TIFF image saver module.
 *
 * Called by Evas when unloading the module. Currently performs no cleanup.
 *
 * @param em The Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "tiff",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_SAVER, image_saver, tiff);

#ifndef EVAS_STATIC_BUILD_TIFF
EVAS_EINA_MODULE_DEFINE(image_saver, tiff);
#endif

