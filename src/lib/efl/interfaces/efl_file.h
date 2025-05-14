#ifndef _EFL_FILE_H
# define _EFL_FILE_H

/**
 * @file
 * @brief These functions provide a simple way to load and get file properties.
 * @note When passing an Eo object (efl_part) to these functions,
 *       ensure that its reference count is properly managed (efl_ref/efl_unref)
 *       if the object's lifetime is not guaranteed to extend beyond the function call.
 *       These simple_load/get functions internally handle ref/unref for the passed object.
 */

/**
 * @brief Loads the properties of a file into an Eo object.
 *
 * This function sets the file path and key for the object, then attempts to load it.
 * If @p file is NULL, it effectively unloads the object.
 *
 * @param[in] obj The Eo object to load the file into.
 * @param[in] file The path to the file to load. Can be NULL to unload.
 * @param[in] key The key associated with the file, if any. Can be NULL.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool efl_file_simple_load(Eo *obj, const char *file, const char *key);

/**
 * @brief Loads the properties of a memory-mapped file into an Eo object.
 *
 * This function sets the Eina_File (mmap) and key for the object, then attempts to load it.
 * If @p file is NULL, it effectively unloads the object.
 *
 * @param[in] obj The Eo object to load the file into.
 * @param[in] file The Eina_File (mmap) to load. Can be NULL to unload.
 * @param[in] key The key associated with the file, if any. Can be NULL.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool efl_file_simple_mmap_load(Eo *obj, const Eina_File *file, const char *key);

/**
 * @brief Gets the file path and key from an Eo object.
 *
 * @param[in] obj The Eo object.
 * @param[out] file Pointer to store the file path. Contents are valid as long as obj is valid and not modified.
 * @param[out] key Pointer to store the key. Contents are valid as long as obj is valid and not modified.
 */
EAPI void efl_file_simple_get(const Eo *obj, const char **file, const char **key);

/**
 * @brief Gets the Eina_File (mmap) and key from an Eo object.
 *
 * @param[in] obj The Eo object.
 * @param[out] file Pointer to store the Eina_File. Contents are valid as long as obj is valid and not modified.
 * @param[out] key Pointer to store the key. Contents are valid as long as obj is valid and not modified.
 */
EAPI void efl_file_simple_mmap_get(const Eo *obj, const Eina_File **file, const char **key);

#endif
