
/**
 * @brief Internal implementation for evas_obj_table_homogeneous_set().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param homogeneous The homogeneous mode to set.
 * @see evas_obj_table_homogeneous_set()
 */
void _evas_table_homogeneous_set(Eo *obj, Evas_Table_Data *pd, Evas_Object_Table_Homogeneous_Mode homogeneous);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_table_homogeneous_set, EFL_FUNC_CALL(homogeneous), Evas_Object_Table_Homogeneous_Mode homogeneous);

/**
 * @brief Internal implementation for evas_obj_table_homogeneous_get().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @return The current homogeneous mode.
 * @see evas_obj_table_homogeneous_get()
 */
Evas_Object_Table_Homogeneous_Mode _evas_table_homogeneous_get(const Eo *obj, Evas_Table_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_table_homogeneous_get, Evas_Object_Table_Homogeneous_Mode, 0);

/**
 * @brief Internal implementation for evas_obj_table_align_set().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param horizontal The horizontal alignment.
 * @param vertical The vertical alignment.
 * @see evas_obj_table_align_set()
 */
void _evas_table_align_set(Eo *obj, Evas_Table_Data *pd, double horizontal, double vertical);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_table_align_set, EFL_FUNC_CALL(horizontal, vertical), double horizontal, double vertical);

/**
 * @brief Internal implementation for evas_obj_table_align_get().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param[out] horizontal Pointer to store the horizontal alignment.
 * @param[out] vertical Pointer to store the vertical alignment.
 * @see evas_obj_table_align_get()
 */
void _evas_table_align_get(const Eo *obj, Evas_Table_Data *pd, double *horizontal, double *vertical);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_table_align_get, EFL_FUNC_CALL(horizontal, vertical), double *horizontal, double *vertical);

/**
 * @brief Internal implementation for evas_obj_table_padding_set().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param horizontal The horizontal padding.
 * @param vertical The vertical padding.
 * @see evas_obj_table_padding_set()
 */
void _evas_table_padding_set(Eo *obj, Evas_Table_Data *pd, int horizontal, int vertical);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_table_padding_set, EFL_FUNC_CALL(horizontal, vertical), int horizontal, int vertical);

/**
 * @brief Internal implementation for evas_obj_table_padding_get().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param[out] horizontal Pointer to store the horizontal padding.
 * @param[out] vertical Pointer to store the vertical padding.
 * @see evas_obj_table_padding_get()
 */
void _evas_table_padding_get(const Eo *obj, Evas_Table_Data *pd, int *horizontal, int *vertical);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_table_padding_get, EFL_FUNC_CALL(horizontal, vertical), int *horizontal, int *vertical);

/**
 * @brief Internal implementation for evas_obj_table_col_row_size_get().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param[out] cols Pointer to store the number of columns.
 * @param[out] rows Pointer to store the number of rows.
 * @see evas_obj_table_col_row_size_get()
 */
void _evas_table_col_row_size_get(const Eo *obj, Evas_Table_Data *pd, int *cols, int *rows);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_table_col_row_size_get, EFL_FUNC_CALL(cols, rows), int *cols, int *rows);

/**
 * @brief Internal implementation for evas_obj_table_children_get().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @return A list of child objects. The caller is responsible for freeing this list.
 * @see evas_obj_table_children_get()
 */
Eina_List *_evas_table_children_get(const Eo *obj, Evas_Table_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_table_children_get, Eina_List *, NULL);

/**
 * @brief Internal implementation for evas_obj_table_child_get().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param col The column of the child.
 * @param row The row of the child.
 * @return The child object at the specified column and row, or @c NULL if not found.
 * @see evas_obj_table_child_get()
 */
Efl_Canvas_Object *_evas_table_child_get(const Eo *obj, Evas_Table_Data *pd, unsigned short col, unsigned short row);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV_CONST(evas_obj_table_child_get, Efl_Canvas_Object *, NULL, EFL_FUNC_CALL(col, row), unsigned short col, unsigned short row);

/**
 * @brief Internal implementation for evas_obj_table_clear().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param clear If @c EINA_TRUE, also delete the child objects.
 * @see evas_obj_table_clear()
 */
void _evas_table_clear(Eo *obj, Evas_Table_Data *pd, Eina_Bool clear);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_table_clear, EFL_FUNC_CALL(clear), Eina_Bool clear);

/**
 * @brief Internal implementation for evas_obj_table_accessor_new().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @return A new Eina_Accessor for the table's children.
 * @see evas_obj_table_accessor_new()
 */
