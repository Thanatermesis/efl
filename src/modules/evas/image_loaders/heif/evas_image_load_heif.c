#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libheif/heif.h>

#include "Evas_Loader.h"
#include "evas_common_private.h"

/**
 * @brief Internal state structure for the HEIF image loader.
 *
 * This structure holds the necessary context and handles required
 * during the loading process of a HEIF image file.
 */
typedef struct _Evas_Loader_Internal Evas_Loader_Internal;
struct _Evas_Loader_Internal
{
   Eina_File *f; /**< Eina file handle for the image file */
   Evas_Image_Load_Opts *opts; /**< Image loading options */
   struct heif_context* ctx; /**< libheif context */
   struct heif_image_handle *handle; /**< libheif image handle */
};

static int _evas_loader_heif_log_dom = -1; /**< Eina log domain for this loader */
static Eina_Module *_evas_loader_heif_mod = NULL; /**< Eina module handle for the dynamically loaded libheif */

/**
 * @brief Macro to dynamically load the libheif shared library.
 * @param x Path or name of the shared library file.
 * Tries to load the specified library and stores the handle in _evas_loader_heif_mod.
 */
#define LOAD(x)                                             \
  if (!_evas_loader_heif_mod) {                             \
       if ((_evas_loader_heif_mod = eina_module_new(x))) {  \
            if (!eina_module_load(_evas_loader_heif_mod)) { \
                 eina_module_free(_evas_loader_heif_mod);   \
                 _evas_loader_heif_mod = NULL;              \
              }                                             \
         }                                                  \
    }

