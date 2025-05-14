/*
 * Copyright © 2008 Kristian Høgsberg
 * Copyright © 2012-2013 Collabora, Ltd.
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
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "ecore_wl2_private.h"

/**
 * @brief Structure to hold data for a DND task.
 * @internal
 */
struct _dnd_task
{
   void *data; /**< User data for the callback */
   Ecore_Fd_Cb cb; /**< File descriptor callback function */
};

/**
 * @brief Structure for DND read context.
 * @internal
 */
struct _dnd_read_ctx
{
   int epoll_fd; /**< Epoll file descriptor */
   struct epoll_event *ep; /**< Pointer to epoll event structure */
};

/**
 * @brief Represents a Wayland data offer.
 * @internal
 *
 * This structure holds information about a data offer, including associated
 * input, the Wayland data offer object, mimetypes, actions, and other
 * relevant data for drag-and-drop or selection operations.
 */
struct _Ecore_Wl2_Offer
{
   Ecore_Wl2_Input *input; /**< The input associated with this offer */
   struct wl_data_offer *offer; /**< The Wayland data offer object */
   Eina_Array *mimetypes; /**< Array of offered MIME types (char *) */
   Ecore_Wl2_Drag_Action actions; /**< Supported DND actions by the source */
   Ecore_Wl2_Drag_Action action; /**< Current DND action */
   uint32_t serial; /**< Serial for events related to this offer */
   Eina_List *reads; /**< List of active read operations (Ecore_Fd_Handler *) */
   int ref; /**< Reference count for the offer object */
   Ecore_Wl2_Window *window; /**< The window associated with this offer during DND */
   Eina_Bool proxied : 1; /**< Flag indicating if the offer is being proxied */
};

/**
 * @internal
 * @brief Retrieves the Ecore_Wl2_Window associated with the current input focus.
 *
 * This function attempts to find the window that currently has pointer focus,
 * previously had pointer focus, or has keyboard focus, in that order.
 *
 * @param input The Ecore_Wl2_Input context.
 * @return The Ecore_Wl2_Window with focus, or NULL if none.
 */
Ecore_Wl2_Window *
_win_id_get(Ecore_Wl2_Input *input)
{
   Ecore_Wl2_Window *win = NULL;

   if (input->focus.pointer)
     win = input->focus.pointer;
   else if (input->focus.prev_pointer)
     win = input->focus.prev_pointer;
   else if (input->focus.keyboard)
     win = input->focus.keyboard;

   return win;
}

/**
 * @internal
 * @brief Frees an Ecore_Wl2_Event_Data_Source_Target event.
 *
 * @param data Unused user data.
 * @param event The event structure to free.
 */
static void
data_source_target_free(void *data EINA_UNUSED, void *event)
{
   Ecore_Wl2_Event_Data_Source_Target *ev;

   ev = event;
   if (!ev) return;

   free(ev->type);
   ecore_wl2_display_disconnect(ev->display);
   free(ev);
}

/**
 * @internal
 * @brief Handles the 'target' event from a wl_data_source.
 *
 * This is called when a target application accepts an offered MIME type.
 * An ECORE_WL2_EVENT_DATA_SOURCE_TARGET event is emitted.
 *
 * @param data The Ecore_Wl2_Input associated with the data source.
 * @param source The wl_data_source object (unused).
 * @param mime_type The MIME type accepted by the target.
 */
static void
data_source_target(void *data, struct wl_data_source *source EINA_UNUSED, const char *mime_type)
{
   Ecore_Wl2_Input *input;
   Ecore_Wl2_Event_Data_Source_Target *ev;

   input = data;
   if (!input) return;

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Data_Source_Target));
   if (!ev) return;
   ev->seat = input->id;
   ev->display = input->display;
   ev->display->refs++;

   if (mime_type) ev->type = strdup(mime_type);

   ecore_event_add(ECORE_WL2_EVENT_DATA_SOURCE_TARGET, ev,
                   data_source_target_free, NULL);
}

static void
data_source_send_free(void *data EINA_UNUSED, void *event)
{
   Ecore_Wl2_Event_Data_Source_Send *ev;

   ev = event;
   if (!ev) return;

   free(ev->type);
   ecore_wl2_display_disconnect(ev->display);
   free(ev);
}

/**
 * @internal
 * @brief Handles the 'send' event from a wl_data_source.
 *
 * This is called when the source should send the data for an offered
 * MIME type to the target. An ECORE_WL2_EVENT_DATA_SOURCE_SEND event is emitted.
 *
 * @param data The Ecore_Wl2_Input associated with the data source.
 * @param source The wl_data_source object.
 * @param mime_type The MIME type for which data should be sent.
 * @param fd The file descriptor to write the data to.
 */
static void
data_source_send(void *data, struct wl_data_source *source, const char *mime_type, int32_t fd)
{
   Ecore_Wl2_Input *input;
   Ecore_Wl2_Event_Data_Source_Send *ev;

   input = data;
   if (!input) return;

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Data_Source_Send));
   if (!ev) return;

   ev->fd = fd;
   ev->type = strdup(mime_type);
   ev->seat = input->id;
   if (source == input->data.selection.source)
     ev->serial = input->data.selection.serial;
   else
     ev->serial = input->data.drag.serial;
   ev->display = input->display;
   ev->display->refs++;

   ecore_event_add(ECORE_WL2_EVENT_DATA_SOURCE_SEND, ev,
                   data_source_send_free, NULL);
}

/**
 * @internal
 * @brief Fills common fields in a data source event structure.
 *
 * @param ev The event structure to fill.
 * @param input The Ecore_Wl2_Input context.
 */
