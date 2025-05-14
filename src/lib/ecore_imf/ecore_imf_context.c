#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <Ecore.h>
#include <ecore_private.h>

#include "Ecore_IMF.h"
#include "ecore_imf_private.h"

/**
 * @internal
 * @brief Stores the context that last requested to be shown.
 * This is used to track which input method context is currently active or
 * was last active, particularly for managing input panel state.
 */
Ecore_IMF_Context *show_req_ctx = NULL;

/**
 * @brief Retrieves a list of available input method context IDs.
 *
 * This function queries the underlying input method framework for a list
 * of all available input method context identifiers (IDs). These IDs can
 * then be used to create specific input method contexts.
 *
 * @return A list of strings, where each string is an available context ID.
 *         The list should be freed using eina_list_free() after use, but
 *         the strings themselves are internal and should not be freed.
 *         Returns @c NULL on failure or if no contexts are available.
 * @see ecore_imf_module_context_ids_get()
 */
EAPI Eina_List *
ecore_imf_context_available_ids_get(void)
{
   return ecore_imf_module_context_ids_get();
}

/**
 * @brief Retrieves a list of available input method context IDs filtered by canvas type.
 *
 * This function queries the underlying input method framework for a list
 * of available input method context identifiers (IDs) that are compatible
 * with the specified canvas type.
 *
 * @param canvas_type A string representing the canvas type (e.g., "evas", "wayland").
 *                    If @c NULL, it may behave like ecore_imf_context_available_ids_get()
 *                    or return contexts suitable for a default canvas type.
 * @return A list of strings, where each string is an available context ID
 *         compatible with the given canvas type. The list should be freed
 *         using eina_list_free() after use, but the strings themselves are
 *         internal and should not be freed. Returns @c NULL on failure or
 *         if no compatible contexts are available.
 * @see ecore_imf_module_context_ids_by_canvas_type_get()
 */
EAPI Eina_List *
ecore_imf_context_available_ids_by_canvas_type_get(const char *canvas_type)
{
   return ecore_imf_module_context_ids_by_canvas_type_get(canvas_type);
}

/*
 * Match @locale against @against.
 *
 * 'en_US' against 'en_US'       => 4
 * 'en_US' against 'en'          => 3
 * 'en', 'en_UK' against 'en_US' => 2
 *  all locales, against '*'     => 1
 */
/* XXX: disable because nto used anymore - see below
 * ecore_imf_context_default_id_by_canvas_type_get
static int
_ecore_imf_context_match_locale(const char *locale, const char *against, int against_len)
{
   if (strcmp(against, "*") == 0)
     return 1;

   if (strcasecmp(locale, against) == 0)
     return 4;

   if (strncasecmp(locale, against, 2) == 0)
     return (against_len == 2) ? 3 : 2;

   return 0;
}
*/

/**
 * @brief Retrieves the default input method context ID.
 *
 * This function determines and returns the ID of the default input method
 * context. The selection might be influenced by environment variables or
 * system configuration. This is a convenience wrapper around
 * ecore_imf_context_default_id_by_canvas_type_get() with a @c NULL canvas_type.
 *
 * @return The ID of the default input method context as a string.
 *         This string is internal and should not be freed.
 *         Returns @c NULL if no default ID can be determined or if an error occurs.
 * @see ecore_imf_context_default_id_by_canvas_type_get()
 */
EAPI const char *
ecore_imf_context_default_id_get(void)
{
   return ecore_imf_context_default_id_by_canvas_type_get(NULL);
}

/**
 * @brief Retrieves the default input method context ID for a specific canvas type.
 *
 * This function determines the most suitable default input method context ID
 * based on the environment (e.g., ECORE_IMF_MODULE, WAYLAND_DISPLAY) and
 * optionally a canvas type.
 *
 * The logic prefers ECORE_IMF_MODULE if set. If not, and WAYLAND_DISPLAY
 * is set, it attempts to use the "wayland" IMF module.
 * The commented-out section previously involved more complex locale-based matching.
 *
 * @param canvas_type The type of canvas the context will be used with (e.g., "evas").
 *                    This parameter is currently marked as EINA_UNUSED, suggesting
 *                    it might not be fully utilized in the current implementation
 *                    for default ID selection but could be relevant for module compatibility.
 * @return The ID of the default input method context as a string.
 *         This string is internal and should not be freed.
 *         Returns @c NULL if no suitable default ID can be determined or if an error occurs.
 *         If ECORE_IMF_MODULE is "none", it returns @c NULL.
 */
EAPI const char *
ecore_imf_context_default_id_by_canvas_type_get(const char *canvas_type EINA_UNUSED)
{
   const char *id = getenv("ECORE_IMF_MODULE");

   if (id)
     {
        if (strcmp(id, "none") == 0) return NULL;
        if (ecore_imf_module_get(id)) return id;
     }
   else
     {
        if (getenv("WAYLAND_DISPLAY"))
          {
             id = "wayland";
             if (ecore_imf_module_get(id)) return id;
          }
     }
   return NULL;
/* XXX: I am not sure we need/want this. this causes imf modules to be
 * used where ECORE_IMF_MODULE is not set (and this causes issues with things
 * like scim where on some distros and some versions an scim connect BLOCKS
 * and if no scim is around this means an app blocks/hangs ... so disable
 * this so either you are in wayland mode OR you have to set
 * ECORE_IMF_MODULE
   Eina_List *modules;
   Ecore_IMF_Module *module;
   char *locale;
   char *tmp;
   int best_goodness = 0;

   modules = ecore_imf_module_available_get();
   if (!modules) return NULL;

   locale = setlocale(LC_CTYPE, NULL);
   if (!locale) return NULL;

   locale = strdup(locale);

   tmp = strchr(locale, '.');
   if (tmp) *tmp = '\0';
   tmp = strchr(locale, '@');
   if (tmp) *tmp = '\0';

   id = NULL;

   EINA_LIST_FREE(modules, module)
     {
        if (canvas_type &&
            strcmp(module->info->canvas_type, canvas_type) == 0)
          continue;

        const char *p = module->info->default_locales;
        while (p)
          {
             const char *q = strchr(p, ':');
             int goodness = _ecore_imf_context_match_locale(locale, p, q ? (size_t)(q - p) : strlen (p));

             if (goodness > best_goodness)
               {
                  id = module->info->id;
                  best_goodness = goodness;
               }

             p = q ? q + 1 : NULL;
          }
     }

   free(locale);
   return id;
 */
}

/**
 * @brief Retrieves information about an input method context by its ID.
 *
 * Given an input method context ID, this function fetches the associated
 * Ecore_IMF_Context_Info structure, which contains details about the
 * context module (like its name, description, canvas type, etc.).
 *
 * @param id The identifier string of the input method context.
 * @return A pointer to a const Ecore_IMF_Context_Info structure containing
 *         information about the specified context ID. This structure is
 *         internal and should not be modified or freed.
 *         Returns @c NULL if the ID is invalid or the module cannot be found.
 */
EAPI const Ecore_IMF_Context_Info *
ecore_imf_context_info_by_id_get(const char *id)
{
   Ecore_IMF_Module *module;

   if (!id) return NULL;
   module = ecore_imf_module_get(id);
   if (!module) return NULL;
   return module->info;
}

/**
 * @brief Creates and initializes a new input method context using the given ID.
 *
 * This function instantiates an input method context corresponding to the
 * provided ID. It involves several initialization steps:
 * - Creating the context via the module.
 * - Calling the context's add() method if available.
 * - Setting default values for various properties like:
 *   - use_preedit (enabled)
 *   - prediction_allow (enabled)
 *   - autocapital_type (sentence)
 *   - input_hint (auto_complete)
 *   - input_panel_enabled (enabled)
 *   - input_panel_layout (normal)
 *   - input_mode (full)
 *   - bidi_direction (neutral)
 *
 * @param id The identifier string of the input method context to create.
 *           Example: "xim", "ibus", "wayland".
 * @return A pointer to the newly created Ecore_IMF_Context.
 *         Returns @c NULL if the ID is invalid, the module cannot be found,
 *         or context creation fails. The returned context should be freed
 *         using ecore_imf_context_del() when no longer needed.
 */
EAPI Ecore_IMF_Context *
ecore_imf_context_add(const char *id)
{
   Ecore_IMF_Context *ctx;

   if (!id) return NULL;
   ctx = ecore_imf_module_context_create(id);
   if (!ctx || !ctx->klass) return NULL;
   if (ctx->klass->add) ctx->klass->add(ctx);
   /* default use_preedit is EINA_TRUE, so let's make sure it's
    * set on the immodule */
   ecore_imf_context_use_preedit_set(ctx, EINA_TRUE);

   /* default prediction is EINA_TRUE, so let's make sure it's
    * set on the immodule */
   ecore_imf_context_prediction_allow_set(ctx, EINA_TRUE);

   /* default autocapital type is SENTENCE type, so let's make sure it's
    * set on the immodule */
   ecore_imf_context_autocapital_type_set(ctx, ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE);

   /* default input hint */
   ecore_imf_context_input_hint_set(ctx, ECORE_IMF_INPUT_HINT_AUTO_COMPLETE);

   /* default input panel enabled status is EINA_TRUE, so let's make sure it's
    * set on the immodule */
   ecore_imf_context_input_panel_enabled_set(ctx, EINA_TRUE);

   /* default input panel layout type is NORMAL type, so let's make sure it's
    * set on the immodule */
   ecore_imf_context_input_panel_layout_set(ctx, ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL);

   /* default input_mode is ECORE_IMF_INPUT_MODE_FULL, so let's make sure it's
    * set on the immodule */
   ecore_imf_context_input_mode_set(ctx, ECORE_IMF_INPUT_MODE_FULL);

   ecore_imf_context_bidi_direction_set(ctx, ECORE_IMF_BIDI_DIRECTION_NEUTRAL);

   return ctx;
}

/**
 * @brief Retrieves the information structure for a given input method context.
 *
 * This function returns a pointer to the Ecore_IMF_Context_Info structure
 * associated with the provided Ecore_IMF_Context instance. This structure
 * contains static information about the input method module that created
 * this context.
 *
 * @param ctx The input method context.
 * @return A pointer to a const Ecore_IMF_Context_Info structure.
 *         This structure is internal and should not be modified or freed.
 *         Returns @c NULL if @p ctx is invalid.
 */
