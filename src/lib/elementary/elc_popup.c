#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_ACCESS_WIDGET_ACTION_PROTECTED
#define ELM_WIDGET_PROTECTED
#define ELM_WIDGET_ITEM_PROTECTED
#define EFL_UI_L10N_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_popup.h"

#include "elm_popup_part.eo.h"
#include "elm_part_helper.h"

#define MY_CLASS ELM_POPUP_CLASS

#define MY_CLASS_NAME "Elm_Popup"
#define MY_CLASS_NAME_LEGACY "elm_popup"

static void _button_remove(Evas_Object *, int, Eina_Bool);

static const char ACCESS_TITLE_PART[] = "access.title";
static const char ACCESS_BODY_PART[] = "access.body";
static const char CONTENT_PART[] = "elm.swallow.content";

static const char SIG_BLOCK_CLICKED[] = "block,clicked";
static const char SIG_TIMEOUT[] = "timeout";
static const char SIG_ITEM_FOCUSED[] = "item,focused";
static const char SIG_ITEM_UNFOCUSED[] = "item,unfocused";

/**
 * @brief Smart callback descriptions for the Elm_Popup widget.
 *
 * This array defines the smart callbacks available for a popup object.
 * The callbacks allow the application to react to various events.
 *
 * The structure of each element is { "event_name", "params_type_string" }.
 * For example:
 * @li {"block,clicked", ""}: Emitted when the user clicks the blocked event area.
 * @li {"timeout", ""}: Emitted when the popup timeout is reached.
 * @li {"item,focused", ""}: Emitted when an item in the popup gains focus.
 * @li {"item,unfocused", ""}: Emitted when an item in the popup loses focus.
 * @li {"language,changed", ""}: Handled by elm_widget; emitted on language change.
 * @li {"access,changed", ""}: Handled by elm_widget; emitted on accessibility state change.
 * @li {"focused", ""}: Handled by elm_layout; emitted when the layout gains focus.
 * @li {"unfocused", ""}: Handled by elm_layout; emitted when the layout loses focus.
 */
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_BLOCK_CLICKED, ""},
   {SIG_TIMEOUT, ""},
   {SIG_ITEM_FOCUSED, ""},
   {SIG_ITEM_UNFOCUSED, ""},
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_LAYOUT_FOCUSED, ""}, /**< handled by elm_layout */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< handled by elm_layout */
   {NULL, NULL}
};

static Eina_Bool _key_action_escape(Evas_Object *obj, const char *params);
static void _parent_geom_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED);
static void _block_clicked_cb(void *data, const Efl_Event *event);
static void _timeout_cb(void *data, const Efl_Event *event);

static void _hide_effect_finished_cb(void *data, const Efl_Event *event);

/**
 * @brief Defines the key actions for the popup widget.
 *
 * This array maps key names to handler functions, allowing for keyboard
 * control of the popup.
 * The structure of each element is { "key_name", callback_function }.
 * For example:
 * @li {"escape", _key_action_escape}: Calls _key_action_escape to dismiss the popup.
 */
static const Elm_Action key_actions[] = {
   {"escape", _key_action_escape},
   {NULL, NULL}
};

EFL_CALLBACKS_ARRAY_DEFINE(_notify_cb,
   { ELM_NOTIFY_EVENT_BLOCK_CLICKED, _block_clicked_cb },
   { ELM_NOTIFY_EVENT_TIMEOUT, _timeout_cb },
   { ELM_NOTIFY_EVENT_DISMISSED, _hide_effect_finished_cb }
);

static void  _on_content_del(void *data, Evas *e, Evas_Object *obj, void *event_info);

/**
 * @brief Updates the translations for the popup and its items.
 *
 * This function is called when the application's language changes. It iterates
 * through all items to trigger their translation updates and then calls the
 * superclass implementation.
 *
 * @param obj The popup object (unused).
 * @param sd The popup's private data.
 */
EOLIAN static void
_elm_popup_efl_ui_l10n_translation_update(Eo *obj EINA_UNUSED, Elm_Popup_Data *sd)
{
   Elm_Popup_Item_Data *it;
   Eina_List *l;

   EINA_LIST_FOREACH(sd->items, l, it)
      elm_wdg_item_translate(EO_OBJ(it));

   efl_ui_l10n_translation_update(efl_super(obj, MY_CLASS));
   efl_ui_l10n_translation_update(sd->main_layout);
}

/**
 * @brief Sets the visibility of the title and action areas based on their content.
 *
 * This function checks if a title text or icon is set and toggles the
 * visibility of the title area accordingly by emitting signals to the theme.
 * It does the same for the action area. This ensures the popup layout
 * adapts to its content.
 *
 * @param obj The popup object.
 */
static void
_visuals_set(Evas_Object *obj)
{
   ELM_POPUP_DATA_GET(obj, sd);

   if (!sd->title_text && !sd->title_icon)
     elm_layout_signal_emit(sd->main_layout, "elm,state,title_area,hidden", "elm");
   else
     elm_layout_signal_emit(sd->main_layout, "elm,state,title_area,visible", "elm");

   if (sd->action_area)
     {
        elm_layout_signal_emit(sd->main_layout, "elm,state,action_area,visible", "elm");
        evas_object_show(sd->action_area);
     }
   else
     elm_layout_signal_emit(sd->main_layout, "elm,state,action_area,hidden", "elm");

   edje_object_message_signal_process(elm_layout_edje_get(sd->main_layout));
}

/**
 * @brief Callback for clicks on the block area.
 *
 * This function is called when the user clicks the area outside the popup,
 * which is blocked to prevent interaction with other controls. It emits the
 * "block,clicked" legacy event.
 *
 * @param data The popup object.
 * @param event The EFL event data (unused).
 */
static void
_block_clicked_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   efl_event_callback_legacy_call(data, ELM_POPUP_EVENT_BLOCK_CLICKED, NULL);
}

/**
 * @brief Callback for popup timeout.
 *
 * This function is called when the popup's timeout is reached. It hides the
 * popup and emits the "timeout" legacy event.
 *
 * @param data The popup object.
 * @param event The EFL event data (unused).
 */
static void
_timeout_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   evas_object_hide(data);
   efl_event_callback_legacy_call(data, ELM_POPUP_EVENT_TIMEOUT, NULL);
}

/**
 * @brief Callback for when the hide effect finishes.
 *
 * This is called after the popup's hiding animation/effect completes. It sets
 * the popup to be invisible and emits the "dismissed" legacy event.
 *
 * @param data The popup object.
 * @param event The EFL event data (unused).
 */
static void
_hide_effect_finished_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   efl_gfx_entity_visible_set(data, EINA_FALSE);
   efl_event_callback_legacy_call(data, ELM_POPUP_EVENT_DISMISSED, NULL);
}


/**
 * @brief Retrieves the accessibility object for a given part.
 *
 * @param obj The popup object.
 * @param part The name of the part to get the access object for.
 * @return The accessibility object associated with the part, or @c NULL.
 */
static Evas_Object *
_access_object_get(const Evas_Object *obj, const char* part)
{
   Evas_Object *po, *ao, *o;
   ELM_POPUP_DATA_GET(obj, sd);

   o = elm_layout_edje_get(sd->main_layout);
   edje_object_freeze(o);
   po = (Evas_Object *)edje_object_part_object_get(o, part);
   edje_object_thaw(o);
   ao = evas_object_data_get(po, "_part_access_obj");

   return ao;
}

/**
 * @brief Callback for when the popup is shown.
 *
 * This function is called when the popup becomes visible. It sets the focus
 * to the popup widget.
 *
 * @param data Unused.
 * @param e The Evas canvas (unused).
 * @param obj The popup object that was shown.
 * @param event_info Unused.
 */
static void
_on_show(void *data EINA_UNUSED,
         Evas *e EINA_UNUSED,
         Evas_Object *obj,
         void *event_info EINA_UNUSED)
{
   elm_object_focus_set(obj, EINA_TRUE);
}

/**
 * @brief Calculates the maximum height for the popup's scroller.
 *
 * The maximum height is determined by the available space within the parent
 * notify widget, after accounting for the space occupied by the title and
 * action areas. This prevents the scroller from growing larger than the
 * popup itself.
 *
 * @param obj The popup object.
 */
static void
_scroller_size_calc(Evas_Object *obj)
{
   Evas_Coord h;
   Evas_Coord h_title = 0;
   Evas_Coord h_action_area = 0;
   const char *action_area_height;

   ELM_POPUP_DATA_GET(obj, sd);

   if (!sd->scroll && !sd->items) return;

   sd->scr_size_recalc = EINA_FALSE;
   sd->max_sc_h = -1;
   evas_object_geometry_get(sd->notify, NULL, NULL, NULL, &h);
   if (sd->title_text || sd->title_icon)
     edje_object_part_geometry_get(elm_layout_edje_get(sd->main_layout),
                                   "elm.bg.title", NULL, NULL, NULL, &h_title);
   if (sd->action_area)
     {
        action_area_height = edje_object_data_get(
            elm_layout_edje_get(sd->action_area), "action_area_height");
        if (action_area_height)
          h_action_area =
            (int)(atoi(action_area_height)
                  * elm_config_scale_get() * elm_object_scale_get(obj))
                  / edje_object_base_scale_get(elm_layout_edje_get(sd->action_area));
     }

   sd->max_sc_h = h - (h_title + h_action_area);
}

