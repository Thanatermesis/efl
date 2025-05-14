#include "evas_common_private.h"
#include "evas_blend_private.h"

#include "Ecore.h"

/**
 * @internal
 * @brief Internal function to scale a region of an RGBA image to a destination region with clipping.
 * This is the core implementation used by evas_common_scale_rgba_in_to_out_clip_sample.
 * @param src Source image.
 * @param dst Destination image.
 * @param dc Draw context.
 * @param src_region_x Source region X offset.
 * @param src_region_y Source region Y offset.
 * @param src_region_w Source region width.
 * @param src_region_h Source region height.
 * @param dst_region_x Destination region X offset.
 * @param dst_region_y Destination region Y offset.
 * @param dst_region_w Destination region width.
 * @param dst_region_h Destination region height.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool scale_rgba_in_to_out_clip_sample_internal(RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h);

/**
 * @internal
 * @brief Structure to hold data for a scaling thread task.
 * This structure contains all necessary information for a worker thread
 * to perform a part of the image scaling operation.
 */
typedef struct _Evas_Scale_Thread Evas_Scale_Thread;
/**
 * @internal
 * @brief Structure for messages passed to the scaling thread queue.
 * It contains a pointer to the actual scaling task data.
 */
typedef struct _Evas_Scale_Msg Evas_Scale_Msg;

struct _Evas_Scale_Msg
{
   Eina_Thread_Queue_Msg head; /**< Eina thread queue message header. */
   Evas_Scale_Thread *task;    /**< Pointer to the scaling task data. If NULL, signals thread to terminate. */
};

struct _Evas_Scale_Thread
{
   RGBA_Image *mask8;         /**< Optional 8-bit alpha mask image. */
   DATA32 **row_ptr;          /**< Array of pointers to the start of each source row within the scaled region.
                               *   Example: `row_ptr[y]` points to the first pixel of the y-th row in the source image
                               *   that corresponds to the destination image's scaled output. */
   DATA32 *dptr;              /**< Pointer to the destination image data buffer where scaled output is written. */
   int *lin_ptr;              /**< Array of X-coordinates in the source image, corresponding to each column in the destination's scaled region.
                               *   Example: `lin_ptr[x]` gives the source X-coordinate for the destination column `x`. */

   RGBA_Gfx_Func func;        /**< Primary graphics function for blending/compositing pixels. */
   RGBA_Gfx_Func func2;       /**< Secondary graphics function, often used for pre-multiplication or color modification before primary blending. */

   int dst_clip_x;            /**< Destination clipping region X offset. */
   int dst_clip_y;            /**< Destination clipping region Y offset. */
   int dst_clip_h;            /**< Destination clipping region height. */
   int dst_clip_w;            /**< Destination clipping region width. */
   int dst_w;                 /**< Full width of the destination image. */

   int mask_x;                /**< X offset for the mask image, if used. */
   int mask_y;                /**< Y offset for the mask image, if used. */

   unsigned int mul_col;     /**< Multiplication color (ARGB). 0xFFFFFFFF means no color multiplication. */
};

static Eina_Bool use_thread = EINA_FALSE; /**< Flag indicating whether to use a separate thread for scaling. */
static Eina_Thread scaling_thread; /**< Handle for the scaling worker thread. */
static Eina_Thread_Queue *thread_queue = NULL; /**< Queue for sending tasks to the worker thread. */
static Eina_Thread_Queue *main_queue = NULL; /**< Queue for receiving completion signals from the worker thread. */

/**
 * @brief Scales a region of an RGBA image to a destination region with clipping, using sample scaling.
 * This function is a wrapper that calls evas_common_scale_rgba_in_to_out_clip_cb
 * with the scale_rgba_in_to_out_clip_sample_internal callback.
 *
 * @param src The source RGBA_Image.
 * @param dst The destination RGBA_Image.
 * @param dc The draw context.
 * @param src_region_x The x-coordinate of the top-left corner of the source region.
 * @param src_region_y The y-coordinate of the top-left corner of the source region.
 * @param src_region_w The width of the source region.
 * @param src_region_h The height of the source region.
 * @param dst_region_x The x-coordinate of the top-left corner of the destination region.
 * @param dst_region_y The y-coordinate of the top-left corner of the destination region.
 * @param dst_region_w The width of the destination region.
 * @param dst_region_h The height of the destination region.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EVAS_API Eina_Bool
evas_common_scale_rgba_in_to_out_clip_sample(RGBA_Image *src, RGBA_Image *dst,
                                             RGBA_Draw_Context *dc,
                                             int src_region_x, int src_region_y,
                                             int src_region_w, int src_region_h,
                                             int dst_region_x, int dst_region_y,
                                             int dst_region_w, int dst_region_h)
{
   return evas_common_scale_rgba_in_to_out_clip_cb
     (src, dst, dc,
      src_region_x, src_region_y, src_region_w, src_region_h,
      dst_region_x, dst_region_y, dst_region_w, dst_region_h,
      scale_rgba_in_to_out_clip_sample_internal);
}

/**
 * @brief Performs sample scaling of an RGBA image region to a destination region,
 * applying clipping and handling cutouts.
 * This function iterates over cutout rectangles if provided, applying the scaling
 * operation for each intersecting area.
 *
 * @param reuse Optional Cutout_Rects structure for reusing previously calculated cutouts.
 *              If NULL, a single scaling operation is performed based on the clip rectangle.
 * @param clip The overall clipping rectangle for the operation.
 * @param src The source RGBA_Image.
 * @param dst The destination RGBA_Image.
 * @param dc The draw context.
 * @param src_region_x The x-coordinate of the top-left corner of the source region.
 * @param src_region_y The y-coordinate of the top-left corner of the source region.
 * @param src_region_w The width of the source region.
 * @param src_region_h The height of the source region.
 * @param dst_region_x The x-coordinate of the top-left corner of the destination region.
 * @param dst_region_y The y-coordinate of the top-left corner of the destination region.
 * @param dst_region_w The width of the destination region.
 * @param dst_region_h The height of the destination region.
 */
