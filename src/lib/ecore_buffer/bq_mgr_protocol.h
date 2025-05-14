/** @file
 *  @brief Wayland protocol for buffer queue management.
 *
 *  This protocol defines interfaces for creating and managing buffer queues
 *  between a provider (e.g., a camera or video decoder) and a consumer
 *  (e.g., a display compositor or video encoder).
 */
#ifndef BQ_MGR_CLIENT_PROTOCOL_H
#define BQ_MGR_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

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
/** @ingroup bq_mgr
 *  @brief Errors that can be emitted by the bq_mgr global.
 */
enum bq_mgr_error {
	/** The client does not have permission to perform the action. */
	BQ_MGR_ERROR_INVALID_PERMISSION = 0,
	/** The provided name for a consumer or provider is invalid or already in use. */
	BQ_MGR_ERROR_INVALID_NAME = 1,
	/** The resource (e.g., name) is already used by another client or entity. */
	BQ_MGR_ERROR_ALREADY_USED = 2,
};
#endif /* BQ_MGR_ERROR_ENUM */

/** @ingroup bq_mgr
 *  @brief Request to create a new buffer queue consumer.
 */
#define BQ_MGR_CREATE_CONSUMER	0
/** @ingroup bq_mgr
 *  @brief Request to create a new buffer queue provider.
 */
#define BQ_MGR_CREATE_PROVIDER	1

/**
 * @ingroup bq_mgr
 * @brief Set user data for a bq_mgr proxy.
 *
 * @param bq_mgr The bq_mgr proxy.
 * @param user_data The user data to associate.
 */
static inline void
bq_mgr_set_user_data(struct bq_mgr *bq_mgr, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) bq_mgr, user_data);
}

/**
 * @ingroup bq_mgr
 * @brief Get user data associated with a bq_mgr proxy.
 *
 * @param bq_mgr The bq_mgr proxy.
 * @return The user data.
 */
static inline void *
bq_mgr_get_user_data(struct bq_mgr *bq_mgr)
{
	return wl_proxy_get_user_data((struct wl_proxy *) bq_mgr);
}

/**
 * @ingroup bq_mgr
 * @brief Destroy a bq_mgr proxy.
 *
 * @param bq_mgr The bq_mgr proxy to destroy.
 */
static inline void
bq_mgr_destroy(struct bq_mgr *bq_mgr)
{
	wl_proxy_destroy((struct wl_proxy *) bq_mgr);
}

/**
 * @ingroup bq_mgr
 * @brief Create a new buffer queue consumer.
 *
 * This request creates a new bq_consumer object. The consumer is identified
 * by a unique name.
 *
 * @param bq_mgr The bq_mgr global object.
 * @param name A unique name for this consumer (e.g., "display_output").
 * @param queue_size The desired size of the buffer queue.
 * @param width The expected width of the buffers.
 * @param height The expected height of the buffers.
 * @return A new bq_consumer proxy object, or NULL on failure.
 */
static inline struct bq_consumer *
bq_mgr_create_consumer(struct bq_mgr *bq_mgr, const char *name, int32_t queue_size, int32_t width, int32_t height)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) bq_mgr,
			 BQ_MGR_CREATE_CONSUMER, &bq_consumer_interface, NULL, name, queue_size, width, height);

	return (struct bq_consumer *) id;
}

/**
 * @ingroup bq_mgr
 * @brief Create a new buffer queue provider.
 *
 * This request creates a new bq_provider object. The provider attempts to
 * connect to a consumer with the given name.
 *
 * @param bq_mgr The bq_mgr global object.
 * @param name The name of the consumer to connect to (e.g., "display_output").
 * @return A new bq_provider proxy object, or NULL on failure.
 */
static inline struct bq_provider *
bq_mgr_create_provider(struct bq_mgr *bq_mgr, const char *name)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) bq_mgr,
			 BQ_MGR_CREATE_PROVIDER, &bq_provider_interface, NULL, name);

	return (struct bq_provider *) id;
}

/**
 * @ingroup bq_consumer
 * @brief Listener for bq_consumer events.
 *
 * These events are sent by the server to the client for a bq_consumer object.
 */