/**
 * @brief Callback for when size hints of a content object change.
 *
 * Triggers a recalculation of the popup's layout.
 *
 * @param data The popup object.
 * @param e The Evas canvas (unused).
 * @param obj The object whose size hints changed (unused).
 * @param event_info Unused.
 */
static void
_size_hints_changed_cb(void *data,
                       Evas *e EINA_UNUSED,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   elm_layout_sizing_eval(data);
}

/**
 * @brief Callback for when the notify widget is resized.
 *
 * Triggers a recalculation of the popup's layout when its container (the
 * notify widget) changes size.
 *
 * @param data The popup object.
 * @param e The Evas canvas (unused).
 * @param obj The notify widget that was resized (unused).
 * @param event_info Unused.
 */
static void
_notify_resize_cb(void *data,
                  Evas *e EINA_UNUSED,
                  Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   Evas_Object *popup = data;

   ELM_POPUP_CHECK(popup);

   elm_layout_sizing_eval(popup);
}

/**
 * @brief Deletes the list-related objects (scroller, box, etc.).
 *
 * @param sd The popup's private data.
 */
static void
_list_del(Elm_Popup_Data *sd)
{
   if (!sd->scr) return;

   evas_object_event_callback_del
     (sd->scr, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _size_hints_changed_cb);

   ELM_SAFE_FREE(sd->tbl, evas_object_del);
   sd->scr = NULL;
   sd->box = NULL;
   sd->spacer = NULL;
}

/**
 * @brief Removes and deletes all items from the popup.
 *
 * @param sd The popup's private data.
 */
static void
_items_remove(Elm_Popup_Data *sd)
{
   Elm_Popup_Item_Data *it;

   if (!sd->items) return;

   EINA_LIST_FREE(sd->items, it)
     efl_del(EO_OBJ(it));

   sd->items = NULL;
}

/**
 * @brief Callback for focus changes within the composition.
 *
 * Mirrors the focus state from the event source to the popup itself.
 *
 * @param data The popup object.
 * @param ev The focus change event.
 */
static void
_focus_changed_popup(void *data, const Efl_Event *ev)
{
   //mirror property
   efl_ui_focus_object_focus_set(data, efl_ui_focus_object_focus_get(ev->object));
}

/**
 * @brief Defines callbacks for composition events on the notify widget.
 *
 * This array maps EFL events to handler functions for events occurring on the
 * internal notify widget that contains the popup.
 *
 * The structure of each element is { EFL_EVENT, callback_function }.
 * For example:
 * @li { EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED, _focus_changed_popup }:
 *     Mirrors focus changes from the notify widget to the popup.
 */
EFL_CALLBACKS_ARRAY_DEFINE(composition_cb,
   { EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED, _focus_changed_popup },
)
static void _on_table_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
/**
 * @brief Destructor for the Elm_Popup object.
 *
 * This function is called when the popup object is being deleted. It cleans up
 * all associated resources, including event callbacks, child objects (notify,
 * layout, buttons, items, etc.), and allocated memory.
 *
 * @param obj The popup object being deleted.
 * @param sd The popup's private data.
 */
