/*
 * Copyright © 2012, 2013 Intel Corporation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Input.h>

#include "wayland_imcontext.h"

#define HIDE_TIMER_INTERVAL     0.05

static Eina_Bool _clear_hide_timer(void);
/** @internal Timer used to delay hiding the input panel. */
static Ecore_Timer *_hide_timer  = NULL;

/**
 * @internal
 * @brief Internal structure representing a Wayland Input Method Context.
 *
 * This structure holds all the state associated with a Wayland input method context,
 * including Wayland protocol objects, preedit information, cursor state, and pending
 * operations.
 */
struct _WaylandIMContext
{
   Ecore_IMF_Context *ctx; /**< The associated Ecore IMF context. */

   struct zwp_text_input_manager_v1 *text_input_manager; /**< Wayland text input manager. */
   struct zwp_text_input_v1 *text_input; /**< Wayland text input object. */

   Ecore_Wl2_Window *window; /**< The Ecore Wayland window associated with this context. */
   Ecore_Wl2_Input  *input;  /**< The Ecore Wayland input device (seat). */
   Evas             *canvas; /**< The Evas canvas associated with this context. */

   char *preedit_text;     /**< Current preedit string. */
   char *preedit_commit;   /**< Text to commit when preedit is finalized. */
   char *language;         /**< Current input language/locale (e.g., "en_US"). */
   Eina_List *preedit_attrs; /**< List of Ecore_IMF_Preedit_Attr for the current preedit_text. */
   int32_t preedit_cursor; /**< Cursor position within the preedit_text (in characters). */

   /** @internal Structure to hold pending preedit state from the compositor. */
   struct
     {
        Eina_List *attrs; /**< Pending preedit attributes. */
        int32_t cursor;   /**< Pending preedit cursor position (byte offset). */
     } pending_preedit;

   /** @internal Structure to hold pending commit state from the compositor. */
   struct
     {
        int32_t cursor;        /**< Pending cursor position after commit (byte offset). */
        int32_t anchor;        /**< Pending anchor position after commit (byte offset). */
        uint32_t delete_index;  /**< Byte offset from cursor to start deletion before commit. */
        uint32_t delete_length; /**< Number of bytes to delete before commit. */
     } pending_commit;

   /** @internal Structure to hold cursor location information. */
   struct
     {
        int x;      /**< X-coordinate of the cursor rectangle relative to the canvas. */
        int y;      /**< Y-coordinate of the cursor rectangle relative to the canvas. */
        int width;  /**< Width of the cursor rectangle. */
        int height; /**< Height of the cursor rectangle. */
        Eina_Bool do_set : 1; /**< Flag indicating if the cursor location needs to be sent to the compositor. */
     } cursor_location;

   xkb_mod_mask_t control_mask; /**< XKB modifier mask for Control. */
   xkb_mod_mask_t alt_mask;     /**< XKB modifier mask for Alt. */
   xkb_mod_mask_t shift_mask;   /**< XKB modifier mask for Shift. */

   uint32_t serial;          /**< Current serial number for Wayland requests. */
   uint32_t reset_serial;    /**< Serial number at the time of the last reset. Used to discard outdated events. */
   uint32_t content_purpose; /**< Current zwp_text_input_v1.content_purpose. */
   uint32_t content_hint;    /**< Current zwp_text_input_v1.content_hint. */
};

/**
 * @internal
 * @brief Converts a UTF-8 byte offset to a character count.
 *
 * Given a UTF-8 string and a byte offset within that string, this function
 * calculates the number of Unicode characters up to that offset.
 *
 * @param str The UTF-8 encoded string.
 * @param offset The byte offset within the string.
 * @return The number of characters corresponding to the byte offset.
 */
static unsigned int
utf8_offset_to_characters(const char *str, int offset)
{
   int index = 0;
   unsigned int i = 0;

   for (; index < offset; i++)
     {
        if (eina_unicode_utf8_next_get(str, &index) == 0)
          break;
     }

   return i;
}

/**
 * @internal
 * @brief Updates the Wayland compositor with the current input state.
 *
 * This function sends surrounding text and cursor rectangle information
 * to the Wayland compositor if they have changed or need to be set.
 * It should be called when these aspects of the input context change.
 *
 * @param imcontext The Wayland input method context.
 */
static void
update_state(WaylandIMContext *imcontext)
{
   char *surrounding = NULL;
   int cursor_pos;
   Ecore_Evas *ee;
   int canvas_x = 0, canvas_y = 0;
   Eina_Bool changed = EINA_FALSE;

   if (!imcontext->ctx)
     return;

   /* cursor_pos is a byte index */
   if (ecore_imf_context_surrounding_get(imcontext->ctx, &surrounding, &cursor_pos))
     {
        if (imcontext->text_input)
          {
             zwp_text_input_v1_set_surrounding_text(imcontext->text_input,
                                                    surrounding,
                                                    cursor_pos, cursor_pos);
             changed = EINA_TRUE;
          }

        if (surrounding)
          free(surrounding);
     }

   if (imcontext->canvas)
     {
        ee = ecore_evas_ecore_evas_get(imcontext->canvas);
        if (ee)
          ecore_evas_geometry_get(ee, &canvas_x, &canvas_y, NULL, NULL);
     }

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom, "canvas (x: %d, y: %d)",
                     canvas_x, canvas_y);

   if (imcontext->text_input)
     {
        if (imcontext->cursor_location.do_set)
          {
             zwp_text_input_v1_set_cursor_rectangle(imcontext->text_input,
                                                    imcontext->cursor_location.x + canvas_x,
                                                    imcontext->cursor_location.y + canvas_y,
                                                    imcontext->cursor_location.width,
                                                    imcontext->cursor_location.height);
             imcontext->cursor_location.do_set = EINA_FALSE;
             changed = EINA_TRUE;
          }
     }

   if (changed)
     zwp_text_input_v1_commit_state(imcontext->text_input, ++imcontext->serial);

   _clear_hide_timer();
}

