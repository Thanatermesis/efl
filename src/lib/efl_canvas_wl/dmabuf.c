/* Shamelessly stolen from weston and modified, original license boiler plate
 * follows.
 */

#if defined(__clang__)
# pragma clang diagnostic ignored "-Wunused-parameter"
#elif (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4
# pragma GCC diagnostic ignored "-Wunused-parameter"
#endif


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
 * @brief Implementation of the Linux DMA-BUF Wayland protocol handling.
 *
 * This file contains the server-side implementation for the
 * `zwp_linux_dmabuf_v1` Wayland protocol. It handles requests from clients
 * to create `wl_buffer` objects from DMA-BUF file descriptors.
 */

#include <sys/mman.h>

#include <assert.h>

#include "linux-dmabuf-unstable-v1-server-protocol.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dmabuf.h"
#include <Eina.h>

__attribute__ ((visibility("hidden"))) Eina_Bool comp_dmabuf_test(struct linux_dmabuf_buffer *dmabuf);
__attribute__ ((visibility("hidden"))) void comp_dmabuf_formats_query(void *c, int **formats, int *num_formats);
__attribute__ ((visibility("hidden"))) void comp_dmabuf_modifiers_query(void *c, int format, uint64_t **modifiers, int *num_modifiers);

/**
 * @brief Destroys a `linux_dmabuf_buffer` instance.
 *
 * Closes all file descriptors associated with the buffer's planes and frees
 * the memory allocated for the `linux_dmabuf_buffer` structure itself.
 * This function does not handle the destruction of associated Wayland resources
 * or user data; those are managed by their respective destructors.
 *
 * @param buffer The DMA-BUF buffer to destroy.
 */
static void
linux_dmabuf_buffer_destroy(struct linux_dmabuf_buffer *buffer)
{
	int i;

	for (i = 0; i < buffer->attributes.n_planes; i++) {
		close(buffer->attributes.fd[i]);
		buffer->attributes.fd[i] = -1;
	}

	buffer->attributes.n_planes = 0;
	free(buffer);
}

/**
 * @brief Destructor for the `zwp_linux_buffer_params_v1` resource.
 *
 * This function is called when a `zwp_linux_buffer_params_v1` resource is
 * destroyed by the Wayland library (e.g., client disconnects or explicitly
 * destroys the object). If the params object was not used to create a
 * `wl_buffer` (i.e., `buffer->buffer_resource` is NULL), this function
 * cleans up the associated `linux_dmabuf_buffer`.
 *
 * @param params_resource The `zwp_linux_buffer_params_v1` resource being destroyed.
 */
static void
destroy_params(struct wl_resource *params_resource)
{
	struct linux_dmabuf_buffer *buffer;

	buffer = wl_resource_get_user_data(params_resource);

	if (!buffer)
		return;

	linux_dmabuf_buffer_destroy(buffer);
}

/**
 * @brief Handles the `destroy` request for `zwp_linux_buffer_params_v1`.
 *
 * This is the Wayland protocol request to explicitly destroy a params object.
 *
 * @param client The client that sent the request.
 * @param resource The `zwp_linux_buffer_params_v1` resource to be destroyed.
 */
static void
params_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

/**
 * @brief Handles the `add` request for `zwp_linux_buffer_params_v1`.
 *
 * Adds a DMA-BUF file descriptor and its associated plane metadata (offset,
 * stride, modifier) to the params object. This can be called multiple times
 * for multi-planar formats.
 *
 * @param client The client that sent the request.
 * @param params_resource The `zwp_linux_buffer_params_v1` resource to which
 *                        the plane information is being added.
 * @param name_fd The file descriptor for the DMA-BUF.
 * @param plane_idx The index of the plane being added (0-based).
 * @param offset The offset within the DMA-BUF for this plane.
 * @param stride The stride (row pitch in bytes) for this plane.
 * @param modifier_hi The upper 32 bits of the 64-bit DRM format modifier.
 * @param modifier_lo The lower 32 bits of the 64-bit DRM format modifier.
 */
