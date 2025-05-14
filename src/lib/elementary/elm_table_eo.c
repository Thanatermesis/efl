
/**
 * @internal
 * @brief Implements the elm_obj_table_homogeneous_set logic.
 *
 * This function is the C implementation called by the Eo layer
 * for elm_obj_table_homogeneous_set(). It sets the homogeneous mode
 * for the table, affecting how children are sized.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @param homogeneous @c EINA_TRUE to enable homogeneous mode, @c EINA_FALSE to disable.
 * @see elm_obj_table_homogeneous_set()
 */
void _elm_table_homogeneous_set(Eo *obj, void *pd, Eina_Bool homogeneous);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_table_homogeneous_set property.
 *
 * This function is called by the Eolian reflection system to set the 'homogeneous'
 * property from an Eina_Value. It converts the Eina_Value to a boolean and
 * then calls elm_obj_table_homogeneous_set().
 *
 * @param[in] obj The Evas object associated with this operation.
 * @param[in] val An Eina_Value containing the boolean value to set for homogeneity.
 * @return EINA_ERROR_NO_ERROR on success, or an Eina_Error code on failure (e.g., EINA_ERROR_VALUE_FAILED if conversion fails).
 */
static Eina_Error
__eolian_elm_table_homogeneous_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_table_homogeneous_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_table_homogeneous_set, EFL_FUNC_CALL(homogeneous), Eina_Bool homogeneous);

/**
 * @internal
 * @brief Implements the elm_obj_table_homogeneous_get logic.
 *
 * This function is the C implementation called by the Eo layer
 * for elm_obj_table_homogeneous_get(). It retrieves the homogeneous mode
 * of the table.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @return @c EINA_TRUE if homogeneous mode is enabled, @c EINA_FALSE otherwise.
 * @see elm_obj_table_homogeneous_get()
 */
Eina_Bool _elm_table_homogeneous_get(const Eo *obj, void *pd);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_table_homogeneous_get property.
 *
 * This function is called by the Eolian reflection system to get the 'homogeneous'
 * property and return it as an Eina_Value. It calls elm_obj_table_homogeneous_get()
 * and then initializes an Eina_Value with the boolean result.
 *
 * @param[in] obj The Evas object associated with this operation.
 * @return An Eina_Value initialized with the boolean state of the 'homogeneous' property.
 *         The caller is responsible for flushing this Eina_Value.
 */
static Eina_Value
__eolian_elm_table_homogeneous_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_table_homogeneous_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_table_homogeneous_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Implements the elm_obj_table_padding_set logic.
 *
 * Sets the padding between cells in the table.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @param horizontal The horizontal padding value.
 * @param vertical The vertical padding value.
 * @see elm_obj_table_padding_set()
 */
void _elm_table_padding_set(Eo *obj, void *pd, int horizontal, int vertical);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_table_padding_set, EFL_FUNC_CALL(horizontal, vertical), int horizontal, int vertical);

/**
 * @internal
 * @brief Implements the elm_obj_table_padding_get logic.
 *
 * Retrieves the padding between cells in the table.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @param[out] horizontal Pointer to store the horizontal padding value.
 * @param[out] vertical Pointer to store the vertical padding value.
 * @see elm_obj_table_padding_get()
 */
void _elm_table_padding_get(const Eo *obj, void *pd, int *horizontal, int *vertical);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_table_padding_get, EFL_FUNC_CALL(horizontal, vertical), int *horizontal, int *vertical);

/**
 * @internal
 * @brief Implements the elm_obj_table_align_set logic.
 *
 * Sets the alignment of the table's content.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @param horizontal The horizontal alignment value (0.0 to 1.0).
 * @param vertical The vertical alignment value (0.0 to 1.0).
 * @see elm_obj_table_align_set()
 */
void _elm_table_align_set(Eo *obj, void *pd, double horizontal, double vertical);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_table_align_set, EFL_FUNC_CALL(horizontal, vertical), double horizontal, double vertical);

/**
 * @internal
 * @brief Implements the elm_obj_table_align_get logic.
 *
 * Retrieves the alignment of the table's content.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @param[out] horizontal Pointer to store the horizontal alignment.
 * @param[out] vertical Pointer to store the vertical alignment.
 * @see elm_obj_table_align_get()
 */
