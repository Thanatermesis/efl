/* @file evas_image_load_dds.c
/**
 * @file evas_image_load_dds.c
 * @brief Evas module for loading Microsoft DirectDraw Surface (DDS) files.
 * @author Jean-Philippe ANDRE <jpeg@videolan.org>
 *
 * This module provides functionality to load DDS image files,
 * including support for S3TC (DXT) texture compression formats.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Evas_Loader.h"
#include "s3tc.h"

#ifdef _WIN32
# include <ddraw.h>
#endif

#define DDS_HEADER_SIZE 128 /**< Standard size of the DDS file header, excluding the "DDS " magic number. */

/**
 * @internal
 * @brief Internal structure for managing DDS loader state.
 *
 * This structure holds all necessary information for loading a DDS file,
 * including the file handle, image properties, and pixel format details.
 */
typedef struct _Evas_Loader_Internal Evas_Loader_Internal;
struct _Evas_Loader_Internal
{
   Eina_File *f; /**< Eina file handle for the opened DDS file. */

   Evas_Colorspace format; /**< Detected Evas colorspace of the DDS image (e.g., EVAS_COLORSPACE_RGB_S3TC_DXT1). */
   unsigned int stride;     /**< Calculated stride (bytes per row) for the main image data. */
   unsigned int block_size; /**< Size in bytes of a single S3TC block (e.g., 8 for DXT1, 16 for DXT2-5). */
   unsigned int data_size;  /**< Total size in bytes of the main image data (excluding header and mipmaps). */

   /**
    * @brief Pixel format information extracted from the DDS header.
    */
   struct {
      unsigned int flags;        /**< Flags indicating which members of the pixel format structure are valid (e.g., DDPF_FOURCC). */
      unsigned int fourcc;       /**< FourCC code for compressed formats (e.g., 'DXT1'). */
      unsigned int rgb_bitcount; /**< Bits per pixel for uncompressed RGB formats. */
      unsigned int r_mask;       /**< Red channel bitmask for uncompressed RGB formats. */
      unsigned int g_mask;       /**< Green channel bitmask for uncompressed RGB formats. */
      unsigned int b_mask;       /**< Blue channel bitmask for uncompressed RGB formats. */
      unsigned int a_mask;       /**< Alpha channel bitmask for uncompressed RGB formats. */
      // TODO: check mipmaps to load faster a small image :)
   } pf; /**< Pixel format details. */
};

#undef FOURCC
/**
 * @def FOURCC(a,b,c,d)
 * @brief Macro to generate a FourCC code from four characters.
 * Handles endianness automatically.
 * @param a First character.
 * @param b Second character.
 * @param c Third character.
 * @param d Fourth character.
 * @return The 32-bit FourCC code.
 * @example FOURCC('D','X','T','1') results in 0x31545844 on little-endian systems.
 */
#ifndef WORDS_BIGENDIAN
# define FOURCC(a,b,c,d) ((d << 24) | (c << 16) | (b << 8) | a)
#else
# define FOURCC(a,b,c,d) ((a << 24) | (b << 16) | (c << 8) | d)
#endif

#ifndef DIRECTDRAW_VERSION
// DIRECTDRAW_VERSION is defined in ddraw.h
// These definitions are from the MSDN reference.

/**
 * @brief Flags to indicate which members of a DDS_HEADER structure are valid.
 * These correspond to the `dwFlags` member of the `DDSURFACEDESC2` structure.
 */
enum DDSFlags {
   DDSD_CAPS = 0x1,        /**< Required in every .dds file. */
   DDSD_HEIGHT = 0x2,      /**< Required in every .dds file. */
   DDSD_WIDTH = 0x4,       /**< Required in every .dds file. */
   DDSD_PITCH = 0x8,       /**< Required when pitch is provided for an uncompressed texture. */
   DDSD_PIXELFORMAT = 0x1000, /**< Required in every .dds file. */
   DDSD_MIPMAPCOUNT = 0x20000, /**< Required in a mipmapped texture. */
   DDSD_LINEARSIZE = 0x80000, /**< Required when pitch is provided for a compressed texture. */
   DDSD_DEPTH = 0x800000   /**< Required in a depth texture. */
};

/**
 * @brief Flags to indicate the nature of the pixel data in a DDS_PIXELFORMAT structure.
 * These correspond to the `dwFlags` member of the `DDS_PIXELFORMAT` structure.
 */
