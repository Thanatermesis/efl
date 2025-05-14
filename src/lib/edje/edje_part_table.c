#include "edje_private.h"
#include "edje_part_helper.h"
#include "efl_canvas_layout_part_table.eo.h"
#define MY_CLASS EFL_CANVAS_LAYOUT_PART_TABLE_CLASS

#include "../evas/canvas/evas_table_eo.h"

PROXY_IMPLEMENTATION(table, MY_CLASS, EINA_FALSE)
#undef PROXY_IMPLEMENTATION

/**
 * @brief Structure for iterating over items within a table part.
 * This iterator is specifically designed to work with table parts,
 * managing a list of items and their associated Evas object.
 */
typedef struct _Part_Item_Iterator Part_Item_Iterator;

/**
 * @brief Implements efl_container_content_iterate for table parts.
 *
 * Creates and returns an iterator for all child objects packed into this table part.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @return A new Eina_Iterator for the table's children, or NULL on failure or if the part is not a container.
 */
EOLIAN static Eina_Iterator *
_efl_canvas_layout_part_table_efl_container_content_iterate(Eo *obj, void *_pd EINA_UNUSED)
{
   Eina_Iterator *it;

   PROXY_DATA_GET(obj, pd);
   if (!pd->rp->typedata.container) return NULL;
   it = evas_object_table_iterator_new(pd->rp->object);

   return efl_canvas_iterator_create(pd->rp->object, it, NULL);
}

/**
 * @brief Implements efl_container_content_count for table parts.
 *
 * Counts the number of child objects packed into this table part.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @return The number of items in the table.
 */
EOLIAN static int
_efl_canvas_layout_part_table_efl_container_content_count(Eo *obj, void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   return evas_obj_table_count(pd->rp->object);
}

/**
 * @brief Implements efl_pack_clear for table parts.
 *
 * Removes all packed objects from the table part and deletes them.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_table_efl_pack_pack_clear(Eo *obj, void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_part_table_clear(pd->ed, pd->part, EINA_TRUE);
}

/**
 * @brief Implements efl_pack_unpack_all for table parts.
 *
 * Removes all packed objects from the table part without deleting them.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_table_efl_pack_unpack_all(Eo *obj, void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_part_table_clear(pd->ed, pd->part, EINA_FALSE);
}

/**
 * @brief Implements efl_pack_unpack for table parts.
 *
 * Removes a specific subobject from the table part without deleting it.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @param subobj The subobject to remove.
 * @return EINA_TRUE if the subobject was successfully unpacked, EINA_FALSE otherwise (e.g., if not found).
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_table_efl_pack_unpack(Eo *obj EINA_UNUSED, void *_pd EINA_UNUSED, Efl_Gfx_Entity *subobj)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_part_table_unpack(pd->ed, pd->part, subobj);
}

/**
 * @brief Implements efl_pack_table_pack for table parts.
 *
 * Packs a subobject into the table at the specified column and row, with a given colspan and rowspan.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @param subobj The subobject to pack.
 * @param col The column to pack the subobject into.
 * @param row The row to pack the subobject into.
 * @param colspan The number of columns the subobject should span.
 * @param rowspan The number of rows the subobject should span.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_table_efl_pack_table_pack_table(Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx_Entity *subobj, int col, int row, int colspan, int rowspan)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_part_table_pack(pd->ed, pd->part, subobj, col, row, colspan, rowspan);
}

/**
 * @brief Implements efl_pack_table_content_get for table parts.
 *
 * Retrieves the content of a cell at the specified column and row.
 * Note: This gets the first child found at this specific cell, not considering spans.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @param col The column of the cell.
 * @param row The row of the cell.
 * @return The Efl_Gfx_Entity at the specified cell, or NULL if the cell is empty or coordinates are out of bounds.
 */
EOLIAN static Efl_Gfx_Entity *
_efl_canvas_layout_part_table_efl_pack_table_table_content_get(Eo *obj, void *_pd EINA_UNUSED, int col, int row)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_part_table_child_get(pd->ed, pd->part, col, row);
}

/**
 * @brief Implements efl_pack_table_size_get for table parts.
 *
 * Gets the dimensions (number of columns and rows) of the table.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @param[out] cols Pointer to store the number of columns. Can be NULL.
 * @param[out] rows Pointer to store the number of rows. Can be NULL.
 */
