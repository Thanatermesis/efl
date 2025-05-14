/**
 * @file
 * @brief Implementation of the Elm_Gengrid_Pan Eo class.
 */

/**
 * @internal
 * @brief Destructor for the Elm_Gengrid_Pan object.
 * @param obj The Eo object.
 * @param pd The private data of the object.
 */
void _elm_gengrid_pan_efl_object_destructor(Eo *obj, Elm_Gengrid_Pan_Data *pd);

/**
 * @internal
 * @brief Sets the position of the Gengrid Pan.
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @param pos The new position.
 */
void _elm_gengrid_pan_efl_gfx_entity_position_set(Eo *obj, Elm_Gengrid_Pan_Data *pd, Eina_Position2D pos);

/**
 * @internal
 * @brief Sets the size of the Gengrid Pan.
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @param size The new size.
 */
void _elm_gengrid_pan_efl_gfx_entity_size_set(Eo *obj, Elm_Gengrid_Pan_Data *pd, Eina_Size2D size);

/**
 * @internal
 * @brief Calculates the layout of the Gengrid Pan.
 * This function is called when the canvas group needs to recalculate its layout.
 * @param obj The Eo object.
 * @param pd The private data of the object.
 */
void _elm_gengrid_pan_efl_canvas_group_group_calculate(Eo *obj, Elm_Gengrid_Pan_Data *pd);

/**
 * @internal
 * @brief Gets the content size of the Gengrid Pan.
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @param w Pointer to store the width.
 * @param h Pointer to store the height.
 */
void _elm_gengrid_pan_elm_pan_content_size_get(const Eo *obj, Elm_Gengrid_Pan_Data *pd, int *w, int *h);

/**
 * @internal
 * @brief Sets the position of the Gengrid Pan content.
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 */
void _elm_gengrid_pan_elm_pan_pos_set(Eo *obj, Elm_Gengrid_Pan_Data *pd, int x, int y);

/**
 * @internal
 * @brief Gets the position of the Gengrid Pan content.
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @param x Pointer to store the x-coordinate.
 * @param y Pointer to store the y-coordinate.
 */
void _elm_gengrid_pan_elm_pan_pos_get(const Eo *obj, Elm_Gengrid_Pan_Data *pd, int *x, int *y);

/**
 * @internal
 * @brief Gets the minimum position of the Gengrid Pan content.
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @param x Pointer to store the minimum x-coordinate.
 * @param y Pointer to store the minimum y-coordinate.
 */
void _elm_gengrid_pan_elm_pan_pos_min_get(const Eo *obj, Elm_Gengrid_Pan_Data *pd, int *x, int *y);

/**
 * @internal
 * @brief Gets the maximum position of the Gengrid Pan content.
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @param x Pointer to store the maximum x-coordinate.
 * @param y Pointer to store the maximum y-coordinate.
 */
void _elm_gengrid_pan_elm_pan_pos_max_get(const Eo *obj, Elm_Gengrid_Pan_Data *pd, int *x, int *y);

/**
 * @internal
 * @brief Initializes the Elm_Gengrid_Pan class.
 * Sets up the Efl_Object operations for the class.
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_gengrid_pan_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_GENGRID_PAN_EXTRA_OPS
#define ELM_GENGRID_PAN_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_gengrid_pan_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _elm_gengrid_pan_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_gengrid_pan_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_calculate, _elm_gengrid_pan_efl_canvas_group_group_calculate),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_content_size_get, _elm_gengrid_pan_elm_pan_content_size_get),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_set, _elm_gengrid_pan_elm_pan_pos_set),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_get, _elm_gengrid_pan_elm_pan_pos_get),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_min_get, _elm_gengrid_pan_elm_pan_pos_min_get),
      EFL_OBJECT_OP_FUNC(elm_obj_pan_pos_max_get, _elm_gengrid_pan_elm_pan_pos_max_get),
      ELM_GENGRID_PAN_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Gengrid_Pan Eo class.
 * This structure provides metadata about the class, such as its name,
 * version, type, size of instance data, and initializer/constructor functions.
 */
static const Efl_Class_Description _elm_gengrid_pan_class_desc = {
   EO_VERSION, /**< Eo ABI version. */
   "Elm.Gengrid.Pan", /**< Class name. */
   EFL_CLASS_TYPE_REGULAR, /**< Class type. */
   sizeof(Elm_Gengrid_Pan_Data), /**< Size of instance data. */
   _elm_gengrid_pan_class_initializer, /**< Class initializer function. */
   _elm_gengrid_pan_class_constructor, /**< Class constructor function. */
   NULL /**< Class destructor function (unused, handled by efl_destructor). */
};

/**
 * @internal
 * @brief Defines the Elm_Gengrid_Pan class.
 * This macro registers the Elm_Gengrid_Pan class with the Eo system,
 * making it available for instantiation. It links the class description
 * and specifies its parent class (ELM_PAN_CLASS).
 */
EFL_DEFINE_CLASS(elm_gengrid_pan_class_get, &_elm_gengrid_pan_class_desc, ELM_PAN_CLASS, NULL);
