/**
 * @brief Definition of the "file,chosen" event.
 * @ingroup Elm_Fileselector_Button
 */
EWAPI const Efl_Event_Description _ELM_FILESELECTOR_BUTTON_EVENT_FILE_CHOSEN =
   EFL_EVENT_DESCRIPTION("file,chosen");

/**
 * @brief Constructor for the Elm_Fileselector_Button object.
 * @internal
 *
 * Implements the Efl.Object.constructor method.
 *
 * @param obj The Efl_Object instance to construct.
 * @param pd Pointer to the private data structure for this object.
 * @return The constructed Efl_Object instance.
 */
Efl_Object *_elm_fileselector_button_efl_object_constructor(Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Applies the theme to the Elm_Fileselector_Button widget.
 * @internal
 *
 * Implements the Efl.Ui.Widget.theme_apply method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @return EINA_ERROR_NONE on success, or an Eina_Error code on failure.
 */
Eina_Error _elm_fileselector_button_efl_ui_widget_theme_apply(Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Gets whether autorepeat is supported by the Elm_Fileselector_Button.
 * @internal
 *
 * Implements the Efl.Ui.Autorepeat.autorepeat_supported_get method.
 * Note: Fileselector buttons typically do not support autorepeat.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @return EINA_FALSE as autorepeat is generally not supported.
 */
Eina_Bool _elm_fileselector_button_efl_ui_autorepeat_autorepeat_supported_get(const Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Gets the list of selected Efl_Io_Model instances.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.selected_models_get method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @return A const Eina_List of Efl_Io_Model pointers, or NULL if none selected or on error.
 *         The list and its contents are owned by the object and should not be modified or freed.
 */
const Eina_List *_elm_fileselector_button_elm_interface_fileselector_selected_models_get(const Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Sets whether the fileselector view is expandable.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.expandable_set method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param expand EINA_TRUE to make it expandable, EINA_FALSE otherwise.
 */
void _elm_fileselector_button_elm_interface_fileselector_expandable_set(Eo *obj, Elm_Fileselector_Button_Data *pd, Eina_Bool expand);

/**
 * @brief Gets whether the fileselector view is expandable.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.expandable_get method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @return EINA_TRUE if expandable, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_button_elm_interface_fileselector_expandable_get(const Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Sets the size for file thumbnails in the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.thumbnail_size_set method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param w The width of the thumbnail.
 * @param h The height of the thumbnail.
 */
void _elm_fileselector_button_elm_interface_fileselector_thumbnail_size_set(Eo *obj, Elm_Fileselector_Button_Data *pd, int w, int h);

/**
 * @brief Gets the size for file thumbnails in the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.thumbnail_size_get method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param[out] w Pointer to store the width of the thumbnail.
 * @param[out] h Pointer to store the height of the thumbnail.
 */
void _elm_fileselector_button_elm_interface_fileselector_thumbnail_size_get(const Eo *obj, Elm_Fileselector_Button_Data *pd, int *w, int *h);

/**
 * @brief Sets the currently selected item in the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.selected_model_set method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param model The Efl_Io_Model representing the item to select.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool _elm_fileselector_button_elm_interface_fileselector_selected_model_set(Eo *obj, Elm_Fileselector_Button_Data *pd, Efl_Io_Model *model);

/**
 * @brief Gets the currently selected Efl_Io_Model in the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.selected_model_get method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @return The selected Efl_Io_Model, or NULL if nothing is selected or on error.
 */
Efl_Io_Model *_elm_fileselector_button_elm_interface_fileselector_selected_model_get(const Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Sets the visibility of hidden files in the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.hidden_visible_set method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param hidden EINA_TRUE to show hidden files, EINA_FALSE to hide them.
 */
void _elm_fileselector_button_elm_interface_fileselector_hidden_visible_set(Eo *obj, Elm_Fileselector_Button_Data *pd, Eina_Bool hidden);

/**
 * @brief Gets the visibility state of hidden files in the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.hidden_visible_get method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @return EINA_TRUE if hidden files are visible, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_button_elm_interface_fileselector_hidden_visible_get(const Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Sets whether the fileselector is in "save" mode.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.is_save_set method.
 * In "save" mode, the fileselector typically allows entering a new filename.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param is_save EINA_TRUE for "save" mode, EINA_FALSE for "open" mode.
 */
void _elm_fileselector_button_elm_interface_fileselector_is_save_set(Eo *obj, Elm_Fileselector_Button_Data *pd, Eina_Bool is_save);

/**
 * @brief Gets whether the fileselector is in "save" mode.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.is_save_get method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @return EINA_TRUE if in "save" mode, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_button_elm_interface_fileselector_is_save_get(const Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Sets the Efl_Model for the fileselector view.
 * @internal
 *
 * Implements the Efl.Ui.View.model_set method.
 * This model represents the directory/content being browsed.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param model The Efl_Model to use for the view.
 */
void _elm_fileselector_button_efl_ui_view_model_set(Eo *obj, Elm_Fileselector_Button_Data *pd, Efl_Model *model);

/**
 * @brief Gets the Efl_Model used by the fileselector view.
 * @internal
 *
 * Implements the Efl.Ui.View.model_get method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @return The Efl_Model used by the view.
 */
Efl_Model *_elm_fileselector_button_efl_ui_view_model_get(const Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Sets the sorting method for files and directories in the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.sort_method_set method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param sort The Elm_Fileselector_Sort method to apply (e.g., by name, by type).
 */
void _elm_fileselector_button_elm_interface_fileselector_sort_method_set(Eo *obj, Elm_Fileselector_Button_Data *pd, Elm_Fileselector_Sort sort);

/**
 * @brief Gets the current sorting method used in the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.sort_method_get method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @return The current Elm_Fileselector_Sort method.
 */
Elm_Fileselector_Sort _elm_fileselector_button_elm_interface_fileselector_sort_method_get(const Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Sets whether multi-selection is enabled in the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.multi_select_set method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param multi EINA_TRUE to enable multi-selection, EINA_FALSE to disable.
 */
void _elm_fileselector_button_elm_interface_fileselector_multi_select_set(Eo *obj, Elm_Fileselector_Button_Data *pd, Eina_Bool multi);

/**
 * @brief Gets whether multi-selection is enabled in the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.multi_select_get method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @return EINA_TRUE if multi-selection is enabled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_button_elm_interface_fileselector_multi_select_get(const Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Sets whether the fileselector should only display folders.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.folder_only_set method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param only EINA_TRUE to display only folders, EINA_FALSE to display files and folders.
 */
void _elm_fileselector_button_elm_interface_fileselector_folder_only_set(Eo *obj, Elm_Fileselector_Button_Data *pd, Eina_Bool only);

/**
 * @brief Gets whether the fileselector is set to display only folders.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.folder_only_get method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @return EINA_TRUE if only folders are displayed, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_button_elm_interface_fileselector_folder_only_get(const Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Sets the display mode of the fileselector (e.g., list, grid).
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.mode_set method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param mode The Elm_Fileselector_Mode to set.
 */
void _elm_fileselector_button_elm_interface_fileselector_mode_set(Eo *obj, Elm_Fileselector_Button_Data *pd, Elm_Fileselector_Mode mode);

/**
 * @brief Gets the current display mode of the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.mode_get method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @return The current Elm_Fileselector_Mode.
 */
Elm_Fileselector_Mode _elm_fileselector_button_elm_interface_fileselector_mode_get(const Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Sets the current name in the fileselector (e.g., for saving a file).
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.current_name_set method.
 * This is typically used in "save" mode to pre-fill the filename input.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param name The name to set.
 */
void _elm_fileselector_button_elm_interface_fileselector_current_name_set(Eo *obj, Elm_Fileselector_Button_Data *pd, const char *name);

/**
 * @brief Gets the current name set in the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.current_name_get method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @return The current name, or NULL if not set or on error. The returned string is an internal copy.
 */
const char *_elm_fileselector_button_elm_interface_fileselector_current_name_get(const Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Appends a custom filter function to the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.custom_filter_append method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param func The custom filter function.
 * @param data User data to be passed to the filter function.
 * @param filter_name The name for this filter (e.g., "Image Files").
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool _elm_fileselector_button_elm_interface_fileselector_custom_filter_append(Eo *obj, Elm_Fileselector_Button_Data *pd, Elm_Fileselector_Filter_Func func, void *data, const char *filter_name);

/**
 * @brief Appends a MIME types based filter to the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.mime_types_filter_append method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 * @param mime_types A comma-separated list of MIME types (e.g., "image/png,image/jpeg").
 * @param filter_name The name for this filter (e.g., "Image Files").
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool _elm_fileselector_button_elm_interface_fileselector_mime_types_filter_append(Eo *obj, Elm_Fileselector_Button_Data *pd, const char *mime_types, const char *filter_name);

/**
 * @brief Clears all currently set filters in the fileselector.
 * @internal
 *
 * Implements the Elm.Interface.Fileselector.filters_clear method.
 *
 * @param obj The Efl_Object instance.
 * @param pd Pointer to the private data structure for this object.
 */
void _elm_fileselector_button_elm_interface_fileselector_filters_clear(Eo *obj, Elm_Fileselector_Button_Data *pd);

/**
 * @brief Initializes the Elm_Fileselector_Button class.
 * @internal
 *
 * This function is called once when the class is being set up.
 * It defines the Efl_Object operations for this class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_fileselector_button_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_FILESELECTOR_BUTTON_EXTRA_OPS
#define ELM_FILESELECTOR_BUTTON_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_fileselector_button_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_fileselector_button_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_autorepeat_enabled_set, _elm_fileselector_button_efl_ui_autorepeat_autorepeat_enabled_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_selected_models_get, _elm_fileselector_button_elm_interface_fileselector_selected_models_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_expandable_set, _elm_fileselector_button_elm_interface_fileselector_expandable_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_expandable_get, _elm_fileselector_button_elm_interface_fileselector_expandable_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_thumbnail_size_set, _elm_fileselector_button_elm_interface_fileselector_thumbnail_size_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_thumbnail_size_get, _elm_fileselector_button_elm_interface_fileselector_thumbnail_size_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_selected_model_set, _elm_fileselector_button_elm_interface_fileselector_selected_model_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_selected_model_get, _elm_fileselector_button_elm_interface_fileselector_selected_model_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_hidden_visible_set, _elm_fileselector_button_elm_interface_fileselector_hidden_visible_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_hidden_visible_get, _elm_fileselector_button_elm_interface_fileselector_hidden_visible_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_is_save_set, _elm_fileselector_button_elm_interface_fileselector_is_save_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_is_save_get, _elm_fileselector_button_elm_interface_fileselector_is_save_get),
      EFL_OBJECT_OP_FUNC(efl_ui_view_model_set, _elm_fileselector_button_efl_ui_view_model_set),
      EFL_OBJECT_OP_FUNC(efl_ui_view_model_get, _elm_fileselector_button_efl_ui_view_model_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_sort_method_set, _elm_fileselector_button_elm_interface_fileselector_sort_method_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_sort_method_get, _elm_fileselector_button_elm_interface_fileselector_sort_method_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_multi_select_set, _elm_fileselector_button_elm_interface_fileselector_multi_select_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_multi_select_get, _elm_fileselector_button_elm_interface_fileselector_multi_select_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_folder_only_set, _elm_fileselector_button_elm_interface_fileselector_folder_only_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_folder_only_get, _elm_fileselector_button_elm_interface_fileselector_folder_only_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_mode_set, _elm_fileselector_button_elm_interface_fileselector_mode_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_mode_get, _elm_fileselector_button_elm_interface_fileselector_mode_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_current_name_set, _elm_fileselector_button_elm_interface_fileselector_current_name_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_current_name_get, _elm_fileselector_button_elm_interface_fileselector_current_name_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_custom_filter_append, _elm_fileselector_button_elm_interface_fileselector_custom_filter_append),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_mime_types_filter_append, _elm_fileselector_button_elm_interface_fileselector_mime_types_filter_append),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_filters_clear, _elm_fileselector_button_elm_interface_fileselector_filters_clear),
      ELM_FILESELECTOR_BUTTON_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_fileselector_button_class_desc = {
   EO_VERSION,
   "Elm.Fileselector_Button",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Fileselector_Button_Data),
   _elm_fileselector_button_class_initializer,
   NULL,
   NULL
};

/**
 * @brief Definition of the Elm_Fileselector_Button Efl_Class.
 * @internal
 *
 * Specifies metadata for the Elm_Fileselector_Button class, including its name,
 * type, data size, and initializer functions.
 */
EFL_DEFINE_CLASS(elm_fileselector_button_class_get, &_elm_fileselector_button_class_desc, EFL_UI_BUTTON_CLASS, ELM_INTERFACE_FILESELECTOR_INTERFACE, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
