#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/xattr.h>

#include <Eina.h>
#include <Ecore_Getopt.h>
#include <wayland-server.h>

#include "bq_mgr_protocol.h"

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

#define BQ_LOG(f, x...)       printf("[ES|%30.30s|%04d] " f "\n", __func__, __LINE__, ##x)
#define BQ_DEBUG(f, x...)     if (debug) printf("[ES|%30.30s|%04d] " f "\n", __func__, __LINE__, ##x)
#define BQ_OBJECT_NEW(x, f)   bq_object_new(sizeof(x), ((Bq_Object_Free_Func)(f)))
#define BQ_OBJECT(x)          ((Bq_Object *)(x))
#define BQ_OBJECT_RESOURCE(x) (((Bq_Object *)(x))->resource)

typedef void (*Bq_Object_Free_Func) (void *obj);

/**
 * @brief Base structure for reference-counted objects.
 */
typedef struct _Bq_Object Bq_Object;

/**
 * @brief Main manager structure for buffer queues.
 *
 * This structure holds the Wayland display, event sources,
 * and a hash table of active buffer queues.
 */
typedef struct _Bq_Mgr Bq_Mgr;

/**
 * @brief Represents a buffer queue, managing communication between a consumer and a provider.
 */
typedef struct _Bq_Buffer_Queue Bq_Buffer_Queue;

/**
 * @brief Represents the consumer side of a buffer queue.
 */
typedef struct _Bq_Buffer_Consumer Bq_Buffer_Consumer;

/**
 * @brief Represents the provider side of a buffer queue.
 */
typedef struct _Bq_Buffer_Provider Bq_Buffer_Provider;

/**
 * @brief Represents a single buffer within a buffer queue.
 */
typedef struct _Bq_Buffer Bq_Buffer;

/**
 * @brief Enumerates the types of buffer identifiers.
 */
typedef enum _Bq_Buffer_Type Bq_Buffer_Type;

/**
 * @struct _Bq_Object
 * @brief Base structure for reference-counted objects.
 *
 * @var _Bq_Object::ref
 * Reference count for the object.
 * @var _Bq_Object::free_fn
 * Function to call when the object is freed.
 * @var _Bq_Object::deleted
 * Flag indicating if the object has been marked for deletion.
 * @var _Bq_Object::resource
 * Associated Wayland resource, if any.
 */
struct _Bq_Object
{
   int ref;
   Bq_Object_Free_Func free_fn;
   Eina_Bool deleted;
   struct wl_resource *resource;
};

/**
 * @struct _Bq_Mgr
 * @brief Main manager structure for buffer queues.
 *
 * @var _Bq_Mgr::bq_obj
 * Base object for reference counting.
 * @var _Bq_Mgr::wdpy
 * Wayland display connection.
 * @var _Bq_Mgr::signals
 * Array of event sources for signal handling (e.g., SIGTERM, SIGINT).
 * Example: signals[0] for SIGTERM, signals[1] for SIGINT.
 * @var _Bq_Mgr::bq_mgr
 * Global Wayland object for the buffer queue manager.
 * @var _Bq_Mgr::buffer_queues
 * Hash table storing active buffer queues, keyed by name.
 *   - Key: (char *) buffer queue name
 *   - Value: (Bq_Buffer_Queue *) pointer to the buffer queue
 */
struct _Bq_Mgr
{
   Bq_Object bq_obj;

   struct wl_display *wdpy;
   struct wl_event_source *signals[3];

   /*BufferQueue manager*/
   struct wl_global *bq_mgr;
   Eina_Hash *buffer_queues;
};

/**
 * @struct _Bq_Buffer_Queue
 * @brief Represents a buffer queue.
 *
 * @var _Bq_Buffer_Queue::bq_obj
 * Base object for reference counting.
 * @var _Bq_Buffer_Queue::link
 * Pointer to the hash table in Bq_Mgr where this queue is stored.
 * @var _Bq_Buffer_Queue::name
 * Unique name of the buffer queue.
 * @var _Bq_Buffer_Queue::connect
 * Wayland signal for connection events.
 * @var _Bq_Buffer_Queue::consumer
 * Pointer to the consumer associated with this queue.
 * @var _Bq_Buffer_Queue::provider
 * Pointer to the provider associated with this queue.
 * @var _Bq_Buffer_Queue::buffers
 * Inlist of Bq_Buffer structures belonging to this queue.
 *   Each element is a `Bq_Buffer` struct.
 */
struct _Bq_Buffer_Queue
{
   Bq_Object bq_obj;
   Eina_Hash *link;

   char *name;
   struct wl_signal connect;

   Bq_Buffer_Consumer *consumer;
   Bq_Buffer_Provider *provider;
   Eina_Inlist *buffers;
};

/**
 * @struct _Bq_Buffer_Consumer
 * @brief Represents the consumer side of a buffer queue.
 *
 * @var _Bq_Buffer_Consumer::bq_obj
 * Base object for reference counting.
 * @var _Bq_Buffer_Consumer::buffer_queue
 * Pointer to the parent buffer queue.
 * @var _Bq_Buffer_Consumer::queue_size
 * Maximum number of buffers the queue can hold.
 * @var _Bq_Buffer_Consumer::width
 * Default width of buffers in the queue.
 * @var _Bq_Buffer_Consumer::height
 * Default height of buffers in the queue.
 */
struct _Bq_Buffer_Consumer
{
   Bq_Object bq_obj;
   Bq_Buffer_Queue *buffer_queue;

   int32_t queue_size;
   int32_t width;
   int32_t height;
};

/**
 * @struct _Bq_Buffer_Provider
 * @brief Represents the provider side of a buffer queue.
 *
 * @var _Bq_Buffer_Provider::bq_obj
 * Base object for reference counting.
 * @var _Bq_Buffer_Provider::buffer_queue
 * Pointer to the parent buffer queue.
 */
