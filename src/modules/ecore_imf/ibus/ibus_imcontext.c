#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include <X11/Xlib.h>
#include <Ecore_X.h>
#include <Ecore_Evas.h>

#include <ibus.h>
#include "ibus_imcontext.h"

/**
 * @struct _IBusIMContext
 * @brief Internal structure representing an IBus Input Method Context.
 *
 * This structure holds all the necessary information for an IBus IM context,
 * including the Ecore IMF context, the IBus input context, preedit string
 * information, cursor position, focus state, and client window/canvas.
 */
struct _IBusIMContext
{
   /* instance members */
   Ecore_IMF_Context *ctx;        /**< The parent Ecore IMF context. */

   IBusInputContext *ibuscontext; /**< The actual IBus input context object. */

   /* preedit status */
   char            *preedit_string;    /**< The current preedit string. */
   Eina_List       *preedit_attrs;     /**< List of Ecore_IMF_Preedit_Attr for the preedit string. */
   int              preedit_cursor_pos; /**< Cursor position within the preedit string. */
   Eina_Bool        preedit_visible;    /**< Whether the preedit string is currently visible. */

   int              cursor_x; /**< X-coordinate of the cursor relative to the client canvas. */
   int              cursor_y; /**< Y-coordinate of the cursor relative to the client canvas. */
   int              cursor_w; /**< Width of the cursor area. */
   int              cursor_h; /**< Height of the cursor area. */

   Eina_Bool        has_focus; /**< Whether this context currently has input focus. */

   Ecore_X_Window   client_window; /**< The client X11 window. */
   Evas            *client_canvas; /**< The Evas canvas associated with the input. */

   int              caps; /**< IBus capabilities flags (e.g., IBUS_CAP_PREEDIT_TEXT). */
};

/**
 * @struct _KeyEvent
 * @brief Structure to hold key event details for asynchronous processing.
 *
 * Used to pass key event information to the GAsyncResult callback when
 * processing key events asynchronously with IBus.
 */
typedef struct _KeyEvent KeyEvent;

struct _KeyEvent
{
   int keysym;  /**< The X11 keysym of the event. */
   int keycode; /**< The X11 keycode of the event. */
   int state;   /**< The X11 modifier state of the event. */
};

/** @brief Global flag to determine if synchronous IBus mode should be used. Defaults to EINA_TRUE. */
static Eina_Bool _sync_mode_use = EINA_TRUE;

/** @brief Pointer to the Ecore_IMF_Context that currently has focus. */
static Ecore_IMF_Context *_focus_im_context = NULL;
/** @brief Global IBusBus object, representing the connection to the IBus daemon. */
static IBusBus           *_bus = NULL;

/* functions prototype */
/* static methods*/
static void _ecore_imf_context_ibus_create(IBusIMContext      *context);
static void _ecore_imf_context_ibus_cursor_location_set(Ecore_IMF_Context  *ctx);
static void _ecore_imf_context_ibus_bus_connected_cb(IBusBus *bus, IBusIMContext *context);
static XKeyEvent _ecore_imf_ibus_x_key_event_generate(Window win,
                                                      Eina_Bool press,
                                                      int keysym,
                                                      int keycode,
                                                      int modifiers);

/**
 * @brief Converts a UTF-8 character offset to a byte index.
 *
 * IBus often works with character offsets, while string manipulations in C
 * often require byte indices. This function performs the conversion.
 *
 * @param str The UTF-8 string.
 * @param offset The character offset.
 * @return The byte index corresponding to the character offset.
 */
static unsigned int
utf8_offset_to_index(const char *str, int offset)
{
   int index = 0;
   int i;
   for (i = 0; i < offset; i++)
     eina_unicode_utf8_next_get(str, &index);

   return index;
}

/**
 * @brief Comparison function for sorting Ecore_IMF_Preedit_Attr by start_index.
 *
 * Used with eina_list_sort to ensure preedit attributes are ordered correctly.
 *
 * @param d1 Pointer to the first Ecore_IMF_Preedit_Attr.
 * @param d2 Pointer to the second Ecore_IMF_Preedit_Attr.
 * @return -1 if attr1 starts before attr2, 1 otherwise.
 */
static int
sort_cb(const void *d1, const void *d2)
{
   const Ecore_IMF_Preedit_Attr *attr1 = d1;
   const Ecore_IMF_Preedit_Attr *attr2 = d2;

   if (!attr1) return 1;
   if (!attr2) return -1;

   if (attr1->start_index < attr2->start_index)
     return -1;
   else
     return 1;
}

/**
 * @brief Gets the screen coordinates of a given X11 window.
 *
 * This function traverses the X11 window hierarchy from the client window
 * up to the root window to calculate its absolute screen position.
 *
 * @param client_win The Ecore_X_Window whose screen coordinates are needed.
 * @param x Pointer to store the resulting screen x-coordinate. Can be NULL.
 * @param y Pointer to store the resulting screen y-coordinate. Can be NULL.
 */
static void
_ecore_imf_ibus_window_to_screen_geometry_get(Ecore_X_Window client_win,
                                              int *x,
                                              int *y)
{
   Ecore_X_Window root_window, win;
   int win_x, win_y;
   int sum_x = 0, sum_y = 0;

   if (!ecore_x_display_get()) goto end;

   root_window = ecore_x_window_root_get(client_win);
   win = client_win;

   while (root_window != win)
     {
        ecore_x_window_geometry_get(win, &win_x, &win_y, NULL, NULL);
        sum_x += win_x;
        sum_y += win_y;
        win = ecore_x_window_parent_get(win);
     }

end:
   if (x)
     *x = sum_x;
   if (y)
     *y = sum_y;
}

/**
 * @brief Converts Ecore IMF keyboard modifiers to IBus modifier flags.
 * @param modifier The Ecore IMF modifier flags (e.g., ECORE_IMF_KEYBOARD_MODIFIER_CTRL).
 * @return The corresponding IBus modifier flags (e.g., IBUS_CONTROL_MASK).
 */