/**
 * @internal
 * @brief Clears (deletes) the input panel hide timer if it exists.
 * @return EINA_TRUE if a timer was cleared, EINA_FALSE otherwise.
 */
static Eina_Bool
_clear_hide_timer(void)
{
   if (_hide_timer)
     {
        ecore_timer_del(_hide_timer);
        _hide_timer = NULL;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @internal
 * @brief Sends a request to the Wayland compositor to hide the input panel.
 * @param ctx The Ecore IMF context.
 */
static void
_send_input_panel_hide_request(Ecore_IMF_Context *ctx)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
   if (imcontext && imcontext->text_input)
     zwp_text_input_v1_hide_input_panel(imcontext->text_input);
}

/**
 * @internal
 * @brief Timer callback function to hide the input panel.
 * This function is called when the hide timer expires. It sends a request
 * to the Wayland compositor to hide the input panel.
 * @param data The Ecore IMF context passed as user data.
 * @return ECORE_CALLBACK_CANCEL to automatically delete the timer.
 */
static Eina_Bool
_hide_timer_handler(void *data)
{
   Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)data;
   _send_input_panel_hide_request(ctx);

   _hide_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

/**
 * @internal
 * @brief Starts a timer to hide the input panel after a short delay.
 * If a timer is already active, this function does nothing.
 * @param data The Ecore IMF context to be passed to the timer handler.
 */
static void
_input_panel_hide_timer_start(void *data)
{
   if (!_hide_timer)
     {
        _hide_timer =
          ecore_timer_add(HIDE_TIMER_INTERVAL, _hide_timer_handler, data);
     }
}

/**
 * @internal
 * @brief Hides the input panel, either instantly or after a delay.
 * @param ctx The Ecore IMF context.
 * @param instant If EINA_TRUE, hide immediately. If EINA_FALSE, start a timer
 *                to hide after a short delay (unless a hide is already pending
 *                and very close to firing).
 */
static void
_input_panel_hide(Ecore_IMF_Context *ctx, Eina_Bool instant)
{
   if (instant || (_hide_timer && ecore_timer_pending_get(_hide_timer) <= 0.0))
     {
        _clear_hide_timer();
        _send_input_panel_hide_request(ctx);
     }
   else
     {
        _input_panel_hide_timer_start(ctx);
     }
}

/**
 * @internal
 * @brief Checks if a received Wayland event serial is current.
 *
 * Wayland events carry serial numbers. This function checks if the given
 * `serial` is not older than the `reset_serial` of the `imcontext`.
 * If the serial is outdated, pending preedit and commit data is cleared.
 *
 * @param imcontext The Wayland input method context.
 * @param serial The serial number from the Wayland event.
 * @return EINA_TRUE if the serial is current, EINA_FALSE if it's outdated.
 */
static Eina_Bool
check_serial(WaylandIMContext *imcontext, uint32_t serial)
{
   Ecore_IMF_Preedit_Attr *attr;

   if ((imcontext->serial - serial) >
       (imcontext->serial - imcontext->reset_serial))
     {
        EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom,
                          "outdated serial: %u, current: %u, reset: %u",
                          serial, imcontext->serial, imcontext->reset_serial);

        /* Clear pending data */
        imcontext->pending_commit.delete_index = 0;
        imcontext->pending_commit.delete_length = 0;
        imcontext->pending_commit.cursor = 0;
        imcontext->pending_commit.anchor = 0;

        imcontext->pending_preedit.cursor = 0;

        if (imcontext->pending_preedit.attrs)
          {
             EINA_LIST_FREE(imcontext->pending_preedit.attrs, attr) free(attr);
             imcontext->pending_preedit.attrs = NULL;
          }

        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Clears the current preedit state in the input method context.
 *
 * This function frees memory associated with the preedit string, commit string,
 * and preedit attributes. It also resets the preedit cursor position.
 *
 * @param imcontext The Wayland input method context.
 */
static void
clear_preedit(WaylandIMContext *imcontext)
{
   Ecore_IMF_Preedit_Attr *attr = NULL;

   imcontext->preedit_cursor = 0;

   if (imcontext->preedit_text)
     {
        free(imcontext->preedit_text);
        imcontext->preedit_text = NULL;
     }

   if (imcontext->preedit_commit)
     {
        free(imcontext->preedit_commit);
        imcontext->preedit_commit = NULL;
     }

   if (imcontext->preedit_attrs)
     {
        EINA_LIST_FREE(imcontext->preedit_attrs, attr)
           free(attr);
     }

   imcontext->preedit_attrs = NULL;
}

/**
 * @internal
 * @brief Wayland listener callback for the 'commit_string' event.
 *
 * This function is called by the Wayland compositor when text should be committed
 * to the application. It handles deleting surrounding text if requested by the
 * compositor, clears any existing preedit, and then triggers the
 * ECORE_IMF_CALLBACK_COMMIT event.
 *
 * @param data The WaylandIMContext.
 * @param text_input The Wayland text input object (unused).
 * @param serial The serial number of the event.
 * @param text The string to be committed.
 */
static void
text_input_commit_string(void *data,
                         struct zwp_text_input_v1 *text_input EINA_UNUSED,
                         uint32_t serial, const char *text)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)data;
   Eina_Bool old_preedit = EINA_FALSE;
   char *surrounding = NULL;
   int cursor_pos, cursor;
   Ecore_IMF_Event_Delete_Surrounding ev;

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom,
                     "commit event (text: `%s', current pre-edit: `%s')",
                     text,
                     imcontext->preedit_text ? imcontext->preedit_text : "");

   old_preedit =
     imcontext->preedit_text && strlen(imcontext->preedit_text) > 0;

   if (!imcontext->ctx)
     return;

   if (!check_serial(imcontext, serial))
     return;

   if (old_preedit)
     {
        ecore_imf_context_event_callback_call(imcontext->ctx,
                                              ECORE_IMF_CALLBACK_PREEDIT_END,
                                              NULL);
     }

   clear_preedit(imcontext);

   if (imcontext->pending_commit.delete_length > 0)
     {
        /* cursor_pos is a byte index */
        if (ecore_imf_context_surrounding_get(imcontext->ctx, &surrounding,
                                              &cursor_pos))
          {
             ev.ctx = imcontext->ctx;
             /* offset and n_chars are in characters */
             ev.offset = utf8_offset_to_characters(surrounding, cursor_pos + imcontext->pending_commit.delete_index);
             ev.n_chars = utf8_offset_to_characters(surrounding,
                                                    cursor_pos + imcontext->pending_commit.delete_index + imcontext->pending_commit.delete_length) - ev.offset;

             /* cursor in characters */
             cursor = utf8_offset_to_characters(surrounding, cursor_pos);

             ev.offset -= cursor;

             EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom,
                     "delete on commit (text: `%s', offset `%d', length: `%d')",
                     surrounding, ev.offset, ev.n_chars);

             if (surrounding)
               free(surrounding);

             ecore_imf_context_event_callback_call(imcontext->ctx, ECORE_IMF_CALLBACK_DELETE_SURROUNDING, &ev);
          }
     }

   imcontext->pending_commit.delete_index = 0;
   imcontext->pending_commit.delete_length = 0;
   imcontext->pending_commit.cursor = 0;
   imcontext->pending_commit.anchor = 0;

   ecore_imf_context_event_callback_call(imcontext->ctx, ECORE_IMF_CALLBACK_COMMIT, (void *)text);
}