struct _Bq_Buffer_Provider
{
   Bq_Object bq_obj;
   Bq_Buffer_Queue *buffer_queue;
};

/**
 * @enum _Bq_Buffer_Type
 * @brief Enumerates the types of buffer identifiers.
 *
 * @var bq_BUFFER_TYPE_ID
 * Buffer is identified by an integer ID (e.g., a TBM surface ID).
 * @var bq_BUFFER_TYPE_FD
 * Buffer is identified by a file descriptor.
 */
enum _Bq_Buffer_Type
{
   bq_BUFFER_TYPE_ID,
   bq_BUFFER_TYPE_FD,
};

/**
 * @struct _Bq_Buffer
 * @brief Represents a single buffer within a buffer queue.
 *
 * @var _Bq_Buffer::bq_obj
 * Base object for reference counting. Note: wl_resource is not used in bq_obj for Bq_Buffer.
 * @var _Bq_Buffer::EINA_INLIST
 * Macro for Eina_Inlist membership.
 * @var _Bq_Buffer::consumer
 * Wayland resource for the consumer side of this buffer.
 * @var _Bq_Buffer::provider
 * Wayland resource for the provider side of this buffer.
 * @var _Bq_Buffer::serial
 * Serial number for buffer enqueue operations.
 * @var _Bq_Buffer::engine
 * Name of the buffer engine (e.g., "tbm").
 * @var _Bq_Buffer::type
 * Type of the buffer identifier (ID or FD).
 * @var _Bq_Buffer::width
 * Width of the buffer in pixels.
 * @var _Bq_Buffer::height
 * Height of the buffer in pixels.
 * @var _Bq_Buffer::format
 * Pixel format of the buffer.
 * @var _Bq_Buffer::flags
 * Flags associated with the buffer.
 * @var _Bq_Buffer::id
 * Identifier for the buffer (either an ID or a file descriptor).
 * @var _Bq_Buffer::offset0
 * Offset for plane 0.
 * @var _Bq_Buffer::stride0
 * Stride for plane 0.
 * @var _Bq_Buffer::offset1
 * Offset for plane 1.
 * @var _Bq_Buffer::stride1
 * Stride for plane 1.
 * @var _Bq_Buffer::offset2
 * Offset for plane 2.
 * @var _Bq_Buffer::stride2
 * Stride for plane 2.
 */
struct _Bq_Buffer
{
   Bq_Object bq_obj; /*Dont use wl_resource in bq_obj*/
   EINA_INLIST;

   struct wl_resource *consumer;
   struct wl_resource *provider;
   uint32_t serial;

   char *engine;
   Bq_Buffer_Type type;
   int32_t width;
   int32_t height;
   int32_t format;
   uint32_t flags;

   int32_t id;
   int32_t offset0;
   int32_t stride0;
   int32_t offset1;
   int32_t stride1;
   int32_t offset2;
   int32_t stride2;
};

static Eina_Bool debug;

/**
 * @brief Allocates and initializes a new Bq_Object.
 * @param size The total size to allocate for the object (including derived structure).
 * @param fn The function to call when this object is freed.
 * @return A pointer to the newly allocated Bq_Object, or NULL on failure.
 *         The returned object has a reference count of 1.
 */
static void *
bq_object_new(size_t size, Bq_Object_Free_Func fn)
{
   Bq_Object *o = calloc(1, size);

   if ((!o)) return NULL;

   o->ref = 1;
   o->free_fn = fn;

   return o;
}

/**
 * @brief Increments the reference count of a Bq_Object.
 * @param o The Bq_Object to reference.
 * @return The new reference count.
 */
static int
bq_object_ref(Bq_Object *o)
{
   o->ref++;

   return o->ref;
}

/**
 * @brief Decrements the reference count of a Bq_Object.
 * If the reference count drops to zero or the object is marked as deleted,
 * the object's free function is called (if set) and the memory is freed.
 * @param o The Bq_Object to unreference.
 * @return The new reference count, or 0 if the object was freed.
 */
static int
bq_object_unref(Bq_Object *o)
{
   o->ref--;

   if (o->ref <= 0 || o->deleted)
     {
        if (o->free_fn)
          o->free_fn(o);

        free(o);
     }

   return o->ref;
}

/**
 * @brief Marks a Bq_Object for deletion and decrements its reference count.
 * This function ensures that the object is eventually freed by `bq_object_unref`
 * once its reference count reaches zero. It prevents double-freeing by checking
 * the `deleted` flag.
 * @param o The Bq_Object to mark for deletion.
 * @return The new reference count after unreferencing, or 0 if the object was already NULL or deleted.
 */
static int
bq_object_free(Bq_Object *o)
{
   if (!o) return 0;
   if (o->deleted) return 0;

   o->deleted = EINA_TRUE;

   return bq_object_unref(o);
}

/**
 * @brief Frees resources associated with a Bq_Buffer_Queue.
 * This function is typically called when the Bq_Buffer_Queue object's
 * reference count drops to zero. It destroys associated consumer and
 * provider resources, cleans up buffers, and removes the queue from
 * the manager's hash table.
 * @param bq The Bq_Buffer_Queue to free.
 */
static void
bq_mgr_buffer_queue_free(Bq_Buffer_Queue *bq)
{
   Bq_Buffer *buf;

   if (!bq) return;

   BQ_DEBUG("destroy buffer queue : %s\n", bq->name);

   if (bq->consumer)
     {
        wl_resource_destroy(BQ_OBJECT_RESOURCE(bq->consumer));
        bq->consumer = NULL;
     }

   if (bq->provider)
     {
        wl_resource_destroy(BQ_OBJECT_RESOURCE(bq->provider));
        bq->provider = NULL;
     }

   while (bq->buffers)
     {
        buf = EINA_INLIST_CONTAINER_GET(bq->buffers,Bq_Buffer);
        bq->buffers = eina_inlist_remove(bq->buffers, bq->buffers);
        if (buf->consumer)
          {
             wl_resource_destroy(buf->consumer);
             buf->consumer = NULL;
          }

        if (buf->provider)
          {
             wl_resource_destroy(buf->provider);
             buf->provider = NULL;
          }
     }

   if (bq->link)
     {
        eina_hash_del(bq->link, bq->name, bq);
        bq->link = NULL;
     }
   if (bq->name)
     {
        free(bq->name);
        bq->name = NULL;
     }
}

