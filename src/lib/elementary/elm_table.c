#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

/**
 * @internal
 * @addtogroup Widget
 * @{
 *
 * @section elm-table-class The Elm_Table Class
 *
 * Elementary table widget.
 *
 * A table is a widget that arranges its children (sub-objects) in a
 * grid. It can have an arbitrary number of rows and columns. Children
 * can span multiple rows or columns.
 *
 * This is the internal implementation of the Elm_Table widget.
 */

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_UI_FOCUS_COMPOSITION_PROTECTED

#include <Elementary.h>
#include <elm_table_eo.h>
#include "elm_priv.h"
#include "elm_widget_table.h"

#define MY_CLASS ELM_TABLE_CLASS

#define MY_CLASS_NAME "Elm_Table"
#define MY_CLASS_NAME_LEGACY "elm_table"

/**
 * @internal
 * @brief Prepares the focus composition for the table.
 *
 * This function retrieves the children of the table, filters out any
 * non-widget elements, and sets the remaining elements as the focus
 * composition order. This is used for keyboard navigation.
 *
 * @param obj The Evas object (table).
 * @param pd Private data (unused).
 */
static void
_elm_table_efl_ui_focus_composition_prepare(Eo *obj, void *pd EINA_UNUSED)
{
   Eina_List *l, *ll;
   Efl_Ui_Widget *elem;

   Elm_Widget_Smart_Data *wpd = efl_data_scope_get(obj, EFL_UI_WIDGET_CLASS);
   Eina_List *order = evas_object_table_children_get(wpd->resize_obj);

   EINA_LIST_FOREACH_SAFE(order, l, ll, elem)
     {
        if (!efl_isa(elem, EFL_UI_WIDGET_CLASS))
          order = eina_list_remove(order, elem);
     }

   efl_ui_focus_composition_elements_set(obj, order);
}

/**
 * @internal
 * @brief Sets the mirrored mode of the table's internal Evas object.
 *
 * This function is called when the widget's mirrored mode changes,
 * typically due to a change in the UI language (RTL/LTR).
 *
 * @param obj The Evas object (table).
 * @param rtl EINA_TRUE if right-to-left mode is enabled, EINA_FALSE otherwise.
 */
static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_table_mirrored_set(wd->resize_obj, rtl);
}

/**
 * @internal
 * @brief Applies the theme to the table widget.
 *
 * This function calls the parent class's theme_apply function and then
 * applies mirroring settings.
 *
 * @param obj The Evas object (table).
 * @param sd Private data (unused).
 * @return Eina_Error Standard Efl_Ui_Theme_Apply error code.
 */
EOLIAN static Eina_Error
_elm_table_efl_ui_widget_theme_apply(Eo *obj, void *sd EINA_UNUSED)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   _mirrored_set(obj, efl_ui_mirrored_get(obj));

   return int_ret;
}

/**
 * @internal
 * @brief Evaluates and sets the minimum size of the table.
 *
 * This function retrieves the combined minimum size of the table's
 * internal Evas object (which considers all packed children) and
 * sets it as the minimum size hint for the table widget itself.
 *
 * @param obj The Evas object (table).
 */
static void
_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = 0, minh = 0;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   if (!efl_alive_get(obj)) return;

   evas_object_size_hint_combined_min_get(wd->resize_obj, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
}

/**
 * @internal
 * @brief Callback function invoked when the size hints of the internal table object change.
 *
 * This triggers a re-evaluation of the table widget's own size.
 *
 * @param data The Evas object (table widget) passed as user data.
 * @param e The Evas canvas (unused).
 * @param obj The Evas object whose size hints changed (internal table, unused).
 * @param event_info Event-specific information (unused).
 */
static void
_on_size_hints_changed(void *data,
                       Evas *e EINA_UNUSED,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   _sizing_eval(data);
}

