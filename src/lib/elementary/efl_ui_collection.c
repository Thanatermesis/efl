#ifdef HAVE_CONFIG_H
#include "elementary_config.h"
#endif

#define ELM_LAYOUT_PROTECTED
#define EFL_UI_SCROLL_MANAGER_PROTECTED
#define EFL_UI_SCROLLBAR_PROTECTED
#define EFL_UI_WIDGET_FOCUS_MANAGER_PROTECTED

#include <Efl_Ui.h>
#include <Elementary.h>
#include "elm_widget.h"
#include "elm_priv.h"
#include "efl_ui_collection_focus_manager.eo.h"

/**
 * @brief Private data for the Efl_Ui_Collection_Focus_Manager class.
 */
typedef struct {
   Eo *collection; /**< The collection object this focus manager belongs to. */
} Efl_Ui_Collection_Focus_Manager_Data;

/**
 * @brief Structure to optimize access to items in an Eina_List.
 *
 * This structure caches the last accessed item and its index to speed up
 * subsequent accesses, especially for sequential or nearby elements.
 */
typedef struct {
   unsigned int last_index; /**< Index of the last accessed item. */
   const Eina_List *current; /**< Pointer to the last accessed Eina_List node. */
   Eina_List **items; /**< Pointer to the Eina_List itself. */
} Fast_Accessor;

/**
 * @brief Retrieves an Eina_List node at a specific index using the fast accessor.
 *
 * This function optimizes list traversal by starting from the last accessed
 * position, or by choosing the shortest path from the beginning or end of the list.
 *
 * @param accessor The fast accessor instance.
 * @param idx The index of the item to retrieve.
 * @return The Eina_List node at the given index, or NULL if out of bounds or not found.
 */
static const Eina_List*
_fast_accessor_get_at(Fast_Accessor *accessor, unsigned int idx)
{
   const Eina_List *over;
   unsigned int middle;
   unsigned int i;

   if (idx >= eina_list_count(*accessor->items))
     return NULL;

   if (accessor->last_index == idx)
     over = accessor->current;
   else if (idx > accessor->last_index)
     {
        /* After current position. */
        middle = ((eina_list_count(*accessor->items) - accessor->last_index))/2;

        if (idx > middle)
          /* Go backward from the end. */
          for (i = eina_list_count(*accessor->items) - 1,
               over = eina_list_last(*accessor->items);
               i > idx && over;
               --i, over = eina_list_prev(over))
            ;
        else
          /* Go forward from current. */
          for (i = accessor->last_index, over = accessor->current;
               i < idx && over;
               ++i, over = eina_list_next(over))
            ;
     }
   else
     {
        /* Before current position. */
        middle = accessor->last_index/2;

        if (idx > middle)
          /* Go backward from current. */
          for (i = accessor->last_index, over = accessor->current;
               i > idx && over;
               --i, over = eina_list_prev(over))
            ;
        else
          /* Go forward from start. */
          for (i = 0, over = *accessor->items;
               i < idx && over;
               ++i, over = eina_list_next(over))
            ;
     }

   if (!over)
     return NULL;

   accessor->last_index = idx;
   accessor->current = over;

   return over;
}

/**
 * @brief Initializes a fast accessor.
 *
 * Sets up the accessor to work with the provided Eina_List.
 * This is a workaround for cases where an accessor might be needed
 * before the list is fully populated.
 *
 * @param accessor The fast accessor instance to initialize.
 * @param items A pointer to the Eina_List that will be accessed.
 */
static void
_fast_accessor_init(Fast_Accessor *accessor, Eina_List **items)
{
   //this is the accessor for accessing the items
   //we have to workaround here the problem that
   //no accessor can be created for a not yet created list.
   accessor->items = items;
}

/**
 * @brief Updates the fast accessor when an element is removed from the list.
 *
 * If the removed element was the currently cached element, this function
 * adjusts the cache to point to a nearby element (next, previous, or NULL
 * if the list becomes empty or the removed element was the only one).
 *
 * @param accessor The fast accessor instance.
 * @param removed_elem The Eina_List node that was removed.
 */
static void
_fast_accessor_remove(Fast_Accessor *accessor, const Eina_List *removed_elem)
{
   if (accessor->current == removed_elem)
     {
        Eina_List *next;
        Eina_List *prev;

        next = eina_list_next(removed_elem);
        prev = eina_list_prev(removed_elem);
        if (next)
          {
             accessor->current = next;
             accessor->last_index ++;
          }
        else if (prev)
          {
             accessor->current = prev;
             accessor->last_index --;
          }
        else
          {
             //everything >= length is invalid, and we need that.
             accessor->last_index = eina_list_count(*accessor->items);
             accessor->current = NULL;
          }

     }

}

#define MY_CLASS      EFL_UI_COLLECTION_CLASS

#define MY_DATA_GET(obj, pd) \
  Efl_Ui_Collection_Data *pd = efl_data_scope_get(obj, MY_CLASS);

/**
 * @brief Private data for the Efl_Ui_Collection class.
 */
typedef struct {
   Efl_Ui_Scroll_Manager *smanager; /**< Scroll manager for the collection. */
   Efl_Ui_Pan *pan; /**< Pan object used for scrolling content. */
   Eina_List *selected; /**< List of currently selected items. */
   Eina_List *items; /**< List of all items in the collection. */
   Efl_Ui_Selection *fallback; /**< Fallback item to select when selection is empty. */
   Efl_Ui_Select_Mode mode; /**< Current selection mode (single, multi, none). */
   Efl_Ui_Layout_Orientation dir; /**< Layout orientation (vertical, horizontal). */
   Eina_Size2D content_min_size; /**< Minimum size of the content. */
   Efl_Ui_Position_Manager_Entity *pos_man; /**< Position manager for item layout. */
   Eina_Future *selection_changed_job; /**< Future for debouncing selection changed events. */
   struct {
      Eina_Bool w; /**< Match content width. */
      Eina_Bool h; /**< Match content height. */
   } match_content; /**< Flags for matching content size. */
   Fast_Accessor obj_accessor; /**< Fast accessor for item objects. */
   Fast_Accessor size_accessor; /**< Fast accessor for item sizes. */
   Efl_Gfx_Entity *sizer; /**< Sizer object for the pan content. */
   unsigned int start_id, end_id; /**< Range of currently visible item IDs. */
   Eina_Bool allow_manual_deselection : 1; /**< Whether manual deselection is allowed. */
   Eina_Bool api_selection_change : 1; /**< Flag to indicate if selection change is API-driven. */
} Efl_Ui_Collection_Data;

static Eina_Bool register_item(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Ui_Item *item);
static Eina_Bool unregister_item(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Ui_Item *item);

/**
 * @brief Flushes the minimum size of the collection to its graphics hint.
 *
 * This function updates the restricted minimum size hint of the collection object
 * based on the current content_min_size and match_content flags.
 * If match_content.w is false, width is set to -1 (unrestricted).
 * If match_content.h is false, height is set to -1 (unrestricted).
 *
 * @param obj The collection object.
 * @param pd The private data of the collection.
 */
static void
flush_min_size(Eo *obj, Efl_Ui_Collection_Data *pd)
{
   Eina_Size2D tmp = pd->content_min_size;

   if (!pd->match_content.w)
     tmp.w = -1;

   if (!pd->match_content.h)
     tmp.h = -1;

   efl_gfx_hint_size_restricted_min_set(obj, tmp);
}

/**
 * @brief Clamps an index to indicate its position relative to list bounds.
 *
 * @param pd The private data of the collection.
 * @param index The index to clamp.
 * @return -1 if index is before the start, 1 if after the end, 0 if within bounds (inclusive of negative indexing).
 * For example, if list count is 5:
 *   index -10 -> returns -1
 *   index  -5 -> returns 0 (valid negative index)
 *   index   0 -> returns 0
 *   index   4 -> returns 0
 *   index   5 -> returns 1
 *   index  10 -> returns 1
 */
