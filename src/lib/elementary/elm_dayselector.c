#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_UI_L10N_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_dayselector.h"
#include "elm_widget_container.h"

#include "elm_dayselector_part.eo.h"
#include "elm_part_helper.h"

#define MY_CLASS ELM_DAYSELECTOR_CLASS

#define MY_CLASS_NAME "Elm_Dayselector"
#define MY_CLASS_NAME_LEGACY "elm_dayselector"

/* signals to edc */
#define ITEM_TYPE_WEEKDAY_DEFAULT "elm,type,weekday,default"
#define ITEM_TYPE_WEEKDAY_STYLE1  "elm,type,weekday,style1"
#define ITEM_TYPE_WEEKEND_DEFAULT "elm,type,weekend,default"
#define ITEM_TYPE_WEEKEND_STYLE1  "elm,type,weekend,style1"
#define ITEM_POS_LEFT             "elm,pos,check,left"
#define ITEM_POS_RIGHT            "elm,pos,check,right"
#define ITEM_POS_MIDDLE           "elm,pos,check,middle"

static const char SIG_CHANGED[] = "dayselector,changed";
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CHANGED, ""},
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {NULL, NULL}
};

/**
 * @internal
 * @brief Callback function for the resize event of the dayselector object.
 *
 * This function is called when the dayselector object is resized. It triggers
 * a re-evaluation of the layout sizing.
 *
 * @param data The Evas_Object (dayselector) that was resized.
 * @param e Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
static void
_dayselector_resize(void *data,
                    Evas *e EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   elm_layout_sizing_eval(data);
}

/**
 * @internal
 * @brief Updates the displayed day names based on the current locale.
 *
 * This function is called when the language changes. It iterates through
 * the dayselector items and updates their text to the abbreviated day name
 * in the current locale (e.g., "Mon", "Tue"). This is skipped if custom
 * weekday names have been set by the application.
 *
 * @param obj The dayselector Evas_Object.
 * @param sd The dayselector's private data.
 */
