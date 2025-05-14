/**
 * @brief Event descriptor for the "changed" event.
 * @details This event is emitted when the selected segment control item changes.
 */
EWAPI const Efl_Event_Description _ELM_SEGMENT_CONTROL_EVENT_CHANGED =
   EFL_EVENT_DESCRIPTION("changed");

/**
 * @brief Internal implementation for elm_obj_segment_control_item_count_get().
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for Elm_Segment_Control.
 * @return The number of items in the segment control.
 */
int _elm_segment_control_item_count_get(const Eo *obj, Elm_Segment_Control_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_segment_control_item_count_get, int, 0);

/**
 * @brief Internal implementation for elm_obj_segment_control_item_selected_get().
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for Elm_Segment_Control.
 * @return The currently selected Elm_Widget_Item, or NULL if none is selected.
 */
Elm_Widget_Item *_elm_segment_control_item_selected_get(const Eo *obj, Elm_Segment_Control_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_segment_control_item_selected_get, Elm_Widget_Item *, NULL);

/**
 * @brief Internal implementation for elm_obj_segment_control_item_label_get().
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for Elm_Segment_Control.
 * @param[in] idx The index of the item.
 * @return The label of the item at the given index, or NULL on failure.
 */
const char *_elm_segment_control_item_label_get(const Eo *obj, Elm_Segment_Control_Data *pd, int idx);

EOAPI EFL_FUNC_BODYV_CONST(elm_obj_segment_control_item_label_get, const char *, NULL, EFL_FUNC_CALL(idx), int idx);

/**
 * @brief Internal implementation for elm_obj_segment_control_item_insert_at().
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for Elm_Segment_Control.
 * @param[in] icon Optional icon for the item.
 * @param[in] label Optional label for the item.
 * @param[in] idx The index at which to insert the item.
 * @return The newly created Elm_Widget_Item, or NULL on failure.
 */
Elm_Widget_Item *_elm_segment_control_item_insert_at(Eo *obj, Elm_Segment_Control_Data *pd, Efl_Canvas_Object *icon, const char *label, int idx);

EOAPI EFL_FUNC_BODYV(elm_obj_segment_control_item_insert_at, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(icon, label, idx), Efl_Canvas_Object *icon, const char *label, int idx);

/**
 * @brief Internal implementation for elm_obj_segment_control_item_get().
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for Elm_Segment_Control.
 * @param[in] idx The index of the item to retrieve.
 * @return The Elm_Widget_Item at the given index, or NULL if the index is invalid.
 */
Elm_Widget_Item *_elm_segment_control_item_get(const Eo *obj, Elm_Segment_Control_Data *pd, int idx);

EOAPI EFL_FUNC_BODYV_CONST(elm_obj_segment_control_item_get, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(idx), int idx);

/**
 * @brief Internal implementation for elm_obj_segment_control_item_del_at().
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for Elm_Segment_Control.
 * @param[in] idx The index of the item to delete.
 */
void _elm_segment_control_item_del_at(Eo *obj, Elm_Segment_Control_Data *pd, int idx);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_segment_control_item_del_at, EFL_FUNC_CALL(idx), int idx);

/**
 * @brief Internal implementation for elm_obj_segment_control_item_add().
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for Elm_Segment_Control.
 * @param[in] icon Optional icon for the item.
 * @param[in] label Optional label for the item.
 * @return The newly created Elm_Widget_Item, or NULL on failure.
 */
Elm_Widget_Item *_elm_segment_control_item_add(Eo *obj, Elm_Segment_Control_Data *pd, Efl_Canvas_Object *icon, const char *label);

EOAPI EFL_FUNC_BODYV(elm_obj_segment_control_item_add, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(icon, label), Efl_Canvas_Object *icon, const char *label);

/**
 * @brief Internal implementation for elm_obj_segment_control_item_icon_get().
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for Elm_Segment_Control.
 * @param[in] idx The index of the item.
 * @return The icon of the item at the given index, or NULL if no icon is set or index is invalid.
 */