static int
clamp_index(Efl_Ui_Collection_Data *pd, int index)
{
   if (index < ((int)eina_list_count(pd->items)) * -1)
     return -1;
   else if (index > (int)eina_list_count(pd->items) - 1)
     return 1;
   return 0;
}

/**
 * @brief Adjusts a potentially negative or out-of-bounds index to a valid list index.
 *
 * Negative indices are counted from the end of the list.
 * Indices beyond the list bounds are clamped to the first or last valid index.
 *
 * @param pd The private data of the collection.
 * @param index The index to adjust.
 * @return A valid index within the range [0, count-1]. If the list is empty, behavior might be unexpected for positive indices.
 * For example, if list count is 5:
 *   index -10 -> returns 0
 *   index  -5 -> returns 0
 *   index  -1 -> returns 4
 *   index   0 -> returns 0
 *   index   4 -> returns 4
 *   index   5 -> returns 4
 *   index  10 -> returns 4
 */
static int
index_adjust(Efl_Ui_Collection_Data *pd, int index)
{
   int c = eina_list_count(pd->items);
   if (index < c * -1)
     return 0;
   else if (index > c - 1)
     return c - 1;
   else if (index < 0)
     return index + c;
   return index;
}

/**
 * @brief Callback for when the pan viewport geometry changes.
 *
 * Updates the position manager with the new viewport.
 *
 * @param data The collection object.
 * @param ev The event information (unused).
 */
static void
_pan_viewport_changed_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   MY_DATA_GET(data, pd);
   Eina_Rect rect = efl_ui_scrollable_viewport_geometry_get(data);

   efl_ui_position_manager_entity_viewport_set(pd->pos_man, rect);
}

/**
 * @brief Callback for when the pan content position changes.
 *
 * Updates the position manager with the new relative scroll position.
 *
 * @param data The collection object.
 * @param ev The event information, containing the new Eina_Position2D.
 */
static void
_pan_position_changed_cb(void *data, const Efl_Event *ev)
{
   MY_DATA_GET(data, pd);
   Eina_Position2D *pos = ev->info;
   Eina_Position2D max = efl_ui_pan_position_max_get(pd->pan);
   Eina_Vector2 rpos = {0.0, 0.0};

   if (max.x > 0.0)
     rpos.x = (double)pos->x/(double)max.x;
   if (max.y > 0.0)
     rpos.y = (double)pos->y/(double)max.y;

   efl_ui_position_manager_entity_scroll_position_set(pd->pos_man, rpos.x, rpos.y);
}

EFL_CALLBACKS_ARRAY_DEFINE(pan_events_cb,
  {EFL_UI_PAN_EVENT_PAN_CONTENT_POSITION_CHANGED, _pan_position_changed_cb},
  {EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, _pan_viewport_changed_cb},
  {EFL_GFX_ENTITY_EVENT_POSITION_CHANGED, _pan_viewport_changed_cb},
)

/**
 * @brief Internal function to scroll an item into view.
 *
 * Calculates the item's position relative to the viewport and scrolls
 * the scroll manager to make the item visible.
 *
 * @param obj The collection object (unused).
 * @param pd The private data of the collection.
 * @param item The item to scroll into view.
 * @param align Alignment parameter (currently unused, FIXME).
 * @param anim Whether to animate the scroll.
 */
static void
_item_scroll_internal(Eo *obj EINA_UNUSED,
                      Efl_Ui_Collection_Data *pd,
                      Efl_Ui_Item *item,
                      double align EINA_UNUSED,
                      Eina_Bool anim)
{
   Eina_Rect ipos, view;
   Eina_Position2D vpos;

   if (!pd->smanager) return;

   ipos = efl_ui_position_manager_entity_position_single_item(pd->pos_man, eina_list_data_idx(pd->items, item));
   view = efl_ui_scrollable_viewport_geometry_get(pd->smanager);
   vpos = efl_ui_scrollable_content_pos_get(pd->smanager);

   // Adjust item position to be relative to the scrollable content origin
   ipos.x = ipos.x + vpos.x - view.x;
   ipos.y = ipos.y + vpos.y - view.y;

   //FIXME scrollable needs some sort of align, the docs do not even garantee to completely move in the element
   efl_ui_scrollable_scroll(pd->smanager, ipos, anim);
}

EOLIAN static void
_efl_ui_collection_item_scroll(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Ui_Item *item, Eina_Bool animation)
{
   _item_scroll_internal(obj, pd, item, -1.0, animation);
}

EOLIAN static void
_efl_ui_collection_item_scroll_align(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Ui_Item *item, double align, Eina_Bool animation)
{
   _item_scroll_internal(obj, pd, item, align, animation);
}

EOLIAN static Efl_Ui_Selectable*
_efl_ui_collection_efl_ui_single_selectable_last_selected_get(const Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd)
{
   return eina_list_last_data_get(pd->selected);
}

EOLIAN static Eina_Iterator*
_efl_ui_collection_efl_ui_multi_selectable_object_range_selected_iterator_new(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd)
{
   return eina_list_iterator_new(pd->selected);
}

/**
 * @brief Fills depth information for an item.
 *
 * Determines if an item is a group leader, a child of a group, or a standalone item,
 * and sets the depth accordingly.
 * Depth 0: Standalone item.
 * Depth 1: Group leader or child of a group.
 *
 * @param item The item to check.
 * @param depth Pointer to store the calculated depth.
 * @param leader Pointer to store whether the item is a depth leader (group header).
 */
static inline void
_fill_depth(Eo *item, unsigned char *depth, Eina_Bool *leader)
{
   if (efl_isa(item, EFL_UI_GROUP_ITEM_CLASS))
     {
        *depth = 1; // Group item itself is a leader at depth 1
        *leader = EINA_TRUE;
     }
   else if (efl_ui_item_parent_get(item))
     {
        *depth = 1; // Child of a group item is at depth 1, but not a leader
        *leader = EINA_FALSE;
     }
   else
     {
        *leader = EINA_FALSE; // Standalone item
        *depth = 0;
     }
}

/**
 * @brief Accessor function for the position manager to get sizes of a batch of items.
 *
 * This function is called by the position manager to retrieve the minimum combined
 * size and depth information for a range of items.
 *
 * @param data The Fast_Accessor for sizes.
 * @param conf Configuration for the size call, including the range of items.
 *             `conf.range.start_id` is the starting index.
 *             `conf.range.end_id` is the ending index (exclusive).
 * @param memory A writable slice of memory to store the Efl_Ui_Position_Manager_Size_Batch_Entity results.
 *               Each Efl_Ui_Position_Manager_Size_Batch_Entity should be filled with:
 *               - `size`: The Eina_Size2D of the item.
 *               - `element_depth`: The depth of the item (0 for root, 1 for child/group).
 *               - `depth_leader`: EINA_TRUE if this item is a group leader.
 * @return Efl_Ui_Position_Manager_Size_Batch_Result containing:
 *         - `filled_items`: The number of items for which size info was written.
 *         - `parent_size`: If the first item in the batch is part of a group and not a leader,
 *                          this is the size of its parent group item.
 */
static Efl_Ui_Position_Manager_Size_Batch_Result
_size_accessor_get_at(void *data, Efl_Ui_Position_Manager_Size_Call_Config conf, Eina_Rw_Slice memory)
{
   Fast_Accessor *accessor = data;
   size_t i;
   const Eina_List *lst = _fast_accessor_get_at(accessor, conf.range.start_id);
   Efl_Ui_Position_Manager_Size_Batch_Entity *sizes = memory.mem;
   Efl_Ui_Position_Manager_Size_Batch_Result result = {0};

   EINA_SAFETY_ON_NULL_RETURN_VAL(lst, result);

   for (i = 0; i < (conf.range.end_id - conf.range.start_id); ++i)
     {
         Efl_Gfx_Entity *geom = eina_list_data_get(lst), *parent;
         Eina_Size2D size = efl_gfx_hint_size_combined_min_get(geom);

         parent = efl_ui_item_parent_get(geom);
         sizes[i].size = size;
         _fill_depth(geom, &sizes[i].element_depth, &sizes[i].depth_leader);
         if (i == 0 && !sizes[0].depth_leader && parent)
           {
              result.parent_size = efl_gfx_hint_size_combined_min_get(parent);
           }
         lst = eina_list_next(lst);
         if (!lst)
           {
              i++;
              break;
           }
     }
   result.filled_items = i;

   return result;
}

