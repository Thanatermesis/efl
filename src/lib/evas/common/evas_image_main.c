#ifdef HAVE_CONFIG_H
# include "config.h"  /* so that EVAS_API in Eet.h is correctly defined */
#endif

#ifdef _WIN32
# include <evil_private.h> /* mmap */
#else
# include <sys/mman.h>
#endif

#ifdef BUILD_LOADER_EET
# include <Eet.h>
#endif

#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_image_private.h"
#include "evas_convert_yuv.h"
//#include "evas_cs.h"

#ifdef HAVE_VALGRIND
# include <valgrind.h>
# include <memcheck.h>
#endif

//#define SURFDBG 1

/** @internal Global image cache instance. */
static Evas_Cache_Image *eci = NULL;
/** @internal Reference count for the Evas image system. */
static int reference = 0;
/** @internal Flag to disable mmap usage for image surfaces. -1: uninitialized, 0: mmap enabled, 1: mmap disabled. */
static int evas_image_no_mmap = -1;

/* static RGBA_Image *evas_rgba_line_buffer = NULL; */

/** @internal Minimum length for a reusable RGBA line buffer. */
#define EVAS_RGBA_LINE_BUFFER_MIN_LEN 256
/** @internal Maximum length for a reusable RGBA line buffer. */
#define EVAS_RGBA_LINE_BUFFER_MAX_LEN 2048

/* static RGBA_Image *evas_alpha_line_buffer = NULL; */

/** @internal Minimum length for a reusable Alpha line buffer. */
#define EVAS_ALPHA_LINE_BUFFER_MIN_LEN 256
/** @internal Maximum length for a reusable Alpha line buffer. */
#define EVAS_ALPHA_LINE_BUFFER_MAX_LEN 2048

/**
 * @internal
 * @brief Creates a new, empty RGBA_Image structure (Image_Entry).
 * @return A pointer to the newly allocated Image_Entry, or NULL on failure.
 */
static Image_Entry *_evas_common_rgba_image_new(void);
/**
 * @internal
 * @brief Deletes an RGBA_Image structure (Image_Entry) and associated resources.
 * @param ie The Image_Entry to delete.
 */
static void _evas_common_rgba_image_delete(Image_Entry *ie);
/**
 * @internal
 * @brief Allocates the pixel surface for an Image_Entry.
 * @param ie The Image_Entry for which to allocate the surface.
 * @param w The width of the surface to allocate.
 * @param h The height of the surface to allocate.
 * @return 0 on success, -1 on failure.
 */
static int _evas_common_rgba_image_surface_alloc(Image_Entry *ie,
                                                 unsigned int w,
                                                 unsigned int h);
/**
 * @internal
 * @brief Deletes the pixel surface of an Image_Entry.
 * @param ie The Image_Entry whose surface is to be deleted.
 */
static void _evas_common_rgba_image_surface_delete(Image_Entry *ie);
/**
 * @internal
 * @brief Retrieves a pointer to the pixel data of an Image_Entry's surface.
 * @param ie The Image_Entry.
 * @return A pointer to the pixel data (DATA32*), or NULL if no surface is allocated.
 */
static DATA32 *_evas_common_rgba_image_surface_pixels(Image_Entry *ie);
/**
 * @internal
 * @brief Unloads an image, potentially deferring the actual unload operation.
 * @param im The Image_Entry to unload.
 */
static void _evas_common_rgba_image_unload(Image_Entry *im);
/**
 * @internal
 * @brief Marks a rectangular region of an image as dirty.
 * @param im The Image_Entry to mark.
 * @param x The x-coordinate of the dirty region.
 * @param y The y-coordinate of the dirty region.
 * @param w The width of the dirty region.
 * @param h The height of the dirty region.
 */
static void _evas_common_rgba_image_dirty_region(Image_Entry *im,
                                                 unsigned int x,
                                                 unsigned int y,
                                                 unsigned int w,
                                                 unsigned int h);
/**
 * @internal
 * @brief Calculates the estimated RAM usage of an Image_Entry.
 * @param ie The Image_Entry.
 * @return The estimated RAM usage in bytes.
 */
static int _evas_common_rgba_image_ram_usage(Image_Entry *ie);

/**
 * @internal
 * @brief Handles the dirty state when an image is duplicated.
 * This function is called when `ie_dst` is a new copy of `ie_src` and `ie_src`
 * might have pending changes or is loaded. It ensures `ie_dst` gets a
 * properly allocated surface if needed and normalizes colorspaces.
 * @param dst The destination Image_Entry (the new copy).
 * @param src The source Image_Entry (the original).
 * @return 0 on success, 1 on failure (e.g., surface allocation failed).
 * @note Only called when references > 0.
 * @note The destination surface might exist but without allocated pixel data.
 */
static int _evas_common_rgba_image_dirty(Image_Entry* dst,
                                         const Image_Entry* src);

/** @internal Structure defining the Evas image cache callbacks. */
static const Evas_Cache_Image_Func _evas_common_image_func =
{
  _evas_common_rgba_image_new,
  _evas_common_rgba_image_delete,
  _evas_common_rgba_image_surface_alloc,
  _evas_common_rgba_image_surface_delete,
  _evas_common_rgba_image_surface_pixels,
  evas_common_load_rgba_image_module_from_file,
  _evas_common_rgba_image_unload,
  _evas_common_rgba_image_dirty_region,
  _evas_common_rgba_image_dirty,
  evas_common_rgba_image_size_set,
  evas_common_rgba_image_from_copied_data,
  evas_common_rgba_image_from_data,
  evas_common_rgba_image_colorspace_set,
  evas_common_load_rgba_image_data_from_file,
  _evas_common_rgba_image_ram_usage,
  NULL // _evas_common_rgba_image_debug
};

/**
 * @internal
 * @brief Calculates the memory size required for an image surface, potentially aligned to page boundaries.
 *
 * This function determines the byte size needed for an image with given dimensions
 * and colorspace. It also considers mmap usage and page alignment for larger images.
 * For compressed formats, it adjusts width and height based on border parameters
 * and ensures dimensions are multiples of block sizes.
 *
 * @param w The width of the image in pixels.
 * @param h The height of the image in pixels.
 * @param cspace The colorspace of the image.
 * @param[in,out] l Pointer to the left border size. If non-NULL and the colorspace
 *                  is compressed, this value is added to `w` before size calculation.
 *                  If the colorspace is not compressed, this is set to 0.
 * @param[in,out] r Pointer to the right border size. Similar to `l`.
 * @param[in,out] t Pointer to the top border size. Similar to `l`, added to `h`.
 * @param[in,out] b Pointer to the bottom border size. Similar to `l`, added to `h`.
 * @return The calculated size in bytes for the image surface, or 0 if dimensions
 *         are invalid for compressed formats.
 */