enum DDSPixelFormatFlags {
   DDPF_ALPHAPIXELS = 0x1, /**< Texture contains alpha data; dwRGBAlphaBitMask is valid. */
   DDPF_ALPHA = 0x2,       /**< Used in some older DDS files for alpha channel only uncompressed data. */
   DDPF_FOURCC = 0x4,      /**< Texture is compressed (FourCC code is valid). */
   DDPF_RGB = 0x40,        /**< Texture contains uncompressed RGB data; dwRGBBitCount and the RGB masks are valid. */
   DDPF_YUV = 0x200,       /**< Texture contains uncompressed YUV data. */
   DDPF_LUMINANCE = 0x20000 /**< Texture contains uncompressed luminance data. */
};

/**
 * @brief Flags to indicate the capabilities of a DirectDraw surface.
 * These correspond to the `ddsCaps.dwCaps1` member of the `DDSURFACEDESC2` structure.
 */
enum DDSCaps {
   DDSCAPS_COMPLEX = 0x8,    /**< Optional; must be used on any file that contains more than one surface. */
   DDSCAPS_MIPMAP = 0x400000, /**< Optional; should be used for a mipmap. */
   DDSCAPS_TEXTURE = 0x1000  /**< Required; must be used on any file that contains a texture. */
};

#endif

/** @brief Supported Evas colorspaces for DXT1 RGB (no alpha) format. */
static const Evas_Colorspace cspaces_s3tc_dxt1_rgb[] = {
   EVAS_COLORSPACE_RGB_S3TC_DXT1,
   EVAS_COLORSPACE_ARGB8888 /**< Fallback/decode target colorspace. */
};

/** @brief Supported Evas colorspaces for DXT1 RGBA (1-bit alpha) format. */
static const Evas_Colorspace cspaces_s3tc_dxt1_rgba[] = {
   //EVAS_COLORSPACE_RGBA_S3TC_DXT1, /**< Native S3TC DXT1 with alpha. */
   EVAS_COLORSPACE_ARGB8888 /**< Fallback/decode target colorspace. */
};

/** @brief Supported Evas colorspaces for DXT2 (premultiplied alpha) format. */
static const Evas_Colorspace cspaces_s3tc_dxt2[] = {
   EVAS_COLORSPACE_RGBA_S3TC_DXT2, /**< Native S3TC DXT2. */
   EVAS_COLORSPACE_ARGB8888 /**< Fallback/decode target colorspace. */
};

/** @brief Supported Evas colorspaces for DXT3 (explicit alpha) format. */
static const Evas_Colorspace cspaces_s3tc_dxt3[] = {
   //EVAS_COLORSPACE_RGBA_S3TC_DXT3, /**< Native S3TC DXT3. */
   EVAS_COLORSPACE_ARGB8888 /**< Fallback/decode target colorspace. */
};

/** @brief Supported Evas colorspaces for DXT4 (premultiplied alpha) format. */
static const Evas_Colorspace cspaces_s3tc_dxt4[] = {
   EVAS_COLORSPACE_RGBA_S3TC_DXT4, /**< Native S3TC DXT4. */
   EVAS_COLORSPACE_ARGB8888 /**< Fallback/decode target colorspace. */
};

/** @brief Supported Evas colorspaces for DXT5 (interpolated alpha) format. */
static const Evas_Colorspace cspaces_s3tc_dxt5[] = {
   //EVAS_COLORSPACE_RGBA_S3TC_DXT5, /**< Native S3TC DXT5. */
   EVAS_COLORSPACE_ARGB8888 /**< Fallback/decode target colorspace. */
};

/**
 * @brief Opens a DDS file for loading.
 *
 * This function is called by Evas to open a DDS image file. It performs
 * basic validation of the file size and allocates an internal loader structure.
 *
 * @param[in] f Eina_File handle to the opened file.
 * @param[in] key Optional key associated with the image file.
 * @param[in] opts Load options for the image.
 * @param[out] animated Information about image animation (not used for DDS).
 * @param[out] error Pointer to an integer to store the error code on failure.
 *                   Set to EVAS_LOAD_ERROR_NONE on success.
 * @return A pointer to an Evas_Loader_Internal structure on success, NULL on failure.
 *         Example error codes:
 *         - EVAS_LOAD_ERROR_CORRUPT_FILE: If file size is too small.
 *         - EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED: If memory allocation fails.
 */
