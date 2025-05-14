EWAPI const Efl_Event_Description _ELM_CTXPOPUP_EVENT_DISMISSED =
   EFL_EVENT_DESCRIPTION("dismissed");
EWAPI const Efl_Event_Description _ELM_CTXPOPUP_EVENT_GEOMETRY_UPDATE =
   EFL_EVENT_DESCRIPTION("geometry,update");

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_selected_item_get.
 * @param obj The object.
 * @param pd The private data.
 * @return The selected item or @c null.
 */
Elm_Widget_Item *_elm_ctxpopup_selected_item_get(const Eo *obj, Elm_Ctxpopup_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_ctxpopup_selected_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_first_item_get.
 * @param obj The object.
 * @param pd The private data.
 * @return The first item or @c null.
 */
Elm_Widget_Item *_elm_ctxpopup_first_item_get(const Eo *obj, Elm_Ctxpopup_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_ctxpopup_first_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_last_item_get.
 * @param obj The object.
 * @param pd The private data.
 * @return The last item or @c null.
 */
Elm_Widget_Item *_elm_ctxpopup_last_item_get(const Eo *obj, Elm_Ctxpopup_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_ctxpopup_last_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_items_get.
 * @param obj The object.
 * @param pd The private data.
 * @return Const list to widget items.
 */
const Eina_List *_elm_ctxpopup_items_get(const Eo *obj, Elm_Ctxpopup_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_ctxpopup_items_get, const Eina_List *, NULL);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_horizontal_set.
 * @param obj The object.
 * @param pd The private data.
 * @param horizontal @c EINA_TRUE for horizontal mode, @c EINA_FALSE for vertical.
 */
void _elm_ctxpopup_horizontal_set(Eo *obj, Elm_Ctxpopup_Data *pd, Eina_Bool horizontal);

/**
 * @internal
 * @brief Reflection function for the "horizontal" property setter.
 *
 * This function is utilized by the Eolian reflection system to set the
 * "horizontal" property. It converts an Eina_Value to a boolean
 * and then invokes the concrete implementation @ref _elm_ctxpopup_horizontal_set.
 *
 * @param obj The EFL object instance.
 * @param val An Eina_Value holding the boolean state for the horizontal property.
 *            For example, an Eina_Value representing @c EINA_TRUE.
 * @return EINA_ERROR_NO_ERROR on successful conversion and call,
 *         otherwise EINA_ERROR_VALUE_FAILED if conversion fails.
 */
static Eina_Error
__eolian_elm_ctxpopup_horizontal_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_ctxpopup_horizontal_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_ctxpopup_horizontal_set, EFL_FUNC_CALL(horizontal), Eina_Bool horizontal);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_horizontal_get.
 * @param obj The object.
 * @param pd The private data.
 * @return @c EINA_TRUE for horizontal mode, @c EINA_FALSE for vertical.
 */
Eina_Bool _elm_ctxpopup_horizontal_get(const Eo *obj, Elm_Ctxpopup_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "horizontal" property getter.
 *
 * This function is utilized by the Eolian reflection system to get the
 * "horizontal" property. It calls the concrete implementation
 * @ref elm_obj_ctxpopup_horizontal_get and wraps the returned boolean
 * in an Eina_Value.
 *
 * @param obj The EFL object instance.
 * @return An Eina_Value containing the boolean state of the horizontal property.
 *         For example, an Eina_Value representing @c EINA_TRUE.
 */
static Eina_Value
__eolian_elm_ctxpopup_horizontal_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_ctxpopup_horizontal_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_ctxpopup_horizontal_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_auto_hide_disabled_set.
 * @param obj The object.
 * @param pd The private data.
 * @param disabled Auto hide enable/disable.
 */
void _elm_ctxpopup_auto_hide_disabled_set(Eo *obj, Elm_Ctxpopup_Data *pd, Eina_Bool disabled);

/**
 * @internal
 * @brief Reflection function for the "auto_hide_disabled" property setter.
 *
 * This function is utilized by the Eolian reflection system to set the
 * "auto_hide_disabled" property. It converts an Eina_Value to a boolean
 * and then invokes the concrete implementation @ref _elm_ctxpopup_auto_hide_disabled_set.
 *
 * @param obj The EFL object instance.
 * @param val An Eina_Value holding the boolean state for the auto_hide_disabled property.
 * @return EINA_ERROR_NO_ERROR on successful conversion and call,
 *         otherwise EINA_ERROR_VALUE_FAILED if conversion fails.
 */
static Eina_Error
__eolian_elm_ctxpopup_auto_hide_disabled_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_ctxpopup_auto_hide_disabled_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_ctxpopup_auto_hide_disabled_set, EFL_FUNC_CALL(disabled), Eina_Bool disabled);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_auto_hide_disabled_get.
 * @param obj The object.
 * @param pd The private data.
 * @return Auto hide enable/disable.
 */
Eina_Bool _elm_ctxpopup_auto_hide_disabled_get(const Eo *obj, Elm_Ctxpopup_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "auto_hide_disabled" property getter.
 *
 * This function is utilized by the Eolian reflection system to get the
 * "auto_hide_disabled" property. It calls the concrete implementation
 * @ref elm_obj_ctxpopup_auto_hide_disabled_get and wraps the returned boolean
 * in an Eina_Value.
 *
 * @param obj The EFL object instance.
 * @return An Eina_Value containing the boolean state of the auto_hide_disabled property.
 */
static Eina_Value
__eolian_elm_ctxpopup_auto_hide_disabled_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_ctxpopup_auto_hide_disabled_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_ctxpopup_auto_hide_disabled_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_hover_parent_set.
 * @param obj The object.
 * @param pd The private data.
 * @param parent The parent to use.
 */
void _elm_ctxpopup_hover_parent_set(Eo *obj, Elm_Ctxpopup_Data *pd, Efl_Canvas_Object *parent);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_ctxpopup_hover_parent_set, EFL_FUNC_CALL(parent), Efl_Canvas_Object *parent);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_hover_parent_get.
 * @param obj The object.
 * @param pd The private data.
 * @return The parent object.
 */
Efl_Canvas_Object *_elm_ctxpopup_hover_parent_get(const Eo *obj, Elm_Ctxpopup_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_ctxpopup_hover_parent_get, Efl_Canvas_Object *, NULL);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_direction_priority_set.
 * @param obj The object.
 * @param pd The private data.
 * @param first 1st priority of direction.
 * @param second 2nd priority of direction.
 * @param third 3rd priority of direction.
 * @param fourth 4th priority of direction.
 */
void _elm_ctxpopup_direction_priority_set(Eo *obj, Elm_Ctxpopup_Data *pd, Elm_Ctxpopup_Direction first, Elm_Ctxpopup_Direction second, Elm_Ctxpopup_Direction third, Elm_Ctxpopup_Direction fourth);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_ctxpopup_direction_priority_set, EFL_FUNC_CALL(first, second, third, fourth), Elm_Ctxpopup_Direction first, Elm_Ctxpopup_Direction second, Elm_Ctxpopup_Direction third, Elm_Ctxpopup_Direction fourth);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_direction_priority_get.
 * @param obj The object.
 * @param pd The private data.
 * @param[out] first 1st priority of direction.
 * @param[out] second 2nd priority of direction.
 * @param[out] third 3rd priority of direction.
 * @param[out] fourth 4th priority of direction.
 */
void _elm_ctxpopup_direction_priority_get(const Eo *obj, Elm_Ctxpopup_Data *pd, Elm_Ctxpopup_Direction *first, Elm_Ctxpopup_Direction *second, Elm_Ctxpopup_Direction *third, Elm_Ctxpopup_Direction *fourth);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_ctxpopup_direction_priority_get, EFL_FUNC_CALL(first, second, third, fourth), Elm_Ctxpopup_Direction *first, Elm_Ctxpopup_Direction *second, Elm_Ctxpopup_Direction *third, Elm_Ctxpopup_Direction *fourth);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_direction_get.
 * @param obj The object.
 * @param pd The private data.
 * @return Current direction of the ctxpopup.
 */
