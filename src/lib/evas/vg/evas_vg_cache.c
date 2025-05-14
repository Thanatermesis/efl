#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_vg_private.h"

static Vg_Cache* vg_cache = NULL;

/**
 * @brief Structure to map file extensions to loader module names.
 */
struct ext_loader_s
{
   unsigned int length; /**< Length of the file extension string. */
   const char *extension; /**< The file extension (e.g., ".svg"). */
   const char *loader; /**< The name of the loader module (e.g., "svg"). */
};

/**
 * @brief Structure to map file extensions to saver module names.
 */
struct ext_saver_s
{
   unsigned int length; /**< Length of the file extension string. */
   const char *extension; /**< The file extension (e.g., ".eet"). */
   const char *saver; /**< The name of the saver module (e.g., "eet"). */
};

#define MATCHING(Ext, Module) { sizeof(Ext)-1, Ext, Module }

/**
 * @brief Array mapping file extensions to their corresponding loader modules.
 * This provides a first-guess for which loader to use based on the file extension.
 * Example: { {sizeof(".eet")-1, ".eet", "eet"} , ... }
 */
static const struct ext_loader_s loaders[] =
{ /* map extensions to loaders to use for good first-guess tries */
   MATCHING(".eet", "eet"),
   MATCHING(".edj", "eet"),
   MATCHING(".svg", "svg"),
   MATCHING(".svgz", "svg"),
   MATCHING(".svg.gz", "svg")
};

/**
 * @brief Array of loader module names, ordered by likelihood of being needed.
 * This is used as a fallback if no loader is found via extension matching.
 * Example: { "eet", "json", "svg" }
 */
static const char *loaders_name[] =
{ /* in order of most likely needed */
  "eet", "json", "svg"
};

/**
 * @brief Array mapping file extensions to their corresponding saver modules.
 * This provides a first-guess for which saver to use based on the file extension.
 * Example: { {sizeof(".eet")-1, ".eet", "eet"} , ... }
 */
static const struct ext_saver_s savers[] =
{ /* map extensions to savers to use for good first-guess tries */
   MATCHING(".eet", "eet"),
   MATCHING(".edj", "eet"),
   MATCHING(".svg", "svg")
};

/**
 * @internal
 * @brief Finds an Evas_Module for loading a vector graphics file based on its extension.
 *
 * This function iterates through a predefined list of known file extensions
 * and their associated loader modules. If a match is found, it attempts
 * to find and return the corresponding Evas_Module.
 *
 * @param file The full path or name of the file to find a loader for.
 * @return A pointer to the Evas_Module if a suitable loader is found, otherwise NULL.
 */
static Evas_Module *
_find_loader_module(const char *file)
{
   const char           *loader = NULL, *end;
   Evas_Module          *em = NULL;
   unsigned int          i;
   int                   len, len2;
   len = strlen(file);
   end = file + len;
   for (i = 0; i < (sizeof (loaders) / sizeof(struct ext_loader_s)); i++)
     {
        len2 = loaders[i].length;
        if (len2 > len) continue;
        if (!strcasecmp(end - len2, loaders[i].extension))
          {
             loader = loaders[i].loader;
             break;
          }
     }
   if (loader)
     em = evas_module_find_type(EVAS_MODULE_TYPE_VG_LOADER, loader);
   return em;
}

/**
 * @internal
 * @brief Loads vector graphics data from a file.
 *
 * This function attempts to load vector graphics data using a suitable loader module.
 * It first tries to find a loader based on the file extension. If that fails or
 * the loader cannot open the file, it iterates through a list of default loaders.
 *
 * @param file An Eina_File handle representing the file to load.
 * @param key An optional key or sub-identifier within the file (e.g., for EET files).
 * @return A pointer to Vg_File_Data if successful, otherwise NULL.
 *         The Vg_File_Data contains the loaded vector graphics data and loader functions.
 */
