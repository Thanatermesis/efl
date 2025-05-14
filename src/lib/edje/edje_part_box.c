#include "edje_private.h"
#include "edje_part_helper.h"
#include "efl_canvas_layout_part_box.eo.h"
#define MY_CLASS EFL_CANVAS_LAYOUT_PART_BOX_CLASS

#include "../evas/canvas/evas_box_eo.h"

PROXY_IMPLEMENTATION(box, MY_CLASS, EINA_FALSE)
#undef PROXY_IMPLEMENTATION

/* Legacy features */

/**
 * @brief Removes all packed sub-objects from the box part.
 *
 * This function clears all child objects from the box part, effectively
 * making it empty. The removed objects may or may not be deleted
 * depending on the underlying Edje implementation.
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_box_efl_pack_pack_clear(Eo *obj, void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_part_box_remove_all(pd->ed, pd->part, EINA_TRUE);
}

/**
 * @brief Removes all packed sub-objects from the box part without deleting them.
 *
 * This function unparks all child objects from the box part.
 * The objects are removed from the box's control but are not deleted.
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_box_efl_pack_unpack_all(Eo *obj, void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_part_box_remove_all(pd->ed, pd->part, EINA_FALSE);
}

/**
 * @brief Removes a specific sub-object from the box part.
 *
 * If the given @p subobj is a child of this box part, it is removed.
 * The removed object is not deleted.
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @param subobj The sub-object to remove.
 * @return @c EINA_TRUE if the object was successfully removed, @c EINA_FALSE otherwise
 *         (e.g., if @p subobj was not a child).
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_box_efl_pack_unpack(Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx_Entity *subobj)
{
   Evas_Object *removed;
   PROXY_DATA_GET(obj, pd);
   removed = _edje_part_box_remove(pd->ed, pd->part, subobj);
   return (removed == subobj);
}

/**
 * @brief Adds a sub-object to the end of the box part.
 *
 * This is a legacy packing function, equivalent to pack_end.
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @param subobj The sub-object to pack.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_box_efl_pack_pack(Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx_Entity *subobj)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_part_box_append(pd->ed, pd->part, subobj);
}

/**
 * @brief Adds a sub-object to the beginning of the box part.
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @param subobj The sub-object to pack at the beginning.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_box_efl_pack_linear_pack_begin(Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx_Entity *subobj)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_part_box_prepend(pd->ed, pd->part, subobj);
}

/**
 * @brief Adds a sub-object to the end of the box part.
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @param subobj The sub-object to pack at the end.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_box_efl_pack_linear_pack_end(Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx_Entity *subobj)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_part_box_append(pd->ed, pd->part, subobj);
}

/**
 * @brief Inserts a sub-object into the box part before a specified existing sub-object.
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @param subobj The sub-object to insert.
 * @param existing The existing sub-object before which @p subobj will be inserted.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., if @p existing is not found).
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_box_efl_pack_linear_pack_before(Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx_Entity *subobj, const Efl_Gfx_Entity *existing)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_part_box_insert_before(pd->ed, pd->part, subobj, existing);
}

/**
 * @brief Inserts a sub-object into the box part after a specified existing sub-object.
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @param subobj The sub-object to insert.
 * @param existing The existing sub-object after which @p subobj will be inserted.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., if @p existing is not found).
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_box_efl_pack_linear_pack_after(Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx_Entity *subobj, const Efl_Gfx_Entity *existing)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_part_box_insert_after(pd->ed, pd->part, subobj, existing);
}

/**
 * @brief Inserts a sub-object into the box part at a specific numerical index.
 *
 * If @p index is negative, it counts from the end of the box
 * (e.g., -1 is the last position). If @p index is out of bounds
 * (after adjusting for negative values), the @p subobj is appended to the end.
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @param subobj The sub-object to insert.
 * @param index The numerical index at which to insert.
 *              Example: 0 for beginning, -1 for end (before appending).
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_box_efl_pack_linear_pack_at(Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx_Entity *subobj, int index)
{
   PROXY_DATA_GET(obj, pd);
   int cnt = efl_content_count(obj);
   // Normalize negative index: -1 means insert at cnt, -2 at cnt-1, etc.
   // If index is - (cnt + 1), it means insert at 0.
   if ((index < 0) && ((-index) <= (cnt + 1)))
     index = cnt + index + 1;
   if ((index >= 0) && (index < cnt))
     return _edje_part_box_insert_at(pd->ed, pd->part, subobj, index);
   else
     return _edje_part_box_append(pd->ed, pd->part, subobj);
}

/**
 * @brief Removes and returns the sub-object at a specific numerical index.
 *
 * If @p index is negative, it counts from the end of the box
 * (e.g., -1 is the last item). The removed object is not deleted.
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @param index The numerical index of the sub-object to remove.
 *              Example: 0 for first, -1 for last.
 * @return The removed sub-object, or @c NULL if the index is out of bounds or on failure.
 */
