/**
 * @file
 * @brief Defines the Evas Object (Eo) API for the Elementary Fileselector Button widget.
 *
 * This header provides the type definitions, class retrieval function, and event
 * descriptions related to the Elm_Fileselector_Button Eo class.
 */
#ifndef _ELM_FILESELECTOR_BUTTON_EO_H_
#define _ELM_FILESELECTOR_BUTTON_EO_H_

#ifndef _ELM_FILESELECTOR_BUTTON_EO_CLASS_TYPE
#define _ELM_FILESELECTOR_BUTTON_EO_CLASS_TYPE

/**
 * @brief The Evas Object (Eo) type for an Elementary Fileselector Button.
 * @ingroup Elm_Fileselector_Button
 */
typedef Eo Elm_Fileselector_Button;

#endif

#ifndef _ELM_FILESELECTOR_BUTTON_EO_TYPES
#define _ELM_FILESELECTOR_BUTTON_EO_TYPES


#endif
/** Elementary fileselector button class
 *
 * @ingroup Elm_Fileselector_Button
 */
#define ELM_FILESELECTOR_BUTTON_CLASS elm_fileselector_button_class_get()

/**
 * @brief Retrieves the Efl_Class definition for the Elm_Fileselector_Button class.
 *
 * @return The Efl_Class for Elm_Fileselector_Button.
 * @ingroup Elm_Fileselector_Button
 */
EWAPI const Efl_Class *elm_fileselector_button_class_get(void) EINA_CONST;

/**
 * @brief Event description for the "file,chosen" event.
 *
 * This event is triggered when a file is selected via the fileselector button's
 * associated fileselector dialog. The event information typically includes the path
 * to the chosen file.
 *
 * @see ELM_FILESELECTOR_BUTTON_EVENT_FILE_CHOSEN
 * @ingroup Elm_Fileselector_Button
 */
EWAPI extern const Efl_Event_Description _ELM_FILESELECTOR_BUTTON_EVENT_FILE_CHOSEN;

/** Called when a file was chosen in the fileselector
 *
 * @ingroup Elm_Fileselector_Button
 */
#define ELM_FILESELECTOR_BUTTON_EVENT_FILE_CHOSEN (&(_ELM_FILESELECTOR_BUTTON_EVENT_FILE_CHOSEN))

#endif