static Vg_File_Data *
_vg_load_from_file(const Eina_File *file, const char *key)
{
   Evas_Module       *em;
   Evas_Vg_Load_Func *loader;
   int                error = EVAS_LOAD_ERROR_GENERIC;
   Vg_File_Data      *vfd;
   unsigned int i;

   const char *file_name = eina_file_filename_get(file);
   em = _find_loader_module(file_name);
   if (em)
     {
        loader = em->functions;
        vfd = loader->file_open((Eina_File *) file, key, &error);
        if (vfd)
          {
             vfd->loader = loader;
             return vfd;
          }
     }
   else
     {
        for (i = 0; i < sizeof (loaders_name) / sizeof (char *); i++)
          {
             em = evas_module_find_type(EVAS_MODULE_TYPE_VG_LOADER, loaders_name[i]);
             if (em)
               {
                  loader = em->functions;
                  vfd = loader->file_open((Eina_File *) file, key, &error);
                  if (vfd)
                    {
                       vfd->loader = loader;
                       return vfd;
                    }
               }
          }
     }
   WRN("Exhausted all means to load vector file = %s", file_name);
   return NULL;
}

/**
 * @internal
 * @brief Finds an Evas_Module for saving a vector graphics file based on its extension.
 *
 * This function iterates through a predefined list of known file extensions
 * and their associated saver modules. If a match is found, it attempts
 * to find and return the corresponding Evas_Module.
 *
 * @param file The full path or name of the file to find a saver for.
 * @return A pointer to the Evas_Module if a suitable saver is found, otherwise NULL.
 */
static Evas_Module *
_find_saver_module(const char *file)
{
   const char           *saver = NULL, *end;
   Evas_Module          *em = NULL;
   unsigned int          i;
   int                   len, len2;
   len = strlen(file);
   end = file + len;
   for (i = 0; i < (sizeof (savers) / sizeof(struct ext_saver_s)); i++)
     {
        len2 = savers[i].length;
        if (len2 > len) continue;
        if (!strcasecmp(end - len2, savers[i].extension))
          {
             saver = savers[i].saver;
             break;
          }
     }
   if (saver)
     em = evas_module_find_type(EVAS_MODULE_TYPE_VG_SAVER, saver);
   return em;
}

/**
 * @internal
 * @brief Callback function to free Vg_File_Data.
 *
 * This function is used by the Eina_Hash when an entry containing
 * Vg_File_Data is removed. It calls the appropriate file_close function
 * from the loader module associated with the Vg_File_Data.
 *
 * @param data A pointer to the Vg_File_Data to be freed.
 */
static void
_evas_cache_vg_data_free_cb(void *data)
{
   Vg_File_Data *vfd = data;
   vfd->loader->file_close(vfd);
}

/**
 * @internal
 * @brief Callback function to free a Vg_Cache_Entry.
 *
 * This function is used by the Eina_Hash when an entry containing
 * a Vg_Cache_Entry is removed. It decrements the reference count of the
 * associated Vg_File_Data and frees it if the count reaches zero.
 * It also cleans up other resources held by the Vg_Cache_Entry,
 * such as the key string, hash key, and root VG node.
 *
 * @param data A pointer to the Vg_Cache_Entry to be freed.
 */
static void
_evas_cache_vg_entry_free_cb(void *data)
{
   Vg_Cache_Entry *vg_entry = data;

   if (vg_entry->vfd)
     {
        vg_entry->vfd->ref--;

        if (vg_entry->vfd->ref <= 0)
          {
             if (vg_entry->vfd->shareable)
               {
                  Eina_Strbuf *hash_key = eina_strbuf_new();
                  eina_strbuf_append_printf(hash_key, "%s/%s/%p",
                                            eina_file_filename_get(vg_entry->file),
                                            vg_entry->key,
                                            vg_entry->evas);
                  if (!eina_hash_del(vg_cache->vfd_hash, eina_strbuf_string_get(hash_key), vg_entry->vfd))
                    ERR("Failed to delete vfd = (%p) from hash", vg_entry->vfd);
                  eina_strbuf_free(hash_key);
               }
             else
               {
                  vg_entry->vfd->loader->file_close(vg_entry->vfd);
               }
          }
     }

   eina_stringshare_del(vg_entry->key);
   free(vg_entry->hash_key);
   efl_unref(vg_entry->root);
   free(vg_entry);
}

