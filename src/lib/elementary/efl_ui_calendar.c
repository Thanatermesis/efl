/**
 * @file
 * @brief This file contains the implementation of the Efl.Ui.Calendar widget.
 *
 * The Efl.Ui.Calendar widget provides a user interface for selecting dates.
 * It displays a month view and allows navigation between months and years.
 * It supports setting minimum and maximum selectable dates, customizing the
 * first day of the week, and provides accessibility features.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_FOCUS_COMPOSITION_PROTECTED
#define EFL_UI_FOCUS_OBJECT_PROTECTED
#define EFL_ACCESS_WIDGET_ACTION_PROTECTED
#define EFL_UI_FORMAT_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"
#include "efl_ui_calendar_private.h"
#include "efl_ui_calendar_item.eo.h"

#define MY_CLASS EFL_UI_CALENDAR_CLASS

#define MY_CLASS_NAME "Efl.Ui.Calendar"
#define MY_CLASS_PFX efl_ui_calendar

#define EFL_UI_CALENDAR_BUTTON_LEFT "efl.calendar.button.left"
#define EFL_UI_CALENDAR_BUTTON_RIGHT "efl.calendar.button.right"
#define EFL_UI_CALENDAR_BUTTON_YEAR_LEFT "efl.calendar.button_year.left"
#define EFL_UI_CALENDAR_BUTTON_YEAR_RIGHT "efl.calendar.button_year.right"

#define FIRST_INTERVAL 0.85
#define INTERVAL 0.2

static const char PART_NAME_DEC_BUTTON[] = "dec_button";
static const char PART_NAME_INC_BUTTON[] = "inc_button";

static const char SIG_CHANGED[] = "changed";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CHANGED, ""},
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_LAYOUT_FOCUSED, ""}, /**< handled by elm_layout */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< handled by elm_layout */
   {NULL, NULL}
};

static void
_inc_dec_btn_clicked_cb(void *data,
                        const Efl_Event *ev);

static void
_inc_dec_btn_repeated_cb(void *data,
                         const Efl_Event *ev);

static Eina_Bool _key_action_activate(Evas_Object *obj, const char *params);

static const Elm_Action key_actions[] = {
   {"activate", _key_action_activate},
   {NULL, NULL}
};

/* Should not be translated, it's used if we failed
 * getting from locale. */
/** @internal
 * @brief Default abbreviated day names.
 * Used as a fallback if locale-specific names cannot be retrieved.
 * For example: `_days_abbrev[0]` is "Sun".
 */
static const char *_days_abbrev[] =
{
   "Sun", "Mon", "Tue", "Wed",
   "Thu", "Fri", "Sat"
};

/** @internal
 * @brief Number of days in each month for common and leap years.
 * `_days_in_month[0]` is for common years, `_days_in_month[1]` for leap years.
 * For example: `_days_in_month[0][1]` is 28 (February in a common year).
 *              `_days_in_month[1][1]` is 29 (February in a leap year).
 */
static int _days_in_month[2][12] =
{
   {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}, // Common year
   {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}  // Leap year
};

static Eina_Bool _efl_ui_calendar_smart_focus_next_enable = EINA_FALSE;

/**
 * @internal
 * @brief Get the maximum number of days for a given month and year.
 *
 * This function calculates the number of days in a month, considering leap years.
 * The month is determined by `date->tm_mon + month_offset`.
 *
 * @param date A pointer to a `struct tm` representing the base date.
 * @param month_offset An integer offset to add to the month of `date`.
 *                     For example, 0 for the current month, -1 for the previous, 1 for the next.
 * @return The number of days in the specified month.
 */
static inline int
_maxdays_get(struct tm *date, int month_offset)
{
   int month, year;

   month = (date->tm_mon + month_offset) % 12;
   year = date->tm_year + 1900;

   if (month < 0) month += 12;

   return _days_in_month
          [((!(year % 4)) && ((!(year % 400)) || (year % 100)))][month];
}

/**
 * @internal
 * @brief Emits a signal to visually unselect a calendar item (day).
 *
 * @param obj The calendar widget object.
 * @param selected The index of the calendar item to unselect (0-41).
 */
static inline void
_unselect(Evas_Object *obj,
          int selected)
{
   char emission[32];

   snprintf(emission, sizeof(emission), "cit_%d,unselected", selected);
   elm_layout_signal_emit(obj, emission, "efl");
}

/**
 * @internal
 * @brief Emits a signal to visually select a calendar item (day) and updates focused item.
 *
 * @param obj The calendar widget object.
 * @param selected The index of the calendar item to select (0-41).
 */
static inline void
_select(Evas_Object *obj,
        int selected)
{
   char emission[32];

   EFL_UI_CALENDAR_DATA_GET(obj, sd);

   sd->focused_it = sd->selected_it = selected;
   snprintf(emission, sizeof(emission), "cit_%d,selected", selected);
   elm_layout_signal_emit(obj, emission, "efl");
}

/**
 * @internal
 * @brief Emits a signal to visually mark a calendar item as not being "today".
 *
 * @param sd Pointer to the private data of the calendar widget.
 */
static inline void
_not_today(Efl_Ui_Calendar_Data *sd)
{
   char emission[32];

   snprintf(emission, sizeof(emission), "cit_%d,not_today", sd->today_it);
   elm_layout_signal_emit(sd->obj, emission, "efl");
   sd->today_it = -1;
}

/**
 * @internal
 * @brief Emits a signal to visually mark a calendar item as "today".
 *
 * @param sd Pointer to the private data of the calendar widget.
 * @param it The index of the calendar item to mark as today (0-41).
 */
static inline void
_today(Efl_Ui_Calendar_Data *sd,
       int it)
{
   char emission[32];

   snprintf(emission, sizeof(emission), "cit_%d,today", it);
   elm_layout_signal_emit(sd->obj, emission, "efl");
   sd->today_it = it;
}

/**
 * @internal
 * @brief Emits a signal to visually enable a calendar item (day).
 *
 * @param sd Pointer to the private data of the calendar widget.
 * @param it The index of the calendar item to enable (0-41).
 */
static inline void
_enable(Efl_Ui_Calendar_Data *sd,
        int it)
{
   char emission[32];

   snprintf(emission, sizeof(emission), "cit_%d,enable", it);
   elm_layout_signal_emit(sd->obj, emission, "efl");
}

/**
 * @internal
 * @brief Emits a signal to visually disable a calendar item (day).
 *
 * @param sd Pointer to the private data of the calendar widget.
 * @param it The index of the calendar item to disable (0-41).
 */
static inline void
_disable(Efl_Ui_Calendar_Data *sd,
         int it)
{
   char emission[32];

   snprintf(emission, sizeof(emission), "cit_%d,disable", it);
   elm_layout_signal_emit(sd->obj, emission, "efl");
}

/**
 * @internal
 * @brief Sets the displayed month and year text in the calendar header.
 *
 * It formats the `sd->shown_date` using the widget's formatter and
 * updates the "month_text" layout part.
 *
 * @param sd Pointer to the private data of the calendar widget.
 */