EVAS_API int
_evas_common_rgba_image_surface_size(unsigned int w, unsigned int h,
                                     Evas_Colorspace cspace,
                                     /*inout*/int *l, int *r, int *t, int *b)
{
#ifndef PAGE_SIZE
# define PAGE_SIZE (4 * 1024)
#endif
#define HUGE_PAGE_SIZE (2 * 1024 * 1024)
#if defined (HAVE_SYS_MMAN_H) && (!defined (_WIN32))
# define ALIGN_TO_PAGE(Siz) \
   (((Siz / PAGE_SIZE) + (Siz % PAGE_SIZE ? 1 : 0)) * PAGE_SIZE)
#else
# define ALIGN_TO_PAGE(Siz) Siz
#endif
   int siz, block_size = 8;
   Eina_Bool reset_borders = EINA_TRUE;

#ifdef HAVE_VALGRIND
   if (RUNNING_ON_VALGRIND) evas_image_no_mmap = 1;
#endif

   if (EINA_UNLIKELY(evas_image_no_mmap == -1))
     {
        if (getenv("EFL_NO_MMAP_ANON")) evas_image_no_mmap = 1;
        else
          {
             const char *s = getenv("EVAS_IMAGE_NO_MMAP");
             evas_image_no_mmap = s && (atoi(s));
             if (evas_image_no_mmap)
               WRN("EVAS_IMAGE_NO_MMAP is set, use this only for debugging!");
          }
     }

   switch (cspace)
     {
      case EVAS_COLORSPACE_GRY8: siz = w * h * sizeof(DATA8); break;
      case EVAS_COLORSPACE_AGRY88: siz = w * h * sizeof(DATA16); break;
      case EVAS_COLORSPACE_RGBA8_ETC2_EAC:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT2:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT3:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT4:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT5:
      case EVAS_COLORSPACE_ETC1_ALPHA:
        block_size = 16;
        // fallthrough
      case EVAS_COLORSPACE_ETC1:
      case EVAS_COLORSPACE_RGB8_ETC2:
      case EVAS_COLORSPACE_RGB_S3TC_DXT1:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT1:
        reset_borders = EINA_FALSE;
        if (l && r && t && b)
          {
             w += *l + *r;
             h += *t + *b;
          }
        EINA_SAFETY_ON_FALSE_RETURN_VAL(!(w & 0x3) && !(h & 0x3), 0);
        siz = (w >> 2) * (h >> 2) * block_size;
        break;
      default:
      case EVAS_COLORSPACE_ARGB8888: siz = w * h * sizeof(DATA32); break;
     }

   if (reset_borders)
     {
        if (l) *l = 0;
        if (r) *r = 0;
        if (t) *t = 0;
        if (b) *b = 0;
     }

   if ((siz < PAGE_SIZE) || evas_image_no_mmap) return siz;

   return ALIGN_TO_PAGE(siz);
#undef ALIGN_TO_PAGE
}

/**
 * @internal
 * @brief Retrieves a specific plane of an image as an Eina_Slice.
 *
 * This function provides access to the raw data of a specific image plane.
 * The interpretation of planes depends on the image's colorspace.
 * For example:
 * - ARGB8888, AGRY88, GRY8: Have 1 plane (plane 0).
 * - YUV formats (e.g., YCBCR422P601_PL): Have multiple planes (Y, U, V).
 * - Compressed formats (e.g., ETC1): Have 1 plane (plane 0).
 *
 * @param im The RGBA_Image to get the plane from.
 * @param plane The index of the plane to retrieve (0-indexed).
 * @param[out] slice Pointer to an Eina_Slice structure to be filled with the
 *                   plane's data (memory pointer and length).
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid image,
 *         invalid plane index, or data not available).
 */
EVAS_API Eina_Bool
_evas_common_rgba_image_plane_get(const RGBA_Image *im, int plane,
                                  Eina_Slice *slice)
{
   unsigned char **csdata = NULL;
   Evas_Colorspace cs;
   size_t w, h;

   if (!im || !slice) return EINA_FALSE;
   cs = im->cache_entry.space;
   w = im->cache_entry.w;
   h = im->cache_entry.h;

   switch (cs)
     {
    case EVAS_COLORSPACE_YCBCR422P601_PL:
    case EVAS_COLORSPACE_YCBCR422P709_PL:
    case EVAS_COLORSPACE_YCBCR422601_PL:
    case EVAS_COLORSPACE_YCBCR420NV12601_PL:
    case EVAS_COLORSPACE_YCBCR420TM12601_PL:
        if (!im->cs.data) return EINA_FALSE;
        csdata = im->cs.data;
        break;

      default:
        if (!im->image.data) return EINA_FALSE;
        break;
     }

   switch (cs)
     {
      case EVAS_COLORSPACE_ARGB8888:
        if (plane != 0) return EINA_FALSE;
        slice->len = w * h * 4;
        slice->mem = im->image.data;
        return EINA_TRUE;

      case EVAS_COLORSPACE_AGRY88:
        if (plane != 0) return EINA_FALSE;
        slice->len = w * h * 2;
        slice->mem = im->image.data;
        return EINA_TRUE;

      case EVAS_COLORSPACE_GRY8:
        if (plane != 0) return EINA_FALSE;
        slice->len = w * h;
        slice->mem = im->image.data;
        return EINA_TRUE;

      case EVAS_COLORSPACE_RGB565_A5P:
        if (plane == 0)
          {
             slice->mem = im->image.data;
             slice->len = w * h * 2;
             return EINA_TRUE;
          }
        else if (plane == 1)
          {
             slice->mem = im->image.data8 + (w * h * 2);
             slice->len = w * h;
             return EINA_TRUE;
          }
        return EINA_FALSE;

        // YUV, assume contiguous memory within a plane - padding ok
        // 1 plane
      case EVAS_COLORSPACE_YCBCR422601_PL:
        if (plane != 0) return EINA_FALSE;
        slice->mem = csdata[0];
        slice->len = (h > 1) ? ((csdata[1] - csdata[0]) * h * 2) : (w * 2);
        return EINA_TRUE;

        // 2 planes
      case EVAS_COLORSPACE_YCBCR420NV12601_PL:
      case EVAS_COLORSPACE_YCBCR420TM12601_PL:
        if (plane == 0)
          {
             slice->mem = csdata[0];
             slice->len = (h > 1) ? ((csdata[1] - csdata[0]) * h) : w;
             return EINA_TRUE;
          }
        else if (plane == 1)
          {
             slice->mem = csdata[h];
             slice->len = (h > 1) ? ((csdata[h+1] - csdata[h]) * h / 2) : w / 2;
             return EINA_TRUE;
          }
        return EINA_FALSE;

        // 3 planes
      case EVAS_COLORSPACE_YCBCR422P601_PL:
      case EVAS_COLORSPACE_YCBCR422P709_PL:
        if (plane == 0)
          {
             slice->mem = csdata[0];
             slice->len = (h > 1) ? ((csdata[1] - csdata[0]) * h) : w;
             return EINA_TRUE;
          }
        else if (plane == 1)
          {
             slice->mem = csdata[h];
             slice->len = (h > 1) ? ((csdata[h+1] - csdata[h]) * h / 2) : w / 2;
             return EINA_TRUE;
          }
        else if (plane == 2)
          {
             slice->mem = csdata[h + h / 2];
             slice->len = (h > 1) ? ((csdata[h+h/2+1] - csdata[h+h/2]) * h / 2) : w / 2;
             return EINA_TRUE;
          }
        return EINA_FALSE;

        // ETC1/2 RGB, S3TC RGB
      case EVAS_COLORSPACE_ETC1:
      case EVAS_COLORSPACE_RGB8_ETC2:
      case EVAS_COLORSPACE_RGB_S3TC_DXT1:
        if (plane != 0) return EINA_FALSE;
        slice->mem = im->image.data;
        slice->len = (w * h * 8) / 16;
        return EINA_TRUE;

        // ETC2 ARGB, S3TC ARGB
      case EVAS_COLORSPACE_RGBA8_ETC2_EAC:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT1:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT2:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT3:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT4:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT5:
        if (plane != 0) return EINA_FALSE;
        slice->mem = im->image.data;
        slice->len = (w * h * 16) / 16;
        return EINA_TRUE;

        // ETC1+Alpha
      case EVAS_COLORSPACE_ETC1_ALPHA:
        if (plane == 0)
          {
             slice->mem = im->image.data;
             slice->len = (w * h * 8) / 16;
             return EINA_TRUE;
          }
        else if (plane == 1)
          {
             slice->mem = im->image.data8 + (w * h * 8) / 16;
             slice->len = (w * h * 8) / 16;
             return EINA_TRUE;
          }
        return EINA_FALSE;

      default:
        return EINA_FALSE;
     }
}

/**
 * @internal
 * @brief Calculates the byte offset for a given rectangular region and plane within an image.
 *
 * This function determines the starting byte offset of a specified sub-region
 * within a particular plane of an image. It's used for accessing specific
 * parts of image data directly. The calculation depends on the image's
 * colorspace and the plane index.
 *
 * @param rx The x-coordinate of the region's top-left corner.
 * @param ry The y-coordinate of the region's top-left corner.
 * @param rw The width of the region.
 * @param rh The height of the region.
 * @param plane The plane index (relevant for multi-planar formats like YUV).
 *              For single-plane formats, this is typically 0.
 * @param im The RGBA_Image to calculate the offset for.
 * @return The byte offset from the beginning of the plane's data to the
 *         start of the specified region, or -1 if the parameters are invalid
 *         (e.g., region out of bounds, invalid plane, unsupported colorspace,
 *         or alignment issues for compressed/YUV formats).
 * @note This function currently does not support strides other than the default
 *       packed layout for each format.
 * @warning For compressed formats (ETC, S3TC) and some YUV formats, `rx`, `ry`,
 *          `rw`, `rh` must adhere to block alignment (e.g., multiples of 4 for
 *          compressed, multiples of 2 for some YUV chroma components).
 */
