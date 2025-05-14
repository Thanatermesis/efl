/**
 * @file
 * @brief Wayland protocol for Buffer Queue Manager.
 *
 * This protocol defines interfaces for managing shared buffers between
 * a provider and a consumer. It allows for creation of buffer queues,
 * attaching and detaching buffers, and signaling buffer availability.
 */
#ifndef BQ_MGR_SERVER_PROTOCOL_H
#define BQ_MGR_SERVER_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-server.h"

struct wl_client;
struct wl_resource;

struct bq_mgr;
struct bq_consumer;
struct bq_provider;
struct bq_buffer;

extern const struct wl_interface bq_mgr_interface;
extern const struct wl_interface bq_consumer_interface;
extern const struct wl_interface bq_provider_interface;
extern const struct wl_interface bq_buffer_interface;

#ifndef BQ_MGR_ERROR_ENUM
#define BQ_MGR_ERROR_ENUM
/**
 * @ingroup iface_bq_mgr
 * bq_mgr error values
 *
 * These errors can be emitted in response to bq_mgr requests.
 */
enum bq_mgr_error {
	/** client has insufficient permissions */
	BQ_MGR_ERROR_INVALID_PERMISSION = 0,
	/** the provided name is not valid or already in use by another consumer/provider */
	BQ_MGR_ERROR_INVALID_NAME = 1,
	/** the resource has already been used in a way that prevents the current request */
	BQ_MGR_ERROR_ALREADY_USED = 2,
};
#endif /* BQ_MGR_ERROR_ENUM */

/**
 * @ingroup iface_bq_mgr
 * @struct bq_mgr_interface
 * @brief Interface for managing buffer queues.
 *
 * The bq_mgr global object is a singleton used to create new
 * buffer queue consumers and providers.
 */
struct bq_mgr_interface {
	/**
	 * @brief create a new buffer consumer
	 *
	 * Creates a new bq_consumer object. The consumer is identified by
	 * a unique name.
	 * @param id new_id for the bq_consumer resource
	 * @param name unique identifier for the consumer queue (e.g., "com.example.MediaPlayer.VideoSurface")
	 * @param queue_size requested size of the buffer queue (e.g., 3 for triple buffering)
	 * @param width default width for buffers in this queue (e.g., 1920)
	 * @param height default height for buffers in this queue (e.g., 1080)
	 */
	void (*create_consumer)(struct wl_client *client,
				struct wl_resource *resource,
				uint32_t id,
				const char *name,
				int32_t queue_size,
				int32_t width,
				int32_t height);
	/**
	 * @brief create a new buffer provider
	 *
	 * Creates a new bq_provider object. The provider connects to an
	 * existing consumer identified by its name.
	 * @param id new_id for the bq_provider resource
	 * @param name name of the consumer queue to connect to
	 */
	void (*create_provider)(struct wl_client *client,
				struct wl_resource *resource,
				uint32_t id,
				const char *name);
};

/**
 * @ingroup iface_bq_consumer
 * @struct bq_consumer_interface
 * @brief Interface for a buffer consumer.
 *
 * A bq_consumer object represents the consumer side of a buffer queue.
 * It receives buffers from a bq_provider and can release them back
 * once they are no longer needed.
 */
struct bq_consumer_interface {
	/**
	 * @brief release a buffer
	 *
	 * Notifies the provider that the consumer has finished using the
	 * specified buffer and it can be reused or freed.
	 * @param buffer the bq_buffer resource to release
	 */
	void (*release_buffer)(struct wl_client *client,
			       struct wl_resource *resource,
			       struct wl_resource *buffer);
};

/** @ingroup iface_bq_consumer */
#define BQ_CONSUMER_CONNECTED	0
/** @ingroup iface_bq_consumer */
#define BQ_CONSUMER_DISCONNECTED	1
/** @ingroup iface_bq_consumer */
#define BQ_CONSUMER_BUFFER_ATTACHED	2
/** @ingroup iface_bq_consumer */
#define BQ_CONSUMER_SET_BUFFER_ID	3
/** @ingroup iface_bq_consumer */
#define BQ_CONSUMER_SET_BUFFER_FD	4
/** @ingroup iface_bq_consumer */
#define BQ_CONSUMER_BUFFER_DETACHED	5
/** @ingroup iface_bq_consumer */
#define BQ_CONSUMER_ADD_BUFFER	6