/**
 * @internal
 * @brief Saves vector graphics data to a file.
 *
 * This function attempts to save the provided Vg_File_Data to the specified
 * file using a suitable saver module. The saver module is determined based
 * on the output file's extension.
 *
 * @param vfd A pointer to the Vg_File_Data containing the vector graphics to save.
 * @param file The path of the file to save to.
 * @param key An optional key or sub-identifier within the file (e.g., for EET files).
 * @param info Optional save information, such as compression level.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_vg_file_save(Vg_File_Data *vfd, const char *file, const char *key, const Efl_File_Save_Info *info)
{
   Evas_Module       *em;
   Evas_Vg_Save_Func *saver;
   Evas_Load_Error    error = EVAS_LOAD_ERROR_GENERIC;
   int                compress = 9;

   if (!file) return EINA_FALSE;

   if (info) compress = info->compression;

   em = _find_saver_module(file);
   if (em)
     {
        saver = em->functions;
        error = saver->file_save(vfd, file, key, compress);
     }

   if (error)
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Updates the root Efl_VG node of a cache entry.
 *
 * This function handles the logic for updating the `root` node in a `Vg_Cache_Entry`.
 * If the `Vg_File_Data` has a static viewbox, the root node is duplicated to ensure
 * that transformations applied to one cache entry (e.g., due to resizing) do not
 * affect others sharing the same `Vg_File_Data`.
 * If the viewbox is not static, or if the root node needs to be updated,
 * it ensures the cache entry's root node references the `Vg_File_Data`'s root node.
 *
 * @param vg_entry The vector graphics cache entry whose root node is to be updated.
 */
static void
_root_update(Vg_Cache_Entry *vg_entry)
{
   Vg_File_Data *vfd = vg_entry->vfd;

   /* Optimization: static viewbox may have same root data regardless of size.
      So we can't use the root data directly, but copy it for each vg_entries.
      In the meantime, non-static viewbox root data may have difference instance for each
      size. So it's affordable to share the root data for each vg_entries. */
   if (vfd->static_viewbox)
     {
        /* TODO: Yet trivial but still we may have a better solution to
           avoid this unnecessary copy. If the ector surface key is not
           to this root pointer. */
        vg_entry->root = efl_duplicate(vfd->root);
     }
   //Shareable??
   else if (vg_entry->root != vfd->root)
     {
        if (vg_entry->root) efl_unref(vg_entry->root);
        vg_entry->root = efl_ref(vfd->root);
     }
}

/**
 * @internal
 * @brief Applies a local transformation to the root VG node to fit a given width and height.
 *
 * This function is called when the `Vg_File_Data` has a static viewbox. It calculates
 * the necessary scaling and translation to make the content fit within the target
 * dimensions `w` and `h`, while respecting the `preserve_aspect` flag of the `Vg_File_Data`.
 * The transformation is applied directly to the `root` Efl_VG node.
 *
 * @param root The Efl_VG node to transform.
 * @param w The target width.
 * @param h The target height.
 * @param vfd The Vg_File_Data containing viewbox information and aspect preservation settings.
 */
static void
_local_transform(Efl_VG *root, double w, double h, Vg_File_Data *vfd)
{
   double sx = 0, sy= 0, scale;
   Eina_Matrix3 m;

   if (!vfd->static_viewbox) return;
   if (EINA_DBL_EQ(vfd->view_box.w, w) && EINA_DBL_EQ(vfd->view_box.h, h)) return;

   sx = w / vfd->view_box.w;
   sy = h / vfd->view_box.h;

   scale = sx < sy ? sx : sy;
   eina_matrix3_identity(&m);

   // align hcenter and vcenter
   if (vfd->preserve_aspect)
     {
        eina_matrix3_translate(&m, (w - vfd->view_box.w * scale)/2.0, (h - vfd->view_box.h * scale)/2.0);
        eina_matrix3_scale(&m, scale, scale);
        eina_matrix3_translate(&m, -vfd->view_box.x, -vfd->view_box.y);
     }
   else
     {
        eina_matrix3_scale(&m, sx, sy);
        eina_matrix3_translate(&m, -vfd->view_box.x, -vfd->view_box.y);
     }
   efl_canvas_vg_node_transformation_set(root, &m);
}

/**
 * @brief Initializes the Evas vector graphics cache system.
 *
 * This function sets up the global vector graphics cache, `vg_cache`.
 * It allocates memory for the cache structure and initializes hash tables
 * for storing `Vg_File_Data` and `Vg_Cache_Entry` objects.
 * It uses a reference counter to allow multiple initializations and
 * shutdowns.
 *
 * @see evas_cache_vg_shutdown()
 */
