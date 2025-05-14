#include "efl_ui_box_private.h"

#define MY_CLASS EFL_UI_BOX_CLASS
#define MY_CLASS_NAME "Efl.Ui.Box"

/* COPIED FROM ELM_BOX
 * - removed transition stuff (TODO: add back - needs clean API first)
 */

#define EFL_UI_BOX_DATA_GET(o, sd) \
   Efl_Ui_Box_Data *sd = efl_data_scope_get(o, EFL_UI_BOX_CLASS)

/**
 * @brief Performs the custom layout calculation for the box.
 * @param ui_box The box object.
 * @param pd The private data of the box.
 */
void _efl_ui_box_custom_layout(Efl_Ui_Box *ui_box, Efl_Ui_Box_Data *pd);

/**
 * @brief Callback triggered when a child's size changes.
 *
 * Requests a layout update for the box.
 * @param data The box object.
 * @param event The Efl_Event details (unused).
 */
static void
_on_child_size_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *box = data;
   efl_pack_layout_request(box);
}

/**
 * @brief Callback triggered when a child object is deleted.
 *
 * Removes the child from the box's internal list and requests a layout update.
 * @param data The box object.
 * @param event The Efl_Event details, where event->object is the child being deleted.
 */
static void
_on_child_del(void *data, const Efl_Event *event)
{
   Eo *box = data;
   EFL_UI_BOX_DATA_GET(box, sd);

   sd->children = eina_list_remove(sd->children, event->object);

   efl_pack_layout_request(box);
}

/**
 * @brief Callback triggered when a child's hints change.
 *
 * Requests a layout update for the box.
 * @param data The box object.
 * @param event The Efl_Event details (unused).
 */
static void
_on_child_hints_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *box = data;
   efl_pack_layout_request(box);
}

EFL_CALLBACKS_ARRAY_DEFINE(efl_ui_box_callbacks,
  { EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, _on_child_size_changed },
  { EFL_GFX_ENTITY_EVENT_HINTS_CHANGED, _on_child_hints_changed },
  { EFL_EVENT_DEL, _on_child_del }
);

/**
 * @brief Registers a child object with the box.
 *
 * This internal helper performs several actions:
 * - Checks if the sub-object is already added or invalid.
 * - Adds the sub-object to the widget if not an internal part.
 * - Sets a key "_elm_leaveme" on the child (legacy, indicates the widget manager should not remove it).
 * - Adds the child to the canvas group.
 * - Sets the clipper for the child using the box's clipper.
 * - Requests a layout update for the box.
 * - Adds event callbacks for size changes, hints changes, and deletion of the child.
 * - Emits EFL_CONTAINER_EVENT_CONTENT_ADDED.
 *
 * @param obj The box object.
 * @param pd The private data of the box.
 * @param subobj The child object to register.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_efl_ui_box_child_register(Eo *obj, Efl_Ui_Box_Data *pd, Efl_Gfx_Entity *subobj)
{
   if (!subobj || (efl_canvas_object_render_parent_get(subobj) == obj))
     {
        ERR("subobj %p %s is already added to this", subobj, efl_class_name_get(subobj) );
        return EINA_FALSE;
     }

   if (!efl_ui_widget_internal_get(obj))
     {
        if (!efl_ui_widget_sub_object_add(obj, subobj))
          return EINA_FALSE;
     }

   efl_key_data_set(subobj, "_elm_leaveme", obj);
   efl_canvas_group_member_add(obj, subobj);
   efl_canvas_object_clipper_set(subobj, pd->clipper);
   efl_pack_layout_request(obj);

   efl_event_callback_array_add(subobj, efl_ui_box_callbacks(), obj);
   efl_event_callback_call(obj, EFL_CONTAINER_EVENT_CONTENT_ADDED, subobj);

   return EINA_TRUE;
}

/**
 * @brief Unregisters a child object from the box.
 *
 * This internal helper performs several actions:
 * - Checks if the sub-object is part of this widget.
 * - Redirects the sub-object to top if not an internal part (legacy behavior).
 * - Removes the child from the canvas group.
 * - Clears the clipper for the child.
 * - Clears the "_elm_leaveme" key on the child.
 * - Requests a layout update for the box.
 * - Removes event callbacks previously added during registration.
 * - Emits EFL_CONTAINER_EVENT_CONTENT_REMOVED.
 *
 * @param obj The box object.
 * @param pd The private data of the box (unused).
 * @param subobj The child object to unregister.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_efl_ui_box_child_unregister(Eo *obj, Efl_Ui_Box_Data *pd EINA_UNUSED, Efl_Gfx_Entity *subobj)
{
   if (efl_canvas_object_render_parent_get(subobj) != obj)
     {
        ERR("subobj %p %s is not part of this widget", subobj, efl_class_name_get(subobj) );
        return EINA_FALSE;
     }

   if (!subobj)
     return EINA_FALSE;

   if (!efl_ui_widget_internal_get(obj))
     {
        if (!_elm_widget_sub_object_redirect_to_top(obj, subobj))
          return EINA_FALSE;
     }

   efl_canvas_group_member_remove(obj, subobj);
   efl_canvas_object_clipper_set(subobj, NULL);
   efl_key_data_set(subobj, "_elm_leaveme", NULL);
   efl_pack_layout_request(obj);

   efl_event_callback_array_del(subobj, efl_ui_box_callbacks(), obj);
   efl_event_callback_call(obj, EFL_CONTAINER_EVENT_CONTENT_REMOVED, subobj);

   return EINA_TRUE;
}

/**
 * @brief Callback for hints changed event on the box itself.
 *
 * Requests a layout update for the box.
 * @param data User data (unused).
 * @param ev Event info, ev->object is the box.
 */
