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

#include <fcntl.h>
#include <sys/epoll.h>
#include "ecore_wl_private.h"

/* local structures */
/**
 * @internal
 * @brief Helper structure to associate a DND source with its read file descriptor.
 *
 * This is used when asynchronously reading data from a DND offer, particularly
 * within the epoll-based data reading mechanism.
 */
struct _dnd_source
{
   Ecore_Wl_Dnd_Source *source; /**< The Ecore Wayland DND source object, which holds the wl_data_offer. */
   int read_fd;                 /**< The file descriptor created from a pipe, used to read incoming data. */
};

/**
 * @internal
 * @brief Structure to hold data and callback for an Ecore Fd Handler-like task.
 *
 * This structure is used with the epoll mechanism to encapsulate the context
 * (struct _dnd_source) and the callback function (_ecore_wl_dnd_selection_data_read)
 * for handling DND data reading when the associated file descriptor becomes readable.
 */
struct _dnd_task
{
   void *data;    /**< Custom data to be passed to the callback, typically a struct _dnd_source. */
   Ecore_Fd_Cb cb; /**< The callback function to execute when the fd is ready for I/O. */
};

/**
 * @internal
 * @brief Context for managing epoll-based DND data reading.
 *
 * This structure holds the epoll file descriptor and the event structure
 * used for monitoring the DND data pipe. It's passed to the idler callback
 * `_ecore_wl_dnd_selection_cb_idle`.
 */
struct _dnd_read_ctx
{
   int epoll_fd;          /**< The epoll instance file descriptor. */
   struct epoll_event *ep; /**< Pointer to the epoll_event structure used for monitoring. */
};

/* local function prototypes */
/**
 * @internal
 * @brief Initiates the process of receiving data for a specific MIME type from a DND source.
 *
 * This function sets up a pipe and uses epoll to asynchronously read data
 * offered by the DND source (via its `wl_data_offer`). It is called when the
 * application requests data of a certain `type` from the current selection or drag
 * (e.g., via `ecore_wl_dnd_selection_get` or `ecore_wl_dnd_drag_get`).
 * The actual reading is handled by `_ecore_wl_dnd_selection_data_read` triggered
 * by `_ecore_wl_dnd_selection_cb_idle`.
 *
 * @param source The DND source (encapsulating a `wl_data_offer`) from which to receive data.
 * @param type The MIME type of the data to receive (e.g., "text/plain").
 */
static void _ecore_wl_dnd_selection_data_receive(Ecore_Wl_Dnd_Source *source, const char *type);

/**
 * @internal
 * @brief Reads available data from the DND source's file descriptor.
 *
 * This function is registered as a callback (indirectly via an idler and epoll)
 * to be called when data is available on the pipe set up by
 * `_ecore_wl_dnd_selection_data_receive`. It reads a chunk of data and
 * emits an `ECORE_WL_EVENT_SELECTION_DATA_READY` Ecore event.
 *
 * @param data A pointer to a `struct _dnd_source` containing the DND source
 *             and the read file descriptor.
 * @param fd_handler The Ecore_Fd_Handler that triggered this callback (unused, as
 *                   this is driven by an epoll mechanism managed by an idler).
 * @return `ECORE_CALLBACK_RENEW` if more data might be available or data was read,
 *         `ECORE_CALLBACK_CANCEL` if reading is complete (EOF) or an error occurred.
 */
static Eina_Bool _ecore_wl_dnd_selection_data_read(void *data, Ecore_Fd_Handler *fd_handler EINA_UNUSED);

/**
 * @internal
 * @brief Frees the data associated with an `ECORE_WL_EVENT_SELECTION_DATA_READY` event.
 *
 * This function is registered as the free callback when an
 * `ECORE_WL_EVENT_SELECTION_DATA_READY` event is added to the Ecore event queue.
 * It is responsible for freeing the event structure itself and any dynamically
 * allocated data within it (specifically, the `data` buffer containing the
 * received content).
 *
 * @param data User data passed to `ecore_event_add` (unused in this case).
 * @param event A pointer to the `Ecore_Wl_Event_Selection_Data_Ready` event structure.
 */
static void _ecore_wl_dnd_selection_data_ready_cb_free(void *data EINA_UNUSED, void *event);

/**
 * @internal
 * @brief Idle handler callback for processing DND data using epoll.
 *
 * This function is added as an Ecore idler. On each invocation, it calls
 * `epoll_wait` with a timeout of 0 (non-blocking) to check if the DND data
 * file descriptor (monitored by epoll) has data available for reading.
 * If data is ready, it invokes the registered callback (which is
 * `_ecore_wl_dnd_selection_data_read`) to process the data.
 * This idler is removed when the callback signals completion or an error.
 *
 * This approach is used as an alternative to `ecore_main_fd_handler_add`
 * due to issues mentioned in http://trac.enlightenment.org/e/ticket/1208.
 *
 * @param data A pointer to a `struct _dnd_read_ctx`, which contains the
 *             epoll file descriptor and event structure.
 * @return `ECORE_CALLBACK_RENEW` to keep the idler running.
 *         `ECORE_CALLBACK_CANCEL` to remove the idler (e.g., when the
 *         data transfer is complete or an error occurs).
 */
static Eina_Bool _ecore_wl_dnd_selection_cb_idle(void *data);

/**
 * @internal
 * @brief Wayland data source listener callback for the 'target' event.
 *
 * This callback is invoked by the Wayland compositor when a client (the target
 * application of a drag or selection) indicates it can accept one of the
 * MIME types offered by our data source. If `mime_type` is NULL, it means
 * the target is no longer interested in any of the offered types, or the
 * data source is being destroyed.
 *
 * This function creates and sends an `ECORE_WL_EVENT_DATA_SOURCE_TARGET` Ecore
 * event to notify the application.
 *
 * @param data The `Ecore_Wl_Input` associated with this data source.
 * @param source The `wl_data_source` object that emitted this event (unused).
 * @param mime_type The MIME type that the target has chosen. If NULL, it
 *                  indicates the target is no longer interested or the data
 *                  source is being destroyed without a target.
 */
static void _ecore_wl_dnd_source_cb_target(void *data, struct wl_data_source *source EINA_UNUSED, const char *mime_type);

/**
 * @internal
 * @brief Frees data associated with an `ECORE_WL_EVENT_DATA_SOURCE_TARGET` event.
 *
 * Registered as a free callback for `ECORE_WL_EVENT_DATA_SOURCE_TARGET` events.
 * Frees the event structure and its `type` string.
 *
 * @param data User data (unused).
 * @param event The `Ecore_Wl_Event_Data_Source_Target` event to free.
 */