static void
params_add(struct wl_client *client,
	   struct wl_resource *params_resource,
	   int32_t name_fd,
	   uint32_t plane_idx,
	   uint32_t offset,
	   uint32_t stride,
	   uint32_t modifier_hi,
	   uint32_t modifier_lo)
{
	struct linux_dmabuf_buffer *buffer;

	buffer = wl_resource_get_user_data(params_resource);
	if (!buffer) {
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_ALREADY_USED,
			"params was already used to create a wl_buffer");
		close(name_fd);
		return;
	}

	assert(buffer->params_resource == params_resource);
	assert(!buffer->buffer_resource);

	if (plane_idx >= MAX_DMABUF_PLANES) {
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_PLANE_IDX,
			"plane index %u is too high", plane_idx);
		close(name_fd);
		return;
	}

	if (buffer->attributes.fd[plane_idx] != -1) {
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_PLANE_SET,
			"a dmabuf has already been added for plane %u",
			plane_idx);
		close(name_fd);
		return;
	}

	buffer->attributes.fd[plane_idx] = name_fd;
	buffer->attributes.offset[plane_idx] = offset;
	buffer->attributes.stride[plane_idx] = stride;
	buffer->attributes.modifier[plane_idx] = ((uint64_t)modifier_hi << 32) |
						 modifier_lo;
	buffer->attributes.n_planes++;
}

static void
linux_dmabuf_wl_buffer_destroy(struct wl_client *client,
			       struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_buffer_interface linux_dmabuf_buffer_implementation = {
	linux_dmabuf_wl_buffer_destroy
};

/**
 * @brief Destructor for the `wl_buffer` resource created from a DMA-BUF.
 *
 * This function is called when a `wl_buffer` (that was created via the
 * DMA-BUF protocol) is destroyed. It calls the user data destructor if one
 * was set, and then cleans up the underlying `linux_dmabuf_buffer` structure.
 *
 * @param resource The `wl_buffer` resource being destroyed.
 */
static void
destroy_linux_dmabuf_wl_buffer(struct wl_resource *resource)
{
	struct linux_dmabuf_buffer *buffer;

	buffer = wl_resource_get_user_data(resource);
	assert(buffer->buffer_resource == resource);
	assert(!buffer->params_resource);

	if (buffer->user_data_destroy_func)
		buffer->user_data_destroy_func(buffer);

	linux_dmabuf_buffer_destroy(buffer);
}

/**
 * @brief Common logic for creating a `wl_buffer` from `zwp_linux_buffer_params_v1`.
 *
 * This function handles both `create` and `create_immed` requests. It validates
 * the parameters (dimensions, format, plane data), checks for completeness,
 * performs sanity checks on offsets and strides against FD sizes, and
 * attempts to import the buffer using the compositor-specific `comp_dmabuf_test`
 * function. If successful, it creates a `wl_buffer` resource.
 *
 * @param client The client that initiated the buffer creation.
 * @param params_resource The `zwp_linux_buffer_params_v1` resource containing
 *                        the buffer definition. This resource will be consumed
 *                        (user_data set to NULL) by this function.
 * @param buffer_id If non-zero, this is the ID for the new `wl_buffer` resource
 *                  (for `create_immed`). If zero (for `create`), a new ID is
 *                  not pre-assigned, and the `created` event will carry the
 *                  new `wl_buffer`.
 * @param width The width of the buffer in pixels.
 * @param height The height of the buffer in pixels.
 * @param format The DRM pixel format (e.g., `DRM_FORMAT_ARGB8888`).
 * @param flags Flags for the buffer (e.g., `ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_Y_INVERT`).
 */
static void
params_create_common(struct wl_client *client,
		     struct wl_resource *params_resource,
		     uint32_t buffer_id,
		     int32_t width,
		     int32_t height,
		     uint32_t format,
		     uint32_t flags)
{
	struct linux_dmabuf_buffer *buffer;
	int i;

	buffer = wl_resource_get_user_data(params_resource);

	if (!buffer) {
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_ALREADY_USED,
			"params was already used to create a wl_buffer");
		return;
	}

	assert(buffer->params_resource == params_resource);
	assert(!buffer->buffer_resource);

	/* Switch the linux_dmabuf_buffer object from params resource to
	 * eventually wl_buffer resource.
	 */
	wl_resource_set_user_data(buffer->params_resource, NULL);
	buffer->params_resource = NULL;

	if (!buffer->attributes.n_planes) {
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INCOMPLETE,
			"no dmabuf has been added to the params");
		goto err_out;
	}

	/* Check for holes in the dmabufs set (e.g. [0, 1, 3]) */
	for (i = 0; i < buffer->attributes.n_planes; i++) {
		if (buffer->attributes.fd[i] == -1) {
			wl_resource_post_error(params_resource,
				ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INCOMPLETE,
				"no dmabuf has been added for plane %i", i);
			goto err_out;
		}
	}

        buffer->attributes.version = 1;
	buffer->attributes.width = width;
	buffer->attributes.height = height;
	buffer->attributes.format = format;
	buffer->attributes.flags = flags;

	if (width < 1 || height < 1) {
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INVALID_DIMENSIONS,
			"invalid width %d or height %d", width, height);
		goto err_out;
	}

	for (i = 0; i < buffer->attributes.n_planes; i++) {
		off_t size;

		if ((uint64_t) buffer->attributes.offset[i] + buffer->attributes.stride[i] > UINT32_MAX) {
			wl_resource_post_error(params_resource,
				ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_OUT_OF_BOUNDS,
				"size overflow for plane %i", i);
			goto err_out;
		}

		if (i == 0 &&
		   (uint64_t) buffer->attributes.offset[i] +
		   (uint64_t) buffer->attributes.stride[i] * height > UINT32_MAX) {
			wl_resource_post_error(params_resource,
				ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_OUT_OF_BOUNDS,
				"size overflow for plane %i", i);
			goto err_out;
		}

		/* Don't report an error as it might be caused
		 * by the kernel not supporting seeking on dmabuf */
		size = lseek(buffer->attributes.fd[i], 0, SEEK_END);
		if (size == -1)
			continue;

		if (buffer->attributes.offset[i] >= size) {
			wl_resource_post_error(params_resource,
				ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_OUT_OF_BOUNDS,
				"invalid offset %i for plane %i",
				buffer->attributes.offset[i], i);
			goto err_out;
		}

		if (buffer->attributes.offset[i] + buffer->attributes.stride[i] > size) {
			wl_resource_post_error(params_resource,
				ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_OUT_OF_BOUNDS,
				"invalid stride %i for plane %i",
				buffer->attributes.stride[i], i);
			goto err_out;
		}

		/* Only valid for first plane as other planes might be
		 * sub-sampled according to fourcc format */
		if (i == 0 &&
		    buffer->attributes.offset[i] + buffer->attributes.stride[i] * height > size) {
			wl_resource_post_error(params_resource,
				ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_OUT_OF_BOUNDS,
				"invalid buffer stride or height for plane %i", i);
			goto err_out;
		}
	}

	/* XXX: Some additional sanity checks could be done with respect
	 * to the fourcc format. A centralized collection (kernel or
	 * libdrm) would be useful to avoid code duplication for these
	 * checks (e.g. drm_format_num_planes).
	 */

	if (!comp_dmabuf_test(buffer))
		goto err_failed;

	buffer->buffer_resource = wl_resource_create(client,
						     &wl_buffer_interface,
						     1, buffer_id);
	if (!buffer->buffer_resource) {
		wl_resource_post_no_memory(params_resource);
		goto err_buffer;
	}

	wl_resource_set_implementation(buffer->buffer_resource,
				       &linux_dmabuf_buffer_implementation,
				       buffer, destroy_linux_dmabuf_wl_buffer);

	/* send 'created' event when the request is not for an immediate
	 * import, ie buffer_id is zero */
	if (buffer_id == 0)
		zwp_linux_buffer_params_v1_send_created(params_resource,
						buffer->buffer_resource);

	return;

