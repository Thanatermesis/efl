#include "evas_common_private.h"
#include "evas_convert_rgb_32.h"
#ifdef BUILD_NEON
# include <arm_neon.h>
#endif

// tiled rotate is faster in every case i've tested, so just use this
// by default.
#define TILE_ROTATE 1

/**
 * @brief Converts an ARGB source image to a 32bpp ARGB destination image.
 * @see evas_common_convert_rgba_to_32bpp_rgb_8888 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_rgb_8888 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA32 *dst_ptr;
   int y;
   Gfx_Func_Copy func;

   dst_ptr = (DATA32 *)dst;
   src_ptr = src;

   func = evas_common_draw_func_copy_get(w, 0);

   for (y = 0; y < h; y++)
     {
        func(src_ptr, dst_ptr, w);
        src_ptr += w + src_jump;
        dst_ptr += w + dst_jump;
     }
   return;
}

/**
 * @brief Converts an ARGB source image to a 32bpp ARGB destination image, with 180-degree rotation.
 * @see evas_common_convert_rgba_to_32bpp_rgb_8888_rot_180 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_rgb_8888_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA32 *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_180();

   *dst_ptr = *src_ptr;

   CONVERT_LOOP_END_ROT_180();
   return;
}

// {{{ TILE_ROTATE implementation
#ifdef TILE_ROTATE

// {{{ NEON optimized rotation macros
# ifdef BUILD_NEON
/**
 * @def ROT90_QUAD_COPY_LOOP(pix_type)
 * @brief Macro for NEON-optimized 90-degree rotation of 4xN blocks.
 *
 * This macro, if NEON is available and width is a multiple of 4, rotates
 * a block of pixels by processing 4 pixels (a quad) at a time using NEON intrinsics.
 * It reads 4 vertically adjacent pixels from the source (transposed) and writes them
 * as 4 horizontally adjacent pixels to the destination.
 * Falls back to a generic loop for widths not divisible by 4 or if NEON is not available.
 *
 * @param pix_type The data type of a pixel (e.g., DATA32).
 */
#  define ROT90_QUAD_COPY_LOOP(pix_type) \
   if (evas_common_cpu_has_feature(CPU_FEATURE_NEON)) { \
      if ((w % 4) == 0) { \
         int klght = 4 * src_stride; \
         for (y = 0; y < h; y++) { \
            const pix_type *s = &(src[h - y - 1]); \
            pix_type *d = &(dst[dst_stride * y]); \
            const pix_type *ptr1 = s; \
            const pix_type *ptr2 = ptr1 + src_stride; \
            const pix_type *ptr3 = ptr2 + src_stride; \
            const pix_type *ptr4 = ptr3 + src_stride; \
            for(x = 0; x < w; x += 4) { \
               pix_type s_array[4] = { *ptr1, *ptr2, *ptr3, *ptr4 }; \
               vst1q_u32(d, vld1q_u32(s_array)); \
               d += 4; \
               ptr1 += klght; \
               ptr2 += klght; \
               ptr3 += klght; \
               ptr4 += klght; \
            } \
         } \
      } \
      else { \
         for (y = 0; y < h; y++) { \
            const pix_type *s = &(src[h - y - 1]); \
            pix_type *d = &(dst[dst_stride * y]); \
            for (x = 0; x < w; x++) { \
               *d++ = *s; \
               s += src_stride; \
            } \
         } \
      } \
   } \
   else
/**
 * @def ROT270_QUAD_COPY_LOOP(pix_type)
 * @brief Macro for NEON-optimized 270-degree rotation of 4xN blocks.
 *
 * Similar to ROT90_QUAD_COPY_LOOP, but performs a 270-degree rotation.
 * It reads 4 vertically adjacent pixels from the source (transposed, reversed order)
 * and writes them as 4 horizontally adjacent pixels to the destination.
 * Falls back to a generic loop for widths not divisible by 4 or if NEON is not available.
 *
 * @param pix_type The data type of a pixel (e.g., DATA32).
 */
