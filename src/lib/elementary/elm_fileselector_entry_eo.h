#ifndef _ELM_FILESELECTOR_ENTRY_EO_H_
#define _ELM_FILESELECTOR_ENTRY_EO_H_

/**
 * @file
 * @brief These routines are routines to operate on Elementary Fileselector Entry objects.
 *
 * Elm_Fileselector_Entry is a widget that allows for file selection.
 * It can be used to browse the file system and select a file or directory.
 *
 * @ingroup Elm_Fileselector_Entry
 */

#ifndef _ELM_FILESELECTOR_ENTRY_EO_CLASS_TYPE
#define _ELM_FILESELECTOR_ENTRY_EO_CLASS_TYPE

/**
 * @brief Opaque handle to the Elm_Fileselector_Entry object.
 * @ingroup Elm_Fileselector_Entry
 */
typedef Eo Elm_Fileselector_Entry;

#endif

#ifndef _ELM_FILESELECTOR_ENTRY_EO_TYPES
#define _ELM_FILESELECTOR_ENTRY_EO_TYPES


#endif
/**
 * @brief Elementary fileselector entry class.
 *
 * This macro defines the Efl_Class for the Elm_Fileselector_Entry.
 *
 * @ingroup Elm_Fileselector_Entry
 */
#define ELM_FILESELECTOR_ENTRY_CLASS elm_fileselector_entry_class_get()

/**
 * @brief Get the Efl_Class for the Elm_Fileselector_Entry.
 *
 * @return The Efl_Class for the Elm_Fileselector_Entry.
 * @ingroup Elm_Fileselector_Entry
 */
EWAPI const Efl_Class *elm_fileselector_entry_class_get(void) EINA_CONST;

/**
 * @brief Event descriptor for the "changed" event.
 *
 * This event is emitted when the text in the entry part of the fileselector changes.
 * @ingroup Elm_Fileselector_Entry
 */
EWAPI extern const Efl_Event_Description _ELM_FILESELECTOR_ENTRY_EVENT_CHANGED;

/**
 * @brief Macro to access the "changed" event.
 *
 * This event is emitted when the text in the entry part of the fileselector changes.
 * @ingroup Elm_Fileselector_Entry
 */
#define ELM_FILESELECTOR_ENTRY_EVENT_CHANGED (&(_ELM_FILESELECTOR_ENTRY_EVENT_CHANGED))

/**
 * @brief Event descriptor for the "activated" event.
 *
 * This event is emitted when the entry is activated, usually by pressing Enter.
 * @ingroup Elm_Fileselector_Entry
 */
EWAPI extern const Efl_Event_Description _ELM_FILESELECTOR_ENTRY_EVENT_ACTIVATED;

/**
 * @brief Macro to access the "activated" event.
 *
 * This event is emitted when the entry is activated, usually by pressing Enter.
 * @ingroup Elm_Fileselector_Entry
 */
#define ELM_FILESELECTOR_ENTRY_EVENT_ACTIVATED (&(_ELM_FILESELECTOR_ENTRY_EVENT_ACTIVATED))

/**
 * @brief Event descriptor for the "file,chosen" event.
 *
 * This event is emitted when a file or directory is selected from the fileselector.
 * The event_info for this event will be the path (const char *) to the chosen item.
 * @ingroup Elm_Fileselector_Entry
 */
EWAPI extern const Efl_Event_Description _ELM_FILESELECTOR_ENTRY_EVENT_FILE_CHOSEN;

/**
 * @brief Macro to access the "file,chosen" event.
 *
 * This event is emitted when a file or directory is selected from the fileselector.
 * The event_info for this event will be the path (const char *) to the chosen item.
 * @ingroup Elm_Fileselector_Entry
 */
#define ELM_FILESELECTOR_ENTRY_EVENT_FILE_CHOSEN (&(_ELM_FILESELECTOR_ENTRY_EVENT_FILE_CHOSEN))

/**
 * @brief Event descriptor for the "press" event.
 *
 * This event is emitted when the entry is pressed.
 * @ingroup Elm_Fileselector_Entry
 */
EWAPI extern const Efl_Event_Description _ELM_FILESELECTOR_ENTRY_EVENT_PRESS;

/**
 * @brief Macro to access the "press" event.
 *
 * This event is emitted when the entry is pressed.
 * @ingroup Elm_Fileselector_Entry
 */
#define ELM_FILESELECTOR_ENTRY_EVENT_PRESS (&(_ELM_FILESELECTOR_ENTRY_EVENT_PRESS))

#endif
