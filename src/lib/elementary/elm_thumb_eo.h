#ifndef _ELM_THUMB_EO_H_
#define _ELM_THUMB_EO_H_

/**
 * @file
 * @brief These routines are bindings for the Elm_Thumb class.
 *
 * @if (!DOXYGEN_API_EVAS)
 * @warning This file should not be used externally.
 * @endif
 */

#ifndef _ELM_THUMB_EO_CLASS_TYPE
#define _ELM_THUMB_EO_CLASS_TYPE

/**
 * @brief Opaque handle to an Elm_Thumb object.
 * @ingroup Elm_Thumb
 */
typedef Eo Elm_Thumb;

#endif

#ifndef _ELM_THUMB_EO_TYPES
#define _ELM_THUMB_EO_TYPES

/**
 * @brief Represents possible thumbnail aspect options.
 * @ingroup Elm_Thumb
 */
typedef enum
{
  ELM_THUMB_ASPECT_PREFER_NONE = 0, /**< No preference for aspect ratio. */
  ELM_THUMB_ASPECT_PREFER_SOURCE, /**< Prefer source aspect ratio. */
  ELM_THUMB_ASPECT_PREFER_CUSTOM /**< Prefer custom aspect ratio. */
} Elm_Thumb_Aspect_Option;


#endif
/** Elementary thumbnail class
 *
 * @ingroup Elm_Thumb
 */
#define ELM_THUMB_CLASS elm_thumb_class_get()

/**
 * @brief Get the Efl_Class for Elm_Thumb.
 * @return The Efl_Class for Elm_Thumb.
 * @ingroup Elm_Thumb
 */
EWAPI const Efl_Class *elm_thumb_class_get(void) EINA_CONST;

/**
 * @brief Event descriptor for a thumbnail generation error.
 * @ingroup Elm_Thumb
 */
EWAPI extern const Efl_Event_Description _ELM_THUMB_EVENT_GENERATE_ERROR;

/**
 * @brief Macro for the #_ELM_THUMB_EVENT_GENERATE_ERROR event.
 * Called when an error occurred during thumbnail generation.
 * @ingroup Elm_Thumb
 */
#define ELM_THUMB_EVENT_GENERATE_ERROR (&(_ELM_THUMB_EVENT_GENERATE_ERROR))

/**
 * @brief Event descriptor for the start of thumbnail generation.
 * @ingroup Elm_Thumb
 */
EWAPI extern const Efl_Event_Description _ELM_THUMB_EVENT_GENERATE_START;

/**
 * @brief Macro for the #_ELM_THUMB_EVENT_GENERATE_START event.
 * Called when thumbnail generation started.
 * @ingroup Elm_Thumb
 */
#define ELM_THUMB_EVENT_GENERATE_START (&(_ELM_THUMB_EVENT_GENERATE_START))

/**
 * @brief Event descriptor for the end of thumbnail generation.
 * @ingroup Elm_Thumb
 */
EWAPI extern const Efl_Event_Description _ELM_THUMB_EVENT_GENERATE_STOP;

/**
 * @brief Macro for the #_ELM_THUMB_EVENT_GENERATE_STOP event.
 * Called when thumbnail generation stopped.
 * @ingroup Elm_Thumb
 */
#define ELM_THUMB_EVENT_GENERATE_STOP (&(_ELM_THUMB_EVENT_GENERATE_STOP))

/**
 * @brief Event descriptor for a thumbnail loading error.
 * @ingroup Elm_Thumb
 */
EWAPI extern const Efl_Event_Description _ELM_THUMB_EVENT_LOAD_ERROR;

/**
 * @brief Macro for the #_ELM_THUMB_EVENT_LOAD_ERROR event.
 * Called when an error occurred during loading.
 * @ingroup Elm_Thumb
 */
#define ELM_THUMB_EVENT_LOAD_ERROR (&(_ELM_THUMB_EVENT_LOAD_ERROR))

/**
 * @brief Event descriptor for a press event on the thumbnail.
 * @ingroup Elm_Thumb
 */
EWAPI extern const Efl_Event_Description _ELM_THUMB_EVENT_PRESS;

/**
 * @brief Macro for the #_ELM_THUMB_EVENT_PRESS event.
 * Called when pressed.
 * @ingroup Elm_Thumb
 */
#define ELM_THUMB_EVENT_PRESS (&(_ELM_THUMB_EVENT_PRESS))

#endif