EVAS_API void
evas_common_scale_rgba_in_to_out_clip_sample_do(const Cutout_Rects *reuse,
                                                const Eina_Rectangle *clip,
                                                RGBA_Image *src, RGBA_Image *dst,
                                                RGBA_Draw_Context *dc,
                                                int src_region_x, int src_region_y,
                                                int src_region_w, int src_region_h,
                                                int dst_region_x, int dst_region_y,
                                                int dst_region_w, int dst_region_h)
{
   Eina_Rectangle area;
   Cutout_Rect *r;
   int i;

   if (!reuse)
     {
        evas_common_draw_context_clip_clip(dc, clip->x, clip->y, clip->w, clip->h);
        scale_rgba_in_to_out_clip_sample_internal(src, dst, dc,
                                                  src_region_x, src_region_y,
                                                  src_region_w, src_region_h,
                                                  dst_region_x, dst_region_y,
                                                  dst_region_w, dst_region_h);
        return;
     }

   for (i = 0; i < reuse->active; ++i)
     {
        r = reuse->rects + i;

        EINA_RECTANGLE_SET(&area, r->x, r->y, r->w, r->h);
        if (!eina_rectangle_intersection(&area, clip)) continue ;
        evas_common_draw_context_set_clip(dc, area.x, area.y, area.w, area.h);
        scale_rgba_in_to_out_clip_sample_internal(src, dst, dc,
                                                  src_region_x, src_region_y,
                                                  src_region_w, src_region_h,
                                                  dst_region_x, dst_region_y,
                                                  dst_region_w, dst_region_h);
     }
}

/**
 * @internal
 * @brief Performs scaling for a set of scanlines without a mask.
 * This function handles the core pixel copying or blending from source to destination
 * for a portion of the image when no alpha mask is applied to the destination.
 *
 * @param y Starting y-coordinate in the destination clip region for processing.
 * @param dst_clip_w Width of the destination clipping region.
 * @param dst_clip_h Height of the destination clipping region (effectively the number of lines to process from start_y).
 * @param dst_w Full width of the destination image (for calculating line stride).
 * @param row_ptr Array of pointers to source image rows, used if `srcptr` is NULL (scaled mode).
 *                `row_ptr[y]` points to the pre-calculated source row for destination row `y`.
 * @param lin_ptr Array of source x-coordinates for each destination column, used if `srcptr` is NULL (scaled mode).
 *                `lin_ptr[x]` gives the source x-offset for destination column `x`.
 * @param dptr Pointer to the current line in the destination image buffer.
 * @param func The graphics function to apply (e.g., copy, blend).
 * @param mul_col Multiplication color. If 0xFFFFFFFF, no color multiplication is done.
 * @param srcptr Pointer to the current line in the source image buffer (direct 1:1 copy/blend mode).
 *               If NULL, `row_ptr` and `lin_ptr` are used for scaling.
 * @param src_w Width of the source image (for calculating line stride if `srcptr` is used).
 */
static void
_evas_common_scale_rgba_sample_scale_nomask(int y,
                                            int dst_clip_w, int dst_clip_h, int dst_w,
                                            DATA32 **row_ptr, int *lin_ptr,
                                            DATA32 *dptr, RGBA_Gfx_Func func, unsigned int mul_col,
                                            DATA32 *srcptr, int src_w)
{
   int x;
   dptr = dptr + dst_w * y;

   if (srcptr)
     {
        for (; y < dst_clip_h; y++)
          {
             /* * blend here [clip_w *] buf -> dptr * */
             func(srcptr, NULL, mul_col, dptr, dst_clip_w);
             dptr += dst_w;
             srcptr += src_w;
          }
     }
   else
     {
        DATA32 *buf = alloca(dst_clip_w * sizeof(DATA32));
        for (; y < dst_clip_h; y++)
          {
             DATA32 *dst_ptr = buf;
             for (x = 0; x < dst_clip_w; x++)
               {
                  DATA32 *ptr = row_ptr[y] + lin_ptr[x];
                  *dst_ptr = *ptr;
                  dst_ptr++;
               }
             /* * blend here [clip_w *] buf -> dptr * */
             func(buf, NULL, mul_col, dptr, dst_clip_w);
             dptr += dst_w;
          }
     }
}

/**
 * @internal
 * @brief Performs scaling for a set of scanlines with a mask.
 * This function handles pixel copying or blending from source to destination,
 * applying an alpha mask during the process.
 *
 * @param y Starting y-coordinate in the destination clip region for processing.
 * @param dst_clip_x X-coordinate of the destination clipping region.
 * @param dst_clip_y Y-coordinate of the destination clipping region.
 * @param dst_clip_w Width of the destination clipping region.
 * @param dst_clip_h Height of the destination clipping region (number of lines to process).
 * @param dst_w Full width of the destination image.
 * @param mask_x X-offset of the mask relative to the destination image.
 * @param mask_y Y-offset of the mask relative to the destination image.
 * @param row_ptr Array of pointers to source image rows (scaled mode).
 * @param lin_ptr Array of source x-coordinates for each destination column (scaled mode).
 * @param mask_ie The RGBA_Image structure representing the 8-bit alpha mask.
 * @param dptr Pointer to the current line in the destination image buffer.
 * @param func The primary graphics function for masked blending.
 * @param func2 Secondary graphics function, usually for color multiplication before masking.
 * @param mul_col Multiplication color.
 * @param srcptr Pointer to the current line in the source image buffer (direct 1:1 mode).
 *               If NULL, `row_ptr` and `lin_ptr` are used.
 * @param src_w Width of the source image (if `srcptr` is used).
 */
