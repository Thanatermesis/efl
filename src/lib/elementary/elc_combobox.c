#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_ACCESS_WIDGET_ACTION_PROTECTED
#define ELM_WIDGET_PROTECTED
#define EFL_UI_L10N_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_combobox.h"
#include "elm_entry_eo.h"
#include "elm_genlist_eo.h"
#include "elm_hover_eo.h"

EOAPI void elm_obj_combobox_hover_begin(Eo *obj);
EOAPI void elm_obj_combobox_hover_end(Eo *obj);

static const Efl_Event_Description _ELM_COMBOBOX_EVENT_DISMISSED =
   EFL_EVENT_DESCRIPTION("dismissed");
static const Efl_Event_Description _ELM_COMBOBOX_EVENT_EXPANDED =
   EFL_EVENT_DESCRIPTION("expanded");
static const Efl_Event_Description _ELM_COMBOBOX_EVENT_ITEM_SELECTED =
   EFL_EVENT_DESCRIPTION("item,selected");
static const Efl_Event_Description _ELM_COMBOBOX_EVENT_ITEM_PRESSED =
   EFL_EVENT_DESCRIPTION("item,pressed");
static const Efl_Event_Description _ELM_COMBOBOX_EVENT_FILTER_DONE =
   EFL_EVENT_DESCRIPTION("filter,done");

#define ELM_COMBOBOX_EVENT_DISMISSED (&(_ELM_COMBOBOX_EVENT_DISMISSED))
#define ELM_COMBOBOX_EVENT_EXPANDED (&(_ELM_COMBOBOX_EVENT_EXPANDED))
#define ELM_COMBOBOX_EVENT_ITEM_SELECTED (&(_ELM_COMBOBOX_EVENT_ITEM_SELECTED))
#define ELM_COMBOBOX_EVENT_ITEM_PRESSED (&(_ELM_COMBOBOX_EVENT_ITEM_PRESSED))
#define ELM_COMBOBOX_EVENT_FILTER_DONE (&(_ELM_COMBOBOX_EVENT_FILTER_DONE))

#define MY_CLASS ELM_COMBOBOX_CLASS

#define MY_CLASS_NAME "Elm_Combobox"
#define MY_CLASS_NAME_LEGACY "elm_combobox"

static const char SIG_DISMISSED[] = "dismissed";
static const char SIG_EXPANDED[] = "expanded";
static const char SIG_ITEM_SELECTED[] = "item,selected";
static const char SIG_ITEM_PRESSED[] = "item,pressed";
static const char SIG_FILTER_DONE[] = "filter,done";
static const char SIG_CLICKED[] = "clicked";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_DISMISSED, ""},
   {SIG_EXPANDED, ""},
   {SIG_ITEM_SELECTED, ""},
   {SIG_ITEM_PRESSED, ""},
   {SIG_FILTER_DONE, ""},
   {SIG_CLICKED, ""}, /**< handled by parent button class */
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {NULL, NULL}
};

/**
 * @brief Resizes the combobox's internal table layout based on item count and available space.
 * @param data The combobox object.
 */
static void _table_resize(void *data);

/**
 * @brief Handles key actions for moving the selection within the combobox list.
 * @param obj The combobox object.
 * @param params The direction of movement (e.g., "up", "down").
 * @return EINA_TRUE if the action was handled, EINA_FALSE otherwise.
 */
static Eina_Bool _key_action_move(Evas_Object *obj, const char *params);

/**
 * @brief Handles key actions for activating an item or expanding/collapsing the combobox.
 * @param obj The combobox object.
 * @param params Action parameters (unused).
 * @return EINA_TRUE if the action was handled, EINA_FALSE otherwise.
 */
static Eina_Bool _key_action_activate(Evas_Object *obj, const char *params);

/**
 * @brief Defines the key actions for the combobox widget.
 *
 * Each Elm_Action structure maps an action name (string) to a callback function.
 * - "activate": Triggers the _key_action_activate function.
 * - "move": Triggers the _key_action_move function.
 *
 * Example structure:
 * @code
 *   {
 *     "action_name_string", // Name of the action
 *     callback_function_pointer // Function to call for this action
 *   }
 * @endcode
 */
static const Elm_Action key_actions[] = {
   {"activate", _key_action_activate},
   {"move", _key_action_move},
   {NULL, NULL}
};