EOLIAN static void
_elm_dayselector_efl_ui_l10n_translation_update(Eo *obj EINA_UNUSED, Elm_Dayselector_Data *sd)
{
   time_t t;
   Eina_List *l;
   char buf[1024];
   struct tm time_daysel;
   Elm_Dayselector_Item_Data *it;

   if (sd->weekdays_names_set)
     return;

   t = time(NULL);
   localtime_r(&t, &time_daysel);
   EINA_LIST_FOREACH(sd->items, l, it)
     {
        time_daysel.tm_wday = it->day;
        strftime(buf, sizeof(buf), "%a", &time_daysel);
        elm_object_text_set(VIEW(it), buf);
     }

   efl_ui_l10n_translation_update(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Updates the visual style and position signals for all day items.
 *
 * This function iterates through each day item in the dayselector and
 * applies the appropriate style (weekday/weekend) and position signals
 * (left/middle/right) based on the current configuration (week start,
 * weekend settings, and RTL mode).
 *
 * @param obj The dayselector Evas_Object.
 */
static void
_update_items(Evas_Object *obj)
{
   Eina_List *l;
   Eina_Bool rtl;
   unsigned int last_day;
   Elm_Dayselector_Item_Data *it;

   ELM_DAYSELECTOR_DATA_GET(obj, sd);

   last_day = sd->week_start + ELM_DAYSELECTOR_MAX - 1;
   if (last_day >= ELM_DAYSELECTOR_MAX)
     last_day = last_day % ELM_DAYSELECTOR_MAX;

   rtl = efl_ui_mirrored_get(obj);
   EINA_LIST_FOREACH(sd->items, l, it)
     {
        elm_object_signal_emit(VIEW(it), it->day_style, ""); // XXX: compat
        elm_object_signal_emit(VIEW(it), it->day_style, "elm");
        if (it->day == sd->week_start)
          {
             if (rtl) elm_object_signal_emit(VIEW(it), ITEM_POS_RIGHT, "elm");
             else elm_object_signal_emit(VIEW(it), ITEM_POS_LEFT, "elm");
          }
        else if (it->day == last_day)
          {
             if (rtl) elm_object_signal_emit(VIEW(it), ITEM_POS_LEFT, "elm");
             else elm_object_signal_emit(VIEW(it), ITEM_POS_RIGHT, "elm");
          }
        else
          elm_object_signal_emit(VIEW(it), ITEM_POS_MIDDLE, "elm");
     }
}

/**
 * @internal
 * @brief Calculates the display location (index) of a day item.
 *
 * This function determines the visual position of a day item within the
 * dayselector's layout, considering the configured start day of the week.
 * For example, if the week starts on Monday (1) and the item is Sunday (0),
 * its location will be 6.
 *
 * @param sd The dayselector's private data.
 * @param it The dayselector item data.
 * @return The calculated display location (0-6).
 */
static inline unsigned int
_item_location_get(Elm_Dayselector_Data *sd,
                   Elm_Dayselector_Item_Data *it)
{
   return (ELM_DAYSELECTOR_MAX - sd->week_start + it->day) %
          ELM_DAYSELECTOR_MAX;
}

/**
 * @internal
 * @brief Applies the current theme to the dayselector and its items.
 *
 * This function is called when the widget's theme needs to be updated.
 * It applies the theme to the base layout and then iterates through each
 * day item, setting its style and emitting visibility signals.
 *
 * @param obj The dayselector Evas_Object.
 * @param sd The dayselector's private data.
 * @return Eina_Error indicating success or failure.
 */
EOLIAN static Eina_Error
_elm_dayselector_efl_ui_widget_theme_apply(Eo *obj, Elm_Dayselector_Data *sd)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;

   Eina_List *l;
   char buf[1024];
   Elm_Dayselector_Item_Data *it;

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        snprintf
          (buf, sizeof(buf), "dayselector/%s", elm_object_style_get(obj));
        elm_object_style_set(VIEW(it), buf);

        /* XXX kept for legacy compatibility, remove eventually */
        snprintf
          (buf, sizeof(buf), "day%d,visible", _item_location_get(sd, it));
        elm_layout_signal_emit(obj, buf, "elm");
        /* XXX */
        snprintf
          (buf, sizeof(buf), "elm,day%d,visible", _item_location_get(sd, it));
        elm_layout_signal_emit(obj, buf, "elm");
     }

   _update_items(obj);
   elm_layout_sizing_eval(obj);

   return int_ret;
}

/**
 * @internal
 * @brief Callback invoked when a dayselector item (check widget) is deleted.
 *
 * This function removes the corresponding item data from the dayselector's
 * internal list and emits signals to reset the item's visual state in the layout.
 *
 * @param data The dayselector Evas_Object (passed as user data).
 * @param e Unused.
 * @param obj The Evas_Object (check widget) that is being deleted.
 * @param event_info Unused.
 */
static void
_item_del_cb(void *data,
             Evas *e EINA_UNUSED,
             Evas_Object *obj,
             void *event_info EINA_UNUSED)
{
   Eina_List *l;
   char buf[1024];
   Elm_Dayselector_Item_Data *it;

   ELM_DAYSELECTOR_DATA_GET(data, sd);

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        if (obj == VIEW(it))
          {
             sd->items = eina_list_remove(sd->items, it);
             eina_stringshare_del(it->day_style);
             /* XXX kept for legacy compatibility, remove eventually */
             snprintf(buf, sizeof(buf), "day%d,default",
                      _item_location_get(sd, it));
             elm_layout_signal_emit(obj, buf, "elm");
             /* XXX */
             snprintf(buf, sizeof(buf), "elm,day%d,default",
                      _item_location_get(sd, it));
             elm_layout_signal_emit(obj, buf, "elm");

             // The object is already being deleted, there is no point in calling efl_del on it nore setting it to NULL.

             elm_layout_sizing_eval(obj);
             break;
          }
     }
}