static void _ecore_wl_dnd_source_cb_target_free(void *data EINA_UNUSED, void *event);

/**
 * @internal
 * @brief Wayland data source listener callback for the 'send' event.
 *
 * This callback is invoked when a client (target application) requests
 * the actual data for a specific MIME type that our data source offers.
 * The compositor provides a file descriptor (`fd`) into which our
 * application should write the data.
 *
 * This function generates an `ECORE_WL_EVENT_DATA_SOURCE_SEND` Ecore event,
 * passing the MIME type and file descriptor to the application. The
 * application is then responsible for writing the data to the `fd` and
 * closing it.
 *
 * @param data The `Ecore_Wl_Input` associated with this data source.
 * @param source The `wl_data_source` object (unused in this function).
 * @param mime_type The MIME type for which data is requested.
 * @param fd The file descriptor to write the data into.
 */
static void _ecore_wl_dnd_source_cb_send(void *data, struct wl_data_source *source EINA_UNUSED, const char *mime_type, int32_t fd);
/**
 * @internal
 * @brief Frees data associated with an `ECORE_WL_EVENT_DATA_SOURCE_SEND` event.
 *
 * Registered as a free callback for `ECORE_WL_EVENT_DATA_SOURCE_SEND` events.
 * Frees the event structure and its `type` string.
 *
 * @param data User data (unused).
 * @param event The `Ecore_Wl_Event_Data_Source_Send` event to free.
 */
static void _ecore_wl_dnd_source_cb_send_free(void *data EINA_UNUSED, void *event);

/**
 * @internal
 * @brief Wayland data source listener callback for the 'cancelled' event.
 *
 * This callback is invoked by the Wayland compositor when the data source
 * is no longer valid and will not be used again (e.g., the drag was
 * cancelled, selection was replaced, or the client was destroyed).
 * The application should destroy the `wl_data_source` object.
 *
 * This function destroys the `source` and generates an
 * `ECORE_WL_EVENT_DATA_SOURCE_CANCELLED` Ecore event.
 *
 * @param data The `Ecore_Wl_Input` associated with this data source.
 * @param source The `wl_data_source` object that was cancelled.
 */
static void _ecore_wl_dnd_source_cb_cancelled(void *data EINA_UNUSED, struct wl_data_source *source);

/**
 * @internal
 * @brief Wayland data offer listener callback for the 'offer' event.
 *
 * This callback is invoked by the Wayland compositor for each MIME type
 * that a data offer supports. When a new data offer is introduced (e.g.,
 * during a drag enter or when a new selection is available), this event
 * will be emitted multiple times, once for each type like "text/plain",
 * "image/png", etc., that the source is offering.
 *
 * This function adds the advertised `type` (by duplicating it) to the list of types
 * stored in the `Ecore_Wl_Dnd_Source` associated with the `data_offer`.
 *
 * @param data A pointer to the `Ecore_Wl_Dnd_Source` associated with this data offer.
 *             This is the private data set when the listener was added.
 * @param data_offer The `wl_data_offer` object that is advertising the type (unused).
 * @param type The MIME type being offered (e.g., "text/plain").
 */
static void _ecore_wl_dnd_offer_cb_offer(void *data, struct wl_data_offer *data_offer EINA_UNUSED, const char *type);

/* local wayland interfaces */
static const struct wl_data_source_listener
_ecore_wl_dnd_source_listener =
{
   _ecore_wl_dnd_source_cb_target,
   _ecore_wl_dnd_source_cb_send,
   _ecore_wl_dnd_source_cb_cancelled
};

static const struct wl_data_offer_listener
_ecore_wl_dnd_offer_listener =
{
   _ecore_wl_dnd_offer_cb_offer
};

/**
 * @deprecated use ecore_wl_dnd_selection_set
 * @since 1.7
*/
EINA_DEPRECATED EAPI Eina_Bool
ecore_wl_dnd_set_selection(Ecore_Wl_Dnd *dnd, const char **types_offered)
{
   LOGFN;

   return ecore_wl_dnd_selection_set(dnd->input, types_offered);
}

/**
 * @deprecated use ecore_wl_dnd_selection_get
 * @since 1.7
*/
EINA_DEPRECATED EAPI Eina_Bool
ecore_wl_dnd_get_selection(Ecore_Wl_Dnd *dnd, const char *type)
{
   LOGFN;

   return ecore_wl_dnd_selection_get(dnd->input, type);
}

/**
 * @deprecated Do Not Use
 * @since 1.7
 */
EINA_DEPRECATED EAPI Ecore_Wl_Dnd *
ecore_wl_dnd_get(void)
{
   return NULL;
}

/**
 * @deprecated use ecore_wl_dnd_drag_start
 * @since 1.7
 */