#define SYM(x)                                                           \
   if (!(x ## _f = eina_module_symbol_get(_evas_loader_heif_mod, #x))) { \
      ERR("Cannot find symbol '%s' in '%s'",                             \
          #x, eina_module_file_get(_evas_loader_heif_mod));              \
      goto error;                                                        \
   }

/**
 * @brief Function pointer type for heif_check_filetype.
 * Checks if the given data buffer likely contains a HEIF file.
 */
typedef enum heif_filetype_result (*heif_check_filetype_t)(const uint8_t* data,
                                                           int len);
static heif_check_filetype_t heif_check_filetype_f = NULL; /**< Function pointer for heif_check_filetype */

/**
 * @brief Function pointer type for heif_context_alloc.
 * Allocates a new HEIF context.
 */
typedef struct heif_context* (*heif_context_alloc_t)(void);
static heif_context_alloc_t heif_context_alloc_f = NULL; /**< Function pointer for heif_context_alloc */

/**
 * @brief Function pointer type for heif_context_free.
 * Frees a HEIF context.
 */
typedef void (*heif_context_free_t)(struct heif_context*);
static heif_context_free_t heif_context_free_f = NULL; /**< Function pointer for heif_context_free */

/**
 * @brief Function pointer type for heif_context_get_primary_image_handle.
 * Gets the primary image handle from a HEIF context.
 */
typedef struct heif_error (*heif_context_get_primary_image_handle_t)(struct heif_context* ctx,
                                                                     struct heif_image_handle**);
static heif_context_get_primary_image_handle_t heif_context_get_primary_image_handle_f = NULL; /**< Function pointer for heif_context_get_primary_image_handle */

/**
 * @brief Function pointer type for heif_context_read_from_memory_without_copy.
 * Reads HEIF data from a memory buffer without copying it.
 */
typedef struct heif_error (*heif_context_read_from_memory_without_copy_t)(struct heif_context*,
                                                                          const void* mem, size_t size,
                                                                          const struct heif_reading_options*);
static heif_context_read_from_memory_without_copy_t heif_context_read_from_memory_without_copy_f = NULL; /**< Function pointer for heif_context_read_from_memory_without_copy */

/**
 * @brief Function pointer type for heif_decode_image.
 * Decodes a HEIF image handle into an image structure.
 */
typedef struct heif_error (*heif_decode_image_t)(const struct heif_image_handle* in_handle,
                                                 struct heif_image** out_img,
                                                 enum heif_colorspace colorspace,
                                                 enum heif_chroma chroma,
                                                 const struct heif_decoding_options* options);
static heif_decode_image_t heif_decode_image_f = NULL; /**< Function pointer for heif_decode_image */

/**
 * @brief Function pointer type for heif_deinit.
 * Deinitializes the libheif library globally.
 */
typedef void (*heif_deinit_t)();
static heif_deinit_t heif_deinit_f = NULL; /**< Function pointer for heif_deinit */

/**
 * @brief Function pointer type for heif_image_get_plane_readonly.
 * Gets a read-only pointer to a specific color plane of a decoded image.
 */
typedef const uint8_t* (*heif_image_get_plane_readonly_t)(const struct heif_image*,
                                                          enum heif_channel channel,
                                                          int* out_stride);
static heif_image_get_plane_readonly_t heif_image_get_plane_readonly_f = NULL; /**< Function pointer for heif_image_get_plane_readonly */

/**
 * @brief Function pointer type for heif_image_handle_get_height.
 * Gets the height of the image associated with a handle.
 */
typedef int (*heif_image_handle_get_height_t)(const struct heif_image_handle* handle);
static heif_image_handle_get_height_t heif_image_handle_get_height_f = NULL; /**< Function pointer for heif_image_handle_get_height */

/**
 * @brief Function pointer type for heif_image_handle_get_width.
 * Gets the width of the image associated with a handle.
 */
typedef int (*heif_image_handle_get_width_t)(const struct heif_image_handle* handle);
static heif_image_handle_get_width_t heif_image_handle_get_width_f = NULL; /**< Function pointer for heif_image_handle_get_width */

/**
 * @brief Function pointer type for heif_image_handle_has_alpha_channel.
 * Checks if the image associated with a handle has an alpha channel.
 */
typedef int (*heif_image_handle_has_alpha_channel_t)(const struct heif_image_handle*);
static heif_image_handle_has_alpha_channel_t heif_image_handle_has_alpha_channel_f = NULL; /**< Function pointer for heif_image_handle_has_alpha_channel */

/**
 * @brief Function pointer type for heif_init.
 * Initializes the libheif library globally.
 */
typedef struct heif_error (*heif_init_t)(struct heif_init_params*);
static heif_init_t heif_init_f = NULL; /**< Function pointer for heif_init */

/**
 * @brief Function pointer type for heif_image_handle_release.
 * Releases an image handle.
 */
typedef void (*heif_image_handle_release_t)(const struct heif_image_handle*);
static heif_image_handle_release_t heif_image_handle_release_f = NULL; /**< Function pointer for heif_image_handle_release */

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_loader_heif_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_loader_heif_log_dom, __VA_ARGS__) /**< Information logging macro */

/**
 * @brief Initializes HEIF loading and reads image header properties.
 *
 * This function checks if the provided memory map contains a valid HEIF file,
 * allocates a HEIF context, reads the header information, and populates the
 * image properties (width, height, alpha). It stores the context and handle
 * in the loader structure for potential later use (e.g., by data loading).
 *
 * @param loader The internal loader state.
 * @param prop Pointer to the image property structure to fill.
 * @param map Pointer to the memory-mapped file data.
 * @param length Size of the memory-mapped data.
 * @param[out] error Pointer to an integer where the Evas load error code will be stored.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
// Original line below appears to be part of a malformed/missing function signature.
// The function signature static Eina_Bool evas_image_load_file_head_heif_init(...)
// seems to be missing between the Doxygen comment and the function body's opening brace.
                                    int *error)
{
   struct heif_context *ctx;
   struct heif_image_handle *handle;
   struct heif_error err;
   Eina_Bool ret;

   ret = EINA_FALSE;
   prop->w = 0;
   prop->h = 0;
   prop->alpha = EINA_FALSE;

   /* heif file must have a 12 bytes long header */
   if ((length < 12) ||
       (heif_check_filetype_f(map, length) != heif_filetype_yes_supported))
     {
        INF("HEIF header invalid");
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        return ret;
     }

   ctx = heif_context_alloc_f();
   if (!ctx)
     {
        INF("cannot allocate heif_context");
        *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
        return ret;
     }

   err = heif_context_read_from_memory_without_copy_f(ctx, map, length, NULL);
   if (err.code != heif_error_Ok)
     {
        INF("%s", err.message);
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        heif_context_free_f(ctx);
        return ret;
   }

   err = heif_context_get_primary_image_handle_f(ctx, &handle);
   if (err.code != heif_error_Ok)
     {
        INF("%s", err.message);
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        heif_context_free_f(ctx);
        return ret;
     }

   prop->w = heif_image_handle_get_width_f(handle);
   prop->h = heif_image_handle_get_height_f(handle);

   /* if size is invalid, we exit */
   if ((prop->w < 1) || (prop->h < 1) ||
       (prop->w > IMG_MAX_SIZE) || (prop->h > IMG_MAX_SIZE) ||
       IMG_TOO_BIG(prop->w, prop->h))
     {
        if (IMG_TOO_BIG(prop->w, prop->h))
          *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        else
          *error= EVAS_LOAD_ERROR_GENERIC;
        heif_image_handle_release_f(handle);
        heif_context_free_f(ctx);
        return ret;
     }

   prop->alpha = !!heif_image_handle_has_alpha_channel_f(handle);
   loader->ctx = ctx;
   loader->handle = handle;

   *error = EVAS_LOAD_ERROR_NONE;
   ret = EINA_TRUE;

   return ret;
}