void
evas_cache_vg_init(void)
{
   if (vg_cache)
     {
        vg_cache->ref++;
        return;
     }
   vg_cache = calloc(1, sizeof(Vg_Cache));
   if (!vg_cache)
     {
        CRI("Failed to alloc Vg_Cache");
        return;
     }

   vg_cache->vfd_hash = eina_hash_string_superfast_new(_evas_cache_vg_data_free_cb);
   vg_cache->vg_entry_hash = eina_hash_string_superfast_new(_evas_cache_vg_entry_free_cb);
   vg_cache->ref++;
}

/**
 * @brief Generates or retrieves a unique key for a vector graphics surface.
 *
 * This function creates a string-based key derived from the root node pointer,
 * width, height, and frame index. This key is used to uniquely identify
 * a specific rendering state of a vector graphic, primarily for caching
 * rasterized surfaces (not directly managed by this VG cache, but the key
 * generation is provided here).
 * The generated keys are stored in a list to ensure that the same pointer
 * is returned for identical parameters, allowing the key itself to be used
 * in hash tables or comparisons.
 *
 * @param root The root Efl_Canvas_Vg_Node of the vector graphic.
 * @param w The width of the surface.
 * @param h The height of the surface.
 * @param frame_idx The frame index for animated vector graphics.
 * @return A unique `void *` key representing the surface parameters.
 *         The caller should not free this pointer; it is managed by the cache.
 *         Returns NULL on allocation failure.
 *
 * @note The returned key is a `char*` but cast to `void*` for generic use.
 *       It remains valid until `evas_cache_vg_shutdown()` is called and the
 *       cache reference count drops to zero.
 */
void *
evas_cache_vg_surface_key_get(Efl_Canvas_Vg_Node *root, int w, int h, int frame_idx)
{
   //This make a unique key pointer by arguments.
   Eina_Strbuf *hash_key = eina_strbuf_new();
   eina_strbuf_append_printf(hash_key, "%p/%d/%d/%d", root, w, h, frame_idx);
   const char *new_key = eina_strbuf_string_get(hash_key);
   if (!new_key)
     {
        eina_strbuf_free(hash_key);
        return NULL;
     }

   Eina_List *l;
   char *key;
   EINA_LIST_FOREACH(vg_cache->vg_surface_keys, l, key)
     {
        //Exisiting key!
        if (!strcmp(key, new_key))
          {
             eina_strbuf_free(hash_key);
             return key;
          }
     }

   //New key comes.
   key = eina_strbuf_string_steal(hash_key);
   vg_cache->vg_surface_keys = eina_list_append(vg_cache->vg_surface_keys, key);

   eina_strbuf_free(hash_key);

   return (void *) key;
}

/**
 * @brief Shuts down the Evas vector graphics cache system.
 *
 * This function decrements the reference counter for the `vg_cache`.
 * If the reference count drops to zero, it frees all resources
 * associated with the cache, including stored `Vg_File_Data`,
 * `Vg_Cache_Entry` objects, and the surface keys list.
 *
 * @see evas_cache_vg_init()
 */
void
evas_cache_vg_shutdown(void)
{
   if (!vg_cache) return;
   vg_cache->ref--;
   if (vg_cache->ref > 0) return;

   char *key;
   EINA_LIST_FREE(vg_cache->vg_surface_keys, key)
     free(key);
   eina_list_free(vg_cache->vg_surface_keys);

   eina_hash_free(vg_cache->vg_entry_hash);
   eina_hash_free(vg_cache->vfd_hash);
   free(vg_cache);
   vg_cache = NULL;
}