static unsigned int
_ecore_imf_modifier_to_ibus_modifier(unsigned int modifier)
{
   unsigned int state = 0;

   /**< "Control" is pressed */
   if (modifier & ECORE_IMF_KEYBOARD_MODIFIER_CTRL)
     state |= IBUS_CONTROL_MASK;

   /**< "Alt" is pressed */
   if (modifier & ECORE_IMF_KEYBOARD_MODIFIER_ALT)
     state |= IBUS_MOD1_MASK;

   /**< "Shift" is pressed */
   if (modifier & ECORE_IMF_KEYBOARD_MODIFIER_SHIFT)
     state |= IBUS_SHIFT_MASK;

   /**< "Win" (between "Ctrl" and "Alt") */
   if (modifier & ECORE_IMF_KEYBOARD_MODIFIER_WIN)
     state |= IBUS_SUPER_MASK;

   /**< "AltGr" is pressed */
   if (modifier & ECORE_IMF_KEYBOARD_MODIFIER_ALTGR)
     state |= IBUS_MOD5_MASK;

   return state;
}

/**
 * @brief Converts Ecore IMF keyboard lock states to IBus modifier flags.
 * @param locks The Ecore IMF lock flags (e.g., ECORE_IMF_KEYBOARD_LOCK_CAPS).
 * @return The corresponding IBus modifier flags (e.g., IBUS_LOCK_MASK).
 */
static unsigned int
_ecore_imf_locks_to_ibus_modifier(unsigned int locks)
{
   unsigned int state = 0;

   /**< "Num lock" is pressed */
   if (locks & ECORE_IMF_KEYBOARD_LOCK_NUM)
     state |= IBUS_MOD2_MASK;

   /**< "Caps lock" is pressed */
   if (locks & ECORE_IMF_KEYBOARD_LOCK_CAPS)
     state |= IBUS_LOCK_MASK;

   return state;
}

/**
 * @brief Sends a synthesized XKeyEvent to the currently focused X11 window.
 *
 * This is used when IBus does not consume a key event, and it needs to be
 * forwarded to the application.
 *
 * @param keysym The X11 keysym of the key.
 * @param keycode The X11 keycode of the key.
 * @param state The X11 modifier state (including IBUS_RELEASE_MASK if it's a key release).
 */
static void
_ecore_imf_ibus_key_event_put(int keysym, int keycode, int state)
{
   // Find the window which has the current keyboard focus.
   Window winFocus = 0;
   int revert = RevertToParent;

   if (!ecore_x_display_get()) return;
   XGetInputFocus(ecore_x_display_get(), &winFocus, &revert);

   XKeyEvent event;
   if (state & IBUS_RELEASE_MASK)
     {
        event = _ecore_imf_ibus_x_key_event_generate(winFocus,
                                                     EINA_FALSE,
                                                     keysym,
                                                     keycode,
                                                     state);
        XSendEvent(event.display, event.window, True, KeyReleaseMask, (XEvent *)&event);
     }
   else
     {
        event = _ecore_imf_ibus_x_key_event_generate(winFocus,
                                                     EINA_TRUE,
                                                     keysym,
                                                     keycode,
                                                     state);
        XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
     }
}

/**
 * @brief Creates a copy of key event data.
 *
 * Allocates and populates a KeyEvent structure, used for asynchronous
 * key event processing.
 *
 * @param keysym The X11 keysym.
 * @param keycode The X11 keycode.
 * @param state The X11 modifier state.
 * @return A newly allocated KeyEvent structure containing the event data.
 *         The caller is responsible for freeing this structure.
 */
static KeyEvent *
_ecore_imf_ibus_key_event_copy(int keysym, int keycode, int state)
{
   KeyEvent *kev = calloc(1, sizeof(KeyEvent));
   kev->keysym = keysym;
   kev->keycode = keycode;
   kev->state = state;

   return kev;
}

/**
 * @brief Callback for asynchronous key event processing completion.
 *
 * This function is called when ibus_input_context_process_key_event_async()
 * finishes. It checks if IBus handled the event; if not, it forwards
 * the event to the application using _ecore_imf_ibus_key_event_put().
 *
 * @param object The IBusInputContext on which the async operation was called.
 * @param res The GAsyncResult of the operation.
 * @param user_data A pointer to the KeyEvent data associated with this event.
 */
static void
_ecore_imf_ibus_process_key_event_done(GObject      *object,
                                       GAsyncResult *res,
                                       gpointer      user_data)
{
   IBusInputContext *context = (IBusInputContext *)object;
   KeyEvent *event = (KeyEvent *)user_data;

   GError *error = NULL;
   Eina_Bool retval = ibus_input_context_process_key_event_async_finish(context,
                                                                        res,
                                                                        &error);

   if (error != NULL)
     {
        g_warning("Process Key Event failed: %s.", error->message);
        g_error_free(error);
     }

   if (retval == EINA_FALSE)
     {
        _ecore_imf_ibus_key_event_put(event->keysym,
                                      event->keycode,
                                      event->state);
     }
   free(event);
}

/**
 * @brief Requests surrounding text from the application if IBus needs it.
 *
 * Some input methods require context (the text around the cursor) to function
 * correctly. This function checks if IBus needs surrounding text and, if so,
 * retrieves it from the Ecore IMF context and provides it to IBus.
 *
 * @param ibusimcontext The IBusIMContext.
 */
static void
_request_surrounding_text(IBusIMContext *ibusimcontext)
{
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext->ibuscontext);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext->ctx);

   if ((ibusimcontext->caps & IBUS_CAP_SURROUNDING_TEXT) != 0 &&
       ibus_input_context_needs_surrounding_text(ibusimcontext->ibuscontext))
     {
        char *surrounding = NULL;
        int cursor_pos;
        IBusText *ibustext;

        EINA_LOG_DBG ("requesting surrounding text...\n");

        if (ecore_imf_context_surrounding_get(ibusimcontext->ctx,
                                              &surrounding,
                                              &cursor_pos))
          {
             if (!surrounding)
               return;

             if (cursor_pos < 0)
               {
                  free(surrounding);
                  return;
               }

             ibustext = ibus_text_new_from_string(surrounding);

             ibus_input_context_set_surrounding_text(ibusimcontext->ibuscontext,
                                                     ibustext,
                                                     cursor_pos,
                                                     cursor_pos);

             free(surrounding);
          }
        else
          {
             ibusimcontext->caps &= ~IBUS_CAP_SURROUNDING_TEXT;
             ibus_input_context_set_capabilities(ibusimcontext->ibuscontext,
                                                 ibusimcontext->caps);
          }
     }
}

