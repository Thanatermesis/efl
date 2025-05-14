#include "evas_font_private.h"
#include "evas_font_draw.h"

#include FT_OUTLINE_H
#include FT_SYNTHESIS_H
#include FT_BITMAP_H
#include FT_TRUETYPE_DRIVER_H

FT_Library      evas_ft_lib = 0;
static int      initialised = 0;

LK(lock_font_draw); // for freetype2 API calls
LK(lock_bidi); // for evas bidi internal usage.
LK(lock_ot); // for evas bidi internal usage.

int             _evas_font_log_dom_global = -1;
int             _evas_font_texture_cache = -1;

/**
 * @brief Initializes the Evas common font subsystem.
 *
 * This function sets up the FreeType library, initializes font loading
 * and drawing mechanisms, and registers a log domain. It also handles
 * setting the DPI from the EVAS_FONT_DPI environment variable if present.
 * This function maintains an initialization counter to ensure FreeType
 * is initialized only once.
 */
EVAS_API void
evas_common_font_init(void)
{
   int error;
   const char *s;
   FT_UInt interpreter_version =
#ifndef TT_INTERPRETER_VERSION_35
   TT_INTERPRETER_VERSION_35;
#else
   35;
#endif
   _evas_font_log_dom_global = eina_log_domain_register
     ("evas_font_main", EVAS_FONT_DEFAULT_LOG_COLOR);
   if (_evas_font_log_dom_global < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
     }

   initialised++;
   if (initialised != 1) return;
   error = FT_Init_FreeType(&evas_ft_lib);
   if (error) return;
   FT_Property_Set(evas_ft_lib, "truetype", "interpreter-version",
                   &interpreter_version);
   evas_common_font_load_init();
   evas_common_font_draw_init();
   s = getenv("EVAS_FONT_DPI");
   if (s)
     {
        int dpi_h = 75, dpi_v = 0;

        if (sscanf(s, "%dx%d", &dpi_h, &dpi_v) < 2)
          dpi_h = dpi_v = atoi(s);

        if (dpi_h > 0) evas_common_font_dpi_set(dpi_h, dpi_v);
     }
   LKI(lock_font_draw);
   LKI(lock_bidi);
   LKI(lock_ot);
}

/**
 * @brief Shuts down the Evas common font subsystem.
 *
 * This function cleans up resources used by the font subsystem, including
 * shutting down font loading, clearing caches, and finalizing the FreeType
 * library. It uses an initialization counter to ensure FreeType is
 * finalized only when no longer in use.
 */
EVAS_API void
evas_common_font_shutdown(void)
{
   if (initialised < 1) return;
   initialised--;
   if (initialised != 0) return;

   evas_common_font_load_shutdown();
   evas_common_font_cache_set(0);
   evas_common_font_flush();

   FT_Done_FreeType(evas_ft_lib);
   evas_ft_lib = 0;

   LKD(lock_font_draw);
   LKD(lock_bidi);
   LKD(lock_ot);
   eina_log_domain_unregister(_evas_font_log_dom_global);
}

/**
 * @brief Unloads all currently loaded fonts.
 *
 * This function clears all font data, effectively unloading them from memory.
 */
EVAS_API void
evas_common_font_font_all_unload(void)
{
   evas_common_font_all_clear();
}

/**
 * @brief Retrieves the FreeType face associated with an RGBA_Font.
 * @param font The RGBA_Font from which to get the FreeType face.
 * @return A pointer to the FT_Face if successful, NULL otherwise.
 *
 * @note This function is a short-cut and might be fragile if the underlying
 * font rendering engine changes. It reloads the font instance if necessary.
 */
void *
evas_common_font_freetype_face_get(RGBA_Font *font)
{
   RGBA_Font_Int *fi = font->fonts->data;

   if (!fi)
      return NULL;

   evas_common_font_int_reload(fi);

   return fi->src->ft.face;
}

/**
 * @brief Gets the ascent of a specific font instance.
 * @param fi The font instance (RGBA_Font_Int).
 * @return The ascent value in pixels.
 *
 * The ascent is the distance from the baseline to the highest point reached by
 * glyphs in the font. This function ensures the font instance is loaded and
 * the correct size is activated in FreeType. It also handles scaling for
 * color bitmap fonts.
 */
EVAS_API int
evas_common_font_instance_ascent_get(RGBA_Font_Int *fi)
{
   int val;
   evas_common_font_int_reload(fi);
   if (fi->src->current_size != fi->size)
     {
        FTLOCK();
        FT_Activate_Size(fi->ft.size);
        FTUNLOCK();
        fi->src->current_size = fi->size;
     }
   if (!FT_IS_SCALABLE(fi->src->ft.face))
     {
        WRN("NOT SCALABLE!");
     }
   val = (int)fi->src->ft.face->size->metrics.ascender;

   if (FT_HAS_FIXED_SIZES(fi->src->ft.face))
     {
        if (FT_HAS_COLOR(fi->src->ft.face) &&
            fi->bitmap_scalable & EFL_TEXT_FONT_BITMAP_SCALABLE_COLOR)
          val *= fi->scale_factor;
     }

   return FONT_METRIC_ROUNDUP(val);
//   printf("%i | %i\n", val, val >> 6);
//   if (fi->src->ft.face->units_per_EM == 0)
//     return val;
//   dv = (fi->src->ft.orig_upem * 2048) / fi->src->ft.face->units_per_EM;
//   ret = (val * fi->src->ft.face->size->metrics.y_scale) / (dv * dv);
//   return ret;
}

/**
 * @brief Gets the descent of a specific font instance.
 * @param fi The font instance (RGBA_Font_Int).
 * @return The descent value in pixels (always positive).
 *
 * The descent is the distance from the baseline to the lowest point reached by
 * glyphs in the font. This function ensures the font instance is loaded and
 * the correct size is activated in FreeType. It also handles scaling for
 * color bitmap fonts.
 */
EVAS_API int
evas_common_font_instance_descent_get(RGBA_Font_Int *fi)
{
   int val;
   evas_common_font_int_reload(fi);
   if (fi->src->current_size != fi->size)
     {
        FTLOCK();
        FT_Activate_Size(fi->ft.size);
        FTUNLOCK();
        fi->src->current_size = fi->size;
     }
   val = -(int)fi->src->ft.face->size->metrics.descender;

   if (FT_HAS_FIXED_SIZES(fi->src->ft.face))
     {
        if (FT_HAS_COLOR(fi->src->ft.face) &&
            fi->bitmap_scalable & EFL_TEXT_FONT_BITMAP_SCALABLE_COLOR)
          val *= fi->scale_factor;
     }

   return FONT_METRIC_ROUNDUP(val);
//   if (fi->src->ft.face->units_per_EM == 0)
//     return val;
//   dv = (fi->src->ft.orig_upem * 2048) / fi->src->ft.face->units_per_EM;
//   ret = (val * fi->src->ft.face->size->metrics.y_scale) / (dv * dv);
//   return ret;
}

/**
 * @brief Gets the maximum ascent of a specific font instance.
 * @param fi The font instance (RGBA_Font_Int).
 * @return The maximum ascent value in pixels.
 *
 * This value represents the maximum vertical distance from the baseline to the
 * top of the font's bounding box. It considers the font's global bounding box
 * (yMax) if available, otherwise falls back to the ascender metric.
 * Handles scaling for color bitmap fonts and unit conversion if necessary.
 */
EVAS_API int
evas_common_font_instance_max_ascent_get(RGBA_Font_Int *fi)
{
   int val, dv;
   int ret;

   evas_common_font_int_reload(fi);
  if (fi->src->current_size != fi->size)
     {
        FTLOCK();
        FT_Activate_Size(fi->ft.size);
        FTUNLOCK();
        fi->src->current_size = fi->size;
     }
   if ((fi->src->ft.face->bbox.yMax == 0) &&
       (fi->src->ft.face->bbox.yMin == 0) &&
       (fi->src->ft.face->units_per_EM == 0))
     val = FONT_METRIC_ROUNDUP((int)fi->src->ft.face->size->metrics.ascender);
   else
     val = (int)fi->src->ft.face->bbox.yMax;

   if (FT_HAS_FIXED_SIZES(fi->src->ft.face))
     {
        if (FT_HAS_COLOR(fi->src->ft.face) &&
            fi->bitmap_scalable & EFL_TEXT_FONT_BITMAP_SCALABLE_COLOR)
          val *= fi->scale_factor;
     }

   if (fi->src->ft.face->units_per_EM == 0)
     return val;
   dv = (fi->src->ft.orig_upem * 2048) / fi->src->ft.face->units_per_EM;
   ret = FONT_METRIC_CONV(val, dv, fi->src->ft.face->size->metrics.y_scale);
   return ret;
}