EAPI const Ecore_IMF_Context_Info *
ecore_imf_context_info_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_info_get");
        return NULL;
     }
   return ctx->module->info;
}

/**
 * @brief Deletes an input method context.
 *
 * This function cleans up and frees resources associated with an
 * Ecore_IMF_Context. It performs several actions:
 * - If this context was the last one to request showing (show_req_ctx),
 *   it clears show_req_ctx.
 * - Calls the context's del() method if available.
 * - Frees any registered event callbacks.
 * - Frees any registered input panel event callbacks.
 * - Frees the prediction hint hash table if it exists.
 * - Invalidates the context's magic number and frees the context structure itself.
 *
 * @param ctx The input method context to delete.
 */
EAPI void
ecore_imf_context_del(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Func_Node *fn;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_del");
        return;
     }

   if (show_req_ctx == ctx)
     show_req_ctx = NULL;

   if (ctx->klass && ctx->klass->del) ctx->klass->del(ctx);

   if (ctx->callbacks)
     {
        EINA_LIST_FREE(ctx->callbacks, fn)
           free(fn);
     }

   if (ctx->input_panel_callbacks)
     {
        EINA_LIST_FREE(ctx->input_panel_callbacks, fn)
           free(fn);
     }

   if (ctx->prediction_hint_hash)
     eina_hash_free(ctx->prediction_hint_hash);

   ECORE_MAGIC_SET(ctx, ECORE_MAGIC_NONE);
   free(ctx);
}

/**
 * @brief Sets the client window for an input method context.
 *
 * Associates a client window (e.g., an Ecore_Evas window, an X11 window ID)
 * with the input method context. This window information is often necessary
 * for the input method to position its UI elements (like candidate windows)
 * correctly or to manage focus.
 *
 * @param ctx The input method context.
 * @param window A pointer to the client window. The exact type and meaning
 *               depend on the specific IMF module and canvas type.
 */
EAPI void
ecore_imf_context_client_window_set(Ecore_IMF_Context *ctx, void *window)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_client_window_set");
        return;
     }

   if (ctx->klass && ctx->klass->client_window_set) ctx->klass->client_window_set(ctx, window);
   ctx->window = window;
}

/**
 * @brief Gets the client window associated with an input method context.
 *
 * Retrieves the client window that was previously set using
 * ecore_imf_context_client_window_set().
 *
 * @param ctx The input method context.
 * @return A pointer to the client window. Returns @c NULL if no window
 *         is set or if @p ctx is invalid.
 */
EAPI void *
ecore_imf_context_client_window_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_client_window_get");
        return NULL;
     }
   return ctx->window;
}

/**
 * @brief Sets the client canvas for an input method context.
 *
 * Associates a client canvas (e.g., an Evas canvas) with the input method
 * context. This information can be used by the input method, for example,
 * to understand the rendering surface or coordinate system.
 *
 * @param ctx The input method context.
 * @param canvas A pointer to the client canvas. The exact type and meaning
 *               depend on the specific IMF module and canvas type.
 */
EAPI void
ecore_imf_context_client_canvas_set(Ecore_IMF_Context *ctx, void *canvas)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_client_canvas_set");
        return;
     }

   if (ctx->klass && ctx->klass->client_canvas_set) ctx->klass->client_canvas_set(ctx, canvas);
   ctx->client_canvas = canvas;
}

/**
 * @brief Gets the client canvas associated with an input method context.
 *
 * Retrieves the client canvas that was previously set using
 * ecore_imf_context_client_canvas_set().
 *
 * @param ctx The input method context.
 * @return A pointer to the client canvas. Returns @c NULL if no canvas
 *         is set or if @p ctx is invalid.
 */
EAPI void *
ecore_imf_context_client_canvas_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_client_canvas_get");
        return NULL;
     }
   return ctx->client_canvas;
}

/**
 * @brief Requests the input method context to show its UI.
 *
 * This function signals the input method context that it should make its
 * user interface (e.g., preedit string display, candidate window) visible.
 * It also updates `show_req_ctx` to this context.
 *
 * @param ctx The input method context.
 */
EAPI void
ecore_imf_context_show(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_show");
        return;
     }

   show_req_ctx = ctx;
   if (ctx->klass && ctx->klass->show) ctx->klass->show(ctx);
}

/**
 * @brief Requests the input method context to hide its UI.
 *
 * This function signals the input method context that it should hide its
 * user interface elements.
 *
 * @param ctx The input method context.
 */
EAPI void
ecore_imf_context_hide(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_hide");
        return;
     }

   if (ctx->klass && ctx->klass->hide) ctx->klass->hide(ctx);
}

/**
 * @brief Retrieves the current preedit string and cursor position.
 *
 * The preedit string is the text currently being composed by the input method
 * but not yet committed to the application. This function fetches this string
 * and the current position of the cursor within it.
 *
 * @param ctx The input method context.
 * @param[out] str A pointer to a character pointer that will be updated to
 *                 point to a newly allocated string containing the preedit text.
 *                 The caller is responsible for freeing this string using free().
 *                 If no preedit string exists or an error occurs, it's set to
 *                 a an empty, allocated string.
 * @param[out] cursor_pos A pointer to an integer that will be updated with the
 *                        byte offset of the cursor within the preedit string.
 *                        If no preedit string or an error, it's set to 0.
 */
EAPI void
ecore_imf_context_preedit_string_get(Ecore_IMF_Context *ctx, char **str, int *cursor_pos)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_preedit_string_get");
        return;
     }

   if (ctx->klass && ctx->klass->preedit_string_get)
     ctx->klass->preedit_string_get(ctx, str, cursor_pos);
   else
     {
        if (str) *str = strdup("");
        if (cursor_pos) *cursor_pos = 0;
     }
}

/**
 * @brief Retrieves the current preedit string, its attributes, and cursor position.
 *
 * Similar to ecore_imf_context_preedit_string_get(), but additionally retrieves
 * a list of attributes associated with the preedit string. These attributes
 * provide styling or other semantic information for parts of the preedit text
 * (e.g., underlining, color).
 *
 * Each element in the @p attrs list is an `Ecore_IMF_Preedit_Attr`.
 * The caller is responsible for freeing the @p str string and the @p attrs list
 * (including its elements if they were allocated by the IMF module).
 *
 * @param ctx The input method context.
 * @param[out] str A pointer to a character pointer that will be updated to
 *                 point to a newly allocated string containing the preedit text.
 *                 The caller must free this string. Set to an empty allocated
 *                 string on error or if no preedit.
 * @param[out] attrs A pointer to an Eina_List pointer that will be updated to
 *                   point to a list of Ecore_IMF_Preedit_Attr structures.
 *                   The caller must free this list (and potentially its contents,
 *                   depending on the IMF module). Set to @c NULL on error or if no attributes.
 * @param[out] cursor_pos A pointer to an integer that will be updated with the
 *                        byte offset of the cursor within the preedit string.
 *                        Set to 0 on error or if no preedit.
 */
EAPI void
ecore_imf_context_preedit_string_with_attributes_get(Ecore_IMF_Context *ctx, char **str, Eina_List **attrs, int *cursor_pos)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_preedit_string_with_attributes_get");
        return;
     }
   if (ctx->klass && ctx->klass->preedit_string_with_attributes_get)
     ctx->klass->preedit_string_with_attributes_get(ctx, str, attrs, cursor_pos);
   else
     {
        if (str) *str = strdup("");
        if (attrs) *attrs = NULL;
        if (cursor_pos) *cursor_pos = 0;
     }
}

/**
 * @brief Notifies the input method context that it has gained focus.
 *
 * This function should be called when the widget associated with this
 * input method context receives input focus. The IMF module might use this
 * to activate itself or change its state.
 *
 * @param ctx The input method context.
 */
EAPI void
ecore_imf_context_focus_in(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_focus_in");
        return;
     }

   if (ctx->klass && ctx->klass->focus_in) ctx->klass->focus_in(ctx);
}

/**
 * @brief Notifies the input method context that it has lost focus.
 *
 * This function should be called when the widget associated with this
 * input method context loses input focus. The IMF module might use this
 * to deactivate itself, commit any pending preedit text, or hide its UI.
 *
 * @param ctx The input method context.
 */
EAPI void
ecore_imf_context_focus_out(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_focus_out");
        return;
     }

   if (ctx->klass && ctx->klass->focus_out) ctx->klass->focus_out(ctx);
}

/**
 * @brief Resets the input method context.
 *
 * This function instructs the input method context to clear any internal
 * state, such as the preedit string or conversion status. It's often called
 * when the input field content is cleared or significantly changed externally.
 *
 * @param ctx The input method context.
 */
EAPI void
ecore_imf_context_reset(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_reset");
        return;
     }

   if (ctx->klass && ctx->klass->reset) ctx->klass->reset(ctx);
}

/**
 * @brief Sets the cursor position within the input field.
 *
 * This function informs the input method context about the current cursor
 * position (caret location) in the text entry widget it's associated with.
 * The position is typically a byte offset from the beginning of the text.
 *
 * @param ctx The input method context.
 * @param cursor_pos The new cursor position (byte offset).
 */
EAPI void
ecore_imf_context_cursor_position_set(Ecore_IMF_Context *ctx, int cursor_pos)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_cursor_position_set");
        return;
     }

   if (ctx->klass && ctx->klass->cursor_position_set) ctx->klass->cursor_position_set(ctx, cursor_pos);
}

/**
 * @brief Sets the location of the cursor (caret) on the screen.
 *
 * This function informs the input method context about the screen coordinates
 * and dimensions of the cursor (or the character at the cursor position).
 * This is crucial for IMs that display UI elements (like candidate lists)
 * near the cursor. Coordinates are usually relative to the client window.
 *
 * @param ctx The input method context.
 * @param x The x-coordinate of the cursor rectangle.
 * @param y The y-coordinate of the cursor rectangle.
 * @param w The width of the cursor rectangle.
 * @param h The height of the cursor rectangle.
 */