Elm_Ctxpopup_Direction _elm_ctxpopup_direction_get(const Eo *obj, Elm_Ctxpopup_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_ctxpopup_direction_get, Elm_Ctxpopup_Direction, 4 /* Elm.Ctxpopup.Direction.unknown */);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_dismiss.
 * @param obj The object.
 * @param pd The private data.
 */
void _elm_ctxpopup_dismiss(Eo *obj, Elm_Ctxpopup_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_ctxpopup_dismiss);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_clear.
 * @param obj The object.
 * @param pd The private data.
 */
void _elm_ctxpopup_clear(Eo *obj, Elm_Ctxpopup_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_ctxpopup_clear);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_item_insert_before.
 * @param obj The object.
 * @param pd The private data.
 * @param before The ctxpopup item to insert before.
 * @param label The Label of the new item.
 * @param icon Icon to be set on new item.
 * @param func Convenience function called when item selected.
 * @param data Data passed to @c func.
 * @return A handle to the item added or @c null, on errors.
 */
Elm_Widget_Item *_elm_ctxpopup_item_insert_before(Eo *obj, Elm_Ctxpopup_Data *pd, Elm_Widget_Item *before, const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_ctxpopup_item_insert_before, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(before, label, icon, func, data), Elm_Widget_Item *before, const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_item_insert_after.
 * @param obj The object.
 * @param pd The private data.
 * @param after The ctxpopup item to insert after.
 * @param label The Label of the new item.
 * @param icon Icon to be set on new item.
 * @param func Convenience function called when item selected.
 * @param data Data passed to @c func.
 * @return A handle to the item added or @c null, on errors.
 */
Elm_Widget_Item *_elm_ctxpopup_item_insert_after(Eo *obj, Elm_Ctxpopup_Data *pd, Elm_Widget_Item *after, const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_ctxpopup_item_insert_after, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(after, label, icon, func, data), Elm_Widget_Item *after, const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_item_append.
 * @param obj The object.
 * @param pd The private data.
 * @param label The Label of the new item.
 * @param icon Icon to be set on new item.
 * @param func Convenience function called when item selected.
 * @param data Data passed to @c func.
 * @return A handle to the item added or @c null, on errors.
 */
Elm_Widget_Item *_elm_ctxpopup_item_append(Eo *obj, Elm_Ctxpopup_Data *pd, const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_ctxpopup_item_append, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(label, icon, func, data), const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data);

/**
 * @internal
 * @brief Implements the EO API function @ref elm_obj_ctxpopup_item_prepend.
 * @param obj The object.
 * @param pd The private data.
 * @param label The Label of the new item.
 * @param icon Icon to be set on new item.
 * @param func Convenience function called when item selected.
 * @param data Data passed to @c func.
 * @return A handle to the item added or @c null, on errors.
 */
Elm_Widget_Item *_elm_ctxpopup_item_prepend(Eo *obj, Elm_Ctxpopup_Data *pd, const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_ctxpopup_item_prepend, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(label, icon, func, data), const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data);

/**
 * @internal
 * @brief Implements the Efl.Object.constructor interface.
 *
 * This function is called when a new Elm_Ctxpopup object is constructed.
 * It handles the initialization of the Ctxpopup specific data and properties.
 *
 * @param obj The EFL object (Elm_Ctxpopup instance) being constructed.
 * @param pd The private data structure for this Elm_Ctxpopup instance.
 * @return The constructed EFL object, or @c NULL on failure.
 */
Efl_Object *_elm_ctxpopup_efl_object_constructor(Eo *obj, Elm_Ctxpopup_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.widget_sub_object_add interface.
 *
 * Handles the addition of a sub-object to the Ctxpopup widget.
 *
 * @param obj The EFL object (Elm_Ctxpopup instance).
 * @param pd The private data structure for this Elm_Ctxpopup instance.
 * @param sub_obj The sub-object to add.
 * @return @c EINA_TRUE if the sub-object was successfully added, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_ctxpopup_efl_ui_widget_widget_sub_object_add(Eo *obj, Elm_Ctxpopup_Data *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Implements the Efl.Ui.L10n.translation_update interface.
 *
 * Called when the language is changed, to allow the Ctxpopup to update
 * any translatable strings it displays.
 *
 * @param obj The EFL object (Elm_Ctxpopup instance).
 * @param pd The private data structure for this Elm_Ctxpopup instance.
 */
void _elm_ctxpopup_efl_ui_l10n_translation_update(Eo *obj, Elm_Ctxpopup_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.theme_apply interface.
 *
 * Called when the theme is changed, to allow the Ctxpopup to re-apply its styling.
 *
 * @param obj The EFL object (Elm_Ctxpopup instance).
 * @param pd The private data structure for this Elm_Ctxpopup instance.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if applying the theme fails.
 */
Eina_Error _elm_ctxpopup_efl_ui_widget_theme_apply(Eo *obj, Elm_Ctxpopup_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.widget_input_event_handler interface.
 *
 * Handles input events (like mouse clicks, key presses) for the Ctxpopup widget.
 *
 * @param obj The EFL object (Elm_Ctxpopup instance).
 * @param pd The private data structure for this Elm_Ctxpopup instance.
 * @param eo_event The EFL event information.
 * @param source The source canvas object of the event.
 * @return @c EINA_TRUE if the event was handled, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_ctxpopup_efl_ui_widget_widget_input_event_handler(Eo *obj, Elm_Ctxpopup_Data *pd, const Efl_Event *eo_event, Efl_Canvas_Object *source);

/**
 * @internal
 * @brief Implements the Elm.Widget.Item.Container.focused_item_get interface.
 *
 * Retrieves the currently focused item within the Ctxpopup, if any.
 *
 * @param obj The EFL object (Elm_Ctxpopup instance).
 * @param pd The private data structure for this Elm_Ctxpopup instance.
 * @return The focused Elm_Widget_Item, or @c NULL if no item is focused.
 */
Elm_Widget_Item *_elm_ctxpopup_elm_widget_item_container_focused_item_get(const Eo *obj, Elm_Ctxpopup_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Access.Widget.Action.elm_actions_get interface.
 *
 * Retrieves a list of Elementary actions available for the Ctxpopup.
 * This is used for accessibility purposes.
 *
 * @param obj The EFL object (Elm_Ctxpopup instance).
 * @param pd The private data structure for this Elm_Ctxpopup instance.
 * @return A const pointer to an array of Efl_Access_Action_Data, terminated by an
 *         entry with a @c NULL name. Returns @c NULL if no actions are available.
 *         Example:
 *         @code
 *         static const Efl_Access_Action_Data actions[] = {
 *            { "activate", "activate" }, // { "action_name", "action_localized_name" }
 *            { NULL, NULL }
 *         };
 *         return actions;
 *         @endcode
 */
const Efl_Access_Action_Data *_elm_ctxpopup_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Ctxpopup_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Access.Object.state_set_get interface.
 *
 * Retrieves the accessibility state set for the Ctxpopup.
 *
 * @param obj The EFL object (Elm_Ctxpopup instance).
 * @param pd The private data structure for this Elm_Ctxpopup instance.
 * @return An Efl_Access_State_Set representing the current states of the widget.
 *         For example, if the widget is visible and sensitive:
 *         (EFL_ACCESS_STATE_TYPE_VISIBLE | EFL_ACCESS_STATE_TYPE_SENSITIVE)
 */
Efl_Access_State_Set _elm_ctxpopup_efl_access_object_state_set_get(const Eo *obj, Elm_Ctxpopup_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Part.part_get interface.
 *
 * Retrieves a specific part object from the Ctxpopup's internal layout.
 *
 * @param obj The EFL object (Elm_Ctxpopup instance).
 * @param pd The private data structure for this Elm_Ctxpopup instance.
 * @param name The name of the part to retrieve (e.g., "arrow", "background").
 * @return The Efl_Object representing the part, or @c NULL if the part is not found.
 */
Efl_Object *_elm_ctxpopup_efl_part_part_get(const Eo *obj, Elm_Ctxpopup_Data *pd, const char *name);

/**
 * @internal
 * @brief Initializes the Elm_Ctxpopup class.
 *
 * This function is called once when the Elm_Ctxpopup class is first loaded.
 * It sets up the Efl_Object operations, properties, and other class-level
 * configurations for Ctxpopup.
 *
 * @param klass The Efl_Class (Elm_Ctxpopup_Class) to initialize.
 * @return @c EINA_TRUE on successful initialization, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_ctxpopup_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_CTXPOPUP_EXTRA_OPS
#define ELM_CTXPOPUP_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_selected_item_get, _elm_ctxpopup_selected_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_first_item_get, _elm_ctxpopup_first_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_last_item_get, _elm_ctxpopup_last_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_items_get, _elm_ctxpopup_items_get),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_horizontal_set, _elm_ctxpopup_horizontal_set),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_horizontal_get, _elm_ctxpopup_horizontal_get),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_auto_hide_disabled_set, _elm_ctxpopup_auto_hide_disabled_set),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_auto_hide_disabled_get, _elm_ctxpopup_auto_hide_disabled_get),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_hover_parent_set, _elm_ctxpopup_hover_parent_set),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_hover_parent_get, _elm_ctxpopup_hover_parent_get),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_direction_priority_set, _elm_ctxpopup_direction_priority_set),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_direction_priority_get, _elm_ctxpopup_direction_priority_get),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_direction_get, _elm_ctxpopup_direction_get),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_dismiss, _elm_ctxpopup_dismiss),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_clear, _elm_ctxpopup_clear),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_item_insert_before, _elm_ctxpopup_item_insert_before),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_item_insert_after, _elm_ctxpopup_item_insert_after),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_item_append, _elm_ctxpopup_item_append),
      EFL_OBJECT_OP_FUNC(elm_obj_ctxpopup_item_prepend, _elm_ctxpopup_item_prepend),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_ctxpopup_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_sub_object_add, _elm_ctxpopup_efl_ui_widget_widget_sub_object_add),
      EFL_OBJECT_OP_FUNC(efl_ui_l10n_translation_update, _elm_ctxpopup_efl_ui_l10n_translation_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_ctxpopup_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_ctxpopup_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(elm_widget_item_container_focused_item_get, _elm_ctxpopup_elm_widget_item_container_focused_item_get),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_ctxpopup_efl_access_widget_action_elm_actions_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_state_set_get, _elm_ctxpopup_efl_access_object_state_set_get),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_ctxpopup_efl_part_part_get),
      ELM_CTXPOPUP_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"horizontal", __eolian_elm_ctxpopup_horizontal_set_reflect, __eolian_elm_ctxpopup_horizontal_get_reflect},
      {"auto_hide_disabled", __eolian_elm_ctxpopup_auto_hide_disabled_set_reflect, __eolian_elm_ctxpopup_auto_hide_disabled_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_ctxpopup_class_desc = {
   EO_VERSION,
   "Elm.Ctxpopup",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Ctxpopup_Data),
   _elm_ctxpopup_class_initializer,
   _elm_ctxpopup_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_ctxpopup_class_get, &_elm_ctxpopup_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_UI_FOCUS_LAYER_MIXIN, EFL_ACCESS_WIDGET_ACTION_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, ELM_WIDGET_ITEM_CONTAINER_INTERFACE, NULL);

#include "elm_ctxpopup_eo.legacy.c"
