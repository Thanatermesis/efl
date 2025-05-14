EVAS_API EVAS_API_WEAK const Efl_Event_Description _EVAS_BOX_EVENT_CHILD_ADDED =
   EFL_EVENT_DESCRIPTION("child,added");
EVAS_API EVAS_API_WEAK const Efl_Event_Description _EVAS_BOX_EVENT_CHILD_REMOVED =
   EFL_EVENT_DESCRIPTION("child,removed");

/**
 * @internal
 * @brief Internal implementation for evas_obj_box_align_set().
 * This function contains the actual logic for setting the box alignment.
 * @param obj The Evas_Box object.
 * @param pd Private data for the Evas_Box object.
 * @param horizontal The horizontal alignment (0.0 to 1.0).
 * @param vertical The vertical alignment (0.0 to 1.0).
 */
void _evas_box_align_set(Eo *obj, Evas_Object_Box_Data *pd, double horizontal, double vertical);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_box_align_set, EFL_FUNC_CALL(horizontal, vertical), double horizontal, double vertical);

/**
 * @internal
 * @brief Internal implementation for evas_obj_box_align_get().
 * This function contains the actual logic for getting the box alignment.
 * @param obj The Evas_Box object.
 * @param pd Private data for the Evas_Box object.
 * @param[out] horizontal Pointer to store the horizontal alignment.
 * @param[out] vertical Pointer to store the vertical alignment.
 */
void _evas_box_align_get(const Eo *obj, Evas_Object_Box_Data *pd, double *horizontal, double *vertical);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_box_align_get, EFL_FUNC_CALL(horizontal, vertical), double *horizontal, double *vertical);

/**
 * @internal
 * @brief Internal implementation for evas_obj_box_padding_set().
 * This function contains the actual logic for setting the box padding.
 * @param obj The Evas_Box object.
 * @param pd Private data for the Evas_Box object.
 * @param horizontal The horizontal padding in pixels.
 * @param vertical The vertical padding in pixels.
 */
void _evas_box_padding_set(Eo *obj, Evas_Object_Box_Data *pd, int horizontal, int vertical);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_box_padding_set, EFL_FUNC_CALL(horizontal, vertical), int horizontal, int vertical);

/**
 * @internal
 * @brief Internal implementation for evas_obj_box_padding_get().
 * This function contains the actual logic for getting the box padding.
 * @param obj The Evas_Box object.
 * @param pd Private data for the Evas_Box object.
 * @param[out] horizontal Pointer to store the horizontal padding.
 * @param[out] vertical Pointer to store the vertical padding.
 */
void _evas_box_padding_get(const Eo *obj, Evas_Object_Box_Data *pd, int *horizontal, int *vertical);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_box_padding_get, EFL_FUNC_CALL(horizontal, vertical), int *horizontal, int *vertical);

/**
 * @internal
 * @brief Internal implementation for evas_obj_box_layout_set().
 * This function contains the actual logic for setting the box layout function.
 * @param obj The Evas_Box object.
 * @param pd Private data for the Evas_Box object.
 * @param cb The layout callback function.
 * @param data User data for the callback.
 * @param free_data Callback to free the user data.
 */
void _evas_box_layout_set(Eo *obj, Evas_Object_Box_Data *pd, Evas_Object_Box_Layout cb, const void *data, Eina_Free_Cb free_data);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_box_layout_set, EFL_FUNC_CALL(cb, data, free_data), Evas_Object_Box_Layout cb, const void *data, Eina_Free_Cb free_data);

/**
 * @internal
 * @brief Internal implementation for evas_obj_box_layout_horizontal().
 * This function performs the horizontal layout logic.
 * @param obj The Evas_Box object.
 * @param pd Private data for the Evas_Box object. (Note: API shows priv, pd is likely the Eo private data)
 * @param priv The Evas_Object_Box_Data specific to the layout operation.
 * @param data User data associated with the layout.
 */
void _evas_box_layout_horizontal(Eo *obj, Evas_Object_Box_Data *pd, Evas_Object_Box_Data *priv, void *data);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_box_layout_horizontal, EFL_FUNC_CALL(priv, data), Evas_Object_Box_Data *priv, void *data);

