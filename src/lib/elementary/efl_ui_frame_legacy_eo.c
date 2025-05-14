/**
 * @file
 * @brief Legacy Frame widget Eo API C source.
 *
 * This file contains the C source code for the Efl.Ui.Frame_Legacy Eo API.
 * It implements the class initialization, constructor, and other internal
 * functions for the legacy frame widget.
 */

Efl_Object *_efl_ui_frame_legacy_efl_object_constructor(Eo *obj, void *pd);

/**
 * @internal
 * @brief Initializes the Efl.Ui.Frame_Legacy class.
 *
 * This function is called once when the Efl.Ui.Frame_Legacy class is
 * initialized. It sets up the operations (methods) for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_frame_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_FRAME_LEGACY_EXTRA_OPS
#define EFL_UI_FRAME_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _efl_ui_frame_legacy_efl_object_constructor),
      EFL_UI_FRAME_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief The Efl_Class_Description for the Efl.Ui.Frame_Legacy class.
 *
 * This structure provides metadata about the Efl.Ui.Frame_Legacy class,
 * such as its version, name, type, and pointers to its initializer and
 * constructor functions.
 */
static const Efl_Class_Description _efl_ui_frame_legacy_class_desc = {
   EO_VERSION, /**< The Eo version for this class. */
   "Efl.Ui.Frame_Legacy", /**< The name of the class. */
   EFL_CLASS_TYPE_REGULAR, /**< The type of the class (regular, abstract, mixin, etc.). */
   0, /**< The size of the instance data. */
   _efl_ui_frame_legacy_class_initializer, /**< Pointer to the class initializer function. */
   _efl_ui_frame_legacy_class_constructor, /**< Pointer to the class constructor function (not shown in snippet). */
   NULL /**< Pointer to the class destructor function (none for this class). */
};

EFL_DEFINE_CLASS(efl_ui_frame_legacy_class_get, &_efl_ui_frame_legacy_class_desc, EFL_UI_FRAME_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
