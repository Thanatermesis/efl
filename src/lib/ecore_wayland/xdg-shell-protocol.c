/*
 * Copyright © 2008-2013 Kristian Høgsberg
 * Copyright © 2013      Rafael Antognolli
 * Copyright © 2013      Jasper St. Pierre
 * Copyright © 2010-2013 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this
 * software and its documentation for any purpose is hereby granted
 * without fee, provided that the above copyright notice appear in
 * all copies and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of
 * the copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 */

#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_seat_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface xdg_popup_interface;
extern const struct wl_interface xdg_surface_interface;

/**
 * @brief Array of Wayland interface pointers.
 *
 * This array is used to define the types of arguments for Wayland messages.
 * Each element corresponds to a Wayland interface or is NULL if the argument
 * is a basic type (e.g., int, uint, string). The indices of this array are
 * referenced in the `types` field of `wl_message` structures.
 *
 * For example, `types + 4` points to `&xdg_surface_interface`.
 */
static const struct wl_interface *types[] = {
	NULL, /* No interface type, e.g., for basic types like int, uint, string */
	NULL, /* No interface type */
	NULL,
	NULL,
	&xdg_surface_interface,
	&wl_surface_interface,
	&xdg_popup_interface,
	&wl_surface_interface,
	&wl_surface_interface,
	&wl_seat_interface,
	NULL,
	NULL,
	NULL,
	&xdg_surface_interface,
	&wl_seat_interface,
	NULL,
	NULL,
	NULL,
	&wl_seat_interface,
	NULL,
	&wl_seat_interface,
	NULL,
	NULL,
	&wl_output_interface, /* wl_output interface */
};

/**
 * @brief Requests for the xdg_shell interface.
 *
 * These are the messages a client can send to the compositor
 * via the xdg_shell interface.
 */
static const struct wl_message xdg_shell_requests[] = {
	/**
	 * @brief Destroy the xdg_shell object.
	 *
	 * Destroys the xdg_shell object. This request should be made when the
	 * client no longer needs the xdg_shell functionality.
	 *
	 * Signature: "" (no arguments)
	 * Types: `types + 0` (NULL)
	 */
	{ "destroy", "", types + 0 },
	/**
	 * @brief Use an unstable version of the protocol.
	 *
	 * This request allows a client to use a specific unstable version
	 * of the xdg-shell protocol.
	 *
	 * Signature: "i" (int version)
	 * Types: `types + 0` (NULL) - version is a basic type
	 */
	{ "use_unstable_version", "i", types + 0 },
	/**
	 * @brief Create a new xdg_surface.
	 *
	 * Creates an xdg_surface object for a given wl_surface.
	 *
	 * Signature: "no" (new_id xdg_surface, object wl_surface)
	 * Types: `types + 4` (xdg_surface_interface, wl_surface_interface)
	 */
	{ "get_xdg_surface", "no", types + 4 },
	/**
	 * @brief Create a new xdg_popup.
	 *
	 * Creates an xdg_popup object.
	 *
	 * Signature: "nooouii" (new_id xdg_popup, object wl_surface parent_surface, object wl_surface grab_surface, object wl_seat seat, uint32_t serial, int32_t x, int32_t y)
	 * Types: `types + 6` (xdg_popup_interface, wl_surface_interface, wl_surface_interface, wl_seat_interface, NULL, NULL, NULL)
	 */
	{ "get_xdg_popup", "nooouii", types + 6 },
	/**
	 * @brief Respond to a ping event.
	 *
	 * A client must respond to a ping event with a pong request.
	 *
	 * Signature: "u" (uint32_t serial)
	 * Types: `types + 0` (NULL) - serial is a basic type
	 */
	{ "pong", "u", types + 0 },
};

/**
 * @brief Events for the xdg_shell interface.
 *
 * These are the messages the compositor can send to a client
 * via the xdg_shell interface.
 */
