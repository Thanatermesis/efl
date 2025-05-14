#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

#include "elm_priv.h"
#include "efl_ui_datepicker_private.h"

#define MY_CLASS EFL_UI_DATEPICKER_CLASS

#define MY_CLASS_NAME "Efl.Ui.Datepicker"

/** @brief Maximum length for date format string buffer. */
#define FMT_LEN_MAX 32

/**
 * @brief Macro to get the current date from the datetime manager and store it in pd->cur_date.
 * pd->cur_date is an array where:
 * - pd->cur_date[DATEPICKER_YEAR] stores the year (e.g., 2023)
 * - pd->cur_date[DATEPICKER_MONTH] stores the month (1-12)
 * - pd->cur_date[DATEPICKER_DAY] stores the day (1-31)
 */
#define DATE_GET()                                                   \
   do {                                                              \
     Efl_Time t = efl_datetime_manager_value_get(pd->dt_manager);    \
     pd->cur_date[DATEPICKER_YEAR] = t.tm_year + 1900;               \
     pd->cur_date[DATEPICKER_MONTH] = t.tm_mon + 1;                  \
     pd->cur_date[DATEPICKER_DAY] = t.tm_mday;                   \
   } while (0)

/**
 * @brief Macro to set the date in the datetime manager from pd->cur_date.
 * pd->cur_date is an array where:
 * - pd->cur_date[DATEPICKER_YEAR] stores the year (e.g., 2023)
 * - pd->cur_date[DATEPICKER_MONTH] stores the month (1-12)
 * - pd->cur_date[DATEPICKER_DAY] stores the day (1-31)
 */
#define DATE_SET()                                                   \
   do {                                                              \
     Efl_Time t = { 0 };                                             \
     t.tm_year = pd->cur_date[DATEPICKER_YEAR] - 1900;               \
     t.tm_mon = pd->cur_date[DATEPICKER_MONTH] - 1;                  \
     t.tm_mday = pd->cur_date[DATEPICKER_DAY];                  \
     t.tm_sec = 0;                                                   \
     efl_datetime_manager_value_set(pd->dt_manager, t);              \
   } while (0)

/** @brief Array of characters representing date field types for format parsing.
 * "Yy" for year, "mbBh" for month, "de" for day.
 */
static const char *fmt_char[] = {"Yy", "mbBh", "de"};

/**
 * @brief Validates if the given year, month, and day form a plausible date range.
 *
 * @param year The year to validate.
 * @param month The month to validate (1-12).
 * @param day The day to validate (0-31, 0 can be valid in some contexts before adjustment).
 * @return @c EINA_TRUE if parameters are within a basic valid range, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_validate_params(int year, int month, int day)
{
  if (year < 1900 || year > 2037 || month < 1 || month > 12 || day < 0 || day > 31)
    return EINA_FALSE;
  else return EINA_TRUE;
}

/**
 * @brief Compares two date arrays.
 * The date arrays (time1, time2) are expected to have:
 * - timeX[DATEPICKER_YEAR]
 * - timeX[DATEPICKER_MONTH]
 * - timeX[DATEPICKER_DAY]
 *
 * @param time1 First date array.
 * @param time2 Second date array.
 * @return @c EINA_TRUE if dates are identical, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_date_cmp(int time1[], int time2[])
{
   unsigned int idx;

   for (idx = 0; idx < EFL_UI_DATEPICKER_TYPE_COUNT; idx++)
     {
        if (time1[idx] != time2[idx])
          return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Validates if time1 is before time2 and optionally swaps them if not.
 * This function is used to ensure min_date <= cur_date <= max_date.
 * If swap is EINA_TRUE and time1 > time2, time1's content will be replaced by time2's.
 * The date arrays (time1, time2) are expected to have:
 * - timeX[DATEPICKER_YEAR]
 * - timeX[DATEPICKER_MONTH]
 * - timeX[DATEPICKER_DAY]
 *
 * @param time1 First date array.
 * @param time2 Second date array.
 * @param swap If @c EINA_TRUE, and time1 is found to be later than time2,
 *             the contents of time2 will be copied into time1.
 * @return @c EINA_TRUE if time1 was adjusted (swapped), @c EINA_FALSE otherwise.
 */