EOLIAN static void
_elm_combobox_efl_ui_l10n_translation_update(Eo *obj EINA_UNUSED, Elm_Combobox_Data *sd)
{
   efl_ui_l10n_translation_update(efl_super(obj, MY_CLASS));
   efl_ui_l10n_translation_update(sd->genlist);
   efl_ui_l10n_translation_update(sd->entry);

   if (sd->hover)
     efl_ui_l10n_translation_update(sd->hover);
}

/**
 * @brief Applies the theme to the combobox widget and its sub-objects.
 *
 * This function handles applying the style to the combobox itself,
 * the hover popup, the genlist, and the entry. It also manages
 * mirrored mode settings.
 *
 * @param obj The combobox object.
 * @param sd The private data of the combobox.
 * @return Eina_Error EFL_UI_THEME_APPLY_ERROR_GENERIC on failure, or the result of the superclass theme_apply.
 */
EOLIAN static Eina_Error
_elm_combobox_efl_ui_widget_theme_apply(Eo *obj, Elm_Combobox_Data *sd)
{
   const char *style;
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   Eina_Bool mirrored;
   char buf[128];

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EFL_UI_THEME_APPLY_ERROR_GENERIC);

   style = eina_stringshare_add(elm_widget_style_get(obj));

   snprintf(buf, sizeof(buf), "combobox_vertical/%s", style);

   /* combobox's style has no extra bit for orientation but could have... */
   eina_stringshare_replace(&(wd->style), buf);

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   eina_stringshare_replace(&(wd->style), style);

   mirrored = efl_ui_mirrored_get(obj);

   if (sd->hover)
     {
        efl_ui_mirrored_set(sd->hover, mirrored);
        elm_widget_style_set(sd->hover, buf);
     }

   efl_ui_mirrored_set(sd->genlist, mirrored);
   efl_ui_mirrored_set(sd->entry, mirrored);

   elm_widget_style_set(sd->genlist, buf);
   elm_widget_style_set(sd->entry, buf);

   eina_stringshare_del(style);

   return int_ret;
}

/**
 * @brief Callback invoked when the hover object is clicked.
 *
 * This function handles dismissing the hover if the "dismiss" layout data
 * is not set to "on". This provides backward compatibility.
 *
 * @param data The combobox object (passed as user data).
 * @param obj The hover Evas_Object that was clicked.
 * @param event_info Click event information (unused).
 */
static void
_on_hover_clicked(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   const char *dismissstr;

   dismissstr = elm_layout_data_get(obj, "dismiss");

   if (!dismissstr || strcmp(dismissstr, "on"))
     elm_combobox_hover_end(data); // for backward compatibility
}

/**
 * @brief Callback invoked when the hover's "hide,finished" signal is emitted.
 *
 * This function checks if the hover was dismissed via a layout signal
 * (e.g., "elm,action,hide,no_animate"). If so, it updates the expanded state
 * and emits the "dismissed" event.
 *
 * @param data The combobox object (passed as user data).
 * @param obj The hover Evas_Object (unused).
 * @param emission The emitted signal name (unused).
 * @param source The source of the signal (unused).
 */
static void
_hover_end_finished(void *data,
                    Evas_Object *obj EINA_UNUSED,
                    const char *emission EINA_UNUSED,
                    const char *source EINA_UNUSED)
{
   const char *dismissstr;
   ELM_COMBOBOX_DATA_GET(data, sd);
   dismissstr = elm_layout_data_get(sd->hover, "dismiss");
   if (dismissstr && !strcmp(dismissstr, "on"))
     {
        sd->expanded = EINA_FALSE;
        evas_object_hide(sd->hover);
        efl_event_callback_legacy_call(data, ELM_COMBOBOX_EVENT_DISMISSED, NULL);
     }
}

/**
 * @brief Counts the number of items currently visible in the genlist after filtering.
 *
 * Updates the `count` field in the combobox's private data.
 *
 * @param data The combobox object.
 */
static void
count_items_genlist(void *data)
{
   ELM_COMBOBOX_DATA_GET(data, sd);
   Eina_Iterator *filter_iter;
   int count = 0;
   Elm_Object_Item *item;

   filter_iter = elm_genlist_filter_iterator_new(sd->genlist);
   if (!filter_iter) return;
   EINA_ITERATOR_FOREACH(filter_iter, item)
     if (item) count++;
   sd->count = count;
   eina_iterator_free(filter_iter);
}

