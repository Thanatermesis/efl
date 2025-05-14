/**
 * @file
 * @brief This file contains utility functions and structures for text manipulation in Evas.
 * It includes definitions for text properties, glyph handling, font arrays,
 * and color parsing.
 */
#ifndef _EVAS_TEXT_UTILS_H
# define _EVAS_TEXT_UTILS_H

/**
 * @brief Structure representing text properties.
 * @see _Evas_Text_Props
 */
typedef struct _Evas_Text_Props      Evas_Text_Props;
/**
 * @brief Special case text properties for a single character.
 * @see _Evas_Text_Props_One
 */
typedef struct _Evas_Text_Props_One  Evas_Text_Props_One;

/**
 * @brief Structure holding information about text properties, like glyphs and OpenType data.
 * @see _Evas_Text_Props_Info
 */
typedef struct _Evas_Text_Props_Info Evas_Text_Props_Info;
/**
 * @brief Structure for font array data, including color, position, and glyphs.
 * @see _Evas_Font_Array_Data
 */
typedef struct _Evas_Font_Array_Data Evas_Font_Array_Data;
/**
 * @brief Structure representing an array of fonts.
 * @see _Evas_Font_Array
 */
typedef struct _Evas_Font_Array      Evas_Font_Array;
/**
 * @brief Structure holding information for a single font glyph.
 * @see _Evas_Font_Glyph_Info
 */
typedef struct _Evas_Font_Glyph_Info Evas_Font_Glyph_Info;

/**
 * @brief Enumeration for text properties processing mode.
 */
typedef enum
{
   EVAS_TEXT_PROPS_MODE_NONE = 0, /**< No special processing mode. */
   EVAS_TEXT_PROPS_MODE_SHAPE     /**< Text shaping mode (e.g., for complex scripts). */
} Evas_Text_Props_Mode;

# include "evas_font_ot.h"
# include "language/evas_language_utils.h"

/**
 * @brief Unicode replacement character (U+FFFD).
 * Used for displaying "malformed" or missing characters.
 */
#define REPLACEMENT_CHAR 0xFFFD

/** @brief Forward declaration for Evas_Glyph structure. */
typedef struct _Evas_Glyph Evas_Glyph;
/**
 * @brief Structure representing an array of glyphs.
 * @see _Evas_Glyph_Array
 */
typedef struct _Evas_Glyph_Array Evas_Glyph_Array;

/**
 * @struct _Evas_Glyph_Array
 * @brief Represents an array of glyphs associated with a font instance.
 *
 * This structure holds an inarray of glyphs, a pointer to the font instance (`fi`),
 * and a reference count for managing its lifecycle.
 *
 * @example
 * // An example of what glyph_array might represent for the text "Hi":
 * Evas_Glyph_Array *glyph_array;
 * // glyph_array->array would be an Eina_Inarray containing two Evas_Glyph elements,
 * // one for 'H' and one for 'i'. Each Evas_Glyph would hold renderable glyph
 * // data (like a bitmap).
 * //
 * // Conceptual structure:
 * // glyph_array->array: [ (Evas_Glyph for 'H'), (Evas_Glyph for 'i') ]
 * // glyph_array->fi: (pointer to RGBA_Font_Int for the font "Sans 10")
 * // glyph_array->refcount: 1
 */
struct _Evas_Glyph_Array
{
   Eina_Inarray *array;    /**< Eina_Inarray storing the actual glyph data. */
   void *fi;               /**< Pointer to the font instance (e.g., RGBA_Font_Int). */
   unsigned int refcount;  /**< Reference count for memory management. */
};