/**
 * @brief Creates a new IBusIMContext structure.
 *
 * Initializes an IBusIMContext structure and establishes a connection to the
 * IBus daemon if one doesn't already exist.
 *
 * @return A pointer to the newly allocated IBusIMContext, or NULL on failure.
 */
IBusIMContext *
ecore_imf_context_ibus_new(void)
{
   EINA_LOG_DBG("%s", __func__);

   IBusIMContext *context = calloc(1, sizeof(IBusIMContext));

   /* init bus object */
   if (_bus == NULL)
     {
        char *display_name = NULL;

        if ((display_name = getenv("DISPLAY")))
          ibus_set_display(display_name);
        else
          ibus_set_display(":0.0");

        _bus = ibus_bus_new();
     }

   return context;
}

/**
 * @brief Shuts down the global IBus connection.
 *
 * Releases the global IBusBus object. This should be called when the
 * application is exiting.
 */
void
ecore_imf_context_ibus_shutdown(void)
{
   if (_bus)
     {
        g_object_unref(_bus);
        _bus = NULL;
     }
}

/**
 * @brief Initializes an IBusIMContext for a given Ecore_IMF_Context.
 *
 * Sets up the IBus-specific data, initializes preedit status, cursor area,
 * and connects to the IBus "connected" signal to create the IBus input
 * context once the bus is ready.
 *
 * @param ctx The Ecore_IMF_Context to associate with IBus.
 */
void
ecore_imf_context_ibus_add(Ecore_IMF_Context *ctx)
{
   EINA_LOG_DBG("%s", __func__);

   char *s = NULL;
   IBusIMContext *ibusimcontext = (IBusIMContext *)ecore_imf_context_data_get(ctx);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   ibusimcontext->client_window = 0;

   // Init preedit status
   ibusimcontext->preedit_string = NULL;
   ibusimcontext->preedit_attrs = NULL;
   ibusimcontext->preedit_cursor_pos = 0;
   ibusimcontext->preedit_visible = EINA_FALSE;

   // Init cursor area
   ibusimcontext->cursor_x = -1;
   ibusimcontext->cursor_y = -1;
   ibusimcontext->cursor_w = 0;
   ibusimcontext->cursor_h = 0;

   ibusimcontext->ibuscontext = NULL;
   ibusimcontext->has_focus = EINA_FALSE;
   ibusimcontext->caps = IBUS_CAP_PREEDIT_TEXT | IBUS_CAP_FOCUS | IBUS_CAP_SURROUNDING_TEXT;;
   ibusimcontext->ctx = ctx;

   s = getenv("IBUS_ENABLE_SYNC_MODE");
   if (s)
     _sync_mode_use = !!atoi(s);

   if (ibus_bus_is_connected(_bus))
     _ecore_imf_context_ibus_create(ibusimcontext);

   g_signal_connect(_bus, "connected", G_CALLBACK (_ecore_imf_context_ibus_bus_connected_cb), ibusimcontext);
}

/**
 * @brief Cleans up and destroys an IBusIMContext.
 *
 * Disconnects signals, destroys the IBus input context proxy, frees preedit
 * string and attributes, and frees the IBusIMContext structure itself.
 *
 * @param ctx The Ecore_IMF_Context whose IBus data is to be deleted.
 */
void
ecore_imf_context_ibus_del(Ecore_IMF_Context *ctx)
{
   EINA_LOG_DBG("%s", __func__);

   IBusIMContext *ibusimcontext = (IBusIMContext*)ecore_imf_context_data_get(ctx);
   Ecore_IMF_Preedit_Attr *attr = NULL;

   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   g_signal_handlers_disconnect_by_func(_bus,
                                        G_CALLBACK(_ecore_imf_context_ibus_bus_connected_cb),
                                        ibusimcontext);

   if (ibusimcontext->ibuscontext)
     ibus_proxy_destroy((IBusProxy *)ibusimcontext->ibuscontext);

   // release preedit
   if (ibusimcontext->preedit_string)
     free(ibusimcontext->preedit_string);
   ibusimcontext->preedit_string = NULL;

   if (ibusimcontext->preedit_attrs)
     {
        EINA_LIST_FREE(ibusimcontext->preedit_attrs, attr)
           free(attr);
     }

   if (_focus_im_context == ctx)
     _focus_im_context = NULL;

   free(ibusimcontext);
}

/**
 * @brief Filters key events through the IBus input context.
 *
 * Converts Ecore IMF key events to IBus key events and sends them to the
 * IBus input context for processing. This can be done synchronously or
 * asynchronously based on the _sync_mode_use flag.
 *
 * @param ctx The Ecore_IMF_Context.
 * @param type The type of Ecore IMF event (ECORE_IMF_EVENT_KEY_UP or ECORE_IMF_EVENT_KEY_DOWN).
 * @param event The Ecore_IMF_Event data.
 * @return EINA_TRUE if IBus handled (consumed) the event, EINA_FALSE otherwise.
 */
