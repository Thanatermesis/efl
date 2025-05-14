#include "evas_common_private.h"
#include "evas_convert_main.h"
#include "evas_private.h"

/**
 * @brief Creates a new Cutout_Rects structure.
 *
 * This function allocates and initializes a new Cutout_Rects structure,
 * which is used to manage a list of cutout rectangles.
 *
 * @return A pointer to the newly created Cutout_Rects structure, or NULL on failure.
 * @see evas_common_draw_context_cutouts_free()
 * @see evas_common_draw_context_cutouts_real_free()
 */
EVAS_API Cutout_Rects *
evas_common_draw_context_cutouts_new(void)
{
   Cutout_Rects *rects;

   rects = calloc(1, sizeof(Cutout_Rects));
   return rects;
}

/**
 * @brief Duplicates a Cutout_Rects structure.
 *
 * This function creates a deep copy of the source Cutout_Rects structure
 * into the destination Cutout_Rects structure. If the source is NULL,
 * the function does nothing.
 *
 * @param rects2 The destination Cutout_Rects structure.
 * @param rects The source Cutout_Rects structure to duplicate.
 */
static void
evas_common_draw_context_cutouts_dup(Cutout_Rects *rects2, const Cutout_Rects *rects)
{
   if (!rects) return;
   rects2->active = rects->active;
   rects2->max = rects->active;
   rects2->last_add = rects->last_add;
   if (rects2->max > 0)
     {
        const size_t sz = sizeof(Cutout_Rect) * rects2->max;
        rects2->rects = malloc(sz);
        memcpy(rects2->rects, rects->rects, sz);
        return;
     }
   else rects2->rects = NULL;
}

/**
 * @brief Resets a Cutout_Rects structure for reuse.
 *
 * This function resets the active count and last added rectangle information
 * in a Cutout_Rects structure, making it appear empty. It does not free
 * the memory allocated for the rectangles themselves. This is intended for
 * reusing the structure without deallocating and reallocating memory.
 *
 * @param rects The Cutout_Rects structure to reset.
 * @see evas_common_draw_context_cutouts_real_free()
 * @see evas_common_draw_context_cutouts_new()
 */
EVAS_API void
evas_common_draw_context_cutouts_free(Cutout_Rects* rects)
{
   if (!rects) return;
   rects->active = 0;
   rects->last_add.w = 0;
}

/**
 * @brief Frees all memory associated with a Cutout_Rects structure.
 *
 * This function frees the memory allocated for the array of rectangles
 * and the Cutout_Rects structure itself.
 *
 * @param rects The Cutout_Rects structure to free.
 * @see evas_common_draw_context_cutouts_new()
 * @see evas_common_draw_context_cutouts_free()
 */
EVAS_API void
evas_common_draw_context_cutouts_real_free(Cutout_Rects* rects)
{
   if (!rects) return;
   free(rects->rects);
   free(rects);
}

/**
 * @brief Deletes a cutout rectangle at a specific index.
 *
 * This function removes the Cutout_Rect at the given index from the
 * Cutout_Rects list. Subsequent rectangles are shifted to fill the gap.
 *
 * @param rects The Cutout_Rects structure.
 * @param idx The index of the cutout rectangle to delete.
 */
EVAS_API void
evas_common_draw_context_cutouts_del(Cutout_Rects* rects, int idx)
{
   if ((idx >= 0) && (idx < rects->active))
     {
        Cutout_Rect *rect;

        rect = rects->rects + idx;
        memmove(rect, rect + 1,
                sizeof(Cutout_Rect) * (rects->active - idx - 1));
        rects->active--;
        rects->last_add.w = 0;
     }
}

static int _init_count = 0;
static Eina_Trash *_ctxt_spares = NULL;
static int _ctxt_spares_count = 0;
static SLK(_ctx_spares_lock);

/**
 * @brief Frees the resources associated with an RGBA_Draw_Context.
 *
 * This function releases all resources held by the draw context,
 * including Pixman images (if compiled with Pixman support),
 * cutout data, and the context structure itself.
 *
 * @param dc The RGBA_Draw_Context to free.
 */
static void
_evas_common_draw_context_real_free(RGBA_Draw_Context *dc)
{
#ifdef HAVE_PIXMAN
# if defined(PIXMAN_FONT) || defined(PIXMAN_RECT) || defined(PIXMAN_LINE) || defined(PIXMAN_POLY)
   if (dc->col.pixman_color_image)
     pixman_image_unref(dc->col.pixman_color_image);
# endif
#endif
   evas_common_draw_context_apply_clean_cutouts(&dc->cutout);
   evas_common_draw_context_cutouts_real_free(dc->cache.rects);
   free(dc);
}

/**
 * @brief Stashes an RGBA_Draw_Context for later reuse.
 *
 * If the number of stashed contexts is below a threshold (8), this function
 * cleans up some resources within the context (like Pixman images and cutouts)
 * and adds it to a spare list. Otherwise, it fully frees the context.
 * This helps in reducing frequent allocations/deallocations.
 *
 * @param dc The RGBA_Draw_Context to stash.
 */
static void
_evas_common_draw_context_stash(RGBA_Draw_Context *dc)
{
   if (_ctxt_spares_count >= 8)
     {
        _evas_common_draw_context_real_free(dc);
        return ;
     }

#ifdef HAVE_PIXMAN
# if defined(PIXMAN_FONT) || defined(PIXMAN_RECT) || defined(PIXMAN_LINE) || defined(PIXMAN_POLY)
   if (dc->col.pixman_color_image)
     {
        pixman_image_unref(dc->col.pixman_color_image);
        dc->col.pixman_color_image = NULL;
     }
# endif
#endif
   evas_common_draw_context_apply_clean_cutouts(&dc->cutout);
   evas_common_draw_context_cutouts_real_free(dc->cache.rects);
   SLKL(_ctx_spares_lock);
   eina_trash_push(&_ctxt_spares, dc);
   _ctxt_spares_count++;
   SLKU(_ctx_spares_lock);
}

