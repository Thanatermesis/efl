#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_LAYOUT_PROTECTED
#define EFL_UI_SCROLL_MANAGER_PROTECTED
#define EFL_UI_SCROLLBAR_PROTECTED

#include <Efl_Ui.h>
#include "elm_priv.h"
#include "efl_ui_item_private.h"

#define MY_CLASS      EFL_UI_LIST_CLASS
#define MY_CLASS_PFX  efl_ui_list

/**
 * @def MY_CLASS_NAME
 * @brief The name of the Efl.Ui.List class.
 * @since 1.24
 */
#define MY_CLASS_NAME "Efl.Ui.List"

/**
 * @internal
 * @brief Constructor for the Efl.Ui.List object.
 *
 * This function is called when a new Efl.Ui.List object is created.
 * It initializes the object by calling the parent class's constructor
 * and sets up a default position manager for handling item layout.
 *
 * @param[in] obj The Efl.Ui.List object to construct.
 * @param[in] pd Private data for the Efl.Ui.List class (unused).
 * @return The constructed Efl.Ui.List object.
 * @since 1.24
 */
static Eo *
_efl_ui_list_efl_object_constructor(Eo *obj, void *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   efl_ui_collection_position_manager_set(obj, efl_new(EFL_UI_POSITION_MANAGER_LIST_CLASS));

   return obj;
}

#include "efl_ui_list.eo.c"