/**
 * @internal
 * @brief Internal implementation for evas_obj_box_layout_vertical().
 * This function performs the vertical layout logic.
 * @param obj The Evas_Box object.
 * @param pd Private data for the Evas_Box object.
 * @param priv The Evas_Object_Box_Data specific to the layout operation.
 * @param data User data associated with the layout.
 */
void _evas_box_layout_vertical(Eo *obj, Evas_Object_Box_Data *pd, Evas_Object_Box_Data *priv, void *data);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_box_layout_vertical, EFL_FUNC_CALL(priv, data), Evas_Object_Box_Data *priv, void *data);

void _evas_box_layout_homogeneous_max_size_horizontal(Eo *obj, Evas_Object_Box_Data *pd, Evas_Object_Box_Data *priv, void *data);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_box_layout_homogeneous_max_size_horizontal, EFL_FUNC_CALL(priv, data), Evas_Object_Box_Data *priv, void *data);

Efl_Canvas_Object *_evas_box_internal_remove(Eo *obj, Evas_Object_Box_Data *pd, Efl_Canvas_Object *child);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_internal_remove, Efl_Canvas_Object *, NULL, EFL_FUNC_CALL(child), Efl_Canvas_Object *child);

void _evas_box_layout_flow_vertical(Eo *obj, Evas_Object_Box_Data *pd, Evas_Object_Box_Data *priv, void *data);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_box_layout_flow_vertical, EFL_FUNC_CALL(priv, data), Evas_Object_Box_Data *priv, void *data);

void _evas_box_internal_option_free(Eo *obj, Evas_Object_Box_Data *pd, Evas_Object_Box_Option *opt);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_box_internal_option_free, EFL_FUNC_CALL(opt), Evas_Object_Box_Option *opt);

Evas_Object_Box_Option *_evas_box_insert_after(Eo *obj, Evas_Object_Box_Data *pd, Efl_Canvas_Object *child, const Efl_Canvas_Object *reference);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_insert_after, Evas_Object_Box_Option *, NULL, EFL_FUNC_CALL(child, reference), Efl_Canvas_Object *child, const Efl_Canvas_Object *reference);

Eina_Bool _evas_box_remove_all(Eo *obj, Evas_Object_Box_Data *pd, Eina_Bool clear);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_remove_all, Eina_Bool, 0, EFL_FUNC_CALL(clear), Eina_Bool clear);

Eina_Iterator *_evas_box_iterator_new(const Eo *obj, Evas_Object_Box_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_box_iterator_new, Eina_Iterator *, NULL);

Efl_Canvas_Object *_evas_box_add_to(Eo *obj, Evas_Object_Box_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY(evas_obj_box_add_to, Efl_Canvas_Object *, NULL);

Evas_Object_Box_Option *_evas_box_append(Eo *obj, Evas_Object_Box_Data *pd, Efl_Canvas_Object *child);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_append, Evas_Object_Box_Option *, NULL, EFL_FUNC_CALL(child), Efl_Canvas_Object *child);

int _evas_box_option_property_id_get(const Eo *obj, Evas_Object_Box_Data *pd, const char *name);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV_CONST(evas_obj_box_option_property_id_get, int, 0, EFL_FUNC_CALL(name), const char *name);

Evas_Object_Box_Option *_evas_box_prepend(Eo *obj, Evas_Object_Box_Data *pd, Efl_Canvas_Object *child);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_prepend, Evas_Object_Box_Option *, NULL, EFL_FUNC_CALL(child), Efl_Canvas_Object *child);

Eina_Accessor *_evas_box_accessor_new(const Eo *obj, Evas_Object_Box_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_box_accessor_new, Eina_Accessor *, NULL);

Evas_Object_Box_Option *_evas_box_internal_append(Eo *obj, Evas_Object_Box_Data *pd, Efl_Canvas_Object *child);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_internal_append, Evas_Object_Box_Option *, NULL, EFL_FUNC_CALL(child), Efl_Canvas_Object *child);

Eina_Bool _evas_box_option_property_vset(Eo *obj, Evas_Object_Box_Data *pd, Evas_Object_Box_Option *opt, int property, va_list *args);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_option_property_vset, Eina_Bool, 0, EFL_FUNC_CALL(opt, property, args), Evas_Object_Box_Option *opt, int property, va_list *args);