/**
 * @brief Retrieves or allocates an RGBA_Draw_Context.
 *
 * This function first attempts to retrieve a stashed (reusable)
 * RGBA_Draw_Context from a spare list. If no spare context is available,
 * it allocates a new one.
 *
 * @return A pointer to an RGBA_Draw_Context, or NULL if allocation fails.
 */
static RGBA_Draw_Context *
_evas_common_draw_context_find(void)
{
   RGBA_Draw_Context *dc = NULL;

   if (_ctxt_spares)
     {
        SLKL(_ctx_spares_lock);
        dc = eina_trash_pop(&_ctxt_spares);
        _ctxt_spares_count--;
        SLKU(_ctx_spares_lock);
     }

   if (!dc) dc = malloc(sizeof(RGBA_Draw_Context));

   return dc;
}

/**
 * @brief Initializes common Evas subsystems.
 *
 * This function initializes various common components used by Evas,
 * such as CPU feature detection, blending, image handling, conversion,
 * scaling, drawing primitives (rectangle, polygon, line), font handling,
 * and tile buffers. It uses a reference counter (`_init_count`) to ensure
 * initialization occurs only once.
 *
 * @see evas_common_shutdown()
 */
EVAS_API void
evas_common_init(void)
{
   if (_init_count++) return;

   SLKI(_ctx_spares_lock);
   evas_common_cpu_init();

   evas_common_blend_init();
   evas_common_image_init();
   evas_common_convert_init();
   evas_common_scale_init();
   evas_common_scale_sample_init();
   evas_common_rectangle_init();
   evas_common_polygon_init();
   evas_common_line_init();
   evas_common_font_init();
   evas_common_draw_init();
   evas_common_tilebuf_init();
}

/**
 * @brief Shuts down common Evas subsystems.
 *
 * This function cleans up resources used by the common Evas components
 * initialized by evas_common_init(). It uses a reference counter
 * (`_init_count`) to ensure shutdown occurs only when all users have
 * finished.
 *
 * @note The freeing of stashed RGBA_Draw_Contexts is commented out,
 *       possibly to avoid issues with contexts still in use by other threads.
 * @see evas_common_init()
 */
EVAS_API void
evas_common_shutdown(void)
{
   if (--_init_count) return;

   evas_font_dir_cache_free();
   evas_common_font_shutdown();
   evas_common_image_shutdown();
   evas_common_image_cache_free();
   evas_common_scale_sample_shutdown();
// just in case any thread is still doing things... don't del this here
//   RGBA_Draw_Context *dc;
//   SLKL(_ctx_spares_lock);
//   EINA_LIST_FREE(_ctxt_spares, dc) _evas_common_draw_context_real_free(dc);
//   _ctxt_spares_count = 0;
//   SLKU(_ctx_spares_lock);
//   SLKD(_ctx_spares_lock);
}

/**
 * @brief Initializes the common drawing subsystem.
 *
 * Currently, this function is a no-op.
 */
EVAS_API void
evas_common_draw_init(void)
{
}

/**
 * @brief Creates a new RGBA_Draw_Context.
 *
 * This function obtains an RGBA_Draw_Context (either from a stash or by
 * new allocation), initializes it to a default state (e.g., sets cutout
 * count_max and size_min), and returns it.
 *
 * @return A pointer to the newly created RGBA_Draw_Context, or NULL on failure.
 * @see evas_common_draw_context_free()
 * @see evas_common_draw_context_dup()
 */
EVAS_API RGBA_Draw_Context *
evas_common_draw_context_new(void)
{
   RGBA_Draw_Context *dc;
   dc = _evas_common_draw_context_find();
   if (!dc) return NULL;
   memset(dc, 0, sizeof(RGBA_Draw_Context));
   dc->cutout.count_max = 0x7fffffff;
   dc->cutout.size_min = 8 * 8;
   return dc;
}

/**
 * @brief Duplicates an RGBA_Draw_Context.
 *
 * This function creates a new RGBA_Draw_Context and copies the state from
 * the source context `dc` into it. This includes a deep copy of the cutout
 * information. Pixman images and cache rectangles are not directly copied
 * but are reset in the new context.
 *
 * @param dc The source RGBA_Draw_Context to duplicate. If NULL, a new,
 *           default-initialized context is returned.
 * @return A pointer to the duplicated RGBA_Draw_Context, or NULL on failure.
 * @see evas_common_draw_context_new()
 */
EVAS_API RGBA_Draw_Context *
evas_common_draw_context_dup(RGBA_Draw_Context *dc)
{
   RGBA_Draw_Context *dc2 = _evas_common_draw_context_find();

   if (!dc) return dc2;
   memcpy(dc2, dc, sizeof(RGBA_Draw_Context));
   evas_common_draw_context_cutouts_dup(&dc2->cutout, &dc->cutout);
#ifdef HAVE_PIXMAN
# if defined(PIXMAN_FONT) || defined(PIXMAN_RECT) || defined(PIXMAN_LINE) || defined(PIXMAN_POLY)
   dc2->col.pixman_color_image = NULL;
# endif
#endif
   dc2->cache.rects = NULL;
   dc2->cache.used = 0;
   return dc2;
}

/**
 * @brief Frees or stashes an RGBA_Draw_Context.
 *
 * This function doesn't necessarily free the context immediately. Instead,
 * it passes the context to `_evas_common_draw_context_stash`, which may
 * keep it for reuse if the stash is not full.
 *
 * @param dc The RGBA_Draw_Context to free or stash.
 * @see evas_common_draw_context_new()
 * @see _evas_common_draw_context_stash()
 */