static void *
evas_image_load_file_open_dds(Eina_File *f, Eina_Stringshare *key EINA_UNUSED,
                              Evas_Image_Load_Opts *opts EINA_UNUSED,
                              Evas_Image_Animated *animated EINA_UNUSED,
                              int *error)
{
   Evas_Loader_Internal *loader;

   if (eina_file_size_get(f) <= DDS_HEADER_SIZE)
     {
        *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
        return NULL;
     }

   loader = calloc(1, sizeof (Evas_Loader_Internal));
   if (!loader)
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return NULL;
     }


   loader->f = eina_file_dup(f);
   if (!loader->f)
     {
        free(loader);
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return NULL;
     }

   return loader;
}

/**
 * @brief Closes a DDS file and frees associated resources.
 *
 * This function is called by Evas to close a previously opened DDS image file
 * and release any resources allocated by evas_image_load_file_open_dds().
 *
 * @param[in] loader_data Pointer to the Evas_Loader_Internal structure.
 */
static void
evas_image_load_file_close_dds(void *loader_data)
{
   Evas_Loader_Internal *loader = loader_data;

   if (loader->f) eina_file_close(loader->f);
   free(loader);
}

/**
 * @brief Reads a 32-bit unsigned integer (DWORD) from a memory mapped region.
 *
 * Advances the memory pointer by 4 bytes after reading.
 * This function assumes little-endian byte order.
 *
 * @param[in,out] m Pointer to a const char pointer, which points to the current
 *                  position in the memory map. This pointer is advanced by 4 bytes.
 * @return The 32-bit unsigned integer value read from memory.
 */
static inline unsigned int
_dword_read(const char **m)
{
   unsigned int val = *((unsigned int *) *m);
   *m += 4;
   return val;
}

#define FAIL() do { /*fprintf(stderr, "DDS: ERROR at %s:%d\n", __func__, __LINE__);*/ goto on_error; } while (0) /**< Macro to simplify error handling by jumping to the on_error label. */

/**
 * @brief Reads the header of a DDS file and populates image properties.
 *
 * This function is called by Evas to read the DDS header, validate it,
 * and extract image properties like width, height, alpha presence, and
 * supported colorspaces.
 *
 * @param[in] loader_data Pointer to the Evas_Loader_Internal structure.
 * @param[out] prop Pointer to an Emile_Image_Property structure to be filled
 *                  with image properties.
 *                  - `prop->w`: width of the image.
 *                  - `prop->h`: height of the image.
 *                  - `prop->alpha`: EINA_TRUE if alpha channel is present, EINA_FALSE otherwise.
 *                  - `prop->cspaces`: array of supported Evas_Colorspace.
 * @param[out] error Pointer to an integer to store the error code on failure.
 *                   Set to EVAS_LOAD_ERROR_NONE on success.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *         Example error codes:
 *         - EVAS_LOAD_ERROR_CORRUPT_FILE: If the header is malformed or essential data is missing.
 *         - EVAS_LOAD_ERROR_UNKNOWN_FORMAT: If the DDS format (e.g., DX10, uncompressed non-FourCC) is not supported.
 */