/**
 * @brief Creates a new Bq_Buffer_Queue or retrieves an existing one by name.
 * If a buffer queue with the given name already exists, its reference count
 * is incremented and it is returned. Otherwise, a new queue is created,
 * added to the manager's hash table, and returned with a reference count of 1.
 * @param bq_mgr The buffer queue manager.
 * @param name The name of the buffer queue to create or retrieve.
 * @return A pointer to the Bq_Buffer_Queue, or NULL on failure.
 */
static Bq_Buffer_Queue*
bq_mgr_buffer_queue_new(Bq_Mgr *bq_mgr, const char *name)
{
   Bq_Buffer_Queue *bq;

   bq = eina_hash_find(bq_mgr->buffer_queues, name);
   if (bq)
     {
        bq_object_ref(BQ_OBJECT(bq));
        return bq;
     }

   bq = BQ_OBJECT_NEW(Bq_Buffer_Queue, bq_mgr_buffer_queue_free);
   EINA_SAFETY_ON_NULL_RETURN_VAL(bq, NULL);

   bq->link = bq_mgr->buffer_queues;
   bq->name = strdup(name);
   if (!eina_hash_add(bq->link,bq->name,bq))
     {
        bq_object_free(BQ_OBJECT(bq));
        return NULL;
     }

   wl_signal_init(&bq->connect);
   bq->buffers = NULL;
   return bq;
}

/**
 * @brief Handles the `release_buffer` request from a bq_consumer.
 * This function is called when a consumer releases a buffer. It informs
 * the provider (if connected) that the buffer is available again by
 * sending an `add_buffer` event.
 * @param client The Wayland client that sent the request.
 * @param resource The bq_consumer resource.
 * @param buffer The bq_buffer resource being released.
 */
static void
bq_mgr_buffer_consumer_release_buffer(struct wl_client *client EINA_UNUSED,
                                      struct wl_resource *resource,
                                      struct wl_resource *buffer)
{
   Bq_Buffer_Queue *bq;
   Bq_Buffer_Consumer *bq_consumer;
   Bq_Buffer_Provider *bq_provider;
   Bq_Buffer *bq_buffer;

   bq_consumer = (Bq_Buffer_Consumer*)wl_resource_get_user_data(resource);
   bq_buffer = (Bq_Buffer*)wl_resource_get_user_data(buffer);
   bq = bq_consumer->buffer_queue;
   bq_provider = bq->provider;

   if (bq_provider && bq_buffer->provider)
     {
        bq_provider_send_add_buffer(BQ_OBJECT_RESOURCE(bq_provider),
                                    bq_buffer->provider, bq_buffer->serial);
     }
}

static const struct bq_consumer_interface _bq_consumer_interface = {
     bq_mgr_buffer_consumer_release_buffer
};

/**
 * @brief Destructor for bq_consumer Wayland resources.
 * This function is called when a bq_consumer resource is destroyed.
 * It cleans up associated resources, informs the provider of disconnection,
 * and releases references to the consumer object and its buffer queue.
 * @param resource The bq_consumer resource being destroyed.
 */
static void
bq_mgr_buffer_consumer_destroy(struct wl_resource *resource)
{
   Bq_Buffer_Consumer *bq_consumer = wl_resource_get_user_data(resource);
   Bq_Buffer_Provider *bq_provider;
   Bq_Buffer_Queue *bq;
   Bq_Buffer *buf;

   if (!bq_consumer) return;

   BQ_DEBUG("destroy buffer consumer : %s\n", bq_consumer->buffer_queue->name);

   bq = bq_consumer->buffer_queue;
   bq_provider = bq->provider;

   bq->consumer = NULL;
   BQ_OBJECT_RESOURCE(bq_consumer) = NULL;

   if (bq_provider)
     {
        bq_provider_send_disconnected(BQ_OBJECT_RESOURCE(bq_provider));
     }

   while (bq->buffers)
     {
        buf = EINA_INLIST_CONTAINER_GET(bq->buffers,Bq_Buffer);
        bq->buffers = eina_inlist_remove(bq->buffers,bq->buffers);

        BQ_DEBUG("destroy BUFFER : %d\n", buf->type);
        if (buf->consumer)
          {
             wl_resource_destroy(buf->consumer);
             buf->consumer = NULL;
             bq_object_unref(BQ_OBJECT(buf));
          }

        if (buf->provider)
          {
             wl_resource_destroy(buf->provider);
             buf->provider = NULL;
             bq_object_unref(BQ_OBJECT(buf));
          }
     }

   bq_object_unref(BQ_OBJECT(bq_consumer));
   bq_object_unref(BQ_OBJECT(bq));
}

/**
 * @brief Destructor for bq_buffer Wayland resources.
 * This function is called when a bq_buffer resource (either consumer or provider side)
 * is destroyed. It currently only logs the destruction.
 * @param resource The bq_buffer resource being destroyed.
 */
static void
bq_mgr_buffer_destroy(struct wl_resource *resource)
{
   Bq_Buffer *buf = wl_resource_get_user_data(resource);

   if (resource == buf->consumer)
     {
        BQ_DEBUG("destroy buffer : consumer\n");
     }
   else if (resource == buf->provider)
     {
        BQ_DEBUG("destroy buffer : provider\n");
     }
}