EVAS_API void
evas_common_draw_context_free(RGBA_Draw_Context *dc)
{
   if (!dc) return;
   _evas_common_draw_context_stash(dc);
}

/**
 * @brief Clears the cutouts from a draw context.
 *
 * This function resets the cutout information within the draw context,
 * effectively removing all defined cutouts. It calls
 * `evas_common_draw_context_cutouts_free` on the context's cutout data.
 *
 * @param dc The RGBA_Draw_Context whose cutouts are to be cleared.
 * @see evas_common_draw_context_cutouts_free()
 */
EVAS_API void
evas_common_draw_context_clear_cutouts(RGBA_Draw_Context *dc)
{
   evas_common_draw_context_cutouts_free(&dc->cutout);
}

/**
 * @brief Sets font extension callbacks for a draw context.
 *
 * This function allows custom font rendering logic to be plugged into the
 * draw context. It sets various callback functions for creating, freeing,
 * and drawing font glyphs and glyph images.
 *
 * @param dc The RGBA_Draw_Context to configure.
 * @param data User data to be passed to the callback functions.
 * @param gl_new Callback to create new glyph-specific data.
 * @param gl_free Callback to free glyph-specific data.
 * @param gl_draw Callback to draw a font glyph.
 * @param gl_image_new Callback to create a new image for a glyph.
 * @param gl_image_free Callback to free a glyph image.
 * @param gl_image_draw Callback to draw a glyph image.
 */
EVAS_API void
evas_common_draw_context_font_ext_set(RGBA_Draw_Context *dc,
                                      void *data,
                                      void *(*gl_new)  (void *data, RGBA_Font_Glyph *fg),
                                      void  (*gl_free) (void *ext_dat),
                                      void  (*gl_draw) (void *data, void *dest, void *context, RGBA_Font_Glyph *fg, int x, int y, int w, int h),
                                      void *(*gl_image_new) (void *gc, RGBA_Font_Glyph *fg, int alpha, Evas_Colorspace cspace),
                                      void  (*gl_image_free) (void *image),
                                      void  (*gl_image_draw) (void *gc, void *im, int dx, int dy, int dw, int dh, int smooth))
{
   dc->font_ext.data = data;
   dc->font_ext.func.gl_new = gl_new;
   dc->font_ext.func.gl_free = gl_free;
   dc->font_ext.func.gl_draw = gl_draw;
   dc->font_ext.func.gl_image_new = gl_image_new;
   dc->font_ext.func.gl_image_free = gl_image_free;
   dc->font_ext.func.gl_image_draw = gl_image_draw;
}

/**
 * @brief Intersects the current clip region with a new rectangle.
 *
 * If a clip region is already set on the draw context, this function
 * modifies the existing clip region to be the intersection of itself
 * and the new rectangle defined by (x, y, w, h). If no clip region is
 * set, this function behaves like evas_common_draw_context_set_clip().
 *
 * @param dc The RGBA_Draw_Context.
 * @param x The x-coordinate of the new clipping rectangle.
 * @param y The y-coordinate of the new clipping rectangle.
 * @param w The width of the new clipping rectangle.
 * @param h The height of the new clipping rectangle.
 * @see evas_common_draw_context_set_clip()
 * @see evas_common_draw_context_unset_clip()
 */
EVAS_API void
evas_common_draw_context_clip_clip(RGBA_Draw_Context *dc, int x, int y, int w, int h)
{
   if (dc->clip.use)
     {
	RECTS_CLIP_TO_RECT(dc->clip.x, dc->clip.y, dc->clip.w, dc->clip.h,
			   x, y, w, h);
     }
   else
     evas_common_draw_context_set_clip(dc, x, y, w, h);
}

/**
 * @brief Sets the clip region for a draw context.
 *
 * This function defines a rectangular clipping region for all subsequent
 * drawing operations using this context.
 *
 * @param dc The RGBA_Draw_Context.
 * @param x The x-coordinate of the clipping rectangle.
 * @param y The y-coordinate of the clipping rectangle.
 * @param w The width of the clipping rectangle.
 * @param h The height of the clipping rectangle.
 * @see evas_common_draw_context_clip_clip()
 * @see evas_common_draw_context_unset_clip()
 */
EVAS_API void
evas_common_draw_context_set_clip(RGBA_Draw_Context *dc, int x, int y, int w, int h)
{
   dc->clip.use = 1;
   dc->clip.x = x;
   dc->clip.y = y;
   dc->clip.w = w;
   dc->clip.h = h;
}

/**
 * @brief Unsets the clip region for a draw context.
 *
 * Disables clipping for subsequent drawing operations using this context.
 *
 * @param dc The RGBA_Draw_Context.
 * @see evas_common_draw_context_set_clip()
 * @see evas_common_draw_context_clip_clip()
 */
EVAS_API void
evas_common_draw_context_unset_clip(RGBA_Draw_Context *dc)
{
   dc->clip.use = 0;
}

/**
 * @brief Sets the drawing color for a draw context.
 *
 * Defines the color (including alpha) to be used for subsequent drawing
 * operations. If Pixman is enabled and used for certain operations,
 * this function also creates a Pixman solid fill image for the color.
 *
 * @param dc The RGBA_Draw_Context.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 */
EVAS_API void
evas_common_draw_context_set_color(RGBA_Draw_Context *dc, int r, int g, int b, int a)
{
   R_VAL(&(dc->col.col)) = (DATA8)r;
   G_VAL(&(dc->col.col)) = (DATA8)g;
   B_VAL(&(dc->col.col)) = (DATA8)b;
   A_VAL(&(dc->col.col)) = (DATA8)a;
#ifdef HAVE_PIXMAN
#if defined(PIXMAN_FONT) || defined(PIXMAN_RECT) || defined(PIXMAN_LINE) || defined(PIXMAN_POLY)
   if (dc->col.pixman_color_image)
     pixman_image_unref(dc->col.pixman_color_image);

   pixman_color_t pixman_color;

   pixman_color.alpha =  (dc->col.col & 0xff000000) >> 16;
   pixman_color.red = (dc->col.col & 0x00ff0000) >> 8;
   pixman_color.green = (dc->col.col & 0x0000ff00);
   pixman_color.blue = (dc->col.col & 0x000000ff) << 8;

   dc->col.pixman_color_image = pixman_image_create_solid_fill(&pixman_color);
#endif
#endif

}

