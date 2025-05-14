#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define ELM_WIDGET_ITEM_PROTECTED
#define EFL_UI_L10N_PROTECTED
#define EFL_UI_FOCUS_COMPOSITION_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_segment_control_eo.h"
#include "elm_segment_control_item_eo.h"
#include "elm_widget_segment_control.h"

/**
 * @internal
 * @brief The Elementary Segment Control widget.
 *
 * See @ref Elm_Segment_Control for more details on this widget.
 * This file contains the C implementation of the Segment Control widget.
 */

#define MY_CLASS ELM_SEGMENT_CONTROL_CLASS

#define MY_CLASS_NAME "Elm_Segment_Control"
#define MY_CLASS_NAME_LEGACY "elm_segment_control"

// "changed" signal name
static const char SIG_CHANGED[] = "changed";

// Smart callback descriptions
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CHANGED, ""},
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {NULL, NULL}
};

/**
 * @internal
 * @brief Updates translations for all items in the segment control.
 *
 * This function is called when the application's language changes.
 * It iterates over all segment items and triggers their translation update.
 *
 * @param obj The Evas object (segment control).
 * @param sd The segment control's private data.
 */
EOLIAN static void
_elm_segment_control_efl_ui_l10n_translation_update(Eo *obj EINA_UNUSED, Elm_Segment_Control_Data *sd)
{
   Elm_Object_Item *it;
   Eina_List *l;

   EINA_LIST_FOREACH(sd->items, l, it)
     elm_wdg_item_translate(it);

   efl_ui_l10n_translation_update(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Frees resources associated with a segment control item.
 *
 * This includes removing the item from the segment control's list of items,
 * deleting its icon, and freeing its label string.
 *
 * @param it The segment control item data to free.
 */
static void
_item_free(Elm_Segment_Control_Item_Data *it)
{
   ELM_SEGMENT_CONTROL_DATA_GET(WIDGET(it), sd);

   if (sd->selected_item == it) sd->selected_item = NULL;
   if (sd->items) sd->items = eina_list_remove(sd->items, EO_OBJ(it));

   evas_object_del(it->icon);
   eina_stringshare_del(it->label);
}

/**
 * @internal
 * @brief Calculates and sets the position and size of each segment item.
 *
 * Items are distributed equally within the widget's bounds.
 * Considers Right-To-Left (RTL) mode for positioning.
 *
 * @param sd The segment control's private data.
 */
static void
_position_items(Elm_Segment_Control_Data *sd)
{
   Eina_List *l;
   Eina_Bool rtl;
   int item_count;
   Elm_Object_Item *eo_item;
   int bx, by, bw, bh, pos;
   ELM_WIDGET_DATA_GET_OR_RETURN(sd->obj, wd);

   item_count = eina_list_count(sd->items);
   if (item_count <= 0) return;

   evas_object_geometry_get
     (wd->resize_obj, &bx, &by, &bw, &bh);
   sd->item_width = bw / item_count;
   rtl = efl_ui_mirrored_get(sd->obj);

   if (rtl) pos = bx + bw - sd->item_width;
   else pos = bx;

   EINA_LIST_FOREACH(sd->items, l, eo_item)
     {
        ELM_SEGMENT_ITEM_DATA_GET(eo_item, it);
        evas_object_geometry_set(VIEW(it), pos, by, sd->item_width, bh);
        if (rtl) pos -= sd->item_width;
        else pos += sd->item_width;
     }

   elm_layout_sizing_eval(sd->obj);
}

/**
 * @internal
 * @brief Swallows the icon and sets the text for a segment item in its Edje layout.
 *
 * Emits signals to the Edje theme to show or hide icon and text parts
 * based on their presence.
 *
 * @param it The segment control item data.
 */
static void
_swallow_item_objects(Elm_Segment_Control_Item_Data *it)
{
   if (!it) return;

   if (it->icon)
     {
        edje_object_part_swallow(VIEW(it), "elm.swallow.icon", it->icon);
        edje_object_signal_emit(VIEW(it), "elm,state,icon,visible", "elm");
     }
   else edje_object_signal_emit(VIEW(it), "elm,state,icon,hidden", "elm");

   if (it->label)
     {
        edje_object_part_text_escaped_set(VIEW(it), "elm.text", it->label);
        edje_object_signal_emit(VIEW(it), "elm,state,text,visible", "elm");
     }
   else
     edje_object_signal_emit(VIEW(it), "elm,state,text,hidden", "elm");

   edje_object_message_signal_process(VIEW(it));
}

/**
 * @internal
 * @brief Updates the visual state of all segment items.
 *
 * This function repositions items, sets their focus composition,
 * adjusts finger size multiplier, and updates the Edje theme signals
 * for type (single, left, middle, right), state (selected, normal, disabled),
 * and content visibility.
 *
 * @param sd The segment control's private data.
 */
static void
_update_list(Elm_Segment_Control_Data *sd)
{
   int idx = 0;
   Eina_List *l;
   Eina_Bool rtl;
   int item_count;
   Elm_Object_Item *eo_it;

   efl_ui_focus_composition_elements_set(sd->obj, eina_list_clone(sd->items));
   _position_items(sd);

   item_count = eina_list_count(sd->items);
   efl_ui_layout_finger_size_multiplier_set(sd->obj, item_count, 1);

   if (item_count == 1)
     {
        eo_it = eina_list_nth(sd->items, 0);
        ELM_SEGMENT_ITEM_DATA_GET(eo_it, it);

        it->seg_index = 0;

        //Set the segment type
        edje_object_signal_emit(VIEW(it), "elm,type,segment,single", "elm");

        //Set the segment state
        if (sd->selected_item == it)
          edje_object_signal_emit
            (VIEW(it), "elm,state,segment,selected", "elm");
        else
          edje_object_signal_emit(VIEW(it), "elm,state,segment,normal", "elm");

        if (elm_widget_disabled_get(sd->obj))
          edje_object_signal_emit(VIEW(it), "elm,state,disabled", "elm");
        else
          edje_object_signal_emit(VIEW(it), "elm,state,enabled", "elm");

        _swallow_item_objects(it);
        return;
     }

   rtl = efl_ui_mirrored_get(sd->obj);
   EINA_LIST_FOREACH(sd->items, l, eo_it)
     {
        ELM_SEGMENT_ITEM_DATA_GET(eo_it, it);
        it->seg_index = idx;

        //Set the segment type
        if (idx == 0)
          {
             if (rtl)
               edje_object_signal_emit
                 (VIEW(it), "elm,type,segment,right", "elm");
             else
               edje_object_signal_emit
                 (VIEW(it), "elm,type,segment,left", "elm");
          }
        else if (idx == (item_count - 1))
          {
             if (rtl)
               edje_object_signal_emit
                 (VIEW(it), "elm,type,segment,left", "elm");
             else
               edje_object_signal_emit
                 (VIEW(it), "elm,type,segment,right", "elm");
          }
        else
          edje_object_signal_emit(VIEW(it), "elm,type,segment,middle", "elm");

        //Set the segment state
        if (sd->selected_item == it)
          edje_object_signal_emit
            (VIEW(it), "elm,state,segment,selected", "elm");
        else
          edje_object_signal_emit(VIEW(it), "elm,state,segment,normal", "elm");

        if (elm_widget_disabled_get(sd->obj)
            || elm_object_item_disabled_get(eo_it))
          edje_object_signal_emit(VIEW(it), "elm,state,disabled", "elm");
        else
          edje_object_signal_emit(VIEW(it), "elm,state,enabled", "elm");

        _swallow_item_objects(it);
        idx++;
     }
}

/**
 * @internal
 * @brief Applies the theme to the segment control and its items.
 *
 * This function is called when the widget's theme needs to be updated.
 * It applies the theme to the base widget and then iterates through
 * each item, setting its theme, scale, and mirrored mode.
 *
 * @param obj The Evas object (segment control).
 * @param sd The segment control's private data.
 * @return Eina_Error EFL_UI_THEME_APPLY_ERROR_GENERIC on failure, or success from super.
 */
EOLIAN static Eina_Error
_elm_segment_control_efl_ui_widget_theme_apply(Eo *obj, Elm_Segment_Control_Data *sd)
{
   Eina_List *l;
   Eina_Bool rtl;
   Elm_Object_Item *eo_item;

   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   rtl = efl_ui_mirrored_get(obj);

   EINA_LIST_FOREACH(sd->items, l, eo_item)
     {
        ELM_SEGMENT_ITEM_DATA_GET(eo_item, it);
        elm_widget_theme_object_set
          (obj, VIEW(it), "segment_control", "item",
          elm_widget_style_get(obj));
        edje_object_scale_set(VIEW(it), efl_gfx_entity_scale_get(WIDGET(it)) *
                              elm_config_scale_get());
        edje_object_mirrored_set(VIEW(it), rtl);
     }

   _update_list(sd);

   return int_ret;
}

/**
 * @internal
 * @brief Sets the disabled state of the segment control.
 *
 * Propagates the disabled state to the superclass and then updates
 * the visual state of all items to reflect the change.
 *
 * @param obj The Evas object (segment control).
 * @param sd The segment control's private data.
 * @param disabled EINA_TRUE to disable, EINA_FALSE to enable.
 */
EOLIAN static void
_elm_segment_control_efl_ui_widget_disabled_set(Eo *obj, Elm_Segment_Control_Data *sd, Eina_Bool disabled)
{
   efl_ui_widget_disabled_set(efl_super(obj, MY_CLASS), disabled);
   _update_list(sd);
}

// TODO: elm_widget_focus_list_next_get supports only Elm_widget list,
// not the Elm_Widget_item. Focus switching within widget not
// supported until it is supported in elm_widget
#if 0
static void *
_elm_list_data_get(const Eina_List *list)
{
   Elm_Object_Item *eo_item = eina_list_data_get(list);

   if (eo_item) return NULL;

   ELM_SEGMENT_ITEM_DATA_GET(eo_item, it);
   edje_object_signal_emit(VIEW(it), "elm,state,segment,selected", "elm");

   return VIEW(it);
}

EOLIAN static Eina_Bool
_elm_segment_control_elm_widget_focus_next(Eo *obj, Elm_Segment_Control_Data *sd, Elm_Focus_Direction dir, Evas_Object **next)
{
   static int count = 0;
   const Eina_List *items;
   void *(*list_data_get)(const Eina_List *list);

   /* Focus chain */
   if ((items = efl_ui_widget_focus_custom_chain_get(obj)))
     list_data_get = eina_list_data_get;
   else
     {
        items = sd->items;
        list_data_get = _elm_list_data_get;
        if (!items) return EINA_FALSE;
     }

   return elm_widget_focus_list_next_get(obj, items, list_data_get, dir, next);
}

#endif

/**
 * @internal
 * @brief Sets a segment item to the 'off' (unselected) state.
 *
 * Emits a signal to the Edje theme to reflect the normal state.
 * If this item was the currently selected one, it clears the selection.
 *
 * @param it The segment control item data.
 */
static void
_segment_off(Elm_Segment_Control_Item_Data *it)
{
   ELM_SEGMENT_CONTROL_DATA_GET(WIDGET(it), sd);

   edje_object_signal_emit(VIEW(it), "elm,state,segment,normal", "elm");

   if (sd->selected_item == it) sd->selected_item = NULL;
}

/**
 * @internal
 * @brief Sets a segment item to the 'on' (selected) state.
 *
 * If the item is already selected or disabled, no action is taken.
 * Otherwise, it deselects any previously selected item, emits a signal
 * to the Edje theme for the selected state, updates the selected item,
 * and triggers the "changed" callback.
 *
 * @param it The segment control item data.
 */
static void
_segment_on(Elm_Segment_Control_Item_Data *it)
{
   ELM_SEGMENT_CONTROL_DATA_GET(WIDGET(it), sd);

   if (it == sd->selected_item) return;

   if (elm_object_item_disabled_get(EO_OBJ(it))) return;

   if (sd->selected_item) _segment_off(sd->selected_item);

   edje_object_signal_emit(VIEW(it), "elm,state,segment,selected", "elm");

   sd->selected_item = it;
   efl_event_callback_legacy_call(sd->obj, ELM_SEGMENT_CONTROL_EVENT_CHANGED, EO_OBJ(it));
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_RESIZE and EVAS_CALLBACK_MOVE events on the segment control.
 *
 * Triggers repositioning of the items when the widget is moved or resized.
 *
 * @param data The segment control widget (passed as user data).
 * @param e The Evas canvas.
 * @param obj The Evas object that triggered the event.
 * @param event_info Event-specific information (unused).
 */
static void
_on_move_resize(void *data, Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   ELM_SEGMENT_CONTROL_DATA_GET(data, sd);

   _position_items(sd);
}

static void
_on_mouse_up(void *data,
             Evas *e EINA_UNUSED,
             Evas_Object *obj EINA_UNUSED,
             void *event_info)
{
   // it: The specific segment item that received the mouse up event.
   Elm_Segment_Control_Item_Data *it = data;
   Evas_Event_Mouse_Up *ev = event_info;
   Evas_Coord x, y, w, h;

   ELM_SEGMENT_CONTROL_DATA_GET(WIDGET(it), sd);

   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if (elm_widget_disabled_get(sd->obj)) return;

   if (it == sd->selected_item) return;

   ev = event_info;
   evas_object_geometry_get(VIEW(it), &x, &y, &w, &h);

   if ((ev->canvas.x >= x) && (ev->canvas.x <= (x + w)) && (ev->canvas.y >= y)
       && (ev->canvas.y <= (y + h)))
     _segment_on(it);
   else
     edje_object_signal_emit(VIEW(it), "elm,state,segment,normal", "elm");
}

static void
_on_mouse_down(void *data,
               Evas *e EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   // it: The specific segment item that received the mouse down event.
   Elm_Segment_Control_Item_Data *it = data;
   Evas_Event_Mouse_Down *ev = event_info;

   ELM_SEGMENT_CONTROL_DATA_GET(WIDGET(it), sd);

   if (ev->button != 1) return;
   if (elm_widget_disabled_get(sd->obj)) return;

   if (it == sd->selected_item) return;

   edje_object_signal_emit(VIEW(it), "elm,state,segment,pressed", "elm");
}

/**
 * @internal
 * @brief Finds a segment control item by its index.
 *
 * @param obj The segment control widget.
 * @param idx The index of the item to find.
 * @return The Elm_Segment_Control_Item_Data for the item at the given index,
 *         or NULL if not found.
 */
static Elm_Segment_Control_Item_Data *
_item_find(const Evas_Object *obj,
           int idx)
{
   Elm_Object_Item *eo_it;

   ELM_SEGMENT_CONTROL_DATA_GET(obj, sd);

   eo_it = eina_list_nth(sd->items, idx);
   ELM_SEGMENT_ITEM_DATA_GET(eo_it, it);
   return it;
}

EOLIAN static void
_elm_segment_control_item_elm_widget_item_part_text_set(Eo *eo_item EINA_UNUSED,
                                Elm_Segment_Control_Item_Data *item,
                                const char *part,
                                const char *label)
{
   // Buffer for constructing signal strings
   char buf[1024];

   if ((!part) || (!strcmp(part, "default")) ||
       (!strcmp(part, "elm.text")))
     {
        eina_stringshare_replace(&item->label, label);
        if (label)
          edje_object_signal_emit(VIEW(item), "elm,state,text,visible", "elm");
        else
          edje_object_signal_emit(VIEW(item), "elm,state,text,hidden", "elm");
        edje_object_part_text_escaped_set(VIEW(item), "elm.text", label);
     }
   else
     {
        if (label)
          {
             snprintf(buf, sizeof(buf), "elm,state,%s,visible", part);
             edje_object_signal_emit(VIEW(item), buf, "elm");
          }
        else
          {
             snprintf(buf, sizeof(buf), "elm,state,%s,hidden", part);
             edje_object_signal_emit(VIEW(item), buf, "elm");
          }
        edje_object_part_text_escaped_set(VIEW(item), part, label);
     }

   edje_object_message_signal_process(VIEW(item));
}

EOLIAN static const char *
_elm_segment_control_item_elm_widget_item_part_text_get(const Eo *eo_item EINA_UNUSED,
                                                Elm_Segment_Control_Item_Data *it,
                                                const char *part)
{
   // Buffer for constructing part name string
   char buf[1024];

   if (!part || !strcmp(part, "default"))
     snprintf(buf, sizeof(buf), "elm.text");
   else
     snprintf(buf, sizeof(buf), "%s", part);

   return edje_object_part_text_get(VIEW(it), buf);
}

EOLIAN static void
_elm_segment_control_item_elm_widget_item_part_content_set(Eo *eo_item EINA_UNUSED,
                                                   Elm_Segment_Control_Item_Data *item,
                                                   const char *part,
                                                   Evas_Object *content)
{
   // Buffer for constructing signal strings
   char buf[1024];

   if (!part || !strcmp("icon", part))
     {
        if (content == item->icon) return;

        evas_object_del(item->icon);
        item->icon = content;
        if (!item->icon)
          {
             elm_widget_sub_object_add(VIEW(item), item->icon);
             edje_object_part_swallow(VIEW(item), "elm.swallow.icon", item->icon);
             edje_object_signal_emit(VIEW(item), "elm,state,icon,visible", "elm");
          }
        else
          edje_object_signal_emit(VIEW(item), "elm,state,icon,hidden", "elm");
     }
   else
     {
        if (content)
          {
             edje_object_part_swallow(VIEW(item), part, content);
             snprintf(buf, sizeof(buf), "elm,state,%s,visible", part);
             edje_object_signal_emit(VIEW(item), buf, "elm");
          }
        else
          {
             snprintf(buf, sizeof(buf), "elm,state,%s,hidden", part);
             edje_object_signal_emit(VIEW(item), buf, "elm");
          }
     }
}

EOLIAN static Evas_Object *
_elm_segment_control_item_elm_widget_item_part_content_get(const Eo *eo_item EINA_UNUSED,
                                                   Elm_Segment_Control_Item_Data *item,
                                                   const char *part)
{
   if (part && !strcmp(part, "icon"))
     return item->icon;
   else
     return edje_object_part_swallow_get(VIEW(item), part);
}

EOLIAN static void
_elm_segment_control_item_efl_object_destructor(Eo *eo_item, Elm_Segment_Control_Item_Data *item)
{
   ELM_SEGMENT_CONTROL_DATA_GET(WIDGET(item), sd);

   // Free item's internal data
   _item_free(item);
   // Refresh the layout and state of remaining items
   _update_list(sd);

   efl_destructor(efl_super(eo_item, ELM_SEGMENT_CONTROL_ITEM_CLASS));
}

/**
 * @internal
 * @brief Accessibility callback to get information about a segment item.
 *
 * Provides the item's label as its accessibility information.
 *
 * @param data The segment item data (Elm_Segment_Control_Item_Data *).
 * @param obj The Evas object (unused).
 * @return A newly allocated string containing the item's label, or NULL.
 *         The caller is responsible for freeing the returned string.
 */
static char *
_access_info_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   const char *txt = NULL;
   Elm_Segment_Control_Item_Data *it = (Elm_Segment_Control_Item_Data *)data;
   ELM_SEGMENT_CONTROL_ITEM_CHECK_OR_RETURN(it, NULL);

   if (!txt) txt = it->label; // Prioritize item's label
   if (txt) return strdup(txt);

   return NULL;
}

/**
 * @internal
 * @brief Accessibility callback to get the state of a segment item.
 *
 * Provides a string describing the item's state (e.g., "State: Disabled",
 * "State: Selected", "State: Unselected").
 *
 * @param data The segment item data (Elm_Segment_Control_Item_Data *).
 * @param obj The Evas object (unused).
 * @return A newly allocated string describing the item's state, or NULL.
 *         The caller is responsible for freeing the returned string.
 */
static char *
_access_state_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   Elm_Segment_Control_Item_Data *it = (Elm_Segment_Control_Item_Data *)data;
   ELM_SEGMENT_CONTROL_ITEM_CHECK_OR_RETURN(it, NULL);
   ELM_SEGMENT_CONTROL_DATA_GET(WIDGET(it), sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(WIDGET(it), wd, NULL);

   if (wd->disabled)
     return strdup(E_("State: Disabled"));

   if (it == sd->selected_item)
     return strdup(E_("State: Selected"));
   else
     return strdup(E_("State: Unselected"));
}

/**
 * @internal
 * @brief Registers accessibility features for a segment control item.
 *
 * Sets up accessibility callbacks for type, information, and state.
 *
 * @param eo_it The Evas object for the item.
 * @param it The segment control item data.
 * @return The Evas_Object used for accessibility, typically the item's view.
 */
static Evas_Object*
_elm_segment_control_item_elm_widget_item_access_register(Eo *eo_it EINA_UNUSED,
                                                  Elm_Segment_Control_Item_Data *it)
{
   Elm_Access_Info *ai;
   Evas_Object *ret;

   ret = elm_wdg_item_access_register(efl_super(eo_it, ELM_SEGMENT_CONTROL_ITEM_CLASS));

   ai = _elm_access_info_get(it->base->access_obj);

   _elm_access_text_set(ai, ELM_ACCESS_TYPE, E_("Segment Control Item"));
   _elm_access_callback_set(ai, ELM_ACCESS_INFO, _access_info_cb, it);
   _elm_access_callback_set(ai, ELM_ACCESS_STATE, _access_state_cb, it);

   return ret;
}

/**
 * @internal
 * @brief Creates a new segment control item.
 *
 * Initializes the item, sets its label and icon, and sets up mouse event
 * callbacks. Also registers accessibility if enabled.
 *
 * @param obj The parent segment control widget.
 * @param icon Optional icon for the item. Can be NULL.
 * @param label Optional label for the item. Can be NULL.
 * @return The newly created Elm_Object_Item, or NULL on failure.
 */
static Elm_Object_Item *
_item_new(Evas_Object *obj,
          Evas_Object *icon,
          const char *label)
{
   Eo *eo_item;

   eo_item = efl_add(ELM_SEGMENT_CONTROL_ITEM_CLASS, obj);
   if (!eo_item) return NULL;

   ELM_SEGMENT_ITEM_DATA_GET(eo_item, it);

   if (label) eina_stringshare_replace(&it->label, label);
   if (it->label)
     edje_object_signal_emit(VIEW(it), "elm,state,text,visible", "elm");
   else
     edje_object_signal_emit(VIEW(it), "elm,state,text,hidden", "elm");
   edje_object_message_signal_process(VIEW(it));
   edje_object_part_text_escaped_set(VIEW(it), "elm.text", label);

   it->icon = icon;
   if (it->icon) elm_widget_sub_object_add(obj, it->icon);
   evas_object_event_callback_add
     (VIEW(it), EVAS_CALLBACK_MOUSE_DOWN, _on_mouse_down, it);
   evas_object_event_callback_add
     (VIEW(it), EVAS_CALLBACK_MOUSE_UP, _on_mouse_up, it);
   evas_object_show(VIEW(it));

   // ACCESS
   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     elm_wdg_item_access_register(eo_item);

   return eo_item;
}

/**
 * @internal
 * @brief Constructor for an Elm_Segment_Control_Item.
 *
 * Initializes the item's Edje view, sets up focus redirection,
 * scaling, and theming.
 *
 * @param obj The Evas object (segment control item).
 * @param it The segment control item's private data.
 * @return The constructed Evas object.
 */
EOLIAN static Eo *
_elm_segment_control_item_efl_object_constructor(Eo *obj, Elm_Segment_Control_Item_Data *it)
{
   obj = efl_constructor(efl_super(obj, ELM_SEGMENT_CONTROL_ITEM_CLASS));
   it->base = efl_data_scope_get(obj, ELM_WIDGET_ITEM_CLASS);

   Evas_Object *parent;
   parent = efl_parent_get(obj);

   VIEW_SET(it, edje_object_add(evas_object_evas_get(parent)));
   _efl_ui_focus_event_redirector(VIEW(it), obj);
   edje_object_scale_set(VIEW(it),efl_gfx_entity_scale_get(WIDGET(it)) *
                         elm_config_scale_get());
   evas_object_smart_member_add(VIEW(it), parent);

   elm_widget_sub_object_add(parent, VIEW(it));
   elm_widget_theme_object_set
     (parent, VIEW(it), "segment_control", "item", elm_object_style_get(parent));
   edje_object_mirrored_set(VIEW(it), efl_ui_mirrored_get(WIDGET(it)));

   return obj;
}

/**
 * @internal
 * @brief Handles the addition of the segment control to a canvas group.
 *
 * Sets the theme for the segment control base and registers event callbacks
 * for resize and move to handle item repositioning.
 *
 * @param obj The Evas object (segment control).
 * @param sd The segment control's private data.
 */
EOLIAN static void
_elm_segment_control_efl_canvas_group_group_add(Eo *obj, Elm_Segment_Control_Data *sd)
{
   sd->obj = obj; // Store the Evas object in private data
   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   if (!elm_layout_theme_set
       (obj, "segment_control", "base", elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_RESIZE, _on_move_resize, obj);
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_MOVE, _on_move_resize, obj);

   elm_layout_sizing_eval(obj);
}

/**
 * @internal
 * @brief Handles the deletion of the segment control from a canvas group.
 *
 * Unregisters event callbacks and frees all associated segment items.
 *
 * @param obj The Evas object (segment control).
 * @param sd The segment control's private data.
 */
EOLIAN static void
_elm_segment_control_efl_canvas_group_group_del(Eo *obj, Elm_Segment_Control_Data *sd)
{
   Elm_Object_Item *eo_it;

   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_RESIZE,
                                       _on_move_resize, obj);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_MOVE,
                                       _on_move_resize, obj);
   EINA_LIST_FREE(sd->items, eo_it)
     {
        efl_del(eo_it);
     }

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

// Flag to enable/disable smart focus for accessibility.
static Eina_Bool _elm_segment_control_smart_focus_next_enable = EINA_FALSE;

/**
 * @internal
 * @brief Registers or unregisters accessibility for all items in the segment control.
 *
 * Iterates through all items and calls the appropriate accessibility
 * registration function based on the `is_access` flag.
 *
 * @param sd The segment control's private data.
 * @param is_access EINA_TRUE to register accessibility, EINA_FALSE to unregister.
 */
static void
_access_obj_process(Elm_Segment_Control_Data *sd, Eina_Bool is_access)
{
   Eina_List *l;
   Eo *eo_item;

   EINA_LIST_FOREACH(sd->items, l, eo_item)
     {
        if (is_access) elm_wdg_item_access_register(eo_item);
        else
          elm_wdg_item_access_unregister(eo_item);
     }
}

/**
 * @internal
 * @brief Callback for when accessibility features are updated/changed for the widget.
 *
 * Updates the internal flag for smart focus and processes accessibility
 * registration for all items accordingly.
 *
 * @param obj The Evas object (segment control, unused).
 * @param sd The segment control's private data.
 * @param acs EINA_TRUE if accessibility is now active, EINA_FALSE otherwise.
 */
EOLIAN static void
_elm_segment_control_efl_ui_widget_on_access_update(Eo *obj EINA_UNUSED, Elm_Segment_Control_Data *sd, Eina_Bool acs)
{
   _elm_segment_control_smart_focus_next_enable = acs;
   _access_obj_process(sd, _elm_segment_control_smart_focus_next_enable);
}

EAPI Evas_Object *
elm_segment_control_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   // Legacy way to create a new Segment Control widget.
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Constructor for the Elm_Segment_Control widget.
 *
 * Initializes the widget, sets its legacy type name, smart callbacks,
 * and accessibility role.
 *
 * @param obj The Evas object (segment control).
 * @param sd The segment control's private data (unused in this function).
 * @return The constructed Evas object.
 */
EOLIAN static Eo *
_elm_segment_control_efl_object_constructor(Eo *obj, Elm_Segment_Control_Data *sd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_LIST);

   return obj;
}

