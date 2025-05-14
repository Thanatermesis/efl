/**
 * @file
 * @brief Font loading and caching mechanisms for Evas.
 *
 * This file implements the core logic for loading font sources (from files or memory),
 * managing font instances (specific sizes and rendering options), and caching them
 * for efficient reuse. It interacts with FreeType for font rendering and HarfBuzz
 * (if enabled) for text shaping.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "evas_common_private.h"
#include <assert.h>
#include "evas_font_ot.h"

#ifdef USE_HARFBUZZ
# include <hb.h>
#endif

#include "evas_font_private.h" /* for Frame-Queuing support */

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H /* Freetype2 OS/2 font table. */

extern FT_Library         evas_ft_lib; /**< Global FreeType library instance. */

static int                font_cache_usage = 0; /**< Current total memory usage of the font cache. */
static int                font_cache = 0; /**< Maximum allowed memory usage for the font cache. */
static int                font_dpi_h = 75; /**< Horizontal DPI setting for font rendering. */
static int                font_dpi_v = 75; /**< Vertical DPI setting for font rendering. */

static Eina_Hash   *fonts_src = NULL; /**< Hash table storing loaded RGBA_Font_Source structures, keyed by font name. */
static Eina_Hash   *fonts = NULL; /**< Hash table storing loaded RGBA_Font_Int structures (font instances), keyed by a composite of source name, size, and rendering flags. */
static Eina_List   *fonts_lru = NULL; /**< LRU list of RGBA_Font_Int structures that have no references and are candidates for eviction from the cache. */
static Eina_Inlist *fonts_use_lru = NULL; /**< LRU list of RGBA_Font_Int structures currently in use. */
static int          fonts_use_usage = 0; /**< Total memory usage of fonts currently in use. */

/**
 * @brief Clears cached glyph data and resets usage for a font instance.
 *
 * This function is typically called when font hinting changes or when a font
 * instance is being prepared for removal from active use. It frees the
 * associated Fash (glyph cache) if the reference count is low enough and
 * updates usage statistics.
 *
 * @param fi The font instance (RGBA_Font_Int) to clear.
 */
static void _evas_common_font_int_clear(RGBA_Font_Int *fi);

/**
 * @brief Compares two RGBA_Font_Int structures for hashing and equality checks.
 *
 * Comparison is based on font source name, size, wanted rendering flags,
 * and bitmap scalability.
 *
 * @param k1 Pointer to the first RGBA_Font_Int.
 * @param k1_length Unused.
 * @param k2 Pointer to the second RGBA_Font_Int.
 * @param k2_length Unused.
 * @return An integer less than, equal to, or greater than zero if k1 is found,
 *         respectively, to be less than, to match, or be greater than k2.
 */
static int
_evas_font_cache_int_cmp(const RGBA_Font_Int *k1, int k1_length EINA_UNUSED,
			 const RGBA_Font_Int *k2, int k2_length EINA_UNUSED)
{
   /* RGBA_Font_Source->name is a stringshare */
   if (k1->src->name == k2->src->name)
     {
        if (k1->size == k2->size)
          {
             if (k1->wanted_rend == k2->wanted_rend)
               return k1->bitmap_scalable - k2->bitmap_scalable;
             else
               return k1->wanted_rend - k2->wanted_rend;
          }
        else
          {
             return k1->size - k2->size;
          }
     }
   return strcmp(k1->src->name, k2->src->name);
}

/**
 * @brief Generates a hash value for an RGBA_Font_Int structure.
 *
 * The hash is computed based on the font source name, size, wanted rendering flags,
 * and bitmap scalability. This is used for storing and retrieving font instances
 * in a hash table.
 *
 * @param key Pointer to the RGBA_Font_Int to hash.
 * @param key_length Unused.
 * @return The computed hash value.
 */
static int
_evas_font_cache_int_hash(const RGBA_Font_Int *key, int key_length EINA_UNUSED)
{
   int hash;
   unsigned int wanted_rend = key->wanted_rend;
   hash = eina_hash_djb2(key->src->name, eina_stringshare_strlen(key->src->name) + 1);
   hash ^= eina_hash_int32(&key->size, sizeof (int));
   hash ^= eina_hash_int32(&wanted_rend, sizeof (int));
   hash ^= eina_hash_int32(&key->bitmap_scalable, sizeof (int));
   return hash;
}

/**
 * @brief Frees an RGBA_Font_Source structure and its associated resources.
 *
 * This includes releasing the FreeType face, stringshared name and file,
 * and the structure itself.
 *
 * @param fs The font source (RGBA_Font_Source) to free.
 */
static void
_evas_common_font_source_free(RGBA_Font_Source *fs)
{
   FTLOCK();
   FT_Done_Face(fs->ft.face);
   FTUNLOCK();
   if (fs->name) eina_stringshare_del(fs->name);
   if (fs->file) eina_stringshare_del(fs->file);
   free(fs);
}

/**
 * @brief Frees an RGBA_Font_Int structure and its associated resources.
 *
 * This function handles the complete cleanup of a font instance, including
 * its FreeType size object, kerning cache, HarfBuzz font (if used),
 * and the underlying font source if its reference count drops to zero.
 * It also updates cache usage statistics and removes the font instance
 * from LRU lists.
 *
 * @param fi The font instance (RGBA_Font_Int) to free.
 */
static void
_evas_common_font_int_free(RGBA_Font_Int *fi)
{
   FTLOCK();
   FT_Done_Size(fi->ft.size);
   FTUNLOCK();

   evas_common_font_int_modify_cache_by(fi, -1);
   _evas_common_font_int_clear(fi);
   eina_hash_free(fi->kerning);

   LKD(fi->ft_mutex);
#ifdef USE_HARFBUZZ
   hb_font_destroy(fi->ft.hb_font);
#endif
   evas_common_font_source_free(fi->src);
   if (fi->references <= 0) fonts_lru = eina_list_remove(fonts_lru, fi);
   if (fi->fash) fi->fash->freeme(fi->fash);
   if (fi->inuse)
    {
      fonts_use_lru = eina_inlist_remove(fonts_use_lru, EINA_INLIST_GET(fi));
      fi->inuse = 0;
      fonts_use_usage -= fi->usage;
      fi->usage = 0;
    }
   free(fi);
}

/**
 * @brief Initializes the font loading system.
 *
 * Sets up the hash tables used for caching font sources and font instances.
 * This function must be called before any other font loading functions.
 */
void
evas_common_font_load_init(void)
{
   fonts_src = eina_hash_string_small_new(EINA_FREE_CB(_evas_common_font_source_free));
   fonts = eina_hash_new(NULL,
			 EINA_KEY_CMP(_evas_font_cache_int_cmp),
			 EINA_KEY_HASH(_evas_font_cache_int_hash),
			 EINA_FREE_CB(_evas_common_font_int_free),
			 5);
}

/**
 * @brief Shuts down the font loading system.
 *
 * Frees all cached font sources and font instances, and releases the
 * hash tables. This function should be called during application cleanup.
 */
void
evas_common_font_load_shutdown(void)
{
   eina_hash_free(fonts);
   fonts = NULL;
   eina_hash_free(fonts_src);
   fonts_src = NULL;
}

/**
 * @brief Sets the DPI (Dots Per Inch) for font rendering.
 *
 * This affects how font sizes are interpreted and rendered.
 *
 * @param dpi_h Horizontal DPI.
 * @param dpi_v Vertical DPI. If less than or equal to 0, it's set to dpi_h.
 */
EVAS_API void
evas_common_font_dpi_set(int dpi_h, int dpi_v)
{
   if (dpi_v <= 0) dpi_v = dpi_h;
   font_dpi_h = dpi_h;
   font_dpi_v = dpi_v;
}

