#ifdef HAVE_CONFIG_H
# include "config.h"  /* so that EMODAPI in Eet.h is correctly defined */
#endif

#include <Eet.h>

#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @internal
 * @brief Internal structure to hold loader-specific data for EET images.
 *
 * This structure maintains the state required during the loading process
 * of an image stored within an Eet file. It holds a reference to the
 * memory-mapped Eet file and the specific key identifying the image data
 * within that file.
 */
typedef struct _Evas_Loader_Internal Evas_Loader_Internal;
struct _Evas_Loader_Internal
{
   Eet_File *ef; /**< Pointer to the memory-mapped Eet file handle. */
   const char *key; /**< Stringshare key identifying the image entry within the Eet file. */
};

/**
 * @internal
 * @brief Opens an Eet file for image loading.
 *
 * This function is called by Evas to open an image file identified as an
 * Eet file. It memory-maps the file and prepares an internal loader
 * structure.
 *
 * @param f Eina_File handle representing the file to open.
 * @param key The key (entry name) within the Eet file that identifies the image data.
 * @param opts Load options (unused in this loader).
 * @param animated Pointer to store animated properties (unused in this loader).
 * @param error Pointer to an integer where the load error code will be stored.
 * @return A pointer to the internal loader data structure on success, NULL on failure.
 *         On failure, the error code is stored in @p error.
 */
static void *
evas_image_load_file_open_eet(Eina_File *f, Eina_Stringshare *key,
                              Evas_Image_Load_Opts *opts EINA_UNUSED,
                              Evas_Image_Animated *animated EINA_UNUSED,
                              int *error)
{
   Evas_Loader_Internal *loader;

   if (!key)
     {
	*error = EVAS_LOAD_ERROR_DOES_NOT_EXIST;
	return NULL;
     }

   loader = calloc(1, sizeof (Evas_Loader_Internal));
   if (!loader)
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return NULL;
     }

   loader->ef = eet_mmap(f);
   if (!loader->ef)
     {
        free(loader);
        *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
        return NULL;
     }

   loader->key = eina_stringshare_ref(key);
   eina_stringshare_ref(key);
   return loader;
}

/**
 * @internal
 * @brief Closes an Eet file previously opened for image loading.
 *
 * This function is called by Evas to release resources associated with
 * an opened Eet image file. It closes the Eet file handle, releases the
 * stringshare key, and frees the internal loader structure.
 *
 * @param loader_data Pointer to the internal loader data structure created by
 *                    evas_image_load_file_open_eet().
 */
static void
evas_image_load_file_close_eet(void *loader_data)
{
   Evas_Loader_Internal *loader = loader_data;

   eet_close(loader->ef);
   eina_stringshare_del(loader->key);
   free(loader);
}

/**
 * @internal
 * @brief Helper function to set the error code and return EINA_FALSE.
 *
 * Simplifies error handling by setting the error code pointed to by @p error
 * and returning EINA_FALSE, typically used in functions returning Eina_Bool.
 *
 * @param err The error code (Evas_Load_Error) to set.
 * @param error Pointer to the integer where the error code should be stored.
 * @return Always returns EINA_FALSE.
 */