Eina_Bool
ecore_imf_context_ibus_filter_event(Ecore_IMF_Context *ctx,
                                    Ecore_IMF_Event_Type type,
                                    Ecore_IMF_Event *event)
{
   IBusIMContext *ibusimcontext = (IBusIMContext*)ecore_imf_context_data_get(ctx);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ibusimcontext, EINA_FALSE);

   if (!ecore_x_display_get()) return EINA_FALSE;
   if (type != ECORE_IMF_EVENT_KEY_UP && type != ECORE_IMF_EVENT_KEY_DOWN)
     return EINA_FALSE;

   EINA_LOG_DBG("%s", __func__);

   if (G_LIKELY(ibusimcontext->ibuscontext && ibusimcontext->has_focus))
     {
        /* If context does not have focus, ibus will process key event in sync mode.
         * It is a workaround for increase search in treeview.
         */
        Eina_Bool retval = EINA_FALSE;
        int keycode;
        int keysym;
        unsigned int state = 0;

        if (type == ECORE_IMF_EVENT_KEY_UP)
          {
             Ecore_IMF_Event_Key_Up *ev = (Ecore_IMF_Event_Key_Up *)event;
             if (ev->timestamp == 0)
               return EINA_FALSE;

             keycode = ecore_x_keysym_keycode_get(ev->keyname);
             keysym = XStringToKeysym(ev->key);
             state = _ecore_imf_modifier_to_ibus_modifier(ev->modifiers) |
                     _ecore_imf_locks_to_ibus_modifier(ev->locks) | IBUS_RELEASE_MASK;

             if (_sync_mode_use)
               {
                  retval = ibus_input_context_process_key_event(ibusimcontext->ibuscontext,
                                                                keysym,
                                                                keycode - 8,
                                                                state);
               }
             else
               {
                  ibus_input_context_process_key_event_async(ibusimcontext->ibuscontext,
                                                             keysym,
                                                             keycode - 8,
                                                             state,
                                                             -1,
                                                             NULL,
                                                             _ecore_imf_ibus_process_key_event_done,
                                                             _ecore_imf_ibus_key_event_copy(keysym, keycode, state));
                  retval = EINA_TRUE;
               }
          }
        else if (type == ECORE_IMF_EVENT_KEY_DOWN)
          {
             Ecore_IMF_Event_Key_Down *ev = (Ecore_IMF_Event_Key_Down *)event;
             if (ev->timestamp == 0)
               return EINA_FALSE;

             _request_surrounding_text(ibusimcontext);

             keycode = ecore_x_keysym_keycode_get(ev->keyname);
             keysym = XStringToKeysym(ev->key);
             state = _ecore_imf_modifier_to_ibus_modifier(ev->modifiers) |
                     _ecore_imf_locks_to_ibus_modifier(ev->locks);

             if (_sync_mode_use)
               {
                  retval = ibus_input_context_process_key_event(ibusimcontext->ibuscontext,
                                                                keysym,
                                                                keycode - 8,
                                                                state);
               }
             else
               {
                  ibus_input_context_process_key_event_async(ibusimcontext->ibuscontext,
                                                             keysym,
                                                             keycode - 8,
                                                             state,
                                                             -1,
                                                             NULL,
                                                             _ecore_imf_ibus_process_key_event_done,
                                                             _ecore_imf_ibus_key_event_copy(keysym, keycode, state));
                  retval = EINA_TRUE;
               }
          }

        if (retval)
          return EINA_TRUE;
        else
          return EINA_FALSE;
     }
   else
     return EINA_FALSE;
}

/**
 * @brief Handles the focus-in event for the IBus context.
 *
 * Notifies the IBus input context that it has gained focus. Also requests
 * surrounding text if needed. Manages the global _focus_im_context.
 *
 * @param ctx The Ecore_IMF_Context that gained focus.
 */
void
ecore_imf_context_ibus_focus_in(Ecore_IMF_Context *ctx)
{
   EINA_LOG_DBG("ctx : %p", ctx);

   IBusIMContext *ibusimcontext = (IBusIMContext*)ecore_imf_context_data_get(ctx);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   if (ibusimcontext->has_focus)
     return;

   if (_focus_im_context != NULL)
     ecore_imf_context_focus_out(_focus_im_context);

   ibusimcontext->has_focus = EINA_TRUE;
   if (ibusimcontext->ibuscontext)
     ibus_input_context_focus_in(ibusimcontext->ibuscontext);

   _request_surrounding_text(ibusimcontext);

   if (_focus_im_context != ctx)
     _focus_im_context = ctx;
}

/**
 * @brief Handles the focus-out event for the IBus context.
 *
 * Notifies the IBus input context that it has lost focus. Updates the
 * global _focus_im_context.
 *
 * @param ctx The Ecore_IMF_Context that lost focus.
 */
void
ecore_imf_context_ibus_focus_out(Ecore_IMF_Context *ctx)
{
   EINA_LOG_DBG("ctx : %p", ctx);

   IBusIMContext *ibusimcontext = (IBusIMContext *)ecore_imf_context_data_get(ctx);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   if (ibusimcontext->has_focus == EINA_FALSE)
     return;

   if (_focus_im_context == ctx)
     _focus_im_context = NULL;

   ibusimcontext->has_focus = EINA_FALSE;
   if (ibusimcontext->ibuscontext)
     ibus_input_context_focus_out(ibusimcontext->ibuscontext);
}

/**
 * @brief Resets the IBus input context.
 *
 * Tells the IBus input context to clear its current state (e.g., preedit string).
 *
 * @param ctx The Ecore_IMF_Context to reset.
 */
void
ecore_imf_context_ibus_reset(Ecore_IMF_Context *ctx)
{
   IBusIMContext *ibusimcontext = (IBusIMContext*)ecore_imf_context_data_get(ctx);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   if (ibusimcontext->ibuscontext)
     ibus_input_context_reset(ibusimcontext->ibuscontext);
}

/**
 * @brief Retrieves the current preedit string and cursor position.
 *
 * Provides the application with the current preedit text being composed by
 * the input method.
 *
 * @param ctx The Ecore_IMF_Context.
 * @param[out] str Pointer to a char* that will be allocated and filled with the
 *                 preedit string. The caller must free this string.
 * @param[out] cursor_pos Pointer to an int that will be filled with the byte offset
 *                        of the cursor within the preedit string.
 */
void
ecore_imf_context_ibus_preedit_string_get(Ecore_IMF_Context *ctx,
                                          char          **str,
                                          int            *cursor_pos)
{
   IBusIMContext *ibusimcontext = (IBusIMContext*)ecore_imf_context_data_get(ctx);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   if (ibusimcontext->preedit_visible)
     {
        if (str)
          *str = strdup(ibusimcontext->preedit_string ? ibusimcontext->preedit_string: "");

        if (cursor_pos)
          *cursor_pos = ibusimcontext->preedit_cursor_pos;
     }
   else
     {
        if (str)
          *str = strdup("");

        if (cursor_pos)
          *cursor_pos = 0;
     }

   if (str)
     EINA_LOG_DBG("str : %s", *str);

   if (cursor_pos)
     EINA_LOG_DBG("cursor_pos : %d", *cursor_pos);
}

