/**
 * @file
 * @brief EFL UI Panes Legacy internal implementation.
 */

/**
 * @internal
 * @brief Constructor for Efl_Ui_Panes_Legacy objects.
 *
 * @param[in] obj The object to construct.
 * @param[in] pd The private data for the object.
 * @return The constructed object.
 */
Efl_Object *_efl_ui_panes_legacy_efl_object_constructor(Eo *obj, void *pd);


/**
 * @internal
 * @brief Initializes the Efl_Ui_Panes_Legacy class.
 *
 * Sets up the operations and functions for the class.
 *
 * @param[in] klass The class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_panes_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_PANES_LEGACY_EXTRA_OPS
#define EFL_UI_PANES_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _efl_ui_panes_legacy_efl_object_constructor),
      EFL_UI_PANES_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief The Efl_Class_Description for the Efl_Ui_Panes_Legacy class.
 *
 * Contains metadata about the class, such as its version, name, type,
 * and pointers to its initializer and constructor functions.
 */
static const Efl_Class_Description _efl_ui_panes_legacy_class_desc = {
   EO_VERSION,
   "Efl.Ui.Panes_Legacy",
   EFL_CLASS_TYPE_REGULAR,
   0,
   _efl_ui_panes_legacy_class_initializer,
   _efl_ui_panes_legacy_class_constructor,
   NULL
};

/**
 * @internal
 * @brief Defines the Efl_Ui_Panes_Legacy class.
 *
 * This macro generates the necessary boilerplate code to define the
 * Efl_Ui_Panes_Legacy class, associating it with its class description,
 * parent class(es), and mixin(s).
 */
EFL_DEFINE_CLASS(efl_ui_panes_legacy_class_get, &_efl_ui_panes_legacy_class_desc, EFL_UI_PANES_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