EOLIAN static void
_efl_canvas_layout_part_table_efl_pack_table_table_size_get(const Eo *obj, void *_pd EINA_UNUSED, int *cols, int *rows)
{
   PROXY_DATA_GET(obj, pd);
   _edje_part_table_col_row_size_get(pd->ed, pd->part, cols, rows);
}

/**
 * @brief Implements efl_pack_table_columns_get for table parts.
 *
 * Gets the number of columns in the table.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @return The number of columns.
 */
EOLIAN static int
_efl_canvas_layout_part_table_efl_pack_table_table_columns_get(const Eo *obj, void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   int cols = 0, rows = 0;
   _edje_part_table_col_row_size_get(pd->ed, pd->part, &cols, &rows);
   return cols;
}

/**
 * @brief Implements efl_pack_table_rows_get for table parts.
 *
 * Gets the number of rows in the table.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @return The number of rows.
 */
EOLIAN static int
_efl_canvas_layout_part_table_efl_pack_table_table_rows_get(const Eo *obj, void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   int cols = 0, rows = 0;
   _edje_part_table_col_row_size_get(pd->ed, pd->part, &cols, &rows);
   return rows;
}

/* New table apis with eo */

/**
 * @internal
 * @brief Advances the Part_Item_Iterator to the next item.
 *
 * This function is used by the Eina_Iterator interface.
 *
 * @param it The Part_Item_Iterator instance.
 * @param data Pointer to store the next item.
 * @return EINA_TRUE if there is a next item, EINA_FALSE otherwise.
 */