/**
 * @brief Retrieves the current preedit string, attributes, and cursor position.
 *
 * Provides the application with the current preedit text and a list of
 * attributes (e.g., underlining) that describe how to render it.
 *
 * @param ctx The Ecore_IMF_Context.
 * @param[out] str Pointer to a char* that will be allocated and filled with the
 *                 preedit string. The caller must free this string.
 * @param[out] attrs Pointer to an Eina_List* that will be filled with
 *                   Ecore_IMF_Preedit_Attr elements. The caller must free this
 *                   list and its contents.
 *                   Example of Ecore_IMF_Preedit_Attr structure:
 *                   struct _Ecore_IMF_Preedit_Attr {
 *                       Ecore_IMF_Preedit_Type preedit_type; // e.g., ECORE_IMF_PREEDIT_TYPE_SUB1 (underline)
 *                       unsigned int           start_index;  // Start character index of the attribute
 *                       unsigned int           end_index;    // End character index of the attribute
 *                   };
 * @param[out] cursor_pos Pointer to an int that will be filled with the byte offset
 *                        of the cursor within the preedit string.
 */
void
ecore_imf_context_ibus_preedit_string_with_attributes_get(Ecore_IMF_Context *ctx,
                                                          char          **str,
                                                          Eina_List     **attrs,
                                                          int            *cursor_pos)
{
   IBusIMContext *ibusimcontext = (IBusIMContext*)ecore_imf_context_data_get(ctx);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);
   Eina_List *l;
   Ecore_IMF_Preedit_Attr *attr1 = NULL, *attr2 = NULL;

   ecore_imf_context_ibus_preedit_string_get(ctx, str, cursor_pos);

   if (attrs)
     {
        if (ibusimcontext->preedit_attrs)
          {
             EINA_LIST_FOREACH(ibusimcontext->preedit_attrs, l, attr1)
               {
                  attr2 = (Ecore_IMF_Preedit_Attr *)calloc(1, sizeof(Ecore_IMF_Preedit_Attr));
                  if (!attr2) continue;
                  attr2->preedit_type = attr1->preedit_type;
                  attr2->start_index = attr1->start_index;
                  attr2->end_index = attr1->end_index;

                  *attrs = eina_list_append(*attrs, (void *)attr2);
               }
          }
        else
          *attrs = NULL;
     }
}

/**
 * @brief Sets the client window associated with the IBus context.
 *
 * The client window is typically the top-level window containing the input widget.
 * IBus uses this for various purposes, like positioning candidate windows.
 *
 * @param ctx The Ecore_IMF_Context.
 * @param window A pointer to the client window (expected to be an Ecore_X_Window).
 */
void
ecore_imf_context_ibus_client_window_set(Ecore_IMF_Context *ctx, void *window)
{
   EINA_LOG_DBG("canvas : %p", window);
   IBusIMContext *ibusimcontext = (IBusIMContext *)ecore_imf_context_data_get(ctx);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   if (window != NULL)
     ibusimcontext->client_window = (Ecore_X_Window)(Ecore_Window)window;
}

/**
 * @brief Sets the client Evas canvas associated with the IBus context.
 *
 * The client canvas is used to determine the screen position of the input area
 * if a specific client window is not set or to get window from Evas.
 *
 * @param ctx The Ecore_IMF_Context.
 * @param canvas A pointer to the client Evas canvas.
 */
void
ecore_imf_context_ibus_client_canvas_set(Ecore_IMF_Context *ctx, void *canvas)
{
   EINA_LOG_DBG("canvas : %p", canvas);
   IBusIMContext *ibusimcontext = (IBusIMContext *)ecore_imf_context_data_get(ctx);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   if (canvas != NULL)
     ibusimcontext->client_canvas = canvas;
}

/**
 * @brief Updates the IBus input context with the current cursor location.
 *
 * Calculates the absolute screen coordinates of the cursor based on the
 * client window/canvas and the relative cursor position, then informs IBus.
 * This is typically called internally when the cursor position changes.
 *
 * @param ctx The Ecore_IMF_Context.
 */
static void
_ecore_imf_context_ibus_cursor_location_set(Ecore_IMF_Context *ctx)
{
   IBusIMContext *ibusimcontext = (IBusIMContext *)ecore_imf_context_data_get(ctx);
   Ecore_Evas *ee;
   int canvas_x, canvas_y;
   Ecore_X_Window client_window = 0;

   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   if (ibusimcontext->ibuscontext == NULL)
     return;

   if (ibusimcontext->client_window)
     client_window = ibusimcontext->client_window;
   else
     {
        if (ibusimcontext->client_canvas)
          {
             ee = ecore_evas_ecore_evas_get(ibusimcontext->client_canvas);
             if (ee)
               client_window = (Ecore_X_Window)ecore_evas_window_get(ee);
          }
     }

   _ecore_imf_ibus_window_to_screen_geometry_get(client_window,
                                                 &canvas_x,
                                                 &canvas_y);

   ibus_input_context_set_cursor_location(ibusimcontext->ibuscontext,
                                          ibusimcontext->cursor_x + canvas_x,
                                          ibusimcontext->cursor_y + canvas_y,
                                          ibusimcontext->cursor_w,
                                          ibusimcontext->cursor_h);
}

/**
 * @brief Sets the cursor location relative to the client widget.
 *
 * Informs the IBus context about the position and size of the text cursor
 * within the input widget. This is used by IBus to position helper UIs
 * like candidate lists.
 *
 * @param ctx The Ecore_IMF_Context.
 * @param x The x-coordinate of the cursor rectangle, relative to the client widget.
 * @param y The y-coordinate of the cursor rectangle, relative to the client widget.
 * @param w The width of the cursor rectangle.
 * @param h The height of the cursor rectangle.
 */
