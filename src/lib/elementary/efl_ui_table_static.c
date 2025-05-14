#include "efl_ui_table_private.h"

#define MY_CLASS EFL_UI_TABLE_STATIC_CLASS
#define MY_CLASS_NAME "Efl.Ui.Table_Static"
#define MY_CLASS_NAME_LEGACY "elm_grid"

/**
 * @brief Constructor for the Efl.Ui.Table_Static class.
 *
 * This function is called when a new Efl.Ui.Table_Static object is created.
 * It initializes the object, sets its canvas type, accessibility role,
 * and default requested columns and rows for the table.
 *
 * @param obj The Efl.Object to be constructed.
 * @param pd Private data for the Efl.Ui.Table_Static class (unused).
 * @return The constructed Efl.Object.
 */
EOLIAN static Eo *
_efl_ui_table_static_efl_object_constructor(Eo *obj, void *pd EINA_UNUSED)
{
   Efl_Ui_Table_Data *gd;

   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_FILLER);

   gd = efl_data_scope_get(obj, EFL_UI_TABLE_CLASS);
   gd->req_cols = 100;
   gd->req_rows = 100;

   return obj;
}

/**
 * @brief Updates the layout of the table and its items.
 *
 * This function is called when the layout of the table needs to be recalculated.
 * It iterates through all packed items and sets their geometry (position and size)
 * based on the table's current dimensions, the item's specified column, row,
 * column span, and row span, and whether the UI is mirrored.
 *
 * The position and size of each item are calculated proportionally to the
 * table's total width and height, and the requested number of columns and rows.
 *
 * For example, if an item is at `col=0`, `row=0` with `col_span=1`, `row_span=1`
 * in a table with `req_cols=10`, `req_rows=10`, and the table's geometry is
 * `x=0, y=0, w=100, h=100`:
 * - If not mirrored:
 *   - x1 = 0 + (100 * 0) / 10 = 0
 *   - x2 = 0 + (100 * (0 + 1)) / 10 = 10
 *   - y1 = 0 + (100 * 0) / 10 = 0
 *   - y2 = 0 + (100 * (0 + 1)) / 10 = 10
 *   - Item geometry: x=0, y=0, w=10, h=10
 * - If mirrored:
 *   - x1 = 0 + (100 * (10 - (0 + 1))) / 10 = 90
 *   - x2 = 0 + (100 * (10 - 0)) / 10 = 100
 *   - Item geometry: x=90, y=0, w=10, h=10
 *
 * @param obj The Efl.Ui.Table_Static object whose layout is to be updated.
 * @param _pd Private data for the Efl.Ui.Table_Static class (unused).
 */
EOLIAN static void
_efl_ui_table_static_efl_pack_layout_layout_update(Eo *obj, void *_pd EINA_UNUSED)
{
   Efl_Ui_Table_Data *gd;
   Table_Item *gi;
   Evas *e;
   long long xl, yl, wl, hl, vwl, vhl;
   Eina_Bool mirror;
   Eina_Rect r;

   gd = efl_data_scope_get(obj, EFL_UI_TABLE_CLASS);
   if (!gd->items) return;

   e = evas_object_evas_get(obj);
   efl_event_freeze(e);

   r = efl_gfx_entity_geometry_get(obj);
   xl = r.x;
   yl = r.y;
   wl = r.w;
   hl = r.h;
   mirror = efl_ui_mirrored_get(obj);

   if (!gd->req_cols || !gd->req_rows)
     {
        WRN("Table_Static size must be set before using! Default to 100x100.");
        efl_pack_table_size_set(obj, 100, 100);
        if (!gd->req_cols || !gd->req_rows) goto err;
     }
   vwl = gd->req_cols;
   vhl = gd->req_rows;

   EINA_INLIST_FOREACH(gd->items, gi)
     {
        long long x1, y1, x2, y2;

        if (!mirror)
          {
             x1 = xl + ((wl * (long long)gi->col) / vwl);
             x2 = xl + ((wl * (long long)(gi->col + gi->col_span)) / vwl);
          }
        else
          {
             x1 = xl + ((wl * (vwl - (long long)(gi->col + gi->col_span))) / vwl);
             x2 = xl + ((wl * (vwl - (long long)gi->col)) / vwl);
          }
        y1 = yl + ((hl * (long long)gi->row) / vhl);
        y2 = yl + ((hl * (long long)(gi->row + gi->row_span)) / vhl);
        efl_gfx_entity_geometry_set(gi->object, EINA_RECT(x1, y1, x2 - x1, y2 - y1));
     }
err:
   efl_event_thaw(e);
}

#include "efl_ui_table_static.eo.c"