/**
 * @internal
 * @brief Commits the current preedit string.
 *
 * If there is a `preedit_commit` string set (meaning the current preedit
 * should be committed as a whole), this function triggers the necessary
 * Ecore IMF callbacks: PREEDIT_CHANGED, PREEDIT_END, and COMMIT.
 *
 * @param imcontext The Wayland input method context.
 */
static void
commit_preedit(WaylandIMContext *imcontext)
{
   if (!imcontext->preedit_commit)
     return;

   if (!imcontext->ctx)
     return;

   ecore_imf_context_event_callback_call(imcontext->ctx,
                                         ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
                                         NULL);

   ecore_imf_context_event_callback_call(imcontext->ctx,
                                         ECORE_IMF_CALLBACK_PREEDIT_END, NULL);

   ecore_imf_context_event_callback_call(imcontext->ctx,
                                         ECORE_IMF_CALLBACK_COMMIT,
                                         (void *)imcontext->preedit_commit);
}

/**
 * @internal
 * @brief Sets the input focus for the Wayland text input object.
 *
 * This function finds the default Wayland input (seat) and activates the
 * `text_input` object for the given context's window surface.
 *
 * @param ctx The Ecore IMF context.
 */
static void
set_focus(Ecore_IMF_Context *ctx)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
   Ecore_Wl2_Input *input;

   input = ecore_wl2_display_input_find_by_name(ecore_wl2_window_display_get(imcontext->window), "default");
   if (!input)
     return;

   struct wl_seat *seat = ecore_wl2_input_seat_get(input);
   if (!seat)
     return;

   imcontext->input = input;

   zwp_text_input_v1_activate(imcontext->text_input, seat,
                          ecore_wl2_window_surface_get(imcontext->window));
}

/**
 * @internal
 * @brief Shows the input panel (e.g., virtual keyboard).
 *
 * This function ensures the text input object is focused, clears any pending
 * hide timer, sets content type hints, updates surrounding text, and then
 * requests the Wayland compositor to show the input panel.
 *
 * @param ctx The Ecore IMF context.
 * @return EINA_TRUE if the request to show the panel was made, EINA_FALSE on failure
 *         (e.g., context, window, or text_input not available).
 */
static Eina_Bool
show_input_panel(Ecore_IMF_Context *ctx)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
   char *surrounding = NULL;
   int cursor_pos;

   if ((!imcontext) || (!imcontext->window) || (!imcontext->text_input))
     return EINA_FALSE;

   if (!imcontext->input)
     set_focus(ctx);

   _clear_hide_timer();

   zwp_text_input_v1_set_content_type(imcontext->text_input,
                                  imcontext->content_hint,
                                  imcontext->content_purpose);

   if (ecore_imf_context_surrounding_get(imcontext->ctx, &surrounding, &cursor_pos))
     {
        if (imcontext->text_input)
          zwp_text_input_v1_set_surrounding_text(imcontext->text_input, surrounding,
                                             cursor_pos, cursor_pos);

        if (surrounding)
          {
            free(surrounding);
            surrounding = NULL;
          }
     }

   zwp_text_input_v1_show_input_panel(imcontext->text_input);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Wayland listener callback for the 'preedit_string' event.
 *
 * This function is called by the Wayland compositor to update the preedit string.
 * It updates the internal preedit state (text, commit string, cursor, attributes)
 * and triggers ECORE_IMF_CALLBACK_PREEDIT_START (if no preedit was active),
 * ECORE_IMF_CALLBACK_PREEDIT_CHANGED, and ECORE_IMF_CALLBACK_PREEDIT_END
 * (if the new preedit string is empty).
 *
 * @param data The WaylandIMContext.
 * @param text_input The Wayland text input object (unused).
 * @param serial The serial number of the event.
 * @param text The new preedit string.
 * @param commit The string that should be committed if this preedit is finalized.
 */