/**
 * @brief Accessor function for the position manager to get a batch of item objects.
 *
 * This function is called by the position manager to retrieve the Efl_Gfx_Entity
 * objects and depth information for a range of items.
 *
 * @param data The Fast_Accessor for objects.
 * @param range The range of items to retrieve.
 *              `range.start_id` is the starting index.
 *              `range.end_id` is the ending index (exclusive).
 * @param memory A writable slice of memory to store the Efl_Ui_Position_Manager_Object_Batch_Entity results.
 *               Each Efl_Ui_Position_Manager_Object_Batch_Entity should be filled with:
 *               - `entity`: The Efl_Gfx_Entity of the item.
 *               - `element_depth`: The depth of the item.
 *               - `depth_leader`: EINA_TRUE if this item is a group leader.
 * @return Efl_Ui_Position_Manager_Object_Batch_Result containing:
 *         - `filled_items`: The number of items for which object info was written.
 *         - `group`: If the first item in the batch is part of a group and not a leader,
 *                    this is the Efl_Gfx_Entity of its parent group item.
 */
static Efl_Ui_Position_Manager_Object_Batch_Result
_obj_accessor_get_at(void *data, Efl_Ui_Position_Manager_Request_Range range, Eina_Rw_Slice memory)
{
   Fast_Accessor *accessor = data;
   size_t i;
   const Eina_List *lst = _fast_accessor_get_at(accessor, range.start_id);
   Efl_Ui_Position_Manager_Object_Batch_Entity *objs = memory.mem;
   Efl_Ui_Position_Manager_Object_Batch_Result result = {0};

   for (i = 0; i < range.end_id - range.start_id; ++i)
     {
         Efl_Gfx_Entity *geom = eina_list_data_get(lst), *parent;

         parent = efl_ui_item_parent_get(geom);
         objs[i].entity = geom;
         _fill_depth(geom, &objs[i].element_depth, &objs[i].depth_leader);
         if (i == 0 && !objs[0].depth_leader && parent)
           {
              result.group = parent;
           }

         lst = eina_list_next(lst);
         if (!lst)
           {
              i++;
              break;
           }
     }
   result.filled_items = i;

   return result;
}


EOLIAN static Efl_Object*
_efl_ui_collection_efl_object_constructor(Eo *obj, Efl_Ui_Collection_Data *pd EINA_UNUSED)
{
   Eo *o;

   efl_ui_selectable_allow_manual_deselection_set(obj, EINA_TRUE);

   pd->dir = EFL_UI_LAYOUT_ORIENTATION_VERTICAL;

   _fast_accessor_init(&pd->obj_accessor, &pd->items);
   _fast_accessor_init(&pd->size_accessor, &pd->items);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "collection");

   o = efl_constructor(efl_super(obj, MY_CLASS));

   pd->sizer = efl_add(EFL_CANVAS_RECTANGLE_CLASS, evas_object_evas_get(obj));
   efl_gfx_color_set(pd->sizer, 0, 0, 0, 0);

   pd->pan = efl_add(EFL_UI_PAN_CLASS, obj);
   efl_content_set(pd->pan, pd->sizer);
   efl_event_callback_array_add(pd->pan, pan_events_cb(), obj);

   pd->smanager = efl_add(EFL_UI_SCROLL_MANAGER_CLASS, obj);
   efl_composite_attach(obj, pd->smanager);
   efl_ui_mirrored_set(pd->smanager, efl_ui_mirrored_get(obj));
   efl_ui_scroll_manager_pan_set(pd->smanager, pd->pan);

   efl_ui_scroll_connector_bind(obj, pd->smanager);

   return o;
}

EOLIAN static Efl_Object*
_efl_ui_collection_efl_object_finalize(Eo *obj, Efl_Ui_Collection_Data *pd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->pos_man, NULL);

   return efl_finalize(efl_super(obj, MY_CLASS));
}

EOLIAN static Eina_Error
_efl_ui_collection_efl_ui_widget_theme_apply(Eo *obj, Efl_Ui_Collection_Data *pd)
{
   Eina_Error res;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EFL_UI_THEME_APPLY_ERROR_GENERIC);
   res = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (res == EFL_UI_THEME_APPLY_ERROR_GENERIC) return res;
   efl_ui_mirrored_set(pd->smanager, efl_ui_mirrored_get(obj));
   efl_content_set(efl_part(wd->resize_obj, "efl.content"), pd->pan);

   return res;
}

