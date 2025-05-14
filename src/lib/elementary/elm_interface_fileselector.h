/**
 * @file
 * @brief This file defines the Efl Gen Interface for fileselector widgets
 *        within the Elementary toolkit. It includes declarations for
 *        internal functions used to manage fileselector states such as
 *        current path, selected items, and interactions with UI elements
 *        like entry fields and buttons. It also provides a macro for
 *        interface adherence checking.
 */
#ifndef ELM_INTEFARCE_FILESELECTOR_H
#define ELM_INTEFARCE_FILESELECTOR_H

#ifndef EFL_NOLEGACY_API_SUPPORT
#include "elm_interface_fileselector_eo.legacy.h"
#endif

/**
 * @brief Macro to check if an object implements the Elm_Interface_Fileselector.
 *
 * If the object does not implement the interface, an error is logged,
 * and if the ELM_ERROR_ABORT environment variable is set, the program aborts.
 * Otherwise, it returns the specified value(s) from __VA_ARGS__.
 *
 * @param obj The object to check.
 * @param ... Values to return if the check fails and not aborting.
 */
#define ELM_FILESELECTOR_INTERFACE_CHECK(obj, ...) \
  if (EINA_UNLIKELY(!efl_isa(obj, ELM_INTERFACE_FILESELECTOR_INTERFACE))) \
    { \
       ERR("The object (%p) doesn't implement the Elementary fileselector" \
            " interface", obj); \
       if (getenv("ELM_ERROR_ABORT")) abort(); \
       return __VA_ARGS__; \
    }

/**
 * @internal
 * @brief Sets the current directory being browsed by the fileselector.
 *
 * This function updates the internal state of the fileselector to display
 * the contents of the specified @p path. For example, if the fileselector
 * is showing files in "/home/user/", calling this with "/home/user/documents/"
 * will change the view to the "documents" directory.
 *
 * @param obj The fileselector object.
 * @param path The absolute directory path to navigate to.
 *             Example: "/usr/local/lib"
 */
void
_elm_fileselector_path_set_internal(Evas_Object *obj, const char *path);

/**
 * @internal
 * @brief Gets the current directory being browsed by the fileselector.
 *
 * This retrieves the path that was last set by
 * _elm_fileselector_path_set_internal() or through user interaction
 * that changes the browsed directory.
 *
 * @param obj The fileselector object.
 * @return The current absolute directory path (e.g., "/var/log/").
 *         The returned string is an interned string and should not be freed.
 *         Returns @c NULL on failure or if no path is set.
 */
const char *
_elm_fileselector_path_get_internal(const Evas_Object *obj);

/**
 * @internal
 * @brief Sets the final selected file or directory path for the fileselector.
 *
 * This function is used to programmatically set the item that is considered
 * "selected" by the fileselector, as if the user had chosen it.
 * This is typically a full path to a file or directory.
 *
 * @param obj The fileselector object.
 * @param _path The absolute path of the file or directory to be marked as selected.
 *              Example: "/home/user/file.txt" or "/home/user/mydir"
 * @return @c EINA_TRUE on success, @c EINA_FALSE if the path could not be set
 *         (e.g., invalid path, or fileselector mode does not allow it).
 */
Eina_Bool
_elm_fileselector_selected_set_internal(Evas_Object *obj, const char *_path);

/**
 * @internal
 * @brief Gets the currently selected file or directory path.
 *
 * This retrieves the path that has been confirmed as the selection,
 * either programmatically via _elm_fileselector_selected_set_internal()
 * or through user interaction (e.g., double-clicking a file, pressing "Ok").
 *
 * @param obj The fileselector object.
 * @return The absolute path of the selected file or directory (e.g., "/etc/hosts").
 *         The returned string is an interned string and should not be freed.
 *         Returns @c NULL if nothing is currently selected or on failure.
 */
const char *
_elm_fileselector_selected_get_internal(const Evas_Object *obj);

