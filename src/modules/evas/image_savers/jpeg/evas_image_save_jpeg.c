#include "evas_common_private.h"
#include "evas_private.h"

#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>

static int evas_image_save_file_jpeg(RGBA_Image *im, const char *file, const char *key, int quality, int compress, const char *encoding);

static Evas_Image_Save_Func evas_image_save_jpeg_func =
{
   evas_image_save_file_jpeg
};

/**
 * @brief Custom error manager structure for libjpeg.
 *
 * This structure extends the standard jpeg_error_mgr to include a jump buffer
 * for handling fatal errors gracefully.
 */
struct _JPEG_error_mgr
{
   struct     jpeg_error_mgr pub; /**< Public part of the error manager. */
   jmp_buf    setjmp_buffer; /**< Jump buffer for error recovery. */
};
typedef struct _JPEG_error_mgr *emptr;

/**
 * @brief Fatal error handler for libjpeg.
 *
 * This function is called by libjpeg when a fatal error occurs.
 * It uses longjmp to return control to the point set by setjmp,
 * allowing the program to clean up and exit gracefully instead of aborting.
 *
 * @param cinfo Pointer to the JPEG compression/decompression object.
 */
static void _JPEGFatalErrorHandler(j_common_ptr cinfo);
static void
_JPEGFatalErrorHandler(j_common_ptr cinfo)
{
   emptr errmgr;

   errmgr = (emptr) cinfo->err;
   longjmp(errmgr->setjmp_buffer, 1);
   return;
}

/**
 * @brief Non-fatal error handler for libjpeg (output_message).
 *
 * This function is registered as the output_message handler.
 * Currently, it does nothing, effectively suppressing warning/trace messages.
 *
 * @param cinfo Pointer to the JPEG compression/decompression object.
 */
static void
_JPEGErrorHandler(j_common_ptr cinfo EINA_UNUSED)
{
/*    emptr errmgr; */

/*    errmgr = (emptr) cinfo->err; */
   return;
}

/**
 * @brief Non-fatal error handler for libjpeg (emit_message).
 *
 * This function is registered as the emit_message handler.
 * Currently, it does nothing, effectively suppressing warning/trace messages
 * based on message level.
 *
 * @param cinfo Pointer to the JPEG compression/decompression object.
 * @param msg_level Message level (e.g., warning, trace).
 */
static void
_JPEGErrorHandler2(j_common_ptr cinfo EINA_UNUSED, int msg_level EINA_UNUSED)
{
/*    emptr errmgr; */

/*    errmgr = (emptr) cinfo->err; */
   return;
}

/**
 * @brief Saves an RGBA_Image structure to a JPEG file.
 *
 * This function takes an Evas internal image representation (RGBA_Image),
 * converts it to RGB, and saves it as a JPEG file using libjpeg.
 * It handles setting up the compression parameters, error handling,
 * and writing the scanlines.
 *
 * @param im Pointer to the RGBA_Image to save. Must contain valid image data.
 *           The image data is expected in 32-bit ARGB format.
 * @param file The path to the output JPEG file.
 * @param quality The JPEG quality setting (0-100). Higher values mean better
 *                quality and larger file size. Quality < 60 uses JDCT_IFAST,
 *                >= 90 disables chroma subsampling (4:4:4).
 * @return 1 on success, 0 on failure (e.g., file cannot be opened,
 *         libjpeg error, invalid input).
 */
static int
save_image_jpeg(RGBA_Image *im, const char *file, int quality)
{
   struct jpeg_compress_struct cinfo;
   struct _JPEG_error_mgr jerr;
   FILE               *f;
   DATA8              *buf;
   DATA32             *ptr;
   JSAMPROW           *jbuf;
   int                 y = 0;

   if (!im || !im->image.data || !file)
      return 0;

   buf = alloca(im->cache_entry.w * 3 * sizeof(DATA8));
   f = fopen(file, "wb");
   if (!f)
     {
	return 0;
     }
   memset(&cinfo, 0, sizeof(cinfo));
   cinfo.err = jpeg_std_error(&(jerr.pub));
   jerr.pub.error_exit = _JPEGFatalErrorHandler;
   jerr.pub.emit_message = _JPEGErrorHandler2;
   jerr.pub.output_message = _JPEGErrorHandler;
   if (setjmp(jerr.setjmp_buffer))
     {
	jpeg_destroy_compress(&cinfo);
	fclose(f);
	return 0;
     }
   jpeg_create_compress(&cinfo);
   jpeg_stdio_dest(&cinfo, f);
   cinfo.image_width = im->cache_entry.w;
   cinfo.image_height = im->cache_entry.h;
   cinfo.input_components = 3;
   cinfo.in_color_space = JCS_RGB;
   cinfo.optimize_coding = FALSE;
   cinfo.dct_method = JDCT_ISLOW; // JDCT_FLOAT JDCT_IFAST(quality loss)
   if (quality < 60) cinfo.dct_method = JDCT_IFAST;
   jpeg_set_defaults(&cinfo);
   jpeg_set_quality(&cinfo, quality, TRUE);
   if (quality >= 90)
     {
        cinfo.comp_info[0].h_samp_factor = 1;
        cinfo.comp_info[0].v_samp_factor = 1;
        cinfo.comp_info[1].h_samp_factor = 1;
        cinfo.comp_info[1].v_samp_factor = 1;
        cinfo.comp_info[2].h_samp_factor = 1;
        cinfo.comp_info[2].v_samp_factor = 1;
     }
   jpeg_start_compress(&cinfo, TRUE);
   ptr = im->image.data;
   while (cinfo.next_scanline < cinfo.image_height)
     {
	unsigned int i, j;
	for (j = 0, i = 0; i < im->cache_entry.w; i++)
	  {
	     buf[j++] = ((*ptr) >> 16) & 0xff;
	     buf[j++] = ((*ptr) >> 8) & 0xff;
	     buf[j++] = ((*ptr)) & 0xff;
	     ptr++;
	  }
	jbuf = (JSAMPROW *) (&buf);
	jpeg_write_scanlines(&cinfo, jbuf, 1);
	y++;
     }
   jpeg_finish_compress(&cinfo);
   jpeg_destroy_compress(&cinfo);
   fclose(f);
   return 1;
}

/**
 * @brief Evas image saver function for JPEG format.
 *
 * This function conforms to the Evas_Image_Save_Func interface. It acts as a
 * wrapper around save_image_jpeg, ignoring the unused key, compress, and
 * encoding parameters specific to other formats.
 *
 * @param im The RGBA_Image to save.
 * @param file The output filename.
 * @param key Optional key (unused for JPEG).
 * @param quality JPEG quality (0-100).
 * @param compress Compression level (unused for JPEG).
 * @param encoding Encoding type (unused for JPEG).
 * @return 1 on success, 0 on failure.
 */
static int evas_image_save_file_jpeg(RGBA_Image *im, const char *file, const char *key EINA_UNUSED,
                                     int quality, int compress EINA_UNUSED, const char *encoding EINA_UNUSED)
{
   return save_image_jpeg(im, file, quality);
}

/**
 * @brief Opens the JPEG image saver module.
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
   em->functions = (void *)(&evas_image_save_jpeg_func);
   return 1;
}

/**
 * @brief Closes the JPEG image saver module.
 *
 * Called by Evas when unloading the module. Currently does nothing.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "jpeg",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_SAVER, image_saver, jpeg);

#ifndef EVAS_STATIC_BUILD_JPEG
EVAS_EINA_MODULE_DEFINE(image_saver, jpeg);
#endif