static void
_evas_common_scale_rgba_sample_scale_mask(int y,
                                          int dst_clip_x, int dst_clip_y,
                                          int dst_clip_w, int dst_clip_h, int dst_w,
                                          int mask_x, int mask_y,
                                          DATA32 **row_ptr, int *lin_ptr, RGBA_Image *mask_ie,
                                          DATA32 *dptr, RGBA_Gfx_Func func, RGBA_Gfx_Func func2,
                                          unsigned int mul_col,
                                          DATA32 *srcptr, int src_w)
{
   DATA32 *buf;
   int x;

   /* clamp/map to mask geometry */
   if (EINA_UNLIKELY(dst_clip_x < mask_x))
     dst_clip_x = mask_x;
   if (EINA_UNLIKELY(dst_clip_y < mask_y))
     dst_clip_y = mask_y;
   if (EINA_UNLIKELY(dst_clip_x + dst_clip_w > mask_x + (int)mask_ie->cache_entry.w))
     dst_clip_w = mask_x + mask_ie->cache_entry.w - dst_clip_x;
   if (EINA_UNLIKELY(dst_clip_y + dst_clip_h > mask_y + (int)mask_ie->cache_entry.h))
     dst_clip_h = mask_y + mask_ie->cache_entry.h - dst_clip_y;

   /* a scanline buffer */
   buf = alloca(dst_clip_w * sizeof(DATA32));

   dptr = dptr + dst_w * y;
   for (; y < dst_clip_h; y++)
     {
        DATA8 *mask;

        mask = mask_ie->image.data8
          + ((dst_clip_y - mask_y + y) * mask_ie->cache_entry.w)
          + (dst_clip_x - mask_x);

        if (!srcptr)
          {
             DATA32 *dst_ptr = buf;
             for (x = 0; x < dst_clip_w; x++)
               {
                  DATA32 *ptr;

                  ptr = row_ptr[y] + lin_ptr[x];
                  *dst_ptr = *ptr;
                  dst_ptr++;
               }
          }

        /* * blend here [clip_w *] buf -> dptr * */
        if (mul_col != 0xFFFFFFFF)
          {
             func2(srcptr ?: buf, NULL, mul_col, buf, dst_clip_w);
             func(buf, mask, 0, dptr, dst_clip_w);
          }
        else
          func(srcptr ?: buf, mask, 0, dptr, dst_clip_w);

        dptr += dst_w;
        if (srcptr) srcptr += src_w;
     }
}

/**
 * @brief Draws a scaled sample of an RGBA image to a destination image.
 * This is a comprehensive function that handles various parameters for scaling,
 * clipping, color multiplication, render operations, and masking. It prepares
 * data (like lookup tables for scaling if needed) and then calls helper
 * functions (_evas_common_scale_rgba_sample_scale_nomask or
 * _evas_common_scale_rgba_sample_scale_mask) to perform the actual pixel operations.
 *
 * @param src Source RGBA_Image.
 * @param dst Destination RGBA_Image.
 * @param dst_clip_x Destination clip X.
 * @param dst_clip_y Destination clip Y.
 * @param dst_clip_w Destination clip width.
 * @param dst_clip_h Destination clip height.
 * @param mul_col Multiplication color (ARGB). 0xFFFFFFFF for no multiplication.
 * @param render_op Render operation (e.g., EVAS_RENDER_COPY, EVAS_RENDER_BLEND).
 * @param src_region_x Source region X.
 * @param src_region_y Source region Y.
 * @param src_region_w Source region width.
 * @param src_region_h Source region height.
 * @param dst_region_x Destination region X.
 * @param dst_region_y Destination region Y.
 * @param dst_region_w Destination region width.
 * @param dst_region_h Destination region height.
 * @param mask_ie Optional mask image.
 * @param mask_x Mask X offset.
 * @param mask_y Mask Y offset.
 */