/**
 * @internal
 * @brief Callback for signals emitted by individual day items (check widgets).
 *
 * This function is triggered when a day item emits a style-related signal
 * (e.g., "elm,type,weekday,default"). It updates the internal `day_style`
 * for the corresponding item.
 *
 * @param data The Elm_Dayselector_Item_Data for the item.
 * @param obj Unused.
 * @param emission The emitted signal string (e.g., "elm,type,weekday,default").
 * @param source Unused.
 */
static void
_item_signal_emit_cb(void *data,
                     Evas_Object *obj EINA_UNUSED,
                     const char *emission,
                     const char *source EINA_UNUSED)
{
   Elm_Dayselector_Item_Data *it = data;

   eina_stringshare_replace(&it->day_style, emission);
}

/**
 * @internal
 * @brief Callback invoked when a day item (check widget) is clicked.
 *
 * This function forwards the "changed" signal from the individual check item
 * to the parent dayselector widget.
 *
 * @param data The Elm_Dayselector_Item_Data for the clicked item.
 * @param obj Unused.
 * @param event_info Event information from the click.
 */
static void
_item_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Dayselector_Item_Data *it = data;

   evas_object_smart_callback_call(WIDGET(it), "changed", event_info);
}

/**
 * @internal
 * @brief Finds a dayselector item by its day enum value.
 *
 * Iterates through the internal list of items to find the one
 * corresponding to the given Elm_Dayselector_Day.
 *
 * @param obj The dayselector Evas_Object.
 * @param day The Elm_Dayselector_Day to find (e.g., ELM_DAYSELECTOR_SUNDAY).
 * @return Pointer to Elm_Dayselector_Item_Data if found, otherwise NULL.
 */
static Elm_Dayselector_Item_Data *
_item_find(const Evas_Object *obj,
           Elm_Dayselector_Day day)
{
   Eina_List *l;
   Elm_Dayselector_Item_Data *it;

   ELM_DAYSELECTOR_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->items, l, it)
     if (day == it->day) return it;

   return NULL;
}