/**
 * @internal
 * @brief Handles deletion of a sub-object from the table.
 *
 * This function calls the parent class's sub_object_del and then
 * re-evaluates the table's size.
 *
 * @param obj The Evas object (table).
 * @param _pd Private data (unused).
 * @param child The sub-object being deleted.
 * @return EINA_TRUE if the sub-object was successfully deleted, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_table_efl_ui_widget_widget_sub_object_del(Eo *obj, void *_pd EINA_UNUSED, Evas_Object *child)
{
   Eina_Bool int_ret = EINA_FALSE;

   int_ret = elm_widget_sub_object_del(efl_super(obj, MY_CLASS), child);
   if (!int_ret) return EINA_FALSE;

   _sizing_eval(obj);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Adds the table to the canvas group.
 *
 * This function creates the internal Evas table object, sets it as the
 * resize object for the widget, registers a callback for size hints changes,
 * calls the parent's group_add, and sets initial widget properties.
 *
 * @param obj The Evas object (table).
 * @param _pd Private data (unused).
 */
EOLIAN static void
_elm_table_efl_canvas_group_group_add(Eo *obj, void *_pd EINA_UNUSED)
{
   Evas_Object *table;

   table = evas_object_table_add(evas_object_evas_get(obj));
   elm_widget_resize_object_set(obj, table);

   evas_object_event_callback_add
     (table, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _on_size_hints_changed, obj);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   elm_widget_can_focus_set(obj, EINA_FALSE);
   elm_widget_highlight_ignore_set(obj, EINA_FALSE);

   efl_ui_widget_theme_apply(obj);
}

/**
 * @internal
 * @brief Deletes the table from the canvas group.
 *
 * This function removes the size hints changed callback and ensures the
 * internal table object is processed last during deletion, as it might
 * parent other sub-objects. Finally, it calls the parent's group_del.
 *
 * @param obj The Evas object (table).
 * @param _pd Private data (unused).
 */
EOLIAN static void
_elm_table_efl_canvas_group_group_del(Eo *obj, void *_pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_event_callback_del_full
     (wd->resize_obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
     _on_size_hints_changed, obj);

   /* let's make our table object the *last* to be processed, since it
    * may (smart) parent other sub objects here */
   {
      unsigned int resize_id = 0;
      if (eina_array_find(wd->children, wd->resize_obj, &resize_id))
        {
           //exchange with last
           eina_array_data_set(wd->children, resize_id, eina_array_data_get(wd->children, eina_array_count(wd->children) - 1));
           eina_array_data_set(wd->children, eina_array_count(wd->children) - 1, wd->resize_obj);
        }
   }

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @brief Add a new table to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Elm_Table_Group
 */
EAPI Evas_Object *
elm_table_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Constructor for the Elm_Table object.
 *
 * Initializes the object, sets its legacy type name, and default
 * accessibility role.
 *
 * @param obj The Evas object (table).
 * @param _pd Private data (unused).
 * @return The constructed Evas object.
 */
EOLIAN static Eo *
_elm_table_efl_object_constructor(Eo *obj, void *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_FILLER);

   return obj;
}

/**
 * @internal
 * @brief Sets the homogeneous mode for the table.
 * @see elm_table_homogeneous_set()
 */
EOLIAN static void
_elm_table_homogeneous_set(Eo *obj, void *_pd EINA_UNUSED, Eina_Bool homogeneous)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_table_homogeneous_set
     (wd->resize_obj, homogeneous);
}

/**
 * @internal
 * @brief Gets the homogeneous mode for the table.
 * @see elm_table_homogeneous_get()
 */
EOLIAN static Eina_Bool
_elm_table_homogeneous_get(const Eo *obj, void *_pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);
   return evas_object_table_homogeneous_get(wd->resize_obj);
}

/**
 * @internal
 * @brief Sets the padding between cells for the table.
 * @see elm_table_padding_set()
 */
EOLIAN static void
_elm_table_padding_set(Eo *obj, void *_pd EINA_UNUSED, Evas_Coord horizontal, Evas_Coord vertical)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_table_padding_set
     (wd->resize_obj, horizontal, vertical);
}