static Eina_Bool
_validate_date_limits(int time1[], int time2[], Eina_Bool swap)
{
   unsigned int idx;
   int *t1, *t2;

   t1 = (swap) ? time2 : time1;
   t2 = (swap) ? time1 : time2;

   for (idx = 0; idx < EFL_UI_DATEPICKER_TYPE_COUNT; idx++)
     {
        if (time1[idx] < time2[idx])
          {
             memcpy(t1, t2, (sizeof(int) * EFL_UI_DATEPICKER_TYPE_COUNT));
             return EINA_TRUE;
          }
        else if (time1[idx] > time2[idx])
          return EINA_FALSE;
     }

   return EINA_FALSE;
}

/**
 * @brief Calculates the maximum number of days in a given month and year.
 *
 * @param year The year (e.g., 2000). Note: This function expects year as is (e.g. 2023),
 *             not year - 1900.
 * @param month The month (0-11, as per struct tm).
 * @return The number of days in the specified month and year.
 */
static int
_max_days_get(int year, int month)
{
   struct tm time1;
   time_t t;
   int day;

   t = time(NULL);
   localtime_r(&t, &time1);
   time1.tm_year = year;
   time1.tm_mon = month;
   for (day = 28; day <= 31;
        day++)
     {
        time1.tm_mday = day;
        mktime(&time1);
        /* To restrict month wrapping because of summer time in some locales,
         * ignore day light saving mode in mktime(). */
        time1.tm_isdst = -1;
        if (time1.tm_mday == 1) break;
     }
   day--;

   return day;
}

/**
 * @brief Updates the spin button field values from pd->cur_date and sets the date manager.
 * This ensures the UI elements (spin buttons) reflect the internal current date
 * and that the datetime manager is also updated.
 *
 * @param obj The datepicker widget.
 */
static void
_field_value_update(Eo *obj)
{
   Efl_Ui_Datepicker_Data *pd = efl_data_scope_get(obj, MY_CLASS);

   efl_ui_range_value_set(pd->year, pd->cur_date[DATEPICKER_YEAR]);
   efl_ui_range_value_set(pd->month, pd->cur_date[DATEPICKER_MONTH]);
   efl_ui_range_value_set(pd->day, pd->cur_date[DATEPICKER_DAY]);

   DATE_SET();
}

/**
 * @brief Callback invoked when a datepicker field (year, month, or day spin button) value changes.
 * It updates the internal current date (pd->cur_date), adjusts day limits if month/year changed,
 * validates against min/max dates, and fires a date_changed event.
 *
 * @param data The datepicker widget (Eo *obj).
 * @param ev The event information.
 */
static void
_field_changed_cb(void *data, const Efl_Event *ev)
{
   int max_day;

   Efl_Ui_Datepicker_Data *pd = efl_data_scope_get(data, MY_CLASS);

   if (ev->object == pd->year)
     pd->cur_date[DATEPICKER_YEAR] = efl_ui_range_value_get(pd->year);
   else if (ev->object == pd->month)
     pd->cur_date[DATEPICKER_MONTH] = efl_ui_range_value_get(pd->month);
   else
     pd->cur_date[DATEPICKER_DAY] = efl_ui_range_value_get(pd->day);

   if (!(ev->object == pd->day))
     {
        max_day = _max_days_get((pd->cur_date[DATEPICKER_YEAR] - 1900), (pd->cur_date[DATEPICKER_MONTH] - 1));
        efl_ui_range_limits_set(pd->day, 1, max_day);
     }

   if (_validate_date_limits(pd->cur_date, pd->min_date, EINA_FALSE) ||
       _validate_date_limits(pd->max_date, pd->cur_date, EINA_TRUE))
     {
        _field_value_update(data);
        return;
     }

   DATE_SET();
   efl_event_callback_call(data, EFL_UI_DATEPICKER_EVENT_DATE_CHANGED, NULL);
}

/**
 * @brief Initializes the datepicker fields (year, month, day spin buttons).
 * This includes creating the spin buttons, setting their initial ranges,
 * connecting change callbacks, setting initial min/max/current dates,
 * and arranging them according to the system's date format.
 *
 * @param obj The datepicker widget.
 */
