/**
 * @brief Event descriptor for a thumbnail generation error.
 * @ingroup Elm_Thumb
 */
EWAPI const Efl_Event_Description _ELM_THUMB_EVENT_GENERATE_ERROR =
   EFL_EVENT_DESCRIPTION("generate,error");

/**
 * @brief Event descriptor for the start of thumbnail generation.
 * @ingroup Elm_Thumb
 */
EWAPI const Efl_Event_Description _ELM_THUMB_EVENT_GENERATE_START =
   EFL_EVENT_DESCRIPTION("generate,start");

/**
 * @brief Event descriptor for the end of thumbnail generation.
 * @ingroup Elm_Thumb
 */
EWAPI const Efl_Event_Description _ELM_THUMB_EVENT_GENERATE_STOP =
   EFL_EVENT_DESCRIPTION("generate,stop");

/**
 * @brief Event descriptor for a thumbnail loading error.
 * @ingroup Elm_Thumb
 */
EWAPI const Efl_Event_Description _ELM_THUMB_EVENT_LOAD_ERROR =
   EFL_EVENT_DESCRIPTION("load,error");

/**
 * @brief Event descriptor for a press event on the thumbnail.
 * @ingroup Elm_Thumb
 */
EWAPI const Efl_Event_Description _ELM_THUMB_EVENT_PRESS =
   EFL_EVENT_DESCRIPTION("press");

/**
 * @brief Constructs a new Elm_Thumb object.
 * @param[in] obj The Efl_Object to construct.
 * @param[in] pd The Elm_Thumb_Data private data for the object.
 * @return The newly constructed Efl_Object.
 * @ingroup Elm_Thumb_Group
 */
Efl_Object *_elm_thumb_efl_object_constructor(Eo *obj, Elm_Thumb_Data *pd);

/**
 * @brief Finalizes an Elm_Thumb object.
 * @param[in] obj The Efl_Object to finalize.
 * @param[in] pd The Elm_Thumb_Data private data for the object.
 * @return The finalized Efl_Object.
 * @ingroup Elm_Thumb_Group
 */
Efl_Object *_elm_thumb_efl_object_finalize(Eo *obj, Elm_Thumb_Data *pd);

/**
 * @brief Loads the thumbnail from the file set.
 * @param[in] obj The Efl_Object to operate on.
 * @param[in] pd The Elm_Thumb_Data private data for the object.
 * @return #EINA_ERROR_NONE on success, or an error code on failure.
 * @ingroup Elm_Thumb_Group
 */
Eina_Error _elm_thumb_efl_file_load(Eo *obj, Elm_Thumb_Data *pd);

/**
 * @brief Gets the loaded state of the thumbnail.
 * @param[in] obj The Efl_Object to query.
 * @param[in] pd The Elm_Thumb_Data private data for the object.
 * @return #EINA_TRUE if the thumbnail is loaded, #EINA_FALSE otherwise.
 * @ingroup Elm_Thumb_Group
 */
Eina_Bool _elm_thumb_efl_file_loaded_get(const Eo *obj, Elm_Thumb_Data *pd);

/**
 * @brief Sets the file from which to load the thumbnail.
 * @param[in] obj The Efl_Object to operate on.
 * @param[in] pd The Elm_Thumb_Data private data for the object.
 * @param[in] file The path to the image or video file.
 * @return #EINA_ERROR_NONE on success, or an error code on failure.
 * @ingroup Elm_Thumb_Group
 */
Eina_Error _elm_thumb_efl_file_file_set(Eo *obj, Elm_Thumb_Data *pd, const char *file);

/**
 * @brief Gets the file from which the thumbnail is loaded.
 * @param[in] obj The Efl_Object to query.
 * @param[in] pd The Elm_Thumb_Data private data for the object.
 * @return The path to the image or video file, or @c NULL if not set.
 * @ingroup Elm_Thumb_Group
 */
const char *_elm_thumb_efl_file_file_get(const Eo *obj, Elm_Thumb_Data *pd);

/**
 * @brief Sets the key used for caching the thumbnail.
 * @param[in] obj The Efl_Object to operate on.
 * @param[in] pd The Elm_Thumb_Data private data for the object.
 * @param[in] key The key for caching.
 * @ingroup Elm_Thumb_Group
 */
void _elm_thumb_efl_file_key_set(Eo *obj, Elm_Thumb_Data *pd, const char *key);

/**
 * @brief Gets the key used for caching the thumbnail.
 * @param[in] obj The Efl_Object to query.
 * @param[in] pd The Elm_Thumb_Data private data for the object.
 * @return The key for caching, or @c NULL if not set.
 * @ingroup Elm_Thumb_Group
 */