static Eina_Bool
_table_item_iterator_next(Part_Item_Iterator *it, void **data)
{
   Efl_Gfx_Entity *sub;

   if (!it->object) return EINA_FALSE;
   if (!eina_iterator_next(it->real_iterator, (void **) &sub))
     return EINA_FALSE;

   if (data) *data = sub;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the container object associated with the Part_Item_Iterator.
 *
 * This function is used by the Eina_Iterator interface.
 *
 * @param it The Part_Item_Iterator instance.
 * @return The container Eo object.
 */
static Eo *
_table_item_iterator_get_container(Part_Item_Iterator *it)
{
   return it->object;
}

/**
 * @internal
 * @brief Frees the resources used by the Part_Item_Iterator.
 *
 * This function is used by the Eina_Iterator interface.
 *
 * @param it The Part_Item_Iterator instance to free.
 */
static void
_table_item_iterator_free(Part_Item_Iterator *it)
{
   eina_iterator_free(it->real_iterator);
   eina_list_free(it->list);
   efl_wref_del(it->object, &it->object);
   free(it);
}

/**
 * @brief Implements efl_pack_table_contents_get for table parts.
 *
 * Retrieves an iterator for all Efl_Gfx_Entity objects packed into a specific cell (col, row).
 * If @p below is EINA_TRUE, it also includes objects that span multiple cells and cover this particular cell.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @param col The column of the cell.
 * @param row The row of the cell.
 * @param below If EINA_TRUE, include items that span into this cell from above/left.
 *              If EINA_FALSE, only items starting exactly at (col, row) are returned.
 * @return A new Eina_Iterator for the cell's contents, or NULL on failure.
 *         The iterator contains Efl_Gfx_Entity objects.
 */
EOLIAN static Eina_Iterator *
_efl_canvas_layout_part_table_efl_pack_table_table_contents_get(Eo *obj, void *_pd EINA_UNUSED, int col, int row, Eina_Bool below)
{
   Evas_Object *sobj;
   Eina_Iterator *it;
   Part_Item_Iterator *pit;
   Eina_List *list = NULL;
   unsigned short c, r, cs, rs;

   /* FIXME: terrible performance because there is no proper evas table api */

   PROXY_DATA_GET(obj, pd);
   it = evas_object_table_iterator_new(pd->rp->object);
   EINA_ITERATOR_FOREACH(it, sobj)
     {
        evas_object_table_pack_get(pd->rp->object, sobj, &c, &r, &cs, &rs);
        if (((int) c == col) && ((int) r == row))
          list = eina_list_append(list, sobj);
        else if (below)
          {
             if (((int) c <= col) && ((int) (c + cs) >= col) &&
                 ((int) r <= row) && ((int) (r + rs) >= row))
               list = eina_list_append(list, sobj);
          }
     }
   eina_iterator_free(it);

   pit = calloc(1, sizeof(*pit));
   if (!pit) return NULL;

   EINA_MAGIC_SET(&pit->iterator, EINA_MAGIC_ITERATOR);

   pit->list = list;
   pit->real_iterator = eina_list_iterator_new(pit->list);
   pit->iterator.version = EINA_ITERATOR_VERSION;
   pit->iterator.next = FUNC_ITERATOR_NEXT(_table_item_iterator_next);
   pit->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(_table_item_iterator_get_container);
   pit->iterator.free = FUNC_ITERATOR_FREE(_table_item_iterator_free);
   efl_wref_add(obj, &pit->object);

   return &pit->iterator;
}

/**
 * @brief Implements efl_pack_table_cell_column_get for table parts.
 *
 * Gets the column and colspan of a specific subobject within the table.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @param subobj The subobject to query.
 * @param[out] col Pointer to store the column index. Can be NULL.
 * @param[out] colspan Pointer to store the column span. Can be NULL.
 * @return EINA_TRUE if the subobject is found and information is retrieved, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_table_efl_pack_table_table_cell_column_get(const Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx_Entity * subobj, int *col, int *colspan)
{
   unsigned short c, cs;
   Eina_Bool ret;

   PROXY_DATA_GET(obj, pd);
   ret = evas_object_table_pack_get(pd->rp->object, subobj, &c, NULL, &cs, NULL);
   if (col) *col = c;
   if (colspan) *colspan = cs;

   return ret;
}

/**
 * @brief Implements efl_pack_table_cell_column_set for table parts.
 *
 * Sets the column and colspan for a specific subobject already in the table.
 * The subobject's row and rowspan remain unchanged.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @param subobj The subobject to modify.
 * @param col The new column index.
 * @param colspan The new column span.
 */
EOLIAN static void
_efl_canvas_layout_part_table_efl_pack_table_table_cell_column_set(Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx_Entity * subobj, int col, int colspan)
{
   unsigned short r, rs;

   PROXY_DATA_GET(obj, pd);
   evas_object_table_pack_get(pd->rp->object, subobj, NULL, &r, NULL, &rs);
   evas_object_table_pack(pd->rp->object, subobj, col, r, colspan, rs);
}

/**
 * @brief Implements efl_pack_table_cell_row_get for table parts.
 *
 * Gets the row and rowspan of a specific subobject within the table.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @param subobj The subobject to query.
 * @param[out] row Pointer to store the row index. Can be NULL.
 * @param[out] rowspan Pointer to store the row span. Can be NULL.
 * @return EINA_TRUE if the subobject is found and information is retrieved, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_table_efl_pack_table_table_cell_row_get(const Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx_Entity * subobj, int *row, int *rowspan)
{
   unsigned short r, rs;
   Eina_Bool ret;

   PROXY_DATA_GET(obj, pd);
   ret = evas_object_table_pack_get(pd->rp->object, subobj, NULL, &r, NULL, &rs);
   if (row) *row = r;
   if (rowspan) *rowspan = rs;

   return ret;
}

/**
 * @brief Implements efl_pack_table_cell_row_set for table parts.
 *
 * Sets the row and rowspan for a specific subobject already in the table.
 * The subobject's column and colspan remain unchanged.
 *
 * @param obj The Efl_Canvas_Layout_Part_Table object.
 * @param _pd Private data for the Efl_Canvas_Layout_Part_Table object.
 * @param subobj The subobject to modify.
 * @param row The new row index.
 * @param rowspan The new row span.
 */
EOLIAN static void
_efl_canvas_layout_part_table_efl_pack_table_table_cell_row_set(Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx_Entity * subobj, int row, int rowspan)
{
   unsigned short c, cs;

   PROXY_DATA_GET(obj, pd);
   evas_object_table_pack_get(pd->rp->object, subobj, &c, NULL, &cs, NULL);
   evas_object_table_pack(pd->rp->object, subobj, c, row, cs, rowspan);
}

/* Legacy API implementation */

#ifdef DEGUG
#define PART_BOX_GET(obj, part, ...) ({ \
   Eo *__box = efl_part(obj, part); \
   if (!__box || !efl_isa(__box, EFL_CANVAS_LAYOUT_PART_BOX_CLASS)) \
     { \
        ERR("No such box part '%s' in layout %p", part, obj); \
        return __VA_ARGS__; \
     } \
   __box; })
