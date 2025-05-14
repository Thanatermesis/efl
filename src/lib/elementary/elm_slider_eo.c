/**
 * @brief Constructor for the Elm_Slider object.
 *
 * This function is called when a new Elm_Slider object is created.
 * It initializes the object's private data and sets up its initial state.
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the Elm_Slider object.
 * @return The constructed Efl_Object.
 */
Efl_Object *_elm_slider_efl_object_constructor(Eo *obj, Elm_Slider_Data *pd);

/**
 * @brief Destructor for the Elm_Slider object.
 *
 * This function is called when an Elm_Slider object is being destroyed.
 * It cleans up any resources allocated by the object.
 *
 * @param obj The Eo object to destruct.
 * @param pd The private data for the Elm_Slider object.
 */
void _elm_slider_efl_object_destructor(Eo *obj, Elm_Slider_Data *pd);

/**
 * @brief Calculates the size and position of the Elm_Slider object.
 *
 * This function is part of the Efl_Canvas_Group interface and is called
 * when the layout of the slider needs to be recalculated.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Slider object.
 */
void _elm_slider_efl_canvas_group_group_calculate(Eo *obj, Elm_Slider_Data *pd);

/**
 * @brief Applies the theme to the Elm_Slider object.
 *
 * This function is part of the Efl_Ui_Widget interface and is called
 * when the widget's theme needs to be applied or updated.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Slider object.
 * @return EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Error _elm_slider_efl_ui_widget_theme_apply(Eo *obj, Elm_Slider_Data *pd);

/**
 * @brief Handles input events for the Elm_Slider object.
 *
 * This function is part of the Efl_Ui_Widget interface and is called
 * to process input events such as mouse clicks or key presses.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Slider object.
 * @param eo_event The Efl_Event containing event details.
 * @param source The source Efl_Canvas_Object of the event.
 * @return EINA_TRUE if the event was handled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_slider_efl_ui_widget_widget_input_event_handler(Eo *obj, Elm_Slider_Data *pd, const Efl_Event *eo_event, Efl_Canvas_Object *source);

/**
 * @brief Updates the focus state of the Elm_Slider object.
 *
 * This function is part of the Efl_Ui_Focus_Object interface and is called
 * when the focus state of the object changes.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Slider object.
 * @return EINA_TRUE if the focus update was handled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_slider_efl_ui_focus_object_on_focus_update(Eo *obj, Elm_Slider_Data *pd);

/**
 * @brief Sets the text for the Elm_Slider object.
 *
 * This function is part of the Efl_Text interface and is used to set
 * the text content associated with the slider (e.g., a label or value display).
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Slider object.
 * @param text The text string to set.
 */
void _elm_slider_efl_text_text_set(Eo *obj, Elm_Slider_Data *pd, const char *text);

/**
 * @brief Gets the text from the Elm_Slider object.
 *
 * This function is part of the Efl_Text interface and is used to retrieve
 * the text content associated with the slider.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Slider object.
 * @return A pointer to the text string, or NULL if no text is set.
 */
const char *_elm_slider_efl_text_text_get(const Eo *obj, Elm_Slider_Data *pd);

/**
 * @brief Sets the markup text for the Elm_Slider object.
 *
 * This function is part of the Efl_Text_Markup interface and is used to set
 * text with Pango markup for rich text formatting.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Slider object.
 * @param markup The markup string to set.
 */
void _elm_slider_efl_text_markup_markup_set(Eo *obj, Elm_Slider_Data *pd, const char *markup);

/**
 * @brief Gets the markup text from the Elm_Slider object.
 *
 * This function is part of the Efl_Text_Markup interface and is used to retrieve
 * the markup text content associated with the slider.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Slider object.
 * @return A pointer to the markup string, or NULL if no markup is set.
 */
const char *_elm_slider_efl_text_markup_markup_get(const Eo *obj, Elm_Slider_Data *pd);

/**
 * @brief Sets the format callback function for the Elm_Slider object.
 *
 * This function is part of the Efl_Ui_Format interface and allows setting a
 * custom function to format the slider's value for display.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Slider object.
 * @param func_data User data to be passed to the format function.
 * @param func The Efl_Ui_Format_Func callback function.
 * @param func_free_cb Callback to free func_data when no longer needed.
 */
void _elm_slider_efl_ui_format_format_cb_set(Eo *obj, Elm_Slider_Data *pd, void *func_data, Efl_Ui_Format_Func func, Eina_Free_Cb func_free_cb);

/**
 * @brief Sets the translatable text for the Elm_Slider object.
 *
 * This function is part of the Efl_Ui_L10n interface and is used to set
 * a translatable string for the slider, specifying the text and its domain.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Slider object.
 * @param label The translatable text (label).
 * @param domain The localization domain for the text.
 */