/**
 * @internal
 * @brief Adds a new item to the segment control.
 *
 * Creates a new item with the given icon and label, appends it to the
 * end of the item list, and updates the widget's layout.
 *
 * @param obj The segment control widget.
 * @param sd The segment control's private data.
 * @param icon Optional icon for the new item.
 * @param label Optional label for the new item.
 * @return The newly added Elm_Object_Item, or NULL on failure.
 */
EOLIAN static Elm_Object_Item*
_elm_segment_control_item_add(Eo *obj, Elm_Segment_Control_Data *sd, Evas_Object *icon, const char *label)
{
   Elm_Object_Item *eo_item;

   eo_item = _item_new(obj, icon, label);
   if (!eo_item) return NULL;

   sd->items = eina_list_append(sd->items, eo_item);
   _update_list(sd);

   return eo_item;
}

/**
 * @internal
 * @brief Inserts a new item into the segment control at a specific index.
 *
 * Creates a new item and inserts it before the item currently at `idx`.
 * If `idx` is out of bounds, it behaves like append or prepend.
 *
 * @param obj The segment control widget.
 * @param sd The segment control's private data.
 * @param icon Optional icon for the new item.
 * @param label Optional label for the new item.
 * @param idx The index at which to insert the item.
 * @return The newly inserted Elm_Object_Item, or NULL on failure.
 */