/**
 * @internal
 * @brief Sets the content of a specific day item part.
 *
 * This function allows replacing the check widget for a specific day.
 * The `item` string is expected to be in the format "dayN" (e.g., "day0" for Sunday).
 * If the content is NULL, the existing item is effectively removed.
 * If an item for the day doesn't exist, it's created.
 *
 * @param obj The dayselector Evas_Object.
 * @param sd The dayselector's private data.
 * @param item A string identifying the day part (e.g., "day0", "day1").
 * @param content The new Evas_Object (must be an EFL_UI_CHECK_CLASS) to set as content.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_elm_dayselector_content_set(Eo *obj, Elm_Dayselector_Data *sd, const char *item, Evas_Object *content)
{
   Eina_Bool int_ret = EINA_FALSE;

   int day;
   char buf[1024];
   Elm_Dayselector_Item_Data *it = NULL;

   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(content, EFL_UI_CHECK_CLASS), EINA_FALSE);
   if (!item) return EINA_FALSE;

   day = atoi(item + (strlen(item) - 1));
   if (day < 0 || day > ELM_DAYSELECTOR_MAX) return EINA_FALSE;

   it = _item_find(obj, day);
   if (it)
     {
        snprintf(buf, sizeof(buf), "elm.swallow.day%d", _item_location_get(sd, it));

        int_ret = efl_content_set(efl_part(efl_super(obj, MY_CLASS), buf), content);
        if (!int_ret)
          {
             /* XXX kept for legacy compatibility, remove eventually */
             snprintf(buf, sizeof(buf), "day%d", _item_location_get(sd, it));

             int_ret = efl_content_set(efl_part(efl_super(obj, MY_CLASS), buf), content);
             /* XXX */
          }
        if (!int_ret) return EINA_FALSE;

        if (!content) return EINA_TRUE; /* item deletion already handled */

        evas_object_del(VIEW(it));
        VIEW_SET(it, content);
     }
   else
     {
        Eo *eo_it = efl_add(ELM_DAYSELECTOR_ITEM_CLASS, obj);
        it = efl_data_scope_get(eo_it, ELM_DAYSELECTOR_ITEM_CLASS);
        it->day = day;

        snprintf(buf, sizeof(buf), "elm.swallow.day%d", _item_location_get(sd, it));

        int_ret = efl_content_set(efl_part(efl_super(obj, MY_CLASS), buf), content);
        if (!int_ret)
          {
             /* XXX kept for legacy compatibility, remove eventually */
             snprintf(buf, sizeof(buf), "day%d", _item_location_get(sd, it));

             int_ret = efl_content_set(efl_part(efl_super(obj, MY_CLASS), buf), content);
             /* XXX */
          }
        if (!int_ret)
          {
             efl_del(eo_it);
             return EINA_FALSE;
          }

        sd->items = eina_list_append(sd->items, it);
        VIEW_SET(it, content);
     }

   snprintf(buf, sizeof(buf), "elm,day%d,visible", _item_location_get(sd, it));
   elm_layout_signal_emit(obj, buf, "elm");
   /* XXX kept for legacy compatibility, remove eventually */
   snprintf(buf, sizeof(buf), "day%d,visible", _item_location_get(sd, it));
   elm_layout_signal_emit(obj, buf, "elm");

   evas_object_smart_callback_add(VIEW(it), "changed", _item_clicked_cb, it);
   evas_object_event_callback_add
     (VIEW(it), EVAS_CALLBACK_DEL, _item_del_cb, obj);

   elm_object_signal_callback_add
     (VIEW(it), ITEM_TYPE_WEEKDAY_DEFAULT, "*", _item_signal_emit_cb, it);
   elm_object_signal_callback_add
     (VIEW(it), ITEM_TYPE_WEEKDAY_STYLE1, "*", _item_signal_emit_cb, it);
   elm_object_signal_callback_add
     (VIEW(it), ITEM_TYPE_WEEKEND_DEFAULT, "*", _item_signal_emit_cb, it);
   elm_object_signal_callback_add
     (VIEW(it), ITEM_TYPE_WEEKEND_STYLE1, "*", _item_signal_emit_cb, it);

   elm_layout_sizing_eval(obj);
   _update_items(obj);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Constructor for an Elm_Dayselector_Item.
 *
 * Initializes a new dayselector item object.
 *
 * @param eo_item The Eolian object for the item.
 * @param item The private data structure for the item.
 * @return The constructed Eolian object.
 */
EOLIAN static Eo *
_elm_dayselector_item_efl_object_constructor(Eo *eo_item, Elm_Dayselector_Item_Data *item)
{
   eo_item = efl_constructor(efl_super(eo_item, ELM_DAYSELECTOR_ITEM_CLASS));
   item->base = efl_data_scope_get(eo_item, ELM_WIDGET_ITEM_CLASS);

   return eo_item;
}

/**
 * @internal
 * @brief Unsets (removes) the content of a specific day item part.
 *
 * This function removes and returns the check widget for a specific day.
 * The `item` string is expected to be in the format "dayN" (e.g., "day0" for Sunday).
 *
 * @param obj The dayselector Evas_Object.
 * @param sd The dayselector's private data.
 * @param item A string identifying the day part (e.g., "day0", "day1").
 * @return The Evas_Object that was unset (the check widget), or NULL if not found or on error.
 */
