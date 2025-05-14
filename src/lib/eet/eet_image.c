#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#ifdef __OpenBSD__
# include <sys/types.h>
#endif /* ifdef __OpenBSD__ */

#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>

#include "Eet.h"
#include "Eet_private.h"

#include "rg_etc1.h"

#ifdef BUILD_NEON
#include <arm_neon.h>
#endif

#ifndef WORDS_BIGENDIAN
/* x86 */
#define A_VAL(p) (((uint8_t *)(p))[3])
#define R_VAL(p) (((uint8_t *)(p))[2])
#define G_VAL(p) (((uint8_t *)(p))[1])
#define B_VAL(p) (((uint8_t *)(p))[0])
#else
/* ppc */
#define A_VAL(p) (((uint8_t *)(p))[0])
#define R_VAL(p) (((uint8_t *)(p))[1])
#define G_VAL(p) (((uint8_t *)(p))[2])
#define B_VAL(p) (((uint8_t *)(p))[3])
#endif

#define ARGB_JOIN(a,r,g,b) \
        (((a) << 24) + ((r) << 16) + ((g) << 8) + (b))

#define OFFSET_BLOCK_SIZE 4
#define OFFSET_ALGORITHM 5
#define OFFSET_OPTIONS 6
#define OFFSET_WIDTH 8
#define OFFSET_HEIGHT 12
#define OFFSET_BLOCKS 16

/*---*/

typedef struct _JPEG_error_mgr *emptr;

/*---*/

/**
 * @brief Custom error manager for libjpeg.
 * This structure extends the standard jpeg_error_mgr to include a jmp_buf
 * for error handling via longjmp.
 */
struct _JPEG_error_mgr
{
   struct jpeg_error_mgr pub; /**< Standard libjpeg error manager. */
   jmp_buf               setjmp_buffer; /**< Buffer for longjmp, used to return to a safe point on error. */
};

/**
 * @brief Custom libjpeg destination manager for writing to an in-memory buffer.
 * This structure is used to direct libjpeg's output to a dynamically allocated
 * memory buffer, which can grow as needed.
 */
struct jpeg_membuf_dst
{
   struct jpeg_destination_mgr pub; /**< Standard libjpeg destination manager. */

   void                      **dst_buf; /**< Pointer to the pointer that will hold the final compressed data buffer. */
   size_t                     *dst_len; /**< Pointer to the variable that will hold the length of the final compressed data. */

   unsigned char              *buf;    /**< Current internal buffer for compressed data. */
   size_t                      len;     /**< Current allocated size of the internal buffer. */
   int                         failed;  /**< Flag indicating if buffer allocation failed. */
   struct jpeg_membuf_dst     *self;   /**< Self-pointer, used in term_destination to access the full struct. */
};

/**
 * @brief Initializes the custom libjpeg destination manager.
 * This function is a callback for libjpeg, called before writing any data.
 * @param cinfo Pointer to the jpeg_compress_struct.
 */
static void
_eet_jpeg_membuf_dst_init(j_compress_ptr cinfo)
{
   /* FIXME: Use eina attribute */
   (void)cinfo;
}

/**
 * @brief Flushes the output buffer for the custom libjpeg destination manager.
 * This function is a callback for libjpeg, called when the output buffer is full.
 * It attempts to reallocate the buffer to twice its current size. If reallocation
 * fails, it sets the failed flag.
 * @param cinfo Pointer to the jpeg_compress_struct.
 * @return TRUE if the buffer was successfully flushed (or if an error occurred and
 *         it's pretending to be flushed to allow libjpeg to terminate), FALSE if
 *         libjpeg should continue normally after reallocation.
 */
static boolean
_eet_jpeg_membuf_dst_flush(j_compress_ptr cinfo)
{
   struct jpeg_membuf_dst *dst = (struct jpeg_membuf_dst *)cinfo->dest;
   unsigned char *buf;

   if (dst->len >= 0x40000000 ||
       !(buf = realloc(dst->buf, dst->len * 2)))
     {
        dst->failed = 1;
        dst->pub.next_output_byte = dst->buf;
        dst->pub.free_in_buffer = dst->len;
        return TRUE;
     }

      dst->pub.next_output_byte =
        buf + ((unsigned char *)dst->pub.next_output_byte - dst->buf);
      dst->buf = buf;
      dst->pub.free_in_buffer += dst->len;
      dst->len *= 2;

      return FALSE;
}

/**
 * @brief Terminates the custom libjpeg destination manager.
 * This function is a callback for libjpeg, called after all data has been written.
 * It finalizes the output buffer and length, or clears them if an error occurred.
 * It also frees the destination manager structure itself.
 * @param cinfo Pointer to the jpeg_compress_struct.
 */
static void
_eet_jpeg_membuf_dst_term(j_compress_ptr cinfo)
{
   struct jpeg_membuf_dst *dst = ((struct jpeg_membuf_dst *)cinfo->dest)->self;

   if (dst->failed)
     {
        *dst->dst_buf = NULL;
        *dst->dst_len = 0;
        free(dst->buf);
     }
   else
     {
        *dst->dst_buf = dst->buf;
        *dst->dst_len = (unsigned char *)dst->pub.next_output_byte - dst->buf;
     }

   free(dst);
   cinfo->dest = NULL;
}

/**
 * @brief Sets up a custom libjpeg destination manager for in-memory compression.
 * This function allocates and initializes a jpeg_membuf_dst structure and
 * configures the provided jpeg_compress_struct to use it.
 *
 * @param cinfo Pointer to the jpeg_compress_struct to configure.
 * @param buf Pointer to a void pointer that will receive the address of the
 *            compressed data buffer upon successful compression. The caller
 *            is responsible for freeing this buffer.
 * @param len Pointer to a size_t variable that will receive the length of
 *            the compressed data.
 * @return 0 on success, -1 on allocation failure.
 */
static int
eet_jpeg_membuf_dst(j_compress_ptr cinfo,
                    void         **buf,
                    size_t        *len)
{
   struct jpeg_membuf_dst *dst;

   dst = calloc(1, sizeof(*dst));
   if (!dst)
     return -1;

   dst->buf = malloc(32768);
   if (!dst->buf)
     {
        free(dst);
        return -1;
     }

   dst->self = dst;
   dst->len = 32768;

   cinfo->dest = &dst->pub;
   dst->pub.init_destination = _eet_jpeg_membuf_dst_init;
   dst->pub.empty_output_buffer = _eet_jpeg_membuf_dst_flush;
   dst->pub.term_destination = _eet_jpeg_membuf_dst_term;
   dst->pub.free_in_buffer = dst->len;
   dst->pub.next_output_byte = dst->buf;
   dst->dst_buf = buf;
   dst->dst_len = len;
   dst->failed = 0;

   return 0;
}

/*---*/

/**
 * @brief Decodes the header of a JPEG image from memory.
 * This function reads JPEG image metadata (width, height, colorspaces)
 * without decompressing the entire image.
 *
 * @param data Pointer to the JPEG image data in memory.
 * @param size Size of the JPEG image data in bytes.
 * @param w Pointer to an unsigned int to store the image width.
 * @param h Pointer to an unsigned int to store the image height.
 * @param cspaces Pointer to a const Eet_Colorspace pointer to store available colorspaces.
 *                Can be NULL if not needed.
 * @return 1 on success, 0 on failure (e.g., invalid JPEG data, memory error).
 */
static int
eet_data_image_jpeg_header_decode(const void   *data,
                                  int           size,
                                  unsigned int *w,
                                  unsigned int *h,
                                  const Eet_Colorspace **cspaces);

/**
 * @brief Decodes JPEG image data into an RGB(A) buffer.
 * This function decompresses JPEG image data from memory into a raw pixel buffer.
 * The output format depends on the `cspace` parameter.
 *
 * @param data Pointer to the JPEG image data in memory.
 * @param size Size of the JPEG image data in bytes.
 * @param src_x X-coordinate of the top-left corner of the source region to decode (currently unused, full image decoded).
 * @param src_y Y-coordinate of the top-left corner of the source region to decode (currently unused, full image decoded).
 * @param d Pointer to the destination buffer for decoded pixel data.
 *          The buffer should be large enough to hold `w * h * (bytes_per_pixel)` based on `cspace`.
 *          Example for ARGB8888: `w * h * 4` bytes.
 * @param w Width of the region to decode.
 * @param h Height of the region to decode.
 * @param cspace The desired colorspace for the output data (e.g., EET_COLORSPACE_ARGB8888).
 * @return 1 on success, 0 on failure.
 */
static int
eet_data_image_jpeg_rgb_decode(const void   *data,
                               int           size,
                               unsigned int  src_x,
                               unsigned int  src_y,
                               unsigned int *d,
                               unsigned int  w,
                               unsigned int  h,
                               Eet_Colorspace cspace);

/**
 * @brief Decodes the alpha channel from a separate JPEG image and merges it into an existing pixel buffer.
 * This function is used when alpha is stored as a separate greyscale JPEG.
 * It decodes this greyscale JPEG and writes its values into the alpha component
 * of the `pixels` buffer.
 *
 * @param data Pointer to the JPEG image data (alpha channel) in memory.
 * @param size Size of the JPEG image data in bytes.
 * @param src_x X-coordinate of the top-left corner of the source region to decode (currently unused).
 * @param src_y Y-coordinate of the top-left corner of the source region to decode (currently unused).
 * @param d Pointer to the destination pixel buffer (e.g., ARGB8888 or AGRY88 data)
 *               which already contains color information. The alpha channel of this buffer
 *               will be updated. In the implementation this parameter is named `pixels`.
 *               Example for ARGB8888: `unsigned int *pixels; pixels[i] = (alpha << 24) | (existing_rgb);`
 *               Example for AGRY88: `unsigned short *pixels; pixels[i] = (alpha << 8) | (existing_grey);`
 * @param w Width of the region to decode.
 * @param h Height of the region to decode.
 * @param cspace The colorspace of the `pixels` buffer (e.g., EMILE_COLORSPACE_ARGB8888, EMILE_COLORSPACE_AGRY88).
 *               This determines how the alpha is merged.
 * @return 1 on success, 0 on failure.
 */
static int
eet_data_image_jpeg_alpha_decode(const void   *data,
                                 int           size,
                                 unsigned int  src_x,
                                 unsigned int  src_y,
                                 void *d,
                                 unsigned int  w,
                                 unsigned int  h,
                                 Eet_Colorspace cspace);

/**
 * @brief Converts raw ARGB pixel data to Eet's internal lossless image format.
 * The format consists of a header followed by the raw pixel data.
 * Header (32 bytes total, 8 integers):
 * - `[0]`: Magic number (0xac1dfeed)
 * - `[1]`: Width
 * - `[2]`: Height
 * - `[3]`: Alpha flag (1 if alpha, 0 otherwise)
 * - `[4-7]`: Reserved (0)
 * Pixel data follows immediately, in ARGB8888 format. Endianness is handled.
 *
 * @param[out] size Pointer to an integer to store the size of the converted data.
 * @param[in] data Pointer to the raw ARGB8888 pixel data.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param alpha Integer flag indicating if the image has an alpha channel (1 for alpha, 0 for no alpha).
 * @return A pointer to the allocated buffer containing the converted image data.
 *         The caller is responsible for freeing this buffer. Returns NULL on failure.
 */
static void *
eet_data_image_lossless_convert(int         *size,
                                const void  *data,
                                unsigned int w,
                                unsigned int h,
                                int          alpha);

/**
 * @brief Converts raw ARGB pixel data to Eet's internal compressed lossless image format.
 * The format consists of a header followed by the compressed pixel data.
 * Header (32 bytes total, 8 integers):
 * - `[0]`: Magic number (0xac1dfeed)
 * - `[1]`: Width
 * - `[2]`: Height
 * - `[3]`: Alpha flag (1 if alpha, 0 otherwise)
 * - `[4]`: Compression type (e.g., EMILE_LZ4HC)
 * - `[5-7]`: Reserved (0)
 * Compressed pixel data (ARGB8888 compressed) follows. Endianness of header is handled.
 * If compression results in a larger size, it returns NULL and sets *size to -1.
 *
 * @param[out] size Pointer to an integer to store the size of the converted data, or -1 on compression failure.
 * @param[in] data Pointer to the raw ARGB8888 pixel data.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param alpha Integer flag indicating if the image has an alpha channel.
 * @param compression Integer specifying the compression algorithm (e.g., from Emile_Compressor).
 * @return A pointer to the allocated buffer containing the converted and compressed image data.
 *         The caller is responsible for freeing this buffer. Returns NULL on failure or if compression is ineffective.
 */
static void *
eet_data_image_lossless_compressed_convert(int         *size,
                                           const void  *data,
                                           unsigned int w,
                                           unsigned int h,
                                           int          alpha,
                                           int          compression);

