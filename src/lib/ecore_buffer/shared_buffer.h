/**
 * @file
 * @brief This file defines the Shared_Buffer structure and related functions
 *        for managing shared buffers between a provider and a consumer.
 */
#ifndef _SHARED_BUFFER_H_
# define _SHARED_BUFFER_H_

#include <stdio.h>
#include <Eina.h>
#include <Ecore_Buffer.h>

#include "bq_mgr_protocol.h"
#include "ecore_buffer_private.h"

/**
 * @brief Opaque structure representing a shared buffer.
 */
typedef struct _Shared_Buffer Shared_Buffer;

/**
 * @brief Enumeration of possible states for a Shared_Buffer.
 *
 * These states track the buffer's lifecycle from creation to release,
 * covering both provider and consumer perspectives.
 */
typedef enum _Shared_Buffer_State
{
   // common
   SHARED_BUFFER_STATE_UNKNOWN, /**< Initial or error state. */
   SHARED_BUFFER_STATE_ENQUEUE, /**< Buffer is enqueued (provider or consumer). */
   // provider side type
   SHARED_BUFFER_STATE_NEW,     /**< Buffer newly created by provider. */
   SHARED_BUFFER_STATE_SUBMIT,  /**< Buffer submitted by provider. */
   SHARED_BUFFER_STATE_DEQUEUE, /**< Buffer dequeued by provider. */
   // consumer side type
   SHARED_BUFFER_STATE_ATTACH,  /**< Buffer attached by consumer. */
   SHARED_BUFFER_STATE_IMPORT,  /**< Buffer imported by consumer. */
   SHARED_BUFFER_STATE_DETACH,  /**< Buffer detached by consumer. */
   SHARED_BUFFER_STATE_ACQUIRE, /**< Buffer acquired by consumer. */
   SHARED_BUFFER_STATE_RELEASE, /**< Buffer released by consumer. */
} Shared_Buffer_State;

/**
 * @brief Creates a new Shared_Buffer instance.
 *
 * @param engine The name of the engine associated with this buffer.
 * @param resource A pointer to the underlying buffer queue resource.
 * @param w The width of the buffer in pixels.
 * @param h The height of the buffer in pixels.
 * @param format The pixel format of the buffer.
 * @param flags Flags associated with the buffer.
 * @return A pointer to the newly created Shared_Buffer, or NULL on failure.
 */
Shared_Buffer        *_shared_buffer_new(const char *engine, struct bq_buffer *resource, int w, int h, int format, unsigned int flags);

/**
 * @brief Frees a Shared_Buffer instance.
 *
 * @param sb The Shared_Buffer to free.
 */
void                  _shared_buffer_free(Shared_Buffer *sb);

/**
 * @brief Retrieves information about the Shared_Buffer.
 *
 * @param sb The Shared_Buffer instance.
 * @param[out] engine Pointer to store the engine name.
 * @param[out] w Pointer to store the width.
 * @param[out] h Pointer to store the height.
 * @param[out] format Pointer to store the format.
 * @param[out] flags Pointer to store the flags.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool             _shared_buffer_info_get(Shared_Buffer *sb, const char **engine, int *w, int *h, int *format, unsigned int *flags);

/**
 * @brief Sets the Ecore_Buffer associated with this Shared_Buffer.
 *
 * This is typically used to link an Ecore_Buffer wrapper to the shared resource.
 *
 * @param sb The Shared_Buffer instance.
 * @param buffer The Ecore_Buffer to associate.
 * @return EINA_TRUE on success, EINA_FALSE if a buffer is already set or on error.
 */
Eina_Bool             _shared_buffer_buffer_set(Shared_Buffer *sb, Ecore_Buffer *buffer);

/**
 * @brief Gets the Ecore_Buffer associated with this Shared_Buffer.
 *
 * @param sb The Shared_Buffer instance.
 * @return The associated Ecore_Buffer, or NULL if not set or on error.
 */
Ecore_Buffer         *_shared_buffer_buffer_get(Shared_Buffer *sb);

/**
 * @brief Sets the bq_buffer resource for this Shared_Buffer.
 *
 * @param sb The Shared_Buffer instance.
 * @param resource The bq_buffer resource to associate.
 * @return EINA_TRUE on success, EINA_FALSE if a resource is already set or on error.
 */
Eina_Bool             _shared_buffer_resource_set(Shared_Buffer *sb, struct bq_buffer *resource);

/**
 * @brief Gets the bq_buffer resource associated with this Shared_Buffer.
 *
 * @param sb The Shared_Buffer instance.
 * @return The associated bq_buffer resource, or NULL if not set or on error.
 */
struct bq_buffer     *_shared_buffer_resource_get(Shared_Buffer *sb);

/**
 * @brief Sets the state of the Shared_Buffer.
 *
 * @param sb The Shared_Buffer instance.
 * @param state The new state to set.
 */
void                  _shared_buffer_state_set(Shared_Buffer *sb, Shared_Buffer_State state);

/**
 * @brief Gets the current state of the Shared_Buffer.
 *
 * @param sb The Shared_Buffer instance.
 * @return The current Shared_Buffer_State.
 */
Shared_Buffer_State   _shared_buffer_state_get(Shared_Buffer *sb);

/**
 * @brief Gets a string representation of the Shared_Buffer's current state.
 *
 * @param sb The Shared_Buffer instance.
 * @return A string describing the state, or "INVALID OBJECT" if sb is NULL.
 */
const char           *_shared_buffer_state_string_get(Shared_Buffer *sb);

#endif /* _SHARED_BUFFER_H_ */