EVAS_API int
_evas_common_rgba_image_data_offset(int rx, int ry, int rw, int rh,
                                    int plane, const RGBA_Image *im)
{
   // note: no stride support
   EINA_SAFETY_ON_NULL_RETURN_VAL(im, -1);

   const Image_Entry *ie = &im->cache_entry;

   if ((rx < 0) || (ry < 0) || (rw < 0) || (rh < 0)) return -1;

   if (((rx + rw) > (int) ie->w) || ((ry + rh) > (int) ie->h)) return -1;

   switch (ie->space)
     {
      case EVAS_COLORSPACE_ARGB8888:
        return (ry * ie->w + rx) * 4;
      case EVAS_COLORSPACE_AGRY88:
        return (ry * ie->w + rx) * 2;
      case EVAS_COLORSPACE_GRY8:
        return ry * ie->w + rx;
      case EVAS_COLORSPACE_RGB565_A5P:
        if (plane == 0) return (ry * ie->w + rx) * 2;
        else if (plane == 1) return ry * ie->w + rx + (ie->w * ie->h) * 2;
        else return -1;

        // YUV
      case EVAS_COLORSPACE_YCBCR422P601_PL:
      case EVAS_COLORSPACE_YCBCR422P709_PL:
      case EVAS_COLORSPACE_YCBCR422601_PL:
        if ((rx & 1) || (rw & 1)) return -1;
        if (plane == 0) return ry * ie->w + rx;
        else if (plane == 1) return (ry * ie->w) / 2 + rx + ie->w * ie->h;
        else return -1;

      case EVAS_COLORSPACE_YCBCR420NV12601_PL:
      case EVAS_COLORSPACE_YCBCR420TM12601_PL:
        if ((rx & 1) || (ry & 1) || (rw & 1) || (rh & 1)) return -1;
        if (plane == 0) return ry * ie->w + rx;
        else if (plane == 1) return (ry * ie->w + rx) / 2 + ie->w * ie->h;
        else return -1;

        // ETC1/2 RGB, S3TC RGB
      case EVAS_COLORSPACE_ETC1:
      case EVAS_COLORSPACE_RGB8_ETC2:
      case EVAS_COLORSPACE_RGB_S3TC_DXT1:
        if ((rx & 3) || (ry & 3) || (rw & 3) || (rh & 3)) return -1;
        return (ry * ie->w + rx) * 8 / 16;

        // ETC2 ARGB, S3TC ARGB
      case EVAS_COLORSPACE_RGBA8_ETC2_EAC:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT1:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT2:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT3:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT4:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT5:
        if ((rx & 3) || (ry & 3) || (rw & 3) || (rh & 3)) return -1;
        return (ry * ie->w + rx) * 16 / 16;

        // ETC1+Alpha
      case EVAS_COLORSPACE_ETC1_ALPHA:
        if ((rx & 3) || (ry & 3) || (rw & 3) || (rh & 3)) return -1;
        if (plane == 0) return (ry * ie->w + rx) * 8 / 16;
        else if (plane == 1)
          return (ry * ie->w + rx) * 8 / 16 + (ie->w * ie->h) * 8 / 16;
        else return -1;

      default:
        CRI("unknown colorspace %d", ie->space);
        return EINA_FALSE;
     }
}

/**
 * @internal
 * @brief Allocates memory for an image surface, attempting to use mmap for larger images.
 *
 * This function calculates the required size using
 * `_evas_common_rgba_image_surface_size` and then allocates memory.
 * If the size is large enough and mmap is not disabled (via `evas_image_no_mmap`
 * or environment variables), it tries to use `mmap` (potentially with
 * `MAP_HUGETLB` for very large allocations). Otherwise, it falls back to `malloc`.
 *
 * @param ie The Image_Entry for which the surface is being allocated.
 *           Used to access `ie->space` for size calculation.
 * @param w The base width of the image.
 * @param h The base height of the image.
 * @param[in,out] pl Pointer to the left border size. Passed to
 *                  `_evas_common_rgba_image_surface_size`.
 * @param[in,out] pr Pointer to the right border size. Passed to
 *                  `_evas_common_rgba_image_surface_size`.
 * @param[in,out] pt Pointer to the top border size. Passed to
 *                  `_evas_common_rgba_image_surface_size`.
 * @param[in,out] pb Pointer to the bottom border size. Passed to
 *                  `_evas_common_rgba_image_surface_size`.
 * @return A pointer to the allocated memory block, or NULL on allocation failure.
 */
