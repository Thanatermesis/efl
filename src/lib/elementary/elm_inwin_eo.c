
/**
 * @internal
 * @brief Implements elm_obj_win_inwin_activate().
 * @param obj The inwin object.
 * @param pd The private data of the inwin object.
 */
void _elm_inwin_activate(Eo *obj, Elm_Inwin_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_win_inwin_activate);

/**
 * @internal
 * @brief Implements efl_constructor for Elm_Inwin.
 * @param obj The inwin object being constructed.
 * @param pd The private data of the inwin object.
 * @return The constructed Efl_Object.
 */
Efl_Object *_elm_inwin_efl_object_constructor(Eo *obj, Elm_Inwin_Data *pd);

/**
 * @internal
 * @brief Implements efl_content_set for Elm_Inwin.
 * @param obj The inwin object.
 * @param pd The private data of the inwin object.
 * @param content The content to set.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool _elm_inwin_efl_content_content_set(Eo *obj, Elm_Inwin_Data *pd, Efl_Gfx_Entity *content);

/**
 * @internal
 * @brief Implements efl_content_get for Elm_Inwin.
 * @param obj The inwin object.
 * @param pd The private data of the inwin object.
 * @return The content of the inwin object.
 */
Efl_Gfx_Entity *_elm_inwin_efl_content_content_get(const Eo *obj, Elm_Inwin_Data *pd);

/**
 * @internal
 * @brief Implements efl_content_unset for Elm_Inwin.
 * @param obj The inwin object.
 * @param pd The private data of the inwin object.
 * @return The previously set content, or NULL if none was set.
 */
Efl_Gfx_Entity *_elm_inwin_efl_content_content_unset(Eo *obj, Elm_Inwin_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Inwin class.
 *
 * This function sets up the operations for the Elm_Inwin class.
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_inwin_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_INWIN_EXTRA_OPS
#define ELM_INWIN_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_win_inwin_activate, _elm_inwin_activate),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_inwin_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_content_set, _elm_inwin_efl_content_content_set),
      EFL_OBJECT_OP_FUNC(efl_content_get, _elm_inwin_efl_content_content_get),
      EFL_OBJECT_OP_FUNC(efl_content_unset, _elm_inwin_efl_content_content_unset),
      ELM_INWIN_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief The Efl_Class_Description for the Elm_Inwin class.
 *
 * This structure provides metadata for the Elm_Inwin class,
 * including its version, name, type, size of instance data,
 * and initializer/constructor functions.
 */
static const Efl_Class_Description _elm_inwin_class_desc = {
   EO_VERSION,
   "Elm.Inwin",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Inwin_Data),
   _elm_inwin_class_initializer,
   _elm_inwin_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_inwin_class_get, &_elm_inwin_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_UI_FOCUS_LAYER_MIXIN, EFL_CONTENT_INTERFACE, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_inwin_eo.legacy.c"