static Eina_Bool
evas_image_load_file_head_dds(void *loader_data,
                              Emile_Image_Property *prop,
                              int *error)
{
   static const unsigned int base_flags = /* 0x1007 */
         DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT; /**< Minimum required flags in DDS_HEADER.dwFlags. */

   Evas_Loader_Internal *loader = loader_data;
   unsigned int flags, height, width, pitchOrLinearSize, caps, caps2;
   Eina_Bool has_linearsize, has_mipmapcount;
   const char *m;
   char *map;

   map = eina_file_map_all(loader->f, EINA_FILE_SEQUENTIAL);
   if (!map)
     {
        *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
        return EINA_FALSE;
     }

   m = map;

   *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
   if (strncmp(m, "DDS ", 4) != 0)
     // TODO: Add support for DX10
     goto on_error;
   m += 4;

   // Read DDS_HEADER
   if (_dword_read(&m) != 124)
     FAIL();

   flags = _dword_read(&m);
   if ((flags & base_flags) != (base_flags))
     FAIL();

   if ((flags & ~(DDSD_MIPMAPCOUNT | DDSD_LINEARSIZE)) != base_flags)
     {
        // TODO: A lot of modes are not supported.
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        FAIL();
     }

   has_linearsize = !!(flags & DDSD_LINEARSIZE);
   if (!has_linearsize)
     FAIL();

   has_mipmapcount = !!(flags & DDSD_MIPMAPCOUNT);
   (void) has_mipmapcount; // We don't really care about it.

   height = _dword_read(&m);
   width = _dword_read(&m);
   pitchOrLinearSize = _dword_read(&m);
   if (!width || !height)
     FAIL();

   // Skip depth & mipmap count + reserved[11]
   m += 13 * sizeof(unsigned int);
   // Entering DDS_PIXELFORMAT ddspf
   if (_dword_read(&m) != 32)
     FAIL();
   loader->pf.flags = _dword_read(&m);
   if (!(loader->pf.flags & DDPF_FOURCC))
     FAIL(); // Unsupported (uncompressed formats may not have a FOURCC)
   loader->pf.fourcc = _dword_read(&m);
   loader->block_size = 16;
   switch (loader->pf.fourcc)
     {
      case FOURCC('D', 'X', 'T', '1'):
        loader->block_size = 8;
        if ((loader->pf.flags & DDPF_ALPHAPIXELS) == 0)
          {
             prop->alpha = EINA_FALSE;
             prop->cspaces = cspaces_s3tc_dxt1_rgb;
             loader->format = EVAS_COLORSPACE_RGB_S3TC_DXT1;
          }
        else
          {
             prop->alpha = EINA_TRUE;
             prop->cspaces = cspaces_s3tc_dxt1_rgba;
             loader->format = EVAS_COLORSPACE_RGBA_S3TC_DXT1;
          }
        break;
      case FOURCC('D', 'X', 'T', '2'):
        loader->format = EVAS_COLORSPACE_RGBA_S3TC_DXT2;
        prop->alpha = EINA_TRUE;
        prop->cspaces = cspaces_s3tc_dxt2;
        break;
      case FOURCC('D', 'X', 'T', '3'):
        loader->format = EVAS_COLORSPACE_RGBA_S3TC_DXT3;
        prop->alpha = EINA_TRUE;
        prop->cspaces = cspaces_s3tc_dxt3;
        break;
      case FOURCC('D', 'X', 'T', '4'):
        loader->format = EVAS_COLORSPACE_RGBA_S3TC_DXT4;
        prop->alpha = EINA_TRUE;
        prop->cspaces = cspaces_s3tc_dxt4;
        break;
      case FOURCC('D', 'X', 'T', '5'):
        loader->format = EVAS_COLORSPACE_RGBA_S3TC_DXT5;
        prop->alpha = EINA_TRUE;
        prop->cspaces = cspaces_s3tc_dxt5;
        break;
      case FOURCC('D', 'X', '1', '0'):
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        FAIL();
      default:
        // TODO: Implement decoding support for uncompressed formats
        FAIL();
     }
   loader->pf.rgb_bitcount = _dword_read(&m);
   loader->pf.r_mask = _dword_read(&m);
   loader->pf.g_mask = _dword_read(&m);
   loader->pf.b_mask = _dword_read(&m);
   loader->pf.a_mask = _dword_read(&m);
   caps = _dword_read(&m);
   if ((caps & DDSCAPS_TEXTURE) == 0)
     {
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        FAIL();
     }
   caps2 = _dword_read(&m);
   if (caps2 != 0)
     {
        // Cube maps not supported
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        FAIL();
     }
   // Since the rest is unused, just ignore it.

   loader->stride = ((width + 3) >> 2) * loader->block_size;
   loader->data_size = loader->stride * ((height + 3) >> 2);
   if (loader->data_size != pitchOrLinearSize)
     FAIL(); // Invalid size!

   // Check file size
   if (eina_file_size_get(loader->f) < (DDS_HEADER_SIZE + loader->data_size))
     FAIL();

   prop->h = height;
   prop->w = width;
   prop->borders.l = 4;
   prop->borders.t = 4;
   prop->borders.r = 4 - (prop->w & 0x3);
   prop->borders.b = 4 - (prop->h & 0x3);
   *error = EVAS_LOAD_ERROR_NONE;

on_error:
   eina_file_map_free(loader->f, map);
   return (*error == EVAS_LOAD_ERROR_NONE);
}