/**
 * @brief Loads a font source from memory.
 *
 * Creates an RGBA_Font_Source from a raw memory buffer containing font data.
 * The provided name is used as a unique identifier for this font source.
 * The memory buffer is copied internally.
 *
 * @param name A unique name for this font source (e.g., "MyEmbeddedFont").
 * @param data Pointer to the font data in memory.
 * @param data_size Size of the font data in bytes.
 * @return A pointer to the newly created RGBA_Font_Source on success,
 *         or NULL on failure (e.g., invalid font data, memory allocation error).
 */
EVAS_API RGBA_Font_Source *
evas_common_font_source_memory_load(const char *name, const void *data, int data_size)
{
   int error;
   RGBA_Font_Source *fs;

   assert(name != NULL);
   if (data_size <= 0) return NULL;
   fs = calloc(1, sizeof(RGBA_Font_Source) + data_size);
   if (!fs) return NULL;
   fs->data = ((unsigned char *)fs) + sizeof(RGBA_Font_Source);
   fs->data_size = data_size;
   fs->current_size = 0;
   memcpy(fs->data, data, data_size);
   FTLOCK();
   error = FT_New_Memory_Face(evas_ft_lib, fs->data, fs->data_size, 0, &(fs->ft.face));
   FTUNLOCK();
   if (error)
     {
	free(fs);
	return NULL;
     }
   fs->name = eina_stringshare_add(name);
   fs->file = NULL;
   FTLOCK();
   error = FT_Select_Charmap(fs->ft.face, ft_encoding_unicode);

   if (error)
     {
        FT_Done_Face(fs->ft.face);
        FTUNLOCK();
        fs->ft.face = NULL;
        free(fs);
        return NULL;
     }

   FTUNLOCK();
   fs->ft.orig_upem = fs->ft.face->units_per_EM;
   fs->references = 1;
   eina_hash_direct_add(fonts_src, fs->name, fs);
   return fs;
}

/**
 * @brief Creates a font source structure for a font file.
 *
 * This function initializes an RGBA_Font_Source structure for a font specified
 * by its file path (name). It does not immediately load the font data from disk;
 * the actual loading (FT_New_Face) is deferred until
 * evas_common_font_source_load_complete() is called, typically when a specific
 * font size (RGBA_Font_Int) is requested.
 *
 * @param name The file path of the font. This string is stringshared.
 * @return A pointer to the newly created RGBA_Font_Source on success,
 *         or NULL on memory allocation failure. The returned source will have
 *         its ft.face as NULL initially.
 */
EVAS_API RGBA_Font_Source *
evas_common_font_source_load(const char *name)
{
   RGBA_Font_Source *fs;

   assert(name != NULL);
   fs = calloc(1, sizeof(RGBA_Font_Source));
   if (!fs) return NULL;
   fs->data = NULL;
   fs->data_size = 0;
   fs->current_size = 0;
   fs->ft.face = NULL;
   fs->name = eina_stringshare_add(name);
   fs->file = eina_stringshare_ref(fs->name);
   fs->ft.orig_upem = 0;
   fs->references = 1;
   eina_hash_direct_add(fonts_src, fs->name, fs);
   return fs;
}

/**
 * @brief Unloads the FreeType face associated with a font source.
 *
 * This function calls FT_Done_Face to release the FreeType face, effectively
 * unloading the font data from memory if it was loaded. The RGBA_Font_Source
 * structure itself is not freed. This is typically used to reduce memory
 * when a font source is not actively being used to render text.
 *
 * @param fs The font source (RGBA_Font_Source) whose FreeType face is to be unloaded.
 */
void
evas_common_font_source_unload(RGBA_Font_Source *fs)
{
   FTLOCK();
   FT_Done_Face(fs->ft.face);
   fs->ft.face = NULL;
   FTUNLOCK();
}

/**
 * @brief Reloads a font source if it's not already loaded.
 *
 * If the font source's FreeType face (fs->ft.face) is NULL, this function
 * attempts to load it. If the source was originally from memory (fs->data is not NULL),
 * it uses FT_New_Memory_Face. Otherwise, it calls
 * evas_common_font_source_load_complete() to load from file.
 *
 * @param fs The font source (RGBA_Font_Source) to reload.
 */
void
evas_common_font_source_reload(RGBA_Font_Source *fs)
{
  if (fs->ft.face) return;
  if (fs->data)
    {
      int error;

      FTLOCK();
      error = FT_New_Memory_Face(evas_ft_lib, fs->data, fs->data_size, 0, &(fs->ft.face));
      FTUNLOCK();
      if (error) return;
      FTLOCK();
      error = FT_Select_Charmap(fs->ft.face, ft_encoding_unicode);
      if (error)
        {
          FT_Done_Face(fs->ft.face);
          fs->ft.face = NULL;
        }
      FTUNLOCK();
    }
  else
    evas_common_font_source_load_complete(fs);
}

/**
 * @brief Completes the loading of a font source from a file.
 *
 * This function performs the actual loading of the font face from the file
 * specified in `fs->file` using `FT_New_Face`. It also selects the Unicode
 * charmap. This is typically called when a font instance (RGBA_Font_Int)
 * is first created from a file-based font source.
 *
 * @param fs The font source (RGBA_Font_Source) to load. `fs->file` must be
 *           a valid path to a font file.
 * @return 0 on success, or a FreeType error code on failure.
 *         If successful, `fs->ft.face` will be populated.
 */
EVAS_API int
evas_common_font_source_load_complete(RGBA_Font_Source *fs)
{
   int error;

   FTLOCK();
   error = FT_New_Face(evas_ft_lib, fs->file, 0, &(fs->ft.face));
   if (error)
     {
        FTUNLOCK();
	fs->ft.face = NULL;
	return error;
     }
   error = FT_Select_Charmap(fs->ft.face, ft_encoding_unicode);
   if (error)
     {
	FT_Done_Face(fs->ft.face);
        FTUNLOCK();
	fs->ft.face = NULL;
	return error;
     }
   FTUNLOCK();
   fs->ft.orig_upem = fs->ft.face->units_per_EM;
   return error;
}

/**
 * @brief Finds an already loaded font source by its name.
 *
 * Searches the cache of loaded font sources (`fonts_src`) for a source
 * matching the given name. If found, its reference count is incremented.
 *
 * @param name The name of the font source to find (typically a file path or
 *             the unique name given to a memory-loaded font).
 * @return A pointer to the found RGBA_Font_Source if it exists,
 *         otherwise NULL.
 */
EVAS_API RGBA_Font_Source *
evas_common_font_source_find(const char *name)
{
   RGBA_Font_Source *fs;

   if (!name) return NULL;
   fs = eina_hash_find(fonts_src, name);
   if (fs)
     {
	fs->references++;
	return fs;
     }
   return NULL;
}

/**
 * @brief Decrements the reference count of a font source.
 *
 * If the reference count drops to zero, the font source is removed from
 * the global cache (`fonts_src`) and freed via `_evas_common_font_source_free`.
 *
 * @param fs The font source (RGBA_Font_Source) to release.
 */
EVAS_API void
evas_common_font_source_free(RGBA_Font_Source *fs)
{
   fs->references--;
   fs->current_size = 0;
   if (fs->references > 0) return;
   eina_hash_del(fonts_src, fs->name, fs);
}

/**
 * @brief Ensures that all font instances within an RGBA_Font are active and correctly sized.
 *
 * Iterates through each RGBA_Font_Int in the RGBA_Font. If an instance's
 * current FreeType size (fi->src->current_size) doesn't match its desired
 * size (fi->size), it reloads the font source (if necessary) and activates
 * the correct FreeType size object. This is important for contexts where
 * FreeType might switch active sizes.
 *
 * @param fn The RGBA_Font whose instances are to be checked and activated.
 */