/**
 * @brief Converts raw ARGB pixel data to JPEG format.
 * This function encodes RGB data (alpha channel is ignored if present in input,
 * but the `alpha` parameter to this function should be 0).
 *
 * @param[out] size Pointer to an integer to store the size of the JPEG data.
 * @param[in] data Pointer to the raw ARGB8888 pixel data.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param alpha Flag indicating presence of alpha in source, but for this function it's effectively ignored for output (should be 0).
 * @param quality JPEG quality (0-100).
 * @return A pointer to the allocated buffer containing the JPEG image data.
 *         The caller is responsible for freeing this buffer. Returns NULL on failure.
 */
static void *
eet_data_image_jpeg_convert(int         *size,
                            const void  *data,
                            unsigned int w,
                            unsigned int h,
                            int          alpha,
                            int          quality);

/**
 * @brief Converts raw ARGB pixel data to a custom JPEG format with a separate alpha channel.
 * The output format consists of a header, followed by JPEG-compressed RGB data,
 * followed by JPEG-compressed alpha data (as greyscale).
 * Header (12 bytes total, 3 integers):
 * - `[0]`: Magic number (0xbeeff00d)
 * - `[1]`: Size of the RGB JPEG data (sz1)
 * - `[2]`: Size of the alpha JPEG data (sz2)
 * RGB JPEG data (sz1 bytes) follows.
 * Alpha JPEG data (sz2 bytes) follows.
 *
 * @param[out] size Pointer to an integer to store the total size of the converted data.
 * @param[in] data Pointer to the raw ARGB8888 pixel data.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param alpha Integer flag indicating if the image has an alpha channel (should be 1 for this function).
 * @param quality JPEG quality (0-100) for both RGB and alpha JPEGs.
 * @return A pointer to the allocated buffer containing the custom JPEG-with-alpha data.
 *         The caller is responsible for freeing this buffer. Returns NULL on failure.
 */
static void *
eet_data_image_jpeg_alpha_convert(int         *size,
                                  const void  *data,
                                  unsigned int w,
                                  unsigned int h,
                                  int          alpha,
                                  int          quality);

/*---*/

#define SWAP64(x) x = eina_swap64(x)
#define SWAP32(x) x = eina_swap32(x)
#define SWAP16(x) x = eina_swap16(x)

#ifdef CONV8
# undef CONV8
#endif /* ifdef CONV8 */
#ifdef CONV16
# undef CONV16
#endif /* ifdef CONV16 */
#ifdef CONV32
# undef CONV32
#endif /* ifdef CONV32 */
#ifdef CONV64
# undef CONV64
#endif /* ifdef CONV64 */

#define CONV8(x)
#define CONV16(x) do {if (_eet_image_words_bigendian) {SWAP16(x); }} while(0)
#define CONV32(x) do {if (_eet_image_words_bigendian) {SWAP32(x); }} while(0)
#define CONV64(x) do {if (_eet_image_words_bigendian) {SWAP64(x); }} while(0)

/*---*/

/** Global variable to cache the system's endianness. -1: unknown, 0: little-endian, 1: big-endian. */
static int _eet_image_words_bigendian = -1;

/**
 * @brief Checks and caches the system's endianness.
 * This function determines if the system is big-endian or little-endian
 * and stores the result in the global `_eet_image_words_bigendian` variable
 * to avoid repeated checks.
 */
static inline void
_eet_image_endian_check(void)
{
   if (_eet_image_words_bigendian == -1)
     {
        unsigned long int v;

        v = eina_htonl(0x12345678);
        if (v == 0x12345678)
          _eet_image_words_bigendian = 1;
        else
          _eet_image_words_bigendian = 0;
     }
}

/**
 * @brief Swaps the byte order of an array of 32-bit integers if the system is big-endian.
 * This is typically used when reading/writing data from/to a format that assumes
 * little-endian byte order.
 *
 * @param data Pointer to the array of 32-bit integers (e.g., `unsigned int *`).
 * @param length The number of 32-bit integers in the array.
 */
static inline void
_eet_image_endian_swap(void *data, unsigned int length)
{
   if (_eet_image_words_bigendian)
     {
        unsigned int *buffer = data;
        unsigned int i;

        for (i = 0; i < length; i++) SWAP32(buffer[i]);
     }
}

/*---*/

/**
 * @brief libjpeg error exit callback.
 * This function is called by libjpeg when a fatal error occurs.
 * It formats the error message, logs it, and then performs a longjmp
 * to the stored `setjmp_buffer` in the custom error manager.
 * @param cinfo Pointer to the libjpeg common struct.
 */
static void
_eet_image_jpeg_error_exit_cb(j_common_ptr cinfo)
{
   char buffer[JMSG_LENGTH_MAX];
   emptr errmgr;

   (*cinfo->err->format_message)(cinfo, buffer);
   ERR("%s", buffer);
   errmgr = (emptr)cinfo->err;
   longjmp(errmgr->setjmp_buffer, 1);
}

/**
 * @brief libjpeg output message callback.
 * This function is called by libjpeg to output informational messages.
 * It formats the message and logs it as an error.
 * @param cinfo Pointer to the libjpeg common struct.
 */
static void
_eet_image_jpeg_output_message_cb(j_common_ptr cinfo)
{
   char buffer[JMSG_LENGTH_MAX];
   /* emptr errmgr; */

   (*cinfo->err->format_message)(cinfo, buffer);
   ERR("%s", buffer);
   /*
   errmgr = (emptr)cinfo->err;
   longjmp(errmgr->setjmp_buffer, 1);
   */
}

/**
 * @brief libjpeg emit message callback.
 * This function is called by libjpeg to emit warnings or trace messages.
 * It formats the message and logs it as a warning or informational message
 * based on the `msg_level` and trace_level settings.
 * @param cinfo Pointer to the libjpeg common struct.
 * @param msg_level The message level (negative for warnings, positive for trace).
 */
static void
_eet_image_jpeg_emit_message_cb(j_common_ptr cinfo,
                                int          msg_level)
{
   char buffer[JMSG_LENGTH_MAX];
   struct jpeg_error_mgr *err;
   /* emptr errmgr; */

   err = cinfo->err;
   if (msg_level < 0)
     {
        if ((err->num_warnings == 0) || (err->trace_level >= 3))
          {
             (*cinfo->err->format_message)(cinfo, buffer);
             WRN("%s", buffer);
          }
        err->num_warnings++;
     }
   else
     {
        if (err->trace_level >= msg_level)
          {
             (*cinfo->err->format_message)(cinfo, buffer);
             INF("%s", buffer);
          }
     }
   /*
   errmgr = (emptr)cinfo->err;
   longjmp(errmgr->setjmp_buffer, 1);
   */
}

static int
eet_data_image_jpeg_header_decode(const void   *data,
                                  int           size,
                                  unsigned int *w,
                                  unsigned int *h,
                                  const Eet_Colorspace **cspaces)
{
   Emile_Image_Load_Opts opts;
   Emile_Image_Property prop;
   Eina_Binbuf *bin;
   Emile_Image *image;
   Emile_Image_Load_Error error;
   int r = 0;

   bin = eina_binbuf_manage_new(data, size, EINA_TRUE);
   if (!bin) return 0;

   memset(&opts, 0, sizeof (opts));

   image = emile_image_jpeg_memory_open(bin, &opts, NULL, &error);
   if (!image) goto on_error;

   memset(&prop, 0, sizeof (prop));

   if (!emile_image_head(image, &prop, sizeof (prop), &error))
     goto on_error;

   *w = prop.w;
   *h = prop.h;

   if (cspaces) *cspaces = prop.cspaces;

   if (*w > 0 && *w <= 8192 &&
       *h > 0 && *h <= 8192)
     r = 1;

 on_error:
   emile_image_close(image);
   eina_binbuf_free(bin);

   return r;
}

static int
eet_data_image_jpeg_rgb_decode(const void   *data,
                               int           size,
                               unsigned int  src_x,
                               unsigned int  src_y,
                               unsigned int *d,
                               unsigned int  w,
                               unsigned int  h,
                               Eet_Colorspace cspace)
{
   Emile_Image_Load_Opts opts;
   Emile_Image_Property prop;
   Emile_Image *image;
   Eina_Binbuf *bin;
   Emile_Image_Load_Error error;
   int r = 0;

   /* FIXME: handle src_x, src_y correctly */
   if (!d)
     return 0;

   // Fix for ABI incompatibility between 1.10 and 1.11
   if (cspace == 8) cspace = 9;

   bin = eina_binbuf_manage_new(data, size, EINA_TRUE);
   if (!bin) return 0;

   memset(&opts, 0, sizeof (opts));
   opts.region.x = src_x;
   opts.region.y = src_y;
   opts.region.w = w;
   opts.region.h = h;

   image = emile_image_jpeg_memory_open(bin, &opts, NULL, &error);
   if (!image) goto on_error;

   memset(&prop, 0, sizeof (prop));

   if (!emile_image_head(image, &prop, sizeof (prop), &error))
     goto on_error;

   prop.cspace = cspace;

   if (!emile_image_data(image, &prop, sizeof (prop), d, &error))
     goto on_error;

   r = 1;

 on_error:
   emile_image_close(image);
   eina_binbuf_free(bin);

   return r;
}

static int
eet_data_image_jpeg_alpha_decode(const void   *data,
                                 int           size,
                                 unsigned int  src_x,
                                 unsigned int  src_y,
                                 void         *pixels,
                                 unsigned int  w,
                                 unsigned int  h,
                                 Eet_Colorspace cspace)
{
   Emile_Image_Load_Opts opts;
   Emile_Image_Property prop;
   Emile_Image *image;
   Eina_Binbuf *bin;
   unsigned char *remember = NULL, *tmp;
   Emile_Image_Load_Error error;
   int r = 0;
   unsigned int i;

   /* FIXME: handle src_x, src_y and row_stride correctly */
   if (!pixels)
     return 0;

   bin = eina_binbuf_manage_new(data, size, EINA_TRUE);
   if (!bin) return 0;

   memset(&opts, 0, sizeof (opts));
   opts.region.x = src_x;
   opts.region.y = src_y;
   opts.region.w = w;
   opts.region.h = h;

   image = emile_image_jpeg_memory_open(bin, &opts, NULL, &error);
   if (!image) goto on_error;

   memset(&prop, 0, sizeof (prop));

   if (!emile_image_head(image, &prop, sizeof (prop), &error))
     goto on_error;

   remember = tmp = malloc(sizeof (unsigned char) * w * h);
   if (!tmp) goto on_error;

   // Alpha should always be encoded as GRY8
   prop.cspace = EMILE_COLORSPACE_GRY8;

   if (!emile_image_data(image, &prop, sizeof (prop), tmp, &error))
     goto on_error;

   if (cspace == EMILE_COLORSPACE_AGRY88)
     {
        unsigned short *d = pixels;

        for (i = 0; i < w * h; i++)
          {
             *d = ((*d) & 0x00ff) |
               ((*tmp) << 8);
             tmp++;
             d++;
          }
     }
   else if (cspace == EMILE_COLORSPACE_ARGB8888)
     {
        unsigned int *d = pixels;

        for (i = 0; i < w * h; i++)
          {
             *d = ((*d) & 0x00ffffff) |
               ((*tmp) << 24);
             tmp++;
             d++;
          }
     }

   r = 1;

 on_error:
   free(remember);

   emile_image_close(image);
   eina_binbuf_free(bin);

   return r;
}

// FIXME: Importing two functions from evas here: premul & unpremul
/**
 * @brief Premultiplies ARGB pixel data.
 * For each pixel, R, G, and B components are multiplied by Alpha/255.
 * `data` is an array of pixels, where each pixel is an `unsigned int` in ARGB8888 format.
 * Example: `0xAARRGGBB`.
 *
 * @param data Pointer to the array of ARGB pixels to be premultiplied in place.
 * @param len The number of pixels in the data array.
 */
static void
_eet_argb_premul(unsigned int *data, unsigned int len)
{
   unsigned int *de = data + len;

   while (data < de)
     {
        unsigned int  a = 1 + (*data >> 24);

        *data = (*data & 0xff000000) +
          (((((*data) >> 8) & 0xff) * a) & 0xff00) +
          (((((*data) & 0x00ff00ff) * a) >> 8) & 0x00ff00ff);
        data++;
     }
}

/**
 * @brief Un-premultiplies ARGB pixel data.
 * For each pixel, R, G, and B components are divided by Alpha/255 (if Alpha > 0).
 * `data` is an array of pixels, where each pixel is an `unsigned int` in ARGB8888 format.
 * Example: `0xAARRGGBB`.
 * Includes a small optimization for repeated pixel values.
 *
 * @param data Pointer to the array of ARGB pixels to be un-premultiplied in place.
 * @param len The number of pixels in the data array.
 */
static void
_eet_argb_unpremul(unsigned int *data, unsigned int len)
{
   unsigned int *de = data + len;
   unsigned int p_val = 0x00000000, p_res = 0x00000000;

   while (data < de)
     {
        unsigned int  a = (*data >> 24) + 1;

        if (p_val == *data) *data = p_res;
        else
          {
             p_val = *data;
             if ((a > 1) && (a < 256))
               *data = ARGB_JOIN(a,
                                 (R_VAL(data) * 255) / a,
                                 (G_VAL(data) * 255) / a,
                                 (B_VAL(data) * 255) / a);
             else if (a == 1)
               *data = 0x00000000;
             p_res = *data;
          }
        data++;
     }
}