#define BQ_CONSUMER_CONNECTED_SINCE_VERSION	1
#define BQ_CONSUMER_DISCONNECTED_SINCE_VERSION	1
#define BQ_CONSUMER_BUFFER_ATTACHED_SINCE_VERSION	1
#define BQ_CONSUMER_SET_BUFFER_ID_SINCE_VERSION	1
#define BQ_CONSUMER_SET_BUFFER_FD_SINCE_VERSION	1
#define BQ_CONSUMER_BUFFER_DETACHED_SINCE_VERSION	1
#define BQ_CONSUMER_ADD_BUFFER_SINCE_VERSION	1

/**
 * @ingroup iface_bq_consumer
 * @brief Notifies the consumer that a provider has connected.
 *
 * This event is sent when a bq_provider successfully connects to this
 * consumer's queue.
 * @param resource_ The bq_consumer resource.
 */
static inline void
bq_consumer_send_connected(struct wl_resource *resource_)
{
	wl_resource_post_event(resource_, BQ_CONSUMER_CONNECTED);
}

/**
 * @ingroup iface_bq_consumer
 * @brief Notifies the consumer that the provider has disconnected.
 *
 * This event is sent when the bq_provider disconnects from this
 * consumer's queue.
 * @param resource_ The bq_consumer resource.
 */
static inline void
bq_consumer_send_disconnected(struct wl_resource *resource_)
{
	wl_resource_post_event(resource_, BQ_CONSUMER_DISCONNECTED);
}

/**
 * @ingroup iface_bq_consumer
 * @brief Notifies the consumer that a buffer has been attached by the provider.
 *
 * This event informs the consumer about a new buffer that the provider
 * has made available. The consumer should not use the buffer until it
 * receives an `add_buffer` event for this buffer.
 * @param resource_ The bq_consumer resource.
 * @param buffer The new bq_buffer resource.
 * @param engine Name of the graphics engine or type of buffer (e.g., "DRM", "SHM", "EGLStream", "viv_fb").
 * @param width Width of the buffer in pixels.
 * @param height Height of the buffer in pixels.
 * @param format Pixel format of the buffer (e.g., a `DRM_FORMAT_*` value, `WL_SHM_FORMAT_*` value, or a custom identifier).
 * @param flags Additional flags for the buffer (e.g., `0` if no special flags).
 */
static inline void
bq_consumer_send_buffer_attached(struct wl_resource *resource_, struct wl_resource *buffer, const char *engine, int32_t width, int32_t height, int32_t format, uint32_t flags)
{
	wl_resource_post_event(resource_, BQ_CONSUMER_BUFFER_ATTACHED, buffer, engine, width, height, format, flags);
}

/**
 * @ingroup iface_bq_consumer
 * @brief Sets buffer metadata (ID and plane info) for an attached buffer.
 *
 * This event provides detailed plane information for a previously attached
 * buffer, identifying it by a specific ID.
 * @param resource_ The bq_consumer resource.
 * @param buffer The bq_buffer resource.
 * @param id The identifier for this buffer (e.g., a dma-buf handle or EGLImage ID).
 * @param offset0 Offset for plane 0.
 * @param stride0 Stride for plane 0.
 * @param offset1 Offset for plane 1.
 * @param stride1 Stride for plane 1.
 * @param offset2 Offset for plane 2.
 * @param stride2 Stride for plane 2.
 */
static inline void
bq_consumer_send_set_buffer_id(struct wl_resource *resource_, struct wl_resource *buffer, int32_t id, int32_t offset0, int32_t stride0, int32_t offset1, int32_t stride1, int32_t offset2, int32_t stride2)
{
	wl_resource_post_event(resource_, BQ_CONSUMER_SET_BUFFER_ID, buffer, id, offset0, stride0, offset1, stride1, offset2, stride2);
}

/**
 * @ingroup iface_bq_consumer
 * @brief Sets buffer metadata (FD and plane info) for an attached buffer.
 *
 * This event provides detailed plane information for a previously attached
 * buffer, identifying it by a file descriptor.
 * @param resource_ The bq_consumer resource.
 * @param buffer The bq_buffer resource.
 * @param fd The file descriptor for this buffer (e.g., a dma-buf fd).
 * @param offset0 Offset for plane 0.
 * @param stride0 Stride for plane 0.
 * @param offset1 Offset for plane 1.
 * @param stride1 Stride for plane 1.
 * @param offset2 Offset for plane 2.
 * @param stride2 Stride for plane 2.
 */
static inline void
bq_consumer_send_set_buffer_fd(struct wl_resource *resource_, struct wl_resource *buffer, int32_t fd, int32_t offset0, int32_t stride0, int32_t offset1, int32_t stride1, int32_t offset2, int32_t stride2)
{
	wl_resource_post_event(resource_, BQ_CONSUMER_SET_BUFFER_FD, buffer, fd, offset0, stride0, offset1, stride1, offset2, stride2);
}

