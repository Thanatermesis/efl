#ifndef __EFREETD_CACHE_H
#define __EFREETD_CACHE_H

/**
 * @file
 * @brief Functions and data structures for managing Efreetd caches.
 *
 * This file declares the public API for interacting with the Efreetd
 * caching mechanisms, including desktop file caches, icon caches,
 * and MIME type caches. It handles initialization, shutdown, updates,
 * and monitoring of these caches.
 */

/**
 * @brief Mempool for stat structures.
 * Used to efficiently allocate and deallocate struct stat instances
 * during directory scanning and monitoring.
 */
extern Eina_Mempool *efreetd_mp_stat;

/**
 * @brief Adds a directory to the list of watched desktop directories.
 * If the directory is not already being monitored, it will be added,
 * and the desktop cache will be scheduled for an update.
 * @param dir The directory path to add. The path will be sanitized.
 */
void cache_desktop_dir_add(const char *dir);

/**
 * @brief Adds a directory to the list of watched icon directories.
 * If the directory is not already being monitored, it will be added,
 * and the icon cache will be scheduled for an update.
 * @param dir The directory path to add. The path will be sanitized.
 */
void cache_icon_dir_add(const char *dir);

/**
 * @brief Adds an extension to the list of watched icon extensions.
 * If the extension is not already being monitored, it will be added,
 * and the icon cache will be scheduled for an update.
 * @param ext The icon extension to add (e.g., "png", "svg").
 */
void cache_icon_ext_add(const char *ext);

/**
 * @brief Schedules an update for the desktop cache.
 * This function will trigger a rebuild of the desktop cache after a short delay.
 */
void cache_desktop_update(void);

/**
 * @brief Checks if the desktop cache has been successfully built at least once.
 * @return EINA_TRUE if the desktop cache exists, EINA_FALSE otherwise.
 */
Eina_Bool cache_desktop_exists(void);

/**
 * @brief Initializes the caching system.
 * Sets up event handlers, initializes data structures for monitoring,
 * loads existing cache lists, and starts initial cache generation processes.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool cache_init(void);

/**
 * @brief Shuts down the caching system.
 * Frees allocated resources, stops monitoring, and cleans up event handlers.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool cache_shutdown(void);

#endif