/**
 * @brief Sets the color multiplier for a draw context.
 *
 * Defines a color multiplier (including alpha) to be applied to colors
 * during drawing operations. This is typically used for effects like tinting.
 *
 * @param dc The RGBA_Draw_Context.
 * @param r Red component of the multiplier (0-255).
 * @param g Green component of the multiplier (0-255).
 * @param b Blue component of the multiplier (0-255).
 * @param a Alpha component of the multiplier (0-255).
 * @see evas_common_draw_context_unset_multiplier()
 */
EVAS_API void
evas_common_draw_context_set_multiplier(RGBA_Draw_Context *dc, int r, int g, int b, int a)
{
   dc->mul.use = 1;
   R_VAL(&(dc->mul.col)) = (DATA8)r;
   G_VAL(&(dc->mul.col)) = (DATA8)g;
   B_VAL(&(dc->mul.col)) = (DATA8)b;
   A_VAL(&(dc->mul.col)) = (DATA8)a;
}

/**
 * @brief Unsets the color multiplier for a draw context.
 *
 * Disables the color multiplier for subsequent drawing operations.
 *
 * @param dc The RGBA_Draw_Context.
 * @see evas_common_draw_context_set_multiplier()
 */
EVAS_API void
evas_common_draw_context_unset_multiplier(RGBA_Draw_Context *dc)
{
   dc->mul.use = 0;
}

/**
 * @brief Sets the maximum number of cutouts allowed.
 *
 * Defines the upper limit for the number of cutout rectangles that can be
 * added to the draw context.
 *
 * @param dc The RGBA_Draw_Context.
 * @param max The maximum number of cutouts.
 */
EVAS_API void
evas_common_draw_context_cutout_max_set(RGBA_Draw_Context *dc, int max)
{
   dc->cutout.count_max = max;
}

/**
 * @brief Sets the minimum size for a cutout to be considered.
 *
 * Defines the minimum area (width * height) a rectangle must have to be
 * added as a cutout. Rectangles smaller than this will be ignored.
 *
 * @param dc The RGBA_Draw_Context.
 * @param min The minimum area (width * height) for a cutout.
 */
EVAS_API void
evas_common_draw_context_cutout_size_min_set(RGBA_Draw_Context *dc, int min)
{
  dc->cutout.size_min = min;
}

/**
 * @brief Adds a cutout rectangle to the draw context.
 *
 * This function adds a new rectangle to the list of cutouts.
 * The rectangle is first clipped to the current clip region of the draw context,
 * if one is set. Cutouts smaller than `dc->cutout.size_min` (area) or
 * if `dc->cutout.active >= dc->cutout.count_max` are ignored.
 * It also avoids adding duplicate consecutive rectangles.
 *
 * @param dc The RGBA_Draw_Context.
 * @param x The x-coordinate of the cutout rectangle.
 * @param y The y-coordinate of the cutout rectangle.
 * @param w The width of the cutout rectangle.
 * @param h The height of the cutout rectangle.
 */
EVAS_API void
evas_common_draw_context_add_cutout(RGBA_Draw_Context *dc, int x, int y, int w, int h)
{
   if (dc->cutout.active >= dc->cutout.count_max) return;
   if ((w * h) < dc->cutout.size_min) return;
   if (dc->clip.use)
     {
#if 1 // this is a bit faster
        int x1, x2, y1, y2;
        int cx1, cx2, cy1, cy2;

        x2 = x + w;
        cx1 = dc->clip.x;
        if (x2 <= cx1) return;
        x1 = x;
        cx2 = cx1 + dc->clip.w;
        if (x1 >= cx2) return;

        if (x1 < cx1) x1 = cx1;
        if (x2 > cx2) x2 = cx2;

        y2 = y + h;
        cy1 = dc->clip.y;
        if (y2 <= cy1) return;
        y1 = y;
        cy2 = cy1 + dc->clip.h;
        if (y1 >= cy2) return;

        if (y1 < cy1) y1 = cy1;
        if (y2 > cy2) y2 = cy2;

        x = x1;
        y = y1;
        w = x2 - x1;
        h = y2 - y1;
#else
        RECTS_CLIP_TO_RECT(x, y, w, h,
                           dc->clip.x, dc->clip.y, dc->clip.w, dc->clip.h);
#endif
       if ((w * h) < dc->cutout.size_min) return;
     }
   if (dc->cutout.last_add.w > 0)
     {
        if ((dc->cutout.last_add.x == x) && (dc->cutout.last_add.y == y) &&
            (dc->cutout.last_add.w == w) && (dc->cutout.last_add.h == h)) return;
     }
   dc->cutout.last_add.x = x;
   dc->cutout.last_add.y = y;
   dc->cutout.last_add.w = w;
   dc->cutout.last_add.h = h;
   evas_common_draw_context_cutouts_add(&dc->cutout, x, y, w, h);
}