static void *
_evas_common_rgba_image_surface_mmap(Image_Entry *ie,
                                     unsigned int w, unsigned int h,
                                     /*inout*/int *pl, int *pr,
                                     int *pt, int *pb)
{
   int siz;
#if defined (HAVE_SYS_MMAN_H) && (!defined (_WIN32))
   void *r = MAP_FAILED;
#endif

   siz = _evas_common_rgba_image_surface_size(w, h, ie->space, pl, pr, pt, pb);

#if defined (HAVE_SYS_MMAN_H) && (!defined (_WIN32))
#ifndef MAP_HUGETLB
# define MAP_HUGETLB 0
#endif
   if (siz < 0) return NULL;

   if ((siz < PAGE_SIZE) || evas_image_no_mmap) return malloc(siz);

   if (siz > ((HUGE_PAGE_SIZE * 75) / 100))
     r = mmap(NULL, siz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_HUGETLB, -1, 0);
   if (r == MAP_FAILED)
     r = mmap(NULL, siz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
   if (r == MAP_FAILED)
     r = NULL;

   return r;
#else
   return malloc(siz);
#endif
}

/**
 * @internal
 * @brief Frees memory previously allocated for an image surface by
 *        `_evas_common_rgba_image_surface_mmap`.
 *
 * This function determines if the memory was allocated via `mmap` or `malloc`
 * based on its size and the `evas_image_no_mmap` flag, and calls the
 * corresponding deallocation function (`munmap` or `free`).
 *
 * @param data Pointer to the memory block to free.
 * @param w The original width used for allocation (needed for size recalculation).
 * @param h The original height used for allocation (needed for size recalculation).
 * @param cspace The colorspace used for allocation (needed for size recalculation).
 */
void
evas_common_rgba_image_surface_munmap(void *data, unsigned int w, unsigned int h, Evas_Colorspace cspace)
{
   if (!data) return;
#if defined (HAVE_SYS_MMAN_H) && (!defined (_WIN32))
   size_t siz;

   siz = _evas_common_rgba_image_surface_size(w, h, cspace,
                                              NULL, NULL, NULL, NULL);
   if ((siz < PAGE_SIZE) || evas_image_no_mmap) free(data);
   else munmap(data, siz);
#else
   (void)w;
   (void)h;
   (void)cspace;
   free(data);
#endif
}

/**
 * @brief Initializes the Evas common image handling system.
 * @api
 *
 * This function sets up the global image cache (`eci`) if it hasn't been
 * initialized yet, increments the reference counter for the image system,
 * and initializes the scale cache. It should be called before any other
 * Evas image functions are used.
 */
EVAS_API void
evas_common_image_init(void)
{
   if (!eci) eci = evas_cache_image_init(&_evas_common_image_func);
   reference++;

   evas_common_scalecache_init();
}

/**
 * @brief Shuts down the Evas common image handling system.
 * @api
 *
 * This function decrements the reference counter for the image system.
 * If the reference count drops to zero, it shuts down and frees the
 * global image cache (`eci`). It also shuts down the scale cache.
 * This should be called when Evas image handling is no longer needed
 * to release resources.
 */
EVAS_API void
evas_common_image_shutdown(void)
{
   if (--reference == 0)
     {
////	printf("REF--=%i\n", reference);
// DISABLE for now - something wrong with cache shutdown freeing things
// still in use - rage_thumb segv's now.
//
// actually - i think i see it. cache ref goes to 0 (and thus gets freed)
// because in eng_setup() when a buffer changes size it is FIRST freed
// THEN allocated again - thus brignhjing ref to 0 then back to 1 immediately
// where it should stay at 1. - see evas_engine.c in the buffer enigne for
// example. eng_output_free() is called BEFORE _output_setup(). although this
// is only a SIGNE of the problem. we can patch this up with either freeing
// after the setup (so we just pt a ref of 2 then back to 1), or just
// evas_common_image_init() at the start and evas_common_image_shutdown()
// after it all. really ref 0 should only be reached when no more canvases
// with no more objects exist anywhere.

// ENABLE IT AGAIN, hope it is fixed. Gustavo @ January 22nd, 2009.
       evas_cache_image_shutdown(eci);
       eci = NULL;
     }
   evas_common_scalecache_shutdown();
}

/**
 * @brief Unloads all images currently held in the Evas image cache.
 * @api
 *
 * This function first dumps the RGBA image scale cache and then instructs
 * the main image cache (`eci`) to unload all its entries. This is typically
 * used to free up memory by clearing cached image data.
 */
EVAS_API void
evas_common_image_image_all_unload(void)
{
   evas_common_rgba_image_scalecache_dump();
   evas_cache_image_unload_all(eci);
}

/**
 * @internal
 * @brief Allocates and initializes a new RGBA_Image structure.
 *
 * This function allocates memory for an RGBA_Image structure, initializes its
 * flags to RGBA_IMAGE_NOTHING, and initializes its scale cache entry.
 *
 * @return A pointer to the `cache_entry` member of the newly allocated
 *         RGBA_Image, cast to Image_Entry*. Returns NULL on allocation failure.
 */
static Image_Entry *
_evas_common_rgba_image_new(void)
{
   RGBA_Image *im;

   im = calloc(1, sizeof(RGBA_Image));
   if (!im) return NULL;
   im->flags = RGBA_IMAGE_NOTHING;

   evas_common_rgba_image_scalecache_init(&im->cache_entry);
   return &im->cache_entry;
}

/**
 * @internal
 * @brief Frees an RGBA_Image structure and its associated resources.
 *
 * This function performs cleanup for an RGBA_Image:
 * - Removes it from pending unloads.
 * - Frees pipe render data if applicable.
 * - Closes the loader if data was loaded via a module.
 * - Shuts down its scale cache entry.
 * - Unrefs the loader module if present.
 * - Frees animated frame data if any.
 * - Closes the Eina_File if it was opened and not a given mmap.
 * - Finally, queues the RGBA_Image structure itself for freeing.
 *
 * @param ie Pointer to the Image_Entry (cast from RGBA_Image*) to be deleted.
 */
static void
_evas_common_rgba_image_delete(Image_Entry *ie)
{
   RGBA_Image *im = (RGBA_Image *)ie;

   evas_common_rgba_pending_unloads_remove(ie);
#ifdef BUILD_PIPE_RENDER
   evas_common_pipe_free(im);
#endif
   if (ie->loader_data)
     {
        Evas_Image_Load_Func *evas_image_load_func = NULL;

        evas_image_load_func = ie->info.loader;
        if (evas_image_load_func)
          evas_image_load_func->file_close(ie->loader_data);
        ie->loader_data = NULL;
     }
   evas_common_rgba_image_scalecache_shutdown(&im->cache_entry);
   if (ie->info.module) evas_module_unref((Evas_Module *)ie->info.module);

   if (ie->animated.frames)
     {
        Image_Entry_Frame *frame;

        EINA_LIST_FREE(ie->animated.frames, frame)
          {
             if (frame->data) free(frame->data);
             if (frame->info) free(frame->info);
             free(frame);
          }
     }
   if (ie->f && !ie->flags.given_mmap)
     {
        eina_file_close(ie->f); // close matching open (dup in _evas_image_file_header) OK
        ie->f = NULL;
     }
   eina_freeq_ptr_add(eina_freeq_main_get(), im, free, sizeof(*im));
}

/**
 * @internal
 * @brief Performs the actual unloading of an image's pixel data.
 *
 * This function is responsible for freeing the pixel data associated with an
 * Image_Entry. It handles cases where color-space-specific data (`im->cs.data`)
 * might be separate from or the same as the main image data (`im->image.data`).
 * It also resets loaded flags and allocated dimensions.
 *
 * @param ie The Image_Entry whose data is to be unloaded.
 */
static void
evas_common_rgba_image_unload_real(Image_Entry *ie)
{
   RGBA_Image   *im = (RGBA_Image *) ie;

   ie->flags.loaded = 0;

   if ((im->cs.data) && (im->image.data))
     {
        if (im->cs.data != im->image.data)
          {
             if (!im->cs.no_free) free(im->cs.data);
          }
     }
   else if (im->cs.data)
     {
        if (!im->cs.no_free) free(im->cs.data);
     }
   im->cs.data = NULL;

   if (im->image.data && !im->image.no_free)
     {
        evas_common_rgba_image_surface_munmap(im->image.data,
                                              ie->allocated.w, ie->allocated.h,
                                              ie->space);
#ifdef SURFDBG
        surfs = eina_list_remove(surfs, ie);
#endif
     }
   im->image.data = NULL;
   ie->allocated.w = 0;
   ie->allocated.h = 0;
   ie->flags.loaded = 0;
   ie->flags.preload_done = 0;
   ie->need_unload = 0;
#ifdef SURFDBG
   surf_debug();
#endif
}

/** @internal List of Image_Entry objects that are pending unload. */
static Eina_List *pending_unloads = NULL;

/**
 * @internal
 * @brief Processes the list of images pending unload and unloads them.
 *
 * Iterates through the `pending_unloads` list. For each image entry that
 * `need_unload` is true and is not currently in a preload state,
 * this function calls `evas_common_rgba_image_unload_real` to free its
 * pixel data and then removes it from the `pending_unloads` list.
 */
EVAS_API void
evas_common_rgba_pending_unloads_cleanup(void)
{
   Image_Entry *ie;
   Eina_List *l;
   Eina_List *l_next;

   EINA_LIST_FOREACH_SAFE(pending_unloads, l, l_next, ie)
     {
        if ((ie->need_unload) && (!ie->preload))
          {
             evas_common_rgba_image_unload_real(ie);
             pending_unloads = eina_list_remove_list(pending_unloads, l);
          }
     }
}

/**
 * @internal
 * @brief Removes an Image_Entry from the list of pending unloads.
 *
 * If the given Image_Entry `ie` is marked as `need_unload`, this function
 * resets the `need_unload` flag and removes the entry from the global
 * `pending_unloads` list.
 *
 * @param ie The Image_Entry to remove from the pending unload list.
 */
EVAS_API void
evas_common_rgba_pending_unloads_remove(Image_Entry *ie)
{
   if (!ie->need_unload) return;
   ie->need_unload = 0;
   pending_unloads = eina_list_remove(pending_unloads, ie);
}

/**
 * @internal
 * @brief Frees an Image_Entry if its reference count is zero.
 *
 * This function checks if an Image_Entry has any references. If not (references <= 0),
 * it proceeds to:
 * 1. Remove it from any pending unloads.
 * 2. Delete its surface data using `_evas_common_rgba_image_surface_delete`.
 * 3. Delete the Image_Entry structure itself using `_evas_common_rgba_image_delete`.
 *
 * @param ie The Image_Entry to potentially free.
 */
EVAS_API void
evas_common_rgba_image_free(Image_Entry *ie)
{
   if (ie->references > 0) return;
   evas_common_rgba_pending_unloads_remove(ie);
   _evas_common_rgba_image_surface_delete(ie);
   _evas_common_rgba_image_delete(ie);
}

#ifdef SURFDBG
/** @internal List used for debugging allocated surfaces. Only active if SURFDBG is defined. */
static Eina_List *surfs = NULL;

/**
 * @internal
 * @brief Prints debugging information about currently allocated image surfaces.
 *
 * This function iterates through the `surfs` list (if SURFDBG is defined)
 * and prints details for each allocated surface, including its memory address,
 * dimensions, file path, and key.
 *
 * @note This function is only compiled if `SURFDBG` is defined.
 */
static void
surf_debug(void)
{
   Eina_List *l;
   Image_Entry *ie;
   RGBA_Image *im;
   int i = 0;

   printf("----SURFS----\n");
   EINA_LIST_FOREACH(surfs, l, ie)
     {
        im = ie;
        printf("%i - %p - %ix%i  [%s][%s]\n",
               i, im->image.data, ie->allocated.w, ie->allocated.h,
               ie->file, ie->key
              );
        i++;
     }
}
#endif

/**
 * @internal
 * @brief Unloads an image's data, potentially deferring the operation.
 *
 * This function handles the unloading of an image's pixel data.
 * - If the image is not loaded, or has no associated module/data1 (for non-file based images),
 *   or has no file/mmap source, it returns.
 * - If asynchronous operations are not frozen and the image still has references,
 *   it marks the image as `need_unload` and adds it to the `pending_unloads` list
 *   for later processing by `evas_common_rgba_pending_unloads_cleanup`.
 * - Otherwise (async frozen or no references), or if already marked for unload,
 *   it calls `evas_common_rgba_image_unload_real` to perform the unload immediately.
 *
 * @param ie The Image_Entry to unload.
 */
EVAS_API void
evas_common_rgba_image_unload(Image_Entry *ie)
{
   if (!ie->flags.loaded) return;
   if ((!ie->info.module) && (!ie->data1)) return;
   if (!ie->file && !ie->f) return;
   if ((evas_cache_async_frozen_get() == 0) &&
       (ie->references > 0))
     {
        if (!ie->need_unload)
          {
             pending_unloads = eina_list_append(pending_unloads, ie);
             ie->need_unload = 1;
          }
        return;
     }
   if (!ie->need_unload) evas_common_rgba_image_unload_real(ie);
}

/**
 * @internal
 * @brief Updates or creates a Pixman image associated with an Image_Entry.
 *
 * This function is called after an image's surface data (`im->image.data`)
 * has been allocated or modified. If Pixman support is enabled
 * (`HAVE_PIXMAN` and `PIXMAN_IMAGE` are defined), it unrefs any existing
 * Pixman image and creates a new one (`im->pixman.im`) wrapping the
 * current pixel data. The Pixman image format (e.g., `PIXMAN_a8r8g8b8` or
 * `PIXMAN_x8r8g8b8`) is chosen based on whether the image has an alpha channel.
 *
 * @param ie The Image_Entry whose Pixman representation needs to be updated.
 * @note This function does nothing if Pixman support is not compiled in.
 */
void
_evas_common_rgba_image_post_surface(Image_Entry *ie)
{
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_IMAGE
   RGBA_Image *im = (RGBA_Image *)ie;
   int w, h;

   if (!im->image.data) return;
   if (im->pixman.im) pixman_image_unref(im->pixman.im);
   w = ie->allocated.w;
   h = ie->allocated.h;
   if ((w <= 0) || (h <= 0))
     {
        w = im->cache_entry.w;
        h = im->cache_entry.h;
     }
   if (im->cache_entry.flags.alpha)
     {
        im->pixman.im = pixman_image_create_bits
          (
#ifdef WORDS_BIGENDIAN
           PIXMAN_b8g8r8a8,
#else
           PIXMAN_a8r8g8b8,
#endif
           w, h, im->image.data, w * 4);
     }
   else
     {
        im->pixman.im = pixman_image_create_bits
          (
#ifdef WORDS_BIGENDIAN
           PIXMAN_b8g8r8x8,
#else
           PIXMAN_x8r8g8b8,
#endif
           w, h, im->image.data, w * 4);
     }
# else
   (void)ie;
# endif
#else
   (void)ie;
#endif
}

/**
 * @internal
 * @brief Allocates or reallocates the pixel data surface for an Image_Entry.
 *
 * If the image already has pixel data (`im->image.data`) and it's not marked
 * as `no_free`, the existing data is unmapped/freed.
 * Then, new memory is allocated using `_evas_common_rgba_image_surface_mmap`,
 * considering any borders defined in `ie->borders`. The actual allocated
 * dimensions (`ie->allocated.w`, `ie->allocated.h`) and border values
 * might be adjusted by the mmap function (e.g., due to page alignment for
 * compressed textures).
 * After successful allocation, Valgrind macros are used (if enabled) to mark
 * the memory as readable/defined, and `_evas_common_rgba_image_post_surface`
 * is called to update any associated Pixman image.
 *
 * @param ie The Image_Entry for which to allocate the surface.
 * @param w The desired logical width of the image surface.
 * @param h The desired logical height of the image surface.
 * @return 0 on success, -1 on allocation failure.
 */
static int
_evas_common_rgba_image_surface_alloc(Image_Entry *ie,
                                      unsigned int w, unsigned int h)
{
   RGBA_Image *im = (RGBA_Image *) ie;
   int l = 0, r = 0, t = 0, b = 0;

   if (im->image.no_free) return 0;

   if (im->image.data)
     {
        evas_common_rgba_image_surface_munmap(im->image.data,
                                              ie->allocated.w, ie->allocated.h,
                                              ie->space);
        im->image.data = NULL;
#ifdef SURFDBG
        surfs = eina_list_remove(surfs, ie);
#endif
     }

   l = ie->borders.l;
   r = ie->borders.r;
   t = ie->borders.t;
   b = ie->borders.b;
   im->image.data = _evas_common_rgba_image_surface_mmap(ie, w, h,
                                                         &l, &r, &t, &b);
   if (!im->image.data) return -1;
   ie->borders.l = l;
   ie->borders.r = r;
   ie->borders.t = t;
   ie->borders.b = b;
   ie->allocated.w = w + l + r;
   ie->allocated.h = h + t + b;
#ifdef SURFDBG
   surfs = eina_list_append(surfs, ie);
#endif
#ifdef HAVE_VALGRIND
   int        siz = 0;
   siz = _evas_common_rgba_image_surface_size(w, h, ie->space, &l, &r, &t, &b);
# ifdef VALGRIND_MAKE_READABLE
   if (siz > 0) VALGRIND_MAKE_READABLE(im->image.data, siz);
# else
#  ifdef VALGRIND_MAKE_MEM_DEFINED
   if (siz > 0) VALGRIND_MAKE_MEM_DEFINED(im->image.data, siz);
#  endif
# endif
#endif
   _evas_common_rgba_image_post_surface(ie);
#ifdef SURFDBG
   surf_debug();
#endif
   return 0;
}

/**
 * @internal
 * @brief Deallocates the pixel data surface and related resources of an Image_Entry.
 *
 * This function performs the following cleanup actions:
 * - Unrefs and NULLs the Pixman image (`im->pixman.im`) if Pixman is used.
 * - Frees color-space-specific data (`im->cs.data`) if it's separate from
 *   the main image data and not marked `no_free`.
 * - Unmaps/frees the main image data (`im->image.data`) if it exists and
 *   is not marked `no_free`, using `evas_common_rgba_image_surface_munmap`.
 * - Resets `im->image.data` to NULL and allocated dimensions to 0.
 * - Clears `preload_done` and `loaded` flags.
 * - Marks the scale cache entry as dirty.
 *
 * @param ie The Image_Entry whose surface is to be deleted.
 */
static void
_evas_common_rgba_image_surface_delete(Image_Entry *ie)
{
   RGBA_Image   *im = (RGBA_Image *) ie;

#ifdef HAVE_PIXMAN
# ifdef PIXMAN_IMAGE
   if (im->pixman.im)
     {
        pixman_image_unref(im->pixman.im);
        im->pixman.im = NULL;
     }
# endif
#endif
   if (ie->file) DBG("unload: [%p] %s %s", ie, ie->file, ie->key);
   if ((im->cs.data) && (im->image.data))
     {
        if (im->cs.data != im->image.data)
          {
             if (!im->cs.no_free) free(im->cs.data);
          }
     }
   else if (im->cs.data)
     {
        if (!im->cs.no_free) free(im->cs.data);
     }
   im->cs.data = NULL;

   if (im->image.data && !im->image.no_free)
     {
        evas_common_rgba_image_surface_munmap(im->image.data,
                                              ie->allocated.w, ie->allocated.h,
                                              ie->space);
#ifdef SURFDBG
        surfs = eina_list_remove(surfs, ie);
#endif
     }

   im->image.data = NULL;
   ie->allocated.w = 0;
   ie->allocated.h = 0;
   ie->flags.preload_done = 0;
   ie->flags.loaded = 0;
   evas_common_rgba_image_scalecache_dirty(&im->cache_entry);
#ifdef SURFDBG
   surf_debug();
#endif
}

/**
 * @internal
 * @brief Marks an image's scale cache as dirty and then unloads the image.
 * This is typically a callback function used by the image cache system.
 * @param im The Image_Entry to process.
 */
static void
_evas_common_rgba_image_unload(Image_Entry *im)
{
   evas_common_rgba_image_scalecache_dirty(im);
   evas_common_rgba_image_unload(im);
}

/**
 * @internal
 * @brief Marks an image as dirty and its scale cache entry as dirty.
 * This is typically a callback function used by the image cache system when
 * a region of the image has been modified.
 * @param ie The Image_Entry to mark as dirty.
 * @param x The x-coordinate of the start of the dirty region (unused in this impl).
 * @param y The y-coordinate of the start of the dirty region (unused in this impl).
 * @param w The width of the dirty region (unused in this impl).
 * @param h The height of the dirty region (unused in this impl).
 */
static void
_evas_common_rgba_image_dirty_region(Image_Entry* ie,
                                     unsigned int x EINA_UNUSED,
                                     unsigned int y EINA_UNUSED,
                                     unsigned int w EINA_UNUSED,
                                     unsigned int h EINA_UNUSED)
{
   RGBA_Image   *im = (RGBA_Image *) ie;

   im->flags |= RGBA_IMAGE_IS_DIRTY;
   evas_common_rgba_image_scalecache_dirty(&im->cache_entry);
}

/**
 * @internal
 * @brief Handles image duplication when the source image might be dirty or needs loading.
 *
 * This function is called by the image cache when a new image entry (`ie_dst`)
 * is created as a copy of an existing one (`ie_src`), and `ie_src` might have
 * pending changes or its data needs to be loaded.
 *
 * It performs the following steps:
 * 1. Marks the scale caches of both source and destination images as dirty.
 * 2. Ensures the source image data is loaded via `evas_cache_image_load_data`.
 * 3. If the destination image does not have its pixel surface allocated
 *    (`evas_cache_image_pixels(ie_dst)` returns NULL), it allocates the surface
 *    for `ie_dst` with the same dimensions as `ie_src`.
 * 4. Normalizes the colorspaces of both source and destination images.
 *
 * @param ie_dst The destination Image_Entry (the new copy).
 * @param ie_src The source Image_Entry (the original).
 * @return 0 on success. Returns 1 if surface allocation for `ie_dst` fails.
 * @note This function is only called when the image cache determines it's
 *       necessary, typically when `ie_src` has references.
 */
static int
_evas_common_rgba_image_dirty(Image_Entry *ie_dst, const Image_Entry *ie_src)
{
   RGBA_Image   *dst = (RGBA_Image *) ie_dst;
   RGBA_Image   *src = (RGBA_Image *) ie_src;

   evas_common_rgba_image_scalecache_dirty((Image_Entry *)ie_src);
   evas_common_rgba_image_scalecache_dirty(ie_dst);
   evas_cache_image_load_data(&src->cache_entry);
   if (!evas_cache_image_pixels(ie_dst))
     {
        if (_evas_common_rgba_image_surface_alloc(&dst->cache_entry,
                                                  src->cache_entry.w,
                                                  src->cache_entry.h))
          {
             return 1;
          }
     }
   evas_common_image_colorspace_normalize(src);
   evas_common_image_colorspace_normalize(dst);
   return 0;
}

/**
 * @internal
 * @brief Estimates the RAM usage of an Image_Entry and its associated data.
 *
 * This function calculates an approximate memory footprint for an image, including:
 * - Size of the RGBA_Image structure itself.
 * - Length of `cache_key`, `file`, and `key` strings if they exist.
 * - Size of the mmapped file if `ie->f` is a virtual Eina_File.
 * - Size of the pixel data (`im->image.data`) if allocated and not `no_free`,
 *   approximated as `w * h * sizeof(DATA32)`.
 * - Usage reported by `evas_common_rgba_image_scalecache_usage_get`.
 *
 * @param ie The Image_Entry for which to estimate RAM usage.
 * @return The estimated total RAM usage in bytes.
 * @note The pixel data size calculation is a simplification and might not be
 *       accurate for all colorspaces, especially compressed ones or those not
 *       using 4 bytes per pixel.
 */
static int
_evas_common_rgba_image_ram_usage(Image_Entry *ie)
{
   RGBA_Image *im = (RGBA_Image *)ie;
   int size = sizeof(struct _RGBA_Image);

   if (ie->cache_key) size += strlen(ie->cache_key);
   if (ie->file) size += strlen(ie->file);
   if (ie->key) size += strlen(ie->key);
   if (ie->f && eina_file_virtual(ie->f)) size += eina_file_size_get(ie->f);

   if (im->image.data)
     {
        if ((!im->image.no_free))
          size += im->cache_entry.w * im->cache_entry.h * sizeof(DATA32);
     }
   size += evas_common_rgba_image_scalecache_usage_get(&im->cache_entry);
   return size;
}

/**
 * @internal
 * @brief Retrieves a direct pointer to the pixel data of an Image_Entry's surface.
 * This is typically a callback function for the image cache.
 * @param ie The Image_Entry.
 * @return A pointer to the raw pixel data (DATA32*), or NULL if the surface
 *         is not allocated or `ie` is invalid.
 */
static DATA32 *
_evas_common_rgba_image_surface_pixels(Image_Entry *ie)
{
   RGBA_Image *im = (RGBA_Image *)ie;
   return im->image.data;
}

/**
 * @internal
 * @brief Creates a new RGBA_Image with specified dimensions, without an alpha channel.
 *
 * This function allocates a new RGBA_Image structure using
 * `_evas_common_rgba_image_new`, sets its width and height, and then
 * allocates its pixel surface using `_evas_common_rgba_image_surface_alloc`.
 * The image is marked as not having an alpha channel by default and is not
 * considered part of the cache (`im->cache_entry.flags.cached = 0`).
 *
 * @param w The width of the image to create.
 * @param h The height of the image to create.
 * @return A pointer to the newly created RGBA_Image, or NULL on failure
 *         (either structure allocation or surface allocation fails).
 */
static RGBA_Image *
evas_common_image_create(unsigned int w, unsigned int h)
{
   RGBA_Image *im;

   im = (RGBA_Image *) _evas_common_rgba_image_new();
   if (!im) return NULL;
   im->cache_entry.w = w;
   im->cache_entry.h = h;
   if (_evas_common_rgba_image_surface_alloc(&im->cache_entry, w, h))
     {
        _evas_common_rgba_image_delete(&im->cache_entry);
        return NULL;
     }
   im->cache_entry.flags.cached = 0;
   return im;
}

/**
 * @brief Creates a new RGBA_Image with specified dimensions and an alpha channel.
 * @api
 *
 * This function allocates a new RGBA_Image structure using
 * `_evas_common_rgba_image_new`, sets its width and height, and explicitly
 * marks it as having an alpha channel (`im->cache_entry.flags.alpha = 1`).
 * It then allocates its pixel surface using `_evas_common_rgba_image_surface_alloc`.
 * The image is not considered part of the cache (`im->cache_entry.flags.cached = 0`).
 *
 * @param w The width of the image to create.
 * @param h The height of the image to create.
 * @return A pointer to the newly created RGBA_Image, or NULL on failure
 *         (either structure allocation or surface allocation fails).
 */
EVAS_API RGBA_Image *
evas_common_image_alpha_create(unsigned int w, unsigned int h)
{
   RGBA_Image *im;

   im = (RGBA_Image *) _evas_common_rgba_image_new();
   if (!im) return NULL;
   im->cache_entry.w = w;
   im->cache_entry.h = h;
   im->cache_entry.flags.alpha = 1;
   if (_evas_common_rgba_image_surface_alloc(&im->cache_entry, w, h))
     {
        _evas_common_rgba_image_delete(&im->cache_entry);
        return NULL;
     }
   im->cache_entry.flags.cached = 0;
   return im;
}

/**
 * @brief Creates a new RGBA_Image with specified dimensions, optionally with an alpha channel.
 * @api
 *
 * This is a convenience function that calls either `evas_common_image_alpha_create`
 * (if `alpha` is non-zero) or `evas_common_image_create` (if `alpha` is zero)
 * to construct the image.
 *
 * @param w The width of the image to create.
 * @param h The height of the image to create.
 * @param alpha If non-zero, the image will be created with an alpha channel.
 *              If zero, it will be created without an alpha channel.
 * @return A pointer to the newly created RGBA_Image, or NULL on failure.
 */
EVAS_API RGBA_Image *
evas_common_image_new(unsigned int w, unsigned int h, unsigned int alpha)
{
   if (alpha) return evas_common_image_alpha_create(w, h);
   return evas_common_image_create(w, h);
}

/**
 * @internal
 * @brief Normalizes an image's pixel data to the ARGB8888 colorspace if necessary.
 *
 * This function checks if an image's colorspace-specific data (`im->cs.data`)
 * needs to be converted to the canonical ARGB8888 format stored in `im->image.data`.
 * This typically happens if `im->cs.dirty` is set or the image has the
 * `RGBA_IMAGE_IS_DIRTY` flag.
 *
 * - If the image is already in ARGB8888, GRY8, or AGRY88:
 *   It ensures `im->image.data` points to `im->cs.data`. If `im->image.data`
 *   was previously separate and not marked `no_free`, it's deallocated.
 * - For YUV colorspaces (e.g., YCBCR422P601_PL, YCBCR420NV12601_PL):
 *   It calls the appropriate YUV-to-RGBA conversion function (e.g.,
 *   `evas_common_convert_yuv_422p_601_rgba`) to convert data from
 *   `im->cs.data` (which holds YUV planes) into `im->image.data` (ARGB8888).
 * - Other colorspaces are currently not handled for conversion in this function.
 *
 * After processing, `im->cs.dirty` is cleared.
 *
 * @param im The RGBA_Image to normalize.
 */
void
evas_common_image_colorspace_normalize(RGBA_Image *im)
{
   if ((!im->cs.data) ||
       ((!im->cs.dirty) && (!(im->flags & RGBA_IMAGE_IS_DIRTY)))) return;
   switch (im->cache_entry.space)
     {
      case EVAS_COLORSPACE_ARGB8888:
      case EVAS_COLORSPACE_GRY8:
      case EVAS_COLORSPACE_AGRY88:
        if (im->image.data != im->cs.data)
          {
             if (!im->image.no_free)
               {
                  evas_common_rgba_image_surface_munmap
                    (im->image.data,
                     im->cache_entry.allocated.w,
                     im->cache_entry.allocated.h,
                     im->cache_entry.space);
#ifdef SURFDBG
                  surfs = eina_list_remove(surfs, im);
#endif
                  ((Image_Entry *)im)->allocated.w = 0;
                  ((Image_Entry *)im)->allocated.h = 0;
               }
             im->image.data = im->cs.data;
             im->cs.no_free = im->image.no_free;
          }
        break;
      case EVAS_COLORSPACE_YCBCR422P601_PL:
        if ((im->image.data) && (*((unsigned char **)im->cs.data)))
          evas_common_convert_yuv_422p_601_rgba(im->cs.data,
                                                (DATA8 *)im->image.data,
                                                im->cache_entry.w,
                                                im->cache_entry.h);
        break;
      case EVAS_COLORSPACE_YCBCR422601_PL:
        if ((im->image.data) && (*((unsigned char **)im->cs.data)))
          evas_common_convert_yuv_422_601_rgba(im->cs.data,
                                               (DATA8 *)im->image.data,
                                               im->cache_entry.w,
                                               im->cache_entry.h);
        break;
      case EVAS_COLORSPACE_YCBCR420NV12601_PL:
        if ((im->image.data) && (*((unsigned char **)im->cs.data)))
          evas_common_convert_yuv_420_601_rgba(im->cs.data,
                                               (DATA8 *)im->image.data,
                                               im->cache_entry.w,
                                               im->cache_entry.h);
        break;
      case EVAS_COLORSPACE_YCBCR420TM12601_PL:
        if ((im->image.data) && (*((unsigned char **)im->cs.data)))
          evas_common_convert_yuv_420T_601_rgba(im->cs.data,
                                                (DATA8 *)im->image.data,
                                                im->cache_entry.w,
                                                im->cache_entry.h);
         break;
      case EMILE_COLORSPACE_YCBCR422P709_PL:
        if ((im->image.data) && (*((unsigned char **)im->cs.data)))
          evas_common_convert_yuv_422p_709_rgba(im->cs.data,
                                                (DATA8 *)im->image.data,
                                                im->cache_entry.w,
                                                im->cache_entry.h);
        break;
      default:
        break;
     }
   im->cs.dirty = 0;
#ifdef SURFDBG
   surf_debug();
#endif
}

/**
 * @brief Marks an image's colorspace data as dirty.
 * @api
 *
 * This function sets the `cs.dirty` flag on the given RGBA_Image to 1,
 * indicating that its colorspace-specific data (`im->cs.data`) may be out of
 * sync with its canonical ARGB representation (`im->image.data`) or that
 * a conversion is needed. It also marks the image's scale cache entry as dirty.
 * If Pixman support is enabled, it unrefs any existing Pixman image associated
 * with this RGBA_Image and triggers an update via
 * `_evas_common_rgba_image_post_surface`. This ensures that subsequent
 * operations will correctly handle or reconvert the data.
 *
 * @param im The RGBA_Image to mark as dirty.
 */
EVAS_API void
evas_common_image_colorspace_dirty(RGBA_Image *im)
{
   im->cs.dirty = 1;
   evas_common_rgba_image_scalecache_dirty(&im->cache_entry);
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_IMAGE
   if (im->pixman.im)
     {
        pixman_image_unref(im->pixman.im);
        im->pixman.im = NULL;
     }
   _evas_common_rgba_image_post_surface((Image_Entry *)im);
# endif
#endif
}

/**
 * @brief Sets the maximum size of the Evas image cache.
 * @api
 * @param size The desired maximum cache size in bytes.
 * If the global image cache (`eci`) is initialized, this function calls
 * `evas_cache_image_set` to adjust its size.
 */
EVAS_API void
evas_common_image_set_cache(unsigned int size)
{
   if (eci)
     evas_cache_image_set(eci, size);
}

/**
 * @brief Gets the current maximum size of the Evas image cache.
 * @api
 * @return The current maximum cache size in bytes, or 0 if the cache
 *         is not initialized.
 */
EVAS_API int
evas_common_image_get_cache(void)
{
   return evas_cache_image_get(eci);
}

/**
 * @brief Loads an image from a file using the Evas image cache.
 * @api
 * @param file The path to the image file.
 * @param key Optional key for caching; can be NULL.
 * @param lo Pointer to Evas_Image_Load_Opts structure for load options; can be NULL.
 * @param[out] error Pointer to an integer where the load error code will be stored.
 * @return A pointer to the loaded RGBA_Image, or NULL on failure.
 *         The image is managed by the cache.
 * If `file` is NULL, sets `*error` to `EVAS_LOAD_ERROR_GENERIC` and returns NULL.
 * Otherwise, requests the image from the global cache `eci`.
 */
EVAS_API RGBA_Image *
evas_common_load_image_from_file(const char *file, const char *key,
                                 Evas_Image_Load_Opts *lo, int *error)
{
   if (!file)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        return NULL;
     }
   return (RGBA_Image *) evas_cache_image_request(eci, file, key, lo, error);
}

