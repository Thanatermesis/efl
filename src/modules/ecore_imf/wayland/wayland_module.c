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
#include "config.h"
#endif

#include <Ecore.h>
#include <Ecore_IMF.h>
#include <Ecore_Wl2.h>
#include <stdio.h>

#include "wayland_imcontext.h"
#include "text-input-unstable-v1-client-protocol.h"

/**
 * @brief Logging domain for the Ecore IMF Wayland module.
 * Initialized to -1 and registered in im_module_init().
 */
int _ecore_imf_wayland_log_dom = -1;

/**
 * @brief Pointer to the Ecore_Wl2_Display structure.
 * Represents the connection to the Wayland display.
 * Initialized in im_module_init() and used throughout the module.
 */
Ecore_Wl2_Display *ewd;

/**
 * @brief Information structure for the Wayland Input Method module.
 *
 * This structure provides metadata about the IM module, such as its
 * ID, human-readable name, and supported languages.
 */
static const Ecore_IMF_Context_Info wayland_im_info =
{
   "wayland",
   "Wayland",
   "*",
   NULL,
   0
};

/**
 * @brief Class structure for the Wayland Input Method context.
 *
 * This structure defines the set of callback functions that implement
 * the Ecore_IMF_Context interface for the Wayland backend. Each field
 * points to a specific function handling an aspect of the input method
 * context's lifecycle and behavior.
 */
static Ecore_IMF_Context_Class wayland_imf_class =
{
   wayland_im_context_add,                    /**< Called when a new IM context is added. @see wayland_im_context_add */
   wayland_im_context_del,                    /**< Called when an IM context is deleted. @see wayland_im_context_del */
   wayland_im_context_client_window_set,      /**< Sets the client window for the IM context. @see wayland_im_context_client_window_set */
   wayland_im_context_client_canvas_set,      /**< Sets the client canvas for the IM context. @see wayland_im_context_client_canvas_set */
   wayland_im_context_show,                   /**< Shows the input panel. @see wayland_im_context_show */
   wayland_im_context_hide,                   /**< Hides the input panel. @see wayland_im_context_hide */
   wayland_im_context_preedit_string_get,     /**< Retrieves the preedit string. @see wayland_im_context_preedit_string_get */
   wayland_im_context_focus_in,               /**< Called when the IM context gains focus. @see wayland_im_context_focus_in */
   wayland_im_context_focus_out,              /**< Called when the IM context loses focus. @see wayland_im_context_focus_out */
   wayland_im_context_reset,                  /**< Resets the IM context. @see wayland_im_context_reset */
   wayland_im_context_cursor_position_set,    /**< Sets the cursor position. @see wayland_im_context_cursor_position_set */
   wayland_im_context_use_preedit_set,        /**< Enables or disables preedit usage. @see wayland_im_context_use_preedit_set */
   wayland_im_context_input_mode_set,         /**< Sets the input mode. @see wayland_im_context_input_mode_set */
   wayland_im_context_filter_event,           /**< Filters an input event. @see wayland_im_context_filter_event */
   wayland_im_context_preedit_string_with_attributes_get, /**< Retrieves the preedit string with attributes. @see wayland_im_context_preedit_string_with_attributes_get */
   wayland_im_context_prediction_allow_set,   /**< Sets whether text prediction is allowed. @see wayland_im_context_prediction_allow_set */
   wayland_im_context_autocapital_type_set,   /**< Sets the autocapitalization type. @see wayland_im_context_autocapital_type_set */
   NULL,                                      /**< Shows the control panel (not implemented). */
   NULL,                                      /**< Hides the control panel (not implemented). */
   wayland_im_context_input_panel_layout_set, /**< Sets the input panel layout. @see wayland_im_context_input_panel_layout_set */
   NULL,                                      /**< Gets the input panel layout (not implemented). */
   wayland_im_context_input_panel_language_set,/**< Sets the input panel language. @see wayland_im_context_input_panel_language_set */
   NULL,                                      /**< Gets the input panel language (not implemented). */
   wayland_im_context_cursor_location_set,    /**< Sets the cursor location. @see wayland_im_context_cursor_location_set */
   NULL,                                      /**< Sets input panel IM data (not implemented). */
   NULL,                                      /**< Gets input panel IM data (not implemented). */
   NULL,                                      /**< Sets input panel return key type (not implemented). */
   NULL,                                      /**< Sets input panel return key disabled state (not implemented). */
   NULL,                                      /**< Sets input panel caps lock mode (not implemented). */
   NULL,                                      /**< Gets input panel geometry (not implemented). */
   NULL,                                      /**< Gets input panel state (not implemented). */
   NULL,                                      /**< Adds input panel event callback (not implemented). */
   NULL,                                      /**< Deletes input panel event callback (not implemented). */
   wayland_im_context_input_panel_language_locale_get, /**< Gets the input panel language locale. @see wayland_im_context_input_panel_language_locale_get */
   NULL,                                      /**< Gets candidate window geometry (not implemented). */
   wayland_im_context_input_hint_set,         /**< Sets input hints. @see wayland_im_context_input_hint_set */
   NULL,                                      /**< Sets BiDi direction (not implemented). */
   NULL,                                      /**< Gets keyboard mode (not implemented). */
   NULL,                                      /**< Sets prediction hint (not implemented). */
   NULL,                                      /**< Sets MIME type accept (not implemented). */
   NULL                                       /**< Sets input panel position (not implemented). */
};