EOLIAN static Elm_Object_Item*
_elm_segment_control_item_insert_at(Eo *obj, Elm_Segment_Control_Data *sd, Evas_Object *icon, const char *label, int idx)
{
   Elm_Object_Item *eo_item;
   Elm_Segment_Control_Item_Data *it_rel; // Item relative to which new item is inserted


   if (idx < 0) idx = 0;

   eo_item = _item_new(obj, icon, label);
   if (!eo_item) return NULL;

   it_rel = _item_find(obj, idx);
   if (it_rel) sd->items = eina_list_prepend_relative(sd->items, eo_item, EO_OBJ(it_rel));
   else sd->items = eina_list_append(sd->items, eo_item);

   _update_list(sd);

   return eo_item;
}

/**
 * @internal
 * @brief Deletes an item from the segment control at a specific index.
 *
 * @param obj The segment control widget.
 * @param _pd The segment control's private data (unused).
 * @param idx The index of the item to delete.
 */
EOLIAN static void
_elm_segment_control_item_del_at(Eo *obj, Elm_Segment_Control_Data *_pd EINA_UNUSED, int idx)
{
   Elm_Segment_Control_Item_Data *it;

   it = _item_find(obj, idx);
   if (!it) return;

   efl_del(EO_OBJ(it));
}