static inline Eina_Bool
_evas_image_load_return_error(int err, int *error)
{
   *error = err;
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Rounds up an integer value to the nearest multiple of another integer.
 *
 * Calculates the smallest multiple of @p rup that is greater than or equal to @p val.
 * Used for calculating padding required for certain texture formats (like ETC1).
 *
 * @param val The value to round up. Must be non-negative.
 * @param rup The multiple to round up to. Must be positive.
 * @return The rounded-up value, or 0 if input constraints are not met.
 * @note Example: _roundup(10, 4) returns 12. _roundup(8, 4) returns 8.
 */
static int
_roundup(int val, int rup)
{
   // Formula: roundup(val, rup) = val + (rup - (val % rup)) % rup
   // Simplified using integer arithmetic: (val + rup - 1) / rup * rup
   // Further simplified to avoid potential division by zero if rup were 0 (though guarded):
   if (val >= 0 && rup > 0)
     return (val + rup - 1) - ((val + rup - 1) % rup);
   return 0;
}

/**
 * @internal
 * @brief Preferred colorspace loading order for ETC1 encoded images.
 *
 * Defines the sequence of colorspaces Evas should attempt to use when
 * loading an image encoded with ETC1 (without alpha). The loader prefers
 * native ETC1, falls back to ETC2 RGB (if hardware supports it), and finally
 * decodes to raw ARGB8888.
 */
static const Evas_Colorspace cspaces_etc1[] = {
  EVAS_COLORSPACE_ETC1,         /**< Preferred: Native ETC1 */
  EVAS_COLORSPACE_RGB8_ETC2,    /**< Fallback 1: ETC2 RGB (superset) */
  EVAS_COLORSPACE_ARGB8888      /**< Fallback 2: Raw pixel data */
};

/**
 * @internal
 * @brief Preferred colorspace loading order for ETC1 encoded images with alpha.
 *
 * Defines the sequence of colorspaces Evas should attempt to use when
 * loading an image encoded with ETC1 and a separate alpha channel.
 * The loader prefers the combined ETC1_ALPHA format or falls back to raw ARGB8888.
 */
static const Evas_Colorspace cspaces_etc1_alpha[] = {
  EVAS_COLORSPACE_ETC1_ALPHA,   /**< Preferred: Native ETC1 + Alpha */
  EVAS_COLORSPACE_ARGB8888      /**< Fallback: Raw pixel data */
};

/**
 * @internal
 * @brief Preferred colorspace loading order for ETC2 RGB encoded images.
 *
 * Defines the sequence of colorspaces Evas should attempt to use when
 * loading an image encoded with ETC2 RGB (no alpha). The loader prefers
 * native ETC2 RGB or falls back to raw ARGB8888.
 */
static const Evas_Colorspace cspaces_etc2_rgb[] = {
  EVAS_COLORSPACE_RGB8_ETC2,    /**< Preferred: Native ETC2 RGB */
  EVAS_COLORSPACE_ARGB8888      /**< Fallback: Raw pixel data */
};

/**
 * @internal
 * @brief Preferred colorspace loading order for ETC2 RGBA encoded images.
 *
 * Defines the sequence of colorspaces Evas should attempt to use when
 * loading an image encoded with ETC2 RGBA (EAC). The loader prefers
 * native ETC2 RGBA or falls back to raw ARGB8888.
 */
static const Evas_Colorspace cspaces_etc2_rgba[] = {
  EVAS_COLORSPACE_RGBA8_ETC2_EAC, /**< Preferred: Native ETC2 RGBA */
  EVAS_COLORSPACE_ARGB8888        /**< Fallback: Raw pixel data */
};

/**
 * @internal
 * @brief Reads the header information of an Eet image entry.
 *
 * This function is called by Evas to retrieve metadata about the image
 * (dimensions, alpha presence, etc.) without loading the actual pixel data.
 * It also determines the preferred colorspace loading order based on the
 * encoding found in the Eet header.
 *
 * @param loader_data Pointer to the internal loader data structure.
 * @param prop Pointer to an Emile_Image_Property structure to be filled with
 *             image properties (width, height, alpha, colorspaces, borders).
 * @param error Pointer to an integer where the load error code will be stored.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *         On failure, the error code is stored in @p error.
 */
static Eina_Bool
evas_image_load_file_head_eet(void *loader_data,
			      Emile_Image_Property *prop,
			      int *error)
{
   Evas_Loader_Internal *loader = loader_data;
   int       a, compression, quality;
   Eet_Image_Encoding lossy;
   const Eet_Colorspace *cspaces = NULL;
   Eina_Bool border_set = EINA_FALSE;
   int       ok;

   ok = eet_data_image_header_read(loader->ef, loader->key,
                                   &prop->w, &prop->h, &a, &compression, &quality, &lossy);
   if (!ok)
     return _evas_image_load_return_error(EVAS_LOAD_ERROR_DOES_NOT_EXIST, error);
   if (IMG_TOO_BIG(prop->w, prop->h))
     return _evas_image_load_return_error(EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED, error);

   if (eet_data_image_colorspace_get(loader->ef, loader->key, NULL, &cspaces))
     {
        unsigned int i;

	if (cspaces != NULL)
	  {
	    for (i = 0; cspaces[i] != EET_COLORSPACE_ARGB8888; i++)
              {
                 if (cspaces[i] == EET_COLORSPACE_ETC1)
                   {
                      prop->cspaces = cspaces_etc1;
                      border_set = EINA_TRUE;
                      break;
                   }
                 else if (cspaces[i] == EET_COLORSPACE_ETC1_ALPHA)
                   {
                      prop->cspaces = cspaces_etc1_alpha;
                      border_set = EINA_TRUE;
                      break;
                   }
                 else if (cspaces[i] == EET_COLORSPACE_RGB8_ETC2)
                   {
                      prop->cspaces = cspaces_etc2_rgb;
                      border_set = EINA_TRUE;
                      break;
                   }
                 else if (cspaces[i] == EET_COLORSPACE_RGBA8_ETC2_EAC)
                   {
                      prop->cspaces = cspaces_etc2_rgba;
                      border_set = EINA_TRUE;
                      break;
                   }
              }
	  }
     }

   prop->alpha = !!a;
   if (border_set)
     {
        prop->borders.l = 1;
        prop->borders.t = 1;
        prop->borders.r = _roundup(prop->w + 2, 4) - prop->w - 1;
        prop->borders.b = _roundup(prop->h + 2, 4) - prop->h - 1;
     }
   *error = EVAS_LOAD_ERROR_NONE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Loads the actual pixel data of an Eet image entry.
 *
 * This function is called by Evas to load the image pixel data into the
 * provided buffer (@p pixels). It uses the colorspace specified in @p prop
 * (which should be one of the supported ones determined during the header read)
 * to request the data from the Eet library. If the image has alpha and is
 * loaded as ARGB8888, it performs post-load checks for alpha sparsity and
 * ensures pre-multiplication (though the comment suggests premultiplication
 * might already be handled depending on build configuration).
 *
 * @param loader_data Pointer to the internal loader data structure.
 * @param prop Pointer to an Emile_Image_Property structure containing image
 *             properties, including the target colorspace (`prop->cspace`).
 * @param pixels Pointer to the memory buffer where the decoded pixel data
 *               should be stored. The buffer must be large enough to hold
 *               `prop->w * prop->h * bytes_per_pixel` bytes, where
 *               bytes_per_pixel depends on `prop->cspace`.
 * @param error Pointer to an integer where the load error code will be stored.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *         On failure, the error code is stored in @p error.
 */
Eina_Bool
evas_image_load_file_data_eet(void *loader_data,
                              Emile_Image_Property *prop,
                              void *pixels,
			      int *error)
{
   Evas_Loader_Internal *loader = loader_data;
   int       alpha, compression, quality, ok;
   Eet_Image_Encoding lossy;
   DATA32   *body, *p, *end;
   DATA32    nas = 0;
   Eet_Colorspace cspace;

   switch (prop->cspace)
     {
      case EVAS_COLORSPACE_ETC1: cspace = EET_COLORSPACE_ETC1; break;
      case EVAS_COLORSPACE_ETC1_ALPHA: cspace = EET_COLORSPACE_ETC1_ALPHA; break;
      case EVAS_COLORSPACE_RGB8_ETC2: cspace = EET_COLORSPACE_RGB8_ETC2; break;
      case EVAS_COLORSPACE_RGBA8_ETC2_EAC: cspace = EET_COLORSPACE_RGBA8_ETC2_EAC; break;
      case EVAS_COLORSPACE_ARGB8888: cspace = EET_COLORSPACE_ARGB8888; break;
      default:
         return _evas_image_load_return_error(EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED, error);
     }

   ok = eet_data_image_read_to_cspace_surface_cipher(loader->ef, loader->key, NULL, 0, 0,
                                                     pixels, prop->w, prop->h, prop->w * 4,
                                                     cspace,
                                                     &alpha, &compression, &quality, &lossy);
   if (!ok)
     return _evas_image_load_return_error(EVAS_LOAD_ERROR_GENERIC, error);

   if (alpha)
     {
        prop->alpha = 1;
	body = pixels;

	end = body + (prop->w * prop->h);
	for (p = body; p < end; p++)
	  {
	     DATA32 r, g, b, a;

	     a = A_VAL(p);
	     r = R_VAL(p);
	     g = G_VAL(p);
	     b = B_VAL(p);
	     if ((a == 0) || (a == 255)) nas++;
	     if (r > a) r = a;
	     if (g > a) g = a;
	     if (b > a) b = a;
	     *p = ARGB_JOIN(a, r, g, b);
	  }
	if ((ALPHA_SPARSE_INV_FRACTION * nas) >= (prop->w * prop->h))
	  prop->alpha_sparse = 1;
     }
// result is already premultiplied now if u compile with edje
//   evas_common_image_premul(im);
   *error = EVAS_LOAD_ERROR_NONE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Structure defining the Eet image loader functions for Evas.
 *
 * This structure maps the Evas image loader API functions to the
 * specific implementations provided in this file for handling Eet images.
 */
Evas_Image_Load_Func evas_image_load_eet_func =
{
  EVAS_IMAGE_LOAD_VERSION, /**< Loader API version compatibility check. */
  evas_image_load_file_open_eet, /**< Function to open the image file. */
  evas_image_load_file_close_eet, /**< Function to close the image file. */
  (void*) evas_image_load_file_head_eet, /**< Function to read image header. Cast to void* for API compatibility. */
  NULL, /**< Function to read image header from data (not implemented). */
  (void*) evas_image_load_file_data_eet, /**< Function to read image pixel data. Cast to void* for API compatibility. */
  NULL, /**< Function to read image data frame by frame (for animations, not implemented). */
  EINA_TRUE, /**< Indicates the loader supports loading from Eina_File. */
  EINA_FALSE /**< Indicates the loader does not support loading from raw memory directly. */
};

/**
 * @internal
 * @brief Evas module initialization function.
 *
 * Called by Evas when loading this module. It registers the loader functions.
 *
 * @param em Pointer to the Evas_Module structure for this module.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_image_load_eet_func);
   return 1;
}

/**
 * @internal
 * @brief Evas module shutdown function.
 *
 * Called by Evas when unloading this module. Currently does nothing.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Evas module API structure.
 *
 * Provides metadata about the module, including its API version, name,
 * and the open/close functions.
 */
static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION, /**< Evas module API version. */
   "eet",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, eet);

#ifndef EVAS_STATIC_BUILD_EET
EVAS_EINA_MODULE_DEFINE(image_loader, eet);
#endif