Efl_Canvas_Object *_elm_segment_control_item_icon_get(const Eo *obj, Elm_Segment_Control_Data *pd, int idx);

EOAPI EFL_FUNC_BODYV_CONST(elm_obj_segment_control_item_icon_get, Efl_Canvas_Object *, NULL, EFL_FUNC_CALL(idx), int idx);

/**
 * @brief Internal implementation for efl_object_constructor().
 * @param[in] obj The Efl_Object instance being constructed.
 * @param[in] pd The private data for Elm_Segment_Control.
 * @return The constructed Efl_Object instance.
 */
Efl_Object *_elm_segment_control_efl_object_constructor(Eo *obj, Elm_Segment_Control_Data *pd);

/**
 * @brief Internal implementation for efl_ui_widget_theme_apply().
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for Elm_Segment_Control.
 * @return EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Error _elm_segment_control_efl_ui_widget_theme_apply(Eo *obj, Elm_Segment_Control_Data *pd);

/**
 * @brief Internal implementation for efl_ui_widget_on_access_update().
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for Elm_Segment_Control.
 * @param[in] enable EINA_TRUE if accessibility is enabled, EINA_FALSE otherwise.
 */
void _elm_segment_control_efl_ui_widget_on_access_update(Eo *obj, Elm_Segment_Control_Data *pd, Eina_Bool enable);

/**
 * @brief Internal implementation for efl_ui_widget_disabled_set().
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for Elm_Segment_Control.
 * @param[in] disabled EINA_TRUE to disable the widget, EINA_FALSE to enable.
 */
void _elm_segment_control_efl_ui_widget_disabled_set(Eo *obj, Elm_Segment_Control_Data *pd, Eina_Bool disabled);

/**
 * @brief Internal implementation for efl_ui_l10n_translation_update().
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for Elm_Segment_Control.
 */
void _elm_segment_control_efl_ui_l10n_translation_update(Eo *obj, Elm_Segment_Control_Data *pd);

/**
 * @brief Initializes the Elm_Segment_Control class.
 * @details This function sets up the operations (methods) for the Elm_Segment_Control class.
 * @param[in] klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_segment_control_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_SEGMENT_CONTROL_EXTRA_OPS
#define ELM_SEGMENT_CONTROL_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_segment_control_item_count_get, _elm_segment_control_item_count_get),
      EFL_OBJECT_OP_FUNC(elm_obj_segment_control_item_selected_get, _elm_segment_control_item_selected_get),
      EFL_OBJECT_OP_FUNC(elm_obj_segment_control_item_label_get, _elm_segment_control_item_label_get),
      EFL_OBJECT_OP_FUNC(elm_obj_segment_control_item_insert_at, _elm_segment_control_item_insert_at),
      EFL_OBJECT_OP_FUNC(elm_obj_segment_control_item_get, _elm_segment_control_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_segment_control_item_del_at, _elm_segment_control_item_del_at),
      EFL_OBJECT_OP_FUNC(elm_obj_segment_control_item_add, _elm_segment_control_item_add),
      EFL_OBJECT_OP_FUNC(elm_obj_segment_control_item_icon_get, _elm_segment_control_item_icon_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_segment_control_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_segment_control_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_on_access_update, _elm_segment_control_efl_ui_widget_on_access_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_disabled_set, _elm_segment_control_efl_ui_widget_disabled_set),
      EFL_OBJECT_OP_FUNC(efl_ui_l10n_translation_update, _elm_segment_control_efl_ui_l10n_translation_update),
      ELM_SEGMENT_CONTROL_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief Describes the Elm_Segment_Control Efl_Class.
 * @details This structure provides metadata for the Elm_Segment_Control class,
 * including its version, name, type, data size, and initializer/constructor functions.
 */
static const Efl_Class_Description _elm_segment_control_class_desc = {
   EO_VERSION,
   "Elm.Segment_Control",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Segment_Control_Data),
   _elm_segment_control_class_initializer,
   _elm_segment_control_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_segment_control_class_get, &_elm_segment_control_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_UI_FOCUS_COMPOSITION_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_segment_control_eo.legacy.c"