/**
 * @internal
 * @brief Gets the label of an item at a specific index.
 *
 * @param obj The segment control widget.
 * @param _pd The segment control's private data (unused).
 * @param idx The index of the item.
 * @return The label of the item, or NULL if not found or item has no label.
 */
EOLIAN static const char*
_elm_segment_control_item_label_get(const Eo *obj, Elm_Segment_Control_Data *_pd EINA_UNUSED, int idx)
{
   Elm_Segment_Control_Item_Data *it;

   it = _item_find(obj, idx);
   if (it) return it->label;

   return NULL;
}

/**
 * @internal
 * @brief Gets the icon of an item at a specific index.
 *
 * @param obj The segment control widget.
 * @param _pd The segment control's private data (unused).
 * @param idx The index of the item.
 * @return The icon Evas_Object of the item, or NULL if not found or item has no icon.
 */
EOLIAN static Evas_Object*
_elm_segment_control_item_icon_get(const Eo *obj, Elm_Segment_Control_Data *_pd EINA_UNUSED, int idx)
{
   Elm_Segment_Control_Item_Data *it = _item_find(obj, idx);
   if (it) return it->icon; // Return the icon object if item is found
   return NULL;
}

/**
 * @internal
 * @brief Gets the total number of items in the segment control.
 *
 * @param obj The segment control widget (unused).
 * @param sd The segment control's private data.
 * @return The count of items.
 */
