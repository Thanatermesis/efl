#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Efl.h>

/**
 * @internal
 * @brief Private data structure for Efl_File_Mixin.
 */
typedef struct _Efl_File_Data Efl_File_Data;
struct _Efl_File_Data
{
   Eina_Stringshare *vpath; /**< Virtual path to the file, set by efl_file_set() */
   Eina_Stringshare *key;   /**< Optional key associated with the file, set by efl_file_key_set() */
   Eina_File        *file;  /**< Memory-mapped file handle, set by efl_file_mmap_set() */
   time_t            mtime; /**< Last modification time of the file, used to check if file changed */
   Eina_Bool file_opened : 1; /**< Flag: EINA_TRUE if `file` was opened implicitly during load, EINA_FALSE otherwise */
   Eina_Bool setting     : 1; /**< Flag: EINA_TRUE if properties are being set internally, to avoid recursion */
   Eina_Bool loaded      : 1; /**< Flag: EINA_TRUE if the file properties have been successfully loaded */
};

/**
 * @internal
 * @brief Unloads the file data.
 *
 * If the file was opened implicitly (pd->file_opened is true), it closes the file handle
 * and resets the mmap property. Sets the object state to unloaded.
 *
 * @param obj The Efl_File object.
 * @param pd The private data of the Efl_File object.
 */
EOLIAN static void
_efl_file_unload(Eo *obj, Efl_File_Data *pd)
{
   if (!pd->loaded) return;
   if (!pd->file) return;
   if (!pd->file_opened) return;
   pd->setting = 1;
   eina_file_close(pd->file); // close matching open (dup in _efl_file_mmap_set) OK
   pd->file = NULL;
   efl_file_mmap_set(obj, NULL);
   pd->setting = 0;
   pd->loaded = pd->file_opened = EINA_FALSE;
}

/**
 * @internal
 * @brief Loads the file data.
 *
 * If not already loaded, this function attempts to open the file specified by pd->vpath
 * if no Eina_File (pd->file) is already set. It then tries to memory-map this file.
 * The loaded state (pd->loaded) is updated based on success or failure.
 *
 * @param obj The Efl_File object.
 * @param pd The private data of the Efl_File object.
 * @return 0 on success, or an Eina_Error code on failure.
 */
EOLIAN static Eina_Error
_efl_file_load(Eo *obj, Efl_File_Data *pd)
{
   Eina_Error ret = 0;

   if (pd->loaded) return 0;
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->vpath, ENOENT);
   errno = 0;
   if (!pd->file)
     {
        Eina_File *f;
        f = eina_file_open(pd->vpath, EINA_FALSE);
        if (!f) return errno ?: ENOENT;
        pd->file_opened = EINA_TRUE;
        pd->setting = 1;
        ret = efl_file_mmap_set(obj, f);
        pd->setting = 0;
        if (ret) pd->file_opened = EINA_FALSE;
        eina_file_close(f); // close matching open OK
     }
   pd->loaded = !ret;
   return ret;
}

/**
 * @internal
 * @brief Sets the memory-mapped file for the object.
 *
 * Duplicates the provided Eina_File handle @p f and stores it.
 * If @p f is different from the current file, the existing file handle is closed.
 * The object's loaded state is reset. If not called internally (pd->setting is false),
 * it updates the regular file path property (efl_file_set) based on the new mmap.
 *
 * @param obj The Efl_File object.
 * @param pd The private data of the Efl_File object.
 * @param f The Eina_File to set. Can be NULL to unset.
 * @return 0 on success, or an Eina_Error code on failure (e.g., if eina_file_dup fails).
 */
EOLIAN static Eina_Error
_efl_file_mmap_set(Eo *obj, Efl_File_Data *pd, const Eina_File *f)
{
   Eina_Error err = 0;
   Eina_File *file = NULL;

   if (f == pd->file) return 0;
   if (f)
     {
        file = eina_file_dup(f);
        if (!file) return errno;
     }
   if (pd->file) eina_file_close(pd->file); // close matching open (dup above) OK
   pd->file = file;
   pd->loaded = EINA_FALSE;

   if (!pd->setting)
     {
        /* avoid infinite recursion */
        pd->setting = 1;
        err = efl_file_set(obj, eina_file_filename_get(pd->file));
        pd->setting = 0;
     }
   return err;
}

/**
 * @internal
 * @brief Gets the memory-mapped file from the object.
 *
 * @param obj The Efl_File object (unused).
 * @param pd The private data of the Efl_File object.
 * @return A const pointer to the Eina_File, or NULL if not set.
 */
EOLIAN static const Eina_File *
_efl_file_mmap_get(const Eo *obj EINA_UNUSED, Efl_File_Data *pd)
{
   return pd->file;
}

/**
 * @internal
 * @brief Sets the file path for the object.
 *
 * Resolves the virtual path @p file and updates the internal path stringshare.
 * It checks if the file or its modification time has changed. If so,
 * the loaded state is reset. If not called internally (pd->setting is false),
 * it resets the mmap property (efl_file_mmap_set to NULL).
 *
 * @param obj The Efl_File object.
 * @param pd The private data of the Efl_File object.
 * @param file The file path to set. Can be NULL to unset.
 * @return 0 on success or if the file path is the same and unmodified,
 *         or an Eina_Error code on failure (e.g., from stat).
 */