#  define ROT270_QUAD_COPY_LOOP(pix_type) \
   if (evas_common_cpu_has_feature(CPU_FEATURE_NEON)) { \
      if ((w % 4) == 0) { \
         int klght = 4 * src_stride; \
         for (y = 0; y < h; y++) { \
            const pix_type *s = &(src[(src_stride * (w - 1)) + y]); \
            pix_type *d = &(dst[dst_stride * y]); \
            const pix_type *ptr1 = s; \
            const pix_type *ptr2 = ptr1 - src_stride; \
            const pix_type *ptr3 = ptr2 - src_stride; \
            const pix_type *ptr4 = ptr3 - src_stride; \
            for(x = 0; x < w; x += 4) { \
               pix_type s_array[4] = { *ptr1, *ptr2, *ptr3, *ptr4 }; \
               vst1q_u32(d, vld1q_u32(s_array)); \
               d += 4; \
               ptr1 -= klght; \
               ptr2 -= klght; \
               ptr3 -= klght; \
               ptr4 -= klght; \
            } \
         } \
      } \
      else { \
        for (y = 0; y < h; y++) { \
           const pix_type *s = &(src[(src_stride * (w - 1)) + y]); \
           pix_type *d = &(dst[dst_stride * y]); \
           for (x = 0; x < w; x++) { \
              *d++ = *s; \
              s -= src_stride; \
           } \
        } \
      } \
   } \
   else
# else // BUILD_NEON not defined
// Define empty macros if NEON is not being built
#  define ROT90_QUAD_COPY_LOOP(pix_type)
#  define ROT270_QUAD_COPY_LOOP(pix_type)
# endif // BUILD_NEON
// }}} NEON optimized rotation macros

/**
 * @def FAST_SIMPLE_ROTATE(suffix, pix_type)
 * @brief Macro to generate a set of optimized rotation functions for a given pixel type.
 *
 * This macro generates four static functions for a given `pix_type` and `suffix`:
 * - `blt_rotated_90_trivial_##suffix`: Performs a basic 90-degree clockwise rotation.
 * - `blt_rotated_270_trivial_##suffix`: Performs a basic 270-degree clockwise (90-degree counter-clockwise) rotation.
 * - `blt_rotated_90_##suffix`: Performs a 90-degree rotation optimized for cache performance using tiling.
 * - `blt_rotated_270_##suffix`: Performs a 270-degree rotation optimized for cache performance using tiling.
 *
 * The `_trivial_` versions are the core rotation logic, potentially using NEON via
 * `ROT90_QUAD_COPY_LOOP` and `ROT270_QUAD_COPY_LOOP`.
 * The non-trivial versions wrap the trivial ones, adding logic to process the image in
 * cache-line-sized tiles to improve memory access patterns.
 *
 * @param suffix Suffix to append to the generated function names (e.g., "8888").
 * @param pix_type The data type of a pixel (e.g., DATA32).
 */