/**
 * @brief Creates the consumer-side Wayland resource for a buffer and notifies the consumer.
 * This function is called when a new buffer is attached by the provider.
 * It creates a `bq_buffer` resource for the consumer, associates it with the
 * `Bq_Buffer` data, and sends a `buffer_attached` event to the consumer.
 * @param bq_consumer The consumer to notify.
 * @param bq_buffer The buffer for which to create the consumer-side resource.
 */
static void
bq_buffer_create_consumer_side(Bq_Buffer_Consumer *bq_consumer, Bq_Buffer *bq_buffer)
{
   if (!bq_consumer) return;

   bq_buffer->consumer = wl_resource_create(wl_resource_get_client(BQ_OBJECT_RESOURCE(bq_consumer)),
                                            &bq_buffer_interface, 1, 0);
   wl_resource_set_implementation(bq_buffer->consumer, NULL, bq_buffer, bq_mgr_buffer_destroy);
   bq_object_ref(BQ_OBJECT(bq_buffer));

   bq_consumer_send_buffer_attached(BQ_OBJECT_RESOURCE(bq_consumer),
                                    bq_buffer->consumer,
                                    bq_buffer->engine,
                                    bq_buffer->width,
                                    bq_buffer->height,
                                    bq_buffer->format,
                                    bq_buffer->flags);
}

/**
 * @brief Sends buffer details (ID or FD) to the consumer.
 * This function is called after a buffer's details (like ID/FD and plane info)
 * are set by the provider. It sends either `set_buffer_id` or `set_buffer_fd`
 * event to the consumer. If the buffer type is FD, the FD is closed after sending.
 * @param bq_consumer The consumer to notify.
 * @param bq_buffer The buffer whose details are being set.
 */
static void
bq_buffer_set_consumer_side(Bq_Buffer_Consumer *bq_consumer, Bq_Buffer *bq_buffer)
{
   if (!bq_consumer) return;
   EINA_SAFETY_ON_NULL_RETURN(bq_buffer);
   EINA_SAFETY_ON_NULL_RETURN(bq_buffer->consumer);

   if (bq_buffer->type == bq_BUFFER_TYPE_ID)
     bq_consumer_send_set_buffer_id(BQ_OBJECT_RESOURCE(bq_consumer),
                                    bq_buffer->consumer,
                                    bq_buffer->id,
                                    bq_buffer->offset0,
                                    bq_buffer->stride0,
                                    bq_buffer->offset1,
                                    bq_buffer->stride1,
                                    bq_buffer->offset2,
                                    bq_buffer->stride2);
   else
     {
        bq_consumer_send_set_buffer_fd(BQ_OBJECT_RESOURCE(bq_consumer),
                                       bq_buffer->consumer,
                                       bq_buffer->id,
                                       bq_buffer->offset0,
                                       bq_buffer->stride0,
                                       bq_buffer->offset1,
                                       bq_buffer->stride1,
                                       bq_buffer->offset2,
                                       bq_buffer->stride2);
        close(bq_buffer->id);
     }
}

/**
 * @brief Handles the `create_consumer` request from a bq_mgr client.
 * This function creates a new consumer for a named buffer queue.
 * If the queue doesn't exist, it's created. If a consumer already exists
 * for the queue, an error is posted. It initializes the consumer,
 * associates it with the queue, and notifies both consumer and provider
 * (if the provider is already connected) about the connection.
 * Any pre-existing buffers in the queue are also communicated to the new consumer.
 * @param client The Wayland client that sent the request.
 * @param resource The bq_mgr resource.
 * @param id The ID for the new bq_consumer resource.
 * @param name The name of the buffer queue.
 * @param queue_size The requested size of the buffer queue.
 * @param width The default width for buffers in this queue.
 * @param height The default height for buffers in this queue.
 */
static void
bq_mgr_buffer_queue_create_consumer(struct wl_client *client,
                                    struct wl_resource *resource,
                                    uint32_t id,
                                    const char *name,
                                    int32_t queue_size,
                                    int32_t width,
                                    int32_t height)
{
   Bq_Mgr *bq_mgr = (Bq_Mgr*)wl_resource_get_user_data(resource);
   Bq_Buffer_Queue *bq;
   Bq_Buffer_Consumer *bq_consumer;
   Bq_Buffer_Provider *bq_provider;
   Bq_Buffer *buf;

   EINA_SAFETY_ON_NULL_RETURN(bq_mgr);

   bq = bq_mgr_buffer_queue_new(bq_mgr,name);
   EINA_SAFETY_ON_NULL_RETURN(bq);

   if (bq->consumer)
     {
        bq_object_unref(BQ_OBJECT(bq));
        wl_resource_post_error(resource,
                               BQ_MGR_ERROR_ALREADY_USED,
                               "%s consumer already used",name);
        return;
     }

   bq_consumer = BQ_OBJECT_NEW(Bq_Buffer_Consumer, NULL);
   EINA_SAFETY_ON_NULL_RETURN(bq_consumer);
   BQ_OBJECT_RESOURCE(bq_consumer) = wl_resource_create(client,
                                                        &bq_consumer_interface,
                                                        1, id);
   if (!BQ_OBJECT_RESOURCE(bq_consumer))
     {
        bq_object_unref(BQ_OBJECT(bq_consumer));
        wl_client_post_no_memory(client);
        return;
     }

   wl_resource_set_implementation(BQ_OBJECT_RESOURCE(bq_consumer),
                                  &_bq_consumer_interface,
                                  bq_consumer,
                                  bq_mgr_buffer_consumer_destroy);

   bq_consumer->buffer_queue = bq;
   bq_consumer->queue_size = queue_size;
   bq_consumer->width = width;
   bq_consumer->height = height;

   bq_provider = bq->provider;
   bq->consumer = bq_consumer;
   if (bq_provider)
     {
        bq_provider_send_connected(BQ_OBJECT_RESOURCE(bq_provider),
                                   queue_size, width, height);
        bq_consumer_send_connected(BQ_OBJECT_RESOURCE(bq_consumer));
     }

   EINA_INLIST_FOREACH(bq->buffers,buf)
     {
        bq_buffer_create_consumer_side(bq_consumer, buf);
        bq_buffer_set_consumer_side(bq_consumer, buf);
     }
}

