/**
 * @file
 * @brief Implementation of the Elementary Fileselector Entry object.
 *
 * This file contains the internal implementation details for the
 * Elm_Fileselector_Entry widget.
 *
 * @ingroup Elm_Fileselector_Entry
 */

/**
 * @internal
 * @brief Event descriptor for the "changed" event.
 * @see ELM_FILESELECTOR_ENTRY_EVENT_CHANGED
 */
EWAPI const Efl_Event_Description _ELM_FILESELECTOR_ENTRY_EVENT_CHANGED =
   EFL_EVENT_DESCRIPTION("changed");
/**
 * @internal
 * @brief Event descriptor for the "activated" event.
 * @see ELM_FILESELECTOR_ENTRY_EVENT_ACTIVATED
 */
EWAPI const Efl_Event_Description _ELM_FILESELECTOR_ENTRY_EVENT_ACTIVATED =
   EFL_EVENT_DESCRIPTION("activated");
/**
 * @internal
 * @brief Event descriptor for the "file,chosen" event.
 * @see ELM_FILESELECTOR_ENTRY_EVENT_FILE_CHOSEN
 */
EWAPI const Efl_Event_Description _ELM_FILESELECTOR_ENTRY_EVENT_FILE_CHOSEN =
   EFL_EVENT_DESCRIPTION("file,chosen");
/**
 * @internal
 * @brief Event descriptor for the "press" event.
 * @see ELM_FILESELECTOR_ENTRY_EVENT_PRESS
 */
EWAPI const Efl_Event_Description _ELM_FILESELECTOR_ENTRY_EVENT_PRESS =
   EFL_EVENT_DESCRIPTION("press");

/**
 * @internal
 * @brief Implements the Efl.Object.constructor method.
 *
 * This function is called when a new Elm_Fileselector_Entry object is constructed.
 * It initializes the object's private data.
 *
 * @param[in] obj The Efl_Object to construct.
 * @param[in,out] pd The private data of the Elm_Fileselector_Entry.
 * @return The constructed Efl_Object.
 */