Efl_Canvas_Object *_evas_box_internal_remove_at(Eo *obj, Evas_Object_Box_Data *pd, unsigned int pos);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_internal_remove_at, Efl_Canvas_Object *, NULL, EFL_FUNC_CALL(pos), unsigned int pos);

Eina_Bool _evas_box_remove_at(Eo *obj, Evas_Object_Box_Data *pd, unsigned int pos);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_remove_at, Eina_Bool, 0, EFL_FUNC_CALL(pos), unsigned int pos);

Eina_Bool _evas_box_option_property_vget(const Eo *obj, Evas_Object_Box_Data *pd, Evas_Object_Box_Option *opt, int property, va_list *args);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV_CONST(evas_obj_box_option_property_vget, Eina_Bool, 0, EFL_FUNC_CALL(opt, property, args), Evas_Object_Box_Option *opt, int property, va_list *args);

Evas_Object_Box_Option *_evas_box_internal_insert_at(Eo *obj, Evas_Object_Box_Data *pd, Efl_Canvas_Object *child, unsigned int pos);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_internal_insert_at, Evas_Object_Box_Option *, NULL, EFL_FUNC_CALL(child, pos), Efl_Canvas_Object *child, unsigned int pos);

Evas_Object_Box_Option *_evas_box_insert_before(Eo *obj, Evas_Object_Box_Data *pd, Efl_Canvas_Object *child, const Efl_Canvas_Object *reference);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_insert_before, Evas_Object_Box_Option *, NULL, EFL_FUNC_CALL(child, reference), Efl_Canvas_Object *child, const Efl_Canvas_Object *reference);

const char *_evas_box_option_property_name_get(const Eo *obj, Evas_Object_Box_Data *pd, int property);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV_CONST(evas_obj_box_option_property_name_get, const char *, NULL, EFL_FUNC_CALL(property), int property);

Evas_Object_Box_Option *_evas_box_internal_insert_before(Eo *obj, Evas_Object_Box_Data *pd, Efl_Canvas_Object *child, const Efl_Canvas_Object *reference);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_internal_insert_before, Evas_Object_Box_Option *, NULL, EFL_FUNC_CALL(child, reference), Efl_Canvas_Object *child, const Efl_Canvas_Object *reference);

void _evas_box_layout_homogeneous_horizontal(Eo *obj, Evas_Object_Box_Data *pd, Evas_Object_Box_Data *priv, void *data);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_box_layout_homogeneous_horizontal, EFL_FUNC_CALL(priv, data), Evas_Object_Box_Data *priv, void *data);

Evas_Object_Box_Option *_evas_box_internal_option_new(Eo *obj, Evas_Object_Box_Data *pd, Efl_Canvas_Object *child);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_internal_option_new, Evas_Object_Box_Option *, NULL, EFL_FUNC_CALL(child), Efl_Canvas_Object *child);

void _evas_box_layout_homogeneous_max_size_vertical(Eo *obj, Evas_Object_Box_Data *pd, Evas_Object_Box_Data *priv, void *data);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_box_layout_homogeneous_max_size_vertical, EFL_FUNC_CALL(priv, data), Evas_Object_Box_Data *priv, void *data);

Evas_Object_Box_Option *_evas_box_internal_insert_after(Eo *obj, Evas_Object_Box_Data *pd, Efl_Canvas_Object *child, const Efl_Canvas_Object *reference);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_internal_insert_after, Evas_Object_Box_Option *, NULL, EFL_FUNC_CALL(child, reference), Efl_Canvas_Object *child, const Efl_Canvas_Object *reference);

Evas_Object_Box_Option *_evas_box_insert_at(Eo *obj, Evas_Object_Box_Data *pd, Efl_Canvas_Object *child, unsigned int pos);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_insert_at, Evas_Object_Box_Option *, NULL, EFL_FUNC_CALL(child, pos), Efl_Canvas_Object *child, unsigned int pos);

Evas_Object_Box_Option *_evas_box_internal_prepend(Eo *obj, Evas_Object_Box_Data *pd, Efl_Canvas_Object *child);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_internal_prepend, Evas_Object_Box_Option *, NULL, EFL_FUNC_CALL(child), Efl_Canvas_Object *child);

