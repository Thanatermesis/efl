#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_WIDGET_FOCUS_MANAGER_PROTECTED

#include <Elementary.h>
#include <Elementary_Cursor.h>

#include "elm_priv.h"

#define MY_CLASS EFL_UI_WIDGET_FOCUS_MANAGER_MIXIN

/**
 * @brief Private data structure for the Efl_Ui_Widget_Focus_Manager_Mixin.
 *
 * This structure holds a pointer to the actual focus manager instance
 * that this mixin wraps and manages.
 */
typedef struct
{
   Efl_Ui_Focus_Manager *manager; /**< The internal focus manager instance. */
} Efl_Ui_Widget_Focus_Manager_Data;

/**
 * @brief Constructor for the Efl_Ui_Widget_Focus_Manager_Mixin.
 *
 * This function is called when an object implementing this mixin is constructed.
 * It initializes the internal focus manager, sets up references, and attaches
 * it to the composite object. It also registers for event redirection.
 *
 * @param obj The Eo object being constructed.
 * @param pd The private data for this mixin instance.
 * @return The constructed Eo object, or NULL on failure.
 */
EOLIAN static Efl_Object*
_efl_ui_widget_focus_manager_efl_object_constructor(Eo *obj, Efl_Ui_Widget_Focus_Manager_Data *pd)
{
   Eo *res = NULL;

   pd->manager = efl_ui_widget_focus_manager_create(obj, obj);

   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->manager, res);

   efl_ref(pd->manager);
   efl_composite_attach(obj, pd->manager);
   _efl_ui_focus_manager_redirect_events_add(pd->manager, obj);
   res = efl_constructor(efl_super(obj, MY_CLASS));


   return res;
}

/**
 * @brief Destructor for the Efl_Ui_Widget_Focus_Manager_Mixin.
 *
 * This function is called when an object implementing this mixin is destructed.
 * It cleans up resources, unregisters event redirection, and releases the
 * reference to the internal focus manager.
 *
 * @param obj The Eo object being destructed.
 * @param pd The private data for this mixin instance.
 */
EOLIAN static void
_efl_ui_widget_focus_manager_efl_object_destructor(Eo *obj, Efl_Ui_Widget_Focus_Manager_Data *pd)
{
   efl_destructor(efl_super(obj, MY_CLASS));

   _efl_ui_focus_manager_redirect_events_del(pd->manager, obj);
   efl_unref(pd->manager);
   pd->manager = NULL;
}

/**
 * @brief Applies the focus state to the widget.
 *
 * This function is responsible for applying the given focus state to the widget.
 * It calls the superclass's implementation and then, if the focus manager
 * was part of the current state but not the configured state (meaning focus
 * was lost from this manager), it resets the focus history for the object.
 *
 * @param obj The Eo object.
 * @param pd The private data for this mixin instance.
 * @param current_state The current focus state of the widget.
 * @param configured_state A pointer to the focus state that the widget should be configured to.
 *                         This function may modify this structure.
 * @param redirect The widget to redirect focus to, if any.
 * @return EINA_TRUE if the focus state was successfully applied, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_widget_focus_manager_efl_ui_widget_focus_state_apply(Eo *obj, Efl_Ui_Widget_Focus_Manager_Data *pd EINA_UNUSED, Efl_Ui_Widget_Focus_State current_state, Efl_Ui_Widget_Focus_State *configured_state, Efl_Ui_Widget *redirect)
{
   Eina_Bool state;

   state = efl_ui_widget_focus_state_apply(efl_super(obj, MY_CLASS), current_state, configured_state, redirect);

   if (!state && !configured_state->manager && current_state.manager)
     efl_ui_focus_manager_reset_history(obj);

   return state;
}

#include "efl_ui_widget_focus_manager.eo.c"