/**
 * @brief Gets the maximum descent of a specific font instance.
 * @param fi The font instance (RGBA_Font_Int).
 * @return The maximum descent value in pixels (always positive).
 *
 * This value represents the maximum vertical distance from the baseline to the
 * bottom of the font's bounding box. It considers the font's global bounding box
 * (yMin) if available, otherwise falls back to the descender metric.
 * Handles scaling for color bitmap fonts and unit conversion if necessary.
 */
EVAS_API int
evas_common_font_instance_max_descent_get(RGBA_Font_Int *fi)
{
   int val, dv;
   int ret;

   evas_common_font_int_reload(fi);
   if (fi->src->current_size != fi->size)
     {
        FTLOCK();
        FT_Activate_Size(fi->ft.size);
        FTUNLOCK();
        fi->src->current_size = fi->size;
     }
   if ((fi->src->ft.face->bbox.yMax == 0) &&
       (fi->src->ft.face->bbox.yMin == 0) &&
       (fi->src->ft.face->units_per_EM == 0))
     val = FONT_METRIC_ROUNDUP(-(int)fi->src->ft.face->size->metrics.descender);
   else
     val = -(int)fi->src->ft.face->bbox.yMin;

   if (FT_HAS_FIXED_SIZES(fi->src->ft.face))
     {
        if (FT_HAS_COLOR(fi->src->ft.face) &&
            fi->bitmap_scalable & EFL_TEXT_FONT_BITMAP_SCALABLE_COLOR)
          val *= fi->scale_factor;
     }

   if (fi->src->ft.face->units_per_EM == 0)
     return val;
   dv = (fi->src->ft.orig_upem * 2048) / fi->src->ft.face->units_per_EM;
   ret = FONT_METRIC_CONV(val, dv, fi->src->ft.face->size->metrics.y_scale);
   return ret;
}

/**
 * @brief Gets the ascent of a font.
 * @param fn The font (RGBA_Font).
 * @return The ascent value in pixels.
 * @see evas_common_font_instance_ascent_get
 */
EVAS_API int
evas_common_font_ascent_get(RGBA_Font *fn)
{
//   evas_common_font_size_use(fn);
   return evas_common_font_instance_ascent_get(fn->fonts->data);
}

/**
 * @brief Gets the descent of a font.
 * @param fn The font (RGBA_Font).
 * @return The descent value in pixels.
 * @see evas_common_font_instance_descent_get
 */
EVAS_API int
evas_common_font_descent_get(RGBA_Font *fn)
{
//   evas_common_font_size_use(fn);
   return evas_common_font_instance_descent_get(fn->fonts->data);
}

/**
 * @brief Gets the maximum ascent of a font.
 * @param fn The font (RGBA_Font).
 * @return The maximum ascent value in pixels.
 * @see evas_common_font_instance_max_ascent_get
 */
EVAS_API int
evas_common_font_max_ascent_get(RGBA_Font *fn)
{
//   evas_common_font_size_use(fn);
   return evas_common_font_instance_max_ascent_get(fn->fonts->data);
}

/**
 * @brief Gets the maximum descent of a font.
 * @param fn The font (RGBA_Font).
 * @return The maximum descent value in pixels.
 * @see evas_common_font_instance_max_descent_get
 */
EVAS_API int
evas_common_font_max_descent_get(RGBA_Font *fn)
{
//   evas_common_font_size_use(fn);
   return evas_common_font_instance_max_descent_get(fn->fonts->data);
}

/**
 * @brief Gets the recommended vertical distance between baselines (line height).
 * @param fn The font (RGBA_Font).
 * @return The line advance value in pixels.
 *
 * This function retrieves the `height` metric from the FreeType face, which
 * typically represents the recommended distance between two consecutive
 * baselines of text. It handles scaling for color bitmap fonts.
 */
EVAS_API int
evas_common_font_get_line_advance(RGBA_Font *fn)
{
   int val;
   RGBA_Font_Int *fi;

//   evas_common_font_size_use(fn);
   fi = fn->fonts->data;
   evas_common_font_int_reload(fi);
   if (fi->src->current_size != fi->size)
     {
        FTLOCK();
        FT_Activate_Size(fi->ft.size);
        FTUNLOCK();
        fi->src->current_size = fi->size;
     }
   val = (int)fi->src->ft.face->size->metrics.height;

   if (FT_HAS_FIXED_SIZES(fi->src->ft.face))
     {
        if ((fi->bitmap_scalable & EFL_TEXT_FONT_BITMAP_SCALABLE_COLOR) &&
            FT_HAS_COLOR(fi->src->ft.face))
          val *= fi->scale_factor;
     }

   if ((fi->src->ft.face->bbox.yMax == 0) &&
       (fi->src->ft.face->bbox.yMin == 0) &&
       (fi->src->ft.face->units_per_EM == 0))
     return FONT_METRIC_ROUNDUP(val);
   else if (fi->src->ft.face->units_per_EM == 0)
     return val;
   return FONT_METRIC_ROUNDUP(val);
//   dv = (fi->src->ft.orig_upem * 2048) / fi->src->ft.face->units_per_EM;
//   ret = (val * fi->src->ft.face->size->metrics.y_scale) / (dv * dv);
//   return ret;
}

/**
 * @brief Gets the underline position for a font instance.
 * @param fi The font instance (RGBA_Font_Int).
 * @return The underline position in pixels from the baseline (typically negative).
 *         Returns 1 if the font provides no position or a zero position.
 *
 * This function retrieves the `underline_position` metric from the FreeType
 * face and scales it appropriately. If the resulting position is zero (which
 * often indicates a broken font or missing data), it defaults to 1 pixel.
 */
EVAS_API int
evas_common_font_instance_underline_position_get(RGBA_Font_Int *fi)
{
   int position = 0;

   if (!fi) goto end;

   evas_common_font_int_reload(fi);
   if (fi->src->current_size != fi->size)
     {
        FTLOCK();
        FT_Activate_Size(fi->ft.size);
        FTUNLOCK();
        fi->src->current_size = fi->size;
     }

   position = FT_MulFix(fi->src->ft.face->underline_position,
         fi->src->ft.face->size->metrics.x_scale);
   position = FONT_METRIC_ROUNDUP(abs(position));

end:
   /* This almost surely means a broken font, offset at least by one pixel. */
   if (position == 0)
      position = 1;

   return position;
}

/**
 * @brief Gets the underline thickness for a font instance.
 * @param fi The font instance (RGBA_Font_Int).
 * @return The underline thickness in pixels.
 *         Returns 1 if the font provides no thickness or a zero thickness.
 *
 * This function retrieves the `underline_thickness` metric from the FreeType
 * face and scales it appropriately. If the resulting thickness is zero (which
 * often indicates a broken font or missing data), it defaults to 1 pixel.
 */
EVAS_API int
evas_common_font_instance_underline_thickness_get(RGBA_Font_Int *fi)
{
   int thickness = 0;

   if (!fi) goto end;

   evas_common_font_int_reload(fi);
   if (fi->src->current_size != fi->size)
     {
        FTLOCK();
        FT_Activate_Size(fi->ft.size);
        FTUNLOCK();
        fi->src->current_size = fi->size;
     }

   thickness = FT_MulFix(fi->src->ft.face->underline_thickness,
         fi->src->ft.face->size->metrics.x_scale);
   thickness = FONT_METRIC_ROUNDUP(thickness);

end:
   /* This almost surely means a broken font, make it at least one pixel. */
   if (thickness == 0)
      thickness = 1;

   return thickness;
}