static void
event_fill(struct _Ecore_Wl2_Event_Data_Source_Event *ev, Ecore_Wl2_Input *input)
{
   if (input->focus.keyboard)
     ev->source = input->focus.keyboard;

   ev->win = _win_id_get(input);
   ev->action = input->data.drag.action;
   ev->seat = input->id;
   ev->serial = input->data.drag.serial;
   ev->display = input->display;
   ev->display->refs++;
}

static void
data_source_event_emit(Ecore_Wl2_Input *input, int event, Eina_Bool cancel)
{
   struct _Ecore_Wl2_Event_Data_Source_Event *ev;
   Ecore_Wl2_Event_Data_Source_End *ev2 = NULL;

   if (event == ECORE_WL2_EVENT_DATA_SOURCE_END)
     {
        ev2 = calloc(1, sizeof(Ecore_Wl2_Event_Data_Source_End));
        ev = (void*)ev2;
     }
   else
     ev = calloc(1, sizeof(struct _Ecore_Wl2_Event_Data_Source_Event));
   EINA_SAFETY_ON_NULL_RETURN(ev);

   event_fill((void*)ev, input);
   if (event == ECORE_WL2_EVENT_DATA_SOURCE_END)
     ev2->cancelled = cancel;

   ecore_event_add(event, ev, _display_event_free, ev->display);
}

/**
 * @internal
 * @brief Handles the 'cancelled' event from a wl_data_source.
 *
 * This is called when the drag-and-drop operation is cancelled.
 * The data source is destroyed, and an ECORE_WL2_EVENT_DATA_SOURCE_END
 * event is emitted with the 'cancelled' flag set to true.
 *
 * @param data The Ecore_Wl2_Input associated with the data source.
 * @param source The wl_data_source object that was cancelled.
 */
static void
data_source_cancelled(void *data, struct wl_data_source *source)
{
   Ecore_Wl2_Input *input = data;

   if (input->data.drag.source == source) input->data.drag.source = NULL;
   if (input->data.selection.source == source) input->data.selection.source = NULL;
   input->data.drag.action = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
   wl_data_source_destroy(source);
   data_source_event_emit(input, ECORE_WL2_EVENT_DATA_SOURCE_END, 1);
}

/**
 * @internal
 * @brief Handles the 'dnd_drop_performed' event from a wl_data_source.
 *
 * This is called when the drop has been performed by the client.
 * An ECORE_WL2_EVENT_DATA_SOURCE_DROP event is emitted.
 *
 * @param data The Ecore_Wl2_Input associated with the data source.
 * @param source The wl_data_source object (unused).
 */
static void
data_source_dnd_drop_performed(void *data, struct wl_data_source *source EINA_UNUSED)
{
   Ecore_Wl2_Input *input = data;
   data_source_event_emit(input, ECORE_WL2_EVENT_DATA_SOURCE_DROP, 0);
}

/**
 * @internal
 * @brief Handles the 'dnd_finished' event from a wl_data_source.
 *
 * This is called when the drag-and-drop operation is finished.
 * The data source is destroyed, and an ECORE_WL2_EVENT_DATA_SOURCE_END
 * event is emitted with the 'cancelled' flag set to false.
 *
 * @param data The Ecore_Wl2_Input associated with the data source.
 * @param source The wl_data_source object that finished.
 */
static void
data_source_dnd_finished(void *data, struct wl_data_source *source)
{
   Ecore_Wl2_Input *input = data;

   if (input->data.drag.source == source) input->data.drag.source = NULL;
   if (input->data.selection.source == source) input->data.selection.source = NULL;
   wl_data_source_destroy(source);
   data_source_event_emit(input, ECORE_WL2_EVENT_DATA_SOURCE_END, 0);
}

/**
 * @internal
 * @brief Handles the 'action' event from a wl_data_source.
 *
 * This is called when the drag-and-drop action changes.
 * An ECORE_WL2_EVENT_DATA_SOURCE_ACTION event is emitted.
 *
 * @param data The Ecore_Wl2_Input associated with the data source.
 * @param source The wl_data_source object (unused).
 * @param dnd_action The new DND action chosen by the compositor.
 */
static void
data_source_action(void *data, struct wl_data_source *source EINA_UNUSED, uint32_t dnd_action)
{
   Ecore_Wl2_Input *input = data;

   input->data.drag.action = dnd_action;
   data_source_event_emit(input, ECORE_WL2_EVENT_DATA_SOURCE_ACTION, 0);
}

/**
 * @internal
 * @brief Listener for wl_data_source events.
 */
static const struct wl_data_source_listener _source_listener =
{
   data_source_target,
   data_source_send,
   data_source_cancelled,
   data_source_dnd_drop_performed,
   data_source_dnd_finished,
   data_source_action,
};

/**
 * @internal
 * @brief Callback to unset the serial of a data offer after an event.
 *
 * This is typically used as a free function for events like DND enter/motion
 * to clear the serial on the offer, indicating it's no longer valid for
 * accepting that specific event instance.
 *
 * @param user_data The Ecore_Wl2_Offer associated with the event.
 * @param event The Ecore_Wl2_Event_Dnd_Enter or similar event.
 */
static void
_unset_serial(void *user_data, void *event)
{
   Ecore_Wl2_Offer *offer = user_data;
   Ecore_Wl2_Event_Dnd_Enter *ev = event;

   if (offer)
     offer->serial = 0;
   ecore_wl2_display_disconnect(ev->display);
   free(event);
}

