/**
 * @file
 * @brief These routines are legacy routines for the Elm_Code_Widget.
 */

static Eina_Bool
_elm_code_widget_legacy_class_initializer(Efl_Class *klass)
{
   /**< The Efl_Object_Ops operations, or NULL. */
   const Efl_Object_Ops *opsp = NULL;

   /**< The Efl_Object_Property_Reflection_Ops operations, or NULL. */
   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifdef ELM_CODE_WIDGET_LEGACY_EXTRA_OPS
   EFL_OPS_DEFINE(ops, ELM_CODE_WIDGET_LEGACY_EXTRA_OPS);
   opsp = &ops;
#endif

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief The Efl_Class_Description for the Elm_Code_Widget_Legacy class.
 *
 * This structure provides metadata for the Elm_Code_Widget_Legacy class,
 * including its version, name, type, and initializer functions.
 */
static const Efl_Class_Description _elm_code_widget_legacy_class_desc = {
   EO_VERSION, /**< The EO_VERSION of this class. */
   "Elm.Code_Widget_Legacy", /**< The name of this class. */
   EFL_CLASS_TYPE_REGULAR, /**< The type of this class (regular). */
   0, /**< The size of the instance data. */
   _elm_code_widget_legacy_class_initializer, /**< The class initializer function. */
   NULL, /**< The class constructor function (none). */
   NULL /**< The class destructor function (none). */
};

EFL_DEFINE_CLASS(elm_code_widget_legacy_class_get, &_elm_code_widget_legacy_class_desc, ELM_CODE_WIDGET_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