static void
_set_month_year(Efl_Ui_Calendar_Data *sd)
{
   Eina_Strbuf *strbuf = eina_strbuf_new();
   Eina_Value val;

   sd->filling = EINA_TRUE;

   eina_value_setup(&val, EINA_VALUE_TYPE_TM);
   eina_value_set(&val, sd->shown_date);
   efl_ui_format_formatted_value_get(sd->obj, strbuf, val);
   elm_layout_text_set(sd->obj, "month_text", eina_strbuf_string_get(strbuf));
   eina_value_flush(&val);
   eina_strbuf_free(strbuf);

   sd->filling = EINA_FALSE;
}

/**
 * @internal
 * @brief Callback to provide accessibility information for a calendar item (day).
 *
 * @param data User data (unused).
 * @param obj The Evas_Object for which accessibility information is requested (a calendar day item).
 * @return A newly allocated string containing the accessibility information (e.g., "day 15").
 *         The caller is responsible for freeing this string.
 */
static char *
_access_info_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   char *ret;
   Eina_Strbuf *buf;
   buf = eina_strbuf_new();

   eina_strbuf_append_printf(buf, "day %s", elm_widget_access_info_get(obj));

   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @internal
 * @brief Registers accessibility objects for individual calendar day items.
 *
 * Iterates through the visible day cells (up to 42) and, for valid days
 * in the current month, registers an accessibility object.
 * It sets the type to "calendar item" and provides the day number as info.
 *
 * @param obj The calendar widget object.
 */
static void
_access_calendar_item_register(Evas_Object *obj)
{
   unsigned int maxdays, i;
   char day_s[13], pname[32];
   unsigned day = 0;
   Evas_Object *ao;

   EFL_UI_CALENDAR_DATA_GET(obj, sd);

   maxdays = _maxdays_get(&sd->shown_date, 0);
   for (i = 0; i < 42; i++)
     {
        if ((!day) && (i == sd->first_day_it)) day = 1;
        if ((day) && (day <= maxdays))
          {
             snprintf(pname, sizeof(pname), "efl.cit_%d.access", i);

             ao = _elm_access_edje_object_part_object_register
                        (obj, elm_layout_edje_get(obj), pname);
             _elm_access_text_set(_elm_access_info_get(ao),
                         ELM_ACCESS_TYPE, E_("calendar item"));
             _elm_access_callback_set(_elm_access_info_get(ao),
                           ELM_ACCESS_INFO, _access_info_cb, NULL);

             snprintf(day_s, sizeof(day_s), "%d", (int) (day++));
             elm_widget_access_info_set(ao, (const char*)day_s);
          }
        else
          {
             snprintf(pname, sizeof(pname), "efl.cit_%d.access", i);
             _elm_access_edje_object_part_object_unregister
               (obj, elm_layout_edje_get(obj), pname);
          }
     }
}

/**
 * @internal
 * @brief Registers accessibility objects for the calendar's month/year spinner controls.
 *
 * This includes the previous/next month buttons and the month/year display text.
 *
 * @param obj The calendar widget object.
 */
static void
_access_calendar_spinner_register(Evas_Object *obj)
{
   Evas_Object *po, *o;
   Elm_Access_Info *ai;
   EFL_UI_CALENDAR_DATA_GET(obj, sd);

   if (!sd->dec_btn_month)
     sd->dec_btn_month = _elm_access_edje_object_part_object_register
        (obj, elm_layout_edje_get(obj), "left_bt");
   ai = _elm_access_info_get(sd->dec_btn_month);
   _elm_access_text_set(ai, ELM_ACCESS_TYPE, E_("calendar decrement month button"));

   if (!sd->inc_btn_month)
     sd->inc_btn_month = _elm_access_edje_object_part_object_register
        (obj, elm_layout_edje_get(obj), "right_bt");
   ai = _elm_access_info_get(sd->inc_btn_month);
   _elm_access_text_set(ai, ELM_ACCESS_TYPE, E_("calendar increment month button"));

   sd->month_access = _elm_access_edje_object_part_object_register
                          (obj, elm_layout_edje_get(obj), "text_month");
   ai = _elm_access_info_get(sd->month_access);
   _elm_access_text_set(ai, ELM_ACCESS_TYPE, E_("calendar month"));

   o = elm_layout_edje_get(obj);
   edje_object_freeze(o);
   po = (Evas_Object *)edje_object_part_object_get(o, "month_text");
   edje_object_thaw(o);
   evas_object_pass_events_set(po, EINA_FALSE);
}

/**
 * @internal
 * @brief Registers all accessibility objects for the calendar.
 *
 * This function calls helper functions to register accessibility for
 * both the spinner controls and the individual day items.
 *
 * @param obj The calendar widget object.
 */
static void
_access_calendar_register(Evas_Object *obj)
{
   _access_calendar_spinner_register(obj);
   _access_calendar_item_register(obj);
}

/**
 * @internal
 * @brief Updates the list of composite elements for focus composition.
 *
 * This function gathers all focusable elements within the calendar (month/year controls,
 * and visible day items) and sets them for focus management.
 *
 * @param obj The calendar widget object.
 * @param sd Pointer to the private data of the calendar widget.
 */
static void
_flush_calendar_composite_elements(Evas_Object *obj, Efl_Ui_Calendar_Data *sd)
{
   Eina_List *items = NULL;
   int max_day = _maxdays_get(&sd->shown_date, 0);

#define EXTEND(v) \
    if (v) items = eina_list_append(items, v); \

    EXTEND(sd->month_access);
    EXTEND(sd->dec_btn_month);
    EXTEND(sd->inc_btn_month);

#undef EXTEND

   for (int i = sd->first_day_it; i <= max_day; ++i)
     items = eina_list_append(items, sd->items[i]);

   efl_ui_focus_composition_elements_set(obj, items);
}

/**
 * @internal
 * @brief Populates the calendar grid with day numbers for the currently shown month.
 *
 * This function is responsible for:
 * - Setting the month and year display.
 * - Determining the starting day of the week for the current month.
 * - Filling in the day numbers in the 6x7 grid.
 * - Marking "today", the selected day, and disabled days (outside min/max range).
 * - Handling accessibility registration for items if needed.
 * - Flushing composite elements for focus.
 *
 * @param obj The calendar widget object.
 */