err_buffer:
	if (buffer->user_data_destroy_func)
		buffer->user_data_destroy_func(buffer);

err_failed:
	if (buffer_id == 0)
		zwp_linux_buffer_params_v1_send_failed(params_resource);
	else
		/* since the behavior is left implementation defined by the
		 * protocol in case of create_immed failure due to an unknown cause,
		 * we choose to treat it as a fatal error and immediately kill the
		 * client instead of creating an invalid handle and waiting for it
		 * to be used.
		 */
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INVALID_WL_BUFFER,
			"importing the supplied dmabufs failed");

err_out:
	linux_dmabuf_buffer_destroy(buffer);
}

static void
params_create(struct wl_client *client,
	      struct wl_resource *params_resource,
	      int32_t width,
	      int32_t height,
	      uint32_t format,
	      uint32_t flags)
{
	params_create_common(client, params_resource, 0, width, height, format,
			     flags);
}

/**
 * @brief Handles the `create_immed` request for `zwp_linux_buffer_params_v1`.
 *
 * This request creates a `wl_buffer` immediately with a client-supplied ID.
 * It calls `params_create_common` to perform the actual creation and validation.
 * If creation fails, it's a protocol error, and the client is typically
 * disconnected.
 *
 * @param client The client that sent the request.
 * @param params_resource The `zwp_linux_buffer_params_v1` resource.
 * @param buffer_id The ID to be used for the new `wl_buffer` resource.
 * @param width The width of the buffer.
 * @param height The height of the buffer.
 * @param format The DRM pixel format.
 * @param flags Buffer flags.
 */
static void
params_create_immed(struct wl_client *client,
		    struct wl_resource *params_resource,
		    uint32_t buffer_id,
		    int32_t width,
		    int32_t height,
		    uint32_t format,
		    uint32_t flags)
{
	params_create_common(client, params_resource, buffer_id, width, height,
			     format, flags);
}