/**
 * @ingroup iface_bq_consumer
 * @brief Notifies the consumer that a buffer has been detached by the provider.
 *
 * This event informs the consumer that a previously attached buffer is no
 * longer available from the provider. The consumer should release any
 * references to this buffer.
 * @param resource_ The bq_consumer resource.
 * @param buffer The bq_buffer resource that was detached.
 */
static inline void
bq_consumer_send_buffer_detached(struct wl_resource *resource_, struct wl_resource *buffer)
{
	wl_resource_post_event(resource_, BQ_CONSUMER_BUFFER_DETACHED, buffer);
}

/**
 * @ingroup iface_bq_consumer
 * @brief Notifies the consumer that a buffer is ready for consumption.
 *
 * This event is sent by the provider when a buffer has been filled with
 * content and is ready for the consumer to use. The serial is used for
 * synchronization.
 * @param resource_ The bq_consumer resource.
 * @param buffer The bq_buffer resource that is ready.
 * @param serial A serial number for this buffer event.
 */
static inline void
bq_consumer_send_add_buffer(struct wl_resource *resource_, struct wl_resource *buffer, uint32_t serial)
{
	wl_resource_post_event(resource_, BQ_CONSUMER_ADD_BUFFER, buffer, serial);
}

#ifndef BQ_PROVIDER_ERROR_ENUM
#define BQ_PROVIDER_ERROR_ENUM
/**
 * @ingroup iface_bq_provider
 * bq_provider error values
 *
 * These errors can be emitted in response to bq_provider requests.
 */
enum bq_provider_error {
	/** the queue is full and cannot accept more buffers */
	BQ_PROVIDER_ERROR_OVERFLOW_QUEUE_SIZE = 0,
	/** error connecting to the consumer */
	BQ_PROVIDER_ERROR_CONNECTION = 1,
};
#endif /* BQ_PROVIDER_ERROR_ENUM */

/**
 * @ingroup iface_bq_provider
 * @struct bq_provider_interface
 * @brief Interface for a buffer provider.
 *
 * A bq_provider object represents the provider side of a buffer queue.
 * It creates and manages buffers, attaching them to the queue and
 * notifying the consumer when they are ready.
 */
struct bq_provider_interface {
	/**
	 * @brief attach a buffer to the queue
	 *
	 * The provider uses this request to introduce a new buffer to the
	 * queue. The `buffer` argument is a new_id for a bq_buffer
	 * resource.
	 * @param buffer new_id for the bq_buffer resource
	 * @param engine Name of the graphics engine or type of buffer (e.g., "DRM", "SHM", "EGLStream", "viv_fb").
	 * @param width Width of the buffer in pixels.
	 * @param height Height of the buffer in pixels.
	 * @param format Pixel format of the buffer (e.g., a `DRM_FORMAT_*` value, `WL_SHM_FORMAT_*` value, or a custom identifier).
	 * @param flags Additional flags for the buffer (e.g., `0` if no special flags).
	 */
	void (*attach_buffer)(struct wl_client *client,
			      struct wl_resource *resource,
			      uint32_t buffer,
			      const char *engine,
			      int32_t width,
			      int32_t height,
			      int32_t format,
			      uint32_t flags);
	/**
	 * @brief set buffer metadata (ID and plane info)
	 *
	 * Sets detailed plane information for a previously attached buffer,
	 * identifying it by a specific ID.
	 * @param buffer The bq_buffer resource.
	 * @param id The identifier for this buffer (e.g., a dma-buf handle or EGLImage ID).
	 * @param offset0 Offset for plane 0.
	 * @param stride0 Stride for plane 0.
	 * @param offset1 Offset for plane 1.
	 * @param stride1 Stride for plane 1.
	 * @param offset2 Offset for plane 2.
	 * @param stride2 Stride for plane 2.
	 */
	void (*set_buffer_id)(struct wl_client *client,
			      struct wl_resource *resource,
			      struct wl_resource *buffer,
			      int32_t id,
			      int32_t offset0,
			      int32_t stride0,
			      int32_t offset1,
			      int32_t stride1,
			      int32_t offset2,
			      int32_t stride2);
	/**
	 * @brief set buffer metadata (FD and plane info)
	 *
	 * Sets detailed plane information for a previously attached buffer,
	 * identifying it by a file descriptor.
	 * @param buffer The bq_buffer resource.
	 * @param fd The file descriptor for this buffer (e.g., a dma-buf fd).
	 * @param offset0 Offset for plane 0.
	 * @param stride0 Stride for plane 0.
	 * @param offset1 Offset for plane 1.
	 * @param stride1 Stride for plane 1.
	 * @param offset2 Offset for plane 2.
	 * @param stride2 Stride for plane 2.
	 */
	void (*set_buffer_fd)(struct wl_client *client,
			      struct wl_resource *resource,
			      struct wl_resource *buffer,
			      int32_t fd,
			      int32_t offset0,
			      int32_t stride0,
			      int32_t offset1,
			      int32_t stride1,
			      int32_t offset2,
			      int32_t stride2);
	/**
	 * @brief detach a buffer from the queue
	 *
	 * The provider uses this request to remove a buffer from the
	 * queue. The consumer will be notified via the `buffer_detached`
	 * event.
	 * @param buffer The bq_buffer resource to detach.
	 */
	void (*detach_buffer)(struct wl_client *client,
			      struct wl_resource *resource,
			      struct wl_resource *buffer);
	/**
	 * @brief enqueue a buffer for the consumer
	 *
	 * Notifies the consumer that the specified buffer is filled with
	 * content and ready for processing. The serial is used for
	 * synchronization.
	 * @param buffer The bq_buffer resource to enqueue.
	 * @param serial A serial number for this buffer event.
	 */
	void (*enqueue_buffer)(struct wl_client *client,
			       struct wl_resource *resource,
			       struct wl_resource *buffer,
			       uint32_t serial);
};

