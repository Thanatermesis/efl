#ifndef __IBUS_IM_CONTEXT_H_
#define __IBUS_IM_CONTEXT_H_

#include <Ecore_IMF.h>

/**
 * @brief Structure representing an IBus Input Method Context.
 * This structure holds all the necessary information for an IBus IM context,
 * including the Ecore IMF context, the IBus input context, preedit string
 * information, cursor position, focus state, and client window/canvas.
 */
typedef struct _IBusIMContext IBusIMContext;

/**
 * @brief Adds an IBus input method context to the Ecore IMF system.
 * This function initializes the IBus specific data for the given Ecore IMF context.
 * @param ctx The Ecore IMF context to which the IBus context will be added.
 */
void ecore_imf_context_ibus_add(Ecore_IMF_Context *ctx);

/**
 * @brief Deletes an IBus input method context from the Ecore IMF system.
 * This function cleans up and frees resources associated with the IBus context.
 * @param ctx The Ecore IMF context from which the IBus context will be deleted.
 */
void ecore_imf_context_ibus_del(Ecore_IMF_Context *ctx);

/**
 * @brief Resets the IBus input method context.
 * This typically involves clearing any active preedit string or composition state.
 * @param context The Ecore IMF context to reset.
 */
void ecore_imf_context_ibus_reset(Ecore_IMF_Context *context);

/**
 * @brief Notifies the IBus input method context that it has gained focus.
 * @param context The Ecore IMF context that gained focus.
 */
void ecore_imf_context_ibus_focus_in(Ecore_IMF_Context *context);

/**
 * @brief Notifies the IBus input method context that it has lost focus.
 * @param context The Ecore IMF context that lost focus.
 */
void ecore_imf_context_ibus_focus_out(Ecore_IMF_Context *context);

/**
 * @brief Retrieves the current preedit string from the IBus input method context.
 * @param context The Ecore IMF context.
 * @param str Pointer to a character pointer where the preedit string will be stored.
 *            The caller is responsible for freeing this string.
 * @param cursor_pos Pointer to an integer where the current cursor position within
 *                   the preedit string will be stored.
 */
void ecore_imf_context_ibus_preedit_string_get(Ecore_IMF_Context     *context,
                                               char                  **str,
                                               int                   *cursor_pos);

/**
 * @brief Retrieves the current preedit string along with its attributes.
 * Attributes describe visual properties (like underline, color) of segments
 * within the preedit string.
 * @param context The Ecore IMF context.
 * @param str Pointer to a character pointer where the preedit string will be stored.
 *            The caller is responsible for freeing this string.
 * @param attrs Pointer to an Eina_List pointer where the list of
 *              Ecore_IMF_Preedit_Attr attributes will be stored. The caller is
 *              responsible for freeing this list and its elements.
 *              Example of Ecore_IMF_Preedit_Attr structure:
 *              typedef struct _Ecore_IMF_Preedit_Attr Ecore_IMF_Preedit_Attr;
 *              struct _Ecore_IMF_Preedit_Attr {
 *                  Ecore_IMF_Preedit_Type preedit_type; // Type of preedit attribute (e.g., underline)
 *                  unsigned int           start_index;  // Start index of the attribute in the preedit string
 *                  unsigned int           end_index;    // End index of the attribute in the preedit string
 *              };
 * @param cursor_pos Pointer to an integer where the current cursor position within
 *                   the preedit string will be stored.
 */
void ecore_imf_context_ibus_preedit_string_with_attributes_get(Ecore_IMF_Context     *context,
                                                               char                  **str,
                                                               Eina_List             **attrs,
                                                               int                   *cursor_pos);

/**
 * @brief Sets the cursor location for the IBus input method context.
 * This informs the input method editor about the position of the text cursor
 * on the screen, which is often used to position candidate windows.
 * @param context The Ecore IMF context.
 * @param x The x-coordinate of the cursor.
 * @param y The y-coordinate of the cursor.
 * @param w The width of the cursor area.
 * @param h The height of the cursor area.
 */
void ecore_imf_context_ibus_cursor_location_set(Ecore_IMF_Context *context,
                                                int x, int y, int w, int h);

/**
 * @brief Sets whether the IBus input method context should use a preedit string.
 * @param context The Ecore IMF context.
 * @param use_preedit EINA_TRUE to enable preedit, EINA_FALSE to disable.
 */
void ecore_imf_context_ibus_use_preedit_set(Ecore_IMF_Context *context,
                                            Eina_Bool use_preedit);

/**
 * @brief Sets the client window for the IBus input method context.
 * The client window is the top-level window associated with the input field.
 * @param context The Ecore IMF context.
 * @param window A pointer to the client window (e.g., Ecore_X_Window).
 */
void ecore_imf_context_ibus_client_window_set(Ecore_IMF_Context *context, void *window);

/**
 * @brief Sets the client canvas for the IBus input method context.
 * The client canvas is the Evas canvas where text input occurs.
 * @param context The Ecore IMF context.
 * @param canvas A pointer to the client canvas (e.g., Evas *).
 */
void ecore_imf_context_ibus_client_canvas_set(Ecore_IMF_Context *context, void *canvas);

/**
 * @brief Filters an Ecore IMF event through the IBus input method context.
 * This function processes key events and determines if they are consumed by the
 * input method.
 * @param ctx The Ecore IMF context.
 * @param type The type of the Ecore IMF event (e.g., ECORE_IMF_EVENT_KEY_DOWN).
 * @param event The Ecore IMF event data.
 * @return EINA_TRUE if the event was consumed by IBus, EINA_FALSE otherwise.
 */
Eina_Bool ecore_imf_context_ibus_filter_event(Ecore_IMF_Context *ctx, Ecore_IMF_Event_Type type, Ecore_IMF_Event *event);

/**
 * @brief Creates a new IBus input method context.
 * This function initializes the IBus connection if not already done.
 * @return A pointer to the newly created IBusIMContext, or NULL on failure.
 */
IBusIMContext *ecore_imf_context_ibus_new(void);

/**
 * @brief Shuts down the IBus connection and cleans up global IBus resources.
 * This should be called when the application is exiting and no longer needs IBus.
 */
void ecore_imf_context_ibus_shutdown(void);
#endif