EINA_DEPRECATED EAPI Eina_Bool
ecore_wl_dnd_start_drag(Ecore_Wl_Dnd *dnd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @deprecated use ecore_wl_dnd_selection_owner_has
 * @since 1.7
 */
EINA_DEPRECATED EAPI Eina_Bool
ecore_wl_dnd_selection_has_owner(Ecore_Wl_Dnd *dnd)
{
   return ecore_wl_dnd_selection_owner_has(dnd->input);
}

/**
 * @ingroup Ecore_Wl_Dnd_Group
 * @brief Sets the current selection (clipboard) data source.
 *
 * This function creates a new Wayland data source, offers the specified MIME types,
 * and sets it as the current selection for the given input device.
 * The application will later receive 'target' and 'send' events on this
 * data source when another application attempts to paste the selection.
 *
 * @param input The Ecore_Wl_Input representing the seat.
 * @param types_offered A NULL-terminated array of C strings, where each string is a
 *                      MIME type being offered.
 *                      Example: `const char *types[] = {"text/plain;charset=utf-8", "text/uri-list", NULL};`
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., cannot create data source).
 * @since 1.8
 */
EAPI Eina_Bool
ecore_wl_dnd_selection_set(Ecore_Wl_Input *input, const char **types_offered)
{
   struct wl_data_device_manager *man;
   const char **type;
   char **t;

   LOGFN;

   if (!input) return EINA_FALSE;

   man = input->display->wl.data_device_manager;

   /* free any old types offered */
   if (input->data_types.data)
     {
        wl_array_for_each(t, &input->data_types)
          free(*t);
        wl_array_release(&input->data_types);
        wl_array_init(&input->data_types);
     }

   input->data_source = NULL;

   if (!types_offered[0]) return EINA_FALSE;

   /* try to create a new data source */
   if (!(input->data_source = wl_data_device_manager_create_data_source(man)))
     return EINA_FALSE;

   /* add these types to the data source */
   for (type = types_offered; *type; type++)
     {
        t = wl_array_add(&input->data_types, sizeof(*t));
        if (t)
          {
             *t = strdup(*type);
             wl_data_source_offer(input->data_source, *t);
          }
     }

   /* add a listener for data source events */
   wl_data_source_add_listener(input->data_source,
                               &_ecore_wl_dnd_source_listener, input);

   /* set the selection */
   wl_data_device_set_selection(input->data_device, input->data_source,
                                input->display->serial);

   return EINA_TRUE;
}

/**
 * @ingroup Ecore_Wl_Dnd_Group
 * @brief Requests data for a specific MIME type from the current selection owner.
 *
 * If there is a current selection and it offers the requested `type`, this
 * function initiates the data transfer. The actual data will be delivered
 * asynchronously via `ECORE_WL_EVENT_SELECTION_DATA_READY` events.
 *
 * @param input The Ecore_Wl_Input representing the seat.
 * @param type The desired MIME type to request from the selection.
 *             Example: `"text/plain;charset=utf-8"`.
 * @return EINA_TRUE if the request was successfully initiated, EINA_FALSE otherwise
 *         (e.g., no selection owner, or requested type not offered).
 * @since 1.8
 */
EAPI Eina_Bool
ecore_wl_dnd_selection_get(Ecore_Wl_Input *input, const char *type)
{
   char **t;

   LOGFN;

   /* check for valid input and selection source */
   if ((!input) || (!input->selection_source)) return EINA_FALSE;

   for (t = input->selection_source->types.data; *t; t++)
     {
        if (!strcmp(type, *t)) break;
     }

   if (!*t) return EINA_FALSE;

   _ecore_wl_dnd_selection_data_receive(input->selection_source, type);

   return EINA_TRUE;
}

/**
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI Eina_Bool
ecore_wl_dnd_selection_owner_has(Ecore_Wl_Input *input)
{
   LOGFN;

   if (!input) return EINA_FALSE;
   return (input->selection_source != NULL);
}

/**
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI Eina_Bool
ecore_wl_dnd_selection_clear(Ecore_Wl_Input *input)
{
   LOGFN;

   /* check for valid input */
   if (!input) return EINA_FALSE;

   /* set the selection to NULL */
   wl_data_device_set_selection(input->data_device, NULL,
                                input->display->serial);

   return EINA_TRUE;
}

/**
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI void
ecore_wl_dnd_drag_start(Ecore_Wl_Input *input, Ecore_Wl_Window *win, Ecore_Wl_Window *dragwin, int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED)
{
   struct wl_surface *drag_surface;
   struct wl_surface *origin_surface;

   LOGFN;

   /* check for valid input. if not, get the default one */
   if (!input) input = _ecore_wl_disp->input;

   /* check for valid data source */
   if (!input->data_source) return;

   /* get the surface from this drag window */
   drag_surface = ecore_wl_window_surface_get(dragwin);

   /* release any existing grabs */
   ecore_wl_input_ungrab(input);

   /* add a listener for data source events */
   wl_data_source_add_listener(input->data_source,
                               &_ecore_wl_dnd_source_listener, input);

   /* start the drag */
   if ((origin_surface = ecore_wl_window_surface_get(win)))
     {
        wl_data_device_start_drag(input->data_device, input->data_source,
                                  origin_surface, drag_surface,
                                  input->display->serial);

        /* set pointer image */
        ecore_wl_input_cursor_from_name_set(input, "move");
     }

   /* NB: Below code disabled for now
    *
    * This Was for adjusting the "drag icon" to be centered on the mouse
    * based on the hotspot, but it crashes for some reason :(
    */

   /* struct wl_buffer *drag_buffer; */
   /* struct wl_cursor_image *cursor; */
   /* int cx = 0, cy = 0; */
   /* drag_buffer = wl_surface_get_user_data(drag_surface); */
   /* cursor = input->cursor->images[input->cursor_current_index]; */
   /* cx = cursor->hotspot_x - x; */
   /* cy = cursor->hotspot_y - y; */
   /* wl_surface_attach(drag_surface, drag_buffer, cx, cy); */
   /* wl_surface_damage(drag_surface, 0, 0, w, h); */
   /* wl_surface_commit(drag_surface); */
}

/**
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI void
ecore_wl_dnd_drag_end(Ecore_Wl_Input *input)
{
   Ecore_Wl_Event_Dnd_End *ev;

   LOGFN;

   /* check for valid input. if not, get the default one */
   if (!input) input = _ecore_wl_disp->input;

   if (input->data_types.data)
     {
        char **t;

        wl_array_for_each(t, &input->data_types)
          free(*t);
        wl_array_release(&input->data_types);
        wl_array_init(&input->data_types);
     }

   /* if (input->drag_source) _ecore_wl_dnd_del(input->drag_source); */
   /* input->drag_source = NULL; */

   /* destroy any existing data source */
   if (input->data_source) wl_data_source_destroy(input->data_source);
   input->data_source = NULL;

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Dnd_End)))) return;

   if (input->pointer_focus)
     ev->win = input->pointer_focus->id;

   if (input->keyboard_focus)
     ev->source = input->keyboard_focus->id;

   ecore_event_add(ECORE_WL_EVENT_DND_END, ev, NULL, NULL);
}

/**
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI Eina_Bool
ecore_wl_dnd_drag_get(Ecore_Wl_Input *input, const char *type)
{
   char **t;

   LOGFN;

   /* check for valid input and drag source */
   if ((!input) || (!input->drag_source)) return EINA_FALSE;

   wl_array_for_each(t, &input->drag_source->types)
     if (!strcmp(type, *t)) break;

   if (!*t) return EINA_FALSE;

   _ecore_wl_dnd_selection_data_receive(input->drag_source, type);

   return EINA_TRUE;
}

/**
 * @ingroup Ecore_Wl_Dnd_Group
 * @brief Sets the MIME types offered for an upcoming drag-and-drop operation.
 *
 * This function creates or updates the data source that will be used when
 * `ecore_wl_dnd_drag_start()` is called. It specifies the list of MIME types
 * that this client can provide if a drop occurs.
 *
 * @param input The Ecore_Wl_Input representing the seat. If NULL, the default
 *              display's input is used.
 * @param types_offered A NULL-terminated array of C strings, where each string is a
 *                      MIME type being offered for the drag.
 *                      Example: `const char *types[] = {"text/plain", "application/x-my-custom-type", NULL};`
 * @since 1.8
 */