EOLIAN static void
_efl_ui_collection_efl_object_destructor(Eo *obj, Efl_Ui_Collection_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Deselects all currently selected items in the collection.
 *
 * Iterates through the list of selected items and sets their selected state to EINA_FALSE.
 * This function directly modifies the selection state of items, which might trigger
 * individual item selection changed events.
 *
 * @param pd The private data of the collection.
 */
static void
deselect_all(Efl_Ui_Collection_Data *pd)
{
   while(pd->selected)
     {
        Eo *item = eina_list_data_get(pd->selected);
        // Setting selected to FALSE will trigger _selection_changed,
        // which removes the item from pd->selected.
        efl_ui_selectable_selected_set(item, EINA_FALSE);
        // Safety check: ensure the item was indeed removed or list became NULL.
        EINA_SAFETY_ON_TRUE_RETURN(pd->selected && eina_list_data_get(pd->selected) == item);
     }
}

EOLIAN static void
_efl_ui_collection_efl_object_invalidate(Eo *obj, Efl_Ui_Collection_Data *pd EINA_UNUSED)
{
   efl_ui_collection_position_manager_set(obj, NULL);

   efl_ui_selectable_fallback_selection_set(obj, NULL);

   deselect_all(pd);

   while(pd->items)
     efl_del(pd->items->data);

   // pan is given to edje, which reparents it, which forces us to manually deleting it
   efl_del(pd->pan);

   efl_invalidate(efl_super(obj, MY_CLASS));
}

EOLIAN static Eina_Iterator*
_efl_ui_collection_efl_container_content_iterate(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd)
{
   return eina_list_iterator_new(pd->items);
}

EOLIAN static int
_efl_ui_collection_efl_container_content_count(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd)
{
   return eina_list_count(pd->items);
}

EOLIAN static void
_efl_ui_collection_efl_ui_layout_orientable_orientation_set(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd, Efl_Ui_Layout_Orientation dir)
{
   if (pd->dir == dir) return;

   pd->dir = dir;
   if (pd->pos_man)
     efl_ui_layout_orientation_set(pd->pos_man, dir);
}

EOLIAN static Efl_Ui_Layout_Orientation
_efl_ui_collection_efl_ui_layout_orientable_orientation_get(const Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd)
{
   return pd->dir;
}

EOLIAN static void
_efl_ui_collection_efl_ui_scrollable_match_content_set(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd, Eina_Bool w, Eina_Bool h)
{
   if (pd->match_content.w == w && pd->match_content.h == h)
     return;

   pd->match_content.w = w;
   pd->match_content.h = h;

   efl_ui_scrollable_match_content_set(pd->smanager, w, h);
   flush_min_size(obj, pd);
}

EOLIAN static void
_efl_ui_collection_efl_ui_multi_selectable_select_mode_set(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd, Efl_Ui_Select_Mode mode)
{
   pd->mode = mode;
   if ((mode == EFL_UI_SELECT_MODE_SINGLE) &&
       eina_list_count(pd->selected) > 0)
     {
        Efl_Ui_Item *last = eina_list_last_data_get(pd->selected);

        pd->selected = eina_list_remove_list(pd->selected, eina_list_last(pd->selected));
        deselect_all(pd);
        pd->selected = eina_list_append(pd->selected, last);
     }
   else if (mode == EFL_UI_SELECT_MODE_NONE && pd->selected)
     {
        deselect_all(pd);
     }
}

EOLIAN static Efl_Ui_Select_Mode
_efl_ui_collection_efl_ui_multi_selectable_select_mode_get(const Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd)
{
   return pd->mode;
}

/**
 * @brief Callback for the scheduled selection changed job.
 *
 * This function is executed after a short delay (via efl_loop_job)
 * to coalesce multiple selection changes into a single SELECTION_CHANGED event.
 *
 * @param o The collection object.
 * @param data User data (unused).
 * @param value Future value (unused).
 * @return EINA_VALUE_EMPTY.
 */
static Eina_Value
_schedule_selection_job_cb(Eo *o, void *data EINA_UNUSED, const Eina_Value value EINA_UNUSED)
{
   MY_DATA_GET(o, pd);

   pd->selection_changed_job = NULL;

   efl_event_callback_call(o, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, NULL);

   return EINA_VALUE_EMPTY;
}

/**
 * @brief Schedules a job to emit the SELECTION_CHANGED event.
 *
 * This is used to debounce selection changes, ensuring that the
 * SELECTION_CHANGED event is emitted only once after a series of
 * rapid selection updates.
 *
 * @param obj The collection object.
 * @param pd The private data of the collection.
 */
static void
_schedule_selection_changed(Eo *obj, Efl_Ui_Collection_Data *pd)
{
   Eina_Future *f;

   if (pd->selection_changed_job) return; // Job already scheduled

   f = efl_loop_job(efl_main_loop_get());
   pd->selection_changed_job = efl_future_then(obj, f, _schedule_selection_job_cb);

}

/**
 * @brief Applies the fallback selection if no items are currently selected.
 *
 * If a fallback item is set and the `pd->selected` list is empty,
 * this function selects the fallback item.
 *
 * @param obj The collection object (unused).
 * @param pd The private data of the collection.
 */
static void
_apply_fallback(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd)
{
   if (pd->fallback && !pd->selected)
     {
        efl_ui_selectable_selected_set(pd->fallback, EINA_TRUE);
     }
}

/**
 * @brief Handles selection logic for single selection mode.
 *
 * If the `new_selection` is already selected, it deselects it (if manual deselection is allowed,
 * though that check is typically done by the item itself before emitting SELECTED_CHANGED).
 * Otherwise, it deselects all currently selected items. The `new_selection` will be
 * added to `pd->selected` by the caller (`_selection_changed`).
 *
 * @param obj The collection object (unused).
 * @param pd The private data of the collection.
 * @param new_selection The item whose selection state is changing.
 */
static inline void
_single_selection_behaviour(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd, Efl_Ui_Selectable *new_selection)
{
   // If the item being selected is already the sole selected item.
   if (eina_list_data_get(pd->selected) == new_selection)
     {
       // This path is typically taken when an item is deselected.
       // `deselect_all` (called by item's selected_set(FALSE)) would clear pd->selected.
       // Here, we ensure pd->selected is cleared if it somehow still points to new_selection.
       // This effectively means if the item is clicked again to deselect it (and it's the only one selected),
       // pd->selected will be cleared.
       pd->selected = eina_list_free(pd->selected);
     }
   else
     {
        // A new item is being selected, or an additional item in a mode that became single.
        // Deselect all other items.
        deselect_all(pd);
     }
}

/**
 * @brief Handles changes in an item's selection state.
 *
 * This function is called when an item within the collection emits the
 * EFL_UI_EVENT_SELECTED_CHANGED event. It updates the collection's
 * internal list of selected items (`pd->selected`) based on the selection mode
 * and the item's new state. It also handles fallback selection and schedules
 * the emission of the collection's own SELECTION_CHANGED event.
 *
 * @param data The collection object.
 * @param ev The event information, where `ev->object` is the item whose
 *           selection changed, and `ev->info` is a pointer to an Eina_Bool
 *           indicating the new selection state (EINA_TRUE for selected).
 */
static void
_selection_changed(void *data, const Efl_Event *ev)
{
   Eina_Bool selection = *((Eina_Bool*) ev->info);
   Eo *obj = data;
   MY_DATA_GET(obj, pd);
   Efl_Ui_Selection *fallback;

   //if this is the highest call in the tree of selection changes, safe the fallback and apply it later
   //this way we ensure that we are not accidentally adding fallback even if we just want to have a empty selection list
   fallback = pd->fallback;
   pd->fallback = NULL;

   if (selection)
     {
        if (pd->mode == EFL_UI_SELECT_MODE_SINGLE)
          {
             _single_selection_behaviour(obj, pd, ev->object);
          }
        else if (pd->mode == EFL_UI_SELECT_MODE_MULTI && _elm_config->desktop_entry && !pd->api_selection_change)
          {
             const Evas_Modifier *mod = evas_key_modifier_get(evas_object_evas_get(ev->object));
             if (!(efl_input_clickable_interaction_get(ev->object)
                   && evas_key_modifier_is_set(mod, "Control")))
               _single_selection_behaviour(obj, pd, ev->object);
          }
        else if (pd->mode == EFL_UI_SELECT_MODE_NONE)
          {
             ERR("Selection while mode is NONE, uncaught state!");
             return;
          }
        pd->selected = eina_list_append(pd->selected, ev->object);
     }
   else
     {
        pd->selected = eina_list_remove(pd->selected, ev->object);
     }

   pd->fallback = fallback;
   _apply_fallback(obj, pd);
   _schedule_selection_changed(obj, pd);
}

/**
 * @brief Callback for when an item in the collection is invalidated.
 *
 * This function unregisters the invalidated item from the collection.
 *
 * @param data The collection object.
 * @param ev The event information, where `ev->object` is the item being invalidated.
 */
static void
_invalidate_cb(void *data, const Efl_Event *ev)
{
   Eo *obj = data;
   MY_DATA_GET(obj, pd);

   unregister_item(obj, pd, ev->object);
}

/**
 * @brief Callback for when an item's hints (size, etc.) change.
 *
 * Notifies the position manager that the item's size has changed,
 * so the layout can be updated.
 *
 * @param data The collection object.
 * @param ev The event information, where `ev->object` is the item whose hints changed.
 */
static void
_hints_changed_cb(void *data, const Efl_Event *ev)
{
   Eo *obj = data;
   MY_DATA_GET(obj, pd);
   int idx = eina_list_data_idx(pd->items, ev->object);

   if (idx != -1 && pd->pos_man) // Ensure item is found and pos_man exists
     efl_ui_position_manager_entity_item_size_changed(pd->pos_man, idx, idx);
}

/**
 * @brief Redirects input events from items to collection-level item events.
 *
 * This function listens to input events (pressed, unpressed, longpressed, clicked)
 * on individual items and re-emits them as corresponding EFL_UI_EVENT_ITEM_*
 * events from the collection itself. The event info is augmented to include
 * the item that originated the event.
 *
 * @param data The collection object.
 * @param ev The input event from an item. `ev->object` is the item.
 */
static void
_redirect_cb(void *data, const Efl_Event *ev)
{
   Eo *obj = data;

#define REDIRECT_EVT(Desc, Item_Desc)                           \
   if (Desc == ev->desc)                                        \
     {                                                          \
        Efl_Ui_Item_Clickable_Clicked item_clicked;             \
        Efl_Input_Clickable_Clicked *clicked = ev->info;        \
                                                                \
        item_clicked.clicked = *clicked;                        \
        item_clicked.item = ev->object;                         \
                                                                \
        efl_event_callback_call(obj, Item_Desc, &item_clicked); \
     }
#define REDIRECT_EVT_PRESS(Desc, Item_Desc)                           \
   if (Desc == ev->desc)                                        \
     {                                                          \
        Efl_Ui_Item_Clickable_Pressed item_pressed;             \
        int *button = ev->info;        \
                                                                \
        item_pressed.button = *button;                        \
        item_pressed.item = ev->object;                         \
                                                                \
        efl_event_callback_call(obj, Item_Desc, &item_pressed); \
     }

   REDIRECT_EVT_PRESS(EFL_INPUT_EVENT_PRESSED, EFL_UI_EVENT_ITEM_PRESSED);
   REDIRECT_EVT_PRESS(EFL_INPUT_EVENT_UNPRESSED, EFL_UI_EVENT_ITEM_UNPRESSED);
   REDIRECT_EVT_PRESS(EFL_INPUT_EVENT_LONGPRESSED, EFL_UI_EVENT_ITEM_LONGPRESSED);
   REDIRECT_EVT(EFL_INPUT_EVENT_CLICKED_ANY, EFL_UI_EVENT_ITEM_CLICKED_ANY);
   REDIRECT_EVT(EFL_INPUT_EVENT_CLICKED, EFL_UI_EVENT_ITEM_CLICKED);
#undef REDIRECT_EVT
#undef REDIRECT_EVT_PRESS
}

EFL_CALLBACKS_ARRAY_DEFINE(active_item,
  {EFL_GFX_ENTITY_EVENT_HINTS_CHANGED, _hints_changed_cb},
  {EFL_UI_EVENT_SELECTED_CHANGED, _selection_changed},
  {EFL_INPUT_EVENT_PRESSED, _redirect_cb},
  {EFL_INPUT_EVENT_UNPRESSED, _redirect_cb},
  {EFL_INPUT_EVENT_LONGPRESSED, _redirect_cb},
  {EFL_INPUT_EVENT_CLICKED, _redirect_cb},
  {EFL_INPUT_EVENT_CLICKED_ANY, _redirect_cb},
  {EFL_EVENT_INVALIDATE, _invalidate_cb},
)

static Eina_Bool
register_item(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Ui_Item *item)
{
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(item, EFL_UI_ITEM_CLASS), EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!!eina_list_data_find(pd->items, item), EINA_FALSE);

   if (!efl_ui_widget_sub_object_add(obj, item))
     return EINA_FALSE;

   efl_ui_item_container_set(item, obj);
   efl_canvas_group_member_add(pd->pan, item);
   efl_event_callback_array_add(item, active_item(), obj);
   efl_ui_mirrored_set(item, efl_ui_mirrored_get(obj));

   return EINA_TRUE;
}