/**
 * @struct _Evas_Text_Props
 * @brief Defines the properties of a segment of text.
 *
 * This structure holds all relevant information for a piece of text,
 * including its glyphs, font instance, BiDi direction, script type,
 * and layout information.
 *
 * The `szlen_mode` field indicates whether the structure uses full size/length
 * fields or optimized ones (e.g., for a single character).
 *
 * @example
 * // Let's assume we have a text object for the string "Hello World" and we
 * // want to represent the "Hello" part.
 * Evas_Text_Props props;
 * // props.info would point to a shared Evas_Text_Props_Info containing
 * // glyph and OT data for the entire "Hello World" string.
 * // props.glyphs would point to an Evas_Glyph_Array with rendered glyphs for "Hello".
 * // props.font_instance: (pointer to RGBA_Font_Int for "Sans 10")
 * // props.generation: (cache generation counter)
 * // props.bidi_dir: EVAS_BIDI_DIRECTION_LTR
 * // props.script: EVAS_SCRIPT_LATIN
 * // props.start: 0 (index in the shared info arrays)
 * // props.len: 5 (number of glyphs for "Hello")
 * // props.text_offset: 0 (offset in original "Hello World" string)
 * // props.text_len: 5 (length in original "Hello World" string)
 */
struct _Evas_Text_Props
{
   Evas_Text_Props_Info *info; // 8/4
   Evas_Glyph_Array *glyphs; // 8/4
   void *font_instance; // 8/4
   /* Start and len represent the start offset and the length in the
    * glyphs_info and ot_data fields, they are both internal */
   // i only wonder if generation needs 32bits... :)
   int generation; // 4
//   Evas_BiDi_Props bidi; // 4 // bidi.dir == enum
//   Evas_Script_Type script; // 4 // enum
//   Eina_Bool changed : 1; // 1
//   Eina_Bool prepare : 1;
//   // +3 pad
// ** the below saves 8 bytes (72 -> 64 on 64bit)
   Evas_BiDi_Direction bidi_dir : 2; // 2 (enough for values)
   Evas_Script_Type script : 7; // cont (enough for values)
   Eina_Bool changed : 1; // cont (bool)
   Eina_Bool prepare : 1; // cont (bool)
   // we have space here for at LEAST 5 bits (round up to 2 bytes) but due
   // to common padding we actually can add 5 + 16 (21) more bits for free
   Eina_Bool szlen_mode : 5; // use 5 of he 21 bits
// this can go here as the above is nicely aligned....
   // this is really big... 32 bytes. MOSt of the time the following...
   // start == text_offset == 0 AND len == text_len == smallish value (8 or
   // 16 bit is enough to store it most of the time).
   size_t start;          /**< Start index in info->glyph and info->ot arrays. */
   size_t len;            /**< Number of entries (glyphs/OT info) this prop covers. */
   size_t text_offset;    /**< The offset of this text segment from the start of the original full text string. */
   size_t text_len;       /**< The length of the original text string that this property segment corresponds to. */
};

/** @brief Indicates that Evas_Text_Props uses full size and length fields. */
#define EVAS_TP_SZLEN_FULL 0
/** @brief Indicates that Evas_Text_Props might use optimized size/length (potentially for Evas_Text_Props_One). */
#define EVAS_TP_SZLEN_ONE 1

/**
 * @struct _Evas_Text_Props_One
 * @brief A special case of Evas_Text_Props optimized for a single character.
 *
 * This structure is similar to Evas_Text_Props but is intended for scenarios
 * like text grids where individual character properties are managed. It omits
 * the size/length fields (`start`, `len`, `text_offset`, `text_len`) present
 * in the general `_Evas_Text_Props` structure, as these are implicitly 1 or 0
 * for a single character.
 *
 * @example
 * Evas_Text_Props_One props_one;
 * // props_one.info, props_one.glyphs, props_one.font_instance, etc.
 * // are used similarly to _Evas_Text_Props, but contextually for one char.
 */
struct _Evas_Text_Props_One
{
   Evas_Text_Props_Info *info; /**< Pointer to shared text property information. @see _Evas_Text_Props_Info */
   Evas_Glyph_Array *glyphs;   /**< Pointer to an array of glyphs. @see _Evas_Glyph_Array */
   void *font_instance; // 8/4
   /* Start and len represent the start offset and the length in the
    * glyphs_info and ot_data fields, they are both internal */
   // i only wonder if generation needs 32bits... :)
   int generation; // 4
//   Evas_BiDi_Props bidi; // 4 // bidi.dir == enum
//   Evas_Script_Type script; // 4 // enum
//   Eina_Bool changed : 1; // 1
//   Eina_Bool prepare : 1;
//   // +3 pad
// ** the below saves 8 bytes (72 -> 64 on 64bit)
   Evas_BiDi_Direction bidi_dir : 2; // 2 (enough for values)
   Evas_Script_Type script : 7; // cont (enough for values)
   Eina_Bool changed : 1; // cont (bool)
   Eina_Bool prepare : 1; // cont (bool)
   // we have space here for at LEAST 5 bits (round up to 2 bytes) but due
   // to common padding we actually can add 5 + 16 (21) more bits for free
   Eina_Bool szlen_mode : 5; /**< Mode for size/length fields, e.g., EVAS_TP_SZLEN_ONE. */
};

