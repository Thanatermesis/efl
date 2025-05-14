/*
 * Copyright © 2012-2013 Collabora, Ltd.
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

extern const struct wl_interface wl_subsurface_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface wl_surface_interface;

/**
 * @brief Array of Wayland interface pointers.
 *
 * This array is used to specify the types of arguments in Wayland messages.
 * Each element corresponds to a Wayland interface type.
 * For example:
 * - types[2] is &wl_subsurface_interface
 * - types[3] is &wl_surface_interface (parent surface for get_subsurface)
 * - types[4] is &wl_surface_interface (unused in this specific file, but reserved)
 * - types[5] is &wl_surface_interface (sibling surface for place_above)
 * - types[6] is &wl_surface_interface (sibling surface for place_below)
 * NULL entries are typically used for arguments that are not objects (e.g., integers, strings)
 * or for requests/events that don't have arguments of a specific interface type at that position.
 */
static const struct wl_interface *types[] = {
        NULL, // wl_subcompositor.destroy, wl_subsurface.destroy, wl_subsurface.set_position, wl_subsurface.set_sync, wl_subsurface.set_desync
        NULL, // wl_subcompositor.get_subsurface (id of new wl_subsurface)
        &wl_subsurface_interface,
        &wl_surface_interface,
        &wl_surface_interface,
        &wl_surface_interface,
        &wl_surface_interface, // wl_subsurface.place_below (sibling surface)
};

/**
 * @brief Defines the requests for the wl_subcompositor interface.
 *
 * These are the messages a client can send to a wl_subcompositor object.
 * Each wl_message struct contains:
 * - name: The name of the request (e.g., "destroy", "get_subsurface").
 * - signature: A string describing the argument types.
 *   - 'n': new_id (object to be created)
 *   - 'o': object (existing Wayland object)
 *   - 'i': int32_t
 *   - 'u': uint32_t
 *   - 's': string
 *   - 'a': array
 *   - 'h': fd (file descriptor)
 * - types: A pointer to an array within the `types` global array, indicating the
 *          Wayland interface types for object arguments.
 *
 * Example:
 * - { "destroy", "", types + 0 }: The "destroy" request takes no arguments.
 *   `types + 0` points to {NULL, NULL, ...}
 * - { "get_subsurface", "noo", types + 2 }: The "get_subsurface" request.
 *   - 'n': id of the new wl_subsurface (types[2] = &wl_subsurface_interface)
 *   - 'o': parent wl_surface (types[3] = &wl_surface_interface)
 *   - 'o': wl_surface to be turned into a subsurface (types[4] = &wl_surface_interface)
 *   `types + 2` points to {&wl_subsurface_interface, &wl_surface_interface, &wl_surface_interface, ...}
 */
static const struct wl_message wl_subcompositor_requests[] = {
        { "destroy", "", types + 0 },
        { "get_subsurface", "noo", types + 2 }, // new_id<wl_subsurface>, wl_surface (parent), wl_surface (surface to be subsurfaced)
};

/**
 * @brief The wl_subcompositor interface.
 *
 * This interface allows clients to create wl_subsurface objects.
 * A wl_subcompositor is a global object.
 * - name: "wl_subcompositor" - The official name of the interface.
 * - version: 1 - The version of this interface definition.
 * - method_count: 2 - The number of requests (methods) this interface has.
 * - methods: wl_subcompositor_requests - Pointer to the array of request definitions.
 * - event_count: 0 - The number of events this interface can send.
 * - events: NULL - Pointer to the array of event definitions (none for this interface).
 */
WL_EXPORT const struct wl_interface wl_subcompositor_interface = {
        "wl_subcompositor", 1,
        2, wl_subcompositor_requests,
        0, NULL,
};

/**
 * @brief Defines the requests for the wl_subsurface interface.
 *
 * These are the messages a client can send to a wl_subsurface object.
 *
 * Example:
 * - { "destroy", "", types + 0 }: The "destroy" request.
 * - { "set_position", "ii", types + 0 }: The "set_position" request.
 *   - 'i': x coordinate (int32_t)
 *   - 'i': y coordinate (int32_t)
 *   `types + 0` indicates no Wayland object arguments.
 * - { "place_above", "o", types + 5 }: The "place_above" request.
 *   - 'o': sibling wl_surface (types[5] = &wl_surface_interface)
 *   `types + 5` points to {&wl_surface_interface, ...}
 * - { "place_below", "o", types + 6 }: The "place_below" request.
 *   - 'o': sibling wl_surface (types[6] = &wl_surface_interface)
 *   `types + 6` points to {&wl_surface_interface, ...}
 */
static const struct wl_message wl_subsurface_requests[] = {
        { "destroy", "", types + 0 },
        { "set_position", "ii", types + 0 }, // x, y
        { "place_above", "o", types + 5 }, // wl_surface (sibling)
        { "place_below", "o", types + 6 }, // wl_surface (sibling)
        { "set_sync", "", types + 0 },
        { "set_desync", "", types + 0 },
};

/**
 * @brief The wl_subsurface interface.
 *
 * A wl_subsurface allows a wl_surface to be displayed as part of a hierarchy
 * of surfaces, positioned relative to a parent wl_surface.
 * - name: "wl_subsurface" - The official name of the interface.
 * - version: 1 - The version of this interface definition.
 * - method_count: 6 - The number of requests (methods) this interface has.
 * - methods: wl_subsurface_requests - Pointer to the array of request definitions.
 * - event_count: 0 - The number of events this interface can send.
 * - events: NULL - Pointer to the array of event definitions (none for this interface).
 */
WL_EXPORT const struct wl_interface wl_subsurface_interface = {
        "wl_subsurface", 1,
        6, wl_subsurface_requests,
        0, NULL,
};
