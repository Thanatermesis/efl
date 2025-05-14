/**
 * @file
 * @brief This file contains declarations for OpenType (OT) font handling in Evas.
 * It includes structures and functions for managing OT font information,
 * such as cluster details and populating text properties using HarfBuzz.
 */
#ifndef _EVAS_FONT_OT_H
# define _EVAS_FONT_OT_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

# ifdef HAVE_HARFBUZZ
#  define OT_SUPPORT
#  define USE_HARFBUZZ
# endif

# ifdef OT_SUPPORT
#  include <stdlib.h>
/**
 * @struct _Evas_Font_OT_Info
 * @brief Holds OpenType specific information for a glyph.
 *
 * This structure stores details about a glyph that are relevant for
 * OpenType layout, such as its original cluster in the source text
 * and rendering offsets.
 */
typedef struct _Evas_Font_OT_Info Evas_Font_OT_Info;
# else
/**
 * @typedef Evas_Font_OT_Info
 * @brief A void pointer placeholder for Evas_Font_OT_Info when OT_SUPPORT is not defined.
 */
typedef void *Evas_Font_OT_Info;
# endif

# ifdef OT_SUPPORT
struct _Evas_Font_OT_Info
{
   size_t source_cluster; /**< The cluster in the original text string that this glyph corresponds to.
                               A cluster is a sequence of one or more characters that form a single,
                               indivisible unit for typographic purposes. For example, in Devanagari,
                               the sequence क + ् + ष can form the ligature क्ष, which is a single cluster. */
   int x_offset;         /**< The horizontal offset for rendering this glyph, in 26.6 fractional pixel units. */
   int y_offset;         /**< The vertical offset for rendering this glyph, in 26.6 fractional pixel units. */
};
# endif

# ifdef OT_SUPPORT
#  define EVAS_FONT_OT_X_OFF_GET(a) ((a).x_offset)
#  define EVAS_FONT_OT_Y_OFF_GET(a) ((a).y_offset)
#  define EVAS_FONT_OT_POS_GET(a)   ((a).source_cluster)
# endif

#include "evas_font.h"
#include "Evas.h"

/**
 * @brief Gets the number of characters belonging to the same cluster as the character at char_index.
 *
 * A cluster is a sequence of one or more characters that are treated as a single
 * unit by the shaping engine. This function determines how many characters in the
 * original text form the cluster that the character at `char_index` (relative to `props->start`)
 * belongs to.
 *
 * @param props Pointer to the Evas_Text_Props structure containing shaped text information.
 *              `props->info->ot` must be populated.
 *              Example:
 *              If `props->text` is "text" and `props->start` is 0,
 *              `props->info->ot` might look like:
 *              `props->info->ot[0] = { .source_cluster = 0, ... }` (for 't')
 *              `props->info->ot[1] = { .source_cluster = 1, ... }` (for 'e')
 *              `props->info->ot[2] = { .source_cluster = 2, ... }` (for 's')
 *              `props->info->ot[3] = { .source_cluster = 3, ... }` (for 't')
 *
 *              If `props->text` is an Arabic word like "كتاب" (kitāb) which might be shaped into
 *              fewer glyphs, `source_cluster` helps map glyphs back to original characters.
 *              For a ligature like "ﻻ" (LĀM + ALEF), it might map to two original characters.
 *              `props->info->ot[0] = { .source_cluster = 0, ... }` (glyph for 'ﻻ')
 *              This means the glyph at index 0 corresponds to the character(s) starting at cluster 0
 *              in the input string.
 * @param char_index The index of the character (relative to the start of the props, i.e., `props->start`)
 *                   within the shaped text for which to find the cluster size.
 * @return The number of characters in the cluster. Returns 1 if no cluster information is
 *         available or if the character forms a cluster by itself.
 */
EVAS_API int
evas_common_font_ot_cluster_size_get(const Evas_Text_Props *props, size_t char_index);

/**
 * @brief Populates Evas_Text_Props with OpenType layout information using HarfBuzz.
 *
 * This function takes a Unicode string and uses HarfBuzz to perform text shaping
 * (e.g., applying ligatures, reordering characters, selecting glyph forms).
 * The results, including glyph indices, positions, and OpenType-specific data,
 * are stored in the `props` structure.
 *
 * @param text Pointer to the Unicode string to be shaped.
 * @param props Pointer to the Evas_Text_Props structure to be populated.
 *              The `props->font_instance` must be a valid font instance.
 *              The `props->script` and `props->bidi_dir` should be set appropriately.
 *              On successful return, `props->info->glyph` and `props->info->ot` will be
 *              allocated and filled.
 *              Example of `props->info->glyph` elements after population:
 *              `props->info->glyph[i].index` = glyph_id
 *              `props->info->glyph[i].pen_after` = accumulated_advance_width_up_to_this_glyph
 *
 *              Example of `props->info->ot` elements after population:
 *              `props->info->ot[i].source_cluster` = original_character_index_this_glyph_maps_to
 *              `props->info->ot[i].x_offset` = horizontal_glyph_offset
 *              `props->info->ot[i].y_offset` = vertical_glyph_offset
 * @param len The length of the `text` string in Unicode characters. If -1, the
 *            string is assumed to be null-terminated.
 * @param mode The shaping mode (e.g., EVAS_TEXT_PROPS_MODE_SHAPE).
 * @param lang The BCP47 language code (e.g., "en", "ar", "hi-IN") for language-specific shaping.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., memory allocation error).
 *         Note: The current implementation always returns EINA_FALSE, this seems to be a bug or outdated.
 */
EVAS_API Eina_Bool
evas_common_font_ot_populate_text_props(const Eina_Unicode *text,
      Evas_Text_Props *props, int len, Evas_Text_Props_Mode mode, const char *lang);
#endif