/**
 * @internal
 * @brief Handles a DND enter event.
 *
 * This function is called when the pointer enters a surface during a
 * drag-and-drop operation. It associates the data offer with the input,
 * sets the serial, and emits an ECORE_WL2_EVENT_DND_ENTER event.
 *
 * @param input The Ecore_Wl2_Input context.
 * @param offer The wl_data_offer for the DND operation.
 * @param surface The wl_surface that was entered.
 * @param x The x-coordinate of the pointer relative to the surface.
 * @param y The y-coordinate of the pointer relative to the surface.
 * @param serial The serial for this event.
 */
void
_ecore_wl2_dnd_enter(Ecore_Wl2_Input *input, struct wl_data_offer *offer, struct wl_surface *surface, int x, int y, uint32_t serial)
{
   Ecore_Wl2_Window *window;
   Ecore_Wl2_Event_Dnd_Enter *ev;

   window = _ecore_wl2_display_window_surface_find(input->display, surface);
   if (!window) return;

   if (offer)
     {
        input->drag.offer = wl_data_offer_get_user_data(offer);

        if (input->drag.offer)
          {
             input->drag.offer->serial = serial;
             input->drag.offer->window = window;

             if (input->display->wl.data_device_manager_version >=
                 WL_DATA_OFFER_SET_ACTIONS_SINCE_VERSION)
               ecore_wl2_offer_actions_set(input->drag.offer,
                                           ECORE_WL2_DRAG_ACTION_MOVE |
                                           ECORE_WL2_DRAG_ACTION_COPY,
                                           ECORE_WL2_DRAG_ACTION_MOVE);
          }
     }
   else input->drag.offer = NULL;

   input->drag.enter_serial = serial;
   input->drag.window = window;

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Dnd_Enter));
   if (!ev) return;

   if (input->focus.keyboard)
     ev->source = input->focus.keyboard;
   ev->win = input->drag.window;

   ev->x = x;
   ev->y = y;
   ev->offer = input->drag.offer;
   ev->seat = input->id;
   ev->display = input->display;
   ev->display->refs++;

   ecore_event_add(ECORE_WL2_EVENT_DND_ENTER, ev, _unset_serial, input->drag.offer);
}

static void
_delay_offer_destroy(void *user_data, void *event)
{
   Ecore_Wl2_Offer *offer = user_data;
   Ecore_Wl2_Event_Dnd_Leave *ev = event;

   if (offer)
     _ecore_wl2_offer_unref(offer);
   ecore_wl2_display_disconnect(ev->display);
   free(event);
}

/**
 * @internal
 * @brief Handles a DND leave event.
 *
 * This function is called when the pointer leaves a surface during a
 * drag-and-drop operation. It clears DND-related state from the input
 * and emits an ECORE_WL2_EVENT_DND_LEAVE event. The associated offer
 * is unreferenced after the event is processed.
 *
 * @param input The Ecore_Wl2_Input context.
 */
void
_ecore_wl2_dnd_leave(Ecore_Wl2_Input *input)
{
   Ecore_Wl2_Event_Dnd_Leave *ev;

   EINA_SAFETY_ON_TRUE_RETURN(!input->drag.enter_serial);

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Dnd_Leave));
   if (!ev) return;

   if (input->focus.keyboard)
     ev->source = input->focus.keyboard;

   ev->win = input->drag.window;
   ev->offer = input->drag.offer;
   if (ev->offer)
     ev->offer->ref++;
   ev->seat = input->id;
   ev->display = input->display;
   ev->display->refs++;

   input->drag.window = NULL;
   input->drag.enter_serial = 0;
   input->drag.offer = NULL;
   ecore_event_add(ECORE_WL2_EVENT_DND_LEAVE, ev, _delay_offer_destroy, ev->offer);
}

/**
 * @internal
 * @brief Handles a DND motion event.
 *
 * This function is called when the pointer moves during a drag-and-drop
 * operation. It updates pointer coordinates, sets the serial on the
 * current offer, and emits an ECORE_WL2_EVENT_DND_MOTION event.
 *
 * @param input The Ecore_Wl2_Input context.
 * @param x The new x-coordinate of the pointer.
 * @param y The new y-coordinate of the pointer.
 * @param serial The serial for this event.
 */
void
_ecore_wl2_dnd_motion(Ecore_Wl2_Input *input, int x, int y, uint32_t serial)
{
   Ecore_Wl2_Event_Dnd_Motion *ev;

   input->pointer.sx = x;
   input->pointer.sy = y;

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Dnd_Motion));
   if (!ev) return;

   if (input->drag.offer)
     input->drag.offer->serial = serial;

   if (input->focus.keyboard)
     ev->source = input->focus.keyboard;

   ev->win = input->drag.window;
   ev->x = x;
   ev->y = y;
   ev->offer = input->drag.offer;
   ev->seat = input->id;
   ev->display = input->display;
   ev->display->refs++;

   ecore_event_add(ECORE_WL2_EVENT_DND_MOTION, ev, _unset_serial, input->drag.offer);
}

/**
 * @internal
 * @brief Handles a DND drop event.
 *
 * This function is called when the user performs a drop action.
 * It emits an ECORE_WL2_EVENT_DND_DROP event.
 *
 * @param input The Ecore_Wl2_Input context.
 */
void
_ecore_wl2_dnd_drop(Ecore_Wl2_Input *input)
{
   Ecore_Wl2_Event_Dnd_Drop *ev;

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Dnd_Drop));
   if (!ev) return;

   if (input->focus.keyboard)
     ev->source = input->focus.keyboard;

   ev->win = input->drag.window;
   ev->x = input->pointer.sx;
   ev->y = input->pointer.sy;
   ev->offer = input->drag.offer;
   ev->seat = input->id;
   ev->display = input->display;
   ev->display->refs++;

   ecore_event_add(ECORE_WL2_EVENT_DND_DROP, ev, _display_event_free, ev->display);
}

