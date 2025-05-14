/**
 * @brief Event descriptor for the "done" event.
 * @details This event is triggered when the user confirms a selection (e.g., clicks "OK").
 * The event data typically contains the selected path(s).
 */
EWAPI const Efl_Event_Description _ELM_FILESELECTOR_EVENT_DONE =
   EFL_EVENT_DESCRIPTION("done");
/**
 * @brief Event descriptor for the "activated" event.
 * @details This event is triggered when an item in the fileselector is activated (e.g., double-clicked).
 * The event data typically contains the path of the activated item.
 */
EWAPI const Efl_Event_Description _ELM_FILESELECTOR_EVENT_ACTIVATED =
   EFL_EVENT_DESCRIPTION("activated");
/**
 * @brief Event descriptor for the "selected,invalid" event.
 * @details This event is triggered when an invalid selection is made.
 */
EWAPI const Efl_Event_Description _ELM_FILESELECTOR_EVENT_SELECTED_INVALID =
   EFL_EVENT_DESCRIPTION("selected,invalid");
/**
 * @brief Event descriptor for the "directory,open" event.
 * @details This event is triggered when a directory is opened/navigated into.
 * The event data typically contains the path of the directory.
 */
EWAPI const Efl_Event_Description _ELM_FILESELECTOR_EVENT_DIRECTORY_OPEN =
   EFL_EVENT_DESCRIPTION("directory,open");

/**
 * @brief Internal implementation for elm_obj_fileselector_buttons_ok_cancel_set.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param visible EINA_TRUE to show buttons, EINA_FALSE to hide.
 */
void _elm_fileselector_buttons_ok_cancel_set(Eo *obj, Elm_Fileselector_Data *pd, Eina_Bool visible);


/**
 * @brief Eolian reflection function for setting the 'buttons_ok_cancel' property.
 * @details This function is invoked by the Eolian reflection system to set the property value.
 * It converts an Eina_Value (expected to be a boolean) and calls the C implementation
 * _elm_fileselector_buttons_ok_cancel_set (via elm_obj_fileselector_buttons_ok_cancel_set).
 * @param obj The EFL object.
 * @param val The Eina_Value containing the boolean value to set.
 * @return EINA_ERROR_NO_ERROR on success, or an Eina_Error code if conversion fails.
 */
static Eina_Error
__eolian_elm_fileselector_buttons_ok_cancel_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_fileselector_buttons_ok_cancel_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_fileselector_buttons_ok_cancel_set, EFL_FUNC_CALL(visible), Eina_Bool visible);

/**
 * @brief Internal implementation for elm_obj_fileselector_buttons_ok_cancel_get.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return EINA_TRUE if buttons are visible, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_buttons_ok_cancel_get(const Eo *obj, Elm_Fileselector_Data *pd);

/**
 * @brief Eolian reflection function for getting the 'buttons_ok_cancel' property.
 * @details This function is invoked by the Eolian reflection system to get the property value.
 * It calls the C implementation _elm_fileselector_buttons_ok_cancel_get (via
 * elm_obj_fileselector_buttons_ok_cancel_get) and wraps the result in an Eina_Value.
 * @param obj The EFL object.
 * @return An Eina_Value containing the boolean state of button visibility.
 */
static Eina_Value
__eolian_elm_fileselector_buttons_ok_cancel_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_fileselector_buttons_ok_cancel_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_fileselector_buttons_ok_cancel_get, Eina_Bool, 0);

/**
 * @brief Implements the Efl_Object constructor for Elm_Fileselector.
 * @param obj The Evas object being constructed.
 * @param pd The private data for the fileselector.
 * @return The constructed Efl_Object.
 */
Efl_Object *_elm_fileselector_efl_object_constructor(Eo *obj, Elm_Fileselector_Data *pd);

/**
 * @brief Implements the Efl_Ui_Widget input event handler for Elm_Fileselector.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param eo_event The Efl_Event details.
 * @param source The source canvas object of the event.
 * @return EINA_TRUE if the event was handled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_efl_ui_widget_widget_input_event_handler(Eo *obj, Elm_Fileselector_Data *pd, const Efl_Event *eo_event, Efl_Canvas_Object *source);

/**
 * @brief Implements the Efl_Ui_Widget theme apply function for Elm_Fileselector.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return EINA_ERROR_NO_ERROR on success, or an error code.
 */