void
ecore_imf_context_ibus_cursor_location_set(Ecore_IMF_Context *ctx,
                                           int x,
                                           int y,
                                           int w,
                                           int h)
{
   EINA_LOG_DBG("x : %d, y : %d, w, %d, h :%d", x, y, w, h);
   IBusIMContext *ibusimcontext = (IBusIMContext *)ecore_imf_context_data_get(ctx);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   if (ibusimcontext->cursor_x != x ||
       ibusimcontext->cursor_y != y ||
       ibusimcontext->cursor_w != w ||
       ibusimcontext->cursor_h != h)
     {
        ibusimcontext->cursor_x = x;
        ibusimcontext->cursor_y = y;
        ibusimcontext->cursor_w = w;
        ibusimcontext->cursor_h = h;

        _ecore_imf_context_ibus_cursor_location_set(ctx);
     }
}

/**
 * @brief Enables or disables the use of preedit text for the IBus context.
 *
 * Informs IBus whether the application can display preedit text. If disabled,
 * IBus might use an alternative way to show preedit (e.g., a separate window).
 *
 * @param ctx The Ecore_IMF_Context.
 * @param use_preedit EINA_TRUE if the application handles preedit display,
 *                    EINA_FALSE otherwise.
 */
void
ecore_imf_context_ibus_use_preedit_set(Ecore_IMF_Context *ctx, Eina_Bool use_preedit)
{
   EINA_LOG_DBG("preedit : %d", use_preedit);
   IBusIMContext *ibusimcontext = (IBusIMContext *)ecore_imf_context_data_get(ctx);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   if (ibusimcontext->ibuscontext)
     {
        if (use_preedit)
          ibusimcontext->caps |= IBUS_CAP_PREEDIT_TEXT;
        else
          ibusimcontext->caps &= ~IBUS_CAP_PREEDIT_TEXT;

        ibus_input_context_set_capabilities(ibusimcontext->ibuscontext, ibusimcontext->caps);
     }
}

/**
 * @brief Callback invoked when the IBus bus (connection to daemon) is established.
 *
 * When the connection to the IBus daemon is successfully made, this function
 * proceeds to create the actual IBus input context using
 * _ecore_imf_context_ibus_create().
 *
 * @param bus The IBusBus object (unused in this function).
 * @param ibusimcontext The IBusIMContext associated with this connection.
 */
static void
_ecore_imf_context_ibus_bus_connected_cb(IBusBus       *bus EINA_UNUSED,
                                         IBusIMContext *ibusimcontext)
{
   EINA_LOG_DBG("ibus is connected");

   if (ibusimcontext)
     _ecore_imf_context_ibus_create(ibusimcontext);
}

/**
 * @brief Callback invoked when IBus commits text.
 *
 * This function is called when the input method finalizes a string (e.g.,
 * after preediting). It sends an ECORE_IMF_CALLBACK_COMMIT event to the
 * application with the committed string.
 *
 * @param ibuscontext The IBusInputContext that emitted the signal (unused).
 * @param text The IBusText object containing the text to be committed.
 * @param ibusimcontext The IBusIMContext associated with this event.
 */
static void
_ecore_imf_context_ibus_commit_text_cb(IBusInputContext *ibuscontext EINA_UNUSED,
                                       IBusText         *text,
                                       IBusIMContext    *ibusimcontext)
{
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);
   EINA_SAFETY_ON_NULL_RETURN(text);
   char *commit_str = text->text ? text->text : "";

   EINA_LOG_DBG("commit string : %s", commit_str);

   if (ibusimcontext->ctx)
     {
        ecore_imf_context_event_callback_call(ibusimcontext->ctx,
                                              ECORE_IMF_CALLBACK_COMMIT,
                                              (void *)commit_str);

        _request_surrounding_text(ibusimcontext);
     }
}

/**
 * @brief Generates an XKeyEvent structure.
 *
 * Helper function to create an XKeyEvent, which can then be sent to an
 * X11 window. This is used for forwarding key events not handled by IBus.
 *
 * @param win The target X11 window for the event.
 * @param press EINA_TRUE for KeyPress, EINA_FALSE for KeyRelease.
 * @param keysym The X11 keysym.
 * @param keycode The X11 keycode. If -1, it's derived from keysym.
 * @param modifiers The X11 modifier state.
 * @return The generated XKeyEvent structure.
 */
static XKeyEvent _ecore_imf_ibus_x_key_event_generate(Window win,
                                                      Eina_Bool press,
                                                      int keysym,
                                                      int keycode,
                                                      int modifiers)
{
   XKeyEvent event;
   Display *display = ecore_x_display_get();

   event.display     = display;
   event.window      = win;
   event.root        = display ? ecore_x_window_root_get(win) : 0;
   event.subwindow   = None;
   event.time        = 0;
   event.x           = 1;
   event.y           = 1;
   event.x_root      = 1;
   event.y_root      = 1;
   event.same_screen = EINA_TRUE;
   if (keycode == -1)
     {
        event.keycode     = display ? XKeysymToKeycode(display, keysym) : 0;
        event.state       = 0;
     }
   else
     {
        event.keycode     = keycode;
        event.state       = modifiers;
     }
   if (press)
     event.type = KeyPress;
   else
     event.type = KeyRelease;
   event.send_event  = EINA_FALSE;
   event.serial = 0;

   return event;
}

/**
 * @brief Callback invoked when IBus decides to forward a key event.
 *
 * This means IBus did not consume the key event and it should be passed
 * on to the application. This function uses _ecore_imf_ibus_key_event_put()
 * to send the event.
 *
 * @param ibuscontext The IBusInputContext (unused).
 * @param keyval The keysym of the event.
 * @param state The IBus modifier state of the event.
 * @param ibusimcontext The IBusIMContext (unused).
 */
static void
_ecore_imf_context_ibus_forward_key_event_cb(IBusInputContext  *ibuscontext EINA_UNUSED,
                                             guint              keyval,
                                             guint              state,
                                             IBusIMContext     *ibusimcontext EINA_UNUSED)
{
   EINA_LOG_DBG("keyval : %d, state : %d", keyval, state);

   _ecore_imf_ibus_key_event_put(keyval, -1, state);
}

/**
 * @brief Callback invoked when IBus requests deletion of surrounding text.
 *
 * Some input methods need to delete text around the cursor (e.g., to replace
 * it with a candidate). This function sends an
 * ECORE_IMF_CALLBACK_DELETE_SURROUNDING event to the application.
 *
 * @param ibuscontext The IBusInputContext (unused).
 * @param offset_from_cursor The character offset from the cursor where deletion
 *                           should start. Negative values mean before the cursor.
 * @param nchars The number of characters to delete.
 * @param ibusimcontext The IBusIMContext associated with this event.
 */