/**
 * @struct _Evas_Text_Props_Info
 * @brief Contains shared information for text properties, like glyph and OpenType data.
 *
 * This structure is referenced by Evas_Text_Props and allows multiple
 * Evas_Text_Props instances to share the same underlying glyph and OT information,
 * managed by a reference count.
 *
 * @example
 * // For a string "Hi", this shared info could be:
 * Evas_Text_Props_Info info;
 * // info.glyph is an array of Evas_Font_Glyph_Info structs:
 * // info.glyph -> [
 * //   { .index = 72, .pen_after = 640, .x_bear = 1, .y_bear = 12, .width = 8 }, // 'H'
 * //   { .index = 73, .pen_after = 960, .x_bear = 1, .y_bear =  8, .width = 3 }  // 'i'
 * // ]
 * //
 * // info.ot is an array of Evas_Font_OT_Info structs (if OT enabled):
 * // info.ot -> [
 * //   { .source_cluster = 0 }, // 'H'
 * //   { .source_cluster = 1 }  // 'i'
 * // ]
 * //
 * // info.refcount would be 1 if one Evas_Text_Props instance uses this info.
 */
struct _Evas_Text_Props_Info
{
   Evas_Font_Glyph_Info *glyph; /**< Array of glyph information. @see _Evas_Font_Glyph_Info */
   Evas_Font_OT_Info *ot;       /**< Array of OpenType information (if OT_SUPPORT is defined). @see Evas_Font_OT_Info */
   unsigned int refcount;       /**< Reference count for managing the lifecycle of this info. */
};

/**
 * @struct _Evas_Font_Array_Data
 * @brief Represents data for an element in a font array, typically for styled text.
 *
 * This includes color, x-coordinate (position), and a pointer to glyph array.
 *
 * @example
 * Evas_Font_Array_Data fad;
 * // fad.color = { 255, 0, 0, 255 }; // Red color
 * // fad.x = 100; // X position
 * // fad.glyphs points to an _Evas_Glyph_Array instance.
 */
struct _Evas_Font_Array_Data
{
   struct {
      unsigned char r, g, b, a; /**< RGBA color components. */
   } color;
   int x;                         /**< X-coordinate for positioning. */
   Evas_Glyph_Array *glyphs;      /**< Pointer to the glyph array for this segment. @see _Evas_Glyph_Array */
};

/**
 * @struct _Evas_Font_Array
 * @brief Represents an array of font data entries, used for rendering text with multiple styles/fonts.
 *
 * Contains an Eina_Inarray of _Evas_Font_Array_Data elements and a reference count.
 *
 * @example
 * // For a text where "Hello" is red and "World" is blue:
 * Evas_Font_Array *font_array;
 * // font_array->array is an Eina_Inarray with two _Evas_Font_Array_Data elements:
 * // font_array->array -> [
 * //   {
 * //     .color = { 255, 0, 0, 255 }, // Red
 * //     .x = 0,
 * //     .glyphs = (Evas_Glyph_Array for "Hello")
 * //   },
 * //   {
 * //     .color = { 0, 0, 255, 255 }, // Blue
 * //     .x = (width of "Hello"),
 * //     .glyphs = (Evas_Glyph_Array for "World")
 * //   }
 * // ]
 * // font_array->refcount would be 1.
 */
struct _Evas_Font_Array
{
   Eina_Inarray *array;    /**< Eina_Inarray storing _Evas_Font_Array_Data elements. */
   unsigned int refcount;  /**< Reference count for memory management. */
};