static const struct wl_message xdg_shell_events[] = {
	/**
	 * @brief Ping the client.
	 *
	 * The compositor can send this event to check if the client is responsive.
	 * The client must respond with a "pong" request.
	 *
	 * Signature: "u" (uint32_t serial)
	 * Types: `types + 0` (NULL) - serial is a basic type
	 */
	{ "ping", "u", types + 0 },
};

/**
 * @brief Definition of the xdg_shell Wayland interface.
 *
 * This interface is the entry point for creating surfaces managed by the
 * xdg-shell protocol. It allows clients to create xdg_surface and xdg_popup
 * objects.
 */
WL_EXPORT const struct wl_interface xdg_shell_interface = {
	"xdg_shell", 1, /* Interface name and version */
	5, xdg_shell_requests, /* Number of requests and the requests array */
	1, xdg_shell_events, /* Number of events and the events array */
};

/**
 * @brief Requests for the xdg_surface interface.
 *
 * These are the messages a client can send to the compositor
 * via an xdg_surface object.
 */
static const struct wl_message xdg_surface_requests[] = {
	/**
	 * @brief Destroy the xdg_surface object.
	 *
	 * Destroys the xdg_surface object.
	 *
	 * Signature: "" (no arguments)
	 * Types: `types + 0` (NULL)
	 */
	{ "destroy", "", types + 0 },
	/**
	 * @brief Set the parent of this surface.
	 *
	 * Sets the parent surface for this xdg_surface. This is used for
	 * creating transient surfaces (e.g., dialogs).
	 *
	 * Signature: "?o" (object xdg_surface parent_surface or NULL)
	 * Types: `types + 13` (xdg_surface_interface)
	 */
	{ "set_parent", "?o", types + 13 },
	/**
	 * @brief Set the title of the window.
	 *
	 * Sets the title for the xdg_surface.
	 *
	 * Signature: "s" (string title)
	 * Types: `types + 0` (NULL) - title is a basic type
	 */
	{ "set_title", "s", types + 0 },
	/**
	 * @brief Set the application ID.
	 *
	 * Sets the application ID for the xdg_surface. This is typically
	 * a desktop file ID.
	 *
	 * Signature: "s" (string app_id)
	 * Types: `types + 0` (NULL) - app_id is a basic type
	 */
	{ "set_app_id", "s", types + 0 },
	/**
	 * @brief Show the window menu.
	 *
	 * Requests the compositor to show the window menu.
	 *
	 * Signature: "ouii" (object wl_seat, uint32_t serial, int32_t x, int32_t y)
	 * Types: `types + 14` (wl_seat_interface, NULL, NULL, NULL)
	 */
	{ "show_window_menu", "ouii", types + 14 },
	/**
	 * @brief Start an interactive move.
	 *
	 * Requests the compositor to start an interactive move operation for the window.
	 *
	 * Signature: "ou" (object wl_seat, uint32_t serial)
	 * Types: `types + 18` (wl_seat_interface, NULL)
	 */
	{ "move", "ou", types + 18 },
	/**
	 * @brief Start an interactive resize.
	 *
	 * Requests the compositor to start an interactive resize operation for the window.
	 *
	 * Signature: "ouu" (object wl_seat, uint32_t serial, uint32_t edges)
	 * Types: `types + 20` (wl_seat_interface, NULL, NULL)
	 */
	{ "resize", "ouu", types + 20 },
	/**
	 * @brief Acknowledge a configure event.
	 *
	 * Client acknowledges a configure event.
	 *
	 * Signature: "u" (uint32_t serial)
	 * Types: `types + 0` (NULL) - serial is a basic type
	 */
	{ "ack_configure", "u", types + 0 },
	/**
	 * @brief Set the window geometry.
	 *
	 * Sets the geometry of the window.
	 *
	 * Signature: "iiii" (int32_t x, int32_t y, int32_t width, int32_t height)
	 * Types: `types + 0` (NULL) - geometry components are basic types
	 */
	{ "set_window_geometry", "iiii", types + 0 },
	/**
	 * @brief Request to maximize the window.
	 *
	 * Signature: "" (no arguments)
	 * Types: `types + 0` (NULL)
	 */
	{ "set_maximized", "", types + 0 },
	/**
	 * @brief Request to unmaximize the window.
	 *
	 * Signature: "" (no arguments)
	 * Types: `types + 0` (NULL)
	 */
	{ "unset_maximized", "", types + 0 },
	/**
	 * @brief Request to make the window fullscreen.
	 *
	 * Signature: "?o" (object wl_output or NULL)
	 * Types: `types + 23` (wl_output_interface)
	 */
	{ "set_fullscreen", "?o", types + 23 },
	/**
	 * @brief Request to unset fullscreen state.
	 *
	 * Signature: "" (no arguments)
	 * Types: `types + 0` (NULL)
	 */
	{ "unset_fullscreen", "", types + 0 },
	/**
	 * @brief Request to minimize the window.
	 *
	 * Signature: "" (no arguments)
	 * Types: `types + 0` (NULL)
	 */
	{ "set_minimized", "", types + 0 },
};