/**
 * @internal
 * @brief Loads S3TC compressed image data with borders.
 *
 * This function handles loading of S3TC (DXT) compressed data when Evas requests
 * the image in its native compressed format, including handling of border pixels
 * required by S3TC decoding. It copies the main image data and then generates
 * border blocks by flipping edge blocks.
 *
 * @param[in] loader Pointer to the Evas_Loader_Internal structure.
 * @param[in] prop Pointer to the Emile_Image_Property structure containing image properties.
 *                 The `prop->cspace` must match `loader->format`.
 * @param[in] map Pointer to the memory-mapped DDS file content.
 * @param[out] pixels Pointer to the destination buffer where the S3TC data (including borders) will be written.
 *                    The layout is expected to accommodate border pixels around the main image data.
 *                    Example for a 4x4 block image (W pixels wide, H pixels high):
 *                    - Top border row of blocks
 *                    - Left border block | Main image block 0,0 | Main image block 1,0 | ... | Right border block
 *                    - Left border block | Main image block 0,1 | Main image block 1,1 | ... | Right border block
 *                    - ...
 *                    - Bottom border row of blocks
 * @param[out] error Pointer to an integer to store the error code on failure.
 *                   Set to EVAS_LOAD_ERROR_NONE on success.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *         Example error codes:
 *         - EVAS_LOAD_ERROR_GENERIC: If `loader->format` doesn't match `prop->cspace` or an unknown format is encountered.
 *         - EVAS_LOAD_ERROR_CORRUPT_FILE: If the file is smaller than expected.
 */
static Eina_Bool
_dds_data_load(Evas_Loader_Internal *loader, Emile_Image_Property *prop,
               unsigned char *map, void *pixels, int *error)
{
   const unsigned char *src;
   int bsize = 16, srcstride, dststride, w, h;
   unsigned char *dst;

   void (* flip) (unsigned char *dst_block, const unsigned char *src_block, int vertical_flip); /**< Function pointer for S3TC block flipping. */

   *error = EVAS_LOAD_ERROR_GENERIC;

   if (loader->format != prop->cspace)
     FAIL(); // Requested colorspace must be the native S3TC format.

   switch (loader->format)
     {
      case EVAS_COLORSPACE_RGB_S3TC_DXT1:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT1:
        flip = s3tc_encode_dxt1_flip;
        bsize = 8; // DXT1 uses 8 bytes per 4x4 block.
        break;
      case EVAS_COLORSPACE_RGBA_S3TC_DXT2:
        flip = s3tc_encode_dxt2_rgba_flip;
        bsize = 16; // DXT2 uses 16 bytes per 4x4 block.
        break;
      case EVAS_COLORSPACE_RGBA_S3TC_DXT3:
        flip = s3tc_encode_dxt3_rgba_flip;
        bsize = 16; // DXT3 uses 16 bytes per 4x4 block.
        break;
      case EVAS_COLORSPACE_RGBA_S3TC_DXT4:
        flip = s3tc_encode_dxt4_rgba_flip;
        bsize = 16; // DXT4 uses 16 bytes per 4x4 block.
        break;
      case EVAS_COLORSPACE_RGBA_S3TC_DXT5:
        flip = s3tc_encode_dxt5_rgba_flip;
        bsize = 16; // DXT5 uses 16 bytes per 4x4 block.
        break;
      default: FAIL(); // Should not happen if header parsing was correct.
     }

   src = map + DDS_HEADER_SIZE; // Start of pixel data in the mapped file.
   w = prop->w;
   h = prop->h;
   srcstride = ((prop->w + 3) / 4) * bsize;
   dststride = ((prop->w + prop->borders.l + prop->borders.r) / 4) * bsize;

   // asserts
   EINA_SAFETY_ON_FALSE_GOTO(prop->borders.l == 4, on_error);
   EINA_SAFETY_ON_FALSE_GOTO(prop->borders.t == 4, on_error);
   EINA_SAFETY_ON_FALSE_GOTO(prop->borders.r == (4 - (w & 0x3)), on_error);
   EINA_SAFETY_ON_FALSE_GOTO(prop->borders.b == (4 - (h & 0x3)), on_error);

   if (eina_file_size_get(loader->f) <
       (size_t) (DDS_HEADER_SIZE + srcstride * h / 4))
     {
        *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
        goto on_error;
     }

   // First, copy the real data
   for (int y = 0; y < h; y += 4)
     {
        dst = ((unsigned char *) pixels) + ((y / 4) + 1) * dststride + bsize;
        memcpy(dst, src, srcstride);
        src += srcstride;
     }
   // Top
   for (int x = 0; x < w; x += 4)
     {
        src = map + DDS_HEADER_SIZE + (x / 4) * bsize;
        dst = ((unsigned char *) pixels) + ((x / 4) + 1) * bsize;
        flip(dst, src, EINA_TRUE);
     }
   // Left
   for (int y = 0; y < h; y += 4)
     {
        src = map + DDS_HEADER_SIZE + (y / 4) * srcstride;
        dst = ((unsigned char *) pixels) + ((y / 4) + 1) * dststride;
        flip(dst, src, EINA_FALSE);
     }
   // Top-left
   dst = pixels;
   src = dst + bsize;
   flip(dst, src, EINA_FALSE);
   // Right
   if ((prop->w & 0x3) == 0)
     {
        for (int y = 0; y < h; y += 4)
          {
             src = map + DDS_HEADER_SIZE + ((y / 4) + 1) * srcstride - bsize;
             dst = ((unsigned char *) pixels) + ((y / 4) + 2) * dststride - bsize;
             flip(dst, src, EINA_FALSE);
          }
        // Top-right
        dst = ((unsigned char *) pixels) + dststride - bsize;
        src = dst - bsize;
        flip(dst, src, EINA_FALSE);
     }
   // Bottom
   if ((prop->h & 0x3) == 0)
     {
        for (int x = 0; x < w; x += 4)
          {
             src = map + DDS_HEADER_SIZE + ((h / 4) - 1) * srcstride + (x / 4) * bsize;
             dst = ((unsigned char *) pixels) + ((h / 4) + 1) * dststride + ((x / 4) + 1) * bsize;
             flip(dst, src, EINA_TRUE);
          }
        // Bottom-left
        dst = ((unsigned char *) pixels) + ((h / 4) + 1) * dststride;
        src = dst + bsize;
        flip(dst, src, EINA_FALSE);
        if ((prop->w & 0x3) == 0)
          {
             // Bottom-right
             dst = ((unsigned char *) pixels) + ((h / 4) + 2) * dststride - bsize;
             src = dst - bsize;
             flip(dst, src, EINA_FALSE);
          }
     }

   *error = EVAS_LOAD_ERROR_NONE;

on_error:
   eina_file_map_free(loader->f, (void *) map);
   return (*error == EVAS_LOAD_ERROR_NONE);
}