EVAS_API void
evas_common_scale_rgba_sample_draw(RGBA_Image *src, RGBA_Image *dst, int dst_clip_x, int dst_clip_y, int dst_clip_w, int dst_clip_h, DATA32 mul_col, int render_op, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h, RGBA_Image *mask_ie, int mask_x, int mask_y)
{
   int      x, y;
   int     *lin_ptr;
   DATA32 **row_ptr;
   DATA32  *ptr, *dst_ptr, *src_data, *dst_data;
   int      src_w, src_h, dst_w, dst_h;
   RGBA_Gfx_Func func, func2 = NULL;

   if ((!src->image.data) || (!dst->image.data)) return;
   if (!(RECTS_INTERSECT(dst_region_x, dst_region_y, dst_region_w, dst_region_h,
                         0, 0, dst->cache_entry.w, dst->cache_entry.h))) return;
   if (!(RECTS_INTERSECT(src_region_x, src_region_y, src_region_w, src_region_h,
                         0, 0, src->cache_entry.w, src->cache_entry.h))) return;

   if ((src_region_w <= 0) || (src_region_h <= 0) ||
       (dst_region_w <= 0) || (dst_region_h <= 0)) return;

   src_w = src->cache_entry.w;
   if (src_region_x >= src_w) return;

   src_h = src->cache_entry.h;
   if (src_region_y >= src_h) return;

   dst_w = dst->cache_entry.w;
   dst_h = dst->cache_entry.h;

   src_data = src->image.data;
   dst_data = dst->image.data;

   /* sanitise clip x */
   if (dst_clip_x < 0)
     {
        dst_clip_w += dst_clip_x;
        dst_clip_x = 0;
     }

   if ((dst_clip_x + dst_clip_w) > dst_w)
     dst_clip_w = dst_w - dst_clip_x;

   if (dst_clip_x < dst_region_x)
     {
        dst_clip_w += dst_clip_x - dst_region_x;
        dst_clip_x = dst_region_x;
     }

   if (dst_clip_x >= dst_w) return;

   if ((dst_clip_x + dst_clip_w) > (dst_region_x + dst_region_w))
     dst_clip_w = dst_region_x + dst_region_w - dst_clip_x;

   if (dst_clip_w <= 0) return;

   /* sanitise clip y */
   if (dst_clip_y < 0)
     {
        dst_clip_h += dst_clip_y;
        dst_clip_y = 0;
     }

   if ((dst_clip_y + dst_clip_h) > dst_h)
     dst_clip_h = dst_h - dst_clip_y;

   if (dst_clip_y < dst_region_y)
     {
        dst_clip_h += dst_clip_y - dst_region_y;
        dst_clip_y = dst_region_y;
     }

   if (dst_clip_y >= dst_h) return;

   if ((dst_clip_y + dst_clip_h) > (dst_region_y + dst_region_h))
     dst_clip_h = dst_region_y + dst_region_h - dst_clip_y;

   if (dst_clip_h <= 0) return;

   /* sanitise region x */
   if (src_region_x < 0)
     {
        dst_region_x -= (src_region_x * dst_region_w) / src_region_w;
        dst_region_w += (src_region_x * dst_region_w) / src_region_w;
        src_region_w += src_region_x;
        src_region_x = 0;

        if (dst_clip_x < dst_region_x)
          {
             dst_clip_w += (dst_clip_x - dst_region_x);
             dst_clip_x = dst_region_x;
          }
     }

   if ((dst_clip_x + dst_clip_w) > dst_w)
     dst_clip_w = dst_w - dst_clip_x;

   if (dst_clip_w <= 0) return;

   if ((src_region_x + src_region_w) > src_w)
     {
        dst_region_w = (dst_region_w * (src_w - src_region_x)) / (src_region_w);
        src_region_w = src_w - src_region_x;
     }

   if ((dst_region_w <= 0) || (src_region_w <= 0)) return;

   /* sanitise region y */
   if (src_region_y < 0)
     {
        dst_region_y -= (src_region_y * dst_region_h) / src_region_h;
        dst_region_h += (src_region_y * dst_region_h) / src_region_h;
        src_region_h += src_region_y;
        src_region_y = 0;

        if (dst_clip_y < dst_region_y)
          {
             dst_clip_h += (dst_clip_y - dst_region_y);
             dst_clip_y = dst_region_y;
          }
     }

   if ((dst_clip_y + dst_clip_h) > dst_h)
     dst_clip_h = dst_h - dst_clip_y;

   if (dst_clip_h <= 0) return;

   if ((src_region_y + src_region_h) > src_h)
     {
        dst_region_h = (dst_region_h * (src_h - src_region_y)) / (src_region_h);
        src_region_h = src_h - src_region_y;
     }

   if ((dst_region_h <= 0) || (src_region_h <= 0)) return;

   /* figure out dst jump */
   //dst_jump = dst_w - dst_clip_w;

   /* figure out dest start ptr */
   dst_ptr = dst_data + dst_clip_x + (dst_clip_y * dst_w);

   if (!mask_ie)
     {
         if (mul_col != 0xffffffff)
           func = evas_common_gfx_func_composite_pixel_color_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, mul_col, dst->cache_entry.flags.alpha, dst_clip_w, render_op);
         else
           func = evas_common_gfx_func_composite_pixel_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dst->cache_entry.flags.alpha, dst_clip_w, render_op);
     }
   else
     {
        if (mul_col != 0xffffffff)
          {
             func = evas_common_gfx_func_composite_pixel_mask_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dst->cache_entry.flags.alpha, dst_clip_w, render_op);
             func2 = evas_common_gfx_func_composite_pixel_color_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, mul_col, dst->cache_entry.flags.alpha, dst_clip_w, EVAS_RENDER_COPY);
          }
        else
          func = evas_common_gfx_func_composite_pixel_mask_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dst->cache_entry.flags.alpha, dst_clip_w, render_op);
     }

   if ((dst_region_w == src_region_w) && (dst_region_h == src_region_h))
     {
        ptr = src_data + (((dst_clip_y - dst_region_y) + src_region_y) * src_w) + ((dst_clip_x - dst_region_x) + src_region_x);


        if (mask_ie)
          _evas_common_scale_rgba_sample_scale_mask(0,
            dst_clip_x, dst_clip_y, dst_clip_w, dst_clip_h,
            dst_w, mask_x, mask_y,
            NULL, NULL,
            mask_ie, dst_ptr,
            func, func2, mul_col,
            ptr, src_w);
        else
          _evas_common_scale_rgba_sample_scale_nomask(0,
            dst_clip_w, dst_clip_h, dst_w,
            NULL, NULL,
            dst_ptr,
            func, mul_col,
            ptr, src_w);
     }
   else
     {
        /* allocate scale lookup tables */
        lin_ptr = alloca(dst_clip_w * sizeof(int));
        row_ptr = alloca(dst_clip_h * sizeof(DATA32 *));

        /* fill scale tables */
        for (x = 0; x < dst_clip_w; x++)
          lin_ptr[x] = (((x + dst_clip_x - dst_region_x) * src_region_w) / dst_region_w) + src_region_x;
        for (y = 0; y < dst_clip_h; y++)
          row_ptr[y] = src_data + (((((y + dst_clip_y - dst_region_y) * src_region_h) / dst_region_h)
                                    + src_region_y) * src_w);

        if (mask_ie)
          _evas_common_scale_rgba_sample_scale_mask(0,
            dst_clip_x, dst_clip_y, dst_clip_w, dst_clip_h,
            dst_w, mask_x, mask_y,
            row_ptr, lin_ptr,
            mask_ie, dst_ptr,
            func, func2, mul_col,
            NULL, 0);
        else
          _evas_common_scale_rgba_sample_scale_nomask(0,
            dst_clip_w, dst_clip_h, dst_w,
            row_ptr, lin_ptr,
            dst_ptr,
            func, mul_col,
            NULL, 0);
     }
}