EAPI void
ecore_imf_context_cursor_location_set(Ecore_IMF_Context *ctx, int x, int y, int w, int h)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_cursor_location_set");
        return;
     }
   if (ctx->klass && ctx->klass->cursor_location_set) ctx->klass->cursor_location_set(ctx, x, y, w, h);
}

/**
 * @brief Enables or disables the use of preedit string for the context.
 *
 * This function tells the input method context whether the application
 * can handle and display a preedit string. If disabled, the IM might
 * operate in a direct input mode or use other mechanisms.
 * By default, preedit is enabled when a context is created with
 * ecore_imf_context_add().
 *
 * @param ctx The input method context.
 * @param use_preedit @c EINA_TRUE to enable preedit, @c EINA_FALSE to disable.
 */
EAPI void
ecore_imf_context_use_preedit_set(Ecore_IMF_Context *ctx, Eina_Bool use_preedit)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_use_preedit_set");
        return;
     }
   if (ctx->klass && ctx->klass->use_preedit_set) ctx->klass->use_preedit_set(ctx, use_preedit);
}

/**
 * @brief Sets whether text prediction (autocompletion) is allowed for the context.
 *
 * This function informs the input method context if text prediction features
 * (like suggesting words or completing phrases) should be enabled.
 * The actual availability and behavior of prediction depend on the IMF module.
 * By default, prediction is allowed when a context is created.
 *
 * @param ctx The input method context.
 * @param prediction @c EINA_TRUE to allow prediction, @c EINA_FALSE to disallow.
 */
EAPI void
ecore_imf_context_prediction_allow_set(Ecore_IMF_Context *ctx, Eina_Bool prediction)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_prediction_allow_set");
        return;
     }

   if (ctx->allow_prediction != prediction)
     {
        ctx->allow_prediction = prediction;

        if (ctx->klass && ctx->klass->prediction_allow_set)
          ctx->klass->prediction_allow_set(ctx, prediction);
     }
}

/**
 * @brief Gets whether text prediction (autocompletion) is allowed for the context.
 *
 * Retrieves the current state of the prediction allowance flag set by
 * ecore_imf_context_prediction_allow_set().
 *
 * @param ctx The input method context.
 * @return @c EINA_TRUE if prediction is allowed, @c EINA_FALSE otherwise.
 *         Returns @c EINA_FALSE if @p ctx is invalid.
 */
EAPI Eina_Bool
ecore_imf_context_prediction_allow_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_prediction_allow_get");
        return EINA_FALSE;
     }

   return ctx->allow_prediction;
}

/**
 * @brief Sets the autocapitalization type for the input method context.
 *
 * This function configures the desired autocapitalization behavior for the
 * input context (e.g., capitalize first letter of sentences, words, all characters, or none).
 * The actual implementation depends on the IMF module.
 * By default, sentence-level autocapitalization is set.
 *
 * @param ctx The input method context.
 * @param autocapital_type The desired autocapitalization type from the
 *                         #Ecore_IMF_Autocapital_Type enumeration.
 *                         Example: ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE.
 */
EAPI void
ecore_imf_context_autocapital_type_set(Ecore_IMF_Context *ctx, Ecore_IMF_Autocapital_Type autocapital_type)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_autocapital_type_set");
        return;
     }

   if (ctx->autocapital_type != autocapital_type)
     {
        ctx->autocapital_type = autocapital_type;

        if (ctx->klass && ctx->klass->autocapital_type_set) ctx->klass->autocapital_type_set(ctx, autocapital_type);
     }
}

/**
 * @brief Gets the current autocapitalization type for the input method context.
 *
 * Retrieves the autocapitalization type previously set by
 * ecore_imf_context_autocapital_type_set().
 *
 * @param ctx The input method context.
 * @return The current #Ecore_IMF_Autocapital_Type.
 *         Returns ECORE_IMF_AUTOCAPITAL_TYPE_NONE if @p ctx is invalid.
 */
EAPI Ecore_IMF_Autocapital_Type
ecore_imf_context_autocapital_type_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_autocapital_allow_get"); // Note: Mismatch in error message string
        return ECORE_IMF_AUTOCAPITAL_TYPE_NONE;
     }

   return ctx->autocapital_type;
}

/**
 * @brief Sets a callback function to retrieve surrounding text.
 *
 * This function registers a callback that the input method context can invoke
 * to get the text surrounding the current cursor position from the application.
 * This is essential for context-aware input methods.
 *
 * The callback function `func` should:
 * - Allocate memory for `*text` and fill it with the surrounding text.
 * - Set `*cursor_pos` to the byte offset of the current cursor within `*text`.
 * - Return `EINA_TRUE` on success, `EINA_FALSE` on failure.
 * The IMF module is responsible for freeing the `*text` if it was allocated by the callback.
 *
 * @param ctx The input method context.
 * @param func The callback function.
 * @param data User data to be passed to the callback function.
 */
EAPI void
ecore_imf_context_retrieve_surrounding_callback_set(Ecore_IMF_Context *ctx, Eina_Bool (*func)(void *data, Ecore_IMF_Context *ctx, char **text, int *cursor_pos), const void *data)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_retrieve_surrounding_callback_set");
        return;
     }

   ctx->retrieve_surrounding_func = func;
   ctx->retrieve_surrounding_data = (void *) data;
}

/**
 * @brief Sets a callback function to retrieve selected text.
 *
 * This function registers a callback that the input method context can invoke
 * to get the currently selected text from the application.
 *
 * The callback function `func` should:
 * - Allocate memory for `*text` and fill it with the selected text.
 * - Return `EINA_TRUE` on success, `EINA_FALSE` on failure.
 * The IMF module is responsible for freeing the `*text` if it was allocated by the callback.
 *
 * @param ctx The input method context.
 * @param func The callback function.
 * @param data User data to be passed to the callback function.
 */
EAPI void
ecore_imf_context_retrieve_selection_callback_set(Ecore_IMF_Context *ctx, Eina_Bool (*func)(void *data, Ecore_IMF_Context *ctx, char **text), const void *data)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_retrieve_selection_callback_set");
        return;
     }

   ctx->retrieve_selection_func = func;
   ctx->retrieve_selection_data = (void *) data;
}

/**
 * @brief Sets the input mode for the context.
 *
 * This function informs the input method context about the desired input mode
 * (e.g., full, numeric, telephone, etc.). The IMF module can use this to
 * optimize its behavior or change the layout of an on-screen keyboard.
 * By default, ECORE_IMF_INPUT_MODE_FULL is set.
 *
 * @param ctx The input method context.
 * @param input_mode The desired input mode from the #Ecore_IMF_Input_Mode enumeration.
 *                   Example: ECORE_IMF_INPUT_MODE_NUMERIC.
 */
EAPI void
ecore_imf_context_input_mode_set(Ecore_IMF_Context *ctx, Ecore_IMF_Input_Mode input_mode)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_mode_set");
        return;
     }
   if (ctx->klass && ctx->klass->input_mode_set) ctx->klass->input_mode_set(ctx, input_mode);
   ctx->input_mode = input_mode;
}

/**
 * @brief Gets the current input mode of the context.
 *
 * Retrieves the input mode previously set by ecore_imf_context_input_mode_set().
 *
 * @param ctx The input method context.
 * @return The current #Ecore_IMF_Input_Mode.
 *         Returns 0 (which might map to ECORE_IMF_INPUT_MODE_FULL or an invalid state)
 *         if @p ctx is invalid.
 */
EAPI Ecore_IMF_Input_Mode
ecore_imf_context_input_mode_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_mode_get");
        return 0;
     }
   return ctx->input_mode;
}

/**
 * @brief Allows the input method context to filter (process) an input event.
 *
 * This function passes an input event (like key press/release) to the
 * input method context. The IM can then process this event. If the IM
 * handles the event (e.g., uses it for composition), it should return
 * @c EINA_TRUE, indicating that the application should not process this
 * event further. If the IM does not handle the event, it should return
 * @c EINA_FALSE.
 *
 * @param ctx The input method context.
 * @param type The type of the event (e.g., ECORE_IMF_EVENT_KEY_DOWN, ECORE_IMF_EVENT_KEY_UP).
 *             From the #Ecore_IMF_Event_Type enumeration.
 * @param event A pointer to an #Ecore_IMF_Event union containing the event details.
 *              The specific member of the union to use depends on @p type.
 *              Example: for ECORE_IMF_EVENT_KEY_DOWN, use event->key_down.
 * @return @c EINA_TRUE if the event was handled by the input method context,
 *         @c EINA_FALSE otherwise. Returns @c EINA_FALSE if @p ctx is invalid
 *         or if the context has no filter_event method.
 */
EAPI Eina_Bool
ecore_imf_context_filter_event(Ecore_IMF_Context *ctx, Ecore_IMF_Event_Type type, Ecore_IMF_Event *event)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_filter_event");
        return EINA_FALSE;
     }
   if (ctx->klass && ctx->klass->filter_event) return ctx->klass->filter_event(ctx, type, event);
   return EINA_FALSE;
}

/**
 * @brief Creates a new, generic input method context.
 * @internal
 *
 * This function is typically used by IMF modules themselves to allocate
 * a base Ecore_IMF_Context structure. The module then populates it with
 * its specific class functions and data. Applications usually use
 * ecore_imf_context_add() to get a fully functional context.
 *
 * Initializes default values:
 * - data = NULL
 * - retrieve_surrounding_func = NULL
 * - retrieve_surrounding_data = NULL
 *
 * @param ctxc A pointer to the Ecore_IMF_Context_Class structure that defines
 *             the behavior of this context type.
 * @return A pointer to the newly allocated Ecore_IMF_Context, or @c NULL on failure.
 *         The returned context should be freed using ecore_imf_context_del()
 *         or by the module's specific cleanup.
 */
EAPI Ecore_IMF_Context *
ecore_imf_context_new(const Ecore_IMF_Context_Class *ctxc)
{
   Ecore_IMF_Context *ctx;

   if (!ctxc) return NULL;
   ctx = calloc(1, sizeof(Ecore_IMF_Context));
   if (!ctx) return NULL;
   ECORE_MAGIC_SET(ctx, ECORE_MAGIC_CONTEXT);
   ctx->klass = ctxc;
   ctx->data = NULL;
   ctx->retrieve_surrounding_func = NULL;
   ctx->retrieve_surrounding_data = NULL;
   return ctx;
}

