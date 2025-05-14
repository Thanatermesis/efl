/**
 * @internal
 * @brief Constructor for Efl_Ui_Flip_Legacy objects.
 *
 * This function is called when a new Efl_Ui_Flip_Legacy object is created.
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the object.
 * @return The constructed Eo object.
 */
Efl_Object *_efl_ui_flip_legacy_efl_object_constructor(Eo *obj, void *pd);

/**
 * @internal
 * @brief Initializes the Efl_Ui_Flip_Legacy class.
 *
 * This function is called once when the Efl_Ui_Flip_Legacy class is first used.
 * It sets up the operations (methods) for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_flip_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_FLIP_LEGACY_EXTRA_OPS
#define EFL_UI_FLIP_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _efl_ui_flip_legacy_efl_object_constructor),
      EFL_UI_FLIP_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Efl_Ui_Flip_Legacy class.
 *
 * This structure contains metadata about the Efl_Ui_Flip_Legacy class,
 * such as its version, name, type, and initializer functions.
 */
static const Efl_Class_Description _efl_ui_flip_legacy_class_desc = {
   EO_VERSION, /**< The EO API version for this class. */
   "Efl.Ui.Flip_Legacy", /**< The full name of the class. */
   EFL_CLASS_TYPE_REGULAR,
   0, /**< The size of the instance data. */
   _efl_ui_flip_legacy_class_initializer, /**< Function to initialize the class. */
   _efl_ui_flip_legacy_class_constructor, /**< Function to construct the class. */
   NULL /**< Function to destruct the class. */
};

/**
 * @internal
 * @brief Defines the Efl_Ui_Flip_Legacy class.
 *
 * This macro generates the efl_ui_flip_legacy_class_get() function and
 * registers the Efl_Ui_Flip_Legacy class with the Eo system.
 * It specifies the class description, parent class (EFL_UI_FLIP_CLASS),
 * and any implemented interfaces (EFL_UI_LEGACY_INTERFACE).
 */
EFL_DEFINE_CLASS(efl_ui_flip_legacy_class_get, &_efl_ui_flip_legacy_class_desc, EFL_UI_FLIP_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);
