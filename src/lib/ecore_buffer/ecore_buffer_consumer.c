#include <stdio.h>
#include <unistd.h>

#include <Eina.h>
#include <Ecore_Buffer.h>
#include <Ecore_Buffer_Queue.h>

#include "bq_mgr_protocol.h"
#include "shared_buffer.h"
#include "buffer_queue.h"
#include "ecore_buffer_private.h"
#include "ecore_buffer_con.h"

/**
 * @internal
 * @brief Represents a consumer of Ecore_Buffer objects.
 *
 * This structure manages the consumption of buffers from a buffer queue,
 * handling communication with the provider and managing the lifecycle of
 * shared buffers.
 */
struct _Ecore_Buffer_Consumer
{
   struct bq_consumer *resource; /**< Low-level buffer queue consumer resource. */
   Ecore_Buffer_Queue *ebq; /**< The Ecore_Buffer_Queue instance used by this consumer. */
   struct
     {
        void  (*provider_add) (Ecore_Buffer_Consumer *consumer, void *data); /**< Callback for provider connection. */
        void  (*provider_del) (Ecore_Buffer_Consumer *consumer, void *data); /**< Callback for provider disconnection. */
        void  (*enqueue)      (Ecore_Buffer_Consumer *consumer, void *data); /**< Callback for buffer enqueue event. */
        void *data; /**< User data for the callbacks. */
     } cb; /**< Callbacks for consumer events. */
};

static void _ecore_buffer_consumer_cb_provider_connected(void *data, struct bq_consumer *bq_consumer);
static void _ecore_buffer_consumer_cb_provider_disconnected(void *data, struct bq_consumer *bq_consumer);
static void _ecore_buffer_consumer_cb_buffer_attached(void *data, struct bq_consumer *bq_consumer, struct bq_buffer *id, const char *engine, int32_t width, int32_t height, int32_t format, uint32_t flags);
static void _ecore_buffer_consumer_cb_buffer_id_set(void *data, struct bq_consumer *bq_consumer, struct bq_buffer *buffer, int32_t id, int32_t offset0, int32_t stride0, int32_t offset1, int32_t stride1, int32_t offset2, int32_t stride2);
static void _ecore_buffer_consumer_cb_buffer_fd_set(void *data, struct bq_consumer *bq_consumer, struct bq_buffer *buffer, int32_t fd, int32_t offset0, int32_t stride0, int32_t offset1, int32_t stride1, int32_t offset2, int32_t stride2);
static void _ecore_buffer_consumer_cb_buffer_detached(void *data, struct bq_consumer *bq_consumer, struct bq_buffer *id);
static void _ecore_buffer_consumer_cb_add_buffer(void *data, struct bq_consumer *bq_consumer, struct bq_buffer *buffer, uint32_t serial);
static void _ecore_buffer_consumer_cb_buffer_free(Ecore_Buffer *buf, void *data);
static Eina_Bool _ecore_buffer_consumer_buffer_import(Ecore_Buffer_Consumer *consumer, Shared_Buffer *sb, int32_t seed, Ecore_Export_Type export_type);

/**
 * @internal
 * @brief Listener for bq_consumer events.
 *
 * This structure maps low-level bq_consumer events to higher-level
 * Ecore_Buffer_Consumer callbacks.
 */
struct bq_consumer_listener _ecore_buffer_consumer_listener =
{
   _ecore_buffer_consumer_cb_provider_connected,
   _ecore_buffer_consumer_cb_provider_disconnected,
   _ecore_buffer_consumer_cb_buffer_attached,
   _ecore_buffer_consumer_cb_buffer_id_set,
   _ecore_buffer_consumer_cb_buffer_fd_set,
   _ecore_buffer_consumer_cb_buffer_detached,
   _ecore_buffer_consumer_cb_add_buffer
};

/**
 * @brief Creates a new Ecore_Buffer_Consumer.
 *
 * This function initializes a new buffer consumer with a given name,
 * queue size, and buffer dimensions.
 *
 * @param name The name for the consumer. This is used to identify the
 *             consumer in the system.
 * @param queue_size The desired size of the buffer queue. If less than
 *                   the default (2), the default size will be used.
 * @param w The width of the buffers to be consumed.
 * @param h The height of the buffers to be consumed.
 * @return A new Ecore_Buffer_Consumer instance on success, or NULL on failure.
 */
