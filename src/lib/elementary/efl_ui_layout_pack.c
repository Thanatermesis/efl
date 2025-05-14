/**
 * @file
 * @brief These routines are used for managing Edje part proxies for BOX and TABLE.
 *
 * They allow treating parts of an Edje layout as if they were Efl.Pack_Linear
 * or Efl.Pack_Table containers themselves. This is achieved by creating proxy
 * objects that forward packing operations to the actual Edje part.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_LAYOUT_PROTECTED
#define EFL_UI_LAYOUT_PART_BOX_PROTECTED
#define EFL_UI_LAYOUT_PART_TABLE_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_layout.h"
#include "elm_part_helper.h"

#include "../evas/canvas/evas_box_eo.h"
#include "../evas/canvas/evas_table_eo.h"

/* layout internals for box & table */
Eina_Bool    _efl_ui_layout_box_append(Eo *obj, Efl_Ui_Layout_Data *sd, const char *part, Evas_Object *child);
Eina_Bool    _efl_ui_layout_box_prepend(Eo *obj, Efl_Ui_Layout_Data *sd, const char *part, Evas_Object *child);
Eina_Bool    _efl_ui_layout_box_insert_before(Eo *obj, Efl_Ui_Layout_Data *sd, const char *part, Evas_Object *child, const Evas_Object *reference);
Eina_Bool    _efl_ui_layout_box_insert_at(Eo *obj, Efl_Ui_Layout_Data *sd, const char *part, Evas_Object *child, unsigned int pos);
Evas_Object *_efl_ui_layout_box_remove(Eo *obj, Efl_Ui_Layout_Data *sd, const char *part, Evas_Object *child);
Eina_Bool    _efl_ui_layout_box_remove_all(Eo *obj, Efl_Ui_Layout_Data *sd, const char *part, Eina_Bool clear);
Eina_Bool    _efl_ui_layout_table_pack(Eo *obj, Efl_Ui_Layout_Data *sd, const char *part, Evas_Object *child, unsigned short col, unsigned short row, unsigned short colspan, unsigned short rowspan);
Evas_Object *_efl_ui_layout_table_unpack(Eo *obj, Efl_Ui_Layout_Data *sd, const char *part, Evas_Object *child);
Eina_Bool    _efl_ui_layout_table_clear(Eo *obj, Efl_Ui_Layout_Data *sd, const char *part, Eina_Bool clear);

#define BOX_CLASS   EFL_UI_LAYOUT_PART_BOX_CLASS
#define TABLE_CLASS EFL_UI_LAYOUT_PART_TABLE_CLASS

typedef struct _Layout_Part_Data   Efl_Ui_Layout_Box_Data;
typedef struct _Layout_Part_Data   Efl_Ui_Layout_Table_Data;

/**
 * @brief Internal data structure for layout part proxies (Box and Table).
 *
 * This structure holds the necessary information to link a proxy object
 * to the actual Edje part it represents.
 */
struct _Layout_Part_Data
{
   Efl_Ui_Layout         *obj; /**< The parent Efl_Ui_Layout object (no ref). */
   Efl_Ui_Layout_Data    *sd;  /**< Pointer to the parent layout's private data (xref). */
   Eina_Stringshare      *part; /**< The name of the Edje part this proxy controls. */
   unsigned char          temp; /**< Flag, possibly for temporary state, usage seems minimal. */
};

/**
 * @internal
 * @brief Initializes the layout part data for a proxy object.
 *
 * This function sets up the connection between the proxy object (`obj`)
 * and the actual Edje layout (`layout`) and part (`part`) it will manage.
 * It stores references to the layout and its data, and the part name.
 *
 * @param obj The proxy Evas_Object (either BOX_CLASS or TABLE_CLASS).
 * @param pd The private data of the proxy object.
 * @param layout The parent Efl_Ui_Layout object that contains the part.
 * @param part The name of the Edje part (e.g., "my_box_part").
 */