#else
#define PART_BOX_GET(obj, part, ...) ({ \
   Eo *__box = efl_part(obj, part); \
   if (!__box) return __VA_ARGS__; \
   __box; })
#endif

/**
 * @brief Appends a child object to the end of a box part.
 * @param obj The Edje object.
 * @param part The name of the box part.
 * @param child The Evas_Object to append.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 * @see efl_pack_end()
 */
EAPI Eina_Bool
edje_object_part_box_append(Edje_Object *obj, const char *part, Evas_Object *child)
{
   Eo *box = PART_BOX_GET(obj, part, EINA_FALSE);
   return efl_pack_end(box, child);
}

/**
 * @brief Prepends a child object to the beginning of a box part.
 * @param obj The Edje object.
 * @param part The name of the box part.
 * @param child The Evas_Object to prepend.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 * @see efl_pack_begin()
 */
EAPI Eina_Bool
edje_object_part_box_prepend(Edje_Object *obj, const char *part, Evas_Object *child)
{
   Eo *box = PART_BOX_GET(obj, part, EINA_FALSE);
   return efl_pack_begin(box, child);
}

/**
 * @brief Inserts a child object into a box part before a reference object.
 * @param obj The Edje object.
 * @param part The name of the box part.
 * @param child The Evas_Object to insert.
 * @param reference The Evas_Object before which to insert.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 * @see efl_pack_before()
 */
EAPI Eina_Bool
edje_object_part_box_insert_before(Edje_Object *obj, const char *part, Evas_Object *child, const Evas_Object *reference)
{
   Eo *box = PART_BOX_GET(obj, part, EINA_FALSE);
   return efl_pack_before(box, child, reference);
}

/**
 * @brief Inserts a child object into a box part after a reference object.
 * @param obj The Edje object.
 * @param part The name of the box part.
 * @param child The Evas_Object to insert.
 * @param reference The Evas_Object after which to insert.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 * @see efl_pack_after()
 */
EAPI Eina_Bool
edje_object_part_box_insert_after(Edje_Object *obj, const char *part, Evas_Object *child, const Evas_Object *reference)
{
   Eo *box = PART_BOX_GET(obj, part, EINA_FALSE);
   return efl_pack_after(box, child, reference);
}

/**
 * @brief Inserts a child object into a box part at a specific position.
 * @param obj The Edje object.
 * @param part The name of the box part.
 * @param child The Evas_Object to insert.
 * @param pos The position at which to insert the child.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 * @see efl_pack_at()
 */
EAPI Eina_Bool
edje_object_part_box_insert_at(Edje_Object *obj, const char *part, Evas_Object *child, unsigned int pos)
{
   Eo *box = PART_BOX_GET(obj, part, EINA_FALSE);
   return efl_pack_at(box, child, pos);
}

/**
 * @brief Removes a child object from a box part at a specific position.
 * The removed object is returned but not deleted.
 * @param obj The Edje object.
 * @param part The name of the box part.
 * @param pos The position of the child to remove.
 * @return The removed Evas_Object, or @c NULL on failure.
 * @see efl_pack_unpack_at()
 */
EAPI Evas_Object *
edje_object_part_box_remove_at(Edje_Object *obj, const char *part, unsigned int pos)
{
   Eo *box = PART_BOX_GET(obj, part, NULL);
   return efl_pack_unpack_at(box, pos);
}

/**
 * @brief Removes a specific child object from a box part.
 * The removed object is returned but not deleted.
 * @param obj The Edje object.
 * @param part The name of the box part.
 * @param child The Evas_Object to remove.
 * @return The removed Evas_Object if successful, otherwise @c NULL.
 * @see efl_pack_unpack()
 */
EAPI Evas_Object *
edje_object_part_box_remove(Edje_Object *obj, const char *part, Evas_Object *child)
{
   Eo *box = PART_BOX_GET(obj, part, NULL);
   if (efl_pack_unpack(box, child))
     return child;
   return NULL;
}