static const struct zwp_linux_buffer_params_v1_interface
zwp_linux_buffer_params_implementation = {
	params_destroy,
	params_add,
	params_create,
	params_create_immed
};

/**
 * @brief Handles the `destroy` request for `zwp_linux_dmabuf_v1`.
 *
 * This is the Wayland protocol request to explicitly destroy the main
 * `zwp_linux_dmabuf_v1` factory object.
 *
 * @param client The client that sent the request.
 * @param resource The `zwp_linux_dmabuf_v1` resource to be destroyed.
 */
static void
linux_dmabuf_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

/**
 * @brief Handles the `create_params` request for `zwp_linux_dmabuf_v1`.
 *
 * Creates a new `zwp_linux_buffer_params_v1` object. This object is then
 * used by the client to specify the attributes of a DMA-BUF and subsequently
 * create a `wl_buffer` from it.
 *
 * @param client The client that sent the request.
 * @param linux_dmabuf_resource The main `zwp_linux_dmabuf_v1` factory resource.
 * @param params_id The ID to be used for the new `zwp_linux_buffer_params_v1` resource.
 */
static void
linux_dmabuf_create_params(struct wl_client *client,
			   struct wl_resource *linux_dmabuf_resource,
			   uint32_t params_id)
{
	void *compositor;
	struct linux_dmabuf_buffer *buffer;
	uint32_t version;
	int i;

	version = wl_resource_get_version(linux_dmabuf_resource);
	compositor = wl_resource_get_user_data(linux_dmabuf_resource);

	buffer = calloc(1, sizeof *buffer);
	if (!buffer)
		goto err_out;

	for (i = 0; i < MAX_DMABUF_PLANES; i++)
		buffer->attributes.fd[i] = -1;

	buffer->compositor = compositor;
	buffer->params_resource =
		wl_resource_create(client,
				   &zwp_linux_buffer_params_v1_interface,
				   version, params_id);
	if (!buffer->params_resource)
		goto err_dealloc;

	wl_resource_set_implementation(buffer->params_resource,
				       &zwp_linux_buffer_params_implementation,
				       buffer, destroy_params);

	return;

err_dealloc:
	free(buffer);

err_out:
	wl_resource_post_no_memory(linux_dmabuf_resource);
}

/** Get the linux_dmabuf_buffer from a wl_buffer resource
 *
 * If the given wl_buffer resource was created through the linux_dmabuf
 * protocol interface, returns the linux_dmabuf_buffer object. This can
 * be used as a type check for a wl_buffer.
 *
 * \param resource A wl_buffer resource.
 * \return The linux_dmabuf_buffer if it exists, or NULL otherwise.
 */
struct linux_dmabuf_buffer *
linux_dmabuf_buffer_get(struct wl_resource *resource)
{
	struct linux_dmabuf_buffer *buffer;

	if (!resource)
		return NULL;

	if (!wl_resource_instance_of(resource, &wl_buffer_interface,
				     &linux_dmabuf_buffer_implementation))
		return NULL;

	buffer = wl_resource_get_user_data(resource);
	assert(buffer);
	assert(!buffer->params_resource);
	assert(buffer->buffer_resource == resource);

	return buffer;
}

/** Set renderer-private data
 *
 * Set the user data for the linux_dmabuf_buffer. It is invalid to overwrite
 * a non-NULL user data with a new non-NULL pointer. This is meant to
 * protect against renderers fighting over linux_dmabuf_buffer user data
 * ownership.
 *
 * The renderer-private data is usually set from the
 * weston_renderer::import_dmabuf hook.
 *
 * \param buffer The linux_dmabuf_buffer object to set for.
 * \param data The new renderer-private data pointer.
 * \param func Destructor function to be called for the renderer-private
 *             data when the linux_dmabuf_buffer gets destroyed.
 *
 * \sa weston_compositor_import_dmabuf
 */
void
linux_dmabuf_buffer_set_user_data(struct linux_dmabuf_buffer *buffer,
				  void *data,
				  dmabuf_user_data_destroy_func func)
{
	assert(data == NULL || buffer->user_data == NULL);

	buffer->user_data = data;
	buffer->user_data_destroy_func = func;
}

/** Get renderer-private data
 *
 * Get the user data from the linux_dmabuf_buffer.
 *
 * \param buffer The linux_dmabuf_buffer to query.
 * \return Renderer-private data pointer.
 *
 * \sa linux_dmabuf_buffer_set_user_data
 */
void *
linux_dmabuf_buffer_get_user_data(struct linux_dmabuf_buffer *buffer)
{
	return buffer->user_data;
}