void _elm_table_align_get(const Eo *obj, void *pd, double *horizontal, double *vertical);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_table_align_get, EFL_FUNC_CALL(horizontal, vertical), double *horizontal, double *vertical);

/**
 * @internal
 * @brief Implements the elm_obj_table_clear logic.
 *
 * Removes all child objects from the table.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @param clear If @c EINA_TRUE, child objects are deleted; otherwise, they are just unpacked.
 * @see elm_obj_table_clear()
 */
void _elm_table_clear(Eo *obj, void *pd, Eina_Bool clear);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_table_clear, EFL_FUNC_CALL(clear), Eina_Bool clear);

/**
 * @internal
 * @brief Implements the elm_obj_table_child_get logic.
 *
 * Retrieves the child object at the specified column and row.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @param col The column index.
 * @param row The row index.
 * @return The child object at the given coordinates, or @c NULL if not found.
 * @see elm_obj_table_child_get()
 */
Efl_Canvas_Object *_elm_table_child_get(const Eo *obj, void *pd, int col, int row);

EOAPI EFL_FUNC_BODYV_CONST(elm_obj_table_child_get, Efl_Canvas_Object *, NULL, EFL_FUNC_CALL(col, row), int col, int row);

/**
 * @internal
 * @brief Implements the elm_obj_table_pack_set logic.
 *
 * Sets the packing parameters for an existing child object in the table.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @param subobj The child object whose packing is to be set.
 * @param column The new column for the child.
 * @param row The new row for the child.
 * @param colspan The new column span for the child.
 * @param rowspan The new row span for the child.
 * @see elm_obj_table_pack_set()
 */
void _elm_table_pack_set(Eo *obj, void *pd, Efl_Canvas_Object *subobj, int column, int row, int colspan, int rowspan);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_table_pack_set, EFL_FUNC_CALL(subobj, column, row, colspan, rowspan), Efl_Canvas_Object *subobj, int column, int row, int colspan, int rowspan);

/**
 * @internal
 * @brief Implements the elm_obj_table_pack_get logic.
 *
 * Retrieves the packing parameters of a child object in the table.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @param subobj The child object whose packing is to be retrieved.
 * @param[out] column Pointer to store the column.
 * @param[out] row Pointer to store the row.
 * @param[out] colspan Pointer to store the column span.
 * @param[out] rowspan Pointer to store the row span.
 * @see elm_obj_table_pack_get()
 */
void _elm_table_pack_get(Eo *obj, void *pd, Efl_Canvas_Object *subobj, int *column, int *row, int *colspan, int *rowspan);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_table_pack_get, EFL_FUNC_CALL(subobj, column, row, colspan, rowspan), Efl_Canvas_Object *subobj, int *column, int *row, int *colspan, int *rowspan);

/**
 * @internal
 * @brief Implements the elm_obj_table_unpack logic.
 *
 * Removes a child object from the table.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @param subobj The child object to unpack.
 * @see elm_obj_table_unpack()
 */
void _elm_table_unpack(Eo *obj, void *pd, Efl_Canvas_Object *subobj);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_table_unpack, EFL_FUNC_CALL(subobj), Efl_Canvas_Object *subobj);

/**
 * @internal
 * @brief Implements the elm_obj_table_pack logic.
 *
 * Adds a child object to the table at the specified packing position.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @param subobj The child object to pack.
 * @param column The column to pack the child into.
 * @param row The row to pack the child into.
 * @param colspan The number of columns the child should span.
 * @param rowspan The number of rows the child should span.
 * @see elm_obj_table_pack()
 */
void _elm_table_pack(Eo *obj, void *pd, Efl_Canvas_Object *subobj, int column, int row, int colspan, int rowspan);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_table_pack, EFL_FUNC_CALL(subobj, column, row, colspan, rowspan), Efl_Canvas_Object *subobj, int column, int row, int colspan, int rowspan);

/**
 * @internal
 * @brief Implements the Efl.Object.constructor for Elm.Table.
 *
 * This function is the C implementation called by the Eo layer for
 * efl_constructor(). It performs initial setup for a new table object.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object being constructed.
 * @param pd The private data for the table object.
 * @return The constructed Efl_Object, typically @p obj after initialization.
 * @see efl_constructor()
 */
Efl_Object *_elm_table_efl_object_constructor(Eo *obj, void *pd);