/**
 * @brief Loads an image from an Eina_File (memory-mapped file) using the Evas image cache.
 * @api
 * @param f Pointer to the Eina_File representing the memory-mapped image.
 * @param key Optional key for caching; can be NULL.
 * @param lo Pointer to Evas_Image_Load_Opts structure for load options; can be NULL.
 * @param[out] error Pointer to an integer where the load error code will be stored.
 * @return A pointer to the loaded RGBA_Image, or NULL on failure.
 *         The image is managed by the cache.
 * If `f` is NULL, sets `*error` to `EVAS_LOAD_ERROR_GENERIC` and returns NULL.
 * Otherwise, requests the image from the global cache `eci` using `evas_cache_image_mmap_request`.
 */
EVAS_API RGBA_Image *
evas_common_load_image_from_mmap(Eina_File *f, const char *key,
                                 Evas_Image_Load_Opts *lo, int *error)
{
   if (!f)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        return NULL;
     }
   return (RGBA_Image *) evas_cache_image_mmap_request(eci, f, key, lo, error);
}

/**
 * @brief Frees the Evas image cache by setting its size to 0.
 * @api
 * This effectively unloads all images from the cache that are not currently
 * referenced elsewhere.
 */
EVAS_API void
evas_common_image_cache_free(void)
{
   evas_common_image_set_cache(0);
}