static Evas_Object*
_elm_dayselector_content_unset(Eo *obj, Elm_Dayselector_Data *sd, const char *item)
{
   int day;
   char buf[1024];
   Evas_Object *content;
   Elm_Dayselector_Item_Data *it = NULL;

   day = atoi(item + (strlen(item) - 1));
   if (day < 0 || day > ELM_DAYSELECTOR_MAX) return NULL;

   it = _item_find(obj, day);
   if (!it) return NULL;

   content = efl_content_unset(efl_part(efl_super(obj, MY_CLASS), buf));
   if (!content) return NULL;

   sd->items = eina_list_remove(sd->items, it);
   evas_object_smart_callback_del_full(content, "changed", _item_clicked_cb, it);
   evas_object_event_callback_del(content, EVAS_CALLBACK_DEL, _item_del_cb);

   elm_object_signal_callback_del
     (content, ITEM_TYPE_WEEKDAY_DEFAULT, "*", _item_signal_emit_cb);
   elm_object_signal_callback_del
     (content, ITEM_TYPE_WEEKDAY_STYLE1, "*", _item_signal_emit_cb);
   elm_object_signal_callback_del
     (content, ITEM_TYPE_WEEKEND_DEFAULT, "*", _item_signal_emit_cb);
   elm_object_signal_callback_del
     (content, ITEM_TYPE_WEEKEND_STYLE1, "*", _item_signal_emit_cb);

   snprintf(buf, sizeof(buf), "elm,day%d,default", _item_location_get(sd, it));
   elm_layout_signal_emit(obj, buf, "elm");
   /* XXX kept for legacy compatibility, remove eventually */
   snprintf(buf, sizeof(buf), "day%d,default", _item_location_get(sd, it));
   elm_layout_signal_emit(obj, buf, "elm");

   efl_del(EO_OBJ(it));

   elm_layout_sizing_eval(obj);

   return content;
}

/**
 * @internal
 * @brief Sets the default style (weekday/weekend) for all day items.
 *
 * This function iterates through all day items and applies either the
 * weekday or weekend default style based on the configured weekend start
 * day and length.
 *
 * @param obj The dayselector Evas_Object.
 */
static void
_items_style_set(Evas_Object *obj)
{
   Eina_List *l;
   Elm_Dayselector_Item_Data *it;
   unsigned int weekend_last;

   ELM_DAYSELECTOR_DATA_GET(obj, sd);

   weekend_last = sd->weekend_start + sd->weekend_len - 1;
   if (weekend_last >= ELM_DAYSELECTOR_MAX)
     weekend_last = weekend_last % ELM_DAYSELECTOR_MAX;

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        if (weekend_last >= sd->weekend_start)
          {
             if ((it->day >= sd->weekend_start) && (it->day <= weekend_last))
               eina_stringshare_replace(&it->day_style,
                                        ITEM_TYPE_WEEKEND_DEFAULT);
             else
               eina_stringshare_replace(&it->day_style,
                                        ITEM_TYPE_WEEKDAY_DEFAULT);
          }
        else
          {
             if ((it->day >= sd->weekend_start) || (it->day <= weekend_last))
               eina_stringshare_replace(&it->day_style,
                                        ITEM_TYPE_WEEKEND_DEFAULT);
             else
               eina_stringshare_replace(&it->day_style,
                                        ITEM_TYPE_WEEKDAY_DEFAULT);
          }
     }
}

/**
 * @internal
 * @brief Creates and initializes all the day items (check widgets).
 *
 * This function is called during the dayselector's construction. It creates
 * seven check widgets, one for each day of the week. It sets their default
 * style, localized text (abbreviated day name), and swallows them into the
 * appropriate parts of the dayselector's layout.
 *
 * @param obj The dayselector Evas_Object.
 */
static void
_items_create(Evas_Object *obj)
{
   time_t t;
   char buf[1024];
   unsigned int idx;
   struct tm time_daysel;

   t = time(NULL);
   localtime_r(&t, &time_daysel);

   for (idx = 0; idx < ELM_DAYSELECTOR_MAX; idx++)
     {
        Evas_Object *chk;

        chk = elm_check_add(obj);
        elm_object_style_set(chk, "dayselector/default");

        time_daysel.tm_wday = idx;
        strftime(buf, sizeof(buf), "%a", &time_daysel);
        elm_object_text_set(chk, buf);

        snprintf(buf, sizeof(buf), "elm.swallow.day%u", idx);
        if (!elm_layout_content_set(obj, buf, chk))
          {
             snprintf(buf, sizeof(buf), "day%u", idx);
             elm_layout_content_set(obj, buf, chk);
          }

        // XXX: ACCESS
        _elm_access_text_set(_elm_access_info_get(chk),
                 ELM_ACCESS_TYPE, E_("day selector item"));
     }

   _items_style_set(obj);
   _update_items(obj);
}