/**
 * @brief Callback invoked when a genlist item is realized.
 *
 * Triggers a resize of the combobox's internal table to accommodate the item.
 *
 * @param data The combobox object (passed as user data).
 * @param obj The genlist Evas_Object (unused).
 * @param event_info Realization event information (unused).
 */
static void
_item_realized(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _table_resize(data);
}

/**
 * @brief Resizes the combobox's internal table layout.
 *
 * Calculates the appropriate height for the hover popup based on the number of items,
 * individual item height, and available space within the hover parent.
 * It sets the minimum size of a spacer object within the table to control the
 * genlist's visible area.
 *
 * @param data The combobox object.
 */
static void
_table_resize(void *data)
{
   ELM_COMBOBOX_DATA_GET(data, sd);
   if (sd->count > 0)
     {
        int hover_parent_w, hover_parent_h, obj_h, obj_w, obj_y, win_y_offset;
        int current_height, h;
        Eina_List *realized;

        sd->item = elm_genlist_first_item_get(sd->genlist);

        if (!(realized = elm_genlist_realized_items_get(sd->genlist)))
          {
             //nothing realized and wait until at least one item is realized
             h = 1;
             evas_object_smart_callback_add(sd->genlist, "realized", _item_realized, data);
          }
        else
          {
             // take the first, and update according to that
             evas_object_geometry_get(elm_object_item_track(eina_list_data_get(realized)), NULL, NULL,
                                      NULL, &h);
             eina_list_free(realized);
          }

        sd->item_height = h;
        evas_object_geometry_get(sd->entry, NULL, NULL, &obj_w, NULL);
        evas_object_geometry_get(data, NULL, &obj_y, NULL, &obj_h);
        evas_object_geometry_get(sd->hover_parent, NULL, NULL, &hover_parent_w,
                                 &hover_parent_h);
        current_height = sd->item_height * sd->count;
        sd->best_location = elm_hover_best_content_location_get(sd->hover,
                                                       ELM_HOVER_AXIS_VERTICAL);
        if (sd->best_location && !strcmp(sd->best_location , "bottom"))
          win_y_offset = hover_parent_h - obj_y - obj_h;
        else win_y_offset = obj_y;

        if (current_height < win_y_offset)
          evas_object_size_hint_min_set(sd->spacer, obj_w, current_height);
        else evas_object_size_hint_min_set(sd->spacer, obj_w, win_y_offset);
     }
}

/**
 * @brief Activates or expands the combobox.
 *
 * If the combobox is already expanded, it calls `elm_combobox_hover_end` to dismiss it.
 * Otherwise, it sets the expanded state, counts items, resizes the table,
 * sets up the hover content, shows the genlist and hover, and emits the "expanded" event.
 *
 * @param obj The combobox object.
 */
static void
_activate(Evas_Object *obj)
{
   ELM_COMBOBOX_DATA_GET(obj, sd);
   if (elm_widget_disabled_get(obj)) return;

   if (sd->expanded)
     {
        elm_combobox_hover_end(obj);
        return;
     }

   sd->expanded = EINA_TRUE;

   count_items_genlist(obj);

   if (sd->count <= 0) return;

   _table_resize(obj);
   elm_object_part_content_set(sd->hover, sd->best_location, sd->tbl);
   evas_object_show(sd->genlist);
   elm_genlist_item_selected_set(sd->item, EINA_TRUE);
   evas_object_show(sd->hover);
   efl_event_callback_legacy_call(obj, ELM_COMBOBOX_EVENT_EXPANDED, NULL);
}

/**
 * @brief Callback invoked when an item in the genlist is selected.
 *
 * Sets focus to the entry part of the combobox and emits the "item,selected" event.
 *
 * @param data The combobox object (passed as user data).
 * @param obj The genlist Evas_Object (unused).
 * @param event The selected Elm_Object_Item.
 */
static void
_on_item_selected(void *data , Evas_Object *obj EINA_UNUSED, void *event)
{
   ELM_COMBOBOX_DATA_GET(data, sd);
   elm_object_focus_set(sd->entry, EINA_TRUE);

   efl_event_callback_legacy_call(data, ELM_COMBOBOX_EVENT_ITEM_SELECTED, event);
}

