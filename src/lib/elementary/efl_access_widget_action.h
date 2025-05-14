#ifndef ELM_INTERFACE_ATSPI_WIDGET_ACTION_H
#define ELM_INTERFACE_ATSPI_WIDGET_ACTION_H

#ifdef EFL_BETA_API_SUPPORT

/**
 * @brief Structure defining an accessible action.
 *
 * This structure holds information about a specific action that can be
 * performed on an accessible widget. It includes the action's name,
 * a string identifier for the action, parameters for the action, and
 * a function pointer to execute the action.
 *
 * @since 1.26
 */
struct _Efl_Access_Action_Data
{
   const char *name; /**< The human-readable name of the action (e.g., "click", "open"). */
   const char *action; /**< A string identifying the action, used for keybindings (e.g., "activate", "toggle"). */
   const char *param; /**< Optional parameters for the action. */
   Eina_Bool (*func)(Evas_Object *obj, const char *params); /**< Function pointer to execute the action. Takes the Evas_Object and parameters as input. Returns EINA_TRUE on success, EINA_FALSE on failure. */
};

/**
 * @typedef Efl_Access_Action_Data
 * @brief Typedef for struct _Efl_Access_Action_Data.
 * @since 1.26
 */
typedef struct _Efl_Access_Action_Data Efl_Access_Action_Data;

#include "efl_access_widget_action.eo.h"

#endif
#endif