/**
 * @brief Reads HEIF image header properties and immediately cleans up.
 *
 * This function calls evas_image_load_file_head_heif_init to read the header
 * but immediately releases the HEIF context and handle afterwards. It's used
 * when only the header information is needed, not the full image data.
 *
 * @param loader The internal loader state.
 * @param prop Pointer to the image property structure to fill.
 * @param map Pointer to the memory-mapped file data.
 * @param length Size of the memory-mapped data.
 * @param[out] error Pointer to an integer where the Evas load error code will be stored.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_head_heif_internal(Evas_Loader_Internal *loader,
                                        Emile_Image_Property *prop,
                                        void *map, size_t length,
                                        int *error)
{
   /* Initialize and read header */
   if (!evas_image_load_file_head_heif_init(loader, prop, map, length, error))
     return EINA_FALSE;

   /* Clean up immediately as we only needed the header */
   heif_image_handle_release_f(loader->handle);
   heif_context_free_f(loader->ctx);
   loader->handle = NULL;
   loader->ctx = NULL;

   return EINA_TRUE;
}

/**
 * @brief Reads HEIF header and decodes image data into a pixel buffer.
 *
 * This function first calls evas_image_load_file_head_heif_init to read the
 * header and initialize the context. Then, it decodes the HEIF image data
 * into the provided pixel buffer, converting it to Evas's expected BGRA or
 * BGR format.
 *
 * @param loader The internal loader state (must have context and handle initialized).
 * @param prop Pointer to the image property structure (already filled by head).
 * @param pixels Pointer to the destination pixel buffer (BGRA format).
 * @param map Pointer to the memory-mapped file data.
 * @param length Size of the memory-mapped data.
 * @param[out] error Pointer to an integer where the Evas load error code will be stored.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_data_heif_internal(Evas_Loader_Internal *loader,
                                        Emile_Image_Property *prop,
                                        void *pixels,
                                        void *map, size_t length,
                                        int *error)
{
   struct heif_image *img = NULL; /* Decoded HEIF image structure */
   struct heif_error err;
   const unsigned char *data;
   unsigned char *dd;
   unsigned char *plane;
   int stride;
   unsigned int x;
   unsigned int y;
   Eina_Bool ret = EINA_FALSE;

   /* Initialize and read header first */
   if (!evas_image_load_file_head_heif_init(loader, prop, map, length, error))
     return ret;

   /* Decode the image to RGB or RGBA interleaved format */
   err = heif_decode_image_f(loader->handle, &img, heif_colorspace_RGB,
                             prop->alpha ? heif_chroma_interleaved_RGBA
                                         : heif_chroma_interleaved_RGB,
                             NULL);

   if (err.code != heif_error_Ok)
     {
        INF("%s", err.message);
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        goto on_error;
     }

   data = heif_image_get_plane_readonly_f(img, heif_channel_interleaved, &stride);
   if (!data)
     {
        INF("Failed to get image plane data");
        *error = EVAS_LOAD_ERROR_CORRUPT_FILE; // Or generic error
        goto on_error;
     }

   dd = (unsigned char *)pixels; /* Destination buffer (BGRA) */
   plane = (unsigned char *)data; /* Source buffer (RGB or RGBA) */

   /* Copy pixel data, converting RGB/RGBA to BGRA */
   if (!prop->alpha) /* Source is RGB */
     {
       for (y = 0; y < prop->h; y++, plane += stride)
          {
             int from = 0;
             for (x = 0; x < prop->w; x++, from += 3)
               {
                  dd[0] = *(plane + from + 2);
                  dd[1] = *(plane + from + 1);
                  dd[2] = *(plane + from + 0);
                  dd[3] = 0xff;
                  dd[3] = 0xff; /* Set alpha to opaque */
                  dd += 4;
               }
          }
     }
   else /* Source is RGBA */
     {
        for (y = 0; y < prop->h; y++, plane += stride)
          {
             int from = 0;
             for (x = 0; x < prop->w; x++, from += 4)
               {
                  dd[0] = *(plane + from + 2);
                  dd[1] = *(plane + from + 1);
                  dd[2] = *(plane + from + 0);
                  dd[3] = *(plane + from + 3);
                  dd[3] = *(plane + from + 3); /* Copy alpha */
                  dd += 4;
               }
          }
     }

   ret = EINA_TRUE;
   *error = EVAS_LOAD_ERROR_NONE;
   prop->premul = EINA_TRUE; /* Assume data is premultiplied (libheif doesn't specify) */

 on_error:
   /* Note: context and handle are released by evas_image_load_file_close_heif */
   /* We might need to release 'img' here if heif_decode_image succeeded but plane access failed */
   /* TODO: Check libheif documentation for heif_image release requirements */
   return ret;
}