static void
text_input_preedit_string(void *data,
                          struct zwp_text_input_v1 *text_input EINA_UNUSED,
                          uint32_t serial, const char *text, const char *commit)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)data;
   Eina_Bool old_preedit = EINA_FALSE;

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom,
                     "preedit event (text: `%s', current pre-edit: `%s')",
                     text,
                     imcontext->preedit_text ? imcontext->preedit_text : "");

   if (!check_serial(imcontext, serial))
     return;

   old_preedit =
     imcontext->preedit_text && strlen(imcontext->preedit_text) > 0;

   clear_preedit(imcontext);

   imcontext->preedit_text = strdup(text);
   imcontext->preedit_commit = strdup(commit);
   imcontext->preedit_cursor =
     utf8_offset_to_characters(text, imcontext->pending_preedit.cursor);
   imcontext->preedit_attrs = imcontext->pending_preedit.attrs;

   imcontext->pending_preedit.attrs = NULL;

   if (!old_preedit)
     {
        ecore_imf_context_event_callback_call(imcontext->ctx,
                                              ECORE_IMF_CALLBACK_PREEDIT_START,
                                              NULL);
     }

   ecore_imf_context_event_callback_call(imcontext->ctx,
                                         ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
                                         NULL);

   if (imcontext->preedit_text && strlen(imcontext->preedit_text) == 0)
     {
        ecore_imf_context_event_callback_call(imcontext->ctx,
                                              ECORE_IMF_CALLBACK_PREEDIT_END,
                                              NULL);
     }
}

/**
 * @internal
 * @brief Wayland listener callback for the 'delete_surrounding_text' event.
 *
 * This function is called by the Wayland compositor to request deletion of text
 * surrounding the cursor. It stores the deletion parameters in `pending_commit`
 * (as this deletion often precedes a commit) and triggers the
 * ECORE_IMF_CALLBACK_DELETE_SURROUNDING event.
 *
 * @param data The WaylandIMContext.
 * @param text_input The Wayland text input object (unused).
 * @param index Byte offset relative to the cursor position where deletion should start.
 *              Can be negative.
 * @param length Number of bytes to delete.
 */
static void
text_input_delete_surrounding_text(void *data,
                                   struct zwp_text_input_v1 *text_input EINA_UNUSED,
                                   int32_t index, uint32_t length)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)data;
   Ecore_IMF_Event_Delete_Surrounding ev;
   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom,
                     "delete surrounding text (index: %d, length: %u)",
                     index, length);

   imcontext->pending_commit.delete_index = ev.offset = index;
   imcontext->pending_commit.delete_length = ev.n_chars = length;

   ecore_imf_context_event_callback_call(imcontext->ctx, ECORE_IMF_CALLBACK_DELETE_SURROUNDING, &ev);
}

/**
 * @internal
 * @brief Wayland listener callback for the 'cursor_position' event.
 *
 * This function is called by the Wayland compositor to inform about the
 * cursor and anchor positions that should be set after the next commit.
 * The positions are stored in `pending_commit`.
 *
 * @param data The WaylandIMContext.
 * @param text_input The Wayland text input object (unused).
 * @param index The new cursor byte offset.
 * @param anchor The new anchor byte offset (for selections).
 */
static void
text_input_cursor_position(void *data,
                           struct zwp_text_input_v1 *text_input EINA_UNUSED,
                           int32_t index, int32_t anchor)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)data;

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom,
                     "cursor_position for next commit (index: %d, anchor: %d)",
                     index, anchor);

   imcontext->pending_commit.cursor = index;
   imcontext->pending_commit.anchor = anchor;
}

/**
 * @internal
 * @brief Wayland listener callback for the 'preedit_styling' event.
 *
 * This function is called by the Wayland compositor to provide styling
 * information for a portion of the preedit string. It creates an
 * Ecore_IMF_Preedit_Attr structure based on the style and appends it
 * to the `pending_preedit.attrs` list. These attributes will be applied
 * when the `preedit_string` event is received.
 *
 * @param data The WaylandIMContext.
 * @param text_input The Wayland text input object (unused).
 * @param index Starting byte offset of the styled segment in the preedit string.
 * @param length Length in bytes of the styled segment.
 * @param style The `zwp_text_input_v1_preedit_style` value.
 */
static void
text_input_preedit_styling(void *data,
                           struct zwp_text_input_v1 *text_input EINA_UNUSED,
                           uint32_t index, uint32_t length, uint32_t style)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)data;
   Ecore_IMF_Preedit_Attr *attr = calloc(1, sizeof(*attr));
   if (!attr) return;

   switch (style)
     {
      case ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_DEFAULT:
      case ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_UNDERLINE:
      case ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_INCORRECT:
      case ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_HIGHLIGHT:
      case ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_ACTIVE:
      case ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_INACTIVE:
         attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB1;
         break;
      case ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_SELECTION:
         attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB2;
         break;
      default:
         attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB1;
         break;
     }

   attr->start_index = index;
   attr->end_index = index + length;

   imcontext->pending_preedit.attrs =
     eina_list_append(imcontext->pending_preedit.attrs, attr);
}