/**
 * @brief Gets the global Evas_Cache_Image instance.
 * @api
 * @return A pointer to the global Evas_Cache_Image instance (`eci`).
 *         This can be used for more direct interaction with the cache.
 */
EVAS_API Evas_Cache_Image*
evas_common_image_cache_get(void)
{
   return eci;
}

/**
 * @brief Obtains a temporary RGBA_Image suitable for use as a line buffer.
 * @api
 *
 * Creates an RGBA_Image of 1 pixel height and the specified length.
 * The length is clamped to be between `EVAS_RGBA_LINE_BUFFER_MIN_LEN`
 * and `EVAS_RGBA_LINE_BUFFER_MAX_LEN` (though MAX_LEN is not currently enforced here,
 * only MIN_LEN). The image is created without an alpha channel by default.
 *
 * @param len The desired length (width) of the line buffer.
 * @return A pointer to the created RGBA_Image, or NULL if `len` is less than 1
 *         or if allocation fails.
 * @see evas_common_image_line_buffer_release()
 * @see evas_common_image_line_buffer_free()
 */
EVAS_API RGBA_Image *
evas_common_image_line_buffer_obtain(int len)
{
   if (len < 1) return NULL;
   if (len < EVAS_RGBA_LINE_BUFFER_MIN_LEN)
     len = EVAS_RGBA_LINE_BUFFER_MIN_LEN;
   return evas_common_image_create(len, 1);
}

