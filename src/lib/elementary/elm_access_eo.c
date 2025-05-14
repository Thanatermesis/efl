/**
 * @file
 * @brief Implementation of the Elm_Access class.
 */

/**
 * @internal
 * @brief Constructor for Elm_Access objects.
 * @param obj The Eo object.
 * @param pd Private data for the object.
 * @return The constructed Efl_Object.
 */
Efl_Object *_elm_access_efl_object_constructor(Eo *obj, void *pd);

/**
 * @internal
 * @brief Handles the access activation for an Elm_Access widget.
 * @param obj The Eo object.
 * @param pd Private data for the object.
 * @param act The activation type.
 * @return EINA_TRUE if activation was handled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_access_efl_ui_widget_on_access_activate(Eo *obj, void *pd, Efl_Ui_Activate act);

/**
 * @internal
 * @brief Handles focus updates for an Elm_Access object.
 * @param obj The Eo object.
 * @param pd Private data for the object.
 * @return EINA_TRUE if focus update was handled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_access_efl_ui_focus_object_on_focus_update(Eo *obj, void *pd);

/**
 * @internal
 * @brief Gets the Elm-specific accessibility actions for an Elm_Access widget.
 * @param obj The Eo object.
 * @param pd Private data for the object.
 * @return A pointer to an array of Efl_Access_Action_Data, or NULL.
 *         The last element of the array should have its `name` field set to NULL.
 *         Example:
 *         static const Efl_Access_Action_Data actions[] = {
 *           { "action_name_1", "action_keybinding_1", _action_cb_1 },
 *           { "action_name_2", NULL, _action_cb_2 },
 *           { NULL, NULL, NULL }
 *         };
 *         return actions;
 */
const Efl_Access_Action_Data *_elm_access_efl_access_widget_action_elm_actions_get(const Eo *obj, void *pd);

/**
 * @internal
 * @brief Gets the state set for an Elm_Access object.
 * @param obj The Eo object.
 * @param pd Private data for the object.
 * @return The Efl_Access_State_Set representing the object's states.
 */
Efl_Access_State_Set _elm_access_efl_access_object_state_set_get(const Eo *obj, void *pd);

/**
 * @internal
 * @brief Initializes the Elm_Access class.
 *
 * Sets up the operations and functions for the class.
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_access_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_ACCESS_EXTRA_OPS
#define ELM_ACCESS_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_access_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_on_access_activate, _elm_access_efl_ui_widget_on_access_activate),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_on_focus_update, _elm_access_efl_ui_focus_object_on_focus_update),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_access_efl_access_widget_action_elm_actions_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_state_set_get, _elm_access_efl_access_object_state_set_get),
      ELM_ACCESS_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Access class.
 *
 * Contains metadata about the class, such as its name, version,
 * and initializer functions.
 */
static const Efl_Class_Description _elm_access_class_desc = {
   EO_VERSION,
   "Elm.Access", /**< Class name */
   EFL_CLASS_TYPE_REGULAR, /**< Class type */
   0,
   _elm_access_class_initializer, /**< Class initializer function */
   _elm_access_class_constructor, /**< Class constructor function for instances */
   NULL /**< Class destructor function for instances */
};

/**
 * @internal
 * @brief Defines the Elm_Access class.
 *
 * This macro takes the class description and parent classes to create
 * the Elm_Access class.
 * @param elm_access_class_get The function to get the Elm_Access class.
 * @param _elm_access_class_desc The description of the Elm_Access class.
 * @param EFL_UI_WIDGET_CLASS The parent Efl_Ui_Widget class.
 * @param EFL_ACCESS_WIDGET_ACTION_MIXIN The Efl_Access_Widget_Action mixin.
 */
EFL_DEFINE_CLASS(elm_access_class_get, &_elm_access_class_desc, EFL_UI_WIDGET_CLASS, EFL_ACCESS_WIDGET_ACTION_MIXIN, NULL);
