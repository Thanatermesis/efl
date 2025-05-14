
/**
 * @internal
 * @brief Sets the visible mode of the indicator part of the slider.
 *
 * This function is the internal implementation for setting the visibility mode
 * of the slider's indicator.
 *
 * @param[in] obj The Evas object.
 * @param[in] pd The private data for the object.
 * @param[in] mode The desired visibility mode.
 *                 Example: ELM_SLIDER_INDICATOR_VISIBLE_MODE_ALWAYS
 */
void _elm_slider_part_indicator_visible_mode_set(Eo *obj, void *pd, Elm_Slider_Indicator_Visible_Mode mode);

EOAPI EFL_VOID_FUNC_BODYV(elm_slider_part_indicator_visible_mode_set, EFL_FUNC_CALL(mode), Elm_Slider_Indicator_Visible_Mode mode);

/**
 * @internal
 * @brief Gets the visible mode of the indicator part of the slider.
 *
 * This function is the internal implementation for retrieving the visibility mode
 * of the slider's indicator.
 *
 * @param[in] obj The Evas object.
 * @param[in] pd The private data for the object.
 * @return The current visibility mode.
 *         Example: ELM_SLIDER_INDICATOR_VISIBLE_MODE_ON_FOCUS
 */
Elm_Slider_Indicator_Visible_Mode _elm_slider_part_indicator_visible_mode_get(const Eo *obj, void *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_slider_part_indicator_visible_mode_get, Elm_Slider_Indicator_Visible_Mode, 0);

/**
 * @internal
 * @brief Sets the format callback function for the slider indicator.
 *
 * This function allows a custom callback to be set for formatting the indicator's text.
 *
 * @param[in] obj The Evas object.
 * @param[in] pd The private data for the object.
 * @param[in] func_data User data to be passed to the callback function.
 * @param[in] func The callback function for formatting.
 * @param[in] func_free_cb Callback function to free func_data when no longer needed.
 */
void _elm_slider_part_indicator_efl_ui_format_format_cb_set(Eo *obj, void *pd, void *func_data, Efl_Ui_Format_Func func, Eina_Free_Cb func_free_cb);

/**
 * @internal
 * @brief Sets the format string for the slider indicator.
 *
 * This function sets a template string used to format the indicator's text.
 *
 * @param[in] obj The Evas object.
 * @param[in] _pd The private data for the object.
 * @param[in] template The format string template. Example: "Value: %.2f"
 * @param[in] type The type of format string.
 */
void _elm_slider_part_indicator_efl_ui_format_format_string_set(Eo *obj, void *_pd, const char *template, Efl_Ui_Format_String_Type type);

/**
 * @internal
 * @brief Gets the format string for the slider indicator.
 *
 * This function retrieves the template string used to format the indicator's text.
 *
 * @param[in] obj The Evas object.
 * @param[in] _pd The private data for the object.
 * @param[out] template Pointer to store the format string template.
 * @param[out] type Pointer to store the type of format string.
 */
void _elm_slider_part_indicator_efl_ui_format_format_string_get (const Eo *obj, void *_pd, const char **template, Efl_Ui_Format_String_Type *type);

/**
 * @internal
 * @brief Applies the current formatting to the slider indicator's value.
 *
 * This function is called to update the indicator's text based on the
 * current value and formatting rules (either callback or string).
 *
 * @param[in] obj The Evas object.
 * @param[in] pd The private data for the Elm_Part_Data.
 */
void _elm_slider_part_indicator_efl_ui_format_apply_formatted_value(Eo *obj, Elm_Part_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Slider_Part_Indicator class.
 *
 * This function is called once when the class is first created. It sets up
 * the operations (methods) for the class.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_slider_part_indicator_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_SLIDER_PART_INDICATOR_EXTRA_OPS
#define ELM_SLIDER_PART_INDICATOR_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_slider_part_indicator_visible_mode_set, _elm_slider_part_indicator_visible_mode_set),
      EFL_OBJECT_OP_FUNC(elm_slider_part_indicator_visible_mode_get, _elm_slider_part_indicator_visible_mode_get),
      EFL_OBJECT_OP_FUNC(efl_ui_format_func_set, _elm_slider_part_indicator_efl_ui_format_format_cb_set),
      EFL_OBJECT_OP_FUNC(efl_ui_format_string_set, _elm_slider_part_indicator_efl_ui_format_format_string_set),
      EFL_OBJECT_OP_FUNC(efl_ui_format_string_get, _elm_slider_part_indicator_efl_ui_format_format_string_get),
      EFL_OBJECT_OP_FUNC(efl_ui_format_apply_formatted_value, _elm_slider_part_indicator_efl_ui_format_apply_formatted_value),
      ELM_SLIDER_PART_INDICATOR_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Slider_Part_Indicator class.
 *
 * This static constant structure holds metadata about the Elm_Slider_Part_Indicator class,
 * such as its version, name, type, and initializer functions.
 */
static const Efl_Class_Description _elm_slider_part_indicator_class_desc = {
   EO_VERSION, /**< The EO API version for this class. */
   "Elm.Slider_Part_Indicator", /**< The name of the class. */
   EFL_CLASS_TYPE_REGULAR, /**< The type of the class (regular, abstract, mixin, etc.). */
   0,
   _elm_slider_part_indicator_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_slider_part_indicator_class_get, &_elm_slider_part_indicator_class_desc, EFL_UI_LAYOUT_PART_CLASS, EFL_UI_LEGACY_INTERFACE, EFL_UI_FORMAT_MIXIN, NULL);
