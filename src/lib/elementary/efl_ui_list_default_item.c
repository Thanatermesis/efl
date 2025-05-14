#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_LIST_DEFAULT_ITEM_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_part_helper.h"

#define MY_CLASS      EFL_UI_LIST_DEFAULT_ITEM_CLASS
#define MY_CLASS_PFX  efl_ui_list_default_item

#define MY_CLASS_NAME "Efl.Ui.List_Default_Item"

/**
 * @brief Constructor for the Efl.Ui.List_Default_Item.
 *
 * This function is called when a new Efl.Ui.List_Default_Item object is
 * created. It initializes the object by calling the superclass constructor
 * and sets a default theme ("list_item") if no theme has been
 * explicitly set for the widget.
 *
 * @param obj The Efl_Object to construct.
 * @param pd Private data for the object (unused in this function).
 * @return The constructed Efl_Object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_ui_list_default_item_efl_object_constructor(Eo *obj, void *pd EINA_UNUSED)
{
   Eo *eo;

   eo = efl_constructor(efl_super(obj, MY_CLASS));

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "list_item");

   return eo;
}
#include "efl_ui_list_default_item.eo.c"