static void
_efl_ui_layout_part_set_real_part(Eo *obj, struct _Layout_Part_Data *pd, Eo *layout, const char *part)
{
   pd->obj = layout;
   pd->sd = efl_data_xref(pd->obj, EFL_UI_LAYOUT_BASE_CLASS, obj);
   eina_stringshare_replace(&pd->part, part);
   pd->temp = 1;
}

/**
 * @internal
 * @brief Creates and returns a proxy object for an Edje part.
 *
 * Depending on the `type` (BOX or TABLE), this function instantiates
 * a proxy object (either EFL_UI_LAYOUT_PART_BOX_CLASS or
 * EFL_UI_LAYOUT_PART_TABLE_CLASS). This proxy will then intercept
 * packing operations and redirect them to the specified `part` within
 * the given Efl_Ui_Layout `obj`.
 *
 * @param obj The parent Efl_Ui_Layout object.
 * @param type The type of the Edje part (EDJE_PART_TYPE_BOX or EDJE_PART_TYPE_TABLE).
 * @param part The name of the Edje part to proxy.
 * @return A new Eo proxy object if successful, otherwise NULL.
 *         Example: If `part` is "my_content_area" of type BOX, this returns
 *         an object that behaves like an Efl.Pack_Linear, but operations on it
 *         affect "my_content_area".
 */
Eo *
_efl_ui_layout_pack_proxy_get(Efl_Ui_Layout *obj, Edje_Part_Type type, const char *part)
{
   if (type == EDJE_PART_TYPE_BOX)
     return efl_add(BOX_CLASS, obj,
                   _efl_ui_layout_part_set_real_part(efl_added, efl_data_scope_get(efl_added, BOX_CLASS), obj, part));
   else if (type == EDJE_PART_TYPE_TABLE)
     return efl_add(TABLE_CLASS, obj,
                   _efl_ui_layout_part_set_real_part(efl_added, efl_data_scope_get(efl_added, TABLE_CLASS), obj, part));
   else
     return NULL;
}

/**
 * @internal
 * @brief Destructor for the Box part proxy object.
 *
 * Cleans up resources associated with the Box part proxy, including
 * unreferencing the parent layout's data and freeing the part name string.
 *
 * @param obj The Box part proxy object being destroyed.
 * @param pd The private data of the Box part proxy.
 */
EOLIAN static void
_efl_ui_layout_part_box_efl_object_destructor(Eo *obj, Efl_Ui_Layout_Table_Data *pd)
{
   ELM_PART_HOOK;
   efl_data_xunref(pd->obj, pd->sd, obj);
   eina_stringshare_del(pd->part);
   efl_destructor(efl_super(obj, BOX_CLASS));
}

/**
 * @internal
 * @brief Provides an iterator for the content of the proxied Box part.
 *
 * Implements Efl.Container.content_iterate for the Box part proxy.
 * It retrieves the actual Evas_Object representing the Box part from the
 * Edje layout and then creates an iterator over its children.
 *
 * @param obj The Box part proxy object.
 * @param pd The private data of the Box part proxy.
 * @return An Eina_Iterator for the children of the Box part, or NULL on failure.
 *         The iterator elements are Evas_Object pointers.
 */
EOLIAN static Eina_Iterator *
_efl_ui_layout_part_box_efl_container_content_iterate(Eo *obj, Efl_Ui_Layout_Box_Data *pd)
{
   Eina_Iterator *it;

   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);
   it = evas_object_box_iterator_new(pack);
   return efl_canvas_iterator_create(obj, it, NULL);
}

/**
 * @internal
 * @brief Counts the number of items in the proxied Box part.
 *
 * Implements Efl.Container.content_count for the Box part proxy.
 * It retrieves the actual Evas_Object for the Box part and returns its child count.
 *
 * @param obj The Box part proxy object (unused).
 * @param pd The private data of the Box part proxy.
 * @return The number of children in the Box part.
 */
EOLIAN static int
_efl_ui_layout_part_box_efl_container_content_count(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Box_Data *pd)
{
   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);
   return evas_obj_box_count(pack);
}

