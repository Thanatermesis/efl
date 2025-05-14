/**
 * @file
 * @brief This file implements the Efl.Ui.List_View class, a container
 *        that arranges its items in a single column or row.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>

#define MY_CLASS      EFL_UI_LIST_VIEW_CLASS
#define MY_CLASS_PFX  efl_ui_list_view

#define MY_CLASS_NAME "Efl.Ui.List_View"

/**
 * @internal
 * @brief Constructor for the Efl.Ui.List_View class.
 *
 * This function is called when a new Efl.Ui.List_View object is created.
 * It initializes the object by calling the parent class's constructor and
 * sets up a default EFL_UI_POSITION_MANAGER_LIST_CLASS as its position manager.
 *
 * @param[in] obj The Efl.Ui.List_View object to construct.
 * @param[in] pd Private data for the Efl.Ui.List_View class (unused).
 * @return The constructed Efl.Ui.List_View object.
 */
static Eo *
_efl_ui_list_view_efl_object_constructor(Eo *obj, void *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   efl_ui_collection_view_position_manager_set(obj, efl_new(EFL_UI_POSITION_MANAGER_LIST_CLASS));

   return obj;
}

#include "efl_ui_list_view.eo.c"