EVAS_API void
evas_common_font_size_use(RGBA_Font *fn)
{
   RGBA_Font_Int *fi;
   Eina_List *l;

   EINA_LIST_FOREACH(fn->fonts, l, fi)
     {
	if (fi->src->current_size != fi->size)
	  {
             evas_common_font_source_reload(fi->src);
             FTLOCK();
	     FT_Activate_Size(fi->ft.size);
             FTUNLOCK();
	     fi->src->current_size = fi->size;
	  }
     }
}

/**
 * @brief Compares two integer pairs for kerning cache.
 *
 * Used as a key comparison function for the Eina_Hash that stores kerning pairs.
 * The keys are arrays of two integers, typically representing `(left_glyph_index, right_glyph_index)`.
 *
 * @param key1 Pointer to the first integer array (pair).
 * @param key1_length Unused.
 * @param key2 Pointer to the second integer array (pair).
 * @param key2_length Unused.
 * @return 0 if pairs are equal, a negative value if key1 < key2,
 *         or a positive value if key1 > key2.
 */
static int
_evas_common_font_double_int_cmp(const int *key1, EINA_UNUSED int key1_length,
				 const int *key2, EINA_UNUSED int key2_length)
{
   if (key1[0] - key2[0] == 0) return key1[1] - key2[1];
   return key1[0] - key2[0];
}

/**
 * @brief Hashes an integer pair for kerning cache.
 *
 * Used as a key hash function for the Eina_Hash that stores kerning pairs.
 * The keys are arrays of two unsigned integers.
 *
 * @param key Pointer to the unsigned integer array (pair) to hash.
 * @param key_length The number of elements in the key array (should be 2).
 * @return The computed hash value.
 */
static int
_evas_common_font_double_int_hash(const unsigned int key[2], int key_length)
{
   return
    eina_hash_int32(&key[0], key_length) ^
    eina_hash_int32(&key[1], key_length);
}

/**
 * @brief Initializes the cache-related members of an RGBA_Font_Int.
 *
 * Specifically, this creates the Eina_Hash table used for storing kerning pairs
 * for this font instance and initializes its FreeType mutex.
 *
 * @param fi The font instance (RGBA_Font_Int) to initialize.
 */
static void
_evas_common_font_int_cache_init(RGBA_Font_Int *fi)
{
   /* Add some font kerning cache. */
  fi->kerning = eina_hash_new(NULL,
			       EINA_KEY_CMP(_evas_common_font_double_int_cmp),
			       EINA_KEY_HASH(_evas_common_font_double_int_hash),
			       free, 3);
   LKI(fi->ft_mutex);
}

/**
 * @brief Loads a specific font instance (size and style) from memory data.
 *
 * This function first attempts to find an existing matching font instance in the cache.
 * If not found, it creates a new RGBA_Font_Int. It ensures the underlying
 * font source (RGBA_Font_Source) is loaded from the provided memory data if it
 * doesn't already exist.
 *
 * The `source` and `name` parameters are combined to create a unique identifier
 * for the font source, typically `source/name`.
 *
 * @param source A string identifying the origin or group of the font (e.g., "application_fonts").
 * @param name The specific name of the font within that source (e.g., "MyEmbeddedFont.ttf").
 * @param size The desired font size in points.
 * @param data Pointer to the raw font data in memory.
 * @param data_size Size of the font data in bytes.
 * @param wanted_rend Rendering flags (e.g., FONT_REND_SLANT, FONT_REND_WEIGHT).
 * @param bitmap_scalable Bitmap scaling preference.
 * @return A pointer to the RGBA_Font_Int on success, or NULL on failure.
 *         The returned font instance has its reference count incremented.
 */
EVAS_API RGBA_Font_Int *
evas_common_font_int_memory_load(const char *source, const char *name, int size, const void *data, int data_size, Font_Rend_Flags wanted_rend, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   RGBA_Font_Int *fi;
   char fake_name[PATH_MAX];

   eina_file_path_join(fake_name, sizeof(fake_name), source, name);
   fi = evas_common_font_int_find(fake_name, size, wanted_rend, bitmap_scalable);
   if (fi)
     {
        return fi;
     }
   fi = calloc(1, sizeof(RGBA_Font_Int));
   if (!fi)
     {
        return NULL;
     }
   fi->src = evas_common_font_source_find(fake_name);
   if (!fi->src)
    fi->src = evas_common_font_source_memory_load(fake_name, data, data_size);
   if (!fi->src)
     {
        free(fi);
        return NULL;
     }
   fi->size = size;
   fi->bitmap_scalable = bitmap_scalable;
   _evas_common_font_int_cache_init(fi);
   fi = evas_common_font_int_load_init(fi);
   evas_common_font_int_load_complete(fi);
   return fi;
}

/**
 * @brief Helper function to check if a path points to a regular file.
 *
 * Uses stat() to determine if the given path exists and is a regular file.
 *
 * @param path The file path to check.
 * @return 1 if the path is a regular file, 0 otherwise.
 */
static int
_file_path_is_file_helper(const char *path)
{
   struct stat st;

   if (stat(path, &st) == -1) return 0;
   if (S_ISREG(st.st_mode)) return 1;
   return 0;
}

/**
 * @brief Loads a specific font instance (size and style) from a file path.
 *
 * This function first attempts to find an existing matching font instance in the cache
 * using `evas_common_font_int_find`. If not found, it creates a new RGBA_Font_Int.
 * It ensures the underlying font source (RGBA_Font_Source) is loaded from the
 * specified file path if it doesn't already exist and the path is a valid file.
 *
 * The actual FreeType face loading and size setting for the new instance is
 * typically deferred or handled by `evas_common_font_int_load_init` and
 * `evas_common_font_int_load_complete`.
 *
 * @param name The file path of the font.
 * @param size The desired font size in points.
 * @param wanted_rend Rendering flags (e.g., FONT_REND_SLANT, FONT_REND_WEIGHT).
 * @param bitmap_scalable Bitmap scaling preference.
 * @return A pointer to the RGBA_Font_Int on success, or NULL on failure.
 *         The returned font instance has its reference count incremented if found
 *         or initialized to 1 if newly created.
 */
EVAS_API RGBA_Font_Int *
evas_common_font_int_load(const char *name, int size,
                          Font_Rend_Flags wanted_rend,
						  Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   RGBA_Font_Int *fi;

   fi = evas_common_font_int_find(name, size, wanted_rend, bitmap_scalable);
   if (fi) return fi;
   fi = calloc(1, sizeof(RGBA_Font_Int));
   if (!fi) return NULL;
   fi->src = evas_common_font_source_find(name);
   if (!fi->src && _file_path_is_file_helper(name))
     fi->src = evas_common_font_source_load(name);

   if (!fi->src)
     {
	free(fi);
	return NULL;
     }
   fi->size = size;
   fi->wanted_rend = wanted_rend;
   fi->bitmap_scalable = bitmap_scalable;
   _evas_common_font_int_cache_init(fi);
   fi = evas_common_font_int_load_init(fi);
//   evas_common_font_int_load_complete(fi);
   return fi;
}

/**
 * @brief Initializes a newly created RGBA_Font_Int structure.
 *
 * Sets the initial reference count to 1, sets `fi->ft.size` to NULL (actual
 * FreeType size object creation is deferred), and adds the font instance to
 * the global `fonts` hash table.
 *
 * @param fi The RGBA_Font_Int structure to initialize. It is assumed that
 *           `fi->src`, `fi->size`, `fi->wanted_rend`, and `fi->bitmap_scalable`
 *           have already been set.
 * @return The initialized RGBA_Font_Int (same as the input `fi`).
 */