/**
 * @internal
 * @brief Clears all items from the proxied Box part and deletes them.
 *
 * Implements Efl.Pack.pack_clear for the Box part proxy.
 *
 * @param obj The Box part proxy object (unused).
 * @param pd The private data of the Box part proxy.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_box_efl_pack_pack_clear(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Box_Data *pd)
{
   return _efl_ui_layout_box_remove_all(pd->obj, pd->sd, pd->part, EINA_TRUE);
}

/**
 * @internal
 * @brief Removes all items from the proxied Box part without deleting them.
 *
 * Implements Efl.Pack.unpack_all for the Box part proxy.
 *
 * @param obj The Box part proxy object (unused).
 * @param pd The private data of the Box part proxy.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_box_efl_pack_unpack_all(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Box_Data *pd)
{
   return _efl_ui_layout_box_remove_all(pd->obj, pd->sd, pd->part, EINA_FALSE);
}

/**
 * @internal
 * @brief Unpacks a specific item from the proxied Box part.
 *
 * Implements Efl.Pack.unpack for the Box part proxy.
 *
 * @param obj The Box part proxy object (unused).
 * @param pd The private data of the Box part proxy.
 * @param subobj The Efl_Gfx_Entity to remove from the Box part.
 * @return EINA_TRUE if the item was successfully unpacked, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_box_efl_pack_unpack(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Box_Data *pd, Efl_Gfx_Entity *subobj)
{
   return _efl_ui_layout_box_remove(pd->obj, pd->sd, pd->part, subobj) != NULL;
}

/**
 * @internal
 * @brief Packs an item into the proxied Box part (appends it).
 *
 * Implements Efl.Pack.pack for the Box part proxy. This typically appends
 * the subobj to the end of the box.
 *
 * @param obj The Box part proxy object (unused).
 * @param pd The private data of the Box part proxy.
 * @param subobj The Efl_Gfx_Entity to pack into the Box part.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_box_efl_pack_pack(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Box_Data *pd, Efl_Gfx_Entity *subobj)
{
   return _efl_ui_layout_box_append(pd->obj, pd->sd, pd->part, subobj);
}

/**
 * @internal
 * @brief Packs an item at the beginning of the proxied Box part.
 *
 * Implements Efl.Pack_Linear.pack_begin for the Box part proxy.
 *
 * @param obj The Box part proxy object (unused).
 * @param pd The private data of the Box part proxy.
 * @param subobj The Efl_Gfx_Entity to pack at the beginning.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_box_efl_pack_linear_pack_begin(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Box_Data *pd, Efl_Gfx_Entity *subobj)
{
   return _efl_ui_layout_box_prepend(pd->obj, pd->sd, pd->part, subobj);
}

/**
 * @internal
 * @brief Packs an item at the end of the proxied Box part.
 *
 * Implements Efl.Pack_Linear.pack_end for the Box part proxy.
 *
 * @param obj The Box part proxy object (unused).
 * @param pd The private data of the Box part proxy.
 * @param subobj The Efl_Gfx_Entity to pack at the end.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_box_efl_pack_linear_pack_end(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Box_Data *pd, Efl_Gfx_Entity *subobj)
{
   return _efl_ui_layout_box_append(pd->obj, pd->sd, pd->part, subobj);
}

/**
 * @internal
 * @brief Packs an item before an existing item in the proxied Box part.
 *
 * Implements Efl.Pack_Linear.pack_before for the Box part proxy.
 *
 * @param obj The Box part proxy object (unused).
 * @param pd The private data of the Box part proxy.
 * @param subobj The Efl_Gfx_Entity to pack.
 * @param existing The Efl_Gfx_Entity before which `subobj` should be packed.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_box_efl_pack_linear_pack_before(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Box_Data *pd, Efl_Gfx_Entity *subobj, const Efl_Gfx_Entity *existing)
{
   return _efl_ui_layout_box_insert_before(pd->obj, pd->sd, pd->part, subobj, existing);
}

/**
 * @internal
 * @brief Packs an item after an existing item in the proxied Box part.
 *
 * Implements Efl.Pack_Linear.pack_after for the Box part proxy.
 *
 * @param obj The Box part proxy object.
 * @param pd The private data of the Box part proxy.
 * @param subobj The Efl_Gfx_Entity to pack.
 * @param existing The Efl_Gfx_Entity after which `subobj` should be packed.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_box_efl_pack_linear_pack_after(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Box_Data *pd, Efl_Gfx_Entity *subobj, const Efl_Gfx_Entity *existing)
{
   const Efl_Gfx_Entity *other;
   int index;

   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);
   index = efl_pack_index_get(pack, existing);
   if (index < 0) return EINA_FALSE;

   other = efl_pack_content_get(pack, index + 1);
   if (other)
     return _efl_ui_layout_box_insert_before(pd->obj, pd->sd, pd->part, subobj, other);

   return efl_pack_end(obj, subobj);
}

/**
 * @internal
 * @brief Packs an item at a specific index in the proxied Box part.
 *
 * Implements Efl.Pack_Linear.pack_at for the Box part proxy.
 *
 * @param obj The Box part proxy object (unused).
 * @param pd The private data of the Box part proxy.
 * @param subobj The Efl_Gfx_Entity to pack.
 * @param index The 0-based index at which to pack `subobj`.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_box_efl_pack_linear_pack_at(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Box_Data *pd, Efl_Gfx_Entity *subobj, int index)
{
   return _efl_ui_layout_box_insert_at(pd->obj, pd->sd, pd->part, subobj, index);
}

/**
 * @internal
 * @brief Retrieves the content (item) at a specific index from the proxied Box part.
 *
 * Implements Efl.Pack_Linear.pack_content_get for the Box part proxy.
 *
 * @param obj The Box part proxy object (unused).
 * @param pd The private data of the Box part proxy.
 * @param index The 0-based index of the item to retrieve.
 * @return The Efl_Gfx_Entity at the given index, or NULL if not found.
 */