/**
 * @brief Loads the image data from a DDS file.
 *
 * This function is called by Evas to load the actual pixel data.
 * If the requested colorspace (`prop->cspace`) is a native S3TC format,
 * it calls `_dds_data_load()` to copy the compressed data with borders.
 * If the requested colorspace is ARGB8888, it decodes the S3TC data
 * into an ARGB8888 pixel buffer.
 *
 * @param[in] loader_data Pointer to the Evas_Loader_Internal structure.
 * @param[in,out] prop Pointer to an Emile_Image_Property structure.
 *                     `prop->cspace` specifies the desired output colorspace.
 *                     `prop->premul` will be set based on the S3TC format if decoding to ARGB8888.
 * @param[out] pixels Pointer to the destination buffer for the pixel data.
 *                    If `prop->cspace` is native S3TC, this buffer receives S3TC blocks with borders.
 *                    If `prop->cspace` is ARGB8888, this buffer receives decoded 32-bit ARGB pixels.
 *                    Example for ARGB8888 output (pixels is `unsigned int*`):
 *                    `pixels[y * prop->w + x]` corresponds to the pixel at (x,y).
 * @param[out] error Pointer to an integer to store the error code on failure.
 *                   Set to EVAS_LOAD_ERROR_NONE on success.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *         Example error codes:
 *         - EVAS_LOAD_ERROR_CORRUPT_FILE: If file mapping fails or file is too small.
 *         - EVAS_LOAD_ERROR_GENERIC: For internal errors or unsupported decode paths.
 */