EOLIAN static int
_elm_segment_control_item_count_get(const Eo *obj EINA_UNUSED, Elm_Segment_Control_Data *sd)
{
   return eina_list_count(sd->items);
}

/**
 * @internal
 * @brief Gets the Evas_Object (view) associated with a segment control item.
 *
 * @param eo_it The segment control item (unused).
 * @param it The segment control item's private data.
 * @return The Evas_Object representing the item's view.
 */
EOLIAN static Evas_Object *
_elm_segment_control_item_object_get(const Eo *eo_it EINA_UNUSED, Elm_Segment_Control_Item_Data *it)
{
   return VIEW(it);
}

/**
 * @internal
 * @brief Gets the currently selected item in the segment control.
 *
 * @param obj The segment control widget (unused).
 * @param sd The segment control's private data.
 * @return The selected Elm_Object_Item, or NULL if no item is selected.
 */
EOLIAN static Elm_Object_Item*
_elm_segment_control_item_selected_get(const Eo *obj EINA_UNUSED, Elm_Segment_Control_Data *sd)
{
   return EO_OBJ(sd->selected_item);
}

/**
 * @internal
 * @brief Sets the selected state of a specific segment control item.
 *
 * @param eo_item The segment control item (unused).
 * @param item The segment control item's private data.
 * @param selected EINA_TRUE to select the item, EINA_FALSE to unselect.
 */