/**
 * @internal
 * @brief Implements the Efl.Canvas.Group.group_calculate for Elm.Table.
 *
 * This function is the C implementation called by the Eo layer for
 * efl_canvas_group_calculate(). It handles the size calculation and layout
 * of the table's children.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @see efl_canvas_group_calculate()
 */
void _elm_table_efl_canvas_group_group_calculate(Eo *obj, void *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.theme_apply for Elm.Table.
 *
 * This function is the C implementation called by the Eo layer for
 * efl_ui_widget_theme_apply(). It applies the current theme to the table.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @return EINA_ERROR_NO_ERROR on success, or an Eina_Error code on failure.
 * @see efl_ui_widget_theme_apply()
 */
Eina_Error _elm_table_efl_ui_widget_theme_apply(Eo *obj, void *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.widget_sub_object_del for Elm.Table.
 *
 * This function is the C implementation called by the Eo layer for
 * efl_ui_widget_sub_object_del(). It handles the deletion of a sub-object
 * from the table.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @param sub_obj The sub-object to be deleted.
 * @return @c EINA_TRUE if the sub-object was successfully handled, @c EINA_FALSE otherwise.
 * @see efl_ui_widget_sub_object_del()
 */
Eina_Bool _elm_table_efl_ui_widget_widget_sub_object_del(Eo *obj, void *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Implements the Efl.Ui.Focus.Composition.prepare for Elm.Table.
 *
 * This function is the C implementation called by the Eo layer for
 * efl_ui_focus_composition_prepare(). It prepares the table for focus composition.
 * The definition of this function is expected in the widget's main C file.
 *
 * @param obj The table object.
 * @param pd The private data for the table object.
 * @see efl_ui_focus_composition_prepare()
 */
void _elm_table_efl_ui_focus_composition_prepare(Eo *obj, void *pd);

/**
 * @internal
 * @brief Initializes the Elm.Table Efl_Class.
 *
 * This function is called once when the Elm.Table class is being set up.
 * It defines the Efl_Object operations (ops) and property reflections
 * for the class by populating the ops and ropsp structures.
 *
 * @param[in] klass The Efl_Class to initialize for Elm.Table.
 * @return @c EINA_TRUE on successful initialization, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_table_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_TABLE_EXTRA_OPS
#define ELM_TABLE_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_table_homogeneous_set, _elm_table_homogeneous_set),
      EFL_OBJECT_OP_FUNC(elm_obj_table_homogeneous_get, _elm_table_homogeneous_get),
      EFL_OBJECT_OP_FUNC(elm_obj_table_padding_set, _elm_table_padding_set),
      EFL_OBJECT_OP_FUNC(elm_obj_table_padding_get, _elm_table_padding_get),
      EFL_OBJECT_OP_FUNC(elm_obj_table_align_set, _elm_table_align_set),
      EFL_OBJECT_OP_FUNC(elm_obj_table_align_get, _elm_table_align_get),
      EFL_OBJECT_OP_FUNC(elm_obj_table_clear, _elm_table_clear),
      EFL_OBJECT_OP_FUNC(elm_obj_table_child_get, _elm_table_child_get),
      EFL_OBJECT_OP_FUNC(elm_obj_table_pack_set, _elm_table_pack_set),
      EFL_OBJECT_OP_FUNC(elm_obj_table_pack_get, _elm_table_pack_get),
      EFL_OBJECT_OP_FUNC(elm_obj_table_unpack, _elm_table_unpack),
      EFL_OBJECT_OP_FUNC(elm_obj_table_pack, _elm_table_pack),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_table_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_calculate, _elm_table_efl_canvas_group_group_calculate),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_table_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_sub_object_del, _elm_table_efl_ui_widget_widget_sub_object_del),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_composition_prepare, _elm_table_efl_ui_focus_composition_prepare),
      ELM_TABLE_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"homogeneous", __eolian_elm_table_homogeneous_set_reflect, __eolian_elm_table_homogeneous_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_table_class_desc = {
   EO_VERSION,
   "Elm.Table",
   EFL_CLASS_TYPE_REGULAR,
   0,
   _elm_table_class_initializer,
   _elm_table_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_table_class_get, &_elm_table_class_desc, EFL_UI_WIDGET_CLASS, EFL_UI_FOCUS_COMPOSITION_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_table_eo.legacy.c"