EOLIAN static Efl_Gfx_Entity *
_efl_ui_layout_part_box_efl_pack_linear_pack_content_get(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Box_Data *pd, int index)
{
   Evas_Object_Box_Option *opt;
   Evas_Object_Box_Data *priv;

   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);

   priv = efl_data_scope_get(pack, EVAS_BOX_CLASS);
   opt = eina_list_nth(priv->children, index);
   if (!opt) return NULL;
   return opt->obj;
}

/**
 * @internal
 * @brief Unpacks (removes) the item at a specific index from the proxied Box part.
 *
 * Implements Efl.Pack_Linear.pack_unpack_at for the Box part proxy.
 *
 * @param obj The Box part proxy object.
 * @param pd The private data of the Box part proxy.
 * @param index The 0-based index of the item to unpack.
 * @return The unpacked Efl_Gfx_Entity if successful, NULL otherwise.
 */
EOLIAN static Efl_Gfx_Entity *
_efl_ui_layout_part_box_efl_pack_linear_pack_unpack_at(Eo *obj, Efl_Ui_Layout_Box_Data *pd, int index)
{
   Efl_Gfx_Entity *subobj;

   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);

   subobj = efl_pack_content_get(pack, index);
   if (!subobj) return NULL;
   if (efl_pack_unpack(obj, subobj))
     return subobj;

   ERR("failed to remove %p from %p (item %d)", subobj, pd->obj, index);
   return NULL;
}

/**
 * @internal
 * @brief Gets the index of a specific item within the proxied Box part.
 *
 * Implements Efl.Pack_Linear.pack_index_get for the Box part proxy.
 *
 * @param obj The Box part proxy object (unused).
 * @param pd The private data of the Box part proxy.
 * @param subobj The Efl_Gfx_Entity whose index is to be found.
 * @return The 0-based index of `subobj`, or -1 if not found.
 */
EOLIAN static int
_efl_ui_layout_part_box_efl_pack_linear_pack_index_get(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Box_Data *pd, const Efl_Gfx_Entity *subobj)
{
   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);
   return efl_pack_index_get(pack, subobj);
}