Eina_Accessor *_evas_table_accessor_new(const Eo *obj, Evas_Table_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_table_accessor_new, Eina_Accessor *, NULL);

/**
 * @brief Internal implementation for evas_obj_table_iterator_new().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @return A new Eina_Iterator for the table's children.
 * @see evas_obj_table_iterator_new()
 */
Eina_Iterator *_evas_table_iterator_new(const Eo *obj, Evas_Table_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_table_iterator_new, Eina_Iterator *, NULL);

/**
 * @brief Internal implementation for evas_obj_table_add_to().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @return Typically the Evas_Table object itself or a relevant Efl_Canvas_Object.
 * @see evas_obj_table_add_to()
 */
Efl_Canvas_Object *_evas_table_add_to(Eo *obj, Evas_Table_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY(evas_obj_table_add_to, Efl_Canvas_Object *, NULL);

/**
 * @brief Internal implementation for evas_obj_table_pack_get().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param child The child object.
 * @param[out] col Pointer to store the column.
 * @param[out] row Pointer to store the row.
 * @param[out] colspan Pointer to store the column span.
 * @param[out] rowspan Pointer to store the row span.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 * @see evas_obj_table_pack_get()
 */
Eina_Bool _evas_table_pack_get(const Eo *obj, Evas_Table_Data *pd, Efl_Canvas_Object *child, unsigned short *col, unsigned short *row, unsigned short *colspan, unsigned short *rowspan);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV_CONST(evas_obj_table_pack_get, Eina_Bool, 0, EFL_FUNC_CALL(child, col, row, colspan, rowspan), Efl_Canvas_Object *child, unsigned short *col, unsigned short *row, unsigned short *colspan, unsigned short *rowspan);

/**
 * @brief Internal implementation for evas_obj_table_pack().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param child The child object to pack.
 * @param col The column to pack the child into.
 * @param row The row to pack the child into.
 * @param colspan The number of columns the child should span.
 * @param rowspan The number of rows the child should span.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 * @see evas_obj_table_pack()
 */
Eina_Bool _evas_table_pack(Eo *obj, Evas_Table_Data *pd, Efl_Canvas_Object *child, unsigned short col, unsigned short row, unsigned short colspan, unsigned short rowspan);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_table_pack, Eina_Bool, 0, EFL_FUNC_CALL(child, col, row, colspan, rowspan), Efl_Canvas_Object *child, unsigned short col, unsigned short row, unsigned short colspan, unsigned short rowspan);

/**
 * @brief Internal implementation for evas_obj_table_unpack().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param child The child object to unpack.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 * @see evas_obj_table_unpack()
 */
Eina_Bool _evas_table_unpack(Eo *obj, Evas_Table_Data *pd, Efl_Canvas_Object *child);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_table_unpack, Eina_Bool, 0, EFL_FUNC_CALL(child), Efl_Canvas_Object *child);

/**
 * @brief Internal implementation for evas_obj_table_count().
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @return The number of items in the table.
 * @see evas_obj_table_count()
 */