/**
 * @internal
 * @brief Wayland listener callback for the 'preedit_cursor' event.
 *
 * This function is called by the Wayland compositor to set the cursor position
 * within the preedit string. The position (byte offset) is stored in
 * `pending_preedit.cursor` and will be applied when the `preedit_string`
 * event is received.
 *
 * @param data The WaylandIMContext.
 * @param text_input The Wayland text input object (unused).
 * @param index The new cursor byte offset within the preedit string.
 */
static void
text_input_preedit_cursor(void *data,
                          struct zwp_text_input_v1 *text_input EINA_UNUSED,
                          int32_t index)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)data;

   imcontext->pending_preedit.cursor = index;
}

/**
 * @internal
 * @brief Retrieves the XKB modifier index for a given modifier name.
 *
 * This function iterates through the `modifiers_map` (provided by the
 * Wayland compositor) to find the index corresponding to the specified
 * modifier name (e.g., "Shift", "Control").
 *
 * @param modifiers_map A `wl_array` containing null-terminated strings of modifier names.
 *                      Example: ["Shift", "Control", "Mod1", ...]
 * @param name The name of the modifier to find (e.g., "Shift").
 * @return The `xkb_mod_index_t` for the modifier, or `XKB_MOD_INVALID` if not found.
 */
static xkb_mod_index_t
modifiers_get_index(struct wl_array *modifiers_map, const char *name)
{
   xkb_mod_index_t index = 0;
   char *p = modifiers_map->data;

   while ((const char *)p < ((const char *)modifiers_map->data + modifiers_map->size))
     {
        if (strcmp(p, name) == 0)
          return index;

        index++;
        p += strlen(p) + 1;
     }

   return XKB_MOD_INVALID;
}

/**
 * @internal
 * @brief Retrieves the XKB modifier mask for a given modifier name.
 *
 * This function first gets the modifier index using `modifiers_get_index`
 * and then calculates the corresponding bitmask (1 << index).
 *
 * @param modifiers_map A `wl_array` containing null-terminated strings of modifier names.
 * @param name The name of the modifier to find (e.g., "Control").
 * @return The `xkb_mod_mask_t` for the modifier, or `XKB_MOD_INVALID` if not found.
 */
static xkb_mod_mask_t
modifiers_get_mask(struct wl_array *modifiers_map, const char *name)
{
   xkb_mod_index_t index = modifiers_get_index(modifiers_map, name);

   if (index == XKB_MOD_INVALID)
     return XKB_MOD_INVALID;

   return 1 << index;
}

/**
 * @internal
 * @brief Wayland listener callback for the 'modifiers_map' event.
 *
 * This function is called by the Wayland compositor to provide the mapping
 * of modifier names to XKB modifier masks. It stores the masks for Shift,
 * Control, and Alt (Mod1) in the `imcontext`.
 *
 * @param data The WaylandIMContext.
 * @param text_input The Wayland text input object (unused).
 * @param map A `wl_array` containing null-terminated strings of modifier names,
 *            ordered by their XKB modifier index.
 */
static void
text_input_modifiers_map(void *data,
                         struct zwp_text_input_v1 *text_input EINA_UNUSED,
                         struct wl_array *map)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)data;

   imcontext->shift_mask = modifiers_get_mask(map, "Shift");
   imcontext->control_mask = modifiers_get_mask(map, "Control");
   imcontext->alt_mask = modifiers_get_mask(map, "Mod1");
}

/**
 * @internal
 * @brief Wayland listener callback for the 'keysym' event.
 *
 * This function is called by the Wayland compositor when a key event occurs
 * that should be processed by the client (application). It converts the
 * keysym and modifiers into an Ecore_Event_Key and adds it to the Ecore
 * event queue (ECORE_EVENT_KEY_DOWN or ECORE_EVENT_KEY_UP).
 *
 * @param data The WaylandIMContext.
 * @param text_input The Wayland text input object (unused).
 * @param serial The serial number of the event (unused here, but could be used for synchronization).
 * @param time Timestamp of the event.
 * @param sym The XKB keysym.
 * @param state 0 for key release, 1 for key press.
 * @param modifiers Bitmask of active XKB modifiers.
 */
static void
text_input_keysym(void *data,
                  struct zwp_text_input_v1 *text_input EINA_UNUSED,
                  uint32_t serial EINA_UNUSED, uint32_t time, uint32_t sym,
                  uint32_t state, uint32_t modifiers)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)data;
   char string[32], key[32], keyname[32];
   Ecore_Event_Key *e;

   memset(key, 0, sizeof(key));
   xkb_keysym_get_name(sym, key, sizeof(key));

   memset(keyname, 0, sizeof(keyname));
   xkb_keysym_get_name(sym, keyname, sizeof(keyname));
   if (keyname[0] == '\0')
     snprintf(keyname, sizeof(keyname), "Keysym-%u", sym);

   memset(string, 0, sizeof(string));
   if (!xkb_keysym_to_utf8(sym, string, 32)) return;

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom,
                     "key event (key: %s)",
                     keyname);

   e = calloc(1, sizeof(Ecore_Event_Key) + strlen(key) + strlen(keyname) +
              strlen(string) + 3);
   if (!e) return;

   e->keyname = (char *)(e + 1);
   e->key = e->keyname + strlen(keyname) + 1;
   e->string = e->key + strlen(key) + 1;
   e->compose = e->string;

   strcpy((char *)e->keyname, keyname);
   strcpy((char *)e->key, key);
   strcpy((char *)e->string, string);

   e->window = (Ecore_Window)imcontext->window;
   e->event_window = (Ecore_Window)imcontext->window;
   e->timestamp = time;

   e->modifiers = 0;
   if (modifiers & imcontext->shift_mask)
     e->modifiers |= ECORE_EVENT_MODIFIER_SHIFT;

   if (modifiers & imcontext->control_mask)
     e->modifiers |= ECORE_EVENT_MODIFIER_CTRL;

   if (modifiers & imcontext->alt_mask)
     e->modifiers |= ECORE_EVENT_MODIFIER_ALT;

   if (state)
     ecore_event_add(ECORE_EVENT_KEY_DOWN, e, NULL, NULL);
   else
     ecore_event_add(ECORE_EVENT_KEY_UP, e, NULL, NULL);
}