Efl_Object *_elm_fileselector_entry_efl_object_constructor(Eo *obj, Elm_Fileselector_Entry_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.theme_apply method.
 *
 * This function is called when the theme is applied or changed for the widget.
 * It applies the styling for the fileselector entry.
 *
 * @param[in] obj The Efl_Object to apply the theme to.
 * @param[in,out] pd The private data of the Elm_Fileselector_Entry.
 * @return EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Error _elm_fileselector_entry_efl_ui_widget_theme_apply(Eo *obj, Elm_Fileselector_Entry_Data *pd);

/**
 * @internal
 * @brief Implements the Elm.Interface.Fileselector.selected_model_set method.
 *
 * Sets the currently selected file or directory model.
 *
 * @param[in] obj The Elm_Fileselector_Entry object.
 * @param[in,out] pd The private data of the Elm_Fileselector_Entry.
 * @param[in] model The Efl_Io_Model representing the selected item.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_entry_elm_interface_fileselector_selected_model_set(Eo *obj, Elm_Fileselector_Entry_Data *pd, Efl_Io_Model *model);

/**
 * @internal
 * @brief Implements the Elm.Interface.Fileselector.selected_model_get method.
 *
 * Gets the currently selected file or directory model.
 *
 * @param[in] obj The Elm_Fileselector_Entry object.
 * @param[in] pd The private data of the Elm_Fileselector_Entry.
 * @return The Efl_Io_Model representing the selected item, or NULL if none is selected.
 */
Efl_Io_Model *_elm_fileselector_entry_elm_interface_fileselector_selected_model_get(const Eo *obj, Elm_Fileselector_Entry_Data *pd);

/**
 * @internal
 * @brief Implements the Elm.Interface.Fileselector.folder_only_set method.
 *
 * Sets whether the fileselector should only allow selecting folders.
 *
 * @param[in] obj The Elm_Fileselector_Entry object.
 * @param[in,out] pd The private data of the Elm_Fileselector_Entry.
 * @param[in] only If EINA_TRUE, only folders can be selected.
 */
void _elm_fileselector_entry_elm_interface_fileselector_folder_only_set(Eo *obj, Elm_Fileselector_Entry_Data *pd, Eina_Bool only);

/**
 * @internal
 * @brief Implements the Elm.Interface.Fileselector.folder_only_get method.
 *
 * Gets whether the fileselector is set to only allow selecting folders.
 *
 * @param[in] obj The Elm_Fileselector_Entry object.
 * @param[in] pd The private data of the Elm_Fileselector_Entry.
 * @return EINA_TRUE if only folders can be selected, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_entry_elm_interface_fileselector_folder_only_get(const Eo *obj, Elm_Fileselector_Entry_Data *pd);

/**
 * @internal
 * @brief Implements the Elm.Interface.Fileselector.is_save_set method.
 *
 * Sets whether the fileselector is in "save" mode.
 * In "save" mode, the user can type a filename that does not yet exist.
 *
 * @param[in] obj The Elm_Fileselector_Entry object.
 * @param[in,out] pd The private data of the Elm_Fileselector_Entry.
 * @param[in] is_save If EINA_TRUE, enables "save" mode.
 */
void _elm_fileselector_entry_elm_interface_fileselector_is_save_set(Eo *obj, Elm_Fileselector_Entry_Data *pd, Eina_Bool is_save);

/**
 * @internal
 * @brief Implements the Elm.Interface.Fileselector.is_save_get method.
 *
 * Gets whether the fileselector is in "save" mode.
 *
 * @param[in] obj The Elm_Fileselector_Entry object.
 * @param[in] pd The private data of the Elm_Fileselector_Entry.
 * @return EINA_TRUE if in "save" mode, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_entry_elm_interface_fileselector_is_save_get(const Eo *obj, Elm_Fileselector_Entry_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.View.model_set method.
 *
 * Sets the model for the fileselector entry. This model typically represents the
 * current directory being browsed.
 *
 * @param[in] obj The Elm_Fileselector_Entry object.
 * @param[in,out] pd The private data of the Elm_Fileselector_Entry.
 * @param[in] model The Efl_Model to set.
 */
void _elm_fileselector_entry_efl_ui_view_model_set(Eo *obj, Elm_Fileselector_Entry_Data *pd, Efl_Model *model);

/**
 * @internal
 * @brief Implements the Efl.Ui.View.model_get method.
 *
 * Gets the model for the fileselector entry.
 *
 * @param[in] obj The Elm_Fileselector_Entry object.
 * @param[in] pd The private data of the Elm_Fileselector_Entry.
 * @return The Efl_Model used by the fileselector entry.
 */
Efl_Model *_elm_fileselector_entry_efl_ui_view_model_get(const Eo *obj, Elm_Fileselector_Entry_Data *pd);

/**
 * @internal
 * @brief Implements the Elm.Interface.Fileselector.expandable_set method.
 *
 * Sets whether the fileselector entry is expandable (e.g., to show a list of files).
 *
 * @param[in] obj The Elm_Fileselector_Entry object.
 * @param[in,out] pd The private data of the Elm_Fileselector_Entry.
 * @param[in] expand If EINA_TRUE, the entry is expandable.
 */
void _elm_fileselector_entry_elm_interface_fileselector_expandable_set(Eo *obj, Elm_Fileselector_Entry_Data *pd, Eina_Bool expand);

/**
 * @internal
 * @brief Implements the Elm.Interface.Fileselector.expandable_get method.
 *
 * Gets whether the fileselector entry is expandable.
 *
 * @param[in] obj The Elm_Fileselector_Entry object.
 * @param[in] pd The private data of the Elm_Fileselector_Entry.
 * @return EINA_TRUE if the entry is expandable, EINA_FALSE otherwise.
 */
Eina_Bool _elm_fileselector_entry_elm_interface_fileselector_expandable_get(const Eo *obj, Elm_Fileselector_Entry_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Part.part_get method.
 *
 * Gets a specific part of the fileselector entry widget.
 * For example, "path" for the entry displaying the path, or "button" for the browse button.
 *
 * @param[in] obj The Elm_Fileselector_Entry object.
 * @param[in] pd The private data of the Elm_Fileselector_Entry.
 * @param[in] name The name of the part to get.
 * @return The Efl_Object representing the part, or NULL if not found.
 */
Efl_Object *_elm_fileselector_entry_efl_part_part_get(const Eo *obj, Elm_Fileselector_Entry_Data *pd, const char *name);

/**
 * @internal
 * @brief Initializes the Elm_Fileselector_Entry class.
 *
 * This function is called once when the class is first used.
 * It sets up the Efl_Object operations for the class.
 *
 * @param[in,out] klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_fileselector_entry_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_FILESELECTOR_ENTRY_EXTRA_OPS
#define ELM_FILESELECTOR_ENTRY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_fileselector_entry_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_fileselector_entry_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_selected_model_set, _elm_fileselector_entry_elm_interface_fileselector_selected_model_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_selected_model_get, _elm_fileselector_entry_elm_interface_fileselector_selected_model_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_folder_only_set, _elm_fileselector_entry_elm_interface_fileselector_folder_only_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_folder_only_get, _elm_fileselector_entry_elm_interface_fileselector_folder_only_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_is_save_set, _elm_fileselector_entry_elm_interface_fileselector_is_save_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_is_save_get, _elm_fileselector_entry_elm_interface_fileselector_is_save_get),
      EFL_OBJECT_OP_FUNC(efl_ui_view_model_set, _elm_fileselector_entry_efl_ui_view_model_set),
      EFL_OBJECT_OP_FUNC(efl_ui_view_model_get, _elm_fileselector_entry_efl_ui_view_model_get),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_expandable_set, _elm_fileselector_entry_elm_interface_fileselector_expandable_set),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_expandable_get, _elm_fileselector_entry_elm_interface_fileselector_expandable_get),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_fileselector_entry_efl_part_part_get),
      ELM_FILESELECTOR_ENTRY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Fileselector_Entry class.
 *
 * This structure provides metadata about the Elm_Fileselector_Entry class,
 * such as its name, version, size of private data, and initializer functions.
 */
static const Efl_Class_Description _elm_fileselector_entry_class_desc = {
   EO_VERSION, /**< EO_VERSION */
   "Elm.Fileselector_Entry", /**< Class name */
   EFL_CLASS_TYPE_REGULAR, /**< Class type */
   sizeof(Elm_Fileselector_Entry_Data), /**< Size of private data structure */
   _elm_fileselector_entry_class_initializer, /**< Class initializer function */
   _elm_fileselector_entry_class_constructor, /**< Class constructor function (typically NULL for auto-generated EO files) */
   NULL /**< Class destructor function (typically NULL for auto-generated EO files) */
};

/**
 * @internal
 * @brief Defines the Elm_Fileselector_Entry class.
 *
 * This macro effectively registers the Elm_Fileselector_Entry class with the
 * Efl object system, specifying its description and parent classes/mixins.
 *
 * Parent classes and mixins:
 * - EFL_UI_LAYOUT_BASE_CLASS: Base class for UI layout objects.
 * - ELM_INTERFACE_FILESELECTOR_INTERFACE: Interface for fileselector functionality.
 * - EFL_INPUT_CLICKABLE_MIXIN: Mixin for clickable behavior.
 * - ELM_LAYOUT_MIXIN: Mixin for Elementary layout capabilities.
 * - EFL_UI_LEGACY_INTERFACE: Interface for legacy UI compatibility.
 */
EFL_DEFINE_CLASS(elm_fileselector_entry_class_get, &_elm_fileselector_entry_class_desc, EFL_UI_LAYOUT_BASE_CLASS, ELM_INTERFACE_FILESELECTOR_INTERFACE, EFL_INPUT_CLICKABLE_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
