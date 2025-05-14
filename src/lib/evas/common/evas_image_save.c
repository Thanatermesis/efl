#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "evas_options.h"

#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @brief Saves an RGBA_Image to a file.
 *
 * This function attempts to save the provided RGBA_Image to the specified file.
 * The image format is determined by the file extension. If a suitable saver
 * module is found for the determined format, it will be used to perform the save
 * operation.
 *
 * @param im Pointer to the RGBA_Image structure to save.
 * @param file The path to the file where the image will be saved.
 *             Example: "/path/to/image.png"
 * @param key Optional key for image formats that support it (e.g., EET).
 *            Can be NULL if not used.
 * @param quality The quality setting for the save operation (0-100).
 *                Used by formats like JPEG.
 * @param compress The compression level for the save operation.
 *                 Used by formats like PNG.
 * @param encoding The encoding to use for the image. For example, "lossless"
 *                 for WebP. Can be NULL.
 * @return 1 on success, 0 on failure (e.g., unknown file type,
 *         saver module not found, or save operation failed).
 */
int
evas_common_save_image_to_file(RGBA_Image *im, const char *file, const char *key,
                               int quality, int compress, const char *encoding)
{
   Evas_Image_Save_Func *evas_image_save_func = NULL;
   char *p;
   char *saver = NULL;

   // Determine the image saver type based on the file extension.
   p = strrchr(file, '.');
   if (p)
     {
        p++; // Move past the dot.

        // Common image formats and their corresponding saver identifiers.
        if (!strcasecmp(p, "png"))
          saver = "png";
        if ((!strcasecmp(p, "jpg")) || (!strcasecmp(p, "jpeg")) ||
            (!strcasecmp(p, "jfif")))
          saver = "jpeg"; // "jpeg" saver handles .jpg, .jpeg, .jfif
        if ((!strcasecmp(p, "eet")) || (!strcasecmp(p, "edj")) ||
            (!strcasecmp(p, "eap")))
          saver = "eet"; // "eet" saver handles .eet, .edj, .eap
        if (!strcasecmp(p, "webp"))
          saver = "webp";
        if (!strcasecmp(p, "tgv"))
          saver = "tgv"; // Evas specific lossy format
        if (!strcasecmp(p, "avif"))
          saver = "avif";
        if (!strcasecmp(p, "jxl"))
          saver = "jxl"; // JPEG XL
        if (!strcasecmp(p, "qoi"))
          saver = "qoi"; // Quite OK Image Format
     }

   // If a saver type was determined, try to load and use the corresponding module.
   if (saver)
     {
        Evas_Module *em;

        // Find the module for the determined saver type.
        em = evas_module_find_type(EVAS_MODULE_TYPE_IMAGE_SAVER, saver);
        if (em)
          {
             evas_module_use(em);
             if (evas_module_load(em))
               {
                  evas_image_save_func = em->functions;
                  return evas_image_save_func->image_save(im, file, key, quality,
                                                          compress, encoding);
               }
          }
     }
   return 0;
}