/**
 * @internal
 * @brief Efl.Canvas.Group group_add override for Elm_Dayselector.
 *
 * This function is called when the dayselector is added to a canvas.
 * It sets up the theme, initializes default values for week start,
 * weekend start, and weekend length from configuration, sets finger size
 * multiplier, creates the individual day items, and sets up a resize callback.
 *
 * @param obj The dayselector Eolian object.
 * @param priv The dayselector's private data.
 */
EOLIAN static void
_elm_dayselector_efl_canvas_group_group_add(Eo *obj, Elm_Dayselector_Data *priv)
{
   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   if (!elm_layout_theme_set(obj, "dayselector", "base",
                             elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   priv->week_start = _elm_config->week_start;
   priv->weekend_start = _elm_config->weekend_start;
   priv->weekend_len = _elm_config->weekend_len;
   efl_ui_layout_finger_size_multiplier_set(obj, ELM_DAYSELECTOR_MAX, 1);
   _items_create(obj);

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_RESIZE, _dayselector_resize, obj);

   elm_layout_sizing_eval(obj);
}

/**
 * @internal
 * @brief Efl.Canvas.Group group_del override for Elm_Dayselector.
 *
 * This function is called when the dayselector is being deleted.
 * It frees the list of dayselector items and their associated data.
 * The actual Evas_Objects for items are expected to be handled by their parent's
 * deletion or by content_unset.
 *
 * @param obj The dayselector Eolian object.
 * @param sd The dayselector's private data.
 */
EOLIAN static void
_elm_dayselector_efl_canvas_group_group_del(Eo *obj, Elm_Dayselector_Data *sd)
{
   Elm_Dayselector_Item_Data *it;

   EINA_LIST_FREE(sd->items, it)
     {
        sd->items = eina_list_remove(sd->items, it);
        eina_stringshare_del(it->day_style);
        // No need to efl_del the object as they have been created by efl_add and are dead by now.
     }

   /* handles freeing sd */
   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

EAPI Evas_Object *
elm_dayselector_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Efl.Object constructor override for Elm_Dayselector.
 *
 * Initializes the dayselector object, sets its legacy type name,
 * registers smart callbacks, and sets the accessibility role.
 *
 * @param obj The dayselector Eolian object.
 * @param _pd Unused.
 * @return The constructed Eolian object.
 */
EOLIAN static Eo *
_elm_dayselector_efl_object_constructor(Eo *obj, Elm_Dayselector_Data *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_PANEL);

   return obj;
}

/**
 * @internal
 * @brief Sets the selected state of a specific day.
 *
 * Finds the item corresponding to the given day and sets its check state.
 *
 * @param obj The dayselector Eolian object.
 * @param _pd Unused.
 * @param day The Elm_Dayselector_Day to modify (e.g., ELM_DAYSELECTOR_MONDAY).
 * @param selected EINA_TRUE to select the day, EINA_FALSE to deselect.
 */
EOLIAN static void
_elm_dayselector_day_selected_set(Eo *obj, Elm_Dayselector_Data *_pd EINA_UNUSED, Elm_Dayselector_Day day, Eina_Bool selected)
{
   Elm_Dayselector_Item_Data *it = _item_find(obj, day);
   if (!it)
     {
        ERR("Failed to find item");
        return;
     }
   elm_check_state_set(VIEW(it), selected);
}

/**
 * @internal
 * @brief Gets the selected state of a specific day.
 *
 * Finds the item corresponding to the given day and returns its check state.
 *
 * @param obj The dayselector Eolian object.
 * @param _pd Unused.
 * @param day The Elm_Dayselector_Day to query (e.g., ELM_DAYSELECTOR_TUESDAY).
 * @return EINA_TRUE if the day is selected, EINA_FALSE otherwise or on error.
 */
EOLIAN static Eina_Bool
_elm_dayselector_day_selected_get(const Eo *obj, Elm_Dayselector_Data *_pd EINA_UNUSED, Elm_Dayselector_Day day)
{
   Elm_Dayselector_Item_Data *it = _item_find(obj, day);
   if (!it)
     {
        ERR("Failed to find item");
        return EINA_FALSE;
     }
   return elm_check_state_get(VIEW(it));
}

/**
 * @internal
 * @brief Sets the first day of the week for the dayselector.
 *
 * Updates the internal week_start day and re-swallows the day items into
 * the layout parts according to the new starting day. Then updates item styles.
 *
 * @param obj The dayselector Eolian object.
 * @param sd The dayselector's private data.
 * @param day The Elm_Dayselector_Day to set as the start of the week.
 */
EOLIAN static void
_elm_dayselector_week_start_set(Eo *obj, Elm_Dayselector_Data *sd, Elm_Dayselector_Day day)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   Eina_List *l;
   char buf[1024];
   Elm_Dayselector_Item_Data *it;

   /* just shuffling items, so swalling them directly */
   sd->week_start = day;
   EINA_LIST_FOREACH(sd->items, l, it)
     {
        snprintf(buf, sizeof(buf), "elm.swallow.day%d", _item_location_get(sd, it));
        if (!edje_object_part_swallow(wd->resize_obj, buf, VIEW(it)))
          {
             snprintf(buf, sizeof(buf), "day%d", _item_location_get(sd, it));
             edje_object_part_swallow(wd->resize_obj, buf, VIEW(it));
          }
     }

   _update_items(obj);
}