static void
_populate(Evas_Object *obj)
{
   int maxdays, prev_month_maxdays, day, mon, yr, i;
   char part[16], day_s[16];
   struct tm first_day;

   EFL_UI_CALENDAR_DATA_GET(obj, sd);

   elm_layout_freeze(obj);

   sd->filling = EINA_FALSE;
   if (sd->today_it > 0) _not_today(sd);

   maxdays = _maxdays_get(&sd->shown_date, 0);
   prev_month_maxdays = _maxdays_get(&sd->shown_date, -1);
   mon = sd->shown_date.tm_mon;
   yr = sd->shown_date.tm_year;

   _set_month_year(sd);
   sd->filling = EINA_TRUE;

   /* Set days */
   day = 0;
   first_day = sd->shown_date;
   first_day.tm_mday = 1;
   if (mktime(&first_day) == -1)
     {
        ERR("mktime can not give week day for this month properly. Please check year or month is proper.");
        return;
     }

   // Layout of the calendar is changed for removing the unfilled last row.
   if (first_day.tm_wday < (int)sd->first_week_day)
     sd->first_day_it = first_day.tm_wday + ELM_DAY_LAST - sd->first_week_day;
   else
     sd->first_day_it = first_day.tm_wday - sd->first_week_day;

   for (i = 0; i < 42; i++)
     {
        if ((!day) && (i == sd->first_day_it)) day = 1;

        if ((day == sd->current_date.tm_mday)
            && (mon == sd->current_date.tm_mon)
            && (yr == sd->current_date.tm_year))
          _today(sd, i);

        if (day == sd->date.tm_mday)
          {
             if ((sd->selected_it > -1) && (sd->selected_it != i))
               _unselect(obj, sd->selected_it);

             if ((mon == sd->date.tm_mon) && (yr == sd->date.tm_year))
               _select(obj, i);
          }

        if ((day) && (day <= maxdays))
          {
             if (((yr == sd->date_min.tm_year) && (mon == sd->date_min.tm_mon) && (day < sd->date_min.tm_mday))
                 || ((yr == sd->date_max.tm_year) && (mon == sd->date_max.tm_mon) && (day > sd->date_max.tm_mday)))
               _disable(sd, i);
             else
               _enable(sd, i);

             snprintf(day_s, sizeof(day_s), "%d", day++);
          }
        else
          {
             _disable(sd, i);

             if (day <= maxdays)
               snprintf(day_s, sizeof(day_s), "%d", prev_month_maxdays - sd->first_day_it + i + 1);
             else
               snprintf(day_s, sizeof(day_s), "%d", i - sd->first_day_it - maxdays + 1);
          }

        snprintf(part, sizeof(part), "efl.cit_%d.text", i);
        elm_layout_text_set(obj, part, day_s);
     }

   // ACCESS
   if ((_elm_config->access_mode != ELM_ACCESS_MODE_OFF))
     _access_calendar_item_register(obj);

   sd->filling = EINA_FALSE;

   elm_layout_thaw(obj);
   edje_object_message_signal_process(elm_layout_edje_get(obj));

   _flush_calendar_composite_elements(obj, sd);
}

/**
 * @internal
 * @brief Sets the weekday headers (e.g., Sun, Mon, Tue) in the calendar.
 *
 * It retrieves abbreviated weekday names based on the locale (or defaults)
 * and displays them according to the `first_week_day` setting.
 *
 * @param obj The calendar widget object.
 */
static void
_set_headers(Evas_Object *obj)
{
   static char part[] = "efl.ch_0.text";
   int i;
   struct tm *t;
   time_t temp = 259200; // the first sunday since epoch
   EFL_UI_CALENDAR_DATA_GET(obj, sd);

   elm_layout_freeze(obj);

   sd->filling = EINA_TRUE;

   t = gmtime(&temp);
   if (t)
     {
        t->tm_wday = 0;
        for (i = 0; i < ELM_DAY_LAST; i++)
          {
             char *buf;
             buf = eina_strftime("%a", t);
             if (buf)
               {
                  sd->weekdays[i] = eina_stringshare_add(buf);
                  free(buf);
               }
             else
               {
                  /* If we failed getting day, get a default value */
                  sd->weekdays[i] = _days_abbrev[i];
                  WRN("Failed getting weekday name for '%s' from locale.",
                      _days_abbrev[i]);
               }
             t->tm_wday++;
          }
     }

   for (i = 0; i < ELM_DAY_LAST; i++)
     {
        part[7] = i + '0';
        elm_layout_text_set(obj, part, sd->weekdays[(i + sd->first_week_day) % ELM_DAY_LAST]);
     }

   sd->filling = EINA_FALSE;

   elm_layout_thaw(obj);
}

/**
 * @internal
 * @brief Creates a button for the calendar (e.g., previous/next month).
 *
 * @param obj The parent calendar widget object.
 * @param style The style to apply to the button (unused, uses part name as style).
 * @param part The Edje part name in the parent's layout to swallow the button.
 * @return The newly created button object, or NULL on failure.
 */
static Eo *
_btn_create(Eo *obj, const char *style, char *part)
{
   return efl_add(EFL_UI_BUTTON_CLASS, obj,
                  elm_widget_element_update(obj, efl_added, style),
                  efl_ui_autorepeat_enabled_set(efl_added, EINA_TRUE),
                  efl_ui_autorepeat_initial_timeout_set(efl_added, FIRST_INTERVAL),
                  efl_ui_autorepeat_gap_timeout_set(efl_added, INTERVAL),
                  efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED,
                                         _inc_dec_btn_clicked_cb, obj),
                  efl_event_callback_add(efl_added, EFL_UI_AUTOREPEAT_EVENT_REPEATED,
                                         _inc_dec_btn_repeated_cb, obj),
                  efl_content_set(efl_part(obj, part), efl_added));
}

/**
 * @internal
 * @brief Adds or updates the increment/decrement month buttons.
 *
 * Checks if the theme defines parts for left/right buttons and creates/destroys
 * them as necessary. This is typically called during theme updates.
 *
 * @param obj The calendar widget object.
 * @param sd Pointer to the private data of the calendar widget.
 */
static void
_spinner_buttons_add(Evas_Object *obj, Efl_Ui_Calendar_Data *sd)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (edje_object_part_exists(wd->resize_obj, EFL_UI_CALENDAR_BUTTON_LEFT))
     {
        if (sd->dec_btn_month && efl_isa(sd->dec_btn_month, ELM_ACCESS_CLASS))
          {
             _elm_access_edje_object_part_object_unregister
               (obj, elm_layout_edje_get(obj), "left_bt");
             sd->dec_btn_month = NULL;
          }

        if (!sd->dec_btn_month)
          sd->dec_btn_month = _btn_create(obj, PART_NAME_DEC_BUTTON, EFL_UI_CALENDAR_BUTTON_LEFT);
     }

   else if (sd->dec_btn_month && !efl_isa(sd->dec_btn_month, ELM_ACCESS_CLASS))
     {
        evas_object_del(sd->dec_btn_month);
        sd->dec_btn_month = NULL;
     }

   if (edje_object_part_exists(wd->resize_obj, EFL_UI_CALENDAR_BUTTON_RIGHT))
     {
        if (sd->inc_btn_month && efl_isa(sd->inc_btn_month, ELM_ACCESS_CLASS))
          {
             _elm_access_edje_object_part_object_unregister
               (obj, elm_layout_edje_get(obj), "right_bt");
             sd->inc_btn_month = NULL;
          }

        if (!sd->inc_btn_month)
             sd->inc_btn_month = _btn_create(obj, PART_NAME_INC_BUTTON, EFL_UI_CALENDAR_BUTTON_RIGHT);
     }
   else if (sd->inc_btn_month && !efl_isa(sd->inc_btn_month, ELM_ACCESS_CLASS))
     {
        evas_object_del(sd->inc_btn_month);
        sd->inc_btn_month = NULL;
     }
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_widget_theme_apply.
 *
 * Applies the theme to the calendar widget. It calls the superclass's
 * theme apply function and then specifically handles the creation or
 * update of spinner buttons based on the new theme.
 *
 * @param obj The calendar widget object.
 * @param sd Pointer to the private data of the calendar widget.
 * @return An Eina_Error code indicating success or failure.
 */
