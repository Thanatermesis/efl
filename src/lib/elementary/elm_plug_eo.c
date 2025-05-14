/** @brief Identifier for the #ELM_PLUG_EVENT_IMAGE_DELETED event.
 * This event is triggered when the underlying image of the plug is deleted.
 */
EWAPI const Efl_Event_Description _ELM_PLUG_EVENT_IMAGE_DELETED =
   EFL_EVENT_DESCRIPTION("image,deleted");

/** @brief Identifier for the #ELM_PLUG_EVENT_IMAGE_RESIZED event.
 * This event is triggered when the underlying image of the plug is resized.
 * The event data contains the new Eina_Position2D size.
 */
EWAPI const Efl_Event_Description _ELM_PLUG_EVENT_IMAGE_RESIZED =
   EFL_EVENT_DESCRIPTION("image,resized");

/**
 * @brief Internal implementation for #elm_obj_plug_image_object_get.
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Plug instance.
 * @return The Evas_Object image, or @c NULL on failure.
 */
Efl_Canvas_Object *_elm_plug_image_object_get(const Eo *obj, void *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_plug_image_object_get, Efl_Canvas_Object *, NULL);

/**
 * @brief Internal implementation for #elm_obj_plug_connect.
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Plug instance.
 * @param svcname Service name to connect to.
 * @param svcnum Service number to connect to.
 * @param svcsys Boolean indicating if the service is system-wide.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
Eina_Bool _elm_plug_connect(Eo *obj, void *pd, const char *svcname, int svcnum, Eina_Bool svcsys);

EOAPI EFL_FUNC_BODYV(elm_obj_plug_connect, Eina_Bool, 0, EFL_FUNC_CALL(svcname, svcnum, svcsys), const char *svcname, int svcnum, Eina_Bool svcsys);

/**
 * @brief EFL object constructor for Elm_Plug.
 *
 * This function is called when a new Elm_Plug instance is created.
 * It handles basic initialization of the object.
 * @param obj The Eo object to construct.
 * @param pd Private data for the Elm_Plug instance.
 * @return The constructed Eo object, or @c NULL on failure.
 */
Efl_Object *_elm_plug_efl_object_constructor(Eo *obj, void *pd);

/**
 * @brief Applies the theme to the Elm_Plug widget.
 *
 * This function is called when the widget's theme needs to be updated,
 * for example, when the widget is created or the global theme changes.
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Plug instance.
 * @return An Eina_Error code, #EINA_ERROR_NONE on success.
 */
Eina_Error _elm_plug_efl_ui_widget_theme_apply(Eo *obj, void *pd);

/**
 * @brief Handles focus updates for the Elm_Plug widget.
 *
 * This function is called when the focus state of the widget might change.
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Plug instance.
 * @return @c EINA_TRUE if focus handling was successful, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_plug_efl_ui_focus_object_on_focus_update(Eo *obj, void *pd);

/**
 * @brief Initializes the Elm_Plug Efl_Class.
 *
 * This function is called once during class construction to set up
 * the operations (methods) for the Elm_Plug class.
 * @param klass The Efl_Class to initialize.
 * @return #EINA_TRUE on success, #EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_plug_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_PLUG_EXTRA_OPS
#define ELM_PLUG_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_plug_image_object_get, _elm_plug_image_object_get),
      EFL_OBJECT_OP_FUNC(elm_obj_plug_connect, _elm_plug_connect),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_plug_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_plug_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_on_focus_update, _elm_plug_efl_ui_focus_object_on_focus_update),
      ELM_PLUG_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief Describes the Elm_Plug Efl_Class.
 *
 * This structure provides metadata for the Elm_Plug class,
 * including its EO version, name, type, size of private data,
 * and initializer functions (class_initializer, class_constructor, class_destructor).
 */
static const Efl_Class_Description _elm_plug_class_desc = {
   EO_VERSION,
   "Elm.Plug",
   EFL_CLASS_TYPE_REGULAR,
   0,
   _elm_plug_class_initializer,
   _elm_plug_class_constructor,
   NULL
};

/**
 * @brief Macro that defines the Elm_Plug Efl_Class.
 *
 * This macro generates the necessary boilerplate code to register the Elm_Plug
 * class with the EFL object system. It specifies the class getter function,
 * the class description, and its parent/mixin classes.
 */
EFL_DEFINE_CLASS(elm_plug_class_get, &_elm_plug_class_desc, EFL_UI_WIDGET_CLASS, EFL_INPUT_CLICKABLE_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

// Include legacy C-specific API implementations for Elm_Plug.
// This is typically for functions that were part of the pre-EO API
// and are maintained for compatibility.
#include "elm_plug_eo.legacy.c"
