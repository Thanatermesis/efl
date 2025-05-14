#include "buffer_queue.h"

/**
 * @brief Structure representing a buffer queue.
 *
 * This structure holds information about the queue, including its dimensions,
 * a list of shared buffers, connection state, and the queue itself with its capacity.
 */
struct _Ecore_Buffer_Queue
{
   int w, h; /**< Width and height of the buffers in the queue. */
   Eina_List *shared_buffers; /**< List of all shared buffers associated with this queue. */
   Eina_Bool connected; /**< Flag indicating if the queue is connected. */
   struct
   {
      unsigned int capacity; /**< Maximum number of buffers the queue can hold. */
      Eina_List *list; /**< The actual list of buffers in the queue. */
   } queue; /**< Queue specific data. */
};

/**
 * @brief Checks if the buffer queue is full.
 * @param ebq The buffer queue to check.
 * @return EINA_TRUE if the queue is full, EINA_FALSE otherwise.
 */
static Eina_Bool
_queue_is_full(Ecore_Buffer_Queue *ebq)
{
   return (eina_list_count(ebq->queue.list) == ebq->queue.capacity);
}

/**
 * @brief Checks if the buffer queue is empty.
 * @param ebq The buffer queue to check.
 * @return EINA_TRUE if the queue is empty, EINA_FALSE otherwise.
 */
static Eina_Bool
_queue_is_empty(Ecore_Buffer_Queue *ebq)
{
   return (eina_list_count(ebq->queue.list) == 0);
}

/**
 * @brief Creates a new buffer queue.
 * @param w The width of the buffers in the queue.
 * @param h The height of the buffers in the queue.
 * @param queue_size The maximum number of buffers the queue can hold. Must be >= 1.
 * @return A pointer to the newly created Ecore_Buffer_Queue, or NULL on failure.
 */
Ecore_Buffer_Queue *
_ecore_buffer_queue_new(int w, int h, int queue_size)
{
   Ecore_Buffer_Queue *ebq;

   if (queue_size < 1) return NULL;

   ebq = calloc(1, sizeof(Ecore_Buffer_Queue));
   if (!ebq)
     return NULL;

   ebq->w = w;
   ebq->h = h;
   ebq->queue.capacity = queue_size;

   return ebq;
}

/**
 * @brief Frees the resources associated with a buffer queue.
 * @param ebq The buffer queue to free.
 */
void
_ecore_buffer_queue_free(Ecore_Buffer_Queue *ebq)
{
   if (!ebq) return;

   if (ebq->shared_buffers) eina_list_free(ebq->shared_buffers);
   if (ebq->queue.list) eina_list_free(ebq->queue.list);
   free(ebq);
}

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
void
_ecore_buffer_queue_enqueue(Ecore_Buffer_Queue *ebq, Shared_Buffer *sb)
{
   if (!ebq) return;
   if (_queue_is_full(ebq)) return;

   if (!eina_list_data_find(ebq->shared_buffers, sb))
     {
        WARN("Couldn't enqueue not shared buffer.");
        return;
     }

   ebq->queue.list = eina_list_prepend(ebq->queue.list, sb);
}

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
Eina_Bool
_ecore_buffer_queue_dequeue(Ecore_Buffer_Queue *ebq, Shared_Buffer **ret_sb)
{
   Eina_List *last;
   Shared_Buffer *sb;

   if (!ebq) return EINA_FALSE;
   if (_queue_is_empty(ebq)) return EINA_FALSE;

   sb = eina_list_last_data_get(ebq->queue.list);
   last = eina_list_last(ebq->queue.list);
   ebq->queue.list = eina_list_remove_list(ebq->queue.list, last);

   if (ret_sb) *ret_sb = sb;

   return EINA_TRUE;
}

/**
 * @brief Checks if the buffer queue is empty.
 * @param ebq The buffer queue.
 * @return EINA_TRUE if the queue is empty, EINA_FALSE otherwise.
 */
Eina_Bool
_ecore_buffer_queue_is_empty(Ecore_Buffer_Queue *ebq)
{
   if (!ebq) return EINA_FALSE;

   return _queue_is_empty(ebq);
}

/**
 * @brief Adds a shared buffer to the list of buffers managed by the queue.
 *
 * This function registers a shared buffer with the queue, allowing it to be
 * enqueued later. It does not add the buffer to the actual processing queue.
 *
 * @param ebq The buffer queue.
 * @param sb The shared buffer to add.
 */
void
_ecore_buffer_queue_shared_buffer_add(Ecore_Buffer_Queue *ebq, Shared_Buffer *sb)
{
   if (!ebq) return;

   ebq->shared_buffers = eina_list_append(ebq->shared_buffers, sb);
}

/**
 * @brief Removes a shared buffer from the list of managed buffers and from the queue itself.
 *
 * This function unregisters a shared buffer. If the buffer is currently in the
 * processing queue, it will also be removed from there.
 *
 * @param ebq The buffer queue.
 * @param sb The shared buffer to remove.
 */
void
_ecore_buffer_queue_shared_buffer_remove(Ecore_Buffer_Queue *ebq, Shared_Buffer *sb)
{
   if (!ebq) return;

   ebq->shared_buffers = eina_list_remove(ebq->shared_buffers, sb);
   while (eina_list_data_find(ebq->queue.list, sb) != NULL)
     ebq->queue.list = eina_list_remove(ebq->queue.list, sb);
}

/**
 * @brief Finds a shared buffer associated with a given Ecore_Buffer.
 * @param ebq The buffer queue.
 * @param buffer The Ecore_Buffer to search for.
 * @return The Shared_Buffer if found, NULL otherwise.
 */
Shared_Buffer *
_ecore_buffer_queue_shared_buffer_find(Ecore_Buffer_Queue *ebq, Ecore_Buffer *buffer)
{
   Eina_List *l;
   Shared_Buffer *sb;

   if (!ebq) return NULL;

   EINA_LIST_FOREACH(ebq->shared_buffers, l, sb)
     {
        if (_shared_buffer_buffer_get(sb) == buffer)
          return sb;
     }

   return NULL;
}

/**
 * @brief Gets the list of all shared buffers managed by the queue.
 * @param ebq The buffer queue.
 * @return A pointer to the Eina_List of Shared_Buffer objects.
 *         The list should not be modified by the caller.
 */
Eina_List *
_ecore_buffer_queue_shared_buffer_list_get(Ecore_Buffer_Queue *ebq)
{
   if (!ebq) return NULL;

   return ebq->shared_buffers;
}

/**
 * @brief Sets the connection state of the buffer queue.
 * @param ebq The buffer queue.
 * @param connect The connection state to set (EINA_TRUE for connected, EINA_FALSE for disconnected).
 */
void
_ecore_buffer_queue_connection_state_set(Ecore_Buffer_Queue *ebq, Eina_Bool connect)
{
   if (!ebq) return;

   ebq->connected = connect;
}

/**
 * @brief Gets the connection state of the buffer queue.
 * @param ebq The buffer queue.
 * @return EINA_TRUE if connected, EINA_FALSE otherwise.
 */
Eina_Bool
_ecore_buffer_queue_connection_state_get(Ecore_Buffer_Queue *ebq)
{
   if (!ebq) return EINA_FALSE;

   return ebq->connected;
}
