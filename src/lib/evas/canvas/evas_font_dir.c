#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eet.h>

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>
#endif

#include "evas_font.h"

/**
 * @def OPAQUE_TYPE(type)
 * @brief Macro to define an opaque type.
 * This is used to forward-declare structures where the detailed definition
 * is not needed by the user of the type.
 * @param type The name of the type to be defined.
 */
#define OPAQUE_TYPE(type) struct __##type { int a; }; \
   typedef struct __##type type

OPAQUE_TYPE(Evas_Font_Set); /**< Opaque type for a set of fonts (RGBA_Font). */
OPAQUE_TYPE(Evas_Font_Instance); /**< Opaque type for a font instance (RGBA_Font_Int). */

/* font dir cache */
static Eina_Hash *font_dirs = NULL; /**< Hash table caching font directories. Key: directory path (char *), Data: Evas_Font_Dir *. */
static Eina_List *fonts_cache = NULL; /**< List of actively used Fndat objects, MRU sorted. */
static Eina_List *fonts_zero = NULL; /**< List of Fndat objects with zero references, candidates for freeing. */
static Eina_List *global_font_path = NULL; /**< List of globally configured font directory paths (char *). */

typedef struct _Fndat Fndat;

/**
 * @struct _Fndat
 * @brief Structure to hold cached font data and its associated properties.
 * This structure is used to manage loaded fonts, their descriptions, sources,
 * and rendering parameters to avoid redundant loading and processing.
 */
struct _Fndat
{
   Evas_Font_Description *fdesc; /**< Pointer to the font description. */
   const char      *source; /**< The source of the font (e.g., EET file path or NULL for system fonts). */
   Evas_Font_Size   size; /**< The size of the font in points or pixels. */
   Evas_Font_Set   *font; /**< Pointer to the loaded font set (Evas_Font_Set). */
   int              ref; /**< Reference count for this cached font entry. */
   Font_Rend_Flags  wanted_rend; /**< Rendering flags requested for this font (e.g., slant, weight). */
   Efl_Text_Font_Bitmap_Scalable bitmap_scalable; /**< Bitmap scalable mode. */

#ifdef HAVE_FONTCONFIG
   FcFontSet *set; /**< Fontconfig font set, if applicable. */
   FcPattern *p_nm; /**< Fontconfig pattern used for matching, if applicable. */

   Eina_Bool file_font : 1; /**< EINA_TRUE if this font was loaded directly from a file, EINA_FALSE if via Fontconfig. */
#endif
};

/* private methods for font dir cache */
static Eina_Bool font_cache_dir_free(const Eina_Hash *hash, const void *key, void *data, void *fdata);
static Evas_Font_Dir *object_text_font_cache_dir_update(char *dir, Evas_Font_Dir *fd);
static Evas_Font *object_text_font_cache_font_find_x(Evas_Font_Dir *fd, char *font);
static Evas_Font *object_text_font_cache_font_find_file(Evas_Font_Dir *fd, char *font);
static Evas_Font *object_text_font_cache_font_find_alias(Evas_Font_Dir *fd, char *font);
static Evas_Font *object_text_font_cache_font_find(Evas_Font_Dir *fd, char *font);
static Evas_Font_Dir *object_text_font_cache_dir_add(char *dir);
static void object_text_font_cache_dir_del(char *dir, Evas_Font_Dir *fd);
static int evas_object_text_font_string_parse(char *buffer, char dest[14][256]);

#ifdef HAVE_FONTCONFIG
static FcConfig *fc_config = NULL; /**< Global Fontconfig configuration. */
#endif

/* FIXME move these helper function to eina_file or eina_path */
/* get the casefold feature! */
#include <unistd.h>
#include <sys/param.h>

/**
 * @internal
 * @brief Get the last modification time of a file.
 *
 * This function retrieves the modification time (mtime or ctime, whichever is newer)
 * of the specified file.
 *
 * @param file The path to the file.
 * @return The modification time as a DATA64 timestamp, or 0 on error.
 */
static DATA64
_file_modified_time(const char *file)
{
   struct stat st;

   if (stat(file, &st) < 0) return 0;
   if (st.st_ctime > st.st_mtime) return (DATA64)st.st_ctime;
   else return (DATA64)st.st_mtime;
   return 0;
}

/**
 * @internal
 * @brief List files in a directory that match a given pattern.
 *
 * This function scans a directory and returns a list of filenames
 * (not full paths) that match the specified pattern.
 *
 * @param path The directory path to scan.
 * @param match The glob pattern to match filenames against (e.g., "*.ttf").
 *              If NULL, all files are listed.
 * @param match_case If 0, matching is case-insensitive. Otherwise, case-sensitive.
 * @return A new Eina_List of strings (char *) containing matching filenames.
 *         The caller is responsible for freeing the list and its contents.
 *         Returns NULL on error or if no files match.
 */
Eina_List *
_file_path_list(char *path, const char *match, int match_case)
{
   Eina_File_Direct_Info *info;
   Eina_Iterator *it;
   Eina_List *files = NULL;
   int flags;

   flags = EINA_FNMATCH_PATHNAME;
   if (!match_case)
     flags |= EINA_FNMATCH_CASEFOLD;
#if defined FNM_IGNORECASE
   if (!match_case)
     flags |= FNM_IGNORECASE;
#else
/*#warning "Your libc does not provide case-insensitive matching!"*/
#endif

   it = eina_file_direct_ls(path);
   EINA_ITERATOR_FOREACH(it, info)
     {
        if (match)
          {
             if (eina_fnmatch(match, info->path + info->name_start, flags))
               files = eina_list_append(files, strdup(info->path + info->name_start));
          }
        else
          files = eina_list_append(files, strdup(info->path + info->name_start));
     }
   if (it) eina_iterator_free(it);
   return files;
}

/**
 * @internal
 * @brief Initializes the Evas font system, particularly Fontconfig.
 *
 * This function ensures that Fontconfig is initialized and configured.
 * It loads the default configuration and fonts, and adds any paths
 * from `global_font_path` to the Fontconfig configuration.
 * This function is called internally when font operations requiring
 * Fontconfig are performed.
 */
static void
evas_font_init(void)
{
#ifdef HAVE_FONTCONFIG
   if (!fc_config)
     {
        Eina_List *l;
        char *path;

        fc_config = FcInitLoadConfigAndFonts();

        EINA_LIST_FOREACH(global_font_path, l, path)
           FcConfigAppFontAddDir(fc_config, (const FcChar8 *) path);
     }
#endif
}

/**
 * @brief Frees the entire Evas font directory cache.
 *
 * This function releases all resources associated with the font directory cache,
 * including cached directory information and Fontconfig configurations if used.
 * It should be called during Evas shutdown or when a full cache reset is needed.
 */
void
evas_font_dir_cache_free(void)
{
   if (font_dirs)
     {
        eina_hash_foreach(font_dirs, font_cache_dir_free, NULL);
        eina_hash_free(font_dirs);
        font_dirs = NULL;
     }
#ifdef HAVE_FONTCONFIG
   if (fc_config)
     {
        FcConfigDestroy(fc_config);
        fc_config = NULL;
     }
#endif
}

/**
 * @brief Finds a font file path within a given directory using the cache.
 *
 * This function searches for a specific font within a directory. It utilizes
 * the font directory cache for efficiency. If the directory or font information
 * is not cached or is outdated, the cache is updated.
 *
 * @param dir The directory path to search within.
 * @param font The name of the font to find (e.g., "DejaVu Sans").
 * @return A const char* pointing to the full path of the font file if found,
 *         otherwise NULL. The returned string is an Eina_Stringshare, so it
 *         should not be freed by the caller directly but will be managed by
 *         the stringsharing mechanism.
 */
const char *
evas_font_dir_cache_find(char *dir, char *font)
{
   Evas_Font_Dir *fd = NULL;

   if (!font_dirs) font_dirs = eina_hash_string_superfast_new(NULL);
   else fd = eina_hash_find(font_dirs, dir);
   fd = object_text_font_cache_dir_update(dir, fd);
   if (fd)
     {
        Evas_Font *fn;

        fn = object_text_font_cache_font_find(fd, font);
        if (fn)
          {
             return fn->path;
          }
     }
   return NULL;
}