/**
 * @internal scale_rgba_in_to_out_clip_sample_internal
 * @brief Core implementation for scaling a region of an RGBA image to a destination region with clipping.
 *
 * This function handles the detailed logic of scaling, including:
 * - Sanity checks for region and image dimensions.
 * - Clipping adjustments based on draw context and destination regions.
 * - Source and destination region sanitization to prevent out-of-bounds access.
 * - Selection of appropriate graphics functions based on alpha, color multiplication, and render operation.
 * - Handling of 1:1 scaling (direct copy/blend) versus arbitrary scaling (using lookup tables).
 * - Optional use of Pixman for optimization if available and applicable.
 * - Optional threading for large scaling operations to improve performance.
 *
 * If scaling is not 1:1, it computes `lin_ptr` (mapping destination X to source X)
 * and `row_ptr` (mapping destination Y to source Y start pointer).
 *
 * It then dispatches to either `_evas_common_scale_rgba_sample_scale_mask` or
 * `_evas_common_scale_rgba_sample_scale_nomask` for the actual pixel processing,
 * potentially splitting the work with a worker thread if `use_thread` is true
 * and the image dimensions are large enough.
 *
 * @param src Source image.
 * @param dst Destination image.
 * @param dc Draw context, containing clipping information, multiplication color, and render operation.
 * @param src_region_x Source region X offset.
 * @param src_region_y Source region Y offset.
 * @param src_region_w Source region width.
 * @param src_region_h Source region height.
 * @param dst_region_x Destination region X offset.
 * @param dst_region_y Destination region Y offset.
 * @param dst_region_w Destination region width.
 * @param dst_region_h Destination region height.
 * @return EINA_TRUE on success, EINA_FALSE on error or if nothing to draw.
 */
