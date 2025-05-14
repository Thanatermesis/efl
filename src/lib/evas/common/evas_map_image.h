#ifndef _EVAS_MAP_H
#define _EVAS_MAP_H

/**
 * @brief Callback function type for RGBA map operations.
 * @param src The source image.
 * @param dst The destination image.
 * @param dc The draw context.
 * @param p Array of map points defining the transformation.
 * @param smooth EINA_TRUE for smooth scaling, EINA_FALSE otherwise.
 * @param level The quality level for the map rendering.
 */
typedef void (*Evas_Common_Map_RGBA_Cb)            (RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, RGBA_Map_Point *p, int smooth, int level);

/**
 * @brief Callback function type for threaded RGBA map operations.
 * @param src The source image.
 * @param dst The destination image.
 * @param dc The draw context.
 * @param map The map structure containing transformation points.
 * @param smooth EINA_TRUE for smooth scaling, EINA_FALSE otherwise.
 * @param level The quality level for the map rendering.
 * @param offset The offset for threaded processing.
 * @return EINA_TRUE if the operation was successful, EINA_FALSE otherwise.
 */
typedef Eina_Bool (*Evas_Common_Map_Thread_RGBA_Cb)     (RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, RGBA_Map *map, int smooth, int level, int offset);

/**
 * @brief Applies a generic RGBA map operation using a callback.
 *
 * This function processes an image transformation defined by map points
 * and applies it using a provided callback function. It handles clipping
 * and cutouts.
 *
 * @param src The source RGBA_Image.
 * @param dst The destination RGBA_Image.
 * @param dc The draw context.
 * @param npoints The number of points in the `points` array (typically 4 for a quad).
 * @param points An array of RGBA_Map_Point defining the transformation.
 *               Example for a quad:
 *               points[0] = {x=0, y=0, u=0, v=0, z=0, col=0xffffffff}; // Top-left
 *               points[1] = {x=W, y=0, u=SW, v=0, z=0, col=0xffffffff}; // Top-right
 *               points[2] = {x=W, y=H, u=SW, v=SH, z=0, col=0xffffffff}; // Bottom-right
 *               points[3] = {x=0, y=H, u=0, v=SH, z=0, col=0xffffffff}; // Bottom-left
 *               (W,H are destination width/height, SW,SH are source width/height in fixed point)
 * @param smooth EINA_TRUE for smooth scaling, EINA_FALSE otherwise.
 * @param level The quality level for the map rendering.
 * @param cb The callback function to perform the actual map rendering.
 */
EVAS_API void
evas_common_map_rgba_cb(RGBA_Image *src, RGBA_Image *dst,
                        RGBA_Draw_Context *dc,
                        int npoints, RGBA_Map_Point *points,
                        int smooth, int level,
                        Evas_Common_Map_RGBA_Cb cb);

/**
 * @brief Applies a generic threaded RGBA map operation using a callback.
 *
 * Similar to evas_common_map_rgba_cb, but designed for threaded execution.
 *
 * @param src The source RGBA_Image.
 * @param dst The destination RGBA_Image.
 * @param dc The draw context.
 * @param map The RGBA_Map structure containing pre-calculated span data and points.
 * @param smooth EINA_TRUE for smooth scaling, EINA_FALSE otherwise.
 * @param level The quality level for the map rendering.
 * @param offset An offset value, potentially for distributing work in threaded scenarios.
 * @param cb The callback function to perform the actual map rendering for a portion of the map.
 * @return EINA_TRUE if the operation was successful for the given portion, EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool evas_common_map_thread_rgba_cb(RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, RGBA_Map *map, int smooth, int level, int offset, Evas_Common_Map_Thread_RGBA_Cb cb);

/**
 * @brief Renders a mapped RGBA image.
 *
 * This function selects the appropriate rendering function (MMX, NEON, or generic)
 * based on CPU capabilities and anti-aliasing/smoothing flags, then calls
 * evas_common_map_rgba_cb to perform the mapping.
 *
 * @param src The source RGBA_Image.
 * @param dst The destination RGBA_Image.
 * @param dc The draw context.
 * @param npoints The number of points in the `points` array.
 * @param points An array of RGBA_Map_Point defining the transformation.
 * @param smooth EINA_TRUE for smooth scaling, EINA_FALSE otherwise.
 * @param level The quality level for the map rendering.
 */