/**
 * @brief Releases a temporary RGBA_Image previously obtained via `evas_common_image_line_buffer_obtain`.
 * @api
 * @param im The RGBA_Image to release.
 * Internally calls `_evas_common_rgba_image_delete` to free the image.
 */
EVAS_API void
evas_common_image_line_buffer_release(RGBA_Image *im)
{
   _evas_common_rgba_image_delete(&im->cache_entry);
}

/**
 * @brief Frees a temporary RGBA_Image previously obtained via `evas_common_image_line_buffer_obtain`.
 * @api
 * @param im The RGBA_Image to free.
 * This is an alias for `evas_common_image_line_buffer_release`.
 * Internally calls `_evas_common_rgba_image_delete` to free the image.
 */
EVAS_API void
evas_common_image_line_buffer_free(RGBA_Image *im)
{
   _evas_common_rgba_image_delete(&im->cache_entry);
}

/**
 * @brief Obtains a temporary RGBA_Image with an alpha channel, suitable for use as a line buffer.
 * @api
 *
 * Creates an RGBA_Image of 1 pixel height and the specified length, with an alpha channel.
 * The length is clamped to be at least `EVAS_ALPHA_LINE_BUFFER_MIN_LEN`.
 *
 * @param len The desired length (width) of the alpha line buffer.
 * @return A pointer to the created RGBA_Image (with alpha), or NULL if `len` is less than 1
 *         or if allocation fails.
 * @see evas_common_image_alpha_line_buffer_release()
 */