EOLIAN static void
_elm_segment_control_item_selected_set(Eo *eo_item EINA_UNUSED,
                               Elm_Segment_Control_Item_Data *item,
                               Eina_Bool selected)
{
   ELM_SEGMENT_CONTROL_ITEM_CHECK_OR_RETURN(item);
   ELM_SEGMENT_CONTROL_DATA_GET(WIDGET(item), sd);

   if (item == sd->selected_item)
     {
        //already in selected state.
        if (selected) return;

        //unselect case
        _segment_off(item);
     }
   else if (selected)
     _segment_on(item);
}

/**
 * @internal
 * @brief Gets the segment control item at a specific index.
 *
 * @param obj The segment control widget.
 * @param _pd The segment control's private data (unused).
 * @param idx The index of the item.
 * @return The Elm_Object_Item at the given index, or NULL if not found.
 */
EOLIAN static Elm_Object_Item*
_elm_segment_control_item_get(const Eo *obj, Elm_Segment_Control_Data *_pd EINA_UNUSED, int idx)
{
   Elm_Segment_Control_Item_Data *it = _item_find(obj, idx);
   return EO_OBJ(it);
}

/**
 * @internal
 * @brief Gets the index of a specific segment control item.
 *
 * @param eo_it The segment control item (unused).
 * @param it The segment control item's private data.
 * @return The index of the item, or -1 if the item is invalid.
 */