EVAS_API RGBA_Font_Int *
evas_common_font_int_load_init(RGBA_Font_Int *fi)
{
   fi->ft.size = NULL;
   fi->references = 1;
   eina_hash_direct_add(fonts, fi, fi);
   return fi;
}

/**
 * @brief Completes the loading process for an RGBA_Font_Int.
 *
 * This function performs the necessary FreeType operations to set up the font
 * instance for rendering at the specified size. This includes:
 * - Creating a new FreeType size object (FT_New_Size).
 * - Setting the character size using FT_Set_Char_Size or FT_Set_Pixel_Sizes.
 * - Handling fixed-size (bitmap) fonts by selecting the best available strike.
 * - Calculating scaling factors if a bitmap font is scaled.
 * - Determining font metrics like max height (ascender + descender).
 * - Setting runtime rendering flags (e.g., for software bold/italic if the
 *   font doesn't natively support them).
 *
 * @param fi The RGBA_Font_Int to complete loading for. `fi->src->ft.face` must
 *           be a valid, loaded FreeType face. `fi->size` specifies the target size.
 * @return The RGBA_Font_Int (same as input `fi`) after completion, or potentially
 *         after attempting to recover from errors by choosing alternative sizes.
 */
EVAS_API RGBA_Font_Int *
evas_common_font_int_load_complete(RGBA_Font_Int *fi)
{
   int val, dv;
   int ret;
   int error;

   FTLOCK();
   error = FT_New_Size(fi->src->ft.face, &(fi->ft.size));
   if (!error)
     {
	FT_Activate_Size(fi->ft.size);
     }
   fi->real_size = fi->size * 64;
   fi->scale_factor = 1.0;
   error = FT_Set_Char_Size(fi->src->ft.face, 0, fi->real_size, font_dpi_h, font_dpi_v);
   if (error)
     error = FT_Set_Pixel_Sizes(fi->src->ft.face, 0, fi->real_size);
   FTUNLOCK();
   if (error)
     {
	int i, maxd = 0x7fffffff;
	int chosen_size = 0;
	int chosen_size2 = 0;
        FT_Int strike_index = 0;

	for (i = 0; i < fi->src->ft.face->num_fixed_sizes; i++)
	  {
	     int s, cd;

	     s = fi->src->ft.face->available_sizes[i].size;
	     cd = chosen_size - fi->real_size;
	     if (cd < 0) cd = -cd;
             if (cd < maxd)
               {
                  maxd = cd;
		  chosen_size = s;
		  chosen_size2 = fi->src->ft.face->available_sizes[i].y_ppem;
                  strike_index = (FT_Int)i;
                  if (maxd == 0) break;
	       }
	  }
	fi->real_size = chosen_size;
        FTLOCK();

        if (FT_HAS_FIXED_SIZES(fi->src->ft.face))
          {
             error = FT_Select_Size(fi->src->ft.face, strike_index);

             if (!error)
               {
                  if (FT_HAS_COLOR(fi->src->ft.face) &&
                      fi->bitmap_scalable & EFL_TEXT_FONT_BITMAP_SCALABLE_COLOR)
                    {
                       if (fi->real_size > 0)
                         {
                           fi->scale_factor = (double)fi->size * 64.0 / (double)fi->real_size;
                           fi->is_resized = EINA_FALSE;
                           if ((fi->scale_factor <= 0.9) && (fi->scale_factor != 0))
                             fi->is_resized = EINA_TRUE;
                         }
                       else
                         fi->scale_factor = 64.0;
                    }
               }
          }
        else
          {
             error = FT_Set_Pixel_Sizes(fi->src->ft.face, 0, fi->real_size);
          }

        FTUNLOCK();
	if (error)
	  {
             error = FT_Set_Char_Size(fi->src->ft.face, 0, fi->real_size, font_dpi_h, font_dpi_v);
             if (error)
               {
                  /* hack around broken fonts */
                  fi->real_size = (chosen_size2 / 64) * 60;
                  error = FT_Set_Char_Size(fi->src->ft.face, 0, fi->real_size, font_dpi_h, font_dpi_v);
                  if (error)
                    {
                       /* couldn't choose the size anyway... what now? */
                    }
               }
	  }
     }
   fi->src->current_size = 0;
   fi->max_h = 0;
   val = (int)fi->src->ft.face->bbox.yMax;
   if (fi->src->ft.face->units_per_EM != 0)
     {
        dv = (fi->src->ft.orig_upem * 2048) / fi->src->ft.face->units_per_EM;
        ret = FONT_METRIC_CONV(val, dv, fi->src->ft.face->size->metrics.y_scale);
     }
   else
     {
        if ((fi->src->ft.face->bbox.yMax == 0) &&
            (fi->src->ft.face->bbox.yMin == 0))
          ret = FONT_METRIC_ROUNDUP((int)fi->ft.size->metrics.ascender);
        else
          ret = val;
     }
   fi->max_h += ret;
   val = -(int)fi->src->ft.face->bbox.yMin;
   if (fi->src->ft.face->units_per_EM != 0)
     {
        dv = (fi->src->ft.orig_upem * 2048) / fi->src->ft.face->units_per_EM;
        ret = FONT_METRIC_CONV(val, dv, fi->src->ft.face->size->metrics.y_scale);
     }
   else
     {
        if ((fi->src->ft.face->bbox.yMax == 0) &&
            (fi->src->ft.face->bbox.yMin == 0))
          ret = FONT_METRIC_ROUNDUP(-(int)fi->ft.size->metrics.descender);
        else
          ret = val;
     }
   fi->max_h += ret;

   /* If the loaded font doesn't match with wanted_rend value requested by
    * textobject and textblock, Set the runtime_rend value as FONT_REND_SLANT
    * or FONT_REND_WEIGHT for software rendering. */
   fi->runtime_rend = FONT_REND_REGULAR;
   if ((fi->wanted_rend & FONT_REND_SLANT) &&
       !(fi->src->ft.face->style_flags & FT_STYLE_FLAG_ITALIC))
      fi->runtime_rend |= FONT_REND_SLANT;

   if ((fi->wanted_rend & FONT_REND_WEIGHT) &&
       !(fi->src->ft.face->style_flags & FT_STYLE_FLAG_BOLD))
     {
        TT_OS2 *tt_os2 = FT_Get_Sfnt_Table(fi->src->ft.face, ft_sfnt_os2);
        if (!tt_os2 || (tt_os2->usWeightClass < 600))
          {
             fi->runtime_rend |= FONT_REND_WEIGHT;
          }
     }

   return fi;
}

/**
 * @brief Loads a font from memory data and returns it as an RGBA_Font.
 *
 * This is a convenience function that wraps `evas_common_font_int_memory_load`
 * to create an RGBA_Font_Int, and then encapsulates this RGBA_Font_Int within
 * a new RGBA_Font structure. The RGBA_Font acts as a container for one or
 * more RGBA_Font_Int instances (though in this case, just one).
 *
 * @param source A string identifying the origin or group of the font.
 * @param name The specific name of the font within that source.
 * @param size The desired font size in points.
 * @param data Pointer to the raw font data in memory.
 * @param data_size Size of the font data in bytes.
 * @param wanted_rend Rendering flags.
 * @param bitmap_scalable Bitmap scaling preference.
 * @return A pointer to the new RGBA_Font on success, or NULL on failure.
 *         The returned RGBA_Font has its reference count initialized to 1.
 */