int _evas_table_count(Eo *obj, Evas_Table_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY(evas_obj_table_count, int, 0);

/**
 * @brief Implements the EFL object constructor for Evas_Table.
 *
 * This function is called when a new Evas_Table object is constructed.
 * It initializes the private data structure @p pd and performs
 * any other necessary setup for a new table instance.
 *
 * @param obj The Evas_Table object being constructed.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @return The constructed Evas_Table object, or @c NULL on failure.
 */
Efl_Object *_evas_table_efl_object_constructor(Eo *obj, Evas_Table_Data *pd);

/**
 * @brief Implements the EFL Gfx Entity size_set method for Evas_Table.
 *
 * This function is called when the size of the Evas_Table object is set.
 * It should trigger recalculation of the table layout if necessary.
 *
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param size The new size (width and height) for the table.
 */
void _evas_table_efl_gfx_entity_size_set(Eo *obj, Evas_Table_Data *pd, Eina_Size2D size);

/**
 * @brief Implements the EFL Gfx Entity position_set method for Evas_Table.
 *
 * This function is called when the position of the Evas_Table object is set.
 *
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param pos The new position (x and y coordinates) for the table.
 */
void _evas_table_efl_gfx_entity_position_set(Eo *obj, Evas_Table_Data *pd, Eina_Position2D pos);

/**
 * @brief Implements the EFL Canvas Group group_calculate method for Evas_Table.
 *
 * This function is responsible for calculating the layout of the child objects
 * within the table. It applies homogeneous mode, padding, alignment, and
 * child-specific packing rules to determine the size and position of each child.
 *
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 */
void _evas_table_efl_canvas_group_group_calculate(Eo *obj, Evas_Table_Data *pd);

/**
 * @brief Implements the EFL UI I18n mirrored_set method for Evas_Table.
 *
 * Sets the mirrored (right-to-left) mode for the table. When mirrored mode
 * is enabled, the layout of columns may be reversed.
 *
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @param rtl @c EINA_TRUE to enable mirrored mode, @c EINA_FALSE to disable.
 */
void _evas_table_efl_ui_i18n_mirrored_set(Eo *obj, Evas_Table_Data *pd, Eina_Bool rtl);

/**
 * @brief Implements the EFL UI I18n mirrored_get method for Evas_Table.
 *
 * Gets the current mirrored (right-to-left) mode of the table.
 *
 * @param obj The Evas_Table object.
 * @param pd Pointer to the private data of the Evas_Table object.
 * @return @c EINA_TRUE if mirrored mode is enabled, @c EINA_FALSE otherwise.
 */
Eina_Bool _evas_table_efl_ui_i18n_mirrored_get(const Eo *obj, Evas_Table_Data *pd);

/**
 * @brief Initializes the Evas_Table class.
 *
 * This function is called by the EO system when the Evas_Table class is
 * first used. It sets up the Efl_Object_Ops structure, mapping the
 * class's methods (operations) to their C implementations.
 *
 * @param klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on successful initialization, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_evas_table_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EVAS_TABLE_EXTRA_OPS
#define EVAS_TABLE_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(evas_obj_table_homogeneous_set, _evas_table_homogeneous_set),
      EFL_OBJECT_OP_FUNC(evas_obj_table_homogeneous_get, _evas_table_homogeneous_get),
      EFL_OBJECT_OP_FUNC(evas_obj_table_align_set, _evas_table_align_set),
      EFL_OBJECT_OP_FUNC(evas_obj_table_align_get, _evas_table_align_get),
      EFL_OBJECT_OP_FUNC(evas_obj_table_padding_set, _evas_table_padding_set),
      EFL_OBJECT_OP_FUNC(evas_obj_table_padding_get, _evas_table_padding_get),
      EFL_OBJECT_OP_FUNC(evas_obj_table_col_row_size_get, _evas_table_col_row_size_get),
      EFL_OBJECT_OP_FUNC(evas_obj_table_children_get, _evas_table_children_get),
      EFL_OBJECT_OP_FUNC(evas_obj_table_child_get, _evas_table_child_get),
      EFL_OBJECT_OP_FUNC(evas_obj_table_clear, _evas_table_clear),
      EFL_OBJECT_OP_FUNC(evas_obj_table_accessor_new, _evas_table_accessor_new),
      EFL_OBJECT_OP_FUNC(evas_obj_table_iterator_new, _evas_table_iterator_new),
      EFL_OBJECT_OP_FUNC(evas_obj_table_add_to, _evas_table_add_to),
      EFL_OBJECT_OP_FUNC(evas_obj_table_pack_get, _evas_table_pack_get),
      EFL_OBJECT_OP_FUNC(evas_obj_table_pack, _evas_table_pack),
      EFL_OBJECT_OP_FUNC(evas_obj_table_unpack, _evas_table_unpack),
      EFL_OBJECT_OP_FUNC(evas_obj_table_count, _evas_table_count),
      EFL_OBJECT_OP_FUNC(efl_constructor, _evas_table_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _evas_table_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _evas_table_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_calculate, _evas_table_efl_canvas_group_group_calculate),
      EFL_OBJECT_OP_FUNC(efl_ui_mirrored_set, _evas_table_efl_ui_i18n_mirrored_set),
      EFL_OBJECT_OP_FUNC(efl_ui_mirrored_get, _evas_table_efl_ui_i18n_mirrored_get),
      EVAS_TABLE_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _evas_table_class_desc = {
   EO_VERSION,
   "Evas.Table",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Evas_Table_Data),
   _evas_table_class_initializer,
   _evas_table_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(evas_table_class_get, &_evas_table_class_desc, EFL_CANVAS_GROUP_CLASS, EFL_UI_I18N_INTERFACE, NULL);

#include "evas_table_eo.legacy.c"
