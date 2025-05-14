#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include <Elementary_Cursor.h>
#include "elm_priv.h"

#define MY_CLASS EFL_UI_TEXT_FACTORY_IMAGES_CLASS

/**
 * @brief Private data for the Efl_Ui_Text_Factory_Images class.
 * @since 1.24
 */
typedef struct _Efl_Ui_Text_Factory_Images_Data Efl_Ui_Text_Factory_Images_Data;

struct _Efl_Ui_Text_Factory_Images_Data
{
   const char *name; /**< A name associated with this factory instance, currently unused. */
   Eina_Hash  *hash; /**< Hash table storing File_Entry objects, keyed by a name. */
};

/**
 * @brief Structure to hold image file information.
 * @since 1.24
 */
typedef struct
{
   Eina_File  *file; /**< Pointer to an Eina_File representing the image file. */
   const char *key;  /**< Optional key for images within a container file (e.g., EET). */
} File_Entry;

/**
 * @brief Frees a File_Entry structure.
 *
 * This function is used as a callback for eina_hash_free_buckets.
 * It closes the Eina_File, deletes the stringshared key, and frees the entry itself.
 *
 * @param data Pointer to the File_Entry to be freed.
 */
static void
_entry_free_cb(void *data)
{
   File_Entry *e = data;
   eina_file_close(e->file);
   eina_stringshare_del(e->key);
   free(e);
}

/**
 * @brief Constructor for the Efl_Ui_Text_Factory_Images class.
 *
 * Initializes the internal hash table for storing image mappings.
 *
 * @param obj The Efl_Object instance.
 * @param pd The private data for the instance.
 * @return The constructed Efl_Object.
 */
EOLIAN static Eo *
_efl_ui_text_factory_images_efl_object_constructor(Eo *obj,
     Efl_Ui_Text_Factory_Images_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   pd->hash = eina_hash_string_superfast_new(_entry_free_cb);
   return obj;
}

/**
 * @brief Destructor for the Efl_Ui_Text_Factory_Images class.
 *
 * Frees the internal hash table.
 *
 * @param obj The Efl_Object instance.
 * @param pd The private data for the instance.
 */
EOLIAN static void
_efl_ui_text_factory_images_efl_object_destructor(Eo *obj,
     Efl_Ui_Text_Factory_Images_Data *pd EINA_UNUSED)
{
   eina_hash_free(pd->hash);
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Creates an image object based on a key.
 *
 * This function is called by Efl.Canvas.Textblock to create an image object
 * when an image tag (e.g., <img src=KEY>) is encountered in the text.
 * The KEY is used to look up a pre-registered image file or mmap.
 *
 * @param obj The Efl_Ui_Text_Factory_Images instance.
 * @param pd The private data for the instance.
 * @param object The parent Efl_Canvas_Object (usually the textblock itself).
 * @param key The key specified in the text (e.g., the 'src' attribute of an <img> tag).
 * @return A new Efl_Canvas_Object (image) on success, or @c NULL on failure.
 */
EOLIAN static Efl_Canvas_Object *
_efl_ui_text_factory_images_efl_canvas_textblock_factory_create(Eo *obj EINA_UNUSED,
      Efl_Ui_Text_Factory_Images_Data *pd EINA_UNUSED,
      Efl_Canvas_Object *object,
      const char *key)
{
   Efl_Canvas_Object *o;
   File_Entry *e;

   o = efl_add(EFL_CANVAS_IMAGE_CLASS, object);
   e = eina_hash_find(pd->hash, key);
   if (e)
     {
        efl_file_key_set(o, e->key);
        if (efl_file_mmap_set(o, e->file)) goto error;
     }
   else
     {
        if (efl_file_set(o, key)) goto error;
     }
   if (efl_file_load(o)) goto error;

   return o;

error:
   efl_del(o);
   return NULL;
}

/**
 * @brief Adds an image file path to be associated with a name.
 *
 * When textblock encounters an image tag with `src=name`, it will
 * load the image from the given `file` path. If `key` is provided,
 * it's used for images within container files (like EET).
 *
 * @param obj The Efl_Ui_Text_Factory_Images instance.
 * @param pd The private data for the instance.
 * @param name The name to associate with the image (used as `src` in text).
 * @param file The path to the image file.
 * @param key Optional key for images within a container file. Can be @c NULL.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_ui_text_factory_images_matches_add(Eo *obj EINA_UNUSED,
      Efl_Ui_Text_Factory_Images_Data *pd,
      const char *name, const char *file, const char *key)
{
   File_Entry *e;
   Eina_File *f = eina_file_open(file, EINA_FALSE);

   if (!f) return EINA_FALSE;

   e = malloc(sizeof(*e));
   e->file = f;
   e->key = eina_stringshare_add(key);

   if (!eina_hash_add(pd->hash, name, e))
     {
        ERR("Failed to add file path %s to key %s\n", file, key);
        eina_file_close(f);
        free(e);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Deletes an image association by its name.
 *
 * Removes a previously added image mapping.
 *
 * @param obj The Efl_Ui_Text_Factory_Images instance.
 * @param pd The private data for the instance.
 * @param name The name of the image association to delete.
 * @return @c EINA_TRUE if the entry was found and deleted, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_text_factory_images_matches_del(Eo *obj EINA_UNUSED,
      Efl_Ui_Text_Factory_Images_Data *pd,
      const char *name)
{
   return eina_hash_del(pd->hash, name, NULL);
}

/**
 * @brief Adds an Eina_File (mmap) to be associated with a name.
 *
 * Similar to efl_ui_text_factory_images_matches_add(), but uses an existing
 * Eina_File (which might be an mmap). The factory will duplicate the Eina_File.
 *
 * @param obj The Efl_Ui_Text_Factory_Images instance.
 * @param pd The private data for the instance.
 * @param name The name to associate with the image (used as `src` in text).
 * @param file Pointer to the Eina_File to use.
 * @param key Optional key for images within a container file. Can be @c NULL.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_ui_text_factory_images_matches_mmap_add(Eo *obj EINA_UNUSED,
      Efl_Ui_Text_Factory_Images_Data *pd,
      const char *name, const Eina_File *file, const char *key)
{
   File_Entry *e;
   Eina_File *f;

   if (!file) return EINA_FALSE;

   f = eina_file_dup(file);
   e = malloc(sizeof(*e));
   e->file = f;
   e->key = eina_stringshare_add(key);

   if (!eina_hash_add(pd->hash, name, e))
     {
        ERR("Failed to add Eina_File %p to key %s\n", file, key);
        eina_file_close(f);
        free(e);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Deletes an mmap image association by its name.
 *
 * Removes a previously added mmap image mapping. This is functionally
 * identical to efl_ui_text_factory_images_matches_del() as the underlying
 * storage mechanism is the same.
 *
 * @param obj The Efl_Ui_Text_Factory_Images instance.
 * @param pd The private data for the instance.
 * @param name The name of the mmap image association to delete.
 * @return @c EINA_TRUE if the entry was found and deleted, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_text_factory_images_matches_mmap_del(Eo *obj EINA_UNUSED,
      Efl_Ui_Text_Factory_Images_Data *pd,
      const char *name)
{
   return eina_hash_del(pd->hash, name, NULL);
}

#include "efl_ui_text_factory_images.eo.c"