EVAS_API RGBA_Font *
evas_common_font_memory_load(const char *source, const char *name, int size, const void *data, int data_size, Font_Rend_Flags wanted_rend, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   RGBA_Font *fn;
   RGBA_Font_Int *fi;

   fi = evas_common_font_int_memory_load(source, name, size, data, data_size,
                                         wanted_rend, bitmap_scalable);
   if (!fi) return NULL;
   fn = calloc(1, sizeof(RGBA_Font));
   if (!fn)
     {
        evas_common_font_int_unref(fi);
	return NULL;
     }
   fn->fonts = eina_list_append(fn->fonts, fi);
   fn->hinting = FONT_BYTECODE_HINT;
   fi->hinting = fn->hinting;
   fn->references = 1;
   LKI(fn->lock);
   if (fi->inuse) evas_common_font_int_promote(fi);
   else
    {
      fi->inuse = 1;
      fonts_use_lru = eina_inlist_prepend(fonts_use_lru, EINA_INLIST_GET(fi));
    }
   return fn;
}


//ZZZ: font struct looks like:
// fn->(fi, fi, fi, ...)
//   fi->fs

/**
 * @brief Loads a font from a file path and returns it as an RGBA_Font.
 *
 * This function first loads (or finds an existing) RGBA_Font_Int using
 * `evas_common_font_int_load`. If the font instance is newly loaded (i.e.,
 * `fi->ft.size` is NULL), it ensures the underlying font source is fully loaded
 * via `evas_common_font_source_load_complete` and then completes the font
 * instance loading with `evas_common_font_int_load_complete`.
 *
 * The resulting RGBA_Font_Int is then wrapped in a new RGBA_Font structure.
 *
 * @param name The file path of the font.
 * @param size The desired font size in points.
 * @param wanted_rend Rendering flags.
 * @param bitmap_scalable Bitmap scaling preference.
 * @return A pointer to the new RGBA_Font on success, or NULL on failure.
 *         The returned RGBA_Font has its reference count initialized to 1.
 */
EVAS_API RGBA_Font *
evas_common_font_load(const char *name, int size, Font_Rend_Flags wanted_rend, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   RGBA_Font *fn;
   RGBA_Font_Int *fi;

   fi = evas_common_font_int_load(name, size, wanted_rend, bitmap_scalable);
   if (!fi) return NULL;
   /* First font, complete load */
   if (!fi->ft.size)
     {
	if (!fi->src->ft.face)
	  {
	     if (evas_common_font_source_load_complete(fi->src))
	       {
                  evas_common_font_int_unref(fi);
		  return NULL;
	       }
	  }
	evas_common_font_int_load_complete(fi);
     }
   fn = calloc(1, sizeof(RGBA_Font));
   if (!fn)
     {
        evas_common_font_int_unref(fi);
	return NULL;
     }

   fn->fonts = eina_list_append(fn->fonts, fi);
   fn->hinting = FONT_BYTECODE_HINT;
   fi->hinting = fn->hinting;
   fn->references = 1;
   LKI(fn->lock);
   if (fi->inuse) evas_common_font_int_promote(fi);
   else
    {
      fi->inuse = 1;
      fonts_use_lru = eina_inlist_prepend(fonts_use_lru, EINA_INLIST_GET(fi));
    }
   return fn;
}

/**
 * @brief Adds a font instance (loaded from a file) to an existing RGBA_Font.
 *
 * This allows creating composite fonts (font fallbacks) by adding multiple
 * font instances (potentially from different font files or with different settings)
 * to a single RGBA_Font object. The new font instance is loaded using
 * `evas_common_font_int_load`.
 *
 * @param fn The RGBA_Font to which the new font instance will be added.
 *           If NULL, the function returns NULL.
 * @param name The file path of the font for the new instance.
 * @param size The desired font size in points for the new instance.
 * @param wanted_rend Rendering flags for the new instance.
 * @param bitmap_scalable Bitmap scaling preference for the new instance.
 * @return The modified RGBA_Font `fn` on success, or NULL if `fn` was NULL
 *         or if the new font instance could not be loaded.
 */
EVAS_API RGBA_Font *
evas_common_font_add(RGBA_Font *fn, const char *name, int size, Font_Rend_Flags wanted_rend, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   RGBA_Font_Int *fi;

   if (!fn) return NULL;
   fi = evas_common_font_int_load(name, size, wanted_rend, bitmap_scalable);
   if (fi)
     {
	fn->fonts = eina_list_append(fn->fonts, fi);
	fi->hinting = fn->hinting;
        if (fi->inuse) evas_common_font_int_promote(fi);
        else
         {
           fi->inuse = 1;
           fonts_use_lru = eina_inlist_prepend(fonts_use_lru, EINA_INLIST_GET(fi));
         }
	return fn;
     }
   return NULL;
}

/**
 * @brief Adds a font instance (loaded from memory) to an existing RGBA_Font.
 *
 * Similar to `evas_common_font_add`, but loads the new font instance from
 * memory data using `evas_common_font_int_memory_load`. This allows adding
 * embedded fonts or dynamically generated font data as fallbacks.
 *
 * @param fn The RGBA_Font to which the new font instance will be added.
 *           If NULL, the function returns NULL.
 * @param source A string identifying the origin or group of the font for the new instance.
 * @param name The specific name of the font within that source for the new instance.
 * @param size The desired font size in points for the new instance.
 * @param data Pointer to the raw font data in memory for the new instance.
 * @param data_size Size of the font data in bytes for the new instance.
 * @param wanted_rend Rendering flags for the new instance.
 * @param bitmap_scalable Bitmap scaling preference for the new instance.
 * @return The modified RGBA_Font `fn` on success, or NULL if `fn` was NULL
 *         or if the new font instance could not be loaded.
 */
EVAS_API RGBA_Font *
evas_common_font_memory_add(RGBA_Font *fn, const char *source, const char *name, int size, const void *data, int data_size, Font_Rend_Flags wanted_rend, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   RGBA_Font_Int *fi;

   if (!fn)
      return NULL;
   fi = evas_common_font_int_memory_load(source, name, size, data, data_size, wanted_rend, bitmap_scalable);
   if (fi)
     {
	fn->fonts = eina_list_append(fn->fonts, fi);
	fi->hinting = fn->hinting;
        if (fi->inuse) evas_common_font_int_promote(fi);
        else
         {
           fi->inuse = 1;
           fonts_use_lru = eina_inlist_prepend(fonts_use_lru, EINA_INLIST_GET(fi));
         }
	return fn;
     }
   return NULL;
}

/**
 * @brief Decrements the reference count of an RGBA_Font_Int.
 *
 * If the reference count drops to zero, the font instance is not immediately
 * freed. Instead, it's added to an LRU list (`fonts_lru`) of candidates for
 * eviction. The `evas_common_font_int_modify_cache_by` function is called to
 * update cache usage (conceptually marking it as "potentially freeable"),
 * and `evas_common_font_flush` is called to potentially trigger actual freeing
 * if cache limits are exceeded.
 *
 * @param fi The font instance (RGBA_Font_Int) to unreference.
 */
EVAS_API void
evas_common_font_int_unref(RGBA_Font_Int *fi)
{
   fi->references--;
   if (fi->references == 0)
     {
        fonts_lru = eina_list_append(fonts_lru, fi);
        evas_common_font_int_modify_cache_by(fi, 1);
        evas_common_font_flush();
     }
}

/**
 * @brief Frees an RGBA_Font structure and unreferences its contained font instances.
 *
 * Decrements the reference count of the RGBA_Font itself. If the count reaches
 * zero, it iterates through all RGBA_Font_Int instances held by `fn->fonts`,
 * calling `evas_common_font_int_unref` for each. This may lead to those
 * instances being cached or freed depending on their own reference counts and
 * cache policies. Finally, the RGBA_Font structure `fn` itself is freed.
 *
 * @param fn The RGBA_Font to free. If NULL, the function does nothing.
 */