static Eina_Bool
scale_rgba_in_to_out_clip_sample_internal(RGBA_Image *src, RGBA_Image *dst,
                                         RGBA_Draw_Context *dc,
                                         int src_region_x, int src_region_y,
                                         int src_region_w, int src_region_h,
                                         int dst_region_x, int dst_region_y,
                                         int dst_region_w, int dst_region_h)
{
   int      x, y;
   int     *lin_ptr;
   DATA32 *dptr;
   DATA32 **row_ptr;
   DATA32  *ptr, *dst_ptr, *src_data, *dst_data;
   int      dst_clip_x, dst_clip_y, dst_clip_w, dst_clip_h;
   int      src_w, src_h, dst_w, dst_h, mask_x, mask_y;
   RGBA_Gfx_Func func, func2 = NULL;
   RGBA_Image *mask_ie = dc->clip.mask;

   if (!(RECTS_INTERSECT(dst_region_x, dst_region_y, dst_region_w, dst_region_h, 0, 0, dst->cache_entry.w, dst->cache_entry.h)))
     return EINA_FALSE;
   if (!(RECTS_INTERSECT(src_region_x, src_region_y, src_region_w, src_region_h, 0, 0, src->cache_entry.w, src->cache_entry.h)))
     return EINA_FALSE;

   src_w = src->cache_entry.w;
   src_h = src->cache_entry.h;
   dst_w = dst->cache_entry.w;
   dst_h = dst->cache_entry.h;

   src_data = src->image.data;
   dst_data = dst->image.data;

   mask_x = dc->clip.mask_x;
   mask_y = dc->clip.mask_y;

   if (dc->clip.use)
     {
        dst_clip_x = dc->clip.x;
        dst_clip_y = dc->clip.y;
        dst_clip_w = dc->clip.w;
        dst_clip_h = dc->clip.h;
        if (dst_clip_x < 0)
          {
             dst_clip_w += dst_clip_x;
             dst_clip_x = 0;
          }
        if (dst_clip_y < 0)
          {
             dst_clip_h += dst_clip_y;
             dst_clip_y = 0;
          }
        if ((dst_clip_x + dst_clip_w) > dst_w)
          dst_clip_w = dst_w - dst_clip_x;
        if ((dst_clip_y + dst_clip_h) > dst_h)
          dst_clip_h = dst_h - dst_clip_y;
     }
   else
     {
        dst_clip_x = 0;
        dst_clip_y = 0;
        dst_clip_w = dst_w;
        dst_clip_h = dst_h;
     }

   if (dst_clip_x < dst_region_x)
     {
        dst_clip_w += dst_clip_x - dst_region_x;
        dst_clip_x = dst_region_x;
     }
   if ((dst_clip_x + dst_clip_w) > (dst_region_x + dst_region_w))
     dst_clip_w = dst_region_x + dst_region_w - dst_clip_x;
   if (dst_clip_y < dst_region_y)
     {
        dst_clip_h += dst_clip_y - dst_region_y;
        dst_clip_y = dst_region_y;
     }
   if ((dst_clip_y + dst_clip_h) > (dst_region_y + dst_region_h))
     dst_clip_h = dst_region_y + dst_region_h - dst_clip_y;

   if ((src_region_w <= 0) || (src_region_h <= 0) ||
       (dst_region_w <= 0) || (dst_region_h <= 0) ||
       (dst_clip_w <= 0) || (dst_clip_h <= 0))
     return EINA_FALSE;

   /* sanitise x */
   if (src_region_x < 0)
     {
        dst_region_x -= (src_region_x * dst_region_w) / src_region_w;
        dst_region_w += (src_region_x * dst_region_w) / src_region_w;
        src_region_w += src_region_x;
        src_region_x = 0;
     }
   if (src_region_x >= src_w) return EINA_FALSE;
   if ((src_region_x + src_region_w) > src_w)
     {
        dst_region_w = (dst_region_w * (src_w - src_region_x)) / (src_region_w);
        src_region_w = src_w - src_region_x;
     }
   if (dst_region_w <= 0) return EINA_FALSE;
   if (src_region_w <= 0) return EINA_FALSE;
   if (dst_clip_x >= dst_w) return EINA_FALSE;
   if (dst_clip_x < dst_region_x)
     {
        dst_clip_w += (dst_clip_x - dst_region_x);
        dst_clip_x = dst_region_x;
     }
   if ((dst_clip_x + dst_clip_w) > dst_w)
     {
        dst_clip_w = dst_w - dst_clip_x;
     }
   if (dst_clip_w <= 0) return EINA_FALSE;

   /* sanitise y */
   if (src_region_y < 0)
     {
        dst_region_y -= (src_region_y * dst_region_h) / src_region_h;
        dst_region_h += (src_region_y * dst_region_h) / src_region_h;
        src_region_h += src_region_y;
        src_region_y = 0;
     }
   if (src_region_y >= src_h) return EINA_FALSE;
   if ((src_region_y + src_region_h) > src_h)
     {
        dst_region_h = (dst_region_h * (src_h - src_region_y)) / (src_region_h);
        src_region_h = src_h - src_region_y;
     }
   if (dst_region_h <= 0) return EINA_FALSE;
   if (src_region_h <= 0) return EINA_FALSE;
   if (dst_clip_y >= dst_h) return EINA_FALSE;
   if (dst_clip_y < dst_region_y)
     {
        dst_clip_h += (dst_clip_y - dst_region_y);
        dst_clip_y = dst_region_y;
     }
   if ((dst_clip_y + dst_clip_h) > dst_h)
     {
        dst_clip_h = dst_h - dst_clip_y;
     }
   if (dst_clip_h <= 0) return EINA_FALSE;

   /* figure out dst jump */
   //dst_jump = dst_w - dst_clip_w;

   /* figure out dest start ptr */
   dst_ptr = dst_data + dst_clip_x + (dst_clip_y * dst_w);

   if (!mask_ie)
     {
        if (dc->mul.use)
          func = evas_common_gfx_func_composite_pixel_color_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dc->mul.col, dst->cache_entry.flags.alpha, dst_clip_w, dc->render_op);
        else
          func = evas_common_gfx_func_composite_pixel_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dst->cache_entry.flags.alpha, dst_clip_w, dc->render_op);
     }
   else
     {
        func = evas_common_gfx_func_composite_pixel_mask_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dst->cache_entry.flags.alpha, dst_clip_w, dc->render_op);
        if (dc->mul.use)
          func2 = evas_common_gfx_func_composite_pixel_color_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dc->mul.col, dst->cache_entry.flags.alpha, dst_clip_w, EVAS_RENDER_COPY);
     }

   if ((dst_region_w == src_region_w) && (dst_region_h == src_region_h))
     {
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_IMAGE_SCALE_SAMPLE
        if ((src->pixman.im) && (dst->pixman.im) && (!dc->clip.mask) &&
            ((!dc->mul.use) || ((dc->mul.use) && (dc->mul.col == 0xffffffff))) &&
            ((dc->render_op == _EVAS_RENDER_COPY) ||
             (dc->render_op == _EVAS_RENDER_BLEND)))
          {
             pixman_op_t op = PIXMAN_OP_SRC; // _EVAS_RENDER_COPY
             if (dc->render_op == _EVAS_RENDER_BLEND)
               op = PIXMAN_OP_OVER;

             pixman_image_composite(op,
                                    src->pixman.im, NULL,
                                    dst->pixman.im,
                                    (dst_clip_x - dst_region_x) + src_region_x,
                                    (dst_clip_y - dst_region_y) + src_region_y,
                                    0, 0,
                                    dst_clip_x, dst_clip_y,
                                    dst_clip_w, dst_clip_h);
          }
        else
# endif
#endif
          {
             int mul_col = dc->mul.use ? dc->mul.col : 0xffffffff;
             ptr = src_data + ((dst_clip_y - dst_region_y + src_region_y) * src_w) + (dst_clip_x - dst_region_x) + src_region_x;

             if (mask_ie)
               _evas_common_scale_rgba_sample_scale_mask(0,
                 dst_clip_x, dst_clip_y, dst_clip_w, dst_clip_h,
                 dst_w, mask_x, mask_y,
                 NULL, NULL,
                 mask_ie, dst_ptr,
                 func, func2, mul_col,
                 ptr, src_w);
             else
               _evas_common_scale_rgba_sample_scale_nomask(0,
                 dst_clip_w, dst_clip_h, dst_w,
                 NULL, NULL,
                 dst_ptr,
                 func, mul_col,
                 ptr, src_w);
          }
     }
   else
     {
        /* allocate scale lookup tables */
        lin_ptr = alloca(dst_clip_w * sizeof(int));
        row_ptr = alloca(dst_clip_h * sizeof(DATA32 *));

        /* fill scale tables */
        for (x = 0; x < dst_clip_w; x++)
          lin_ptr[x] = (((x + dst_clip_x - dst_region_x) * src_region_w) / dst_region_w) + src_region_x;
        for (y = 0; y < dst_clip_h; y++)
          row_ptr[y] = src_data + (((((y + dst_clip_y - dst_region_y) * src_region_h) / dst_region_h)
                                    + src_region_y) * src_w);
        /* scale to dst */
        dptr = dst_ptr;
#ifdef DIRECT_SCALE
        if ((!src->cache_entry.flags.alpha) &&
            (!dst->cache_entry.flags.alpha) &&
            (!dc->mul.use) &&
            (!dc->clip.mask))
          {
             for (y = 0; y < dst_clip_h; y++)
               {

                  dst_ptr = dptr;
                  for (x = 0; x < dst_clip_w; x++)
                    {
                       ptr = row_ptr[y] + lin_ptr[x];
                       *dst_ptr = *ptr;
                       dst_ptr++;
                    }

                  dptr += dst_w;
               }
          }
        else
#endif
          {
             unsigned int mul_col;

             mul_col = dc->mul.use ? dc->mul.col : 0xFFFFFFFF;

             /* do we have enough data to start some additional thread ? */
             if (use_thread && dst_clip_h > 32 && dst_clip_w * dst_clip_h > 4096)
               {
                  /* Yes, we do ! */
                  Evas_Scale_Msg *msg;
                  void *ref;
                  Evas_Scale_Thread local;

                  local.mask8 = dc->clip.mask;
                  local.row_ptr = row_ptr;
                  local.dptr = dptr;
                  local.lin_ptr = lin_ptr;
                  local.func = func;
                  local.func2 = func2;
                  local.dst_clip_x = dst_clip_x;
                  local.dst_clip_y = dst_clip_y;
                  local.dst_clip_h = dst_clip_h;
                  local.dst_clip_w = dst_clip_w;
                  local.dst_w = dst_w;
                  local.mask_x = mask_x;
                  local.mask_y = mask_y;
                  local.mul_col = mul_col;

                  msg = eina_thread_queue_send(thread_queue, sizeof (Evas_Scale_Msg), &ref);
                  msg->task = &local;
                  eina_thread_queue_send_done(thread_queue, ref);

                  /* image masking */
                  if (dc->clip.mask)
                    {
                       _evas_common_scale_rgba_sample_scale_mask(0,
                                                                 dst_clip_x, dst_clip_y,
                                                                 dst_clip_w, dst_clip_h >> 1, dst_w,
                                                                 dc->clip.mask_x, dc->clip.mask_y,
                                                                 row_ptr, lin_ptr, dc->clip.mask,
                                                                 dptr, func, func2, mul_col,
                                                                 NULL, 0);

                    }
                  else
                    {
                       _evas_common_scale_rgba_sample_scale_nomask(0,
                                                                   dst_clip_w, dst_clip_h >> 1, dst_w,
                                                                   row_ptr, lin_ptr,
                                                                   dptr, func, mul_col,
                                                                   NULL, 0);
                    }

                  msg = eina_thread_queue_wait(main_queue, &ref);
                  if (msg) eina_thread_queue_wait_done(main_queue, ref);
               }
             else
               {
                  /* No we don't ! */

                  /* image masking */
                  if (dc->clip.mask)
                    {
                       _evas_common_scale_rgba_sample_scale_mask(0,
                                                                 dst_clip_x, dst_clip_y,
                                                                 dst_clip_w, dst_clip_h, dst_w,
                                                                 dc->clip.mask_x, dc->clip.mask_y,
                                                                 row_ptr, lin_ptr, dc->clip.mask,
                                                                 dptr, func, func2, mul_col,
                                                                 NULL, 0);

                    }
                  else
                    {
                       _evas_common_scale_rgba_sample_scale_nomask(0,
                                                                   dst_clip_w, dst_clip_h, dst_w,
                                                                   row_ptr, lin_ptr,
                                                                   dptr, func, mul_col,
                                                                   NULL, 0);
                    }
               }
          }
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Worker thread function for parallel scaling.
 * This thread waits for messages on `thread_queue`. Each message contains
 * an `Evas_Scale_Thread` task. The thread processes the task, typically
 * handling the second half of the image data if threading is used by
 * `scale_rgba_in_to_out_clip_sample_internal`.
 * If a message with a NULL task is received, the thread terminates.
 *
 * @param data Unused thread data.
 * @param t Eina_Thread handle for this thread.
 * @return NULL always.
 */
static void *
_evas_common_scale_sample_thread(void *data EINA_UNUSED,
                                 Eina_Thread t EINA_UNUSED)
{
   Evas_Scale_Msg *msg;
   Evas_Scale_Thread *todo = NULL;

   eina_thread_name_set(eina_thread_self(), "Evas-scale-sam");
   do
     {
        void *ref;

        todo = NULL;

        msg = eina_thread_queue_wait(thread_queue, &ref);
        if (msg)
          {
             int h;

             todo = msg->task;
             eina_thread_queue_wait_done(thread_queue, ref);

             if (!todo) goto end;

             h = todo->dst_clip_h >> 1;

             if (todo->mask8)
               _evas_common_scale_rgba_sample_scale_mask(h,
                                                         todo->dst_clip_x, todo->dst_clip_y,
                                                         todo->dst_clip_w, todo->dst_clip_h,
                                                         todo->dst_w,
                                                         todo->mask_x, todo->mask_y,
                                                         todo->row_ptr, todo->lin_ptr, todo->mask8,
                                                         todo->dptr, todo->func, todo->func2,
                                                         todo->mul_col,
                                                         NULL, 0);
             else
               _evas_common_scale_rgba_sample_scale_nomask(h,
                                                           todo->dst_clip_w, todo->dst_clip_h,
                                                           todo->dst_w,
                                                           todo->row_ptr, todo->lin_ptr,
                                                           todo->dptr, todo->func, todo->mul_col,
                                                           NULL, 0);
          }

     end:
        msg = eina_thread_queue_send(main_queue, sizeof (Evas_Scale_Msg), &ref);
        msg->task = NULL;
        eina_thread_queue_send_done(main_queue, ref);
     }
   while (todo);

   return NULL;
}

/**
 * @internal
 * @brief Callback function to re-initialize threading resources after a fork.
 * This function is registered with Ecore's fork handling. If the process forks,
 * this function is called in the child process to clean up old thread queues
 * and recreate them, along with restarting the scaling worker thread. This is
 * crucial because thread states and inter-process communication mechanisms
 * like queues are not typically safe across forks.
 *
 * @param data Unused data pointer, part of Ecore's callback signature.
 */
static void
evas_common_scale_sample_fork_reset(void *data EINA_UNUSED)
{
   eina_thread_queue_free(thread_queue);
   eina_thread_queue_free(main_queue);

   thread_queue = eina_thread_queue_new();
   main_queue = eina_thread_queue_new();

   if (!eina_thread_create(&scaling_thread, EINA_THREAD_NORMAL, -1,
                           _evas_common_scale_sample_thread, NULL))
     {
        CRI("We failed to recreate the upscaling thread.");
        use_thread = EINA_FALSE;
     }
}

/**
 * @brief Initializes the threaded sample scaling subsystem.
 * This function checks if the system has more than 2 CPU cores. If so,
 * it sets up thread queues (`thread_queue` for tasks, `main_queue` for
 * completion signals) and creates a worker thread (`scaling_thread`)
 * that executes `_evas_common_scale_sample_thread`.
 * It also registers `evas_common_scale_sample_fork_reset` to handle
 * re-initialization after a fork.
 * If initialization is successful, `use_thread` is set to EINA_TRUE.
 * Threading is disabled on Windows due to Eina_Thread_Queue limitations.
 */
EVAS_API void
evas_common_scale_sample_init(void)
{
   if (eina_cpu_count() <= 2) return ;

//Eina_Thread_Queue doesn't work on WIN32.
#ifdef _WIN32
   return;
#endif

   ecore_fork_reset_callback_add(evas_common_scale_sample_fork_reset, NULL);

   thread_queue = eina_thread_queue_new();
   if (EINA_UNLIKELY(!thread_queue))
     {
        ERR("Failed to create thread queue");
        goto cleanup;
     }
   main_queue = eina_thread_queue_new();
   if (EINA_UNLIKELY(!thread_queue))
     {
        ERR("Failed to create thread queue");
        goto cleanup;
     }

   if (!eina_thread_create(&scaling_thread, EINA_THREAD_NORMAL, -1,
                           _evas_common_scale_sample_thread, NULL))
     {
        CRI("We failed to create the upscaling thread.");
        goto cleanup;
     }

   use_thread = EINA_TRUE;
   return;

cleanup:
   if (thread_queue) eina_thread_queue_free(thread_queue);
   if (main_queue) eina_thread_queue_free(main_queue);
}

/**
 * @brief Shuts down the threaded sample scaling subsystem.
 * If threading was initialized (`use_thread` is EINA_TRUE):
 * - Unregisters the fork reset callback.
 * - Sends a NULL task message to the worker thread to signal termination.
 * - Waits for the worker thread to acknowledge termination via the main_queue.
 * - Joins the worker thread to ensure it has fully exited.
 * - Frees the thread queues.
 */
EVAS_API void
evas_common_scale_sample_shutdown(void)
{
   Evas_Scale_Msg *msg;
   void *ref;

   if (!use_thread) return ;

   ecore_fork_reset_callback_del(evas_common_scale_sample_fork_reset, NULL);

   msg = eina_thread_queue_send(thread_queue, sizeof (Evas_Scale_Msg), &ref);
   msg->task = NULL;
   eina_thread_queue_send_done(thread_queue, ref);

   /* Here is the thread commiting succide*/

   msg = eina_thread_queue_wait(main_queue, &ref);
   if (msg) eina_thread_queue_wait_done(main_queue, ref);

   eina_thread_join(scaling_thread);

   eina_thread_queue_free(thread_queue);
   eina_thread_queue_free(main_queue);
}