/* Set of common functions that are used in a couple of places. */

/**
 * @brief Frees a Fash_Int_Map structure and its associated variation lists.
 * @param map Pointer to the Fash_Int_Map to be freed.
 *
 * This function iterates through all items in the map. If an item has
 * a variation list, it frees the list's internal array and then the
 * list structure itself. Finally, it frees the map structure.
 */
static void
_fash_int_map_and_variations_free(Fash_Int_Map *map)
 {
   if(!map)
     return;
   int i;

   for (i = 0; i < 256; i++)
     {
        if (map->items[i].variations)
          {
             if (map->items[i].variations->list)
               {
                  free(map->items[i].variations->list);
                  map->items[i].variations->list = NULL;
                  map->items[i].variations->capacity = 0;
                  map->items[i].variations->length = 0;
               }
             free(map->items[i].variations);
             map->items[i].variations = NULL;
          }
     }

   free(map);
}

/**
 * @brief Frees a Fash_Int_Map2 structure.
 * @param fash Pointer to the Fash_Int_Map2 to be freed.
 *
 * This function iterates through all buckets in the Fash_Int_Map2.
 * For each non-NULL bucket (which is a Fash_Int_Map), it calls
 * _fash_int_map_and_variations_free to free it. Finally, it frees
 * the Fash_Int_Map2 structure itself.
 */
static void
_fash_int2_free(Fash_Int_Map2 *fash)
 {
   int i;
   if (fash)
     {
        for (i = 0; i < 256; i++)
          if (fash->bucket[i])
            {
               _fash_int_map_and_variations_free(fash->bucket[i]);
               fash->bucket[i] = NULL;
            }
        free(fash);
        fash = NULL;
     }
}

/**
 * @brief Frees a Fash_Int structure.
 * @param fash Pointer to the Fash_Int to be freed.
 *
 * This function is the main deallocator for Fash_Int structures,
 * typically assigned to `fash->freeme`. It checks for a magic number
 * to ensure it's a valid Fash_Int. It then iterates through its
 * buckets, calling _fash_int2_free for each non-NULL bucket
 * (which is a Fash_Int_Map2). Finally, it frees the Fash_Int
 * structure itself.
 */
static void
_fash_int_free(Fash_Int *fash)
{
   int i;
   if (fash)
     {
        if (fash->MAGIC != FASH_INT_MAGIC)
          {
             return;
          }

        for (i = 0; i < 256; i++)
          {
             if (fash->bucket[i])
               {
                  _fash_int2_free(fash->bucket[i]);
                  fash->bucket[i] = NULL;
               }
          }
        free(fash);
     }
}

/**
 * @brief Allocates and initializes a new Fash_Int structure.
 * @return A pointer to the newly allocated Fash_Int, or NULL on failure.
 *
 * Fash_Int is a three-level hash-like structure used for caching font
 * character to font instance/index mappings.
 * - Level 1 (Fash_Int): `bucket[256]` (indexed by `(unicode >> 16) & 0xff`)
 *   - Level 2 (Fash_Int_Map2): `bucket[256]` (indexed by `(unicode >> 8) & 0xff`)
 *     - Level 3 (Fash_Int_Map): `items[256]` (indexed by `unicode & 0xff`)
 *       - Each item contains `Fash_Item_Index_Map item` (for base char)
 *         and `Fash_Item_variation_List *variations` (for variation sequences).
 *
 * This function initializes the structure with zeros, sets a magic number
 * for identification, and assigns the _fash_int_free function as its
 * deallocator.
 */
static Fash_Int *
_fash_int_new(void)
{
   Fash_Int *fash = calloc(1, sizeof(Fash_Int));
   EINA_SAFETY_ON_NULL_RETURN_VAL(fash, NULL);
   fash->MAGIC = FASH_INT_MAGIC;
   fash->freeme = _fash_int_free;
   return fash;
}

/**
 * @brief Allocates and initializes a new Fash_Item_variation_List structure.
 * @return A pointer to the newly allocated Fash_Item_variation_List, or NULL on failure.
 *
 * This list stores mappings for font character variations.
 * It contains a dynamically sized array (`list`) of `Fash_Item_variation_Index_Item`.
 * Each `Fash_Item_variation_Index_Item` stores:
 *  - `item`: A `Fash_Item_Index_Map` (containing `fint` and `index`).
 *  - `variation_sequence`: The Unicode variation selector.
 * The list is kept sorted by `variation_sequence`.
 */
static Fash_Item_variation_List *
_variations_list_new(void)
{
   Fash_Item_variation_List *variations = calloc(1, sizeof(Fash_Item_variation_List));
   EINA_SAFETY_ON_NULL_RETURN_VAL(variations, NULL);
   variations->capacity = 0;
   variations->length = 0;
   variations->list = 0;
   return variations;
}

/**
 * @brief Adds or updates an item in a Fash_Item_variation_List, keeping it sorted.
 * @param variations The variation list to add to.
 * @param fint The font instance (RGBA_Font_Int) for the character variation.
 * @param index The glyph index for the character variation.
 * @param variation_sequence The Unicode variation selector.
 *
 * This function adds a new entry or updates an existing one for the given
 * `variation_sequence`. The list is maintained in ascending order of
 * `variation_sequence`. If the capacity of the list is reached, it is
 * reallocated. The insertion uses a binary search (`lower_bound` like logic)
 * to find the correct position, then shifts elements if necessary to insert
 * the new item, or updates the item if the `variation_sequence` already exists.
 *
 * Example of `variations->list` structure after adding items:
 * variations->list = [
 *   { item: {fint: ptr1, index: idx1}, variation_sequence: 0xFE00 },
 *   { item: {fint: ptr2, index: idx2}, variation_sequence: 0xFE01 },
 *   ...
 * ]
 */
static void
_variations_list_add(Fash_Item_variation_List *variations,RGBA_Font_Int *fint, int index, Eina_Unicode variation_sequence)
{
   Fash_Item_variation_Index_Item *list = variations->list;
   if (variations->capacity == variations->length)
     {
        list = (Fash_Item_variation_Index_Item *) realloc(list, (variations->capacity + 4) * sizeof(Fash_Item_variation_Index_Item));
        if (list)
          {
             variations->list = list;
             variations->capacity += 4;
          }
     }

   EINA_SAFETY_ON_NULL_RETURN(list);

   int start = 0;
   int end = variations->length;
   if (end == 0)
     {
        // if only on element just add it in 0 index
        variations->list[0].item.fint = fint;
        variations->list[0].item.index = index;
        variations->list[0].variation_sequence = variation_sequence;
        variations->length++;
     }
   else
     {
        // find lower bound
        while (end > start)
          {
             int middle = start + (end - start) / 2;
             if (variations->list[middle].variation_sequence >= variation_sequence)
               end = middle;
             else
               start = middle + 1;
          }

        // if passed value founded in list, just replace it
        if (start < (int)variations->length && variations->list[start].variation_sequence == variation_sequence)
          {
             variations->list[start].item.fint = fint;
             variations->list[start].item.index = index;
             variations->list[start].variation_sequence = variation_sequence;
             return;
          }

        // shift array to insert item
        for (int i = (variations->length - 1) ; i >= start; i--)
          {
             variations->list[i + 1] = variations->list[i];
          }

        // insert new item and keep array sorted
        variations->list[start].item.fint = fint;
        variations->list[start].item.index = index;
        variations->list[start].variation_sequence = variation_sequence;
        variations->length++;
     }
}

/**
 * @brief Finds an item in a Fash_Item_variation_List by its variation sequence.
 * @param variations The variation list to search in.
 * @param variation_sequence The Unicode variation selector to find.
 * @return A pointer to the `Fash_Item_Index_Map` if found, NULL otherwise.
 *
 * This function performs a binary search on the `variations->list` (which is
 * sorted by `variation_sequence`) to locate the entry matching the given
 * `variation_sequence`.
 */