/**
 * @internal
 * @brief Gets the orientation of the proxied Box part.
 *
 * Implements Efl.Ui.Layout_Orientable.orientation_get for the Box part proxy.
 * It queries the orientation of the actual Edje part.
 *
 * @param obj The Box part proxy object (unused).
 * @param pd The private data of the Box part proxy.
 * @return The orientation of the Box part (e.g., EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL,
 *         EFL_UI_LAYOUT_ORIENTATION_VERTICAL). Returns EFL_UI_LAYOUT_ORIENTATION_DEFAULT
 *         if the widget data cannot be retrieved or the part doesn't have an orientation.
 */
EOLIAN static Efl_Ui_Layout_Orientation
_efl_ui_layout_part_box_efl_ui_layout_orientable_orientation_get(const Eo *obj EINA_UNUSED, Efl_Ui_Layout_Box_Data *pd)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EFL_UI_LAYOUT_ORIENTATION_DEFAULT);

   return efl_ui_layout_orientation_get(efl_part(wd->resize_obj, pd->part));
}


/* Table proxy implementation */

/**
 * @internal
 * @brief Destructor for the Table part proxy object.
 *
 * Cleans up resources associated with the Table part proxy, including
 * unreferencing the parent layout's data and freeing the part name string.
 *
 * @param obj The Table part proxy object being destroyed.
 * @param pd The private data of the Table part proxy.
 */
EOLIAN static void
_efl_ui_layout_part_table_efl_object_destructor(Eo *obj, Efl_Ui_Layout_Table_Data *pd)
{
   ELM_PART_HOOK;
   efl_data_xunref(pd->obj, pd->sd, obj);
   eina_stringshare_del(pd->part);
   efl_destructor(efl_super(obj, TABLE_CLASS));
}

/**
 * @internal
 * @brief Provides an iterator for the content of the proxied Table part.
 *
 * Implements Efl.Container.content_iterate for the Table part proxy.
 * It retrieves the actual Evas_Object representing the Table part from the
 * Edje layout and then creates an iterator over its children.
 *
 * @param obj The Table part proxy object.
 * @param pd The private data of the Table part proxy.
 * @return An Eina_Iterator for the children of the Table part, or NULL on failure.
 *         The iterator elements are Evas_Object pointers.
 */
EOLIAN static Eina_Iterator *
_efl_ui_layout_part_table_efl_container_content_iterate(Eo *obj, Efl_Ui_Layout_Table_Data *pd)
{
   Eina_Iterator *it;

   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);

   it = evas_object_table_iterator_new(pack);

   return efl_canvas_iterator_create(obj, it, NULL);
}

/**
 * @internal
 * @brief Counts the number of items in the proxied Table part.
 *
 * Implements Efl.Container.content_count for the Table part proxy.
 * It retrieves the actual Evas_Object for the Table part and returns its child count.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @return The number of children in the Table part.
 */
EOLIAN static int
_efl_ui_layout_part_table_efl_container_content_count(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd)
{
   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);
   return evas_obj_table_count(pack);
}