/**
 * @internal
 * @brief Gets the first day of the week for the dayselector.
 *
 * @param obj Unused.
 * @param sd The dayselector's private data.
 * @return The Elm_Dayselector_Day that is the current start of the week.
 */
EOLIAN static Elm_Dayselector_Day
_elm_dayselector_week_start_get(const Eo *obj EINA_UNUSED, Elm_Dayselector_Data *sd)
{
   return sd->week_start;
}

/**
 * @internal
 * @brief Sets the starting day of the weekend.
 *
 * Updates the internal weekend_start day and then refreshes the styles of all items.
 *
 * @param obj The dayselector Eolian object.
 * @param sd The dayselector's private data.
 * @param day The Elm_Dayselector_Day to set as the start of the weekend.
 */
EOLIAN static void
_elm_dayselector_weekend_start_set(Eo *obj, Elm_Dayselector_Data *sd, Elm_Dayselector_Day day)
{
   sd->weekend_start = day;

   _items_style_set(obj);
   _update_items(obj);
}

/**
 * @internal
 * @brief Gets the starting day of the weekend.
 *
 * @param obj Unused.
 * @param sd The dayselector's private data.
 * @return The Elm_Dayselector_Day that is the current start of the weekend.
 */
EOLIAN static Elm_Dayselector_Day
_elm_dayselector_weekend_start_get(const Eo *obj EINA_UNUSED, Elm_Dayselector_Data *sd)
{
   return sd->weekend_start;
}

/**
 * @internal
 * @brief Sets the length of the weekend in days.
 *
 * Updates the internal weekend_len and then refreshes the styles of all items.
 *
 * @param obj The dayselector Eolian object.
 * @param sd The dayselector's private data.
 * @param length The duration of the weekend in days (e.g., 2 for Saturday and Sunday).
 */
EOLIAN static void
_elm_dayselector_weekend_length_set(Eo *obj, Elm_Dayselector_Data *sd, unsigned int length)
{
   sd->weekend_len = length;

   _items_style_set(obj);
   _update_items(obj);
}

/**
 * @internal
 * @brief Gets the length of the weekend in days.
 *
 * @param obj Unused.
 * @param sd The dayselector's private data.
 * @return The current length of the weekend in days.
 */