static Fash_Item_Index_Map *
_variations_list_find(Fash_Item_variation_List * variations, Eina_Unicode variation_sequence)
{
   if (!variations)
     return NULL;

   if (!variations->list)
     return NULL;

   int start = 0;
   int end = variations->length;

   while(end > start)
     {
        int middle = start + (end - start) / 2;
        if (variations->list[middle].variation_sequence == variation_sequence)
          return &(variations->list[middle].item);
        else if (variations->list[middle].variation_sequence < variation_sequence)
          start = middle + 1;
        else
          end = middle - 1;
     }

   return NULL;
}

/**
 * @brief Finds a font character mapping in the Fash_Int cache.
 * @param fash The Fash_Int cache structure.
 * @param item The Unicode character code.
 * @param variation_sequence The Unicode variation selector (0 if none).
 * @return A pointer to the `Fash_Item_Index_Map` if found, NULL otherwise.
 *
 * This function performs a three-level lookup based on the Unicode `item`:
 * 1. `grp = (item >> 16) & 0xff;`
 * 2. `maj = (item >> 8) & 0xff;`
 * 3. `min = item & 0xff;`
 * If `variation_sequence` is non-zero, it then calls `_variations_list_find`
 * on the found item's variation list. Otherwise, it returns the base item.
 */
static const Fash_Item_Index_Map *
_fash_int_find(Fash_Int *fash, int item, Eina_Unicode variation_sequence)
{
   int grp, maj, min;

   // 24bits for unicode - v6 up to E01EF (chrs) & 10FFFD for private use (plane 16)
   grp = (item >> 16) & 0xff;
   maj = (item >> 8) & 0xff;
   min = item & 0xff;
   if (!fash->bucket[grp]) return NULL;
   if (!fash->bucket[grp]->bucket[maj]) return NULL;
   if (!variation_sequence)
     return &(fash->bucket[grp]->bucket[maj]->items[min].item);
   else
     return _variations_list_find(fash->bucket[grp]->bucket[maj]->items[min].variations, variation_sequence);
}

/**
 * @brief Adds a font character mapping to the Fash_Int cache.
 * @param fash The Fash_Int cache structure.
 * @param item The Unicode character code.
 * @param fint The font instance (RGBA_Font_Int) for the character.
 * @param idx The glyph index for the character.
 * @param variation_sequence The Unicode variation selector (0 if none).
 *
 * This function first checks if the item already exists in the cache using
 * `_fash_int_find`. If it does, the function returns. Otherwise, it performs
 * a three-level lookup/creation:
 * 1. `grp = (item >> 16) & 0xff;`
 * 2. `maj = (item >> 8) & 0xff;`
 * 3. `min = item & 0xff;`
 * Buckets at each level (Fash_Int_Map2, Fash_Int_Map) are allocated if they
 * don't exist.
 * If `variation_sequence` is non-zero, the item is added to the variation list
 * of the corresponding base character. Otherwise, the base character's `fint`
 * and `index` are set.
 */
static void
_fash_int_add(Fash_Int *fash, int item, RGBA_Font_Int *fint, int idx, Eina_Unicode variation_sequence)
{
   int grp, maj, min;

   // If we already have cached passed item, skip adding it again
   const Fash_Item_Index_Map *fm = _fash_int_find(fash, item, variation_sequence);
   if (fm && fm->fint)
     return;

   // 24bits for unicode - v6 up to E01EF (chrs) & 10FFFD for private use (plane 16)
   grp = (item >> 16) & 0xff;
   maj = (item >> 8) & 0xff;
   min = item & 0xff;
   if (!fash->bucket[grp])
     fash->bucket[grp] = calloc(1, sizeof(Fash_Int_Map2));
   EINA_SAFETY_ON_NULL_RETURN(fash->bucket[grp]);
   if (!fash->bucket[grp]->bucket[maj])
     fash->bucket[grp]->bucket[maj] = calloc(1, sizeof(Fash_Int_Map));
   EINA_SAFETY_ON_NULL_RETURN(fash->bucket[grp]->bucket[maj]);
   if (variation_sequence)
     {
         if (!fash->bucket[grp]->bucket[maj]->items[min].variations)
           {
              fash->bucket[grp]->bucket[maj]->items[min].variations =_variations_list_new();
              EINA_SAFETY_ON_NULL_RETURN(fash->bucket[grp]->bucket[maj]->items[min].variations);
           }
         _variations_list_add(fash->bucket[grp]->bucket[maj]->items[min].variations, fint, idx, variation_sequence);
     }
   else
     {
        fash->bucket[grp]->bucket[maj]->items[min].item.fint = fint;
        fash->bucket[grp]->bucket[maj]->items[min].item.index = idx;
     }
}

/**
 * @brief Frees an RGBA_Font_Glyph structure and its associated data.
 * @param fg Pointer to the RGBA_Font_Glyph to be freed.
 *
 * This function handles the deallocation of a font glyph.
 * It checks if the glyph is valid or a placeholder for a non-existent glyph.
 * - If `fg->glyph_out` (rendered output) exists:
 *   - Frees FreeType bitmap data if not RLE compressed and allocated by FT.
 *   - Frees RLE data if present and allocated by Evas.
 *   - Frees bitmap buffer if present and allocated by Evas (e.g., for scaled color glyphs).
 *   - Frees `fg->glyph_out` itself unless `bitmap.no_free_glout` is set.
 * - Calls `FT_Done_Glyph` to free the FreeType glyph object.
 * - Calls `fg->ext_dat_free` if an extension data free function is provided.
 * - Drops image cache data if `fg->col_dat` (color data) exists.
 * - Finally, frees the `RGBA_Font_Glyph` structure itself.
 */
static void
_glyph_free(RGBA_Font_Glyph *fg)
{
   if ((!fg) || (fg == (void *)(-1))) return;

   if (fg->glyph_out)
     {
        if ((!fg->glyph_out->rle) && (!fg->glyph_out->bitmap.rle_alloc))
          {
             FT_BitmapGlyph fbg = (FT_BitmapGlyph)fg->glyph;
             FT_Bitmap_Done(evas_ft_lib, &(fbg->bitmap));
          }

        if ((fg->glyph_out->rle) && (fg->glyph_out->bitmap.rle_alloc))
          free(fg->glyph_out->rle);
        else if ((fg->glyph_out->bitmap.buffer) && (fg->glyph_out->bitmap.rle_alloc))
          {
             free(fg->glyph_out->bitmap.buffer);
             fg->glyph_out->bitmap.buffer = NULL;
          }
        fg->glyph_out->rle = NULL;
        if (!fg->glyph_out->bitmap.no_free_glout) free(fg->glyph_out);
        fg->glyph_out = NULL;
     }
   FT_Done_Glyph(fg->glyph);
   /* extension calls */
   if (fg->ext_dat_free) fg->ext_dat_free(fg->ext_dat);
   if (fg->col_dat) evas_cache_image_drop(fg->col_dat);
   free(fg);
}

/**
 * @brief Frees a Fash_Glyph_Map structure.
 * @param fmap Pointer to the Fash_Glyph_Map to be freed.
 *
 * This function iterates through all items in the `fmap->item` array.
 * For each valid (non-NULL and not placeholder) RGBA_Font_Glyph,
 * it calls `_glyph_free` to deallocate it. Finally, it frees the
 * Fash_Glyph_Map structure itself.
 */
static void
_fash_glyph_free(Fash_Glyph_Map *fmap)
{
   int i;

   for (i = 0; i <= 0xff; i++)
     {
        RGBA_Font_Glyph *fg = fmap->item[i];
        if ((fg) && (fg != (void *)(-1)))
          {
             _glyph_free(fg);
             fmap->item[i] = NULL;
          }
     }
  free(fmap);
}

/**
 * @brief Frees a Fash_Glyph_Map2 structure.
 * @param fash Pointer to the Fash_Glyph_Map2 to be freed.
 *
 * This function iterates through all buckets in the Fash_Glyph_Map2.
 * For each non-NULL bucket (which is a Fash_Glyph_Map), it calls
 * `_fash_glyph_free` to free it. Finally, it frees the
 * Fash_Glyph_Map2 structure itself.
 */
static void
_fash_gl2_free(Fash_Glyph_Map2 *fash)
{
   int i;

   // 24bits for unicode - v6 up to E01EF (chrs) & 10FFFD for private use (plane 16)
   for (i = 0; i < 256; i++)
     {
        if (fash->bucket[i])
          {
             _fash_glyph_free(fash->bucket[i]);
             fash->bucket[i] = NULL;
          }
     }
   free(fash);
}