/**
 * @brief Unregisters an item from the collection.
 *
 * This involves removing it as a sub-object, removing it from internal lists,
 * detaching event callbacks, notifying the position manager, and cleaning up
 * its relationship with the collection.
 *
 * @param obj The collection object.
 * @param pd The private data of the collection.
 * @param item The item to unregister.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., item not found).
 */
static Eina_Bool
unregister_item(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Ui_Item *item)
{
   Eina_List *elem = eina_list_data_find_list(pd->items, item);
   if (!elem)
     {
        ERR("Item %p is not part of this widget", item);
        return EINA_FALSE;
     }

   if (!efl_ui_widget_sub_object_del(obj, item))
     return EINA_FALSE;

   unsigned int id = eina_list_data_idx(pd->items, item);

   _fast_accessor_remove(&pd->obj_accessor, elem);
   _fast_accessor_remove(&pd->size_accessor, elem);

   pd->items = eina_list_remove_list(pd->items, elem); // Use remove_list with the found elem
   pd->selected = eina_list_remove(pd->selected, item); // item might not be in selected list, this is fine
   efl_event_callback_array_del(item, active_item(), obj);
   if (pd->pos_man)
     efl_ui_position_manager_entity_item_removed(pd->pos_man, id, item);
   efl_ui_item_container_set(item, NULL);
   efl_canvas_group_member_remove(pd->pan, item);

   return EINA_TRUE;
}

/**
 * @brief Updates the position manager after an item is added.
 *
 * Notifies the position manager about the newly added item and its index.
 * Also, resets fast accessors if the item is added at the beginning.
 *
 * @param obj The collection object (unused).
 * @param pd The private data of the collection.
 * @param subobj The item that was added.
 */
static void
update_pos_man(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd, Efl_Gfx_Entity *subobj)
{
   int id = eina_list_data_idx(pd->items, subobj);
   if (id == 0)
     {
        pd->obj_accessor.last_index = id;
        pd->obj_accessor.current = pd->items;
        pd->size_accessor.last_index = id;
        pd->size_accessor.current = pd->items;
     }
   if (pd->pos_man)
     efl_ui_position_manager_entity_item_added(pd->pos_man, id, subobj);
}

/**
 * @brief Fetches the representative parent (group item) of an item in a list node.
 *
 * @param lst The Eina_List node containing the item.
 * @return The parent Efl_Ui_Item if it's a group, otherwise NULL.
 *         Returns NULL if `lst` is NULL.
 */
static inline Efl_Ui_Item*
fetch_rep_parent(Eina_List *lst)
{
   if (!lst)
     return NULL;

   Efl_Ui_Item *it = eina_list_data_get(lst);

   return efl_ui_item_parent_get(it); // This returns the group item if 'it' is a child
}

/**
 * @brief Checks the integrity of item grouping when an item is inserted.
 *
 * This function verifies that an item being inserted maintains consistent
 * grouping. For example, an item should not be inserted into the middle of
 * another group if it doesn't belong to that group, or between a group header
 * and its children if it's not part of that group.
 * If an integrity violation is detected, an error is logged, and the
 * offending item is unregistered.
 *
 * @param obj The collection object.
 * @param pd The private data of the collection.
 * @param subobj The item that was just inserted and needs its group integrity checked.
 * @return EINA_TRUE if integrity is maintained, EINA_FALSE if a violation occurred
 *         and the item was unregistered.
 */
