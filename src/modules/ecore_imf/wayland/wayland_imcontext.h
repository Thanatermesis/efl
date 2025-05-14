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

#ifndef __WAYLAND_IM_CONTEXT_H_
#define __WAYLAND_IM_CONTEXT_H_

#include <Ecore_IMF.h>
#include <Ecore_Wl2.h>

#include "text-input-unstable-v1-client-protocol.h"

extern Ecore_Wl2_Display *ewd;

/**
 * @brief Structure representing a Wayland Input Method Context.
 * This structure holds all the necessary state for managing text input
 * via the Wayland text-input-unstable-v1 protocol.
 */
typedef struct _WaylandIMContext WaylandIMContext;

/**
 * @brief Adds (initializes) a Wayland input method context.
 * This function is typically called when the Ecore_IMF_Context is created.
 * It sets up the Wayland-specific parts of the input method context.
 * @param ctx The Ecore IMF context to associate with this Wayland context.
 */
void wayland_im_context_add                (Ecore_IMF_Context    *ctx);

/**
 * @brief Deletes (finalizes) a Wayland input method context.
 * This function is called when the Ecore_IMF_Context is being destroyed.
 * It cleans up Wayland-specific resources.
 * @param ctx The Ecore IMF context.
 */
void wayland_im_context_del                (Ecore_IMF_Context    *ctx);

/**
 * @brief Resets the Wayland input method context.
 * This typically involves clearing any preedit string and committing any pending text.
 * It also sends a reset request to the Wayland text input object.
 * @param ctx The Ecore IMF context.
 */
void wayland_im_context_reset              (Ecore_IMF_Context    *ctx);

/**
 * @brief Handles the focus-in event for the input method context.
 * This function is called when the widget associated with the context gains focus.
 * It activates the Wayland text input object.
 * @param ctx The Ecore IMF context.
 */
void wayland_im_context_focus_in           (Ecore_IMF_Context    *ctx);

/**
 * @brief Handles the focus-out event for the input method context.
 * This function is called when the widget associated with the context loses focus.
 * It deactivates the Wayland text input object.
 * @param ctx The Ecore IMF context.
 */
void wayland_im_context_focus_out          (Ecore_IMF_Context    *ctx);

/**
 * @brief Retrieves the current preedit string and cursor position.
 * @param ctx The Ecore IMF context.
 * @param str Pointer to a character pointer that will be updated to point to the
 *            newly allocated preedit string. The caller is responsible for freeing this string.
 * @param cursor_pos Pointer to an integer that will be updated with the current
 *                   cursor position (in characters) within the preedit string.
 */
void wayland_im_context_preedit_string_get (Ecore_IMF_Context    *ctx,
                                            char                **str,
                                            int                  *cursor_pos);

/**
 * @brief Retrieves the current preedit string, attributes, and cursor position.
 * @param ctx The Ecore IMF context.
 * @param str Pointer to a character pointer that will be updated to point to the
 *            newly allocated preedit string. The caller is responsible for freeing this string.
 * @param attr Pointer to an Eina_List pointer that will be updated with a list
 *             of Ecore_IMF_Preedit_Attr attributes. The caller is responsible for
 *             freeing the list and its contents.
 *             Example of Ecore_IMF_Preedit_Attr structure:
 *             - preedit_type: ECORE_IMF_PREEDIT_TYPE_SUB1, ECORE_IMF_PREEDIT_TYPE_SUB2, etc.
 *             - start_index: Starting character index of the attribute.
 *             - end_index: Ending character index of the attribute.
 * @param cursor_pos Pointer to an integer that will be updated with the current
 *                   cursor position (in characters) within the preedit string.
 */
void wayland_im_context_preedit_string_with_attributes_get(Ecore_IMF_Context  *ctx,
                                                           char              **str,
                                                           Eina_List         **attr,
                                                           int                *cursor_pos);

/**
 * @brief Sets the cursor position within the input field.
 * This function informs the input method about the current cursor position.
 * @param ctx The Ecore IMF context.
 * @param cursor_pos The new cursor position (in characters).
 */
void wayland_im_context_cursor_position_set(Ecore_IMF_Context    *ctx,
                                            int                   cursor_pos);

/**
 * @brief Sets whether to use the preedit string.
 * @param ctx The Ecore IMF context.
 * @param use_preedit EINA_TRUE to use preedit, EINA_FALSE otherwise.
 */
void wayland_im_context_use_preedit_set    (Ecore_IMF_Context    *ctx,
                                            Eina_Bool             use_preedit);

/**
 * @brief Sets the client window associated with the input method context.
 * The client window is the Ecore_Wl2_Window where text input occurs.
 * @param ctx The Ecore IMF context.
 * @param window Pointer to the client window (Ecore_Wl2_Window *).
 */
void wayland_im_context_client_window_set  (Ecore_IMF_Context    *ctx,
                                            void                 *window);

/**
 * @brief Sets the client canvas associated with the input method context.
 * The client canvas is the Evas canvas where text input occurs.
 * @param ctx The Ecore IMF context.
 * @param canvas Pointer to the client canvas (Evas *).
 */