/**
 * @brief Decodes ETC1/ETC2 (TGV) compressed image data.
 * This function uses the Emile library to decode TGV formatted image data,
 * which can contain ETC1, ETC2_RGB, or ETC2_RGBA compressed textures.
 *
 * @param data Pointer to the TGV compressed image data.
 * @param length Length of the compressed data in bytes.
 * @param p Pointer to the destination buffer for the decoded ARGB8888 pixel data.
 *          The buffer must be large enough for `dst_w * dst_h * 4` bytes.
 * @param dst_x X-coordinate of the region to load.
 * @param dst_y Y-coordinate of the region to load.
 * @param dst_w Width of the region to load and the width of the output buffer `p`.
 * @param dst_h Height of the region to load and the height of the output buffer `p`.
 * @param alpha Expected alpha presence in the decoded data (used for validation against `lossy` type).
 * @param cspace Desired output colorspace (e.g., EMILE_COLORSPACE_ARGB8888, or specific ETC formats).
 * @param lossy The specific ETC encoding format (e.g., EET_IMAGE_ETC1, EET_IMAGE_ETC2_RGBA).
 * @return 1 on successful decoding, 0 on failure.
 */
static int
eet_data_image_etc2_decode(const void *data,
                           unsigned int length,
                           unsigned int *p,
                           unsigned int dst_x,
                           unsigned int dst_y,
                           unsigned int dst_w,
                           unsigned int dst_h,
                           Eina_Bool alpha,
                           Eet_Colorspace cspace,
                           Eet_Image_Encoding lossy)
{
   Emile_Image_Load_Opts opts;
   Emile_Image_Property prop;
   Emile_Image *image;
   Eina_Binbuf *bin;
   Emile_Image_Load_Error error;
   Eina_Bool found = EINA_FALSE;
   int i;

   // Fix for ABI incompatibility between 1.10 and 1.11
   if (cspace == 8) cspace = 9;

   bin = eina_binbuf_manage_new(data, length, EINA_TRUE);
   if (!bin) return 0;

   memset(&opts, 0, sizeof (opts));
   opts.region.x = dst_x;
   opts.region.y = dst_y;
   opts.region.w = dst_w;
   opts.region.h = dst_h;

   image = emile_image_tgv_memory_open(bin, &opts, NULL, &error);
   if (!image) goto on_error;

   memset(&prop, 0, sizeof (prop));

   if (!emile_image_head(image, &prop, sizeof (prop), &error))
     goto on_error;

   if (prop.cspaces)
     {
        for (i = 0; prop.cspaces[i] != EMILE_COLORSPACE_ARGB8888; i++)
          {
             if (prop.cspaces[i] == cspace)
               {
                  found = EINA_TRUE;
                  break;
               }
          }
        if (!found && (cspace != EMILE_COLORSPACE_ARGB8888))
          goto on_error;
     }

   switch (cspace)
     {
      case EMILE_COLORSPACE_ETC1:
         if (lossy != EET_IMAGE_ETC1) goto on_error;
         if (alpha != EINA_FALSE) goto on_error;
         break;
      case EMILE_COLORSPACE_RGB8_ETC2:
         if ((lossy != EET_IMAGE_ETC2_RGB) && (lossy != EET_IMAGE_ETC1)) goto on_error;
         if (alpha != EINA_FALSE) goto on_error;
         break;
      case EMILE_COLORSPACE_RGBA8_ETC2_EAC:
         if (lossy != EET_IMAGE_ETC2_RGBA) goto on_error;
         if (alpha != EINA_TRUE) goto on_error;
         break;
      case EMILE_COLORSPACE_ETC1_ALPHA:
         if (lossy != EET_IMAGE_ETC1_ALPHA) goto on_error;
         if (alpha != EINA_TRUE) goto on_error;
         break;
      case EMILE_COLORSPACE_ARGB8888:
         break;
      default:
         goto on_error;
     }

   prop.cspace = cspace;

   if (!emile_image_data(image, &prop, sizeof (prop), p, &error))
     goto on_error;

   // TODO: Add support for more unpremultiplied modes (ETC2)
   if ((cspace == EMILE_COLORSPACE_ARGB8888) && !prop.premul)
     _eet_argb_premul(p, prop.w * prop.h);

   emile_image_close(image);
   eina_binbuf_free(bin);
   return 1;

 on_error:
   ERR("Failed to decode image inside Eet");
   emile_image_close(image);
   eina_binbuf_free(bin);
   return 0;
}

static void *
eet_data_image_lossless_convert(int         *size,
                                const void  *data,
                                unsigned int w,
                                unsigned int h,
                                int          alpha)
{
   unsigned char *d;
   int *header;

   _eet_image_endian_check();

   d = malloc((w * h * 4) + (8 * 4));
   if (!d)
     return NULL;

   header = (int *)d;
   memset(d, 0, 32);

   header[0] = 0xac1dfeed;
   header[1] = w;
   header[2] = h;
   header[3] = alpha;

   memcpy(d + 32, data, w * h * 4);

   _eet_image_endian_swap(header, ((w * h) + 8));

   *size = ((w * h * 4) + (8 * 4));
   return d;
}

static void *
eet_data_image_lossless_compressed_convert(int         *size,
                                           const void  *data,
                                           unsigned int w,
                                           unsigned int h,
                                           int          alpha,
                                           int          compression)
{
   _eet_image_endian_check();

   {
      Eina_Binbuf *in;
      Eina_Binbuf *out;
      unsigned char *result;
      int *bigend_data = NULL;
      int header[8];

      if (_eet_image_words_bigendian)
        {
           bigend_data = (int *) malloc(w * h * 4);
           if (!bigend_data) return NULL;

           memcpy(bigend_data, data, w * h * 4);
           _eet_image_endian_swap(bigend_data, w * h);

           data = (const char *) bigend_data;
        }

      in = eina_binbuf_manage_new(data, w * h * 4, EINA_TRUE);
      if (!in)
        {
           free(bigend_data);
           return NULL;
        }

      out = emile_compress(in, eet_2_emile_compressor(compression), compression);

      if (!out || (eina_binbuf_length_get(out) > eina_binbuf_length_get(in)))
        {
           eina_binbuf_free(in);
           eina_binbuf_free(out);
           free(bigend_data);
           *size = -1;
           return NULL;
        }

      eina_binbuf_free(in);

      memset(header, 0, 8 * sizeof(int));
      header[0] = 0xac1dfeed;
      header[1] = w;
      header[2] = h;
      header[3] = alpha;
      header[4] = compression;

      _eet_image_endian_swap(header, 8);
      free(bigend_data);

      eina_binbuf_insert_length(out, (const unsigned char*) header, sizeof (header), 0);

      *size = eina_binbuf_length_get(out);
      result = eina_binbuf_string_steal(out);

      eina_binbuf_free(out);

      return result;
   }
}

/**
 * @brief Calculates a block size parameter for TGV1 encoding.
 * This determines the size of macro blocks used in TGV1 compression,
 * represented as a power of 2 (4 << k).
 *
 * @param size The dimension (width or height) for which to calculate the block size.
 * @return An integer k, where the block dimension is (4 << k).
 *         The result is clamped between 0 and MAX_BLOCK (6).
 */
static int
_block_size_get(int size)
{
   static const int MAX_BLOCK = 6; // 256 pixels

   int k = 0;
   while ((4 << k) < size) k++;
   k = MAX(0, k - 1);
   if ((size * 3 / 2) >= (4 << k)) return MAX(0, MIN(k - 1, MAX_BLOCK));
   return MIN(k, MAX_BLOCK);
}

/**
 * @brief Converts an ARGB image buffer to a greyscale image buffer using the alpha channel.
 * Each pixel in the output buffer will have R=G=B=Alpha_input. The original alpha
 * value is also preserved in the output alpha channel.
 * Used for preparing the alpha plane for ETC1+Alpha encoding.
 *
 * @param data Pointer to the ARGB pixel data to be converted in place.
 *             Each pixel is `0xAARRGGBB`. After conversion, it becomes `0xAAaaaaaa` where `aa` is original alpha.
 * @param len The number of pixels in the data array.
 */
static inline void
_alpha_to_greyscale_convert(uint32_t *data, int len)
{
   for (int k = 0; k < len; k++)
     {
        int alpha = A_VAL(data);
        *data++ = ARGB_JOIN(alpha, alpha, alpha, alpha);
     }
}

/**
 * @brief Converts raw ARGB pixel data to TGV1 format (ETC1, ETC2_RGB, ETC2_RGBA, ETC1_ALPHA).
 * This function encodes an image into the TGV1 container format, which uses
 * ETC-family texture compression.
 *
 * TGV1 Header Structure (total 16 bytes initially, then data blocks):
 * - `[0-3]`: "TGV1" (magic bytes)
 * - `[4]`:   bits 0-3: block_width_exponent (k_w for 4 << k_w pixels)
 *            bits 4-7: block_height_exponent (k_h for 4 << k_h pixels)
 * - `[5]`:   Algorithm:
 *            - 0: ETC1
 *            - 1: RGB8_ETC2
 *            - 2: RGBA8_ETC2_EAC
 *            - 3: ETC1_ALPHA (ETC1 for color, separate ETC1 for alpha)
 * - `[6]`:   Options:
 *            - bit 0: 0 for raw ETC blocks, 1 for LZ4 compressed ETC blocks.
 *            - bit 2: 0 for premultiplied alpha, 1 for unpremultiplied alpha (if RGBA).
 * - `[7]`:   Unused options (0).
 * - `[8-11]`: Network byte order unsigned int: image width.
 * - `[12-15]`: Network byte order unsigned int: image height.
 * Following this header are the compressed data blocks. Each block's size is
 * encoded using a variable-length scheme (1-3 bytes) before the block data itself.
 *
 * @param[out] size Pointer to an integer to store the size of the TGV1 data.
 * @param[in] data8 Pointer to the raw ARGB8888 pixel data (cast from `uint32_t*`).
 * @param w Width of the image.
 * @param h Height of the image.
 * @param quality ETC packing quality (influences `rg_etc1_pack_params.m_quality`).
 *                >95: high, >30: medium, <=30: low.
 * @param compress Boolean flag (0 or 1) to enable/disable LZ4 compression of ETC blocks.
 * @param lossy The target Eet_Image_Encoding (EET_IMAGE_ETC1, ETC2_RGB, ETC2_RGBA, ETC1_ALPHA).
 * @return A pointer to the allocated buffer containing the TGV1 image data.
 *         The caller is responsible for freeing this buffer. Returns NULL on failure.
 */