Eina_Error _elm_fileselector_efl_ui_widget_theme_apply(Eo *obj, Elm_Fileselector_Data *pd);

/**
 * @brief Implements elm_interface_fileselector_selected_models_get.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return A list of selected Efl_Io_Model instances.
 */
const Eina_List *_elm_fileselector_elm_interface_fileselector_selected_models_get(const Eo *obj, Elm_Fileselector_Data *pd);

/**
 * @brief Implements elm_interface_fileselector_selected_model_set.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param model The Efl_Io_Model to set as selected.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_elm_interface_fileselector_selected_model_set(Eo *obj, Elm_Fileselector_Data *pd, Efl_Io_Model *model);

/**
 * @brief Implements elm_interface_fileselector_selected_model_get.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return The currently selected Efl_Io_Model.
 */
Efl_Io_Model *_elm_fileselector_elm_interface_fileselector_selected_model_get(const Eo *obj, Elm_Fileselector_Data *pd);

/**
 * @brief Implements elm_interface_fileselector_custom_filter_append.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param func The custom filter function.
 * @param data User data for the filter function.
 * @param filter_name The name of the filter.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_elm_interface_fileselector_custom_filter_append(Eo *obj, Elm_Fileselector_Data *pd, Elm_Fileselector_Filter_Func func, void *data, const char *filter_name);

/**
 * @brief Implements elm_interface_fileselector_expandable_set.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param expand EINA_TRUE to make the fileselector expandable, EINA_FALSE otherwise.
 */
void _elm_fileselector_elm_interface_fileselector_expandable_set(Eo *obj, Elm_Fileselector_Data *pd, Eina_Bool expand);

/**
 * @brief Implements elm_interface_fileselector_expandable_get.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return EINA_TRUE if the fileselector is expandable, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_elm_interface_fileselector_expandable_get(const Eo *obj, Elm_Fileselector_Data *pd);


/**
 * @brief Implements elm_interface_fileselector_thumbnail_size_set.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param w The width of the thumbnail.
 * @param h The height of the thumbnail.
 */
void _elm_fileselector_elm_interface_fileselector_thumbnail_size_set(Eo *obj, Elm_Fileselector_Data *pd, int w, int h);

/**
 * @brief Implements elm_interface_fileselector_thumbnail_size_get.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param w Pointer to store the width of the thumbnail.
 * @param h Pointer to store the height of the thumbnail.
 */
void _elm_fileselector_elm_interface_fileselector_thumbnail_size_get(const Eo *obj, Elm_Fileselector_Data *pd, int *w, int *h);

/**
 * @brief Implements elm_interface_fileselector_mime_types_filter_append.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param mime_types A comma-separated list of MIME types.
 * @param filter_name The name for this MIME type filter.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_elm_interface_fileselector_mime_types_filter_append(Eo *obj, Elm_Fileselector_Data *pd, const char *mime_types, const char *filter_name);

/**
 * @brief Implements elm_interface_fileselector_hidden_visible_set.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param hidden EINA_TRUE to show hidden files, EINA_FALSE to hide them.
 */
void _elm_fileselector_elm_interface_fileselector_hidden_visible_set(Eo *obj, Elm_Fileselector_Data *pd, Eina_Bool hidden);

/**
 * @brief Implements elm_interface_fileselector_hidden_visible_get.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return EINA_TRUE if hidden files are visible, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_elm_interface_fileselector_hidden_visible_get(const Eo *obj, Elm_Fileselector_Data *pd);

/**
 * @brief Implements elm_interface_fileselector_filters_clear.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 */
void _elm_fileselector_elm_interface_fileselector_filters_clear(Eo *obj, Elm_Fileselector_Data *pd);

/**
 * @brief Implements elm_interface_fileselector_is_save_set.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param is_save EINA_TRUE if the fileselector is for saving, EINA_FALSE for opening.
 */
void _elm_fileselector_elm_interface_fileselector_is_save_set(Eo *obj, Elm_Fileselector_Data *pd, Eina_Bool is_save);