/**
 * @brief Sets private data for an input method context.
 * @internal
 *
 * This function allows an IMF module to associate its own private data
 * structure with an Ecore_IMF_Context instance.
 *
 * @param ctx The input method context.
 * @param data A pointer to the private data to associate.
 */
EAPI void
ecore_imf_context_data_set(Ecore_IMF_Context *ctx, void *data)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_data_set");
        return;
     }
   ctx->data = data;
}

/**
 * @brief Gets private data from an input method context.
 * @internal
 *
 * This function allows an IMF module to retrieve its private data structure
 * previously associated with an Ecore_IMF_Context instance using
 * ecore_imf_context_data_set().
 *
 * @param ctx The input method context.
 * @return A pointer to the private data, or @c NULL if none is set or @p ctx is invalid.
 */
EAPI void *
ecore_imf_context_data_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_data_get");
        return NULL;
     }
   return ctx->data;
}

/**
 * @brief Retrieves the surrounding text and cursor position via the registered callback.
 *
 * This function invokes the callback previously set by
 * ecore_imf_context_retrieve_surrounding_callback_set() to get the
 * text surrounding the current cursor position from the application.
 *
 * If the callback is successful:
 * - `*text` will point to a newly allocated string with the surrounding text.
 *   The caller (usually the IMF module that initiated this call) is responsible
 *   for freeing this string.
 * - `*cursor_pos` will contain the byte offset of the cursor within `*text`.
 *
 * If the callback is not set or fails:
 * - `*text` will be set to @c NULL.
 * - `*cursor_pos` will be set to 0.
 *
 * @param ctx The input method context.
 * @param[out] text A pointer to a char pointer, which will be updated to point
 *                  to the surrounding text.
 * @param[out] cursor_pos A pointer to an int, which will be updated with the
 *                        cursor position within the surrounding text.
 * @return @c EINA_TRUE if the surrounding text was successfully retrieved,
 *         @c EINA_FALSE otherwise (e.g., no callback set, callback failed).
 *         Also returns @c EINA_FALSE if @p ctx is invalid.
 */
EAPI Eina_Bool
ecore_imf_context_surrounding_get(Ecore_IMF_Context *ctx, char **text, int *cursor_pos)
{
   int result = EINA_FALSE;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_surrounding_get");
        return EINA_FALSE;
     }

   if (ctx->retrieve_surrounding_func)
     {
        result = ctx->retrieve_surrounding_func(ctx->retrieve_surrounding_data, ctx, text, cursor_pos);
        if (!result)
          {
             if (text) *text = NULL;
             if (cursor_pos) *cursor_pos = 0;
          }
     }
   return result;
}

/**
 * @brief Retrieves the currently selected text via the registered callback.
 *
 * This function invokes the callback previously set by
 * ecore_imf_context_retrieve_selection_callback_set() to get the
 * currently selected text from the application.
 *
 * If the callback is successful:
 * - `*text` will point to a newly allocated string with the selected text.
 *   The caller (usually the IMF module that initiated this call) is responsible
 *   for freeing this string.
 *
 * If the callback is not set or fails:
 * - `*text` will be set to @c NULL.
 *
 * @param ctx The input method context.
 * @param[out] text A pointer to a char pointer, which will be updated to point
 *                  to the selected text.
 * @return @c EINA_TRUE if the selection was successfully retrieved,
 *         @c EINA_FALSE otherwise (e.g., no callback set, callback failed).
 *         Also returns @c EINA_FALSE if @p ctx is invalid.
 */
EAPI Eina_Bool
ecore_imf_context_selection_get(Ecore_IMF_Context *ctx, char **text)
{
   Eina_Bool result = EINA_FALSE;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_selection_get");
        return EINA_FALSE;
     }

   if (ctx->retrieve_selection_func)
     {
        result = ctx->retrieve_selection_func(ctx->retrieve_selection_data, ctx, text);
        if (!result)
          {
             if (text) *text = NULL;
          }
     }
   return result;
}

/**
 * @internal
 * @brief Frees memory allocated for a generic preedit-related event.
 * This function is a simple wrapper around free() and is used as a callback
 * for ecore_event_add when dispatching preedit start, end, or changed events.
 *
 * @param data User data associated with the event (unused).
 * @param event Pointer to the event structure to be freed.
 */
static void
_ecore_imf_event_free_preedit(void *data EINA_UNUSED, void *event)
{
   free(event);
}

/**
 * @brief Adds a preedit start event to the Ecore event queue.
 *
 * This function is called by an IMF module to signal that a preedit
 * session has started for the given context. An ECORE_IMF_EVENT_PREEDIT_START
 * event will be emitted.
 *
 * @param ctx The input method context for which preedit has started.
 */
EAPI void
ecore_imf_context_preedit_start_event_add(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Event_Preedit_Start *ev;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_preedit_start_event_add");
        return;
     }

   ev = malloc(sizeof(Ecore_IMF_Event_Preedit_Start));
   EINA_SAFETY_ON_NULL_RETURN(ev);

   ev->ctx = ctx;
   ecore_event_add(ECORE_IMF_EVENT_PREEDIT_START,
                   ev, _ecore_imf_event_free_preedit, NULL);
}

/**
 * @brief Adds a preedit end event to the Ecore event queue.
 *
 * This function is called by an IMF module to signal that a preedit
 * session has ended for the given context. An ECORE_IMF_EVENT_PREEDIT_END
 * event will be emitted. This typically happens when preedit text is committed
 * or cleared.
 *
 * @param ctx The input method context for which preedit has ended.
 */
EAPI void
ecore_imf_context_preedit_end_event_add(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Event_Preedit_End *ev;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_preedit_end_event_add");
        return;
     }

   ev = malloc(sizeof(Ecore_IMF_Event_Preedit_End));
   EINA_SAFETY_ON_NULL_RETURN(ev);

   ev->ctx = ctx;
   ecore_event_add(ECORE_IMF_EVENT_PREEDIT_END,
                   ev, _ecore_imf_event_free_preedit, NULL);
}

/**
 * @brief Adds a preedit changed event to the Ecore event queue.
 *
 * This function is called by an IMF module to signal that the preedit string
 * (or its attributes, or cursor position) has changed for the given context.
 * An ECORE_IMF_EVENT_PREEDIT_CHANGED event will be emitted. Applications
 * should then call ecore_imf_context_preedit_string_with_attributes_get()
 * to get the updated preedit information.
 *
 * @param ctx The input method context for which preedit has changed.
 */
EAPI void
ecore_imf_context_preedit_changed_event_add(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Event_Preedit_Changed *ev;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_preedit_changed_event_add");
        return;
     }

   ev = malloc(sizeof(Ecore_IMF_Event_Preedit_Changed));
   EINA_SAFETY_ON_NULL_RETURN(ev);

   ev->ctx = ctx;
   ecore_event_add(ECORE_IMF_EVENT_PREEDIT_CHANGED,
                   ev, _ecore_imf_event_free_preedit, NULL);
}

/**
 * @internal
 * @brief Frees memory allocated for a commit event.
 * This function is used as a callback for ecore_event_add when dispatching
 * commit events. It frees the event structure itself and the duplicated
 * commit string within it.
 *
 * @param data User data associated with the event (unused).
 * @param event Pointer to the Ecore_IMF_Event_Commit structure to be freed.
 */
static void
_ecore_imf_event_free_commit(void *data EINA_UNUSED, void *event)
{
   Ecore_IMF_Event_Commit *ev;

   ev = event;
   if (ev->str) free(ev->str);
   free(ev);
}

/**
 * @brief Adds a commit event to the Ecore event queue.
 *
 * This function is called by an IMF module to signal that a string
 * should be committed (i.e., inserted into the application's text widget).
 * An ECORE_IMF_EVENT_COMMIT event will be emitted, carrying the string
 * to be committed. The provided string @p str is duplicated.
 *
 * @param ctx The input method context that is committing the string.
 * @param str The string to be committed. Can be @c NULL, in which case
 *            the event's string field will also be @c NULL.
 */
EAPI void
ecore_imf_context_commit_event_add(Ecore_IMF_Context *ctx, const char *str)
{
   Ecore_IMF_Event_Commit *ev;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_commit_event_add");
        return;
     }

   ev = malloc(sizeof(Ecore_IMF_Event_Commit));
   EINA_SAFETY_ON_NULL_RETURN(ev);

   ev->ctx = ctx;
   ev->str = str ? strdup(str) : NULL;
   ecore_event_add(ECORE_IMF_EVENT_COMMIT,
                   ev, _ecore_imf_event_free_commit, NULL);
}

/**
 * @internal
 * @brief Frees memory allocated for a delete surrounding event.
 * This function is a simple wrapper around free() and is used as a callback
 * for ecore_event_add when dispatching delete surrounding events.
 *
 * @param data User data associated with the event (unused).
 * @param event Pointer to the Ecore_IMF_Event_Delete_Surrounding structure to be freed.
 */
static void
_ecore_imf_event_free_delete_surrounding(void *data EINA_UNUSED, void *event)
{
   free(event);
}

/**
 * @brief Adds a delete surrounding text event to the Ecore event queue.
 *
 * This function is called by an IMF module to request the application
 * to delete a range of text surrounding the current cursor position.
 * An ECORE_IMF_EVENT_DELETE_SURROUNDING event will be emitted.
 *
 * @param ctx The input method context requesting the deletion.
 * @param offset The byte offset from the current cursor position where deletion
 *               should start. A negative offset means characters before the cursor,
 *               a positive offset means characters after the cursor.
 *               Example: -1 means delete the character before the cursor.
 * @param n_chars The number of characters (bytes) to delete.
 */
EAPI void
ecore_imf_context_delete_surrounding_event_add(Ecore_IMF_Context *ctx, int offset, int n_chars)
{
   Ecore_IMF_Event_Delete_Surrounding *ev;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_delete_surrounding_event_add");
        return;
     }

   ev = malloc(sizeof(Ecore_IMF_Event_Delete_Surrounding));
   ev->ctx = ctx;
   ev->offset = offset;
   ev->n_chars = n_chars;
   ecore_event_add(ECORE_IMF_EVENT_DELETE_SURROUNDING,
                   ev, _ecore_imf_event_free_delete_surrounding, NULL);
}