static void *
eet_data_image_etc1_compressed_convert(int         *size,
                                       const unsigned char *data8,
                                       unsigned int w,
                                       unsigned int h,
                                       int          quality,
                                       int          compress,
                                       Eet_Image_Encoding lossy)
{
   rg_etc1_pack_params param;
   uint8_t *buffer;
   uint32_t *data;
   uint32_t nl_width, nl_height;
   uint8_t header[8] = "TGV1";
   int block_width, block_height, macro_block_width, macro_block_height;
   int block_count, image_stride, image_height, etc_block_size;
   int num_planes = 1;
   Eet_Colorspace cspace;
   Eina_Bool unpremul = EINA_FALSE, alpha_texture = EINA_FALSE;
   Eina_Binbuf *r = NULL;
   void *result;
   const char *codec;

   data = NULL;
   r = eina_binbuf_new();
   if (!r) return NULL;

   image_stride = w;
   image_height = h;
   nl_width = eina_htonl(image_stride);
   nl_height = eina_htonl(image_height);
   compress = !!compress;

   // Disable dithering, as it will deteriorate the quality of flat surfaces
   param.m_dithering = 0;

   if (quality > 95)
     param.m_quality = rg_etc1_high_quality;
   else if (quality > 30)
     param.m_quality = rg_etc1_medium_quality;
   else
     param.m_quality = rg_etc1_low_quality;

   // header[4]: 4 bit block width, 4 bit block height
   block_width = _block_size_get(image_stride + 2);
   block_height = _block_size_get(image_height + 2);
   header[4] = (block_height << 4) | block_width;

   // header[5]: 0 for ETC1, 1 for RGB8_ETC2, 2 for RGBA8_ETC2_EAC, 3 for ETC1_ALPHA
   switch (lossy)
     {
      case EET_IMAGE_ETC1:
        cspace = EET_COLORSPACE_ETC1;
        etc_block_size = 8;
        header[5] = 0;
        codec = "ETC1";
        break;
      case EET_IMAGE_ETC2_RGB:
        cspace = EET_COLORSPACE_RGB8_ETC2;
        etc_block_size = 8;
        header[5] = 1;
        codec = "ETC2 (RGB)";
        break;
      case EET_IMAGE_ETC2_RGBA:
        cspace = EET_COLORSPACE_RGBA8_ETC2_EAC;
        etc_block_size = 16;
        header[5] = 2;
        codec = "ETC2 (RGBA)";
        break;
      case EET_IMAGE_ETC1_ALPHA:
        cspace = EET_COLORSPACE_ETC1_ALPHA;
        etc_block_size = 8;
        num_planes = 2; // RGB and Alpha
        header[5] = 3;
        alpha_texture = EINA_TRUE;
        codec = "ETC1+Alpha";
        break;
      default: abort();
     }

   // header[6]: 0 for raw, 1, for LZ4 compressed, 4 for unpremultiplied RGBA
   // blockless mode (0x2) is never used here
   header[6] = (compress ? 0x1 : 0x0) | (unpremul ? 0x4 : 0x0);

   // header[7]: unused options
   // Note: consider extending the header instead of filling all the bits here
   header[7] = 0;

   // Encoding being super slow, let's inform the user first.
   // FIXME: Ctrl+C must be handled
   INF("Encoding %dx%d image to %s, this may take a while...", w, h, codec);

   // Write header
   eina_binbuf_append_length(r, header, sizeof (header));
   eina_binbuf_append_length(r, (unsigned char*) &nl_width, sizeof (nl_width));
   eina_binbuf_append_length(r, (unsigned char*) &nl_height, sizeof (nl_height));

   // Real block size in pixels, obviously a multiple of 4
   macro_block_width = 4 << block_width;
   macro_block_height = 4 << block_height;

   // Number of ETC1 blocks in a compressed block
   block_count = (macro_block_width * macro_block_height) / (4 * 4);
   buffer = alloca(block_count * etc_block_size);

   // Write a whole plane (RGB or Alpha)
   for (int plane = 0; plane < num_planes; plane++)
     {
        if (!alpha_texture)
          {
             // Normal mode
             data = (uint32_t *) data8;
          }
        else if (!plane)
          {
             int len = image_stride * image_height;
             // RGB plane for ETC1+Alpha
             data = malloc(len * 4);
             if (!data) goto finish;
             memcpy(data, data8, len * 4);
             if (unpremul) _eet_argb_unpremul(data, len);
          }
        else
          {
             // Alpha plane for ETC1+Alpha
             _alpha_to_greyscale_convert(data, image_stride * image_height);
          }

        // Write macro block
        for (int y = 0; y < image_height + 2; y += macro_block_height)
          {
             uint32_t *input, *last_col, *last_row, *last_pix;
             int real_y;

             if (y == 0) real_y = 0;
             else if (y < image_height + 1) real_y = y - 1;
             else real_y = image_height - 1;

             for (int x = 0; x < image_stride + 2; x += macro_block_width)
               {
                  Eina_Binbuf *in;
                  uint8_t *offset = buffer;
                  int real_x = x;

                  if (x == 0) real_x = 0;
                  else if (x < image_stride + 1) real_x = x - 1;
                  else real_x = image_stride - 1;

                  input = data + real_y * image_stride + real_x;
                  last_row = data + image_stride * (image_height - 1) + real_x;
                  last_col = data + (real_y + 1) * image_stride - 1;
                  last_pix = data + image_height * image_stride - 1;

                  for (int by = 0; by < macro_block_height; by += 4)
                    {
                       int dup_top = ((y + by) == 0) ? 1 : 0;
                       int max_row = MAX(0, MIN(4, image_height - real_y - by));
                       int oy = (y == 0) ? 1 : 0;

                       for (int bx = 0; bx < macro_block_width; bx += 4)
                         {
                            int dup_left = ((x + bx) == 0) ? 1 : 0;
                            int max_col = MAX(0, MIN(4, image_stride - real_x - bx));
                            uint32_t todo[16] = { 0 };
                            int row, col;
                            int ox = (x == 0) ? 1 : 0;

                            if (dup_left)
                              {
                                 // Duplicate left column
                                 for (row = 0; row < max_row; row++)
                                   todo[row * 4] = input[row * image_stride];
                                 for (row = max_row; row < 4; row++)
                                   todo[row * 4] = last_row[0];
                              }

                            if (dup_top)
                              {
                                 // Duplicate top row
                                 for (col = 0; col < max_col; col++)
                                   todo[col] = input[MAX(col + bx - ox, 0)];
                                 for (col = max_col; col < 4; col++)
                                   todo[col] = last_col[0];
                              }

                            for (row = dup_top; row < 4; row++)
                              {
                                 for (col = dup_left; col < max_col; col++)
                                   {
                                      if (row < max_row)
                                        {
                                           // Normal copy
                                           todo[row * 4 + col] = input[(row + by - oy) * image_stride + bx + col - ox];
                                        }
                                      else
                                        {
                                           // Copy last line
                                           todo[row * 4 + col] = last_row[col + bx - ox];
                                        }
                                   }
                                 for (col = max_col; col < 4; col++)
                                   {
                                      // Right edge
                                      if (row < max_row)
                                        {
                                           // Duplicate last column
                                           todo[row * 4 + col] = last_col[MAX(row + by - oy, 0) * image_stride];
                                        }
                                      else
                                        {
                                           // Duplicate very last pixel again and again
                                           todo[row * 4 + col] = *last_pix;
                                        }
                                   }
                              }

                            switch (cspace)
                              {
                               case EET_COLORSPACE_ETC1:
                               case EET_COLORSPACE_ETC1_ALPHA:
                                 rg_etc1_pack_block(offset, (uint32_t *) todo, &param);
                                 break;
                               case EET_COLORSPACE_RGB8_ETC2:
                                 etc2_rgb8_block_pack(offset, (uint32_t *) todo, &param);
                                 break;
                               case EET_COLORSPACE_RGBA8_ETC2_EAC:
                                 etc2_rgba8_block_pack(offset, (uint32_t *) todo, &param);
                                 break;
                               default: return 0;
                              }

#ifdef DEBUG_STATS
                            if (plane == 0)
                              {
                                 // Decode to compute PSNR, this is slow.
                                 uint32_t done[16];

                                 if (alpha)
                                   rg_etc2_rgba8_decode_block(offset, done);
                                 else
                                    rg_etc2_rgb8_decode_block(offset, done);

                                 for (int k = 0; k < 16; k++)
                                   {
                                      const int r = (R_VAL(&(todo[k])) - R_VAL(&(done[k])));
                                      const int g = (G_VAL(&(todo[k])) - G_VAL(&(done[k])));
                                      const int b = (B_VAL(&(todo[k])) - B_VAL(&(done[k])));
                                      const int a = (A_VAL(&(todo[k])) - A_VAL(&(done[k])));
                                      mse += r*r + g*g + b*b;
                                      if (alpha) mse_alpha += a*a;
                                      mse_div++;
                                   }
                              }
#endif

                            offset += etc_block_size;
                         }
                    }

                  in = eina_binbuf_manage_new(buffer, block_count * etc_block_size, EINA_TRUE);
                  if (compress)
                    {
                       Eina_Binbuf *out;

                       out = emile_compress(in, EMILE_LZ4HC, EMILE_COMPRESSOR_BEST);
                       eina_binbuf_free(in);
                       in = out;
                    }

                  if (eina_binbuf_length_get(in) > 0)
                    {
                       unsigned int blen = eina_binbuf_length_get(in);

                       while (blen)
                         {
                            unsigned char plen;

                            plen = blen & 0x7F;
                            blen = blen >> 7;

                            if (blen) plen = 0x80 | plen;
                            eina_binbuf_append_length(r, &plen, 1);
                         }
                       eina_binbuf_append_buffer(r, in);
                    }
                  eina_binbuf_free(in);
               } // 4 rows
          } // macroblocks
     } // planes

finish:
   if (alpha_texture) free(data);
   *size = eina_binbuf_length_get(r);
   result = eina_binbuf_string_steal(r);
   eina_binbuf_free(r);

   return result;
}

/**
 * @brief Encodes greyscale pixel data (stored in alpha channel of ARGB) into JPEG format.
 * This function takes ARGB data where the greyscale information is expected to be
 * in the alpha channel (e.g., `0xGGxxxxxx`, where GG is the grey value). It extracts
 * this alpha value and encodes it as a JCS_GRAYSCALE JPEG.
 *
 * @param data Pointer to the ARGB pixel data. The alpha component of each pixel is used as the grey value.
 *             Example: `data[i] = 0xGGRRBB;` -> grey value is `(data[i] >> 24) & 0xff`.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param quality JPEG quality (0-100).
 * @param[out] size Pointer to an integer to store the size of the resulting JPEG data.
 * @return A pointer to the allocated buffer containing the JPEG image data.
 *         The caller is responsible for freeing this buffer. Returns NULL on failure.
 */
static inline void *
_eet_data_image_grey_encode(const void *data,
                            unsigned int w,
                            unsigned int h,
                            int quality,
                            int *size)
{
   const int *ptr;
   void *dst = NULL;
   size_t sz = 0;
   struct _JPEG_error_mgr jerr;
   JSAMPROW *jbuf;
   struct jpeg_compress_struct cinfo;
   unsigned char *buf;

   buf = alloca(3 * w);

   cinfo.err = jpeg_std_error(&(jerr.pub));
   jerr.pub.error_exit = _eet_image_jpeg_error_exit_cb;
   jerr.pub.emit_message = _eet_image_jpeg_emit_message_cb;
   jerr.pub.output_message = _eet_image_jpeg_output_message_cb;
   if (setjmp(jerr.setjmp_buffer))
     {
        return NULL;
     }

   jpeg_create_compress(&cinfo);
   if (eet_jpeg_membuf_dst(&cinfo, &dst, &sz))
     {
        jpeg_destroy_compress(&cinfo);
        return NULL;
     }

   cinfo.image_width = w;
   cinfo.image_height = h;
   cinfo.input_components = 1;
   cinfo.in_color_space = JCS_GRAYSCALE;
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

   while (cinfo.next_scanline < cinfo.image_height)
     {
        unsigned int i, j;

        ptr = ((const int *)data) + cinfo.next_scanline * w;
        /* convert scaline from ARGB to RGB packed */
        for (j = 0, i = 0; i < w; i++)
          {
             buf[j++] = ((*ptr) >> 24) & 0xff;
             ptr++;
          }
        jbuf = (JSAMPROW *)(&buf);
        jpeg_write_scanlines(&cinfo, jbuf, 1);
     }

   jpeg_finish_compress(&cinfo);
   jpeg_destroy_compress(&cinfo);

   *size = sz;
   return dst;
}

/**
 * @brief Encodes ARGB pixel data into JPEG format (RGB).
 * This function first checks if the image is entirely greyscale (R=G=B for all pixels).
 * If so, it delegates to `_eet_data_image_grey_encode` using the R component as the grey value.
 * Otherwise, it encodes the image as a standard JCS_RGB JPEG, discarding the alpha channel.
 *
 * @param data Pointer to the ARGB8888 pixel data.
 *             Example: `data[i] = 0xAARRGGBB;`
 * @param w Width of the image.
 * @param h Height of the image.
 * @param quality JPEG quality (0-100).
 * @param[out] size Pointer to an integer to store the size of the resulting JPEG data.
 * @return A pointer to the allocated buffer containing the JPEG image data.
 *         The caller is responsible for freeing this buffer. Returns NULL on failure.
 */
static inline void *
_eet_data_image_rgb_encode(const void *data,
                           unsigned int w,
                           unsigned int h,
                           int quality,
                           int *size)
{
   struct jpeg_compress_struct cinfo;
   struct _JPEG_error_mgr jerr;
   const int *ptr;
   void *d = NULL;
   size_t sz = 0;
   JSAMPROW *jbuf;
   unsigned int *grey;
   unsigned char *buf;

   /* Try to encode this buffer as GRY8 or AGRY88.
      It is difficult to know if a different approach
      would have payed of, but for now I do walk and
      copy the pixels as long as they are grey. If I
      manage to copy them up to the end, I will be
      able to encode them as a GRY8 texture.
    */
   grey = malloc(sizeof (int) * w * h);
   if (grey)
     {
        const unsigned int *pixels = data;
        unsigned int i;

        for (i = 0; i < w * h; i++)
          {
             uint8_t r, g, b;

             r = R_VAL(&pixels[i]);
             g = G_VAL(&pixels[i]);
             b = B_VAL(&pixels[i]);
             if (!(r == g && g == b))
               break ;
             grey[i] = r << 24;
          }

        if (i == w * h)
          {
             d = _eet_data_image_grey_encode(grey, w, h, quality, size);
             free(grey);
             return d;
          }
        free(grey);
     }

   buf = alloca(3 * w);

   memset(&cinfo, 0, sizeof (struct jpeg_compress_struct));

   cinfo.err = jpeg_std_error(&(jerr.pub));
   jerr.pub.error_exit = _eet_image_jpeg_error_exit_cb;
   jerr.pub.emit_message = _eet_image_jpeg_emit_message_cb;
   jerr.pub.output_message = _eet_image_jpeg_output_message_cb;
   if (setjmp(jerr.setjmp_buffer))
     return NULL;

   jpeg_create_compress(&cinfo);

   if (eet_jpeg_membuf_dst(&cinfo, &d, &sz))
     {
        jpeg_destroy_compress(&cinfo);
        return NULL;
     }

   cinfo.image_width = w;
   cinfo.image_height = h;
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

   while (cinfo.next_scanline < cinfo.image_height)
     {
        unsigned int i, j;

        /* convert scaline from ARGB to RGB packed */
        ptr = ((const int *)data) + cinfo.next_scanline * w;
        for (j = 0, i = 0; i < w; i++)
          {
             buf[j++] = ((*ptr) >> 16) & 0xff;
             buf[j++] = ((*ptr) >> 8) & 0xff;
             buf[j++] = ((*ptr)) & 0xff;
             ptr++;
          }
        jbuf = (JSAMPROW *)(&buf);
        jpeg_write_scanlines(&cinfo, jbuf, 1);
     }

   jpeg_finish_compress(&cinfo);
   jpeg_destroy_compress(&cinfo);

   *size = sz;
   return d;
}