/**
 * @brief Implements elm_interface_fileselector_is_save_get.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return EINA_TRUE if the fileselector is for saving, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_elm_interface_fileselector_is_save_get(const Eo *obj, Elm_Fileselector_Data *pd);


/**
 * @brief Implements efl_ui_view_model_set for Elm_Fileselector.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param model The Efl_Model to set for the view.
 */
void _elm_fileselector_efl_ui_view_model_set(Eo *obj, Elm_Fileselector_Data *pd, Efl_Model *model);

/**
 * @brief Implements efl_ui_view_model_get for Elm_Fileselector.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return The Efl_Model used by the view.
 */
Efl_Model *_elm_fileselector_efl_ui_view_model_get(const Eo *obj, Elm_Fileselector_Data *pd);

/**
 * @brief Implements elm_interface_fileselector_sort_method_set.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param sort The sorting method to apply.
 */
void _elm_fileselector_elm_interface_fileselector_sort_method_set(Eo *obj, Elm_Fileselector_Data *pd, Elm_Fileselector_Sort sort);

/**
 * @brief Implements elm_interface_fileselector_sort_method_get.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return The current sorting method.
 */
Elm_Fileselector_Sort _elm_fileselector_elm_interface_fileselector_sort_method_get(const Eo *obj, Elm_Fileselector_Data *pd);

/**
 * @brief Implements elm_interface_fileselector_multi_select_set.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param multi EINA_TRUE to enable multi-selection, EINA_FALSE to disable.
 */
void _elm_fileselector_elm_interface_fileselector_multi_select_set(Eo *obj, Elm_Fileselector_Data *pd, Eina_Bool multi);

/**
 * @brief Implements elm_interface_fileselector_multi_select_get.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return EINA_TRUE if multi-selection is enabled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_elm_interface_fileselector_multi_select_get(const Eo *obj, Elm_Fileselector_Data *pd);

/**
 * @brief Implements elm_interface_fileselector_folder_only_set.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param only EINA_TRUE to allow selecting folders only, EINA_FALSE otherwise.
 */
void _elm_fileselector_elm_interface_fileselector_folder_only_set(Eo *obj, Elm_Fileselector_Data *pd, Eina_Bool only);

/**
 * @brief Implements elm_interface_fileselector_folder_only_get.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return EINA_TRUE if only folders can be selected, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_elm_interface_fileselector_folder_only_get(const Eo *obj, Elm_Fileselector_Data *pd);


/**
 * @brief Implements elm_interface_fileselector_mode_set.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param mode The display mode for the fileselector (e.g., list or grid).
 */
void _elm_fileselector_elm_interface_fileselector_mode_set(Eo *obj, Elm_Fileselector_Data *pd, Elm_Fileselector_Mode mode);

/**
 * @brief Implements elm_interface_fileselector_mode_get.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return The current display mode.
 */
Elm_Fileselector_Mode _elm_fileselector_elm_interface_fileselector_mode_get(const Eo *obj, Elm_Fileselector_Data *pd);

/**
 * @brief Implements elm_interface_fileselector_current_name_set.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param name The current name to set (e.g., for saving a file).
 */
void _elm_fileselector_elm_interface_fileselector_current_name_set(Eo *obj, Elm_Fileselector_Data *pd, const char *name);

/**
 * @brief Implements elm_interface_fileselector_current_name_get.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return The current name.
 */
const char *_elm_fileselector_elm_interface_fileselector_current_name_get(const Eo *obj, Elm_Fileselector_Data *pd);

/**
 * @brief Implements efl_access_widget_action_elm_actions_get for Elm_Fileselector.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @return A list of Efl_Access_Action_Data.
 */
const Efl_Access_Action_Data *_elm_fileselector_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Fileselector_Data *pd);

/**
 * @brief Implements efl_part_get for Elm_Fileselector.
 * @param obj The Evas object.
 * @param pd The private data for the fileselector.
 * @param name The name of the part to get.
 * @return The Efl_Object representing the part, or NULL if not found.
 */
Efl_Object *_elm_fileselector_efl_part_part_get(const Eo *obj, Elm_Fileselector_Data *pd, const char *name);

