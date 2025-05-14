/**
 * @file
 * @brief This file implements the Efl_Ui_Pager widget.
 *
 * The pager widget is a container that displays one child at a time,
 * allowing the user to navigate between children, typically by swiping
 * horizontally.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>
#include "elm_priv.h"

#define MY_CLASS EFL_UI_PAGER_CLASS

/**
 * @brief Private data structure for the Efl_Ui_Pager widget.
 *
 * This structure holds any private data specific to the pager widget's
 * implementation. Currently, it is empty but serves as a placeholder
 * for future extensions.
 */
typedef struct {

} Efl_Ui_Pager_Data;

/**
 * @internal
 * @brief Constructor for the Efl_Ui_Pager object.
 *
 * This function is called when a new Efl_Ui_Pager object is created.
 * It initializes the pager, including setting up its scroll manager.
 *
 * @param[in] obj The Efl_Ui_Pager object being constructed.
 * @param[in] sd Pointer to the private data of the Efl_Ui_Pager object.
 * @return The constructed Efl_Ui_Pager object.
 */
EOLIAN static Efl_Object *
_efl_ui_pager_efl_object_constructor(Eo *obj, Efl_Ui_Pager_Data *sd EINA_UNUSED)
{
   Eo *scroller;

   obj = efl_constructor(efl_super(obj, MY_CLASS));

   scroller = efl_new(EFL_UI_SPOTLIGHT_SCROLL_MANAGER_CLASS);
   efl_ui_spotlight_manager_set(obj, scroller);

   return obj;
}

#include "efl_ui_pager.eo.c"
