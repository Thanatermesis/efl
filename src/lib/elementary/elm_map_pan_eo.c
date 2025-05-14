/**
 * @brief Destructor for the Elm_Map_Pan object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 */
void _elm_map_pan_efl_object_destructor(Eo *obj, Elm_Map_Pan_Data *pd);

/**
 * @brief Sets the position of the Elm_Map_Pan object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @param[in] pos The new position.
 */
void _elm_map_pan_efl_gfx_entity_position_set(Eo *obj, Elm_Map_Pan_Data *pd, Eina_Position2D pos);

/**
 * @brief Sets the size of the Elm_Map_Pan object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @param[in] size The new size.
 */
void _elm_map_pan_efl_gfx_entity_size_set(Eo *obj, Elm_Map_Pan_Data *pd, Eina_Size2D size);

/**
 * @brief Calculates the size of the group.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 */
void _elm_map_pan_efl_canvas_group_group_calculate(Eo *obj, Elm_Map_Pan_Data *pd);

/**
 * @brief Gets the content size of the pan object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @param[out] w Pointer to store the width.
 * @param[out] h Pointer to store the height.
 */
void _elm_map_pan_elm_pan_content_size_get(const Eo *obj, Elm_Map_Pan_Data *pd, int *w, int *h);

/**
 * @brief Sets the position of the pan object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @param[in] x The x coordinate.
 * @param[in] y The y coordinate.
 */
void _elm_map_pan_elm_pan_pos_set(Eo *obj, Elm_Map_Pan_Data *pd, int x, int y);

/**
 * @brief Gets the position of the pan object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @param[out] x Pointer to store the x coordinate.
 * @param[out] y Pointer to store the y coordinate.
 */
void _elm_map_pan_elm_pan_pos_get(const Eo *obj, Elm_Map_Pan_Data *pd, int *x, int *y);

/**
 * @brief Gets the minimum position of the pan object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @param[out] x Pointer to store the minimum x coordinate.
 * @param[out] y Pointer to store the minimum y coordinate.
 */
void _elm_map_pan_elm_pan_pos_min_get(const Eo *obj, Elm_Map_Pan_Data *pd, int *x, int *y);

/**
 * @brief Gets the maximum position of the pan object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @param[out] x Pointer to store the maximum x coordinate.
 * @param[out] y Pointer to store the maximum y coordinate.
 */
void _elm_map_pan_elm_pan_pos_max_get(const Eo *obj, Elm_Map_Pan_Data *pd, int *x, int *y);

/**
 * @brief Initializes the Elm_Map_Pan class.
 * @param[in] klass The class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_map_pan_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_MAP_PAN_EXTRA_OPS
#define ELM_MAP_PAN_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_map_pan_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _elm_map_pan_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_map_pan_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_calculate, _elm_map_pan_efl_canvas_group_group_calculate),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_content_size_get, _elm_map_pan_elm_pan_content_size_get),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_set, _elm_map_pan_elm_pan_pos_set),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_get, _elm_map_pan_elm_pan_pos_get),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_min_get, _elm_map_pan_elm_pan_pos_min_get),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_max_get, _elm_map_pan_elm_pan_pos_max_get),
      ELM_MAP_PAN_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief Class description for Elm_Map_Pan.
 */
static const Efl_Class_Description _elm_map_pan_class_desc = {
   EO_VERSION,
   "Elm.Map.Pan",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Map_Pan_Data),
   _elm_map_pan_class_initializer,
   _elm_map_pan_class_constructor,
   NULL
};

/**
 * @brief Defines the Elm_Map_Pan class.
 */
EFL_DEFINE_CLASS(elm_map_pan_class_get, &_elm_map_pan_class_desc, ELM_PAN_CLASS, NULL);
