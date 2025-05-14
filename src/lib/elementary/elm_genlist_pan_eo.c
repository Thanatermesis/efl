/**
 * @internal
 * @brief Destructor for the Elm_Genlist_Pan object.
 *
 * This function is called when the Elm_Genlist_Pan object is being destroyed.
 * It should free any resources allocated by the object.
 *
 * @param obj The Efl_Object to be destructed.
 * @param pd The private data of the Elm_Genlist_Pan object.
 */
void _elm_genlist_pan_efl_object_destructor(Eo *obj, Elm_Genlist_Pan_Data *pd);

/**
 * @internal
 * @brief Sets the position of the Elm_Genlist_Pan object.
 *
 * This function is part of the Efl_Gfx_Entity interface implementation.
 * It sets the 2D position (x, y) of the pan object.
 *
 * @param obj The Efl_Object whose position is to be set.
 * @param pd The private data of the Elm_Genlist_Pan object.
 * @param pos The new position as an Eina_Position2D structure (contains x and y).
 *            Example: Eina_Position2D pos = { .x = 10, .y = 20 };
 */
void _elm_genlist_pan_efl_gfx_entity_position_set(Eo *obj, Elm_Genlist_Pan_Data *pd, Eina_Position2D pos);

/**
 * @internal
 * @brief Sets the size of the Elm_Genlist_Pan object.
 *
 * This function is part of the Efl_Gfx_Entity interface implementation.
 * It sets the 2D size (width, height) of the pan object.
 *
 * @param obj The Efl_Object whose size is to be set.
 * @param pd The private data of the Elm_Genlist_Pan object.
 * @param size The new size as an Eina_Size2D structure (contains w and h).
 *             Example: Eina_Size2D size = { .w = 100, .h = 200 };
 */
void _elm_genlist_pan_efl_gfx_entity_size_set(Eo *obj, Elm_Genlist_Pan_Data *pd, Eina_Size2D size);

/**
 * @internal
 * @brief Calculates the layout of the Elm_Genlist_Pan object's content.
 *
 * This function is part of the Efl_Canvas_Group interface implementation.
 * It is responsible for triggering the recalculation of the layout
 * of the elements within the pan.
 *
 * @param obj The Efl_Object whose group needs calculation.
 * @param pd The private data of the Elm_Genlist_Pan object.
 */
void _elm_genlist_pan_efl_canvas_group_group_calculate(Eo *obj, Elm_Genlist_Pan_Data *pd);

/**
 * @internal
 * @brief Gets the content size of the Elm_Genlist_Pan object.
 *
 * This function implements the elm_obj_pan_content_size_get method.
 * It retrieves the total width and height of the content managed by the pan.
 *
 * @param obj The Elm_Pan object.
 * @param pd The private data of the Elm_Genlist_Pan object.
 * @param w Pointer to an integer where the content width will be stored.
 * @param h Pointer to an integer where the content height will be stored.
 */
void _elm_genlist_pan_elm_pan_content_size_get(const Eo *obj, Elm_Genlist_Pan_Data *pd, int *w, int *h);

/**
 * @internal
 * @brief Sets the position of the content within the Elm_Genlist_Pan.
 *
 * This function implements the elm_obj_pan_pos_set method.
 * It sets the current scroll position (top-left corner) of the panned content.
 *
 * @param obj The Elm_Pan object.
 * @param pd The private data of the Elm_Genlist_Pan object.
 * @param x The x-coordinate of the content position.
 * @param y The y-coordinate of the content position.
 */
void _elm_genlist_pan_elm_pan_pos_set(Eo *obj, Elm_Genlist_Pan_Data *pd, int x, int y);

/**
 * @internal
 * @brief Gets the current position of the content within the Elm_Genlist_Pan.
 *
 * This function implements the elm_obj_pan_pos_get method.
 * It retrieves the current scroll position (top-left corner) of the panned content.
 *
 * @param obj The Elm_Pan object.
 * @param pd The private data of the Elm_Genlist_Pan object.
 * @param x Pointer to an integer where the x-coordinate of the content position will be stored.
 * @param y Pointer to an integer where the y-coordinate of the content position will be stored.
 */
void _elm_genlist_pan_elm_pan_pos_get(const Eo *obj, Elm_Genlist_Pan_Data *pd, int *x, int *y);

/**
 * @internal
 * @brief Gets the minimum allowed position of the content within the Elm_Genlist_Pan.
 *
 * This function implements the elm_obj_pan_pos_min_get method.
 * It retrieves the minimum scroll position (typically (0,0)).
 *
 * @param obj The Elm_Pan object.
 * @param pd The private data of the Elm_Genlist_Pan object.
 * @param x Pointer to an integer where the minimum x-coordinate will be stored.
 * @param y Pointer to an integer where the minimum y-coordinate will be stored.
 */
void _elm_genlist_pan_elm_pan_pos_min_get(const Eo *obj, Elm_Genlist_Pan_Data *pd, int *x, int *y);

/**
 * @internal
 * @brief Gets the maximum allowed position of the content within the Elm_Genlist_Pan.
 *
 * This function implements the elm_obj_pan_pos_max_get method.
 * It retrieves the maximum scroll position, determined by the content size and pan viewport size.
 *
 * @param obj The Elm_Pan object.
 * @param pd The private data of the Elm_Genlist_Pan object.
 * @param x Pointer to an integer where the maximum x-coordinate will be stored.
 * @param y Pointer to an integer where the maximum y-coordinate will be stored.
 */
void _elm_genlist_pan_elm_pan_pos_max_get(const Eo *obj, Elm_Genlist_Pan_Data *pd, int *x, int *y);


/**
 * @internal
 * @brief Initializes the Elm_Genlist_Pan class.
 *
 * This function is called once when the Elm_Genlist_Pan class is being set up.
 * It defines the operations (methods) for this class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_genlist_pan_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_GENLIST_PAN_EXTRA_OPS
#define ELM_GENLIST_PAN_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_genlist_pan_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _elm_genlist_pan_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_genlist_pan_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_calculate, _elm_genlist_pan_efl_canvas_group_group_calculate),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_content_size_get, _elm_genlist_pan_elm_pan_content_size_get),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_set, _elm_genlist_pan_elm_pan_pos_set),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_get, _elm_genlist_pan_elm_pan_pos_get),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_min_get, _elm_genlist_pan_elm_pan_pos_min_get),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_max_get, _elm_genlist_pan_elm_pan_pos_max_get),
      ELM_GENLIST_PAN_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Genlist_Pan class.
 *
 * This structure holds metadata about the Elm_Genlist_Pan class,
 * including its version, name, type, size of instance data,
 * and pointers to initializer and constructor functions.
 */
static const Efl_Class_Description _elm_genlist_pan_class_desc = {
   EO_VERSION,
   "Elm.Genlist.Pan",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Genlist_Pan_Data),
   _elm_genlist_pan_class_initializer,
   _elm_genlist_pan_class_constructor,
   NULL
};

/**
 * @brief Gets the Efl_Class for Elm_Genlist_Pan.
 *
 * This function is part of the Efl object system and provides access
 * to the class description for Elm_Genlist_Pan. It's typically used
 * internally by EFL or when creating new instances of this class type.
 *
 * @return The Efl_Class for Elm_Genlist_Pan.
 * @ingroup Elm_Genlist_Pan
 */
EFL_DEFINE_CLASS(elm_genlist_pan_class_get, &_elm_genlist_pan_class_desc, ELM_PAN_CLASS, NULL);