/**
 * @internal
 * @brief Parses a comma-separated font name string into a list of font names.
 *
 * This function takes a string that may contain multiple font names separated
 * by commas (e.g., "FontA,FontB,FontC") and splits it into a list of
 * individual font name strings.
 *
 * @param name The comma-separated string of font names.
 * @return An Eina_List of Eina_Stringshare instances, where each string is a
 *         font name. Returns NULL if the input name is NULL or empty.
 *         The caller is responsible for freeing the list and its stringshared items
 *         (e.g., using eina_list_free() and eina_stringshare_del() for each item,
 *         or by iterating and deleting if the list owns the shares).
 *
 * @par Example:
 * @code
 * Eina_List *font_list = evas_font_set_get("Arial,Times New Roman,Courier New");
 * // font_list will contain: "Arial", "Times New Roman", "Courier New"
 * // Remember to free the list and its contents.
 * @endcode
 */
static Eina_List *
evas_font_set_get(const char *name)
{
   Eina_List *fonts = NULL;
   char *p;

   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   if (!*name) return NULL;

   p = strchr(name, ',');
   if (!p)
     {
        fonts = eina_list_append(fonts, eina_stringshare_add(name));
     }
   else
     {
        const char *pp;
        char *nm;

        pp = name;
        while (p)
          {
             nm = alloca(p - pp + 1);
             strncpy(nm, pp, p - pp);
             nm[p - pp] = 0;
             fonts = eina_list_append(fonts, eina_stringshare_add(nm));
             pp = p + 1;
             p = strchr(pp, ',');
             if (!p) fonts = eina_list_append(fonts, eina_stringshare_add(pp));
          }
     }
   return fonts;
}

/**
 * @brief Frees all font data entries currently in the `fonts_zero` list.
 *
 * The `fonts_zero` list contains `Fndat` entries for fonts that have
 * their reference count dropped to zero, making them candidates for cleanup.
 * This function iterates through this list and frees each `Fndat` entry,
 * including its associated font description, source string, font data,
 * and Fontconfig resources if applicable.
 */
void
evas_fonts_zero_free()
{
   Fndat *fd;

   EINA_LIST_FREE(fonts_zero, fd)
     {
        if (fd->fdesc) evas_font_desc_unref(fd->fdesc);
        if (fd->source) eina_stringshare_del(fd->source);
        evas_common_font_free((RGBA_Font *)fd->font);
#ifdef HAVE_FONTCONFIG
        if (fd->set) FcFontSetDestroy(fd->set);
        if (fd->p_nm) FcPatternDestroy(fd->p_nm);
#endif
        free(fd);
     }
}

/**
 * @brief Reduces the number of entries in the `fonts_zero` list under memory pressure.
 *
 * If the `fonts_zero` list (containing unreferenced `Fndat` entries) grows
 * beyond a certain threshold (currently 4), this function removes and frees
 * the oldest entries from the list until the count is below the threshold or
 * an entry with a non-zero reference count is encountered (which shouldn't happen
 * for `fonts_zero` but is checked as a safeguard).
 * This helps manage memory by cleaning up unused font data.
 */
void
evas_fonts_zero_pressure()
{
   Fndat *fd;

   while (fonts_zero
          && eina_list_count(fonts_zero) > 4) /* 4 is arbitrary */
     {
        fd = eina_list_data_get(fonts_zero);

        if (fd->ref != 0) break;
        fonts_zero = eina_list_remove_list(fonts_zero, fonts_zero);

        if (fd->fdesc) evas_font_desc_unref(fd->fdesc);
        if (fd->source) eina_stringshare_del(fd->source);
        evas_common_font_free((RGBA_Font *)fd->font);
      #ifdef HAVE_FONTCONFIG
        if (fd->set) FcFontSetDestroy(fd->set);
        if (fd->p_nm) FcPatternDestroy(fd->p_nm);
      #endif
        free(fd);

        if (eina_list_count(fonts_zero) < 5) break;
     }
}

/**
 * @brief Decrements the reference count of a loaded font.
 *
 * This function is called when a part of the system no longer needs a specific
 * font instance. It finds the corresponding `Fndat` entry in the `fonts_cache`,
 * decrements its reference count. If the reference count drops to zero,
 * the `Fndat` entry is moved from `fonts_cache` to `fonts_zero`, making it
 * a candidate for cleanup by `evas_fonts_zero_pressure` or `evas_fonts_zero_free`.
 *
 * @param font A pointer to the Evas_Font_Set (cast to void*) to be released.
 */
void
evas_font_free(void *font)
{
   Eina_List *l;
   Fndat *fd;

   EINA_LIST_FOREACH(fonts_cache, l, fd)
     {
        if (fd->font == font)
          {
             fd->ref--;
             if (fd->ref == 0)
               {
                  fonts_cache = eina_list_remove_list(fonts_cache, l);
                  fonts_zero = eina_list_append(fonts_zero, fd);
               }
             break;
          }
     }
   while (fonts_zero
          && eina_list_count(fonts_zero) > 42) /* 42 is arbitrary */
     {
        fd = eina_list_data_get(fonts_zero);

        if (fd->ref != 0) break;
        fonts_zero = eina_list_remove_list(fonts_zero, fonts_zero);

        if (fd->fdesc) evas_font_desc_unref(fd->fdesc);
        if (fd->source) eina_stringshare_del(fd->source);
        evas_common_font_free((RGBA_Font *)fd->font);
      #ifdef HAVE_FONTCONFIG
        if (fd->set) FcFontSetDestroy(fd->set);
        if (fd->p_nm) FcPatternDestroy(fd->p_nm);
      #endif
        free(fd);

        if (eina_list_count(fonts_zero) < 43) break;
     }
}

#ifdef HAVE_FONTCONFIG
/**
 * @internal
 * @brief Loads or adds fonts to an Evas_Font_Set using a Fontconfig font set.
 *
 * This function iterates through a Fontconfig font set (`FcFontSet`). For each
 * font pattern in the set, it extracts the filename and attempts to load it
 * using `evas_common_font_load` (if `font` is NULL) or add it to an existing
 * `Evas_Font_Set` using `evas_common_font_add`.
 *
 * @param font An existing Evas_Font_Set to add fonts to, or NULL to create a new set.
 * @param set The Fontconfig font set containing font patterns to load.
 * @param size The desired font size.
 * @param wanted_rend Rendering flags (e.g., slant, weight).
 * @param bitmap_scalable Bitmap scalable mode.
 * @return The Evas_Font_Set with the loaded fonts. This will be the `font`
 *         parameter if it was non-NULL, or a new Evas_Font_Set if `font` was NULL.
 *         Returns NULL if loading fails and `font` was NULL.
 */
static Evas_Font_Set *
_evas_load_fontconfig(Evas_Font_Set *font, FcFontSet *set, int size,
      Font_Rend_Flags wanted_rend, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   int i;

   /* Do loading for all in family */
   for (i = 0; i < set->nfont; i++)
     {
        FcValue filename;

        if (FcPatternGet(set->fonts[i], FC_FILE, 0, &filename) == FcResultMatch)
          {
             if (font)
               evas_common_font_add((RGBA_Font *)font, (char *)filename.u.s, size, wanted_rend, bitmap_scalable);
             else
               font = (Evas_Font_Set *)evas_common_font_load((char *)filename.u.s, size, wanted_rend, bitmap_scalable);
          }
     }

   return font;
}
#endif

#ifdef HAVE_FONTCONFIG
/* In sync with Evas_Font_Style, Evas_Font_Weight and Evas_Font_Width */
/** @internal @brief Maps Evas_Font_Slant enum values to Fontconfig slant constants. */
static int _fc_slant_map[] =
{
   FC_SLANT_ROMAN,
   FC_SLANT_OBLIQUE,
   FC_SLANT_ITALIC
};

/* Apparently EXTRABLACK is not always available, hardcode. */
# ifndef FC_WEIGHT_EXTRABLACK
#  define FC_WEIGHT_EXTRABLACK 215 /**< Fallback define for FC_WEIGHT_EXTRABLACK if not provided by Fontconfig. */
# endif
/** @internal @brief Maps Evas_Font_Weight enum values to Fontconfig weight constants. */
static int _fc_weight_map[] =
{
   FC_WEIGHT_NORMAL,
   FC_WEIGHT_THIN,
   FC_WEIGHT_ULTRALIGHT,
   FC_WEIGHT_EXTRALIGHT,
   FC_WEIGHT_LIGHT,
   FC_WEIGHT_BOOK,
   FC_WEIGHT_MEDIUM,
   FC_WEIGHT_SEMIBOLD,
   FC_WEIGHT_BOLD,
   FC_WEIGHT_ULTRABOLD,
   FC_WEIGHT_EXTRABOLD,
   FC_WEIGHT_BLACK,
   FC_WEIGHT_EXTRABLACK
};