/**
 * @internal
 * @brief Handles a selection event (new data offer for clipboard).
 *
 * This function is called when the selection changes (e.g., something new
 * is copied to the clipboard). It updates the input's selection offer
 * and emits an ECORE_WL2_EVENT_SEAT_SELECTION event.
 *
 * @param input The Ecore_Wl2_Input context.
 * @param offer The new wl_data_offer for the selection, or NULL if cleared.
 */
void
_ecore_wl2_dnd_selection(Ecore_Wl2_Input *input, struct wl_data_offer *offer)
{
   Ecore_Wl2_Event_Seat_Selection *ev;

   if (input->selection.offer) _ecore_wl2_offer_unref(input->selection.offer);
   input->selection.offer = NULL;

   if (offer)
     input->selection.offer = wl_data_offer_get_user_data(offer);
   input->selection.enter_serial = input->display->serial;
   ev = malloc(sizeof(Ecore_Wl2_Event_Seat_Selection));
   EINA_SAFETY_ON_NULL_RETURN(ev);
   ev->seat = input->id;
   ev->display = input->display;
   ev->display->refs++;
   ecore_event_add(ECORE_WL2_EVENT_SEAT_SELECTION, ev, _display_event_free, ev->display);
}

/**
 * @internal
 * @brief Deletes a DND source structure.
 *
 * Cleans up resources associated with an Ecore_Wl2_Dnd_Source, including
 * closing the file descriptor if open, deleting the fd handler, destroying
 * the Wayland data offer, and freeing allocated memory.
 *
 * @param source The DND source to delete.
 */
void
_ecore_wl2_dnd_del(Ecore_Wl2_Dnd_Source *source)
{
   if (!source) return;
   if (source->fdh)
     {
        int fd;

        fd = ecore_main_fd_handler_fd_get(source->fdh);
        if (fd >= 0)
          close(ecore_main_fd_handler_fd_get(source->fdh));
        ecore_main_fd_handler_del(source->fdh);
     }
   if (source->offer)
     {
        wl_data_offer_destroy(source->offer);
        source->offer = NULL;
     }
   wl_array_release(&source->types);
   free(source);
}

/**
 * @brief Sets the MIME types for a drag operation.
 *
 * This function prepares a new data source for a drag-and-drop operation
 * and offers the specified MIME types.
 *
 * @param input The input device (seat) initiating the drag.
 * @param types A NULL-terminated array of MIME type strings.
 *              Example: `const char *types[] = {"text/plain", "image/png", NULL};`
 */
EAPI void
ecore_wl2_dnd_drag_types_set(Ecore_Wl2_Input *input, const char **types)
{
   struct wl_data_device_manager *manager;
   const char **type;
   char **t;

   EINA_SAFETY_ON_NULL_RETURN(input);
   EINA_SAFETY_ON_NULL_RETURN(input->display);

   manager = input->display->wl.data_device_manager;
   if (!manager) return;

   if (input->data.drag.types.data)
     {
        wl_array_for_each(t, &input->data.drag.types)
          free(*t);
        wl_array_release(&input->data.drag.types);
        wl_array_init(&input->data.drag.types);
     }

   if (input->data.drag.source) wl_data_source_destroy(input->data.drag.source);
   input->data.drag.source = NULL;

   input->data.drag.source = wl_data_device_manager_create_data_source(manager);
   if (!input->data.drag.source)
     {
        ERR("Could not create data source");
        return;
     }

   for (type = types; *type; type++)
     {
        t = wl_array_add(&input->data.drag.types, sizeof(*t));
        if (t)
          {
             *t = strdup(*type);
             wl_data_source_offer(input->data.drag.source, *t);
          }
     }
}

EAPI uint32_t
ecore_wl2_dnd_drag_start(Ecore_Wl2_Input *input, Ecore_Wl2_Window *window, Ecore_Wl2_Window *drag_window)
{
   struct wl_surface *dsurface = NULL, *osurface;

   EINA_SAFETY_ON_NULL_RETURN_VAL(input, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(input->data.drag.source, 0);

   if (drag_window)
     dsurface = ecore_wl2_window_surface_get(drag_window);

   _ecore_wl2_input_ungrab(input);

   wl_data_source_add_listener(input->data.drag.source, &_source_listener, input);

   osurface = ecore_wl2_window_surface_get(window);
   if (osurface)
     {
        if (input->display->wl.data_device_manager_version >= WL_DATA_SOURCE_SET_ACTIONS_SINCE_VERSION)
          wl_data_source_set_actions(input->data.drag.source,
            WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE | WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);

        wl_data_device_start_drag(input->data.device, input->data.drag.source,
                                  osurface, dsurface, input->display->serial);
        input->data.drag.serial = input->display->serial;

        ecore_wl2_input_cursor_from_name_set(input, "move");
     }
   return input->data.drag.serial;
}

EAPI void
ecore_wl2_dnd_set_actions(Ecore_Wl2_Input *input)
{
   EINA_SAFETY_ON_NULL_RETURN(input);
   EINA_SAFETY_ON_NULL_RETURN(input->data.drag.source);
   EINA_SAFETY_ON_NULL_RETURN(input->data.drag.types.data);
   if (input->display->wl.data_device_manager_version >= WL_DATA_SOURCE_SET_ACTIONS_SINCE_VERSION)
     wl_data_source_set_actions(input->data.drag.source,
       WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE | WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
}

/**
 * @brief Ends the current drag-and-drop operation.
 *
 * This function cleans up resources used for the drag operation and emits
 * an ECORE_WL2_EVENT_DND_END event.
 *
 * @param input The input device (seat) that was dragging.
 */
EAPI void
ecore_wl2_dnd_drag_end(Ecore_Wl2_Input *input)
{
   Ecore_Wl2_Event_Dnd_End *ev;

   EINA_SAFETY_ON_NULL_RETURN(input);

   if (input->data.drag.types.data)
     {
        char **t;

        wl_array_for_each(t, &input->data.drag.types)
          free(*t);
        wl_array_release(&input->data.drag.types);
        wl_array_init(&input->data.drag.types);
     }

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Dnd_End));
   if (!ev) return;

   if (input->focus.keyboard)
     ev->source = input->focus.keyboard;


   ev->win = _win_id_get(input);
   ev->seat = input->id;
   ev->display = input->display;
   ev->display->refs++;

   ecore_event_add(ECORE_WL2_EVENT_DND_END, ev, _display_event_free, ev->display);
}