EVAS_API void
evas_common_map_rgba(RGBA_Image *src, RGBA_Image *dst,
                      RGBA_Draw_Context *dc,
                      int npoints, RGBA_Map_Point *points,
                      int smooth, int level);

/**
 * @brief Draws a mapped RGBA image directly with specified parameters.
 *
 * This is a lower-level drawing function that bypasses some of the context setup
 * and directly calls the internal rendering routines.
 *
 * @param src The source RGBA_Image.
 * @param dst The destination RGBA_Image.
 * @param clip_x Clipping region X offset.
 * @param clip_y Clipping region Y offset.
 * @param clip_w Clipping region width.
 * @param clip_h Clipping region height.
 * @param mul_col Multiplier color (e.g., for tinting). 0xffffffff for no multiplication.
 * @param render_op The rendering operation (e.g., RENDER_OP_BLEND, RENDER_OP_COPY).
 * @param npoints The number of points in the `p` array.
 * @param p An array of RGBA_Map_Point defining the transformation.
 * @param smooth EINA_TRUE for smooth scaling, EINA_FALSE otherwise.
 * @param anti_alias EINA_TRUE for anti-aliased rendering, EINA_FALSE otherwise.
 * @param level The quality level for the map rendering.
 * @param mask_ie Optional mask image.
 * @param mask_x X offset for the mask image.
 * @param mask_y Y offset for the mask image.
 */
EVAS_API void evas_common_map_rgba_draw(RGBA_Image *src, RGBA_Image *dst, int clip_x, int clip_y, int clip_w, int clip_h, DATA32 mul_col, int render_op, int npoints, RGBA_Map_Point *p, int smooth, Eina_Bool anti_alias, int level, RGBA_Image *mask_ie, int mask_x, int mask_y);

/**
 * @brief Prepares an RGBA_Map structure for rendering.
 *
 * This function calculates span data based on the map points, source/destination images,
 * and draw context (including clipping and cutouts). The prepared data is stored
 * in `m->engine_data`.
 *
 * @param src The source RGBA_Image.
 * @param dst The destination RGBA_Image.
 * @param dc The draw context.
 * @param m Pointer to the RGBA_Map structure to prepare. The `m->pts` should be set
 *          before calling this function. `m->engine_data` will be populated.
 * @return EINA_TRUE if preparation was successful and rendering can proceed,
 *         EINA_FALSE otherwise (e.g., if the map is entirely clipped).
 */
EVAS_API Eina_Bool
evas_common_map_rgba_prepare(RGBA_Image *src, RGBA_Image *dst,
                             RGBA_Draw_Context *dc,
                             RGBA_Map *m);
/**
 * @brief Executes the rendering of a previously prepared RGBA_Map.
 *
 * This function uses the span data calculated by `evas_common_map_rgba_prepare`
 * (stored in `m->engine_data`) to render the mapped image. It selects the
 * appropriate internal rendering function (MMX, NEON, generic) based on CPU capabilities.
 *
 * @param clip The overall clipping rectangle for this rendering operation.
 * @param src The source RGBA_Image.
 * @param dst The destination RGBA_Image.
 * @param dc The draw context.
 * @param m Pointer to the prepared RGBA_Map structure (after calling `evas_common_map_rgba_prepare`).
 * @param smooth EINA_TRUE for smooth scaling, EINA_FALSE otherwise.
 * @param level The quality level for the map rendering.
 */
EVAS_API void
evas_common_map_rgba_do(const Eina_Rectangle *clip,
                        RGBA_Image *src, RGBA_Image *dst,
			RGBA_Draw_Context *dc,
			const RGBA_Map *m,
			int smooth, int level);
/**
 * @brief Cleans up resources associated with an RGBA_Map.
 *
 * Frees memory allocated for span data and cutouts stored in `m->engine_data`
 * during `evas_common_map_rgba_prepare`.
 *
 * @param m Pointer to the RGBA_Map structure to clean.
 */
EVAS_API void
evas_common_map_rgba_clean(RGBA_Map *m);

#endif /* _EVAS_MAP_H */