# ifdef FC_WIDTH
/** @internal @brief Maps Evas_Font_Width enum values to Fontconfig width constants. */
static int _fc_width_map[] =
{
   FC_WIDTH_NORMAL,
   FC_WIDTH_ULTRACONDENSED,
   FC_WIDTH_EXTRACONDENSED,
   FC_WIDTH_CONDENSED,
   FC_WIDTH_SEMICONDENSED,
   FC_WIDTH_SEMIEXPANDED,
   FC_WIDTH_EXPANDED,
   FC_WIDTH_EXTRAEXPANDED,
   FC_WIDTH_ULTRAEXPANDED
};
# endif

/** @internal @brief Maps Evas_Font_Spacing enum values to Fontconfig spacing constants. */
static int _fc_spacing_map[] =
{
   FC_PROPORTIONAL,
   FC_DUAL,
   FC_MONO,
   FC_CHARCELL
};

#endif

/**
 * @internal
 * @struct _Style_Map
 * @brief A structure to map style names (strings) to their corresponding integer enum types.
 * Used for parsing font style strings.
 */
struct _Style_Map
{
   const char *name; /**< The string representation of the style (e.g., "bold", "italic"). */
   int type; /**< The integer enum value corresponding to the style name. */
};
typedef struct _Style_Map Style_Map;

/** @internal @brief Maps font width style strings to Evas_Font_Width enum values. */
static Style_Map _style_width_map[] =
{
     {"normal", EVAS_FONT_WIDTH_NORMAL},
     {"ultracondensed", EVAS_FONT_WIDTH_ULTRACONDENSED},
     {"extracondensed", EVAS_FONT_WIDTH_EXTRACONDENSED},
     {"condensed", EVAS_FONT_WIDTH_CONDENSED},
     {"semicondensed", EVAS_FONT_WIDTH_SEMICONDENSED},
     {"semiexpanded", EVAS_FONT_WIDTH_SEMIEXPANDED},
     {"expanded", EVAS_FONT_WIDTH_EXPANDED},
     {"extraexpanded", EVAS_FONT_WIDTH_EXTRAEXPANDED},
     {"ultraexpanded", EVAS_FONT_WIDTH_ULTRAEXPANDED},
};

/** @internal @brief Maps font weight style strings to Evas_Font_Weight enum values. */
static Style_Map _style_weight_map[] =
{
     {"normal", EVAS_FONT_WEIGHT_NORMAL},
     {"thin", EVAS_FONT_WEIGHT_THIN},
     {"ultralight", EVAS_FONT_WEIGHT_ULTRALIGHT},
     {"extralight", EVAS_FONT_WEIGHT_EXTRALIGHT},
     {"light", EVAS_FONT_WEIGHT_LIGHT},
     {"book", EVAS_FONT_WEIGHT_BOOK},
     {"medium", EVAS_FONT_WEIGHT_MEDIUM},
     {"semibold", EVAS_FONT_WEIGHT_SEMIBOLD},
     {"bold", EVAS_FONT_WEIGHT_BOLD},
     {"ultrabold", EVAS_FONT_WEIGHT_ULTRABOLD},
     {"extrabold", EVAS_FONT_WEIGHT_EXTRABOLD},
     {"black", EVAS_FONT_WEIGHT_BLACK},
     {"extrablack", EVAS_FONT_WEIGHT_EXTRABLACK}
};

/** @internal @brief Maps font slant style strings to Evas_Font_Slant enum values. */
static Style_Map _style_slant_map[] =
{
     {"normal", EVAS_FONT_SLANT_NORMAL},
     {"oblique", EVAS_FONT_SLANT_OBLIQUE},
     {"italic", EVAS_FONT_SLANT_ITALIC}
};

/** @internal @brief Maps font spacing style strings to Evas_Font_Spacing enum values. */
static Style_Map _style_spacing_map[] =
{
     {"proportional", EVAS_FONT_SPACING_PROPORTIONAL},
     {"dualwidth", EVAS_FONT_SPACING_DUAL},
     {"monospace", EVAS_FONT_SPACING_MONO},
     {"charcell", EVAS_FONT_SPACING_CHARCELL}
};

#define _STYLE_MAP_LEN(x) (sizeof(x) / sizeof(*(x))) /**< @internal @brief Macro to calculate the number of elements in a Style_Map array. */

/**
 * @internal
 * @brief Finds the string representation for a given style type from a Style_Map.
 *
 * This function searches a provided Style_Map array for an entry whose `type`
 * matches the given `type`, and returns the corresponding `name` string.
 *
 * @param type The integer enum value of the style to find.
 * @param _map The array of Style_Map entries to search.
 * @param map_len The number of elements in the `_map` array.
 * @return The string name of the style if found, otherwise NULL.
 */
static const char*
_evas_font_style_find_str_internal(int type, Style_Map _map[], size_t map_len)
{
   size_t i;
   for ( i = 0; i < map_len; i++ )
     {
        if (_map[i].type == type)
          return _map[i].name;
     }
   return NULL;
}

/**
 * @brief Finds the string representation for a given Evas font style attribute.
 *
 * This function takes an Evas_Font_Style enum (specifying whether to look for
 * slant, weight, or width) and an integer `type` (the specific value of that
 * attribute, e.g., EVAS_FONT_SLANT_ITALIC) and returns its string name
 * (e.g., "italic").
 *
 * @param type The integer value of the specific style attribute (e.g., EVAS_FONT_SLANT_ITALIC).
 * @param style The category of the style attribute (e.g., EVAS_FONT_STYLE_SLANT).
 * @return The string name of the style attribute if found, otherwise NULL.
 *
 * @par Example:
 * @code
 * const char *slant_name = evas_font_style_find_str(EVAS_FONT_SLANT_ITALIC, EVAS_FONT_STYLE_SLANT);
 * // slant_name will be "italic"
 * @endcode
 */