/**
 * @brief Frees resources associated with a Bq_Buffer.
 * This function is typically called when the Bq_Buffer object's
 * reference count drops to zero. It frees the engine name string.
 * @param buf The Bq_Buffer to free.
 */
static void
bq_buffer_free(Bq_Buffer *buf)
{
   if (buf->engine)
     free(buf->engine);
}

/**
 * @brief Handles the `attach_buffer` request from a bq_provider.
 * This function is called when a provider wants to add a new buffer to the queue.
 * It creates a `Bq_Buffer` object, a provider-side `bq_buffer` resource,
 * and then calls `bq_buffer_create_consumer_side` to create the consumer-side
 * resource and notify the consumer (if connected).
 * @param client The Wayland client that sent the request.
 * @param resource The bq_provider resource.
 * @param buffer The ID for the new bq_buffer resource (provider side).
 * @param engine The name of the buffer engine (e.g., "tbm").
 * @param width The width of the buffer.
 * @param height The height of the buffer.
 * @param format The pixel format of the buffer.
 * @param flags Flags for the buffer.
 */
static void
bq_mgr_buffer_provider_attatch_buffer(struct wl_client *client,
                                      struct wl_resource *resource,
                                      uint32_t buffer,
                                      const char *engine,
                                      int32_t width,
                                      int32_t height,
                                      int32_t format,
                                      uint32_t flags)
{
   Bq_Buffer_Provider *bq_provider = wl_resource_get_user_data(resource);
   Bq_Buffer_Consumer *bq_consumer;
   Bq_Buffer_Queue *bq;
   Bq_Buffer *bq_buffer;

   EINA_SAFETY_ON_NULL_RETURN(bq_provider);
   bq = bq_provider->buffer_queue;
   bq_consumer = bq->consumer;

   bq_buffer = BQ_OBJECT_NEW(Bq_Buffer, bq_buffer_free);
   bq_buffer->provider = wl_resource_create(client, &bq_buffer_interface, 1, buffer);
   wl_resource_set_implementation(bq_buffer->provider, NULL, bq_buffer, bq_mgr_buffer_destroy);

   if (!bq_buffer->provider)
     {
        wl_client_post_no_memory(client);
        bq_object_unref(BQ_OBJECT(bq_buffer));
        return;
     }

   bq_buffer->engine = strdup(engine);
   bq_buffer->width = width;
   bq_buffer->height = height;
   bq_buffer->format = format;
   bq_buffer->flags = flags;

   bq->buffers = eina_inlist_append(bq->buffers,EINA_INLIST_GET(bq_buffer));
   BQ_DEBUG("add BUFFER : %d\n", bq_buffer->type);

   bq_buffer_create_consumer_side(bq_consumer, bq_buffer);

}

/**
 * @brief Handles the `set_buffer_id` request from a bq_provider.
 * This function sets the details for a buffer identified by an ID (e.g., TBM surface ID).
 * It updates the `Bq_Buffer` structure with the provided plane information and
 * then calls `bq_buffer_set_consumer_side` to notify the consumer.
 * @param client The Wayland client that sent the request.
 * @param resource The bq_provider resource.
 * @param buffer The bq_buffer resource (provider side) whose ID is being set.
 * @param id The buffer ID.
 * @param offset0 Offset for plane 0.
 * @param stride0 Stride for plane 0.
 * @param offset1 Offset for plane 1.
 * @param stride1 Stride for plane 1.
 * @param offset2 Offset for plane 2.
 * @param stride2 Stride for plane 2.
 */
static void
bq_mgr_buffer_provider_set_buffer_id(struct wl_client *client EINA_UNUSED,
                                     struct wl_resource *resource,
                                     struct wl_resource *buffer,
                                     int32_t id,
                                     int32_t offset0,
                                     int32_t stride0,
                                     int32_t offset1,
                                     int32_t stride1,
                                     int32_t offset2,
                                     int32_t stride2)
{
   Bq_Buffer_Provider *bq_provider = wl_resource_get_user_data(resource);
   Bq_Buffer_Consumer *bq_consumer;
   Bq_Buffer_Queue *bq;
   Bq_Buffer *bq_buffer;

   EINA_SAFETY_ON_NULL_RETURN(bq_provider);
   bq = bq_provider->buffer_queue;
   bq_consumer = bq->consumer;
   bq_buffer = wl_resource_get_user_data(buffer);
   EINA_SAFETY_ON_NULL_RETURN(bq_buffer);

   bq_buffer->type = bq_BUFFER_TYPE_ID;
   bq_buffer->id = id;
   bq_buffer->offset0 = offset0;
   bq_buffer->stride0 = stride0;
   bq_buffer->offset1 = offset1;
   bq_buffer->stride1 = stride1;
   bq_buffer->offset2 = offset2;
   bq_buffer->stride2 = stride2;

   bq_buffer_set_consumer_side(bq_consumer, bq_buffer);
}

/**
 * @brief Handles the `set_buffer_fd` request from a bq_provider.
 * This function sets the details for a buffer identified by a file descriptor.
 * It updates the `Bq_Buffer` structure with the provided FD and plane information,
 * then calls `bq_buffer_set_consumer_side` to notify the consumer. The FD will
 * be closed by `bq_buffer_set_consumer_side` after it's sent to the consumer.
 * @param client The Wayland client that sent the request.
 * @param resource The bq_provider resource.
 * @param buffer The bq_buffer resource (provider side) whose FD is being set.
 * @param fd The file descriptor for the buffer.
 * @param offset0 Offset for plane 0.
 * @param stride0 Stride for plane 0.
 * @param offset1 Offset for plane 1.
 * @param stride1 Stride for plane 1.
 * @param offset2 Offset for plane 2.
 * @param stride2 Stride for plane 2.
 */