EAPI void
ecore_wl_dnd_drag_types_set(Ecore_Wl_Input *input, const char **types_offered)
{
   struct wl_data_device_manager *man;
   const char **type;
   char **t;

   LOGFN;

   /* check for valid input. if not, get the default one */
   if (!input) input = _ecore_wl_disp->input;

   man = input->display->wl.data_device_manager;

   /* free any old types offered */
   if (input->data_types.data)
     {
        wl_array_for_each(t, &input->data_types)
          free(*t);
        wl_array_release(&input->data_types);
        wl_array_init(&input->data_types);
     }

   /* destroy any existing data source */
   if (input->data_source) wl_data_source_destroy(input->data_source);
   input->data_source = NULL;

   /* try to create a new data source */
   if (!(input->data_source = wl_data_device_manager_create_data_source(man)))
     {
        printf("Failed to create new data source for drag\n");
        return;
     }

   /* add these types to the data source */
   for (type = types_offered; *type; type++)
     {
        t = wl_array_add(&input->data_types, sizeof(*t));
        if (t)
          {
             *t = strdup(*type);
             wl_data_source_offer(input->data_source, *t);
          }
     }
}

/**
 * @ingroup Ecore_Wl_Dnd_Group
 * @brief Gets the list of MIME types currently set for an outgoing drag operation.
 *
 * This retrieves the types previously set by `ecore_wl_dnd_drag_types_set()`.
 *
 * @param input The Ecore_Wl_Input representing the seat. If NULL, the default
 *              display's input is used.
 * @return A pointer to a `struct wl_array` containing the offered MIME types.
 *         Each element in the array is a `char *` (string).
 *         The array is owned by `Ecore_Wl_Input` and must not be modified or freed
 *         by the caller. Its lifetime is tied to the input's internal data source
 *         state for DND operations. Returns NULL if input is invalid or no types are set.
 *         Example of iterating:
 *         <pre>
 *         struct wl_array *types_array = ecore_wl_dnd_drag_types_get(input);
 *         if (types_array) {
 *            char **type_ptr;
 *            wl_array_for_each(type_ptr, types_array) {
 *               if (*type_ptr) // Important: last element might be NULL from _ecore_wl_dnd_selection
 *                 printf("Offered DND type: %s\n", *type_ptr);
 *            }
 *         }
 *         </pre>
 * @since 1.8
 */
EAPI struct wl_array *
ecore_wl_dnd_drag_types_get(Ecore_Wl_Input *input)
{
   LOGFN;

   /* check for valid input. if not, get the default one */
   if (!input) input = _ecore_wl_disp->input;
   if (!input) return NULL;

   return &input->data_types;
}

/* private functions */
/**
 * @internal
 * @brief Creates and initializes an `Ecore_Wl_Dnd_Source` for a new Wayland data offer.
 *
 * This function is called when the compositor introduces a new data offer,
 * typically associated with a drag-and-drop operation entering a window or
 * a new clipboard selection becoming available. It allocates an
 * `Ecore_Wl_Dnd_Source` structure, initializes its type array, sets its
 * reference count, associates it with the given `Ecore_Wl_Input` and
 * `wl_data_offer`, and adds a listener (`_ecore_wl_dnd_offer_listener`)
 * to the `wl_data_offer`. This listener will subsequently receive the
 * offered MIME types via the `_ecore_wl_dnd_offer_cb_offer` callback.
 *
 * The newly created `Ecore_Wl_Dnd_Source` is then set as user data for the
 * `wl_data_offer`, allowing it to be retrieved later when events related
 * to this offer are received.
 *
 * @param input The `Ecore_Wl_Input` associated with the current seat/device.
 * @param data_device The `wl_data_device` (unused in this function, but part of
 *                    the Wayland event signature that might lead to this call).
 * @param offer The new `wl_data_offer` object provided by the compositor.
 *              This represents the data being offered by another client.
 */
void
_ecore_wl_dnd_add(Ecore_Wl_Input *input, struct wl_data_device *data_device EINA_UNUSED, struct wl_data_offer *offer)
{
   Ecore_Wl_Dnd_Source *source;

   LOGFN;

   if (!(source = malloc(sizeof(Ecore_Wl_Dnd_Source))))
     return;

   wl_array_init(&source->types);
   source->refcount = 1;
   source->input = input;
   source->data_offer = offer;

   wl_data_offer_add_listener(source->data_offer,
                              &_ecore_wl_dnd_offer_listener, source);
}

/**
 * @internal
 * @brief Handles the Wayland `data_device` 'enter' event.
 *
 * This function is called when a drag operation enters a surface (window)
 * managed by this Ecore_Wl instance. It updates the input state (pointer focus,
 * serial, drag source) and creates and dispatches an `ECORE_WL_EVENT_DND_ENTER`
 * Ecore event.
 *
 * The event contains information about the window entered, the source window
 * (if available via keyboard focus), the Wayland data offer, serial, position,
 * and the list of MIME types offered by the `input->drag_source`.
 *
 * @param data A pointer to the `Ecore_Wl_Input` associated with the data device.
 * @param data_device The `wl_data_device` that received the enter event (unused).
 * @param timestamp The timestamp of the enter event (serial).
 * @param surface The `wl_surface` that was entered by the drag.
 * @param x The x-coordinate of the pointer relative to the surface, in surface-local coordinates (fixed-point).
 * @param y The y-coordinate of the pointer relative to the surface, in surface-local coordinates (fixed-point).
 * @param offer The `wl_data_offer` associated with the current drag operation.
 *              If NULL, it means there's no data associated with this enter event.
 */
void
_ecore_wl_dnd_enter(void *data, struct wl_data_device *data_device EINA_UNUSED, unsigned int timestamp, struct wl_surface *surface, int x, int y, struct wl_data_offer *offer)
{
   Ecore_Wl_Event_Dnd_Enter *ev;
   Ecore_Wl_Window *win;
   Ecore_Wl_Input *input;
   char **types;
   int num = 0;

   LOGFN;

   if (!(input = data)) return;

   win = ecore_wl_window_surface_find(surface);

   input->pointer_enter_serial = timestamp;
   input->pointer_focus = win;

   if (offer)
     {
        input->drag_source = wl_data_offer_get_user_data(offer);

        num = (input->drag_source->types.size / sizeof(char *));
        types = input->drag_source->types.data;
     }
   else
     {
        input->drag_source = NULL;
        types = NULL;
     }

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Dnd_Enter)))) return;

   if (win)
     ev->win = win->id;

   if (input->keyboard_focus)
     ev->source = input->keyboard_focus->id;

   ev->offer = offer;
   ev->serial = timestamp;
   ev->position.x = wl_fixed_to_int(x);
   ev->position.y = wl_fixed_to_int(y);
   ev->num_types = num;
   ev->types = types;

   ecore_event_add(ECORE_WL_EVENT_DND_ENTER, ev, NULL, NULL);
}

