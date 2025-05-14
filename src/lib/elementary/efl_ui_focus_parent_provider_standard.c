/**
 * @file
 * @brief This file implements the Efl.Ui.Focus.Parent_Provider.Standard interface.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"

/**
 * @brief Private data for the Efl.Ui.Focus.Parent_Provider.Standard class.
 *
 * This structure holds any private data specific to instances of this class.
 * Currently, it is empty as no instance-specific data is needed.
 */
typedef struct {

} Efl_Ui_Focus_Parent_Provider_Standard_Data;

/**
 * @brief Finds the logical parent widget for focus management.
 *
 * This function implements the Efl.Ui.Focus.Parent_Provider.find_logical_parent
 * Eolian interface. It determines the parent widget that should be considered
 * for focus chain calculations. In this standard implementation, it simply
 * returns the Elm_Widget's parent.
 *
 * @param[in] obj The Eolian object instance (not used).
 * @param[in] pd The private data for this object (not used).
 * @param[in] widget The widget for which to find the logical parent.
 * @return The logical parent widget, or @c NULL if none is found.
 */
EOLIAN static Efl_Ui_Focus_Object*
_efl_ui_focus_parent_provider_standard_efl_ui_focus_parent_provider_find_logical_parent(Eo *obj EINA_UNUSED, Efl_Ui_Focus_Parent_Provider_Standard_Data *pd EINA_UNUSED, Efl_Ui_Focus_Object *widget)
{
   return elm_object_parent_widget_get(widget);
}


#include "efl_ui_focus_parent_provider_standard.eo.c"
