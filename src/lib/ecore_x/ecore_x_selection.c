#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#include <stdlib.h>
#include <string.h>

#include "Ecore.h"
#include "ecore_private.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"
#include "Ecore_X_Atoms.h"

/** @internal
 * @brief Array storing data for the different X selections.
 * Indexed by a value derived from the selection atom (PRIMARY, SECONDARY, XDND, CLIPBOARD).
 */
static Ecore_X_Selection_Intern selections[4];

/** @internal
 * @brief Linked list of registered selection data converters.
 * Converters are responsible for transforming owned selection data into a requested target format.
 */
static Ecore_X_Selection_Converter *converters = NULL;

/** @internal
 * @brief Linked list of registered selection data parsers.
 * Parsers are responsible for interpreting received selection data based on its target type.
 */
static Ecore_X_Selection_Parser *parsers = NULL;

/**
 * @internal
 * @brief Frees an Ecore_X_Selection_Data structure whose data member was allocated with malloc.
 * @param data Pointer to the Ecore_X_Selection_Data to free.
 * @return Always returns 1 (historical, could be void).
 */
static int   _ecore_x_selection_data_default_free(void *data);

/**
 * @internal
 * @brief Parses raw selection data for file list targets (e.g., "text/uri-list").
 * @param target The target type string (e.g., "text/uri-list").
 * @param data The raw data buffer.
 * @param size The size of the data buffer.
 * @param format The format of the data (e.g., 8 for 8-bit characters).
 * @return A pointer to an Ecore_X_Selection_Data_Files structure, or NULL on failure.
 *         The returned structure contains an array of strings, where each string is a URI.
 *         Example: data might be "file:///home/user/file1.txt\r\nfile:///home/user/file2.png".
 *         The Ecore_X_Selection_Data_Files->files array would be {"file:///home/user/file1.txt", "file:///home/user/file2.png"}.
 */
static void *_ecore_x_selection_parser_files(const char *target,
                                             void *data,
                                             int size,
                                             int format);

/**
 * @internal
 * @brief Frees an Ecore_X_Selection_Data_Files structure.
 * @param data Pointer to the Ecore_X_Selection_Data_Files to free.
 * @return Always returns 0 (historical, could be void).
 */
static int   _ecore_x_selection_data_files_free(void *data);

/**
 * @internal
 * @brief Parses raw selection data for text-based targets (e.g., "text/plain", "UTF8_STRING").
 * @param target The target type string.
 * @param data The raw data buffer.
 * @param size The size of the data buffer.
 * @param format The format of the data.
 * @return A pointer to an Ecore_X_Selection_Data_Text structure, or NULL on failure.
 *         The returned structure contains the text data as a null-terminated string.
 */
static void *_ecore_x_selection_parser_text(const char *target,
                                            void *data,
                                            int size,
                                            int format);

/**
 * @internal
 * @brief Parses raw selection data for the "text/x-moz-url" target.
 * This format typically contains pairs of URL and title, UTF-16LE encoded, newline separated.
 * @param target The target type string ("text/x-moz-url").
 * @param data The raw data buffer (UTF-16LE encoded).
 * @param size The size of the data buffer.
 * @param format The format of the data.
 * @return A pointer to an Ecore_X_Selection_Data_X_Moz_Url structure, or NULL on failure.
 *         The structure contains two Eina_Inarray fields: `links` for URLs and `link_names` for titles.
 *         Example: data (after UTF-16LE to UTF-8 conversion) might be "http://example.com\nExample Site".
 *         sel->links would contain {"http://example.com"}.
 *         sel->link_names would contain {"Example Site"}.
 */
static void *_ecore_x_selection_parser_xmozurl(const char *target,
                                            void *data,
                                            int size,
                                            int format);
/**
 * @internal
 * @brief Frees an Ecore_X_Selection_Data_Text structure.
 * @param data Pointer to the Ecore_X_Selection_Data_Text to free.
 * @return Always returns 1 (historical, could be void).
 */
static int   _ecore_x_selection_data_text_free(void *data);

/**
 * @internal
 * @brief Parses raw selection data for the "TARGETS" target.
 * The data is expected to be a list of X Atoms.
 * @param target The target type string ("TARGETS").
 * @param data The raw data buffer (array of Atoms).
 * @param size The number of Atoms in the data buffer.
 * @param format The format of the data (should be 32 for Atoms).
 * @return A pointer to an Ecore_X_Selection_Data_Targets structure, or NULL on failure.
 *         The returned structure contains an array of strings, where each string is the name of a supported target Atom.
 */
static void *_ecore_x_selection_parser_targets(const char *target,
                                               void *data,
                                               int size,
                                               int format);

/**
 * @internal
 * @brief Frees an Ecore_X_Selection_Data_Targets structure.
 * @param data Pointer to the Ecore_X_Selection_Data_Targets to free.
 * @return Always returns 1 (historical, could be void).
 */
static int   _ecore_x_selection_data_targets_free(void *data);

#define ECORE_X_SELECTION_DATA(x) ((Ecore_X_Selection_Data *)(x))

/**
 * @internal
 * @brief Initializes the Ecore_X selection handling system.
 * This function sets up the internal data structures and registers
 * default converters and parsers for common selection types.
 * It should be called once during Ecore_X initialization.
 */
void
_ecore_x_selection_data_init(void)
{
   /* Initialize global data */
   memset(selections, 0, sizeof(selections));

   /* Initialize converters */
   ecore_x_selection_converter_atom_add(ECORE_X_ATOM_TEXT,
                                        ecore_x_selection_converter_text);
#ifdef X_HAVE_UTF8_STRING
   ecore_x_selection_converter_atom_add(ECORE_X_ATOM_UTF8_STRING,
                                        ecore_x_selection_converter_text);
#endif /* ifdef X_HAVE_UTF8_STRING */
   ecore_x_selection_converter_atom_add(ECORE_X_ATOM_COMPOUND_TEXT,
                                        ecore_x_selection_converter_text);
   ecore_x_selection_converter_atom_add(ECORE_X_ATOM_STRING,
                                        ecore_x_selection_converter_text);

   /* Initialize parsers */
   ecore_x_selection_parser_add("text/plain",
                                _ecore_x_selection_parser_text);
   ecore_x_selection_parser_add(ECORE_X_SELECTION_TARGET_UTF8_STRING,
                                _ecore_x_selection_parser_text);
   ecore_x_selection_parser_add("text/uri-list",
                                _ecore_x_selection_parser_files);
   ecore_x_selection_parser_add("text/x-moz-url",
                                _ecore_x_selection_parser_xmozurl);
   ecore_x_selection_parser_add("_NETSCAPE_URL",
                                _ecore_x_selection_parser_files);
   ecore_x_selection_parser_add(ECORE_X_SELECTION_TARGET_TARGETS,
                                _ecore_x_selection_parser_targets);
}