/**
 * @brief Evas loader function to open a HEIF file.
 *
 * Allocates and initializes the internal loader state structure.
 *
 * @param f Eina file handle.
 * @param key Optional key (unused).
 * @param opts Load options.
 * @param animated Animated properties (unused for HEIF).
 * @param[out] error Pointer to store load error code.
 * @return Pointer to the allocated loader state, or NULL on error.
 */
static void *
evas_image_load_file_open_heif(Eina_File *f, Eina_Stringshare *key EINA_UNUSED,
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
 * @brief Evas loader function to close a HEIF file.
 *
 * Frees the internal loader state and any associated libheif resources.
 *
 * @param loader_data Pointer to the loader state allocated by open.
 */
static void
evas_image_load_file_close_heif(void *loader_data)
{
   Evas_Loader_Internal *loader = loader_data;

   if (!loader) return;
   if (loader->handle)
     heif_image_handle_release_f(loader->handle);
   if (loader->ctx)
     heif_context_free_f(loader->ctx);
   free(loader);
}

/**
 * @brief Evas loader function to read the header of a HEIF file.
 *
 * Memory maps the file and calls the internal header reading function.
 *
 * @param loader_data Pointer to the loader state.
 * @param prop Pointer to the image property structure to fill.
 * @param[out] error Pointer to store load error code.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_head_heif(void *loader_data,
                               Evas_Image_Property *prop,
                               int *error)
{
   Evas_Loader_Internal *loader = loader_data;
   Eina_File *f = loader->f;
   void *map;
   Eina_Bool val;

   f = loader->f;

   map = eina_file_map_all(f, EINA_FILE_RANDOM);
   if (!map)
     {
	*error = EVAS_LOAD_ERROR_DOES_NOT_EXIST;
        return EINA_FALSE;
     }

   val = evas_image_load_file_head_heif_internal(loader,
                                                 (Emile_Image_Property *)prop,
                                                 map, eina_file_size_get(f),
                                                 error);

   eina_file_map_free(f, map);

   return val;
}

/**
 * @brief Evas loader function to read the image data of a HEIF file.
 *
 * Memory maps the file and calls the internal data loading and decoding function.
 *
 * @param loader_data Pointer to the loader state.
 * @param prop Pointer to the image property structure (already filled by head).
 * @param pixels Pointer to the destination pixel buffer.
 * @param[out] error Pointer to store load error code.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_data_heif(void *loader_data,
                               Evas_Image_Property *prop,
			       void *pixels,
			       int *error)
{
   Evas_Loader_Internal *loader = loader_data;
   Eina_File *f = loader->f;
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

   val = evas_image_load_file_data_heif_internal(loader,
                                                 (Emile_Image_Property *)prop,
                                                 pixels,
                                                 map, eina_file_size_get(f),
                                                 error);

   eina_file_map_free(f, map);

 on_error:
   return val;
}

/**
 * @brief Structure defining the HEIF loader functions for Evas.
 */
static const Evas_Image_Load_Func evas_image_load_heif_func = {
   EVAS_IMAGE_LOAD_VERSION, /**< Loader API version */
   evas_image_load_file_open_heif, /**< Open function */
   evas_image_load_file_close_heif, /**< Close function */
   evas_image_load_file_head_heif, /**< Head function */
   NULL, /**< Head sequence function (unused) */
   evas_image_load_file_data_heif, /**< Data function */
   NULL, /**< Data sequence function (unused) */
   EINA_TRUE, /**< Does thread */
   EINA_FALSE /**< Does sequence (unused) */
};

/**
 * @brief Evas module initialization function.
 *
 * Called when the Evas HEIF loader module is loaded. It registers the log
 * domain, dynamically loads the libheif shared library, resolves required
 * function symbols, and optionally initializes libheif.
 *
 * @param em The Evas module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;

   _evas_loader_heif_log_dom = eina_log_domain_register("evas-heif", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_loader_heif_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }

   em->functions = (void *)(&evas_image_load_heif_func);

#if defined (_WIN32)
   LOAD("libheif-1.dll");
   LOAD("libheif.dll");
#elif defined (_CYGWIN__)
   LOAD("cygheif-1.dll");
#elif defined(__APPLE__) && defined(__MACH__)
   LOAD("libheif.1.dylib");
#else
   LOAD("libheif.so.1");
#endif

   if (!_evas_loader_heif_mod)
     {
        EINA_LOG_ERR("Can not open libheif shared library.");
        goto error;
     }

   SYM(heif_check_filetype);
   SYM(heif_context_alloc);
   SYM(heif_context_free);
   SYM(heif_context_get_primary_image_handle);
   SYM(heif_context_read_from_memory_without_copy);
   SYM(heif_decode_image);
   SYM(heif_image_get_plane_readonly);
   SYM(heif_image_handle_get_height);
   SYM(heif_image_handle_get_width);
   SYM(heif_image_handle_has_alpha_channel);
   SYM(heif_image_handle_release);

   heif_init_f = eina_module_symbol_get(_evas_loader_heif_mod, "heif_init");
   heif_deinit_f = eina_module_symbol_get(_evas_loader_heif_mod, "heif_deinit");

   if (heif_init_f)
     {
        heif_init_f(NULL);
     }

   return 1;

 error:
   eina_log_domain_unregister(_evas_loader_heif_log_dom);
   _evas_loader_heif_log_dom = -1;
   eina_log_domain_unregister(_evas_loader_heif_log_dom);
   _evas_loader_heif_log_dom = -1;
   if (_evas_loader_heif_mod)
     {
        eina_module_free(_evas_loader_heif_mod);
        _evas_loader_heif_mod = NULL;
     }
   return 0;
}

/**
 * @brief Evas module shutdown function.
 *
 * Called when the Evas HEIF loader module is unloaded. It deinitializes
 * libheif (if initialized), unloads the shared library module, and
 * unregisters the log domain.
 *
 * @param em The Evas module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   /* Deinitialize libheif if the function was found and called */
   if (heif_deinit_f)
     heif_deinit_f();

   if (_evas_loader_heif_mod)
     eina_module_free(_evas_loader_heif_mod);

   if (_evas_loader_heif_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_loader_heif_log_dom);
        _evas_loader_heif_log_dom = -1;
     }
}

/**
 * @brief Evas module API structure.
 * Defines the module type, name, and open/close functions.
 */
static Evas_Module_Api evas_modapi =
  {
    EVAS_MODULE_API_VERSION, /**< Evas API version */
    "heif", /**< Module name */
    "none",
    {
      module_open,
      module_close
    }
  };

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, heif);


#ifndef EVAS_STATIC_BUILD_HEIF
EVAS_EINA_MODULE_DEFINE(image_loader, heif);
#endif
