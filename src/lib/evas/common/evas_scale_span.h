#ifndef _EVAS_SCALE_SPAN_H
#define _EVAS_SCALE_SPAN_H

/**
 * @file
 * @brief Functions for scaling spans of pixel data.
 *
 * These functions provide various methods for scaling pixel data,
 * including RGBA, RGBA with alpha mask, alpha mask only, and
 * operations in HSV(A) color space.
 */

/**
 * @brief Scales a span of RGBA pixels.
 * @param src Pointer to the source RGBA pixel data (0xAARRGGBB).
 * @param mask Unused parameter, typically NULL.
 * @param src_len Length of the source pixel span.
 * @param mul_col Multiplier color (0xAARRGGBB). Applied to source pixels if not 0xffffffff.
 * @param dst Pointer to the destination RGBA pixel data array.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of scaling (<0 for reverse).
 * @see evas_common_scale_rgba_span_()
 */
EVAS_API void evas_common_scale_rgba_span                       (DATA32 *src, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir);

/**
 * @brief Scales a span of RGBA pixels with an alpha mask.
 * @param src Pointer to the source RGBA pixel data (0xAARRGGBB).
 * @param mask Pointer to the alpha mask data (array of DATA8).
 * @param src_len Length of the source pixel span and mask.
 * @param mul_col Multiplier color (0xAARRGGBB). Applied to masked source pixels if not 0xffffffff.
 * @param dst Pointer to the destination RGBA pixel data array.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of scaling (<0 for reverse).
 * @see evas_common_scale_rgba_a8_span_()
 */
EVAS_API void evas_common_scale_rgba_a8_span                    (DATA32 *src, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir);

/**
 * @brief Scales an alpha mask and applies a multiplier color.
 * @param src Unused parameter, typically NULL.
 * @param mask Pointer to the source alpha mask data (array of DATA8).
 * @param src_len Length of the source alpha mask span.
 * @param mul_col Multiplier color (0xAARRGGBB). Modulated by scaled alpha.
 * @param dst Pointer to the destination RGBA pixel data array.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of scaling (<0 for reverse).
 * @see evas_common_scale_a8_span_()
 */
EVAS_API void evas_common_scale_a8_span                         (DATA32 *src, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir);

/**
 * @brief Scales an alpha mask and uses it to modulate existing destination pixels.
 * @param src Unused parameter, typically NULL.
 * @param mask Pointer to the source alpha mask data (array of DATA8).
 * @param src_len Length of the source alpha mask span.
 * @param mul_col Multiplier color (0xAARRGGBB). If not 0xffffffff, its alpha component
 *                further modulates the scaled mask.
 * @param dst Pointer to the destination RGBA pixel data array (read and modified).
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of processing (<0 for reverse).
 * @see evas_common_scale_clip_a8_span_()
 */
EVAS_API void evas_common_scale_clip_a8_span                    (DATA32 *src, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir);

/**
 * @brief Scales a span of RGBA pixels, with interpolation in HSV(A) color space.
 * @param src Pointer to the source RGBA pixel data (0xAARRGGBB).
 * @param mask Unused parameter, typically NULL.
 * @param src_len Length of the source pixel span.
 * @param mul_col Multiplier color (0xAARRGGBB). Applied after HSV interpolation and RGB conversion.
 * @param dst Pointer to the destination RGBA pixel data array.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of scaling (<0 for reverse).
 */
EVAS_API void evas_common_scale_hsva_span                       (DATA32 *src, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir);

/**
 * @brief Scales RGBA pixels with an alpha mask, with interpolation in HSV(A) color space.
 * @param src Pointer to the source RGBA pixel data (0xAARRGGBB).
 * @param mask Pointer to the alpha mask data (array of DATA8).
 * @param src_len Length of the source pixel span and mask.
 * @param mul_col Multiplier color (0xAARRGGBB). Applied after HSV interpolation and RGB conversion.
 * @param dst Pointer to the destination RGBA pixel data array.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of scaling (<0 for reverse).
 */
EVAS_API void evas_common_scale_hsva_a8_span                    (DATA32 *src, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir);


#endif /* _EVAS_SCALE_SPAN_H */