/**
 * @brief Opens and loads vector graphics file data, potentially using a shared cache.
 *
 * This function retrieves `Vg_File_Data` for a given file and key.
 * If `shareable` is EINA_TRUE, it first checks a hash (`vg_cache->vfd_hash`)
 * for existing `Vg_File_Data` associated with the file, key, and Evas instance.
 * If found, the existing data is returned. Otherwise, or if `shareable` is EINA_FALSE,
 * it loads the data from the file using `_vg_load_from_file`.
 * If loaded and shareable, the new `Vg_File_Data` is added to the hash.
 *
 * @param file The Eina_File handle of the vector graphics file.
 * @param key An optional key specifying a particular dataset within the file (e.g., for EET).
 * @param e The Evas canvas instance associated with this open operation, used for hashing shared data.
 * @param shareable If EINA_TRUE, the loaded Vg_File_Data can be shared among multiple cache entries
 *                  and is stored in `vg_cache->vfd_hash`. If EINA_FALSE, the data is loaded fresh
 *                  and not stored in `vg_cache->vfd_hash`.
 * @return A pointer to the `Vg_File_Data` on success, or NULL on failure.
 *         The `shareable` field within the returned `Vg_File_Data` is set accordingly.
 */
Vg_File_Data *
evas_cache_vg_file_open(const Eina_File *file, const char *key, Evas *e, Eina_Bool shareable)
{
   Vg_File_Data *vfd;
   Eina_Strbuf *hash_key;

   if (shareable)
     {
        hash_key = eina_strbuf_new();
        eina_strbuf_append_printf(hash_key, "%s/%s/%p", eina_file_filename_get(file), key, e);
        vfd = eina_hash_find(vg_cache->vfd_hash, eina_strbuf_string_get(hash_key));
        if (!vfd)
          {
             vfd = _vg_load_from_file(file, key);
             //File exists.
             if (vfd) eina_hash_add(vg_cache->vfd_hash, eina_strbuf_string_get(hash_key), vfd);
          }
        eina_strbuf_free(hash_key);
     }
   else
     {
        vfd = _vg_load_from_file(file, key);
     }
   if (vfd) vfd->shareable = shareable;
   return vfd;
}

/**
 * @brief Creates a new vector graphics cache entry with a different size, based on an existing entry.
 *
 * This is a convenience function that calls `evas_cache_vg_entry_create` using
 * properties from an existing `vg_entry` but with new width `w` and height `h`.
 * The value provider list (`vp_list`) from the original entry's `Vg_File_Data` is reused.
 *
 * @param vg_entry The existing Vg_Cache_Entry to base the new entry on.
 * @param w The new width for the cache entry.
 * @param h The new height for the cache entry.
 * @return A pointer to the new or existing (if parameters match an existing one) `Vg_Cache_Entry`,
 *         or NULL on failure.
 * @see evas_cache_vg_entry_create()
 */
Vg_Cache_Entry*
evas_cache_vg_entry_resize(Vg_Cache_Entry *vg_entry, int w, int h)
{
   return evas_cache_vg_entry_create(vg_entry->evas, vg_entry->file, vg_entry->key, w, h, vg_entry->vfd->vp_list);
}

/**
 * @brief Creates or retrieves a vector graphics cache entry.
 *
 * This function manages `Vg_Cache_Entry` objects. It first generates a hash key
 * based on the Evas instance, file, key, width, height, and value provider list.
 * It then attempts to find an existing entry in `vg_cache->vg_entry_hash`.
 *
 * If an entry is found, its reference count is incremented.
 * If not found, a new `Vg_Cache_Entry` is allocated, initialized, and added to the hash.
 *
 * In both cases, it then ensures the associated `Vg_File_Data` is opened (or its
 * reference count incremented if already open) via `evas_cache_vg_file_open`.
 * The `shareable` flag for `evas_cache_vg_file_open` is determined by whether `vp_list` is NULL.
 * If `Vg_File_Data` cannot be obtained, the (newly created) cache entry is cleaned up.
 *
 * @param evas The Evas canvas instance.
 * @param file The Eina_File handle of the vector graphics file.
 * @param key An optional key for the data within the file.
 * @param w The target width for this cache entry.
 * @param h The target height for this cache entry.
 * @param vp_list A list of value providers. If non-NULL, the underlying `Vg_File_Data`
 *                will not be shared via `vg_cache->vfd_hash`.
 * @return A pointer to the `Vg_Cache_Entry` on success, or NULL on failure.
 *         The returned entry has its reference count incremented.
 */