/**
 * @internal
 * @brief Wayland listener callback for the 'enter' event.
 *
 * This function is called by the Wayland compositor when the text input
 * focus enters a new surface. It calls `update_state` to send current
 * input state to the compositor and updates `reset_serial` to the current
 * `serial`. This `reset_serial` is used to ignore stale events that might
 * arrive after a focus change or reset.
 *
 * @param data The WaylandIMContext.
 * @param text_input The Wayland text input object (unused).
 * @param surface The Wayland surface that gained focus (unused).
 */
static void
text_input_enter(void *data,
                 struct zwp_text_input_v1 *text_input EINA_UNUSED,
                 struct wl_surface *surface EINA_UNUSED)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)data;

   update_state(imcontext);

   imcontext->reset_serial = imcontext->serial;
}

/**
 * @internal
 * @brief Wayland listener callback for the 'leave' event.
 *
 * This function is called by the Wayland compositor when the text input
 * focus leaves the current surface. It commits any pending preedit text
 * and then clears the preedit state.
 *
 * @param data The WaylandIMContext.
 * @param text_input The Wayland text input object (unused).
 */
static void
text_input_leave(void *data,
                 struct zwp_text_input_v1 *text_input EINA_UNUSED)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)data;

   /* clear preedit */
   commit_preedit(imcontext);
   clear_preedit(imcontext);
}

/**
 * @internal
 * @brief Wayland listener callback for the 'input_panel_state' event.
 *
 * This function is called by the Wayland compositor to inform about changes
 * in the input panel's state (e.g., visibility, position). Currently, this
 * implementation is a no-op.
 *
 * @param data The WaylandIMContext (unused).
 * @param text_input The Wayland text input object (unused).
 * @param state The new state of the input panel (unused).
 */
static void
text_input_input_panel_state(void *data EINA_UNUSED,
                             struct zwp_text_input_v1 *text_input EINA_UNUSED,
                             uint32_t state EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Wayland listener callback for the 'language' event.
 *
 * This function is called by the Wayland compositor to inform about a change
 * in the input language (locale). If the language has changed, it updates
 * the `imcontext->language` field and triggers the
 * ECORE_IMF_INPUT_PANEL_LANGUAGE_EVENT callback.
 *
 * @param data The WaylandIMContext.
 * @param text_input The Wayland text input object (unused).
 * @param serial The serial number of the event (unused).
 * @param language The new language string (e.g., "en_US").
 */
static void
text_input_language(void *data,
                    struct zwp_text_input_v1 *text_input EINA_UNUSED,
                    uint32_t serial EINA_UNUSED, const char *language)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    Eina_Bool changed = EINA_FALSE;

    if (!imcontext || !language) return;

    if (imcontext->language)
      {
         if (strcmp(imcontext->language, language) != 0)
           {
              changed = EINA_TRUE;
              free(imcontext->language);
           }
      }
    else
      changed = EINA_TRUE;

    if (changed)
      {
         imcontext->language = strdup(language);

         if (imcontext->ctx)
           ecore_imf_context_input_panel_event_callback_call(imcontext->ctx, ECORE_IMF_INPUT_PANEL_LANGUAGE_EVENT, 0);
      }
}

/**
 * @internal
 * @brief Wayland listener callback for the 'text_direction' event.
 *
 * This function is called by the Wayland compositor to inform about a change
 * in the text direction (e.g., left-to-right, right-to-left). Currently, this
 * implementation is a no-op.
 *
 * @param data The WaylandIMContext (unused).
 * @param text_input The Wayland text input object (unused).
 * @param serial The serial number of the event (unused).
 * @param direction The new text direction (unused).
 */
static void
text_input_text_direction(void *data EINA_UNUSED,
                          struct zwp_text_input_v1 *text_input EINA_UNUSED,
                          uint32_t serial EINA_UNUSED,
                          uint32_t direction EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Listener for Wayland text_input_v1 events.
 *
 * This structure maps Wayland text input protocol events to their
 * corresponding handler functions.
 */
static const struct zwp_text_input_v1_listener text_input_listener =
{
   text_input_enter,
   text_input_leave,
   text_input_modifiers_map,
   text_input_input_panel_state,
   text_input_preedit_string,
   text_input_preedit_styling,
   text_input_preedit_cursor,
   text_input_commit_string,
   text_input_cursor_position,
   text_input_delete_surrounding_text,
   text_input_keysym,
   text_input_language,
   text_input_text_direction
};

void
wayland_im_context_add(Ecore_IMF_Context *ctx)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom, "context_add");

   imcontext->ctx = ctx;

   imcontext->text_input =
     zwp_text_input_manager_v1_create_text_input(imcontext->text_input_manager);
   if (imcontext->text_input)
     zwp_text_input_v1_add_listener(imcontext->text_input,
                                &text_input_listener, imcontext);
}