EOLIAN static Eina_Error
_efl_ui_calendar_efl_ui_widget_theme_apply(Eo *obj, Efl_Ui_Calendar_Data *sd)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   _spinner_buttons_add(obj, sd);

   evas_object_smart_changed(obj);
   return int_ret;
}

/**
 * @internal
 * @brief Adjusts the calendar's selected date (`sd->date`) to be within min/max limits.
 *
 * If the current `sd->date` is outside the `sd->date_min` or `sd->date_max`
 * boundaries, it clamps `sd->date` to the respective limit. It also ensures
 * `sd->shown_date` (the currently displayed month/year) is updated if `sd->date`
 * changes due to clamping.
 * This function does not directly update `tm_wday` or `tm_yday`; `mktime`
 * should be used elsewhere if those fields need to be canonicalized.
 *
 * @param sd Pointer to the private data of the calendar widget.
 * @return EINA_TRUE if the date was already within limits or successfully adjusted,
 *         EINA_FALSE if clamping occurred (indicating the original date was out of bounds).
 *         Note: The return value primarily indicates if a change was made due to clamping.
 */
static inline Eina_Bool
_fix_date(Efl_Ui_Calendar_Data *sd)
{
   Eina_Bool no_change = EINA_TRUE;

   if ((sd->date.tm_year < sd->date_min.tm_year) ||
       ((sd->date.tm_year == sd->date_min.tm_year) &&
        (sd->date.tm_mon < sd->date_min.tm_mon)) ||
       ((sd->date.tm_year == sd->date_min.tm_year) &&
        (sd->date.tm_mon == sd->date_min.tm_mon) &&
        (sd->date.tm_mday < sd->date_min.tm_mday)))
     {
        sd->date.tm_year = sd->shown_date.tm_year = sd->date_min.tm_year;
        sd->date.tm_mon = sd->shown_date.tm_mon = sd->date_min.tm_mon;
        sd->date.tm_mday = sd->shown_date.tm_mday = sd->date_min.tm_mday;
        no_change = EINA_FALSE;
     }
   else if ((sd->date_max.tm_year != -1) &&
            ((sd->date.tm_year > sd->date_max.tm_year) ||
            ((sd->date.tm_year == sd->date_max.tm_year) &&
             (sd->date.tm_mon > sd->date_max.tm_mon)) ||
            ((sd->date.tm_year == sd->date_max.tm_year) &&
             (sd->date.tm_mon == sd->date_max.tm_mon) &&
             (sd->date.tm_mday > sd->date_max.tm_mday))))
     {
        sd->date.tm_year = sd->shown_date.tm_year = sd->date_max.tm_year;
        sd->date.tm_mon = sd->shown_date.tm_mon = sd->date_max.tm_mon;
        sd->date.tm_mday = sd->shown_date.tm_mday = sd->date_max.tm_mday;
        no_change = EINA_FALSE;
     }
   else
     {
        if (sd->date.tm_mon != sd->shown_date.tm_mon)
          sd->date.tm_mon = sd->shown_date.tm_mon;
        if (sd->date.tm_year != sd->shown_date.tm_year)
          sd->date.tm_year = sd->shown_date.tm_year;
     }

   return no_change;
}

/**
 * @internal
 * @brief Updates the `shown_date` of the calendar by a given month delta.
 *
 * This function changes the month (and year, if necessary) of `sd->shown_date`
 * by `delta` months. It respects the `date_min` and `date_max` limits,
 * preventing navigation beyond these boundaries. It also adjusts `sd->date.tm_mday`
 * if the new month has fewer days than the current `tm_mday`.
 *
 * @param obj The calendar widget object.
 * @param delta The number of months to change by (e.g., 1 for next, -1 for previous).
 * @return EINA_TRUE if the `shown_date` was successfully updated,
 *         EINA_FALSE if the update was prevented by min/max limits or `mktime` failure.
 */