static void *
eet_data_image_jpeg_convert(int         *size,
                            const void  *data,
                            unsigned int w,
                            unsigned int h,
                            int          alpha EINA_UNUSED,
                            int          quality)
{
   return _eet_data_image_rgb_encode(data, w, h, quality, size);
}

static void *
eet_data_image_jpeg_alpha_convert(int         *size,
                                  const void  *data,
                                  unsigned int w,
                                  unsigned int h,
                                  int          alpha EINA_UNUSED,
                                  int          quality)
{
   unsigned char *d1, *d2;
   unsigned char *d;
   int *header;
   int sz1, sz2;

   _eet_image_endian_check();

   d1 = _eet_data_image_rgb_encode(data, w, h, quality, &sz1);
   d2 = _eet_data_image_grey_encode(data, w, h, quality, &sz2);

   if (!d1 || !d2)
     {
        free(d1);
        free(d2);
        return NULL;
     }

   d = malloc(12 + sz1 + sz2);
   if (!d)
     {
        free(d1);
        free(d2);
        return NULL;
     }

   header = (int *)d;
   header[0] = 0xbeeff00d;
   header[1] = sz1;
   header[2] = sz2;
   _eet_image_endian_swap(header, 3);

   memcpy(d + 12, d1, sz1);
   memcpy(d + 12 + sz1, d2, sz2);

   free(d1);
   free(d2);
   *size = 12 + sz1 + sz2;
   return d;
}

/**
 * @brief Encodes image data and writes it to an Eet file with optional encryption.
 *
 * This function first encodes the raw pixel data according to the specified parameters
 * (lossy/lossless, compression, quality) using `eet_data_image_encode()`.
 * Then, it writes the resulting encoded data to the Eet file under the given `name`,
 * optionally encrypting it with `cipher_key`.
 *
 * @param ef Pointer to the open Eet_File.
 * @param name The key name for the data entry in the Eet file.
 * @param cipher_key Optional encryption key. If NULL, data is not encrypted.
 * @param data Pointer to the raw pixel data (e.g., ARGB8888).
 * @param w Width of the image.
 * @param h Height of the image.
 * @param alpha 1 if the image has alpha, 0 otherwise.
 * @param comp Compression level or type (depends on `lossy`).
 *             For lossless: compression algorithm (e.g., Emile_Compressor value).
 *             For lossy (JPEG/ETC): typically 0 (JPEG quality is separate), or boolean for TGV compression.
 * @param quality Quality setting (e.g., for JPEG: 0-100; for ETC: influences packer).
 * @param lossy The encoding type (EET_IMAGE_LOSSLESS, EET_IMAGE_JPEG, EET_IMAGE_ETC1, etc.).
 * @return The number of bytes written on success, or 0 on failure.
 */
EAPI int
eet_data_image_write_cipher(Eet_File    *ef,
                            const char  *name,
                            const char  *cipher_key,
                            const void  *data,
                            unsigned int w,
                            unsigned int h,
                            int          alpha,
                            int          comp,
                            int          quality,
                            Eet_Image_Encoding lossy)
{
   void *d = NULL;
   int size = 0;

   d = eet_data_image_encode(data, &size, w, h, alpha, comp, quality, lossy);
   if (d)
     {
        int v;

        v = eet_write_cipher(ef, name, d, size, 0, cipher_key);
        free(d);
        return v;
     }

   return 0;
}

/**
 * @brief Encodes image data and writes it to an Eet file.
 * This is a convenience wrapper around `eet_data_image_write_cipher` with `cipher_key` set to NULL.
 *
 * @param ef Pointer to the open Eet_File.
 * @param name The key name for the data entry in the Eet file.
 * @param data Pointer to the raw pixel data (e.g., ARGB8888).
 * @param w Width of the image.
 * @param h Height of the image.
 * @param alpha 1 if the image has alpha, 0 otherwise.
 * @param comp Compression level or type.
 * @param quality Quality setting.
 * @param lossy The encoding type.
 * @return The number of bytes written on success, or 0 on failure.
 * @see eet_data_image_write_cipher() for more details on parameters.
 */
EAPI int
eet_data_image_write(Eet_File    *ef,
                     const char  *name,
                     const void  *data,
                     unsigned int w,
                     unsigned int h,
                     int          alpha,
                     int          comp,
                     int          quality,
                     Eet_Image_Encoding lossy)
{
   return eet_data_image_write_cipher(ef,
                                      name,
                                      NULL,
                                      data,
                                      w,
                                      h,
                                      alpha,
                                      comp,
                                      quality,
                                      lossy);
}

/**
 * @brief Reads image data from an Eet file, decodes it, and optionally decrypts it.
 *
 * This function reads an encoded image entry from the Eet file. If a `cipher_key`
 * is provided, it attempts to decrypt the data. Then, it decodes the image data
 * into raw ARGB8888 pixel format and returns metadata about the image.
 *
 * @param ef Pointer to the open Eet_File.
 * @param name The key name of the data entry in the Eet file.
 * @param cipher_key Optional decryption key. If NULL, data is assumed not encrypted.
 * @param[out] w Pointer to store the width of the decoded image. Can be NULL.
 * @param[out] h Pointer to store the height of the decoded image. Can be NULL.
 * @param[out] alpha Pointer to store the alpha flag (1 if alpha, 0 otherwise). Can be NULL.
 * @param[out] comp Pointer to store compression information from the image header. Can be NULL.
 * @param[out] quality Pointer to store quality information from the image header. Can be NULL.
 * @param[out] lossy Pointer to store the encoding type of the image. Can be NULL.
 * @return A pointer to the allocated buffer containing the decoded ARGB8888 pixel data.
 *         The caller is responsible for freeing this buffer. Returns NULL on failure.
 */
EAPI void *
eet_data_image_read_cipher(Eet_File     *ef,
                           const char   *name,
                           const char   *cipher_key,
                           unsigned int *w,
                           unsigned int *h,
                           int          *alpha,
                           int          *comp,
                           int          *quality,
                           Eet_Image_Encoding *lossy)
{
   unsigned int *d = NULL;
   void *data = NULL;
   int free_data = 0;
   int size;

   if (!cipher_key)
     data = (void *)eet_read_direct(ef, name, &size);

   if (!data)
     {
        data = eet_read_cipher(ef, name, &size, cipher_key);
        free_data = 1;
        if (!data)
          return NULL;
     }

   d = eet_data_image_decode(data, size, w, h, alpha, comp, quality, lossy);

   if (free_data)
     free(data);

   return d;
}

/**
 * @brief Reads image data from an Eet file and decodes it.
 * This is a convenience wrapper around `eet_data_image_read_cipher` with `cipher_key` set to NULL.
 *
 * @param ef Pointer to the open Eet_File.
 * @param name The key name of the data entry in the Eet file.
 * @param[out] w Pointer to store the width of the decoded image. Can be NULL.
 * @param[out] h Pointer to store the height of the decoded image. Can be NULL.
 * @param[out] alpha Pointer to store the alpha flag. Can be NULL.
 * @param[out] comp Pointer to store compression information. Can be NULL.
 * @param[out] quality Pointer to store quality information. Can be NULL.
 * @param[out] lossy Pointer to store the encoding type. Can be NULL.
 * @return A pointer to the allocated buffer containing the decoded ARGB8888 pixel data.
 *         The caller is responsible for freeing this buffer. Returns NULL on failure.
 * @see eet_data_image_read_cipher() for more details on parameters.
 */
EAPI void *
eet_data_image_read(Eet_File     *ef,
                    const char   *name,
                    unsigned int *w,
                    unsigned int *h,
                    int          *alpha,
                    int          *comp,
                    int          *quality,
                    Eet_Image_Encoding *lossy)
{
   return eet_data_image_read_cipher(ef, name, NULL, w, h, alpha,
                                     comp, quality, lossy);
}

/**
 * @brief Reads image data from an Eet file, decodes it to a specified colorspace and region into a pre-allocated surface.
 *
 * This function reads an encoded image entry, optionally decrypts it, and then decodes
 * a specified region (`src_x`, `src_y`, `w`, `h`) of the image directly into the
 * provided buffer `d`. The output is formatted according to `cspace` and `row_stride`.
 *
 * @param ef Pointer to the open Eet_File.
 * @param name The key name of the data entry in the Eet file.
 * @param cipher_key Optional decryption key. If NULL, data is assumed not encrypted.
 * @param src_x X-coordinate of the top-left corner of the source region in the stored image.
 * @param src_y Y-coordinate of the top-left corner of the source region in the stored image.
 * @param d Pointer to the pre-allocated destination buffer for the decoded pixel data.
 *          The buffer must be large enough to hold `row_stride * h` bytes.
 *          Pixel data is typically `unsigned int` per pixel for ARGB formats.
 * @param w Width of the region to decode and the width of the target surface area in `d`.
 * @param h Height of the region to decode and the height of the target surface area in `d`.
 * @param row_stride The number of bytes from the start of one row of pixels in `d` to the start of the next.
 *                   Typically `w * bytes_per_pixel_for_cspace`.
 * @param cspace The desired Eet_Colorspace for the output data in buffer `d`.
 * @param[out] alpha Pointer to store the alpha flag of the original image. Can be NULL.
 * @param[out] comp Pointer to store compression information. Can be NULL.
 * @param[out] quality Pointer to store quality information. Can be NULL.
 * @param[out] lossy Pointer to store the encoding type. Can be NULL.
 * @return 1 on success, 0 on failure.
 */
EAPI int
eet_data_image_read_to_cspace_surface_cipher(Eet_File     *ef,
                                             const char   *name,
                                             const char   *cipher_key,
                                             unsigned int  src_x,
                                             unsigned int  src_y,
                                             unsigned int *d,
                                             unsigned int  w,
                                             unsigned int  h,
                                             unsigned int  row_stride,
                                             Eet_Colorspace cspace,
                                             int          *alpha,
                                             int          *comp,
                                             int          *quality,
                                             Eet_Image_Encoding *lossy)
{
   void *data = NULL;
   int free_data = 0;
   int res = 1;
   int size;

   if (!cipher_key)
     data = (void *)eet_read_direct(ef, name, &size);

   if (!data)
     {
        data = eet_read_cipher(ef, name, &size, cipher_key);
        free_data = 1;
        if (!data)
          return 0;
     }

   res = eet_data_image_decode_to_cspace_surface_cipher(data, NULL, size, src_x, src_y, d,
                                                        w, h, row_stride, cspace, alpha,
                                                        comp, quality, lossy);

   if (free_data)
     free(data);

   return res;
}

/**
 * @brief Reads image data from an Eet file and decodes it to an ARGB8888 surface.
 * This is a convenience wrapper for `eet_data_image_read_to_cspace_surface_cipher`
 * with `cspace` fixed to `EET_COLORSPACE_ARGB8888`.
 *
 * @param ef Pointer to the open Eet_File.
 * @param name The key name of the data entry.
 * @param cipher_key Optional decryption key.
 * @param src_x X-coordinate of the source region.
 * @param src_y Y-coordinate of the source region.
 * @param d Pointer to the pre-allocated destination buffer (ARGB8888).
 * @param w Width of the region to decode.
 * @param h Height of the region to decode.
 * @param row_stride Row stride of the destination buffer `d`.
 * @param[out] alpha Pointer to store the alpha flag. Can be NULL.
 * @param[out] comp Pointer to store compression information. Can be NULL.
 * @param[out] quality Pointer to store quality information. Can be NULL.
 * @param[out] lossy Pointer to store the encoding type. Can be NULL.
 * @return 1 on success, 0 on failure.
 * @see eet_data_image_read_to_cspace_surface_cipher() for more details.
 */
EAPI int
eet_data_image_read_to_surface_cipher(Eet_File     *ef,
                                      const char   *name,
                                      const char   *cipher_key,
                                      unsigned int  src_x,
                                      unsigned int  src_y,
                                      unsigned int *d,
                                      unsigned int  w,
                                      unsigned int  h,
                                      unsigned int  row_stride,
                                      int          *alpha,
                                      int          *comp,
                                      int          *quality,
                                      Eet_Image_Encoding *lossy)
{
   return eet_data_image_read_to_cspace_surface_cipher(ef, name, cipher_key,
                                                       src_x, src_y, d, w, h, row_stride,
                                                       EET_COLORSPACE_ARGB8888,
                                                       alpha, comp, quality, lossy);
}