EOLIAN static int
_elm_segment_control_item_index_get(const Eo *eo_it EINA_UNUSED, Elm_Segment_Control_Item_Data *it)
{
   ELM_SEGMENT_CONTROL_ITEM_CHECK_OR_RETURN(it, -1);

   return it->seg_index;
}

/**
 * @internal
 * @brief Class constructor for Elm_Segment_Control.
 *
 * Registers the legacy smart type and enables smart focus for accessibility
 * if access mode is on in the configuration.
 *
 * @param klass The Efl_Class for Elm_Segment_Control.
 */
EOLIAN static void
_elm_segment_control_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);

   // Enable smart focus if accessibility is globally enabled
   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
      _elm_segment_control_smart_focus_next_enable = EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the focus geometry for a segment control item.
 *
 * This is typically the geometry of the item's view.
 *
 * @param obj The segment control item (unused).
 * @param pd The segment control item's private data.
 * @return The Eina_Rect representing the focus geometry.
 */
EOLIAN static Eina_Rect
_elm_segment_control_item_efl_ui_focus_object_focus_geometry_get(const Eo *obj EINA_UNUSED, Elm_Segment_Control_Item_Data *pd)
{
   return efl_gfx_entity_geometry_get(VIEW(pd));
}

/**
 * @internal
 * @brief Gets the focus parent for a segment control item.
 *
 * The focus parent is the main segment control widget.
 *
 * @param obj The segment control item (unused).
 * @param pd The segment control item's private data.
 * @return The Efl_Ui_Focus_Object that is the focus parent (the segment control widget).
 */
EOLIAN static Efl_Ui_Focus_Object*
_elm_segment_control_item_efl_ui_focus_object_focus_parent_get(const Eo *obj EINA_UNUSED, Elm_Segment_Control_Item_Data *pd)
{
   return WIDGET(pd);
}

/* Internal EO APIs and hidden overrides */
// These macros and includes are part of the Eolian generation process
// and typically do not require manual Doxygen comments unless there's
// specific, non-obvious logic within them that isn't covered by the
// .eo file documentation.

#define ELM_SEGMENT_CONTROL_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_segment_control)

#include "elm_segment_control_item_eo.c"
#include "elm_segment_control_eo.c"
