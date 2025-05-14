/**
 * @file
 * @brief These routines are legacy routines used for Efl.Ui.Layout.Legacy.
 *
 * This is an internal Eo file, not to be used from application code.
 */

Efl_Object *_efl_ui_layout_legacy_efl_object_constructor(Eo *obj, void *pd);

/**
 * @internal
 * @brief Initializes the Efl.Ui.Layout_Legacy class.
 *
 * This function sets up the operations for the Efl.Ui.Layout_Legacy class.
 *
 * @param klass The class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_layout_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_LAYOUT_LEGACY_EXTRA_OPS
#define EFL_UI_LAYOUT_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _efl_ui_layout_legacy_efl_object_constructor),
      EFL_UI_LAYOUT_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Efl.Ui.Layout_Legacy class.
 *
 * This structure contains metadata about the Efl.Ui.Layout_Legacy class,
 * including its version, name, type, and initializer functions.
 */
static const Efl_Class_Description _efl_ui_layout_legacy_class_desc = {
   EO_VERSION,
   "Efl.Ui.Layout_Legacy",
   EFL_CLASS_TYPE_REGULAR,
   0,
   _efl_ui_layout_legacy_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(efl_ui_layout_legacy_class_get, &_efl_ui_layout_legacy_class_desc, EFL_UI_LAYOUT_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

/**
 * @internal
 * @brief Implements the sizing evaluation for Elm_Layout.
 *
 * This function is the actual implementation for elm_layout_sizing_eval.
 *
 * @param obj The Efl.Ui.Layout_Legacy object.
 * @param ld The Elm_Layout_Data associated with the object.
 */
static void _elm_layout_sizing_eval(Eo *obj, Elm_Layout_Data *ld);
EAPI EFL_VOID_FUNC_BODY(elm_layout_sizing_eval);

/**
 * @internal
 * @brief Initializes the Elm.Layout mixin class.
 *
 * This function sets up the operations for the Elm.Layout mixin class.
 *
 * @param klass The class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_layout_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_LAYOUT_EXTRA_OPS
#define ELM_LAYOUT_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_layout_sizing_eval, _elm_layout_sizing_eval),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_change, _elm_layout_efl_canvas_group_change),
      EFL_OBJECT_OP_FUNC(efl_gfx_hint_size_restricted_min_set, _elm_layout_efl_gfx_hint_size_restricted_min_set),
      ELM_LAYOUT_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm.Layout mixin class.
 *
 * This structure contains metadata about the Elm.Layout mixin class,
 * including its version, name, type, size of instance data, and initializer functions.
 */
static const Efl_Class_Description _elm_layout_class_desc = {
   EO_VERSION,
   "Elm.Layout",
   EFL_CLASS_TYPE_MIXIN,
   sizeof(Elm_Layout_Data),
   _elm_layout_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_layout_mixin_get, &_elm_layout_class_desc, NULL, NULL);