static void
_ecore_imf_context_ibus_delete_surrounding_text_cb(IBusInputContext *ibuscontext EINA_UNUSED,
                                                   gint              offset_from_cursor,
                                                   guint             nchars,
                                                   IBusIMContext    *ibusimcontext)
{
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   if (_focus_im_context != ibusimcontext->ctx)
     return;

   Ecore_IMF_Event_Delete_Surrounding ev;
   ev.ctx = _focus_im_context;
   ev.n_chars = nchars;
   ev.offset = offset_from_cursor;
   ecore_imf_context_event_callback_call(_focus_im_context,
                                         ECORE_IMF_CALLBACK_DELETE_SURROUNDING,
                                         &ev);
}

/**
 * @brief Callback invoked when IBus updates the preedit text.
 *
 * This function is called when the preedit string, its attributes, cursor
 * position, or visibility changes. It updates the internal preedit state
 * in IBusIMContext and sends ECORE_IMF_CALLBACK_PREEDIT_START,
 * ECORE_IMF_CALLBACK_PREEDIT_CHANGED, or ECORE_IMF_CALLBACK_PREEDIT_END
 * events to the application as appropriate.
 *
 * @param ibuscontext The IBusInputContext (unused).
 * @param text The IBusText object containing the updated preedit string and attributes.
 * @param cursor_pos The new cursor position within the preedit string (character offset).
 * @param visible Whether the preedit text should be visible.
 * @param ibusimcontext The IBusIMContext associated with this event.
 */
static void
_ecore_imf_context_ibus_update_preedit_text_cb(IBusInputContext  *ibuscontext EINA_UNUSED,
                                               IBusText          *text,
                                               gint               cursor_pos,
                                               gboolean           visible,
                                               IBusIMContext     *ibusimcontext)
{
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);
   EINA_SAFETY_ON_NULL_RETURN(text);

   const char *str;
   gboolean flag;
   Ecore_IMF_Preedit_Attr *attr = NULL;

   if (ibusimcontext->preedit_string)
     free(ibusimcontext->preedit_string);

   if (ibusimcontext->preedit_attrs)
     {
        EINA_LIST_FREE(ibusimcontext->preedit_attrs, attr)
           free(attr);

        ibusimcontext->preedit_attrs = NULL;
     }

   str = text->text;

   if (str)
     ibusimcontext->preedit_string = strdup(str);
   else
     ibusimcontext->preedit_string = strdup("");

   if (text->attrs)
     {
        unsigned int i;
        unsigned int pos;
        unsigned int preedit_length;
        preedit_length = strlen(ibusimcontext->preedit_string);
        Eina_Bool *attrs_flag = calloc(1, sizeof(Eina_Bool)*preedit_length);

        for (i = 0; ; i++)
          {
             attr = NULL;
             IBusAttribute *ibus_attr = ibus_attr_list_get(text->attrs, i);
             if (ibus_attr == NULL)
               break;

             attr = (Ecore_IMF_Preedit_Attr *)calloc(1, sizeof(Ecore_IMF_Preedit_Attr));
             if (attr == NULL)
               continue;

             attr->start_index = utf8_offset_to_index(ibusimcontext->preedit_string,
                                                      ibus_attr->start_index);
             attr->end_index = utf8_offset_to_index(ibusimcontext->preedit_string,
                                                    ibus_attr->end_index);

             switch (ibus_attr->type)
               {
                case IBUS_ATTR_TYPE_FOREGROUND:
                   attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB2;
                   for (pos = attr->start_index; pos < attr->end_index; ++pos)
                     attrs_flag[pos] = 1;
                   break;
                default:
                   if (attr)
                     {
                        free(attr);
                        attr = NULL;
                     }
                   continue;
               }

             if (attr)
               ibusimcontext->preedit_attrs = eina_list_append(ibusimcontext->preedit_attrs,
                                                               (void *)attr);
          }

        // Add underline for all characters which don't have attribute.
        for (pos = 0; pos < preedit_length; ++pos)
          {
             if (!attrs_flag[pos])
               {
                  int begin_pos = pos;

                  while (pos < preedit_length && !attrs_flag[pos])
                    ++pos;

                  attr = (Ecore_IMF_Preedit_Attr *)calloc(1, sizeof(Ecore_IMF_Preedit_Attr));
                  if (attr == NULL)
                    continue;
                  attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB1;
                  attr->start_index = begin_pos;
                  attr->end_index = pos;
                  ibusimcontext->preedit_attrs = eina_list_append(ibusimcontext->preedit_attrs,
                                                                  (void *)attr);
               }
          }

        if (attrs_flag)
          free(attrs_flag);

        ibusimcontext->preedit_attrs = eina_list_sort(ibusimcontext->preedit_attrs,
                                                      eina_list_count(ibusimcontext->preedit_attrs),
                                                      sort_cb);
     }

   ibusimcontext->preedit_cursor_pos = cursor_pos;

   EINA_LOG_DBG("string : %s, cursor : %d",
                ibusimcontext->preedit_string,
                ibusimcontext->preedit_cursor_pos);

   flag = ibusimcontext->preedit_visible != visible;
   ibusimcontext->preedit_visible = visible;

   if (ibusimcontext->preedit_visible)
     {
        if (flag)
          {
             ecore_imf_context_event_callback_call(ibusimcontext->ctx,
                                                   ECORE_IMF_CALLBACK_PREEDIT_START,
                                                   NULL);
          }

        ecore_imf_context_event_callback_call(ibusimcontext->ctx,
                                              ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
                                              NULL);
     }
   else
     {
        if (flag)
          {
             ecore_imf_context_event_callback_call(ibusimcontext->ctx,
                                                   ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
                                                   NULL);
          }

        ecore_imf_context_event_callback_call(ibusimcontext->ctx,
                                              ECORE_IMF_CALLBACK_PREEDIT_END,
                                              NULL);
     }
}