/**
 * @brief Splits a rectangle in a Cutout_Rects list by another rectangle.
 *
 * This function takes a rectangle `in` (at `res->rects[idx]`) and a `split`
 * rectangle. If they intersect, `in` is modified and/or new rectangles
 * are added to `res` such that the area previously covered by `in` that
 * intersected with `split` is removed. The original `in` rectangle at `idx`
 * might be modified or deleted.
 *
 * The logic handles 16 different intersection cases by checking how the
 * `split` rectangle overlaps with the `in` rectangle (e.g., `split` fully
 * contains `in`, `in` fully contains `split`, `split` overlaps one edge,
 * `split` overlaps a corner, etc.).
 *
 * @param res The Cutout_Rects list containing the rectangle to be split.
 *            New resulting rectangles are also added to this list.
 * @param idx The index of the rectangle in `res->rects` to be split.
 * @param split The rectangle used to split the rectangle at `res->rects[idx]`.
 * @return 1 if the rectangle at `idx` (or its remnants) still exists, 0 if it was deleted.
 */
static int
evas_common_draw_context_cutout_split(Cutout_Rects *res, int idx, Cutout_Rect *split)
{
   /* 1 input rect, multiple out */
   Cutout_Rect in = res->rects[idx];

   /* this is to save me a LOT of typing */
#define INX1 (in.x)
#define INX2 (in.x + in.w)
#define SPX1 (split->x)
#define SPX2 (split->x + split->w)
#define INY1 (in.y)
#define INY2 (in.y + in.h)
#define SPY1 (split->y)
#define SPY2 (split->y + split->h)
#define X1_IN (in.x < split->x)
#define X2_IN ((in.x + in.w) > (split->x + split->w))
#define Y1_IN (in.y < split->y)
#define Y2_IN ((in.y + in.h) > (split->y + split->h))
#define R_NEW(_r, _x, _y, _w, _h) { evas_common_draw_context_cutouts_add(_r, _x, _y, _w, _h); }
   if (!RECTS_INTERSECT(in.x, in.y, in.w, in.h,
			split->x, split->y, split->w, split->h))
     {
        /* No colision => no clipping, don't touch it. */
	return 1;
     }

   /* S    = split (ie cut out rect) */
   /* +--+ = in (rect to be cut) */

   /*
    *  +---+
    *  |   |
    *  | S |
    *  |   |
    *  +---+
    *
    */
   if (X1_IN && X2_IN && Y1_IN && Y2_IN)
     {
        R_NEW(res, in.x, in.y, in.w, SPY1 - in.y);
	R_NEW(res, in.x, SPY1, SPX1 - in.x, SPY2 - SPY1);
	R_NEW(res, SPX2, SPY1, INX2 - SPX2, SPY2 - SPY1);
        /* out => (in.x, SPY2, in.w, INY2 - SPY2) */
        res->rects[idx].h = INY2 - SPY2;
        res->rects[idx].y = SPY2;
	return 1;
     }
   /* SSSSSSS
    * S+---+S
    * S|SSS|S
    * S|SSS|S
    * S|SSS|S
    * S+---+S
    * SSSSSSS
    */
   if (!X1_IN && !X2_IN && !Y1_IN && !Y2_IN)
     {
        evas_common_draw_context_cutouts_del(res, idx);
	return 0;
     }
   /* SSS
    * S+---+
    * S|S  |
    * S|S  |
    * S|S  |
    * S+---+
    * SSS
    */
   if (!X1_IN && X2_IN && !Y1_IN && !Y2_IN)
     {
        /* in => (SPX2, in.y, INX2 - SPX2, in.h) */
        res->rects[idx].w = INX2 - SPX2;
        res->rects[idx].x = SPX2;
	return 1;
     }
   /*    S
    *  +---+
    *  | S |
    *  | S |
    *  | S |
    *  +---+
    *    S
    */
   if (X1_IN && X2_IN && !Y1_IN && !Y2_IN)
     {
        R_NEW(res, in.x, in.y, SPX1 - in.x, in.h);
        /* in => (SPX2, in.y, INX2 - SPX2, in.h) */
        res->rects[idx].w = INX2 - SPX2;
        res->rects[idx].x = SPX2;
	return 1;
     }
   /*     SSS
    *  +---+S
    *  |  S|S
    *  |  S|S
    *  |  S|S
    *  +---+S
    *     SSS
    */
   if (X1_IN && !X2_IN && !Y1_IN && !Y2_IN)
     {
        /* in => (in.x, in.y, SPX1 - in.x, in.h) */
        res->rects[idx].w = SPX1 - in.x;
	return 1;
     }
   /* SSSSSSS
    * S+---+S
    * S|SSS|S
    *  |   |
    *  |   |
    *  +---+
    *
    */
   if (!X1_IN && !X2_IN && !Y1_IN && Y2_IN)
     {
        /* in => (in.x, SPY2, in.w, INY2 - SPY2) */
        res->rects[idx].h = INY2 - SPY2;
        res->rects[idx].y = SPY2;
	return 1;
     }
   /*
    *  +---+
    *  |   |
    * S|SSS|S
    *  |   |
    *  +---+
    *
    */
   if (!X1_IN && !X2_IN && Y1_IN && Y2_IN)
     {
        R_NEW(res, in.x, SPY2, in.w, INY2 - SPY2);
        /* in => (in.x, in.y, in.w, SPY1 - in.y) */
        res->rects[idx].h = SPY1 - in.y;
	return 1;
     }
   /*
    *  +---+
    *  |   |
    *  |   |
    * S|SSS|S
    * S+---+S
    * SSSSSSS
    */
   if (!X1_IN && !X2_IN && Y1_IN && !Y2_IN)
     {
        /* in => (in.x, in.y, in.w, SPY1 - in.y) */
        res->rects[idx].h = SPY1 - in.y;
	return 1;
     }
   /* SSS
    * S+---+
    * S|S  |
    *  |   |
    *  |   |
    *  +---+
    *
    */
   if (!X1_IN && X2_IN && !Y1_IN && Y2_IN)
     {
	R_NEW(res, SPX2, in.y, INX2 - SPX2, SPY2 - in.y);
        /* in => (in.x, SPY2, in.w, INY2 - SPY2) */
        res->rects[idx].h = INY2 - SPY2;
        res->rects[idx].y = SPY2;
	return 1;
     }
   /*    S
    *  +---+
    *  | S |
    *  |   |
    *  |   |
    *  +---+
    *
    */
   if (X1_IN && X2_IN && !Y1_IN && Y2_IN)
     {
	R_NEW(res, in.x, in.y, SPX1 - in.x, SPY2 - in.y);
	R_NEW(res, SPX2, in.y, INX2 - SPX2, SPY2 - in.y);
        /* in => (in.x, SPY2, in.w, INY2 - SPY2) */
        res->rects[idx].h = INY2 - SPY2;
        res->rects[idx].y = SPY2;
	return 1;
     }
   /*     SSS
    *  +---+S
    *  |  S|S
    *  |   |
    *  |   |
    *  +---+
    *
    */
   if (X1_IN && !X2_IN && !Y1_IN && Y2_IN)
     {
	R_NEW(res, in.x, in.y, SPX1 - in.x, SPY2 - in.y);
        /* in => (in.x, SPY2, in.w, INY2 - SPY2) */
        res->rects[idx].h = INY2 - SPY2;
        res->rects[idx].y = SPY2;
	return 1;
     }
   /*
    *  +---+
    *  |   |
    * S|S  |
    *  |   |
    *  +---+
    *
    */
   if (!X1_IN && X2_IN && Y1_IN && Y2_IN)
     {
	R_NEW(res, in.x, SPY2, in.w, INY2 - SPY2);
	R_NEW(res, SPX2, SPY1, INX2 - SPX2, SPY2 - SPY1);
        /* in => (in.x, SPY2, in.w, INY2 - SPY2) */
        res->rects[idx].h = SPY1 - in.y;
	return 1;
     }
   /*
    *  +---+
    *  |   |
    *  |  S|S
    *  |   |
    *  +---+
    *
    */
   if (X1_IN && !X2_IN && Y1_IN && Y2_IN)
     {
	R_NEW(res, in.x, SPY2, in.w, INY2 - SPY2);
	R_NEW(res, in.x, SPY1, SPX1 - in.x, SPY2 - SPY1);
        /* in => (in.x, in.y, in.w, SPY1 - in.y) */
        res->rects[idx].h = SPY1 - in.y;
	return 1;
     }
   /*
    *  +---+
    *  |   |
    *  |   |
    * S|S  |
    * S+---+
    * SSS
    */
   if (!X1_IN && X2_IN && Y1_IN && !Y2_IN)
     {
        R_NEW(res, SPX2, SPY1, INX2 - SPX2, INY2 - SPY1);
        /* in => (in.x, in.y, in.w, SPY1 - in.y) */
        res->rects[idx].h = SPY1 - in.y;
	return 1;
     }
   /*
    *  +---+
    *  |   |
    *  |   |
    *  | S |
    *  +---+
    *    S
    */
   if (X1_IN && X2_IN && Y1_IN && !Y2_IN)
     {
	R_NEW(res, in.x, SPY1, SPX1 - in.x, INY2 - SPY1);
        R_NEW(res, SPX2, SPY1, INX2 - SPX2, INY2 - SPY1);
        /* in => (in.x, in.y, in.w, SPY1 - in.y) */
        res->rects[idx].h = SPY1 - in.y;
	return 1;
     }
   /*
    *  +---+
    *  |   |
    *  |   |
    *  |  S|S
    *  +---+S
    *     SSS
    */
   if (X1_IN && !X2_IN && Y1_IN && !Y2_IN)
     {
        R_NEW(res, in.x, SPY1, SPX1 - in.x, INY2 - SPY1);
        /* in => (in.x, in.y, in.w, SPY1 - in.y) */
        res->rects[idx].h = SPY1 - in.y;
	return 1;
     }
   evas_common_draw_context_cutouts_del(res, idx);
   return 0;
#undef INX1
#undef INX2
#undef SPX1
#undef SPX2
#undef INY1
#undef INY2
#undef SPY1
#undef SPY2
#undef X1_IN
#undef X2_IN
#undef Y1_IN
#undef Y2_IN
#undef R_NEW
}