static void
_efl_ui_box_size_hints_changed_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   efl_pack_layout_request(ev->object);
}

EOLIAN static void
_efl_ui_box_homogeneous_set(Eo *obj, Efl_Ui_Box_Data *pd, Eina_Bool homogeneous)
{
   homogeneous = !!homogeneous;

   if (pd->homogeneous == homogeneous)
     return;

   pd->homogeneous = homogeneous;
   efl_pack_layout_request(obj);
}

EOLIAN static Eina_Bool
_efl_ui_box_homogeneous_get(const Eo *obj EINA_UNUSED, Efl_Ui_Box_Data *pd)
{
   return pd->homogeneous;
}

EOLIAN static void
_efl_ui_box_efl_pack_layout_layout_update(Eo *obj, Efl_Ui_Box_Data *pd)
{
   _efl_ui_box_custom_layout(obj, pd);
}

EOLIAN static void
_efl_ui_box_efl_canvas_group_group_calculate(Eo *obj, Efl_Ui_Box_Data *_pd EINA_UNUSED)
{
   efl_canvas_group_need_recalculate_set(obj, EINA_FALSE);
   efl_pack_layout_update(obj);
}

EOLIAN static void
_efl_ui_box_efl_gfx_entity_size_set(Eo *obj, Efl_Ui_Box_Data *_pd EINA_UNUSED, Eina_Size2D sz)
{
   efl_gfx_entity_size_set(efl_super(obj, MY_CLASS), sz);
   efl_pack_layout_request(obj);
}

EOLIAN static void
_efl_ui_box_efl_gfx_entity_position_set(Eo *obj, Efl_Ui_Box_Data *_pd EINA_UNUSED, Eina_Position2D pos)
{
   efl_gfx_entity_position_set(efl_super(obj, MY_CLASS), pos);
   efl_canvas_group_change(obj);
}

EOLIAN static void
_efl_ui_box_efl_canvas_group_group_add(Eo *obj, Efl_Ui_Box_Data *pd)
{
   pd->clipper = efl_add(EFL_CANVAS_RECTANGLE_CLASS, obj);
   evas_object_static_clip_set(pd->clipper, EINA_TRUE);
   efl_gfx_entity_geometry_set(pd->clipper, EINA_RECT(-49999, -49999, 99999, 99999));
   efl_canvas_group_member_add(obj, pd->clipper);
   efl_ui_widget_sub_object_add(obj, pd->clipper);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   efl_ui_widget_focus_allow_set(obj, EINA_FALSE);
   elm_widget_highlight_ignore_set(obj, EINA_TRUE);

   efl_event_callback_add(obj, EFL_GFX_ENTITY_EVENT_HINTS_CHANGED,
                          _efl_ui_box_size_hints_changed_cb, NULL);
}