/**
 * @internal
 * @brief Shuts down the Ecore_X selection handling system.
 * This function frees all registered converters, parsers, and any
 * other resources allocated by the selection system.
 * It should be called once during Ecore_X shutdown.
 */
void
_ecore_x_selection_shutdown(void)
{
   Ecore_X_Selection_Converter *cnv;
   Ecore_X_Selection_Parser *prs;
   Eina_Inlist *inlist;

   /* free the selection converters */
   EINA_INLIST_FOREACH_SAFE(converters, inlist, cnv)
      free(cnv);
   converters = NULL;

   /* free the selection parsers */
   EINA_INLIST_FOREACH_SAFE(parsers, inlist, prs)
     {
        free(prs->target);
        free(prs);
     }
   parsers = NULL;
}

/**
 * @internal
 * @brief Retrieves the internal data structure for a given selection.
 * @param selection The X Atom identifying the selection (e.g., ECORE_X_ATOM_SELECTION_PRIMARY).
 * @return A pointer to the Ecore_X_Selection_Intern structure for the specified selection,
 *         or NULL if the selection atom is not recognized.
 */
Ecore_X_Selection_Intern *
_ecore_x_selection_get(Ecore_X_Atom selection)
{
   if (selection == ECORE_X_ATOM_SELECTION_PRIMARY)
     return &selections[0];
   else if (selection == ECORE_X_ATOM_SELECTION_SECONDARY)
     return &selections[1];
   else if (selection == ECORE_X_ATOM_SELECTION_XDND)
     return &selections[2];
   else if (selection == ECORE_X_ATOM_SELECTION_CLIPBOARD)
     return &selections[3];
   else
     return NULL;
}

/**
 * @internal
 * @brief Sets the owner and data for a specified X selection.
 * This is the core function used by the public ecore_x_selection_*_set functions.
 * It claims ownership of the selection and stores the provided data internally.
 *
 * @param w The window that will own the selection. Use 'None' to clear ownership.
 * @param data A pointer to the selection data.
 * @param size The size of the selection data in bytes.
 * @param selection The X Atom of the selection to set (e.g., ECORE_X_ATOM_SELECTION_PRIMARY).
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., could not claim ownership,
 *         or invalid selection atom).
 */
Eina_Bool
_ecore_x_selection_set(Window w,
                       const void *data,
                       int size,
                       Ecore_X_Atom selection)
{
   int in;
   unsigned char *buf = NULL;

   XSetSelectionOwner(_ecore_x_disp, selection, w, _ecore_x_event_last_time);
   if (XGetSelectionOwner(_ecore_x_disp, selection) != w)
     return EINA_FALSE;

   if (selection == ECORE_X_ATOM_SELECTION_PRIMARY)
     in = 0;
   else if (selection == ECORE_X_ATOM_SELECTION_SECONDARY)
     in = 1;
   else if (selection == ECORE_X_ATOM_SELECTION_XDND)
     in = 2;
   else if (selection == ECORE_X_ATOM_SELECTION_CLIPBOARD)
     in = 3;
   else
     return EINA_FALSE;

   if (selections[in].data)
     {
        free(selections[in].data);
        memset(&selections[in], 0, sizeof(Ecore_X_Selection_Intern));
     }

   if (data)
     {
        selections[in].win = w;
        selections[in].selection = selection;
        selections[in].length = size;
        selections[in].time = _ecore_x_event_last_time;

        buf = malloc(size);
        if (!buf) return EINA_FALSE;
        memcpy(buf, data, size);
        selections[in].data = buf;
     }

   return EINA_TRUE;
}

/* No Doxygen for EAPI that already has it */
/**
 * Claim ownership of the PRIMARY selection and set its data.
 * @param w    The window to which this selection belongs
 * @param data The data associated with the selection
 * @param size The size of the data buffer in bytes
 * @return     Returns 1 if the ownership of the selection was successfully
 *             claimed, or 0 if unsuccessful.
 */
EAPI Eina_Bool
ecore_x_selection_primary_set(Ecore_X_Window w,
                              const void *data,
                              int size)
{
   LOGFN;
   return _ecore_x_selection_set(w, data, size, ECORE_X_ATOM_SELECTION_PRIMARY);
}

/**
 * Release ownership of the primary selection
 * @return     Returns 1 if the selection was successfully cleared,
 *             or 0 if unsuccessful.
 *
 */
EAPI Eina_Bool
ecore_x_selection_primary_clear(void)
{
   LOGFN;
   return _ecore_x_selection_set(None, NULL, 0, ECORE_X_ATOM_SELECTION_PRIMARY);
}

/**
 * Claim ownership of the SECONDARY selection and set its data.
 * @param w    The window to which this selection belongs
 * @param data The data associated with the selection
 * @param size The size of the data buffer in bytes
 * @return     Returns 1 if the ownership of the selection was successfully
 *             claimed, or 0 if unsuccessful.
 */
EAPI Eina_Bool
ecore_x_selection_secondary_set(Ecore_X_Window w,
                                const void *data,
                                int size)
{
   LOGFN;
   return _ecore_x_selection_set(w,
                                 data,
                                 size,
                                 ECORE_X_ATOM_SELECTION_SECONDARY);
}

/**
 * Release ownership of the secondary selection
 * @return     Returns 1 if the selection was successfully cleared,
 *             or 0 if unsuccessful.
 *
 */
EAPI Eina_Bool
ecore_x_selection_secondary_clear(void)
{
   LOGFN;
   return _ecore_x_selection_set(None,
                                 NULL,
                                 0,
                                 ECORE_X_ATOM_SELECTION_SECONDARY);
}

/**
 * Claim ownership of the XDND selection and set its data.
 * @param w    The window to which this selection belongs
 * @param data The data associated with the selection
 * @param size The size of the data buffer in bytes
 * @return     Returns 1 if the ownership of the selection was successfully
 *             claimed, or 0 if unsuccessful.
 */
EAPI Eina_Bool
ecore_x_selection_xdnd_set(Ecore_X_Window w,
                           const void *data,
                           int size)
{
   LOGFN;
   return _ecore_x_selection_set(w, data, size, ECORE_X_ATOM_SELECTION_XDND);
}

/**
 * Release ownership of the XDND selection
 * @return     Returns 1 if the selection was successfully cleared,
 *             or 0 if unsuccessful.
 *
 */
EAPI Eina_Bool
ecore_x_selection_xdnd_clear(void)
{
   LOGFN;
   return _ecore_x_selection_set(None, NULL, 0, ECORE_X_ATOM_SELECTION_XDND);
}