EVAS_API void
evas_common_font_free(RGBA_Font *fn)
{
   Eina_List *l;
   RGBA_Font_Int *fi;

   if (!fn) return;
   fn->references--;
   if (fn->references > 0) return;
   EINA_LIST_FOREACH(fn->fonts, l, fi)
     evas_common_font_int_unref(fi);
   evas_common_font_flush();
   eina_list_free(fn->fonts);
   if (fn->fash)
     {
        fn->fash->freeme(fn->fash);
        fn->fash = NULL;
     }
   LKD(fn->lock);
   free(fn);
}

/**
 * @brief Sets the hinting mode for an RGBA_Font and all its contained instances.
 *
 * Updates the `hinting` flag for the given RGBA_Font `fn`. Then, iterates
 * through all RGBA_Font_Int instances within `fn`. If an instance's current
 * hinting mode differs from the new mode, its cached glyph data is cleared
 * (via `_evas_common_font_int_clear`) because hinting changes invalidate
 * existing rendered glyphs. The instance's hinting mode is then updated.
 *
 * @param fn The RGBA_Font whose hinting mode is to be set. If NULL, the function does nothing.
 * @param hinting The new hinting mode (e.g., FONT_NO_HINT, FONT_AUTO_HINT, FONT_BYTECODE_HINT).
 */
EVAS_API void
evas_common_font_hinting_set(RGBA_Font *fn, Font_Hint_Flags hinting)
{
   Eina_List *l;
   RGBA_Font_Int *fi;

   if (!fn) return;
   fn->hinting = hinting;
   EINA_LIST_FOREACH(fn->fonts, l, fi)
     {
        if (fi->hinting != fn->hinting)
          _evas_common_font_int_clear(fi);
        fi->hinting = fn->hinting;
     }
}

/**
 * @brief Checks if a specific font hinting mode is available/supported.
 *
 * - FONT_NO_HINT and FONT_AUTO_HINT are always considered available.
 * - FONT_BYTECODE_HINT availability depends on the FreeType build and version,
 *   specifically whether the patented TrueType bytecode interpreter is included.
 *   This function queries FreeType to determine this.
 *
 * @param hinting The hinting mode to check.
 * @return EINA_TRUE if the hinting mode is available, EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_common_hinting_available(Font_Hint_Flags hinting)
{
   switch (hinting)
     {
      case FONT_NO_HINT:
      case FONT_AUTO_HINT:
	 /* these two hinting modes are always available */
	 return EINA_TRUE;
      case FONT_BYTECODE_HINT:
	 /* Only use the bytecode interpreter if support for the _patented_
	  * algorithms is available because the free bytecode
	  * interpreter's results are too crappy.
	  *
	  * On freetyp 2.2+, we can ask the library about support for
	  * the patented interpreter. On older versions, we need to use
	  * macros to check for it.
	  */
#if FREETYPE_MINOR >= 2
	 return FT_Get_TrueType_Engine_Type(evas_ft_lib) >=
		FT_TRUETYPE_ENGINE_TYPE_PATENTED;
#else
	 /* we may not rely on TT_CONFIG_OPTION_BYTECODE_INTERPRETER
	  * here to find out whether it's supported.
	  *
	  * so, assume it is. o_O
	  */
	 return EINA_TRUE;
#endif
     }
   /* shouldn't get here - need to add another case statement */
   return EINA_FALSE;
}

/**
 * @brief Loads a font from memory with a specific hinting mode.
 *
 * This is a convenience function that first loads the font from memory using
 * `evas_common_font_memory_load` and then applies the specified hinting mode
 * using `evas_common_font_hinting_set`.
 *
 * @param source A string identifying the origin or group of the font.
 * @param name The specific name of the font within that source.
 * @param size The desired font size in points.
 * @param data Pointer to the raw font data in memory.
 * @param data_size Size of the font data in bytes.
 * @param hinting The desired hinting mode.
 * @param wanted_rend Rendering flags.
 * @param bitmap_scalable Bitmap scaling preference.
 * @return A pointer to the new RGBA_Font on success, or NULL on failure.
 */
EVAS_API RGBA_Font *
evas_common_font_memory_hinting_load(const char *source, const char *name, int size, const void *data, int data_size, Font_Hint_Flags hinting, Font_Rend_Flags wanted_rend, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   RGBA_Font *fn;

   fn = evas_common_font_memory_load(source, name, size, data, data_size, wanted_rend, bitmap_scalable);
   if (fn) evas_common_font_hinting_set(fn, hinting);
   return fn;
}

/**
 * @brief Loads a font from a file with a specific hinting mode.
 *
 * This is a convenience function that first loads the font from a file using
 * `evas_common_font_load` and then applies the specified hinting mode
 * using `evas_common_font_hinting_set`.
 *
 * @param name The file path of the font.
 * @param size The desired font size in points.
 * @param hinting The desired hinting mode.
 * @param wanted_rend Rendering flags.
 * @param bitmap_scalable Bitmap scaling preference.
 * @return A pointer to the new RGBA_Font on success, or NULL on failure.
 */
EVAS_API RGBA_Font *
evas_common_font_hinting_load(const char *name, int size, Font_Hint_Flags hinting, Font_Rend_Flags wanted_rend, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   RGBA_Font *fn;

   fn = evas_common_font_load(name, size, wanted_rend, bitmap_scalable);
   if (fn) evas_common_font_hinting_set(fn, hinting);
   return fn;
}

/**
 * @brief Adds a font instance (loaded from a file) to an RGBA_Font and sets hinting.
 *
 * This function first adds a font instance from a file to the given RGBA_Font `fn`
 * using `evas_common_font_add`. Then, it applies the specified hinting mode to
 * the entire RGBA_Font (including the newly added instance and any existing ones)
 * using `evas_common_font_hinting_set`.
 *
 * @param fn The RGBA_Font to which the new font instance will be added.
 * @param name The file path of the font for the new instance.
 * @param size The desired font size in points for the new instance.
 * @param hinting The desired hinting mode for the entire RGBA_Font.
 * @param wanted_rend Rendering flags for the new instance.
 * @param bitmap_scalable Bitmap scaling preference for the new instance.
 * @return The modified RGBA_Font `fn` on success, or NULL if `fn` was NULL
 *         or if the new font instance could not be added.
 */
EVAS_API RGBA_Font *
evas_common_font_hinting_add(RGBA_Font *fn, const char *name, int size, Font_Hint_Flags hinting, Font_Rend_Flags wanted_rend, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   fn = evas_common_font_add(fn, name, size, wanted_rend, bitmap_scalable);
   if (fn) evas_common_font_hinting_set(fn, hinting);
   return fn;
}

/**
 * @brief Adds a font instance (loaded from memory) to an RGBA_Font and sets hinting.
 *
 * This function first adds a font instance from memory data to the given RGBA_Font `fn`
 * using `evas_common_font_memory_add`. Then, it applies the specified hinting mode
 * to the entire RGBA_Font (including the newly added instance and any existing ones)
 * using `evas_common_font_hinting_set`.
 *
 * @param fn The RGBA_Font to which the new font instance will be added.
 * @param source A string identifying the origin or group of the font for the new instance.
 * @param name The specific name of the font within that source for the new instance.
 * @param size The desired font size in points for the new instance.
 * @param data Pointer to the raw font data in memory for the new instance.
 * @param data_size Size of the font data in bytes for the new instance.
 * @param hinting The desired hinting mode for the entire RGBA_Font.
 * @param wanted_rend Rendering flags for the new instance.
 * @param bitmap_scalable Bitmap scaling preference for the new instance.
 * @return The modified RGBA_Font `fn` on success, or NULL if `fn` was NULL
 *         or if the new font instance could not be added.
 */