/**
 * @brief Frees a Fash_Glyph structure.
 * @param fash Pointer to the Fash_Glyph to be freed.
 *
 * This function is the main deallocator for Fash_Glyph structures,
 * typically assigned to `fash->freeme`. It checks for a magic number
 * to ensure it's a valid Fash_Glyph. It then iterates through its
 * buckets, calling `_fash_gl2_free` for each non-NULL bucket
 * (which is a Fash_Glyph_Map2). Finally, it frees the Fash_Glyph
 * structure itself.
 */
static void
_fash_gl_free(Fash_Glyph *fash)
{
   if (fash)
     {
        if (fash->MAGIC != FASH_GLYPH_MAGIC)
          return;

        int i;
          // 24bits for unicode - v6 up to E01EF (chrs) & 10FFFD for private use (plane 16)
        for (i = 0; i < 256; i++)
          {
            if (fash->bucket[i])
              {
                 _fash_gl2_free(fash->bucket[i]);
                 fash->bucket[i] = NULL;
              }
          }
         free(fash);
     }
}

/**
 * @brief Allocates and initializes a new Fash_Glyph structure.
 * @return A pointer to the newly allocated Fash_Glyph, or NULL on failure.
 *
 * Fash_Glyph is a three-level hash-like structure used for caching
 * glyph index to RGBA_Font_Glyph mappings.
 * - Level 1 (Fash_Glyph): `bucket[256]` (indexed by `(glyph_index >> 16) & 0xff`)
 *   - Level 2 (Fash_Glyph_Map2): `bucket[256]` (indexed by `(glyph_index >> 8) & 0xff`)
 *     - Level 3 (Fash_Glyph_Map): `item[256]` (indexed by `glyph_index & 0xff`)
 *       - Each item is a pointer to an `RGBA_Font_Glyph`.
 *
 * This function initializes the structure with zeros, sets a magic number
 * for identification, and assigns the `_fash_gl_free` function as its
 * deallocator.
 */
static Fash_Glyph *
_fash_gl_new(void)
{
   Fash_Glyph *fash = calloc(1, sizeof(Fash_Glyph));
   EINA_SAFETY_ON_NULL_RETURN_VAL(fash, NULL);
   fash->MAGIC = FASH_GLYPH_MAGIC;
   fash->freeme = _fash_gl_free;
   return fash;
}

/**
 * @brief Finds a glyph in the Fash_Glyph cache by its index.
 * @param fash The Fash_Glyph cache structure.
 * @param item The glyph index.
 * @return A pointer to the `RGBA_Font_Glyph` if found, NULL otherwise.
 *         Can also return `(void *)(-1)` if the glyph is known to be non-existent.
 *
 * This function performs a three-level lookup based on the glyph `item` (index):
 * 1. `grp = (item >> 16) & 0xff;`
 * 2. `maj = (item >> 8) & 0xff;`
 * 3. `min = item & 0xff;`
 * It returns the `RGBA_Font_Glyph` pointer stored at the found location.
 */
static RGBA_Font_Glyph *
_fash_gl_find(Fash_Glyph *fash, int item)
{
   int grp, maj, min;

   // 24bits for unicode - v6 up to E01EF (chrs) & 10FFFD for private use (plane 16)
   grp = (item >> 16) & 0xff;
   maj = (item >> 8) & 0xff;
   min = item & 0xff;
   if (!fash->bucket[grp]) return NULL;
   if (!fash->bucket[grp]->bucket[maj]) return NULL;
   return fash->bucket[grp]->bucket[maj]->item[min];
}

/**
 * @brief Adds a glyph to the Fash_Glyph cache.
 * @param fash The Fash_Glyph cache structure.
 * @param item The glyph index.
 * @param glyph Pointer to the `RGBA_Font_Glyph` to add.
 *              Can be `(void *)(-1)` to mark a glyph as non-existent.
 *
 * This function performs a three-level lookup/creation based on the glyph `item` (index):
 * 1. `grp = (item >> 16) & 0xff;`
 * 2. `maj = (item >> 8) & 0xff;`
 * 3. `min = item & 0xff;`
 * Buckets at each level (Fash_Glyph_Map2, Fash_Glyph_Map) are allocated if they
 * don't exist. The `glyph` pointer is then stored at the determined location.
 */
static void
_fash_gl_add(Fash_Glyph *fash, int item, RGBA_Font_Glyph *glyph)
{
   int grp, maj, min;

   // 24bits for unicode - v6 up to E01EF (chrs) & 10FFFD for private use (plane 16)
   grp = (item >> 16) & 0xff;
   maj = (item >> 8) & 0xff;
   min = item & 0xff;
   if (!fash->bucket[grp])
     fash->bucket[grp] = calloc(1, sizeof(Fash_Glyph_Map2));
   EINA_SAFETY_ON_NULL_RETURN(fash->bucket[grp]);
   if (!fash->bucket[grp]->bucket[maj])
     fash->bucket[grp]->bucket[maj] = calloc(1, sizeof(Fash_Glyph_Map));
   EINA_SAFETY_ON_NULL_RETURN(fash->bucket[grp]->bucket[maj]);
   fash->bucket[grp]->bucket[maj]->item[min] = glyph;
}

/**
 * @brief Loads the FreeType glyph object for an RGBA_Font_Glyph if not already loaded.
 * @param fg The RGBA_Font_Glyph for which to load the FT_Glyph.
 *
 * This function checks if `fg->glyph` (the FT_Glyph) is already populated.
 * If not, it reloads the parent font instance (`fi`), then loads the glyph
 * from the FreeType face using `FT_Load_Glyph`. It applies hinting flags
 * and handles color fonts appropriately. If runtime rendering options like
 * slant or embolden are set for the font instance, it applies these
 * transformations to the glyph outline. Finally, it calls `FT_Get_Glyph`
 * to store the loaded FT_Glyph object in `fg->glyph`.
 */
static void evas_font_glyph_load(RGBA_Font_Glyph *fg)
{
   if(fg->glyph) return;

   RGBA_Font_Int *fi = fg->fi;
   FT_UInt idx = fg->index;
   FT_Error error;

   const FT_Int32 hintflags[3] =
     { FT_LOAD_NO_HINTING, FT_LOAD_FORCE_AUTOHINT, FT_LOAD_NO_AUTOHINT };
   static FT_Matrix transform = {0x10000, _EVAS_FONT_SLANT_TAN * 0x10000,
        0x00000, 0x10000};

   evas_common_font_int_reload(fi);
   FTLOCK();
   error = FT_Load_Glyph(fi->src->ft.face, idx,
                         (FT_HAS_COLOR(fi->src->ft.face) ?
                          (FT_LOAD_COLOR | hintflags[fi->hinting]) :
                          (FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP | hintflags[fi->hinting])));

   FTUNLOCK();
   if (error)
     {
        return;
     }

   /* Transform the outline of Glyph according to runtime_rend. */
   if (fi->runtime_rend & FONT_REND_SLANT)
     FT_Outline_Transform(&fi->src->ft.face->glyph->outline, &transform);
   /* Embolden the outline of Glyph according to rundtime_rend. */
   if (fi->runtime_rend & FONT_REND_WEIGHT)
     FT_GlyphSlot_Embolden(fi->src->ft.face->glyph);

   FTLOCK();
   error = FT_Get_Glyph(fi->src->ft.face->glyph, &(fg->glyph));
   FTUNLOCK();

   return;
}

