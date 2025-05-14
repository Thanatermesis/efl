#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_GRID_DEFAULT_ITEM_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_part_helper.h"

#define MY_CLASS      EFL_UI_GRID_DEFAULT_ITEM_CLASS
#define MY_CLASS_PFX  efl_ui_grid_default_item

#define MY_CLASS_NAME "Efl.Ui.Grid_Default_Item"

/**
 * @internal
 * @brief Constructor for the Efl.Ui.Grid_Default_Item.
 *
 * This function is called when a new Efl.Ui.Grid_Default_Item object is
 * created. It initializes the object by calling the superclass constructor
 * and sets a default theme class "grid_item" if no theme class has been
 * previously set for this widget.
 *
 * @param[in] obj The Efl.Ui.Grid_Default_Item object to construct.
 * @param[in] pd Private data for the Efl.Ui.Grid_Default_Item object (unused).
 * @return The constructed Efl_Object, or @c NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_ui_grid_default_item_efl_object_constructor(Eo *obj, void *pd EINA_UNUSED)
{
   Eo *eo;

   eo = efl_constructor(efl_super(obj, MY_CLASS));

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "grid_item");

   return eo;
}

#include "efl_ui_grid_default_item.eo.c"