# define FAST_SIMPLE_ROTATE(suffix, pix_type) \
   /** @brief Performs a basic 90-degree clockwise rotation of an image block. */ \
   static void \
   blt_rotated_90_trivial_##suffix(pix_type       * restrict dst, \
                                   int              dst_stride, \
                                   const pix_type * restrict src, \
                                   int              src_stride, \
                                   int              w, \
                                   int              h) \
   { \
      int x, y; \
      ROT90_QUAD_COPY_LOOP(pix_type) { \
         for (y = 0; y < h; y++) { \
            const pix_type *s = &(src[h - y - 1]); \
            pix_type *d = &(dst[dst_stride * y]); \
            for (x = 0; x < w; x++) { \
               *d++ = *s; \
               s += src_stride; \
            } \
         } \
      } \
   } \
   /** @brief Performs a basic 270-degree clockwise rotation (90-degree counter-clockwise) of an image block. */ \
   static void \
   blt_rotated_270_trivial_##suffix(pix_type       * restrict dst, \
                                    int              dst_stride, \
                                    const pix_type * restrict src, \
                                    int              src_stride, \
                                    int              w, \
                                    int              h) \
   { \
      int x, y; \
      ROT270_QUAD_COPY_LOOP(pix_type) { \
         for (y = 0; y < h; y++) { \
            const pix_type *s = &(src[(src_stride * (w - 1)) + y]); \
            pix_type *d = &(dst[dst_stride * y]); \
            for (x = 0; x < w; x++) { \
               *d++ = *s; \
               s -= src_stride; \
            } \
         } \
      } \
   } \
   /** @brief Performs a 90-degree clockwise rotation using tiling for cache optimization. */ \
   static void \
   blt_rotated_90_##suffix(pix_type       * restrict dst, \
                           int              dst_stride, \
                           const pix_type * restrict src, \
                           int              src_stride, \
                           int              w, \
                           int              h) \
   { \
      int x, leading_pixels = 0, trailing_pixels = 0; \
      const int TILE_SIZE = TILE_CACHE_LINE_SIZE / sizeof(pix_type); \
      if ((uintptr_t)dst & (TILE_CACHE_LINE_SIZE - 1)) { \
         leading_pixels = TILE_SIZE - \
         (((uintptr_t)dst & (TILE_CACHE_LINE_SIZE - 1)) / sizeof(pix_type)); \
         if (leading_pixels > w) leading_pixels = w; \
         blt_rotated_90_trivial_##suffix(dst, \
                                         dst_stride, \
                                         src, \
                                         src_stride, \
                                         leading_pixels, \
                                         h); \
         dst += leading_pixels; \
         src += leading_pixels * src_stride; \
         w -= leading_pixels; \
      } \
      if ((uintptr_t)(dst + w) & (TILE_CACHE_LINE_SIZE - 1)) { \
         trailing_pixels = (((uintptr_t)(dst + w) & \
                             (TILE_CACHE_LINE_SIZE - 1)) / sizeof(pix_type)); \
         if (trailing_pixels > w) trailing_pixels = w; \
         w -= trailing_pixels; \
      } \
      for (x = 0; x < w; x += TILE_SIZE) { \
         blt_rotated_90_trivial_##suffix(dst + x, \
                                         dst_stride, \
                                         &(src[src_stride * x]), \
                                         src_stride, \
                                         TILE_SIZE, \
                                         h); \
      } \
      if (trailing_pixels) \
        blt_rotated_90_trivial_##suffix(dst + w, \
                                        dst_stride, \
                                        &(src[src_stride * w]), \
                                        src_stride, \
                                        trailing_pixels, \
                                        h); \
   } \
   /** @brief Performs a 270-degree clockwise rotation (90-degree counter-clockwise) using tiling for cache optimization. */ \
   static void \
   blt_rotated_270_##suffix(pix_type       * restrict dst, \
                            int              dst_stride, \
                            const pix_type * restrict src, \
                            int              src_stride, \
                            int              w, \
                            int              h) \
   { \
      int x, leading_pixels = 0, trailing_pixels = 0; \
      const int TILE_SIZE = TILE_CACHE_LINE_SIZE / sizeof(pix_type); \
      if ((uintptr_t)dst & (TILE_CACHE_LINE_SIZE - 1)) { \
         leading_pixels = TILE_SIZE - \
         (((uintptr_t)dst & (TILE_CACHE_LINE_SIZE - 1)) / sizeof(pix_type)); \
         if (leading_pixels > w) leading_pixels = w; \
         blt_rotated_270_trivial_##suffix(dst, \
                                          dst_stride, \
                                          &(src[src_stride * (w - leading_pixels)]), \
                                          src_stride, \
                                          leading_pixels, \
                                          h); \
         dst += leading_pixels; \
         w -= leading_pixels; \
      } \
      if ((uintptr_t)(dst + w) & (TILE_CACHE_LINE_SIZE - 1)) { \
         trailing_pixels = (((uintptr_t)(dst + w) & \
                             (TILE_CACHE_LINE_SIZE - 1)) / sizeof(pix_type)); \
         if (trailing_pixels > w) trailing_pixels = w; \
         w -= trailing_pixels; \
         src += trailing_pixels * src_stride; \
      } \
      for (x = 0; x < w; x += TILE_SIZE) { \
         blt_rotated_270_trivial_##suffix(dst + x, \
                                          dst_stride, \
                                          &(src[src_stride * (w - x - TILE_SIZE)]), \
                                          src_stride, \
                                          TILE_SIZE, \
                                          h); \
      } \
      if (trailing_pixels) \
        blt_rotated_270_trivial_##suffix(dst + w, \
                                         dst_stride, \
                                         src - (trailing_pixels * src_stride), \
                                         src_stride, \
                                         trailing_pixels, \
                                         h); \
   }