/**
 * @internal
 * @brief Gets the padding between cells for the table.
 * @see elm_table_padding_get()
 */
EOLIAN static void
_elm_table_padding_get(const Eo *obj, void *_pd EINA_UNUSED, Evas_Coord *horizontal, Evas_Coord *vertical)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_table_padding_get
     (wd->resize_obj, horizontal, vertical);
}

/**
 * @internal
 * @brief Sets the alignment of the whole table object.
 * @see elm_table_align_set()
 */
EOLIAN static void
_elm_table_align_set(Eo *obj, void *_pd EINA_UNUSED, double horizontal, double vertical)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_table_align_set
     (wd->resize_obj, horizontal, vertical);
}

/**
 * @internal
 * @brief Gets the alignment of the whole table object.
 * @see elm_table_align_get()
 */
EOLIAN static void
_elm_table_align_get(const Eo *obj, void *_pd EINA_UNUSED, double *horizontal, double *vertical)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_table_align_get
     (wd->resize_obj, horizontal, vertical);
}

/**
 * @internal
 * @brief Add a subobject to a table.
 * @see elm_table_pack()
 *
 * @param obj The table object.
 * @param _pd Private data (unused).
 * @param subobj The subobject to add.
 * @param col The column in which to add the subobject (0-indexed).
 * @param row The row in which to add the subobject (0-indexed).
 * @param colspan The number of columns to span (1 or more).
 * @param rowspan The number of rows to span (1 or more).
 */
EOLIAN static void
_elm_table_pack(Eo *obj, void *_pd EINA_UNUSED, Evas_Object *subobj, int col, int row, int colspan, int rowspan)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (col < 0)
     {
        ERR("col < 0");
        return;
     }
   if (colspan < 1)
     {
        ERR("colspan < 1");
        return;
     }
   if ((0xffff - col) < colspan)
     {
        ERR("col + colspan > 0xffff");
        return;
     }
   if ((col + colspan) >= 0x7ffff)
     {
        WRN("col + colspan getting rather large (>32767)");
     }
   if (row < 0)
     {
        ERR("row < 0");
        return;
     }
   if (rowspan < 1)
     {
        ERR("rowspan < 1");
        return;
     }
   if ((0xffff - row) < rowspan)
     {
        ERR("row + rowspan > 0xffff");
        return;
     }
   if ((row + rowspan) >= 0x7ffff)
     {
        WRN("row + rowspan getting rather large (>32767)");
     }

   elm_widget_sub_object_add(obj, subobj);
   evas_object_table_pack(wd->resize_obj, subobj, col, row, colspan, rowspan);
   efl_ui_focus_composition_dirty(obj);
}

/**
 * @internal
 * @brief Unpack a subobject from the table.
 * @see elm_table_unpack()
 *
 * @param obj The table object.
 * @param _pd Private data (unused).
 * @param subobj The subobject to unpack.
 */
EOLIAN static void
_elm_table_unpack(Eo *obj, void *_pd EINA_UNUSED, Evas_Object *subobj)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   _elm_widget_sub_object_redirect_to_top(obj, subobj);
   evas_object_table_unpack(wd->resize_obj, subobj);
}

/**
 * @brief Set the packing location of an existing child of the table
 *
 * Modifies the position of an object already in the table.
 *
 * @param subobj The subobject to be modified in the table.
 * @param col The column in which to add the subobject (0-indexed).
 * @param row The row in which to add the subobj (0-indexed).
 * @param colspan The number of columns to span (1 or more).
 * @param rowspan The number of rows to span (1 or more).
 *
 * @see elm_table_pack() for more details
 * @ingroup Elm_Table_Group
 */
EAPI void
elm_table_pack_set(Evas_Object *subobj,
                   int col,
                   int row,
                   int colspan,
                   int rowspan)
{
   Evas_Object *obj = elm_widget_parent_widget_get(subobj);

   ELM_TABLE_CHECK(obj);
   elm_obj_table_pack_set(obj, subobj, col, row, colspan, rowspan);
}