/**
 * Claim ownership of the CLIPBOARD selection and set its data.
 * @param w    The window to which this selection belongs
 * @param data The data associated with the selection
 * @param size The size of the data buffer in bytes
 * @return     Returns 1 if the ownership of the selection was successfully
 *             claimed, or 0 if unsuccessful.
 *
 * Get the converted data from a previous CLIPBOARD selection
 * request. The buffer must be freed when done with.
 */
EAPI Eina_Bool
ecore_x_selection_clipboard_set(Ecore_X_Window w,
                                const void *data,
                                int size)
{
   LOGFN;
   return _ecore_x_selection_set(w,
                                 data,
                                 size,
                                 ECORE_X_ATOM_SELECTION_CLIPBOARD);
}

/**
 * Release ownership of the clipboard selection
 * @return     Returns 1 if the selection was successfully cleared,
 *             or 0 if unsuccessful.
 *
 */
EAPI Eina_Bool
ecore_x_selection_clipboard_clear(void)
{
   LOGFN;
   return _ecore_x_selection_set(None,
                                 NULL,
                                 0,
                                 ECORE_X_ATOM_SELECTION_CLIPBOARD);
}

/**
 * @internal
 * @brief Converts a target name string to its corresponding X Atom.
 * Handles common predefined target strings and falls back to ecore_x_atom_get()
 * for custom targets.
 *
 * @param target The string name of the target (e.g., "UTF8_STRING", "TARGETS").
 * @return The X Atom corresponding to the target string.
 */
Ecore_X_Atom
_ecore_x_selection_target_atom_get(const char *target)
{
   Ecore_X_Atom x_target;

   if (!strcmp(target, ECORE_X_SELECTION_TARGET_TEXT))
     x_target = ECORE_X_ATOM_TEXT;
   else if (!strcmp(target, ECORE_X_SELECTION_TARGET_COMPOUND_TEXT))
     x_target = ECORE_X_ATOM_COMPOUND_TEXT;
   else if (!strcmp(target, ECORE_X_SELECTION_TARGET_STRING))
     x_target = ECORE_X_ATOM_STRING;
   else if (!strcmp(target, ECORE_X_SELECTION_TARGET_UTF8_STRING))
     x_target = ECORE_X_ATOM_UTF8_STRING;
   else if (!strcmp(target, ECORE_X_SELECTION_TARGET_FILENAME))
     x_target = ECORE_X_ATOM_FILE_NAME;
   else if (!strcmp(target, ECORE_X_SELECTION_TARGET_X_MOZ_URL))
     x_target = ECORE_X_ATOM_X_MOZ_URL;
   else
     x_target = ecore_x_atom_get(target);

   return x_target;
}

/**
 * @internal
 * @brief Converts an X Atom representing a selection target to its string name.
 * Handles common predefined target atoms and falls back to XGetAtomName()
 * for other atoms.
 *
 * @note The caller is responsible for freeing the returned string.
 *       If the atom is one of the predefined ones (e.g. ECORE_X_ATOM_FILE_NAME),
 *       the string is allocated with strdup() and should be freed with free().
 *       Otherwise, it's allocated with XGetAtomName() and should be freed with XFree().
 *       This is a known issue (FIXME in code).
 *
 * @param target The X Atom of the target.
 * @return A newly allocated string representing the target name, or NULL if the
 *         atom name cannot be retrieved.
 */
char *
_ecore_x_selection_target_get(Ecore_X_Atom target)
{
   /* FIXME: Should not return mem allocated with strdup or X mixed,
    * one should use free to free, the other XFree */
   if (target == ECORE_X_ATOM_FILE_NAME)
     return strdup(ECORE_X_SELECTION_TARGET_FILENAME);
   else if (target == ECORE_X_ATOM_STRING)
     return strdup(ECORE_X_SELECTION_TARGET_STRING);
   else if (target == ECORE_X_ATOM_UTF8_STRING)
     return strdup(ECORE_X_SELECTION_TARGET_UTF8_STRING);
   else if (target == ECORE_X_ATOM_TEXT)
     return strdup(ECORE_X_SELECTION_TARGET_TEXT);
   else if (target == ECORE_X_ATOM_X_MOZ_URL)
     return strdup(ECORE_X_SELECTION_TARGET_X_MOZ_URL);
   else
     return XGetAtomName(_ecore_x_disp, target);
}

/**
 * @internal
 * @brief Initiates a request for selection data.
 * This function performs an XConvertSelection call to ask the current
 * owner of the specified selection to convert its data to the given target format.
 * The result will be delivered via a SelectionNotify event.
 *
 * @param w The window that is requesting the selection.
 * @param selection The X Atom of the selection to request (e.g., ECORE_X_ATOM_SELECTION_PRIMARY).
 * @param target_str The string name of the desired target format (e.g., "UTF8_STRING").
 */
static void
_ecore_x_selection_request(Ecore_X_Window w,
                           Ecore_X_Atom selection,
                           const char *target_str)
{
   Ecore_X_Atom target, prop;

   target = _ecore_x_selection_target_atom_get(target_str);

   if (selection == ECORE_X_ATOM_SELECTION_PRIMARY)
     prop = ECORE_X_ATOM_SELECTION_PROP_PRIMARY;
   else if (selection == ECORE_X_ATOM_SELECTION_SECONDARY)
     prop = ECORE_X_ATOM_SELECTION_PROP_SECONDARY;
   else if (selection == ECORE_X_ATOM_SELECTION_CLIPBOARD)
     prop = ECORE_X_ATOM_SELECTION_PROP_CLIPBOARD;
   else
     return;

   XConvertSelection(_ecore_x_disp, selection, target, prop,
                     w, CurrentTime);
}

/**
 * @brief Request the content of the PRIMARY selection.
 *
 * This function asks the X server to convert the PRIMARY selection to the
 * specified @p target format and deliver it to window @p w.
 * A @c ECORE_X_EVENT_SELECTION_NOTIFY event will be sent to the application
 * when the selection data is available or if the request fails.
 *
 * @param w The window to which the selection data should be delivered.
 * @param target The desired format of the selection data (e.g., "UTF8_STRING", "TARGETS").
 */
EAPI void
ecore_x_selection_primary_request(Ecore_X_Window w,
                                  const char *target)
{
   LOGFN;
   _ecore_x_selection_request(w, ECORE_X_ATOM_SELECTION_PRIMARY, target);
}

/**
 * @brief Request the content of the SECONDARY selection.
 *
 * This function asks the X server to convert the SECONDARY selection to the
 * specified @p target format and deliver it to window @p w.
 * A @c ECORE_X_EVENT_SELECTION_NOTIFY event will be sent to the application
 * when the selection data is available or if the request fails.
 *
 * @param w The window to which the selection data should be delivered.
 * @param target The desired format of the selection data (e.g., "UTF8_STRING").
 */