FAST_SIMPLE_ROTATE(8888, DATA32)

#endif // TILE_ROTATE
// }}} TILE_ROTATE implementation

/**
 * @brief Converts an ARGB source image to a 32bpp ARGB destination image, with 270-degree rotation.
 * @see evas_common_convert_rgba_to_32bpp_rgb_8888_rot_270 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_rgb_8888_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
#ifdef TILE_ROTATE
   blt_rotated_270_8888((DATA32 *)dst, dst_jump + w,
                        src, src_jump + h,
                        w, h);
#else
   DATA32 *src_ptr *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_270();

   *dst_ptr = *src_ptr;

   CONVERT_LOOP_END_ROT_270();
#endif
   return;
}

/*
 * The following section includes commented-out performance measuring code.
 * It can be enabled during optimization work to compare different implementations.
 * It uses clock_gettime(CLOCK_MONOTONIC, ...) to measure execution time.
 */
/* speed measuring code - enable when optimizing to compare
#include <time.h>
static double
get_time(void)
{
   struct timespec t;

   clock_gettime(CLOCK_MONOTONIC, &t);
   return (double)t.tv_sec + (((double)t.tv_nsec) / 1000000000.0);
}
*/

/**
 * @brief Converts an ARGB source image to a 32bpp ARGB destination image, with 90-degree rotation.
 * @see evas_common_convert_rgba_to_32bpp_rgb_8888_rot_90 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_rgb_8888_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
/*
   static double tt = 0.0;
   static unsigned long long pt = 0;
   double t0 = get_time();
 */
#ifdef TILE_ROTATE
   blt_rotated_90_8888((DATA32 *)dst, dst_jump + w,
                       src, src_jump + h,
                       w, h);
#else
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;
   CONVERT_LOOP_START_ROT_90();

   *dst_ptr = *src_ptr;

   CONVERT_LOOP_END_ROT_90();
#endif
/*
   double t1 = get_time();
   tt += t1 - t0;
   pt += (w * h);
   printf("%1.2f mpix/sec (%1.9f @ %1.9f)\n", (double)pt / (tt * 1000000), tt, t1);
*/
}