/**
 * @brief Adds an event callback for a specific IMF event type.
 *
 * Registers a callback function to be invoked when a specific type of
 * IMF event occurs for this context. These are direct callbacks, distinct
 * from the Ecore event system, and are called via
 * ecore_imf_context_event_callback_call().
 *
 * The `Ecore_IMF_Func_Node` structure stores the callback, data, and type.
 * These nodes are stored in the `ctx->callbacks` list.
 *
 * @param ctx The input method context.
 * @param type The type of IMF callback to register for (e.g.,
 *             ECORE_IMF_CALLBACK_PREEDIT_START, ECORE_IMF_CALLBACK_COMMIT).
 *             From the #Ecore_IMF_Callback_Type enumeration.
 * @param func The callback function to execute.
 *             Its signature is `void (*Ecore_IMF_Event_Cb)(void *data, Ecore_IMF_Context *ctx, void *event_info)`.
 *             The `event_info` type depends on the `type`.
 * @param data User data to be passed to the callback function.
 */
EAPI void
ecore_imf_context_event_callback_add(Ecore_IMF_Context *ctx, Ecore_IMF_Callback_Type type, Ecore_IMF_Event_Cb func, const void *data)
{
   Ecore_IMF_Func_Node *fn = NULL;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_event_callback_add");
        return;
     }

   if (!func) return;

   fn = calloc(1, sizeof (Ecore_IMF_Func_Node));
   if (!fn) return;

   fn->func = (void *)func;
   fn->data = data;
   fn->type = type;

   ctx->callbacks = eina_list_append(ctx->callbacks, fn);
}

/**
 * @brief Deletes a previously added event callback.
 *
 * Unregisters a callback function that was previously added with
 * ecore_imf_context_event_callback_add(). The callback is identified
 * by its type and function pointer.
 *
 * @param ctx The input method context.
 * @param type The type of IMF callback that was registered.
 * @param func The callback function pointer that was registered.
 * @return The user data associated with the callback if found and removed,
 *         otherwise @c NULL.
 */
EAPI void *
ecore_imf_context_event_callback_del(Ecore_IMF_Context *ctx, Ecore_IMF_Callback_Type type, Ecore_IMF_Event_Cb func)
{
   Eina_List *l = NULL;
   Eina_List *l_next = NULL;
   Ecore_IMF_Func_Node *fn = NULL;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_event_callback_del");
        return NULL;
     }

   if (!func) return NULL;
   if (!ctx->callbacks) return NULL;

   EINA_LIST_FOREACH_SAFE(ctx->callbacks, l, l_next, fn)
     {
        if ((fn) && (fn->func == (void *)func) && (fn->type == type))
          {
             void *tmp = (void *)fn->data;
             free(fn);
             ctx->callbacks = eina_list_remove_list(ctx->callbacks, l);
             return tmp;
          }
     }
   return NULL;
}

/**
 * @brief Calls registered event callbacks of a specific type.
 * @internal
 *
 * This function is typically called by IMF modules to invoke application-registered
 * callbacks for a specific IMF event type. It iterates through the list of
 * registered callbacks for the given context and calls those matching the type.
 *
 * @param ctx The input method context.
 * @param type The type of IMF callback to call.
 *             From the #Ecore_IMF_Callback_Type enumeration.
 * @param event_info Event-specific information to pass to the callback function.
 *                   The actual type of `event_info` depends on the `type`.
 *                   For example, for ECORE_IMF_CALLBACK_COMMIT, it would be `const char *`.
 */
EAPI void
ecore_imf_context_event_callback_call(Ecore_IMF_Context *ctx, Ecore_IMF_Callback_Type type, void *event_info)
{
   Ecore_IMF_Func_Node *fn = NULL;
   Eina_List *l = NULL;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_event_callback_call");
        return;
     }

   EINA_LIST_FOREACH(ctx->callbacks, l, fn)
     {
        if ((fn) && (fn->type == type) && (fn->func))
         {
           Ecore_IMF_Event_Cb cb = (Ecore_IMF_Event_Cb)fn->func;

           cb((void *)fn->data, ctx, event_info);
         }
     }
}

/**
 * @brief Requests the input method context to show its control panel.
 *
 * Some input methods provide a separate control panel for configuration
 * (e.g., language selection, input mode switching). This function requests
 * the IM to display this panel. The availability and behavior depend on the
 * specific IMF module.
 *
 * @param ctx The input method context.
 */
EAPI void
ecore_imf_context_control_panel_show(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_control_panel_show");
        return;
     }

   if (ctx->klass && ctx->klass->control_panel_show) ctx->klass->control_panel_show(ctx);
}

/**
 * @brief Requests the input method context to hide its control panel.
 *
 * If the input method's control panel is visible, this function requests
 * it to be hidden.
 *
 * @param ctx The input method context.
 */
EAPI void
ecore_imf_context_control_panel_hide(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_control_panel_hide");
        return;
     }

   if (ctx->klass && ctx->klass->control_panel_hide) ctx->klass->control_panel_hide(ctx);
}

/**
 * @brief Sets input hints for the context.
 *
 * Input hints provide the input method with information about the expected
 * type of input or the nature of the text field. This can influence
 * keyboard layout, autocapitalization, or other behaviors. Hints are bitmasks
 * from the #Ecore_IMF_Input_Hints enumeration.
 * By default, ECORE_IMF_INPUT_HINT_AUTO_COMPLETE is set.
 *
 * @param ctx The input method context.
 * @param input_hints A bitmask of #Ecore_IMF_Input_Hints.
 *                    Example: ECORE_IMF_INPUT_HINT_SENSITIVE_DATA | ECORE_IMF_INPUT_HINT_NO_SPELLCHECK.
 */
EAPI void
ecore_imf_context_input_hint_set(Ecore_IMF_Context *ctx, Ecore_IMF_Input_Hints input_hints)
{
    if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
      {
         ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                          "ecore_imf_context_input_hint_set");
         return;
      }

   if (ctx->input_hints != input_hints)
     {
        if (ctx->klass && ctx->klass->input_hint_set)
          ctx->klass->input_hint_set(ctx, input_hints);

        ctx->input_hints = input_hints;
     }
}

/**
 * @brief Gets the current input hints for the context.
 *
 * Retrieves the input hints bitmask previously set by
 * ecore_imf_context_input_hint_set().
 *
 * @param ctx The input method context.
 * @return A bitmask of #Ecore_IMF_Input_Hints.
 *         Returns ECORE_IMF_INPUT_HINT_NONE if @p ctx is invalid.
 */
EAPI Ecore_IMF_Input_Hints
ecore_imf_context_input_hint_get(Ecore_IMF_Context *ctx)
{
    if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
      {
         ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                          "ecore_imf_context_input_hint_get");
         return ECORE_IMF_INPUT_HINT_NONE;
      }

    return ctx->input_hints;
}

/**
 * @brief Requests the input method context to show its input panel (e.g., virtual keyboard).
 *
 * This function signals the input method context to display its main input
 * panel, which is often an on-screen keyboard. The visibility is also
 * contingent on `ctx->input_panel_enabled` being true or the
 * `ECORE_IMF_INPUT_PANEL_ENABLED` environment variable being set.
 * It also updates `show_req_ctx` to this context.
 *
 * @param ctx The input method context.
 */
EAPI void
ecore_imf_context_input_panel_show(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_show");
        return;
     }

   show_req_ctx = ctx;
   if ((ctx->input_panel_enabled) ||
       (getenv("ECORE_IMF_INPUT_PANEL_ENABLED")))
     {
        if (ctx->klass && ctx->klass->show) ctx->klass->show(ctx);
     }
}

/**
 * @brief Requests the input method context to hide its input panel.
 *
 * This function signals the input method context to hide its main input panel.
 * The action is contingent on `ctx->input_panel_enabled` being true or the
 * `ECORE_IMF_INPUT_PANEL_ENABLED` environment variable being set.
 *
 * @param ctx The input method context.
 */
EAPI void
ecore_imf_context_input_panel_hide(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_hide");
        return;
     }

   if ((ctx->input_panel_enabled) ||
       (getenv("ECORE_IMF_INPUT_PANEL_ENABLED")))
     {
        if (ctx->klass && ctx->klass->hide) ctx->klass->hide(ctx);
     }
}

/**
 * @brief Sets the layout for the input panel (e.g., virtual keyboard).
 *
 * This function specifies the desired layout for the input panel, such as
 * normal, number, email, password, etc. The IMF module can use this to
 * display an appropriate keyboard or input interface.
 * By default, ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL is set.
 * If the layout is set to ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD,
 * autocapitalization is automatically set to ECORE_IMF_AUTOCAPITAL_TYPE_NONE.
 *
 * @param ctx The input method context.
 * @param layout The desired layout from the #Ecore_IMF_Input_Panel_Layout enumeration.
 *               Example: ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBER.
 */
EAPI void
ecore_imf_context_input_panel_layout_set(Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Layout layout)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_layout_set");
        return;
     }

   if (ctx->input_panel_layout != layout)
     {
        if (ctx->klass && ctx->klass->input_panel_layout_set)
          ctx->klass->input_panel_layout_set(ctx, layout);

        ctx->input_panel_layout = layout;

        if (layout == ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD)
          ecore_imf_context_autocapital_type_set(ctx, ECORE_IMF_AUTOCAPITAL_TYPE_NONE);
     }
}

/**
 * @brief Gets the current layout of the input panel.
 *
 * Retrieves the input panel layout previously set by
 * ecore_imf_context_input_panel_layout_set().
 *
 * @param ctx The input method context.
 * @return The current #Ecore_IMF_Input_Panel_Layout.
 *         Returns ECORE_IMF_INPUT_PANEL_LAYOUT_INVALID if @p ctx is invalid or
 *         the module doesn't support getting the layout.
 */