EOLIAN static Eina_Error
_efl_file_file_set(Eo *obj, Efl_File_Data *pd, const char *file)
{
   char *tmp;
   Eina_Error err = 0;
   Eina_Bool same;
   struct stat st;

   tmp = (char*)(file);
   if (tmp)
     tmp = eina_vpath_resolve(tmp);

   same = !eina_stringshare_replace(&pd->vpath, tmp ?: file);
   free(tmp);
   if (file)
     {
        err = stat(pd->vpath, &st);
        if (same && (!err)) same = st.st_mtime == pd->mtime;
     }
   if (same) return err;
   pd->mtime = file && (!err) ? st.st_mtime : 0;
   pd->loaded = EINA_FALSE;
   if (pd->setting)
     err = 0; /* this is from mmap_set, which may provide a virtual file */
   else
     {
        pd->setting = 1;
        err = efl_file_mmap_set(obj, NULL);
        pd->setting = 0;
     }
   return err;
}

/**
 * @internal
 * @brief Gets the file path from the object.
 *
 * @param obj The Efl_File object (unused).
 * @param pd The private data of the Efl_File object.
 * @return The Eina_Stringshare for the file path, or NULL if not set.
 */
EOLIAN static Eina_Stringshare *
_efl_file_file_get(const Eo *obj EINA_UNUSED, Efl_File_Data *pd)
{
   return pd->vpath;
}

/**
 * @internal
 * @brief Sets the key associated with the file.
 *
 * If the key changes, the object's loaded state is reset.
 *
 * @param obj The Efl_File object (unused).
 * @param pd The private data of the Efl_File object.
 * @param key The key to set. Can be NULL.
 */
EOLIAN static void
_efl_file_key_set(Eo *obj EINA_UNUSED, Efl_File_Data *pd, const char *key)
{
   if (eina_stringshare_replace(&pd->key, key))
     pd->loaded = 0;
}

/**
 * @internal
 * @brief Gets the key associated with the file.
 *
 * @param obj The Efl_File object (unused).
 * @param pd The private data of the Efl_File object.
 * @return The Eina_Stringshare for the key, or NULL if not set.
 */
EOLIAN static Eina_Stringshare *
_efl_file_key_get(const Eo *obj EINA_UNUSED, Efl_File_Data *pd)
{
   return pd->key;
}

/**
 * @internal
 * @brief Gets the loaded state of the object.
 *
 * @param obj The Efl_File object (unused).
 * @param pd The private data of the Efl_File object.
 * @return @c EINA_TRUE if the file is loaded, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_file_loaded_get(const Eo *obj EINA_UNUSED, Efl_File_Data *pd)
{
   return pd->loaded;
}

/**
 * @internal
 * @brief Destructor for the Efl_File object.
 *
 * Cleans up resources: deletes stringshares for path and key,
 * and closes the Eina_File handle if open.
 * Calls the superclass destructor.
 *
 * @param obj The Efl_File object.
 * @param pd The private data of the Efl_File object.
 */
EOLIAN static void
_efl_file_efl_object_destructor(Eo *obj, Efl_File_Data *pd)
{
   eina_stringshare_del(pd->vpath);
   eina_stringshare_del(pd->key);
   eina_file_close(pd->file); // close matching open (dup in _efl_file_mmap_set) OK
   pd->vpath = NULL;
   pd->key = NULL;
   pd->file = NULL;
   efl_destructor(efl_super(obj, EFL_FILE_MIXIN));
}

/**
 * @internal
 * @brief Finalizer for the Efl_File object.
 *
 * Calls the superclass finalizer. If successful and if a file path or
 * mmap is set, it attempts to load the file.
 *
 * @param obj The Efl_File object.
 * @param pd The private data of the Efl_File object.
 * @return The finalized Eo object, or NULL on failure.
 */
EOLIAN static Eo *
_efl_file_efl_object_finalize(Eo *obj, Efl_File_Data *pd)
{
   obj = efl_finalize(efl_super(obj, EFL_FILE_MIXIN));
   if (!obj) return NULL;
   if (pd->file || pd->vpath) efl_file_load(obj);
   return obj;
}

////////////////////////////////////////////////////////////////////////////

EAPI Eina_Bool
efl_file_simple_load(Eo *obj, const char *file, const char *key)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, EINA_FALSE);
   efl_ref(obj);
   if (efl_file_set(obj, file))
     {
        EINA_LOG_ERR("File set to '%s' on '%s' failed.", file, efl_debug_name_get(obj));
        goto fail;
     }
   efl_file_key_set(obj, key);
   if (file)
     {
        if (efl_file_load(obj)) goto fail;
        efl_unref(obj);
        return EINA_TRUE;
     }
   efl_file_unload(obj);
   efl_unref(obj);
   return EINA_TRUE;
fail:
   efl_unref(obj);
   return EINA_FALSE;
}

EAPI Eina_Bool
efl_file_simple_mmap_load(Eo *obj, const Eina_File *file, const char *key)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, EINA_FALSE);
   efl_ref(obj);
   EINA_SAFETY_ON_TRUE_GOTO(efl_file_mmap_set(obj, file), fail);
   efl_file_key_set(obj, key);
   if (file)
     {
        if (efl_file_load(obj)) goto fail;
        efl_unref(obj);
        return EINA_TRUE;
     }
   efl_file_unload(obj);
   efl_unref(obj);
   return EINA_TRUE;
fail:
   efl_unref(obj);
   return EINA_FALSE;
}

EAPI void
efl_file_simple_get(const Eo *obj, const char **file, const char **key)
{
   efl_ref((Eo*)obj);
   if (file) *file = efl_file_get(obj);
   if (key) *key = efl_file_key_get(obj);
   efl_unref((Eo*)obj);
}

EAPI void
efl_file_simple_mmap_get(const Eo *obj, const Eina_File **file, const char **key)
{
   efl_ref((Eo*)obj);
   if (file) *file = efl_file_mmap_get(obj);
   if (key) *key = efl_file_key_get(obj);
   efl_unref((Eo*)obj);
}

#include "interfaces/efl_file.eo.c"
#include "interfaces/efl_file_save.eo.c"
