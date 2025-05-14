#ifndef _BUFFER_QUEUE_H_
#define _BUFFER_QUEUE_H_

#include <stdio.h>
#include <Eina.h>

#include "shared_buffer.h"

/**
 * @file buffer_queue.h
 * @brief Defines the Ecore_Buffer_Queue data structure and its associated functions.
 *
 * This queue is used to manage shared buffers, typically in a producer-consumer scenario.
 * It allows for enqueuing and dequeuing buffers, managing a list of known shared buffers,
 * and tracking a connection state.
 */

/**
 * @typedef Ecore_Buffer_Queue
 * @brief An opaque type representing a buffer queue.
 *
 * This structure manages a queue of Shared_Buffer objects.
 */
typedef struct _Ecore_Buffer_Queue Ecore_Buffer_Queue;

/**
 * @brief Creates a new buffer queue.
 * @param w The width of the buffers in the queue.
 * @param h The height of the buffers in the queue.
 * @param queue_size The maximum number of buffers the queue can hold. Must be >= 1.
 * @return A pointer to the newly created Ecore_Buffer_Queue, or NULL on failure.
 */
Ecore_Buffer_Queue   *_ecore_buffer_queue_new(int w, int h, int queue_size);

/**
 * @brief Frees the resources associated with a buffer queue.
 * @param ebq The buffer queue to free.
 */
void                  _ecore_buffer_queue_free(Ecore_Buffer_Queue *ebq);

/**
 * @brief Enqueues a shared buffer into the buffer queue.
 *
 * The buffer is added to the beginning of the queue.
 * The buffer must already be registered as a shared buffer with the queue.
 * If the queue is full or the buffer is not shared, the operation fails.
 *
 * @param ebq The buffer queue.
 * @param sb The shared buffer to enqueue.
 */
void                  _ecore_buffer_queue_enqueue(Ecore_Buffer_Queue *ebq, Shared_Buffer *sb);

/**
 * @brief Dequeues a shared buffer from the buffer queue.
 *
 * The buffer is removed from the end of the queue (FIFO).
 * If the queue is empty, the operation fails.
 *
 * @param ebq The buffer queue.
 * @param ret_sb Pointer to store the dequeued Shared_Buffer. Can be NULL if not needed.
 * @return EINA_TRUE if a buffer was successfully dequeued, EINA_FALSE otherwise.
 */
Eina_Bool             _ecore_buffer_queue_dequeue(Ecore_Buffer_Queue *ebq, Shared_Buffer **ret_sb);

/**
 * @brief Checks if the buffer queue is empty.
 * @param ebq The buffer queue.
 * @return EINA_TRUE if the queue is empty, EINA_FALSE otherwise.
 */
Eina_Bool             _ecore_buffer_queue_is_empty(Ecore_Buffer_Queue *ebq);

/**
 * @brief Adds a shared buffer to the list of buffers managed by the queue.
 *
 * This function registers a shared buffer with the queue, allowing it to be
 * enqueued later. It does not add the buffer to the actual processing queue.
 *
 * @param ebq The buffer queue.
 * @param sb The shared buffer to add.
 */
void                  _ecore_buffer_queue_shared_buffer_add(Ecore_Buffer_Queue *ebq, Shared_Buffer *sb);

/**
 * @brief Removes a shared buffer from the list of managed buffers and from the queue itself.
 *
 * This function unregisters a shared buffer. If the buffer is currently in the
 * processing queue, it will also be removed from there.
 *
 * @param ebq The buffer queue.
 * @param sb The shared buffer to remove.
 */
void                  _ecore_buffer_queue_shared_buffer_remove(Ecore_Buffer_Queue *ebq, Shared_Buffer *sb);

/**
 * @brief Finds a shared buffer associated with a given Ecore_Buffer.
 * @param ebq The buffer queue.
 * @param buffer The Ecore_Buffer to search for.
 * @return The Shared_Buffer if found, NULL otherwise.
 */
Shared_Buffer        *_ecore_buffer_queue_shared_buffer_find(Ecore_Buffer_Queue *ebq, Ecore_Buffer *buffer);

/**
 * @brief Gets the list of all shared buffers managed by the queue.
 * @param ebq The buffer queue.
 * @return A pointer to the Eina_List of Shared_Buffer objects.
 *         The list should not be modified by the caller.
 */
Eina_List            *_ecore_buffer_queue_shared_buffer_list_get(Ecore_Buffer_Queue *ebq);

/**
 * @brief Sets the connection state of the buffer queue.
 * @param ebq The buffer queue.
 * @param connect The connection state to set (EINA_TRUE for connected, EINA_FALSE for disconnected).
 */
void                  _ecore_buffer_queue_connection_state_set(Ecore_Buffer_Queue *ebq, Eina_Bool connect);

/**
 * @brief Gets the connection state of the buffer queue.
 * @param ebq The buffer queue.
 * @return EINA_TRUE if connected, EINA_FALSE otherwise.
 */
Eina_Bool             _ecore_buffer_queue_connection_state_get(Ecore_Buffer_Queue *ebq);

#endif
