/**
 * @brief Definition of the "changed" event for Elm_Pan.
 * @ingroup Elm_Pan
 */
EWAPI const Efl_Event_Description _ELM_PAN_EVENT_CHANGED =
   EFL_EVENT_DESCRIPTION("changed");

/**
 * @brief Internal implementation for setting the pan position.
 * @param obj The Elm_Pan object.
 * @param pd The smart data associated with the object.
 * @param x The x-coordinate to set.
 * @param y The y-coordinate to set.
 */
void _elm_pan_pos_set(Eo *obj, Elm_Pan_Smart_Data *pd, int x, int y);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_pan_pos_set, EFL_FUNC_CALL(x, y), int x, int y);

/**
 * @brief Internal implementation for getting the pan position.
 * @param obj The Elm_Pan object.
 * @param pd The smart data associated with the object.
 * @param[out] x Pointer to store the x-coordinate.
 * @param[out] y Pointer to store the y-coordinate.
 */
void _elm_pan_pos_get(const Eo *obj, Elm_Pan_Smart_Data *pd, int *x, int *y);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_pan_pos_get, EFL_FUNC_CALL(x, y), int *x, int *y);

/**
 * @brief Internal implementation for getting the content size of the pan area.
 * @param obj The Elm_Pan object.
 * @param pd The smart data associated with the object.
 * @param[out] w Pointer to store the width.
 * @param[out] h Pointer to store the height.
 */
void _elm_pan_content_size_get(const Eo *obj, Elm_Pan_Smart_Data *pd, int *w, int *h);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_pan_content_size_get, EFL_FUNC_CALL(w, h), int *w, int *h);

/**
 * @brief Internal implementation for getting the minimum pannable position.
 * @param obj The Elm_Pan object.
 * @param pd The smart data associated with the object.
 * @param[out] x Pointer to store the minimum x-coordinate.
 * @param[out] y Pointer to store the minimum y-coordinate.
 */
void _elm_pan_pos_min_get(const Eo *obj, Elm_Pan_Smart_Data *pd, int *x, int *y);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_pan_pos_min_get, EFL_FUNC_CALL(x, y), int *x, int *y);

/**
 * @brief Internal implementation for getting the maximum pannable position.
 * @param obj The Elm_Pan object.
 * @param pd The smart data associated with the object.
 * @param[out] x Pointer to store the maximum x-coordinate.
 * @param[out] y Pointer to store the maximum y-coordinate.
 */
void _elm_pan_pos_max_get(const Eo *obj, Elm_Pan_Smart_Data *pd, int *x, int *y);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_pan_pos_max_get, EFL_FUNC_CALL(x, y), int *x, int *y);

/**
 * @brief EFL object constructor for Elm_Pan.
 *
 * This function is called when a new Elm_Pan object is constructed.
 * It initializes the object and its smart data.
 *
 * @param obj The Elm_Pan object being constructed.
 * @param pd The smart data associated with the object.
 * @return The constructed Efl_Object.
 */
Efl_Object *_elm_pan_efl_object_constructor(Eo *obj, Elm_Pan_Smart_Data *pd);

/**
 * @brief Sets the visibility of the Elm_Pan object.
 *
 * Implements the Efl_Gfx_Entity interface for visibility.
 *
 * @param obj The Elm_Pan object.
 * @param pd The smart data associated with the object.
 * @param v EINA_TRUE if visible, EINA_FALSE otherwise.
 */
void _elm_pan_efl_gfx_entity_visible_set(Eo *obj, Elm_Pan_Smart_Data *pd, Eina_Bool v);

/**
 * @brief Sets the position of the Elm_Pan object.
 *
 * Implements the Efl_Gfx_Entity interface for position.
 *
 * @param obj The Elm_Pan object.
 * @param pd The smart data associated with the object.
 * @param pos The new position as Eina_Position2D (struct with x and y).
 */
void _elm_pan_efl_gfx_entity_position_set(Eo *obj, Elm_Pan_Smart_Data *pd, Eina_Position2D pos);

/**
 * @brief Sets the size of the Elm_Pan object.
 *
 * Implements the Efl_Gfx_Entity interface for size.
 *
 * @param obj The Elm_Pan object.
 * @param pd The smart data associated with the object.
 * @param size The new size as Eina_Size2D (struct with w and h).
 */
void _elm_pan_efl_gfx_entity_size_set(Eo *obj, Elm_Pan_Smart_Data *pd, Eina_Size2D size);

/**
 * @brief Class initializer for Elm_Pan.
 *
 * This function is called once when the Elm_Pan class is being initialized.
 * It sets up the operations (methods) for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_pan_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_PAN_EXTRA_OPS
#define ELM_PAN_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_set, _elm_pan_pos_set),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_get, _elm_pan_pos_get),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_content_size_get, _elm_pan_content_size_get),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_min_get, _elm_pan_pos_min_get),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_max_get, _elm_pan_pos_max_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_pan_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_visible_set, _elm_pan_efl_gfx_entity_visible_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _elm_pan_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_pan_efl_gfx_entity_size_set),
      ELM_PAN_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief The Efl_Class_Description for the Elm_Pan class.
 *
 * This structure provides metadata about the Elm_Pan class,
 * including its version, name, type, size of instance data,
 * and pointers to initializer/constructor functions.
 */
static const Efl_Class_Description _elm_pan_class_desc = {
   EO_VERSION,
   "Elm.Pan",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Pan_Smart_Data),
   _elm_pan_class_initializer,
   _elm_pan_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_pan_class_get, &_elm_pan_class_desc, EFL_CANVAS_GROUP_CLASS, NULL);

#include "elm_pan_eo.legacy.c"