/**
 * @brief Converts an ARGB source image to a 32bpp RGB0 destination image.
 * @see evas_common_convert_rgba_to_32bpp_rgbx_8888 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_rgbx_8888 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_0();

//   *dst_ptr = (R_VAL(src_ptr) << 24) | (G_VAL(src_ptr) << 16) | (B_VAL(src_ptr) << 8);
   *dst_ptr = (*src_ptr << 8);

   CONVERT_LOOP_END_ROT_0();
   return;
}

/**
 * @brief Converts an ARGB source image to a 32bpp RGB0 destination image, with 180-degree rotation.
 * @see evas_common_convert_rgba_to_32bpp_rgbx_8888_rot_180 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_rgbx_8888_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_180();

//   *dst_ptr = (R_VAL(src_ptr) << 24) | (G_VAL(src_ptr) << 16) | (B_VAL(src_ptr) << 8);
   *dst_ptr = (*src_ptr << 8);

   CONVERT_LOOP_END_ROT_180();
   return;
}

/**
 * @brief Converts an ARGB source image to a 32bpp RGB0 destination image, with 270-degree rotation.
 * @see evas_common_convert_rgba_to_32bpp_rgbx_8888_rot_270 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_rgbx_8888_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_270();

//   *dst_ptr = (R_VAL(src_ptr) << 24) | (G_VAL(src_ptr) << 16) | (B_VAL(src_ptr) << 8);
   *dst_ptr = (*src_ptr << 8);

   CONVERT_LOOP_END_ROT_270();
   return;
}

/**
 * @brief Converts an ARGB source image to a 32bpp RGB0 destination image, with 90-degree rotation.
 * @see evas_common_convert_rgba_to_32bpp_rgbx_8888_rot_90 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_rgbx_8888_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_90();

//   *dst_ptr = (R_VAL(src_ptr) << 24) | (G_VAL(src_ptr) << 16) | (B_VAL(src_ptr) << 8);
   *dst_ptr = (*src_ptr << 8);

   CONVERT_LOOP_END_ROT_90();
   return;
}

/**
 * @brief Converts an ARGB source image to a 32bpp 0BGR destination image.
 * @see evas_common_convert_rgba_to_32bpp_bgr_8888 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_bgr_8888 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_0();

   *dst_ptr = (B_VAL(src_ptr) << 16) | (G_VAL(src_ptr) << 8) | (R_VAL(src_ptr));

   CONVERT_LOOP_END_ROT_0();
   return;
}

/**
 * @brief Converts an ARGB source image to a 32bpp 0BGR destination image, with 180-degree rotation.
 * @see evas_common_convert_rgba_to_32bpp_bgr_8888_rot_180 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_bgr_8888_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_180();

   *dst_ptr = (B_VAL(src_ptr) << 16) | (G_VAL(src_ptr) << 8) | (R_VAL(src_ptr));

   CONVERT_LOOP_END_ROT_180();
   return;
}

/**
 * @brief Converts an ARGB source image to a 32bpp 0BGR destination image, with 270-degree rotation.
 * @see evas_common_convert_rgba_to_32bpp_bgr_8888_rot_270 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_bgr_8888_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_270();

   *dst_ptr = (B_VAL(src_ptr) << 16) | (G_VAL(src_ptr) << 8) | (R_VAL(src_ptr));

   CONVERT_LOOP_END_ROT_270();
   return;
}

/**
 * @brief Converts an ARGB source image to a 32bpp 0BGR destination image, with 90-degree rotation.
 * @see evas_common_convert_rgba_to_32bpp_bgr_8888_rot_90 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_bgr_8888_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_90();

   *dst_ptr = (B_VAL(src_ptr) << 16) | (G_VAL(src_ptr) << 8) | (R_VAL(src_ptr));

   CONVERT_LOOP_END_ROT_90();
   return;
}

/**
 * @brief Converts an ARGB source image to a 32bpp BGR0 destination image.
 * @see evas_common_convert_rgba_to_32bpp_bgrx_8888 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_bgrx_8888 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_0();

   *dst_ptr = (B_VAL(src_ptr) << 24) | (G_VAL(src_ptr) << 16) | (R_VAL(src_ptr) << 8);

   CONVERT_LOOP_END_ROT_0();
   return;
}

/**
 * @brief Converts an ARGB source image to a 32bpp BGR0 destination image, with 180-degree rotation.
 * @see evas_common_convert_rgba_to_32bpp_bgrx_8888_rot_180 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_bgrx_8888_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_180();

   *dst_ptr = (B_VAL(src_ptr) << 24) | (G_VAL(src_ptr) << 16) | (R_VAL(src_ptr) << 8);

   CONVERT_LOOP_END_ROT_180();
   return;
}

/**
 * @brief Converts an ARGB source image to a 32bpp BGR0 destination image, with 270-degree rotation.
 * @see evas_common_convert_rgba_to_32bpp_bgrx_8888_rot_270 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_bgrx_8888_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_270();

   *dst_ptr = (B_VAL(src_ptr) << 24) | (G_VAL(src_ptr) << 16) | (R_VAL(src_ptr) << 8);

   CONVERT_LOOP_END_ROT_270();
   return;
}

/**
 * @brief Converts an ARGB source image to a 32bpp BGR0 destination image, with 90-degree rotation.
 * @see evas_common_convert_rgba_to_32bpp_bgrx_8888_rot_90 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_bgrx_8888_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_90();

   *dst_ptr = (B_VAL(src_ptr) << 24) | (G_VAL(src_ptr) << 16) | (R_VAL(src_ptr) << 8);

   CONVERT_LOOP_END_ROT_90();
   return;
}

/**
 * @brief Converts an ARGB source image to a 32bpp RGB666 destination image.
 * @see evas_common_convert_rgba_to_32bpp_rgb_666 in evas_convert_rgb_32.h
 */
void
evas_common_convert_rgba_to_32bpp_rgb_666(DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr, *dst_ptr;
   int x, y;

   dst_ptr = (DATA32 *)dst;

   CONVERT_LOOP_START_ROT_0();

   *dst_ptr =
     (((R_VAL(src_ptr) << 12) | (B_VAL(src_ptr) >> 2)) & 0x03f03f) |
     ((G_VAL(src_ptr) << 4) & 0x000fc0);

   CONVERT_LOOP_END_ROT_0();
   return;
}