/**
 * @brief Sets the target area for cutout application.
 *
 * When applying cutouts, only those cutouts that intersect with this
 * target area will be considered. If the target width is 0, all cutouts
 * are considered (effectively disabling this specific filter).
 *
 * @param dc The RGBA_Draw_Context.
 * @param x The x-coordinate of the target area.
 * @param y The y-coordinate of the target area.
 * @param w The width of the target area.
 * @param h The height of the target area.
 */
EVAS_API void
evas_common_draw_context_target_set(RGBA_Draw_Context *dc, int x, int y, int w, int h)
{
   dc->cutout_target.x = x;
   dc->cutout_target.y = y;
   dc->cutout_target.w = w;
   dc->cutout_target.h = h;
}

/**
 * @brief Comparison function for qsort to sort Cutout_Rects by Y, then X.
 *
 * Used to sort an array of Cutout_Rect structures primarily by their
 * y-coordinate, and secondarily by their x-coordinate for tie-breaking.
 *
 * @param d1 Pointer to the first Cutout_Rect.
 * @param d2 Pointer to the second Cutout_Rect.
 * @return An integer less than, equal to, or greater than zero if the
 *         first argument is considered to be respectively less than,
 *         equal to, or greater than the second.
 */
static int
_srt_y(const void *d1, const void *d2)
{
   const Cutout_Rect *r1 = d1, *r2 = d2;
   if (r1->y == r2->y) return r1->x - r2->x;
   return r1->y - r2->y;
}