static void
bq_mgr_buffer_provider_set_buffer_fd(struct wl_client *client EINA_UNUSED,
                                     struct wl_resource *resource,
                                     struct wl_resource *buffer,
                                     int32_t fd,
                                     int32_t offset0,
                                     int32_t stride0,
                                     int32_t offset1,
                                     int32_t stride1,
                                     int32_t offset2,
                                     int32_t stride2)
{
   Bq_Buffer_Provider *bq_provider = wl_resource_get_user_data(resource);
   Bq_Buffer_Consumer *bq_consumer;
   Bq_Buffer_Queue *bq;
   Bq_Buffer *bq_buffer;

   EINA_SAFETY_ON_NULL_RETURN(bq_provider);
   bq = bq_provider->buffer_queue;
   bq_consumer = bq->consumer;
   bq_buffer = wl_resource_get_user_data(buffer);
   EINA_SAFETY_ON_NULL_RETURN(bq_buffer);

   bq_buffer->type = bq_BUFFER_TYPE_FD;
   bq_buffer->id = fd;
   bq_buffer->offset0 = offset0;
   bq_buffer->stride0 = stride0;
   bq_buffer->offset1 = offset1;
   bq_buffer->stride1 = stride1;
   bq_buffer->offset2 = offset2;
   bq_buffer->stride2 = stride2;

   bq_buffer_set_consumer_side(bq_consumer, bq_buffer);
}

/**
 * @brief Handles the `detach_buffer` request from a bq_provider.
 * This function is called when a provider wants to remove a buffer from the queue.
 * It notifies the consumer (if connected) by sending `buffer_detached`,
 * destroys the consumer-side buffer resource, and then destroys the
 * provider-side buffer resource. The `Bq_Buffer` object is unreferenced
 * (once for the consumer side, once for the provider side).
 * @param client The Wayland client that sent the request.
 * @param resource The bq_provider resource.
 * @param buffer The bq_buffer resource (provider side) to detach.
 */
static void
bq_mgr_buffer_provider_detach_buffer(struct wl_client *client EINA_UNUSED,
                                     struct wl_resource *resource,
                                     struct wl_resource *buffer)
{
   Bq_Buffer_Provider *bq_provider = wl_resource_get_user_data(resource);
   Bq_Buffer_Consumer *bq_consumer;
   Bq_Buffer_Queue *bq;
   Bq_Buffer *bq_buffer;

   EINA_SAFETY_ON_NULL_RETURN(bq_provider);
   bq = bq_provider->buffer_queue;
   bq_consumer = bq->consumer;
   bq_buffer = wl_resource_get_user_data(buffer);

   if (bq_consumer)
     {
        bq_consumer_send_buffer_detached(BQ_OBJECT_RESOURCE(bq_consumer),
                                         bq_buffer->consumer);
        wl_resource_destroy(bq_buffer->consumer);
        bq_object_unref(BQ_OBJECT(bq_buffer));
     }

   wl_resource_destroy(bq_buffer->provider);
   bq->buffers = eina_inlist_remove(bq->buffers,EINA_INLIST_GET(bq_buffer));
   bq_object_unref(BQ_OBJECT(bq_buffer));
}

/**
 * @brief Handles the `enqueue_buffer` request from a bq_provider.
 * This function is called when a provider has filled a buffer and wants to
 * make it available to the consumer. It updates the buffer's serial number
 * and sends an `add_buffer` event to the consumer (if connected).
 * If the consumer is not connected, an error is posted.
 * @param client The Wayland client that sent the request.
 * @param resource The bq_provider resource.
 * @param buffer The bq_buffer resource (provider side) to enqueue.
 * @param serial A serial number for this enqueue operation, used by the consumer
 *               to associate this buffer with a specific frame or content.
 */
static void
bq_mgr_buffer_provider_enqueue_buffer(struct wl_client *client EINA_UNUSED,
                                      struct wl_resource *resource,
                                      struct wl_resource *buffer,
                                      uint32_t serial)
{
   Bq_Buffer_Provider *bq_provider = wl_resource_get_user_data(resource);
   Bq_Buffer_Consumer *bq_consumer;
   Bq_Buffer_Queue *bq;
   Bq_Buffer *bq_buffer;

   EINA_SAFETY_ON_NULL_RETURN(bq_provider);
   bq = bq_provider->buffer_queue;
   bq_consumer = bq->consumer;
   if (!bq_consumer)
     {
        wl_resource_post_error(BQ_OBJECT_RESOURCE(bq_consumer),
                               BQ_PROVIDER_ERROR_CONNECTION,
                               "Not connected:%s", bq->name);
        return;
     }

   bq_buffer = wl_resource_get_user_data(buffer);
   EINA_SAFETY_ON_NULL_RETURN(bq_buffer);
   bq_buffer->serial = serial;

   bq_consumer_send_add_buffer(BQ_OBJECT_RESOURCE(bq_consumer),
                               bq_buffer->consumer,
                               bq_buffer->serial);
}

static const struct bq_provider_interface _bq_provider_interface = {
     bq_mgr_buffer_provider_attatch_buffer,
     bq_mgr_buffer_provider_set_buffer_id,
     bq_mgr_buffer_provider_set_buffer_fd,
     bq_mgr_buffer_provider_detach_buffer,
     bq_mgr_buffer_provider_enqueue_buffer
};

/**
 * @brief Destructor for bq_provider Wayland resources.
 * This function is called when a bq_provider resource is destroyed.
 * It cleans up all buffers associated with the queue from both provider
 * and consumer perspectives, informs the consumer of disconnection,
 * and releases references to the provider object and its buffer queue.
 * @param resource The bq_provider resource being destroyed.
 */