/**
 * @brief Global Wayland text input manager (zwp_text_input_manager_v1).
 *
 * This object is obtained from the Wayland compositor and is used to create
 * text_input objects for handling text input. It is initialized in
 * im_module_create() if not already available.
 */
static struct zwp_text_input_manager_v1 *text_input_manager = NULL;

/**
 * @brief Callback function for exiting/unloading the IM module.
 *
 * This function is registered with Ecore_IMF to be called when the
 * IM module is no longer needed.
 *
 * @return Always returns NULL, as per Ecore_IMF_Module_Exit_Func requirements.
 */
static Ecore_IMF_Context *
im_module_exit(void)
{
   return NULL;
}

/**
 * @brief Creates a new Wayland Input Method context.
 *
 * This function is called by Ecore_IMF to create a new instance of an
 * input method context. It attempts to bind to the
 * `zwp_text_input_manager_v1` Wayland global interface if not already
 * done. Then, it creates a new WaylandIMContext and associates it
 * with a new Ecore_IMF_Context.
 *
 * @return A pointer to the newly created Ecore_IMF_Context on success,
 *         or NULL on failure (e.g., if `zwp_text_input_manager_v1`
 *         is unavailable or context creation fails).
 */
static Ecore_IMF_Context *
im_module_create()
{
   Ecore_IMF_Context *ctx = NULL;
   WaylandIMContext *ctxd = NULL;

   if (!text_input_manager)
     {
        Eina_Iterator *itr;

        itr = ecore_wl2_display_globals_get(ewd);
        if (itr)
          {
             Ecore_Wl2_Global *global;
             struct wl_registry *registry;

             registry = ecore_wl2_display_registry_get(ewd);
             EINA_ITERATOR_FOREACH(itr, global)
               {
                  if (!strcmp(global->interface, "zwp_text_input_manager_v1"))
                    {
                       text_input_manager =
                         wl_registry_bind(registry, global->id,
                                          &zwp_text_input_manager_v1_interface, 1);
                       EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom,
                                         "bound wl_text_input_manager interface");
                       break;
                    }
               }
             eina_iterator_free(itr);
          }

        if (!text_input_manager)
          return NULL;
     }

   ctxd = wayland_im_context_new(text_input_manager);
   if (!ctxd) return NULL;

   ctx = ecore_imf_context_new(&wayland_imf_class);
   if (!ctx)
     {
        free(ctxd);
        return NULL;
     }

   ecore_imf_context_data_set(ctx, ctxd);

   return ctx;
}

/**
 * @brief Initializes the Wayland Input Method module.
 *
 * This function is the entry point for the IM module, called by Ecore_IMF
 * during its initialization. It performs several crucial steps:
 * 1. Registers a logging domain for the module.
 * 2. Checks for the presence of a Wayland display (WAYLAND_DISPLAY env var).
 * 3. Optionally checks ELM_DISPLAY environment variable.
 * 4. Initializes the Ecore_Wl2 library.
 * 5. Connects to the Wayland display using Ecore_Wl2.
 * 6. Registers the IM module with Ecore_IMF, providing callbacks for
 *    creating and exiting IM contexts.
 *
 * @return EINA_TRUE on successful initialization, EINA_FALSE otherwise.
 */
static Eina_Bool
im_module_init(void)
{
   const char *s;

   _ecore_imf_wayland_log_dom =
     eina_log_domain_register("ecore_imf_wayland", EINA_COLOR_YELLOW);

   if (!getenv("WAYLAND_DISPLAY")) return EINA_FALSE;
   if ((s = getenv("ELM_DISPLAY")))
     {
        if (strcmp(s, "wl")) return EINA_FALSE;
     }

   if (!ecore_wl2_init())
     return EINA_FALSE;

   ewd = ecore_wl2_display_connect(NULL);
   if (!ewd) goto err;

   ecore_imf_module_register(&wayland_im_info, im_module_create,
                             im_module_exit);
   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom, "im module initialized");

   return EINA_TRUE;

err:
   ecore_wl2_shutdown();
   return EINA_FALSE;
}

/**
 * @brief Shuts down the Wayland Input Method module.
 *
 * This function is called by Ecore_IMF when the module is being unloaded.
 * It performs cleanup tasks:
 * 1. Logs the shutdown event.
 * 2. Disconnects from the Wayland display using Ecore_Wl2.
 * 3. Shuts down the Ecore_Wl2 library.
 */
static void
im_module_shutdown(void)
{
   EINA_LOG_DOM_INFO(_ecore_imf_wayland_log_dom, "im module shutdown");
   ecore_wl2_display_disconnect(ewd);
   ecore_wl2_shutdown();
}

EINA_MODULE_INIT(im_module_init);
EINA_MODULE_SHUTDOWN(im_module_shutdown);

/* vim:ts=8 sw=3 sts=3 expandtab cino=>5n-3f0^-2{2(0W1st0
*/