static Eina_Bool
check_group_integrity(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd, Efl_Gfx_Entity *subobj)
{
   Eina_List *carrier_list = eina_list_data_find_list(pd->items, subobj), *prev_lst;
   Efl_Ui_Item *next_parent, *prev_parent, *carrier_parent;

   prev_lst = eina_list_prev(carrier_list);
   next_parent = fetch_rep_parent(eina_list_next(carrier_list));
   prev_parent = fetch_rep_parent(prev_lst);
   carrier_parent = fetch_rep_parent(carrier_list);

   // Case 1: Item inserted into the middle of an existing group, but doesn't belong to it.
   // Example: [GroupA_Child1, NewItem_NoGroup, GroupA_Child2]
   // Here, prev_parent is GroupA, next_parent is GroupA, carrier_parent is NULL.
   if (next_parent && next_parent == prev_parent && carrier_parent != prev_parent)
     {
        //a item got inserted into the middle of one group, but does not have the correct group header, that is a bug
        ERR("Inserting a item with the wrong group into another group (prev_parent:%p, carrier_parent:%p, next_parent:%p)", prev_parent, carrier_parent, next_parent);
        unregister_item(obj, pd, subobj);
        return EINA_FALSE;
     }

   // Case 2: Item inserted between a group header and its children, but doesn't belong to that group.
   // This check seems complex. `eina_list_data_get(prev_lst) == next_parent` implies prev_lst item is the group header for next_parent's group.
   // If carrier_parent is not this group header, it's an error.
   // Example: [GroupA_Header, NewItem_NoGroup, GroupA_Child1]
   // Here, prev_lst contains GroupA_Header. next_parent is GroupA. carrier_parent is NULL.
   // `eina_list_data_get(prev_lst)` would be GroupA_Header. `next_parent` would be GroupA.
   // This condition might be `efl_isa(eina_list_data_get(prev_lst), EFL_UI_GROUP_ITEM_CLASS) && eina_list_data_get(prev_lst) == next_parent`
   // and `carrier_parent != next_parent`.
   if (prev_lst && efl_isa(eina_list_data_get(prev_lst), EFL_UI_GROUP_ITEM_CLASS) && eina_list_data_get(prev_lst) == next_parent && carrier_parent != next_parent)
     {
        //a item got inserted between group header and group children, also a error
        ERR("Inserting a item between group header and group elements (prev_item_header:%p, carrier_parent:%p, next_item_parent:%p)", eina_list_data_get(prev_lst), carrier_parent, next_parent);
        unregister_item(obj, pd, subobj);
        return EINA_FALSE;
     }
   // Case 3: Item with a group parent inserted, but its neighbors don't match its group, or it's at an edge incorrectly.
   // Example: [OtherItem, GroupB_Child1_With_ParentB] where OtherItem is not ParentB.
   // `!next_parent && !prev_parent` means item is at an edge or standalone regarding neighbors' parents.
   // `carrier_parent` means the item itself has a parent.
   // `prev_lst && eina_list_data_get(prev_lst) != carrier_parent` means previous item is not its parent.
   // This implies it's the first child of its group, but the item before it is not its group header.
   if (!next_parent && !prev_parent && carrier_parent && prev_lst && eina_list_data_get(prev_lst) != carrier_parent)
     {
        ERR("Tried to insert a item with group, outside its group (next_parent:%p, prev_parent:%p, carrier_parent:%p, prev_item:%p)", next_parent, prev_parent, carrier_parent, eina_list_data_get(prev_lst));
        unregister_item(obj, pd, subobj);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_ui_collection_efl_pack_pack_clear(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd)
{
   while(pd->items)
     {
        efl_del(pd->items->data);
     }

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_ui_collection_efl_pack_unpack_all(Eo *obj, Efl_Ui_Collection_Data *pd)
{
   while(pd->items)
     {
        if (!unregister_item(obj, pd, pd->items->data))
          return EINA_FALSE;
     }
   return EINA_TRUE;
}

EOLIAN static Efl_Gfx_Entity*
_efl_ui_collection_efl_pack_linear_pack_unpack_at(Eo *obj, Efl_Ui_Collection_Data *pd, int index)
{
   Efl_Ui_Item *it = eina_list_nth(pd->items, index_adjust(pd, index));

   EINA_SAFETY_ON_NULL_RETURN_VAL(it, NULL);

   if (!unregister_item(obj, pd, it))
     return NULL;

   return it;
}

EOLIAN static Eina_Bool
_efl_ui_collection_efl_pack_unpack(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Gfx_Entity *subobj)
{
   return unregister_item(obj, pd, subobj);
}


EOLIAN static Eina_Bool
_efl_ui_collection_efl_pack_pack(Eo *obj, Efl_Ui_Collection_Data *pd EINA_UNUSED, Efl_Gfx_Entity *subobj)
{
   return efl_pack_end(obj, subobj);
}


EOLIAN static Eina_Bool
_efl_ui_collection_efl_pack_linear_pack_end(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Gfx_Entity *subobj)
{
   if (!register_item(obj, pd, subobj))
     return EINA_FALSE;
   pd->items = eina_list_append(pd->items, subobj);
   update_pos_man(obj, pd, subobj);
   return EINA_TRUE;
}


EOLIAN static Eina_Bool
_efl_ui_collection_efl_pack_linear_pack_begin(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Gfx_Entity *subobj)
{
   if (!register_item(obj, pd, subobj))
     return EINA_FALSE;
   pd->items = eina_list_prepend(pd->items, subobj);
   update_pos_man(obj, pd, subobj);
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_ui_collection_efl_pack_linear_pack_before(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Gfx_Entity *subobj, const Efl_Gfx_Entity *existing)
{
   Eina_List *subobj_list = eina_list_data_find_list(pd->items, existing);
   if (existing)
     EINA_SAFETY_ON_NULL_RETURN_VAL(subobj_list, EINA_FALSE);

   if (!register_item(obj, pd, subobj))
     return EINA_FALSE;
   pd->items = eina_list_prepend_relative_list(pd->items, subobj, subobj_list);
   if (!check_group_integrity(obj, pd, subobj)) return EINA_FALSE;
   update_pos_man(obj, pd, subobj);
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_ui_collection_efl_pack_linear_pack_after(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Gfx_Entity *subobj, const Efl_Gfx_Entity *existing)
{
   Eina_List *subobj_list = eina_list_data_find_list(pd->items, existing);
   if (existing)
     EINA_SAFETY_ON_NULL_RETURN_VAL(subobj_list, EINA_FALSE);

   if (!register_item(obj, pd, subobj))
     return EINA_FALSE;
   pd->items = eina_list_append_relative_list(pd->items, subobj, subobj_list);
   if (!check_group_integrity(obj, pd, subobj)) return EINA_FALSE;
   update_pos_man(obj, pd, subobj);
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_ui_collection_efl_pack_linear_pack_at(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Gfx_Entity *subobj, int index)
{
   Eina_List *subobj_list;
   int clamp;

   clamp = clamp_index(pd, index);
   index = index_adjust(pd, index);
   subobj_list = eina_list_nth_list(pd->items, index);
   if (pd->items)
     EINA_SAFETY_ON_NULL_RETURN_VAL(subobj_list, EINA_FALSE);
   if (!register_item(obj, pd, subobj))
     return EINA_FALSE;
   if (clamp == 0)
     pd->items = eina_list_prepend_relative_list(pd->items, subobj, subobj_list);
   else if (clamp == 1)
     pd->items = eina_list_append(pd->items, subobj);
   else
     pd->items = eina_list_prepend(pd->items, subobj);
   if (!check_group_integrity(obj, pd, subobj)) return EINA_FALSE;
   update_pos_man(obj, pd, subobj);
   return EINA_TRUE;
}

EOLIAN static int
_efl_ui_collection_efl_pack_linear_pack_index_get(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd, const Efl_Gfx_Entity *subobj)
{
   return eina_list_data_idx(pd->items, (void*)subobj);
}

EOLIAN static Efl_Gfx_Entity*
_efl_ui_collection_efl_pack_linear_pack_content_get(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd, int index)
{
   return eina_list_nth(pd->items, index_adjust(pd, index));
}

static void
_pos_content_size_changed_cb(void *data, const Efl_Event *ev)
{
   Eina_Size2D *size = ev->info;
   MY_DATA_GET(data, pd);

   efl_gfx_entity_size_set(pd->sizer, *size);
}

/**
 * @brief Callback for when the position manager reports a change in content minimum size.
 *
 * Updates the collection's internal `content_min_size` and then calls
 * `flush_min_size` to apply this to the collection's hints.
 *
 * @param data The collection object.
 * @param ev The event information, where `ev->info` is a pointer to Eina_Size2D.
 */
static void
_pos_content_min_size_changed_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   Eina_Size2D *size = ev->info;
   MY_DATA_GET(data, pd);

   pd->content_min_size = *size;

   flush_min_size(data, pd);
}

/**
 * @brief Callback for when the position manager reports a change in the visible range of items.
 *
 * Updates the collection's internal `start_id` and `end_id` which track the
 * indices of the first and last (exclusive end) visible items.
 *
 * @param data The collection object.
 * @param ev The event information, where `ev->info` is a pointer to
 *           Efl_Ui_Position_Manager_Range_Update.
 *           `info->start_id` is the first visible item index.
 *           `info->end_id` is one past the last visible item index.
 */
static void
_visible_range_changed_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   Efl_Ui_Position_Manager_Range_Update *info = ev->info;
   MY_DATA_GET(data, pd);

   pd->start_id = info->start_id;
   pd->end_id = info->end_id;
}

EFL_CALLBACKS_ARRAY_DEFINE(pos_manager_cbs,
  {EFL_UI_POSITION_MANAGER_ENTITY_EVENT_CONTENT_SIZE_CHANGED, _pos_content_size_changed_cb},
  {EFL_UI_POSITION_MANAGER_ENTITY_EVENT_CONTENT_MIN_SIZE_CHANGED, _pos_content_min_size_changed_cb},
  {EFL_UI_POSITION_MANAGER_ENTITY_EVENT_VISIBLE_RANGE_CHANGED, _visible_range_changed_cb}
)

EOLIAN static void
_efl_ui_collection_position_manager_set(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Ui_Position_Manager_Entity *layouter)
{
   if (layouter)
     EINA_SAFETY_ON_FALSE_RETURN(efl_isa(layouter, EFL_UI_POSITION_MANAGER_ENTITY_INTERFACE));

   if (pd->pos_man)
     {
        efl_event_callback_array_del(pd->pos_man, pos_manager_cbs(), obj);
        efl_del(pd->pos_man);
     }
   pd->pos_man = layouter;
   if (pd->pos_man)
     {
        efl_parent_set(pd->pos_man, obj);
        efl_event_callback_array_add(pd->pos_man, pos_manager_cbs(), obj);
        switch(efl_ui_position_manager_entity_version(pd->pos_man, 1))
          {
            case 1:
              efl_ui_position_manager_data_access_v1_data_access_set(pd->pos_man,
                efl_provider_find(obj, EFL_UI_WIN_CLASS),
                &pd->obj_accessor, _obj_accessor_get_at, NULL,
                &pd->size_accessor, _size_accessor_get_at, NULL,
                eina_list_count(pd->items));
            break;
          }

        efl_ui_position_manager_entity_viewport_set(pd->pos_man, efl_ui_scrollable_viewport_geometry_get(obj));
        efl_ui_layout_orientation_set(pd->pos_man, pd->dir);
     }
}

EOLIAN static Efl_Ui_Position_Manager_Entity*
_efl_ui_collection_position_manager_get(const Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd)
{
  return pd->pos_man;
}

EOLIAN static Efl_Ui_Focus_Manager*
_efl_ui_collection_efl_ui_widget_focus_manager_focus_manager_create(Eo *obj, Efl_Ui_Collection_Data *pd EINA_UNUSED, Efl_Ui_Focus_Object *root)
{
   Eo *man = efl_add(EFL_UI_COLLECTION_FOCUS_MANAGER_CLASS, obj,
                 efl_ui_focus_manager_root_set(efl_added, root));
   Efl_Ui_Collection_Focus_Manager_Data *fm_pd = efl_data_scope_safe_get(man, EFL_UI_COLLECTION_FOCUS_MANAGER_CLASS);
   EINA_SAFETY_ON_NULL_RETURN_VAL(fm_pd, NULL);
   fm_pd->collection = obj;
   return man;
}

EOLIAN static Eina_Bool
_efl_ui_collection_efl_ui_widget_focus_state_apply(Eo *obj, Efl_Ui_Collection_Data *pd EINA_UNUSED, Efl_Ui_Widget_Focus_State current_state, Efl_Ui_Widget_Focus_State *configured_state, Efl_Ui_Widget *redirect EINA_UNUSED)
{
   return efl_ui_widget_focus_state_apply(efl_super(obj, MY_CLASS), current_state, configured_state, obj);
}

/**
 * @brief Finds the Efl_Ui_Item that contains or is the given focused_element.
 *
 * Traverses up the widget hierarchy from `focused_element` until an
 * Efl_Ui_Item is found, or until the top of the hierarchy is reached.
 * This is used to map a focused sub-element back to its containing item
 * in the collection.
 *
 * @param obj The collection object (unused).
 * @param pd The private data of the collection (unused).
 * @param focused_element The element that currently has focus, or is a candidate.
 * @return The Efl_Ui_Item containing `focused_element`, or `focused_element` itself
 *         if it is an Efl_Ui_Item. Returns NULL if no Efl_Ui_Item is found in the
 *         ancestry or if `focused_element` is NULL.
 */
static Efl_Ui_Item *
_find_item(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd EINA_UNUSED, Eo *focused_element)
{
   if (!focused_element) return NULL;

   // Traverse up until an Efl_Ui_Item is found or no parent.
   while (focused_element && !efl_isa(focused_element, EFL_UI_ITEM_CLASS))
     {
        focused_element = efl_ui_widget_parent_get(focused_element);
     }

   return focused_element;
}

EOLIAN static Efl_Ui_Focus_Object*
_efl_ui_collection_efl_ui_focus_manager_move(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Ui_Focus_Direction direction)
{
   Eo *new_obj, *focus;
   Eina_Size2D step;

   focus = efl_ui_focus_manager_focus_get(obj);
   new_obj = efl_ui_focus_manager_move(efl_super(obj, MY_CLASS), direction);
   step = efl_gfx_hint_size_combined_min_get(focus);

   if (new_obj)
     {
        /* if this is outside the viewport, then we must bring that in first */
        Eina_Rect viewport;
        Eina_Rect element;
        element = efl_gfx_entity_geometry_get(focus);
        viewport = efl_gfx_entity_geometry_get(obj);
        if (!eina_spans_intersect(element.x, element.w, viewport.x, viewport.w) &&
            !eina_spans_intersect(element.y, element.h, viewport.y, viewport.h))
          {
             efl_ui_scrollable_scroll(obj, element, EINA_TRUE);
             return focus;
          }
     }

   if (!new_obj)
     {
        Eina_Rect pos = efl_gfx_entity_geometry_get(focus);
        Eina_Rect view = efl_ui_scrollable_viewport_geometry_get(pd->smanager);
        Eina_Position2D vpos = efl_ui_scrollable_content_pos_get(pd->smanager);

        pos.x = pos.x + vpos.x - view.x;
        pos.y = pos.y + vpos.y - view.y;
        Eina_Position2D max = efl_ui_pan_position_max_get(pd->pan);

        if (direction == EFL_UI_FOCUS_DIRECTION_RIGHT)
          {
             if (pos.x < max.x)
               {
                  pos.x = MIN(max.x, pos.x + step.w);
                  efl_ui_scrollable_scroll(obj, pos, EINA_TRUE);
                  new_obj = focus;
               }
          }
        else if (direction == EFL_UI_FOCUS_DIRECTION_LEFT)
          {
             if (pos.x > 0)
               {
                  pos.x = MAX(0, pos.x - step.w);
                  efl_ui_scrollable_scroll(obj, pos, EINA_TRUE);
                  new_obj = focus;
               }
          }
        else if (direction == EFL_UI_FOCUS_DIRECTION_UP)
          {
             if (pos.y > 0)
               {
                  pos.y = MAX(0, pos.y - step.h);
                  efl_ui_scrollable_scroll(obj, pos, EINA_TRUE);
                  new_obj = focus;
               }
          }
        else if (direction == EFL_UI_FOCUS_DIRECTION_DOWN)
          {
             if (pos.y < max.y)
               {
                  pos.y = MAX(0, pos.y + step.h);
                  efl_ui_scrollable_scroll(obj, pos, EINA_TRUE);
                  new_obj = focus;
               }
          }
     }
   else
     {
        _item_scroll_internal(obj, pd, efl_provider_find(new_obj, EFL_UI_ITEM_CLASS), .0, EINA_TRUE);
     }

   return new_obj;
}

/**
 * @brief Applies a selection state to a range of items starting from a given list node.
 *
 * Iterates through the Eina_List starting from `start` and sets the selected
 * state of each Efl_Ui_Selectable item to `flag`.
 *
 * @param start The Eina_List node to start applying the selection state from.
 *              The iteration proceeds through `eina_list_next()`.
 * @param flag The selection state to apply (EINA_TRUE for select, EINA_FALSE for unselect).
 */
static void
_selectable_range_apply(Eina_List *start, Eina_Bool flag)
{
   Efl_Ui_Selectable *sel;
   Eina_List *n;

   EINA_LIST_FOREACH(start, n, sel)
     {
        efl_ui_selectable_selected_set(sel, flag);
     }
}

EOLIAN static void
_efl_ui_collection_efl_ui_multi_selectable_all_select(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd)
{
   pd->api_selection_change = EINA_TRUE;
   _selectable_range_apply(pd->items, EINA_TRUE);
   pd->api_selection_change = EINA_FALSE;
}

EOLIAN static void
_efl_ui_collection_efl_ui_multi_selectable_all_unselect(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd)
{
   pd->api_selection_change = EINA_TRUE;
   _selectable_range_apply(pd->items, EINA_FALSE);
   pd->api_selection_change = EINA_FALSE;
}

/**
 * @brief Selects or unselects a range of items between two specified items (inclusive).
 *
 * Iterates through all items in the collection. Once the first item (`a` or `b`)
 * is found, it starts applying the selection `flag` to it and all subsequent items
 * until the second item (`b` or `a`) is found and processed.
 * The order of `a` and `b` in the list determines the range.
 *
 * @param obj The collection object.
 * @param pd The private data of the collection.
 * @param a One boundary of the range.
 * @param b The other boundary of the range.
 * @param flag EINA_TRUE to select the range, EINA_FALSE to unselect.
 */
static void
_range_selection_find(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Ui_Selectable *a, Efl_Ui_Selectable *b, Eina_Bool flag)
{
   Eina_List *n;
   Efl_Ui_Selectable *c;
   Eina_List *first_node = NULL, *second_node = NULL;
   Eina_Bool in_range = EINA_FALSE;

   EINA_SAFETY_ON_FALSE_RETURN(efl_ui_widget_parent_get(a) == obj);
   EINA_SAFETY_ON_FALSE_RETURN(efl_ui_widget_parent_get(b) == obj);

   // Find the list nodes for a and b to determine order
   EINA_LIST_FOREACH(pd->items, n, c)
     {
        if (c == a) first_node = n;
        if (c == b) second_node = n;
        if (first_node && second_node) break; // Found both
     }

   // If one or both items are not in the list, do nothing.
   if (!first_node || !second_node) return;

   // Determine which one comes first.
   // This requires iterating again or knowing indices. For simplicity, iterate again.
   // A more efficient way would be to get indices of a and b.
   Efl_Ui_Selectable *start_item = NULL, *end_item = NULL;
   int idx_a = eina_list_data_idx(pd->items, a);
   int idx_b = eina_list_data_idx(pd->items, b);

   if (idx_a <= idx_b)
     {
        start_item = a;
        end_item = b;
     }
   else
     {
        start_item = b;
        end_item = a;
     }

   pd->api_selection_change = EINA_TRUE;
   EINA_LIST_FOREACH(pd->items, n, c)
     {
        if (c == start_item)
          in_range = EINA_TRUE;

        if (in_range)
          efl_ui_selectable_selected_set(c, flag);

        if (c == end_item)
          {
             // If we started with end_item because it came first, ensure start_item (which is later) is also processed.
             // This logic ensures the item itself is processed before breaking.
             if (in_range) break;
          }
     }
   pd->api_selection_change = EINA_FALSE;
   _schedule_selection_changed(obj, pd); // Schedule a single update
}

EOLIAN static void
_efl_ui_collection_efl_ui_multi_selectable_object_range_range_select(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Ui_Selectable *a, Efl_Ui_Selectable *b)
{
   _range_selection_find(obj, pd, a, b, EINA_TRUE);
}

EOLIAN static void
_efl_ui_collection_efl_ui_multi_selectable_object_range_range_unselect(Eo *obj, Efl_Ui_Collection_Data *pd, Efl_Ui_Selectable *a, Efl_Ui_Selectable *b)
{
   _range_selection_find(obj, pd, a, b, EINA_FALSE);
}

EOLIAN static void
_efl_ui_collection_efl_ui_single_selectable_fallback_selection_set(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd, Efl_Ui_Selectable *fallback)
{
   pd->fallback = fallback;
   _apply_fallback(obj, pd);
}

EOLIAN static Efl_Ui_Selectable*
_efl_ui_collection_efl_ui_single_selectable_fallback_selection_get(const Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd)
{
   return pd->fallback;
}

EOLIAN static void
_efl_ui_collection_efl_ui_single_selectable_allow_manual_deselection_set(Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd, Eina_Bool allow_manual_deselection)
{
   pd->allow_manual_deselection = !!allow_manual_deselection;
}

EOLIAN static Eina_Bool
_efl_ui_collection_efl_ui_single_selectable_allow_manual_deselection_get(const Eo *obj EINA_UNUSED, Efl_Ui_Collection_Data *pd)
{
   return pd->allow_manual_deselection;
}


#include "efl_ui_collection.eo.c"

#define ITEM_IS_OUTSIDE_VISIBLE(id) (id < collection_pd->start_id || id >= collection_pd->end_id) // end_id is exclusive

/**
 * @brief Ensures an item is available (visible and positioned) for focus.
 *
 * If an item is requested for focus but is currently outside the visible
 * range managed by the position manager (e.g., virtualized), this function
 * makes it visible and sets its geometry based on the position manager's layout.
 *
 * @param item The Efl_Ui_Item to make available.
 * @param new_id The index of the item in the collection.
 * @param pd The private data of the Efl_Ui_Collection.
 */
static inline void
_assert_item_available(Eo *item, unsigned int new_id, Efl_Ui_Collection_Data *pd)
{
   EINA_SAFETY_ON_FALSE_RETURN(new_id < eina_list_count(pd->items)); // Ensure ID is valid
   EINA_SAFETY_ON_NULL_RETURN(pd->pos_man); // Position manager must exist

   // Make the item visible and set its geometry as determined by the position manager.
   // This is crucial for items that might be virtualized (not currently realized UI elements).
   efl_gfx_entity_visible_set(item, EINA_TRUE);
   efl_gfx_entity_geometry_set(item, efl_ui_position_manager_entity_position_single_item(pd->pos_man, new_id));
}

EOLIAN static Efl_Ui_Focus_Object*
_efl_ui_collection_focus_manager_efl_ui_focus_manager_request_move(Eo *obj, Efl_Ui_Collection_Focus_Manager_Data *pd, Efl_Ui_Focus_Direction direction, Efl_Ui_Focus_Object *child, Eina_Bool logical)
{
   MY_DATA_GET(pd->collection, collection_pd);
   Efl_Ui_Item *new_item, *item;
   unsigned int item_id;

   if (!child)
     child = efl_ui_focus_manager_focus_get(obj);

   item = _find_item(obj, collection_pd, child);

   //if this is NULL then we are before finalize, we cannot serve any sane value here
   if (!collection_pd->pos_man) return NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);

   item_id = efl_ui_item_index_get(item);

   if (ITEM_IS_OUTSIDE_VISIBLE(item_id))
     {
        unsigned int new_id;

        if (!efl_ui_position_manager_entity_relative_item(collection_pd->pos_man, efl_ui_item_index_get(item), direction, &new_id))
          {
             new_item = NULL;
          }
        else
          {
             new_item = eina_list_nth(collection_pd->items, new_id);;
             _assert_item_available(new_item, new_id, collection_pd);
          }
     }
   else
     {
        new_item = efl_ui_focus_manager_request_move(efl_super(obj, EFL_UI_COLLECTION_FOCUS_MANAGER_CLASS), direction, child, logical);
     }

   return new_item;
}


EOLIAN static void
_efl_ui_collection_focus_manager_efl_ui_focus_manager_manager_focus_set(Eo *obj, Efl_Ui_Collection_Focus_Manager_Data *pd, Efl_Ui_Focus_Object *focus)
{
   MY_DATA_GET(pd->collection, collection_pd);
   Efl_Ui_Item *item;
   unsigned int item_id;

   if (focus == efl_ui_focus_manager_root_get(obj))
     {
        item = eina_list_data_get(collection_pd->items);
     }
   else
     {
        item = _find_item(obj, collection_pd, focus);
     }

   //if this is NULL then we are before finalize, we cannot serve any sane value here
   if (!collection_pd->pos_man) return;
   EINA_SAFETY_ON_NULL_RETURN(item);

   item_id = efl_ui_item_index_get(item);

   if (ITEM_IS_OUTSIDE_VISIBLE(item_id))
     {
        _assert_item_available(item, item_id, collection_pd);
     }
   efl_ui_focus_manager_focus_set(efl_super(obj, EFL_UI_COLLECTION_FOCUS_MANAGER_CLASS), focus);
}


#include "efl_ui_collection_focus_manager.eo.c"