EOLIAN static void
_elm_popup_efl_canvas_group_group_del(Eo *obj, Elm_Popup_Data *sd)
{
   unsigned int i;

   evas_object_event_callback_del_full(sd->parent, EVAS_CALLBACK_RESIZE, _parent_geom_cb, obj);
   evas_object_event_callback_del_full(sd->parent, EVAS_CALLBACK_MOVE, _parent_geom_cb, obj);
   evas_object_event_callback_del_full(sd->notify, EVAS_CALLBACK_RESIZE, _notify_resize_cb, obj);

   efl_event_callback_array_del(sd->notify, _notify_cb(), obj);
   evas_object_event_callback_del
     (sd->content, EVAS_CALLBACK_DEL, _on_content_del);
   evas_object_event_callback_del(obj, EVAS_CALLBACK_SHOW, _on_show);
   if (sd->tbl)
     evas_object_event_callback_del_full(sd->tbl, EVAS_CALLBACK_DEL,
                                    _on_table_del, obj);
   efl_event_callback_array_del(sd->notify, composition_cb(), obj);

   sd->last_button_number = 0;

   for (i = 0; i < ELM_POPUP_ACTION_BUTTON_MAX; i++)
     {
        if (sd->buttons[i])
          {
             evas_object_del(sd->buttons[i]->btn);
             ELM_SAFE_FREE(sd->buttons[i], free);
          }
     }
   if (sd->items)
     {
        _items_remove(sd);
        _list_del(sd);
     }

   // XXX? delete other objects? just to be sure.
   ELM_SAFE_FREE(sd->notify, evas_object_del);
   ELM_SAFE_FREE(sd->title_icon, evas_object_del);
   ELM_SAFE_FREE(sd->text_content_obj, evas_object_del);
   ELM_SAFE_FREE(sd->box, evas_object_del);
   ELM_SAFE_FREE(sd->tbl, evas_object_del);
   ELM_SAFE_FREE(sd->spacer, evas_object_del);
   ELM_SAFE_FREE(sd->scr, evas_object_del);
   ELM_SAFE_FREE(sd->content, evas_object_del);
   ELM_SAFE_FREE(sd->main_layout, evas_object_del);
   ELM_SAFE_FREE(sd->content_area, evas_object_del);
   ELM_SAFE_FREE(sd->action_area, evas_object_del);
   ELM_SAFE_FREE(sd->title_text, eina_stringshare_del);

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @brief Sets the mirrored mode for the popup and its sub-objects.
 *
 * This function propagates the mirrored mode (right-to-left layout) to all
 * relevant child widgets of the popup, such as the notify, main layout,
 * scroller, action area, and all items.
 *
 * @param obj The popup object.
 * @param rtl EINA_TRUE for mirrored mode, EINA_FALSE otherwise.
 */
static void
_mirrored_set(Evas_Object *obj,
              Eina_Bool rtl)
{
   ELM_POPUP_DATA_GET(obj, sd);

   elm_object_mirrored_set(sd->notify, rtl);
   elm_object_mirrored_set(sd->main_layout, rtl);
   if (sd->scr) elm_object_mirrored_set(sd->scr, rtl);
   if (sd->action_area) elm_object_mirrored_set(sd->action_area, rtl);
   if (sd->items)
     {
        Elm_Popup_Item_Data *it;
        Eina_List *l;

        EINA_LIST_FOREACH(sd->items, l, it)
           elm_object_mirrored_set(VIEW(it), rtl);
     }
}

/**
 * @brief Manages accessibility objects for the popup.
 *
 * This function is responsible for creating, registering, updating, and
 * unregistering accessibility objects for the dynamic parts of the popup, such
 * as the title and body text. It is called when the accessibility state
 * changes.
 *
 * @param obj The popup object.
 * @param is_access If @c EINA_TRUE, accessibility is enabled and objects should
 * be registered. If @c EINA_FALSE, they should be unregistered.
 */
static void
_access_obj_process(Eo *obj, Eina_Bool is_access)
{
   Evas_Object *ao;

   ELM_POPUP_DATA_GET(obj, sd);

   if (is_access)
     {
        if (sd->title_text)
          {
             ao = _elm_access_edje_object_part_object_register
                    (obj, elm_layout_edje_get(sd->main_layout), ACCESS_TITLE_PART);
             _elm_access_text_set(_elm_access_info_get(ao),
                                  ELM_ACCESS_TYPE, E_("Popup Title"));
             _elm_access_text_set(_elm_access_info_get(ao),
                                  ELM_ACCESS_INFO, sd->title_text);
          }

        if (sd->text_content_obj)
          {
             ao = _elm_access_edje_object_part_object_register
                    (obj, elm_layout_edje_get(sd->main_layout), ACCESS_BODY_PART);
             _elm_access_text_set(_elm_access_info_get(ao),
                                  ELM_ACCESS_TYPE, E_("Popup Body Text"));
             _elm_access_text_set(_elm_access_info_get(ao),
               ELM_ACCESS_INFO, elm_object_text_get(sd->text_content_obj));
          }
     }
   else
     {
        if (sd->title_text)
          {
             _elm_access_edje_object_part_object_unregister
                    (obj, elm_layout_edje_get(sd->main_layout), ACCESS_TITLE_PART);
          }

        if (sd->text_content_obj)
          {
             _elm_access_edje_object_part_object_unregister
                    (obj, elm_layout_edje_get(sd->main_layout), ACCESS_BODY_PART);
          }
     }
}

/**
 * @brief Checks the theme to determine if scrolling should be enabled by default.
 *
 * This function reads data from the content area's theme file to see if the
 * current theme expects the content to be scrollable. The result is stored in
 * the `theme_scroll` flag of the popup's data.
 *
 * @param sd The popup's private data.
 */
static void
_populate_theme_scroll(Elm_Popup_Data *sd)
{
   if (sd->content_area)
     {
        const char *content_area_width =
          edje_object_data_get(elm_layout_edje_get(sd->content_area),
                               "scroller_enable");
        if ((content_area_width) && (!strcmp(content_area_width, "on")))
          {
             sd->theme_scroll = EINA_TRUE;
             return;
          }
     }
   sd->theme_scroll = EINA_FALSE;
}

/**
 * @brief Applies the current theme to the popup widget.
 *
 * This function is called when the widget's theme needs to be updated. It
 * sets the style for the notify, main layout, action area, content area,
 * and all items. It also updates text and icons, and re-evaluates the layout.
 *
 * @param obj The popup object.
 * @param sd The popup's private data.
 * @return An Eina_Error, @c EFL_UI_THEME_APPLY_ERROR_NONE on success.
 */
EOLIAN static Eina_Error
_elm_popup_efl_ui_widget_theme_apply(Eo *obj, Elm_Popup_Data *sd)
{
   Elm_Popup_Item_Data *it;
   Eina_List *elist;
   char buf[1024], style[1024];

   _mirrored_set(obj, efl_ui_mirrored_get(obj));

   snprintf(style, sizeof(style), "popup/%s", elm_widget_style_get(obj));
   elm_widget_style_set(sd->notify, style);

   if (!elm_layout_theme_set(sd->main_layout, "popup", "base",
                             elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   if (sd->action_area)
     {
        snprintf(buf, sizeof(buf), "buttons%i", sd->last_button_number);
        if (!elm_layout_theme_set(sd->action_area, "popup", buf, style))
          CRI("Failed to set layout!");
     }
   if (!elm_layout_theme_set(sd->content_area, "popup", "content", style))
     CRI("Failed to set layout!");
   if (sd->text_content_obj)
       elm_object_style_set(sd->text_content_obj, style);
   else if (sd->items)
     {
        EINA_LIST_FOREACH(sd->items, elist, it)
          {
             if (!elm_layout_theme_set(VIEW(it), "popup", "item", style))
               CRI("Failed to set layout!");
             else
               {
                  if (it->label)
                    {
                       elm_layout_text_set(VIEW(it), "elm.text", it->label);
                       elm_layout_signal_emit(VIEW(it),
                                              "elm,state,item,text,visible",
                                              "elm");
                    }
                  if (it->icon)
                    elm_layout_signal_emit(VIEW(it),
                                           "elm,state,item,icon,visible",
                                           "elm");
                  if (it->disabled)
                    elm_layout_signal_emit(VIEW(it),
                                           "elm,state,item,disabled", "elm");
                  evas_object_show(VIEW(it));
                  edje_object_message_signal_process(
                     elm_layout_edje_get(VIEW(it)));
               }
          }
     }
   if (sd->title_text)
     {
        elm_layout_text_set(sd->main_layout, "elm.text.title", sd->title_text);
        elm_layout_signal_emit(sd->main_layout, "elm,state,title,text,visible", "elm");
     }
   if (sd->title_icon)
     elm_layout_signal_emit(sd->main_layout, "elm,state,title,icon,visible", "elm");

   _populate_theme_scroll(sd);
   if (!sd->scroll && sd->theme_scroll)
     elm_layout_signal_emit(sd->content_area, "elm,scroll,disable", "elm");
   else if (sd->scroll && sd->theme_scroll)
     elm_layout_signal_emit(sd->content_area, "elm,scroll,enable", "elm");

   _visuals_set(obj);
   elm_layout_sizing_eval(obj);

   /* access */
   if (_elm_config->access_mode) _access_obj_process(obj, EINA_TRUE);

   return EFL_UI_THEME_APPLY_ERROR_NONE;
}

/**
 * @brief Calculates and applies the size hints for a popup item.
 *
 * This function calculates the minimum and maximum size of a single popup item
 * based on its content and the theme. The calculated size hints are then
 * applied to the item's layout object. This is crucial for the parent
 * container to correctly manage the layout of all items.
 *
 * @param it The popup item data.
 */
static void
_item_sizing_eval(Elm_Popup_Item_Data *it)
{
   Evas_Coord min_w = -1, min_h = -1, max_w = -1, max_h = -1;
   Evas_Object *edje = elm_layout_edje_get(VIEW(it));

   edje_object_size_min_restricted_calc
     (edje, &min_w, &min_h, min_w, min_h);
   evas_object_size_hint_min_set(edje, min_w, min_h);
   evas_object_size_hint_max_set(edje, max_w, max_h);
}

EOLIAN static void
_elm_popup_efl_canvas_group_group_calculate(Eo *obj, Elm_Popup_Data *sd)
{
   Eina_List *elist;
   Elm_Popup_Item_Data *it;
   Evas_Coord h_box = 0, minh_box = 0;
   Evas_Coord minw = -1, minh = -1;

   _scroller_size_calc(obj);

   if (sd->items)
     {
        EINA_LIST_FOREACH(sd->items, elist, it)
          {
             _item_sizing_eval(it);
             evas_object_size_hint_combined_min_get(elm_layout_edje_get(VIEW(it)),
                                           NULL, &minh_box);
             if (minh_box != -1) h_box += minh_box;
          }
        evas_object_size_hint_min_set(sd->spacer, 0, MIN(h_box, sd->max_sc_h));
        evas_object_size_hint_max_set(sd->spacer, -1, sd->max_sc_h);

        evas_object_size_hint_combined_min_get(sd->scr, &minw, &minh);
        evas_object_size_hint_max_get(sd->scr, &minw, &minh);
     }
   else if (sd->scroll)
     {
        double horizontal, vertical;
        Evas_Coord w, h;

        edje_object_message_signal_process(elm_layout_edje_get(sd->content_area));

        elm_popup_align_get(obj, &horizontal, &vertical);
        evas_object_geometry_get(sd->parent, NULL, NULL, &w, &h);

        if (EINA_DBL_EQ(horizontal, ELM_NOTIFY_ALIGN_FILL))
          minw = w;
        if (EINA_DBL_EQ(vertical, ELM_NOTIFY_ALIGN_FILL))
          minh = h;
        edje_object_size_min_restricted_calc(elm_layout_edje_get(sd->content_area),
                                             &minw, &minh, minw, minh);

        evas_object_size_hint_min_set(sd->content_area, minw, minh);

        if (minh > sd->max_sc_h)
          evas_object_size_hint_min_set(sd->spacer, minw, sd->max_sc_h);
        else
          evas_object_size_hint_min_set(sd->spacer, minw, minh);

       return;
     }

   if (sd->main_layout)
     {
        edje_object_size_min_calc(elm_layout_edje_get(sd->main_layout), &minw, &minh);
        evas_object_size_hint_min_set(obj, minw, minh);
        evas_object_size_hint_max_set(obj, -1, -1);
     }
}

EOLIAN static void
_elm_popup_efl_layout_signal_signal_emit(Eo *obj EINA_UNUSED, Elm_Popup_Data *sd, const char *emission, const char *source)
{
   elm_layout_signal_emit(sd->main_layout, emission, source);
}

EOLIAN static Eina_Bool
_elm_popup_efl_ui_widget_widget_sub_object_del(Eo *obj, Elm_Popup_Data *sd, Evas_Object *sobj)
{
   Elm_Popup_Item_Data *it;
   Eina_Bool int_ret = EINA_FALSE;

   int_ret = elm_widget_sub_object_del(efl_super(obj, MY_CLASS), sobj);
   if (!int_ret) return EINA_FALSE;

   if (sobj == sd->title_icon)
     {
        elm_layout_signal_emit(sd->main_layout, "elm,state,title,icon,hidden", "elm");
     }
   else if ((it =
               evas_object_data_get(sobj, "_popup_icon_parent_item")) != NULL)
     {
        if (sobj == it->icon)
          {
             efl_content_unset(efl_part(VIEW(it), CONTENT_PART));
             elm_layout_signal_emit(VIEW(it),
                                    "elm,state,item,icon,hidden", "elm");
             it->icon = NULL;
          }
     }

   return EINA_TRUE;
}

static void
_on_content_del(void *data,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   ELM_POPUP_DATA_GET(data, sd);

   sd->content = NULL;
   elm_layout_sizing_eval(data);
}

/**
 * @brief Callback invoked when the text content object of the popup is deleted.
 *
 * This function is registered on the label widget used for content text.
 * When the label is deleted, this callback nullifies the `text_content_obj`
 * pointer in the popup's data and triggers a layout recalculation.
 *
 * @param data The popup object.
 * @param e The Evas canvas (unused).
 * @param obj The label object being deleted (unused).
 * @param event_info The event data (unused).
 */
static void
_on_text_content_del(void *data,
                     Evas *e EINA_UNUSED,
                     Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   ELM_POPUP_DATA_GET(data, sd);

   sd->text_content_obj = NULL;
   elm_layout_sizing_eval(data);
}

/**
 * @brief Callback invoked when the internal table of the popup is deleted.
 *
 * This function is registered on the table object. When the table is
 * deleted, this callback nullifies pointers to the table and its child
 * objects (spacer, scroller, box) in the popup's data, and then triggers a
 * layout recalculation.
 *
 * @param data The popup object.
 * @param e The Evas canvas (unused).
 * @param obj The table object being deleted (unused).
 * @param event_info The event data (unused).
 */
static void
_on_table_del(void *data,
              Evas *e EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED,
              void *event_info EINA_UNUSED)
{
   ELM_POPUP_DATA_GET(data, sd);

   sd->tbl = NULL;
   sd->spacer = NULL;
   sd->scr = NULL;
   sd->box = NULL;
   elm_layout_sizing_eval(data);
}

/**
 * @brief Callback invoked when an action button is deleted.
 *
 * This function is registered on each action button. When a button is
 * deleted, this callback finds the corresponding button in the popup's data
 * and calls `_button_remove` to clean up its resources and update the layout.
 * It only acts if the button was marked for deletion.
 *
 * @param data The popup object.
 * @param e The Evas canvas (unused).
 * @param obj The button object being deleted.
 * @param event_info The event data (unused).
 */
static void
_on_button_del(void *data,
               Evas *e EINA_UNUSED,
               Evas_Object *obj,
               void *event_info EINA_UNUSED)
{
   int i;

   ELM_POPUP_DATA_GET(data, sd);

   for (i = 0; i < ELM_POPUP_ACTION_BUTTON_MAX; i++)
     {
        if (sd->buttons[i] && obj == sd->buttons[i]->btn &&
            sd->buttons[i]->delete_me == EINA_TRUE)
          {
             _button_remove(data, i, EINA_FALSE);
             break;
          }
     }
}

/**
 * @brief Removes a button from the action area of the popup.
 *
 * This function handles the removal of a button at a specific position. It can
 * either just detach the button from the layout or delete it completely. After
 * removing the button, it updates the state and theme of the action area to
 * reflect the new number of buttons.
 *
 * @param obj The popup object.
 * @param pos The position of the button to remove (0-based index).
 * @param delete If @c EINA_TRUE, the button object is deleted.
 */
static void
_button_remove(Evas_Object *obj,
               int pos,
               Eina_Bool delete)
{
   int i = 0;
   char buf[128];

   ELM_POPUP_DATA_GET(obj, sd);

   if (!sd->last_button_number) return;

   if (!sd->buttons[pos]) return;

   if (delete)
     {
        evas_object_del(sd->buttons[pos]->btn);
     }
   else
     {
        evas_object_event_callback_del
          (sd->buttons[pos]->btn, EVAS_CALLBACK_DEL, _on_button_del);
        snprintf(buf, sizeof(buf), "elm.swallow.content.button%i", pos + 1);
        elm_object_part_content_unset(sd->action_area, buf);
     }

   ELM_SAFE_FREE(sd->buttons[pos], free);

   sd->last_button_number = 0;

   for (i = ELM_POPUP_ACTION_BUTTON_MAX - 1; i >= 0; i--)
     {
        if (sd->buttons[i])
          {
             sd->last_button_number = i + 1;
             break;
          }
     }

   if (!sd->last_button_number)
     {
        ELM_SAFE_FREE(sd->action_area, evas_object_del);
        _visuals_set(obj);
     }
   else
     {
        char style[1024];

        snprintf(style, sizeof(style), "popup/%s", elm_widget_style_get(obj));
        snprintf(buf, sizeof(buf), "buttons%i", sd->last_button_number);
        if (!elm_layout_theme_set(sd->action_area, "popup", buf, style))
          CRI("Failed to set layout!");
     }
}

/**
 * @brief Callback for layout-related signals from the theme.
 *
 * This function is called when signals that affect layout (e.g., visibility
 * changes of title/action areas) are emitted from the edje theme. It
 * triggers a sizing evaluation to recalculate the popup's layout.
 *
 * @param data Unused.
 * @param obj The popup object whose layout needs recalculation.
 * @param emission The signal emitted (unused).
 * @param source The source of the signal (unused).
 */
static void
_layout_change_cb(void *data EINA_UNUSED,
                  Evas_Object *obj,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
   elm_layout_sizing_eval(obj);
}

/**
 * @brief Creates and configures the scroller components for the popup.
 *
 * This function sets up the necessary objects for scrollable content, including
 * a table, a spacer, and the scroller itself. The scroller is configured with
 * policies and styles appropriate for the popup.
 *
 * @param obj The popup object.
 */
static void
_create_scroller(Evas_Object *obj)
{
   char style[1024];

   ELM_POPUP_DATA_GET(obj, sd);

   //table
   sd->tbl = elm_table_add(sd->main_layout);
   evas_object_event_callback_add(sd->tbl, EVAS_CALLBACK_DEL,
                                  _on_table_del, obj);
   if (!sd->scroll)
     {
        if (sd->content || sd->text_content_obj) efl_content_unset(efl_part(sd->content_area, CONTENT_PART));
        efl_content_set(efl_part(sd->content_area, CONTENT_PART), sd->tbl);
        efl_content_set(efl_part(sd->main_layout, CONTENT_PART), sd->content_area);
     }

   //spacer
   sd->spacer = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_color_set(sd->spacer, 0, 0, 0, 0);
   elm_table_pack(sd->tbl, sd->spacer, 0, 0, 1, 1);

   //Scroller
   sd->scr = elm_scroller_add(sd->tbl);
   if (!sd->scroll)
     {
        snprintf(style, sizeof(style), "popup/%s", elm_widget_style_get(obj));
        elm_object_style_set(sd->scr, style);
     }
   else
     elm_object_style_set(sd->scr, "popup/no_inset_shadow");
   evas_object_size_hint_weight_set(sd->scr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(sd->scr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
   elm_scroller_content_min_limit(sd->scr, EINA_TRUE, EINA_FALSE);
   elm_scroller_bounce_set(sd->scr, EINA_FALSE, EINA_TRUE);
   evas_object_event_callback_add(sd->scr, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _size_hints_changed_cb, obj);
   efl_ui_mirrored_automatic_set(sd->scr, EINA_FALSE);
   elm_object_mirrored_set(sd->scr, elm_object_mirrored_get(obj));
   elm_table_pack(sd->tbl, sd->scr, 0, 0, 1, 1);
   evas_object_show(sd->scr);
}

/**
 * @brief Adds a list container (box) to the popup for holding items.
 *
 * This function ensures a scroller exists by calling `_create_scroller` if
 * needed, and then adds an `elm_box` to the scroller. This box will serve as
 * the container for popup items.
 *
 * @param obj The popup object.
 */
static void
_list_add(Evas_Object *obj)
{
   ELM_POPUP_DATA_GET(obj, sd);

   if (!sd->scr)
     _create_scroller(obj);
   //Box
   sd->box = elm_box_add(sd->scr);
   evas_object_size_hint_weight_set(sd->box, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(sd->box, EVAS_HINT_FILL, 0.0);
   elm_object_content_set(sd->scr, sd->box);
   evas_object_show(sd->box);
}

static void
_item_select_cb(void *data,
                Evas_Object *obj EINA_UNUSED,
                const char *emission EINA_UNUSED,
                const char *source EINA_UNUSED)
{
   Elm_Popup_Item_Data *it = data;

   if (!it || it->disabled) return;
   if (it->func)
     it->func((void *)WIDGET_ITEM_DATA_GET(EO_OBJ(it)), WIDGET(it), EO_OBJ(it));
}

static void
_item_text_set(Elm_Popup_Item_Data *it,
               const char *label)
{
   if (!eina_stringshare_replace(&it->label, label)) return;

   elm_layout_text_set(VIEW(it), "elm.text", label);

   if (it->label)
     elm_layout_signal_emit(VIEW(it),
                            "elm,state,item,text,visible", "elm");
   else
     elm_layout_signal_emit(VIEW(it),
                            "elm,state,item,text,hidden", "elm");

   edje_object_message_signal_process(elm_layout_edje_get(VIEW(it)));
}

EOLIAN static void
_elm_popup_item_elm_widget_item_part_text_set(Eo *eo_it EINA_UNUSED, Elm_Popup_Item_Data *it,
                    const char *part,
                    const char *label)
{
   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);

   if ((!part) || (!strcmp(part, "default")))
     {
        _item_text_set(it, label);
        return;
     }

   WRN("The part name is invalid! : popup=%p", WIDGET(it));
}

EOLIAN static const char *
_elm_popup_item_elm_widget_item_part_text_get(const Eo *eo_it EINA_UNUSED, Elm_Popup_Item_Data *it,
                    const char *part)
{
   ELM_POPUP_ITEM_CHECK_OR_RETURN(it, NULL);

   if ((!part) || (!strcmp(part, "default")))
     return it->label;

   WRN("The part name is invalid! : popup=%p", WIDGET(it));

   return NULL;
}

static void
_item_icon_set(Elm_Popup_Item_Data *it,
               Evas_Object *icon)
{
   if (it->icon == icon) return;

   evas_object_del(it->icon);
   it->icon = icon;
   if (it->icon)
     {
        elm_widget_sub_object_add(WIDGET(it), it->icon);
        evas_object_data_set(it->icon, "_popup_icon_parent_item", it);
        efl_content_set(efl_part(VIEW(it), CONTENT_PART), it->icon);
        elm_layout_signal_emit(VIEW(it), "elm,state,item,icon,visible", "elm");
     }
   else
     elm_layout_signal_emit(VIEW(it), "elm,state,item,icon,hidden", "elm");

   edje_object_message_signal_process(elm_layout_edje_get(VIEW(it)));
}

EOLIAN static void
_elm_popup_item_elm_widget_item_part_content_set(Eo *eo_it EINA_UNUSED, Elm_Popup_Item_Data *it,
                       const char *part,
                       Evas_Object *content)
{
   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);

   if ((!(part)) || (!strcmp(part, "default")))
     _item_icon_set(it, content);
   else
     WRN("The part name is invalid! : popup=%p", WIDGET(it));
}

EOLIAN static Evas_Object *
_elm_popup_item_elm_widget_item_part_content_get(const Eo *eo_it EINA_UNUSED, Elm_Popup_Item_Data *it,
                       const char *part)
{
   ELM_POPUP_ITEM_CHECK_OR_RETURN(it, NULL);

   if ((!(part)) || (!strcmp(part, "default")))
     return it->icon;

   WRN("The part name is invalid! : popup=%p", WIDGET(it));

   return NULL;
}

static Evas_Object *
_item_icon_unset(Elm_Popup_Item_Data *it)
{
   Evas_Object *icon = it->icon;

   if (!it->icon) return NULL;
   _elm_widget_sub_object_redirect_to_top(WIDGET(it), icon);
   evas_object_data_del(icon, "_popup_icon_parent_item");
   efl_content_unset(efl_part(VIEW(it), CONTENT_PART));
   elm_layout_signal_emit(VIEW(it), "elm,state,item,icon,hidden", "elm");
   it->icon = NULL;

   return icon;
}

EOLIAN static Evas_Object *
_elm_popup_item_elm_widget_item_part_content_unset(Eo *eo_it EINA_UNUSED, Elm_Popup_Item_Data *it,
                         const char *part)
{
   Evas_Object *content = NULL;

   ELM_POPUP_ITEM_CHECK_OR_RETURN(it, NULL);

   if ((!(part)) || (!strcmp(part, "default")))
     content = _item_icon_unset(it);
   else
     WRN("The part name is invalid! : popup=%p", WIDGET(it));

   return content;
}

EOLIAN static void
_elm_popup_item_elm_widget_item_disable(Eo *eo_it, Elm_Popup_Item_Data *it)
{
   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);

   if (elm_wdg_item_disabled_get(eo_it))
     elm_layout_signal_emit(VIEW(it), "elm,state,item,disabled", "elm");
   else
     elm_layout_signal_emit(VIEW(it), "elm,state,item,enabled", "elm");
}

EOLIAN static void
_elm_popup_item_efl_object_destructor(Eo *eo_it, Elm_Popup_Item_Data *it)
{
   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);
   ELM_POPUP_DATA_GET(WIDGET(it), sd);

   evas_object_del(it->icon);
   eina_stringshare_del(it->label);
   sd->items = eina_list_remove(sd->items, it);
   if (!eina_list_count(sd->items))
     {
        sd->items = NULL;
        _list_del(sd);
     }
   efl_destructor(efl_super(eo_it, ELM_POPUP_ITEM_CLASS));
}

EOLIAN static void
_elm_popup_item_elm_widget_item_signal_emit(Eo *eo_it EINA_UNUSED, Elm_Popup_Item_Data *it,
                       const char *emission,
                       const char *source)
{
   elm_layout_signal_emit(VIEW(it), emission, source);
}

/**
 * @brief Callback for focus changes on a popup item.
 *
 * This function is triggered when an item's focus state changes. It emits the
 * corresponding legacy "item,focused" or "item,unfocused" signal on the
 * parent popup widget.
 *
 * @param data The popup item data.
 * @param event The EFL focus changed event.
 */
static void
_item_focus_change(void *data, const Efl_Event *event EINA_UNUSED)
{
   Elm_Popup_Item_Data *it = data;

   if (efl_ui_focus_object_focus_get(event->object))
     {
        efl_event_callback_legacy_call(WIDGET(it), ELM_POPUP_EVENT_ITEM_FOCUSED, EO_OBJ(it));
     }
   else
     {
        efl_event_callback_legacy_call(WIDGET(it), ELM_POPUP_EVENT_ITEM_UNFOCUSED, EO_OBJ(it));
     }
}

EOLIAN static Eo *
_elm_popup_item_efl_object_constructor(Eo *eo_it, Elm_Popup_Item_Data *it)
{
   eo_it = efl_constructor(efl_super(eo_it, ELM_POPUP_ITEM_CLASS));
   it->base = efl_data_scope_get(eo_it, ELM_WIDGET_ITEM_CLASS);

   return eo_it;
}

/**
 * @brief Initializes a new popup item.
 *
 * This function creates the layout for a new popup item, sets its style,
 * and configures its properties like focus handling and mirroring. It also sets
 * up the "click" signal callback.
 *
 * @param it The popup item data for the new item.
 */
static void
_item_new(Elm_Popup_Item_Data *it)
{
   char style[1024];

   VIEW_SET(it, elm_layout_add(WIDGET(it)));
   elm_object_focus_allow_set(VIEW(it), EINA_TRUE);
   efl_ui_mirrored_automatic_set(VIEW(it), EINA_FALSE);
   elm_object_mirrored_set(VIEW(it), elm_object_mirrored_get(WIDGET(it)));

   snprintf(style, sizeof(style), "popup/%s", elm_widget_style_get(WIDGET(it)));
   if (!elm_layout_theme_set(VIEW(it), "popup", "item", style))
     CRI("Failed to set layout!");
   else
     {
        elm_layout_signal_callback_add(VIEW(it), "elm,action,click", "*",
                                       _item_select_cb, it);
        evas_object_size_hint_align_set(VIEW(it), EVAS_HINT_FILL, EVAS_HINT_FILL);
        efl_event_callback_add
              (VIEW(it), EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED, _item_focus_change, it);
        evas_object_show(VIEW(it));
     }
}

/**
 * @brief Sets the text of the popup's title area.
 *
 * This function updates the title text, sharing the string to save memory.
 * It also updates accessibility information and emits signals to the theme
 * to show or hide the title text part. If the overall visibility of the
 * title area changes, it triggers a visual update of the whole popup.
 *
 * @param obj The popup object.
 * @param text The text to set as the title.
 * @return @c EINA_TRUE on success.
 */
static Eina_Bool
_title_text_set(Evas_Object *obj,
                const char *text)
{
   Evas_Object *ao;
   Eina_Bool title_visibility_old, title_visibility_current;

   ELM_POPUP_DATA_GET(obj, sd);

   if (sd->title_text == text) return EINA_TRUE;

   title_visibility_old = (sd->title_text) || (sd->title_icon);
   eina_stringshare_replace(&sd->title_text, text);

   elm_layout_text_set(sd->main_layout, "elm.text.title", text);

   /* access */
   if (_elm_config->access_mode)
     {
        ao = _access_object_get(obj, ACCESS_TITLE_PART);
        if (!ao)
          {
             ao = _elm_access_edje_object_part_object_register
                    (obj, elm_layout_edje_get(sd->main_layout), ACCESS_TITLE_PART);
             _elm_access_text_set(_elm_access_info_get(ao),
                                  ELM_ACCESS_TYPE, E_("Popup Title"));
          }
        _elm_access_text_set(_elm_access_info_get(ao), ELM_ACCESS_INFO, text);
     }

   if (sd->title_text)
     elm_layout_signal_emit(sd->main_layout, "elm,state,title,text,visible", "elm");
   else
     elm_layout_signal_emit(sd->main_layout, "elm,state,title,text,hidden", "elm");

   title_visibility_current = (sd->title_text) || (sd->title_icon);

   if (title_visibility_old != title_visibility_current)
     _visuals_set(obj);

   return EINA_TRUE;
}

/**
 * @brief Sets the text of the popup's content area.
 *
 * This function is used to display a simple text message in the popup. If there
 * are items, they are removed. If there is other content, it is replaced.
 * A new label is created for the text, and accessibility information is
 * updated.
 *
 * @param obj The popup object.
 * @param text The text to set in the content area.
 * @return @c EINA_TRUE on success.
 */
static Eina_Bool
_content_text_set(Evas_Object *obj,
                  const char *text)
{
   Evas_Object *ao;
   char style[1024];

   ELM_POPUP_DATA_GET(obj, sd);

   if (sd->items)
     {
        _items_remove(sd);
        _list_del(sd);
     }
   else
     {
        if (!sd->scroll)
          efl_content_set(efl_part(sd->main_layout, CONTENT_PART), sd->content_area);
        else
          elm_object_content_set(sd->scr, sd->content_area);
     }
   if (!text) goto end;

   if (sd->text_content_obj)
     {
        sd->text_content_obj = efl_content_unset(efl_part(sd->content_area, CONTENT_PART));
        evas_object_del(sd->text_content_obj);
        sd->text_content_obj = NULL;
     }

   sd->text_content_obj = elm_label_add(sd->content_area);
   snprintf(style, sizeof(style), "popup/%s", elm_widget_style_get(obj));
   elm_object_style_set(sd->text_content_obj, style);

   evas_object_event_callback_add
     (sd->text_content_obj, EVAS_CALLBACK_DEL, _on_text_content_del, obj);

   elm_label_line_wrap_set(sd->text_content_obj, ELM_WRAP_NONE);
   elm_object_text_set(sd->text_content_obj, text);
   efl_canvas_group_calculate(sd->text_content_obj);
   elm_label_line_wrap_set(sd->text_content_obj, sd->content_text_wrap_type);

   evas_object_size_hint_weight_set
     (sd->text_content_obj, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set
     (sd->text_content_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
   efl_content_set
     (efl_part(sd->content_area, CONTENT_PART), sd->text_content_obj);

   /* access */
   if (_elm_config->access_mode)
     {
        /* unregister label, ACCESS_BODY_PART will register */
        elm_access_object_unregister(sd->text_content_obj);

        ao = _access_object_get(obj, ACCESS_BODY_PART);
        if (!ao)
          {
             ao = _elm_access_edje_object_part_object_register
                    (obj, elm_layout_edje_get(sd->main_layout), ACCESS_BODY_PART);
             _elm_access_text_set(_elm_access_info_get(ao),
                                  ELM_ACCESS_TYPE, E_("Popup Body Text"));
          }
        _elm_access_text_set(_elm_access_info_get(ao), ELM_ACCESS_INFO, text);
     }

end:
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Sets the text of a part of the popup.
 *
 * This function acts as a dispatcher for setting text on various parts of the
 * popup, such as the main content text ("elm.text") or the title text
 * ("title,text"). It calls the appropriate internal function based on the
 * provided part name.
 *
 * @param obj The popup object.
 * @param _pd The popup's private data.
 * @param part The name of the text part to set.
 * @param label The text to set.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
_elm_popup_text_set(Eo *obj, Elm_Popup_Data *_pd, const char *part, const char *label)
{
   Eina_Bool int_ret = EINA_FALSE;

   if (!_elm_layout_part_aliasing_eval(obj, &part, EINA_TRUE))
      return EINA_FALSE;

   if (!strcmp(part, "elm.text"))
     int_ret = _content_text_set(obj, label);
   else if (!strcmp(part, "title,text"))
     int_ret = _title_text_set(obj, label);
   else
     int_ret = elm_layout_text_set(_pd->main_layout, part, label);

   elm_layout_sizing_eval(obj);

   return int_ret;
}

/**
 * @internal
 * @brief Gets the text of the popup's title area.
 *
 * @param sd The popup's private data.
 * @return The title text, or @c NULL if not set.
 */
static const char *
_title_text_get(const Elm_Popup_Data *sd)
{
   return sd->title_text;
}

/**
 * @internal
 * @brief Gets the text from the popup's content area.
 *
 * This function retrieves text only if the content is a label set by
 * `_content_text_set`.
 *
 * @param sd The popup's private data.
 * @return The content text, or @c NULL if not set or if content is not text.
 */
static const char *
_content_text_get(const Elm_Popup_Data *sd)
{
   const char *str = NULL;

   if (sd->text_content_obj)
     str = elm_object_text_get(sd->text_content_obj);

   return str;
}

/**
 * @internal
 * @brief Gets the text of a part of the popup.
 *
 * This function acts as a dispatcher for getting text from various parts of
 * the popup, such as the main content text ("elm.text") or the title text
 * ("title,text"). It calls the appropriate internal getter based on the
 * provided part name.
 *
 * @param obj The popup object (unused).
 * @param _pd The popup's private data.
 * @param part The name of the text part to get.
 * @return The text of the part, or @c NULL on failure.
 */
static const char *
_elm_popup_text_get(Eo *obj EINA_UNUSED, Elm_Popup_Data *_pd, const char *part)
{
   const char *text = NULL;

   if (!_elm_layout_part_aliasing_eval(obj, &part, EINA_TRUE))
      return NULL;

   if (!strcmp(part, "elm.text"))
     text = _content_text_get(_pd);
   else if (!strcmp(part, "title,text"))
     text = _title_text_get(_pd);
   else
     text = elm_layout_text_get(_pd->main_layout, part);

   return text;
}

static Eina_Bool
_title_icon_set(Evas_Object *obj,
                Evas_Object *icon)
{
   Eina_Bool title_visibility_old, title_visibility_current;

   ELM_POPUP_DATA_GET(obj, sd);

   if (sd->title_icon == icon) return EINA_TRUE;
   title_visibility_old = (sd->title_text) || (sd->title_icon);
   evas_object_del(sd->title_icon);

   sd->title_icon = icon;
   title_visibility_current = (sd->title_text) || (sd->title_icon);

   efl_content_set
      (efl_part(sd->main_layout, "elm.swallow.title.icon"), sd->title_icon);

   if (sd->title_icon)
     elm_layout_signal_emit(sd->main_layout, "elm,state,title,icon,visible", "elm");
   if (title_visibility_old != title_visibility_current) _visuals_set(obj);

   return EINA_TRUE;
}

static Eina_Bool
_content_set(Evas_Object *obj,
             Evas_Object *content)
{
   ELM_POPUP_DATA_GET(obj, sd);

   if (sd->content && sd->content == content) return EINA_TRUE;
   if (sd->items)
     {
        _items_remove(sd);
        _list_del(sd);
     }

   evas_object_del(sd->content);
   sd->content = content;

   if (content)
     {
        if (!sd->scroll)
          efl_content_set(efl_part(sd->main_layout, CONTENT_PART), sd->content_area);
        else
          elm_object_content_set(sd->scr, sd->content_area);

        evas_object_show(content);
        efl_content_set(efl_part(sd->content_area, CONTENT_PART), content);

        evas_object_event_callback_add
          (content, EVAS_CALLBACK_DEL, _on_content_del, obj);
     }

   return EINA_TRUE;
}

static void
_action_button_set(Evas_Object *obj,
                   Evas_Object *btn,
                   unsigned int idx)
{
   char buf[128], style[1024];
   unsigned int i;

   ELM_POPUP_DATA_GET(obj, sd);

   if (idx >= ELM_POPUP_ACTION_BUTTON_MAX) return;

   if (!btn)
     {
        _button_remove(obj, idx, EINA_TRUE);
        return;
     }

   if (sd->buttons[idx])
     {
        evas_object_del(sd->buttons[idx]->btn);
        free(sd->buttons[idx]);
     }

   sd->buttons[idx] = ELM_NEW(Action_Area_Data);
   sd->buttons[idx]->obj = obj;
   sd->buttons[idx]->btn = btn;

   evas_object_event_callback_add
     (btn, EVAS_CALLBACK_DEL, _on_button_del, obj);

   for (i = ELM_POPUP_ACTION_BUTTON_MAX - 1; i >= idx; i--)
     {
        if (sd->buttons[i])
          {
             sd->last_button_number = i + 1;
             break;
          }
     }

   if (!sd->action_area)
     {
        sd->action_area = elm_layout_add(sd->main_layout);
        evas_object_event_callback_add
          (sd->action_area, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
           _size_hints_changed_cb, sd->main_layout);
        efl_ui_mirrored_automatic_set(sd->action_area, EINA_FALSE);
        elm_object_mirrored_set(sd->action_area, elm_object_mirrored_get(obj));
        efl_content_set(efl_part(sd->main_layout, "elm.swallow.action_area"), sd->action_area);

        _visuals_set(obj);
     }

   snprintf(buf, sizeof(buf), "buttons%i", sd->last_button_number);
   snprintf(style, sizeof(style), "popup/%s", elm_widget_style_get(obj));
   if (!elm_layout_theme_set(sd->action_area, "popup", buf, style))
     CRI("Failed to set layout!");

   snprintf(buf, sizeof(buf), "elm.swallow.content.button%i", idx + 1);
   evas_object_show(sd->buttons[idx]->btn);
   elm_object_part_content_set
     (sd->action_area, buf, sd->buttons[idx]->btn);
}

static Eina_Bool
_elm_popup_content_set(Eo *obj, Elm_Popup_Data *_pd EINA_UNUSED, const char *part, Evas_Object *content)
{
   unsigned int i;
   Eina_Bool ret = EINA_TRUE;

   if (!part || !strcmp(part, "elm.swallow.content"))
     ret = _content_set(obj, content);
   else if (!strcmp(part, "title,icon"))
     ret = _title_icon_set(obj, content);
   else if (!strncmp(part, "button", 6))
     {
        i = atoi(part + 6) - 1;

        if (i >= ELM_POPUP_ACTION_BUTTON_MAX)
          {
             ERR("The part name is invalid! : popup=%p", obj);
             return EINA_FALSE;
          }

        _action_button_set(obj, content, i);
     }
   else
     ret = efl_content_set(efl_part(_pd->main_layout, part), content);

   elm_layout_sizing_eval(obj);

   return ret;
}

static Evas_Object *
_title_icon_get(const Elm_Popup_Data *sd)
{
   return sd->title_icon;
}

static Evas_Object *
_content_get(const Elm_Popup_Data *sd)
{
   Evas_Object *ret;

   ret = sd->content ? sd->content : sd->text_content_obj;

   return ret;
}

static Evas_Object *
_action_button_get(const Evas_Object *obj,
                   unsigned int idx)
{
   Evas_Object *button = NULL;

   ELM_POPUP_DATA_GET(obj, sd);
   if (!sd->action_area) return NULL;

   if (sd->buttons[idx])
     button = sd->buttons[idx]->btn;

   return button;
}

static Evas_Object*
_elm_popup_content_get(Eo *obj, Elm_Popup_Data *_pd, const char *part)
{
   Evas_Object *content = NULL;
   unsigned int i;

   if (!part || !strcmp(part, "elm.swallow.content"))
     content = _content_get(_pd);
   else if (!strcmp(part, "title,text"))
     content = _title_icon_get(_pd);
   else if (!strncmp(part, "button", 6))
     {
        i = atoi(part + 6) - 1;

        if (i >= ELM_POPUP_ACTION_BUTTON_MAX)
          goto err;

        content = _action_button_get(obj, i);
     }
   else
      content = efl_content_get(efl_part(_pd->main_layout, part));

   if (!content)
     goto err;

   return content;

err:
   WRN("The part name is invalid! : popup=%p", obj);
   return NULL;
}

static Evas_Object *
_content_unset(Evas_Object *obj)
{
   Evas_Object *content;

   ELM_POPUP_DATA_GET(obj, sd);

   if (!sd->content) return NULL;

   evas_object_event_callback_del
     (sd->content, EVAS_CALLBACK_DEL, _on_content_del);

   content = efl_content_unset(efl_part(sd->content_area, CONTENT_PART));
   sd->content = NULL;

   elm_layout_sizing_eval(obj);

   return content;
}

static Evas_Object *
_title_icon_unset(Evas_Object *obj)
{
   Evas_Object *icon;

   ELM_POPUP_DATA_GET(obj, sd);

   if (!sd->title_icon) return NULL;

   icon = sd->title_icon;
   efl_content_unset(efl_part(sd->main_layout, "elm.swallow.title.icon"));
   sd->title_icon = NULL;

   return icon;
}

static Evas_Object*
_elm_popup_content_unset(Eo *obj, Elm_Popup_Data *_pd EINA_UNUSED, const char *part)
{
   Evas_Object *content = NULL;
   unsigned int i;

   if (!part || !strcmp(part, "elm.swallow.content"))
     content = _content_unset(obj);
   else if (!strcmp(part, "title,icon"))
     content = _title_icon_unset(obj);
   else if (!strncmp(part, "button", 6))
     {
        i = atoi(part + 6) - 1;

        if (i >= ELM_POPUP_ACTION_BUTTON_MAX)
          goto err;

        _button_remove(obj, i, EINA_FALSE);
     }
   else
     goto err;

   return content;

err:
   ERR("The part name is invalid! : popup=%p", obj);

   return NULL;
}

static Eina_Bool
_key_action_escape(Evas_Object *obj, const char *params EINA_UNUSED)
{
   ELM_POPUP_DATA_GET(obj, pd);
   elm_layout_signal_emit(pd->main_layout, "elm,state,hide", "elm");
   elm_notify_dismiss(pd->notify);
   return EINA_TRUE;
}

EOLIAN static void
_elm_popup_efl_canvas_group_group_add(Eo *obj, Elm_Popup_Data *priv)
{
   char style[1024];

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   snprintf(style, sizeof(style), "popup/%s", elm_widget_style_get(obj));

   priv->notify = elm_notify_add(obj);
   elm_object_style_set(priv->notify, style);

   elm_notify_align_set(priv->notify,
                        _elm_config->popup_horizontal_align,
                        _elm_config->popup_vertical_align);
   elm_notify_allow_events_set(priv->notify, EINA_FALSE);
   evas_object_size_hint_weight_set
     (priv->notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set
     (priv->notify, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_member_add(priv->notify, obj);
   efl_ui_mirrored_automatic_set(priv->notify, EINA_FALSE);
   elm_object_mirrored_set(priv->notify, elm_object_mirrored_get(obj));

   evas_object_event_callback_add(priv->notify, EVAS_CALLBACK_RESIZE, _notify_resize_cb, obj);
   efl_event_callback_array_add(priv->notify, composition_cb(), obj);

   priv->main_layout = elm_layout_add(obj);
   if (!elm_layout_theme_set(priv->main_layout, "popup", "base",
                             elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   elm_object_content_set(priv->notify, priv->main_layout);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _on_show, NULL);
   efl_ui_mirrored_automatic_set(priv->main_layout, EINA_FALSE);
   elm_object_mirrored_set(priv->main_layout, elm_object_mirrored_get(obj));

   elm_layout_signal_callback_add
     (priv->main_layout, "elm,state,title_area,visible", "elm", _layout_change_cb, NULL);
   elm_layout_signal_callback_add
     (priv->main_layout, "elm,state,title_area,hidden", "elm", _layout_change_cb, NULL);
   elm_layout_signal_callback_add
     (priv->main_layout, "elm,state,action_area,visible", "elm", _layout_change_cb, NULL);
   elm_layout_signal_callback_add
     (priv->main_layout, "elm,state,action_area,hidden", "elm", _layout_change_cb, NULL);

   priv->content_area = elm_layout_add(priv->main_layout);
   if (!elm_layout_theme_set(priv->content_area, "popup", "content", style))
     CRI("Failed to set layout!");
   else
     evas_object_event_callback_add
        (priv->content_area, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
         _size_hints_changed_cb, priv->main_layout);

   priv->content_text_wrap_type = ELM_WRAP_MIXED;
   efl_event_callback_array_add(priv->notify, _notify_cb(), obj);

   _populate_theme_scroll(priv);

   _visuals_set(obj);

   if (_elm_config->popup_scrollable)
     elm_popup_scrollable_set(obj, _elm_config->popup_scrollable);
}

static void
_parent_geom_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Coord x, y, w, h;
   Evas_Object *popup = data;

   evas_object_geometry_get(obj, &x, &y, &w, &h);

   if (efl_isa(obj, EFL_UI_WIN_CLASS))
     {
        x = 0;
        y = 0;
     }

   evas_object_move(popup, x, y);
   evas_object_resize(popup, w, h);
}

static void
_parent_setup(Eo *obj, Elm_Popup_Data *sd, Evas_Object *parent)
{
   Evas_Coord x, y, w, h;

   evas_object_geometry_get(parent, &x, &y, &w, &h);

   if (efl_isa(parent, EFL_UI_WIN_CLASS))
     {
        x = 0;
        y = 0;
     }
   evas_object_move(obj, x, y);
   evas_object_resize(obj, w, h);

   sd->parent = parent;
   evas_object_event_callback_add(parent, EVAS_CALLBACK_RESIZE, _parent_geom_cb, obj);
   evas_object_event_callback_add(parent, EVAS_CALLBACK_MOVE, _parent_geom_cb, obj);
}

EOLIAN static void
_elm_popup_efl_ui_widget_on_access_update(Eo *obj, Elm_Popup_Data *_pd EINA_UNUSED, Eina_Bool is_access)
{
   _access_obj_process(obj, is_access);
}

EAPI Evas_Object *
elm_popup_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   Evas_Object *obj = elm_legacy_add(MY_CLASS, parent);

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);
   wd->highlight_root = EINA_TRUE;

   return obj;
}

EOLIAN static Eo *
_elm_popup_efl_object_constructor(Eo *obj, Elm_Popup_Data *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_DIALOG);
   legacy_object_focus_handle(obj);
   _parent_setup(obj, _pd, efl_parent_get(obj));

   return obj;
}

EOLIAN static void
_elm_popup_content_text_wrap_type_set(Eo *obj EINA_UNUSED, Elm_Popup_Data *sd, Elm_Wrap_Type wrap)
{
   //Need to wrap the content text, so not allowing ELM_WRAP_NONE
   if (wrap == ELM_WRAP_NONE) return;

   sd->content_text_wrap_type = wrap;
   if (sd->text_content_obj)
     elm_label_line_wrap_set(sd->text_content_obj, wrap);
}

EOLIAN static Elm_Wrap_Type
_elm_popup_content_text_wrap_type_get(const Eo *obj EINA_UNUSED, Elm_Popup_Data *sd)
{
   return sd->content_text_wrap_type;
}

/* keeping old externals orient api for notify, but taking away the
 * introduced deprecation warning by copying the deprecated code
 * here */
static Elm_Notify_Orient
_elm_notify_orient_get(const Evas_Object *obj)
{
   Elm_Notify_Orient orient;
   double horizontal, vertical;

   elm_notify_align_get(obj, &horizontal, &vertical);

   if ((EINA_DBL_EQ(horizontal, 0.5)) && (EINA_DBL_EQ(vertical, 0.0)))
     orient = ELM_NOTIFY_ORIENT_TOP;
   else if ((EINA_DBL_EQ(horizontal, 0.5)) && (EINA_DBL_EQ(vertical, 0.5)))
     orient = ELM_NOTIFY_ORIENT_CENTER;
   else if ((EINA_DBL_EQ(horizontal, 0.5)) && (EINA_DBL_EQ(vertical, 1.0)))
     orient = ELM_NOTIFY_ORIENT_BOTTOM;
   else if ((EINA_DBL_EQ(horizontal, 0.0)) && (EINA_DBL_EQ(vertical, 0.5)))
     orient = ELM_NOTIFY_ORIENT_LEFT;
   else if ((EINA_DBL_EQ(horizontal, 1.0)) && (EINA_DBL_EQ(vertical, 0.5)))
     orient = ELM_NOTIFY_ORIENT_RIGHT;
   else if ((EINA_DBL_EQ(horizontal, 0.0)) && (EINA_DBL_EQ(vertical, 0.0)))
     orient = ELM_NOTIFY_ORIENT_TOP_LEFT;
   else if ((EINA_DBL_EQ(horizontal, 1.0)) && (EINA_DBL_EQ(vertical, 0.0)))
     orient = ELM_NOTIFY_ORIENT_TOP_RIGHT;
   else if ((EINA_DBL_EQ(horizontal, 0.0)) && (EINA_DBL_EQ(vertical, 1.0)))
     orient = ELM_NOTIFY_ORIENT_BOTTOM_LEFT;
   else if ((EINA_DBL_EQ(horizontal, 1.0)) && (EINA_DBL_EQ(vertical, 1.0)))
     orient = ELM_NOTIFY_ORIENT_BOTTOM_RIGHT;
   else
     orient = ELM_NOTIFY_ORIENT_TOP;
   return orient;
}

static void
_elm_notify_orient_set(Evas_Object *obj,
                       Elm_Notify_Orient orient)
{
   double horizontal = 0, vertical = 0;

   switch (orient)
     {
      case ELM_NOTIFY_ORIENT_TOP:
         horizontal = 0.5; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_CENTER:
         horizontal = 0.5; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM:
         horizontal = 0.5; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_LEFT:
         horizontal = 0.0; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_RIGHT:
         horizontal = 1.0; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_TOP_LEFT:
         horizontal = 0.0; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_TOP_RIGHT:
         horizontal = 1.0; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM_LEFT:
         horizontal = 0.0; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM_RIGHT:
         horizontal = 1.0; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_LAST:
        break;
     }

   elm_notify_align_set(obj, horizontal, vertical);
}

EOLIAN static void
_elm_popup_orient_set(Eo *obj EINA_UNUSED, Elm_Popup_Data *sd, Elm_Popup_Orient orient)
{
   if (orient >= ELM_POPUP_ORIENT_LAST) return;
   _elm_notify_orient_set(sd->notify, (Elm_Notify_Orient)orient);
}

EOLIAN static Elm_Popup_Orient
_elm_popup_orient_get(const Eo *obj EINA_UNUSED, Elm_Popup_Data *sd)
{
   return (Elm_Popup_Orient)_elm_notify_orient_get(sd->notify);
}

EOLIAN static void
_elm_popup_align_set(Eo *obj EINA_UNUSED, Elm_Popup_Data *sd, double horizontal, double vertical)
{
   elm_notify_align_set(sd->notify, horizontal, vertical);
}

EOLIAN static void
_elm_popup_align_get(const Eo *obj EINA_UNUSED, Elm_Popup_Data *sd, double *horizontal, double *vertical)
{
   elm_notify_align_get(sd->notify, horizontal, vertical);
}

EOLIAN static void
_elm_popup_timeout_set(Eo *obj EINA_UNUSED, Elm_Popup_Data *sd, double timeout)
{
   elm_notify_timeout_set(sd->notify, timeout);
}

EOLIAN static double
_elm_popup_timeout_get(const Eo *obj EINA_UNUSED, Elm_Popup_Data *sd)
{
   return elm_notify_timeout_get(sd->notify);
}

EOLIAN static void
_elm_popup_allow_events_set(Eo *obj EINA_UNUSED, Elm_Popup_Data *sd, Eina_Bool allow)
{
   Eina_Bool allow_events = !!allow;

   elm_notify_allow_events_set(sd->notify, allow_events);
}

EOLIAN static Eina_Bool
_elm_popup_allow_events_get(const Eo *obj EINA_UNUSED, Elm_Popup_Data *sd)
{
   return elm_notify_allow_events_get(sd->notify);
}

EOLIAN static Elm_Object_Item*
_elm_popup_item_append(Eo *obj, Elm_Popup_Data *sd, const char *label, Evas_Object *icon, Evas_Smart_Cb func, const void *data)
{
   Evas_Object *prev_content;
   Eo *eo_it;

   eo_it = efl_add(ELM_POPUP_ITEM_CLASS, obj);
   if (!eo_it) return NULL;
   ELM_POPUP_ITEM_DATA_GET(eo_it, it);
   if (sd->content || sd->text_content_obj)
     {
        prev_content = efl_content_get(efl_part(sd->content_area, CONTENT_PART));
        evas_object_del(prev_content);
     }

   //The first item is appended.
   if (!sd->items)
     _list_add(obj);

   it->func = func;
   WIDGET_ITEM_DATA_SET(eo_it, data);

   _item_new(it);
   _item_icon_set(it, icon);
   _item_text_set(it, label);

   elm_box_pack_end(sd->box, VIEW(it));
   sd->items = eina_list_append(sd->items, it);

   elm_layout_sizing_eval(obj);

   return eo_it;
}

EOLIAN void
_elm_popup_scrollable_set(Eo *obj, Elm_Popup_Data *pd, Eina_Bool scroll)
{
   scroll = !!scroll;
   if (pd->scroll == scroll) return;
   pd->scroll = scroll;

   if (!pd->scr)
     _create_scroller(obj);
   else
     {
        elm_object_content_unset(pd->scr);
        ELM_SAFE_FREE(pd->tbl, evas_object_del);
        _create_scroller(obj);
     }

   if (!pd->scroll)
     {
        efl_content_set(efl_part(pd->content_area, CONTENT_PART), pd->tbl);
        efl_content_set(efl_part(pd->main_layout, CONTENT_PART), pd->content_area);
        if (pd->content) efl_content_set(efl_part(pd->content_area, CONTENT_PART), pd->content);
        else if (pd->text_content_obj) efl_content_set(efl_part(pd->content_area, CONTENT_PART), pd->text_content_obj);
        if (pd->theme_scroll)
          elm_layout_signal_emit(pd->content_area, "elm,scroll,disable", "elm");
     }
   else
     {
        if (pd->content || pd->text_content_obj)
          {
             efl_content_unset(efl_part(pd->main_layout, CONTENT_PART));
             elm_object_content_set(pd->scr, pd->content_area);
          }
        efl_content_set(efl_part(pd->main_layout, CONTENT_PART), pd->tbl);
        if (pd->theme_scroll)
          elm_layout_signal_emit(pd->content_area, "elm,scroll,enable", "elm");
     }

   elm_layout_sizing_eval(obj);
}

EOLIAN Eina_Bool
_elm_popup_scrollable_get(const Eo *obj EINA_UNUSED, Elm_Popup_Data *pd)
{
   return pd->scroll;
}

EOLIAN static void
_elm_popup_dismiss(Eo *obj EINA_UNUSED, Elm_Popup_Data *pd)
{
   elm_layout_signal_emit(pd->main_layout, "elm,state,hide", "elm");
   elm_notify_dismiss(pd->notify);
}

static void
_elm_popup_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

static Eina_Bool
_action_dismiss(Evas_Object *obj, const char *params EINA_UNUSED)
{
   efl_event_callback_legacy_call(obj, ELM_POPUP_EVENT_BLOCK_CLICKED, NULL);
   return EINA_TRUE;
}

EOLIAN const Efl_Access_Action_Data *
_elm_popup_efl_access_widget_action_elm_actions_get(const Eo *obj EINA_UNUSED, Elm_Popup_Data *pd)
{
   static Efl_Access_Action_Data atspi_actions[] = {
          { "dismiss", NULL, NULL, _action_dismiss},
          { NULL, NULL, NULL, NULL }
   };
   if (pd->action_area)
     return NULL;
   return &atspi_actions[0];
}

EOLIAN static Efl_Access_State_Set
_elm_popup_efl_access_object_state_set_get(const Eo *obj, Elm_Popup_Data *sd EINA_UNUSED)
{
   Efl_Access_State_Set ret;
   ret = efl_access_object_state_set_get(efl_super(obj, MY_CLASS));

   STATE_TYPE_SET(ret, EFL_ACCESS_STATE_TYPE_MODAL);

   return ret;
}

EOLIAN static const char*
_elm_popup_efl_access_object_i18n_name_get(const Eo *obj, Elm_Popup_Data *sd)
{
   const char *name = NULL;
   Eina_Strbuf *buf;

   name = efl_access_object_i18n_name_get(efl_super(obj, ELM_POPUP_CLASS));
   if (name) return name;

   buf = eina_strbuf_new();
   eina_strbuf_append_printf(buf, "%s", E_("Alert"));
   if (sd->title_text)
     eina_strbuf_append_printf(buf, ", %s", sd->title_text);
   else if (sd->text_content_obj)
     eina_strbuf_append_printf(buf, ", %s", elm_object_text_get(sd->text_content_obj));
   else if (elm_object_part_text_get(obj, "elm.text"))
     eina_strbuf_append_printf(buf, ", %s", elm_object_part_text_get(obj, "elm.text"));

   name = _elm_widget_accessible_plain_name_get(obj, eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);

   return name;
}

/* Standard widget overrides */

ELM_WIDGET_KEY_DOWN_DEFAULT_IMPLEMENT(elm_popup, Elm_Popup_Data)

/* Efl.Part begin */

ELM_PART_OVERRIDE(elm_popup, ELM_POPUP, Elm_Popup_Data)
ELM_PART_OVERRIDE_CONTENT_SET(elm_popup, ELM_POPUP, Elm_Popup_Data)
ELM_PART_OVERRIDE_CONTENT_GET(elm_popup, ELM_POPUP, Elm_Popup_Data)
ELM_PART_OVERRIDE_CONTENT_UNSET(elm_popup, ELM_POPUP, Elm_Popup_Data)
ELM_PART_OVERRIDE_TEXT_SET(elm_popup, ELM_POPUP, Elm_Popup_Data)
ELM_PART_OVERRIDE_TEXT_GET(elm_popup, ELM_POPUP, Elm_Popup_Data)
#include "elm_popup_part.eo.c"

/* Efl.Part end */

/* Internal EO APIs and hidden overrides */

#define ELM_POPUP_EXTRA_OPS \
   EFL_CANVAS_GROUP_CALC_OPS(elm_popup), \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_popup)

#include "elm_popup_eo.c"
#include "elm_popup_item_eo.c"