/**
 * @brief Retrieves a glyph from the font instance cache, loading it if necessary.
 * @param fi The font instance (RGBA_Font_Int).
 * @param idx The glyph index.
 * @return Pointer to the RGBA_Font_Glyph, or NULL on failure.
 *         Returns (void *)(-1) if the glyph is known to be non-existent.
 *
 * This function first attempts to find the glyph in the `fi->fash` cache.
 * If not found, it loads the glyph from the FreeType face:
 * - Reloads the font source if needed.
 * - Calls `FT_Load_Glyph` with appropriate flags (handling color, hinting).
 * - Applies runtime transformations (slant, embolden) if specified.
 * - Allocates an `RGBA_Font_Glyph` structure.
 * - For color fonts:
 *   - Sets advance, width, and bearing metrics directly from the slot.
 *   - Scales metrics if the font is a scalable color bitmap font.
 * - For non-color fonts:
 *   - Calls `FT_Get_Glyph` to obtain the FT_Glyph object.
 *   - Sets advance from `fg->glyph->advance`.
 *   - Calculates width and bearings using `FT_Glyph_Get_CBox`.
 * - Stores the glyph index and font instance pointer in the `RGBA_Font_Glyph`.
 * - Adds the new `RGBA_Font_Glyph` to the `fi->fash` cache.
 * If loading fails at any critical step, it marks the glyph as non-existent
 * `(void *)(-1)` in the cache and returns NULL.
 */
EVAS_API RGBA_Font_Glyph *
evas_common_font_int_cache_glyph_get(RGBA_Font_Int *fi, FT_UInt idx)
{
   RGBA_Font_Glyph *fg;
   FT_Error error;
   const FT_Int32 hintflags[3] =
     { FT_LOAD_NO_HINTING, FT_LOAD_FORCE_AUTOHINT, FT_LOAD_NO_AUTOHINT };
   static FT_Matrix transform = {0x10000, _EVAS_FONT_SLANT_TAN * 0x10000,
        0x00000, 0x10000};

   evas_common_font_int_promote(fi);
   if (fi->fash)
     {
        fg = _fash_gl_find(fi->fash, idx);
        if (fg == (void *)(-1)) return NULL;
        else if (fg)
          return fg;
     }
//   fg = eina_hash_find(fi->glyphs, &hindex);
//   if (fg) return fg;

   evas_common_font_int_reload(fi);
   FTLOCK();
   error = FT_Load_Glyph(fi->src->ft.face, idx,
                         (FT_HAS_COLOR(fi->src->ft.face) ?
                          (FT_LOAD_COLOR | hintflags[fi->hinting]) :
                          (FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP | hintflags[fi->hinting])));

   FTUNLOCK();
   if (error)
     {
        if (!fi->fash) fi->fash = _fash_gl_new();
        if (fi->fash) _fash_gl_add(fi->fash, idx, (void *)(-1));
        return NULL;
     }

   /* Transform the outline of Glyph according to runtime_rend. */
   if (fi->runtime_rend & FONT_REND_SLANT)
     FT_Outline_Transform(&fi->src->ft.face->glyph->outline, &transform);
   /* Embolden the outline of Glyph according to rundtime_rend. */
   if (fi->runtime_rend & FONT_REND_WEIGHT)
     FT_GlyphSlot_Embolden(fi->src->ft.face->glyph);

   fg = calloc(1, sizeof(RGBA_Font_Glyph));
   if (!fg) return NULL;

   if (FT_HAS_COLOR(fi->src->ft.face))
     {
        fg->advance.x = fi->src->ft.face->glyph->advance.x * 1024;
        fg->advance.y = fi->src->ft.face->glyph->advance.y * 1024;

        FT_GlyphSlot slot = fi->src->ft.face->glyph;
        fg->width = EVAS_FONT_ROUND_26_6_TO_INT(slot->metrics.width);
        fg->x_bear = EVAS_FONT_ROUND_26_6_TO_INT(slot->metrics.horiBearingX);
        fg->y_bear = EVAS_FONT_ROUND_26_6_TO_INT(slot->metrics.horiBearingY);

        if (FT_HAS_FIXED_SIZES(fi->src->ft.face))
          {
             if (fi->bitmap_scalable & EFL_TEXT_FONT_BITMAP_SCALABLE_COLOR)
               {
                  fg->advance.x *= fi->scale_factor;
                  fg->advance.y *= fi->scale_factor;
                  fg->width *= fi->scale_factor;
                  fg->x_bear *= fi->scale_factor;
                  fg->y_bear *= fi->scale_factor;
               }
          }
     }
   else
     {
        FTLOCK();
        error = FT_Get_Glyph(fi->src->ft.face->glyph, &(fg->glyph));
        FTUNLOCK();
        if (error)
          {
             free(fg);
             if (!fi->fash) fi->fash = _fash_gl_new();
             if (fi->fash) _fash_gl_add(fi->fash, idx, (void *)(-1));
             return NULL;
          }
        fg->advance.x = fg->glyph->advance.x;
        fg->advance.y = fg->glyph->advance.y;

        FT_BBox outbox;
        FT_Glyph_Get_CBox(fg->glyph,
              ((fi->hinting == 0) ? FT_GLYPH_BBOX_UNSCALED :
              FT_GLYPH_BBOX_GRIDFIT),
              &outbox);
        fg->width = EVAS_FONT_ROUND_26_6_TO_INT(outbox.xMax - outbox.xMin);
        fg->x_bear = EVAS_FONT_ROUND_26_6_TO_INT(outbox.xMin);
        fg->y_bear = EVAS_FONT_ROUND_26_6_TO_INT(outbox.yMax);
     }

   fg->index = idx;
   fg->fi = fi;

   if (!fi->fash) fi->fash = _fash_gl_new();
   if (fi->fash) _fash_gl_add(fi->fash, idx, fg);

//   eina_hash_direct_add(fi->glyphs, &fg->index, fg);
   return fg;
}

/**
 * @brief Sets the size for a specific Evas font data cache.
 * @param options A bitmask indicating which cache to configure.
 *                Currently, only EVAS_FONT_DATA_CACHE_TEXTURE is supported.
 * @param bytes The desired cache size in bytes.
 *
 * This function allows configuring the size of internal font data caches.
 * For `EVAS_FONT_DATA_CACHE_TEXTURE`, it sets `_evas_font_texture_cache`.
 * Note: Changes to the texture cache size might not take effect immediately
 * and may require a subsequent render call to free resources.
 */
EVAS_API void
evas_font_data_cache_set(Evas_Font_Data_Cache options, int bytes)
{
   if ((options & EVAS_FONT_DATA_CACHE_TEXTURE) == EVAS_FONT_DATA_CACHE_TEXTURE)
     {
        _evas_font_texture_cache = bytes;
        //FIXME No direct free happend until next render call
     }
}

/**
 * @brief Gets the current size of a specific Evas font data cache.
 * @param options A bitmask indicating which cache to query.
 *                Currently, only EVAS_FONT_DATA_CACHE_TEXTURE is supported.
 * @return The current cache size in bytes for the specified cache, or -1 if
 *         the option is not supported or not set.
 */
EVAS_API int
evas_font_data_cache_get(Evas_Font_Data_Cache options)
{
   if ((options & EVAS_FONT_DATA_CACHE_TEXTURE) == EVAS_FONT_DATA_CACHE_TEXTURE)
     return _evas_font_texture_cache;
   else
     return -1;
}

/**
 * @brief Renders a glyph to a bitmap if not already rendered.
 * @param fg The RGBA_Font_Glyph to render.
 * @return EINA_TRUE on success or if already rendered, EINA_FALSE on failure.
 *
 * This function handles the rendering of a glyph into a bitmap format suitable
 * for Evas.
 * - If `fg->glyph_out` (rendered output) already exists, it returns EINA_TRUE.
 * - Ensures the FreeType glyph object is loaded via `evas_font_glyph_load`.
 * - Calls `FT_Glyph_To_Bitmap` to render the glyph. If this fails, the glyph
 *   is marked as non-existent in the cache, `fg` is freed, and EINA_FALSE is returned.
 * - Allocates `fg->glyph_out` to store rendering information.
 * - Copies bitmap metadata (rows, width, pitch, buffer) from FreeType's
 *   `FT_BitmapGlyph` to `fg->glyph_out->bitmap`.
 * - Updates font instance usage statistics.
 * - For non-color fonts:
 *   - Compresses the bitmap buffer into RLE format using
 *     `evas_common_font_glyph_compress`. The RLE data is stored in
 *     `fg->glyph_out->rle`.
 *   - The original bitmap buffer in `fg->glyph_out->bitmap.buffer` is set to NULL.
 *   - `FT_Bitmap_Done` is called to free FreeType's copy of the bitmap.
 * - For color fonts:
 *   - `fg->glyph_out->rle` is set to NULL.
 *   - If the font instance `fi->is_resized` (meaning the color bitmap font is
 *     being scaled):
 *     - An RGBA_Image is created from FreeType's bitmap buffer.
 *     - A destination RGBA_Image is allocated with scaled dimensions.
 *     - `evas_common_scale_rgba_smooth_draw` is used to scale the image.
 *     - The scaled bitmap data is stored in `fg->glyph_out->bitmap`.
 *     - `FT_Bitmap_Done` is called to free FreeType's original bitmap.
 * Returns EINA_TRUE on successful rendering or if already rendered.
 */