/**
 * @internal
 * @brief Handles the Wayland `data_device` 'leave' event.
 *
 * This function is called when a drag operation leaves a surface (window)
 * that it had previously entered. It creates and dispatches an
 * `ECORE_WL_EVENT_DND_LEAVE` Ecore event.
 *
 * The event contains information about the window that was left and
 * potentially the source window (based on current keyboard focus).
 * After a leave event, `input->pointer_focus` is typically set to NULL by
 * the caller or Wayland event dispatch, and `input->drag_source` might be
 * disassociated or cleaned up if the drag is over.
 *
 * @param data A pointer to the `Ecore_Wl_Input` associated with the data device.
 * @param data_device The `wl_data_device` that received the leave event (unused).
 */
void
_ecore_wl_dnd_leave(void *data, struct wl_data_device *data_device EINA_UNUSED)
{
   Ecore_Wl_Event_Dnd_Leave *ev;
   Ecore_Wl_Input *input;

   LOGFN;

   if (!(input = data)) return;

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Dnd_Leave)))) return;

   if (input->pointer_focus)
     ev->win = input->pointer_focus->id;

   if (input->keyboard_focus)
     ev->source = input->keyboard_focus->id;

   ecore_event_add(ECORE_WL_EVENT_DND_LEAVE, ev, NULL, NULL);
}

/**
 * @internal
 * @brief Handles the Wayland `data_device` 'motion' event.
 *
 * This function is called when the pointer moves during a drag operation
 * while over a surface managed by this Ecore_Wl instance. It updates
 * the stored pointer coordinates (`input->sx`, `input->sy` after conversion
 * from fixed-point) and creates and dispatches an `ECORE_WL_EVENT_DND_POSITION`
 * Ecore event.
 *
 * The event contains the current window under the pointer (from `input->pointer_focus`)
 * and the new pointer coordinates.
 *
 * @param data A pointer to the `Ecore_Wl_Input` associated with the data device.
 * @param data_device The `wl_data_device` that received the motion event (unused).
 * @param timestamp The timestamp of the motion event (unused in this function).
 * @param x The new x-coordinate of the pointer in surface-local, fixed-point format.
 * @param y The new y-coordinate of the pointer in surface-local, fixed-point format.
 */
void
_ecore_wl_dnd_motion(void *data, struct wl_data_device *data_device EINA_UNUSED, unsigned int timestamp EINA_UNUSED, int x, int y)
{
   Ecore_Wl_Event_Dnd_Position *ev;
   Ecore_Wl_Input *input;

   LOGFN;

   if (!(input = data)) return;

   input->sx = wl_fixed_to_int(x);
   input->sy = wl_fixed_to_int(y);

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Dnd_Position)))) return;

   if (input->pointer_focus)
     ev->win = input->pointer_focus->id;

   if (input->keyboard_focus)
     ev->source = input->keyboard_focus->id;

   ev->position.x = input->sx;
   ev->position.y = input->sy;

   ecore_event_add(ECORE_WL_EVENT_DND_POSITION, ev, NULL, NULL);
}

/**
 * @internal
 * @brief Handles the Wayland `data_device` 'drop' event.
 *
 * This function is called when the user performs the drop action (e.g.,
 * releases the mouse button) during a drag operation over one of our surfaces.
 * It creates and dispatches an `ECORE_WL_EVENT_DND_DROP` Ecore event.
 *
 * The event includes the window where the drop occurred (from `input->pointer_focus`)
 * and the coordinates of the drop (from `input->sx`, `input->sy`). The application
 * is then expected to interact with the `wl_data_offer` (retrieved via
 * `input->drag_source`) to select a MIME type and receive the data using functions
 * like `ecore_wl_dnd_drag_get` (which internally calls `wl_data_offer_receive`).
 *
 * @param data A pointer to the `Ecore_Wl_Input` associated with the data device.
 * @param data_device The `wl_data_device` that received the drop event (unused).
 */
void
_ecore_wl_dnd_drop(void *data, struct wl_data_device *data_device EINA_UNUSED)
{
   Ecore_Wl_Event_Dnd_Drop *ev;
   Ecore_Wl_Input *input;

   LOGFN;

   if (!(input = data)) return;

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Dnd_Drop)))) return;

   if (input->drag_source)
     {
        if (input->pointer_focus)
          ev->win = input->pointer_focus->id;
        if (input->keyboard_focus)
          ev->source = input->keyboard_focus->id;
     }

   ev->position.x = input->sx;
   ev->position.y = input->sy;

   ecore_event_add(ECORE_WL_EVENT_DND_DROP, ev, NULL, NULL);
}

/**
 * @internal
 * @brief Handles the Wayland `data_device` 'selection' event.
 *
 * This function is called when the clipboard selection changes. The `offer`
 * parameter represents the new selection data. If `offer` is NULL, it means
 * the selection has been cleared or lost.
 *
 * This function updates the `input->selection_source`. If there was a previous
 * selection source, it's dereferenced (and potentially freed by `_ecore_wl_dnd_del`).
 * If a new `offer` is provided, it retrieves the associated `Ecore_Wl_Dnd_Source`
 * (which should have been created by `_ecore_wl_dnd_add` and set as user data
 * on the `offer`) and sets it as the new `input->selection_source`.
 *
 * A NULL entry is added to the `types` array of the new selection source. This
 * acts as a sentinel for the `wl_array_for_each` macro used elsewhere, as
 * `_ecore_wl_dnd_offer_cb_offer` populates the actual types before this sentinel.
 *
 * @param data A pointer to the `Ecore_Wl_Input` associated with the data device.
 * @param data_device The `wl_data_device` that received the selection event (unused).
 * @param offer The new `wl_data_offer` for the selection, or NULL if the
 *              selection is cleared.
 */
void
_ecore_wl_dnd_selection(void *data, struct wl_data_device *data_device EINA_UNUSED, struct wl_data_offer *offer)
{
   Ecore_Wl_Input *input;

   LOGFN;

   if (!(input = data)) return;

   if (input->selection_source) _ecore_wl_dnd_del(input->selection_source);
   input->selection_source = NULL;

   if (offer)
     {
        char **t;

        input->selection_source = wl_data_offer_get_user_data(offer);
        t = wl_array_add(&input->selection_source->types, sizeof(*t));
        *t = NULL;
     }
}