/**
 * @brief Reads image data from an Eet file and decodes it to an ARGB8888 surface (no cipher).
 * This is a convenience wrapper for `eet_data_image_read_to_surface_cipher`
 * with `cipher_key` set to NULL.
 *
 * @param ef Pointer to the open Eet_File.
 * @param name The key name of the data entry.
 * @param src_x X-coordinate of the source region.
 * @param src_y Y-coordinate of the source region.
 * @param d Pointer to the pre-allocated destination buffer (ARGB8888).
 * @param w Width of the region to decode.
 * @param h Height of the region to decode.
 * @param row_stride Row stride of the destination buffer `d`.
 * @param[out] alpha Pointer to store the alpha flag. Can be NULL.
 * @param[out] comp Pointer to store compression information. Can be NULL.
 * @param[out] quality Pointer to store quality information. Can be NULL.
 * @param[out] lossy Pointer to store the encoding type. Can be NULL.
 * @return 1 on success, 0 on failure.
 * @see eet_data_image_read_to_surface_cipher() for more details.
 */
EAPI int
eet_data_image_read_to_surface(Eet_File     *ef,
                               const char   *name,
                               unsigned int  src_x,
                               unsigned int  src_y,
                               unsigned int *d,
                               unsigned int  w,
                               unsigned int  h,
                               unsigned int  row_stride,
                               int          *alpha,
                               int          *comp,
                               int          *quality,
                               Eet_Image_Encoding *lossy)
{
   return eet_data_image_read_to_surface_cipher(ef, name, NULL,
                                                src_x, src_y, d,
                                                w, h, row_stride,
                                                alpha, comp, quality,
                                                lossy);
}

/**
 * @brief Reads and decodes the header of an image stored in an Eet file, with optional decryption.
 *
 * This function reads the raw image data associated with `name` from the Eet file,
 * optionally decrypts it using `cipher_key`, and then decodes only its header
 * to retrieve metadata like dimensions, alpha presence, compression, quality, and encoding type.
 * It does not decompress or return the full pixel data.
 *
 * @param ef Pointer to the open Eet_File.
 * @param name The key name of the data entry in the Eet file.
 * @param cipher_key Optional decryption key. If NULL, data is assumed not encrypted.
 * @param[out] w Pointer to store the width of the image. Can be NULL.
 * @param[out] h Pointer to store the height of the image. Can be NULL.
 * @param[out] alpha Pointer to store the alpha flag (1 if alpha, 0 otherwise). Can be NULL.
 * @param[out] comp Pointer to store compression information from the image header. Can be NULL.
 * @param[out] quality Pointer to store quality information from the image header. Can be NULL.
 * @param[out] lossy Pointer to store the encoding type of the image. Can be NULL.
 * @return 1 on success, 0 on failure (e.g., entry not found, decryption failed, invalid header).
 */
EAPI int
eet_data_image_header_read_cipher(Eet_File     *ef,
                                  const char   *name,
                                  const char   *cipher_key,
                                  unsigned int *w,
                                  unsigned int *h,
                                  int          *alpha,
                                  int          *comp,
                                  int          *quality,
                                  Eet_Image_Encoding *lossy)
{
   void *data = NULL;
   int size = 0;
   int free_data = 0;
   int d;

   if (!cipher_key)
     data = (void *)eet_read_direct(ef, name, &size);

   if (!data)
     {
        data = eet_read_cipher(ef, name, &size, cipher_key);
        free_data = 1;
        if (!data)
          return 0;
     }

   d = eet_data_image_header_decode(data, size, w, h, alpha,
                                    comp, quality, lossy);
   if (free_data)
     free(data);

   return d;
}

/**
 * @brief Reads and decodes the header of an image stored in an Eet file.
 * This is a convenience wrapper around `eet_data_image_header_read_cipher` with `cipher_key` set to NULL.
 *
 * @param ef Pointer to the open Eet_File.
 * @param name The key name of the data entry.
 * @param[out] w Pointer to store the image width. Can be NULL.
 * @param[out] h Pointer to store the image height. Can be NULL.
 * @param[out] alpha Pointer to store the alpha flag. Can be NULL.
 * @param[out] comp Pointer to store compression information. Can be NULL.
 * @param[out] quality Pointer to store quality information. Can be NULL.
 * @param[out] lossy Pointer to store the encoding type. Can be NULL.
 * @return 1 on success, 0 on failure.
 * @see eet_data_image_header_read_cipher() for more details.
 */
EAPI int
eet_data_image_header_read(Eet_File     *ef,
                           const char   *name,
                           unsigned int *w,
                           unsigned int *h,
                           int          *alpha,
                           int          *comp,
                           int          *quality,
                           Eet_Image_Encoding *lossy)
{
   return eet_data_image_header_read_cipher(ef, name, NULL,
                                            w, h, alpha,
                                            comp, quality, lossy);
}

/**
 * @brief Encodes raw image data into a specified format, with optional encryption.
 *
 * This function takes raw pixel data (assumed ARGB8888) and encodes it based on
 * the `lossy`, `comp`, and `quality` parameters. The resulting encoded data can
 * then be optionally encrypted using `cipher_key`.
 *
 * Supported encodings (`lossy`):
 * - `EET_IMAGE_LOSSLESS`: Stores raw or compressed ARGB data.
 *   - `comp > 0`: Uses compression specified by `comp` (e.g., Emile_Compressor value).
 *   - `comp <= 0`: Stores uncompressed.
 * - `EET_IMAGE_JPEG`: Encodes as JPEG.
 *   - `alpha = 0`: Standard JPEG (RGB).
 *   - `alpha = 1`: Custom format with separate JPEG for RGB and alpha.
 *   - `quality`: JPEG quality (0-100).
 * - `EET_IMAGE_ETC1`, `EET_IMAGE_ETC2_RGB`, `EET_IMAGE_ETC2_RGBA`, `EET_IMAGE_ETC1_ALPHA`:
 *   Encodes as TGV1 format using the respective ETC compression.
 *   - `comp`: Boolean (0 or 1) for LZ4 compression of ETC blocks within TGV1.
 *   - `quality`: Influences ETC packer settings.
 *
 * @param data Pointer to the raw pixel data (e.g., ARGB8888).
 * @param cipher_key Optional encryption key. If NULL, data is not encrypted.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param alpha 1 if the image has alpha, 0 otherwise. Used to guide encoding choices.
 * @param comp Compression setting. Meaning depends on `lossy`.
 * @param quality Quality setting. Meaning depends on `lossy`.
 * @param lossy The target `Eet_Image_Encoding` type.
 * @param[out] size_ret Pointer to an integer to store the size of the resulting encoded (and possibly encrypted) data. Can be NULL.
 * @return A pointer to an allocated buffer containing the encoded (and possibly encrypted) image data.
 *         The caller is responsible for freeing this buffer. Returns NULL on failure or if input `data` is NULL.
 */
EAPI void *
eet_data_image_encode_cipher(const void  *data,
                             const char  *cipher_key,
                             unsigned int w,
                             unsigned int h,
                             int          alpha,
                             int          comp,
                             int          quality,
                             Eet_Image_Encoding lossy,
                             int         *size_ret)
{
   void *d = NULL;
   void *ciphered_d = NULL;
   unsigned int ciphered_sz = 0;
   int size = 0;

   if (!data)
     return NULL;

   switch (lossy)
     {
      case EET_IMAGE_LOSSLESS:
         if (comp > 0)
           d = eet_data_image_lossless_compressed_convert(&size, data,
                                                          w, h, alpha, comp);

         /* eet_data_image_lossless_compressed_convert will refuse to compress something
            if the result is bigger than the entry. */
         if (comp <= 0 || !d)
           d = eet_data_image_lossless_convert(&size, data, w, h, alpha);
         break;
      case EET_IMAGE_JPEG:
         if (!alpha)
           d = eet_data_image_jpeg_convert(&size, data, w, h, alpha, quality);
         else
           d = eet_data_image_jpeg_alpha_convert(&size, data,
                                                 w, h, alpha, quality);
         break;
      case EET_IMAGE_ETC1:
      case EET_IMAGE_ETC2_RGB:
        if (alpha) abort();
        // fallthrough
      case EET_IMAGE_ETC2_RGBA:
      case EET_IMAGE_ETC1_ALPHA:
         d = eet_data_image_etc1_compressed_convert(&size, data, w, h,
                                                    quality, comp, lossy);
         break;
      default:
         abort();
     }

   if (cipher_key)
     {
        if(!eet_cipher(d, size, cipher_key, strlen(cipher_key), &ciphered_d,
                       &ciphered_sz))
          {
             if (d)
               free(d);

             d = ciphered_d;
             size = ciphered_sz;
          }
        else
          if (ciphered_d)
            free(ciphered_d);
     }

   if (size_ret)
     *size_ret = size;

   return d;
}

/**
 * @brief Encodes raw image data into a specified format.
 * This is a convenience wrapper around `eet_data_image_encode_cipher` with `cipher_key` set to NULL.
 *
 * @param data Pointer to the raw pixel data.
 * @param[out] size_ret Pointer to store the size of the encoded data. Can be NULL.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param alpha 1 if the image has alpha, 0 otherwise.
 * @param comp Compression setting.
 * @param quality Quality setting.
 * @param lossy The target encoding type.
 * @return A pointer to an allocated buffer containing the encoded image data.
 *         The caller is responsible for freeing this buffer. Returns NULL on failure.
 * @see eet_data_image_encode_cipher() for detailed parameter descriptions.
 */
EAPI void *
eet_data_image_encode(const void  *data,
                      int         *size_ret,
                      unsigned int w,
                      unsigned int h,
                      int          alpha,
                      int          comp,
                      int          quality,
                      Eet_Image_Encoding lossy)
{
   return eet_data_image_encode_cipher(data, NULL, w, h, alpha,
                                       comp, quality, lossy, size_ret);
}

static const Eet_Colorspace _eet_etc1_colorspace[] = {
  EET_COLORSPACE_ETC1,
  EET_COLORSPACE_RGB8_ETC2,
  EET_COLORSPACE_ARGB8888
};

static const Eet_Colorspace _eet_etc1_alpha_colorspace[] = {
  EET_COLORSPACE_ETC1_ALPHA,
  EET_COLORSPACE_ARGB8888
};

static const Eet_Colorspace _eet_etc2_rgb_colorspace[] = {
  EET_COLORSPACE_RGB8_ETC2,
  EET_COLORSPACE_ARGB8888
};

static const Eet_Colorspace _eet_etc2_rgba_colorspace[] = {
  EET_COLORSPACE_RGBA8_ETC2_EAC,
  EET_COLORSPACE_ARGB8888
};

static const Eet_Colorspace _eet_gry8_alpha_colorspace[] = {
  EET_COLORSPACE_AGRY88,
  EET_COLORSPACE_ARGB8888
};

/**
 * @brief Decodes the header of an image from a raw data buffer, with optional decryption and colorspace information.
 *
 * This function inspects the provided `data` buffer (which might be encrypted if `cipher_key` is given)
 * to determine the image's metadata. It supports several Eet image formats:
 * - Lossless (compressed or uncompressed): Magic 0xac1dfeed.
 * - JPEG with separate alpha: Magic 0xbeeff00d.
 * - TGV1 (ETC1/ETC2 based): Magic "TGV1".
 * - Standard JPEG.
 *
 * It populates the output parameters with the found metadata. Additionally, if `cspaces` is not NULL,
 * it can return a list of supported `Eet_Colorspace`s for certain formats (primarily TGV and JPEG).
 *
 * @param data Pointer to the raw image data buffer.
 * @param cipher_key Optional decryption key. If NULL, data is assumed not encrypted.
 * @param size Size of the `data` buffer in bytes.
 * @param[out] w Pointer to store the image width. Can be NULL.
 * @param[out] h Pointer to store the image height. Can be NULL.
 * @param[out] alpha Pointer to store the alpha flag (1 if alpha, 0 otherwise). Can be NULL.
 * @param[out] comp Pointer to store compression information. Can be NULL.
 * @param[out] quality Pointer to store quality information. Can be NULL.
 * @param[out] lossy Pointer to store the `Eet_Image_Encoding` type. Can be NULL.
 * @param[out] cspaces If not NULL, a pointer to a `const Eet_Colorspace*` which will be
 *                     set to an array of supported colorspaces for the image format.
 *                     The array is terminated by `EET_COLORSPACE_ARGB8888` (or is specific).
 *                     Example for TGV1 ETC1: `_eet_etc1_colorspace` might be returned.
 *                     The caller should not free this array; it points to static data.
 * @return 1 on successful header decoding, 0 on failure (e.g., insufficient data, unknown format, decryption error).
 */