/**
 * @brief Callback invoked when an item in the genlist is pressed.
 *
 * Emits the "item,pressed" event.
 *
 * @param data The combobox object (passed as user data).
 * @param obj The genlist Evas_Object (unused).
 * @param event The pressed Elm_Object_Item.
 */
static void
_on_item_pressed(void *data , Evas_Object *obj EINA_UNUSED, void *event)
{
   efl_event_callback_legacy_call(data, ELM_COMBOBOX_EVENT_ITEM_PRESSED, event);
}

/**
 * @brief Callback invoked when the genlist filtering process is finished.
 *
 * Updates the item count. If it's not the first filter operation, it emits
 * the "filter,done" event. If items remain, it activates or resizes the
 * combobox and selects the first item. If no items remain, it hides the hover.
 *
 * @param data The combobox object (passed as user data).
 * @param event The Efl_Event data associated with the filter finishing.
 */
static void
_gl_filter_finished_cb(void *data, const Efl_Event *event)
{
   char buf[1024];
   ELM_COMBOBOX_DATA_GET(data, sd);

   count_items_genlist(data);

   if (sd->first_filter)
     {
        sd->first_filter = EINA_FALSE;
        return;
     }

   efl_event_callback_legacy_call(data, ELM_COMBOBOX_EVENT_FILTER_DONE, event->info);

   if (sd->count > 0)
     {
        if (!sd->expanded) _activate(data);
        else _table_resize(data);
        elm_genlist_item_selected_set(sd->item, EINA_TRUE);
     }
   else
     {
        sd->expanded = EINA_FALSE;
        elm_layout_signal_emit(sd->hover, "elm,action,hide,no_animate", "elm");
        snprintf(buf, sizeof(buf), "elm,action,slot,%s,hide", sd->best_location);
        elm_layout_signal_emit(sd->hover, buf, "elm");
        edje_object_message_signal_process(elm_layout_edje_get(sd->hover));
     }
}

/**
 * @brief Callback invoked when the entry's "aborted" event occurs (e.g., Escape key pressed).
 *
 * If the combobox is expanded, it dismisses the hover.
 *
 * @param data The combobox object (passed as user data).
 * @param event The Efl_Event data (unused).
 */
static void
_on_aborted(void *data, const Efl_Event *event EINA_UNUSED)
{
   ELM_COMBOBOX_DATA_GET(data, sd);
   if (sd->expanded) elm_combobox_hover_end(data);
}

/**
 * @brief Callback invoked when the entry's content has changed by user interaction.
 *
 * Emits the ELM_ENTRY_EVENT_CHANGED event (legacy).
 *
 * @param data The combobox object (passed as user data).
 * @param event The Efl_Event data (unused).
 */
static void
_on_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   efl_event_callback_legacy_call(data, ELM_ENTRY_EVENT_CHANGED, NULL);
}

/**
 * @brief Callback invoked when the combobox button itself is clicked.
 *
 * Begins the hover process to show the list of items.
 *
 * @param data The combobox object (passed as user data).
 * @param obj The combobox Evas_Object that was clicked (unused).
 * @param event_info Click event information (unused).
 */
static void
_on_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_combobox_hover_begin(data);
}

/**
 * @brief Helper function to create and add a sub-component for the combobox.
 *
 * This function abstracts the creation of legacy or EFL-style widgets
 * based on whether the main combobox object is legacy.
 *
 * @param obj The main combobox object (used to check if it's legacy).
 * @param parent The parent Evas_Object for the new component.
 * @param klass The Efl_Class of the component to create.
 * @param style The style to apply to the new component.
 * @return The newly created component Eo object, or NULL on failure.
 */
static Eo *
_elm_combobox_component_add(Eo *obj, Eo *parent, const Efl_Class *klass, char *style)
{
   Eo *component;

   if (elm_widget_is_legacy(obj))
     {
        component = elm_legacy_add(klass, parent,
                       efl_ui_widget_style_set(efl_added, style));
     }
   else
     {
        component = efl_add(klass, parent,
                       efl_ui_widget_style_set(efl_added, style));
     }

   return component;
}

