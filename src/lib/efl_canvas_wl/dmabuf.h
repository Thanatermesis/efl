/* Shamelessly stolen from weston and modified, original license boiler plate
 * follows.
 */
/*
 * Copyright © 2014, 2015 Collabora, Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @file
 * @brief Weston-based Linux DMA-BUF Wayland protocol implementation.
 *
 * This file defines the structures and functions for handling DMA-BUF
 * buffers within a Wayland compositor, based on the
 * `zwp_linux_dmabuf_v1` protocol. It allows clients to share hardware
 * buffers (typically from a GPU or camera) with the compositor efficiently.
 */

#ifndef WESTON_LINUX_DMABUF_H
#define WESTON_LINUX_DMABUF_H

#include <stdint.h>
#include <wayland-server-core.h> // For wl_resource, wl_display

#define MAX_DMABUF_PLANES 4
#ifndef DRM_FORMAT_MOD_INVALID
#define DRM_FORMAT_MOD_INVALID ((1ULL<<56) - 1)
#endif

struct linux_dmabuf_buffer;

/**
 * @brief Callback function type for destroying user-specific data.
 *
 * This function is called when a `linux_dmabuf_buffer` is being destroyed,
 * allowing for cleanup of any associated user data.
 * @param buffer The DMA-BUF buffer whose user data is to be destroyed.
 */
typedef void (*dmabuf_user_data_destroy_func)(
			struct linux_dmabuf_buffer *buffer);

/**
 * @brief Attributes of a DMA-BUF buffer.
 *
 * This structure holds the metadata for a DMA-BUF, as provided by a client
 * through the `zwp_linux_buffer_params_v1` interface.
 */
struct dmabuf_attributes {
        /** @brief Protocol version of the params object. Currently 1. */
        int version;
	/** @brief Width of the buffer in pixels. */
	int32_t width;
	/** @brief Height of the buffer in pixels. */
	int32_t height;
	/** @brief DRM_FORMAT code (e.g., DRM_FORMAT_ARGB8888). */
	uint32_t format;
	/** @brief Flags, e.g., for Y-inversion. See `enum zlinux_buffer_params_flags`. */
	uint32_t flags;
	/** @brief Number of planes used by this buffer. */
	int n_planes;
	/**
	 * @brief Array of file descriptors for each plane.
	 * Example: `fd[0]` is the FD for the first plane.
	 */
	int fd[MAX_DMABUF_PLANES];
	/**
	 * @brief Array of offsets for each plane within its FD.
	 * Example: `offset[0]` is the offset for the first plane.
	 */
	uint32_t offset[MAX_DMABUF_PLANES];
	/**
	 * @brief Array of strides for each plane in bytes.
	 * Example: `stride[0]` is the stride for the first plane.
	 */
	uint32_t stride[MAX_DMABUF_PLANES];
	/**
	 * @brief Array of DRM format modifiers for each plane.
	 * Example: `modifier[0]` is the modifier for the first plane.
	 * `DRM_FORMAT_MOD_INVALID` if no modifier is set.
	 * `DRM_FORMAT_MOD_LINEAR` for linear layout.
	 */
	uint64_t modifier[MAX_DMABUF_PLANES];
};

/**
 * @brief Represents a DMA-BUF buffer in the compositor.
 *
 * This structure is the compositor-side representation of a client-provided
 * DMA-BUF. It is associated with either a `zwp_linux_buffer_params_v1`
 * resource (during creation) or a `wl_buffer` resource (after creation).
 */
struct linux_dmabuf_buffer {
	/** @brief The `wl_buffer` resource, or NULL if not yet created. */
	struct wl_resource *buffer_resource;
	/** @brief The `zwp_linux_buffer_params_v1` resource, or NULL if consumed. */
	struct wl_resource *params_resource;
	/** @brief Opaque pointer to the compositor's main data structure. */
	void *compositor;
	/** @brief The attributes of this DMA-BUF. */
	struct dmabuf_attributes attributes;