/**
 * @internal
 * @brief Gets the list of currently selected paths when multi-selection is enabled.
 *
 * If the fileselector allows selecting multiple items, this function returns
 * a list of all currently selected absolute paths.
 *
 * @param obj The fileselector object.
 * @return A read-only Eina_List of Eina_Stringshare instances, where each string is an
 *         absolute path to a selected file or directory.
 *         Example:
 *         If "/path/to/file1.txt" and "/path/to/folderA" are selected,
 *         the list would contain these two strings.
 *         The list itself and its string contents are read-only and should not be modified or freed by the caller.
 *         The strings are interned.
 *         Returns @c NULL if multi-selection is not active, nothing is selected, or on failure.
 */
const Eina_List *
_elm_fileselector_selected_paths_get_internal(const Evas_Object* obj);

/**
 * @internal
 * @brief Sets the text content of the path entry field in the fileselector.
 *
 * This function updates the text input field where users can typically type
 * or see the current path or selected file name. This might not necessarily
 * navigate the fileselector or make a selection, but only updates the visual entry.
 *
 * @param obj The fileselector object.
 * @param path The path string to display in the entry field.
 *             Example: "/home/user/new_folder"
 */
void
_elm_fileselector_entry_path_set_internal(Evas_Object *obj, const char *path);

/**
 * @internal
 * @brief Gets the text content currently displayed in the fileselector's path entry field.
 *
 * This retrieves the raw string from the text input field. This value might be
 * a partial path, a full path, or a filename, depending on user input or
 * previous calls to _elm_fileselector_entry_path_set_internal().
 *
 * @param obj The fileselector object.
 * @return The current text content of the path entry field (e.g., "image.png" or "/tmp/").
 *         The returned string is an interned string and should not be freed.
 *         Returns @c NULL on failure or if the entry is empty.
 */
const char *
_elm_fileselector_entry_path_get_internal(const Evas_Object *obj);

/**
 * @internal
 * @brief Sets the selected path based on the content of the fileselector's entry field.
 *
 * This function is typically called when the path entered or displayed in the
 * text entry field is confirmed as a selection (e.g., user presses Enter in the entry).
 * It attempts to validate and apply this path as the fileselector's current selection.
 *
 * @param obj The fileselector object.
 * @param path The absolute path (usually derived from the entry field) to set as selected.
 *             Example: "/home/user/report.pdf"
 * @return @c EINA_TRUE on successful selection, @c EINA_FALSE otherwise (e.g., invalid path).
 */
Eina_Bool
_elm_fileselector_entry_selected_set_internal(Evas_Object *obj, const char *path);

/**
 * @internal
 * @brief Gets the path that was last successfully selected via the fileselector's entry field.
 *
 * This retrieves the path that was confirmed through the entry field, for instance,
 * after a call to _elm_fileselector_entry_selected_set_internal() or equivalent user action.
 *
 * @param obj The fileselector object.
 * @return The absolute path selected via the entry (e.g., "/usr/share/applications/app.desktop").
 *         The returned string is an interned string and should not be freed.
 *         Returns @c NULL if no path has been selected via the entry or on failure.
 */
const char *
_elm_fileselector_entry_selected_get_internal(const Evas_Object *obj);

/**
 * @internal
 * @brief Sets a path associated with a fileselector button (e.g., an "Ok" or "Save" button).
 *
 * This function might be used to pre-fill or suggest a path/filename when a
 * fileselector dialog is opened, often displayed near or on the confirmation button.
 * For example, in a "Save As" dialog, this could set the initial suggested filename.
 *
 * @param obj The fileselector object.
 * @param path The path to associate with the button, often a full suggested save path.
 *             Example: "/home/user/Pictures/my_drawing.png"
 */
void
_elm_fileselector_button_path_set_internal(Evas_Object *obj, const char *path);

/**
 * @internal
 * @brief Gets the path currently associated with a fileselector button.
 *
 * This retrieves the path that might be displayed by or contextually linked to
 * a fileselector's confirmation button. This could be a default save name or
 * the currently selected item's path shown on the button.
 *
 * @param obj The fileselector object.
 * @return The path associated with the button (e.g., "Untitled.odt").
 *         The returned string is an interned string and should not be freed.
 *         Returns @c NULL if no path is associated or on failure.
 */
