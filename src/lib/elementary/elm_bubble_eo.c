/**
 * @internal
 * @brief Internal implementation for elm_obj_bubble_pos_set.
 * @see elm_obj_bubble_pos_set
 */
void _elm_bubble_pos_set(Eo *obj, Elm_Bubble_Data *pd, Elm_Bubble_Pos pos);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_bubble_pos_set, EFL_FUNC_CALL(pos), Elm_Bubble_Pos pos);

/**
 * @internal
 * @brief Internal implementation for elm_obj_bubble_pos_get.
 * @see elm_obj_bubble_pos_get
 */
Elm_Bubble_Pos _elm_bubble_pos_get(const Eo *obj, Elm_Bubble_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_bubble_pos_get, Elm_Bubble_Pos, 0);

/**
 * @internal
 * @brief Constructor for the Elm_Bubble object.
 *
 * This function is called when a new Elm_Bubble object is created.
 * It initializes the object's private data and sets up its initial state.
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the Elm_Bubble object.
 * @return The constructed Eo object, or @c NULL on failure.
 */
Efl_Object *_elm_bubble_efl_object_constructor(Eo *obj, Elm_Bubble_Data *pd);

/**
 * @internal
 * @brief Handles accessibility updates for the Elm_Bubble widget.
 *
 * This function is called when the accessibility state of the widget changes.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Bubble object.
 * @param enable EINA_TRUE if accessibility is enabled, EINA_FALSE otherwise.
 */
void _elm_bubble_efl_ui_widget_on_access_update(Eo *obj, Elm_Bubble_Data *pd, Eina_Bool enable);

/**
 * @internal
 * @brief Retrieves a part of the Elm_Bubble widget.
 *
 * This function is used to get a handle to a named part of the widget,
 * such as "icon" or "label".
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Bubble object.
 * @param name The name of the part to retrieve.
 * @return The Efl_Object representing the part, or @c NULL if not found.
 */
Efl_Object *_elm_bubble_efl_part_part_get(const Eo *obj, Elm_Bubble_Data *pd, const char *name);


/**
 * @internal
 * @brief Initializes the Elm_Bubble class.
 *
 * This function is called once when the Elm_Bubble class is first loaded.
 * It sets up the class's operations (methods) and other class-specific data.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_bubble_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_BUBBLE_EXTRA_OPS
#define ELM_BUBBLE_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_bubble_pos_set, _elm_bubble_pos_set),
      EFL_OBJECT_OP_FUNC(elm_obj_bubble_pos_get, _elm_bubble_pos_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_bubble_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_on_access_update, _elm_bubble_efl_ui_widget_on_access_update),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_bubble_efl_part_part_get),
      ELM_BUBBLE_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Bubble class.
 *
 * This structure contains metadata about the Elm_Bubble class,
 * such as its version, name, type, size of private data,
 * and pointers to initializer/constructor functions.
 */
static const Efl_Class_Description _elm_bubble_class_desc = {
   EO_VERSION,
   "Elm.Bubble",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Bubble_Data),
   _elm_bubble_class_initializer,
   _elm_bubble_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_bubble_class_get, &_elm_bubble_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_INPUT_CLICKABLE_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_bubble_eo.legacy.c"