struct bq_consumer_listener {
	/**
	 * @brief Notifies that a provider has connected to this consumer.
	 * @param data User data associated with the listener.
	 * @param bq_consumer The bq_consumer object.
	 */
	void (*connected)(void *data,
			  struct bq_consumer *bq_consumer);
	/**
	 * @brief Notifies that the provider has disconnected from this consumer.
	 * @param data User data associated with the listener.
	 * @param bq_consumer The bq_consumer object.
	 */
	void (*disconnected)(void *data,
			     struct bq_consumer *bq_consumer);
	/**
	 * @brief Notifies that a buffer has been attached by the provider.
	 *
	 * The consumer receives information about the buffer, such as its
	 * dimensions, format, and usage flags.
	 *
	 * @param data User data associated with the listener.
	 * @param bq_consumer The bq_consumer object.
	 * @param buffer The newly attached bq_buffer object.
	 * @param engine String identifying the buffer allocation engine (e.g., "gbm", "dmabuf").
	 * @param width Width of the buffer in pixels.
	 * @param height Height of the buffer in pixels.
	 * @param format Pixel format of the buffer (e.g., DRM_FORMAT_XRGB8888).
	 * @param flags Usage flags for the buffer.
	 */
	void (*buffer_attached)(void *data,
				struct bq_consumer *bq_consumer,
				struct bq_buffer *buffer,
				const char *engine,
				int32_t width,
				int32_t height,
				int32_t format,
				uint32_t flags);
	/**
	 * @brief Sets backend-specific buffer identifier and plane information.
	 *
	 * This event is used when the buffer is identified by a numerical ID
	 * (e.g., a gralloc handle ID).
	 *
	 * @param data User data associated with the listener.
	 * @param bq_consumer The bq_consumer object.
	 * @param buffer The bq_buffer object this information pertains to.
	 * @param id The backend-specific buffer ID.
	 * @param offset0 Offset for plane 0.
	 * @param stride0 Stride for plane 0.
	 * @param offset1 Offset for plane 1.
	 * @param stride1 Stride for plane 1.
	 * @param offset2 Offset for plane 2.
	 * @param stride2 Stride for plane 2.
	 */
	void (*set_buffer_id)(void *data,
			      struct bq_consumer *bq_consumer,
			      struct bq_buffer *buffer,
			      int32_t id,
			      int32_t offset0,
			      int32_t stride0,
			      int32_t offset1,
			      int32_t stride1,
			      int32_t offset2,
			      int32_t stride2);
	/**
	 * @brief Sets file descriptor and plane information for a DMABUF.
	 *
	 * This event is used when the buffer is represented by a file descriptor
	 * (e.g., a DMABUF fd).
	 *
	 * @param data User data associated with the listener.
	 * @param bq_consumer The bq_consumer object.
	 * @param buffer The bq_buffer object this information pertains to.
	 * @param fd The file descriptor for the buffer.
	 * @param offset0 Offset for plane 0.
	 * @param stride0 Stride for plane 0.
	 * @param offset1 Offset for plane 1.
	 * @param stride1 Stride for plane 1.
	 * @param offset2 Offset for plane 2.
	 * @param stride2 Stride for plane 2.
	 */
	void (*set_buffer_fd)(void *data,
			      struct bq_consumer *bq_consumer,
			      struct bq_buffer *buffer,
			      int32_t fd,
			      int32_t offset0,
			      int32_t stride0,
			      int32_t offset1,
			      int32_t stride1,
			      int32_t offset2,
			      int32_t stride2);
	/**
	 * @brief Notifies that a buffer has been detached by the provider.
	 *
	 * The consumer should release any resources associated with this buffer.
	 * The bq_buffer object is destroyed after this event.
	 *
	 * @param data User data associated with the listener.
	 * @param bq_consumer The bq_consumer object.
	 * @param buffer The bq_buffer object that was detached.
	 */
	void (*buffer_detached)(void *data,
				struct bq_consumer *bq_consumer,
				struct bq_buffer *buffer);
	/**
	 * @brief Notifies that a buffer has been enqueued by the provider.
	 *
	 * The consumer can now acquire and use this buffer.
	 *
	 * @param data User data associated with the listener.
	 * @param bq_consumer The bq_consumer object.
	 * @param buffer The bq_buffer object that was enqueued.
	 * @param serial A serial number associated with this enqueue operation.
	 */
	void (*add_buffer)(void *data,
			   struct bq_consumer *bq_consumer,
			   struct bq_buffer *buffer,
			   uint32_t serial);
};