static void
_fields_init(Eo *obj)
{
   const char *fmt;
   char ch;
   int i;
   int field = 0;
   char buf[FMT_LEN_MAX];

   Efl_Ui_Datepicker_Data *pd = efl_data_scope_get(obj, MY_CLASS);

   //Field create.
   pd->year = efl_add(EFL_UI_SPIN_BUTTON_CLASS, obj,
                      efl_ui_range_limits_set(efl_added, 1900, 2037),
                      efl_ui_spin_button_wraparound_set(efl_added, EINA_TRUE),
                      efl_ui_spin_button_direct_text_input_set(efl_added, EINA_TRUE),
                      efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
                      efl_event_callback_add(efl_added, EFL_UI_RANGE_EVENT_CHANGED,_field_changed_cb, obj));

   pd->month = efl_add(EFL_UI_SPIN_BUTTON_CLASS, obj,
                       efl_ui_range_limits_set(efl_added, 1, 12),
                       efl_ui_spin_button_wraparound_set(efl_added, EINA_TRUE),
                       efl_ui_spin_button_direct_text_input_set(efl_added, EINA_TRUE),
                       efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
                       efl_event_callback_add(efl_added, EFL_UI_RANGE_EVENT_CHANGED,_field_changed_cb, obj));

   pd->day = efl_add(EFL_UI_SPIN_BUTTON_CLASS, obj,
                     efl_ui_range_limits_set(efl_added, 1, 31),
                     efl_ui_spin_button_wraparound_set(efl_added, EINA_TRUE),
                     efl_ui_spin_button_direct_text_input_set(efl_added, EINA_TRUE),
                     efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
                     efl_event_callback_add(efl_added, EFL_UI_RANGE_EVENT_CHANGED,_field_changed_cb, obj));

   DATE_GET();
   //Using system config?
   pd->min_date[DATEPICKER_YEAR] = 1970;
   pd->min_date[DATEPICKER_MONTH] = 1;
   pd->min_date[DATEPICKER_DAY] = 1;
   pd->max_date[DATEPICKER_YEAR] = 2037;
   pd->max_date[DATEPICKER_MONTH] = 12;
   pd->max_date[DATEPICKER_DAY] = 31;

   _field_value_update(obj);

   fmt = efl_datetime_manager_format_get(pd->dt_manager);
   if (!fmt)
     {
        ERR("Failed to get current format.");
        //Gives default format when the gets format failed.
        fmt = "%Y %b %d";
     }

   //Sort fields by format.
   while((ch = *fmt))
     {
        //TODO: ignore extensions and separators.
        for (i = 0; i < EFL_UI_DATEPICKER_TYPE_COUNT; i++)
          {
             if (strchr(fmt_char[i], ch))
               {
                  snprintf(buf, sizeof(buf), "efl.field%d", field++);
                  if (i == DATEPICKER_YEAR)
                    efl_content_set(efl_part(obj, buf), pd->year);
                  else if (i == DATEPICKER_MONTH)
                    efl_content_set(efl_part(obj, buf), pd->month);
                  else
                    efl_content_set(efl_part(obj, buf), pd->day);

                  break;
               }
          }
        fmt++;
     }
}

EOLIAN static Eo *
_efl_ui_datepicker_efl_object_constructor(Eo *obj, Efl_Ui_Datepicker_Data *pd)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "datepicker");
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   if (elm_widget_theme_object_set(obj, wd->resize_obj,
                                       elm_widget_theme_klass_get(obj),
                                       elm_widget_theme_element_get(obj),
                                       elm_widget_theme_style_get(obj)) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     CRI("Failed to set layout!");

   pd->dt_manager = efl_add(EFL_DATETIME_MANAGER_CLASS, obj);

   _fields_init(obj);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   return obj;
}