Vg_Cache_Entry*
evas_cache_vg_entry_create(Evas *evas,
                           const Eina_File *file,
                           const char *key,
                           int w, int h, Eina_List *vp_list)
{
   Vg_Cache_Entry* vg_entry;
   Eina_Strbuf *hash_key;

   if (!vg_cache) return NULL;

   //TODO: zero-sized entry is useless. how to skip it?
   hash_key = eina_strbuf_new();
   eina_strbuf_append_printf(hash_key, "%p/%p/%s/%d/%d/%p", evas, file, key, w, h, vp_list);
   vg_entry = eina_hash_find(vg_cache->vg_entry_hash, eina_strbuf_string_get(hash_key));
   if (!vg_entry)
     {
        vg_entry = calloc(1, sizeof(Vg_Cache_Entry));
        if (!vg_entry)
          {
             CRI("Failed to alloc Vg_Cache_Entry");
             eina_strbuf_free(hash_key);
             return NULL;
          }
        vg_entry->file = file;
        vg_entry->key = eina_stringshare_add(key);
        vg_entry->w = w;
        vg_entry->h = h;
        vg_entry->evas = evas;
        vg_entry->hash_key = eina_strbuf_string_steal(hash_key);
        eina_hash_direct_add(vg_cache->vg_entry_hash, vg_entry->hash_key, vg_entry);
     }
   eina_strbuf_free(hash_key);
   vg_entry->ref++;
   vg_entry->vfd = evas_cache_vg_file_open(file, key, vg_entry->evas, vp_list ? EINA_FALSE : EINA_TRUE);
   //No File??
   if (!vg_entry->vfd)
     {
        evas_cache_vg_entry_del(vg_entry);
        return NULL;
     }
   vg_entry->vfd->ref++;
   vg_entry->vfd->vp_list = vp_list;

   return vg_entry;
}

/**
 * @brief Gets the total duration of the animation for a vector graphics cache entry.
 *
 * @param vg_entry The vector graphics cache entry.
 * @return The duration of the animation in seconds. Returns 0 if the entry is invalid,
 *         has no animation data, or the animation has no duration.
 */
double
evas_cache_vg_anim_duration_get(const Vg_Cache_Entry* vg_entry)
{
   if (!vg_entry->vfd->anim_data) return 0;
   return vg_entry->vfd->anim_data->duration;
}

/**
 * @brief Gets the total number of frames in the animation for a vector graphics cache entry.
 *
 * @param vg_entry The vector graphics cache entry.
 * @return The total number of frames. Returns 0 if the entry is invalid or has no animation data.
 */
unsigned int
evas_cache_vg_anim_frame_count_get(const Vg_Cache_Entry* vg_entry)
{
   if (!vg_entry) return 0;
   Vg_File_Data *vfd = vg_entry->vfd;
   if (!vfd || !vfd->anim_data) return 0;
   return vfd->anim_data->frame_cnt;
}

/**
 * @brief Defines or updates an animation sector (named frame range) for a vector graphics cache entry.
 *
 * If a marker with the given `name` already exists, its start and end frames are updated.
 * Otherwise, a new marker is created and added to the animation data.
 *
 * @param vg_entry The vector graphics cache entry.
 * @param name The name of the sector/marker. The string is shared.
 * @param startframe The starting frame number for this sector.
 * @param endframe The ending frame number for this sector.
 * @return EINA_TRUE if the sector was successfully set or updated, EINA_FALSE otherwise
 *         (e.g., invalid entry, no animation data, no markers array, or null name).
 */
Eina_Bool
evas_cache_vg_anim_sector_set(const Vg_Cache_Entry* vg_entry, const char *name, int startframe, int endframe)
{
   if (!vg_entry) return EINA_FALSE;
   if (!vg_entry->vfd->anim_data) return EINA_FALSE;
   if (!vg_entry->vfd->anim_data->markers) return EINA_FALSE;
   if (!name) return EINA_FALSE;

   Vg_File_Anim_Data_Marker *marker;
   Vg_File_Anim_Data_Marker new_marker;
   int i = 0;

   EINA_INARRAY_FOREACH(vg_entry->vfd->anim_data->markers, marker)
     {
        if (!strcmp(marker->name, name))
          {
             marker->startframe = startframe;
             marker->endframe = endframe;
             return EINA_TRUE;
          }
        i++;
     }

   new_marker.name = eina_stringshare_add(name);
   new_marker.startframe = startframe;
   new_marker.endframe = endframe;
   eina_inarray_push(vg_entry->vfd->anim_data->markers, &new_marker);

   return EINA_TRUE;
}