EOLIAN static void
_efl_ui_box_efl_canvas_group_group_del(Eo *obj, Efl_Ui_Box_Data *_pd EINA_UNUSED)
{
   efl_event_callback_del(obj, EFL_GFX_ENTITY_EVENT_HINTS_CHANGED,
                          _efl_ui_box_size_hints_changed_cb, NULL);

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @brief Constructor for Efl.Ui.Box.
 *
 * Initializes the box object, sets its type, accessibility role,
 * and default layout parameters.
 * @param obj The object being constructed.
 * @param pd Private data for the object.
 * @return The constructed object.
 */
EOLIAN static Eo *
_efl_ui_box_efl_object_constructor(Eo *obj, Efl_Ui_Box_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME);
   efl_access_object_access_type_set(obj, EFL_ACCESS_TYPE_SKIPPED);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_FILLER);

   pd->dir = EFL_UI_LAYOUT_ORIENTATION_VERTICAL;
   pd->align.h = 0.5;
   pd->align.v = 0.5;
   pd->full_recalc = EINA_TRUE;

   return obj;
}

/**
 * @brief Invalidates the Efl.Ui.Box object.
 *
 * Cleans up resources, specifically removing event callbacks from all children
 * before they are potentially deleted or unparented by the superclass invalidation.
 * The children list itself is freed.
 * @param obj The object being invalidated.
 * @param pd Private data for the object.
 */
EOLIAN static void
_efl_ui_box_efl_object_invalidate(Eo *obj, Efl_Ui_Box_Data *pd)
{
   Eo *child;

   efl_invalidate(efl_super(obj, MY_CLASS));

   EINA_LIST_FREE(pd->children, child)
     {
        efl_event_callback_array_del(child, efl_ui_box_callbacks(), obj);
     }
}

/* CLEAN API BELOW */

EOLIAN static int
_efl_ui_box_efl_container_content_count(Eo *obj EINA_UNUSED, Efl_Ui_Box_Data *pd)
{
   return eina_list_count(pd->children);
}

/**
 * @brief Removes all packed children from the box and deletes them.
 *
 * Iterates through all children, removes their event callbacks, and then
 * deletes each child object. Finally, requests a layout update.
 * @param obj The box object.
 * @param pd Private data of the box.
 * @return EINA_TRUE on success (always in this implementation).
 */
EOLIAN static Eina_Bool
_efl_ui_box_efl_pack_pack_clear(Eo *obj, Efl_Ui_Box_Data *pd)
{
   Eo *child;
   EINA_LIST_FREE(pd->children, child)
     {
        efl_event_callback_array_del(child, efl_ui_box_callbacks(), obj);
        efl_del(child);
     }

   efl_pack_layout_request(obj);

   return EINA_TRUE;
}

