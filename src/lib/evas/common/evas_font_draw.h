#ifndef _EVAS_FONT_DRAW_H
#define _EVAS_FONT_DRAW_H

#include "../include/evas_common_private.h"

/**
 * @file
 * @brief Functions for drawing text and glyphs in Evas.
 */

/**
 * @typedef Evas_Common_Font_Draw_Cb
 * @brief Callback function type for drawing glyphs within a specific region.
 *
 * This callback is used by evas_common_font_draw_cb to perform the actual
 * drawing of glyphs after clipping and cutouts have been processed.
 *
 * @param dst The destination RGBA_Image to draw onto.
 * @param dc The RGBA_Draw_Context containing drawing parameters.
 * @param x The base x-coordinate for drawing the glyphs.
 * @param y The base y-coordinate for drawing the glyphs.
 * @param glyphs An Evas_Glyph_Array containing the glyphs to draw.
 * @param func The graphics function to use for rendering (e.g., compositing).
 * @param ext_x The x-coordinate of the current drawing extent (clip region).
 * @param ext_y The y-coordinate of the current drawing extent (clip region).
 * @param ext_w The width of the current drawing extent (clip region).
 * @param ext_h The height of the current drawing extent (clip region).
 * @param im_w The total width of the destination image.
 * @param im_h The total height of the destination image.
 * @return EINA_TRUE if drawing was performed, EINA_FALSE otherwise.
 */
typedef Eina_Bool (*Evas_Common_Font_Draw_Cb)(RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, Evas_Glyph_Array *glyphs, RGBA_Gfx_Func func, int ext_x, int ext_y, int ext_w, int ext_h, int im_w, int im_h);

/**
 * @brief Draws an array of glyphs using a callback function, handling cutouts and clipping.
 * @see evas_common_font_draw_cb in evas_font_draw.c
 */
EVAS_API Eina_Bool         evas_common_font_draw_cb              (RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, Evas_Glyph_Array *glyphs, Evas_Common_Font_Draw_Cb cb);

/**
 * @brief Draws an array of glyphs onto an RGBA image.
 * @see evas_common_font_draw in evas_font_draw.c
 */
EVAS_API void              evas_common_font_draw                 (RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, Evas_Glyph_Array *glyphs);

/**
 * @brief Draws an array of RGBA font glyphs onto an RGBA image.
 * @see evas_common_font_rgba_draw in evas_font_draw.c
 */
EVAS_API Eina_Bool         evas_common_font_rgba_draw            (RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, Evas_Glyph_Array *glyphs, RGBA_Gfx_Func func, int ext_x, int ext_y, int ext_w, int ext_h, int im_w, int im_h);

/**
 * @brief Initializes common font drawing functionalities.
 * @see evas_common_font_draw_init in evas_font_draw.c
 */
EVAS_API void              evas_common_font_draw_init            (void);

/**
 * @brief Prepares an array of glyphs for drawing based on text properties.
 * @see evas_common_font_draw_prepare in evas_font_draw.c
 */
EVAS_API void              evas_common_font_draw_prepare         (Evas_Text_Props *text_props);

/**
 * @brief Draws text applying specified cutouts and clipping.
 * @see evas_common_font_draw_do in evas_font_draw.c
 */
EVAS_API void              evas_common_font_draw_do              (const Cutout_Rects *reuse, const Eina_Rectangle *clip, RGBA_Gfx_Func func, RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, const Evas_Text_Props *text_props);

/**
 * @brief Prepares drawing context and cutouts for font rendering.
 * @see evas_common_font_draw_prepare_cutout in evas_font_draw.c
 */
EVAS_API Eina_Bool         evas_common_font_draw_prepare_cutout  (Cutout_Rects **reuse, RGBA_Image *dst, RGBA_Draw_Context *dc, RGBA_Gfx_Func *func);

/**
 * @brief Draws a single (potentially compressed) font glyph.
 * @see evas_common_font_glyph_draw in evas_font_draw.c
 */
EVAS_API void              evas_common_font_glyph_draw           (RGBA_Font_Glyph *fg, RGBA_Draw_Context *dc, RGBA_Image *dst, int dst_pitch, int dx, int dy, int dw, int dh, int cx, int cy, int cw, int ch);

#endif /* _EVAS_FONT_DRAW_H */