/**
 * @brief Retrieves the start and end frames of a named animation sector for a vector graphics cache entry.
 *
 * @param vg_entry The vector graphics cache entry.
 * @param name The name of the sector/marker to retrieve.
 * @param[out] startframe Pointer to store the starting frame number. Can be NULL.
 * @param[out] endframe Pointer to store the ending frame number. Can be NULL.
 * @return EINA_TRUE if the sector was found and data retrieved, EINA_FALSE otherwise
 *         (e.g., invalid entry, no animation data, no markers array, null name, or sector not found).
 */
Eina_Bool
evas_cache_vg_anim_sector_get(const Vg_Cache_Entry* vg_entry, const char *name, int* startframe, int* endframe)
{
   if (!vg_entry) return EINA_FALSE;
   if (!vg_entry->vfd->anim_data) return EINA_FALSE;
   if (!vg_entry->vfd->anim_data->markers) return EINA_FALSE;
   if (!name) return EINA_FALSE;

   Vg_File_Anim_Data_Marker *marker;
   EINA_INARRAY_FOREACH(vg_entry->vfd->anim_data->markers, marker)
     {
        if (!strcmp(marker->name, name))
          {
             if (startframe) *startframe = marker->startframe;
             if (endframe) *endframe = marker->endframe;
             return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

/**
 * @brief Retrieves the root Efl_VG node (scene graph) for a specific frame of a vector graphics cache entry.
 *
 * This function is responsible for providing the renderable vector graphics tree.
 * It handles several cases:
 * 1. Invalid entry or zero dimensions: returns NULL.
 * 2. Animated VG: If the requested `frame_num` and dimensions match the current state of
 *    `vg_entry->root` and `vfd->anim_data`, the existing `vg_entry->root` is returned.
 * 3. Static VG: If `vg_entry->root` already exists, it's returned.
 *
 * If an update is needed (e.g., new frame, different size for non-static viewbox):
 * - Updates `vfd->view_box` dimensions if the viewbox is not static.
 * - Sets the current frame number in `vfd->anim_data` if animation data exists.
 * - Calls `vfd->loader->file_data(vfd)` to load/decode the frame data into `vfd->root`.
 * - Calls `_root_update()` to correctly set `vg_entry->root` (duplicating if necessary for static viewboxes).
 * - Calls `_local_transform()` to apply scaling and translation if the viewbox is static and dimensions differ.
 *
 * @param vg_entry The vector graphics cache entry.
 * @param frame_num The frame number to retrieve. For non-animated VGs, this is typically 0.
 * @return A pointer to the root Efl_VG node for the specified frame and entry configuration,
 *         or NULL on failure or if dimensions are invalid.
 */
Efl_VG*
evas_cache_vg_tree_get(Vg_Cache_Entry *vg_entry, unsigned int frame_num)
{
   if (!vg_entry) return NULL;
   if ((vg_entry->w < 1) || (vg_entry->h < 1)) return NULL;

   Vg_File_Data *vfd = vg_entry->vfd;
   if (!vfd) return NULL;

   //No need to update.
   if (vfd->anim_data)
     {
        if ((vg_entry->w == vfd->view_box.w) &&
            (vg_entry->h == vfd->view_box.h))
          {
             if (vg_entry->root &&
                 vfd->anim_data->frame_num == frame_num)
               return vg_entry->root;
          }
     }
   else
     {
        if (vg_entry->root)
          return vg_entry->root;
     }

   if (!vfd->static_viewbox)
     {
        vfd->view_box.w = vg_entry->w;
        vfd->view_box.h = vg_entry->h;
     }

   if (vfd->anim_data) vfd->anim_data->frame_num = frame_num;

   if (!vfd->loader->file_data(vfd)) return NULL;

   _root_update(vg_entry);

   _local_transform(vg_entry->root, vg_entry->w, vg_entry->h, vfd);

   return vg_entry->root;
}

/**
 * @brief Updates the value provider list for a vector graphics cache entry.
 *
 * This function sets or replaces the list of value providers (`vp_list`)
 * in the `Vg_File_Data` associated with the given `Vg_Cache_Entry`.
 * Value providers can be used to dynamically alter properties of the
 * vector graphic at load or render time.
 *
 * @param vg_entry The vector graphics cache entry whose value providers are to be updated.
 * @param vp_list The new list of Efl_Canvas_Vg_Value_Provider instances.
 *                The list itself is not copied; the Vg_File_Data will hold this pointer.
 */
void
evas_cache_vg_entry_value_provider_update(Vg_Cache_Entry *vg_entry, Eina_List *vp_list)
{
   if (!vg_entry) return;

   Vg_File_Data *vfd = vg_entry->vfd;
   if (!vfd) return;

   vfd->vp_list = vp_list;
}

/**
 * @brief Decrements the reference count of a vector graphics cache entry and potentially frees it.
 *
 * If the reference count of `vg_entry` drops to zero after decrementing,
 * the entry is removed from the `vg_cache->vg_entry_hash` and freed
 * (which involves calling `_evas_cache_vg_entry_free_cb`).
 *
 * @param vg_entry The vector graphics cache entry to delete or dereference.
 * @see evas_cache_vg_entry_create()
 * @see _evas_cache_vg_entry_free_cb()
 */
void
evas_cache_vg_entry_del(Vg_Cache_Entry *vg_entry)
{
   if (!vg_cache || !vg_entry) return;
   vg_entry->ref--;
   if (vg_entry->ref > 0) return;
   if (!eina_hash_del(vg_cache->vg_entry_hash, vg_entry->hash_key, vg_entry))
     ERR("Failed to delete vg_entry = (%p) from hash", vg_entry);
}

/**
 * @brief Gets the default (intrinsic) size of the vector graphic associated with a cache entry.
 *
 * This size is typically read from the file metadata by the loader.
 *
 * @param vg_entry The vector graphics cache entry.
 * @return An Eina_Size2D structure containing the default width and height.
 *         Returns (0,0) if the entry is invalid or has no Vg_File_Data.
 */
Eina_Size2D
evas_cache_vg_entry_default_size_get(const Vg_Cache_Entry *vg_entry)
{
   if (!vg_entry) return EINA_SIZE2D(0, 0);
   return EINA_SIZE2D(vg_entry->vfd->w, vg_entry->vfd->h);
}

/**
 * @brief Saves the content of a vector graphics cache entry to a file.
 *
 * This function first ensures the `Vg_File_Data` for the cache entry is available
 * by calling `evas_cache_vg_file_open`. The shareable status for this open operation
 * is determined by whether the existing `vfd` (if any) has a `vp_list`.
 * Then, it calls `_vg_file_save` to perform the actual saving operation.
 *
 * @param vg_entry The vector graphics cache entry whose content is to be saved.
 * @param file The path of the file to save to.
 * @param key An optional key or sub-identifier for the data within the output file.
 * @param info Optional save information (e.g., compression level).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
evas_cache_vg_entry_file_save(Vg_Cache_Entry *vg_entry, const char *file, const char *key, const Efl_File_Save_Info *info)
{
   Vg_File_Data *vfd =
      evas_cache_vg_file_open(vg_entry->file, vg_entry->key, vg_entry->evas
                              ,vg_entry->vfd ? (vg_entry->vfd->vp_list ? EINA_FALSE : EINA_TRUE): EINA_TRUE);

   if (!vfd) return EINA_FALSE;

   return _vg_file_save(vfd, file, key, info);
}

/**
 * @brief Saves an arbitrary Efl_VG tree to a vector graphics file.
 *
 * This function is used to save a vector graphics scene that might not be
 * associated with any cache entry or loaded file. It constructs a temporary
 * `Vg_File_Data` structure on the stack, populates it with the provided
 * `root` node and dimensions, and then calls `_vg_file_save`.
 *
 * @param root The root Efl_VG node of the scene to save.
 * @param w The width of the graphic.
 * @param h The height of the graphic.
 * @param file The path of the file to save to.
 * @param key An optional key or sub-identifier for the data within the output file.
 * @param info Optional save information (e.g., compression level).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
evas_cache_vg_file_save(Efl_VG *root, int w, int h, const char *file, const char *key, const Efl_File_Save_Info *info)
{
   Vg_File_Data vfd = {};

   if (!root) return EINA_FALSE;

   vfd.view_box.x = w;
   vfd.view_box.y = h;
   vfd.root = root;
   vfd.preserve_aspect = EINA_FALSE;

   return _vg_file_save(&vfd, file, key, info);
}