Eina_Bool _evas_box_remove(Eo *obj, Evas_Object_Box_Data *pd, Efl_Canvas_Object *child);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_box_remove, Eina_Bool, 0, EFL_FUNC_CALL(child), Efl_Canvas_Object *child);

void _evas_box_layout_stack(Eo *obj, Evas_Object_Box_Data *pd, Evas_Object_Box_Data *priv, void *data);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_box_layout_stack, EFL_FUNC_CALL(priv, data), Evas_Object_Box_Data *priv, void *data);

void _evas_box_layout_homogeneous_vertical(Eo *obj, Evas_Object_Box_Data *pd, Evas_Object_Box_Data *priv, void *data);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_box_layout_homogeneous_vertical, EFL_FUNC_CALL(priv, data), Evas_Object_Box_Data *priv, void *data);

void _evas_box_layout_flow_horizontal(Eo *obj, Evas_Object_Box_Data *pd, Evas_Object_Box_Data *priv, void *data);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_box_layout_flow_horizontal, EFL_FUNC_CALL(priv, data), Evas_Object_Box_Data *priv, void *data);

int _evas_box_count(Eo *obj, Evas_Object_Box_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY(evas_obj_box_count, int, 0);

Efl_Object *_evas_box_efl_object_constructor(Eo *obj, Evas_Object_Box_Data *pd);


void _evas_box_efl_gfx_entity_size_set(Eo *obj, Evas_Object_Box_Data *pd, Eina_Size2D size);


void _evas_box_efl_gfx_entity_position_set(Eo *obj, Evas_Object_Box_Data *pd, Eina_Position2D pos);


void _evas_box_efl_canvas_group_group_calculate(Eo *obj, Evas_Object_Box_Data *pd);

/**
 * @internal
 * @brief Initializes the Evas_Box Efl_Class.
 *
 * This function is called by the EO system when the Evas_Box class is
 * first loaded. It registers the operations (methods) that Evas_Box implements,
 * mapping the public C API function names (e.g., evas_obj_box_align_set)
 * to their corresponding internal implementation functions (e.g., _evas_box_align_set).
 *
 * @param klass The Evas_Box Efl_Class structure to initialize.
 * @return @c EINA_TRUE on successful initialization, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_evas_box_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EVAS_BOX_EXTRA_OPS
#define EVAS_BOX_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(evas_obj_box_align_set, _evas_box_align_set),
      EFL_OBJECT_OP_FUNC(evas_obj_box_align_get, _evas_box_align_get),
      EFL_OBJECT_OP_FUNC(evas_obj_box_padding_set, _evas_box_padding_set),
      EFL_OBJECT_OP_FUNC(evas_obj_box_padding_get, _evas_box_padding_get),
      EFL_OBJECT_OP_FUNC(evas_obj_box_layout_set, _evas_box_layout_set),
      EFL_OBJECT_OP_FUNC(evas_obj_box_layout_horizontal, _evas_box_layout_horizontal),
      EFL_OBJECT_OP_FUNC(evas_obj_box_layout_vertical, _evas_box_layout_vertical),
      EFL_OBJECT_OP_FUNC(evas_obj_box_layout_homogeneous_max_size_horizontal, _evas_box_layout_homogeneous_max_size_horizontal),
      EFL_OBJECT_OP_FUNC(evas_obj_box_internal_remove, _evas_box_internal_remove),
      EFL_OBJECT_OP_FUNC(evas_obj_box_layout_flow_vertical, _evas_box_layout_flow_vertical),
      EFL_OBJECT_OP_FUNC(evas_obj_box_internal_option_free, _evas_box_internal_option_free),
      EFL_OBJECT_OP_FUNC(evas_obj_box_insert_after, _evas_box_insert_after),
      EFL_OBJECT_OP_FUNC(evas_obj_box_remove_all, _evas_box_remove_all),
      EFL_OBJECT_OP_FUNC(evas_obj_box_iterator_new, _evas_box_iterator_new),
      EFL_OBJECT_OP_FUNC(evas_obj_box_add_to, _evas_box_add_to),
      EFL_OBJECT_OP_FUNC(evas_obj_box_append, _evas_box_append),
      EFL_OBJECT_OP_FUNC(evas_obj_box_option_property_id_get, _evas_box_option_property_id_get),
      EFL_OBJECT_OP_FUNC(evas_obj_box_prepend, _evas_box_prepend),
      EFL_OBJECT_OP_FUNC(evas_obj_box_accessor_new, _evas_box_accessor_new),
      EFL_OBJECT_OP_FUNC(evas_obj_box_internal_append, _evas_box_internal_append),
      EFL_OBJECT_OP_FUNC(evas_obj_box_option_property_vset, _evas_box_option_property_vset),
      EFL_OBJECT_OP_FUNC(evas_obj_box_internal_remove_at, _evas_box_internal_remove_at),
      EFL_OBJECT_OP_FUNC(evas_obj_box_remove_at, _evas_box_remove_at),
      EFL_OBJECT_OP_FUNC(evas_obj_box_option_property_vget, _evas_box_option_property_vget),
      EFL_OBJECT_OP_FUNC(evas_obj_box_internal_insert_at, _evas_box_internal_insert_at),
      EFL_OBJECT_OP_FUNC(evas_obj_box_insert_before, _evas_box_insert_before),
      EFL_OBJECT_OP_FUNC(evas_obj_box_option_property_name_get, _evas_box_option_property_name_get),
      EFL_OBJECT_OP_FUNC(evas_obj_box_internal_insert_before, _evas_box_internal_insert_before),
      EFL_OBJECT_OP_FUNC(evas_obj_box_layout_homogeneous_horizontal, _evas_box_layout_homogeneous_horizontal),
      EFL_OBJECT_OP_FUNC(evas_obj_box_internal_option_new, _evas_box_internal_option_new),
      EFL_OBJECT_OP_FUNC(evas_obj_box_layout_homogeneous_max_size_vertical, _evas_box_layout_homogeneous_max_size_vertical),
      EFL_OBJECT_OP_FUNC(evas_obj_box_internal_insert_after, _evas_box_internal_insert_after),
      EFL_OBJECT_OP_FUNC(evas_obj_box_insert_at, _evas_box_insert_at),
      EFL_OBJECT_OP_FUNC(evas_obj_box_internal_prepend, _evas_box_internal_prepend),
      EFL_OBJECT_OP_FUNC(evas_obj_box_remove, _evas_box_remove),
      EFL_OBJECT_OP_FUNC(evas_obj_box_layout_stack, _evas_box_layout_stack),
      EFL_OBJECT_OP_FUNC(evas_obj_box_layout_homogeneous_vertical, _evas_box_layout_homogeneous_vertical),
      EFL_OBJECT_OP_FUNC(evas_obj_box_layout_flow_horizontal, _evas_box_layout_flow_horizontal),
      EFL_OBJECT_OP_FUNC(evas_obj_box_count, _evas_box_count),
      EFL_OBJECT_OP_FUNC(efl_constructor, _evas_box_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _evas_box_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _evas_box_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_calculate, _evas_box_efl_canvas_group_group_calculate),
      EVAS_BOX_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Constant descriptor for the Evas_Box Efl_Class.
 *
 * This structure provides metadata about the Evas_Box class to the EO system.
 * It includes:
 * - EO_VERSION: The EO API version this class complies with.
 * - "Evas.Box": The unique name of the class.
 * - EFL_CLASS_TYPE_REGULAR: Specifies that this is a regular, instantiable class.
 * - sizeof(Evas_Object_Box_Data): The size of the private data structure for instances of this class.
 * - _evas_box_class_initializer: Pointer to the class initializer function.
 * - _evas_box_class_constructor: Pointer to the class constructor function (called for new instances).
 * - NULL: Pointer to the class destructor function (unused here).
 */
static const Efl_Class_Description _evas_box_class_desc = {
   EO_VERSION,
   "Evas.Box",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Evas_Object_Box_Data),
   _evas_box_class_initializer,
   _evas_box_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(evas_box_class_get, &_evas_box_class_desc, EFL_CANVAS_GROUP_CLASS, NULL);

#include "evas_box_eo.legacy.c"