	/** @brief Backend-specific or renderer-specific private data. */
	void *user_data;
	/** @brief Function to destroy `user_data`. */
	dmabuf_user_data_destroy_func user_data_destroy_func;

	/* XXX:
	 *
	 * Add backend private data. This would be for the backend
	 * to do all additional imports it might ever use in advance.
	 * The basic principle, even if not implemented in drivers today,
	 * is that dmabufs are first attached, but the actual allocation
	 * is deferred to first use. This would allow the exporter and all
	 * attachers to agree on how to allocate.
	 *
	 * The DRM backend would use this to create drmFBs for each
	 * dmabuf_buffer, just in case at some point it would become
	 * feasible to scan it out directly. This would improve the
	 * possibilities to successfully scan out, avoiding compositing.
	 */
};

/**
 * @brief Initializes and advertises Linux DMA-BUF support.
 *
 * Sets up the `zwp_linux_dmabuf_v1` global, allowing clients to use the
 * DMA-BUF protocol. This should be called once during compositor initialization.
 *
 * @param display The Wayland display object.
 * @param comp Opaque pointer to the compositor's main data structure,
 *             passed to bind requests.
 * @return 0 on success, -1 on failure (e.g., if global creation fails).
 */
int
linux_dmabuf_setup(struct wl_display *display, void *comp);

/**
 * @brief Retrieves the `linux_dmabuf_buffer` associated with a `wl_buffer` resource.
 *
 * This function checks if the given `wl_resource` is a `wl_buffer` created
 * via the Linux DMA-BUF protocol and, if so, returns its associated
 * `linux_dmabuf_buffer` structure. This can be used as a type-check.
 *
 * @param resource The `wl_resource` to check. Expected to be a `wl_buffer`.
 * @return A pointer to the `linux_dmabuf_buffer` if the resource is a
 *         DMA-BUF wl_buffer, or NULL otherwise (e.g., if it's a shm buffer
 *         or an invalid resource).
 */
struct linux_dmabuf_buffer *
linux_dmabuf_buffer_get(struct wl_resource *resource);

/**
 * @brief Sets backend-specific or renderer-specific private data for a DMA-BUF buffer.
 *
 * This allows a compositor backend (e.g., DRM, GL renderer) to associate its
 * own data with a `linux_dmabuf_buffer`. It's an error to overwrite existing
 * non-NULL user data with new non-NULL data.
 *
 * @param buffer The DMA-BUF buffer to associate data with.
 * @param data The private data pointer.
 * @param func A destructor function for the private data, called when the
 *             `linux_dmabuf_buffer` is destroyed. Can be NULL if no cleanup
 *             is needed for `data`.
 */
void
linux_dmabuf_buffer_set_user_data(struct linux_dmabuf_buffer *buffer,
				  void *data,
				  dmabuf_user_data_destroy_func func);
/**
 * @brief Retrieves the backend-specific or renderer-specific private data.
 *
 * @param buffer The DMA-BUF buffer to query.
 * @return The private data pointer previously set by
 *         `linux_dmabuf_buffer_set_user_data`, or NULL if none was set.
 */
void *
linux_dmabuf_buffer_get_user_data(struct linux_dmabuf_buffer *buffer);

/**
 * @brief Sends a fatal error to the client related to a DMA-BUF buffer.
 *
 * This function is used when a DMA-BUF buffer becomes unusable (e.g., import
 * failed at the renderer level) and the compositor cannot recover. It posts
 * a `WL_DISPLAY_ERROR_INVALID_OBJECT` error on the client's `wl_display`
 * resource, typically leading to client disconnection.
 *
 * @param buffer The problematic `linux_dmabuf_buffer`.
 * @param msg A descriptive error message to be sent to the client.
 */
void
linux_dmabuf_buffer_send_server_error(struct linux_dmabuf_buffer *buffer,
				      const char *msg);

#endif /* WESTON_LINUX_DMABUF_H */
