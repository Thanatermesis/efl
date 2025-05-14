/**
 * @file
 * @brief Legacy Efl UI Background internal implementation
 */

/**
 * @internal
 * @brief Constructor for Efl_Ui_Bg_Legacy objects.
 *
 * This function is called when a new Efl_Ui_Bg_Legacy object is created.
 *
 * @param[in] obj The Efl_Ui_Bg_Legacy object being constructed.
 * @param[in] pd Private data for the Efl_Ui_Bg_Legacy object.
 * @return The initialized Efl_Object.
 */
Efl_Object *_efl_ui_bg_legacy_efl_object_constructor(Eo *obj, void *pd);


/**
 * @internal
 * @brief Initializes the Efl_Ui_Bg_Legacy class.
 *
 * This function is called once when the Efl_Ui_Bg_Legacy class is first used.
 * It sets up the operations and other class-specific data.
 *
 * @param[in,out] klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_bg_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_BG_LEGACY_EXTRA_OPS
#define EFL_UI_BG_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _efl_ui_bg_legacy_efl_object_constructor),
      EFL_UI_BG_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Efl_Ui_Bg_Legacy class.
 *
 * This structure contains metadata about the Efl_Ui_Bg_Legacy class,
 * including its version, name, type, and initializer/constructor functions.
 */
static const Efl_Class_Description _efl_ui_bg_legacy_class_desc = {
   EO_VERSION, /**< EO_VERSION */
   "Efl.Ui.Bg_Legacy", /**< Class name */
   EFL_CLASS_TYPE_REGULAR, /**< Class type */
   0, /**< Size of instance data */
   _efl_ui_bg_legacy_class_initializer, /**< Class initializer */
   _efl_ui_bg_legacy_class_constructor, /**< Class constructor */
   NULL /**< Class destructor */
};

/**
 * @internal
 * @brief Defines the Efl_Ui_Bg_Legacy class.
 *
 * This macro defines the Efl_Ui_Bg_Legacy class, associating it with its
 * class description, parent class(es), and mixins.
 */
EFL_DEFINE_CLASS(efl_ui_bg_legacy_class_get, &_efl_ui_bg_legacy_class_desc, EFL_UI_BG_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