EOLIAN static void
_efl_ui_datepicker_efl_object_destructor(Eo *obj, Efl_Ui_Datepicker_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Sets the minimum allowed date for the datepicker.
 * If the new minimum date is after the current maximum date, the maximum date is adjusted.
 * If the current date is before the new minimum date, the current date is adjusted.
 *
 * @param obj The datepicker widget.
 * @param pd Private data of the datepicker.
 * @param year The minimum year.
 * @param month The minimum month (1-12).
 * @param day The minimum day (1-31).
 */
EOLIAN static void
_efl_ui_datepicker_date_min_set(Eo *obj, Efl_Ui_Datepicker_Data *pd EINA_UNUSED, int year, int month, int day)
{
   int new_time[EFL_UI_DATEPICKER_TYPE_COUNT] = {year, month, day};

   if (!_validate_params(year, month, day)) return;
   if (_date_cmp(pd->min_date, new_time)) return;

   memcpy(pd->min_date, new_time, (sizeof(int) * EFL_UI_DATEPICKER_TYPE_COUNT));

   _validate_date_limits(pd->max_date, pd->min_date, EINA_FALSE);
   _validate_date_limits(pd->cur_date, pd->min_date, EINA_FALSE);

   DATE_SET();
   _field_value_update(obj);
}

/**
 * @brief Gets the minimum allowed date of the datepicker.
 *
 * @param obj The datepicker widget.
 * @param pd Private data of the datepicker.
 * @param year Pointer to store the minimum year.
 * @param month Pointer to store the minimum month (1-12).
 * @param day Pointer to store the minimum day (1-31).
 */
EOLIAN static void
_efl_ui_datepicker_date_min_get(const Eo *obj EINA_UNUSED, Efl_Ui_Datepicker_Data *pd, int *year, int *month, int *day)
{
   *year = pd->min_date[DATEPICKER_YEAR];
   *month = pd->min_date[DATEPICKER_MONTH];
   *day = pd->min_date[DATEPICKER_DAY];
}

/**
 * @brief Sets the maximum allowed date for the datepicker.
 * If the new maximum date is before the current minimum date, the minimum date is adjusted.
 * If the current date is after the new maximum date, the current date is adjusted.
 *
 * @param obj The datepicker widget.
 * @param pd Private data of the datepicker.
 * @param year The maximum year.
 * @param month The maximum month (1-12).
 * @param day The maximum day (1-31).
 */
EOLIAN static void
_efl_ui_datepicker_date_max_set(Eo *obj, Efl_Ui_Datepicker_Data *pd EINA_UNUSED, int year, int month, int day)
{
   int new_time[EFL_UI_DATEPICKER_TYPE_COUNT] = {year, month, day};

   if (!_validate_params(year, month, day)) return;
   if (_date_cmp(pd->max_date, new_time)) return;

   memcpy(pd->max_date, new_time, (sizeof(int) * EFL_UI_DATEPICKER_TYPE_COUNT));

   _validate_date_limits(pd->max_date, pd->min_date, EINA_TRUE);
   _validate_date_limits(pd->max_date, pd->cur_date, EINA_TRUE);

   DATE_SET();
   _field_value_update(obj);
}

/**
 * @brief Gets the maximum allowed date of the datepicker.
 *
 * @param obj The datepicker widget.
 * @param pd Private data of the datepicker.
 * @param year Pointer to store the maximum year.
 * @param month Pointer to store the maximum month (1-12).
 * @param day Pointer to store the maximum day (1-31).
 */
EOLIAN static void
_efl_ui_datepicker_date_max_get(const Eo *obj EINA_UNUSED, Efl_Ui_Datepicker_Data *pd, int *year, int *month, int *day)
{
   *year = pd->max_date[DATEPICKER_YEAR];
   *month = pd->max_date[DATEPICKER_MONTH];
   *day = pd->max_date[DATEPICKER_DAY];
}

/**
 * @brief Sets the current date of the datepicker.
 * The date will be clamped between the minimum and maximum dates if it falls outside.
 *
 * @param obj The datepicker widget.
 * @param pd Private data of the datepicker.
 * @param year The year to set.
 * @param month The month to set (1-12).
 * @param day The day to set (1-31).
 */
EOLIAN static void
_efl_ui_datepicker_date_set(Eo *obj, Efl_Ui_Datepicker_Data *pd, int year, int month, int day)
{
   int new_time[EFL_UI_DATEPICKER_TYPE_COUNT] = {year, month, day};

   if (!_validate_params(year, month, day)) return;
   if (_date_cmp(pd->cur_date, new_time)) return;

   memcpy(pd->cur_date, new_time, (sizeof(int) * EFL_UI_DATEPICKER_TYPE_COUNT));

   _validate_date_limits(pd->cur_date, pd->min_date, EINA_FALSE);
   _validate_date_limits(pd->max_date, pd->cur_date, EINA_TRUE);

   DATE_SET();
   _field_value_update(obj);
}

/**
 * @brief Gets the current date of the datepicker.
 *
 * @param obj The datepicker widget.
 * @param pd Private data of the datepicker.
 * @param year Pointer to store the current year.
 * @param month Pointer to store the current month (1-12).
 * @param day Pointer to store the current day (1-31).
 */
EOLIAN static void
_efl_ui_datepicker_date_get(const Eo *obj EINA_UNUSED, Efl_Ui_Datepicker_Data *pd, int *year, int *month, int *day)
{
   *year = pd->cur_date[DATEPICKER_YEAR];
   *month = pd->cur_date[DATEPICKER_MONTH];
   *day = pd->cur_date[DATEPICKER_DAY];
}

#include "efl_ui_datepicker.eo.c"