EOLIAN static unsigned int
_elm_dayselector_weekend_length_get(const Eo *obj EINA_UNUSED, Elm_Dayselector_Data *sd)
{
   return sd->weekend_len;
}

/**
 * @internal
 * @brief Sets custom names for the days of the week.
 *
 * Allows the application to provide an array of strings to be used as display
 * names for the days. If `weekdays` is NULL, the widget reverts to using
 * localized abbreviations.
 *
 * @param obj The dayselector Eolian object.
 * @param sd The dayselector's private data.
 * @param weekdays An array of 7 strings for the day names (Sunday to Saturday).
 *                 Example: `const char *days[] = {"Sun", "Mon", ..., "Sat"};`
 *                 Pass NULL to revert to default localized names.
 */
EOLIAN static void
_elm_dayselector_weekdays_names_set(Eo *obj, Elm_Dayselector_Data *sd, const char **weekdays)
{
   int idx;
   time_t now;
   struct tm time_daysel;
   Elm_Dayselector_Item_Data *it;
   char buf[1024];

   if (weekdays)
     sd->weekdays_names_set = EINA_TRUE;
   else
     {
        now = time(NULL);
        localtime_r(&now, &time_daysel);
        sd->weekdays_names_set = EINA_FALSE;
     }

   for (idx = 0; idx < ELM_DAYSELECTOR_MAX; idx++)
     {
        it = _item_find(obj, idx);
        if (!it) continue;

        if (sd->weekdays_names_set)
          elm_object_text_set(VIEW(it), weekdays[idx]);
        else
          {
             time_daysel.tm_wday = idx;
             strftime(buf, sizeof(buf), "%a", &time_daysel);
             elm_object_text_set(VIEW(it), buf);
          }
     }
}

/**
 * @internal
 * @brief Gets the current names used for the days of the week.
 *
 * Returns a list of strings representing the display names of the days.
 * The caller is responsible for freeing the list and its stringshare'd contents.
 *
 * @param obj The dayselector Eolian object.
 * @param sd Unused.
 * @return A new Eina_List containing 7 stringshared day names (Sunday to Saturday).
 *         Example list structure:
 *         `["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]` (if default US locale)
 *         or custom names if set.
 *         Returns NULL on error or if items are not found.
 */
EOLIAN static Eina_List *
_elm_dayselector_weekdays_names_get(const Eo *obj, Elm_Dayselector_Data *sd EINA_UNUSED)
{
   int idx;
   const char *weekday;
   Elm_Dayselector_Item_Data *it;
   Eina_List *weekdays = NULL;

   for (idx = 0; idx < ELM_DAYSELECTOR_MAX; idx++)
     {
        it = _item_find(obj, idx);
        if (!it) continue ;

        weekday = elm_object_text_get(VIEW(it));
        weekdays = eina_list_append(weekdays, eina_stringshare_add(weekday));
     }
   return weekdays;
}

/**
 * @internal
 * @brief Class constructor for Elm_Dayselector.
 *
 * Registers the legacy type name for the widget.
 *
 * @param klass The Efl_Class for Elm_Dayselector.
 */
static void
_elm_dayselector_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Efl.Part begin */

ELM_PART_OVERRIDE(elm_dayselector, ELM_DAYSELECTOR, Elm_Dayselector_Data)
ELM_PART_OVERRIDE_CONTENT_SET(elm_dayselector, ELM_DAYSELECTOR, Elm_Dayselector_Data)
ELM_PART_OVERRIDE_CONTENT_UNSET(elm_dayselector, ELM_DAYSELECTOR, Elm_Dayselector_Data)
#include "elm_dayselector_part.eo.c"

/* Efl.Part end */

/* Internal EO APIs and hidden overrides */

#define ELM_DAYSELECTOR_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_dayselector)

#include "elm_dayselector_eo.c"
#include "elm_dayselector_item_eo.c"

