#ifndef _ELM_SLIDER_EO_H_
#define _ELM_SLIDER_EO_H_

#ifndef _ELM_SLIDER_EO_CLASS_TYPE
#define _ELM_SLIDER_EO_CLASS_TYPE

/**
 * @brief Typedef for the Elm_Slider Eo class.
 *
 * This typedef provides a convenience name for the Elm_Slider Eo object.
 * It represents an instance of the Elementary slider widget.
 *
 * @ingroup Elm_Slider
 */
typedef Eo Elm_Slider;

#endif

#ifndef _ELM_SLIDER_EO_TYPES
#define _ELM_SLIDER_EO_TYPES


#endif
/**
 * @brief Elementary slider class.
 *
 * This macro provides a convenient way to get the Elm_Slider Efl_Class.
 * It is used internally and by applications to interact with the slider's class.
 *
 * @ingroup Elm_Slider
 */
#define ELM_SLIDER_CLASS elm_slider_class_get()

/**
 * @brief Get the Efl_Class for the Elm_Slider.
 *
 * This function returns the Efl_Class object associated with the Elm_Slider widget.
 * It is used for type checking and other class-level operations.
 *
 * @return The Efl_Class for Elm_Slider.
 * @ingroup Elm_Slider
 */
EWAPI const Efl_Class *elm_slider_class_get(void) EINA_CONST;

#endif