/**
 * @brief Gets the current selection data offer.
 *
 * This function returns the Ecore_Wl2_Offer associated with the current
 * clipboard selection for the given input device.
 *
 * @param input The input device (seat).
 * @return The current selection offer, or NULL if none.
 */
EAPI Ecore_Wl2_Offer*
ecore_wl2_dnd_selection_get(Ecore_Wl2_Input *input)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(input, NULL);

   return input->selection.offer;
}

/**
 * @brief Sets the current selection (clipboard).
 *
 * This function creates a new data source, offers the specified MIME types,
 * and sets it as the current selection for the given input device.
 *
 * @param input The input device (seat).
 * @param types A NULL-terminated array of MIME type strings.
 *              Example: `const char *types[] = {"text/plain;charset=utf-8", NULL};`
 *              If `types[0]` is NULL or an empty string, the selection is effectively cleared.
 * @return The serial of the selection operation, or 0 on failure.
 */
EAPI uint32_t
ecore_wl2_dnd_selection_set(Ecore_Wl2_Input *input, const char **types)
{
   struct wl_data_device_manager *manager;
   const char **type;
   char **t;

   EINA_SAFETY_ON_NULL_RETURN_VAL(input, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(input->display, 0);

   manager = input->display->wl.data_device_manager;
   if (!manager) return 0;

   if (input->data.selection.types.data)
     {
        wl_array_for_each(t, &input->data.selection.types)
          free(*t);
        wl_array_release(&input->data.selection.types);
        wl_array_init(&input->data.selection.types);
     }

   input->data.selection.source = NULL;

   if (!types[0]) return 0;

   input->data.selection.source = wl_data_device_manager_create_data_source(manager);
   if (!input->data.selection.source)
     {
        ERR("Could not create data source");
        return 0;
     }

   for (type = types; *type; type++)
     {
        t = wl_array_add(&input->data.selection.types, sizeof(*t));
        if (t)
          {
             *t = strdup(*type);
             wl_data_source_offer(input->data.selection.source, *t);
          }
     }

   wl_data_source_add_listener(input->data.selection.source, &_source_listener, input);

   wl_data_device_set_selection(input->data.device, input->data.selection.source,
                                input->display->serial);
   input->data.selection.serial = input->display->serial;
   return input->display->serial;
}

/**
 * @brief Clears the current selection (clipboard).
 *
 * This function sets the selection to NULL for the given input device.
 *
 * @param input The input device (seat).
 * @return The serial of the clear operation, or 0 on failure.
 */
EAPI uint32_t
ecore_wl2_dnd_selection_clear(Ecore_Wl2_Input *input)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(input, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(input->data.device, 0);

   wl_data_device_set_selection(input->data.device,
                                NULL, input->display->serial);
   input->data.selection.serial = 0;
   return input->display->serial;
}

/**
 * @internal
 * @brief Converts a Wayland DND action (wl_data_device_manager_dnd_action)
 * to an Ecore_Wl2_Drag_Action.
 *
 * @param action The Wayland DND action.
 * @return The corresponding Ecore_Wl2_Drag_Action.
 */
static Ecore_Wl2_Drag_Action
_wl_to_action_convert(uint32_t action)
{
#define PAIR(wl, ac) if (action == wl) return ac;
   PAIR(WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY, ECORE_WL2_DRAG_ACTION_COPY)
   PAIR(WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK, ECORE_WL2_DRAG_ACTION_ASK)
   PAIR(WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE, ECORE_WL2_DRAG_ACTION_MOVE)
   PAIR(WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE, ECORE_WL2_DRAG_ACTION_NONE)
#undef PAIR
   return ECORE_WL2_DRAG_ACTION_NONE;
}

/**
 * @internal
 * @brief Converts an Ecore_Wl2_Drag_Action to a Wayland DND action
 * (wl_data_device_manager_dnd_action).
 *
 * @param action The Ecore_Wl2_Drag_Action.
 * @return The corresponding Wayland DND action.
 */
static uint32_t
_action_to_wl_convert(Ecore_Wl2_Drag_Action action)
{
#define PAIR(wl, ac) if (action == ac) return wl;
   PAIR(WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY, ECORE_WL2_DRAG_ACTION_COPY)
   PAIR(WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK, ECORE_WL2_DRAG_ACTION_ASK)
   PAIR(WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE, ECORE_WL2_DRAG_ACTION_MOVE)
   PAIR(WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE, ECORE_WL2_DRAG_ACTION_NONE)
#undef PAIR
   return WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
}

/**
 * @internal
 * @brief Handles the 'offer' event from a wl_data_offer.
 *
 * This is called by the compositor to advertise a new MIME type for the offer.
 * The MIME type is added to the offer's list of mimetypes. If `type` is NULL,
 * it indicates the end of MIME type advertisements for now, and existing
 * mimetypes are cleared (though this specific NULL behavior seems unusual and
 * might be a misinterpretation or specific compositor behavior).
 *
 * @param data The Ecore_Wl2_Offer associated with the wl_data_offer.
 * @param wl_data_offer The wl_data_offer object (unused).
 * @param type The advertised MIME type string, or NULL.
 */
static void
data_offer_offer(void *data, struct wl_data_offer *wl_data_offer EINA_UNUSED, const char *type)
{
   Ecore_Wl2_Offer *offer = data;
   char *str;

   if (type)
     eina_array_push(offer->mimetypes, strdup(type)); /*LEEEAK */
   else
     {
        while((str = eina_array_pop(offer->mimetypes)))
          {
             free(str);
          }
     }
}

/**
 * @internal
 * @brief Handles the 'source_actions' event from a wl_data_offer.
 *
 * This is called by the compositor to inform the client of the actions
 * supported by the drag source.
 *
 * @param data The Ecore_Wl2_Offer associated with the wl_data_offer.
 * @param wl_data_offer The wl_data_offer object (unused).
 * @param source_actions A bitmask of wl_data_device_manager_dnd_action values.
 */
static void
data_offer_source_actions(void *data, struct wl_data_offer *wl_data_offer EINA_UNUSED, uint32_t source_actions)
{
   Ecore_Wl2_Offer *offer;
   unsigned int i;
   uint32_t types[] = {WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE,
                       WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY,
                       WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK,
                       WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE};

   offer = data;

   offer->actions = 0;

   for (i = 0; types[i] != 0; ++i)
   {
      if (source_actions & types[i])
        offer->actions |= _wl_to_action_convert(types[i]);
   }
}

/**
 * @internal
 * @brief Handles the 'action' event from a wl_data_offer.
 *
 * This is called by the compositor to inform the client of the DND action
 * chosen by the user/compositor.
 *
 * @param data The Ecore_Wl2_Offer associated with the wl_data_offer.
 * @param wl_data_offer The wl_data_offer object (unused).
 * @param dnd_action The wl_data_device_manager_dnd_action chosen.
 */
static void
data_offer_action(void *data, struct wl_data_offer *wl_data_offer EINA_UNUSED, uint32_t dnd_action)
{
   Ecore_Wl2_Offer *offer;

   offer = data;
   offer->action = _wl_to_action_convert(dnd_action);
}

/**
 * @internal
 * @brief Listener for wl_data_offer events.
 */
static const struct wl_data_offer_listener _offer_listener =
{
   data_offer_offer,
   data_offer_source_actions,
   data_offer_action
};

/**
 * @internal
 * @brief Creates and initializes an Ecore_Wl2_Offer from a wl_data_offer.
 *
 * This function is called when a new data offer is introduced by the
 * compositor (e.g., for DND enter or selection change). It allocates
 * an Ecore_Wl2_Offer, associates it with the wl_data_offer, and sets up
 * listeners.
 *
 * @param input The Ecore_Wl2_Input context.
 * @param offer The wl_data_offer from the compositor.
 */
void
_ecore_wl2_dnd_add(Ecore_Wl2_Input *input, struct wl_data_offer *offer)
{
   Ecore_Wl2_Offer *result;

   result = calloc(1, sizeof(Ecore_Wl2_Offer));
   result->offer = offer;
   result->input = input;
   result->mimetypes = eina_array_new(10);
   result->ref = 1;

   wl_data_offer_add_listener(offer, &_offer_listener, result);
}

/**
 * @brief Gets the source-supported DND actions for an offer.
 *
 * @param offer The data offer.
 * @return A bitmask of Ecore_Wl2_Drag_Action values supported by the source.
 */
EAPI Ecore_Wl2_Drag_Action
ecore_wl2_offer_actions_get(Ecore_Wl2_Offer *offer)
{
   return offer->actions;
}

/**
 * @brief Sets the DND actions for a data offer (client-side).
 *
 * This function is used by the DND target to indicate which actions it
 * supports from the source_actions, and which action it would prefer.
 *
 * @param offer The data offer.
 * @param actions A bitmask of Ecore_Wl2_Drag_Action values the target supports.
 *                Example: `ECORE_WL2_DRAG_ACTION_COPY | ECORE_WL2_DRAG_ACTION_MOVE`
 * @param action The preferred Ecore_Wl2_Drag_Action by the target.
 *               Example: `ECORE_WL2_DRAG_ACTION_COPY`
 */
EAPI void
ecore_wl2_offer_actions_set(Ecore_Wl2_Offer *offer, Ecore_Wl2_Drag_Action actions, Ecore_Wl2_Drag_Action action)
{
   uint32_t val = 0;
   int i = 0;

   EINA_SAFETY_ON_NULL_RETURN(offer);

   for (i = 0; i < ECORE_WL2_DRAG_ACTION_LAST; ++i)
     {
        if (actions & i)
          val |= _action_to_wl_convert(i);
     }

   offer->action = _action_to_wl_convert(action);

   wl_data_offer_set_actions(offer->offer, val, offer->action);
}

/**
 * @brief Gets the current DND action for an offer.
 *
 * This is the action most recently communicated by the compositor via the
 * wl_data_offer.action event.
 *
 * @param offer The data offer.
 * @return The current Ecore_Wl2_Drag_Action.
 */
EAPI Ecore_Wl2_Drag_Action
ecore_wl2_offer_action_get(Ecore_Wl2_Offer *offer)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(offer, ECORE_WL2_DRAG_ACTION_NONE);
   return offer->action;
}