void
wayland_im_context_del(Ecore_IMF_Context *ctx)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom, "context_del");

   _clear_hide_timer();

   if (imcontext->language)
     {
        free(imcontext->language);
        imcontext->language = NULL;
     }

   if (imcontext->text_input)
     zwp_text_input_v1_destroy(imcontext->text_input);

   clear_preedit(imcontext);

   free(imcontext);
}

void
wayland_im_context_reset(Ecore_IMF_Context *ctx)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   commit_preedit(imcontext);
   clear_preedit(imcontext);

   if (imcontext->text_input)
     zwp_text_input_v1_reset(imcontext->text_input);

   update_state(imcontext);

   imcontext->reset_serial = imcontext->serial;
}

void
wayland_im_context_focus_in(Ecore_IMF_Context *ctx)
{
   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom, "focus-in");

   set_focus(ctx);

   if (ecore_imf_context_input_panel_enabled_get(ctx))
     if (!ecore_imf_context_input_panel_show_on_demand_get (ctx))
       show_input_panel(ctx);
}

void
wayland_im_context_focus_out(Ecore_IMF_Context *ctx)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom, "focus-out");

   if (!imcontext->input) return;

   if (imcontext->text_input)
     {
        if (ecore_imf_context_input_panel_enabled_get(ctx))
          _input_panel_hide(ctx, EINA_FALSE);

        zwp_text_input_v1_deactivate(imcontext->text_input,
                                 ecore_wl2_input_seat_get(imcontext->input));
     }

   imcontext->input = NULL;
}

void
wayland_im_context_preedit_string_get(Ecore_IMF_Context *ctx,
                                      char **str, int *cursor_pos)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom,
                     "pre-edit string requested (preedit: `%s')",
                     imcontext->preedit_text ? imcontext->preedit_text : "");

   if (str)
     *str = strdup(imcontext->preedit_text ? imcontext->preedit_text : "");

   if (cursor_pos)
     *cursor_pos = imcontext->preedit_cursor;
}

void
wayland_im_context_preedit_string_with_attributes_get(Ecore_IMF_Context *ctx,
                                                      char **str,
                                                      Eina_List **attrs,
                                                      int *cursor_pos)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom,
                     "pre-edit string with attributes requested (preedit: `%s')",
                     imcontext->preedit_text ? imcontext->preedit_text : "");

   if (str)
     *str = strdup(imcontext->preedit_text ? imcontext->preedit_text : "");

   if (attrs)
     {
        Eina_List *l;
        Ecore_IMF_Preedit_Attr *a, *attr;

        EINA_LIST_FOREACH(imcontext->preedit_attrs, l, a)
          {
             attr = malloc(sizeof(*attr));
             if (attr)
               {
                  attr = memcpy(attr, a, sizeof(*attr));
                  *attrs = eina_list_append(*attrs, attr);
               }
          }
     }

   if (cursor_pos)
     *cursor_pos = imcontext->preedit_cursor;
}

void
wayland_im_context_cursor_position_set(Ecore_IMF_Context *ctx, int cursor_pos)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom,
                     "set cursor position (cursor: %d)",
                     cursor_pos);

   update_state(imcontext);
}

void
wayland_im_context_use_preedit_set(Ecore_IMF_Context *ctx EINA_UNUSED,
                                   Eina_Bool use_preedit EINA_UNUSED)
{
}

void
wayland_im_context_client_window_set(Ecore_IMF_Context *ctx, void *window)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom, "client window set (window: %p)", window);

   if (window != NULL)
     {
        imcontext->window = window;
     }
}

void
wayland_im_context_client_canvas_set(Ecore_IMF_Context *ctx, void *canvas)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom, "client canvas set (canvas: %p)", canvas);

   if (canvas != NULL)
     imcontext->canvas = canvas;
}

void
wayland_im_context_show(Ecore_IMF_Context *ctx)
{
   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom, "context_show");

   show_input_panel(ctx);
}

void
wayland_im_context_hide(Ecore_IMF_Context *ctx)
{
   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom, "context_hide");

   _input_panel_hide(ctx, EINA_FALSE);
}

Eina_Bool
wayland_im_context_filter_event(Ecore_IMF_Context *ctx,
                                Ecore_IMF_Event_Type type,
                                Ecore_IMF_Event *event EINA_UNUSED)
{

   if (type == ECORE_IMF_EVENT_MOUSE_UP)
     {
        if (ecore_imf_context_input_panel_enabled_get(ctx))
          show_input_panel(ctx);
     }

   return EINA_FALSE;
}

void
wayland_im_context_cursor_location_set(Ecore_IMF_Context *ctx, int x, int y, int width, int height)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom, "cursor_location_set (x: %d, y: %d, w: %d, h: %d)", x, y, width, height);

   if ((imcontext->cursor_location.x != x) ||
       (imcontext->cursor_location.y != y) ||
       (imcontext->cursor_location.width != width) ||
       (imcontext->cursor_location.height != height))
     {
        imcontext->cursor_location.x = x;
        imcontext->cursor_location.y = y;
        imcontext->cursor_location.width = width;
        imcontext->cursor_location.height = height;
        imcontext->cursor_location.do_set = EINA_TRUE;

        update_state(imcontext);
     }
}