/**
 * @struct _Evas_Font_Glyph_Info
 * @brief Detailed information about a single glyph.
 *
 * This structure holds the glyph's index (as per FreeType), its advance width
 * after placement (`pen_after`), and its typographic properties like bearings
 * and width. These are typically sorted in visual order when created.
 *
 * @example
 * Evas_Font_Glyph_Info glyph_info;
 * // glyph_info.index = 123; // Font-specific glyph index
 * // glyph_info.pen_after = 640; // Pen position after this glyph (in 26.6 fixed point, then typically converted)
 * // glyph_info.x_bear = 5;   // Horizontal distance from origin to left edge of bitmap
 * // glyph_info.y_bear = 18;  // Vertical distance from origin to top edge of bitmap
 * // glyph_info.width = 10;   // Width of the glyph's bitmap
 */
struct _Evas_Font_Glyph_Info
{
   unsigned int index; /**< Glyph index, should conform to FreeType's indexing. */
#if 1
   /* Using shorts to save space. If >32k glyphs or larger relative layout
    * info is needed, this might need adjustment. */
   Evas_Coord pen_after; /**< Pen position after this glyph is drawn. */
   short x_bear;         /**< Horizontal bearing (offset from origin to left edge of glyph). */
   short y_bear;         /**< Vertical bearing (offset from origin to top edge of glyph). */
   short width;          /**< Width of the glyph. */
#else
   Evas_Coord x_bear;
   Evas_Coord y_bear;
   Evas_Coord width;
   Evas_Coord pen_after;
#endif
};

/**
 * @brief Increments the reference count of an Evas_Glyph_Array.
 * @param array The glyph array to reference.
 */
void
evas_common_font_glyphs_ref(Evas_Glyph_Array *array);
/**
 * @brief Decrements the reference count of an Evas_Glyph_Array and frees it if the count reaches zero.
 * @param array The glyph array to unreference.
 */
void
evas_common_font_glyphs_unref(Evas_Glyph_Array *array);

/**
 * @brief Increments the reference count of an Evas_Font_Array.
 * @param array The font array to reference.
 */
void
evas_common_font_fonts_ref(Evas_Font_Array *array);
/**
 * @brief Decrements the reference count of an Evas_Font_Array and frees it if the count reaches zero.
 * @param array The font array to unreference.
 */
void
evas_common_font_fonts_unref(Evas_Font_Array *array);

/**
 * @brief Sets the BiDi (bidirectional) properties for a text segment.
 * @param props The text properties structure to modify.
 * @param bidi_par_props Bidirectional properties of the paragraph.
 * @param start The starting character index within the paragraph for this text segment.
 */
void
evas_common_text_props_bidi_set(Evas_Text_Props *props,
      Evas_BiDi_Paragraph_Props *bidi_par_props, size_t start);

/**
 * @brief Sets the script type for a text segment.
 * @param props The text properties structure to modify.
 * @param scr The script type (e.g., EVAS_SCRIPT_LATIN, EVAS_SCRIPT_ARABIC).
 */
void
evas_common_text_props_script_set(Evas_Text_Props *props, Evas_Script_Type scr);

/**
 * @brief Creates or updates the content (glyphs, OT info) for a text properties structure.
 * @param _fi Pointer to the font instance (RGBA_Font_Int).
 * @param text The Unicode text string.
 * @param text_props The text properties structure to populate.
 * @param par_props Bidirectional properties of the paragraph (used if BIDI_SUPPORT is enabled and OT_SUPPORT is not).
 * @param par_pos Starting position in the paragraph (used if BIDI_SUPPORT is enabled and OT_SUPPORT is not).
 * @param len The length of the text string.
 * @param mode The processing mode (e.g., EVAS_TEXT_PROPS_MODE_SHAPE).
 * @param lang The language of the text (ISO 639 code, e.g., "en", "ar"). Used for OpenType features.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EVAS_API Eina_Bool
evas_common_text_props_content_create(void *_fi, const Eina_Unicode *text,
      Evas_Text_Props *text_props, const Evas_BiDi_Paragraph_Props *par_props,
      size_t par_pos, int len, Evas_Text_Props_Mode mode, const char *lang);

/**
 * @brief Copies content from one Evas_Text_Props structure to another and references shared data.
 * Glyphs themselves are not copied but the underlying info is referenced.
 * @param dst The destination text properties structure.
 * @param src The source text properties structure.
 */