/**
 * @brief Callback invoked when IBus requests to show the preedit text.
 *
 * This typically happens when preedit text becomes active. It updates the
 * visibility state and sends ECORE_IMF_CALLBACK_PREEDIT_START and
 * ECORE_IMF_CALLBACK_PREEDIT_CHANGED events to the application.
 *
 * @param ibuscontext The IBusInputContext (unused).
 * @param ibusimcontext The IBusIMContext associated with this event.
 */
static void
_ecore_imf_context_ibus_show_preedit_text_cb(IBusInputContext *ibuscontext EINA_UNUSED,
                                             IBusIMContext    *ibusimcontext)
{
   EINA_LOG_DBG("preedit visible : %d", ibusimcontext->preedit_visible);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   if (ibusimcontext->preedit_visible == EINA_TRUE)
     return;

   ibusimcontext->preedit_visible = EINA_TRUE;

   // call preedit start
   ecore_imf_context_event_callback_call(ibusimcontext->ctx,
                                         ECORE_IMF_CALLBACK_PREEDIT_START,
                                         NULL);

   // call preedit changed
   ecore_imf_context_event_callback_call(ibusimcontext->ctx,
                                         ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
                                         NULL);

   _request_surrounding_text(ibusimcontext);
}

/**
 * @brief Callback invoked when IBus requests to hide the preedit text.
 *
 * This typically happens when preedit text is cleared or committed. It updates
 * the visibility state and sends ECORE_IMF_CALLBACK_PREEDIT_CHANGED and
 * ECORE_IMF_CALLBACK_PREEDIT_END events to the application.
 *
 * @param ibuscontext The IBusInputContext (unused).
 * @param ibusimcontext The IBusIMContext associated with this event.
 */
static void
_ecore_imf_context_ibus_hide_preedit_text_cb(IBusInputContext *ibuscontext EINA_UNUSED,
                                             IBusIMContext    *ibusimcontext)
{
   EINA_LOG_DBG("%s", __func__);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   if (ibusimcontext->preedit_visible == EINA_FALSE)
     return;

   ibusimcontext->preedit_visible = EINA_FALSE;

   // call preedit changed
   ecore_imf_context_event_callback_call(ibusimcontext->ctx,
                                         ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
                                         NULL);

   // call preedit end
   ecore_imf_context_event_callback_call(ibusimcontext->ctx,
                                         ECORE_IMF_CALLBACK_PREEDIT_END,
                                         NULL);
}

/**
 * @brief Callback invoked when the IBus input context is destroyed on the IBus server side.
 *
 * This function handles the cleanup of the local IBusInputContext proxy,
 * clears preedit state, and notifies the application that preediting has ended.
 *
 * @param ibuscontext The IBusInputContext that was destroyed (unused, as it's being unreffed).
 * @param ibusimcontext The IBusIMContext associated with the destroyed context.
 */
static void
_ecore_imf_context_ibus_destroy_cb(IBusInputContext *ibuscontext EINA_UNUSED,
                                   IBusIMContext    *ibusimcontext)
{
   EINA_LOG_DBG("%s", __func__);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   g_object_unref(ibusimcontext->ibuscontext);
   ibusimcontext->ibuscontext = NULL;

   /* clear preedit */
   ibusimcontext->preedit_visible = EINA_FALSE;
   ibusimcontext->preedit_cursor_pos = 0;
   free(ibusimcontext->preedit_string);
   ibusimcontext->preedit_string = NULL;

   // call preedit changed
   ecore_imf_context_event_callback_call(ibusimcontext->ctx,
                                         ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
                                         NULL);

   // call preedit end
   ecore_imf_context_event_callback_call(ibusimcontext->ctx,
                                         ECORE_IMF_CALLBACK_PREEDIT_END,
                                         NULL);
}

/**
 * @brief Creates and initializes the IBusInputContext object.
 *
 * This function is called once the IBus bus is connected. It creates the
 * IBusInputContext proxy, connects to its various signals (commit-text,
 * forward-key-event, etc.), sets capabilities, and focuses the context if
 * it already has application-level focus.
 *
 * @param ibusimcontext The IBusIMContext for which to create the IBus input context.
 */
static void
_ecore_imf_context_ibus_create(IBusIMContext *ibusimcontext)
{
   EINA_LOG_DBG("%s", __func__);
   EINA_SAFETY_ON_NULL_RETURN(ibusimcontext);

   ibusimcontext->ibuscontext = ibus_bus_create_input_context(_bus, "ecore");

   g_return_if_fail(ibusimcontext->ibuscontext != NULL);

   g_signal_connect(ibusimcontext->ibuscontext,
                    "commit-text",
                    G_CALLBACK (_ecore_imf_context_ibus_commit_text_cb),
                    ibusimcontext);
   g_signal_connect(ibusimcontext->ibuscontext,
                    "forward-key-event",
                    G_CALLBACK (_ecore_imf_context_ibus_forward_key_event_cb),
                    ibusimcontext);
   g_signal_connect(ibusimcontext->ibuscontext,
                    "delete-surrounding-text",
                    G_CALLBACK (_ecore_imf_context_ibus_delete_surrounding_text_cb),
                    ibusimcontext);
   g_signal_connect(ibusimcontext->ibuscontext,
                    "update-preedit-text",
                    G_CALLBACK (_ecore_imf_context_ibus_update_preedit_text_cb),
                    ibusimcontext);
   g_signal_connect(ibusimcontext->ibuscontext,
                    "show-preedit-text",
                    G_CALLBACK (_ecore_imf_context_ibus_show_preedit_text_cb),
                    ibusimcontext);
   g_signal_connect(ibusimcontext->ibuscontext,
                    "hide-preedit-text",
                    G_CALLBACK (_ecore_imf_context_ibus_hide_preedit_text_cb),
                    ibusimcontext);
   g_signal_connect(ibusimcontext->ibuscontext, "destroy",
                    G_CALLBACK (_ecore_imf_context_ibus_destroy_cb),
                    ibusimcontext);

   ibus_input_context_set_capabilities(ibusimcontext->ibuscontext,
                                       ibusimcontext->caps);

   if (ibusimcontext->has_focus)
     ibus_input_context_focus_in(ibusimcontext->ibuscontext);
}