/**
 * @internal
 * @brief Decrements the reference count of an `Ecore_Wl_Dnd_Source` and frees it if count reaches zero.
 *
 * This function is used for managing the lifecycle of `Ecore_Wl_Dnd_Source`
 * objects. These objects encapsulate a `wl_data_offer` and its associated MIME types.
 * When a DND source is no longer needed (e.g., selection changes,
 * data transfer complete, drag ends), its reference count is decremented. If the count
 * drops to zero, the associated `wl_data_offer` is destroyed, the internal
 * types array (which stores `char *` MIME types) is released (including freeing
 * each string), and the `Ecore_Wl_Dnd_Source` structure itself is freed.
 *
 * @param source The `Ecore_Wl_Dnd_Source` object to dereference and potentially delete.
 *               If NULL, the function does nothing.
 */
void
_ecore_wl_dnd_del(Ecore_Wl_Dnd_Source *source)
{
   LOGFN;

   if (!source) return;
   source->refcount--;
   if (source->refcount == 0)
     {
        wl_data_offer_destroy(source->data_offer);
        wl_array_release(&source->types);
        free(source);
     }
}

/* local functions */
/**
 * @internal
 * @brief Initiates receiving data for a specific MIME type from a DND source.
 *
 * This function sets up a pipe and uses epoll to asynchronously read data
 * offered by the DND source (via its `wl_data_offer`). It is called when the
 * application requests data of a certain `type` from the current selection or drag
 * (e.g., via `ecore_wl_dnd_selection_get` or `ecore_wl_dnd_drag_get`).
 *
 * A pipe is created, and `wl_data_offer_receive` is called to instruct the
 * source to write data of the specified `type` into the write-end of the pipe.
 * The read-end of the pipe is then monitored using `epoll`. An idler
 * (`_ecore_wl_dnd_selection_cb_idle`) periodically checks epoll for readability.
 * When data is available, `_ecore_wl_dnd_selection_data_read` is invoked.
 *
 * The use of epoll and an idler is a workaround for issues with
 * `ecore_main_fd_handler_add`, as noted by http://trac.enlightenment.org/e/ticket/1208.
 *
 * @param source The DND source from which to receive data. This contains the
 *               Wayland data offer object (`source->data_offer`).
 * @param type The MIME type of the data to receive (e.g., "text/plain", "text/uri-list").
 *             This type must be one of the types offered by the source.
 */
static void
_ecore_wl_dnd_selection_data_receive(Ecore_Wl_Dnd_Source *source, const char *type)
{
   int epoll_fd;
   struct epoll_event *ep = NULL;
   struct _dnd_task *task = NULL;
   struct _dnd_read_ctx *read_ctx = NULL;
   struct _dnd_source *read_source = NULL;
   int p[2];

   LOGFN;

   if (pipe2(p, O_CLOEXEC) == -1)
     return;

   wl_data_offer_receive(source->data_offer, type, p[1]);
   close(p[1]);

   /* Due to http://trac.enlightenment.org/e/ticket/1208,
    * use epoll and idle handler instead of ecore_main_fd_handler_add() */

   ep = calloc(1, sizeof(struct epoll_event));
   if (!ep) goto err;

   task = calloc(1, sizeof(struct _dnd_task));
   if (!task) goto err;

   read_ctx = calloc(1, sizeof(struct _dnd_read_ctx));
   if (!read_ctx) goto err;

   epoll_fd  = epoll_create1(0);
   if (epoll_fd < 0) goto err;

   read_source = calloc(1, sizeof(struct _dnd_source));
   if (!read_source) goto err;

   read_source->source = source;
   read_source->read_fd = p[0];
   task->data = read_source;
   task->cb = _ecore_wl_dnd_selection_data_read;
   ep->events = EPOLLIN;
   ep->data.ptr = task;

   if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, p[0], ep) < 0) goto err;

   read_ctx->epoll_fd = epoll_fd;
   read_ctx->ep = ep;

   if (!ecore_idler_add(_ecore_wl_dnd_selection_cb_idle, read_ctx)) goto err;

   source->refcount++;

   return;

err:
   if (ep) free(ep);
   if (task) free(task);
   if (read_ctx) free(read_ctx);
   if (read_source) free(read_source);
   close(p[0]);
   return;
}

/**
 * @internal
 * @brief Reads available data from the DND source's file descriptor.
 *
 * This function is invoked via the epoll mechanism managed by `_ecore_wl_dnd_selection_cb_idle`
 * when data is available on the read-end of the pipe set up by `_ecore_wl_dnd_selection_data_receive`.
 * It reads a chunk of data (up to `PATH_MAX` bytes) from `read_source->read_fd`.
 *
 * An `ECORE_WL_EVENT_SELECTION_DATA_READY` Ecore event is then created and sent:
 * - If data is read (`len > 0`): The event contains the data chunk, and `done` is `EINA_FALSE`.
 *   This function returns `ECORE_CALLBACK_RENEW` to signal the idler to continue polling.
 * - If EOF is reached (`len == 0`) or an error occurs (`len < 0`): The pipe is closed,
 *   the DND source is dereferenced. The event's `done` flag is set to `EINA_TRUE`,
 *   and `data` and `len` are set to NULL/0. This function returns `ECORE_CALLBACK_CANCEL`
 *   to signal the idler to stop and clean up.
 *
 * @param data A pointer to a `struct _dnd_source`. This structure contains the
 *             `Ecore_Wl_Dnd_Source` and the file descriptor (`read_fd`) from which
 *             to read the data.
 * @param fd_handler The Ecore_Fd_Handler that would have triggered this callback if
 *                   `ecore_main_fd_handler_add` were used (unused here).
 * @return `ECORE_CALLBACK_RENEW` if data was read successfully and more might be
 *         available (signaling the idler to continue).
 *         `ECORE_CALLBACK_CANCEL` if reading is complete (EOF or error),
 *         or if a critical error like memory allocation failure occurs
 *         (signaling the idler to stop).
 */