static int
eet_data_image_header_advance_decode_cipher(const void   *data,
                                            const char   *cipher_key,
                                            int           size,
                                            unsigned int *w,
                                            unsigned int *h,
                                            int          *alpha,
                                            int          *comp,
                                            int          *quality,
                                            Eet_Image_Encoding *lossy,
                                            const Eet_Colorspace **cspaces)
{
   int header[8];
   void *deciphered_d = NULL;
   unsigned int deciphered_sz = 0;
   int r = 0;

   if (!data) return 0;

   if (cipher_key)
     {
        if (!eet_decipher(data, size, cipher_key, strlen(cipher_key),
                          &deciphered_d, &deciphered_sz))
          {
             data = deciphered_d;
             size = deciphered_sz;
          }
        else
          {
             free(deciphered_d);
             deciphered_d = NULL;
          }
     }

   _eet_image_endian_check();

   if (size < 32) goto on_error;

   memcpy(header, data, 32);
   _eet_image_endian_swap(header, 8);

   if ((unsigned)header[0] == 0xac1dfeed)
     {
        int iw, ih, al, cp;

        iw = header[1];
        ih = header[2];
        al = header[3];
        cp = header[4];
        if ((iw < 1) || (ih < 1) || (iw > 8192) || (ih > 8192))
          goto on_error;

        if ((cp == 0) && (size < ((iw * ih * 4) + 32)))
          goto on_error;

        if (w)
          *w = iw;

        if (h)
          *h = ih;

        if (alpha)
          *alpha = al ? 1 : 0;

        if (comp)
          *comp = cp;

        if (lossy)
          *lossy = EET_IMAGE_LOSSLESS;

        if (quality)
          *quality = 100;

        r = 1;
     }
   else if ((unsigned)header[0] == 0xbeeff00d)
     {
        unsigned int iw = 0, ih = 0;
        unsigned const char *dt;
        int sz1, sz2;
        int ok;

        sz1 = header[1];
        sz2 = header[2];
        if ((sz1 <= 0) || (sz2 <= 0) || ((sz1 + sz2) > (size - 12)))
          goto on_error;
        dt = data;
        dt += 12;
        ok = eet_data_image_jpeg_header_decode(dt, sz1, &iw, &ih, cspaces);
        if (ok)
          {
             if (w)
               *w = iw;

             if (h)
               *h = ih;

             if (alpha)
               *alpha = 1;

             if (comp)
               *comp = 0;

             if (lossy)
               *lossy = EET_IMAGE_JPEG;

             if (quality)
               *quality = 75;

             r = 1;
          }
     }
   else if (!strncmp(data, "TGV1", 4))
     {
        const char *m = data;

        // We only use Emile for decoding the actual data, seems simpler this way.
        if (w) *w = eina_ntohl(*((unsigned int*) &(m[OFFSET_WIDTH])));
        if (h) *h = eina_ntohl(*((unsigned int*) &(m[OFFSET_HEIGHT])));
        if (comp) *comp = m[OFFSET_OPTIONS] & 0x1;
        switch (m[OFFSET_ALGORITHM] & 0xFF)
          {
           case 0:
             if (lossy) *lossy = EET_IMAGE_ETC1;
             if (alpha) *alpha = EINA_FALSE;
             if (cspaces) *cspaces = _eet_etc1_colorspace;
             break;
           case 1:
             if (lossy) *lossy = EET_IMAGE_ETC2_RGB;
             if (alpha) *alpha = EINA_FALSE;
             if (cspaces) *cspaces = _eet_etc2_rgb_colorspace;
             break;
           case 2:
             if (alpha) *alpha = EINA_TRUE;
             if (lossy) *lossy = EET_IMAGE_ETC2_RGBA;
             if (cspaces) *cspaces = _eet_etc2_rgba_colorspace;
             break;
           case 3:
             if (alpha) *alpha = EINA_TRUE;
             if (lossy) *lossy = EET_IMAGE_ETC1_ALPHA;
             if (cspaces) *cspaces = _eet_etc1_alpha_colorspace;
             break;
           default:
              goto on_error;
          }
        if (quality) *quality = 50;

        r = 1;
     }
   else
     {
        unsigned int iw = 0, ih = 0;
        int ok;

        ok = eet_data_image_jpeg_header_decode(data, size, &iw, &ih, cspaces);
        if (ok)
          {
             if (w)
               *w = iw;

             if (h)
               *h = ih;

             if (alpha)
               *alpha = 0;

             if (comp)
               *comp = 0;

             if (lossy)
               *lossy = EET_IMAGE_JPEG;

             if (quality)
               *quality = 75;

             if (cspaces && *cspaces)
               {
                  if ((*cspaces)[0] == EMILE_COLORSPACE_GRY8)
                    *cspaces = _eet_gry8_alpha_colorspace;
               }

             r = 1;
          }
     }

 on_error:
   free(deciphered_d);
   return r;
}

/**
 * @brief Decodes the header of an image from a raw data buffer, with optional decryption.
 * This is a wrapper around `eet_data_image_header_advance_decode_cipher` that
 * does not request colorspace information.
 *
 * @param data Pointer to the raw image data buffer.
 * @param cipher_key Optional decryption key.
 * @param size Size of the `data` buffer.
 * @param[out] w Pointer to store image width. Can be NULL.
 * @param[out] h Pointer to store image height. Can be NULL.
 * @param[out] alpha Pointer to store alpha flag. Can be NULL.
 * @param[out] comp Pointer to store compression info. Can be NULL.
 * @param[out] quality Pointer to store quality info. Can be NULL.
 * @param[out] lossy Pointer to store encoding type. Can be NULL.
 * @return 1 on success, 0 on failure.
 * @see eet_data_image_header_advance_decode_cipher() for more details.
 */
EAPI int
eet_data_image_header_decode_cipher(const void   *data,
                                    const char   *cipher_key,
                                    int           size,
                                    unsigned int *w,
                                    unsigned int *h,
                                    int          *alpha,
                                    int          *comp,
                                    int          *quality,
                                    Eet_Image_Encoding *lossy)
{
   return eet_data_image_header_advance_decode_cipher(data, cipher_key, size,
                                                      w, h,
                                                      alpha, comp, quality, lossy,
                                                      NULL);
}

/**
 * @brief Retrieves the list of supported colorspaces for an image stored in an Eet file.
 *
 * This function reads an image entry from the Eet file, optionally decrypts it,
 * and then uses `eet_data_image_header_advance_decode_cipher` to determine the
 * available colorspaces for that image format.
 *
 * @param ef Pointer to the open Eet_File.
 * @param name The key name of the data entry in the Eet file.
 * @param cipher_key Optional decryption key. If NULL, data is assumed not encrypted.
 * @param[out] cspaces A pointer to a `const Eet_Colorspace*` which will be
 *                     set to an array of supported colorspaces for the image format.
 *                     The array is terminated by `EET_COLORSPACE_ARGB8888` or is specific.
 *                     The caller should not free this array; it points to static data.
 * @return 1 if colorspace information was successfully retrieved, 0 on failure.
 */
EAPI int
eet_data_image_colorspace_get(Eet_File *ef,
                              const char *name,
                              const char *cipher_key,
                              const Eet_Colorspace **cspaces)
{
   void *data = NULL;
   int size = 0;
   int free_data = 0;
   int d;

   if (!cipher_key)
     data = (void *)eet_read_direct(ef, name, &size);

   if (!data)
     {
        data = eet_read_cipher(ef, name, &size, cipher_key);
        free_data = 1;
        if (!data)
          return 0;
     }

   d = eet_data_image_header_advance_decode_cipher(data, NULL, size, NULL, NULL,
                                                   NULL, NULL, NULL, NULL,
                                                   cspaces);
   if (free_data)
     free(data);

   return d;
}

/**
 * @brief Decodes the header of an image from a raw data buffer.
 * This is a convenience wrapper around `eet_data_image_header_decode_cipher` with `cipher_key` set to NULL.
 *
 * @param data Pointer to the raw image data buffer.
 * @param size Size of the `data` buffer.
 * @param[out] w Pointer to store image width. Can be NULL.
 * @param[out] h Pointer to store image height. Can be NULL.
 * @param[out] alpha Pointer to store alpha flag. Can be NULL.
 * @param[out] comp Pointer to store compression info. Can be NULL.
 * @param[out] quality Pointer to store quality info. Can be NULL.
 * @param[out] lossy Pointer to store encoding type. Can be NULL.
 * @return 1 on success, 0 on failure.
 * @see eet_data_image_header_decode_cipher() for more details.
 */
EAPI int
eet_data_image_header_decode(const void   *data,
                             int           size,
                             unsigned int *w,
                             unsigned int *h,
                             int          *alpha,
                             int          *comp,
                             int          *quality,
                             Eet_Image_Encoding *lossy)
{
   return eet_data_image_header_decode_cipher(data,
                                              NULL,
                                              size,
                                              w,
                                              h,
                                              alpha,
                                              comp,
                                              quality,
                                              lossy);
}

/**
 * @brief Copies a rectangular region from a source pixel buffer to a destination pixel buffer.
 * This function handles copying a sub-region (`src_x`, `src_y`, `w`, `h`) from a source buffer
 * (`src` with width `src_w`) to a destination buffer (`dst` with `row_stride`).
 * It optimizes for contiguous memory copies if possible.
 * Assumes 4 bytes per pixel (e.g., ARGB8888).
 *
 * @param src Pointer to the source pixel buffer (ARGB8888).
 * @param src_x X-offset in the source buffer.
 * @param src_y Y-offset in the source buffer.
 * @param src_w Width of the source buffer in pixels.
 * @param dst Pointer to the destination pixel buffer.
 * @param w Width of the region to copy in pixels.
 * @param h Height of the region to copy in pixels.
 * @param row_stride Stride of the destination buffer in bytes (bytes from one row to the next).
 */
static void
_eet_data_image_copy_buffer(const unsigned int *src,
                            unsigned int        src_x,
                            unsigned int        src_y,
                            unsigned int        src_w,
                            unsigned int       *dst,
                            unsigned int        w,
                            unsigned int        h,
                            unsigned int        row_stride)
{
   src += src_x + src_y * src_w;

   if (row_stride == src_w * 4 && w == src_w)
     memcpy(dst, src, row_stride * h);
   else
     {
        unsigned int *over = dst;
        unsigned int y;

        for (y = 0; y < h; ++y, src += src_w, over += row_stride)
          memcpy(over, src, w * 4);
     }
}

/**
 * @brief Internal function to decode image data from a buffer into a target surface/colorspace.
 * This function is the core decoding logic, handling different Eet image formats
 * (lossless, JPEG, ETC1/2) and parameters like source region, destination buffer,
 * and target colorspace.
 *
 * @param data Pointer to the raw encoded image data.
 * @param size Size of the encoded image data.
 * @param src_x X-coordinate of the top-left of the source region within the full image.
 * @param src_y Y-coordinate of the top-left of the source region within the full image.
 * @param src_w Width of the full source image (from header).
 * @param src_h Height of the full source image (from header, useful for fast path detection).
 * @param d Pointer to the destination buffer for decoded pixels.
 * @param w Width of the region to decode into `d`.
 * @param h Height of the region to decode into `d`.
 * @param row_stride Stride of the destination buffer `d` in bytes.
 * @param alpha Alpha flag from the image header.
 * @param comp Compression flag/type from the image header.
 * @param quality Quality flag from the image header.
 * @param lossy Encoding type from the image header.
 * @param cspace Target `Eet_Colorspace` for the decoded output in `d`.
 * @return 1 on success, 0 on failure.
 */
static int
_eet_data_image_decode_inside(const void   *data,
                              int           size,
                              unsigned int  src_x,
                              unsigned int  src_y,
                              unsigned int  src_w,
                              unsigned int  src_h, /* useful for fast path detection */
                              unsigned int *d,
                              unsigned int  w,
                              unsigned int  h,
                              unsigned int  row_stride,
                              int           alpha,
                              int           comp,
                              int           quality,
                              Eet_Image_Encoding lossy,
                              Eet_Colorspace cspace)
{
   _eet_image_endian_check();

   if (lossy == EET_IMAGE_LOSSLESS && quality == 100)
     {
        unsigned int *body;

        body = ((unsigned int *)data) + 8;
        if (!comp)
          _eet_data_image_copy_buffer(body, src_x, src_y, src_w, d,
                                      w, h, row_stride);
        else
          {
             Eina_Binbuf *in;
             Eina_Binbuf *out;
             Eina_Bool expanded;

             in = eina_binbuf_manage_new((const unsigned char *) body, size - 8 * sizeof (int), EINA_TRUE);
             if (!in) return 0;

             if ((src_h == h) && (src_w == w) && (row_stride == src_w * 4))
               {
                  out = eina_binbuf_manage_new((void*) d, w * h * 4, EINA_TRUE);
                  expanded = emile_expand(in, out, eet_2_emile_compressor(comp));
                  eina_binbuf_free(in);
                  eina_binbuf_free(out);
                  if (!expanded) return 0;
               }
             else
               {
                  /* FIXME: This could create a huge alloc. So
                     compressed data and tile could not always work.*/
                  out = emile_decompress(in,
                                         eet_2_emile_compressor(comp),
                                         w * h * 4);
                  eina_binbuf_free(in);
                  if (!out) return 0;

                  _eet_data_image_copy_buffer((const unsigned int *) eina_binbuf_string_get(out),
                                              src_x, src_y, src_w, d,
                                              w, h, row_stride);

                  eina_binbuf_free(out);
               }
          }

        /* Fix swapiness. */
        _eet_image_endian_swap(d, w * h);
     }
   else if (comp == 0 && lossy == EET_IMAGE_JPEG)
     {
        if (alpha)
          {
             unsigned const char *dt;
             int header[8];
             int sz1, sz2;

             memcpy(header, data, 32);
             _eet_image_endian_swap(header, 8);

             sz1 = header[1];
             sz2 = header[2];
             if ((sz1 <= 0) || (sz2 <= 0) || ((sz1 + sz2) > (size - 12)))
               {
                  return 0;
               }
             dt = data;
             dt += 12;

             if (eet_data_image_jpeg_rgb_decode(dt, sz1, src_x, src_y, d, w, h,
                                                cspace))
               {
                  dt += sz1;
                  if (!eet_data_image_jpeg_alpha_decode(dt, sz2, src_x, src_y,
                                                        d, w, h, cspace))
                    return 0;
               }
          }
        else if (!eet_data_image_jpeg_rgb_decode(data, size, src_x, src_y, d, w,
                                                 h, cspace))
          return 0;
     }
   else if ((lossy == EET_IMAGE_ETC1) ||
            (lossy == EET_IMAGE_ETC2_RGB) ||
            (lossy == EET_IMAGE_ETC2_RGBA) ||
            (lossy == EET_IMAGE_ETC1_ALPHA))
     {
        return eet_data_image_etc2_decode(data, size, d,
                                          src_x, src_y, src_w, src_h,
                                          alpha, cspace, lossy);
     }
   else
     abort();

   return 1;
}