/**
 * @brief Initializes the Elm_Fileselector Efl_Class.
 * @details This function is called once when the class is first used.
 * It sets up the Efl_Object_Ops and Efl_Object_Property_Reflection_Ops
 * for the class, mapping Eolian functions and properties to their C implementations.
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_fileselector_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_FILESELECTOR_EXTRA_OPS
#define ELM_FILESELECTOR_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_fileselector_buttons_ok_cancel_set, _elm_fileselector_buttons_ok_cancel_set),
      EFL_OBJECT_OP_FUNC(elm_obj_fileselector_buttons_ok_cancel_get, _elm_fileselector_buttons_ok_cancel_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_fileselector_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_fileselector_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_fileselector_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_selected_models_get, _elm_fileselector_elm_interface_fileselector_selected_models_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_selected_model_set, _elm_fileselector_elm_interface_fileselector_selected_model_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_selected_model_get, _elm_fileselector_elm_interface_fileselector_selected_model_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_custom_filter_append, _elm_fileselector_elm_interface_fileselector_custom_filter_append),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_expandable_set, _elm_fileselector_elm_interface_fileselector_expandable_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_expandable_get, _elm_fileselector_elm_interface_fileselector_expandable_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_thumbnail_size_set, _elm_fileselector_elm_interface_fileselector_thumbnail_size_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_thumbnail_size_get, _elm_fileselector_elm_interface_fileselector_thumbnail_size_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_mime_types_filter_append, _elm_fileselector_elm_interface_fileselector_mime_types_filter_append),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_hidden_visible_set, _elm_fileselector_elm_interface_fileselector_hidden_visible_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_hidden_visible_get, _elm_fileselector_elm_interface_fileselector_hidden_visible_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_filters_clear, _elm_fileselector_elm_interface_fileselector_filters_clear),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_is_save_set, _elm_fileselector_elm_interface_fileselector_is_save_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_is_save_get, _elm_fileselector_elm_interface_fileselector_is_save_get),
      EFL_OBJECT_OP_FUNC(efl_ui_view_model_set, _elm_fileselector_efl_ui_view_model_set),
      EFL_OBJECT_OP_FUNC(efl_ui_view_model_get, _elm_fileselector_efl_ui_view_model_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_sort_method_set, _elm_fileselector_elm_interface_fileselector_sort_method_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_sort_method_get, _elm_fileselector_elm_interface_fileselector_sort_method_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_multi_select_set, _elm_fileselector_elm_interface_fileselector_multi_select_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_multi_select_get, _elm_fileselector_elm_interface_fileselector_multi_select_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_folder_only_set, _elm_fileselector_elm_interface_fileselector_folder_only_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_folder_only_get, _elm_fileselector_elm_interface_fileselector_folder_only_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_mode_set, _elm_fileselector_elm_interface_fileselector_mode_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_mode_get, _elm_fileselector_elm_interface_fileselector_mode_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_current_name_set, _elm_fileselector_elm_interface_fileselector_current_name_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_current_name_get, _elm_fileselector_elm_interface_fileselector_current_name_get),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_fileselector_efl_access_widget_action_elm_actions_get),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_fileselector_efl_part_part_get),
      ELM_FILESELECTOR_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"buttons_ok_cancel", __eolian_elm_fileselector_buttons_ok_cancel_set_reflect, __eolian_elm_fileselector_buttons_ok_cancel_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief Efl_Class_Description for the Elm_Fileselector class.
 * @details This structure provides metadata about the Elm_Fileselector class,
 * including its name, version, type, size of instance data, and pointers
 * to initializer, constructor, and destructor functions.
 */
static const Efl_Class_Description _elm_fileselector_class_desc = {
   EO_VERSION,
   "Elm.Fileselector",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Fileselector_Data),
   _elm_fileselector_class_initializer,
   _elm_fileselector_class_constructor,
   _elm_fileselector_class_destructor
};

EFL_DEFINE_CLASS(elm_fileselector_class_get, &_elm_fileselector_class_desc, EFL_UI_LAYOUT_BASE_CLASS, ELM_INTERFACE_FILESELECTOR_INTERFACE, EFL_ACCESS_WIDGET_ACTION_MIXIN, EFL_UI_FOCUS_COMPOSITION_MIXIN, EFL_INPUT_CLICKABLE_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_fileselector_eo.legacy.c"