EVAS_API Eina_Bool
evas_common_font_int_cache_glyph_render(RGBA_Font_Glyph *fg)
{
   int size;
   FT_Error error;
   RGBA_Font_Int *fi = fg->fi;
   FT_BitmapGlyph fbg;

   /* no cserve2 case */
   if (fg->glyph_out)
     return EINA_TRUE;
   evas_font_glyph_load(fg);
   FTLOCK();
   error = FT_Glyph_To_Bitmap(&(fg->glyph), FT_RENDER_MODE_NORMAL, 0, 1);
   if (error)
     {
        FT_Done_Glyph(fg->glyph);
        FTUNLOCK();
        if (!fi->fash) fi->fash = _fash_gl_new();
        if (fi->fash) _fash_gl_add(fi->fash, fg->index, (void *)(-1));
        free(fg);
        return EINA_FALSE;
     }
   FTUNLOCK();

   fbg = (FT_BitmapGlyph)fg->glyph;

   fg->glyph_out = calloc(1, sizeof(RGBA_Font_Glyph_Out));
   fg->glyph_out->bitmap.rows = fbg->bitmap.rows;
   fg->glyph_out->bitmap.width = fbg->bitmap.width;
   fg->glyph_out->bitmap.pitch = fbg->bitmap.pitch;
   fg->glyph_out->bitmap.buffer = fbg->bitmap.buffer;
   fg->glyph_out->bitmap.rle_alloc = EINA_TRUE;

   /* This '+ 100' is just an estimation of how much memory freetype will use
    * on it's size. This value is not really used anywhere in code - it's
    * only for statistics. */
   size = sizeof(RGBA_Font_Glyph) + sizeof(Eina_List) +
    (fg->glyph_out->bitmap.width * fg->glyph_out->bitmap.rows / 2) + 100;
   fi->usage += size;
   if (fi->inuse) evas_common_font_int_use_increase(size);

   if (!FT_HAS_COLOR(fi->src->ft.face))
     {
        fg->glyph_out->rle = evas_common_font_glyph_compress
           (fbg->bitmap.buffer, fbg->bitmap.num_grays, fbg->bitmap.pixel_mode,
            fbg->bitmap.pitch, fbg->bitmap.width, fbg->bitmap.rows,
            &(fg->glyph_out->rle_size));
        fg->glyph_out->bitmap.rle_alloc = EINA_TRUE;

        fg->glyph_out->bitmap.buffer = NULL;

        // this may be technically incorrect as we go and free a bitmap buffer
        // behind the ftglyph's back...
        FT_Bitmap_Done(evas_ft_lib, &(fbg->bitmap));
     }
   else
     {
        fg->glyph_out->rle = NULL;
        fg->glyph_out->bitmap.rle_alloc = EINA_FALSE;
        if (fi->is_resized)
          {
             int w = fbg->bitmap.width;
             int h = fbg->bitmap.rows;

             RGBA_Image src = {0};
             src.image.data = (DATA32 *) fbg->bitmap.buffer;
             src.cache_entry.w = w;
             src.cache_entry.h = h;
             src.cache_entry.flags.alpha = 1;

             RGBA_Image dst = {0};
             dst.cache_entry.w = w * fi->scale_factor;
             dst.cache_entry.h = h * fi->scale_factor;
             dst.image.data = malloc(dst.cache_entry.w * dst.cache_entry.h * 4);
             dst.cache_entry.flags.alpha = 1;

             evas_common_scale_rgba_smooth_draw(&src, &dst,
                                        0, 0, src.cache_entry.w , src.cache_entry.h,
                                        0xffffffff, EVAS_RENDER_COPY,
                                        0, 0, src.cache_entry.w , src.cache_entry.h,
                                        0, 0, dst.cache_entry.w, dst.cache_entry.h,
                                        NULL, 0, 0);

             fg->glyph_out->bitmap.rows = dst.cache_entry.h;
             fg->glyph_out->bitmap.width = dst.cache_entry.w;
             fg->glyph_out->bitmap.buffer = (unsigned char *) dst.image.data;
             fg->glyph_out->bitmap.pitch = dst.cache_entry.w * 4;

             fg->glyph_out->rle = NULL;
             fg->glyph_out->bitmap.rle_alloc = EINA_TRUE;
             // this may be technically incorrect as we go and free a bitmap buffer
             // behind the ftglyph's back...
             FT_Bitmap_Done(evas_ft_lib, &(fbg->bitmap));
          }
     }

   return EINA_TRUE;
}

typedef struct _Font_Char_Index Font_Char_Index;
struct _Font_Char_Index
{
   FT_UInt index;
   Eina_Unicode gl;
};

/**
 * @brief Gets the FreeType glyph index for a given Unicode character and variation sequence.
 * @param fi The font instance (RGBA_Font_Int).
 * @param gl The Unicode character code.
 * @param variation_sequence The Unicode variation selector (e.g., 0xFE00 for text style).
 *                           Pass 0 if no variation sequence is used.
 * @return The FreeType glyph index (FT_UInt). Returns 0 if the character is not found.
 *
 * This function retrieves the glyph index for a character from the specified font instance.
 * It reloads the font instance if necessary.
 * It calls `FT_Get_Char_Index` or `FT_Face_GetCharVariantIndex` (if `variation_sequence` is provided)
 * to get the index.
 *
 * A workaround is included for old-style bitmap fonts that may not correctly map
 * terminal line-drawing characters via their Unicode CMAP. If the initial lookup
 * fails (index <= 0) and the font appears to be a simple fixed-size bitmap font,
 * it attempts to remap the character `gl` using a static table `mapfix` and
 * retries the FreeType lookup with the remapped character.
 * The `mapfix` table contains pairs of {original_unicode, target_codepoint_in_font}.
 * For example: `{0x00b0 ( डिग्री सेल्सियस ), 0x7 ( BEL )}`. This implies that if the font
 * doesn't find U+00B0, it might find it at codepoint 0x07 (often used in legacy fonts).
 */