/**
 * @brief Events for the xdg_surface interface.
 *
 * These are the messages the compositor can send to a client
 * via an xdg_surface object.
 */
static const struct wl_message xdg_surface_events[] = {
	/**
	 * @brief Suggest a new configuration for the surface.
	 *
	 * The compositor suggests a new size or state for the surface.
	 * The client should respond with an ack_configure request.
	 *
	 * Signature: "iiau" (int32_t width, int32_t height, array states, uint32_t serial)
	 *   - width: new width of the surface, or 0 for no change.
	 *   - height: new height of the surface, or 0 for no change.
	 *   - states: array of xdg_surface_state enum values.
	 *     Example: `[XDG_SURFACE_STATE_MAXIMIZED, XDG_SURFACE_STATE_ACTIVATED]`
	 *   - serial: serial of this configure event.
	 * Types: `types + 0` (NULL) - width, height, states, serial are basic types or handled by marshalling code.
	 */
	{ "configure", "iiau", types + 0 },
	/**
	 * @brief The compositor requests the client to close the surface.
	 *
	 * This usually happens when the user clicks the close button.
	 *
	 * Signature: "" (no arguments)
	 * Types: `types + 0` (NULL)
	 */
	{ "close", "", types + 0 },
};

/**
 * @brief Definition of the xdg_surface Wayland interface.
 *
 * An xdg_surface represents a desktop-style window.
 */
WL_EXPORT const struct wl_interface xdg_surface_interface = {
	"xdg_surface", 1, /* Interface name and version */
	14, xdg_surface_requests, /* Number of requests and the requests array */
	2, xdg_surface_events, /* Number of events and the events array */
};

/**
 * @brief Requests for the xdg_popup interface.
 *
 * These are the messages a client can send to the compositor
 * via an xdg_popup object.
 */
static const struct wl_message xdg_popup_requests[] = {
	/**
	 * @brief Destroy the xdg_popup object.
	 *
	 * Destroys the xdg_popup object.
	 *
	 * Signature: "" (no arguments)
	 * Types: `types + 0` (NULL)
	 */
	{ "destroy", "", types + 0 },
};

/**
 * @brief Events for the xdg_popup interface.
 *
 * These are the messages the compositor can send to a client
 * via an xdg_popup object.
 */
static const struct wl_message xdg_popup_events[] = {
	/**
	 * @brief The popup has been dismissed.
	 *
	 * This event is sent when the popup is dismissed by the compositor
	 * (e.g., user clicks outside the popup).
	 *
	 * Signature: "" (no arguments)
	 * Types: `types + 0` (NULL)
	 */
	{ "popup_done", "", types + 0 },
};

/**
 * @brief Definition of the xdg_popup Wayland interface.
 *
 * An xdg_popup represents a short-lived, transient surface, like a menu or a tooltip.
 */
WL_EXPORT const struct wl_interface xdg_popup_interface = {
	"xdg_popup", 1, /* Interface name and version */
	1, xdg_popup_requests, /* Number of requests and the requests array */
	1, xdg_popup_events, /* Number of events and the events array */
};