/**
 * @brief Gets the list of MIME types supported by a data offer.
 *
 * The returned array contains `char *` elements, each being a MIME type string.
 * Example: `["text/plain", "image/png"]`
 * The caller should not modify or free the array or its contents.
 *
 * @param offer The data offer.
 * @return An Eina_Array of MIME type strings, or NULL on failure.
 */
EAPI Eina_Array*
ecore_wl2_offer_mimes_get(Ecore_Wl2_Offer *offer)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(offer, NULL);
   return offer->mimetypes;
}

/**
 * @internal
 * @brief Helper function to call wl_data_offer_accept for each MIME type in an array.
 *
 * Used by ecore_wl2_offer_mimes_set.
 *
 * @param container Unused.
 * @param elem A MIME type string (char *).
 * @param data The Ecore_Wl2_Offer.
 * @return 1 to continue iteration.
 */
static unsigned char
_emit_mime(const void *container EINA_UNUSED, void *elem, void *data)
{
   Ecore_Wl2_Offer *offer = data;

   wl_data_offer_accept(offer->offer, offer->serial, elem);

   return 1;
}

/**
 * @brief Accepts multiple MIME types for a data offer.
 *
 * This function iterates through the provided array of MIME types and calls
 * wl_data_offer_accept for each one using the offer's current serial.
 * If `mimes` is NULL, it calls wl_data_offer_accept with a NULL mime_type,
 * typically to reject all types for the current serial.
 *
 * @param offer The data offer.
 * @param mimes An Eina_Array of MIME type strings (char *) to accept.
 *              Example: An array containing "text/plain" and "text/uri-list".
 *              Can be NULL to reject.
 */