/**
 * @internal
 * @brief Sets the packing of a subobject within the table.
 * @see elm_table_pack_set() (legacy API)
 * @see _elm_table_pack() (EO API for adding new subobject)
 *
 * This function is similar to _elm_table_pack, but it's intended for
 * modifying the packing of an *existing* subobject rather than adding a new one.
 * It directly calls evas_object_table_pack without adding the subobject
 * as a widget child again.
 *
 * @param obj The table object.
 * @param _pd Private data (unused).
 * @param subobj The subobject whose packing is to be set.
 * @param col The new column.
 * @param row The new row.
 * @param colspan The new column span.
 * @param rowspan The new row span.
 */
EOLIAN static void
_elm_table_pack_set(Eo *obj, void *_pd EINA_UNUSED, Evas_Object *subobj, int col, int row, int colspan, int rowspan)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_table_pack(wd->resize_obj, subobj, col, row, colspan, rowspan);
   efl_ui_focus_composition_dirty(obj);
}

/**
 * @brief Get the packing location of an existing child of the table
 *
 * @param subobj The subobject to be queried
 * @param[out] col The column in which the subobject is packed.
 * @param[out] row The row in which the subobject is packed.
 * @param[out] colspan The number of columns the subobject spans.
 * @param[out] rowspan The number of rows the subobject spans.
 *
 * @see elm_table_pack() for more details
 * @ingroup Elm_Table_Group
 */
EAPI void
elm_table_pack_get(Evas_Object *subobj,
                   int *col,
                   int *row,
                   int *colspan,
                   int *rowspan)
{
   Evas_Object *obj = elm_widget_parent_widget_get(subobj);
   ELM_TABLE_CHECK(obj);
   elm_obj_table_pack_get(obj, subobj, col, row, colspan, rowspan);
}

/**
 * @internal
 * @brief Gets the packing of a subobject within the table.
 * @see elm_table_pack_get()
 */
EOLIAN static void
_elm_table_pack_get(Eo *obj, void *_pd EINA_UNUSED, Evas_Object *subobj, int *col, int *row, int *colspan, int *rowspan)
{
   unsigned short icol, irow, icolspan, irowspan;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_table_pack_get
     (wd->resize_obj, subobj, &icol, &irow, &icolspan, &irowspan);
   if (col) *col = icol;
   if (row) *row = irow;
   if (colspan) *colspan = icolspan;
   if (rowspan) *rowspan = irowspan;
}

/**
 * @internal
 * @brief Clears the table of all children.
 * @see elm_table_clear()
 */
EOLIAN static void
_elm_table_clear(Eo *obj, void *_pd EINA_UNUSED, Eina_Bool clear)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_table_clear(wd->resize_obj, clear);
   efl_ui_focus_composition_dirty(obj);
}

/**
 * @internal
 * @brief Gets the child object at a specific cell in the table.
 * @see elm_table_child_get()
 */
EOLIAN static Evas_Object*
_elm_table_child_get(const Eo *obj, void *_pd EINA_UNUSED, int col, int row)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   return evas_object_table_child_get(wd->resize_obj, col, row);
}

/**
 * @internal
 * @brief Class constructor for Elm_Table.
 *
 * Registers the legacy type name for the class.
 *
 * @param klass The Efl_Class.
 */
EOLIAN static void
_elm_table_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @internal
 * @brief Calculates the layout of the table.
 *
 * This function is called when the canvas group needs recalculation.
 * It triggers the smart calculation of the internal Evas table object.
 *
 * @param obj The Evas object (table).
 * @param pd Private data (unused).
 */
EOLIAN void
_elm_table_efl_canvas_group_group_calculate(Eo *obj, void *pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   efl_canvas_group_need_recalculate_set(obj, EINA_FALSE);
   evas_object_smart_calculate(wd->resize_obj);
}

/* Internal EO APIs and hidden overrides */

#define ELM_TABLE_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_table)

#include "elm_table_eo.c"

/**
 * @}
 */