/** @ingroup iface_bq_provider */
#define BQ_PROVIDER_CONNECTED	0
/** @ingroup iface_bq_provider */
#define BQ_PROVIDER_DISCONNECTED	1
/** @ingroup iface_bq_provider */
#define BQ_PROVIDER_ADD_BUFFER	2

#define BQ_PROVIDER_CONNECTED_SINCE_VERSION	1
#define BQ_PROVIDER_DISCONNECTED_SINCE_VERSION	1
#define BQ_PROVIDER_ADD_BUFFER_SINCE_VERSION	1

/**
 * @ingroup iface_bq_provider
 * @brief Notifies the provider that it has successfully connected to a consumer.
 *
 * This event is sent when the bq_provider successfully establishes a
 * connection with the bq_consumer. It includes details about the
 * consumer's queue.
 * @param resource_ The bq_provider resource.
 * @param queue_size The actual size of the buffer queue on the consumer side.
 * @param width The default width for buffers expected by the consumer.
 * @param height The default height for buffers expected by the consumer.
 */
static inline void
bq_provider_send_connected(struct wl_resource *resource_, int32_t queue_size, int32_t width, int32_t height)
{
	wl_resource_post_event(resource_, BQ_PROVIDER_CONNECTED, queue_size, width, height);
}

/**
 * @ingroup iface_bq_provider
 * @brief Notifies the provider that the consumer has disconnected.
 *
 * This event is sent if the bq_consumer disconnects or is destroyed.
 * @param resource_ The bq_provider resource.
 */
static inline void
bq_provider_send_disconnected(struct wl_resource *resource_)
{
	wl_resource_post_event(resource_, BQ_PROVIDER_DISCONNECTED);
}

/**
 * @ingroup iface_bq_provider
 * @brief Notifies the provider that a buffer has been released by the consumer.
 *
 * This event is sent when the consumer has finished with a buffer and
 * released it via the `release_buffer` request. The provider can now
 * reuse or free this buffer. The serial matches the one from the
 * `enqueue_buffer` request.
 * @param resource_ The bq_provider resource.
 * @param buffer The bq_buffer resource that was released.
 * @param serial The serial number associated with this buffer when it was enqueued.
 */
static inline void
bq_provider_send_add_buffer(struct wl_resource *resource_, struct wl_resource *buffer, uint32_t serial)
{
	wl_resource_post_event(resource_, BQ_PROVIDER_ADD_BUFFER, buffer, serial);
}

/**
 * @ingroup iface_bq_buffer
 * @brief Represents a shared buffer.
 *
 * A bq_buffer object is a handle to a shared memory region or other
 * buffer resource. It doesn't have any requests or events itself but
 * is used by bq_consumer and bq_provider to refer to specific buffers.
 * Its lifetime is managed by the provider; it is created via
 * bq_provider.attach_buffer and destroyed implicitly when the provider
 * detaches it or when the provider itself is destroyed.
 */

#ifdef  __cplusplus
}
#endif

#endif
