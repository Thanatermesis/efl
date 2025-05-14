#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @brief Sets an arbitrary data pointer on an Evas object, associated with a key.
 * @param eo_obj The Evas object.
 * @param key A string key to associate the data with.
 * @param data A pointer to the data to store.
 *
 * This function acts as a wrapper around efl_key_data_set(), ensuring
 * that the provided Evas_Object is a valid EFL_CANVAS_OBJECT_CLASS instance.
 */
EVAS_API void
evas_object_data_set(Evas_Object *eo_obj, const char *key, const void *data)
{
   if (!efl_isa(eo_obj, EFL_CANVAS_OBJECT_CLASS)) return;
   efl_key_data_set(eo_obj, key, data);
}

/**
 * @brief Retrieves an arbitrary data pointer from an Evas object, associated with a key.
 * @param eo_obj The Evas object.
 * @param key The string key associated with the data.
 * @return The data pointer previously set with the given key, or NULL if not found
 *         or if eo_obj is not a valid EFL_CANVAS_OBJECT_CLASS instance.
 *
 * This function acts as a wrapper around efl_key_data_get(), ensuring
 * that the provided Evas_Object is a valid EFL_CANVAS_OBJECT_CLASS instance.
 */
EVAS_API void *
evas_object_data_get(const Evas_Object *eo_obj, const char *key)
{
   if (!efl_isa(eo_obj, EFL_CANVAS_OBJECT_CLASS)) return NULL;
   return efl_key_data_get(eo_obj, key);
}

/**
 * @brief Deletes an arbitrary data pointer from an Evas object, associated with a key.
 * @param eo_obj The Evas object.
 * @param key The string key associated with the data to delete.
 * @return The data pointer that was associated with the key, or NULL if not found
 *         or if eo_obj is not a valid EFL_CANVAS_OBJECT_CLASS instance.
 *
 * This function effectively removes the data by setting its value to NULL
 * using efl_key_data_set() after retrieving the current data. It ensures
 * that the provided Evas_Object is a valid EFL_CANVAS_OBJECT_CLASS instance.
 */
EVAS_API void *
evas_object_data_del(Evas_Object *eo_obj, const char *key)
{
   void *data;

   if (!efl_isa(eo_obj, EFL_CANVAS_OBJECT_CLASS)) return NULL;
   data = efl_key_data_get(eo_obj, key);
   efl_key_data_set(eo_obj, key, NULL);
   return data;
}