EAPI void
ecore_x_selection_secondary_request(Ecore_X_Window w,
                                    const char *target)
{
   LOGFN;
   _ecore_x_selection_request(w, ECORE_X_ATOM_SELECTION_SECONDARY, target);
}

/**
 * @brief Request the content of the XDND selection.
 *
 * This function is typically used in drag-and-drop (DND) operations. It asks
 * the X server to convert the XDND selection (representing the dragged data)
 * to the specified @p target format and deliver it to window @p w.
 * The time for the request is taken from the internal DND state.
 * A @c ECORE_X_EVENT_SELECTION_NOTIFY event will be sent to the application
 * when the selection data is available or if the request fails.
 *
 * @param w The window to which the selection data should be delivered (usually the DND drop target).
 * @param target The desired format of the selection data (e.g., "text/uri-list").
 */
EAPI void
ecore_x_selection_xdnd_request(Ecore_X_Window w,
                               const char *target)
{
   Ecore_X_Atom atom;
   Ecore_X_DND_Target *_target;

   LOGFN;
   _target = _ecore_x_dnd_target_get();
   atom = _ecore_x_selection_target_atom_get(target);
   XConvertSelection(_ecore_x_disp, ECORE_X_ATOM_SELECTION_XDND, atom,
                     ECORE_X_ATOM_SELECTION_PROP_XDND, w,
                     _target->time);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Request the content of the CLIPBOARD selection.
 *
 * This function asks the X server to convert the CLIPBOARD selection to the
 * specified @p target format and deliver it to window @p w.
 * A @c ECORE_X_EVENT_SELECTION_NOTIFY event will be sent to the application
 * when the selection data is available or if the request fails.
 *
 * @param w The window to which the selection data should be delivered.
 * @param target The desired format of the selection data (e.g., "UTF8_STRING", "image/png").
 */
EAPI void
ecore_x_selection_clipboard_request(Ecore_X_Window w,
                                    const char *target)
{
   LOGFN;
   _ecore_x_selection_request(w, ECORE_X_ATOM_SELECTION_CLIPBOARD, target);
}

EAPI void
ecore_x_selection_converter_atom_add(Ecore_X_Atom target,
                                     Eina_Bool (*func)(char *target,
                                                       void *data,
                                                       int size,
                                                       void **data_ret,
                                                       int *size_ret,
                                                       Ecore_X_Atom *ttype,
                                                       int *tsize))
{
   Ecore_X_Selection_Converter *cnv;

   LOGFN;
   /**
    * @brief Adds a selection data converter for a target specified by an X Atom.
    *
    * Registers a callback function that will be used to convert the currently
    * owned selection data to the format specified by the @p target Atom.
    * If a converter for this atom already exists, it is replaced.
    *
    * @param target The X Atom representing the target format this converter handles.
    * @param func The converter function. It takes the target name (as string),
    *             the raw selection data and its size. It should allocate and fill
    *             @p data_ret and @p size_ret with the converted data.
    *             It should also set @p ttype (target type atom for property) and
    *             @p tsize (target data unit size: 8, 16, or 32).
    *             Return EINA_TRUE on success, EINA_FALSE on failure.
    *
    * @see ecore_x_selection_converter_text for an example converter.
    */

   EINA_INLIST_FOREACH(converters, cnv)
      if (cnv->target == target)
        {
           cnv->convert = func;
           return;
        }

   cnv = calloc(1, sizeof(Ecore_X_Selection_Converter));
   if (!cnv) return;

   cnv->target = target;
   cnv->convert = func;
   converters = (Ecore_X_Selection_Converter *)eina_inlist_append
      (EINA_INLIST_GET(converters), EINA_INLIST_GET(cnv));
}

EAPI void
ecore_x_selection_converter_add(char *target,
                                Eina_Bool (*func)(char *target,
                                                  void *data,
                                                  int size,
                                                  void **data_ret,
                                                  int *size_ret,
                                                  Ecore_X_Atom *,
                                                  int *))
{
   Ecore_X_Atom x_target;

   if (!func || !target)
     return;

   LOGFN;
   /**
    * @brief Adds a selection data converter for a target specified by a string name.
    *
    * Registers a callback function that will be used to convert the currently
    * owned selection data to the format specified by the @p target string.
    * The string is converted to an X Atom internally.
    * If a converter for this target already exists, it is replaced.
    *
    * @param target The string name of the target format this converter handles (e.g., "UTF8_STRING").
    * @param func The converter function. See ecore_x_selection_converter_atom_add() for details
    *             on the function signature and behavior.
    */
   x_target = _ecore_x_selection_target_atom_get(target);

   ecore_x_selection_converter_atom_add(x_target, func);
}

EAPI void
ecore_x_selection_converter_atom_del(Ecore_X_Atom target)
{
   Ecore_X_Selection_Converter *cnv;

   LOGFN;
   /**
    * @brief Deletes a selection data converter for a target specified by an X Atom.
    *
    * Unregisters and frees a previously added converter function associated with
    * the given @p target Atom.
    *
    * @param target The X Atom of the target format whose converter should be removed.
    */

   EINA_INLIST_FOREACH(converters, cnv)
     {
        if (cnv->target == target)
          {
             converters = (Ecore_X_Selection_Converter *)eina_inlist_remove
                (EINA_INLIST_GET(converters), EINA_INLIST_GET(cnv));
             free(cnv);
             return;
          }
     }
}

EAPI void
ecore_x_selection_converter_del(char *target)
{
   Ecore_X_Atom x_target;

   if (!target)
     return;

   LOGFN;
   /**
    * @brief Deletes a selection data converter for a target specified by a string name.
    *
    * Unregisters and frees a previously added converter function associated with
    * the given @p target string. The string is converted to an X Atom internally.
    *
    * @param target The string name of the target format whose converter should be removed.
    */
   x_target = _ecore_x_selection_target_atom_get(target);
   ecore_x_selection_converter_atom_del(x_target);
}

EAPI Eina_Bool
ecore_x_selection_notify_send(Ecore_X_Window requestor,
                              Ecore_X_Atom selection,
                              Ecore_X_Atom target,
                              Ecore_X_Atom property,
                              Ecore_X_Time tim)
{
   XEvent xev = { 0 };
   XSelectionEvent xnotify;

   LOGFN;
   /**
    * @brief Sends a SelectionNotify event.
    *
    * This function is used by a selection owner to respond to a SelectionRequest
    * event. It informs the requesting window whether the conversion was successful
    * and, if so, what property on the requestor's window now holds the data.
    *
    * @param requestor The window that made the selection request.
    * @param selection The selection atom that was requested (e.g., ECORE_X_ATOM_SELECTION_PRIMARY).
    * @param target The target atom that was requested.
    * @param property The atom of the property on the @p requestor window where the
    *                 data has been stored. If the conversion failed, this should be @c None.
    * @param tim The timestamp from the SelectionRequest event.
    * @return EINA_TRUE if the event was sent successfully, EINA_FALSE otherwise.
    */
   xnotify.type = SelectionNotify;
   xnotify.display = _ecore_x_disp;
   xnotify.requestor = requestor;
   xnotify.selection = selection;
   xnotify.target = target;
   xnotify.property = property;
   xnotify.time = tim;
   xnotify.send_event = True;
   xnotify.serial = 0;

   xev.xselection = xnotify;
   return (XSendEvent(_ecore_x_disp, requestor, False, 0, &xev) > 0) ? EINA_TRUE : EINA_FALSE;
}

/* Locate and run conversion callback for specified selection target */
EAPI Eina_Bool
ecore_x_selection_convert(Ecore_X_Atom selection,
                          Ecore_X_Atom target,
                          void **data_ret,
                          int *size,
                          Ecore_X_Atom *targtype,
                          int *typesize)
{
   Ecore_X_Selection_Intern *sel;
   Ecore_X_Selection_Converter *cnv;
   void *data = NULL;
   char *tgt_str;

   LOGFN;
   /**
    * @brief Converts the data of a currently owned selection to a specified target format.
    *
    * This function is called by the Ecore_X event loop when a SelectionRequest
    * event is received for a selection owned by an Ecore_X client. It looks up
    * a registered converter for the requested @p target atom and calls it.
    *
    * @param selection The atom of the selection being requested (e.g., ECORE_X_ATOM_SELECTION_PRIMARY).
    * @param target The atom of the target format into which the data should be converted.
    * @param[out] data_ret Pointer to store the address of the converted data buffer.
    *                      The caller of the converter (which is this function) does not free this;
    *                      the converter or subsequent XChangeProperty call might manage it.
    *                      Typically, the data is placed on a property, and X server handles it.
    * @param[out] size Pointer to store the size of the converted data in bytes.
    * @param[out] targtype Pointer to store the atom representing the actual type of the converted data
    *                      (e.g., ECORE_X_ATOM_STRING, ECORE_X_ATOM_UTF8_STRING). This is used for XChangeProperty.
    * @param[out] typesize Pointer to store the format of the converted data (8, 16, or 32 bits).
    *                      This is used for XChangeProperty.
    * @return EINA_TRUE if a suitable converter was found and successfully converted the data,
    *         EINA_FALSE otherwise (e.g., no converter for the target, or conversion failed).
    */
   sel = _ecore_x_selection_get(selection);
   tgt_str = _ecore_x_selection_target_get(target);

   EINA_INLIST_FOREACH(converters, cnv)
     {
        if (cnv->target == target)
          {
             int r;
             r = cnv->convert(tgt_str, sel->data, sel->length, &data, size,
                              targtype, typesize);
             free(tgt_str);
             if (r)
               {
                  if (data_ret) *data_ret = data;
                  return r;
               }
             else
               return EINA_FALSE;
          }
     }
   free(tgt_str);

   /* ICCCM says "If the selection cannot be converted into a form based on the target (and parameters, if any), the owner should refuse the SelectionRequest as previously described." */
   return EINA_FALSE;

   /* Default, just return the data
    * data_ret = malloc(sel->length);
      memcpy(*data_ret, sel->data, sel->length);
      free(tgt_str);
      return 1;
    */
}

/* TODO: We need to work out a mechanism for automatic conversion to any requested
 * locale using Ecore_Txt functions */
/* Converter for standard non-utf8 text targets */
EAPI Eina_Bool
ecore_x_selection_converter_text(char *target,
                                 void *data,
                                 int size,
                                 void **data_ret,
                                 int *size_ret,
                                 Ecore_X_Atom *targprop EINA_UNUSED,
                                 int *s EINA_UNUSED)
{
   XTextProperty text_prop;
   char *mystr;
   XICCEncodingStyle style;

   if (!data || !size)
     return EINA_FALSE;

   LOGFN;
   /**
    * @brief Default converter for various text-based selection targets.
    *
    * This function converts raw data (assumed to be a C string, though not necessarily
    * null-terminated initially) into one of the standard X text property formats.
    * It handles targets like "TEXT" (locale-dependent), "COMPOUND_TEXT", "STRING" (ISO 8859-1),
    * and "UTF8_STRING".
    *
    * @param target The string name of the target format (e.g., "UTF8_STRING").
    * @param data Pointer to the raw selection data (owned by the selection owner).
    * @param size Size of the raw selection data in bytes.
    * @param[out] data_ret Pointer to store the address of the newly allocated, converted text data.
    *                      The caller (typically ecore_x_selection_convert, which then uses it
    *                      for XChangeProperty) is responsible for freeing this with XFree
    *                      after XChangeProperty (or if XChangeProperty is not called).
    *                      However, XChangeProperty copies the data, so it's usually XFree'd right after.
    *                      In this specific implementation, text_prop.value is XFree'd, and a malloc'd copy is returned.
    * @param[out] size_ret Pointer to store the size of the converted text data in bytes.
    * @param targprop Unused in this function, but part of the generic converter signature.
    *                 It would typically be set to the atom representing the type of data in data_ret.
    * @param s Unused in this function, but part of the generic converter signature.
    *          It would typically be set to the format (8, 16, 32) of the data in data_ret.
    * @return EINA_TRUE on successful conversion, EINA_FALSE otherwise.
    */
   if (!strcmp(target, ECORE_X_SELECTION_TARGET_TEXT))
     style = XTextStyle;
   else if (!strcmp(target, ECORE_X_SELECTION_TARGET_COMPOUND_TEXT))
     style = XCompoundTextStyle;
   else if (!strcmp(target, ECORE_X_SELECTION_TARGET_STRING))
     style = XStringStyle;

#ifdef X_HAVE_UTF8_STRING
   else if (!strcmp(target, ECORE_X_SELECTION_TARGET_UTF8_STRING))
     style = XUTF8StringStyle;
#endif /* ifdef X_HAVE_UTF8_STRING */
   else
     return EINA_FALSE;

   mystr = alloca(size + 1);
   memcpy(mystr, data, size);
   mystr[size] = '\0';

#ifdef X_HAVE_UTF8_STRING
   if (Xutf8TextListToTextProperty(_ecore_x_disp, &mystr, 1, style,
                                   &text_prop) == Success)
     {
        int bufsize = strlen((char *)text_prop.value);
        char *str = malloc(bufsize + 1);
        if (!str) return EINA_FALSE;
        *data_ret = str;
        memcpy(str, text_prop.value, bufsize);
        str[bufsize] = 0;
        *size_ret = bufsize;
        XFree(text_prop.value);
        return EINA_TRUE;
     }

#else /* ifdef X_HAVE_UTF8_STRING */
   if (XmbTextListToTextProperty(_ecore_x_disp, &mystr, 1, style,
                                 &text_prop) == Success)
     {
        int bufsize = strlen(text_prop.value);
        *data_ret = malloc(bufsize);
        if (!*data_ret) return EINA_FALSE;
        memcpy(*data_ret, text_prop.value, bufsize);
        *size_ret = bufsize;
        XFree(text_prop.value);
        return EINA_TRUE;
     }

#endif /* ifdef X_HAVE_UTF8_STRING */
   else
     {
        return EINA_TRUE;
     }
}

EAPI void
ecore_x_selection_parser_add(const char *target,
                             void *(*func)(const char *target, void *data,
                                           int size,
                                           int format))
{
   Ecore_X_Selection_Parser *prs;

   if (!target)
     return;

   LOGFN;
   /**
    * @brief Adds a selection data parser for a specific target type.
    *
    * Registers a callback function that will be used to parse raw selection data
    * received for the given @p target type. Parsers transform the raw byte stream
    * from an X selection into a more structured Ecore_X_Selection_Data format.
    * If a parser for this target string already exists, it is replaced.
    *
    * @param target The string name of the target format this parser handles (e.g., "text/uri-list").
    * @param func The parser function. It takes the target name, raw data buffer,
    *             data size, and data format. It should return a pointer to a newly
    *             allocated structure derived from Ecore_X_Selection_Data, or NULL on failure.
    *             The returned structure must have its `free` function pointer correctly set
    *             to allow proper deallocation later.
    *
    * @see _ecore_x_selection_parser_files for an example parser.
    * @see Ecore_X_Selection_Data
    */

   EINA_INLIST_FOREACH(parsers, prs)
      if (!strcmp(prs->target, target))
        {
           prs->parse = func;
           return;
        }

   prs = calloc(1, sizeof(Ecore_X_Selection_Parser));
   if (!prs) return;

   prs->target = strdup(target);
   prs->parse = func;

   parsers = (Ecore_X_Selection_Parser *)eina_inlist_append
      (EINA_INLIST_GET(parsers), EINA_INLIST_GET(prs));
}

EAPI void
ecore_x_selection_parser_del(const char *target)
{
   Ecore_X_Selection_Parser *prs;

   if (!target)
     return;

   LOGFN;
   /**
    * @brief Deletes a selection data parser for a specific target type.
    *
    * Unregisters and frees a previously added parser function associated with
    * the given @p target string.
    *
    * @param target The string name of the target format whose parser should be removed.
    */

   EINA_INLIST_FOREACH(parsers, prs)
     {
        if (!strcmp(prs->target, target))
          {
             parsers = (Ecore_X_Selection_Parser *)eina_inlist_remove
                (EINA_INLIST_GET(parsers), EINA_INLIST_GET(prs));
             free(prs->target);
             free(prs);
             return;
          }
     }
}

/**
 * Change the owner and last-change time for the specified selection.
 * @param win The owner of the specified atom.
 * @param atom The selection atom
 * @param tim Specifies the time
 * @since 1.1.0
 */
EAPI void
ecore_x_selection_owner_set(Ecore_X_Window win,
                            Ecore_X_Atom atom,
                            Ecore_X_Time tim)
{
   XSetSelectionOwner(_ecore_x_disp, atom, win, tim);
}

/**
 * Return the window that currently owns the specified selection.
 *
 * @param atom The specified selection atom.
 *
 * @return The window that currently owns the specified selection.
 * @since 1.1.0
 */
EAPI Ecore_X_Window
ecore_x_selection_owner_get(Ecore_X_Atom atom)
{
   return XGetSelectionOwner(_ecore_x_disp, atom);
}

/* Locate and run conversion callback for specified selection target */
/**
 * @internal
 * @brief Parses incoming selection data using a registered parser.
 *
 * This function is called when selection data is received (typically via a
 * SelectionNotify event followed by GetProperty). It iterates through the
 * registered parsers to find one that matches the @p target string.
 * If a matching parser is found, it's invoked to convert the raw @p data
 * into an Ecore_X_Selection_Data structure. If no specific parser is found,
 * a default one is used which wraps the raw data.
 *
 * @param target The string name of the target format of the received data.
 * @param data Pointer to the raw selection data buffer. This function takes ownership
 *             if a parser consumes it or if the default parser is used.
 * @param size Size of the raw data buffer in bytes.
 * @param format Format of the data (8, 16, or 32 bits).
 * @return A pointer to an Ecore_X_Selection_Data structure containing the parsed data,
 *         or NULL on allocation failure. The specific type of the returned structure
 *         depends on the parser used (e.g., Ecore_X_Selection_Data_Text, Ecore_X_Selection_Data_Files).
 *         The `free` member of the returned structure will be set appropriately.
 */
void *
_ecore_x_selection_parse(const char *target,
                         void *data,
                         int size,
                         int format)
{
   Ecore_X_Selection_Parser *prs;
   Ecore_X_Selection_Data *sel;

   EINA_INLIST_FOREACH(parsers, prs)
     {
        if (!strcmp(prs->target, target))
          {
             sel = prs->parse(target, data, size, format);
             if (sel) return sel;
          }
     }

   /* Default, just return the data */
   sel = calloc(1, sizeof(Ecore_X_Selection_Data));
   if (!sel) return NULL;
   sel->free = _ecore_x_selection_data_default_free;
   sel->length = size;
   sel->format = format;
   sel->data = data;
   return sel;
}

static int
_ecore_x_selection_data_default_free(void *data)
{
   Ecore_X_Selection_Data *sel;

   sel = data;
   free(sel->data); // sel->data here is the original raw data from XGetWindowProperty
   free(sel);       // sel is the Ecore_X_Selection_Data wrapper
   return 1;
}

/**
 * @internal _ecore_x_selection_parser_files
 * @brief Parses data for "text/uri-list" or "_NETSCAPE_URL" targets.
 *
 * Input data is expected to be a CR-LF or LF separated list of URIs.
 * Lines starting with '#' are considered comments and ignored.
 * The function allocates an Ecore_X_Selection_Data_Files structure
 * and populates its `files` array with the parsed URIs.
 *
 * @param target The target string, expected to be "text/uri-list" or "_NETSCAPE_URL".
 * @param _data The raw data buffer. This function takes ownership and frees it.
 * @param size The size of the raw data buffer.
 * @param format Unused.
 * @return A pointer to a newly allocated Ecore_X_Selection_Data_Files structure,
 *         or NULL on failure or if the target is not supported.
 *         The `files` member is an array of strings (char **), each being a URI.
 *         `num_files` indicates the count of URIs.
 *         Example: If _data is "file:///tmp/a.txt\r\n#comment\nfile:///tmp/b.png",
 *         sel->files would be {"file:///tmp/a.txt", "file:///tmp/b.png"} and sel->num_files would be 2.
 */
static void *
_ecore_x_selection_parser_files(const char *target,
                                void *_data,
                                int size,
                                int format EINA_UNUSED)
{
   Ecore_X_Selection_Data_Files *sel;
   char *data = _data;

   if (strcmp(target, "text/uri-list") &&
       strcmp(target, "_NETSCAPE_URL"))
     return NULL;

   sel = calloc(1, sizeof(Ecore_X_Selection_Data_Files));
   if (!sel) return NULL;
   ECORE_X_SELECTION_DATA(sel)->free = _ecore_x_selection_data_files_free;

   if (data && (size > 0))
     {
        int i, is;
        char *tmp;
        char **t2;

        if (data[size - 1])
          {
             char *t;

             /* Isn't nul terminated */
             size++;
             t = realloc(data, size);
             if (!t) goto done;
             data = t;
             data[size - 1] = 0;
          }

        tmp = malloc(size);
        if (!tmp) goto done;
        i = 0;
        is = 0;
        while ((is < size) && (data[is]))
          {
             if ((i == 0) && (data[is] == '#'))
               for (; ((data[is]) && (data[is] != '\n')); is++) ;
             else
               {
                  if ((data[is] != '\r') &&
                      (data[is] != '\n'))
                    tmp[i++] = data[is++];
                  else
                    {
                       while ((data[is] == '\r') || (data[is] == '\n'))
                         is++;
                       tmp[i] = 0;
                       sel->num_files++;
                       t2 = realloc(sel->files, sel->num_files * sizeof(char *));
                       if (t2)
                         {
                            sel->files = t2;
                            sel->files[sel->num_files - 1] = strdup(tmp);
                         }
                       tmp[0] = 0;
                       i = 0;
                    }
               }
          }
        if (i > 0)
          {
             tmp[i] = 0;
             sel->num_files++;
             t2 = realloc(sel->files, sel->num_files * sizeof(char *));
             if (t2)
               {
                  sel->files = t2;
                  sel->files[sel->num_files - 1] = strdup(tmp);
               }
          }

        free(tmp);
     }
done:
   free(data);

   ECORE_X_SELECTION_DATA(sel)->content = ECORE_X_SELECTION_CONTENT_FILES;
   ECORE_X_SELECTION_DATA(sel)->length = sel->num_files;

   return ECORE_X_SELECTION_DATA(sel);
}

/**
 * @internal _ecore_x_selection_data_files_free
 * @brief Frees an Ecore_X_Selection_Data_Files structure.
 * This includes freeing the array of file strings and the structure itself.
 * @param data Pointer to the Ecore_X_Selection_Data_Files structure to free.
 * @return Always 0.
 */
static int
_ecore_x_selection_data_files_free(void *data)
{
   Ecore_X_Selection_Data_Files *sel;
   int i;

   sel = data;
   if (sel->files)
     {
        for (i = 0; i < sel->num_files; i++)
          free(sel->files[i]);
        free(sel->files);
     }

   free(sel);
   return 0;
}

static void *
_ecore_x_selection_parser_text(const char *target EINA_UNUSED,
                               void *_data,
                               int size,
                               int format EINA_UNUSED)
{
   Ecore_X_Selection_Data_Text *sel;
   unsigned char *data = _data;
   void *t;

   /**
    * @internal _ecore_x_selection_parser_text
    * @brief Parses data for generic text targets.
    *
    * The input data is treated as a block of text. This function ensures it's
    * null-terminated and wraps it in an Ecore_X_Selection_Data_Text structure.
    *
    * @param target Unused. The target string.
    * @param _data The raw data buffer. This function takes ownership (may realloc and eventually free via the struct's free func).
    * @param size The size of the raw data buffer.
    * @param format Unused.
    * @return A pointer to a newly allocated Ecore_X_Selection_Data_Text structure,
    *         or NULL on failure. The `text` member points to the null-terminated string.
    */
   sel = calloc(1, sizeof(Ecore_X_Selection_Data_Text));
   if (!sel) return NULL;
   if (data && data[size - 1])
     {
        /* Isn't nul terminated */
        size++;
        t = realloc(data, size);
        if (!t)
          {
             free(sel);
             return NULL;
          }
        data = t;
        data[size - 1] = 0;
     }

   sel->text = (char *)data;
   ECORE_X_SELECTION_DATA(sel)->length = size;
   ECORE_X_SELECTION_DATA(sel)->content = ECORE_X_SELECTION_CONTENT_TEXT;
   ECORE_X_SELECTION_DATA(sel)->data = data;
   ECORE_X_SELECTION_DATA(sel)->free = _ecore_x_selection_data_text_free;
   return sel;
}

/**
 * @internal _ecore_x_selection_data_xmozurl_free
 * @brief Frees an Ecore_X_Selection_Data_X_Moz_Url structure.
 * This includes freeing the underlying buffer that stores all strings,
 * and the Eina_Inarray structures for links and link names.
 * @param data Pointer to the Ecore_X_Selection_Data_X_Moz_Url structure to free.
 * @return Always 1.
 */
static int
_ecore_x_selection_data_xmozurl_free(void *data)
{
   Ecore_X_Selection_Data_X_Moz_Url *sel = data;
   char **buf;

   // The strings in sel->links and sel->link_names point into a single
   // large buffer that was allocated by eina_str_convert_len.
   // This buffer is stored as the first element of sel->links.
   // So, we retrieve it and free it.
   // The ECORE_X_SELECTION_DATA(sel)->data also points to the original _data from X,
   // which is freed by the generic free function if this one isn't called or if
   // the parsing fails before this free function is assigned.
   // However, the current logic in _ecore_x_selection_parse will call this free function,
   // and this function is responsible for freeing the converted 'buf' and the original 'data'.
   // The original 'data' is stored in ECORE_X_SELECTION_DATA(sel)->data.
   // The converted 'buf' (UTF-8) is what the inarrays point into.
   // The first element of sel->links is a pointer to the start of this 'buf'.
   buf = eina_inarray_nth(sel->links, 0);
   if (buf && *buf) free(*buf); // Free the converted UTF-8 buffer

   eina_inarray_free(sel->links);
   sel->links = NULL;
   eina_inarray_free(sel->link_names);
   sel->link_names = NULL;

   // Free the original data received from X server, which was stored in the generic part
   free(ECORE_X_SELECTION_DATA(sel)->data);
   ECORE_X_SELECTION_DATA(sel)->data = NULL;

   free(sel);
   return 1;
}
#ifdef HAVE_ICONV
# include <errno.h>
# include <iconv.h>
#endif
static void *
_ecore_x_selection_parser_xmozurl(const char *target EINA_UNUSED,
                               void *_data,
                               int size,
                               int format EINA_UNUSED)
{
   Ecore_X_Selection_Data_X_Moz_Url *sel;
   char *prev, *n, *buf, *orig_data = _data; // Keep original _data pointer
   size_t sz;
   int num = 0;

   /**
    * @internal _ecore_x_selection_parser_xmozurl
    * @brief Parses data for the "text/x-moz-url" target.
    *
    * This format is typically used by Mozilla applications (like Firefox) for drag-and-drop
    * of links. It consists of pairs of URL and title, with each part on a new line.
    * The data is usually UTF-16LE encoded. This function converts it to UTF-8.
    *
    * @param target Unused. The target string ("text/x-moz-url").
    * @param _data The raw data buffer (expected to be UTF-16LE). This function does NOT take ownership
    *              of _data directly for freeing; instead, it stores it in the Ecore_X_Selection_Data
    *              structure to be freed by _ecore_x_selection_data_xmozurl_free.
    * @param size The size of the raw data buffer.
    * @param format Unused.
    * @return A pointer to a newly allocated Ecore_X_Selection_Data_X_Moz_Url structure,
    *         or NULL on failure.
    *         sel->links (Eina_Inarray of char*) contains the URLs.
    *         sel->link_names (Eina_Inarray of char*) contains the corresponding titles.
    *         The actual string data for URLs and titles is stored in a single contiguous buffer,
    *         which is managed by the structure and its free function.
    *         Example: _data (UTF-16LE) for "http://e.org\nEFL\nhttp://g.com\nGNU"
    *         After conversion to UTF-8 `buf`: "http://e.org\0EFL\0http://g.com\0GNU" (nulls are newlines)
    *         sel->links: {"http://e.org", "http://g.com"}
    *         sel->link_names: {"EFL", "GNU"}
    */

   buf = eina_str_convert_len("UTF-16LE", "UTF-8", orig_data, size, &sz);
   if (!buf) return NULL;
   sel = calloc(1, sizeof(Ecore_X_Selection_Data_X_Moz_Url));
   if (!sel)
      goto error_sel;

   sz = strlen(buf);
   sel->links = eina_inarray_new(sizeof(char*), 0);
   if (!sel->links)
      goto error_links;

   sel->link_names = eina_inarray_new(sizeof(char*), 0);
   if (!sel->link_names)
      goto error_link_names;

   prev = buf;
   for (n = memchr(buf, '\n', sz); n; n = memchr(prev, '\n', sz - (prev - buf)))
     {
        n[0] = 0;
        if (num % 2 == 0)
          eina_inarray_push(sel->links, &prev);
        else
          eina_inarray_push(sel->link_names, &prev);
        num++;
        prev = n + 1;
     }
   eina_inarray_push(sel->link_names, &prev);

   ECORE_X_SELECTION_DATA(sel)->length = size; // Original size of UTF-16LE data
   ECORE_X_SELECTION_DATA(sel)->content = ECORE_X_SELECTION_CONTENT_X_MOZ_URL;
   ECORE_X_SELECTION_DATA(sel)->data = (void*)orig_data; // Store original data pointer for freeing
   ECORE_X_SELECTION_DATA(sel)->free = _ecore_x_selection_data_xmozurl_free;
   return sel;

error_link_names:
   eina_inarray_free(sel->links);

error_links:
   free(sel);

error_sel:
   free(buf);
   return NULL;
}

static int
_ecore_x_selection_data_text_free(void *data)
{
   Ecore_X_Selection_Data_Text *sel;

   sel = data;
   // sel->text and sel->data point to the same buffer, which was the original _data (possibly realloc'd)
   // from _ecore_x_selection_parser_text. So, freeing sel->text (or sel->data) is enough.
   free(sel->text);
   sel->text = NULL;
   ECORE_X_SELECTION_DATA(sel)->data = NULL; // Already freed via sel->text
   free(sel);
   return 1;
}

/**
 * @internal _ecore_x_selection_parser_targets
 * @brief Parses data for the "TARGETS" target.
 *
 * The "TARGETS" target is used to query which target formats a selection owner
 * can provide. The data is a list of X Atoms. This function converts these
 * atoms to their string names.
 *
 * @param target Unused. The target string ("TARGETS").
 * @param data The raw data buffer, which is an array of X Atoms (Atom *).
 *             This function takes ownership of this buffer and stores it to be freed later.
 * @param size The number of Atoms in the `data` array.
 * @param format Unused (expected to be 32 for Atoms).
 * @return A pointer to a newly allocated Ecore_X_Selection_Data_Targets structure,
 *         or NULL on failure.
 *         sel->targets (char **) is an array of strings, each being an atom name.
 *         sel->num_targets is the count of these names.
 *         Example: If `data` contains Atoms for "UTF8_STRING" and "text/plain",
 *         sel->targets would be {"UTF8_STRING", "text/plain"}.
 */
static void *
_ecore_x_selection_parser_targets(const char *target EINA_UNUSED,
                                  void *data,
                                  int size,
                                  int format EINA_UNUSED)
{
   Ecore_X_Selection_Data_Targets *sel;
   int *targets;
   int i;

   sel = calloc(1, sizeof(Ecore_X_Selection_Data_Targets));
   if (!sel) return NULL;
   targets = data;

   sel->num_targets = size;
   sel->targets = malloc((sel->num_targets) * sizeof(char *));
   if (!sel->targets)
     {
        free(sel);
        return NULL;
     }
   for (i = 0; i < size; i++)
     sel->targets[i] = XGetAtomName(_ecore_x_disp, targets[i]);

   ECORE_X_SELECTION_DATA(sel)->free = _ecore_x_selection_data_targets_free;
   ECORE_X_SELECTION_DATA(sel)->content = ECORE_X_SELECTION_CONTENT_TARGETS;
   ECORE_X_SELECTION_DATA(sel)->length = size;
   ECORE_X_SELECTION_DATA(sel)->data = data;
   return sel;
}

static int
_ecore_x_selection_data_targets_free(void *data)
{
   Ecore_X_Selection_Data_Targets *sel;
   int i;

   sel = data;
   /**
    * @internal _ecore_x_selection_data_targets_free
    * @brief Frees an Ecore_X_Selection_Data_Targets structure.
    * This includes freeing the array of target name strings (which were allocated
    * by XGetAtomName) and the raw atom data buffer itself.
    * @param data Pointer to the Ecore_X_Selection_Data_Targets structure to free.
    * @return Always 1.
    */

   if (sel->targets)
     {
        for (i = 0; i < sel->num_targets; i++)
          XFree(sel->targets[i]); // Strings obtained from XGetAtomName
        free(sel->targets);
        sel->targets = NULL;
     }

   free(ECORE_X_SELECTION_DATA(sel)->data); // Free the original Atom array
   ECORE_X_SELECTION_DATA(sel)->data = NULL;
   free(sel);
   return 1;
}