EVAS_API RGBA_Font *
evas_common_font_memory_hinting_add(RGBA_Font *fn, const char *source, const char *name, int size, const void *data, int data_size, Font_Hint_Flags hinting, Font_Rend_Flags wanted_rend, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   fn = evas_common_font_memory_add(fn, source, name, size, data, data_size,
                                    wanted_rend, bitmap_scalable);
   if (fn) evas_common_font_hinting_set(fn, hinting);
   return fn;
}

static void
_evas_common_font_int_clear(RGBA_Font_Int *fi)
{
   LKL(fi->ft_mutex);
   if (!fi->fash)
     {
        LKU(fi->ft_mutex);
        return;
     }
   evas_common_font_int_modify_cache_by(fi, -1);
   if (fi->references <= 1)
     {
        if (fi->fash)
          {
             fi->fash->freeme(fi->fash);
             fi->fash = NULL;
          }
     }
   if (fi->inuse) fonts_use_usage -= fi->usage;
   fi->usage = 0;
   fi->generation++;
   LKU(fi->ft_mutex);
}

/**
 * @brief Callback function used by eina_hash_foreach to clear a single font instance.
 *
 * This function is called for each RGBA_Font_Int in the `fonts` hash table
 * when `evas_common_font_all_clear` is invoked. It simply calls
 * `_evas_common_font_int_clear` on the provided font instance.
 *
 * @param hash Unused.
 * @param key Unused.
 * @param data A pointer to an RGBA_Font_Int.
 * @param fdata Unused.
 * @return Always 1 (EINA_TRUE) to continue iteration.
 */
static Eina_Bool
_evas_common_font_all_clear_cb(const Eina_Hash *hash EINA_UNUSED, const void *key EINA_UNUSED, void *data, void *fdata EINA_UNUSED)
{
   RGBA_Font_Int *fi = data;
   _evas_common_font_int_clear(fi);
   return 1;
}

/**
 * @brief Clears cached glyph data for all loaded font instances.
 *
 * Iterates through all RGBA_Font_Int structures in the global `fonts` hash table
 * and calls `_evas_common_font_int_clear` for each one. This effectively
 * invalidates and clears all rendered glyph caches. This might be used when
 * global rendering parameters change significantly (e.g., DPI).
 */
EVAS_API void
evas_common_font_all_clear(void)
{
   eina_hash_foreach(fonts, _evas_common_font_all_clear_cb, NULL);
}

/**
 * @brief Promotes a font instance in the in-use LRU list.
 * (Currently a no-op, code is commented out)
 *
 * This function was intended to move a recently used RGBA_Font_Int to the
 * front of the `fonts_use_lru` list, marking it as more recently used.
 * However, the implementation is currently commented out, making this function
 * a no-op.
 *
 * @param fi The font instance to promote. Unused due to current implementation.
 */
void
evas_common_font_int_promote(RGBA_Font_Int *fi EINA_UNUSED)
{
  return;
/* unused - keep for reference
  if (fonts_use_lru == (Eina_Inlist *)fi) return;
  if (!fi->inuse) return;
  fonts_use_lru = eina_inlist_remove(fonts_use_lru, EINA_INLIST_GET(fi));
  fonts_use_lru = eina_inlist_prepend(fonts_use_lru, EINA_INLIST_GET(fi));
 */
}

/**
 * @brief Increases the total memory usage counter for fonts currently in use.
 *
 * This function is called to update `fonts_use_usage` when a font instance's
 * glyph cache (Fash) consumes more memory.
 *
 * @param size The amount of memory (in bytes) to add to `fonts_use_usage`.
 */
void
evas_common_font_int_use_increase(int size)
{
  fonts_use_usage += size;
}

/**
 * @brief Trims the memory usage of in-use font instances if it exceeds a threshold.
 * (Currently a no-op, code is commented out)
 *
 * This function was intended to iterate through the least recently used font
 * instances in `fonts_use_lru` and clear their glyph caches (`_evas_common_font_int_clear`)
 * and unload them (`evas_common_font_int_unload`) if the total usage
 * (`fonts_use_usage`) significantly exceeds the configured `font_cache` size.
 * The implementation is currently commented out.
 */
void
evas_common_font_int_use_trim(void)
{
  return;
/* unused - keep for reference
  Eina_Inlist *l;

  if (fonts_use_usage <= (font_cache << 1)) return;
  if (!fonts_use_lru) return;
  l = fonts_use_lru->last;
  while (l)
    {
      RGBA_Font_Int *fi = (RGBA_Font_Int *)l;
      if (fonts_use_usage <= (font_cache << 1)) break;
      // FIXME: del fi->kerning content
      _evas_common_font_int_clear(fi);
      evas_common_font_int_unload(fi);
      evas_common_font_int_promote(fi);
      l = l->prev;
    }
 */
}

/**
 * @brief Unloads a font instance, releasing its FreeType resources.
 * (Currently a no-op, code is commented out)
 *
 * This function was intended to clear the glyph cache (`_evas_common_font_int_clear`),
 * release the FreeType size object (`FT_Done_Size`), and unload the underlying
 * font source (`evas_common_font_source_unload`).
 * The implementation is currently commented out.
 *
 * @param fi The font instance to unload. Unused due to current implementation.
 */
void
evas_common_font_int_unload(RGBA_Font_Int *fi EINA_UNUSED)
{
  return;
/* unused - keep for reference
  if (!fi->src->ft.face) return;
  _evas_common_font_int_clear(fi);
  FT_Done_Size(fi->ft.size);
  fi->ft.size = NULL;
  evas_common_font_source_unload(fi->src);
 */
}

/**
 * @brief Reloads a font instance if its underlying source is not loaded.
 *
 * If the FreeType face (`fi->src->ft.face`) for the font instance's source
 * is not loaded, this function calls `evas_common_font_source_load_complete`
 * to load it. The rest of the original implementation, which would then call
 * `evas_common_font_int_load_complete`, is commented out.
 *
 * @param fi The font instance (RGBA_Font_Int) to reload.
 */
void
evas_common_font_int_reload(RGBA_Font_Int *fi)
{
  if (fi->src->ft.face) return;
  evas_common_font_source_load_complete(fi->src);
  return;
/* unused - keep for reference
  evas_common_font_source_reload(fi->src);
  evas_common_font_int_load_complete(fi);
 */
}

/* when the fi->references == 0 we increase this instead of really deleting
 * we then check if the cache_useage size is larger than allowed
 * !If the cache is NOT too large we dont delete font_int
 * !If the cache is too large we really delete font_int */
/**
 * @brief Modifies the global font cache usage counter.
 *
 * This function adjusts `font_cache_usage` based on an estimated size of the
 * font instance `fi`. The size includes the `RGBA_Font` structure size (though
 * `fi` is an `RGBA_Font_Int`), `fi->usage` (glyph cache size), an estimated
 * `FT_FaceRec` size, and a fudge factor.
 *
 * This is called when a font instance becomes a candidate for eviction (dir = 1,
 * increasing potential freeable space) or is reactivated from the LRU list
 * (dir = -1, decreasing potential freeable space).
 *
 * @param fi The font instance (RGBA_Font_Int) affecting the cache usage.
 * @param dir Multiplier for the size adjustment: 1 when adding to potential
 *            freeable space (e.g., fi->references becomes 0), -1 when removing
 *            (e.g., fi->references becomes > 0 from 0).
 */
EVAS_API void
evas_common_font_int_modify_cache_by(RGBA_Font_Int *fi, int dir)
{
   font_cache_usage += dir * (sizeof(RGBA_Font) + fi->usage +
                              sizeof(FT_FaceRec) + 16384); /* fudge values */
}

