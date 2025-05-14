#ifndef _ECORE_BUFFER_con_H_
#define _ECORE_BUFFER_con_H_

#include <stdio.h>
#include <wayland-client.h>

#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Buffer.h>

#include "bq_mgr_protocol.h"
#include "ecore_buffer_private.h"

/**
 * @brief Initializes the Ecore_Buffer connection module.
 *
 * This function establishes a connection to the Wayland display and
 * sets up the necessary listeners and handlers for buffer management.
 *
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool             _ecore_buffer_con_init(void);

/**
 * @brief Shuts down the Ecore_Buffer connection module.
 *
 * This function disconnects from the Wayland display and cleans up
 * all resources used by the connection module.
 */
void                  _ecore_buffer_con_shutdown(void);

/**
 * @brief Waits for the Ecore_Buffer connection to be fully initialized.
 *
 * This function blocks until the initial Wayland display sync is complete,
 * ensuring that the buffer manager is ready for use.
 */
void                  _ecore_buffer_con_init_wait(void);

/**
 * @brief Creates a buffer provider.
 *
 * This function requests the buffer manager to create a new buffer provider
 * with the given name.
 *
 * @param name The name of the provider to create.
 * @return A pointer to the created bq_provider structure on success,
 *         NULL otherwise.
 */
struct bq_provider   *_ecore_buffer_con_provider_create(const char *name);

/**
 * @brief Creates a buffer consumer.
 *
 * This function requests the buffer manager to create a new buffer consumer
 * with the given name and queue parameters.
 *
 * @param name The name of the consumer to create.
 * @param queue_size The desired size of the buffer queue.
 * @param w The width of the buffers in the queue.
 * @param h The height of the buffers in the queue.
 * @return A pointer to the created bq_consumer structure on success,
 *         NULL otherwise.
 */
struct bq_consumer   *_ecore_buffer_con_consumer_create(const char *name, int queue_size, int w, int h);

#endif