EOLIAN static void
_elm_combobox_efl_canvas_group_group_add(Eo *obj, Elm_Combobox_Data *sd EINA_UNUSED)
{
   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   efl_ui_mirrored_automatic_set(obj, EINA_FALSE);

   evas_object_smart_callback_add(obj, "clicked", _on_clicked, obj);

   // Apply the theme during the add operation to ensure the widget is styled correctly.
   efl_ui_widget_theme_apply(obj);
}

/**
 * @brief Handles the deletion of the combobox canvas group.
 *
 * Cleans up resources, specifically nullifying the hover_parent reference.
 *
 * @param obj The combobox object.
 * @param sd The private data of the combobox.
 */
EOLIAN static void
_elm_combobox_efl_canvas_group_group_del(Eo *obj, Elm_Combobox_Data *sd)
{
   sd->hover_parent = NULL;
   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @brief Sets the visibility of the combobox.
 *
 * Also manages the visibility of the hover popup if the combobox is expanded.
 *
 * @param obj The combobox object.
 * @param sd The private data of the combobox.
 * @param vis EINA_TRUE to show, EINA_FALSE to hide.
 */
EOLIAN static void
_elm_combobox_efl_gfx_entity_visible_set(Eo *obj, Elm_Combobox_Data *sd, Eina_Bool vis)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_VISIBLE, 0, vis))
     return;

   efl_gfx_entity_visible_set(efl_super(obj, MY_CLASS), vis);
   if (vis)
     {
        if (sd->expanded) evas_object_show(sd->hover);
     }
   else
     {
        if (sd->hover) evas_object_hide(sd->hover);
     }
}

EOLIAN static void
_elm_combobox_efl_ui_autorepeat_autorepeat_enabled_set(const Eo *obj EINA_UNUSED,
                                                       Elm_Combobox_Data *sd EINA_UNUSED,
                                                       Eina_Bool enabled)
{
   if (enabled)
     ERR("You cannot enable autorepeat on this object");
   efl_ui_autorepeat_enabled_set(efl_super(obj, MY_CLASS), EINA_FALSE);
}

/**
 * @brief Adds a new combobox widget to the given parent Evas_Object.
 *
 * @param parent The parent object.
 * @return The new object or NULL on error.
 *
 * @ingroup Elm_Combobox
 */
EAPI Evas_Object *
elm_combobox_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