Eina_Bool
evas_image_load_file_data_dds(void *loader_data,
                              Emile_Image_Property *prop,
                              void *pixels,
                              int *error)
{
   void (*func) (unsigned int *bgra, const unsigned char *s3tc_block) = NULL; /**< Function pointer for S3TC block decoding. */
   Evas_Loader_Internal *loader = loader_data;
   unsigned int *pix = pixels; // Assuming ARGB8888 output if not native S3TC.
   unsigned char *map = NULL;
   const unsigned char *src;

   *error = EVAS_LOAD_ERROR_CORRUPT_FILE;

   map = eina_file_map_all(loader->f, EINA_FILE_WILLNEED);
   if (!map)
     return EINA_FALSE;

   src = map + DDS_HEADER_SIZE;
   if (eina_file_size_get(loader->f) < (DDS_HEADER_SIZE + loader->data_size))
     FAIL();

   if (prop->cspace != EVAS_COLORSPACE_ARGB8888)
     return _dds_data_load(loader, prop, map, pixels, error);

   // Decode to BGRA
   switch (loader->format)
     {
      case EVAS_COLORSPACE_RGB_S3TC_DXT1:
        func = s3tc_decode_dxt1_rgb;
        prop->premul = EINA_FALSE;
        break;
      case EVAS_COLORSPACE_RGBA_S3TC_DXT1:
        func = s3tc_decode_dxt1_rgba;
        prop->premul = EINA_FALSE;
        break;
      case EVAS_COLORSPACE_RGBA_S3TC_DXT2:
        func = s3tc_decode_dxt2_rgba;
        prop->premul = EINA_FALSE;
        break;
      case EVAS_COLORSPACE_RGBA_S3TC_DXT3:
        func = s3tc_decode_dxt3_rgba;
        prop->premul = EINA_TRUE;
        break;
      case EVAS_COLORSPACE_RGBA_S3TC_DXT4:
        func = s3tc_decode_dxt4_rgba;
        prop->premul = EINA_FALSE;
        break;
      case EVAS_COLORSPACE_RGBA_S3TC_DXT5:
        func = s3tc_decode_dxt5_rgba;
        prop->premul = EINA_TRUE;
        break;
      default:
        FAIL();
     }
   if (!func) FAIL();

   for (unsigned int y = 0; y < prop->h; y += 4)
     {
        int blockh = prop->h - y;
        if (blockh > 4) blockh = 4;

        for (unsigned int x = 0; x < prop->w; x += 4)
          {
             unsigned int bgra[16];
             int k, j;

             func(bgra, src);
             src += loader->block_size;

             j = prop->w - x;
             if (j > 4) j = 4;
             for (k = 0; k < blockh; k++)
               {
                  memcpy(pix + (((y + k) * prop->w) + x), bgra + (k * 4),
                         j * sizeof (unsigned int));
               };
          }
     }

   *error = EVAS_LOAD_ERROR_NONE;

on_error:
   eina_file_map_free(loader->f, (void *) map);
   return (*error == EVAS_LOAD_ERROR_NONE);
}

/**
 * @brief Structure defining the Evas image loader functions for DDS files.
 *
 * This structure provides Evas with the necessary function pointers to handle
 * opening, closing, reading header, and reading data for DDS image files.
 */
Evas_Image_Load_Func evas_image_load_dds_func =
{
  EVAS_IMAGE_LOAD_VERSION, /**< Loader API version. */
  evas_image_load_file_open_dds, /**< Function to open the image file. */
  evas_image_load_file_close_dds, /**< Function to close the image file. */
  (void*) evas_image_load_file_head_dds, /**< Function to read image header. */
  NULL, /**< Function to read image data (legacy, not used). */
  (void*) evas_image_load_file_data_dds, /**< Function to read image pixel data. */
  NULL, /**< Function to get animated image frame information (not applicable). */
  EINA_TRUE, /**< Indicates if the loader supports region loading (not fully utilized here for S3TC). */
  EINA_FALSE /**< Indicates if the loader supports loading into a user-provided buffer for compressed data (not used). */
};

/**
 * @brief Initializes the DDS image loader module.
 *
 * This function is called by Evas when the module is loaded. It registers
 * the loader functions with the Evas module system.
 *
 * @param[in] em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_image_load_dds_func);
   return 1;
}

/**
 * @brief Shuts down the DDS image loader module.
 *
 * This function is called by Evas when the module is unloaded.
 * Currently, it performs no specific cleanup.
 *
 * @param[in] em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
}

/**
 * @brief Evas module API structure for the DDS loader.
 *
 * This structure defines the module's API version, name, and entry points
 * for opening and closing the module.
 */
static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION, /**< Evas module API version. */
   "dds", /**< Module name. */
   "none", /**< Module license (placeholder). */
   {
     module_open, /**< Function to open/initialize the module. */
     module_close /**< Function to close/shutdown the module. */
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, dds); /**< Macro to define the Evas image loader module. */

#ifndef EVAS_STATIC_BUILD_DDS
EVAS_EINA_MODULE_DEFINE(image_loader, dds);
#endif