static Eina_Bool
_ecore_wl_dnd_selection_data_read(void *data, Ecore_Fd_Handler *fd_handler EINA_UNUSED)
{
   int len;
   char buffer[PATH_MAX];
   Ecore_Wl_Dnd_Source *source;
   struct _dnd_source *read_source;
   Ecore_Wl_Event_Selection_Data_Ready *event;
   Eina_Bool ret;

   LOGFN;

   read_source = data;
   source = read_source->source;

   len = read(read_source->read_fd, buffer, sizeof buffer);

   if (!(event = calloc(1, sizeof(Ecore_Wl_Event_Selection_Data_Ready))))
     return ECORE_CALLBACK_CANCEL;

   if (len <= 0)
     {
        close(read_source->read_fd);
        _ecore_wl_dnd_del(source);
        event->done = EINA_TRUE;
        event->data = NULL;
        event->len = 0;
        ret = ECORE_CALLBACK_CANCEL;
     }
   else
     {
        event->data = malloc(len);
        if (!event->data)
          {
             free(event);
             return ECORE_CALLBACK_CANCEL;
          }
        memcpy(event->data, buffer, len);
        event->len = len;
        event->done = EINA_FALSE;
        ret = ECORE_CALLBACK_RENEW;
     }

   ecore_event_add(ECORE_WL_EVENT_SELECTION_DATA_READY, event,
                   _ecore_wl_dnd_selection_data_ready_cb_free, NULL);

   return ret;
}

/**
 * @internal
 * @brief Frees the data associated with an `ECORE_WL_EVENT_SELECTION_DATA_READY` event.
 *
 * This function is registered as the free callback when an
 * `ECORE_WL_EVENT_SELECTION_DATA_READY` event is added to the Ecore event queue.
 * It is responsible for freeing the event structure itself and any dynamically
 * allocated data within it (specifically, the `data` buffer containing the
 * received selection content).
 *
 * @param data User data passed to `ecore_event_add` (unused in this specific callback).
 * @param event A pointer to the `Ecore_Wl_Event_Selection_Data_Ready` event structure
 *              that needs to be freed.
 */
static void
_ecore_wl_dnd_selection_data_ready_cb_free(void *data EINA_UNUSED, void *event)
{
   Ecore_Wl_Event_Selection_Data_Ready *ev;

   LOGFN;

   if (!(ev = event)) return;

   free(ev->data);
   free(ev);
}

/**
 * @internal
 * @brief Idle handler callback for processing DND data using epoll.
 *
 * This function is added as an Ecore idler when `_ecore_wl_dnd_selection_data_receive`
 * sets up data transfer. On each invocation, it calls `epoll_wait` with a timeout
 * of 0 (non-blocking) to check if the DND data file descriptor (monitored by epoll)
 * has data available for reading (`EPOLLIN`).
 *
 * If `epoll_wait` indicates an event on the monitored fd:
 *  - It retrieves the associated task (`struct _dnd_task` containing `_ecore_wl_dnd_selection_data_read`
 *    and its `struct _dnd_source` data).
 *  - It executes the task's callback (`_ecore_wl_dnd_selection_data_read`).
 *  - If the callback returns `ECORE_CALLBACK_CANCEL` (signaling completion or error),
 *    this idler cleans up the epoll context (`ctx`), the task data, and removes itself
 *    by returning `ECORE_CALLBACK_CANCEL`.
 *  - Otherwise (callback returns `ECORE_CALLBACK_RENEW`), this idler returns `ECORE_CALLBACK_RENEW`
 *    to continue polling.
 *
 * @param data A pointer to a `struct _dnd_read_ctx`, which contains the
 *             epoll file descriptor and the `epoll_event` structure used for polling.
 * @return `ECORE_CALLBACK_RENEW` to keep the idler running for further checks.
 *         `ECORE_CALLBACK_CANCEL` to remove the idler once the data transfer
 *         is complete or an unrecoverable error occurs during reading.
 */
static Eina_Bool
_ecore_wl_dnd_selection_cb_idle(void *data)
{
   struct _dnd_read_ctx *ctx;
   struct _dnd_task *task;
   int count, i;

   LOGFN;

   ctx = data;
   count = epoll_wait(ctx->epoll_fd, ctx->ep, 1, 0);
   for (i = 0; i < count; i++)
     {
        task = ctx->ep->data.ptr;
        if (task->cb(task->data, NULL) == ECORE_CALLBACK_CANCEL)
          {
             free(ctx->ep);
             free(task->data);
             free(task);
             free(ctx);
             return ECORE_CALLBACK_CANCEL;
          }
     }
   return ECORE_CALLBACK_RENEW;
}

/**
 * @internal
 * @brief Wayland data source listener callback for the 'target' event.
 *
 * This callback is invoked by the Wayland compositor when a client (the target
 * application of a drag or selection) indicates it can accept one of the
 * MIME types offered by our data source (`input->data_source`). If `mime_type` is NULL,
 * it means the target is no longer interested in any of the offered types, or the
 * data source is being destroyed without a target having expressed interest.
 *
 * This function creates and sends an `ECORE_WL_EVENT_DATA_SOURCE_TARGET` Ecore
 * event to notify the application. The `mime_type` in the event will be
 * a copy of the `mime_type` argument if it's not NULL. This event informs the
 * application that a potential consumer has shown interest (or disinterest if NULL)
 * in one of its offered types. The actual data transfer is initiated by a subsequent 'send' event.
 *
 * @param data A pointer to the `Ecore_Wl_Input` that owns this data source.
 *             This is the private data set when the listener was added.
 * @param source The `wl_data_source` object that emitted this event (unused).
 * @param mime_type The MIME type that the target has chosen, or NULL if the
 *                  target is no longer interested or the data source is being
 *                  destroyed without a target.
 */
static void
_ecore_wl_dnd_source_cb_target(void *data, struct wl_data_source *source EINA_UNUSED, const char *mime_type)
{
   Ecore_Wl_Event_Data_Source_Target *event;

   LOGFN;
   if (!data) return;

   if (!(event = calloc(1, sizeof(Ecore_Wl_Event_Data_Source_Target)))) return;

   if (mime_type)
     event->type = strdup(mime_type);

   ecore_event_add(ECORE_WL_EVENT_DATA_SOURCE_TARGET, event,
                   _ecore_wl_dnd_source_cb_target_free, NULL);
}

/**
 * @internal
 * @brief Frees data associated with an `ECORE_WL_EVENT_DATA_SOURCE_TARGET` event.
 *
 * This function is registered as the free callback when an
 * `ECORE_WL_EVENT_DATA_SOURCE_TARGET` event is added to the Ecore event queue.
 * It is responsible for freeing the event structure itself and the duplicated
 * `type` string (MIME type) within it.
 *
 * @param data User data passed to `ecore_event_add` (unused in this specific callback).
 * @param event A pointer to the `Ecore_Wl_Event_Data_Source_Target` event structure
 *              that needs to be freed.
 */