/**
 * @brief Comparison function for qsort to sort Cutout_Rects by X, then Y.
 *
 * Used to sort an array of Cutout_Rect structures primarily by their
 * x-coordinate, and secondarily by their y-coordinate for tie-breaking.
 *
 * @param d1 Pointer to the first Cutout_Rect.
 * @param d2 Pointer to the second Cutout_Rect.
 * @return An integer less than, equal to, or greater than zero if the
 *         first argument is considered to be respectively less than,
 *         equal to, or greater than the second.
 */
static int
_srt_x(const void *d1, const void *d2)
{
   const Cutout_Rect *r1 = d1, *r2 = d2;
   if (r1->x == r2->x) return r1->y - r2->y;
   return r1->x - r2->x;
}

/**
 * @brief Applies cutouts to the draw context's clip region.
 *
 * This function calculates the final set of drawable rectangles by taking
 * the draw context's current clip region and subtracting all the defined
 * cutouts from it.
 *
 * The process involves:
 * 1. Initializing a result Cutout_Rects list (`res`) with the context's clip rectangle.
 * 2. For each cutout in `dc->cutout`:
 *    a. If a `dc->cutout_target` is set, skip cutouts not intersecting it.
 *    b. Split each rectangle currently in `res` by the current cutout.
 *       This uses `evas_common_draw_context_cutout_split`.
 * 3. Merge adjacent or overlapping rectangles in `res` to simplify the list.
 *    This merging step has two paths:
 *    a. For a small number of rectangles (<= 5), a simpler O(n^2) merge is used.
 *    b. For a larger number, qsort is used (first by Y then X, then by X then Y)
 *       to optimize finding mergeable rectangles.
 * 4. Repack `res` to remove any invalidated (zero-width) rectangles.
 *
 * @param dc The RGBA_Draw_Context containing the clip region and cutouts.
 * @param reuse An optional Cutout_Rects structure to reuse for the result,
 *              reducing allocations. If NULL, a new one is created.
 * @return A pointer to a Cutout_Rects structure containing the final list
 *         of drawable rectangles. Returns NULL if clipping is not enabled or
 *         if the clip region is invalid. The caller is responsible for freeing
 *         this structure using `evas_common_draw_context_apply_clear_cutouts`
 *         or `evas_common_draw_context_cutouts_real_free` if `reuse` was NULL.
 *         If `reuse` was provided, it should be cleaned with
 *         `evas_common_draw_context_apply_clean_cutouts` when no longer needed,
 *         or `evas_common_draw_context_cutouts_real_free` to fully free it.
 *
 * @note The `reuse` parameter allows for optimization by avoiding repeated
 *       allocations and deallocations of the `Cutout_Rects` structure if this
 *       function is called frequently.
 *
 * Example of `res->rects` structure after processing:
 * An array of `Cutout_Rect` elements, where each element is:
 * ```c
 * typedef struct _Cutout_Rect Cutout_Rect;
 * struct _Cutout_Rect
 * {
 *    int x, y, w, h;
 * };
 * ```
 * Example: `res->rects = [{x=0,y=0,w=10,h=10}, {x=20,y=0,w=10,h=10}]`
 * This would represent two drawable areas.
 */
EVAS_API Cutout_Rects *
evas_common_draw_context_apply_cutouts(RGBA_Draw_Context *dc, Cutout_Rects *reuse)
{
   Cutout_Rects        *res = NULL;
   int                  i, j, active, found = 0;

   if (!dc->clip.use) return NULL;
   if ((dc->clip.w <= 0) || (dc->clip.h <= 0)) return NULL;

   if (!reuse) res = evas_common_draw_context_cutouts_new();
   else
     {
        evas_common_draw_context_cutouts_free(reuse);
        res = reuse;
     }
   // this avoids a nasty case of O(n^2)/2 below with lots of rectangles
   // to merge so only do this merging if the number of rects is small enough
   // not to blow out into insanity
   evas_common_draw_context_cutouts_add(res, dc->clip.x, dc->clip.y, dc->clip.w, dc->clip.h);
   for (i = 0; i < dc->cutout.active; i++)
     {
        if ((dc->cutout_target.w != 0) &&
            (!RECTS_INTERSECT(dc->cutout.rects[i].x, dc->cutout.rects[i].y,
                              dc->cutout.rects[i].w, dc->cutout.rects[i].h,
                              dc->cutout_target.x, dc->cutout_target.y,
                              dc->cutout_target.w, dc->cutout_target.h)))
          continue;
        // Don't loop on the element just added to the list as they are
        // already correctly clipped.
        active = res->active;
        for (j = 0; j < active; )
          {
             if (evas_common_draw_context_cutout_split
                 (res, j, dc->cutout.rects + i)) j++;
             else active--;
          }
     }
   /* merge rects */
#define RI res->rects[i]
#define RJ res->rects[j]
   if (res->active > 1)
     {
        if (res->active > 5)
          {
             // fast path for larger numbers of rects to merge by using
             // qsort to sort by y and x to limit the number of rects
             // we have to walk as rects that have a different y cannot
             // be merged anyway (or x).
             qsort(res->rects, res->active, sizeof(res->rects[0]), _srt_y);
             for (i = 0; i < res->active; i++)
               {
                  if (RI.w == 0) continue; // skip empty rect
                  for (j = i + 1; j < res->active; j++)
                    {
                       if (RJ.y != RI.y) break; // new line, sorted thus skip
                       if (RJ.w == 0) continue; // skip empty rect
                       // if J is the same height (could be merged)
                       if (RJ.h == RI.h)
                         {
                            // if J is immediately to the right of I
                            if (RJ.x == (RI.x + RI.w))
                              {
                                 RI.w = (RJ.x + RJ.w) - RI.x; // expand RI
                                 RJ.w = 0; // invalidate
                                 found++;
                              }
                            // since we sort y and THEN x, if height matches
                            // but it's not immediately adjacent, no more
                            // rects exists that can be merged
                            else break;
                         }
                    }
               }
             qsort(res->rects, res->active, sizeof(res->rects[0]), _srt_x);
             for (i = 0; i < res->active; i++)
               {
                  if (RI.w == 0) continue; // skip empty rect
                  for (j = i + 1; j < res->active; j++)
                    {
                       if (RJ.x != RI.x) break; // new line, sorted thus skip
                       if (RJ.w == 0) continue; // skip empty rect
                       // if J is the same height (could be merged)
                       if (RJ.w == RI.w)
                         {
                            // if J is immediately to the right of I
                            if (RJ.y == (RI.y + RI.h))
                              {
                                 RI.h = (RJ.y + RJ.h) - RI.y; // expand RI
                                 RJ.w = 0; // invalidate
                                 found++;
                              }
                            // since we sort y and THEN x, if height matches
                            // but it's not immediately adjacent, no more
                            // rects exists that can be merged
                            else break;
                         }
                    }
               }
          }
        else
          {
             // for a small number of rects, keep things simple as the count
             // is small and big-o complexity isnt a problem yet
             found = 1;
             while (found)
               {
                  found = 0;
                  for (i = 0; i < res->active; i++)
                    {
                       for (j = i + 1; j < res->active; j++)
                         {
                            // skip empty rects we are removing
                            if (RJ.w == 0) continue;
                            // check if its same width, immediately above or below
                            if ((RJ.w == RI.w) && (RJ.x == RI.x))
                              {
                                 if ((RJ.y + RJ.h) == RI.y) // above
                                   {
                                      RI.y = RJ.y;
                                      RI.h += RJ.h;
                                      RJ.w = 0;
                                      found++;
                                   }
                                 else if ((RI.y + RI.h) == RJ.y) // below
                                   {
                                      RI.h += RJ.h;
                                      RJ.w = 0;
                                      found++;
                                   }
                              }
                            // check if its same height, immediately left or right
                            else if ((RJ.h == RI.h) && (RJ.y == RI.y))
                              {
                                 if ((RJ.x + RJ.w) == RI.x) // left
                                   {
                                      RI.x = RJ.x;
                                      RI.w += RJ.w;
                                      RJ.w = 0;
                                      found++;
                                   }
                                 else if ((RI.x + RI.w) == RJ.x) // right
                                   {
                                      RI.w += RJ.w;
                                      RJ.w = 0;
                                      found++;
                                   }
                              }
                         }
                    }
               }
          }

        // Repack the cutout
        j = 0;
        for (i = 0; i < res->active; i++)
          {
             if (RI.w == 0) continue;
             if (i != j) RJ = RI;
             j++;
          }
        res->active = j;
     }
   return res;
}