/**
 * @brief Removes all packed children from the box without deleting them.
 *
 * Iterates through all children and unregisters them using
 * `_efl_ui_box_child_unregister`. The children list becomes empty.
 * @param obj The box object.
 * @param pd Private data of the box.
 * @return EINA_TRUE if all children were successfully unregistered, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_box_efl_pack_unpack_all(Eo *obj, Efl_Ui_Box_Data *pd)
{
   Eo *child;
   Eina_Bool ret = EINA_TRUE;

   EINA_LIST_FREE(pd->children, child)
     ret &= _efl_ui_box_child_unregister(obj, pd, child);

   return ret;
}

/**
 * @brief Removes a specific child from the box without deleting it.
 *
 * Unregisters the child using `_efl_ui_box_child_unregister` and then
 * removes it from the internal list of children.
 * @param obj The box object.
 * @param pd Private data of the box.
 * @param subobj The child object to remove.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_box_efl_pack_unpack(Eo *obj, Efl_Ui_Box_Data *pd, Efl_Gfx_Entity *subobj)
{
   if (!_efl_ui_box_child_unregister(obj, pd, subobj))
     return EINA_FALSE;

   pd->children = eina_list_remove(pd->children, subobj);

   return EINA_TRUE;
}

/**
 * @brief Default pack operation. Adds an object to the end of the box.
 *
 * This is equivalent to `efl_pack_end`.
 * @param obj The box object.
 * @param pd Private data of the box (unused).
 * @param subobj The child object to pack.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_box_efl_pack_pack(Eo *obj, Efl_Ui_Box_Data *pd EINA_UNUSED, Efl_Gfx_Entity *subobj)
{
   return efl_pack_end(obj, subobj);
}

EOLIAN static Eina_Bool
_efl_ui_box_efl_pack_linear_pack_end(Eo *obj, Efl_Ui_Box_Data *pd, Efl_Gfx_Entity *subobj)
{
   if (!_efl_ui_box_child_register(obj, pd, subobj))
     return EINA_FALSE;

   pd->children = eina_list_append(pd->children, subobj);

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_ui_box_efl_pack_linear_pack_begin(Eo *obj, Efl_Ui_Box_Data *pd, Efl_Gfx_Entity *subobj)
{
   if (!_efl_ui_box_child_register(obj, pd, subobj))
     return EINA_FALSE;

   pd->children = eina_list_prepend(pd->children, subobj);

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_ui_box_efl_pack_linear_pack_before(Eo *obj, Efl_Ui_Box_Data *pd, Efl_Gfx_Entity *subobj, const Efl_Gfx_Entity *existing)
{
   if (existing) EINA_SAFETY_ON_FALSE_RETURN_VAL(eina_list_data_find(pd->children, existing), EINA_FALSE);

   if (!_efl_ui_box_child_register(obj, pd, subobj))
     return EINA_FALSE;

   pd->children = eina_list_prepend_relative(pd->children, subobj, existing);

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_ui_box_efl_pack_linear_pack_after(Eo *obj, Efl_Ui_Box_Data *pd, Efl_Gfx_Entity *subobj, const Efl_Gfx_Entity *existing)
{
   if (existing) EINA_SAFETY_ON_FALSE_RETURN_VAL(eina_list_data_find(pd->children, existing), EINA_FALSE);

   if (!_efl_ui_box_child_register(obj, pd, subobj))
     return EINA_FALSE;

   pd->children = eina_list_append_relative(pd->children, subobj, existing);

   return EINA_TRUE;
}

/**
 * @brief Packs a child object at a specific index.
 *
 * Handles negative indexing (from the end) and out-of-bounds indices
 * (packing at beginning or end). Registers the child and inserts it
 * into the children list at the specified position.
 * @param obj The box object.
 * @param pd Private data of the box.
 * @param subobj The child object to pack.
 * @param index The index at which to pack the child.
 *              Example: 0 for beginning, -1 for end (before append).
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_box_efl_pack_linear_pack_at(Eo *obj, Efl_Ui_Box_Data *pd, Efl_Gfx_Entity *subobj, int index)
{
   int count = eina_list_count(pd->children);

   if (index < -count) // Index too small, pack at beginning
     return efl_pack_begin(obj, subobj);

   if (index >= count) // Index too large, pack at end
     return efl_pack_end(obj, subobj);

   if (index < 0) // Negative index, count from the end
     index += count;

   if (!_efl_ui_box_child_register(obj, pd, subobj))
     return EINA_FALSE;

   pd->children = eina_list_prepend_relative_list(pd->children, subobj,
                                       eina_list_nth_list(pd->children, index));

   return EINA_TRUE;
}

/**
 * @brief Retrieves the child object at a specific index.
 *
 * Handles negative indexing and out-of-bounds indices (returns first or last element).
 * @param obj The box object (unused).
 * @param pd Private data of the box.
 * @param index The index of the child to retrieve.
 *              Example: 0 for first, -1 for last.
 * @return The child object at the specified index, or NULL if list is empty.
 */
EOLIAN static Efl_Gfx_Entity *
_efl_ui_box_efl_pack_linear_pack_content_get(Eo *obj EINA_UNUSED, Efl_Ui_Box_Data *pd, int index)
{
   int count = eina_list_count(pd->children);

   if (index <= -count) // Index too small, get first
     return eina_list_data_get(pd->children);

   if (index >= count) // Index too large, get last
     return eina_list_last_data_get(pd->children);

   if (index < 0) // Negative index, count from the end
     index += count;

   return eina_list_nth(pd->children, index);
}

/**
 * @brief Unpacks (removes) the child object at a specific index.
 *
 * Retrieves the content at the index and then unpacks it.
 * @param obj The box object.
 * @param pd Private data of the box (unused).
 * @param index The index of the child to unpack.
 * @return The unpacked child object, or NULL on failure or if no child at index.
 */
