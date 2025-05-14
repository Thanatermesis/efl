#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <webp/encode.h>

#include "evas_common_private.h"
#include "evas_private.h"

static int evas_image_save_file_webp(RGBA_Image *im, const char *file, const char *key, int quality, int compress, const char *encoding);

static Evas_Image_Save_Func evas_image_save_webp_func =
{
   evas_image_save_file_webp
};

/**
 * @brief Callback function for WebPEncode to write encoded data to a file.
 *
 * This function is registered with the WebPPicture object and is called by
 * WebPEncode to write chunks of the encoded WebP data.
 *
 * @param data Pointer to the encoded data chunk.
 * @param data_size Size of the data chunk.
 * @param pic Pointer to the WebPPicture object (contains custom_ptr pointing to the FILE).
 * @return 1 on success, 0 on failure (fwrite error).
 */
static int writer(const uint8_t *data, size_t data_size, const WebPPicture *const pic)
{
	FILE *out = (FILE *)pic->custom_ptr;
	return data_size ? (fwrite(data, data_size, 1, out) == 1) : 1;
}

/**
 * @brief Saves an RGBA_Image to a WebP file.
 *
 * This function handles the core logic of converting an Evas RGBA_Image
 * to the WebP format and writing it to a file. It sets up the WebP
 * configuration and picture objects, performs the encoding, and manages
 * resources.
 *
 * @param im Pointer to the RGBA_Image to save.
 * @param file Path to the output WebP file.
 * @param quality Quality factor (0-100). 100 implies lossless compression.
 * @return 1 on success, 0 on failure.
 */
static int
save_image_webp(RGBA_Image *im, const char *file, int quality)
{
	WebPPicture picture;
	WebPConfig config;
	int result = 0;

	if (!im || !im->image.data || !file)
		return 0;

	if (!WebPPictureInit(&picture) || !WebPConfigInit(&config))
		return 0;

	picture.width = im->cache_entry.w;
	picture.height = im->cache_entry.h;
	picture.use_argb = 1;
	if (im->cache_entry.flags.alpha)
		picture.colorspace |= WEBP_CSP_ALPHA_BIT;
	else
		picture.colorspace &= ~WEBP_CSP_ALPHA_BIT;

	if (!WebPPictureAlloc(&picture)) // allocates picture.argb
		return 0;
	memcpy(picture.argb, im->image.data, picture.width * picture.height * sizeof(DATA32));
	evas_common_convert_argb_unpremul(picture.argb, picture.width * picture.height);

	if (quality == 100)
		config.lossless = 1;
	else
		config.quality = quality;
	// config.method = 6; // slower, but better quality

	if (!WebPValidateConfig(&config))
		goto free_picture;

	FILE *f = fopen(file, "wb");
	if (f == NULL)
		goto free_picture;

	picture.writer = writer;
	picture.custom_ptr = (void *)f;

	result = WebPEncode(&config, &picture);

	fclose(f);
 free_picture:
	WebPPictureFree(&picture);

	return result;
}

/**
 * @brief Evas image saver function for WebP format.
 *
 * This function conforms to the Evas_Image_Save_Func interface for saving
 * images. It acts as a wrapper around save_image_webp.
 *
 * @param im Pointer to the RGBA_Image to save.
 * @param file Path to the output WebP file.
 * @param key Optional key (unused for WebP).
 * @param quality Quality factor (0-100).
 * @param compress Compression level (unused for WebP, quality is used instead).
 * @param encoding Specific encoding details (unused for WebP).
 * @return 1 on success, 0 on failure.
 */
static int evas_image_save_file_webp(RGBA_Image *im, const char *file, const char *key EINA_UNUSED,
                                     int quality, int compress EINA_UNUSED, const char *encoding EINA_UNUSED)
{
	return save_image_webp(im, file, quality);
}

/**
 * @brief Opens the Evas image saver module for WebP.
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
   em->functions = (void *)(&evas_image_save_webp_func);
   return 1;
}

/**
 * @brief Closes the Evas image saver module for WebP.
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
   "webp",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_SAVER, image_saver, webp);

#ifndef EVAS_STATIC_BUILD_WEBP
EVAS_EINA_MODULE_DEFINE(image_saver, webp);
#endif