void
wayland_im_context_autocapital_type_set(Ecore_IMF_Context *ctx,
                                        Ecore_IMF_Autocapital_Type autocapital_type)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   imcontext->content_hint &= ~(ZWP_TEXT_INPUT_V1_CONTENT_HINT_AUTO_CAPITALIZATION |
                                ZWP_TEXT_INPUT_V1_CONTENT_HINT_UPPERCASE |
                                ZWP_TEXT_INPUT_V1_CONTENT_HINT_LOWERCASE);

   if (autocapital_type == ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE)
     imcontext->content_hint |= ZWP_TEXT_INPUT_V1_CONTENT_HINT_AUTO_CAPITALIZATION;
   else if (autocapital_type == ECORE_IMF_AUTOCAPITAL_TYPE_ALLCHARACTER)
     imcontext->content_hint |= ZWP_TEXT_INPUT_V1_CONTENT_HINT_UPPERCASE;
   else
     imcontext->content_hint |= ZWP_TEXT_INPUT_V1_CONTENT_HINT_LOWERCASE;
}

void
wayland_im_context_input_panel_layout_set(Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Layout layout)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   switch (layout) {
      case ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBER:
         imcontext->content_purpose = ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_NUMBER;
         break;
      case ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL:
         imcontext->content_purpose = ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_EMAIL;
         break;
      case ECORE_IMF_INPUT_PANEL_LAYOUT_URL:
         imcontext->content_purpose = ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_URL;
         break;
      case ECORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER:
         imcontext->content_purpose = ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_PHONE;
         break;
      case ECORE_IMF_INPUT_PANEL_LAYOUT_IP:
         imcontext->content_purpose = ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_NUMBER;
         break;
      case ECORE_IMF_INPUT_PANEL_LAYOUT_MONTH:
         imcontext->content_purpose = ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_DATE;
         break;
      case ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY:
        imcontext->content_purpose = ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_DIGITS;
        break;
      case ECORE_IMF_INPUT_PANEL_LAYOUT_TERMINAL:
        imcontext->content_purpose = ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_TERMINAL;
        break;
      case ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD:
        imcontext->content_purpose = ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_PASSWORD;
        break;
      case ECORE_IMF_INPUT_PANEL_LAYOUT_DATETIME:
        imcontext->content_purpose = ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_DATETIME;
        break;
      default:
        imcontext->content_purpose = ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_NORMAL;
        break;
   }
}

void
wayland_im_context_input_mode_set(Ecore_IMF_Context *ctx,
                                  Ecore_IMF_Input_Mode input_mode)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   if (input_mode & ECORE_IMF_INPUT_MODE_INVISIBLE)
     imcontext->content_hint |= ZWP_TEXT_INPUT_V1_CONTENT_HINT_PASSWORD;
   else
     imcontext->content_hint &= ~ZWP_TEXT_INPUT_V1_CONTENT_HINT_PASSWORD;
}

void
wayland_im_context_input_hint_set(Ecore_IMF_Context *ctx,
                                  Ecore_IMF_Input_Hints input_hints)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   if (input_hints & ECORE_IMF_INPUT_HINT_AUTO_COMPLETE)
     imcontext->content_hint |= ZWP_TEXT_INPUT_V1_CONTENT_HINT_AUTO_COMPLETION;
   else
     imcontext->content_hint &= ~ZWP_TEXT_INPUT_V1_CONTENT_HINT_AUTO_COMPLETION;

   if (input_hints & ECORE_IMF_INPUT_HINT_SENSITIVE_DATA)
     imcontext->content_hint |= ZWP_TEXT_INPUT_V1_CONTENT_HINT_SENSITIVE_DATA;
   else
     imcontext->content_hint &= ~ZWP_TEXT_INPUT_V1_CONTENT_HINT_SENSITIVE_DATA;

   if (input_hints & ECORE_IMF_INPUT_HINT_MULTILINE)
     imcontext->content_hint |= ZWP_TEXT_INPUT_V1_CONTENT_HINT_MULTILINE;
   else
     imcontext->content_hint &= ~ZWP_TEXT_INPUT_V1_CONTENT_HINT_MULTILINE;
}

void
wayland_im_context_input_panel_language_set(Ecore_IMF_Context *ctx,
                                            Ecore_IMF_Input_Panel_Lang lang)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   if (lang == ECORE_IMF_INPUT_PANEL_LANG_ALPHABET)
     imcontext->content_hint |= ZWP_TEXT_INPUT_V1_CONTENT_HINT_LATIN;
   else
     imcontext->content_hint &= ~ZWP_TEXT_INPUT_V1_CONTENT_HINT_LATIN;
}

void
wayland_im_context_input_panel_language_locale_get(Ecore_IMF_Context *ctx,
                                                   char **locale)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   if (locale)
     *locale = strdup(imcontext->language ? imcontext->language : "");
}

void
wayland_im_context_prediction_allow_set(Ecore_IMF_Context *ctx,
                                        Eina_Bool prediction)
{
   WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

   if (prediction)
     imcontext->content_hint |= ZWP_TEXT_INPUT_V1_CONTENT_HINT_AUTO_COMPLETION;
   else
     imcontext->content_hint &= ~ZWP_TEXT_INPUT_V1_CONTENT_HINT_AUTO_COMPLETION;
}

WaylandIMContext *
wayland_im_context_new(struct zwp_text_input_manager_v1 *text_input_manager)
{
   WaylandIMContext *context = calloc(1, sizeof(WaylandIMContext));
   if (context)
     {
        EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom, "new context created");
        context->text_input_manager = text_input_manager;
     }

   return context;
}

/* vim:ts=8 sw=3 sts=3 expandtab cino=>5n-3f0^-2{2(0W1st0
*/