EOLIAN static Efl_Gfx_Entity *
_efl_ui_box_efl_pack_linear_pack_unpack_at(Eo *obj, Efl_Ui_Box_Data *pd EINA_UNUSED, int index)
{
   Efl_Gfx_Entity *content;

   content = efl_pack_content_get(obj, index);
   if (!content) return NULL;

   if (!efl_pack_unpack(obj, content))
     return NULL;

   return content;
}

EOLIAN static int
_efl_ui_box_efl_pack_linear_pack_index_get(Eo *obj EINA_UNUSED, Efl_Ui_Box_Data *pd, const Efl_Gfx_Entity *subobj)
{
   return eina_list_data_idx(pd->children, (Efl_Gfx_Entity *)subobj);
}

/**
 * @brief Requests a layout update for the box.
 *
 * Marks the box for a full recalculation and signals that the canvas group
 * needs recalculation. `full_recalc` typically indicates that cached sizes
 * or positions might be invalid and need to be recomputed from scratch.
 * @param obj The box object.
 * @param pd Private data of the box.
 */
EOLIAN static void
_efl_ui_box_efl_pack_layout_layout_request(Eo *obj, Efl_Ui_Box_Data *pd)
{
   pd->full_recalc = EINA_TRUE;
   efl_canvas_group_need_recalculate_set(obj, EINA_TRUE);
}

EOLIAN static Eina_Iterator *
_efl_ui_box_efl_container_content_iterate(Eo *obj EINA_UNUSED, Efl_Ui_Box_Data *pd)
{
   return eina_list_iterator_new(pd->children);
}

EOLIAN static void
_efl_ui_box_efl_ui_layout_orientable_orientation_set(Eo *obj, Efl_Ui_Box_Data *pd, Efl_Ui_Layout_Orientation dir)
{
   if (pd->dir == dir) return;

   switch (dir)
     {
      case EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL:
        pd->dir = EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL;
        break;

      case EFL_UI_LAYOUT_ORIENTATION_VERTICAL:
      case EFL_UI_LAYOUT_ORIENTATION_DEFAULT:
      default:
        pd->dir = EFL_UI_LAYOUT_ORIENTATION_VERTICAL;
        break;
     }

   efl_pack_layout_request(obj);
}

EOLIAN static Efl_Ui_Layout_Orientation
_efl_ui_box_efl_ui_layout_orientable_orientation_get(const Eo *obj EINA_UNUSED, Efl_Ui_Box_Data *pd)
{
   return pd->dir;
}

EOLIAN static void
_efl_ui_box_efl_gfx_arrangement_content_padding_set(Eo *obj, Efl_Ui_Box_Data *pd, unsigned int h, unsigned int v)
{
   if (EINA_DBL_EQ(pd->pad.h, h) && EINA_DBL_EQ(pd->pad.v, v))
     return;

   pd->pad.h = h;
   pd->pad.v = v;

   efl_pack_layout_request(obj);
}

EOLIAN static void
_efl_ui_box_efl_gfx_arrangement_content_padding_get(const Eo *obj EINA_UNUSED, Efl_Ui_Box_Data *pd, unsigned int *h, unsigned int *v)
{
   if (h) *h = pd->pad.h;
   if (v) *v = pd->pad.v;
}

EOLIAN static void
_efl_ui_box_efl_gfx_arrangement_content_align_set(Eo *obj, Efl_Ui_Box_Data *pd, double h, double v)
{
   if (h < 0) h = -1;
   else if (h > 1) h = 1;
   if (v < 0) v = -1;
   else if (v > 1) v = 1;

   if (EINA_DBL_EQ(pd->align.h, h) && EINA_DBL_EQ(pd->align.v, v))
     return;

   pd->align.h = h;
   pd->align.v = v;

   efl_pack_layout_request(obj);
}

EOLIAN static void
_efl_ui_box_efl_gfx_arrangement_content_align_get(const Eo *obj EINA_UNUSED, Efl_Ui_Box_Data *pd, double *h, double *v)
{
   if (h) *h = pd->align.h;
   if (v) *v = pd->align.v;
}

/* Internal EO APIs and hidden overrides */

#define EFL_UI_BOX_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(efl_ui_box)

#include "efl_ui_box.eo.c"