EAPI Ecore_IMF_Input_Panel_Layout
ecore_imf_context_input_panel_layout_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_layout_get");
        return ECORE_IMF_INPUT_PANEL_LAYOUT_INVALID;
     }

   if (ctx->klass && ctx->klass->input_panel_layout_get)
     return ctx->input_panel_layout;
   else
     return ECORE_IMF_INPUT_PANEL_LAYOUT_INVALID;
}

/**
 * @brief Sets a variation for the current input panel layout.
 *
 * Some input panel layouts might have variations (e.g., a normal layout
 * with a dedicated number row, or a compact version). This function allows
 * specifying such a variation. The meaning of the integer @p variation
 * is specific to the IMF module and the current layout.
 *
 * @param ctx The input method context.
 * @param variation An integer specifying the layout variation.
 */
EAPI void
ecore_imf_context_input_panel_layout_variation_set(Ecore_IMF_Context *ctx, int variation)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_layout_variation_set");
        return;
     }

   ctx->input_panel_layout_variation = variation;
}

/**
 * @brief Gets the current variation of the input panel layout.
 *
 * Retrieves the layout variation previously set by
 * ecore_imf_context_input_panel_layout_variation_set().
 *
 * @param ctx The input method context.
 * @return The integer value of the layout variation.
 *         Returns 0 if @p ctx is invalid or no variation is set.
 */
EAPI int
ecore_imf_context_input_panel_layout_variation_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_layout_variation_get");
        return 0;
     }

   return ctx->input_panel_layout_variation;
}

/**
 * @brief Sets the language for the input panel.
 *
 * This function requests a specific language for the input panel (e.g.,
 * for an on-screen keyboard). The IMF module may switch its dictionary,
 * key layout, or other language-specific features.
 *
 * @param ctx The input method context.
 * @param lang The desired language from the #Ecore_IMF_Input_Panel_Lang enumeration.
 *             Example: ECORE_IMF_INPUT_PANEL_LANG_FR (French).
 *             ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC lets the IM decide.
 */
EAPI void
ecore_imf_context_input_panel_language_set(Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Lang lang)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_language_set");
        return;
     }

   if (ctx->input_panel_lang != lang)
     {
        if (ctx->klass && ctx->klass->input_panel_language_set)
          ctx->klass->input_panel_language_set(ctx, lang);

        ctx->input_panel_lang = lang;
     }
}

/**
 * @brief Gets the current language of the input panel.
 *
 * Retrieves the input panel language previously set by
 * ecore_imf_context_input_panel_language_set().
 *
 * @param ctx The input method context.
 * @return The current #Ecore_IMF_Input_Panel_Lang.
 *         Returns ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC if @p ctx is invalid
 *         or if no specific language has been set.
 */
EAPI Ecore_IMF_Input_Panel_Lang
ecore_imf_context_input_panel_language_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_language_get");
        return ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC;
     }

   return ctx->input_panel_lang;
}

/**
 * @brief Sets whether the input panel (e.g., virtual keyboard) is enabled for this context.
 *
 * This function controls whether the input panel associated with the context
 * should be allowed to show. If disabled, calls to
 * ecore_imf_context_input_panel_show() might be ignored by some modules,
 * though the `ECORE_IMF_INPUT_PANEL_ENABLED` environment variable can override this.
 * By default, the input panel is enabled.
 *
 * @param ctx The input method context.
 * @param enabled @c EINA_TRUE to enable the input panel, @c EINA_FALSE to disable.
 */
EAPI void
ecore_imf_context_input_panel_enabled_set(Ecore_IMF_Context *ctx,
                                           Eina_Bool enabled)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_enabled_set");
        return;
     }

   ctx->input_panel_enabled = enabled;
}

/**
 * @brief Gets whether the input panel is enabled for this context.
 *
 * Retrieves the enabled state of the input panel, as set by
 * ecore_imf_context_input_panel_enabled_set().
 *
 * @param ctx The input method context.
 * @return @c EINA_TRUE if the input panel is enabled, @c EINA_FALSE otherwise.
 *         Returns @c EINA_FALSE if @p ctx is invalid.
 */
EAPI Eina_Bool
ecore_imf_context_input_panel_enabled_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_enabled_get");
        return EINA_FALSE;
     }

   return ctx->input_panel_enabled;
}

/**
 * @brief Sets IMF-specific data for the input panel.
 *
 * This function allows passing a block of arbitrary data to the input method
 * module, specifically for input panel purposes. The format and meaning of
 * this data are defined by the specific IMF module being used.
 *
 * @param ctx The input method context.
 * @param data A pointer to the data block.
 * @param len The length of the data block in bytes.
 */
EAPI void
ecore_imf_context_input_panel_imdata_set(Ecore_IMF_Context *ctx, const void *data, int len)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_imdata_set");
        return;
     }

   if (!data) return;

   if (ctx->klass && ctx->klass->input_panel_imdata_set)
     ctx->klass->input_panel_imdata_set(ctx, data, len);
}

/**
 * @brief Gets IMF-specific data from the input panel.
 *
 * This function allows retrieving a block of arbitrary data from the input
 * method module, related to the input panel. The format and meaning of this
 * data are defined by the specific IMF module. The caller provides a buffer
 * (@p data) and a pointer to an integer (@p len) which should initially
 * contain the size of the buffer. Upon return, @p len will contain the
 * actual number of bytes written to @p data.
 *
 * @param ctx The input method context.
 * @param[out] data A buffer to store the retrieved data.
 * @param[in,out] len On input, a pointer to the size of the @p data buffer.
 *                    On output, a pointer to the actual number of bytes written.
 */
EAPI void
ecore_imf_context_input_panel_imdata_get(Ecore_IMF_Context *ctx, void *data, int *len)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_imdata_get");
        return;
     }

   if (!data) return;

   if (ctx->klass && ctx->klass->input_panel_imdata_get)
     ctx->klass->input_panel_imdata_get(ctx, data, len);
}

/**
 * @brief Sets the type of the return key on the input panel (e.g., virtual keyboard).
 *
 * This function suggests to the input method what kind of action the "Return"
 * or "Enter" key on the virtual keyboard should represent (e.g., "Go", "Search",
 * "Next", "Send"). The IMF module may change the key's label or behavior accordingly.
 *
 * @param ctx The input method context.
 * @param return_key_type The desired return key type from the
 *                        #Ecore_IMF_Input_Panel_Return_Key_Type enumeration.
 *                        Example: ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH.
 */
EAPI void
ecore_imf_context_input_panel_return_key_type_set(Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Return_Key_Type return_key_type)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_return_key_type_set");
        return;
     }

   if (ctx->input_panel_return_key_type != return_key_type)
     {
        ctx->input_panel_return_key_type = return_key_type;
        if (ctx->klass && ctx->klass->input_panel_return_key_type_set)
          ctx->klass->input_panel_return_key_type_set(ctx, return_key_type);
     }
}

/**
 * @brief Gets the current type of the return key on the input panel.
 *
 * Retrieves the return key type previously set by
 * ecore_imf_context_input_panel_return_key_type_set().
 *
 * @param ctx The input method context.
 * @return The current #Ecore_IMF_Input_Panel_Return_Key_Type.
 *         Returns ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT if @p ctx is invalid
 *         or no specific type has been set.
 */
EAPI Ecore_IMF_Input_Panel_Return_Key_Type
ecore_imf_context_input_panel_return_key_type_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_return_key_type_get");
        return ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT;
     }

   return ctx->input_panel_return_key_type;
}

/**
 * @brief Sets whether the return key on the input panel should be disabled.
 *
 * This function allows the application to explicitly disable the return key
 * on the virtual keyboard, for example, when input is incomplete or invalid.
 *
 * @param ctx The input method context.
 * @param disabled @c EINA_TRUE to disable the return key, @c EINA_FALSE to enable it.
 */
EAPI void
ecore_imf_context_input_panel_return_key_disabled_set(Ecore_IMF_Context *ctx, Eina_Bool disabled)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_return_key_disabled_set");
        return;
     }

   if (ctx->input_panel_return_key_disabled != disabled)
     {
        ctx->input_panel_return_key_disabled = disabled;
        if (ctx->klass && ctx->klass->input_panel_return_key_disabled_set)
          ctx->klass->input_panel_return_key_disabled_set(ctx, disabled);
     }
}

/**
 * @brief Gets whether the return key on the input panel is disabled.
 *
 * Retrieves the disabled state of the return key, as set by
 * ecore_imf_context_input_panel_return_key_disabled_set().
 *
 * @param ctx The input method context.
 * @return @c EINA_TRUE if the return key is disabled, @c EINA_FALSE otherwise.
 *         Returns @c EINA_FALSE if @p ctx is invalid.
 */
EAPI Eina_Bool
ecore_imf_context_input_panel_return_key_disabled_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_return_key_disabled_get");
        return EINA_FALSE;
     }

   return ctx->input_panel_return_key_disabled;
}

/**
 * @brief Sets the Caps Lock mode for the input panel.
 *
 * This function informs the input method context about the current state
 * of Caps Lock. This is particularly relevant for virtual keyboards that
 * might need to adjust their display or behavior.
 *
 * @param ctx The input method context.
 * @param mode @c EINA_TRUE if Caps Lock is active, @c EINA_FALSE otherwise.
 */
EAPI void
ecore_imf_context_input_panel_caps_lock_mode_set(Ecore_IMF_Context *ctx, Eina_Bool mode)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_caps_lock_mode_set");
        return;
     }

   if (ctx->input_panel_caps_lock_mode != mode)
     {
        if (ctx->klass && ctx->klass->input_panel_caps_lock_mode_set)
          ctx->klass->input_panel_caps_lock_mode_set(ctx, mode);

        ctx->input_panel_caps_lock_mode = mode;
     }
}

/**
 * @brief Gets the current Caps Lock mode of the input panel.
 *
 * Retrieves the Caps Lock mode previously set by
 * ecore_imf_context_input_panel_caps_lock_mode_set().
 *
 * @param ctx The input method context.
 * @return @c EINA_TRUE if Caps Lock mode is set (active), @c EINA_FALSE otherwise.
 *         Returns @c EINA_FALSE if @p ctx is invalid.
 */
EAPI Eina_Bool
ecore_imf_context_input_panel_caps_lock_mode_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_caps_lock_mode_get");
        return EINA_FALSE;
     }

   return ctx->input_panel_caps_lock_mode;
}