/**
 * @internal
 * @brief Clears all items from the proxied Table part and deletes them.
 *
 * Implements Efl.Pack.pack_clear for the Table part proxy.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_table_efl_pack_pack_clear(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd)
{
   return _efl_ui_layout_table_clear(pd->obj, pd->sd, pd->part, EINA_TRUE);
}

/**
 * @internal
 * @brief Removes all items from the proxied Table part without deleting them.
 *
 * Implements Efl.Pack.unpack_all for the Table part proxy.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_table_efl_pack_unpack_all(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd)
{
   return _efl_ui_layout_table_clear(pd->obj, pd->sd, pd->part, EINA_FALSE);
}

/**
 * @internal
 * @brief Unpacks a specific item from the proxied Table part.
 *
 * Implements Efl.Pack.unpack for the Table part proxy.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @param subobj The Efl_Gfx_Entity to remove from the Table part.
 * @return EINA_TRUE if the item was successfully unpacked, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_table_efl_pack_unpack(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd, Efl_Gfx_Entity *subobj)
{
   if (!subobj) return EINA_FALSE;
   return _efl_ui_layout_table_unpack(pd->obj, pd->sd, pd->part, subobj) == subobj;
}

/**
 * @internal
 * @brief Packs an item into the proxied Table part, attempting to find the next available cell.
 *
 * Implements Efl.Pack.pack for the Table part proxy. This function tries to
 * find the bottom-most, right-most occupied cell and places the new item
 * to its right. If this exceeds the current column count, it moves to the
 * next row, first column. This provides a basic "flow" packing.
 *
 * @param obj The Table part proxy object.
 * @param pd The private data of the Table part proxy.
 * @param subobj The Efl_Gfx_Entity to pack into the Table part.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_table_efl_pack_pack(Eo *obj, Efl_Ui_Layout_Table_Data *pd, Efl_Gfx_Entity *subobj)
{
   int last_col = 0, last_row = 0;
   int req_cols, req_rows;
   Eina_Iterator *iter;
   Eo *pack, *element;

   pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);


   //first lookup what the most lower / right element is
   iter = evas_object_table_iterator_new(pack);
   EINA_ITERATOR_FOREACH(iter, element)
     {
        unsigned short item_col, item_row;

        evas_object_table_pack_get(pack, element, &item_col, &item_row, NULL, NULL);
        if (item_row > last_row ||
            (item_row == last_row && item_col > last_col))
          {
             last_col = item_col;
             last_row = item_row;
          }
     }
   eina_iterator_free(iter);

   //now add the new element right to it, or do a linebreak and place
   //that element in the next column on the first element
   evas_object_table_col_row_size_get(pack, &req_cols, &req_rows);
   last_col ++;
   if (last_col > req_cols)
     {
        last_row ++;
        last_col = 0;
     }

   return _efl_ui_layout_table_pack(obj, pd->sd, pd->part, subobj, last_col, last_row, 1, 1);
}


/**
 * @internal
 * @brief Packs an item into a specific cell (col, row) of the proxied Table part.
 *
 * Implements Efl.Pack_Table.pack_table for the Table part proxy.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @param subobj The Efl_Gfx_Entity to pack.
 * @param col The column to pack into (0-indexed).
 * @param row The row to pack into (0-indexed).
 * @param colspan The number of columns the item should span.
 * @param rowspan The number of rows the item should span.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_table_efl_pack_table_pack_table(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd, Efl_Gfx_Entity *subobj, int col, int row, int colspan, int rowspan)
{
   return _efl_ui_layout_table_pack(pd->obj, pd->sd, pd->part, subobj, col, row, colspan, rowspan);
}

/**
 * @internal
 * @brief Retrieves the content (item) at a specific cell (col, row) from the proxied Table part.
 *
 * Implements Efl.Pack_Table.table_content_get for the Table part proxy.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @param col The column of the cell (0-indexed).
 * @param row The row of the cell (0-indexed).
 * @return The Efl_Gfx_Entity at the specified cell, or NULL if the cell is empty or out of bounds.
 */
EOLIAN static Efl_Gfx_Entity *
_efl_ui_layout_part_table_efl_pack_table_table_content_get(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd, int col, int row)
{
   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);
   return evas_object_table_child_get(pack, col, row);
}

/**
 * @internal
 * @brief Retrieves an iterator for contents at a specific cell (col, row) in the proxied Table part.
 *
 * Implements Efl.Pack_Table.table_contents_get for the Table part proxy.
 * This can also find items that span over the given cell if `below` is EINA_TRUE.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @param col The column of the cell (0-indexed).
 * @param row The row of the cell (0-indexed).
 * @param below If EINA_TRUE, include items that span multiple cells and cover this (col, row).
 *              If EINA_FALSE, only include items that start at this (col, row).
 * @return An Eina_Iterator for the items found at or spanning the cell.
 *         The iterator elements are Evas_Object pointers. The list `l`
 *         backing the iterator must be freed by the caller of eina_iterator_free.
 *         Example: If an item spans (0,0) to (1,1), calling with (0,0, EINA_FALSE)
 *         will return it. Calling with (1,1, EINA_TRUE) will also return it.
 */
