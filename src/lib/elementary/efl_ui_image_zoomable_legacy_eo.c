/**
 * @file
 * @brief These routines are legacy routines for the Efl.Ui.Image_Zoomable widget.
 *
 * They should not be used in new code.
 *
 * @ingroup Efl_Ui_Image_Zoomable_Legacy
 */

Efl_Object *_efl_ui_image_zoomable_legacy_efl_object_constructor(Eo *obj, void *pd);


static Eina_Bool
_efl_ui_image_zoomable_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL; /**< Pointer to Efl_Object operations. */

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_IMAGE_ZOOMABLE_LEGACY_EXTRA_OPS
#define EFL_UI_IMAGE_ZOOMABLE_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _efl_ui_image_zoomable_legacy_efl_object_constructor),
      EFL_UI_IMAGE_ZOOMABLE_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief Class description for the Efl.Ui.Image_Zoomable_Legacy class.
 *
 * This structure provides metadata for the Efl.Ui.Image_Zoomable_Legacy class,
 * including its version, name, type, size, and initializer/constructor functions.
 */
static const Efl_Class_Description _efl_ui_image_zoomable_legacy_class_desc = {
   EO_VERSION, /**< EO_VERSION macro defining the Efl Object system version. */
   "Efl.Ui.Image_Zoomable_Legacy", /**< The name of the class. */
   EFL_CLASS_TYPE_REGULAR, /**< Specifies that this is a regular EFL class. */
   0, /**< The size of the instance data. */
   _efl_ui_image_zoomable_legacy_class_initializer, /**< Function to initialize the class. */
   _efl_ui_image_zoomable_legacy_class_constructor, /**< Function to construct an instance of the class. */
   NULL /**< No custom class deconstructor. */
};

EFL_DEFINE_CLASS(efl_ui_image_zoomable_legacy_class_get, &_efl_ui_image_zoomable_legacy_class_desc, EFL_UI_IMAGE_ZOOMABLE_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);