static Eina_Bool
_update_data(Evas_Object *obj, int delta)
{
   struct tm time_check;
   int maxdays;

   EFL_UI_CALENDAR_DATA_GET(obj, sd);

   /* check if it's a valid time. for 32 bits, year greater than 2037 is not */
   time_check = sd->shown_date;
   time_check.tm_mon += delta;

   if (mktime(&time_check) == -1)
     {
        ERR("mktime can not give week day for the next month. Please check what is wrong with update date.");
        return EINA_FALSE;
     }

   sd->shown_date.tm_mon += delta;

   if (delta < 0)
     {
        if (sd->shown_date.tm_year == sd->date_min.tm_year)
          {
             if (sd->shown_date.tm_mon < sd->date_min.tm_mon)
               {
                  sd->shown_date.tm_mon = sd->date_min.tm_mon;
                  return EINA_FALSE;
               }
          }
        else if (sd->shown_date.tm_mon < 0)
          {
             sd->shown_date.tm_mon = 11;
             sd->shown_date.tm_year--;
          }
     }
   else
     {
        if (sd->shown_date.tm_year == sd->date_max.tm_year)
          {
             if (sd->shown_date.tm_mon > sd->date_max.tm_mon)
               {
                  sd->shown_date.tm_mon = sd->date_max.tm_mon;
                  return EINA_FALSE;
               }
          }
        else if (sd->shown_date.tm_mon > 11)
          {
             sd->shown_date.tm_mon = 0;
             sd->shown_date.tm_year++;
          }
     }

   maxdays = _maxdays_get(&sd->shown_date, 0);
   if (sd->date.tm_mday > maxdays)
     sd->date.tm_mday = maxdays;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Callback function to change the displayed month.
 *
 * This is typically called by spinner buttons (next/prev month) when clicked or
 * due to autorepeat. It uses `sd->spin_speed` to determine the direction
 * (1 for next month, -1 for previous month).
 *
 * @param data The calendar widget object (passed as `void *`).
 */
static void
_spin_value(void *data)
{
   EFL_UI_CALENDAR_DATA_GET(data, sd);

   if (_update_data(data, sd->spin_speed))
     evas_object_smart_changed(data);
}

static void
_inc_dec_btn_clicked_cb(void *data,
                        const Efl_Event *ev)
{
   EFL_UI_CALENDAR_DATA_GET(data, sd);

   sd->spin_speed = (ev->object == sd->inc_btn_month) ? 1 : -1;

   _spin_value(data);
}

static void
_inc_dec_btn_repeated_cb(void *data,
                         const Efl_Event *ev)
{
   EFL_UI_CALENDAR_DATA_GET(data, sd);

   sd->spin_speed = (ev->object == sd->inc_btn_month) ? 1 : -1;

   _spin_value(data);
}

/**
 * @internal
 * @brief Gets the day number (1-31) corresponding to a calendar item index.
 *
 * Converts a flat item index (0-41) from the calendar grid to a day of the month.
 * It also checks if this day is within the allowed min/max date range.
 *
 * @param obj The calendar widget object.
 * @param selected_it The index of the calendar item (0-41).
 * @return The day of the month (1-31) if valid and within range, otherwise 0.
 */
static int
_get_item_day(Evas_Object *obj,
              int selected_it)
{
   int day;

   EFL_UI_CALENDAR_DATA_GET(obj, sd);

   day = selected_it - sd->first_day_it + 1;
   if ((day < 0) || (day > _maxdays_get(&sd->shown_date, 0)))
     return 0;

   if ((sd->shown_date.tm_year == sd->date_min.tm_year)
       && (sd->shown_date.tm_mon == sd->date_min.tm_mon)
       && (day < sd->date_min.tm_mday))
     {
        return 0;
     }
   else if ((sd->shown_date.tm_year == sd->date_max.tm_year)
            && (sd->shown_date.tm_mon == sd->date_max.tm_mon)
            && (day > sd->date_max.tm_mday))
     {
        return 0;
     }

   return day;
}

/**
 * @internal
 * @brief Updates the visual state of a calendar item to "unfocused".
 *
 * If the item corresponds to a valid day, it emits a signal to mark it as unfocused
 * and resets the internal `focused_it` tracker.
 *
 * @param obj The calendar widget object.
 * @param unfocused_it The index of the calendar item to unfocus (0-41).
 */
static void
_update_unfocused_it(Evas_Object *obj, int unfocused_it)
{
   int day;
   char emission[32];

   EFL_UI_CALENDAR_DATA_GET(obj, sd);

   day = _get_item_day(obj, unfocused_it);
   if (!day)
     return;

   sd->focused_it = -1;

   snprintf(emission, sizeof(emission), "cit_%d,unfocused", unfocused_it);
   elm_layout_signal_emit(obj, emission, "efl");
}

/**
 * @internal
 * @brief Updates the visual state of a calendar item to "focused".
 *
 * If the `focused_it` corresponds to a valid day, it unfocuses the previously
 * focused item (if any), updates the internal `focused_it` tracker, and emits
 * a signal to mark the new item as focused.
 *
 * @param obj The calendar widget object.
 * @param focused_it The index of the calendar item to focus (0-41).
 * @return EINA_TRUE if the item was successfully focused, EINA_FALSE otherwise (e.g., invalid day).
 */
static Eina_Bool
_update_focused_it(Evas_Object *obj, int focused_it)
{
   int day;
   char emission[32];

   EFL_UI_CALENDAR_DATA_GET(obj, sd);

   day = _get_item_day(obj, focused_it);
   if (!day)
     return EINA_FALSE;

   snprintf(emission, sizeof(emission), "cit_%d,unfocused", sd->focused_it);
   elm_layout_signal_emit(obj, emission, "efl");

   sd->focused_it = focused_it;

   snprintf(emission, sizeof(emission), "cit_%d,focused", sd->focused_it);
   elm_layout_signal_emit(obj, emission, "efl");

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Updates the selected day in the calendar.
 *
 * This function is called when a day item `sel_it` is chosen.
 * It validates the day, unselects the previously selected item, updates
 * the internal date (`sd->date`), selects the new item visually,
 * and emits the "changed" signal.
 *
 * @param obj The calendar widget object.
 * @param sel_it The index of the calendar item to select (0-41).
 */
static void
_update_sel_it(Evas_Object *obj,
               int sel_it)
{
   int day;

   EFL_UI_CALENDAR_DATA_GET(obj, sd);

   day = _get_item_day(obj, sel_it);
   if (!day)
     return;

   _unselect(obj, sd->selected_it);
   if (!sd->selected)
     sd->selected = EINA_TRUE;
   if (sd->focused_it)
     _update_unfocused_it(obj, sd->focused_it);

   sd->date.tm_mday = day;
   _fix_date(sd);
   _select(obj, sel_it);
   efl_event_callback_legacy_call(obj, EFL_UI_CALENDAR_EVENT_CHANGED, NULL);
}

/**
 * @internal
 * @brief Callback for when a day is selected via an Edje signal.
 *
 * This function is triggered by the "efl,action,selected" signal from the
 * Edje layout, where the `source` string contains the index of the selected item.
 *
 * @param data The calendar widget object (passed as `void *`).
 * @param obj The Evas_Object that emitted the signal (unused).
 * @param emission The emission string of the signal (unused).
 * @param source The source string of the signal, expected to be the integer index
 *               of the selected calendar item.
 */
static void
_day_selected(void *data,
              Evas_Object *obj EINA_UNUSED,
              const char *emission EINA_UNUSED,
              const char *source)
{
   int sel_it;

   sel_it = atoi(source);

   _update_sel_it(data, sel_it);
}

/**
 * @internal
 * @brief Calculates the number of seconds remaining until the next day.
 *
 * @param t A pointer to a `struct tm` representing the current time.
 * @return The number of seconds from the given time `t` until midnight.
 */
static inline int
_time_to_next_day(struct tm *t)
{
   return ((((24 - t->tm_hour) * 60) - t->tm_min) * 60) - t->tm_sec;
}

/**
 * @internal
 * @brief Timer callback to update the "today" marker in the calendar.
 *
 * This function is called by a timer, typically once a day or when the
 * calendar is first created. It updates `sd->current_date` to the current system
 * date/time. If the currently displayed month (`sd->shown_date`) is the same as
 * the new current month, it marks the correct day item as "today".
 * It reschedules itself to run again at the start of the next day.
 *
 * @param data The calendar widget object (passed as `void *`).
 * @return ECORE_CALLBACK_RENEW to keep the timer active.
 */
static Eina_Bool
_update_cur_date(void *data)
{
   time_t current_date;
   int t, day;
   EFL_UI_CALENDAR_DATA_GET(data, sd);

   if (sd->today_it > 0) _not_today(sd);

   current_date = time(NULL);
   localtime_r(&current_date, &sd->current_date);
   t = _time_to_next_day(&sd->current_date);
   ecore_timer_interval_set(sd->update_timer, t);

   if ((sd->current_date.tm_mon != sd->shown_date.tm_mon) ||
       (sd->current_date.tm_year != sd->shown_date.tm_year))
     return ECORE_CALLBACK_RENEW;

   day = sd->current_date.tm_mday + sd->first_day_it - 1;
   _today(sd, day);

   return ECORE_CALLBACK_RENEW;
}

/**
 * @internal
 * @brief Handles the "activate" action, typically triggered by keyboard (e.g., Enter key).
 *
 * This function selects the currently focused calendar item (`sd->focused_it`).
 *
 * @param obj The calendar widget object.
 * @param params Action parameters (unused).
 * @return EINA_TRUE if the action was handled (always true in this case).
 */
static Eina_Bool
_key_action_activate(Evas_Object *obj, const char *params EINA_UNUSED)
{
   EFL_UI_CALENDAR_DATA_GET(obj, sd);

   _update_sel_it(obj, sd->focused_it);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_focus_object_on_focus_update.
 *
 * Handles focus state changes for the calendar widget. When the calendar gains
 * focus, it visually marks the selected item as focused. When it loses focus,
 * it visually unfocuses the item.
 *
 * @param obj The calendar widget object.
 * @param sd Pointer to the private data of the calendar widget.
 * @return EINA_TRUE if focus update was handled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_calendar_efl_ui_focus_object_on_focus_update(Eo *obj, Efl_Ui_Calendar_Data *sd)
{
   Eina_Bool int_ret = EINA_FALSE;

   int_ret = efl_ui_focus_object_on_focus_update(efl_super(obj, MY_CLASS));
   if (!int_ret) return EINA_FALSE;

   // FIXME : Currently, focused item is same with selected item.
   //         After arranging focus logic in this widget, we need to make
   //         focused item which is for indicating direction key input movement
   //         on the calendar widget.
   if (efl_ui_focus_object_focus_get(obj))
     _update_focused_it(obj, sd->selected_it);
   else
     _update_unfocused_it(obj, sd->focused_it);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_canvas_group_group_calculate.
 *
 * This function is called when the calendar widget needs to recalculate its layout.
 * It sets the weekday headers and populates the calendar grid with day numbers.
 *
 * @param obj The calendar widget object.
 * @param _pd Pointer to the private data of the calendar widget (unused in this function).
 */
EOLIAN static void
_efl_ui_calendar_efl_canvas_group_group_calculate(Eo *obj, Efl_Ui_Calendar_Data *_pd EINA_UNUSED)
{
   efl_canvas_group_need_recalculate_set(obj, EINA_FALSE);
   _set_headers(obj);
   _populate(obj);

   efl_canvas_group_calculate(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_object_destructor.
 *
 * Cleans up resources used by the calendar widget upon its destruction.
 * This includes deleting the "today" update timer and freeing stringshared weekday names.
 *
 * @param obj The calendar widget object.
 * @param sd Pointer to the private data of the calendar widget.
 */
EOLIAN static void
_efl_ui_calendar_efl_object_destructor(Eo *obj, Efl_Ui_Calendar_Data *sd)
{
   int i;

   ecore_timer_del(sd->update_timer);

   for (i = 0; i < ELM_DAY_LAST; i++)
     eina_stringshare_del(sd->weekdays[i]);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Manages the registration or unregistration of accessibility objects.
 *
 * Based on the `is_access` flag, this function either calls
 * `_access_calendar_register` to set up accessibility information for calendar
 * elements (day items, spinner buttons, month text) or unregisters them.
 *
 * @param obj The calendar widget object.
 * @param is_access If EINA_TRUE, register accessibility objects;
 *                  if EINA_FALSE, unregister them.
 */
static void
_access_obj_process(Evas_Object *obj, Eina_Bool is_access)
{
   int maxdays, day, i;

   EFL_UI_CALENDAR_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (is_access)
     _access_calendar_register(obj);
   else
     {
        day = 0;
        maxdays = _maxdays_get(&sd->shown_date, 0);
        for (i = 0; i < 42; i++)
          {
             if ((!day) && (i == sd->first_day_it)) day = 1;
             if ((day) && (day <= maxdays))
               {
                  char pname[32];
                  snprintf(pname, sizeof(pname), "efl.cit_%d.access", i);

                  _elm_access_edje_object_part_object_unregister
                    (obj, elm_layout_edje_get(obj), pname);
               }
          }

        if (sd->dec_btn_month && efl_isa(sd->dec_btn_month, ELM_ACCESS_CLASS))
          {
             _elm_access_edje_object_part_object_unregister
               (obj, elm_layout_edje_get(obj), "left_bt");
             sd->dec_btn_month = NULL;
          }
        if (sd->inc_btn_month && efl_isa(sd->inc_btn_month, ELM_ACCESS_CLASS))
          {
             _elm_access_edje_object_part_object_unregister
               (obj, elm_layout_edje_get(obj), "right_bt");
             sd->inc_btn_month = NULL;
          }
        if (sd->month_access)
          _elm_access_edje_object_part_object_unregister
            (obj, elm_layout_edje_get(obj), "month_text");
     }
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_widget_on_access_update.
 *
 * Called when the accessibility state changes. It enables or disables
 * accessibility features for the calendar based on the `acs` parameter.
 *
 * @param obj The calendar widget object (unused in this function).
 * @param _pd Pointer to the private data of the calendar widget (unused in this function).
 * @param acs EINA_TRUE if accessibility is enabled, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_ui_calendar_efl_ui_widget_on_access_update(Eo *obj EINA_UNUSED, Efl_Ui_Calendar_Data *_pd EINA_UNUSED, Eina_Bool acs)
{
   _efl_ui_calendar_smart_focus_next_enable = acs;
   _access_obj_process(obj, _efl_ui_calendar_smart_focus_next_enable);
}

/**
 * @internal
 * @brief Internal constructor logic for the Efl.Ui.Calendar widget.
 *
 * This function initializes the calendar's private data, sets default min/max dates,
 * registers signal callbacks for day selection, sets up the current date and
 * "today" update timer, configures focusability, and applies the theme.
 * It also creates the internal calendar item objects.
 *
 * @param obj The calendar widget object being constructed.
 * @param priv Pointer to the private data of the calendar widget.
 * @return The constructed calendar object, or NULL on failure.
 */
static Eo *
_efl_ui_calendar_constructor_internal(Eo *obj, Efl_Ui_Calendar_Data *priv)
{
   time_t current_date;
   int t;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   priv->date_min.tm_year = 2;
   priv->date_min.tm_mon = 0;
   priv->date_min.tm_mday = 1;
   priv->date_max.tm_year = -1;
   priv->date_max.tm_mon = 11;
   priv->date_max.tm_mday = 31;
   priv->today_it = -1;
   priv->selected_it = -1;
   priv->first_day_it = -1;

   edje_object_signal_callback_add
     (wd->resize_obj, "efl,action,selected", "*",
     _day_selected, obj);

   current_date = time(NULL);
   localtime_r(&current_date, &priv->shown_date);
   priv->current_date = priv->shown_date;
   priv->date = priv->shown_date;
   t = _time_to_next_day(&priv->current_date);
   priv->update_timer = ecore_timer_add(t, _update_cur_date, obj);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "calendar");
   if (elm_widget_theme_object_set(obj, wd->resize_obj,
                                       elm_widget_theme_klass_get(obj),
                                       elm_widget_theme_element_get(obj),
                                       elm_widget_theme_style_get(obj)) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     CRI("Failed to set layout!");

   evas_object_smart_changed(obj);

   // ACCESS
   if ((_elm_config->access_mode != ELM_ACCESS_MODE_OFF))
      _access_calendar_spinner_register(obj);

   // Items for composition
   for (int i = 0; i < 42; ++i)
     {
        priv->items[i] = efl_add(EFL_UI_CALENDAR_ITEM_CLASS, obj,
                                 efl_ui_calendar_item_day_number_set(efl_added, i));
     }

   return obj;
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_object_constructor.
 *
 * Main constructor for the Efl.Ui.Calendar widget. It calls the superclass
 * constructor, sets up smart callbacks, sets the accessibility role,
 * and then calls the internal constructor logic.
 *
 * @param obj The calendar widget object being constructed.
 * @param sd Pointer to the private data of the calendar widget.
 * @return The constructed calendar object.
 */
EOLIAN static Eo *
_efl_ui_calendar_efl_object_constructor(Eo *obj, Efl_Ui_Calendar_Data *sd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   sd->obj = obj;
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_DATE_EDITOR);

   obj = _efl_ui_calendar_constructor_internal(obj, sd);
   // 7x8 (1 month+year, days, 6 dates.)
   efl_ui_layout_finger_size_multiplier_set(obj, 7, 8);

   return obj;
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_calendar_date_min_set.
 *
 * Sets the minimum selectable date for the calendar.
 *
 * @param obj The calendar widget object.
 * @param sd Pointer to the private data of the calendar widget.
 * @param min The minimum date to set, as an `Efl_Time` (struct tm).
 *            Example: `{ .tm_year = 120, .tm_mon = 0, .tm_mday = 1 }` for Jan 1, 2020.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid date, min > max).
 */
EOLIAN static Eina_Bool
_efl_ui_calendar_date_min_set(Eo *obj, Efl_Ui_Calendar_Data *sd, Efl_Time min)
{
   Eina_Bool upper = EINA_FALSE;
   struct tm temp;

   temp = min;
   if (mktime(&temp) == -1)
     {
        ERR("mktime can not give week day for your minimum date. Please check the date.");
        return EINA_FALSE;
     }

   if ((sd->date_min.tm_year == min.tm_year)
       && (sd->date_min.tm_mon == min.tm_mon)
       && (sd->date_min.tm_mday == min.tm_mday))
     return EINA_TRUE;

   if (min.tm_year < 2)
     {
        sd->date_min.tm_year = 2;
        sd->date_min.tm_mon = 0;
        sd->date_min.tm_mday = 1;
     }
   else
     {
        if (sd->date_max.tm_year != -1)
          {
             if (min.tm_year > sd->date_max.tm_year)
               {
                  upper = EINA_TRUE;
               }
             else if (min.tm_year == sd->date_max.tm_year)
               {
                  if (min.tm_mon > sd->date_max.tm_mon)
                    upper = EINA_TRUE;
                  else if ((min.tm_mon == sd->date_max.tm_mon) && (min.tm_mday > sd->date_max.tm_mday))
                    upper = EINA_TRUE;
               }
          }

        if (upper)
          {
             sd->date_min.tm_year = sd->date_max.tm_year;
             sd->date_min.tm_mon = sd->date_max.tm_mon;
             sd->date_min.tm_mday = sd->date_max.tm_mday;
          }
        else
          {
             sd->date_min.tm_year = min.tm_year;
             sd->date_min.tm_mon = min.tm_mon;
             sd->date_min.tm_mday = min.tm_mday;
          }
     }

   _fix_date(sd);

   evas_object_smart_changed(obj);

   if (upper)
     {
        ERR("Your minimum date is greater than current maximum date.");
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_calendar_date_min_get.
 *
 * Gets the minimum selectable date of the calendar.
 *
 * @param obj The calendar widget object (unused).
 * @param sd Pointer to the private data of the calendar widget.
 * @return The minimum date, as an `Efl_Time` (struct tm).
 */
EOLIAN static Efl_Time
_efl_ui_calendar_date_min_get(const Eo *obj EINA_UNUSED, Efl_Ui_Calendar_Data *sd)
{
   return sd->date_min;
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_calendar_date_max_set.
 *
 * Sets the maximum selectable date for the calendar.
 *
 * @param obj The calendar widget object.
 * @param sd Pointer to the private data of the calendar widget.
 * @param max The maximum date to set, as an `Efl_Time` (struct tm).
 *            Example: `{ .tm_year = 125, .tm_mon = 11, .tm_mday = 31 }` for Dec 31, 2025.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid date, max < min).
 */
EOLIAN static Eina_Bool
_efl_ui_calendar_date_max_set(Eo *obj, Efl_Ui_Calendar_Data *sd, Efl_Time max)
{
   Eina_Bool lower = EINA_FALSE;
   struct tm temp;

   temp = max;
   if (mktime(&temp) == -1)
     {
        ERR("mktime can not give week day for your maximum date. Please check the date.");
        return EINA_FALSE;
     }

   if ((sd->date_max.tm_year == max.tm_year)
       && (sd->date_max.tm_mon == max.tm_mon)
       && (sd->date_max.tm_mday == max.tm_mday))
     return EINA_TRUE;

   if (max.tm_year < sd->date_min.tm_year)
     {
        lower = EINA_TRUE;
     }
   else if (max.tm_year == sd->date_min.tm_year)
     {
        if (max.tm_mon < sd->date_min.tm_mon)
          lower = EINA_TRUE;
        else if ((max.tm_mon == sd->date_min.tm_mon) && (max.tm_mday < sd->date_min.tm_mday))
          lower = EINA_TRUE;
     }

   if (lower)
     {
        sd->date_max.tm_year = sd->date_min.tm_year;
        sd->date_max.tm_mon = sd->date_min.tm_mon;
        sd->date_max.tm_mday = sd->date_min.tm_mday;
     }
   else
     {
        sd->date_max.tm_year = max.tm_year;
        sd->date_max.tm_mon = max.tm_mon;
        sd->date_max.tm_mday = max.tm_mday;
     }

   _fix_date(sd);

   evas_object_smart_changed(obj);

   if (lower)
     {
        ERR("Your maximum date is less than current minimum date.");
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_calendar_date_max_get.
 *
 * Gets the maximum selectable date of the calendar.
 *
 * @param obj The calendar widget object (unused).
 * @param sd Pointer to the private data of the calendar widget.
 * @return The maximum date, as an `Efl_Time` (struct tm).
 */
EOLIAN static Efl_Time
_efl_ui_calendar_date_max_get(const Eo *obj EINA_UNUSED, Efl_Ui_Calendar_Data *sd)
{
   return sd->date_max;
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_calendar_date_set.
 *
 * Sets the currently selected date of the calendar.
 *
 * @param obj The calendar widget object.
 * @param sd Pointer to the private data of the calendar widget.
 * @param date The date to select, as an `Efl_Time` (struct tm).
 *             Example: `{ .tm_year = 123, .tm_mon = 4, .tm_mday = 15 }` for May 15, 2023.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid date, date out of min/max range).
 */
EOLIAN static Eina_Bool
_efl_ui_calendar_date_set(Eo *obj, Efl_Ui_Calendar_Data *sd, Efl_Time date)
{
   Eina_Bool ret = EINA_TRUE;
   struct tm temp;

   temp = date;
   if (mktime(&temp) == -1)
     {
        ERR("mktime can not give week day for your new date. Please check the date.");
        return EINA_FALSE;
     }

   sd->date.tm_year = date.tm_year;
   sd->date.tm_mon = date.tm_mon;
   sd->date.tm_mday = date.tm_mday;
   if (!sd->selected)
     sd->selected = EINA_TRUE;

   if (sd->date.tm_year != sd->shown_date.tm_year)
     sd->shown_date.tm_year = sd->date.tm_year;
   if (sd->date.tm_mon != sd->shown_date.tm_mon)
     sd->shown_date.tm_mon = sd->date.tm_mon;

   ret = _fix_date(sd);

   evas_object_smart_changed(obj);

   if (!ret)
     ERR("The current date is greater than the maximum date or less than the minimum date.");

   return ret;
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_calendar_date_get.
 *
 * Gets the currently selected date of the calendar.
 *
 * @param obj The calendar widget object (unused).
 * @param sd Pointer to the private data of the calendar widget.
 * @return The currently selected date, as an `Efl_Time` (struct tm).
 */
EOLIAN static Efl_Time
_efl_ui_calendar_date_get(const Eo *obj EINA_UNUSED, Efl_Ui_Calendar_Data *sd)
{
   return sd->date;
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_format_apply_formatted_value.
 *
 * Called when the formatting of the calendar (e.g., date format for month/year display)
 * needs to be reapplied. This typically triggers a redraw/repopulation of the calendar.
 *
 * @param obj The calendar widget object.
 * @param pd Pointer to the private data of the calendar widget (unused in this function).
 */
EOLIAN static void
_efl_ui_calendar_efl_ui_format_apply_formatted_value(Eo *obj, Efl_Ui_Calendar_Data *pd EINA_UNUSED)
{
   evas_object_smart_changed(obj);
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_calendar_first_day_of_week_set.
 *
 * Sets the first day of the week for the calendar display.
 *
 * @param obj The calendar widget object.
 * @param sd Pointer to the private data of the calendar widget.
 * @param day The weekday to set as the first day (e.g., @ref EFL_UI_CALENDAR_WEEKDAY_SUNDAY).
 */
EOLIAN static void
_efl_ui_calendar_first_day_of_week_set(Eo *obj, Efl_Ui_Calendar_Data *sd, Efl_Ui_Calendar_Weekday day)
{
   if (day >= EFL_UI_CALENDAR_WEEKDAY_LAST) return;
   if (sd->first_week_day != day)
     {
        sd->first_week_day = day;
        evas_object_smart_changed(obj);
     }
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_calendar_first_day_of_week_get.
 *
 * Gets the first day of the week used by the calendar.
 *
 * @param obj The calendar widget object (unused).
 * @param sd Pointer to the private data of the calendar widget.
 * @return The first day of the week (e.g., @ref EFL_UI_CALENDAR_WEEKDAY_MONDAY).
 */
EOLIAN static Efl_Ui_Calendar_Weekday
_efl_ui_calendar_first_day_of_week_get(const Eo *obj EINA_UNUSED, Efl_Ui_Calendar_Data *sd)
{
   return sd->first_week_day;
}

/**
 * @internal
 * @brief Class constructor for Efl.Ui.Calendar.
 *
 * Registers the legacy smart type and initializes global settings related to
 * accessibility for the calendar class.
 *
 * @param klass The Efl_Class being constructed.
 */
static void
_efl_ui_calendar_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME, klass);

   if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
      _efl_ui_calendar_smart_focus_next_enable = EINA_TRUE;
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_access_widget_action_elm_actions_get.
 *
 * Provides the list of supported accessibility actions for the calendar widget.
 * Currently, only the "activate" action is supported.
 *
 * @param obj The calendar widget object (unused).
 * @param sd Pointer to the private data of the calendar widget (unused).
 * @return A pointer to a static array of `Efl_Access_Action_Data` structures,
 *         terminated by a NULL entry.
 *         Example of an action: `{ "activate", "activate", NULL, _key_action_activate}`.
 */
EOLIAN static const Efl_Access_Action_Data*
_efl_ui_calendar_efl_access_widget_action_elm_actions_get(const Eo *obj EINA_UNUSED, Efl_Ui_Calendar_Data *sd EINA_UNUSED)
{
   static Efl_Access_Action_Data atspi_actions[] = {
          { "activate", "activate", NULL, _key_action_activate},
          { NULL, NULL, NULL, NULL }
   };
   return &atspi_actions[0];
}

/* Standard widget overrides */

ELM_WIDGET_KEY_DOWN_DEFAULT_IMPLEMENT(efl_ui_calendar, Efl_Ui_Calendar_Data)

#include "efl_ui_calendar.eo.c"

typedef struct {
   int v;
   Evas_Object *part;
}  Efl_Ui_Calendar_Item_Data;

/**
 * @internal
 * @brief Sets the day number (item index) for a calendar item.
 *
 * This function associates a calendar item object with its corresponding
 * Edje part in the main calendar layout and sets up focus event redirection.
 * The day number `i` here is the flat index (0-41) in the 6x7 grid.
 *
 * @param obj The calendar item object.
 * @param pd Pointer to the private data of the calendar item.
 * @param i The day number (index 0-41) to associate with this item.
 */
EOLIAN static void
_efl_ui_calendar_item_day_number_set(Eo *obj, Efl_Ui_Calendar_Item_Data *pd, int i)
{
   char pname[32];
   Evas_Object *po, *o;

   pd->v = i;
   snprintf(pname, sizeof(pname), "efl.cit_%i.access", i);

   o = elm_layout_edje_get(efl_parent_get(obj));
   edje_object_freeze(o);
   po = (Evas_Object *)edje_object_part_object_get(o, pname);
   edje_object_thaw(o);

   if (_elm_config->access_mode != ELM_ACCESS_MODE_ON)
     pd->part = po;
   else
     pd->part = evas_object_data_get(po, "_part_access_obj");
   _efl_ui_focus_event_redirector(pd->part, obj);

   EINA_SAFETY_ON_NULL_RETURN(pd->part);
}

/**
 * @internal
 * @brief Gets the day number (item index) of a calendar item.
 *
 * The day number returned is the flat index (0-41) in the 6x7 grid.
 *
 * @param obj The calendar item object (unused).
 * @param pd Pointer to the private data of the calendar item.
 * @return The day number (index 0-41) associated with this item.
 */
EOLIAN static int
_efl_ui_calendar_item_day_number_get(const Eo *obj EINA_UNUSED, Efl_Ui_Calendar_Item_Data *pd)
{
   return pd->v;
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_focus_object_focus_set for a calendar item.
 *
 * Sets the focus state of an individual calendar item (day).
 * It updates the focus state in the parent calendar and on the item's Edje part.
 *
 * @param obj The calendar item object.
 * @param pd Pointer to the private data of the calendar item.
 * @param focus EINA_TRUE to set focus, EINA_FALSE to unset.
 */
EOLIAN static void
_efl_ui_calendar_item_efl_ui_focus_object_focus_set(Eo *obj, Efl_Ui_Calendar_Item_Data *pd, Eina_Bool focus)
{
   efl_ui_focus_object_focus_set(efl_super(obj, EFL_UI_CALENDAR_ITEM_CLASS), focus);

   _update_focused_it(efl_parent_get(obj), pd->v);
   evas_object_focus_set(pd->part, efl_ui_focus_object_focus_get(obj));
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_focus_object_focus_geometry_get for a calendar item.
 *
 * Gets the geometry of the calendar item, which is derived from its Edje part.
 *
 * @param obj The calendar item object (unused).
 * @param pd Pointer to the private data of the calendar item.
 * @return The geometry of the item as an `Eina_Rect`.
 */
EOLIAN static Eina_Rect
_efl_ui_calendar_item_efl_ui_focus_object_focus_geometry_get(const Eo *obj EINA_UNUSED, Efl_Ui_Calendar_Item_Data *pd)
{
   return efl_gfx_entity_geometry_get(pd->part);
}

/**
 * @internal
 * @brief EOLIAN implementation for @ref efl_ui_focus_object_focus_parent_get for a calendar item.
 *
 * Gets the focus parent of the calendar item, which is the main calendar widget.
 *
 * @param obj The calendar item object.
 * @param pd Pointer to the private data of the calendar item (unused).
 * @return The focus parent object (the main calendar widget).
 */
EOLIAN static Efl_Ui_Focus_Object*
_efl_ui_calendar_item_efl_ui_focus_object_focus_parent_get(const Eo *obj, Efl_Ui_Calendar_Item_Data *pd EINA_UNUSED)
{
   return efl_parent_get(obj);
}

#include "efl_ui_calendar_item.eo.c"
