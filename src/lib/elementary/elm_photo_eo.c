/**
 * @internal
 * @brief Constructor for the Elm_Photo object.
 *
 * This function is called when a new Elm_Photo object is created.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Photo object.
 * @return The newly constructed Efl_Object.
 */
Efl_Object *_elm_photo_efl_object_constructor(Eo *obj, Elm_Photo_Data *pd);

/**
 * @internal
 * @brief Finalizer for the Elm_Photo object.
 *
 * This function is called when an Elm_Photo object is about to be destroyed.
 * It should be used to free any allocated resources.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Photo object.
 * @return The Efl_Object after finalization.
 */
Efl_Object *_elm_photo_efl_object_finalize(Eo *obj, Elm_Photo_Data *pd);

/**
 * @internal
 * @brief Sets the file to be displayed by the Elm_Photo object.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Photo object.
 * @param file The path to the image file. For example, "/path/to/image.png".
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
Eina_Error _elm_photo_efl_file_file_set(Eo *obj, Elm_Photo_Data *pd, const char *file);

/**
 * @internal
 * @brief Gets the file currently displayed by the Elm_Photo object.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Photo object.
 * @return The path to the image file, or NULL if no file is set.
 */
const char *_elm_photo_efl_file_file_get(const Eo *obj, Elm_Photo_Data *pd);

/**
 * @internal
 * @brief Sets a key for the file.
 *
 * This can be used, for example, by an image preloader to identify the file.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Photo object.
 * @param key The key string. For example, "image_id_123".
 */
void _elm_photo_efl_file_key_set(Eo *obj, Elm_Photo_Data *pd, const char *key);

/**
 * @internal
 * @brief Gets the key associated with the file.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Photo object.
 * @return The key string, or NULL if no key is set.
 */
const char *_elm_photo_efl_file_key_get(const Eo *obj, Elm_Photo_Data *pd);

/**
 * @internal
 * @brief Sets the file to be displayed from a memory-mapped Eina_File.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Photo object.
 * @param f The Eina_File object representing the memory-mapped file.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
Eina_Error _elm_photo_efl_file_mmap_set(Eo *obj, Elm_Photo_Data *pd, const Eina_File *f);

/**
 * @internal
 * @brief Gets the memory-mapped Eina_File used by the Elm_Photo object.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Photo object.
 * @return The Eina_File object, or NULL if not set via mmap.
 */
const Eina_File *_elm_photo_efl_file_mmap_get(const Eo *obj, Elm_Photo_Data *pd);

/**
 * @internal
 * @brief Loads the image data into memory.
 *
 * This function is typically called internally when the file is set or the widget becomes visible.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Photo object.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
Eina_Error _elm_photo_efl_file_load(Eo *obj, Elm_Photo_Data *pd);

/**
 * @internal
 * @brief Applies the Elementary theme to the Elm_Photo widget.
 *
 * This function is called when the widget's theme needs to be updated.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Photo object.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
Eina_Error _elm_photo_efl_ui_widget_theme_apply(Eo *obj, Elm_Photo_Data *pd);

/**
 * @internal
 * @brief Sets whether the Elm_Photo object can be a target for drag and drop operations.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Photo object.
 * @param set EINA_TRUE if it can be a drag target, EINA_FALSE otherwise.
 */
void _elm_photo_efl_ui_draggable_drag_target_set(Eo *obj, Elm_Photo_Data *pd, Eina_Bool set);

/**
 * @internal
 * @brief Gets whether the Elm_Photo object can be a target for drag and drop operations.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Photo object.
 * @return EINA_TRUE if it can be a drag target, EINA_FALSE otherwise.
 */
Eina_Bool _elm_photo_efl_ui_draggable_drag_target_get(const Eo *obj, Elm_Photo_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Photo class.
 *
 * This function is called once when the Efl_Class for Elm_Photo is created.
 * It sets up the operations (methods) for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_photo_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_PHOTO_EXTRA_OPS
#define ELM_PHOTO_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_photo_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_finalize, _elm_photo_efl_object_finalize),
      EFL_OBJECT_OP_FUNC(efl_file_set, _elm_photo_efl_file_file_set),
      EFL_OBJECT_OP_FUNC(efl_file_get, _elm_photo_efl_file_file_get),
      EFL_OBJECT_OP_FUNC(efl_file_key_set, _elm_photo_efl_file_key_set),
      EFL_OBJECT_OP_FUNC(efl_file_key_get, _elm_photo_efl_file_key_get),
      EFL_OBJECT_OP_FUNC(efl_file_mmap_set, _elm_photo_efl_file_mmap_set),
      EFL_OBJECT_OP_FUNC(efl_file_mmap_get, _elm_photo_efl_file_mmap_get),
      EFL_OBJECT_OP_FUNC(efl_file_load, _elm_photo_efl_file_load),
      EFL_OBJECT_OP_FUNC(efl_file_unload, _elm_photo_efl_file_unload),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_photo_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_draggable_drag_target_set, _elm_photo_efl_ui_draggable_drag_target_set),
      EFL_OBJECT_OP_FUNC(efl_ui_draggable_drag_target_get, _elm_photo_efl_ui_draggable_drag_target_get),
      ELM_PHOTO_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Photo class.
 *
 * This structure provides metadata about the Elm_Photo class,
 * such as its name, version, size of instance data, and initializer functions.
 */
static const Efl_Class_Description _elm_photo_class_desc = {
   EO_VERSION,
   "Elm.Photo",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Photo_Data),
   _elm_photo_class_initializer,
   _elm_photo_class_constructor,
   NULL
};

/**
 * @internal
 * @brief Defines the Elm_Photo class.
 *
 * This macro effectively registers the Elm_Photo class with the Eo system,
 * specifying its class description, parent class(es), and any mixins or interfaces it implements.
 *
 * - Parent class: EFL_UI_WIDGET_CLASS
 * - Mixins: EFL_FILE_MIXIN, EFL_INPUT_CLICKABLE_MIXIN
 * - Interfaces: EFL_UI_DRAGGABLE_INTERFACE, EFL_UI_LEGACY_INTERFACE
 */
EFL_DEFINE_CLASS(elm_photo_class_get, &_elm_photo_class_desc, EFL_UI_WIDGET_CLASS, EFL_FILE_MIXIN, EFL_INPUT_CLICKABLE_MIXIN, EFL_UI_DRAGGABLE_INTERFACE, EFL_UI_LEGACY_INTERFACE, NULL);
