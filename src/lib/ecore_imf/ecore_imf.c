#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include <ecore_private.h>

#include "Ecore_IMF.h"
#include "ecore_imf_private.h"

/**
 * @brief Event type for the start of preedit string input.
 * This event is triggered when an input method editor (IME) starts a new preedit session.
 */
EAPI int ECORE_IMF_EVENT_PREEDIT_START = 0;
/**
 * @brief Event type for the end of preedit string input.
 * This event is triggered when an IME ends the current preedit session.
 */
EAPI int ECORE_IMF_EVENT_PREEDIT_END = 0;
/**
 * @brief Event type for a change in the preedit string.
 * This event is triggered when the content of the preedit string is modified by the IME.
 */
EAPI int ECORE_IMF_EVENT_PREEDIT_CHANGED = 0;
/**
 * @brief Event type for committing a string.
 * This event is triggered when the IME commits a string to be inserted into the application.
 */
EAPI int ECORE_IMF_EVENT_COMMIT = 0;
/**
 * @brief Event type for deleting surrounding text.
 * This event is triggered when the IME requests the deletion of text surrounding the current cursor position.
 */
EAPI int ECORE_IMF_EVENT_DELETE_SURROUNDING = 0;

/**
 * @internal
 * @brief Log domain for Ecore IMF.
 */
int _ecore_imf_log_dom = -1;
/**
 * @internal
 * @brief Initialization counter for Ecore IMF.
 * Used to ensure Ecore IMF is initialized and shut down correctly.
 */
static int _ecore_imf_init_count = 0;
/**
 * @internal
 * @brief Stores the context that last requested the input panel to be shown.
 * This is used to manage the visibility of the input panel.
 */
extern Ecore_IMF_Context *show_req_ctx;

/**
 * @brief Initializes the Ecore IMF library.
 *
 * This function initializes all the necessary components for Ecore IMF to work,
 * including Ecore itself if it hasn't been initialized yet. It also registers
 * a log domain for Ecore IMF messages and initializes the IMF modules.
 * Event types for various IMF events are created here.
 *
 * This function increments an internal counter. It will return the new
 * value of the counter. It will return 0 if an error occurs.
 *
 * @return The number of times the library has been initialized, or 0 on failure.
 * @see ecore_imf_shutdown()
 */
EAPI int
ecore_imf_init(void)
{
   if (++_ecore_imf_init_count != 1) return _ecore_imf_init_count;

   if (!ecore_init()) return --_ecore_imf_init_count;
   _ecore_imf_log_dom = eina_log_domain_register
      ("ecore_imf", ECORE_IMF_DEFAULT_LOG_COLOR);
   if (_ecore_imf_log_dom < 0)
     {
        EINA_LOG_ERR("Impossible to create a log domain for the Ecore IMF module.");
        ecore_shutdown();
        return --_ecore_imf_init_count;
     }
   ecore_imf_module_init();

   ECORE_IMF_EVENT_PREEDIT_START = ecore_event_type_new();
   ECORE_IMF_EVENT_PREEDIT_END = ecore_event_type_new();
   ECORE_IMF_EVENT_PREEDIT_CHANGED = ecore_event_type_new();
   ECORE_IMF_EVENT_COMMIT = ecore_event_type_new();
   ECORE_IMF_EVENT_DELETE_SURROUNDING = ecore_event_type_new();

   return _ecore_imf_init_count;
}

/**
 * @brief Shuts down the Ecore IMF library.
 *
 * This function cleans up resources used by Ecore IMF. It flushes any pending
 * IMF events, shuts down IMF modules, unregisters the log domain, and
 * shuts down Ecore if this is the last user.
 *
 * This function decrements an internal counter. When the counter reaches 0,
 * all Ecore IMF resources are freed.
 *
 * @return The number of times the library still needs to be shut down.
 * @see ecore_imf_init()
 */
EAPI int
ecore_imf_shutdown(void)
{
   if (--_ecore_imf_init_count != 0) return _ecore_imf_init_count;

   ecore_event_type_flush(ECORE_IMF_EVENT_PREEDIT_START,
                          ECORE_IMF_EVENT_PREEDIT_END,
                          ECORE_IMF_EVENT_PREEDIT_CHANGED,
                          ECORE_IMF_EVENT_COMMIT,
                          ECORE_IMF_EVENT_DELETE_SURROUNDING);

   ecore_imf_module_shutdown();
   eina_log_domain_unregister(_ecore_imf_log_dom);
   _ecore_imf_log_dom = -1;
   ecore_shutdown();
   return _ecore_imf_init_count;
}

/**
 * @brief Hides the input panel (virtual keyboard).
 *
 * This function attempts to hide the currently visible input panel.
 * It checks if there is a context that requested the panel to be shown
 * (show_req_ctx) and if the panel is not already hidden.
 *
 * @return @c EINA_TRUE if the input panel was successfully requested to hide,
 *         @c EINA_FALSE otherwise (e.g., if no panel was shown or it was already hidden).
 */
EAPI Eina_Bool
ecore_imf_input_panel_hide(void)
{
   if (show_req_ctx)
     {
        if (ecore_imf_context_input_panel_state_get(show_req_ctx) != ECORE_IMF_INPUT_PANEL_STATE_HIDE)
          {
             ecore_imf_context_input_panel_hide(show_req_ctx);
             return EINA_TRUE;
          }
     }

   return EINA_FALSE;
}
