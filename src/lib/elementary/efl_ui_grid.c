#ifdef HAVE_CONFIG_H
#include "elementary_config.h"
#endif

#define ELM_LAYOUT_PROTECTED
#define EFL_UI_SCROLL_MANAGER_PROTECTED
#define EFL_UI_SCROLLBAR_PROTECTED

#include <Efl_Ui.h>
#include "elm_priv.h"
#include "efl_ui_item_private.h"

#define MY_CLASS      EFL_UI_GRID_CLASS
#define MY_CLASS_PFX  efl_ui_grid

#define MY_CLASS_NAME "Efl.Ui.Grid"

/**
 * @internal
 * @brief Constructor for the Efl.Ui.Grid class.
 *
 * This function is called when a new Efl.Ui.Grid object is created.
 * It initializes the object and sets up its default position manager.
 *
 * @param[in] obj The Efl.Ui.Grid object to construct.
 * @param[in] pd Private data for the Efl.Ui.Grid class (unused).
 * @return The constructed Efl.Ui.Grid object, or @c NULL on failure.
 */
EOLIAN static Eo *
_efl_ui_grid_efl_object_constructor(Eo *obj, void *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   efl_ui_collection_position_manager_set(obj, efl_new(EFL_UI_POSITION_MANAGER_GRID_CLASS));

   return obj;
}

#include "efl_ui_grid.eo.c"
