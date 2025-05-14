#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

/**
 * @page page_session_recovery_protocol session_recovery Protocol
 * @section page_ifaces_session_recovery Interfaces
 * - @subpage page_iface_zwp_e_session_recovery - E_session_recovery_interface
 * @section page_copyright_session_recovery Copyright
 * <pre>
 *
 * Copyright © 2019 Christopher Billington
 * Copyright © 2020 Ilia Bozhinov
 * Copyright © 2022 Victoria Brekenfeld
 * Copyright © 2022 Simon Ser
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * </pre>
 */

/**
 * @file
 * @brief Wayland protocol for session recovery.
 *
 * This file defines the C-language interface for the E session recovery
 * Wayland protocol. It allows a Wayland compositor to provide a persistent
 * UUID to clients, which can be used to restore a session after a compositor
 * crash or restart.
 */

/** @brief Array of Wayland interface pointers.
 *
 * This array is used by the Wayland library to describe the types of
 * arguments in messages. In this specific protocol, it appears to be
 * unused as all arguments are basic types (string).
 */
static const struct wl_interface *types[] = {
	NULL,
};

/** @brief Defines the requests for the zwp_e_session_recovery interface.
 *
 * This array describes the messages that a client can send to the compositor
 * via the `zwp_e_session_recovery` interface.
 */
static const struct wl_message zwp_e_session_recovery_requests[] = {
	/**
	 * @brief Provide a UUID for session recovery.
	 * @param uuid The UUID string.
	 *
	 * The client provides a UUID to the compositor. This UUID should be
	 * persistent across compositor restarts. The compositor can use this
	 * UUID to identify the client and restore its state.
	 *
	 * Example: "a1b2c3d4-e5f6-7890-1234-567890abcdef"
	 */
	{ "provide_uuid", "s", types + 0 },
};

/** @brief Defines the events for the zwp_e_session_recovery interface.
 *
 * This array describes the messages that the compositor can send to a client
 * via the `zwp_e_session_recovery` interface.
 */
static const struct wl_message zwp_e_session_recovery_events[] = {
	/**
	 * @brief The compositor provides a UUID to the client.
	 * @param uuid The UUID string.
	 *
	 * The compositor sends this event to the client, providing a UUID
	 * that the client should store. This UUID can be used by the client
	 * in a subsequent session to request restoration of its previous state
	 * by sending the `provide_uuid` request.
	 *
	 * Example: "fedcba09-8765-4321-0fed-cba987654321"
	 */
	{ "uuid", "s", types + 0 },
};

/**
 * @ingroup iface_zwp_e_session_recovery
 * @brief E_session_recovery_interface definition.
 *
 * This interface allows a client to provide a UUID to the compositor,
 * which can be used to recover a session after a compositor crash or restart.
 * The compositor can also provide a UUID to the client.
 *
 * This is particularly useful for applications that need to maintain state
 * across compositor sessions, such as terminal emulators or text editors.
 */
WL_EXPORT const struct wl_interface zwp_e_session_recovery_interface = {
	"zwp_e_session_recovery", 1,
	1, zwp_e_session_recovery_requests,
	1, zwp_e_session_recovery_events,
};