EVAS_API RGBA_Image *
evas_common_image_alpha_line_buffer_obtain(int len)
{
   if (len < 1) return NULL;
   if (len < EVAS_ALPHA_LINE_BUFFER_MIN_LEN)
     len = EVAS_ALPHA_LINE_BUFFER_MIN_LEN;
   return evas_common_image_alpha_create(len, 1);
}

/**
 * @brief Releases a temporary RGBA_Image (with alpha) previously obtained via `evas_common_image_alpha_line_buffer_obtain`.
 * @api
 * @param im The RGBA_Image to release.
 * Internally calls `_evas_common_rgba_image_delete` to free the image.
 */
EVAS_API void
evas_common_image_alpha_line_buffer_release(RGBA_Image *im)
{
   _evas_common_rgba_image_delete(&im->cache_entry);
}

/**
 * @brief Premultiplies the alpha channel of an image.
 * @api
 *
 * If the image has an alpha channel (`ie->flags.alpha` is set) and its pixel
 * data is available, this function converts its pixel data to premultiplied alpha
 * format. It supports `EVAS_COLORSPACE_ARGB8888` and `EVAS_COLORSPACE_AGRY88`.
 * After premultiplication, it counts the number of pixels that are either fully
 * transparent or fully opaque (`nas`). If this count suggests that the alpha
 * channel is sparse (i.e., `ALPHA_SPARSE_INV_FRACTION * nas >= total_pixels`),
 * the `ie->flags.alpha_sparse` flag is set.
 *
 * @param ie The Image_Entry whose pixel data is to be premultiplied.
 *           The image data must be loaded.
 */
EVAS_API void
evas_common_image_premul(Image_Entry *ie)
{
   DATA32 nas = 0;

   if (!ie) return;
   if (!evas_cache_image_pixels(ie)) return;
   if (!ie->flags.alpha) return;

   switch (ie->space)
     {
      case EVAS_COLORSPACE_ARGB8888:
        nas = evas_common_convert_argb_premul
          (evas_cache_image_pixels(ie), ie->w * ie->h);
        break;
      case EVAS_COLORSPACE_AGRY88:
        nas = evas_common_convert_ag_premul
          ((void *)evas_cache_image_pixels(ie), ie->w * ie->h);
      default: return;
     }
   if ((ALPHA_SPARSE_INV_FRACTION * nas) >= (ie->w * ie->h))
     ie->flags.alpha_sparse = 1;
}

/**
 * @brief Checks and sets the `alpha_sparse` flag for an image.
 * @api
 *
 * This function iterates through the pixels of an image (assuming ARGB8888 format
 * after `evas_cache_image_pixels` which usually normalizes to it). It counts
 * the number of pixels (`nas`) that are either fully transparent (alpha = 0)
 * or fully opaque (alpha = 0xFF).
 * If this count, scaled by `ALPHA_SPARSE_INV_FRACTION`, is greater than or
 * equal to the total number of pixels in the image, the `ie->flags.alpha_sparse`
 * flag is set to 1. This indicates that the alpha channel has large areas of
 * uniform transparency or opacity, which might allow for rendering optimizations.
 *
 * @param ie The Image_Entry to check. The image must have an alpha channel and
 *           its pixel data must be loaded and accessible.
 * @note This function assumes the pixel data is in a format where the alpha
 *       channel is in the most significant byte of a DATA32 (e.g., ARGB8888).
 */
EVAS_API void
evas_common_image_set_alpha_sparse(Image_Entry *ie)
{
   DATA32 *s, *se;
   DATA32 nas = 0;

   if (!ie) return;
   if (!evas_cache_image_pixels(ie)) return;
   if (!ie->flags.alpha) return;

   s = evas_cache_image_pixels(ie);
   if (!s) return;
   se = s + (ie->w * ie->h);
   while (s < se)
     {
        DATA32  p = *s & 0xff000000;

        if (!p || (p == 0xff000000)) nas++;
        s++;
     }
   if ((ALPHA_SPARSE_INV_FRACTION * nas) >= (ie->w * ie->h))
     ie->flags.alpha_sparse = 1;
}