EVAS_API FT_UInt
evas_common_get_char_index(RGBA_Font_Int* fi, Eina_Unicode gl, Eina_Unicode variation_sequence)
{
   static const unsigned short mapfix[] =
     {
        0x00b0, 0x7,
        0x00b1, 0x8,
        0x00b7, 0x1f,
        0x03c0, 0x1c,
        0x20a4, 0xa3,
        0x2260, 0x1d,
        0x2264, 0x1a,
        0x2265, 0x1b,
        0x23ba, 0x10,
        0x23bb, 0x11,
        0x23bc, 0x13,
        0x23bd, 0x14,
        0x2409, 0x3,
        0x240a, 0x6,
        0x240b, 0xa,
        0x240c, 0x4,
        0x240d, 0x5,
        0x2424, 0x9,
        0x2500, 0x12,
        0x2502, 0x19,
        0x250c, 0xd,
        0x2510, 0xc,
        0x2514, 0xe,
        0x2518, 0xb,
        0x251c, 0x15,
        0x2524, 0x16,
        0x252c, 0x18,
        0x2534, 0x17,
        0x253c, 0xf,
        0x2592, 0x2,
        0x25c6, 0x1,
     };
   Font_Char_Index result;
   //FT_UInt ret;

#ifdef HAVE_PTHREAD
///   pthread_mutex_lock(&fi->ft_mutex);
#endif

//   result = eina_hash_find(fi->indexes, &gl);
//   if (result) goto on_correct;
//
//   result = malloc(sizeof (Font_Char_Index));
//   if (!result)
//     {
//#ifdef HAVE_PTHREAD
//	pthread_mutex_unlock(&fi->ft_mutex);
//#endif
//	return FT_Get_Char_Index(fi->src->ft.face, gl);
//     }

   evas_common_font_int_reload(fi);
   /*
    * There is no point in locking FreeType at this point as all caller
    * are running in the main loop at a time where there is zero chance
    * that something else try to use it.
    */
   /* FTLOCK(); */
   if (variation_sequence)
     result.index = FT_Face_GetCharVariantIndex(fi->src->ft.face, gl, variation_sequence);
   else
     result.index = FT_Get_Char_Index(fi->src->ft.face, gl);
   /* FTUNLOCK(); */
   result.gl = gl;

//   eina_hash_direct_add(fi->indexes, &result->gl, result);
//
// on_correct:
#ifdef HAVE_PTHREAD
//   pthread_mutex_unlock(&fi->ft_mutex);
#endif
   // this is a workaround freetype bugs where for a bitmap old style font
   // even if it has unicode information and mappings, they are not used
   // to find terminal line/drawing chars, so do this by hand with a table
   if ((result.index <= 0) && (fi->src->ft.face->num_fixed_sizes == 1) &&
      (fi->src->ft.face->num_glyphs < 512))
     {
        int i, min = 0, max;

        // binary search through sorted table of codepoints to new
        // codepoints with a guess that bitmap font is playing the old
        // game of putting line drawing chars in specific ranges
        max = sizeof(mapfix) / (sizeof(mapfix[0]) * 2);
        i = (min + max) / 2;
        for (;;)
          {
             unsigned short v;

             v = mapfix[i << 1];
             if (gl == v)
               {
                  gl = mapfix[(i << 1) + 1];
                  FTLOCK();
                  if (variation_sequence)
                    result.index = FT_Face_GetCharVariantIndex(fi->src->ft.face, gl, variation_sequence);
                  else
                    result.index = FT_Get_Char_Index(fi->src->ft.face, gl);
                  FTUNLOCK();
                  break;
               }
             // failure to find at all
             if ((max - min) <= 2) break;
             // if glyph above out position...
             if (gl > v)
               {
                  min = i;
                  if ((max - min) == 1) i = max;
                  else i = (min + max) / 2;
               }
             // if glyph below out position
             else if (gl < v)
               {
                  max = i;
                  if ((max - min) == 1) i = min;
                  else i = (min + max) / 2;
               }
          }
     }
   return result.index;
}


/*
 * @internal
 * Search for unicode glyph inside all font files, and return font and glyph index
 *
 * @param[in] fn the font to use.
 * @param[out] fi_ret founded font.
 * @param[in] gl unicode glyph to search for
 * @param[in] variation_sequence for the gl glyph
 * @param[in] evas_font_search_options Search options.
 *        - `EVAS_FONT_SEARCH_OPTION_NONE`: No special options.
 *        - `EVAS_FONT_SEARCH_OPTION_SKIP_COLOR`: Skip color fonts during search.
 * @return The glyph index if found, 0 otherwise. `fi_ret` is updated with the
 *         font instance containing the glyph.
 *
 * This function searches for a glyph corresponding to the Unicode character `gl`
 * (and `variation_sequence`) within the list of font instances (`fn->fonts`)
 * associated with the `RGBA_Font` `fn`.
 *
 * It first checks the `fn->fash` cache (a Fash_Int structure).
 * - If found in cache:
 *   - If `evas_font_search_options` is `EVAS_FONT_SEARCH_OPTION_NONE`, it returns the cached info.
 *   - If `EVAS_FONT_SEARCH_OPTION_SKIP_COLOR` is set, it checks if the cached font is non-color.
 *     If it's a color font, the cache hit is ignored, and the search continues.
 * - If not found in cache or skipped due to options:
 *   - It iterates through each `RGBA_Font_Int` in `fn->fonts`.
 *   - For each `RGBA_Font_Int`:
 *     - Reloads the font source if not already loaded (`fi->src->ft.face`).
 *     - If `EVAS_FONT_SEARCH_OPTION_SKIP_COLOR` is set and the current font is a color font,
 *       it skips this font instance.
 *     - Calls `evas_common_get_char_index` to get the glyph index.
 *     - If a valid index (>0) is found:
 *       - Ensures the font instance's size information is loaded.
 *       - If not skipping color fonts (or if it's not a color font), adds the mapping
 *         to `fn->fash` for future lookups.
 *       - Sets `*fi_ret` to the current `RGBA_Font_Int` and returns the index.
 *     - If the index is 0 (not found in this font instance):
 *       - If not skipping color fonts, adds a "not found" entry (NULL fint, index -1)
 *         to `fn->fash` to speed up future misses for this character in this `RGBA_Font`.
 * If the glyph is not found in any font instance, `*fi_ret` is set to NULL, and 0 is returned.
 */
EVAS_API int
evas_common_font_glyph_search(RGBA_Font *fn, RGBA_Font_Int **fi_ret, Eina_Unicode gl, Eina_Unicode variation_sequence, uint32_t evas_font_search_options)
{
   Eina_List *l;

   if (fn->fash)
     {
        const Fash_Item_Index_Map *fm = _fash_int_find(fn->fash, gl, variation_sequence);
        if (fm)
          {
             if (fm->fint)
               {
                  if (evas_font_search_options == EVAS_FONT_SEARCH_OPTION_NONE)
                    {
                        *fi_ret = fm->fint;
                        return fm->index;
                    }
                  else if( (evas_font_search_options & EVAS_FONT_SEARCH_OPTION_SKIP_COLOR) == EVAS_FONT_SEARCH_OPTION_SKIP_COLOR)
                    {
                       if (!fm->fint->src->ft.face)
                         {
                            evas_common_font_int_reload(fm->fint);
                         }

                       if (fm->fint->src->ft.face && !FT_HAS_COLOR(fm->fint->src->ft.face))
                         {
                            *fi_ret = fm->fint;
                            return fm->index;
                         }
                    }
               }
             else if (fm->index == -1) return 0;
          }
     }

   for (l = fn->fonts; l; l = l->next)
     {
        RGBA_Font_Int *fi;
        int idx;

        fi = l->data;

#if 0 /* FIXME: charmap user is disabled and use a deprecated data type. */
/*
	if (fi->src->charmap) // Charmap loaded, FI/FS blank
	  {
	     idx = evas_array_hash_search(fi->src->charmap, gl);
	     if (idx != 0)
	       {
		  evas_common_font_source_load_complete(fi->src);
		  evas_common_font_int_load_complete(fi);

		  evas_array_hash_free(fi->src->charmap);
		  fi->src->charmap = NULL;

		  *fi_ret = fi;
		  return idx;
               }
           }
        else
*/
#endif
        if (!fi->src->ft.face) /* Charmap not loaded, FI/FS blank */
          {
             evas_common_font_int_reload(fi);
          }
        if (fi->src->ft.face)
          {
             Eina_Bool is_color_only = (evas_font_search_options & EVAS_FONT_SEARCH_OPTION_SKIP_COLOR) == EVAS_FONT_SEARCH_OPTION_SKIP_COLOR &&
                 FT_HAS_COLOR(fi->src->ft.face);

             if (is_color_only)
               {
                  /* This is color font ignore it */
                  continue;
               }

             idx = (int) evas_common_get_char_index(fi, gl, variation_sequence);
             if (idx != 0)
               {
                  if (!fi->ft.size)
                    evas_common_font_int_load_complete(fi);
                  if (!is_color_only)
                    {
                       if (!fn->fash) fn->fash = _fash_int_new();
                       if (fn->fash) _fash_int_add(fn->fash, gl, fi, idx, variation_sequence);
                    }
                  *fi_ret = fi;
                  return idx;
               }
             else
               {
                  if (!is_color_only)
                    {
                        if (!fn->fash) fn->fash = _fash_int_new();
                        if (fn->fash) _fash_int_add(fn->fash, gl, NULL, -1, variation_sequence);
                    }
               }
          }
     }
   *fi_ret = NULL;
   return 0;
}