EOLIAN static Eo *
_elm_combobox_efl_object_constructor(Eo *obj, Elm_Combobox_Data *sd)
{
   Evas_Object *gl;
   Evas_Object *entry;
   char buf[128];

   obj = efl_constructor(efl_super(obj, MY_CLASS));

   sd->first_filter = EINA_TRUE;

   efl_ui_autorepeat_enabled_set(obj, EINA_FALSE);
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_GLASS_PANE);

   //hover-parent
   sd->hover_parent = elm_object_top_widget_get(obj);

   snprintf(buf, sizeof(buf), "combobox_vertical/%s", elm_widget_style_get(obj));

   //hover
   sd->hover = _elm_combobox_component_add(obj, sd->hover_parent, ELM_HOVER_CLASS, buf);
   efl_gfx_entity_visible_set(sd->hover, EINA_FALSE);
   evas_object_layer_set(sd->hover, EVAS_LAYER_MAX);
   efl_ui_mirrored_automatic_set(sd->hover, EINA_FALSE);
   elm_hover_target_set(sd->hover, obj);
   elm_widget_sub_object_add(obj, sd->hover);

   evas_object_smart_callback_add(sd->hover, "clicked", _on_hover_clicked, obj);
   elm_layout_signal_callback_add
     (sd->hover, "elm,action,hide,finished", "elm", _hover_end_finished, obj);

   //table
   sd->tbl = elm_table_add(obj);
   evas_object_size_hint_weight_set(sd->tbl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(sd->tbl, EVAS_HINT_FILL, EVAS_HINT_FILL);

   //spacer
   sd->spacer = evas_object_rectangle_add(evas_object_evas_get(sd->hover_parent));
   evas_object_size_hint_weight_set(sd->spacer, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(sd->spacer, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_color_set(sd->spacer, 0, 0, 0, 0);
   elm_table_pack(sd->tbl, sd->spacer, 0, 0, 1, 1);

   // This is the genlist object that will take over the genlist call
   sd->genlist = gl = _elm_combobox_component_add(obj, obj, ELM_GENLIST_CLASS, buf);
   elm_genlist_filter_set(gl, NULL);
   efl_ui_mirrored_automatic_set(gl, EINA_FALSE);
   efl_ui_mirrored_set(gl, efl_ui_mirrored_get(obj));
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(gl, "selected", _on_item_selected, obj);
   evas_object_smart_callback_add(gl, "pressed", _on_item_pressed, obj);
   efl_event_callback_add(gl, ELM_GENLIST_EVENT_FILTER_DONE, _gl_filter_finished_cb, obj);
   elm_genlist_homogeneous_set(gl, EINA_TRUE);
   elm_genlist_mode_set(gl, ELM_LIST_COMPRESS);
   elm_table_pack(sd->tbl, gl, 0, 0, 1, 1);

   // This is the entry object that will take over the entry call
   sd->entry = entry = _elm_combobox_component_add(obj, obj, ELM_ENTRY_CLASS, buf);
   efl_ui_mirrored_automatic_set(entry, EINA_FALSE);
   efl_ui_mirrored_set(entry, efl_ui_mirrored_get(obj));
   elm_scroller_policy_set(entry, ELM_SCROLLER_POLICY_OFF,
                           ELM_SCROLLER_POLICY_OFF);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_single_line_set(entry, EINA_TRUE);
   efl_event_callback_add(entry, ELM_ENTRY_EVENT_CHANGED_USER, _on_changed, obj);
   efl_event_callback_add(entry, ELM_ENTRY_EVENT_ABORTED, _on_aborted, obj);
   evas_object_show(entry);

   elm_object_part_content_set(obj, "elm.swallow.content", entry);

   efl_composite_attach(obj, gl);
   efl_composite_attach(obj, entry);

   return obj;
}

/**
 * @brief Initiates the display of the combobox's item list (hover).
 *
 * Sets focus to the entry part and calls _activate to show the hover.
 *
 * @param obj The combobox object.
 * @param sd The private data of the combobox.
 */
EOLIAN static void
_elm_combobox_hover_begin(Eo *obj, Elm_Combobox_Data *sd)
{
   if (!sd->hover) return;
   elm_object_focus_set(sd->entry, EINA_TRUE);

   _activate(obj);
}

/**
 * @brief Ends the display of the combobox's item list (hover).
 *
 * If the hover has "dismiss" set to "on" in its layout data, it calls
 * elm_hover_dismiss(). Otherwise, it directly hides the hover and emits
 * the "dismissed" event (for backward compatibility).
 *
 * @param obj The combobox object.
 * @param sd The private data of the combobox.
 */
EOLIAN static void
_elm_combobox_hover_end(Eo *obj, Elm_Combobox_Data *sd)
{
   const char *dismissstr;
   if (!sd->hover) return;
   dismissstr = elm_layout_data_get(sd->hover, "dismiss");

   if (dismissstr && !strcmp(dismissstr, "on"))
     elm_hover_dismiss(sd->hover);
   else
     {
        sd->expanded = EINA_FALSE;
        evas_object_hide(sd->hover);
        efl_event_callback_legacy_call(obj, ELM_COMBOBOX_EVENT_DISMISSED, NULL);
     } // for backward compatibility
}

/**
 * @brief Gets whether the combobox is currently expanded (hover is visible).
 *
 * @param obj The combobox object (unused).
 * @param sd The private data of the combobox.
 * @return EINA_TRUE if expanded, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_combobox_expanded_get(const Eo *obj EINA_UNUSED, Elm_Combobox_Data *sd)
{
   return sd->expanded;
}

/**
 * @brief Handles key-based navigation within the combobox list.
 *
 * Moves the selection up or down in the genlist based on the `params` string.
 *
 * @param obj The combobox object.
 * @param params Direction of movement: "up" or "down".
 * @return EINA_TRUE if the action was handled, EINA_FALSE otherwise.
 */
static Eina_Bool
_key_action_move(Evas_Object *obj, const char *params)
{
   ELM_COMBOBOX_DATA_GET(obj, sd);
   Elm_Object_Item *it = NULL;
   const char *dir = params;
   if (!sd->hover) return EINA_FALSE;

   if (!strcmp(dir, "up"))
     {
        it = sd->item;
        it = elm_genlist_item_prev_get(it);
        if (!it) sd->item = elm_genlist_last_item_get(sd->genlist);
        else sd->item = it;
        elm_genlist_item_selected_set(sd->item, EINA_TRUE);
     }
   else if (!strcmp(dir, "down"))
     {
        it = sd->item;
        it = elm_genlist_item_next_get(it);
        if (!it) sd->item = elm_genlist_first_item_get(sd->genlist);
        else sd->item = it;
        elm_genlist_item_selected_set(sd->item, EINA_TRUE);
     }
   else return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @brief Handles key-based activation of the combobox or its items.
 *
 * If the combobox is not expanded, it calls `elm_combobox_hover_begin()` to expand it.
 * If already expanded, it simulates a "pressed" event on the currently selected item
 * in the genlist and moves the cursor to the end of the entry.
 *
 * @param obj The combobox object.
 * @param params Action parameters (unused).
 * @return EINA_TRUE if the action was handled, EINA_FALSE otherwise.
 */
static Eina_Bool
_key_action_activate(Evas_Object *obj, const char *params EINA_UNUSED)
{
   ELM_COMBOBOX_DATA_GET(obj, sd);
   if (!sd->hover) return EINA_FALSE;
   if (!sd->expanded)
     elm_combobox_hover_begin(obj);
   else
     {
        evas_object_smart_callback_call(sd->genlist, "pressed", sd->item);
        elm_entry_cursor_end_set(sd->entry);
     }
   return EINA_TRUE;
}

/**
 * @brief Class constructor for Elm_Combobox.
 *
 * Registers the legacy smart type for the combobox.
 *
 * @param klass The Efl_Class being constructed.
 */
static void
_elm_combobox_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @brief Provides accessibility actions for the combobox widget.
 *
 * @param obj The combobox object (unused).
 * @param pd The private data of the combobox (unused).
 * @return A static array of Efl_Access_Action_Data describing available actions.
 *         The array defines actions like "activate", "move,up", and "move,down",
 *         mapping them to their respective handler functions and parameters.
 *         Example Efl_Access_Action_Data element:
 *         @code
 *         {
 *           "action_name_and_params", // e.g., "move,up"
 *           "action_name",            // e.g., "move"
 *           "param_string",           // e.g., "up"
 *           callback_function_pointer // e.g., _key_action_move
 *         }
 *         @endcode
 */
EOLIAN const Efl_Access_Action_Data *
_elm_combobox_efl_access_widget_action_elm_actions_get(const Eo *obj EINA_UNUSED,
                                                                Elm_Combobox_Data *pd
                                                                EINA_UNUSED)
{
   static Efl_Access_Action_Data atspi_actions[] = {
      {"activate", "activate", "return", _key_action_activate},
      {"move,up", "move", "up", _key_action_move},
      {"move,down", "move", "down", _key_action_move},
      {NULL, NULL, NULL, NULL}
   };
   return &atspi_actions[0];
}

/**
 * @brief Sets the filter key for the internal genlist.
 *
 * @param obj The combobox object (unused).
 * @param pd The private data of the combobox.
 * @param key The filter key to be used by the genlist.
 */
EOLIAN void
_elm_combobox_elm_genlist_filter_set(Eo *obj EINA_UNUSED, Elm_Combobox_Data *pd, void *key)
{
   pd->first_filter = EINA_FALSE;

   elm_obj_genlist_filter_set(pd->genlist, key);
}

// FIXME: Combobox part API is badly defined. Efl.Part should be reimplemented
// properly, but this will be tricky: how to set "guide" on the "entry" part?
/**
 * @brief Sets the text for a specific part of the combobox's entry.
 * @deprecated This is a legacy function. Use EFL Part API where possible.
 *
 * This function acts as a proxy to `elm_object_part_text_set` on the internal entry widget.
 *
 * @param obj The combobox object.
 * @param part The name of the part to set text on (e.g., "guide").
 * @param label The text to set.
 */
void
_elm_combobox_part_text_set(Eo *obj, const char * part, const char *label)
{
   Elm_Combobox_Data *pd = efl_data_scope_safe_get(obj, MY_CLASS);
   if (!pd) return;
   elm_object_part_text_set(pd->entry, part, label);
}

/**
 * @brief Gets the text from a specific part of the combobox's entry.
 * @deprecated This is a legacy function. Use EFL Part API where possible.
 *
 * This function acts as a proxy to `elm_object_part_text_get` on the internal entry widget.
 *
 * @param obj The combobox object.
 * @param part The name of the part to get text from.
 * @return The text of the part, or NULL if the part does not exist or an error occurs.
 */
const char *
_elm_combobox_part_text_get(const Eo *obj, const char *part)
{
   Elm_Combobox_Data *pd = efl_data_scope_safe_get(obj, MY_CLASS);
   if (!pd) return NULL;
   return elm_object_part_text_get(pd->entry, part);
}

/**
 * @brief Sets the size of the combobox.
 *
 * If the combobox has items, it triggers a resize of the internal table.
 *
 * @param obj The combobox object.
 * @param pd The private data of the combobox.
 * @param sz The new size (width and height).
 */
EOLIAN static void
_elm_combobox_efl_gfx_entity_size_set(Eo *obj, Elm_Combobox_Data *pd, Eina_Size2D sz)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_RESIZE, 0, sz.w, sz.h))
     return;

   if (pd->count > 0) _table_resize(obj);
   efl_gfx_entity_size_set(efl_super(obj, MY_CLASS), sz);
}