const char *_elm_thumb_efl_file_key_get(const Eo *obj, Elm_Thumb_Data *pd);

/**
 * @brief Sets the visibility of the thumbnail object.
 * @param[in] obj The Efl_Object to operate on.
 * @param[in] pd The Elm_Thumb_Data private data for the object.
 * @param[in] v #EINA_TRUE to make visible, #EINA_FALSE to make hidden.
 * @ingroup Elm_Thumb_Group
 */
void _elm_thumb_efl_gfx_entity_visible_set(Eo *obj, Elm_Thumb_Data *pd, Eina_Bool v);

/**
 * @brief Sets whether the thumbnail can be a target for drag and drop.
 * @param[in] obj The Efl_Object to operate on.
 * @param[in] pd The Elm_Thumb_Data private data for the object.
 * @param[in] set #EINA_TRUE to enable as a drag target, #EINA_FALSE to disable.
 * @ingroup Elm_Thumb_Group
 */
void _elm_thumb_efl_ui_draggable_drag_target_set(Eo *obj, Elm_Thumb_Data *pd, Eina_Bool set);

/**
 * @brief Gets whether the thumbnail can be a target for drag and drop.
 * @param[in] obj The Efl_Object to query.
 * @param[in] pd The Elm_Thumb_Data private data for the object.
 * @return #EINA_TRUE if enabled as a drag target, #EINA_FALSE otherwise.
 * @ingroup Elm_Thumb_Group
 */
Eina_Bool _elm_thumb_efl_ui_draggable_drag_target_get(const Eo *obj, Elm_Thumb_Data *pd);

/**
 * @brief Initializes the Elm_Thumb class.
 *
 * This function sets up the operations for the Elm_Thumb class.
 * @param[in] klass The Efl_Class to initialize.
 * @return #EINA_TRUE on success, #EINA_FALSE otherwise.
 * @ingroup Elm_Thumb_Group
 */
static Eina_Bool
_elm_thumb_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_THUMB_EXTRA_OPS
#define ELM_THUMB_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_thumb_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_finalize, _elm_thumb_efl_object_finalize),
      EFL_OBJECT_OP_FUNC(efl_file_load, _elm_thumb_efl_file_load),
      EFL_OBJECT_OP_FUNC(efl_file_unload, _elm_thumb_efl_file_unload),
      EFL_OBJECT_OP_FUNC(efl_file_loaded_get, _elm_thumb_efl_file_loaded_get),
      EFL_OBJECT_OP_FUNC(efl_file_set, _elm_thumb_efl_file_file_set),
      EFL_OBJECT_OP_FUNC(efl_file_get, _elm_thumb_efl_file_file_get),
      EFL_OBJECT_OP_FUNC(efl_file_key_set, _elm_thumb_efl_file_key_set),
      EFL_OBJECT_OP_FUNC(efl_file_key_get, _elm_thumb_efl_file_key_get),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_visible_set, _elm_thumb_efl_gfx_entity_visible_set),
      EFL_OBJECT_OP_FUNC(efl_ui_draggable_drag_target_set, _elm_thumb_efl_ui_draggable_drag_target_set),
      EFL_OBJECT_OP_FUNC(efl_ui_draggable_drag_target_get, _elm_thumb_efl_ui_draggable_drag_target_get),
      ELM_THUMB_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief The Efl_Class_Description for the Elm_Thumb class.
 * @ingroup Elm_Thumb_Group
 */
static const Efl_Class_Description _elm_thumb_class_desc = {
   EO_VERSION,
   "Elm.Thumb",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Thumb_Data),
   _elm_thumb_class_initializer,
   _elm_thumb_class_constructor,
   NULL
};

/**
 * @brief Defines the Elm_Thumb class.
 *
 * This macro defines the Elm_Thumb class, inheriting from
 * #EFL_UI_LAYOUT_BASE_CLASS, and mixing in #EFL_FILE_MIXIN,
 * #EFL_INPUT_CLICKABLE_MIXIN, #EFL_UI_DRAGGABLE_INTERFACE,
 * #ELM_LAYOUT_MIXIN, and #EFL_UI_LEGACY_INTERFACE.
 * @ingroup Elm_Thumb_Group
 */
EFL_DEFINE_CLASS(elm_thumb_class_get, &_elm_thumb_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_FILE_MIXIN, EFL_INPUT_CLICKABLE_MIXIN, EFL_UI_DRAGGABLE_INTERFACE, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
