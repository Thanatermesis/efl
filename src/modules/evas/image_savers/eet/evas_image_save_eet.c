#ifdef HAVE_CONFIG_H
# include "config.h"  /* so that EMODAPI in Eet.h is correctly defined */
#endif

#include <Eet.h>

#include "evas_common_private.h"
#include "evas_private.h"

static int evas_image_save_file_eet(RGBA_Image *im, const char *file, const char *key, int quality, int compress, const char *encoding);

static Evas_Image_Save_Func evas_image_save_eet_func =
{
   evas_image_save_file_eet
};

/**
 * @brief Saves an RGBA image to an EET file.
 *
 * This function takes an RGBA_Image structure and saves its pixel data
 * into an EET (Eina Data Archive) file. It handles opening/creating the
 * EET file, writing the image data with specified compression and quality
 * settings, and closing the file.
 *
 * @param im Pointer to the RGBA_Image structure containing the image data.
 *           Example: A valid RGBA_Image pointer obtained from Evas.
 * @param file The path to the EET file to save to. Example: "/path/to/output.eet"
 * @param key The key under which to store the image data within the EET file. Example: "images/my_background"
 * @param quality The quality setting for lossy compression (0-100). Ignored if lossy is 0. Example: 85
 * @param compress The compression level (0-9). 0 means no compression. Example: 6
 * @param encoding The desired encoding (currently unused).
 * @return 1 on success, 0 on failure.
 */
static int
evas_image_save_file_eet(RGBA_Image *im, const char *file, const char *key,
                         int quality, int compress, const char *encoding EINA_UNUSED)
{
   Eet_File            *ef;
   int alpha = 0, lossy = 0, ok = 0;
   DATA32   *data;

   if (!im || !im->image.data || !file)
      return 0;

   ef = eet_open((char *)file, EET_FILE_MODE_READ_WRITE);
   if (!ef) ef = eet_open((char *)file, EET_FILE_MODE_WRITE);
   if (!ef) return 0;
   if ((quality <= 100) || (compress < 0)) lossy = 1;
   if (im->cache_entry.flags.alpha) alpha = 1;
//   if (alpha)
//     {
//       data = malloc(im->image->w * im->image->h * sizeof(DATA32));
//       if (!data)
//	 {
//	   eet_close(ef);
//	   return 0;
//	 }
//       memcpy(data, im->image->data, im->image->w * im->image->h * sizeof(DATA32));
//       evas_common_convert_argb_unpremul(data, im->image->w * im->image->h);
//     }
//   else
       data = im->image.data;
   ok = eet_data_image_write(ef, (char *)key, data,
			     im->cache_entry.w, im->cache_entry.h, alpha, compress,
			     quality, lossy);
//   if (alpha)
//     free(data);
   eet_close(ef);
   return ok;
}

/**
 * @brief Opens the Evas image saver module.
 *
 * This function is called when the Evas module system loads this
 * image saver module. It registers the module's save function.
 *
 * @param em Pointer to the Evas_Module structure to initialize.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_image_save_eet_func);
   return 1;
}

/**
 * @brief Closes the Evas image saver module.
 *
 * This function is called when the Evas module system unloads this
 * image saver module. Currently, it performs no specific cleanup for this module.
 *
 * @param em Pointer to the Evas_Module structure (unused in this function).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
}

/**
 * @brief Module API structure for the EET image saver.
 *
 * Defines the API version and the open/close function pointers
 * for interaction with the Evas module system.
 */
static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION, /*< The Evas module API version */
   "eet",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_SAVER, image_saver, eet);

#ifndef EVAS_STATIC_BUILD_EET
EVAS_EINA_MODULE_DEFINE(image_saver, eet);
#endif

