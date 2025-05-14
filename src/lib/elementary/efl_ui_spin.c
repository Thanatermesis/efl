#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_ACCESS_WIDGET_ACTION_PROTECTED
#define EFL_UI_FORMAT_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "efl_ui_spin_private.h"

#define MY_CLASS EFL_UI_SPIN_CLASS

#define MY_CLASS_NAME "Efl.Ui.Spin"

/**
 * @internal
 * @brief Formats and sets the current value as the text label of the spin widget.
 *
 * This function takes the current value from the spin data, formats it
 * using efl_ui_format_formatted_value_get, and then sets the resulting
 * string as the text of the "efl.text" part of the spin widget.
 *
 * @param obj The Evas_Object (spin widget) whose label is to be updated.
 * @param sd Pointer to the Efl_Ui_Spin_Data structure containing the current value.
 */
static void
_label_write(Evas_Object *obj, Efl_Ui_Spin_Data *sd)
{
   Eina_Strbuf *strbuf = eina_strbuf_new();
   Eina_Value val = eina_value_double_init(sd->val);
   efl_ui_format_formatted_value_get(obj, strbuf, val);

   efl_text_set(efl_part(obj, "efl.text"), eina_strbuf_string_get(strbuf));

   eina_value_flush(&val);
   eina_strbuf_free(strbuf);
}

/**
 * @internal
 * @brief Constructor for the Efl.Ui.Spin widget.
 *
 * Initializes the spin widget, sets its default theme,
 * initializes default range values, applies the theme style,
 * writes the initial label, and enables focus.
 *
 * @param obj The Evas_Object to construct.
 * @param sd Pointer to the Efl_Ui_Spin_Data structure for this widget.
 * @return The constructed Evas_Object, or NULL on failure.
 */
EOLIAN static Eo *
_efl_ui_spin_efl_object_constructor(Eo *obj, Efl_Ui_Spin_Data *sd)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "spin");
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   sd->val_max = 100.0;

   if (elm_widget_theme_object_set(obj, wd->resize_obj,
                                       elm_widget_theme_klass_get(obj),
                                       elm_widget_theme_element_get(obj),
                                       elm_widget_theme_style_get(obj)) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     CRI("Failed to set layout!");

   _label_write(obj, sd);
   efl_ui_widget_focus_allow_set(obj, EINA_TRUE);

   return obj;
}

/**
 * @internal
 * @brief Destructor for the Efl.Ui.Spin widget.
 *
 * Cleans up resources used by the spin widget.
 *
 * @param obj The Evas_Object to destruct.
 * @param sd Pointer to the Efl_Ui_Spin_Data structure (unused in this function).
 */
EOLIAN static void
_efl_ui_spin_efl_object_destructor(Eo *obj, Efl_Ui_Spin_Data *sd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Applies the current formatting to the spin widget's value display.
 *
 * This function is typically called when the formatting string or locale changes.
 * It re-formats and updates the displayed value.
 *
 * @param obj The Evas_Object (spin widget).
 * @param sd Pointer to the Efl_Ui_Spin_Data structure.
 */
EOLIAN static void
_efl_ui_spin_efl_ui_format_apply_formatted_value(Eo *obj, Efl_Ui_Spin_Data *sd EINA_UNUSED)
{
   _label_write(obj, sd);
   efl_canvas_group_change(obj);
}

/**
 * @internal
 * @brief Sets the minimum and maximum limits for the spin widget's range.
 *
 * Validates that min is not greater than max and that they are not equal.
 * Updates the internal min/max values and adjusts the current value if it
 * falls outside the new range. Finally, updates the displayed label.
 *
 * @param obj The Evas_Object (spin widget).
 * @param sd Pointer to the Efl_Ui_Spin_Data structure.
 * @param min The minimum value for the range.
 * @param max The maximum value for the range.
 */
EOLIAN static void
_efl_ui_spin_efl_ui_range_display_range_limits_set(Eo *obj, Efl_Ui_Spin_Data *sd, double min, double max)
{
   if (max < min)
     {
        ERR("Wrong params. min(%lf) is greater than max(%lf).", min, max);
        return;
     }
   if (EINA_DBL_EQ(max, min))
     {
        ERR("min and max must have a different value");
        return;
     }
   if ((EINA_DBL_EQ(sd->val_min, min)) && (EINA_DBL_EQ(sd->val_max, max))) return;

   sd->val_min = min;
   sd->val_max = max;

   if (sd->val < sd->val_min) sd->val = sd->val_min;
   if (sd->val > sd->val_max) sd->val = sd->val_max;

   _label_write(obj, sd);
}

/**
 * @internal
 * @brief Retrieves the minimum and maximum limits of the spin widget's range.
 *
 * @param obj The Evas_Object (spin widget, unused in this function).
 * @param sd Pointer to the Efl_Ui_Spin_Data structure containing the range limits.
 * @param min Pointer to a double where the minimum value will be stored. Can be NULL.
 * @param max Pointer to a double where the maximum value will be stored. Can be NULL.
 */
EOLIAN static void
_efl_ui_spin_efl_ui_range_display_range_limits_get(const Eo *obj EINA_UNUSED, Efl_Ui_Spin_Data *sd, double *min, double *max)
{
   if (min) *min = sd->val_min;
   if (max) *max = sd->val_max;
}

/**
 * @internal
 * @brief Sets the current value of the spin widget.
 *
 * Validates that the new value is within the defined min/max range.
 * If the value changes, it updates the internal value, emits "min,reached"
 * or "max,reached" events if the new value is at a limit, emits a "changed"
 * event, and updates the displayed label.
 *
 * @param obj The Evas_Object (spin widget).
 * @param sd Pointer to the Efl_Ui_Spin_Data structure.
 * @param val The new value to set.
 */
EOLIAN static void
_efl_ui_spin_efl_ui_range_display_range_value_set(Eo *obj, Efl_Ui_Spin_Data *sd, double val)
{
   if (val < sd->val_min)
     {
        ERR("Error, value is less than minimum");
        return;
     }

   if (val > sd->val_max)
     {
        ERR("Error, value is greater than maximum");
        return;
     }

   if (EINA_DBL_EQ(val, sd->val)) return;

   sd->val = val;

   if (EINA_DBL_EQ(sd->val, sd->val_min))
     efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_MIN_REACHED, NULL);
   else if (EINA_DBL_EQ(sd->val, sd->val_max))
     efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_MAX_REACHED, NULL);

   efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_CHANGED, NULL);

   _label_write(obj, sd);
}

/**
 * @internal
 * @brief Retrieves the current value of the spin widget.
 *
 * @param obj The Evas_Object (spin widget, unused in this function).
 * @param sd Pointer to the Efl_Ui_Spin_Data structure containing the current value.
 * @return The current value of the spin widget.
 */
EOLIAN static double
_efl_ui_spin_efl_ui_range_display_range_value_get(const Eo *obj EINA_UNUSED, Efl_Ui_Spin_Data *sd)
{
   return sd->val;
}

#include "efl_ui_spin.eo.c"
