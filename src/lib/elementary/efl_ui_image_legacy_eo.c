/**
 * @file
 * @brief These routines are legacy routines for the Efl.Ui.Image widget.
 *
 * They should not be used in new code.
 *
 * @ingroup Efl_Ui_Image_Legacy
 */

Efl_Object *_efl_ui_image_legacy_efl_object_constructor(Eo *obj, void *pd);

/**
 * @internal
 * @brief Initializes the Efl.Ui.Image_Legacy class.
 *
 * This function is called once when the class is first used.
 * It sets up the operations for the class.
 *
 * @param[in] klass The class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_image_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_IMAGE_LEGACY_EXTRA_OPS
#define EFL_UI_IMAGE_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _efl_ui_image_legacy_efl_object_constructor),
      EFL_UI_IMAGE_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Efl.Ui.Image_Legacy class.
 *
 * This structure contains metadata about the class, such as its version,
 * name, type, and initializer functions.
 */
static const Efl_Class_Description _efl_ui_image_legacy_class_desc = {
   EO_VERSION, /**< The EO API version for this class. */
   "Efl.Ui.Image_Legacy", /**< The name of the class. */
   EFL_CLASS_TYPE_REGULAR, /**< The type of the class (regular, abstract, mixin, interface). */
   0,
   _efl_ui_image_legacy_class_initializer, /**< The class initializer function. */
   _efl_ui_image_legacy_class_constructor, /**< The class constructor function. */
   NULL /**< The class destructor function (none in this case). */
};

/**
 * @brief Defines the Efl.Ui.Image_Legacy class.
 *
 * This macro generates the necessary functions and structures to define
 * the Efl.Ui.Image_Legacy class, inheriting from EFL_UI_IMAGE_CLASS and
 * implementing the EFL_UI_LEGACY_INTERFACE.
 *
 * @param efl_ui_image_legacy_class_get The function to get the Efl.Ui.Image_Legacy class.
 * @param &_efl_ui_image_legacy_class_desc A pointer to the class description.
 * @param EFL_UI_IMAGE_CLASS The parent class.
 * @param EFL_UI_LEGACY_INTERFACE The interface implemented by this class.
 * @param NULL No further interfaces are implemented.
 */
EFL_DEFINE_CLASS(efl_ui_image_legacy_class_get, &_efl_ui_image_legacy_class_desc, EFL_UI_IMAGE_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);