EAPI Ecore_Buffer_Consumer *
ecore_buffer_consumer_new(const char *name, int32_t queue_size, int32_t w, int32_t h)
{
   Ecore_Buffer_Consumer *consumer;
   const int default_queue_size = 2;

   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);

   DBG("Consumer New - name %s, queue size %d, size (%dx%d)",
       name, queue_size, w, h);

   if ((w < 1) || (h < 1))
     return NULL;

   if (queue_size < default_queue_size)
     queue_size = default_queue_size;

   _ecore_buffer_con_init_wait();

   consumer = calloc(1, sizeof(Ecore_Buffer_Consumer));
   if (!consumer)
     return NULL;

   consumer->ebq = _ecore_buffer_queue_new(w, h, queue_size);
   if (!consumer->ebq)
     {
        free(consumer);
        return NULL;
     }

   consumer->resource = _ecore_buffer_con_consumer_create(name, queue_size, w, h);
   if (!consumer->resource)
     {
        _ecore_buffer_queue_free(consumer->ebq);
        free(consumer);
        return NULL;
     }

   bq_consumer_add_listener(consumer->resource,
                            &_ecore_buffer_consumer_listener,
                            consumer);

   return consumer;
}

/**
 * @brief Frees an Ecore_Buffer_Consumer.
 *
 * This function releases all resources associated with the given
 * buffer consumer.
 *
 * @param consumer The Ecore_Buffer_Consumer to free.
 */
EAPI void
ecore_buffer_consumer_free(Ecore_Buffer_Consumer *consumer)
{
   EINA_SAFETY_ON_NULL_RETURN(consumer);

   DBG("Consumer Free");

   if (consumer->ebq)
     _ecore_buffer_queue_free(consumer->ebq);

   if (consumer->resource)
     bq_consumer_destroy(consumer->resource);

   free(consumer);
}

/**
 * @brief Releases a buffer back to the provider.
 *
 * This function signals that the consumer is done with the specified buffer,
 * allowing the provider to reuse it.
 *
 * @param consumer The Ecore_Buffer_Consumer instance.
 * @param buffer The Ecore_Buffer to release.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., buffer not
 *         owned by this consumer, consumer not connected, or buffer not in
 *         a releasable state).
 */