/**
 * @brief Cleans and frees a Cutout_Rects structure.
 *
 * This function first cleans the internal data of the Cutout_Rects structure
 * (frees the `rects` array and resets members) using
 * `evas_common_draw_context_apply_clean_cutouts`, and then frees the
 * Cutout_Rects structure itself.
 *
 * This is typically used to free a `Cutout_Rects` structure returned by
 * `evas_common_draw_context_apply_cutouts` when `reuse` was NULL.
 *
 * @param rects The Cutout_Rects structure to clean and free.
 * @see evas_common_draw_context_apply_cutouts()
 * @see evas_common_draw_context_apply_clean_cutouts()
 */
EVAS_API void
evas_common_draw_context_apply_clear_cutouts(Cutout_Rects *rects)
{
   evas_common_draw_context_apply_clean_cutouts(rects);
   free(rects);
}

/**
 * @brief Cleans the internal data of a Cutout_Rects structure for reuse.
 *
 * This function frees the array of rectangles (`rects->rects`) and resets
 * other members of the Cutout_Rects structure (`active`, `max`, `last_add`).
 * The Cutout_Rects structure itself is not freed, allowing it to be reused.
 *
 * This is useful for cleaning a `Cutout_Rects` structure that was passed as
 * the `reuse` parameter to `evas_common_draw_context_apply_cutouts`.
 *
 * @param rects The Cutout_Rects structure to clean.
 * @see evas_common_draw_context_apply_cutouts()
 * @see evas_common_draw_context_apply_clear_cutouts()
 */
EVAS_API void
evas_common_draw_context_apply_clean_cutouts(Cutout_Rects *rects)
{
   free(rects->rects);
   rects->rects = NULL;
   rects->active = 0;
   rects->max = 0;
   rects->last_add.w = 0;
}

/**
 * @brief Sets the anti-aliasing mode for a draw context.
 *
 * @param dc The RGBA_Draw_Context.
 * @param aa If non-zero, anti-aliasing is enabled; otherwise, it's disabled.
 */
EVAS_API void
evas_common_draw_context_set_anti_alias(RGBA_Draw_Context *dc , unsigned char aa)
{
   dc->anti_alias = !!aa;
}

/**
 * @brief Sets the color interpolation mode for a draw context.
 *
 * This affects how colors are interpolated, for example, during gradient
 * rendering or image scaling.
 *
 * @param dc The RGBA_Draw_Context.
 * @param color_space The color space to use for interpolation.
 *                    (e.g., EVAS_COLORSPACE_ARGB8888, EVAS_COLORSPACE_YCBCR422P601_PL)
 */
EVAS_API void
evas_common_draw_context_set_color_interpolation(RGBA_Draw_Context *dc, int color_space)
{
   dc->interpolation.color_space = color_space;
}

/**
 * @brief Sets the rendering operation for a draw context.
 *
 * This defines how source pixels are combined with destination pixels.
 * (e.g., EVAS_RENDER_BLEND, EVAS_RENDER_COPY, etc.)
 *
 * @param dc The RGBA_Draw_Context.
 * @param op The rendering operation to set.
 */
EVAS_API void
evas_common_draw_context_set_render_op(RGBA_Draw_Context *dc , int op)
{
   dc->render_op = op;
}