const char *
_elm_fileselector_button_path_get_internal(const Evas_Object *obj);

/**
 * @internal
 * @brief Sets the selected path, typically triggered by activating a fileselector button.
 *
 * This function is called when a confirmation action (e.g., clicking "Open" or "Save")
 * occurs, using the provided @p _path as the selected item.
 *
 * @param obj The fileselector object.
 * @param _path The absolute path to be confirmed as the selection.
 *              Example: "/mnt/data/archive.zip"
 * @return @c EINA_TRUE on successful selection, @c EINA_FALSE otherwise.
 */
Eina_Bool
_elm_fileselector_button_selected_set_internal(Evas_Object *obj, const char *_path);

/**
 * @internal
 * @brief Gets the path that was last selected via a fileselector button action.
 *
 * This retrieves the path confirmed by a button press, for example, after
 * _elm_fileselector_button_selected_set_internal() is successfully called or
 * an equivalent user interaction.
 *
 * @param obj The fileselector object.
 * @return The absolute path selected via a button action (e.g., "/opt/app/resource.dat").
 *         The returned string is an interned string and should not be freed.
 *         Returns @c NULL if no path has been selected this way or on failure.
 */
const char *
_elm_fileselector_button_selected_get_internal(const Evas_Object *obj);

/**
 * @internal
 * @brief Gets the list of paths selected via a fileselector button action when multi-selection is enabled.
 *
 * If multi-selection is active, this retrieves all paths that were confirmed
 * by a button action (e.g., clicking "Open" with multiple files selected).
 *
 * @param obj The fileselector object.
 * @return A read-only Eina_List of Eina_Stringshare instances, where each string is an
 *         absolute path to a selected file or directory confirmed via a button.
 *         Example:
 *         If "/docs/report.txt" and "/images/logo.png" are selected and confirmed,
 *         the list would contain these two strings.
 *         The list itself and its string contents are read-only and should not be modified or freed by the caller.
 *         The strings are interned.
 *         Returns @c NULL if multi-selection is not active, nothing was selected via button, or on failure.
 */
const Eina_List *
_elm_fileselector_button_selected_paths_get_internal(const Evas_Object *obj);

/**
 * @internal
 * @brief Converts and dispatches an Eo (Efl Object) event to a legacy Evas smart callback.
 *
 * This function serves as a compatibility layer, allowing newer Eo-based event
 * emissions to trigger older Evas-style smart callbacks for objects that
 * might still be listened to by legacy code.
 *
 * @param obj The Eo object that emitted the event. This object should also be an Evas_Object.
 * @param evt A string identifier for the legacy smart callback (e.g., "selected", "activated").
 * @param event_info A pointer to the event-specific data structure to be passed to the legacy callback.
 *                   The type and content of this data depend on the specific event @p evt.
 */
void
_event_to_legacy_call(Eo *obj, const char *evt, void *event_info);

/**
 * @internal
 * @brief Triggers an Efl_Model event and, if specified, a corresponding legacy Evas smart callback.
 *
 * This utility function is used to announce changes related to an Efl_Model,
 * such as an item being selected or a path changing. It ensures that both
 * new Efl_Model event listeners and (optionally) old Evas smart callback
 * listeners are notified.
 *
 * @param obj The Eo object that is the source of the event.
 * @param evt_desc A pointer to the Efl_Event_Description that defines the Efl_Model event being triggered.
 *                 Example: EFL_MODEL_EVENT_PROPERTIES_CHANGED.
 * @param legacy_evt The string name of the equivalent legacy Evas smart callback to trigger (e.g., "changed").
 *                   If @c NULL, no legacy event is triggered.
 * @param model The Efl_Model instance associated with this event. This is often the model
 *              held by @p obj or a related model.
 * @param path A string, typically representing the path or identifier of the item within the
 *             @p model that this event pertains to. This is passed as event_info to the legacy callback
 *             and might be part of the data for the Efl_Model event.
 *             Example: "/home/user/file.txt" if that file's properties changed.
 */
void
_model_event_call(Eo *obj, const Efl_Event_Description *evt_desc, const char *legacy_evt, Efl_Model *model, const char *path);

#endif
