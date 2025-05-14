#include "private.h"
#include "elm_spinner_eo.h"

/**
 * @internal
 * @brief An array defining the supported data types for the spinner prefs item.
 *
 * This widget can handle integer and floating-point values.
 */
static Elm_Prefs_Item_Type supported_types[] =
{
   ELM_PREFS_TYPE_INT,
   ELM_PREFS_TYPE_FLOAT,
   ELM_PREFS_TYPE_UNKNOWN
};

/**
 * @internal
 * @brief Callback function invoked when the spinner's value changes.
 *
 * This function acts as a bridge. It receives an Efl_Event and invokes the
 * user-provided Elm_Prefs_Item_Changed_Cb callback, passing the spinner
 * object itself.
 *
 * @param data The user-provided callback function (Elm_Prefs_Item_Changed_Cb).
 * @param event The event information structure.
 */
static void
_item_changed_cb(void *data, const Efl_Event *event)
{
    Elm_Prefs_Item_Changed_Cb prefs_it_changed_cb = data;

    prefs_it_changed_cb(event->object);
}

/**
 * @internal
 * @brief Creates and configures a spinner widget for a prefs item.
 *
 * This function is the factory for creating spinner widgets used within the
 * prefs system. It initializes the spinner based on the specified type (INT
 * or FLOAT) and parameters.
 *
 * @param iface The prefs item interface (unused).
 * @param prefs The parent prefs widget.
 * @param type The data type for the spinner (ELM_PREFS_TYPE_INT or
 *        ELM_PREFS_TYPE_FLOAT).
 * @param spec A union (Elm_Prefs_Item_Spec) containing type-specific
 *        configuration like min, max, and default values.
 * @param cb A callback function to be invoked when the spinner's value changes.
 * @return The newly created spinner Evas_Object.
 */
static Evas_Object *
elm_prefs_spinner_add(const Elm_Prefs_Item_Iface *iface EINA_UNUSED,
                      Evas_Object *prefs,
                      const Elm_Prefs_Item_Type type,
                      const Elm_Prefs_Item_Spec spec,
                      Elm_Prefs_Item_Changed_Cb cb)
{
   Evas_Object *obj = elm_spinner_add(prefs);

   evas_object_data_set(obj, "prefs_type", (void *)type);

   efl_event_callback_add
     (obj, ELM_SPINNER_EVENT_CHANGED, _item_changed_cb, cb);

   if (type == ELM_PREFS_TYPE_INT)
     {
        elm_spinner_step_set(obj, 1.0);
        elm_spinner_min_max_set(obj, spec.i.min, spec.i.max);
        elm_spinner_value_set(obj, spec.i.def);
     }
   else if (type == ELM_PREFS_TYPE_FLOAT)
     {
        elm_spinner_label_format_set(obj, "%1.2f");
        elm_spinner_step_set(obj, 0.1);
        elm_spinner_min_max_set(obj, spec.f.min, spec.f.max);
        elm_spinner_value_set(obj, spec.f.def);
     }

   return obj;
}

/**
 * @internal
 * @brief Sets the spinner's value from an Eina_Value.
 *
 * This function validates the incoming Eina_Value against the spinner's
 * configured data type and, if compatible, updates the spinner's displayed
 * value.
 *
 * @param obj The spinner widget.
 * @param value An Eina_Value containing the new value. It must match the
 *        spinner's data type (INT or FLOAT).
 * @return EINA_TRUE on success, EINA_FALSE on type mismatch or other errors.
 */
static Eina_Bool
elm_prefs_spinner_value_set(Evas_Object *obj,
                            Eina_Value *value)
{
   union
   {
      int   i;
      float f;
   } val;

   Elm_Prefs_Item_Type pt =
     (Elm_Prefs_Item_Type) evas_object_data_get(obj, "prefs_type");

   const Eina_Value_Type *vt = eina_value_type_get(value);
   if (!vt) return EINA_FALSE;

   if ((pt == ELM_PREFS_TYPE_INT) && (vt == EINA_VALUE_TYPE_INT))
     {
        eina_value_get(value, &(val.i));
        elm_spinner_value_set(obj, val.i);
     }
   else if ((pt == ELM_PREFS_TYPE_FLOAT) && (vt == EINA_VALUE_TYPE_FLOAT))
     {
        eina_value_get(value, &(val.f));
        elm_spinner_value_set(obj, val.f);
     }
   else
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Retrieves the spinner's current value into an Eina_Value.
 *
 * This function reads the numeric value from the spinner widget and populates
 * the provided Eina_Value structure with it, ensuring the type is set
 * correctly (INT or FLOAT).
 *
 * @param obj The spinner widget.
 * @param value A pointer to an Eina_Value to be populated with the spinner's
 *        current value.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_spinner_value_get(Evas_Object *obj,
                            Eina_Value *value)
{
   union
   {
      int   i;
      float f;
   } val;

   Elm_Prefs_Item_Type pt =
     (Elm_Prefs_Item_Type) evas_object_data_get(obj, "prefs_type");

   if (pt == ELM_PREFS_TYPE_INT)
     {
        val.i = elm_spinner_value_get(obj);

        if (!eina_value_setup(value, EINA_VALUE_TYPE_INT)) return EINA_FALSE;
        if (!eina_value_set(value, val.i)) return EINA_FALSE;
     }
   else if (pt == ELM_PREFS_TYPE_FLOAT)
      {
        val.f = elm_spinner_value_get(obj);

        if (!eina_value_setup(value, EINA_VALUE_TYPE_FLOAT)) return EINA_FALSE;
        if (!eina_value_set(value, val.f)) return EINA_FALSE;
      }
   else
     return EINA_FALSE;

   return EINA_TRUE;
}

PREFS_ITEM_WIDGET_ADD(spinner,
                      supported_types,
                      elm_prefs_spinner_value_set,
                      elm_prefs_spinner_value_get,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL);