static void
bq_mgr_buffer_provider_destroy(struct wl_resource *resource)
{
   Bq_Buffer_Queue *bq;
   Bq_Buffer_Provider *bq_provider = wl_resource_get_user_data(resource);
   Bq_Buffer_Consumer *bq_consumer;
   Bq_Buffer *buf;

   EINA_SAFETY_ON_NULL_RETURN(bq_provider);
   BQ_DEBUG("destroy buffer provider : %s\n", bq_provider->buffer_queue->name);
   bq = bq_provider->buffer_queue;
   bq_consumer = bq->consumer;

   BQ_OBJECT_RESOURCE(bq_provider) = NULL;
   bq->provider = NULL;

   while (bq->buffers)
     {
        buf = EINA_INLIST_CONTAINER_GET(bq->buffers, Bq_Buffer);
        bq->buffers = eina_inlist_remove(bq->buffers, bq->buffers);

        if (buf->consumer)
          {
             bq_consumer_send_buffer_detached(BQ_OBJECT_RESOURCE(bq_consumer), buf->consumer);
             wl_resource_destroy(buf->consumer);
             buf->consumer = NULL;
             bq_object_unref(BQ_OBJECT(buf));
          }

        if (buf->provider)
          {
             wl_resource_destroy(buf->provider);
             buf->provider = NULL;
             bq_object_unref(BQ_OBJECT(buf));
          }
     }

   if (bq_consumer)
     {
        if (BQ_OBJECT_RESOURCE(bq_consumer))
          bq_consumer_send_disconnected(BQ_OBJECT_RESOURCE(bq_consumer));
     }

   bq_object_unref(BQ_OBJECT(bq_provider));
   bq_object_unref(BQ_OBJECT(bq));
}

/**
 * @brief Handles the `create_provider` request from a bq_mgr client.
 * This function creates a new provider for a named buffer queue.
 * If the queue doesn't exist, it's created. If a provider already exists
 * for the queue, an error is posted. It initializes the provider,
 * associates it with the queue, and notifies both provider and consumer
 * (if the consumer is already connected) about the connection, including
 * queue parameters (size, width, height).
 * @param client The Wayland client that sent the request.
 * @param resource The bq_mgr resource.
 * @param id The ID for the new bq_provider resource.
 * @param name The name of the buffer queue.
 */
static void
bq_mgr_buffer_queue_create_provider(struct wl_client *client,
                                    struct wl_resource *resource,
                                    uint32_t id,
                                    const char *name)
{
   Bq_Mgr *bq_mgr = (Bq_Mgr*)wl_resource_get_user_data(resource);
   Bq_Buffer_Queue *bq;
   Bq_Buffer_Provider *bq_provider;
   Bq_Buffer_Consumer *bq_consumer;

   EINA_SAFETY_ON_NULL_RETURN(bq_mgr);

   bq = bq_mgr_buffer_queue_new(bq_mgr,name);
   EINA_SAFETY_ON_NULL_RETURN(bq);

   if (bq->provider)
     {
        bq_object_unref(BQ_OBJECT(bq));
        wl_resource_post_error(resource,
                               BQ_MGR_ERROR_ALREADY_USED,
                               "%s rpovider already used",name);
        return;
     }

   bq_provider = BQ_OBJECT_NEW(Bq_Buffer_Provider, NULL);
   EINA_SAFETY_ON_NULL_GOTO(bq_provider, on_error);

   BQ_OBJECT_RESOURCE(bq_provider)= wl_resource_create(client,
                                                       &bq_provider_interface,
                                                       1, id);
   EINA_SAFETY_ON_NULL_GOTO(BQ_OBJECT_RESOURCE(bq_provider), on_error);

   wl_resource_set_implementation(BQ_OBJECT_RESOURCE(bq_provider),
                                  &_bq_provider_interface,
                                  bq_provider,
                                  bq_mgr_buffer_provider_destroy);

   bq_provider->buffer_queue = bq;
   bq->provider = bq_provider;
   bq_consumer = bq->consumer;
   if (bq_consumer)
     {
        /*Send connect*/
        bq_consumer_send_connected(BQ_OBJECT_RESOURCE(bq_consumer));
        bq_provider_send_connected(BQ_OBJECT_RESOURCE(bq_provider),
                                   bq_consumer->queue_size,
                                   bq_consumer->width, bq_consumer->height);
     }

   return;

on_error:
   if (bq) bq_object_unref(BQ_OBJECT(bq));
   if (bq_provider) bq_object_unref(BQ_OBJECT(bq_provider));
   wl_client_post_no_memory(client);
}

static const struct bq_mgr_interface _bq_mgr_interface =
{
   bq_mgr_buffer_queue_create_consumer,
   bq_mgr_buffer_queue_create_provider
};

/**
 * @brief Bind function for the bq_mgr global Wayland object.
 * This function is called when a Wayland client binds to the bq_mgr global.
 * It creates a new bq_mgr resource for the client and sets its implementation.
 * @param client The Wayland client binding to the global.
 * @param data The Bq_Mgr instance (user data for the global).
 * @param version The version requested by the client.
 * @param id The ID for the new bq_mgr resource.
 */
static void
bq_mgr_buffer_queue_bind(struct wl_client *client, void *data,
                         uint32_t version EINA_UNUSED, uint32_t id)
{
   Bq_Mgr *bq_mgr = (Bq_Mgr*)data;
   struct wl_resource *resource;

   resource = wl_resource_create(client, &bq_mgr_interface,
                                 1, id);
   if (resource == NULL) {
        wl_client_post_no_memory(client);
        return;
   }

   wl_resource_set_implementation(resource,
                                  &_bq_mgr_interface,
                                  bq_mgr, NULL);
}