/**
 * @brief Decodes image data from a raw buffer, with optional decryption.
 *
 * This function takes a buffer of encoded image data, optionally decrypts it,
 * decodes its header to get image metadata, allocates a buffer for the full
 * ARGB8888 pixel data, and then decodes the image into this new buffer.
 *
 * @param data Pointer to the raw encoded image data buffer.
 * @param cipher_key Optional decryption key. If NULL, data is assumed not encrypted.
 * @param size Size of the `data` buffer in bytes.
 * @param[out] w Pointer to store the width of the decoded image. Can be NULL.
 * @param[out] h Pointer to store the height of the decoded image. Can be NULL.
 * @param[out] alpha Pointer to store the alpha flag. Can be NULL.
 * @param[out] comp Pointer to store compression information. Can be NULL.
 * @param[out] quality Pointer to store quality information. Can be NULL.
 * @param[out] lossy Pointer to store the encoding type. Can be NULL.
 * @return A pointer to a newly allocated buffer containing the decoded ARGB8888 pixel data.
 *         The caller is responsible for freeing this buffer. Returns NULL on failure.
 */
EAPI void *
eet_data_image_decode_cipher(const void   *data,
                             const char   *cipher_key,
                             int           size,
                             unsigned int *w,
                             unsigned int *h,
                             int          *alpha,
                             int          *comp,
                             int          *quality,
                             Eet_Image_Encoding *lossy)
{
   unsigned int *d = NULL;
   unsigned int iw, ih;
   int ialpha, icompress, iquality;
   Eet_Image_Encoding ilossy;
   void *deciphered_d = NULL;
   unsigned int deciphered_sz = 0;

   if (!data)
     return NULL;

   if (cipher_key)
     {
        if (!eet_decipher(data, size, cipher_key, strlen(cipher_key),
                          &deciphered_d, &deciphered_sz))
          {
             data = deciphered_d;
             size = deciphered_sz;
          }
        else
        if (deciphered_d)
          free(deciphered_d);
     }

   /* All check are done during header decode, this simplify the code a lot. */
   if (!eet_data_image_header_decode(data, size, &iw, &ih, &ialpha, &icompress,
                                     &iquality, &ilossy))
     return NULL;

   d = malloc(iw * ih * 4);
   if (!d)
     return NULL;

   if (!_eet_data_image_decode_inside(data, size, 0, 0, iw, ih, d, iw, ih, iw *
                                      4, ialpha, icompress, iquality, ilossy,
                                      EET_COLORSPACE_ARGB8888))
     {
        free(d);
        return NULL;
     }

   if (w)
     *w = iw;

   if (h)
     *h = ih;

   if (alpha)
     *alpha = ialpha;

   if (comp)
     *comp = icompress;

   if (quality)
     *quality = iquality;

   if (lossy)
     *lossy = ilossy;

   return d;
}

/**
 * @brief Decodes image data from a raw buffer.
 * This is a convenience wrapper around `eet_data_image_decode_cipher` with `cipher_key` set to NULL.
 *
 * @param data Pointer to the raw encoded image data buffer.
 * @param size Size of the `data` buffer.
 * @param[out] w Pointer to store image width. Can be NULL.
 * @param[out] h Pointer to store image height. Can be NULL.
 * @param[out] alpha Pointer to store alpha flag. Can be NULL.
 * @param[out] comp Pointer to store compression info. Can be NULL.
 * @param[out] quality Pointer to store quality info. Can be NULL.
 * @param[out] lossy Pointer to store encoding type. Can be NULL.
 * @return A pointer to a newly allocated buffer containing decoded ARGB8888 pixel data.
 *         The caller is responsible for freeing this buffer. Returns NULL on failure.
 * @see eet_data_image_decode_cipher() for more details.
 */
EAPI void *
eet_data_image_decode(const void   *data,
                      int           size,
                      unsigned int *w,
                      unsigned int *h,
                      int          *alpha,
                      int          *comp,
                      int          *quality,
                      Eet_Image_Encoding *lossy)
{
   return eet_data_image_decode_cipher(data, NULL, size, w, h,
                                       alpha, comp, quality, lossy);
}

/**
 * @brief Decodes a region of an image from a raw buffer into a pre-allocated surface with a specific colorspace.
 *
 * This function takes a buffer of encoded image data, optionally decrypts it,
 * decodes its header, and then decodes a specified region (`src_x`, `src_y`, `w`, `h`)
 * directly into the provided buffer `d`. The output is formatted according to `cspace`
 * and `row_stride`. It also validates if the requested `cspace` is supported by the image format.
 *
 * @param data Pointer to the raw encoded image data buffer.
 * @param cipher_key Optional decryption key. If NULL, data is assumed not encrypted.
 * @param size Size of the `data` buffer in bytes.
 * @param src_x X-coordinate of the top-left of the source region within the stored image.
 * @param src_y Y-coordinate of the top-left of the source region within the stored image.
 * @param d Pointer to the pre-allocated destination buffer for the decoded pixel data.
 * @param w Width of the region to decode and the width of the target surface area in `d`.
 * @param h Height of the region to decode and the height of the target surface area in `d`.
 * @param row_stride Stride of the destination buffer `d` in bytes.
 * @param cspace The desired `Eet_Colorspace` for the output data in buffer `d`.
 * @param[out] alpha Pointer to store the alpha flag of the original image. Can be NULL.
 * @param[out] comp Pointer to store compression information. Can be NULL.
 * @param[out] quality Pointer to store quality information. Can be NULL.
 * @param[out] lossy Pointer to store the encoding type. Can be NULL.
 * @return 1 on success, 0 on failure (e.g., invalid parameters, unsupported colorspace, decode error).
 */
EAPI int
eet_data_image_decode_to_cspace_surface_cipher(const void   *data,
                                               const char   *cipher_key,
                                               int           size,
                                               unsigned int  src_x,
                                               unsigned int  src_y,
                                               unsigned int *d,
                                               unsigned int  w,
                                               unsigned int  h,
                                               unsigned int  row_stride,
                                               Eet_Colorspace cspace,
                                               int          *alpha,
                                               int          *comp,
                                               int          *quality,
                                               Eet_Image_Encoding *lossy)
{
   unsigned int iw = 0, ih = 0;
   int ialpha, icompress, iquality;
   Eet_Image_Encoding ilossy;
   const Eet_Colorspace *cspaces = NULL;
   void *deciphered_d = NULL;
   unsigned int deciphered_sz = 0;

   if (!data) return 0;

   if (cipher_key)
     {
        if (!eet_decipher(data, size, cipher_key, strlen(cipher_key),
                          &deciphered_d, &deciphered_sz))
          {
             data = deciphered_d;
             size = deciphered_sz;
          }
        else
        if (deciphered_d)
          free(deciphered_d);
     }

   /* All check are done during header decode, this simplify the code a lot. */
   if (!eet_data_image_header_advance_decode_cipher(data, NULL, size,
                                                    &iw, &ih,
                                                    &ialpha, &icompress, &iquality,
                                                    &ilossy,
                                                    &cspaces))
     return 0;

   if (!d)
     return 0;

   if (cspaces)
     {
        unsigned int i;

        for (i = 0; cspaces[i] != EET_COLORSPACE_ARGB8888; i++)
          if (cspaces[i] == cspace)
            break ;

        if (cspaces[i] != cspace)
          return 0;
     }
   else
     {
        if ((cspace != EET_COLORSPACE_ARGB8888) ||
            ((cspace == EET_COLORSPACE_ARGB8888) && (w * 4 > row_stride)))
          return 0;
     }

   if (w > iw || h > ih)
     return 0;

   if (!_eet_data_image_decode_inside(data, size, src_x, src_y, iw, ih, d, w, h,
                                      row_stride, ialpha, icompress, iquality,
                                      ilossy, cspace))
     return 0;

   if (alpha)
     *alpha = ialpha;

   if (comp)
     *comp = icompress;

   if (quality)
     *quality = iquality;

   if (lossy)
     *lossy = ilossy;

   return 1;
}

/**
 * @brief Decodes a region of an image from a raw buffer into a pre-allocated ARGB8888 surface.
 * This is a convenience wrapper for `eet_data_image_decode_to_cspace_surface_cipher`
 * with `cspace` fixed to `EET_COLORSPACE_ARGB8888`.
 *
 * @param data Pointer to the raw encoded image data buffer.
 * @param cipher_key Optional decryption key.
 * @param size Size of the `data` buffer.
 * @param src_x X-coordinate of the source region.
 * @param src_y Y-coordinate of the source region.
 * @param d Pointer to the pre-allocated destination buffer (ARGB8888).
 * @param w Width of the region to decode.
 * @param h Height of the region to decode.
 * @param row_stride Row stride of the destination buffer `d`.
 * @param[out] alpha Pointer to store the alpha flag. Can be NULL.
 * @param[out] comp Pointer to store compression information. Can be NULL.
 * @param[out] quality Pointer to store quality information. Can be NULL.
 * @param[out] lossy Pointer to store the encoding type. Can be NULL.
 * @return 1 on success, 0 on failure.
 * @see eet_data_image_decode_to_cspace_surface_cipher() for more details.
 */
EAPI int
eet_data_image_decode_to_surface_cipher(const void   *data,
                                        const char   *cipher_key,
                                        int           size,
                                        unsigned int  src_x,
                                        unsigned int  src_y,
                                        unsigned int *d,
                                        unsigned int  w,
                                        unsigned int  h,
                                        unsigned int  row_stride,
                                        int          *alpha,
                                        int          *comp,
                                        int          *quality,
                                        Eet_Image_Encoding *lossy)
{
   return eet_data_image_decode_to_cspace_surface_cipher(data, cipher_key, size, src_x, src_y, d, w, h, row_stride, EET_COLORSPACE_ARGB8888, alpha, comp, quality, lossy);
}

/**
 * @brief Decodes a region of an image from a raw buffer into a pre-allocated ARGB8888 surface (no cipher).
 * This is a convenience wrapper for `eet_data_image_decode_to_surface_cipher`
 * with `cipher_key` set to NULL.
 *
 * @param data Pointer to the raw encoded image data buffer.
 * @param size Size of the `data` buffer.
 * @param src_x X-coordinate of the source region.
 * @param src_y Y-coordinate of the source region.
 * @param d Pointer to the pre-allocated destination buffer (ARGB8888).
 * @param w Width of the region to decode.
 * @param h Height of the region to decode.
 * @param row_stride Row stride of the destination buffer `d`.
 * @param[out] alpha Pointer to store the alpha flag. Can be NULL.
 * @param[out] comp Pointer to store compression information. Can be NULL.
 * @param[out] quality Pointer to store quality information. Can be NULL.
 * @param[out] lossy Pointer to store the encoding type. Can be NULL.
 * @return 1 on success, 0 on failure.
 * @see eet_data_image_decode_to_surface_cipher() for more details.
 */
EAPI int
eet_data_image_decode_to_surface(const void   *data,
                                 int           size,
                                 unsigned int  src_x,
                                 unsigned int  src_y,
                                 unsigned int *d,
                                 unsigned int  w,
                                 unsigned int  h,
                                 unsigned int  row_stride,
                                 int          *alpha,
                                 int          *comp,
                                 int          *quality,
                                 Eet_Image_Encoding *lossy)
{
   return eet_data_image_decode_to_surface_cipher(data, NULL, size,
                                                  src_x, src_y, d,
                                                  w, h, row_stride,
                                                  alpha, comp, quality,
                                                  lossy);
}