/* Internal EO APIs and hidden overrides */

ELM_WIDGET_KEY_DOWN_DEFAULT_IMPLEMENT(elm_combobox, Elm_Combobox_Data)

EOAPI EFL_FUNC_BODY_CONST(elm_obj_combobox_expanded_get, Eina_Bool, 0);
EOAPI EFL_VOID_FUNC_BODY(elm_obj_combobox_hover_begin);
EOAPI EFL_VOID_FUNC_BODY(elm_obj_combobox_hover_end);

/**
 * @brief Class initializer for Elm_Combobox.
 *
 * Sets up the Eolian operations for the combobox class.
 *
 * @param klass The Efl_Class being initialized.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_combobox_class_initializer(Efl_Class *klass)
{
   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_combobox_expanded_get, _elm_combobox_expanded_get),
      EFL_OBJECT_OP_FUNC(elm_obj_combobox_hover_begin, _elm_combobox_hover_begin),
      EFL_OBJECT_OP_FUNC(elm_obj_combobox_hover_end, _elm_combobox_hover_end),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_combobox_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_visible_set, _elm_combobox_efl_gfx_entity_visible_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_combobox_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_combobox_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_l10n_translation_update, _elm_combobox_efl_ui_l10n_translation_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_combobox_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(efl_ui_autorepeat_enabled_set, _elm_combobox_efl_ui_autorepeat_autorepeat_enabled_set),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_filter_set, _elm_combobox_elm_genlist_filter_set),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_combobox_efl_access_widget_action_elm_actions_get),
      EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_combobox)
   );

   return efl_class_functions_set(klass, &ops, NULL);
}

static const Efl_Class_Description _elm_combobox_class_desc = {
   EO_VERSION,
   "Elm.Combobox",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Combobox_Data),
   _elm_combobox_class_initializer,
   _elm_combobox_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_combobox_class_get, &_elm_combobox_class_desc, EFL_UI_BUTTON_CLASS, EFL_ACCESS_WIDGET_ACTION_MIXIN, ELM_ENTRY_CLASS, ELM_GENLIST_CLASS, ELM_HOVER_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);

/**
 * @brief Get whether the combobox is currently expanded (hover is visible).
 *
 * @param obj The combobox object.
 * @return @c EINA_TRUE if expanded, @c EINA_FALSE otherwise.
 * @ingroup Elm_Combobox
 */
EAPI Eina_Bool
elm_combobox_expanded_get(const Elm_Combobox *obj)
{
   return elm_obj_combobox_expanded_get(obj);
}

/**
 * @brief Programmatically begin the combobox hover.
 *
 * This will show the hover and list of items.
 *
 * @param obj The combobox object.
 * @ingroup Elm_Combobox
 */
EAPI void
elm_combobox_hover_begin(Elm_Combobox *obj)
{
   elm_obj_combobox_hover_begin(obj);
}

/**
 * @brief Programmatically end the combobox hover.
 *
 * This will hide the hover and list of items.
 *
 * @param obj The combobox object.
 * @ingroup Elm_Combobox
 */
EAPI void
elm_combobox_hover_end(Elm_Combobox *obj)
{
   elm_obj_combobox_hover_end(obj);
}