void wayland_im_context_client_canvas_set  (Ecore_IMF_Context    *ctx,
                                            void                 *canvas);

/**
 * @brief Shows the input panel (e.g., virtual keyboard).
 * @param ctx The Ecore IMF context.
 */
void wayland_im_context_show               (Ecore_IMF_Context    *ctx);

/**
 * @brief Hides the input panel.
 * @param ctx The Ecore IMF context.
 */
void wayland_im_context_hide               (Ecore_IMF_Context    *ctx);

/**
 * @brief Filters an IMF event.
 * This function allows the Wayland IM context to process events before the application.
 * For example, it might show the input panel on a mouse up event.
 * @param ctx The Ecore IMF context.
 * @param type The type of the IMF event.
 * @param event The IMF event data.
 * @return EINA_TRUE if the event was handled and should not be processed further,
 *         EINA_FALSE otherwise.
 */
Eina_Bool wayland_im_context_filter_event  (Ecore_IMF_Context    *ctx,
                                            Ecore_IMF_Event_Type  type,
                                            Ecore_IMF_Event      *event);

/**
 * @brief Sets the location of the text cursor on the screen.
 * This information is used by the input method to position UI elements like candidate windows.
 * @param ctx The Ecore IMF context.
 * @param x The x-coordinate of the cursor rectangle.
 * @param y The y-coordinate of the cursor rectangle.
 * @param width The width of the cursor rectangle.
 * @param height The height of the cursor rectangle.
 */
void wayland_im_context_cursor_location_set(Ecore_IMF_Context    *ctx,
                                            int                   x,
                                            int                   y,
                                            int                   width,
                                            int                   height);

/**
 * @brief Sets the autocapitalization type for the input context.
 * @param ctx The Ecore IMF context.
 * @param autocapital_type The desired autocapitalization behavior.
 *                         Example: ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE
 */
void wayland_im_context_autocapital_type_set(Ecore_IMF_Context *ctx,
                                             Ecore_IMF_Autocapital_Type autocapital_type);

/**
 * @brief Sets the layout for the input panel.
 * This hints to the input method about the expected type of input (e.g., numbers, email).
 * @param ctx The Ecore IMF context.
 * @param layout The desired input panel layout.
 *               Example: ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBER
 */
void wayland_im_context_input_panel_layout_set(Ecore_IMF_Context *ctx,
                                               Ecore_IMF_Input_Panel_Layout layout);

/**
 * @brief Sets the input mode for the input context.
 * This can specify, for example, if the input should be invisible (for passwords).
 * @param ctx The Ecore IMF context.
 * @param input_mode The desired input mode.
 *                   Example: ECORE_IMF_INPUT_MODE_INVISIBLE
 */
void wayland_im_context_input_mode_set(Ecore_IMF_Context *ctx,
                                       Ecore_IMF_Input_Mode input_mode);

/**
 * @brief Sets input hints for the input context.
 * Hints can include auto-completion, sensitive data, or multiline input.
 * @param ctx The Ecore IMF context.
 * @param input_hints A bitmask of desired input hints.
 *                    Example: ECORE_IMF_INPUT_HINT_AUTO_COMPLETE | ECORE_IMF_INPUT_HINT_SENSITIVE_DATA
 */
void wayland_im_context_input_hint_set(Ecore_IMF_Context *ctx,
                                       Ecore_IMF_Input_Hints input_hints);

/**
 * @brief Sets the language for the input panel.
 * @param ctx The Ecore IMF context.
 * @param lang The desired input panel language type.
 *             Example: ECORE_IMF_INPUT_PANEL_LANG_ALPHABET
 */
void wayland_im_context_input_panel_language_set(Ecore_IMF_Context *ctx,
                                                 Ecore_IMF_Input_Panel_Lang lang);

/**
 * @brief Retrieves the current language locale of the input panel.
 * @param ctx The Ecore IMF context.
 * @param locale Pointer to a character pointer that will be updated to point to
 *               the newly allocated string containing the locale (e.g., "en_US").
 *               The caller is responsible for freeing this string.
 */
void
wayland_im_context_input_panel_language_locale_get(Ecore_IMF_Context *ctx,
                                                   char **locale);

/**
 * @brief Sets whether text prediction (autocompletion) is allowed.
 * @param ctx The Ecore IMF context.
 * @param prediction EINA_TRUE to allow prediction, EINA_FALSE otherwise.
 */
void
wayland_im_context_prediction_allow_set(Ecore_IMF_Context *ctx,
                                        Eina_Bool prediction);

/**
 * @brief Creates a new Wayland input method context.
 * This is typically called by the Ecore IMF Wayland module when it's initialized.
 * @param text_input_manager The Wayland text input manager global object.
 * @return A pointer to the newly created WaylandIMContext, or NULL on failure.
 */
WaylandIMContext *wayland_im_context_new        (struct zwp_text_input_manager_v1 *text_input_manager);

extern int _ecore_imf_wayland_log_dom;

#endif

/* vim:ts=8 sw=3 sts=3 expandtab cino=>5n-3f0^-2{2(0W1st0
 */
