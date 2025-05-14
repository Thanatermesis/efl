/**
 * @file
 * @brief This file implements the Efl_Ui_Stack widget.
 *
 * The Efl_Ui_Stack widget is a container that manages its children
 * in a stack-like manner, where only one child is visible at a time.
 * It uses a spotlight manager to handle transitions between children.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>
#include "elm_priv.h"

#define MY_CLASS EFL_UI_STACK_CLASS

/**
 * @brief Private data structure for the Efl_Ui_Stack widget.
 *
 * This structure holds any private data specific to the Efl_Ui_Stack
 * instance. Currently, it is empty but serves as a placeholder for
 * future extensions.
 */
typedef struct {

} Efl_Ui_Stack_Data;

/**
 * @brief Constructor for the Efl_Ui_Stack object.
 *
 * This function is called when a new Efl_Ui_Stack object is created.
 * It initializes the object and sets up the spotlight manager
 * which is responsible for managing the visibility and transitions
 * of the stacked child elements.
 *
 * @param obj The Efl_Ui_Stack object to construct.
 * @param sd Pointer to the private data of the Efl_Ui_Stack object.
 * @return The constructed Efl_Object, or @c NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_ui_stack_efl_object_constructor(Eo *obj, Efl_Ui_Stack_Data *sd EINA_UNUSED)
{
   Eo *stack;

   obj = efl_constructor(efl_super(obj, MY_CLASS));

   stack = efl_new(EFL_UI_SPOTLIGHT_FADE_MANAGER_CLASS);
   efl_ui_spotlight_manager_set(obj, stack);

   return obj;
}

#include "efl_ui_stack.eo.c"