/**
 * @ingroup bq_consumer
 * @brief Add a listener for bq_consumer events.
 *
 * @param bq_consumer The bq_consumer proxy.
 * @param listener The listener implementation.
 * @param data User data to be passed to listener callbacks.
 * @return 0 on success, -1 on failure.
 */
static inline int
bq_consumer_add_listener(struct bq_consumer *bq_consumer,
			 const struct bq_consumer_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) bq_consumer,
				     (void (**)(void)) listener, data);
}

/** @ingroup bq_consumer
 *  @brief Request to release a buffer back to the provider.
 */
#define BQ_CONSUMER_RELEASE_BUFFER	0

/**
 * @ingroup bq_consumer
 * @brief Set user data for a bq_consumer proxy.
 *
 * @param bq_consumer The bq_consumer proxy.
 * @param user_data The user data to associate.
 */
static inline void
bq_consumer_set_user_data(struct bq_consumer *bq_consumer, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) bq_consumer, user_data);
}

/**
 * @ingroup bq_consumer
 * @brief Get user data associated with a bq_consumer proxy.
 *
 * @param bq_consumer The bq_consumer proxy.
 * @return The user data.
 */
static inline void *
bq_consumer_get_user_data(struct bq_consumer *bq_consumer)
{
	return wl_proxy_get_user_data((struct wl_proxy *) bq_consumer);
}

/**
 * @ingroup bq_consumer
 * @brief Destroy a bq_consumer proxy.
 *
 * @param bq_consumer The bq_consumer proxy to destroy.
 */
static inline void
bq_consumer_destroy(struct bq_consumer *bq_consumer)
{
	wl_proxy_destroy((struct wl_proxy *) bq_consumer);
}

/**
 * @ingroup bq_consumer
 * @brief Release a buffer back to the provider.
 *
 * The consumer calls this after it has finished using a buffer that was
 * previously acquired via the `add_buffer` event.
 *
 * @param bq_consumer The bq_consumer object.
 * @param buffer The bq_buffer to release.
 */
static inline void
bq_consumer_release_buffer(struct bq_consumer *bq_consumer, struct bq_buffer *buffer)
{
	wl_proxy_marshal((struct wl_proxy *) bq_consumer,
			 BQ_CONSUMER_RELEASE_BUFFER, buffer);
}

#ifndef BQ_PROVIDER_ERROR_ENUM
#define BQ_PROVIDER_ERROR_ENUM
/** @ingroup bq_provider
 *  @brief Errors that can be emitted by the bq_provider.
 */
enum bq_provider_error {
	/** The provider attempted to enqueue more buffers than the queue_size allows. */
	BQ_PROVIDER_ERROR_OVERFLOW_QUEUE_SIZE = 0,
	/** A connection error occurred (e.g., consumer not found or disconnected). */
	BQ_PROVIDER_ERROR_CONNECTION = 1,
};
#endif /* BQ_PROVIDER_ERROR_ENUM */

/**
 * @ingroup bq_provider
 * @brief Listener for bq_provider events.
 *
 * These events are sent by the server to the client for a bq_provider object.
 */
struct bq_provider_listener {
	/**
	 * @brief Notifies that the provider has successfully connected to a consumer.
	 *
	 * The provider receives the actual queue size and buffer dimensions
	 * negotiated with the consumer.
	 *
	 * @param data User data associated with the listener.
	 * @param bq_provider The bq_provider object.
	 * @param queue_size The actual size of the buffer queue.
	 * @param width The width of the buffers to be used.
	 * @param height The height of the buffers to be used.
	 */
	void (*connected)(void *data,
			  struct bq_provider *bq_provider,
			  int32_t queue_size,
			  int32_t width,
			  int32_t height);
	/**
	 * @brief Notifies that the provider has been disconnected from the consumer.
	 * @param data User data associated with the listener.
	 * @param bq_provider The bq_provider object.
	 */
	void (*disconnected)(void *data,
			     struct bq_provider *bq_provider);
	/**
	 * @brief Notifies that a buffer has been released by the consumer.
	 *
	 * The provider can now reuse or re-enqueue this buffer.
	 *
	 * @param data User data associated with the listener.
	 * @param bq_provider The bq_provider object.
	 * @param buffer The bq_buffer object that was released.
	 * @param serial A serial number associated with the release operation.
	 */
	void (*add_buffer)(void *data,
			   struct bq_provider *bq_provider,
			   struct bq_buffer *buffer,
			   uint32_t serial);
};