/**
 * @brief Gets the geometry (position and size) of the input panel.
 *
 * If the input panel (e.g., virtual keyboard) is visible, this function
 * retrieves its screen coordinates and dimensions. This can be used by
 * applications to adjust their layout to avoid overlapping with the panel.
 * The coordinates are typically relative to the screen or top-level window.
 *
 * @param ctx The input method context.
 * @param[out] x Pointer to store the x-coordinate of the input panel.
 * @param[out] y Pointer to store the y-coordinate of the input panel.
 * @param[out] w Pointer to store the width of the input panel.
 * @param[out] h Pointer to store the height of the input panel.
 *             If the panel is not visible or geometry is unavailable, these
 *             values might not be set or could be set to 0.
 */
EAPI void
ecore_imf_context_input_panel_geometry_get(Ecore_IMF_Context *ctx, int *x, int *y, int *w, int *h)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_geometry_get");
        return;
     }

   if (ctx->klass && ctx->klass->input_panel_geometry_get)
     ctx->klass->input_panel_geometry_get(ctx, x, y, w, h);
}

/**
 * @brief Gets the current state of the input panel.
 *
 * Retrieves the current visibility state of the input panel (e.g., shown,
 * hidden, about to show).
 *
 * @param ctx The input method context.
 * @return The current state from the #Ecore_IMF_Input_Panel_State enumeration.
 *         Example: ECORE_IMF_INPUT_PANEL_STATE_SHOW, ECORE_IMF_INPUT_PANEL_STATE_HIDE.
 *         Returns ECORE_IMF_INPUT_PANEL_STATE_HIDE if @p ctx is invalid or
 *         the state cannot be determined.
 */
EAPI Ecore_IMF_Input_Panel_State
ecore_imf_context_input_panel_state_get(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Input_Panel_State state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_state_get");
        return ECORE_IMF_INPUT_PANEL_STATE_HIDE;
     }

   if (ctx->klass && ctx->klass->input_panel_state_get)
     state = ctx->klass->input_panel_state_get(ctx);

   return state;
}

/**
 * @internal
 * @brief Defines the signature for input panel event callbacks.
 *
 * @param data User-provided data.
 * @param ctx The Ecore_IMF_Context that triggered the event.
 * @param value An integer value associated with the event, its meaning depends on the event type.
 *              For ECORE_IMF_INPUT_PANEL_STATE_EVENT, it's an Ecore_IMF_Input_Panel_State.
 *              For ECORE_IMF_INPUT_PANEL_LANGUAGE_EVENT, it's an Ecore_IMF_Input_Panel_Lang.
 *              For ECORE_IMF_INPUT_PANEL_SHIFT_MODE_EVENT, it's an int representing shift mode.
 *              For ECORE_IMF_INPUT_PANEL_LAYOUT_EVENT, it's an Ecore_IMF_Input_Panel_Layout.
 */
typedef void (*Ecore_IMF_Input_Panel_Callback) (void *data, Ecore_IMF_Context *ctx, int value);

/**
 * @brief Adds a callback for input panel specific events.
 *
 * Registers a function to be called when certain events related to the
 * input panel occur, such as state changes (show/hide), language changes,
 * shift mode changes, or layout changes.
 *
 * The `Ecore_IMF_Input_Panel_Callback_Node` structure stores the callback, data, and type.
 * These nodes are stored in the `ctx->input_panel_callbacks` list.
 *
 * @param ctx The input method context.
 * @param type The type of input panel event to listen for, from the
 *             #Ecore_IMF_Input_Panel_Event enumeration.
 *             Example: ECORE_IMF_INPUT_PANEL_STATE_EVENT.
 * @param func The callback function to execute.
 * @param data User data to be passed to the callback function.
 */
EAPI void
ecore_imf_context_input_panel_event_callback_add(Ecore_IMF_Context *ctx,
                                                 Ecore_IMF_Input_Panel_Event type,
                                                 void (*func) (void *data, Ecore_IMF_Context *ctx, int value),
                                                 const void *data)
{
   Ecore_IMF_Input_Panel_Callback_Node *fn = NULL;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_event_callback_add");
        return;
     }

   if (!func) return;

   fn = calloc(1, sizeof (Ecore_IMF_Input_Panel_Callback_Node));
   if (!fn) return;

   fn->func = (void *)func;
   fn->data = data;
   fn->type = type;

   ctx->input_panel_callbacks = eina_list_append(ctx->input_panel_callbacks, fn);
}

/**
 * @brief Deletes a previously added input panel event callback.
 *
 * Unregisters an input panel event callback that was added with
 * ecore_imf_context_input_panel_event_callback_add(). The callback is
 * identified by its type and function pointer.
 *
 * @param ctx The input method context.
 * @param type The type of input panel event that was registered.
 * @param func The callback function pointer that was registered.
 */
EAPI void
ecore_imf_context_input_panel_event_callback_del(Ecore_IMF_Context *ctx,
                                                 Ecore_IMF_Input_Panel_Event type,
                                                 void (*func) (void *data, Ecore_IMF_Context *ctx, int value))
{
   Eina_List *l = NULL;
   Eina_List *l_next = NULL;
   Ecore_IMF_Input_Panel_Callback_Node *fn = NULL;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_event_callback_del");
        return;
     }

   if (!func) return;
   if (!ctx->input_panel_callbacks) return;

   EINA_LIST_FOREACH_SAFE(ctx->input_panel_callbacks, l, l_next, fn)
     {
        if ((fn) && (fn->func == (void *)func) && (fn->type == type))
          {
             free(fn);
             ctx->input_panel_callbacks = eina_list_remove_list(ctx->input_panel_callbacks, l);
             return;
          }
     }
}

/**
 * @brief Calls registered input panel event callbacks of a specific type.
 * @internal
 *
 * This function is typically called by IMF modules to invoke application-registered
 * callbacks for a specific input panel event type. It iterates through the list of
 * registered callbacks for the given context and calls those matching the type.
 * If the event is ECORE_IMF_INPUT_PANEL_STATE_EVENT and the new state is
 * ECORE_IMF_INPUT_PANEL_STATE_HIDE, and this context was the one that
 * last requested to be shown (`show_req_ctx`), then `show_req_ctx` is cleared.
 *
 * @param ctx The input method context.
 * @param type The type of input panel event to call callbacks for.
 *             From the #Ecore_IMF_Input_Panel_Event enumeration.
 * @param value An integer value associated with the event. Its meaning depends
 *              on the @p type. For example, if @p type is
 *              ECORE_IMF_INPUT_PANEL_STATE_EVENT, @p value would be an
 *              #Ecore_IMF_Input_Panel_State.
 */
EAPI void
ecore_imf_context_input_panel_event_callback_call(Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Event type, int value)
{
   Ecore_IMF_Input_Panel_Callback_Node *fn = NULL;
   Eina_List *l = NULL;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_event_callback_call");
        return;
     }

   EINA_LIST_FOREACH(ctx->input_panel_callbacks, l, fn)
     {
        if ((fn) && (fn->type == type) && (fn->func))
          {
             Ecore_IMF_Input_Panel_Callback cb = (Ecore_IMF_Input_Panel_Callback)fn->func;
             cb((void *)fn->data, ctx, value);
             if (type == ECORE_IMF_INPUT_PANEL_STATE_EVENT &&
                 value == ECORE_IMF_INPUT_PANEL_STATE_HIDE &&
                 show_req_ctx == ctx)
               show_req_ctx = NULL;

             if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
               break;
          }
     }
}

/**
 * @brief Clears all registered input panel event callbacks for a context.
 *
 * This function removes and frees all input panel event callbacks that were
 * previously added to the specified context using
 * ecore_imf_context_input_panel_event_callback_add().
 *
 * @param ctx The input method context whose input panel callbacks are to be cleared.
 */
EAPI void
ecore_imf_context_input_panel_event_callback_clear(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Input_Panel_Callback_Node *fn = NULL;
   Eina_List *l = NULL;

   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_event_callback_clear");
        return;
     }

   for (l = ctx->input_panel_callbacks; l;)
     {
        fn = (Ecore_IMF_Input_Panel_Callback_Node *)l->data;

        if (fn)
          {
             ctx->input_panel_callbacks = eina_list_remove(ctx->input_panel_callbacks, fn);
             free (fn);
          }
        l = l->next;
     }
}

/**
 * @brief Gets the locale string for the current input panel language.
 *
 * Retrieves a string representing the locale of the current input panel
 * language (e.g., "en_US", "fr_FR"). The caller is responsible for freeing
 * the returned string @p lang using free().
 *
 * @param ctx The input method context.
 * @param[out] lang A pointer to a character pointer that will be updated to
 *                  point to a newly allocated string containing the locale.
 *                  If no language is set or an error occurs, it's set to
 *                  an empty, allocated string.
 */
EAPI void
ecore_imf_context_input_panel_language_locale_get(Ecore_IMF_Context *ctx, char **lang)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_language_locale_get");
        return;
     }

   if (ctx->klass && ctx->klass->input_panel_language_locale_get)
     ctx->klass->input_panel_language_locale_get(ctx, lang);
   else
     {
        if (lang) *lang = strdup("");
     }
}

/**
 * @brief Gets the geometry (position and size) of the candidate panel.
 *
 * The candidate panel (or window) is used by some input methods to display
 * a list of completion or conversion candidates. This function retrieves its
 * screen coordinates and dimensions if it's visible.
 * The coordinates are typically relative to the screen or top-level window.
 *
 * @param ctx The input method context.
 * @param[out] x Pointer to store the x-coordinate of the candidate panel.
 * @param[out] y Pointer to store the y-coordinate of the candidate panel.
 * @param[out] w Pointer to store the width of the candidate panel.
 * @param[out] h Pointer to store the height of the candidate panel.
 *             If the panel is not visible or geometry is unavailable, these
 *             values might not be set or could be set to 0.
 */
EAPI void
ecore_imf_context_candidate_panel_geometry_get(Ecore_IMF_Context *ctx, int *x, int *y, int *w, int *h)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_candidate_panel_geometry_get");
        return;
     }

   if (ctx->klass && ctx->klass->candidate_panel_geometry_get)
     ctx->klass->candidate_panel_geometry_get(ctx, x, y, w, h);
}