/**
 * @brief Gets the current maximum font cache size.
 *
 * @return The maximum allowed memory usage for the font cache in bytes.
 */
EVAS_API int
evas_common_font_cache_get(void)
{
   return font_cache;
}

/**
 * @brief Sets the maximum font cache size.
 *
 * After setting the new cache size, it calls `evas_common_font_flush` to
 * potentially evict fonts if the current usage exceeds the new limit, and
 * `evas_common_font_int_use_trim` (which is currently a no-op) to potentially
 * trim memory from in-use fonts.
 *
 * @param size The new maximum font cache size in bytes.
 */
EVAS_API void
evas_common_font_cache_set(int size)
{
   font_cache = size;
   evas_common_font_flush();
   evas_common_font_int_use_trim();
}

/**
 * @brief Flushes the font cache if its usage exceeds the configured limit.
 *
 * Continuously calls `evas_common_font_flush_last` to remove least recently
 * used font instances (those with zero references) from the cache until the
 * `font_cache_usage` drops below the `font_cache` limit, or until no more
 * fonts can be flushed.
 */
EVAS_API void
evas_common_font_flush(void)
{
   if (font_cache_usage < font_cache) return;
   while (font_cache_usage > font_cache)
     {
        int pfont_cache_usage;

        pfont_cache_usage = font_cache_usage;
        evas_common_font_flush_last();
        if (pfont_cache_usage == font_cache_usage) break;
     }
}

/* We run this when the cache gets larger than allowed size
 * We check cache size each time a fi->references goes to 0
 * PERFORMS: Find font_int(s) with references == 0 and delete them */
/**
 * @brief Flushes the least recently used font instance from the cache.
 *
 * If the `fonts_lru` list (containing font instances with zero references,
 * ordered by least recent use) is not empty, this function takes the first
 * (oldest) RGBA_Font_Int from it, removes it from the list, and then removes
 * it from the main `fonts` hash table. This triggers the EINA_FREE_CB for
 * the hash, which is `_evas_common_font_int_free`, thereby actually freeing
 * the font instance and its resources.
 */
EVAS_API void
evas_common_font_flush_last(void)
{
   RGBA_Font_Int *fi = NULL;

   if (!fonts_lru) return;
   fi = eina_list_data_get(fonts_lru);
   fonts_lru = eina_list_remove_list(fonts_lru, fonts_lru);
   eina_hash_del(fonts, fi, fi);
}

/**
 * @brief Finds an existing RGBA_Font_Int in the cache.
 *
 * Constructs a temporary RGBA_Font_Int key based on the provided name, size,
 * rendering flags, and bitmap scalability. It then searches the global `fonts`
 * hash table for a matching instance.
 *
 * If a match is found:
 * - If its reference count was 0, it means the font was in the LRU list
 *   pending potential eviction. In this case, `evas_common_font_int_modify_cache_by`
 *   is called to decrement `font_cache_usage` (as it's no longer just
 *   "potentially freeable"), and it's removed from `fonts_lru`.
 * - The reference count of the found font instance is incremented.
 *
 * @param name The name of the font source (file path or memory font name).
 * @param size The desired font size.
 * @param wanted_rend The desired rendering flags.
 * @param bitmap_scalable The desired bitmap scalability.
 * @return A pointer to the found RGBA_Font_Int if it exists, otherwise NULL.
 *         If found, its reference count is incremented.
 */
EVAS_API RGBA_Font_Int *
evas_common_font_int_find(const char *name, int size,
                          Font_Rend_Flags wanted_rend,
						  Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   RGBA_Font_Int tmp_fi;
   RGBA_Font_Source tmp_fn;
   RGBA_Font_Int *fi;

   tmp_fn.name = (char*) eina_stringshare_add(name);
   tmp_fi.src = &tmp_fn;
   tmp_fi.size = size;
   tmp_fi.wanted_rend = wanted_rend;
   tmp_fi.bitmap_scalable = bitmap_scalable;
   fi = eina_hash_find(fonts, &tmp_fi);
   if (fi)
     {
	if (fi->references == 0)
	  {
	     evas_common_font_int_modify_cache_by(fi, -1);
	     fonts_lru = eina_list_remove(fonts_lru, fi);
	  }
	fi->references++;
     }
   eina_stringshare_del(tmp_fn.name);
   return fi;
}

/**
 * @brief Clears extended data and color data for all glyphs in a font instance.
 *
 * Iterates through all glyphs cached in the font instance's Fash structure.
 * For each glyph:
 * - If `fg->ext_dat` (extended data) exists and `fg->ext_dat_free` (a custom
 *   freeing function for `ext_dat`) is set, `ext_dat_free` is called and
 *   `ext_dat` is NULLed.
 * - If `fg->col_dat` (color data, typically an Evas_Cache_Image) exists,
 *   `evas_cache_image_drop` is called to release it, and `col_dat` is NULLed.
 *
 * This is used to free resources associated with individual glyphs that are
 * not part of the main glyph bitmap data, such as pre-rendered color glyphs
 * or other backend-specific extensions.
 *
 * @param fi The font instance (RGBA_Font_Int) whose glyphs' extended data
 *           is to be cleared.
 */
static void
_font_int_ext_clear(RGBA_Font_Int *fi)
{
   RGBA_Font_Glyph *fg;
   Fash_Glyph_Map *fmap;
   Fash_Glyph_Map2 *fash2;
   Fash_Glyph *fash;
   int i, j, k;

   fash = fi->fash;
   if (!fash) return;
   for (k = 0; k <= 0xff; k++)
     {
        fash2 = fash->bucket[k];
        if (fash2)
          {
             for (j = 0; j <= 0xff; j++)
               {
                  fmap = fash2->bucket[j];
                  if (fmap)
                    {
                       for (i = 0; i <= 0xff; i++)
                         {
                            fg = fmap->item[i];
                            if ((fg) && (fg != (void *)(-1)))
                              {
                                 if (fg->ext_dat)
                                   {
                                      if (fg->ext_dat_free)
                                        fg->ext_dat_free(fg->ext_dat);
                                      fg->ext_dat = NULL;
                                      fg->ext_dat_free = NULL;
                                   }
                                 if (fg->col_dat) evas_cache_image_drop(fg->col_dat);
                                 fg->col_dat = NULL;
                              }
                         }
                    }
               }
          }
     }
}

/**
 * @brief Callback function to clear extended data for a single font instance.
 *
 * This function is used with `eina_hash_foreach` to iterate over all font
 * instances in the `fonts` hash. It calls `_font_int_ext_clear` for each
 * font instance.
 *
 * @param hash Unused.
 * @param key Unused.
 * @param data A pointer to an RGBA_Font_Int.
 * @param fdata Unused.
 * @return EINA_TRUE to continue hash iteration.
 */
static Eina_Bool
_cb_hash_font_ext(const Eina_Hash *hash EINA_UNUSED,
                  const void *key EINA_UNUSED,
                  void *data EINA_UNUSED, /* Actually RGBA_Font_Int* */
                  void *fdata EINA_UNUSED)
{
   _font_int_ext_clear(data);
   return EINA_TRUE;
}

/**
 * @brief Clears extended glyph data for all loaded font instances.
 *
 * Iterates through all RGBA_Font_Int structures in the global `fonts` hash table
 * and calls `_font_int_ext_clear` for each one via the `_cb_hash_font_ext`
 * callback. This is used to free extended data (like color glyph data)
 * for all glyphs across all loaded fonts.
 */
EVAS_API void
evas_common_font_ext_clear(void)
{
   eina_hash_foreach(fonts, _cb_hash_font_ext, NULL);
}