/**
 * @ingroup bq_provider
 * @brief Add a listener for bq_provider events.
 *
 * @param bq_provider The bq_provider proxy.
 * @param listener The listener implementation.
 * @param data User data to be passed to listener callbacks.
 * @return 0 on success, -1 on failure.
 */
static inline int
bq_provider_add_listener(struct bq_provider *bq_provider,
			 const struct bq_provider_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) bq_provider,
				     (void (**)(void)) listener, data);
}

/** @ingroup bq_provider
 *  @brief Request to attach a new buffer to the queue.
 */
#define BQ_PROVIDER_ATTACH_BUFFER	0
/** @ingroup bq_provider
 *  @brief Request to set backend-specific buffer ID and plane info.
 */
#define BQ_PROVIDER_SET_BUFFER_ID	1
/** @ingroup bq_provider
 *  @brief Request to set DMABUF file descriptor and plane info.
 */
#define BQ_PROVIDER_SET_BUFFER_FD	2
/** @ingroup bq_provider
 *  @brief Request to detach a buffer from the queue.
 */
#define BQ_PROVIDER_DETACH_BUFFER	3
/** @ingroup bq_provider
 *  @brief Request to enqueue a buffer for the consumer.
 */
#define BQ_PROVIDER_ENQUEUE_BUFFER	4

/**
 * @ingroup bq_provider
 * @brief Set user data for a bq_provider proxy.
 *
 * @param bq_provider The bq_provider proxy.
 * @param user_data The user data to associate.
 */
static inline void
bq_provider_set_user_data(struct bq_provider *bq_provider, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) bq_provider, user_data);
}

/**
 * @ingroup bq_provider
 * @brief Get user data associated with a bq_provider proxy.
 *
 * @param bq_provider The bq_provider proxy.
 * @return The user data.
 */
static inline void *
bq_provider_get_user_data(struct bq_provider *bq_provider)
{
	return wl_proxy_get_user_data((struct wl_proxy *) bq_provider);
}

/**
 * @ingroup bq_provider
 * @brief Destroy a bq_provider proxy.
 *
 * @param bq_provider The bq_provider proxy to destroy.
 */
static inline void
bq_provider_destroy(struct bq_provider *bq_provider)
{
	wl_proxy_destroy((struct wl_proxy *) bq_provider);
}

/**
 * @ingroup bq_provider
 * @brief Attach a new buffer to the queue.
 *
 * The provider calls this to allocate a new bq_buffer object.
 * The actual buffer memory is not necessarily allocated by this call,
 * but the metadata (engine, dimensions, format, flags) is established.
 *
 * @param bq_provider The bq_provider object.
 * @param engine String identifying the buffer allocation engine (e.g., "gbm", "dmabuf").
 * @param width Width of the buffer in pixels.
 * @param height Height of the buffer in pixels.
 * @param format Pixel format of the buffer (e.g., DRM_FORMAT_XRGB8888).
 * @param flags Usage flags for the buffer.
 * @return A new bq_buffer proxy object, or NULL on failure.
 */
static inline struct bq_buffer *
bq_provider_attach_buffer(struct bq_provider *bq_provider, const char *engine, int32_t width, int32_t height, int32_t format, uint32_t flags)
{
	struct wl_proxy *buffer;

	buffer = wl_proxy_marshal_constructor((struct wl_proxy *) bq_provider,
			 BQ_PROVIDER_ATTACH_BUFFER, &bq_buffer_interface, NULL, engine, width, height, format, flags);

	return (struct bq_buffer *) buffer;
}

/**
 * @ingroup bq_provider
 * @brief Set backend-specific buffer identifier and plane information.
 *
 * This request is used when the buffer is identified by a numerical ID.
 * It must be called after `bq_provider_attach_buffer` and before
 * `bq_provider_enqueue_buffer` for the given buffer.
 *
 * @param bq_provider The bq_provider object.
 * @param buffer The bq_buffer object to modify.
 * @param id The backend-specific buffer ID.
 * @param offset0 Offset for plane 0.
 * @param stride0 Stride for plane 0.
 * @param offset1 Offset for plane 1.
 * @param stride1 Stride for plane 1.
 * @param offset2 Offset for plane 2.
 * @param stride2 Stride for plane 2.
 */