static const struct zwp_linux_dmabuf_v1_interface linux_dmabuf_implementation = {
	linux_dmabuf_destroy,
	linux_dmabuf_create_params
};

/**
 * @brief Bind function for the `zwp_linux_dmabuf_v1` global.
 *
 * This function is called when a client binds to the `zwp_linux_dmabuf_v1`
 * global object. It creates a `zwp_linux_dmabuf_v1` resource for the client
 * and, if the protocol version supports it, sends the supported format/modifier
 * combinations to the client.
 *
 * @param client The client binding to the global.
 * @param data The compositor-specific data provided when the global was created
 *             (see `linux_dmabuf_setup`).
 * @param version The protocol version requested by the client.
 * @param id The ID for the new `zwp_linux_dmabuf_v1` resource.
 */
static void
bind_linux_dmabuf(struct wl_client *client,
		  void *data, uint32_t version, uint32_t id)
{
	void *compositor = data;
	struct wl_resource *resource;
	int *formats = NULL;
	uint64_t *modifiers = NULL;
	int num_formats, num_modifiers;
	uint64_t modifier_invalid = DRM_FORMAT_MOD_INVALID;
	int i, j;

	resource = wl_resource_create(client, &zwp_linux_dmabuf_v1_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &linux_dmabuf_implementation,
				       compositor, NULL);

	if (version < ZWP_LINUX_DMABUF_V1_MODIFIER_SINCE_VERSION)
		return;
	/*
	 * Use EGL_EXT_image_dma_buf_import_modifiers to query and advertise
	 * format/modifier codes.
	 */
	comp_dmabuf_formats_query(compositor, &formats,
						   &num_formats);

	for (i = 0; i < num_formats; i++) {
		comp_dmabuf_modifiers_query(compositor,
							     formats[i],
							     &modifiers,
							     &num_modifiers);

		/* send DRM_FORMAT_MOD_INVALID token when no modifiers are supported
		 * for this format */
		if (num_modifiers == 0) {
			num_modifiers = 1;
			modifiers = &modifier_invalid;
		}
		for (j = 0; j < num_modifiers; j++) {
			uint32_t modifier_lo = modifiers[j] & 0xFFFFFFFF;
			uint32_t modifier_hi = modifiers[j] >> 32;
			zwp_linux_dmabuf_v1_send_modifier(resource, formats[i],
							  modifier_hi,
							  modifier_lo);
		}
		if (modifiers != &modifier_invalid)
			free(modifiers);
	}
	free(formats);
}

/** Advertise linux_dmabuf support
 *
 * Calling this initializes the zwp_linux_dmabuf protocol support, so that
 * the interface will be advertised to clients. Essentially it creates a
 * global. Do not call this function multiple times in the compositor's
 * lifetime. There is no way to deinit explicitly, globals will be reaped
 * when the wl_display gets destroyed.
 *
 * \param compositor The compositor to init for.
 * \return Zero on success, -1 on failure.
 */
int
linux_dmabuf_setup(struct wl_display *display, void *comp)
{
	if (!wl_global_create(display,
			      &zwp_linux_dmabuf_v1_interface, 3,
			      comp, bind_linux_dmabuf))
		return -1;

	return 0;
}

/** Resolve an internal compositor error by disconnecting the client.
 *
 * This function is used in cases when the dmabuf-based wl_buffer
 * turns out unusable and there is no fallback path. This is used by
 * renderers which are the fallback path in the first place.
 *
 * It is possible the fault is caused by a compositor bug, the underlying
 * graphics stack bug or normal behaviour, or perhaps a client mistake.
 * In any case, the options are to either composite garbage or nothing,
 * or disconnect the client. This is a helper function for the latter.
 *
 * The error is sent as an INVALID_OBJECT error on the client's wl_display.
 *
 * \param buffer The linux_dmabuf_buffer that is unusable.
 * \param msg A custom error message attached to the protocol error.
 */
void
linux_dmabuf_buffer_send_server_error(struct linux_dmabuf_buffer *buffer,
				      const char *msg)
{
	struct wl_client *client;
	struct wl_resource *display_resource;
	uint32_t id;

	assert(buffer->buffer_resource);
	id = wl_resource_get_id(buffer->buffer_resource);
	client = wl_resource_get_client(buffer->buffer_resource);
	display_resource = wl_client_get_object(client, 1);

	assert(display_resource);
	wl_resource_post_error(display_resource,
			       WL_DISPLAY_ERROR_INVALID_OBJECT,
			       "linux_dmabuf server error with "
			       "wl_buffer@%u: %s", id, msg);
}