EAPI void
ecore_wl2_offer_mimes_set(Ecore_Wl2_Offer *offer, Eina_Array *mimes)
{
   EINA_SAFETY_ON_NULL_RETURN(offer);

   if (mimes)
     eina_array_foreach(mimes, _emit_mime, offer);
   else
     wl_data_offer_accept(offer->offer, offer->serial, NULL);
}

/**
 * @brief Accepts a specific MIME type for a data offer.
 *
 * This function informs the compositor that the client is willing to accept
 * data of the given MIME type for the current drag operation (identified by
 * the offer's serial).
 *
 * @param offer The data offer.
 * @param mime_type The MIME type to accept (e.g., "text/plain"), or NULL
 *                  to indicate that no type is accepted for the current serial.
 */
EAPI void
ecore_wl2_offer_accept(Ecore_Wl2_Offer *offer, const char *mime_type)
{
   EINA_SAFETY_ON_NULL_RETURN(offer);

   wl_data_offer_accept(offer->offer, offer->serial, mime_type);
}

/**
 * @internal
 * @brief Structure to hold data being read from a DND or selection offer.
 */
typedef struct {
   int len; /**< Current length of the data buffer */
   void *data; /**< Buffer holding the received data */
   char *mimetype; /**< The MIME type of the data being read */
   Ecore_Wl2_Offer *offer; /**< The offer this read buffer is associated with */
} Read_Buffer;

/**
 * @internal
 * @brief Frees a Read_Buffer and associated resources after data is ready.
 *
 * This is used as a free function for the ECORE_WL2_EVENT_OFFER_DATA_READY event.
 *
 * @param user_data The Read_Buffer structure.
 * @param event The Ecore_Wl2_Event_Offer_Data_Ready event.
 */
static void
_free_buf(void *user_data, void *event)
{
   Read_Buffer *buf = user_data;
   Ecore_Wl2_Event_Offer_Data_Ready *ev = event;

   _ecore_wl2_offer_unref(buf->offer);

   free(buf->data);
   free(buf->mimetype);
   free(user_data);
   ecore_wl2_display_disconnect(ev->display);
   free(event);
}

/**
 * @internal
 * @brief Callback for reading data from a file descriptor associated with an offer.
 *
 * This function is called when data is available to be read from the fd
 * provided by wl_data_offer_receive. It reads data into a buffer and,
 * when the read is complete (fd closed or error), emits an
 * ECORE_WL2_EVENT_OFFER_DATA_READY event.
 *
 * @param data The Read_Buffer structure.
 * @param fdh The Ecore_Fd_Handler for the file descriptor.
 * @return ECORE_CALLBACK_RENEW to continue monitoring, ECORE_CALLBACK_CANCEL to stop.
 */
static Eina_Bool
_offer_receive_fd_cb(void *data, Ecore_Fd_Handler *fdh)
{
   Read_Buffer *buf = data;
   int fd = -1;
   char buffer[255];
   int len;

   fd = ecore_main_fd_handler_fd_get(fdh);
   if (fd >= 0)
     len = read(fd, buffer, sizeof(buffer));
   else
     return ECORE_CALLBACK_RENEW;

   if (len > 0)
     {
        int old_len = buf->len;

        buf->len += len;
        buf->data = realloc(buf->data, buf->len);

        memcpy(((char*)buf->data) + old_len, buffer, len);
        return ECORE_CALLBACK_RENEW;
     }
   else
     {
        Ecore_Wl2_Event_Offer_Data_Ready *ev;

        ev = calloc(1, sizeof(Ecore_Wl2_Event_Offer_Data_Ready));
        ev->offer = buf->offer;

        ev->data = buf->data;
        ev->len = buf->len;
        ev->mimetype = buf->mimetype;
        ev->seat = buf->offer->input->id;
        ev->display = buf->offer->input->display;
        ev->display->refs++;
        ecore_event_add(ECORE_WL2_EVENT_OFFER_DATA_READY, ev, _free_buf, buf);

        buf->offer->reads = eina_list_remove(buf->offer->reads, fdh);
        return ECORE_CALLBACK_CANCEL;
     }
}