const char*
evas_font_style_find_str(int type, Evas_Font_Style style)
{
#define _RET_STYLE(x) \
   return _evas_font_style_find_str_internal(type, \
                   _style_##x##_map, _STYLE_MAP_LEN(_style_##x##_map));
   switch (style)
     {
        case EVAS_FONT_STYLE_SLANT:
           _RET_STYLE(slant);
        case EVAS_FONT_STYLE_WEIGHT:
           _RET_STYLE(weight);
        case EVAS_FONT_STYLE_WIDTH:
           _RET_STYLE(width);
        default:
           return NULL;
     }
#undef _RET_STYLE
}

/**
 * @internal
 * @brief Finds the integer enum value for a style string within a given range from a Style_Map.
 *
 * This function searches a `Style_Map` array for an entry whose `name` matches
 * (case-insensitively) a word within the provided string segment (`style` to `style_end`).
 * The input string can contain multiple style words separated by spaces.
 *
 * @param style Pointer to the beginning of the style string segment.
 * @param style_end Pointer to the character after the end of the style string segment.
 * @param _map The array of Style_Map entries to search.
 * @param map_len The number of elements in the `_map` array.
 * @return The integer enum `type` of the found style if a match occurs, otherwise 0 (or the default/normal value for that style category).
 */
static unsigned int
_evas_font_style_find_internal(const char *style, const char *style_end,
      Style_Map _map[], size_t map_len)
{
   size_t i;
   while (style < style_end)
     {
        for (i = 0 ; i < map_len ; i++)
          {
             size_t len;
             const char *cur = _map[i].name;
             len = strlen(cur);
             if (!strncasecmp(style, cur, len) &&
                 (!cur[len] || (cur[len] == ' ')))
               {
                  return _map[i].type;
               }
          }
        style = strchr(style, ' ');
        if (!style)
           break;

        while (*style && (*style == ' '))
           style++;
     }
   return 0;
}

/**
 * @brief Finds the Evas font style enum value from a style string.
 *
 * This function parses a style string (e.g., "bold italic") to find the
 * corresponding enum value for a specific style category (slant, weight, or width).
 *
 * @param start Pointer to the beginning of the style string.
 * @param end Pointer to the character after the end of the style string.
 * @param style The category of the style attribute to find (e.g., EVAS_FONT_STYLE_SLANT).
 * @return The unsigned integer enum value of the found style attribute.
 *         Returns the default/normal value for that category if not found or if the
 *         input string does not contain a recognized style for that category.
 *
 * @par Example:
 * @code
 * const char *style_str = "bold condensed";
 * unsigned int weight = evas_font_style_find(style_str, style_str + strlen(style_str), EVAS_FONT_STYLE_WEIGHT);
 * // weight will be EVAS_FONT_WEIGHT_BOLD
 * unsigned int width = evas_font_style_find(style_str, style_str + strlen(style_str), EVAS_FONT_STYLE_WIDTH);
 * // width will be EVAS_FONT_WIDTH_CONDENSED
 * @endcode
 */
unsigned int
evas_font_style_find(const char *start, const char *end,
      Evas_Font_Style style)
{
#define _RET_STYLE(x) \
   return _evas_font_style_find_internal(start, end, \
                   _style_##x##_map, _STYLE_MAP_LEN(_style_##x##_map));
   switch (style)
     {
        case EVAS_FONT_STYLE_SLANT:
           _RET_STYLE(slant);
        case EVAS_FONT_STYLE_WEIGHT:
           _RET_STYLE(weight);
        case EVAS_FONT_STYLE_WIDTH:
           _RET_STYLE(width);
        default:
           return 0;
     }
#undef _RET_STYLE
}

/**
 * @brief Decrements the reference count of an Evas_Font_Description.
 *
 * If the reference count drops to zero, the font description and its
 * associated stringshared members (name, style, fallbacks, lang) are freed.
 *
 * @param fdesc The font description to unreference.
 */
void
evas_font_desc_unref(Evas_Font_Description *fdesc)
{
   if (--(fdesc->ref) == 0)
     {
        eina_stringshare_del(fdesc->name);
        eina_stringshare_del(fdesc->style);
        eina_stringshare_del(fdesc->fallbacks);
        eina_stringshare_del(fdesc->lang);
        free(fdesc);
     }
}

/**
 * @brief Increments the reference count of an Evas_Font_Description.
 *
 * @param fdesc The font description to reference.
 * @return The same Evas_Font_Description pointer passed in.
 */
Evas_Font_Description *
evas_font_desc_ref(Evas_Font_Description *fdesc)
{
   fdesc->ref++;
   return fdesc;
}

/**
 * @brief Creates a new, empty Evas_Font_Description.
 *
 * The new font description is allocated, zeroed, its reference count is
 * initialized to 1, and `is_new` flag is set to EINA_TRUE.
 *
 * @return A pointer to the newly created Evas_Font_Description.
 *         The caller is responsible for eventually unreferencing it using
 *         evas_font_desc_unref().
 */
Evas_Font_Description *
evas_font_desc_new(void)
{
   Evas_Font_Description *fdesc;
   fdesc = calloc(1, sizeof(*fdesc));
   fdesc->ref = 1;
   fdesc->is_new = EINA_TRUE;

   return fdesc;
}

/**
 * @brief Duplicates an existing Evas_Font_Description.
 *
 * A new font description is created and its contents are copied from the
 * provided `fdesc`. Stringshared members (name, fallbacks, lang, style)
 * are also referenced. The new description's reference count is set to 1
 * and `is_new` is set to EINA_TRUE.
 *
 * @param fdesc The font description to duplicate.
 * @return A pointer to the newly created duplicated Evas_Font_Description.
 *         The caller is responsible for eventually unreferencing it using
 *         evas_font_desc_unref().
 */
Evas_Font_Description *
evas_font_desc_dup(const Evas_Font_Description *fdesc)
{
   Evas_Font_Description *new;
   new = evas_font_desc_new();
   memcpy(new, fdesc, sizeof(*new));
   new->ref = 1;
   new->is_new = EINA_TRUE;
   new->name = eina_stringshare_ref(new->name);
   new->fallbacks = eina_stringshare_ref(new->fallbacks);
   new->lang = eina_stringshare_ref(new->lang);
   new->style = eina_stringshare_ref(new->style);

   return new;
}

/**
 * @brief Compares two Evas_Font_Description structures for equality.
 *
 * This function checks if two font descriptions are identical by comparing
 * their name, weight, slant, width, spacing, language, and fallbacks.
 *
 * @param a The first font description.
 * @param b The second font description.
 * @return 0 if the descriptions are identical, non-zero otherwise.
 * @note The FIXME suggests that this function could be extended to provide
 *       ordering (less than/greater than) in the future.
 */
int
evas_font_desc_cmp(const Evas_Font_Description *a,
      const Evas_Font_Description *b)
{
   /* FIXME: Do actual comparison, i.e less than and bigger than. */
   return !((a->name == b->name) && (a->weight == b->weight) &&
            (a->slant == b->slant) && (a->width == b->width) &&
            (a->spacing == b->spacing) && (a->lang == b->lang) &&
            (a->fallbacks == b->fallbacks));
}

/**
 * @brief Normalizes a language string for font matching.
 *
 * - If `lang` is NULL or "none", returns NULL.
 * - If `lang` is "auto", it attempts to get the full language string from
 *   the current locale (e.g., "en_US.UTF-8").
 * - Otherwise, returns the `lang` string as is.
 *
 * @param lang The language string to normalize.
 * @return A normalized language string (potentially from stringshare or locale)
 *         or NULL. The caller should not free the returned string if it's
 *         from locale or stringshare.
 */
const char *
evas_font_lang_normalize(const char *lang)
{
   if (!lang || !strcmp(lang, "none")) return NULL;

   if (!strcmp(lang, "auto"))
     return evas_common_language_from_locale_full_get();

   return lang;
}

/**
 * @brief Parses a font name string and populates an Evas_Font_Description.
 *
 * The font name string can be a simple name (e.g., "DejaVu Sans") or a
 * name followed by style attributes separated by colons (e.g.,
 * "DejaVu Sans:style=Bold Italic:size=12:lang=en:fallbacks=Arial,Helvetica").
 *
 * Recognized attributes:
 * - `:style=<value>`: Sets slant, weight, and width based on `<value>`.
 * - `:slant=<value>`: Sets slant (e.g., "italic", "oblique").
 * - `:weight=<value>`: Sets weight (e.g., "bold", "light").
 * - `:width=<value>`: Sets width (e.g., "condensed", "expanded").
 * - `:spacing=<value>`: Sets spacing (e.g., "monospace", "proportional").
 * - `:lang=<value>`: Sets language (e.g., "en", "fr", "auto").
 * - `:fallbacks=<value>`: Sets comma-separated fallback font names.
 *
 * The `fdesc` structure is updated in place with the parsed values.
 * String members like `name`, `style`, `lang`, `fallbacks` are stringshared.
 *
 * @param fdesc The font description structure to populate.
 * @param name The font name string to parse.
 */
void
evas_font_name_parse(Evas_Font_Description *fdesc, const char *name)
{
   const char *end;

   end = strchr(name, ':');
   if (!end)
     eina_stringshare_replace(&(fdesc->name), name);
   else
     eina_stringshare_replace_length(&(fdesc->name), name, end - name);

   while (end)
     {
        const char *tend;
        name = end;
        end = strchr(end + 1, ':');
        if (!end)
          tend = name + strlen(name);
        else
          tend = end;

        if (!strncmp(name, ":style=", 7))
          {
#define _SET_STYLE(x, len) \
             fdesc->x = _evas_font_style_find_internal(name + len, tend, \
                   _style_##x##_map, _STYLE_MAP_LEN(_style_##x##_map));
             eina_stringshare_replace_length(&(fdesc->style), name + 7, tend - (name + 7));
             _SET_STYLE(slant, 7);
             _SET_STYLE(weight, 7);
             _SET_STYLE(width, 7);
          }
        else if (!strncmp(name, ":slant=", 7))
          {
             _SET_STYLE(slant, 7);
          }
        else if (!strncmp(name, ":weight=", 8))
          {
             _SET_STYLE(weight, 8);
          }
        else if (!strncmp(name, ":width=", 7))
          {
             _SET_STYLE(width, 7);
          }
        else if (!strncmp(name, ":spacing=", 9))
          {
             _SET_STYLE(spacing, 9);
#undef _SET_STYLE
          }
        else if (!strncmp(name, ":lang=", 6))
          {
             const char *tmp = name + 6;
             eina_stringshare_replace_length(&(fdesc->lang), tmp, tend - tmp);
             eina_stringshare_replace(&(fdesc->lang), evas_font_lang_normalize(fdesc->lang));
          }
        else if (!strncmp(name, ":fallbacks=", 11))
          {
             const char *tmp = name + 11;
             eina_stringshare_replace_length(&(fdesc->fallbacks), tmp, tend - tmp);
          }
     }
}

/**
 * @brief Loads a font based on its description, source, size, and other parameters.
 *
 * This is a central function for font loading in Evas. It attempts to load
 * a font by:
 * 1. Checking the active font cache (`fonts_cache`).
 * 2. Checking the zero-reference font cache (`fonts_zero`).
 * 3. If not found in caches:
 *    a. Trying to load directly from a specified `source` (e.g., an EET file).
 *    b. If no `source` or source load fails, trying to load from font file paths:
 *       i. Directly if `fdesc->name` is an absolute/relative path.
 *       ii. Searching in `font_paths` (per-object paths).
 *       iii. Searching in `global_font_path`.
 *    c. If HAVE_FONTCONFIG is defined and still no font is loaded:
 *       i. Using Fontconfig to find a matching font based on `fdesc`.
 *       ii. If a font was loaded directly from a file earlier, Fontconfig might
 *          be used to find fallbacks for it.
 *
 * Once loaded (or found in cache), the font is added to `fonts_cache` with
 * its reference count incremented.
 *
 * @param font_paths An Eina_List of additional directory paths (char *) to search for fonts.
 *                   These are typically per-object font paths. Can be NULL.
 * @param hinting The desired hinting mode for the font. See Evas_Font_Hinting.
 * @param fdesc The Evas_Font_Description detailing the requested font (name, style, etc.).
 *              The `is_new` flag in `fdesc` will be set to EINA_FALSE.
 * @param source Optional path to a source file (e.g., an EET archive) from which
 *               to load the font. If NULL, system fonts are searched.
 * @param size The desired font size (Evas_Font_Size).
 * @param bitmap_scalable Bitmap scalable mode.
 * @return A pointer to the loaded Evas_Font_Set (cast to void*) on success,
 *         or NULL on failure. The returned font set is refcounted.
 */
void *
evas_font_load(const Eina_List *font_paths, int hinting, Evas_Font_Description *fdesc, const char *source, Evas_Font_Size size, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
#ifdef HAVE_FONTCONFIG
   FcPattern *p_nm = NULL;
   FcFontSet *set = NULL;
   Eina_Bool file_font = EINA_FALSE;
#endif

   Evas_Font_Set *font = NULL;
   Eina_List *fonts, *l, *l_next;
   Fndat *fd;
#ifdef HAVE_FONTCONFIG
   Fndat *found_fd = NULL;
#endif
   char *nm;
   Font_Rend_Flags wanted_rend = 0;

   if (!fdesc) return NULL;
   fdesc->is_new = EINA_FALSE;

   if (fdesc->slant != EVAS_FONT_SLANT_NORMAL)
     wanted_rend |= FONT_REND_SLANT;
   if (fdesc->weight == EVAS_FONT_WEIGHT_BOLD)
     wanted_rend |= FONT_REND_WEIGHT;

   evas_font_init();

   EINA_LIST_FOREACH(fonts_cache, l, fd)
     {
        if (!evas_font_desc_cmp(fdesc, fd->fdesc))
          {
              if (((!source) && (!fd->source)) ||
                  ((source) && (fd->source) && (!strcmp(source, fd->source))))
                {
                   if ((size == fd->size) &&
                       (wanted_rend == fd->wanted_rend) &&
                       (bitmap_scalable == fd->bitmap_scalable))
                     {
                        fonts_cache = eina_list_promote_list(fonts_cache, l);
                        fd->ref++;
                        return fd->font;
                     }
                #ifdef HAVE_FONTCONFIG
                   else if (fd->set && fd->p_nm && !fd->file_font)
                     {
                        found_fd = fd;
                     }
                #endif
                }
           }
     }

#ifdef HAVE_FONTCONFIG
   if (found_fd)
     {
        font = _evas_load_fontconfig(font, found_fd->set, size, wanted_rend, bitmap_scalable);
        goto on_find;
     }
#endif

   EINA_LIST_FOREACH_SAFE(fonts_zero, l, l_next, fd)
     {
        if (!evas_font_desc_cmp(fdesc, fd->fdesc))
          {
             if (((!source) && (!fd->source)) ||
                 ((source) && (fd->source) && (!strcmp(source, fd->source))))
               {
                  if ((size == fd->size) &&
                                  (wanted_rend == fd->wanted_rend))
                    {
                       fonts_zero = eina_list_remove_list(fonts_zero, l);
                       fonts_cache = eina_list_prepend(fonts_cache, fd);
                       fd->ref++;
                       return fd->font;
                    }
               #ifdef HAVE_FONTCONFIG
                  else if (fd->set && fd->p_nm && !fd->file_font)
                    {
                       found_fd = fd;
                    }
               #endif
               }
          }
     }

#ifdef HAVE_FONTCONFIG
   if (found_fd)
     {
        font = _evas_load_fontconfig(font, found_fd->set, size, wanted_rend, bitmap_scalable);
        goto on_find;
     }
#endif

   fonts = evas_font_set_get(fdesc->name);
   EINA_LIST_FOREACH(fonts, l, nm) /* Load each font in append */
     {
        if (l == fonts || !font) /* First iteration OR no font */
          {
             /*This will suppress warnings for resource leak*/
             if (font)
               {
                  evas_common_font_free((RGBA_Font*)font);
                  font = NULL;
               }

             if (source) /* Load Font from "eet" source */
               {
                  Eet_File *ef;
                  char fake_name[PATH_MAX];

                   eina_file_path_join(fake_name, PATH_MAX, source, nm);
                   font = (Evas_Font_Set *)evas_common_font_load(fake_name, size, wanted_rend, bitmap_scalable);
                   if (!font) /* Load from fake name failed, probably not cached */
                     {
                        /* read original!!! */
                        ef = eet_open(source, EET_FILE_MODE_READ);
                        if (ef)
                          {
                             void *fdata;
                             int fsize = 0;

                             fdata = eet_read(ef, nm, &fsize);
                             if (fdata)
                               {
                                  font = (Evas_Font_Set *)evas_common_font_memory_load(source, nm, size, fdata, fsize, wanted_rend, bitmap_scalable);
                                  free(fdata);
                               }
                             eet_close(ef);
                          }
                     }
               }
             if (!font) /* Source load failed */
               {
                  if (!eina_file_path_relative((char *)nm)) /* Try filename */
                    font = (Evas_Font_Set *)evas_common_font_load((char *)nm, size, wanted_rend, bitmap_scalable);
                  else /* search font path */
                    {
                       const Eina_List *ll;
                       char *dir;

                       EINA_LIST_FOREACH(font_paths, ll, dir)
                         {
                            const char *f_file;

                            f_file = evas_font_dir_cache_find(dir, (char *)nm);
                            if (f_file)
                              {
                                 font = (Evas_Font_Set *)evas_common_font_load(f_file, size, wanted_rend, bitmap_scalable);
                                 if (font) break;
                              }
                         }

                       if (!font)
                         {
                            EINA_LIST_FOREACH(global_font_path, ll, dir)
                              {
                                 const char *f_file;

                                 f_file = evas_font_dir_cache_find(dir, (char *)nm);
                                 if (f_file)
                                   {
                                      font = (Evas_Font_Set *)evas_common_font_load(f_file, size, wanted_rend, bitmap_scalable);
                                      if (font) break;
                                   }
                              }
                         }
                    }
               }
          }
        else /* Base font loaded, append others */
          {
             void *ok = NULL;

             if (source)
               {
                  Eet_File *ef;
                  char fake_name[PATH_MAX];

                  eina_file_path_join(fake_name, PATH_MAX, source, nm);
                  if (!evas_common_font_add((RGBA_Font *)font, fake_name, size, wanted_rend, bitmap_scalable))
                    {
                       /* read original!!! */
                       ef = eet_open(source, EET_FILE_MODE_READ);
                       if (ef)
                         {
                            void *fdata;
                            int fsize = 0;

                            fdata = eet_read(ef, nm, &fsize);
                            if ((fdata) && (fsize > 0))
                              {
                                 ok = evas_common_font_memory_add((RGBA_Font *)font, source, nm, size, fdata, fsize, wanted_rend, bitmap_scalable);
                              }
                            eet_close(ef);
                            free(fdata);
                         }
                    }
                  else
                    ok = (void *)1;
               }
             if (!ok)
               {
                  if (!eina_file_path_relative((char *)nm))
                    evas_common_font_add((RGBA_Font *)font, (char *)nm, size, wanted_rend, bitmap_scalable);
                  else
                    {
                       const Eina_List *ll;
                       char *dir;
                       RGBA_Font *fn = NULL;

                       EINA_LIST_FOREACH(font_paths, ll, dir)
                         {
                            const char *f_file;

                            f_file = evas_font_dir_cache_find(dir, (char *)nm);
                            if (f_file)
                              {
                                 fn = evas_common_font_add((RGBA_Font *)font, f_file, size, wanted_rend, bitmap_scalable);
                                 if (fn)
                                   break;
                              }
                         }

                       if (!fn)
                         {
                            EINA_LIST_FOREACH(global_font_path, ll, dir)
                              {
                                 const char *f_file;

                                 f_file = evas_font_dir_cache_find(dir, (char *)nm);
                                 if (f_file)
                                   {
                                      fn = evas_common_font_add((RGBA_Font *)font, f_file, size, wanted_rend, bitmap_scalable);
                                      if (fn)
                                         break;
                                   }
                              }
                         }
                    }
               }
          }
       eina_stringshare_del(nm);
     }
   eina_list_free(fonts);

#ifdef HAVE_FONTCONFIG
   if (!font) /* Search using fontconfig */
     {
        FcResult res;

        p_nm = FcPatternBuild (NULL,
              FC_WEIGHT, FcTypeInteger, _fc_weight_map[fdesc->weight],
              FC_SLANT,  FcTypeInteger, _fc_slant_map[fdesc->slant],
              FC_SPACING,  FcTypeInteger, _fc_spacing_map[fdesc->spacing],
#ifdef FC_WIDTH
              FC_WIDTH,  FcTypeInteger, _fc_width_map[fdesc->width],
#endif
              NULL);
        FcPatternAddString (p_nm, FC_FAMILY, (FcChar8*) fdesc->name);

        if (fdesc->style)
          FcPatternAddString (p_nm, FC_STYLE, (FcChar8*) fdesc->style);

        /* Handle font fallbacks */
        if (fdesc->fallbacks)
          {
             const char *start, *end;
             start = fdesc->fallbacks;

             while (start)
               {
                  end = strchr(start, ',');
                  if (end)
                    {
                       char *tmp = alloca((end - start) + 1);
                       strncpy(tmp, start, end - start);
                       tmp[end - start] = 0;
                       FcPatternAddString (p_nm, FC_FAMILY, (FcChar8*) tmp);
                       start = end + 1;
                    }
                  else
                    {
                       FcPatternAddString (p_nm, FC_FAMILY, (FcChar8*) start);
                       break;
                    }
               }
          }

        if (fdesc->lang)
           FcPatternAddString (p_nm, FC_LANG, (FcChar8 *) fdesc->lang);

        FcConfigSubstitute(fc_config, p_nm, FcMatchPattern);
        FcDefaultSubstitute(p_nm);

        /* do matching */
        set = FcFontSort(fc_config, p_nm, FcTrue, NULL, &res);
        if (!set)
          {
              //FIXME add ERR log capability
             //ERR("No fontconfig font matches '%s'. It was the last resource, no font found!", fdesc->name);
             FcPatternDestroy(p_nm);
             p_nm = NULL;
          }
        else
          {
             font = _evas_load_fontconfig(font, set, size, wanted_rend, bitmap_scalable);
          }
     }
   else /* Add a fallback list from fontconfig according to the found font. */
     {
#if FC_MAJOR >= 2 && FC_MINOR >= 11
        FcResult res;

        FT_Face face = evas_common_font_freetype_face_get((RGBA_Font *) font);

        file_font = EINA_TRUE;

        if (face)
          {
             p_nm = FcFreeTypeQueryFace(face, (FcChar8 *) "", 0, NULL);
             FcConfigSubstitute(fc_config, p_nm, FcMatchPattern);
             FcDefaultSubstitute(p_nm);

             /* do matching */
             set = FcFontSort(fc_config, p_nm, FcTrue, NULL, &res);
             if (!set)
               {
                  FcPatternDestroy(p_nm);
                  p_nm = NULL;
               }
             else
               {
                  font = _evas_load_fontconfig(font, set, size, wanted_rend, bitmap_scalable);
               }
          }
#endif
     }
#endif

#ifdef HAVE_FONTCONFIG
 on_find:
#endif
   fd = calloc(1, sizeof(Fndat));
   if (fd)
     {
        fd->fdesc = evas_font_desc_ref(fdesc);
        if (source) fd->source = eina_stringshare_add(source);
        fd->font = font;
        fd->wanted_rend = wanted_rend;
        fd->size = size;
        fd->bitmap_scalable = bitmap_scalable;
        fd->ref = 1;
        fonts_cache = eina_list_prepend(fonts_cache, fd);
#ifdef HAVE_FONTCONFIG
        fd->set = set;
        fd->p_nm = p_nm;
        fd->file_font = file_font;
#endif
     }

   if (font)
     evas_common_font_hinting_set((RGBA_Font *)font, hinting);
   return font;
}

/**
 * @brief Sets the hinting mode for a previously loaded font.
 *
 * @param font A pointer to the Evas_Font_Set (cast to void*) whose hinting is to be set.
 * @param hinting The desired hinting mode. See Evas_Font_Hinting.
 */
void
evas_font_load_hinting_set(void *font, int hinting)
{
   evas_common_font_hinting_set((RGBA_Font *) font, hinting);
}

/**
 * @brief Retrieves a list of available font names.
 *
 * This function compiles a list of font names available to Evas.
 * It queries:
 * 1. Fontconfig (if HAVE_FONTCONFIG is enabled) for system-wide fonts.
 * 2. Fonts found in directories specified in `font_paths` (per-object paths).
 * 3. Fonts found in directories specified in `global_font_path`.
 *
 * The returned list contains stringshared font names.
 *
 * @param font_paths An Eina_List of additional directory paths (char *) to scan for fonts.
 *                   Can be NULL.
 * @return An Eina_List of Eina_Stringshare instances, where each string is an
 *         available font name. Returns NULL on failure or if no fonts are found.
 *         The caller is responsible for freeing the list and its stringshared items
 *         using `evas_font_dir_available_list_free()`.
 *
 * @par Example of a font name in the list:
 * @code
 * "DejaVu Sans,style=Bold"
 * @endcode
 */
Eina_List *
evas_font_dir_available_list(const Eina_List *font_paths)
{
   const Eina_List *l;
   Eina_List *ll;
   Eina_List *available = NULL;
   char *dir;

#ifdef HAVE_FONTCONFIG
   /* Add font config fonts */
   FcPattern *p;
   FcFontSet *set = NULL;
   FcObjectSet *os;
   int i;

   evas_font_init();

   p = FcPatternCreate();
   os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, NULL);

   if (p && os) set = FcFontList(fc_config, p, os);

   if (p) FcPatternDestroy(p);
   if (os) FcObjectSetDestroy(os);

   if (set)
     {
        for (i = 0; i < set->nfont; i++)
          {
             char *font;

             font = (char *)FcNameUnparse(set->fonts[i]);
             available = eina_list_append(available, eina_stringshare_add(font));
             free(font);
          }

        FcFontSetDestroy(set);
     }
#endif

   /* Add fonts in font_paths*/
   if (font_paths)
     {
        if (!font_dirs) font_dirs = eina_hash_string_superfast_new(NULL);

        EINA_LIST_FOREACH(font_paths, l, dir)
          {
             Evas_Font_Dir *fd;

             fd = eina_hash_find(font_dirs, dir);
             fd = object_text_font_cache_dir_update(dir, fd);
             if (fd && fd->aliases)
               {
                  Evas_Font_Alias *fa;

                  EINA_LIST_FOREACH(fd->aliases, ll, fa)
                     available = eina_list_append(available, eina_stringshare_add((char *)fa->alias));
               }
          }
     }

   if (global_font_path)
     {
        if (!font_dirs) font_dirs = eina_hash_string_superfast_new(NULL);

        EINA_LIST_FOREACH(global_font_path, l, dir)
          {
             Evas_Font_Dir *fd;

             fd = eina_hash_find(font_dirs, dir);
             fd = object_text_font_cache_dir_update(dir, fd);
             if (fd && fd->aliases)
               {
                  Evas_Font_Alias *fa;

                  EINA_LIST_FOREACH(fd->aliases, ll, fa)
                     available = eina_list_append(available, eina_stringshare_add((char *)fa->alias));
               }
          }
     }

   return available;
}

/**
 * @brief Frees a list of available font names obtained from `evas_font_dir_available_list()`.
 *
 * This function iterates through the list, deleting each stringshared font name
 * and then frees the list itself.
 *
 * @param available The Eina_List of font names to free.
 */
void
evas_font_dir_available_list_free(Eina_List *available)
{
   while (available)
     {
        eina_stringshare_del(available->data);
        available = eina_list_remove(available, available->data);
     }
}

/* private stuff */
/**
 * @internal
 * @brief Callback function for eina_hash_foreach to free font directory cache entries.
 *
 * This function is used when freeing the `font_dirs` hash table. It calls
 * `object_text_font_cache_dir_del` to clean up the resources associated
 * with a cached font directory.
 *
 * @param hash The hash table being iterated (unused).
 * @param key The key of the hash entry (directory path string).
 * @param data The data of the hash entry (Evas_Font_Dir pointer).
 * @param fdata User data passed to eina_hash_foreach (unused).
 * @return EINA_TRUE to continue iteration.
 */
static Eina_Bool
font_cache_dir_free(const Eina_Hash *hash EINA_UNUSED, const void *key, void *data, void *fdata EINA_UNUSED)
{
   object_text_font_cache_dir_del((char *) key, data);
   return 1;
}

/**
 * @internal
 * @brief Updates a font directory cache entry if it has been modified.
 *
 * Checks if the directory itself, or its `fonts.dir` or `fonts.alias` files,
 * have been modified since they were last cached. If any modification is
 * detected, the existing cache entry is removed and a new one is added by
 * calling `object_text_font_cache_dir_add`.
 *
 * @param dir The path to the font directory.
 * @param fd The existing Evas_Font_Dir cache entry for this directory, or NULL if not cached.
 * @return The up-to-date Evas_Font_Dir cache entry.
 */
static Evas_Font_Dir *
object_text_font_cache_dir_update(char *dir, Evas_Font_Dir *fd)
{
   char file_path[PATH_MAX];
   DATA64 mt;

   if (fd)
     {
        mt = _file_modified_time(dir);
        if (mt != fd->dir_mod_time)
          {
             eina_hash_del(font_dirs, dir, fd);
             object_text_font_cache_dir_del(dir, fd);
          }
        else
          {
             eina_file_path_join(file_path, PATH_MAX, dir, "fonts.dir");
             mt = _file_modified_time(file_path);
             if (mt != fd->fonts_dir_mod_time)
               {
                  eina_hash_del(font_dirs, dir, fd);
                  object_text_font_cache_dir_del(dir, fd);
               }
             else
               {
                  eina_file_path_join(file_path, PATH_MAX, dir, "fonts.alias");
                  mt = _file_modified_time(file_path);
                  if (mt != fd->fonts_alias_mod_time)
                    {
                       eina_hash_del(font_dirs, dir, fd);
                       object_text_font_cache_dir_del(dir, fd);
                    }
                  else
                    return fd;
               }
          }
     }
   return object_text_font_cache_dir_add(dir);
}

/**
 * @internal
 * @brief Finds a font within a cached directory using X Logical Font Description (XLFD) style properties.
 *
 * Parses the `font` string (expected to be an XLFD-like pattern) into 14 property
 * fields. Then, it iterates through the fonts in the `Evas_Font_Dir` that are
 * of XLFD type (fn->type == 1) and compares their properties.
 * Wildcards ('*') in the input `font` pattern are supported.
 *
 * @param fd The cached font directory (Evas_Font_Dir) to search within.
 * @param font The XLFD-like font string to match (e.g., "-*-*-bold-r-normal--*-*-*-*-*-*-*-*").
 * @return An Evas_Font pointer if a match is found, otherwise NULL.
 */
static Evas_Font *
object_text_font_cache_font_find_x(Evas_Font_Dir *fd, char *font)
{
   Eina_List *l;
   char font_prop[14][256];
   int num;
   Evas_Font *fn;

   num = evas_object_text_font_string_parse(font, font_prop);
   if (num != 14) return NULL;
   EINA_LIST_FOREACH(fd->fonts, l, fn)
     {
        if (fn->type == 1)
          {
             int i;
             int match = 0;

             for (i = 0; i < 14; i++)
               {
                  if ((font_prop[i][0] == '*') && (font_prop[i][1] == 0))
                    match++;
                  else
                    {
                       if (!strcasecmp(font_prop[i], fn->x.prop[i])) match++;
                       else break;
                    }
               }
             if (match == 14) return fn;
          }
     }
   return NULL;
}

/**
 * @internal
 * @brief Finds a font within a cached directory by its simple name (filename without extension).
 *
 * Iterates through the fonts in the `Evas_Font_Dir` that are of simple file type
 * (fn->type == 0) and compares their `simple.name` (case-insensitively) with
 * the provided `font` name.
 *
 * @param fd The cached font directory (Evas_Font_Dir) to search within.
 * @param font The simple font name to match (e.g., "DejaVuSans").
 * @return An Evas_Font pointer if a match is found, otherwise NULL.
 */
static Evas_Font *
object_text_font_cache_font_find_file(Evas_Font_Dir *fd, char *font)
{
   Eina_List *l;
   Evas_Font *fn;

   EINA_LIST_FOREACH(fd->fonts, l, fn)
     {
        if (fn->type == 0)
          {
             if (!strcasecmp(font, fn->simple.name)) return fn;
          }
     }
   return NULL;
}

/**
 * @internal
 * @brief Finds a font within a cached directory by its alias.
 *
 * Iterates through the aliases defined in the `Evas_Font_Dir` (typically from
 * a `fonts.alias` file) and compares the alias name (case-insensitively)
 * with the provided `font` name.
 *
 * @param fd The cached font directory (Evas_Font_Dir) to search within.
 * @param font The font alias name to match.
 * @return An Evas_Font pointer if an alias matches and points to a valid font, otherwise NULL.
 */
static Evas_Font *
object_text_font_cache_font_find_alias(Evas_Font_Dir *fd, char *font)
{
   Eina_List *l;
   Evas_Font_Alias *fa;

   EINA_LIST_FOREACH(fd->aliases, l, fa)
     if (!strcasecmp(fa->alias, font)) return fa->fn;
   return NULL;
}

/**
 * @internal
 * @brief Finds a font within a cached directory using various matching strategies.
 *
 * This function attempts to find a font in the following order:
 * 1. Checks the `fd->lookup` hash table for a quick lookup.
 * 2. Tries to find by alias using `object_text_font_cache_font_find_alias()`.
 * 3. Tries to find by XLFD properties using `object_text_font_cache_font_find_x()`.
 * 4. Tries to find by simple file name using `object_text_font_cache_font_find_file()`.
 *
 * If found, the font is added to the `fd->lookup` hash for future faster access.
 *
 * @param fd The cached font directory (Evas_Font_Dir) to search within.
 * @param font The font name or pattern to find.
 * @return An Evas_Font pointer if found, otherwise NULL.
 */
static Evas_Font *
object_text_font_cache_font_find(Evas_Font_Dir *fd, char *font)
{
   Evas_Font *fn;

   fn = eina_hash_find(fd->lookup, font);
   if (fn) return fn;
   fn = object_text_font_cache_font_find_alias(fd, font);
   if (!fn) fn = object_text_font_cache_font_find_x(fd, font);
   if (!fn) fn = object_text_font_cache_font_find_file(fd, font);
   if (!fn) return NULL;
   eina_hash_add(fd->lookup, font, fn);
   return fn;
}

/**
 * @internal
 * @brief Adds a font directory to the cache.
 *
 * This function reads font information from a directory, including:
 * - `fonts.dir`: Contains XLFD-style font definitions.
 * - `fonts.alias`: Contains font aliases.
 * - Actual font files (e.g., `*.ttf`) found by listing the directory.
 *
 * It populates an `Evas_Font_Dir` structure with this information and adds
 * it to the global `font_dirs` hash table.
 *
 * @param dir The path to the font directory to add.
 * @return A pointer to the newly created and populated Evas_Font_Dir structure,
 *         or NULL on failure.
 */
static Evas_Font_Dir *
object_text_font_cache_dir_add(char *dir)
{
   char file_path[PATH_MAX];
   Evas_Font_Dir *fd;
   char *file;
   char tmp2[PATH_MAX];
   Eina_List *fdir;
   Evas_Font *fn;
   FILE *f;

   fd = calloc(1, sizeof(Evas_Font_Dir));
   if (!fd) return NULL;
   fd->lookup = eina_hash_string_superfast_new(NULL);

   eina_hash_add(font_dirs, dir, fd);

   /* READ fonts.alias, fonts.dir and directory listing */

   /* fonts.dir */
   eina_file_path_join(file_path, PATH_MAX, dir, "fonts.dir");

   f = fopen(file_path, "rb");
   if (f)
     {
        int num;
        char fname[4096], fdef[4096];

        if (fscanf(f, "%i\n", &num) != 1) goto cant_read;
        /* read font lines */
        while (fscanf(f, "%4090s %[^\n]\n", fname, fdef) == 2)
          {
             char font_prop[14][256];
             int i;

             /* skip comments */
             if ((fdef[0] == '!') || (fdef[0] == '#')) continue;
             /* parse font def */
             num = evas_object_text_font_string_parse((char *)fdef, font_prop);
             if (num == 14)
               {
                  fn = calloc(1, sizeof(Evas_Font));
                  if (fn)
                    {
                       fn->type = 1;
                       for (i = 0; i < 14; i++)
                         fn->x.prop[i] = eina_stringshare_add(font_prop[i]);
                       eina_file_path_join(tmp2, PATH_MAX, dir, fname);
                       fn->path = eina_stringshare_add(tmp2);
                       fd->fonts = eina_list_append(fd->fonts, fn);
                    }
               }
          }
        cant_read: ;
        fclose(f);
     }

   /* directoy listing */
   fdir = _file_path_list(dir, "*.ttf", 0);
   EINA_LIST_FREE(fdir, file)
     {
        eina_file_path_join(file_path, PATH_MAX, dir, file);
        fn = calloc(1, sizeof(Evas_Font));
        if (fn)
          {
             char *p;

             fn->type = 0;
             p = strrchr(file, '.');
             if (p) fn->simple.name = eina_stringshare_add_length(file, p - file);
             else fn->simple.name = eina_stringshare_add(file);
             eina_file_path_join(tmp2, PATH_MAX, dir, file);
             fn->path = eina_stringshare_add(tmp2);
             fd->fonts = eina_list_append(fd->fonts, fn);
          }
        free(file);
     }

   /* fonts.alias */
   eina_file_path_join(file_path, PATH_MAX, dir, "fonts.alias");

   f = fopen(file_path, "rb");
   if (f)
     {
        char fname[4096], fdef[4096];

        /* read font alias lines */
        while (fscanf(f, "%4090s %4090[^\n]\n", fname, fdef) == 2)
          {
             Evas_Font_Alias *fa;

             /* skip comments */
             if ((fname[0] == '!') || (fname[0] == '#')) continue;
             fa = calloc(1, sizeof(Evas_Font_Alias));
             if (fa)
               {
                  fa->alias = eina_stringshare_add(fname);
                  fa->fn = object_text_font_cache_font_find_x(fd, fdef);
                  if ((!fa->alias) || (!fa->fn))
                    {
                       if (fa->alias) eina_stringshare_del(fa->alias);
                       free(fa);
                    }
                  else
                    fd->aliases = eina_list_append(fd->aliases, fa);
               }
          }
        fclose(f);
     }

   fd->dir_mod_time = _file_modified_time(dir);

   eina_file_path_join(file_path, PATH_MAX, dir, "fonts.dir");
   fd->fonts_dir_mod_time = _file_modified_time(file_path);

   eina_file_path_join(file_path, PATH_MAX, dir, "fonts.alias");
   fd->fonts_alias_mod_time = _file_modified_time(file_path);

   return fd;
}

/**
 * @internal
 * @brief Deletes a font directory cache entry and frees its resources.
 *
 * This function cleans up an `Evas_Font_Dir` structure, freeing its lookup hash,
 * lists of fonts and aliases, and all associated stringshared data and
 * allocated memory.
 *
 * @param dir The directory path (unused in current implementation, but part of the signature
 *            for potential future use or consistency with similar functions).
 * @param fd The Evas_Font_Dir structure to delete.
 */
static void
object_text_font_cache_dir_del(char *dir EINA_UNUSED, Evas_Font_Dir *fd)
{
   if (fd->lookup) eina_hash_free(fd->lookup);
   while (fd->fonts)
     {
        Evas_Font *fn;
        int i;

        fn = fd->fonts->data;
        fd->fonts = eina_list_remove(fd->fonts, fn);
        for (i = 0; i < 14; i++)
          {
             if (fn->x.prop[i]) eina_stringshare_del(fn->x.prop[i]);
          }
        if (fn->simple.name) eina_stringshare_del(fn->simple.name);
        if (fn->path) eina_stringshare_del(fn->path);
        free(fn);
     }
   while (fd->aliases)
     {
        Evas_Font_Alias *fa;

        fa = fd->aliases->data;
        fd->aliases = eina_list_remove(fd->aliases, fa);
        if (fa->alias) eina_stringshare_del(fa->alias);
        free(fa);
     }
   free(fd);
}

/**
 * @internal
 * @brief Parses an X Logical Font Description (XLFD) like string into 14 property fields.
 *
 * An XLFD string is typically a sequence of 14 fields separated by hyphens ('-').
 * Example: "-misc-fixed-medium-r-normal--13-120-75-75-c-60-iso8859-1"
 * This function splits such a string into its constituent parts.
 *
 * @param buffer The input XLFD-like string. Must start with a hyphen.
 * @param dest A 2D array of characters (`char[14][256]`) where the parsed
 *             property strings will be stored. Each of the 14 strings will be
 *             null-terminated.
 *             Example structure of `dest` after parsing:
 *             dest[0] = "misc"
 *             dest[1] = "fixed"
 *             ...
 *             dest[13] = "iso8859-1"
 * @return The number of fields successfully parsed (should be 14 for a valid XLFD string).
 *         Returns 0 if the input string does not start with a hyphen.
 */
static int
evas_object_text_font_string_parse(char *buffer, char dest[14][256])
{
   char *p;
   int n, m, i;

   n = 0;
   m = 0;
   p = buffer;
   if (p[0] != '-') return 0;
   i = 1;
   while (p[i])
     {
        dest[n][m] = p[i];
        if ((p[i] == '-') || (m == 255))
          {
             dest[n][m] = 0;
             n++;
             m = -1;
          }
        i++;
        m++;
        if (n == 14) return n;
     }
   dest[n][m] = 0;
   n++;
   return n;
}

/**
 * @brief Appends a directory path to the global list of font search paths.
 *
 * This path will be used by Evas when searching for fonts if they are not
 * found in per-object font paths or by other means.
 * If Fontconfig is enabled and initialized, the path is also added to the
 * Fontconfig application font directories.
 *
 * @param path The directory path to append. The string is stringshared.
 */
EVAS_API void
evas_font_path_global_append(const char *path)
{
   if (!path) return;
   global_font_path = eina_list_append(global_font_path, eina_stringshare_add(path));
#ifdef HAVE_FONTCONFIG
   if (fc_config)
     FcConfigAppFontAddDir(fc_config, (const FcChar8 *) path);
#endif
}

/**
 * @brief Prepends a directory path to the global list of font search paths.
 *
 * This path will be searched before other paths in the global list.
 * If Fontconfig is enabled and initialized, the path is also added to the
 * Fontconfig application font directories (typically searched with high priority).
 *
 * @param path The directory path to prepend. The string is stringshared.
 */
EVAS_API void
evas_font_path_global_prepend(const char *path)
{
   if (!path) return;
   global_font_path = eina_list_prepend(global_font_path, eina_stringshare_add(path));
#ifdef HAVE_FONTCONFIG
   if (fc_config)
     FcConfigAppFontAddDir(fc_config, (const FcChar8 *) path);
#endif
}

/**
 * @brief Clears all paths from the global font search path list.
 *
 * If Fontconfig is enabled and initialized, this also clears any
 * application-specific font directories that were added to Fontconfig
 * via Evas.
 */
EVAS_API void
evas_font_path_global_clear(void)
{
   while (global_font_path)
     {
        eina_stringshare_del(global_font_path->data);
        global_font_path = eina_list_remove(global_font_path, global_font_path->data);
     }
#ifdef HAVE_FONTCONFIG
   if (fc_config)
     FcConfigAppFontClear(fc_config);
#endif
}

/**
 * @brief Retrieves the global list of font search paths.
 *
 * @return A const Eina_List* containing stringshared directory paths.
 *         The caller should not modify this list.
 */
EVAS_API const Eina_List *
evas_font_path_global_list(void)
{
   return global_font_path;
}

/**
 * @brief Reinitializes the Evas font subsystem, particularly Fontconfig.
 *
 * If Fontconfig is enabled, this function destroys the current Fontconfig
 * configuration and reloads it. It then re-adds all paths from the
 * `global_font_path` list to the new Fontconfig configuration.
 * This can be useful if system font configurations have changed.
 */
EVAS_API void
evas_font_reinit(void)
{
#ifdef HAVE_FONTCONFIG
   Eina_List *l;
   char *path;

   if (fc_config)
     {
        FcConfigDestroy(fc_config);
        fc_config = FcInitLoadConfigAndFonts();

        EINA_LIST_FOREACH(global_font_path, l, path)
           FcConfigAppFontAddDir(fc_config, (const FcChar8 *) path);
     }
#endif
}