void _elm_slider_efl_ui_l10n_l10n_text_set(Eo *obj, Elm_Slider_Data *pd, const char *label, const char *domain);

/**
 * @brief Gets the translatable text from the Elm_Slider object.
 *
 * This function is part of the Efl_Ui_L10n interface and is used to retrieve
 * the translatable string and its domain.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Slider object.
 * @param domain Pointer to store the localization domain.
 * @return A pointer to the translatable text string.
 */
const char *_elm_slider_efl_ui_l10n_l10n_text_get(const Eo *obj, Elm_Slider_Data *pd, const char **domain);

/**
 * @brief Gets a part of the Elm_Slider object.
 *
 * This function is part of the Efl_Part interface and is used to retrieve
 * a named part (sub-object) of the slider widget.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Slider object.
 * @param name The name of the part to retrieve.
 * @return The Efl_Object representing the part, or NULL if not found.
 */
Efl_Object *_elm_slider_efl_part_part_get(const Eo *obj, Elm_Slider_Data *pd, const char *name);

/**
 * @brief Applies the formatted value to the Elm_Slider object.
 *
 * This function is part of the Efl_Ui_Format interface. It triggers the
 * formatting of the current value using the configured format function or
 * default formatting and updates the display.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Slider object.
 */
void _elm_slider_efl_ui_format_apply_formatted_value(Eo *obj, Elm_Slider_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Slider class.
 *
 * This function is called once when the Elm_Slider class is being set up.
 * It defines the operations (member functions) for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_slider_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_SLIDER_EXTRA_OPS
#define ELM_SLIDER_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_slider_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_slider_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_calculate, _elm_slider_efl_canvas_group_group_calculate),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_slider_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_slider_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_on_focus_update, _elm_slider_efl_ui_focus_object_on_focus_update),
      EFL_OBJECT_OP_FUNC(efl_text_set, _elm_slider_efl_text_text_set),
      EFL_OBJECT_OP_FUNC(efl_text_get, _elm_slider_efl_text_text_get),
      EFL_OBJECT_OP_FUNC(efl_text_markup_set, _elm_slider_efl_text_markup_markup_set),
      EFL_OBJECT_OP_FUNC(efl_text_markup_get, _elm_slider_efl_text_markup_markup_get),
      EFL_OBJECT_OP_FUNC(efl_ui_format_func_set, _elm_slider_efl_ui_format_format_cb_set),
      EFL_OBJECT_OP_FUNC(efl_ui_format_apply_formatted_value, _elm_slider_efl_ui_format_apply_formatted_value),
      EFL_OBJECT_OP_FUNC(efl_ui_l10n_text_set, _elm_slider_efl_ui_l10n_l10n_text_set),
      EFL_OBJECT_OP_FUNC(efl_ui_l10n_text_get, _elm_slider_efl_ui_l10n_l10n_text_get),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_slider_efl_part_part_get),
      ELM_SLIDER_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Structure describing the Elm_Slider class.
 *
 * This structure provides metadata for the Elm_Slider class,
 * including its version, name, type, size of instance data,
 * and pointers to initializer and constructor functions.
 */
static const Efl_Class_Description _elm_slider_class_desc = {
   EO_VERSION, /**< EO_VERSION */
   "Elm.Slider", /**< Class name */
   EFL_CLASS_TYPE_REGULAR, /**< Class type */
   sizeof(Elm_Slider_Data), /**< Size of instance data */
   _elm_slider_class_initializer, /**< Class initializer function */
   _elm_slider_class_constructor, /**< Class constructor function */
   NULL /**< Class destructor function (if any) */
};

/**
 * @brief Defines the Elm_Slider class.
 *
 * This macro is used to define the Elm_Slider Efl_Class, associating it
 * with its class description, parent class, and any mixins it incorporates.
 *
 * - `elm_slider_class_get`: The function to retrieve the class.
 * - `&_elm_slider_class_desc`: Pointer to the class description structure.
 * - `EFL_UI_LAYOUT_BASE_CLASS`: The parent class.
 * - `ELM_LAYOUT_MIXIN`: A mixin providing layout capabilities.
 * - `EFL_UI_LEGACY_INTERFACE`: Mixin for legacy UI compatibility.
 * - `EFL_TEXT_INTERFACE`: Mixin for basic text handling.
 * - `EFL_TEXT_MARKUP_INTERFACE`: Mixin for markup text handling.
 * - `EFL_UI_FORMAT_MIXIN`: Mixin for value formatting.
 * - `NULL`: Terminator for the list of mixins/interfaces.
 */
EFL_DEFINE_CLASS(elm_slider_class_get, &_elm_slider_class_desc, EFL_UI_LAYOUT_BASE_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, EFL_TEXT_INTERFACE, EFL_TEXT_MARKUP_INTERFACE, EFL_UI_FORMAT_MIXIN, NULL);