EAPI void
ecore_wl2_offer_receive(Ecore_Wl2_Offer *offer, char *mime)
{
   Read_Buffer *buffer;
   Ecore_Fd_Handler *handler;
   int pipe[2];

   EINA_SAFETY_ON_NULL_RETURN(offer);

   if (pipe2(pipe, O_CLOEXEC) == -1)
     {
        ERR("Failed to create pipe for receiving");
        return;
     }

   buffer = calloc(1, sizeof(Read_Buffer));
   buffer->offer = offer;
   buffer->mimetype = strdup(mime);

   offer->ref ++; // we are keeping this ref until the read is done AND emitted

   wl_data_offer_receive(offer->offer, mime, pipe[1]);
   close(pipe[1]);

   handler =
     ecore_main_fd_handler_file_add(pipe[0], ECORE_FD_READ | ECORE_FD_ERROR,
                                    _offer_receive_fd_cb, buffer, NULL, NULL);

   offer->reads = eina_list_append(offer->reads, handler);
   return;
}

/**
 * @brief Initiates receiving data for an offer by proxying to a given file descriptor.
 *
 * This function is used when the application wants to handle the data transfer
 * using an already existing file descriptor (e.g., for performance reasons or
 * when integrating with other data handling mechanisms). The application is
 * responsible for writing data to this `fd` when the source provides it.
 * The offer's ref count is incremented if it wasn't already proxied.
 *
 * @param offer The data offer.
 * @param mime The MIME type for which data is being received.
 * @param fd The file descriptor to which the compositor will write the data.
 *           The application should ensure this fd is valid and writable from
 *           the compositor's perspective.
 */
EAPI void
ecore_wl2_offer_proxy_receive(Ecore_Wl2_Offer *offer, const char *mime, int fd)
{
   EINA_SAFETY_ON_NULL_RETURN(offer);

   if (!offer->proxied) offer->ref++;
   offer->proxied = 1;
   wl_data_offer_receive(offer->offer, mime, fd);
}

/**
 * @brief Signals the end of a proxied data receive operation.
 *
 * This should be called after `ecore_wl2_offer_proxy_receive` when the
 * data transfer through the provided file descriptor is complete or
 * should be considered finished from the offer's perspective.
 * It decrements the offer's reference count if it was previously proxied.
 *
 * @param offer The data offer for which proxied receive is ending.
 */
EAPI void
ecore_wl2_offer_proxy_receive_end(Ecore_Wl2_Offer *offer)
{
   EINA_SAFETY_ON_NULL_RETURN(offer);

   if (!offer->proxied) return;
   offer->proxied = 0;
   _ecore_wl2_offer_unref(offer);
}

/**
 * @brief Finishes a data offer.
 *
 * This function informs the compositor that the client is done with the
 * data offer. For drag-and-drop, this typically means the drop is accepted
 * or rejected. For selections, it means the data has been pasted or the
 * paste operation is complete.
 * After calling this, the wl_data_offer object is no longer valid.
 *
 * @param offer The data offer to finish.
 */
EAPI void
ecore_wl2_offer_finish(Ecore_Wl2_Offer *offer)
{
   EINA_SAFETY_ON_NULL_RETURN(offer);

   wl_data_offer_finish(offer->offer);
}

/**
 * @internal
 * @brief Decrements the reference count of an Ecore_Wl2_Offer and frees it if count reaches zero.
 *
 * When the reference count drops to zero, this function destroys the
 * associated wl_data_offer, frees the MIME types array, and deallocates
 * the Ecore_Wl2_Offer structure itself. It also clears any pointers to this
 * offer from the input's drag or selection state.
 *
 * @param offer The Ecore_Wl2_Offer to unreference.
 */
void
_ecore_wl2_offer_unref(Ecore_Wl2_Offer *offer)
{
   char *str;

   EINA_SAFETY_ON_NULL_RETURN(offer);

   offer->ref--;

   if (offer->ref > 0) return;

   wl_data_offer_destroy(offer->offer);

   if (offer->mimetypes)
     {
        while((str = eina_array_pop(offer->mimetypes)))
          free(str);
        eina_array_free(offer->mimetypes);
        offer->mimetypes = NULL;
     }

   if (offer->input->drag.offer == offer) offer->input->drag.offer = NULL;
   if (offer->input->selection.offer == offer) offer->input->selection.offer = NULL;

   free(offer);
}

/**
 * @internal
 * @brief Comparison function for eina_array_foreach to find a string.
 *
 * Used by ecore_wl2_offer_supports_mime.
 *
 * @param container Unused.
 * @param elem An element from the array (expected to be a char *).
 * @param data The string to compare against (char *).
 * @return EINA_FALSE if strings match (stops iteration), EINA_TRUE otherwise.
 */
static unsigned char
_compare(const void *container EINA_UNUSED, void *elem, void *data)
{
   if (!strcmp(elem, data))
     return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @brief Checks if a data offer supports a specific MIME type.
 *
 * @param offer The data offer to check.
 * @param mime The MIME type string to look for (e.g., "text/plain").
 * @return EINA_TRUE if the offer supports the MIME type, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl2_offer_supports_mime(Ecore_Wl2_Offer *offer, const char *mime)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(offer, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(mime, EINA_FALSE);

  return !eina_array_foreach(offer->mimetypes, _compare, (void*) mime);
}
