#include "private.h"

static Elm_Prefs_Item_Type supported_types[] =
{
   ELM_PREFS_TYPE_DATE,
   ELM_PREFS_TYPE_UNKNOWN
};

/**
 * @internal
 * @brief Callback for when the datetime widget value changes.
 * @param data The user-provided change callback function.
 * @param event The event information.
 */
static void
_item_changed_cb(void *data, const Efl_Event *event)
{
   Elm_Prefs_Item_Changed_Cb prefs_it_changed_cb = data;

   prefs_it_changed_cb(event->object);
}

/**
 * @internal
 * @brief Adds a datetime widget to the prefs UI.
 *
 * This function creates and configures an elm_datetime widget for use as a
 * preferences item. It specifically configures it as a date picker by
 * hiding time-related fields.
 *
 * @param iface The prefs item interface (unused).
 * @param prefs The parent prefs widget.
 * @param type The item type (unused, expected to be ELM_PREFS_TYPE_DATE).
 * @param spec The specification for the item, containing min/max date values.
 * @param cb The callback to be invoked when the item's value changes.
 * @return The newly created datetime widget object.
 */
static Evas_Object *
elm_prefs_datetime_add(const Elm_Prefs_Item_Iface *iface EINA_UNUSED,
                       Evas_Object *prefs,
                       const Elm_Prefs_Item_Type type EINA_UNUSED,
                       const Elm_Prefs_Item_Spec spec,
                       Elm_Prefs_Item_Changed_Cb cb)
{
   Evas_Object *obj = elm_datetime_add(prefs);
   struct tm t;

   memset(&t, 0, sizeof t);

   elm_datetime_field_visible_set(obj, ELM_DATETIME_HOUR, EINA_FALSE);
   elm_datetime_field_visible_set(obj, ELM_DATETIME_MINUTE, EINA_FALSE);
   elm_datetime_field_visible_set(obj, ELM_DATETIME_AMPM, EINA_FALSE);

   efl_event_callback_add
     (obj, ELM_DATETIME_EVENT_CHANGED, _item_changed_cb, cb);

   t.tm_year = spec.d.min.y - 1900;
   t.tm_mon = spec.d.min.m - 1;
   t.tm_mday = spec.d.min.d;

   elm_datetime_value_min_set(obj, &t);

   t.tm_year = spec.d.max.y - 1900;
   t.tm_mon = spec.d.max.m - 1;
   t.tm_mday = spec.d.max.d;

   elm_datetime_value_max_set(obj, &t);

   return obj;
}

/**
 * @internal
 * @brief Sets the value of the datetime widget.
 *
 * The value is provided as an Eina_Value of type EINA_VALUE_TYPE_TIMEVAL.
 * The time_t from the timeval is converted to a broken-down time structure
 * using gmtime(), which interprets the timestamp as UTC.
 *
 * @param obj The datetime widget object.
 * @param value The new value to set.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_datetime_value_set(Evas_Object *obj,
                             Eina_Value *value)
{
   struct timeval val;
   struct tm *t;
   time_t gmt;

   if (eina_value_type_get(value) != EINA_VALUE_TYPE_TIMEVAL)
     return EINA_FALSE;

   eina_value_get(value, &val);
   gmt = val.tv_sec;
   t = gmtime(&gmt);

   if (elm_datetime_value_set(obj, t)) return EINA_TRUE;

   return EINA_FALSE;
}

/**
 * @internal
 * @brief Gets the value of the datetime widget.
 *
 * The function retrieves the date as a `struct tm` and converts it to a
 * `time_t` timestamp using mktime(). Note that mktime() interprets the
 * `struct tm` components as local time. The resulting timestamp is stored
 * in an Eina_Value of type EINA_VALUE_TYPE_TIMEVAL.
 *
 * @param obj The datetime widget object.
 * @param value A pointer to an Eina_Value to store the retrieved value.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_datetime_value_get(Evas_Object *obj,
                             Eina_Value *value)
{
   struct timeval val;
   struct tm t;

   memset(&val, 0, sizeof val);

   if (!elm_datetime_value_get(obj, &t)) return EINA_FALSE;

   val.tv_sec = mktime(&t);

   if (!eina_value_setup(value, EINA_VALUE_TYPE_TIMEVAL)) return EINA_FALSE;
   if (!eina_value_set(value, val)) return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Registers the datetime widget as a prefs item handler.
 *
 * This macro call defines and registers the implementation for handling
 * datetime preference items, mapping the functions for creation, value
 * setting/getting, etc.
 */
PREFS_ITEM_WIDGET_ADD(datetime,
                      supported_types,
                      elm_prefs_datetime_value_set,
                      elm_prefs_datetime_value_get,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL);
