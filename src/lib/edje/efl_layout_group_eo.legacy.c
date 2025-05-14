/**
 * @brief Legacy implementation for edje_object_data_get.
 *
 * This function wraps efl_layout_group_data_get for legacy compatibility.
 * Refer to the declaration in efl_layout_group_eo.legacy.h for full
 * API documentation.
 *
 * @param obj The Efl_Layout_Group object.
 * @param key The key for the data to retrieve.
 * @return The value associated with the key, or NULL if not found.
 */
EAPI const char *
edje_object_data_get(const Efl_Layout_Group *obj, const char *key)
{
   return efl_layout_group_data_get(obj, key);
}