EOLIAN static Efl_Gfx_Entity *
_efl_canvas_layout_part_box_efl_pack_linear_pack_unpack_at(Eo *obj, void *_pd EINA_UNUSED, int index)
{
   PROXY_DATA_GET(obj, pd);
   // Normalize negative index: -1 means last element, -2 second to last, etc.
   if (index < 0) index += efl_content_count(obj);
   return _edje_part_box_remove_at(pd->ed, pd->part, index);
}

/* New APIs with Eo */

/**
 * @brief Retrieves the sub-object packed at a specific numerical index.
 *
 * If @p index is negative, it counts from the end of the box
 * (e.g., -1 is the last item).
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @param index The numerical index of the sub-object to retrieve.
 *              Example: 0 for first, -1 for last.
 * @return The sub-object at the given @p index, or @c NULL if the index is out of bounds.
 */
EOLIAN static Efl_Gfx_Entity *
_efl_canvas_layout_part_box_efl_pack_linear_pack_content_get(Eo *obj, void *_pd EINA_UNUSED, int index)
{
   PROXY_DATA_GET(obj, pd);
   // Normalize negative index
   if (index < 0) index += efl_content_count(obj);
   return _edje_part_box_content_at(pd->ed, pd->part, index);
}

/**
 * @brief Retrieves the numerical index of a specific sub-object within the box part.
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @param subobj The sub-object to find the index of.
 * @return The numerical index of @p subobj, or -1 if not found or on failure.
 *         Example: 0 for the first element.
 */
EOLIAN static int
_efl_canvas_layout_part_box_efl_pack_linear_pack_index_get(Eo *obj, void *_pd EINA_UNUSED, const Efl_Gfx_Entity * subobj)
{
   Evas_Object_Box_Option *opt;
   Evas_Object_Box_Data *priv;
   Eina_List *l;
   int k = 0;

   PROXY_DATA_GET(obj, pd);
   priv = efl_data_scope_get(pd->rp->object, EVAS_BOX_CLASS);
   if (!priv) return -1;
   EINA_LIST_FOREACH(priv->children, l, opt)
     {
        if (opt->obj == subobj)
          return k;
        k++;
     }
   return -1;
}

/**
 * @brief Returns an iterator over the sub-objects packed in this box part.
 *
 * The iterator will provide Efl_Gfx_Entity pointers.
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @return An Eina_Iterator for the packed sub-objects, or @c NULL on failure or if empty.
 *         The caller is responsible for freeing the iterator.
 */
EOLIAN static Eina_Iterator *
_efl_canvas_layout_part_box_efl_container_content_iterate(Eo *obj, void *_pd EINA_UNUSED)
{
   Eina_Iterator *it;

   PROXY_DATA_GET(obj, pd);
   if (!pd->rp->typedata.container) return NULL;
   it = evas_object_box_iterator_new(pd->rp->object);

   return efl_canvas_iterator_create(pd->rp->object, it, NULL);
}

/**
 * @brief Retrieves the number of sub-objects currently packed in this box part.
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @return The count of packed sub-objects.
 */
EOLIAN static int
_efl_canvas_layout_part_box_efl_container_content_count(Eo *obj, void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   return evas_obj_box_count(pd->rp->object);
}

/**
 * @brief Gets the orientation of the box part as defined in its description.
 *
 * This reads the layout property from the Edje part description (e.g., "horizontal", "vertical").
 *
 * @param obj The Efl_Canvas_Layout_Part_Box object.
 * @param _pd Private data, unused in this function.
 * @return The orientation of the box.
 *         Returns #EFL_UI_LAYOUT_ORIENTATION_DEFAULT if not specified or unknown.
 *         Example: #EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL, #EFL_UI_LAYOUT_ORIENTATION_VERTICAL.
 */
EOLIAN static Efl_Ui_Layout_Orientation
_efl_canvas_layout_part_box_efl_ui_layout_orientable_orientation_get(const Eo *obj, void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   const Edje_Part_Description_Box *desc =
         (Edje_Part_Description_Box *) pd->rp->chosen_description;

   if (!desc || !desc->box.layout)
     return EFL_UI_LAYOUT_ORIENTATION_DEFAULT;

   if (!strncmp(desc->box.layout, "vertical", 8))
     return EFL_UI_LAYOUT_ORIENTATION_VERTICAL;
   else if (!strncmp(desc->box.layout, "horizontal", 10))
     return EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL;

   WRN("unknown orientation '%s'", desc->box.layout);
   return EFL_UI_LAYOUT_ORIENTATION_DEFAULT;
}

#include "efl_canvas_layout_part_box.eo.c"