EAPI Eina_Bool
ecore_buffer_consumer_buffer_release(Ecore_Buffer_Consumer *consumer, Ecore_Buffer *buffer)
{
   Shared_Buffer *sb;

   EINA_SAFETY_ON_NULL_RETURN_VAL(consumer, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(buffer, EINA_FALSE);

   DBG("Buffer Release");

   if (!_ecore_buffer_queue_connection_state_get(consumer->ebq))
     {
        WARN("NOT Connected with provider yet");
        return EINA_FALSE;
     }

   if (!(sb = _ecore_buffer_queue_shared_buffer_find(consumer->ebq, buffer)))
     {
        WARN("NOT shared - buffer %p", buffer);
        return EINA_FALSE;
     }

   // already detached buffer, free buffer by deputy.
   if (_shared_buffer_state_get(sb) == SHARED_BUFFER_STATE_DETACH)
     {
        DBG("Free buffer - buffer %p", sb);
        ecore_buffer_free(buffer);
     }
   else
     {
        if (_shared_buffer_state_get(sb) != SHARED_BUFFER_STATE_DEQUEUE)
          {
             WARN("Failed to Release Buffer -"
                  "DO NOT Release buffer which is not Dequeued: buffer %p state %s",
                  sb, _shared_buffer_state_string_get(sb));
             return EINA_FALSE;
          }

        _shared_buffer_state_set(sb, SHARED_BUFFER_STATE_RELEASE);
        bq_consumer_release_buffer(consumer->resource, _shared_buffer_resource_get(sb));
     }

   return EINA_TRUE;
}

/**
 * @brief Dequeues (acquires) a buffer from the provider.
 *
 * This function attempts to get the next available buffer from the queue
 * that has been filled by the provider.
 *
 * @param consumer The Ecore_Buffer_Consumer instance.
 * @return An Ecore_Buffer instance on success, or NULL if no buffer is
 *         available or an error occurred. The returned buffer should be
 *         released using ecore_buffer_consumer_buffer_release() when no
 *         longer needed.
 */
EAPI Ecore_Buffer *
ecore_buffer_consumer_buffer_dequeue(Ecore_Buffer_Consumer *consumer)
{
   Shared_Buffer *sb;

   EINA_SAFETY_ON_NULL_RETURN_VAL(consumer, EINA_FALSE);

   DBG("Buffer Acquire");

   if (!_ecore_buffer_queue_dequeue(consumer->ebq, &sb))
     {
        DBG("No Available Buffer in Queue");
        return NULL;
     }

   // This should not happen.
   if (_shared_buffer_state_get(sb) != SHARED_BUFFER_STATE_ENQUEUE)
     {
        ERR("Unknown error occurred - Not on Enqueued State: buffer %p, state %s",
            sb, _shared_buffer_state_string_get(sb));
        return NULL;
     }

   _shared_buffer_state_set(sb, SHARED_BUFFER_STATE_DEQUEUE);

   return _shared_buffer_buffer_get(sb);
}

/**
 * @brief Checks if the consumer's buffer queue is empty.
 *
 * An empty queue means there are no buffers currently available to be
 * dequeued by the consumer.
 *
 * @param consumer The Ecore_Buffer_Consumer instance.
 * @return EINA_TRUE if the queue is empty, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_buffer_consumer_queue_is_empty(Ecore_Buffer_Consumer *consumer)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(consumer, EINA_FALSE);

   return _ecore_buffer_queue_is_empty(consumer->ebq);
}

/**
 * @brief Sets the callback function for provider connection events.
 *
 * This callback is invoked when a provider connects to this consumer.
 *
 * @param consumer The Ecore_Buffer_Consumer instance.
 * @param func The callback function to set.
 * @param data User data to be passed to the callback function.
 */
EAPI void
ecore_buffer_consumer_provider_add_cb_set(Ecore_Buffer_Consumer *consumer, Ecore_Buffer_Consumer_Provider_Add_Cb func, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(consumer);

   consumer->cb.provider_add = func;
   consumer->cb.data = data;
}

/**
 * @brief Sets the callback function for provider disconnection events.
 *
 * This callback is invoked when a provider disconnects from this consumer.
 *
 * @param consumer The Ecore_Buffer_Consumer instance.
 * @param func The callback function to set.
 * @param data User data to be passed to the callback function.
 */
EAPI void
ecore_buffer_consumer_provider_del_cb_set(Ecore_Buffer_Consumer *consumer, Ecore_Buffer_Consumer_Provider_Del_Cb func, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(consumer);

   consumer->cb.provider_del = func;
   consumer->cb.data = data;
}

/**
 * @brief Sets the callback function for buffer enqueued events.
 *
 * This callback is invoked when a new buffer has been enqueued by the
 * provider and is available for the consumer.
 *
 * @param consumer The Ecore_Buffer_Consumer instance.
 * @param func The callback function to set.
 * @param data User data to be passed to the callback function.
 */
EAPI void
ecore_buffer_consumer_buffer_enqueued_cb_set(Ecore_Buffer_Consumer *consumer, Ecore_Buffer_Consumer_Enqueue_Cb func, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(consumer);

   consumer->cb.enqueue = func;
   consumer->cb.data = data;
}

/**
 * @internal
 * @brief Callback invoked when a provider connects.
 *
 * This function is called by the underlying bq_consumer when a provider
 * establishes a connection. It updates the connection state and triggers
 * the user-registered callback.
 *
 * @param data The Ecore_Buffer_Consumer instance.
 * @param bq_consumer The bq_consumer resource (unused).
 */
static void
_ecore_buffer_consumer_cb_provider_connected(void *data, struct bq_consumer *bq_consumer EINA_UNUSED)
{
   Ecore_Buffer_Consumer *consumer = data;

   EINA_SAFETY_ON_NULL_RETURN(consumer);

   DBG("Provider Connected");

   _ecore_buffer_queue_connection_state_set(consumer->ebq, EINA_TRUE);

   CALLBACK_CALL(consumer, provider_add);
}

/**
 * @internal
 * @brief Callback invoked when a provider disconnects.
 *
 * This function is called by the underlying bq_consumer when a provider
 * terminates the connection. It updates the connection state, triggers
 * the user-registered callback, and cleans up any existing shared buffers.
 *
 * @param data The Ecore_Buffer_Consumer instance.
 * @param bq_consumer The bq_consumer resource (unused).
 */
static void
_ecore_buffer_consumer_cb_provider_disconnected(void *data, struct bq_consumer *bq_consumer EINA_UNUSED)
{
   Ecore_Buffer_Consumer *consumer = data;
   Eina_List *clone, *shared_buffers, *l;
   Shared_Buffer *sb;

   EINA_SAFETY_ON_NULL_RETURN(consumer);

   DBG("Provider Disconnected");

   _ecore_buffer_queue_connection_state_set(consumer->ebq, EINA_FALSE);

   CALLBACK_CALL(consumer, provider_del);

   shared_buffers = _ecore_buffer_queue_shared_buffer_list_get(consumer->ebq);
   clone = eina_list_clone(shared_buffers);

   EINA_LIST_FOREACH(clone, l, sb)
      ecore_buffer_free(_shared_buffer_buffer_get(sb));

   eina_list_free(clone);
}

/**
 * @internal
 * @brief Callback invoked when a buffer is attached by the provider.
 *
 * This function is called when the provider announces a new buffer.
 * It creates a corresponding Shared_Buffer object and adds it to the
 * consumer's queue management.
 *
 * @param data The Ecore_Buffer_Consumer instance.
 * @param bq_consumer The bq_consumer resource (unused).
 * @param id The bq_buffer resource identifier from the provider.
 * @param engine The rendering engine name associated with the buffer.
 * @param width The width of the buffer.
 * @param height The height of the buffer.
 * @param format The pixel format of the buffer.
 * @param flags Flags associated with the buffer.
 */
static void
_ecore_buffer_consumer_cb_buffer_attached(void *data, struct bq_consumer *bq_consumer EINA_UNUSED, struct bq_buffer *id, const char *engine, int32_t width, int32_t height, int32_t format, uint32_t flags)
{
   Ecore_Buffer_Consumer *consumer = data;
   Shared_Buffer *sb;

   EINA_SAFETY_ON_NULL_RETURN(consumer);
   EINA_SAFETY_ON_NULL_RETURN(consumer->ebq);

   DBG("Buffer Attached - engine %s, size (%dx%d), format %d, flags %d",
       engine, width, height, format, flags);

   sb = _shared_buffer_new(engine, id, width, height, format, flags);
   _shared_buffer_state_set(sb, SHARED_BUFFER_STATE_ATTACH);
   _ecore_buffer_queue_shared_buffer_add(consumer->ebq, sb);
   bq_buffer_set_user_data(id, sb);
}

/**
 * @internal
 * @brief Callback invoked when an Ecore_Buffer associated with a Shared_Buffer is freed.
 *
 * This function is registered as a free callback for Ecore_Buffer instances
 * that are imported by the consumer. It ensures that when the Ecore_Buffer
 * is no longer needed and freed, the corresponding Shared_Buffer and
 * bq_buffer resources are also cleaned up.
 *
 * @param buf The Ecore_Buffer that is being freed.
 * @param data The Ecore_Buffer_Consumer instance.
 */
static void
_ecore_buffer_consumer_cb_buffer_free(Ecore_Buffer *buf, void *data)
{
   Ecore_Buffer_Consumer *consumer = data;
   Shared_Buffer *sb;

   EINA_SAFETY_ON_NULL_RETURN(buf);
   EINA_SAFETY_ON_NULL_RETURN(consumer);

   sb = _ecore_buffer_queue_shared_buffer_find(consumer->ebq, buf);
   if (!sb)
     return;

   _ecore_buffer_queue_shared_buffer_remove(consumer->ebq, sb);
   bq_buffer_destroy(_shared_buffer_resource_get(sb));
   _shared_buffer_free(sb);
}

/**
 * @internal
 * @brief Callback invoked when a buffer's ID is set by the provider.
 *
 * This function is called when the provider sends information to import
 * a buffer using an ID (e.g., a TBM surface ID). It triggers the buffer
 * import process.
 *
 * @param data The Ecore_Buffer_Consumer instance.
 * @param bq_consumer The bq_consumer resource (unused).
 * @param buffer The bq_buffer resource for which the ID is set.
 * @param id The ID (e.g., TBM surface ID) for importing the buffer.
 * @param offset0 Unused offset for plane 0.
 * @param stride0 Unused stride for plane 0.
 * @param offset1 Unused offset for plane 1.
 * @param stride1 Unused stride for plane 1.
 * @param offset2 Unused offset for plane 2.
 * @param stride2 Unused stride for plane 2.
 */
static void
_ecore_buffer_consumer_cb_buffer_id_set(void *data, struct bq_consumer *bq_consumer EINA_UNUSED, struct bq_buffer *buffer, int32_t id, int32_t offset0 EINA_UNUSED, int32_t stride0 EINA_UNUSED, int32_t offset1 EINA_UNUSED, int32_t stride1 EINA_UNUSED, int32_t offset2 EINA_UNUSED, int32_t stride2 EINA_UNUSED)
{
   Ecore_Buffer_Consumer *consumer = data;
   Shared_Buffer *sb = bq_buffer_get_user_data(buffer);

   EINA_SAFETY_ON_NULL_RETURN(consumer);
   EINA_SAFETY_ON_NULL_RETURN(sb);

   if (_ecore_buffer_consumer_buffer_import(consumer, sb, id, EXPORT_TYPE_ID))
     bq_buffer_set_user_data(buffer, sb);
   else
     ERR("Failed to import buffer - buffer resource %p", buffer);
}

/**
 * @internal
 * @brief Callback invoked when a buffer's file descriptor (FD) is set by the provider.
 *
 * This function is called when the provider sends information to import
 * a buffer using a file descriptor (e.g., a DMA-BUF FD). It triggers the
 * buffer import process.
 *
 * @param data The Ecore_Buffer_Consumer instance.
 * @param bq_consumer The bq_consumer resource (unused).
 * @param buffer The bq_buffer resource for which the FD is set.
 * @param fd The file descriptor for importing the buffer.
 * @param offset0 Unused offset for plane 0.
 * @param stride0 Unused stride for plane 0.
 * @param offset1 Unused offset for plane 1.
 * @param stride1 Unused stride for plane 1.
 * @param offset2 Unused offset for plane 2.
 * @param stride2 Unused stride for plane 2.
 */
static void
_ecore_buffer_consumer_cb_buffer_fd_set(void *data, struct bq_consumer *bq_consumer EINA_UNUSED, struct bq_buffer *buffer, int32_t fd, int32_t offset0 EINA_UNUSED, int32_t stride0 EINA_UNUSED, int32_t offset1 EINA_UNUSED, int32_t stride1 EINA_UNUSED, int32_t offset2 EINA_UNUSED, int32_t stride2 EINA_UNUSED)
{
   Ecore_Buffer_Consumer *consumer = data;
   Shared_Buffer *sb = bq_buffer_get_user_data(buffer);

   EINA_SAFETY_ON_NULL_RETURN(consumer);
   EINA_SAFETY_ON_NULL_RETURN(sb);

   if (_ecore_buffer_consumer_buffer_import(consumer, sb, fd, EXPORT_TYPE_FD))
     bq_buffer_set_user_data(buffer, sb);
   else
     ERR("Failed to import buffer - buffer resource %p", buffer);
}

/**
 * @internal
 * @brief Callback invoked when a buffer is detached by the provider.
 *
 * This function is called when the provider signals that a buffer is no
 * longer available. If the buffer is not currently dequeued by the consumer,
 * it is freed immediately. Otherwise, it is marked for freeing when the
 * consumer releases it.
 *
 * @param data The Ecore_Buffer_Consumer instance.
 * @param bq_consumer The bq_consumer resource (unused).
 * @param id The bq_buffer resource that was detached.
 */
static void
_ecore_buffer_consumer_cb_buffer_detached(void *data, struct bq_consumer *bq_consumer EINA_UNUSED, struct bq_buffer *id)
{
   Ecore_Buffer_Consumer *consumer = data;
   Shared_Buffer *sb = bq_buffer_get_user_data(id);

   EINA_SAFETY_ON_NULL_RETURN(consumer);
   EINA_SAFETY_ON_NULL_RETURN(sb);

   DBG("Buffer Detached");

   // buffer is owned by consumer. free right now.
   if (_shared_buffer_state_get(sb) != SHARED_BUFFER_STATE_DEQUEUE)
     {
        DBG("Free buffer - buffer %p, state %s",
            sb, _shared_buffer_state_string_get(sb));
        ecore_buffer_free(_shared_buffer_buffer_get(sb));
        return;
     }

   // mark it as a detached buffer, and then free on buffer release time.
   DBG("Just mark this buffer to free it when released - buffer %p, state %s",
       sb, "SHARED_BUFFER_STATE_DEQUEUE");
   _shared_buffer_state_set(sb, SHARED_BUFFER_STATE_DETACH);
}

/**
 * @internal
 * @brief Callback invoked when a buffer is enqueued by the provider.
 *
 * This function is called when the provider has filled a buffer and made it
 * available to the consumer. It moves the buffer to the consumer's internal
 * queue and triggers the user-registered enqueue callback.
 *
 * @param data The Ecore_Buffer_Consumer instance.
 * @param bq_consumer The bq_consumer resource (unused).
 * @param buffer The bq_buffer resource that was enqueued.
 * @param serial A serial number associated with the enqueue operation (unused).
 */
static void
_ecore_buffer_consumer_cb_add_buffer(void *data, struct bq_consumer *bq_consumer EINA_UNUSED, struct bq_buffer *buffer, uint32_t serial EINA_UNUSED)
{
   Ecore_Buffer_Consumer *consumer = data;
   Shared_Buffer *sb = bq_buffer_get_user_data(buffer);
   Shared_Buffer_State state;

   EINA_SAFETY_ON_NULL_RETURN(consumer);

   DBG("Buffer Enqueued");

   if (!sb)
     {
        ERR("Unknown Error occurred - maybe this buffer is not shared yet");
        return;
     }

   state = _shared_buffer_state_get(sb);
   if ((state != SHARED_BUFFER_STATE_IMPORT) &&
       (state != SHARED_BUFFER_STATE_RELEASE))
     {
        ERR("Unknown Error occurred - Could not enqueued this state of buffer: buffer %p, state %s",
            sb, _shared_buffer_state_string_get(sb));
        return;
     }

   _ecore_buffer_queue_enqueue(consumer->ebq, sb);
   _shared_buffer_state_set(sb, SHARED_BUFFER_STATE_ENQUEUE);

   CALLBACK_CALL(consumer, enqueue);
}

/**
 * @internal
 * @brief Imports a shared buffer into an Ecore_Buffer.
 *
 * This function takes a Shared_Buffer (which represents a buffer from the
 * provider) and an import seed (either an ID or an FD) and creates an
 * Ecore_Buffer instance from it.
 *
 * @param consumer The Ecore_Buffer_Consumer instance.
 * @param sb The Shared_Buffer to import.
 * @param seed The import seed (ID or FD).
 * @param export_type The type of the seed (EXPORT_TYPE_ID or EXPORT_TYPE_FD).
 * @return EINA_TRUE on successful import, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_buffer_consumer_buffer_import(Ecore_Buffer_Consumer *consumer, Shared_Buffer *sb, int32_t seed, Ecore_Export_Type export_type)
{
   Ecore_Buffer *buffer;
   const char *engine = NULL;
   int w, h, format;
   unsigned int flags;

   if ((!sb) ||
       (!_shared_buffer_info_get(sb, &engine, &w, &h, &format, &flags)))
     {
        ERR("Failed to Get Shared Buffer");
        return EINA_FALSE;
     }

   if (_shared_buffer_state_get(sb) != SHARED_BUFFER_STATE_ATTACH)
     {
        ERR("Not Attached Buffer - buffer %p state %s",
            sb, _shared_buffer_state_string_get(sb));
        return EINA_FALSE;
     }

   if (!(buffer = _ecore_buffer_import(engine, w, h, format, export_type, seed, flags)))
     {
        ERR("Failed to Import Buffer - size (%dx%d), foramt %d, seed %d, export_type %d",
            w, h, format, seed, export_type);
        return EINA_FALSE;
     }

   if (export_type == EXPORT_TYPE_FD)
     close(seed);

   _shared_buffer_buffer_set(sb, buffer);
   _shared_buffer_state_set(sb, SHARED_BUFFER_STATE_IMPORT);

   ecore_buffer_free_callback_add(buffer, _ecore_buffer_consumer_cb_buffer_free, consumer);

   return EINA_TRUE;
}