/**
 * @brief Sets whether the input panel should be shown on demand.
 *
 * This function controls a hint for the input panel's behavior. If set to
 * @c EINA_TRUE, it suggests that the input panel (e.g., virtual keyboard)
 * should only appear when explicitly requested by the user (e.g., by tapping
 * a text field that requires it), rather than appearing automatically on focus.
 * The actual behavior depends on the IMF module.
 *
 * @param ctx The input method context.
 * @param ondemand @c EINA_TRUE to suggest on-demand showing, @c EINA_FALSE otherwise.
 */
EAPI void
ecore_imf_context_input_panel_show_on_demand_set(Ecore_IMF_Context *ctx, Eina_Bool ondemand)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_show_on_demand_set");
        return;
     }

   ctx->input_panel_show_on_demand = ondemand;
}

/**
 * @brief Gets whether the input panel is set to show on demand.
 *
 * Retrieves the on-demand showing preference previously set by
 * ecore_imf_context_input_panel_show_on_demand_set().
 *
 * @param ctx The input method context.
 * @return @c EINA_TRUE if on-demand showing is preferred, @c EINA_FALSE otherwise.
 *         Returns @c EINA_FALSE if @p ctx is invalid.
 */
EAPI Eina_Bool
ecore_imf_context_input_panel_show_on_demand_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_show_on_demand_get");
        return EINA_FALSE;
     }

   return ctx->input_panel_show_on_demand;
}

/**
 * @brief Sets the bidirectional (BiDi) text direction for the context.
 *
 * This function informs the input method context about the primary text
 * direction (e.g., left-to-right, right-to-left, or neutral) of the input field.
 * This can be important for IMs handling languages with different writing directions.
 * By default, ECORE_IMF_BIDI_DIRECTION_NEUTRAL is set.
 *
 * @param ctx The input method context.
 * @param direction The desired BiDi direction from the #Ecore_IMF_BiDi_Direction enumeration.
 *                  Example: ECORE_IMF_BIDI_DIRECTION_RTL.
 */
EAPI void
ecore_imf_context_bidi_direction_set(Ecore_IMF_Context *ctx, Ecore_IMF_BiDi_Direction direction)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_bidi_direction_set");
        return;
     }

   if (ctx->bidi_direction != direction)
     {
        if (ctx->klass && ctx->klass->bidi_direction_set)
          ctx->klass->bidi_direction_set(ctx, direction);

        ctx->bidi_direction = direction;
     }
}

/**
 * @brief Gets the current bidirectional (BiDi) text direction of the context.
 *
 * Retrieves the BiDi direction previously set by
 * ecore_imf_context_bidi_direction_set().
 *
 * @param ctx The input method context.
 * @return The current #Ecore_IMF_BiDi_Direction.
 *         Returns ECORE_IMF_BIDI_DIRECTION_NEUTRAL if @p ctx is invalid or
 *         no direction has been explicitly set.
 */
EAPI Ecore_IMF_BiDi_Direction
ecore_imf_context_bidi_direction_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_bidi_direction_get");
        return ECORE_IMF_BIDI_DIRECTION_NEUTRAL;
     }

   return ctx->bidi_direction;
}

/**
 * @brief Gets the current keyboard mode of the input panel.
 *
 * Retrieves the keyboard mode (e.g., software keyboard, hardware keyboard)
 * currently active or preferred by the input panel.
 *
 * @param ctx The input method context.
 * @return The current keyboard mode from the #Ecore_IMF_Input_Panel_Keyboard_Mode enumeration.
 *         Example: ECORE_IMF_INPUT_PANEL_SW_KEYBOARD_MODE (software keyboard),
 *         ECORE_IMF_INPUT_PANEL_HW_KEYBOARD_MODE (hardware keyboard).
 *         Returns ECORE_IMF_INPUT_PANEL_SW_KEYBOARD_MODE if @p ctx is invalid or
 *         the mode cannot be determined by the module.
 */
EAPI Ecore_IMF_Input_Panel_Keyboard_Mode
ecore_imf_context_keyboard_mode_get(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Input_Panel_Keyboard_Mode mode = ECORE_IMF_INPUT_PANEL_SW_KEYBOARD_MODE;
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_keyboard_mode_get");
        return ECORE_IMF_INPUT_PANEL_SW_KEYBOARD_MODE;
     }

   if (ctx->klass && ctx->klass->keyboard_mode_get)
     mode = ctx->klass->keyboard_mode_get(ctx);

   return mode;
}

/**
 * @brief Sets a string-based prediction hint for the context.
 *
 * This function provides a general-purpose string hint to the input method
 * regarding text prediction. The interpretation of this string is entirely
 * up to the IMF module. It could be a comma-separated list of expected
 * words, a category, or any other module-specific directive.
 * For more structured prediction hints, see ecore_imf_context_prediction_hint_hash_set().
 *
 * @param ctx The input method context.
 * @param prediction_hint A string containing the prediction hint.
 */
EAPI void
ecore_imf_context_prediction_hint_set(Ecore_IMF_Context *ctx, const char *prediction_hint)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_prediction_hint_set");
        return;
     }

   if (ctx->klass && ctx->klass->prediction_hint_set)
     ctx->klass->prediction_hint_set(ctx, prediction_hint);
}

/**
 * @brief Sets the acceptable MIME type(s) for content insertion.
 *
 * This function informs the input method context about the MIME types of
 * content that the associated text widget can accept (e.g., "text/plain",
 * "image/png"). This is particularly relevant for IMs that support inserting
 * rich content like images or stickers. The @p mime_type string can be a
 * single MIME type or a list (e.g., comma-separated, depending on module).
 *
 * @param ctx The input method context.
 * @param mime_type A string specifying the acceptable MIME type(s). Must not be @c NULL.
 */
EAPI void
ecore_imf_context_mime_type_accept_set(Ecore_IMF_Context *ctx, const char *mime_type)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_mime_type_accept_set");
        return;
     }

   if (!mime_type) return;

   if (ctx->klass && ctx->klass->mime_type_accept_set)
     ctx->klass->mime_type_accept_set(ctx, mime_type);
}

/**
 * @brief Sets a preferred position for the input panel.
 *
 * This function suggests a desired (x, y) screen coordinate for the top-left
 * corner of the input panel. The IMF module may or may not honor this request,
 * as panel positioning can be complex and subject to various constraints.
 * Negative coordinates are ignored.
 *
 * @param ctx The input method context.
 * @param x The suggested x-coordinate for the input panel.
 * @param y The suggested y-coordinate for the input panel.
 */
EAPI void
ecore_imf_context_input_panel_position_set(Ecore_IMF_Context *ctx, int x, int y)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_input_panel_position_set");
        return;
     }

   if (x < 0 || y < 0) return;

   if (ctx->klass && ctx->klass->input_panel_position_set)
     ctx->klass->input_panel_position_set(ctx, x, y);
}

/**
 * @internal
 * @brief Frees data stored in the prediction hint hash.
 * This callback is used by the Eina_Hash when an item (a string value)
 * is removed or the hash table is freed.
 *
 * @param data Pointer to the string data (value) to be freed.
 */
static void
_prediction_hint_hash_free_cb(void *data)
{
   free(data);
}

/**
 * @brief Sets a key-value pair in the prediction hints hash table.
 *
 * This function allows setting detailed, structured prediction hints as
 * key-value pairs. The input method module can then query this hash table
 * for specific hints. For example, `key="domain", value="email"` could hint
 * that an email address is expected.
 *
 * If the hash table `ctx->prediction_hint_hash` doesn't exist, it's created.
 * Both the key and value strings are duplicated by the hash table or this function.
 * If @p value is @c NULL, an empty string is stored.
 *
 * @param ctx The input method context.
 * @param key The key for the prediction hint (e.g., "expected_format").
 * @param value The value for the prediction hint (e.g., "date").
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., memory allocation error, invalid context).
 */
EAPI Eina_Bool
ecore_imf_context_prediction_hint_hash_set(Ecore_IMF_Context *ctx, const char *key, const char *value)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_prediction_hint_hash_set");
        return EINA_FALSE;
     }

   if (!ctx->prediction_hint_hash)
     ctx->prediction_hint_hash = eina_hash_string_superfast_new(_prediction_hint_hash_free_cb);

   if (!ctx->prediction_hint_hash)
     return EINA_FALSE;

   char *old_value = eina_hash_set(ctx->prediction_hint_hash, key, value ? strdup(value) : strdup(""));
   if (old_value)
     free(old_value);

   return EINA_TRUE;
}

/**
 * @brief Deletes a key-value pair from the prediction hints hash table.
 *
 * Removes a specific prediction hint identified by @p key from the
 * `ctx->prediction_hint_hash` table.
 *
 * @param ctx The input method context.
 * @param key The key of the prediction hint to delete.
 * @return @c EINA_TRUE if the key was found and deleted, @c EINA_FALSE otherwise
 *         (e.g., key not found, hash table doesn't exist, invalid context).
 */
EAPI Eina_Bool
ecore_imf_context_prediction_hint_hash_del(Ecore_IMF_Context *ctx, const char *key)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_prediction_hint_hash_del");
        return EINA_FALSE;
     }

   if (!ctx->prediction_hint_hash)
     return EINA_FALSE;

   return eina_hash_del(ctx->prediction_hint_hash, key, NULL);
}

/**
 * @brief Gets the prediction hints hash table.
 *
 * Retrieves a pointer to the Eina_Hash table containing all key-value
 * prediction hints set for this context. The returned hash table should
 * not be modified or freed by the caller; it is owned by the context.
 *
 * @param ctx The input method context.
 * @return A const pointer to the Eina_Hash table, or @c NULL if no hints
 *         are set or @p ctx is invalid.
 */
EAPI const Eina_Hash *
ecore_imf_context_prediction_hint_hash_get(Ecore_IMF_Context *ctx)
{
   if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
     {
        ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                         "ecore_imf_context_prediction_hint_hash_get");
        return NULL;
     }

   return ctx->prediction_hint_hash;
}