/**
 * @brief Initializes the buffer queue manager.
 * This function creates the bq_mgr Wayland global, which allows clients
 * to connect and create buffer queues. It also initializes the hash table
 * for storing buffer queues.
 * @param bq_mgr The Bq_Mgr instance to initialize.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
bq_mgr_buffer_queue_manager_init(Bq_Mgr *bq_mgr)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(bq_mgr,EINA_FALSE);
   bq_mgr->bq_mgr = wl_global_create(bq_mgr->wdpy
                                     ,&bq_mgr_interface,1
                                     ,bq_mgr
                                     ,bq_mgr_buffer_queue_bind);

   EINA_SAFETY_ON_NULL_RETURN_VAL(bq_mgr->bq_mgr,EINA_FALSE);

   bq_mgr->buffer_queues = eina_hash_string_superfast_new(NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(bq_mgr->buffer_queues,EINA_FALSE);

   return EINA_TRUE;
}

static const Ecore_Getopt optdesc =
{
   "tbm_daemon",
   "%prog [options]",
   "0.1.0",
   "(C) Samsung",
   "BSD 2-Clause",
   "Tizen Buffer Manager Daemon",
   EINA_FALSE,
   {
      ECORE_GETOPT_STORE_STR('s', "socket_name",
                             "socket name"),
      ECORE_GETOPT_STORE_BOOL('d',"debug","enable debug log"),
      ECORE_GETOPT_VERSION('v', "version"),
      ECORE_GETOPT_HELP('h', "help"),
      ECORE_GETOPT_SENTINEL
   }
};

/**
 * @brief Signal handler for termination signals (SIGTERM, SIGINT, SIGQUIT).
 * This function is called when the manager receives a termination signal.
 * It initiates a clean shutdown of the Wayland display.
 * @param signal_number The signal number received.
 * @param data The Bq_Mgr instance (user data for the signal handler).
 * @return Always returns 1 to indicate the signal was handled.
 */
static int
bq_mgr_on_term_signal(int signal_number, void *data)
{
   Bq_Mgr *bq_mgr = data;

   BQ_LOG("caught signal %d\n", signal_number);
   wl_display_terminate(bq_mgr->wdpy);

   return 1;
}

/**
 * @brief Frees resources associated with the Bq_Mgr.
 * This function is typically called when the Bq_Mgr object's reference count
 * drops to zero (e.g., during shutdown). It removes signal handlers
 * and destroys the Wayland display.
 * @param bq_mgr The Bq_Mgr instance to free.
 */
static void
bq_mgr_free(Bq_Mgr *bq_mgr)
{
   int i;

   for (i = ARRAY_LENGTH(bq_mgr->signals) - 1; i >= 0; i--)
     {
        if (bq_mgr->signals[i])
          wl_event_source_remove(bq_mgr->signals[i]);
     }

   if (bq_mgr->wdpy)
     wl_display_destroy(bq_mgr->wdpy);
}

/**
 * @brief Creates and initializes a new Bq_Mgr instance.
 * This function sets up the Wayland display, event loop, signal handlers
 * for termination, and adds the Wayland socket for clients to connect to.
 * @param sock_name The name of the Wayland socket to create. If NULL,
 *                  a default name "bq_mgr_daemon" is used.
 * @return A pointer to the newly created Bq_Mgr, or NULL on failure.
 */
static Bq_Mgr*
bq_mgr_new(char *sock_name)
{
   static char *default_sock_name = "bq_mgr_daemon";
   Bq_Mgr *bq_mgr = BQ_OBJECT_NEW(Bq_Mgr, bq_mgr_free);
   struct wl_event_loop *loop;

   if (!bq_mgr) return NULL;

   bq_mgr->wdpy = wl_display_create();
   loop = wl_display_get_event_loop(bq_mgr->wdpy);
   EINA_SAFETY_ON_NULL_GOTO(loop, on_err);

   bq_mgr->signals[0] = wl_event_loop_add_signal(loop, SIGTERM, bq_mgr_on_term_signal, bq_mgr);
   bq_mgr->signals[1] = wl_event_loop_add_signal(loop, SIGINT, bq_mgr_on_term_signal, bq_mgr);
   bq_mgr->signals[2] = wl_event_loop_add_signal(loop, SIGQUIT, bq_mgr_on_term_signal, bq_mgr);

   if (!sock_name)
     sock_name = default_sock_name;
   wl_display_add_socket(bq_mgr->wdpy, sock_name);

   return bq_mgr;

on_err:
   bq_object_free(BQ_OBJECT(bq_mgr));
   return NULL;
}

/**
 * @brief Main entry point for the buffer queue manager daemon.
 * Parses command-line options, initializes Eina and Ecore_Getopt,
 * creates the Bq_Mgr instance, initializes the buffer queue manager service,
 * and runs the Wayland display event loop.
 * Cleans up resources on exit.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return EXIT_SUCCESS on successful execution, EXIT_FAILURE otherwise.
 */
int
main(int argc, char **argv)
{
   Bq_Mgr *bq_mgr = NULL;
   int res, ret = EXIT_FAILURE;
   char *opt_path = NULL;
   Eina_Bool quit = EINA_FALSE;
   Ecore_Getopt_Value values[] =
     {
        ECORE_GETOPT_VALUE_STR(opt_path),
        ECORE_GETOPT_VALUE_BOOL(debug),
        ECORE_GETOPT_VALUE_BOOL(quit),
        ECORE_GETOPT_VALUE_BOOL(quit),
        ECORE_GETOPT_VALUE_NONE
     };

   eina_init();

   res = ecore_getopt_parse(&optdesc,
                            values,
                            argc, argv);

   if ((res < 0) || (quit)) goto finish;

   if (opt_path)
     BQ_LOG("socket_name : %s\n", opt_path);

   bq_mgr = bq_mgr_new(opt_path);
   if (!bq_mgr) goto finish;

   if (!bq_mgr_buffer_queue_manager_init(bq_mgr))
     {
        bq_mgr_free(bq_mgr);
        goto finish;
     }

   wl_display_run(bq_mgr->wdpy);

   ret = EXIT_SUCCESS;
finish:
   eina_shutdown();
   bq_object_free(BQ_OBJECT(bq_mgr));

   return ret;
}