static inline void
bq_provider_set_buffer_id(struct bq_provider *bq_provider, struct bq_buffer *buffer, int32_t id, int32_t offset0, int32_t stride0, int32_t offset1, int32_t stride1, int32_t offset2, int32_t stride2)
{
	wl_proxy_marshal((struct wl_proxy *) bq_provider,
			 BQ_PROVIDER_SET_BUFFER_ID, buffer, id, offset0, stride0, offset1, stride1, offset2, stride2);
}

/**
 * @ingroup bq_provider
 * @brief Set file descriptor and plane information for a DMABUF.
 *
 * This request is used when the buffer is represented by a file descriptor.
 * It must be called after `bq_provider_attach_buffer` and before
 * `bq_provider_enqueue_buffer` for the given buffer. The ownership of the
 * file descriptor is transferred to the compositor.
 *
 * @param bq_provider The bq_provider object.
 * @param buffer The bq_buffer object to modify.
 * @param fd The file descriptor for the buffer.
 * @param offset0 Offset for plane 0.
 * @param stride0 Stride for plane 0.
 * @param offset1 Offset for plane 1.
 * @param stride1 Stride for plane 1.
 * @param offset2 Offset for plane 2.
 * @param stride2 Stride for plane 2.
 */
static inline void
bq_provider_set_buffer_fd(struct bq_provider *bq_provider, struct bq_buffer *buffer, int32_t fd, int32_t offset0, int32_t stride0, int32_t offset1, int32_t stride1, int32_t offset2, int32_t stride2)
{
	wl_proxy_marshal((struct wl_proxy *) bq_provider,
			 BQ_PROVIDER_SET_BUFFER_FD, buffer, fd, offset0, stride0, offset1, stride1, offset2, stride2);
}

/**
 * @ingroup bq_provider
 * @brief Detach a buffer from the queue.
 *
 * The provider calls this to indicate it will no longer use this buffer.
 * The bq_buffer object is destroyed after this request.
 *
 * @param bq_provider The bq_provider object.
 * @param buffer The bq_buffer to detach.
 */
static inline void
bq_provider_detach_buffer(struct bq_provider *bq_provider, struct bq_buffer *buffer)
{
	wl_proxy_marshal((struct wl_proxy *) bq_provider,
			 BQ_PROVIDER_DETACH_BUFFER, buffer);
}

/**
 * @ingroup bq_provider
 * @brief Enqueue a buffer for the consumer.
 *
 * After attaching and setting buffer details (ID or FD), the provider
 * calls this to make the buffer available to the consumer.
 *
 * @param bq_provider The bq_provider object.
 * @param buffer The bq_buffer to enqueue.
 * @param serial A serial number for this enqueue operation, to be echoed
 *               by the consumer when it releases the buffer.
 */
static inline void
bq_provider_enqueue_buffer(struct bq_provider *bq_provider, struct bq_buffer *buffer, uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *) bq_provider,
			 BQ_PROVIDER_ENQUEUE_BUFFER, buffer, serial);
}

/**
 * @ingroup bq_buffer
 * @brief Set user data for a bq_buffer proxy.
 *
 * @param bq_buffer The bq_buffer proxy.
 * @param user_data The user data to associate.
 */
static inline void
bq_buffer_set_user_data(struct bq_buffer *bq_buffer, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) bq_buffer, user_data);
}

/**
 * @ingroup bq_buffer
 * @brief Get user data associated with a bq_buffer proxy.
 *
 * @param bq_buffer The bq_buffer proxy.
 * @return The user data.
 */
static inline void *
bq_buffer_get_user_data(struct bq_buffer *bq_buffer)
{
	return wl_proxy_get_user_data((struct wl_proxy *) bq_buffer);
}

/**
 * @ingroup bq_buffer
 * @brief Destroy a bq_buffer proxy.
 *
 * Note: Buffers are typically destroyed implicitly when detached or
 * when their parent (provider or consumer) is destroyed.
 * This function provides an explicit way to destroy the proxy if needed.
 *
 * @param bq_buffer The bq_buffer proxy to destroy.
 */
static inline void
bq_buffer_destroy(struct bq_buffer *bq_buffer)
{
	wl_proxy_destroy((struct wl_proxy *) bq_buffer);
}

#ifdef  __cplusplus
}
#endif

#endif
