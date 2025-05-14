/**
 * @brief Definition of the "thumb,done" event.
 * This event is triggered when thumbnail generation for an icon is successfully completed.
 */
EWAPI const Efl_Event_Description _ELM_ICON_EVENT_THUMB_DONE =
   EFL_EVENT_DESCRIPTION("thumb,done");

/**
 * @brief Definition of the "thumb,error" event.
 * This event is triggered when thumbnail generation for an icon fails.
 */
EWAPI const Efl_Event_Description _ELM_ICON_EVENT_THUMB_ERROR =
   EFL_EVENT_DESCRIPTION("thumb,error");

/**
 * @brief EFL object constructor for Elm_Icon.
 *
 * @param obj The Efl_Object to construct.
 * @param pd The private data for the Elm_Icon instance.
 * @return The constructed Efl_Object.
 */
Efl_Object *_elm_icon_efl_object_constructor(Eo *obj, Elm_Icon_Data *pd);

/**
 * @brief Applies the theme to the Elm_Icon widget.
 *
 * @param obj The Elm_Icon object.
 * @param pd The private data for the Elm_Icon instance.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
Eina_Error _elm_icon_efl_ui_widget_theme_apply(Eo *obj, Elm_Icon_Data *pd);

/**
 * @brief Loads the file for the Elm_Icon.
 * This function is typically called when the file property of the icon is set.
 *
 * @param obj The Elm_Icon object.
 * @param pd The private data for the Elm_Icon instance.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
Eina_Error _elm_icon_efl_file_load(Eo *obj, Elm_Icon_Data *pd);

/**
 * @brief Class initializer for Elm_Icon.
 * Sets up the operations for the Elm_Icon class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_icon_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_ICON_EXTRA_OPS
#define ELM_ICON_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_icon_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_icon_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_file_load, _elm_icon_efl_file_load),
      ELM_ICON_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief EFL class description for Elm_Icon.
 * This structure provides metadata for the Elm_Icon class,
 * including its version, name, type, instance size, and initializer/constructor functions.
 */
static const Efl_Class_Description _elm_icon_class_desc = {
   EO_VERSION, /**< The EO API version for this class. */
   "Elm.Icon", /**< The name of the class. */
   EFL_CLASS_TYPE_REGULAR, /**< The type of the class (regular, abstract, mixin, interface). */
   sizeof(Elm_Icon_Data), /**< The size of the instance data. */
   _elm_icon_class_initializer, /**< The class initializer function. */
   _elm_icon_class_constructor, /**< The class constructor function (for EFL_UI_LEGACY_INTERFACE). */
   NULL /**< The class destructor function. */
};

/**
 * @brief Macro to define the Elm_Icon class.
 * This macro registers the Elm_Icon class with the EFL object system,
 * specifying its class description, parent class (EFL_UI_IMAGE_CLASS),
 * and any implemented interfaces (EFL_UI_LEGACY_INTERFACE).
 */
EFL_DEFINE_CLASS(elm_icon_class_get, &_elm_icon_class_desc, EFL_UI_IMAGE_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);