static void
_ecore_wl_dnd_source_cb_target_free(void *data EINA_UNUSED, void *event)
{
   Ecore_Wl_Event_Data_Source_Target *ev;

   LOGFN;

   if (!(ev = event)) return;

   free(ev->type);
   free(ev);
}

/**
 * @internal
 * @brief Wayland data source listener callback for the 'send' event.
 *
 * This callback is invoked by the Wayland compositor when a client (the target
 * application) requests the actual data for a specific MIME type that our
 * data source offers and the target has accepted (usually after a 'target' event).
 * The compositor provides a file descriptor (`fd`) into which our
 * application must write the data corresponding to the `mime_type`.
 *
 * This function creates and sends an `ECORE_WL_EVENT_DATA_SOURCE_SEND` Ecore
 * event. This event carries the `mime_type` (duplicated) and the `fd`. The application
 * handling this Ecore event is responsible for writing the data for the requested
 * `mime_type` to the `fd` and then closing the `fd` to signal completion of the
 * data transfer for that type.
 *
 * @param data A pointer to the `Ecore_Wl_Input` that owns this data source.
 * @param source The `wl_data_source` object that emitted this event (unused).
 * @param mime_type The MIME type for which data is requested by the target.
 * @param fd The file descriptor to which the application should write the data.
 *           The application must close this fd when done writing.
 */
static void
_ecore_wl_dnd_source_cb_send(void *data, struct wl_data_source *source EINA_UNUSED, const char *mime_type, int32_t fd)
{
   Ecore_Wl_Event_Data_Source_Send *event;

   LOGFN;

   if (!data) return;

   if (!(event = calloc(1, sizeof(Ecore_Wl_Event_Data_Source_Send)))) return;

   event->type = strdup(mime_type);
   event->fd = fd;

   ecore_event_add(ECORE_WL_EVENT_DATA_SOURCE_SEND, event,
                   _ecore_wl_dnd_source_cb_send_free, NULL);
}

/**
 * @internal
 * @brief Frees data associated with an `ECORE_WL_EVENT_DATA_SOURCE_SEND` event.
 *
 * This function is registered as the free callback when an
 * `ECORE_WL_EVENT_DATA_SOURCE_SEND` event is added to the Ecore event queue.
 * It is responsible for freeing the event structure itself and the duplicated
 * `type` string (MIME type) within it.
 *
 * @param data User data passed to `ecore_event_add` (unused in this specific callback).
 * @param event A pointer to the `Ecore_Wl_Event_Data_Source_Send` event structure
 *              that needs to be freed.
 */
static void
_ecore_wl_dnd_source_cb_send_free(void *data EINA_UNUSED, void *event)
{
   Ecore_Wl_Event_Data_Source_Send *ev;

   LOGFN;

   if (!(ev = event)) return;

   free(ev->type);
   free(ev);
}

/**
 * @internal
 * @brief Wayland data source listener callback for the 'cancelled' event.
 *
 * This callback is invoked by the Wayland compositor when the data source
 * (`source`) is no longer valid and will not be used by the compositor again.
 * This can happen if a drag-and-drop operation is cancelled, if the selection is
 * claimed by another client, or if the client owning the data source
 * unsets the selection or destroys the data source.
 *
 * Upon receiving this event, the `wl_data_source` object should be destroyed.
 * This function handles the destruction of the `source` and also clears
 * the reference to it in the associated `Ecore_Wl_Input` (`input->data_source`)
 * if it matches the cancelled `source`.
 *
 * An `ECORE_WL_EVENT_DATA_SOURCE_CANCELLED` Ecore event is then generated to
 * notify the application that its data source is no longer active.
 *
 * @param data A pointer to the `Ecore_Wl_Input` that owns this data source.
 * @param source The `wl_data_source` object that has been cancelled by the
 *               compositor and should be destroyed.
 */
static void
_ecore_wl_dnd_source_cb_cancelled(void *data, struct wl_data_source *source)
{
   Ecore_Wl_Input *input;
   Ecore_Wl_Event_Data_Source_Cancelled *ev;

   LOGFN;

   if (!(input = data)) return;

   wl_data_source_destroy(source);
   if (input->data_source == source) input->data_source = NULL;

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Data_Source_Cancelled)))) return;

   if (input->pointer_focus)
     ev->win = input->pointer_focus->id;

   if (input->keyboard_focus)
     ev->source = input->keyboard_focus->id;

   ecore_event_add(ECORE_WL_EVENT_DATA_SOURCE_CANCELLED, ev, NULL, NULL);
}

/**
 * @internal
 * @brief Wayland data offer listener callback for the 'offer' event.
 *
 * This callback is invoked by the Wayland compositor for each MIME type
 * that a `wl_data_offer` supports. When a new data offer is introduced (e.g.,
 * when a drag enters a window, or when a new clipboard selection becomes
 * available), this event will be emitted by the `wl_data_offer` one or more
 * times, once for each MIME type (e.g., "text/plain", "text/uri-list",
 * "image/png") that the source of the data is offering.
 *
 * This function takes the advertised `type`, duplicates it, and adds it to
 * an internal array (`source->types`) within the `Ecore_Wl_Dnd_Source`
 * structure that corresponds to this `wl_data_offer`. This allows Ecore
 * to know all available types for a given DND source or selection.
 * The `Ecore_Wl_Dnd_Source` itself is typically created and associated with
 * the `wl_data_offer` (as its user data) in `_ecore_wl_dnd_add`.
 *
 * @param data A pointer to the `Ecore_Wl_Dnd_Source` structure that is
 *             associated with this `wl_data_offer`. This private data is
 *             set when `wl_data_offer_add_listener` is called.
 * @param data_offer The `wl_data_offer` object that is advertising the
 *                   MIME type (unused in this function, as the relevant
 *                   information is in `data` which points to the `Ecore_Wl_Dnd_Source`
 *                   that already holds this `data_offer`).
 * @param type A C-string representing the MIME type being offered by the source
 *             (e.g., "text/plain"). This string is owned by Wayland and
 *             must be duplicated if stored long-term.
 */
static void
_ecore_wl_dnd_offer_cb_offer(void *data, struct wl_data_offer *data_offer EINA_UNUSED, const char *type)
{
   Ecore_Wl_Dnd_Source *source;
   char **t;

   LOGFN;

   if (!(source = data)) return;
   if (!type) return;

   t = wl_array_add(&source->types, sizeof(*t));
   if (t) *t = strdup(type);
}
