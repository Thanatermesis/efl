#ifndef _EVAS_MODULE_H
#define _EVAS_MODULE_H

/**
 * @file
 * @brief These routines are used for Evas module handling.
 */

/**
 * @internal
 * @struct _Evas_Module_Path
 * @brief Represents a module path with its associated type.
 *
 * This structure is used internally by the module API to associate a file path
 * with a specific Evas_Module_Type, rather than deducing the type from the path string.
 */
struct _Evas_Module_Path
{
   Evas_Module_Type	type; /**< The type of the module (e.g., engine, image loader). */
   char		       *path; /**< The file system path to the module. */
};

/**
 * @brief Initializes the search paths for Evas modules.
 *
 * This function determines and stores the directories where Evas will look for modules.
 * The paths typically include:
 * 1. ~/.evas/modules/
 * 2. $(EVAS_MODULE_DIR)/evas/modules/ (if EVAS_MODULE_DIR is set)
 * 3. Path relative to the libevas.so location (e.g., libevas.so/../evas/modules/)
 * 4. Default system path (e.g., PREFIX/lib/evas/modules/)
 * If `EFL_RUN_IN_TREE` is set and the build directory is found, it will prioritize
 * modules from `PACKAGE_BUILD_DIR/src/modules/evas`.
 */
void         evas_module_paths_init (void);

/**
 * @brief Initializes the Evas module system.
 *
 * This function sets up the necessary structures for module management,
 * including hash tables for different module types and initializes
 * thread-local storage for module tasks. It also initializes any
 * statically linked modules.
 */
void         evas_module_init       (void);

/**
 * @brief Finds a module of a specific type by its name.
 *
 * Searches for a module matching the given type and name. It first checks
 * already registered (e.g., static) modules. If not found, it then searches
 * the file system in the configured module paths. If a dynamic module is found,
 * it attempts to load it.
 *
 * @param type The type of the module to find (e.g., EVAS_MODULE_TYPE_ENGINE).
 * @param name The name of the module (e.g., "software_x11", "png").
 * @return A pointer to the Evas_Module if found and loaded successfully, otherwise NULL.
 *         Example: evas_module_find_type(EVAS_MODULE_TYPE_IMAGE_LOADER, "png");
 */
Evas_Module *evas_module_find_type  (Evas_Module_Type type, const char *name);

/**
 * @brief Retrieves an engine module by its render method ID.
 *
 * This function looks up a pre-loaded engine module based on an integer ID
 * that typically corresponds to a specific rendering method.
 *
 * @param render_method The ID of the render method. This ID is usually
 *        obtained from Evas engine enumeration or configuration.
 * @return A pointer to the Evas_Module if an engine with the given ID exists,
 *         otherwise NULL.
 */
Evas_Module *evas_module_engine_get(int render_method);

/**
 * @brief Iterates over all registered image loader modules.
 *
 * This function applies a callback function to each registered image loader module.
 *
 * @param cb The callback function to apply.
 *           The callback signature is: Eina_Bool (*Eina_Hash_Foreach)(const Eina_Hash *hash, const void *key, void *data, void *fdata);
 *           Where `data` will be an `Evas_Module *`.
 * @param fdata User data to be passed to the callback function.
 */
void         evas_module_foreach_image_loader(Eina_Hash_Foreach cb, const void *fdata);

/**
 * @brief Loads an Evas module.
 *
 * If the module is not already loaded, this function calls the module's
 * open function.
 *
 * @param em Pointer to the Evas_Module to load.
 * @return 1 if the module is successfully loaded or already loaded, 0 on failure.
 */
int          evas_module_load       (Evas_Module *em);

/**
 * @brief Unloads an Evas module.
 *
 * Currently, this function is a no-op to prevent instability. Modules are
 * generally not unloaded once loaded.
 *
 * @param em Pointer to the Evas_Module to unload.
 */
void         evas_module_unload     (Evas_Module *em);

/**
 * @brief Increments the reference count of an Evas module.
 *
 * @param em Pointer to the Evas_Module to reference.
 */
void         evas_module_ref        (Evas_Module *em);

/**
 * @brief Decrements the reference count of an Evas module.
 *
 * @param em Pointer to the Evas_Module to unreference.
 */
void         evas_module_unref      (Evas_Module *em);

/**
 * @brief Marks an Evas module as recently used.
 *
 * This is used by the module cleaning mechanism to determine if a module
 * can be unloaded (though cleaning is currently disabled).
 *
 * @param em Pointer to the Evas_Module to mark as used.
 */
void         evas_module_use        (Evas_Module *em);

/**
 * @brief Cleans up unused Evas modules.
 *
 * This function is intended to unload modules that haven't been used recently
 * and have a reference count of zero. However, module cleaning is currently
 * disabled by default due to potential instability. It can be re-enabled
 * by unsetting the `EVAS_NOCLEAN` environment variable, but this is not recommended.
 * The cleaning logic is also only triggered periodically (every 256 calls).
 */
void         evas_module_clean      (void);

/**
 * @brief Shuts down the Evas module system.
 *
 * This function frees all resources associated with the module system,
 * including hash tables, module paths, and calls the shutdown function
 * for any statically linked modules.
 */
void         evas_module_shutdown   (void);

#endif /* _EVAS_MODULE_H */
