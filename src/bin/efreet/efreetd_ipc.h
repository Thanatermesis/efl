#ifndef __EFREETD_IPC_H
#define __EFREETD_IPC_H

/**
 * @file
 * @brief IPC communication functions for efreetd.
 *
 * This file defines the functions used for inter-process communication
 * between efreetd and its clients. It handles signals for cache updates
 * and manages the IPC server lifecycle.
 */

/**
 * @brief Sends a signal to all connected clients that the icon cache has been updated.
 * @param update EINA_TRUE if the cache was updated, EINA_FALSE otherwise.
 */
void send_signal_icon_cache_update(Eina_Bool update);

/**
 * @brief Sends a signal to all connected clients that the desktop cache has been updated.
 * @param update EINA_TRUE if the cache was updated, EINA_FALSE otherwise.
 */
void send_signal_desktop_cache_update(Eina_Bool update);

/**
 * @brief Sends a signal to all connected clients that the desktop cache is being built.
 */
void send_signal_desktop_cache_build(void);

/**
 * @brief Sends a signal to all connected clients that the MIME cache is being built.
 */
void send_signal_mime_cache_build(void);

/**
 * @brief Initializes the IPC server.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool ipc_init(void);

/**
 * @brief Shuts down the IPC server.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool ipc_shutdown(void);

#endif