EOLIAN static Eina_Iterator *
_efl_ui_layout_part_table_efl_pack_table_table_contents_get(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd, int col, int row, Eina_Bool below)
{
   // contents at col,row - see also Efl.Ui.Table or edje_containers.c
   // not reusing edje's iterator because the container would be wrong

   Eina_List *list, *l = NULL;
   Evas_Object *sobj;
   unsigned short c, r, cs, rs;

   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);

   list = evas_object_table_children_get(pack);
   EINA_LIST_FREE(list, sobj)
     {
        evas_object_table_pack_get(pack, sobj, &c, &r, &cs, &rs);

        if (((int) c == col) && ((int) r == row))
          list = eina_list_append(list, sobj);
        else if (below)
          {
             if (((int) c <= col) && ((int) (c + cs) >= col) &&
                 ((int) r <= row) && ((int) (r + rs) >= row))
               list = eina_list_append(list, sobj);
          }
     }

   return efl_canvas_iterator_create(pd->obj, eina_list_iterator_new(l), l);
}

/**
 * @internal
 * @brief Gets the column and colspan of a specific item within the proxied Table part.
 *
 * Implements Efl.Pack_Table.table_cell_column_get for the Table part proxy.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @param subobj The Efl_Gfx_Entity whose column information is requested.
 * @param[out] col Pointer to store the starting column (0-indexed). Can be NULL.
 * @param[out] colspan Pointer to store the column span. Can be NULL.
 * @return EINA_TRUE if the item is found and information retrieved, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_table_efl_pack_table_table_cell_column_get(const Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd, Efl_Gfx_Entity * subobj, int *col, int *colspan)
{
   unsigned short c, cs;
   Eina_Bool ret;

   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);

   ret = evas_object_table_pack_get(pack, subobj, &c, NULL, &cs, NULL);
   if (col) *col = c;
   if (colspan) *colspan = cs;

   return ret;
}

/**
 * @internal
 * @brief Sets the column and colspan of a specific item within the proxied Table part.
 *
 * Implements Efl.Pack_Table.table_cell_column_set for the Table part proxy.
 * This effectively re-packs the item with new column and colspan, keeping its row and rowspan.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @param subobj The Efl_Gfx_Entity whose column information is to be set.
 * @param col The new starting column (0-indexed).
 * @param colspan The new column span.
 */
EOLIAN static void
_efl_ui_layout_part_table_efl_pack_table_table_cell_column_set(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd, Efl_Gfx_Entity * subobj, int col, int colspan)
{
   unsigned short r, rs;

   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);

   evas_object_table_pack_get(pack, subobj, NULL, &r, NULL, &rs);
   evas_object_table_pack(pack, subobj, col, r, colspan, rs);
}

/**
 * @internal
 * @brief Gets the row and rowspan of a specific item within the proxied Table part.
 *
 * Implements Efl.Pack_Table.table_cell_row_get for the Table part proxy.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @param subobj The Efl_Gfx_Entity whose row information is requested.
 * @param[out] row Pointer to store the starting row (0-indexed). Can be NULL.
 * @param[out] rowspan Pointer to store the row span. Can be NULL.
 * @return EINA_TRUE if the item is found and information retrieved, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_layout_part_table_efl_pack_table_table_cell_row_get(const Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd, Efl_Gfx_Entity * subobj, int *row, int *rowspan)
{
   unsigned short r, rs;
   Eina_Bool ret;

   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);

   ret = evas_object_table_pack_get(pack, subobj, NULL, &r, NULL, &rs);
   if (row) *row = r;
   if (rowspan) *rowspan = rs;

   return ret;
}

/**
 * @internal
 * @brief Sets the row and rowspan of a specific item within the proxied Table part.
 *
 * Implements Efl.Pack_Table.table_cell_row_set for the Table part proxy.
 * This effectively re-packs the item with new row and rowspan, keeping its column and colspan.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @param subobj The Efl_Gfx_Entity whose row information is to be set.
 * @param row The new starting row (0-indexed).
 * @param rowspan The new row span.
 */
