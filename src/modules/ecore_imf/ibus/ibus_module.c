#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include <Ecore.h>
#include <Ecore_IMF.h>

#include <ibus.h>
#include "ibus_imcontext.h"

#define IBUS_LOCALDIR ""

/**
 * @brief Information about the IBus Ecore IMF module.
 * This structure provides metadata for the IBus input method module,
 * such as its ID, human-readable name, and supported languages.
 */
static const Ecore_IMF_Context_Info ibus_im_info = {
    "ibus",                             /**< ID string for the IMF context */
    "IBus (Intelligent Input Bus)",     /**< Human-readable name */
    "*",
    NULL,                               /**< Supported languages (NULL for all) */
    0                                   /**< Flags */
};

/**
 * @brief Defines the Ecore IMF context class for IBus.
 * This structure maps Ecore IMF context operations to their IBus-specific
 * implementations. Each field is a function pointer for a specific IMF action.
 */
static Ecore_IMF_Context_Class ibus_imf_class = {
    ecore_imf_context_ibus_add,                    /**< Called when the context is added to an Ecore_IMF_Context object. */
    ecore_imf_context_ibus_del,                    /**< Called when the context is deleted from an Ecore_IMF_Context object. */
    ecore_imf_context_ibus_client_window_set,      /**< Sets the client window for the IMF context. */
    ecore_imf_context_ibus_client_canvas_set,      /**< Sets the client canvas (widget) for the IMF context. */
    NULL,                                          /**< Shows the input panel (e.g., virtual keyboard). Not implemented directly by IBus core. */
    NULL,                                          /**< Hides the input panel. Not implemented directly by IBus core. */
    ecore_imf_context_ibus_preedit_string_get,     /**< Retrieves the current preedit string. */
    ecore_imf_context_ibus_focus_in,               /**< Called when the associated widget gains focus. */
    ecore_imf_context_ibus_focus_out,              /**< Called when the associated widget loses focus. */
    ecore_imf_context_ibus_reset,                  /**< Resets the IMF context (e.g., clears preedit string). */
    NULL,                                          /**< Sets the cursor position within the preedit string. */
    ecore_imf_context_ibus_use_preedit_set,        /**< Enables or disables the use of a preedit string. */
    NULL,                                          /**< Sets the input mode (e.g., alphanumeric, numeric). */
    ecore_imf_context_ibus_filter_event,           /**< Filters an input event (e.g., key press/release). */
    ecore_imf_context_ibus_preedit_string_with_attributes_get,  /**< Retrieves the preedit string with text attributes. */
    NULL,                                          /**< Enables or disables text prediction. */
    NULL,                                          /**< Sets the auto-capitalization type. */
    NULL,                                          /**< Shows the input method control panel. */
    NULL,                                          /**< Hides the input method control panel. */
    NULL,                                          /**< Sets the layout for the input panel. */
    NULL,                                          /**< Gets the layout of the input panel. */
    NULL,                                          /**< Sets the language for the input panel. */
    NULL,                                          /**< Gets the language of the input panel. */
    ecore_imf_context_ibus_cursor_location_set,    /**< Sets the location of the text input cursor on screen. */
    NULL,                                          /**< Sets IM-specific data for the input panel. */
    NULL,                                          /**< Gets IM-specific data from the input panel. */
    NULL,                                          /**< Sets the return key type for the input panel (e.g., "Done", "Search"). */
    NULL,                                          /**< Sets whether the return key on the input panel is disabled. */
    NULL,                                          /**< Sets the caps lock mode for the input panel. */
    NULL,                                          /**< Gets the geometry of the input panel. */
    NULL,                                          /**< Gets the state of the input panel. */
    NULL,                                          /**< Adds a callback for input panel events. */
    NULL,                                          /**< Deletes a callback for input panel events. */
    NULL,                                          /**< Gets the language locale of the input panel. */
    NULL,                                          /**< Gets the geometry of the candidate panel. */
    NULL,                                          /**< Sets input hints (e.g., content type, purpose). */
    NULL,                                          /**< Sets the bidirectional text direction. */
    NULL,                                          /**< Gets the current keyboard mode (e.g., symbolic, text). */
    NULL,                                          /**< Sets prediction hints. */
    NULL,                                          /**< Sets the accepted MIME types for content insertion. */
    NULL                                           /**< Sets the desired position of the input panel. */
};

static Ecore_IMF_Context *im_module_create(void);
static Ecore_IMF_Context *im_module_exit(void);

/**
 * @brief Initializes the IBus Ecore IMF module.
 *
 * This function is called when the module is loaded. It performs necessary
 * setup, such as checking for a valid display environment, integrating with
 * the GLib main loop (which IBus uses), initializing the IBus library,
 * and registering the IBus IMF module with Ecore.
 *
 * @return EINA_TRUE on successful initialization, EINA_FALSE otherwise.
 */
static Eina_Bool
im_module_init(void)
{
   const char *s;

   if (!getenv("DISPLAY")) return EINA_FALSE;
   if ((s = getenv("ELM_DISPLAY")))
     {
        if (strcmp(s, "x11")) return EINA_FALSE;
     }
   ecore_main_loop_glib_integrate();
   ibus_init();
   ecore_imf_module_register(&ibus_im_info, im_module_create, im_module_exit);

   return EINA_TRUE;
}

/**
 * @brief Shuts down the IBus Ecore IMF module.
 *
 * This function is called when the module is unloaded. It calls the
 * IBus-specific shutdown routine to clean up resources used by the
 * IBus IMF context implementation.
 */
static void im_module_shutdown(void)
{
   ecore_imf_context_ibus_shutdown();
}

/**
 * @brief Called when the Ecore IMF module is exited.
 *
 * This function is registered as the exit callback for the module.
 * In this implementation, it currently does nothing and returns NULL.
 *
 * @return Always returns NULL.
 */
static Ecore_IMF_Context *
im_module_exit(void)
{
   return NULL;
}

/**
 * @brief Creates a new IBus Ecore IMF context.
 *
 * This function is called by Ecore IMF to instantiate an IBus input context.
 * It allocates and initializes the IBus-specific context data (`IBusIMContext`)
 * and then creates a generic `Ecore_IMF_Context`, associating the IBus-specific
 * data with it.
 *
 * @return A pointer to the newly created Ecore_IMF_Context on success,
 *         or NULL on failure.
 */
static Ecore_IMF_Context *
im_module_create(void)
{
   Ecore_IMF_Context *ctx = NULL;
   IBusIMContext *ctxd = NULL;

   ctxd = ecore_imf_context_ibus_new();
   if (!ctxd)
     {
        return NULL;
     }

   ctx = ecore_imf_context_new(&ibus_imf_class);
   if (!ctx)
     {
        free(ctxd);
        return NULL;
     }

   ecore_imf_context_data_set(ctx, ctxd);

   return ctx;
}

EINA_MODULE_INIT(im_module_init);
EINA_MODULE_SHUTDOWN(im_module_shutdown);

