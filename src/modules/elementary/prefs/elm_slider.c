#include "private.h"

/**
 * @internal
 * @brief Lists the data types supported by the slider widget in preferences.
 *
 * This array is used to register the slider widget with the preferences
 * system, indicating that it can handle integer and float values. The list
 * is terminated by ELM_PREFS_TYPE_UNKNOWN.
 */
static Elm_Prefs_Item_Type supported_types[] =
{
   ELM_PREFS_TYPE_INT,
   ELM_PREFS_TYPE_FLOAT,
   ELM_PREFS_TYPE_UNKNOWN
};

/**
 * @internal
 * @brief Callback for the EFL_UI_RANGE_EVENT_CHANGED event on the slider.
 *
 * This function acts as a bridge. It receives the EFL event and calls the
 * higher-level prefs item changed callback, passing the widget object.
 *
 * @param data The user-provided data, which is the
 *             Elm_Prefs_Item_Changed_Cb to be called.
 * @param event The Efl_Event details.
 */
static void
_item_changed_cb(void *data, const Efl_Event *event)
{
   Elm_Prefs_Item_Changed_Cb prefs_it_changed_cb = data;

   prefs_it_changed_cb(event->object);
}

/**
 * @internal
 * @brief Creates and configures a slider widget for a preferences item.
 *
 * This function is the factory for creating slider widgets used in the
 * preferences system. It initializes a standard Elm_Slider, sets its
 * range and default value based on the provided spec, and attaches a
 * value-changed callback.
 *
 * @param iface The prefs item interface (unused).
 * @param prefs The parent prefs widget.
 * @param type The data type for the slider (ELM_PREFS_TYPE_INT or
 *             ELM_PREFS_TYPE_FLOAT).
 * @param spec A union (Elm_Prefs_Item_Spec) containing type-specific
 *             parameters, like min/max range and default value.
 * @param cb The callback function to be invoked when the slider's value
 *           changes.
 * @return A new Evas_Object (slider widget) on success, or NULL on failure.
 */
static Evas_Object *
elm_prefs_slider_add(const Elm_Prefs_Item_Iface *iface EINA_UNUSED,
                     Evas_Object *prefs,
                     const Elm_Prefs_Item_Type type,
                     const Elm_Prefs_Item_Spec spec,
                     Elm_Prefs_Item_Changed_Cb cb)
{
   Evas_Object *obj = elm_slider_add(prefs);

   evas_object_data_set(obj, "prefs_type", (void *)type);

   efl_event_callback_add
     (obj, EFL_UI_RANGE_EVENT_CHANGED, _item_changed_cb, cb);
   if (type == ELM_PREFS_TYPE_INT)
     {
        elm_slider_unit_format_set(obj, "%1.0f");
        elm_slider_indicator_format_set(obj, "%1.0f");
        elm_slider_min_max_set(obj, spec.i.min, spec.i.max);
        elm_slider_value_set(obj, spec.i.def);
     }
   else if (type == ELM_PREFS_TYPE_FLOAT)
     {
        elm_slider_unit_format_set(obj, "%1.2f");
        elm_slider_indicator_format_set(obj, "%1.2f");
        elm_slider_min_max_set(obj, spec.f.min, spec.f.max);
        elm_slider_value_set(obj, spec.f.def);
     }

   return obj;
}

/**
 * @internal
 * @brief Sets the slider's value from an Eina_Value.
 *
 * This function updates the slider's position. It performs type checking to
 * ensure the Eina_Value type matches the slider's configured data type
 * (integer or float).
 *
 * @param obj The slider widget object.
 * @param value The Eina_Value containing the new value for the slider.
 *              Must be of type EINA_VALUE_TYPE_INT or EINA_VALUE_TYPE_FLOAT.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., type mismatch).
 */
static Eina_Bool
elm_prefs_slider_value_set(Evas_Object *obj,
                           Eina_Value *value)
{
   union
   {
      int   i;
      float f;
   } val;

   Elm_Prefs_Item_Type pt =
     (Elm_Prefs_Item_Type)evas_object_data_get(obj, "prefs_type");

   const Eina_Value_Type *vt = eina_value_type_get(value);
   if (!vt) return EINA_FALSE;

   if ((pt == ELM_PREFS_TYPE_INT) && (vt == EINA_VALUE_TYPE_INT))
     {
        eina_value_get(value, &(val.i));
        elm_slider_value_set(obj, val.i);
     }
   else if ((pt == ELM_PREFS_TYPE_FLOAT) && (vt == EINA_VALUE_TYPE_FLOAT))
     {
        eina_value_get(value, &(val.f));
        elm_slider_value_set(obj, val.f);
     }
   else
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Retrieves the slider's value and stores it in an Eina_Value.
 *
 * This function reads the current value from the slider and populates the
 * given Eina_Value with it. It sets the Eina_Value's type to match the
 * slider's configured data type.
 *
 * @param obj The slider widget object.
 * @param value A pointer to an Eina_Value to be populated with the slider's
 *              current value.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_slider_value_get(Evas_Object *obj,
                           Eina_Value *value)
{
   union
   {
      int   i;
      float f;
   } val;

   Elm_Prefs_Item_Type pt =
     (Elm_Prefs_Item_Type)evas_object_data_get(obj, "prefs_type");

   if (pt == ELM_PREFS_TYPE_INT)
     {
        val.i = elm_slider_value_get(obj);

        if (!eina_value_setup(value, EINA_VALUE_TYPE_INT)) return EINA_FALSE;
        if (!eina_value_set(value, val.i)) return EINA_FALSE;
     }
   else if (pt == ELM_PREFS_TYPE_FLOAT)
     {
        val.f = elm_slider_value_get(obj);

        if (!eina_value_setup(value, EINA_VALUE_TYPE_FLOAT)) return EINA_FALSE;
        if (!eina_value_set(value, val.f)) return EINA_FALSE;
     }
   else
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Registers the slider widget with the Elementary preferences system.
 *
 * This macro call creates the necessary interface structure and registers
 * the slider widget type with the name "slider". It links the factory function
 * (elm_prefs_slider_add) and value get/set handlers to the preferences system.
 */
PREFS_ITEM_WIDGET_ADD(slider,
                      supported_types,
                      elm_prefs_slider_value_set,
                      elm_prefs_slider_value_get,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL);
