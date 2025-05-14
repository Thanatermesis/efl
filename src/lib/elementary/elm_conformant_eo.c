/**
 * @brief Event emitted when the virtual keypad is shown.
 * @ingroup Elm_Conformant
 */
EWAPI const Efl_Event_Description _ELM_CONFORMANT_EVENT_VIRTUALKEYPAD_STATE_ON =
   EFL_EVENT_DESCRIPTION("virtualkeypad,state,on");
/**
 * @brief Event emitted when the virtual keypad is hidden.
 * @ingroup Elm_Conformant
 */
EWAPI const Efl_Event_Description _ELM_CONFORMANT_EVENT_VIRTUALKEYPAD_STATE_OFF =
   EFL_EVENT_DESCRIPTION("virtualkeypad,state,off");
/**
 * @brief Event emitted when the clipboard becomes available (e.g., text is selected).
 * @ingroup Elm_Conformant
 */
EWAPI const Efl_Event_Description _ELM_CONFORMANT_EVENT_CLIPBOARD_STATE_ON =
   EFL_EVENT_DESCRIPTION("clipboard,state,on");
/**
 * @brief Event emitted when the clipboard becomes unavailable (e.g., text selection is cleared).
 * @ingroup Elm_Conformant
 */
EWAPI const Efl_Event_Description _ELM_CONFORMANT_EVENT_CLIPBOARD_STATE_OFF =
   EFL_EVENT_DESCRIPTION("clipboard,state,off");

/**
 * @brief Constructor for the Elm_Conformant object.
 *
 * @param[in] obj The Efl_Object to be constructed.
 * @param[in] pd The Elm_Conformant_Data private data for the object.
 * @return The constructed Efl_Object.
 */
Efl_Object *_elm_conformant_efl_object_constructor(Eo *obj, Elm_Conformant_Data *pd);

/**
 * @brief Applies the theme to the Elm_Conformant widget.
 *
 * This function is called when the theme of the widget needs to be updated.
 *
 * @param[in] obj The Efl_Object (Elm_Conformant) to apply the theme to.
 * @param[in] pd The Elm_Conformant_Data private data for the object.
 * @return EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Error _elm_conformant_efl_ui_widget_theme_apply(Eo *obj, Elm_Conformant_Data *pd);

/**
 * @brief Initializes the Elm_Conformant class.
 *
 * This function sets up the operations for the Elm_Conformant class.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_conformant_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_CONFORMANT_EXTRA_OPS
#define ELM_CONFORMANT_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_conformant_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_conformant_efl_ui_widget_theme_apply),
      ELM_CONFORMANT_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief Describes the Elm_Conformant class.
 *
 * This structure contains metadata about the Elm_Conformant class,
 * including its version, name, type, size of instance data,
 * and initializer/constructor functions.
 */
static const Efl_Class_Description _elm_conformant_class_desc = {
   EO_VERSION, /**< EO_VERSION */
   "Elm.Conformant", /**< Class name */
   EFL_CLASS_TYPE_REGULAR, /**< Class type */
   sizeof(Elm_Conformant_Data), /**< Size of instance data */
   _elm_conformant_class_initializer, /**< Class initializer function */
   _elm_conformant_class_constructor, /**< Class constructor function */
   NULL /**< Class destructor function (unused) */
};

/**
 * @brief Defines the Elm_Conformant class.
 *
 * This macro defines the `elm_conformant_class_get` function, which returns
 * the Efl_Class for Elm_Conformant. It also specifies the parent class
 * (EFL_UI_LAYOUT_BASE_CLASS) and any mixins (ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE).
 */
EFL_DEFINE_CLASS(elm_conformant_class_get, &_elm_conformant_class_desc, EFL_UI_LAYOUT_BASE_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