void
evas_common_text_props_content_copy_and_ref(Evas_Text_Props *dst,
      const Evas_Text_Props *src);

/**
 * @brief Increments the reference count of the shared content within an Evas_Text_Props structure.
 * @param props The text properties structure whose content is to be referenced.
 */
void
evas_common_text_props_content_ref(Evas_Text_Props *props);

/**
 * @brief Decrements the reference count of shared content, potentially freeing font instance and glyphs but not the Evas_Text_Props_Info itself if still referenced.
 * This is a special unref that doesn't free the props->info if its refcount hits zero,
 * but handles unreferencing font_instance and glyphs.
 * @param props The text properties structure.
 */
void
evas_common_text_props_content_nofree_unref(Evas_Text_Props *props);

/**
 * @brief Decrements the reference count of the shared content within an Evas_Text_Props structure and frees it if the count reaches zero.
 * @param props The text properties structure whose content is to be unreferenced.
 */
void
evas_common_text_props_content_unref(Evas_Text_Props *props);

/**
 * @brief Finds the position of the next cluster in the text.
 * @param props The text properties.
 * @param pos The current character position (in logical order, relative to the start of props->text_offset).
 * @return The character position of the start of the next cluster.
 */
EVAS_API int
evas_common_text_props_cluster_next(const Evas_Text_Props *props, int pos);

/**
 * @brief Finds the position of the previous cluster in the text.
 * @param props The text properties.
 * @param pos The current character position (in logical order, relative to the start of props->text_offset).
 * @return The character position of the start of the previous cluster.
 */
EVAS_API int
evas_common_text_props_cluster_prev(const Evas_Text_Props *props, int pos);

/**
 * @brief Finds the internal glyph/OT array index corresponding to a logical character position.
 * @param props The text properties.
 * @param _cutoff The logical character position (relative to the start of props->text_offset).
 * @return The index in the props->info->glyph or props->info->ot array, or -1 if not found.
 */
EVAS_API int
evas_common_text_props_index_find(const Evas_Text_Props *props, int _cutoff);

/**
 * @brief Splits a text properties structure into two at a given cutoff point.
 * @param base The original text properties structure, which will be modified to represent the part before the cutoff.
 * @param ext A new text properties structure that will represent the part after the cutoff.
 * @param cutoff The logical character position (relative to the start of base->text_offset) at which to split.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., cutoff is invalid or inside a cluster).
 */
EVAS_API Eina_Bool
evas_common_text_props_split(Evas_Text_Props *base, Evas_Text_Props *ext,
      int cutoff);
/**
 * @brief Merges two adjacent text properties structures that share the same underlying info.
 * @param item1 The first text properties structure, which will be extended.
 * @param item2 The second text properties structure, which will be merged into item1. item2 should not be used after this.
 */
EVAS_API void
evas_common_text_props_merge(Evas_Text_Props *item1, const Evas_Text_Props *item2);

/**
 * @brief Parses a color string into RGBA components.
 *
 * Supports formats like:
 * - "#RRGGBB"
 * - "#RRGGBBAA"
 * - "#RGB"
 * - "#RGBA"
 * - "colorname" (e.g., "red", "blue")
 * - "rgb(r,g,b)"
 * - "rgba(r,g,b,a)" (alpha 0.0-1.0)
 *
 * If alpha is specified and not 0xFF, the R, G, B components are pre-multiplied by alpha.
 *
 * @param str The color string to parse.
 * @param slen The length of the color string.
 * @param[out] r Pointer to store the red component (0-255).
 * @param[out] g Pointer to store the green component (0-255).
 * @param[out] b Pointer to store the blue component (0-255).
 * @param[out] a Pointer to store the alpha component (0-255).
 * @return EINA_TRUE if parsing was successful, EINA_FALSE otherwise.
 */
Eina_Bool evas_common_format_color_parse(const char *str, int slen, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a);

#endif