EOLIAN static void
_efl_ui_layout_part_table_efl_pack_table_table_cell_row_set(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd, Efl_Gfx_Entity * subobj, int row, int rowspan)
{
   unsigned short c, cs;

   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);

   evas_object_table_pack_get(pack, subobj, &c, NULL, &cs, NULL);
   evas_object_table_pack(pack, subobj, c, row, cs, rowspan);
}

/**
 * @internal
 * @brief Gets the dimensions (number of columns and rows) of the proxied Table part.
 *
 * Implements Efl.Pack_Table.table_size_get for the Table part proxy.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @param[out] cols Pointer to store the number of columns. Can be NULL.
 * @param[out] rows Pointer to store the number of rows. Can be NULL.
 */
EOLIAN static void
_efl_ui_layout_part_table_efl_pack_table_table_size_get(const Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd, int *cols, int *rows)
{
   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);
   evas_object_table_col_row_size_get(pack, cols, rows);
}

/**
 * @internal
 * @brief Gets the number of columns in the proxied Table part.
 *
 * Implements Efl.Pack_Table.table_columns_get for the Table part proxy.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @return The number of columns in the table.
 */
EOLIAN static int
_efl_ui_layout_part_table_efl_pack_table_table_columns_get(const Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd)
{
   int cols, rows;

   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);
   evas_object_table_col_row_size_get(pack, &cols, &rows);
   return cols;
}

/**
 * @internal
 * @brief Gets the number of rows in the proxied Table part.
 *
 * Implements Efl.Pack_Table.table_rows_get for the Table part proxy.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy.
 * @return The number of rows in the table.
 */
EOLIAN static int
_efl_ui_layout_part_table_efl_pack_table_table_rows_get(const Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd)
{
   int cols, rows;

   edje_object_freeze(pd->obj);
   Eo *pack = (Eo *) edje_object_part_object_get(pd->obj, pd->part);
   edje_object_thaw(pd->obj);
   evas_object_table_col_row_size_get(pack, &cols, &rows);
   return rows;
}

/**
 * @internal
 * @brief Sets the number of rows for the proxied Table part. (Not Supported)
 *
 * Implements Efl.Pack_Table.table_rows_set for the Table part proxy.
 * This operation is currently not supported for Edje table parts.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy (unused).
 * @param rows The desired number of rows (unused).
 */
EOLIAN static void
_efl_ui_layout_part_table_efl_pack_table_table_rows_set(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd EINA_UNUSED, int rows EINA_UNUSED)
{
   ERR("This API is currently not supported on table parts");
}

/**
 * @internal
 * @brief Sets the number of columns for the proxied Table part. (Not Supported)
 *
 * Implements Efl.Pack_Table.table_columns_set for the Table part proxy.
 * This operation is currently not supported for Edje table parts.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy (unused).
 * @param cols The desired number of columns (unused).
 */
EOLIAN static void
_efl_ui_layout_part_table_efl_pack_table_table_columns_set(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd EINA_UNUSED, int cols EINA_UNUSED)
{
   ERR("This API is currently not supported on table parts");
}

/**
 * @internal
 * @brief Sets the dimensions (columns and rows) for the proxied Table part. (Not Supported)
 *
 * Implements Efl.Pack_Table.table_size_set for the Table part proxy.
 * This operation is currently not supported for Edje table parts.
 *
 * @param obj The Table part proxy object (unused).
 * @param pd The private data of the Table part proxy (unused).
 * @param cols The desired number of columns (unused).
 * @param rows The desired number of rows (unused).
 */
EOLIAN static void
_efl_ui_layout_part_table_efl_pack_table_table_size_set(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Table_Data *pd EINA_UNUSED, int cols EINA_UNUSED, int rows EINA_UNUSED)
{
   ERR("This API is currently not supported on table parts");
}

#include "efl_ui_layout_part_box.eo.c"
#include "efl_ui_layout_part_table.eo.c"