/**
 * @brief Removes all child objects from a box part.
 * @param obj The Edje object.
 * @param part The name of the box part.
 * @param clear If @c EINA_TRUE, the removed children are deleted.
 *              If @c EINA_FALSE, they are only unpacked.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 * @see efl_pack_clear()
 * @see efl_pack_unpack_all()
 */
EAPI Eina_Bool
edje_object_part_box_remove_all(Edje_Object *obj, const char *part, Eina_Bool clear)
{
   Eo *box = PART_BOX_GET(obj, part, EINA_FALSE);
   if (clear)
     return efl_pack_clear(box);
   else
     return efl_pack_unpack_all(box);
}

/**
 * @brief Packs a child object into a table part.
 * @param obj The Edje object.
 * @param part The name of the table part.
 * @param child_obj The Evas_Object to pack.
 * @param col The column to pack into (0-indexed).
 * @param row The row to pack into (0-indexed).
 * @param colspan The number of columns to span.
 * @param rowspan The number of rows to span.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 * @see efl_pack_table()
 */
EAPI Eina_Bool
edje_object_part_table_pack(Edje_Object *obj, const char *part, Evas_Object *child_obj, unsigned short col, unsigned short row, unsigned short colspan, unsigned short rowspan)
{
   Eo *table = PART_TABLE_GET(obj, part, EINA_FALSE);
   return efl_pack_table(table, child_obj, col, row, colspan, rowspan);
}

/**
 * @brief Gets the dimensions (columns and rows) of a table part.
 * @param obj The Edje object.
 * @param part The name of the table part.
 * @param[out] cols Pointer to store the number of columns. Can be NULL.
 * @param[out] rows Pointer to store the number of rows. Can be NULL.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., part not found or not a table).
 * @see efl_pack_table_size_get()
 */
EAPI Eina_Bool
edje_object_part_table_col_row_size_get(const Edje_Object *obj, const char *part, int *cols, int *rows)
{
   Eo *table = PART_TABLE_GET(obj, part, EINA_FALSE);
   efl_pack_table_size_get(table, cols, rows);
   return EINA_TRUE;
}

/**
 * @brief Gets the child object at a specific cell in a table part.
 * This returns the first child found at the exact (col, row) coordinate,
 * not considering spans. For more complex queries including spans,
 * use the Efl_Pack_Table interface on the part object.
 * @param obj The Edje object.
 * @param part The name of the table part.
 * @param col The column index (0-indexed).
 * @param row The row index (0-indexed).
 * @return The Evas_Object at the specified cell, or @c NULL if no object is there or on failure.
 * @see efl_pack_table_content_get()
 * @see _efl_canvas_layout_part_table_efl_pack_table_table_contents_get()
 */
EAPI Evas_Object *
edje_object_part_table_child_get(const Edje_Object *obj, const char *part, unsigned int col, unsigned int row)
{
   Eo *table = PART_TABLE_GET(obj, part, NULL);
   return efl_pack_table_content_get(table, col, row);
}

/**
 * @brief Unpacks (removes) a child object from a table part.
 * The object is not deleted.
 * @param obj The Edje object.
 * @param part The name of the table part.
 * @param child_obj The Evas_Object to unpack.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., child not found).
 * @see efl_pack_unpack()
 */
EAPI Eina_Bool
edje_object_part_table_unpack(Edje_Object *obj, const char *part, Evas_Object *child_obj)
{
   Eo *table = PART_TABLE_GET(obj, part, EINA_FALSE);
   return efl_pack_unpack(table, child_obj);
}

/**
 * @brief Clears a table part, removing all its children.
 * @param obj The Edje object.
 * @param part The name of the table part.
 * @param clear If @c EINA_TRUE, the removed children are deleted.
 *              If @c EINA_FALSE, they are only unpacked.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 * @see efl_pack_clear()
 * @see efl_pack_unpack_all()
 */
EAPI Eina_Bool
edje_object_part_table_clear(Edje_Object *obj, const char *part, Eina_Bool clear)
{
   Eo *table = PART_TABLE_GET(obj, part, EINA_FALSE);
   if (clear)
     return efl_pack_clear(table);
   else
     return efl_pack_unpack_all(table);
}

#include "efl_canvas_layout_part_table.eo.c"
