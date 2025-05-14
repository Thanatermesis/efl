#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <ctype.h>

#ifdef _WIN32
# include <direct.h> /* getcwd */
# include <winsock2.h>
#endif

#include <Ecore.h>
#include <Ecore_File.h>

/* define macros and variable for using the eina logging system  */
#define EFREET_MODULE_LOG_DOM _efreet_desktop_log_dom
extern int _efreet_desktop_log_dom;

#include "Efreet.h"
#include "efreet_private.h"

/**
 * @internal
 * @brief Flags indicating the types of file representations needed for an Exec command.
 *
 * These flags determine whether a full path or a URI (or both) is required
 * when processing files for a desktop entry's Exec command.
 */
typedef enum Efreet_Desktop_Command_Flag
{
    EFREET_DESKTOP_EXEC_FLAG_FULLPATH = 0x0001, /**< Indicates that a full file path is needed. */
    EFREET_DESKTOP_EXEC_FLAG_URI      = 0x0002  /**< Indicates that a URI representation is needed. */
} Efreet_Desktop_Command_Flag;

/**
 * @internal
 * @brief Represents a command to be executed, derived from a desktop entry.
 *
 * This structure holds all necessary information to build and execute
 * a command string based on a desktop file's Exec key. It manages
 * associated files, callbacks for command execution and progress,
 * and flags indicating how file arguments should be formatted.
 */
typedef struct Efreet_Desktop_Command Efreet_Desktop_Command;

/**
 * @internal
 * @struct Efreet_Desktop_Command
 * @brief Holds information on a desktop Exec command entry.
 *
 * This structure encapsulates the context for generating and executing
 * commands from a .desktop file, including handling file arguments,
 * downloads for remote files, and callbacks.
 */
struct Efreet_Desktop_Command
{
  Efreet_Desktop *desktop;              /**< The desktop entry this command is for. */
  int num_pending;                      /**< Number of files pending download or processing. */

  Efreet_Desktop_Command_Flag flags;    /**< Flags indicating required file representations (path/URI). */

  Efreet_Desktop_Command_Cb cb_command; /**< Callback function to execute the generated command. */
  Efreet_Desktop_Progress_Cb cb_progress;/**< Callback function for reporting progress (e.g., downloads). */
  void *data;                           /**< User data to be passed to callbacks. */

  Eina_List *files;                     /**< List of Efreet_Desktop_Command_File structures representing file arguments. */
};

/**
 * @internal
 * @brief Represents a file argument for a desktop command.
 *
 * This structure stores various representations of a file (directory,
 * filename, full path, URI) and its processing state, particularly
 * if it's a remote file requiring download.
 */
typedef struct Efreet_Desktop_Command_File Efreet_Desktop_Command_File;

/**
 * @internal
 * @struct Efreet_Desktop_Command_File
 * @brief Stores information on a file passed to the desktop Exec command.
 *
 * This includes different path formats and a flag to indicate if the
 * file processing is pending (e.g., waiting for a download).
 */
struct Efreet_Desktop_Command_File
{
  Efreet_Desktop_Command *command; /**< The parent command this file belongs to. */
  char *dir;                       /**< The directory part of the file path. */
  char *file;                      /**< The filename part of the file path. */
  char *fullpath;                  /**< The absolute local filesystem path to the file. */
  char *uri;                       /**< The URI representation of the file. */

  int pending;                     /**< Boolean flag, true if the file is pending processing (e.g., download). */
};

/**
 * @internal
 * @brief Callback function to execute a command string.
 * This function is typically called by efreet_desktop_command_get() or
 * efreet_desktop_command_progress_get() once a command string is fully constructed.
 * @param data User-provided data.
 * @param desktop The Efreet_Desktop structure (unused in this specific callback).
 * @param exec The command string to be executed.
 * @param remaining The number of remaining commands to be executed (unused in this specific callback).
 * @return Always returns NULL. The actual return of the executed command is not handled here.
 */
static void *efreet_desktop_exec_cb(void *data, Efreet_Desktop *desktop,
                                            char *exec, int remaining);
/**
 * @internal
 * @brief Determines the required file representation flags (fullpath, URI) based on the Exec string.
 * It parses the Exec string for field codes like %f, %F, %u, %U.
 * @param desktop The Efreet_Desktop structure containing the Exec string.
 * @return A bitmask of Efreet_Desktop_Command_Flag values.
 */
static int efreet_desktop_command_flags_get(Efreet_Desktop *desktop);
/**
 * @internal
 * @brief Processes a list of generated command strings by calling the command callback for each.
 * @param command The Efreet_Desktop_Command context.
 * @param execs An Eina_List of command strings (char *). Each string is a fully formed command.
 *              Example: `eina_list_append(NULL, "gedit /tmp/file1.txt");`
 * @return The return value of the last executed command callback.
 */
static void *efreet_desktop_command_execs_process(Efreet_Desktop_Command *command, Eina_List *execs);

/**
 * @internal
 * @brief Builds a list of executable command strings from the desktop entry's Exec field and file arguments.
 * It substitutes field codes (e.g., %f, %U, %c) with appropriate values.
 * @param command The Efreet_Desktop_Command context, containing the desktop entry and processed files.
 * @return An Eina_List of executable command strings (char *). The caller is responsible for freeing the list and its contents.
 *         Returns NULL on error.
 *         Example list structure:
 *         - "gnome-terminal --profile=Default" (if no file arguments and Exec="gnome-terminal --profile=%c")
 *         - "cat '/tmp/file A.txt'" (if Exec="cat %f" and one file "/tmp/file A.txt")
 *         - "vlc '/media/movie.mkv' '/media/sub.srt'" (if Exec="vlc %F" and two files)
 */
static Eina_List *efreet_desktop_command_build(Efreet_Desktop_Command *command);
/**
 * @internal
 * @brief Frees an Efreet_Desktop_Command structure and its associated data.
 * This includes freeing the list of Efreet_Desktop_Command_File structures.
 * @param command The Efreet_Desktop_Command structure to free.
 */
static void efreet_desktop_command_free(Efreet_Desktop_Command *command);
/**
 * @internal
 * @brief Appends a source string to a destination string, quoting it with single quotes.
 * Handles escaping of single quotes within the source string. The destination buffer is reallocated if necessary.
 * @param dest The destination string buffer. Will be reallocated if more space is needed.
 * @param size Pointer to the current allocated size of dest. Will be updated if realloc occurs.
 * @param len Pointer to the current length of content in dest. Will be updated.
 * @param src The source string to append and quote.
 * @return The (potentially reallocated) destination string, or NULL on allocation failure.
 */
static char *efreet_desktop_command_append_quoted(char *dest, int *size,
                                                    int *len, char *src);
/**
 * @internal
 * @brief Appends multiple file arguments (paths or URIs) to a command string, based on the specified type.
 * Each file argument is quoted. The destination buffer is reallocated if necessary.
 * @param dest The destination string buffer.
 * @param size Pointer to the current allocated size of dest.
 * @param len Pointer to the current length of content in dest.
 * @param command The Efreet_Desktop_Command context, containing the list of files.
 * @param type The field code type ('F' for multiple fullpaths, 'U' for multiple URIs, etc.).
 *             The actual appending uses the lowercase version of this type.
 * @return The (potentially reallocated) destination string, or NULL on allocation failure.
 */
static char *efreet_desktop_command_append_multiple(char *dest, int *size, int *len,
                                                    Efreet_Desktop_Command *command,
                                                    char type);
/**
 * @internal
 * @brief Appends a single file argument (path, URI, directory, or filename) to a command string, quoted.
 * The destination buffer is reallocated if necessary.
 * @param dest The destination string buffer.
 * @param size Pointer to the current allocated size of dest.
 * @param len Pointer to the current length of content in dest.
 * @param file The Efreet_Desktop_Command_File representing the file argument.
 * @param type The field code type ('f' for fullpath, 'u' for URI, 'd' for directory, 'n' for filename).
 * @return The (potentially reallocated) destination string, or NULL on allocation failure.
 */
static char *efreet_desktop_command_append_single(char *dest, int *size, int *len,
                                                Efreet_Desktop_Command_File *file,
                                                char type);
/**
 * @internal
 * @brief Appends the icon argument (e.g., "--icon /path/to/icon.png") to a command string.
 * The icon path is quoted. The destination buffer is reallocated if necessary.
 * @param dest The destination string buffer.
 * @param size Pointer to the current allocated size of dest.
 * @param len Pointer to the current length of content in dest.
 * @param desktop The Efreet_Desktop structure containing the icon information.
 * @return The (potentially reallocated) destination string, or NULL on allocation failure.
 */
static char *efreet_desktop_command_append_icon(char *dest, int *size, int *len,
                                                Efreet_Desktop *desktop);

/**
 * @internal
 * @brief Processes a file argument (path or URI) for a desktop command.
 * This involves determining if it's a local or remote file, converting "file:" URIs,
 * and initiating downloads if necessary for remote files when full paths are required.
 * @param command The Efreet_Desktop_Command context.
 * @param file A string representing the file, which can be a local path, a "file:" URI, or another URI scheme.
 *             Example: "/tmp/foo.txt", "file:///tmp/foo.txt", "http://example.com/foo.txt"
 * @return A newly allocated Efreet_Desktop_Command_File structure, or NULL on failure.
 *         The caller is responsible for freeing the returned structure if not NULL.
 */
static Efreet_Desktop_Command_File *efreet_desktop_command_file_process(
                                                    Efreet_Desktop_Command *command,
                                                    const char *file);
/**
 * @internal
 * @brief Extracts the local path from a "file:" URI.
 * Handles various forms of "file:" URIs, including those with hostnames (e.g., "file://localhost/path").
 * @param uri The "file:" URI string. Example: "file:///etc/fstab", "file:/tmp/test.txt"
 * @return A pointer to the path component within the URI string, or NULL if the URI is not local or malformed.
 *         The returned pointer is part of the input `uri` string, not a new allocation.
 */
static const char *efreet_desktop_command_file_uri_process(const char *uri);
/**
 * @internal
 * @brief Frees an Efreet_Desktop_Command_File structure and its string members.
 * @param file The Efreet_Desktop_Command_File structure to free.
 */
static void efreet_desktop_command_file_free(Efreet_Desktop_Command_File *file);

/**
 * @internal
 * @brief Callback invoked when a file download initiated by ecore_file_download() completes.
 * It updates the pending count in the associated Efreet_Desktop_Command. If all pending
 * operations are complete, it proceeds to build and execute the command(s).
 * @param data Custom data, expected to be an Efreet_Desktop_Command_File pointer.
 * @param file The local path of the downloaded file (unused in this function).
 * @param status The status of the download operation (unused in this function, but should be checked).
 */
static void efreet_desktop_cb_download_complete(void *data, const char *file,
                                                                int status);
/**
 * @internal
 * @brief Callback invoked periodically during a file download to report progress.
 * It calls the user-provided progress callback, if any.
 * @param data Custom data, expected to be an Efreet_Desktop_Command_File pointer.
 * @param file The local path of the file being downloaded (unused in this function).
 * @param dltotal Total download size.
 * @param dlnow Current downloaded size.
 * @param ultotal Total upload size (unused).
 * @param ulnow Current uploaded size (unused).
 * @return The return value of the user's progress callback, or 0 if no callback is set.
 *         A non-zero return from the user callback typically aborts the download.
 */
static int efreet_desktop_cb_download_progress(void *data, const char *file,
                                           long int dltotal, long int dlnow,
                                           long int ultotal, long int ulnow);

/**
 * @internal
 * @brief Converts a potentially relative path to an absolute path.
 * If the input path is already absolute, it's duplicated. If relative, it's resolved
 * against the current working directory.
 * @param path The input path string (can be relative or absolute).
 * @return A newly allocated string containing the absolute path, or NULL on failure (e.g., memory allocation, getcwd error).
 *         The caller is responsible for freeing the returned string.
 */
static char *efreet_desktop_command_path_absolute(const char *path);

/**
 * @internal
 * @brief Appends a source string to a destination buffer, reallocating the buffer if necessary.
 * Ensures the destination buffer has enough space for the appended string and a null terminator.
 * @param dest The destination character buffer. This buffer might be reallocated.
 * @param size Pointer to the integer holding the current allocated size of `dest`. Updated if realloc occurs.
 * @param len Pointer to the integer holding the current string length in `dest` (excluding null terminator). Updated after append.
 * @param src The null-terminated string to append.
 * @return Pointer to the (possibly reallocated) destination buffer, or NULL if reallocation fails.
 */
static char *efreet_string_append(char *dest, int *size,
                                    int *len, const char *src);
/**
 * @internal
 * @brief Appends a single character to a destination buffer, reallocating the buffer if necessary.
 * This is a convenience wrapper around efreet_string_append().
 * @param dest The destination character buffer. This buffer might be reallocated.
 * @param size Pointer to the integer holding the current allocated size of `dest`. Updated if realloc occurs.
 * @param len Pointer to the integer holding the current string length in `dest` (excluding null terminator). Updated after append.
 * @param c The character to append.
 * @return Pointer to the (possibly reallocated) destination buffer, or NULL if reallocation fails.
 */
static char *efreet_string_append_char(char *dest, int *size,
                                        int *len, char c);


EAPI void
efreet_desktop_exec(Efreet_Desktop *desktop, Eina_List *files, void *data)
{
    efreet_desktop_command_get(desktop, files, efreet_desktop_exec_cb, data);
}

EAPI void *
efreet_desktop_command_get(Efreet_Desktop *desktop, Eina_List *files,
                            Efreet_Desktop_Command_Cb func, void *data)
{
    return efreet_desktop_command_progress_get(desktop, files, func, NULL, data);
}

EAPI Eina_List *
efreet_desktop_command_local_get(Efreet_Desktop *desktop, Eina_List *files)
{
    Efreet_Desktop_Command *command;
    char *file;
    Eina_List *execs, *l;

    EINA_SAFETY_ON_NULL_RETURN_VAL(desktop, NULL);
    EINA_SAFETY_ON_NULL_RETURN_VAL(desktop->exec, NULL);

    command = NEW(Efreet_Desktop_Command, 1);
    if (!command) return 0;

    command->desktop = desktop;

    command->flags = efreet_desktop_command_flags_get(desktop);
    /* get the required info for each file passed in */
    if (files)
    {
        EINA_LIST_FOREACH(files, l, file)
        {
            Efreet_Desktop_Command_File *dcf;

            dcf = efreet_desktop_command_file_process(command, file);
            if (!dcf) continue;
            if (dcf->pending)
            {
                efreet_desktop_command_file_free(dcf);
                continue;
            }
            command->files = eina_list_append(command->files, dcf);
        }
    }

    execs = efreet_desktop_command_build(command);
    efreet_desktop_command_free(command);

    return execs;
}

EAPI void *
efreet_desktop_command_progress_get(Efreet_Desktop *desktop, Eina_List *files,
                                    Efreet_Desktop_Command_Cb cb_command,
                                    Efreet_Desktop_Progress_Cb cb_progress,
                                    void *data)
{
    Efreet_Desktop_Command *command;
    Eina_List *l;
    char *file;
    void *ret = NULL;

    EINA_SAFETY_ON_NULL_RETURN_VAL(desktop, NULL);
    EINA_SAFETY_ON_NULL_RETURN_VAL(desktop->exec, NULL);
    EINA_SAFETY_ON_NULL_RETURN_VAL(cb_command, NULL);

    command = NEW(Efreet_Desktop_Command, 1);
    if (!command) return NULL;

    command->cb_command = cb_command;
    command->cb_progress = cb_progress;
    command->data = data;
    command->desktop = desktop;
    command->num_pending = 0;

    command->flags = efreet_desktop_command_flags_get(desktop);
    /* get the required info for each file passed in */
    if (files)
    {
        EINA_LIST_FOREACH(files, l, file)
        {
            Efreet_Desktop_Command_File *dcf;

            dcf = efreet_desktop_command_file_process(command, file);
            if (!dcf) continue;
            command->files = eina_list_append(command->files, dcf);
            command->num_pending += dcf->pending;
        }
    }

    if (command->num_pending == 0)
    {
        Eina_List *execs;

        execs = efreet_desktop_command_build(command);
        if (execs)
        {
            ret = efreet_desktop_command_execs_process(command, execs);
            eina_list_free(execs);
        }
        efreet_desktop_command_free(command);
    }

    return ret;
}

static void *
efreet_desktop_exec_cb(void *data,
                       Efreet_Desktop *desktop EINA_UNUSED,
                       char *exec,
                       int remaining EINA_UNUSED)
{
    ecore_exe_run(exec, data);
    free(exec);

    return NULL;
}

/* efreet_desktop_command_flags_get already has a Doxygen comment */
static int
efreet_desktop_command_flags_get(Efreet_Desktop *desktop)
{
    int flags = 0;
    const char *p;
    /* first, determine which fields are present in the Exec string */
    p = strchr(desktop->exec, '%');
    while (p)
    {
        p++;
        switch(*p)
        {
            case 'f':
            case 'F':
                flags |= EFREET_DESKTOP_EXEC_FLAG_FULLPATH;
                break;
            case 'u':
            case 'U':
                flags |= EFREET_DESKTOP_EXEC_FLAG_URI;
                break;
            case '%':
                p++;
                break;
            default:
                break;
        }

        p = strchr(p, '%');
    }
#ifdef SLOPPY_SPEC
    /* NON-SPEC!!! this is to work around LOTS of 'broken' .desktop files that
     * do not specify %U/%u, %F/F etc. etc. at all. just a command. this is
     * unlikely to be fixed in distributions etc. in the long run as gnome/kde
     * seem to have workarounds too so no one notices.
     */
    if (!flags) flags |= EFREET_DESKTOP_EXEC_FLAG_FULLPATH;
#endif

    return flags;
}


/* efreet_desktop_command_execs_process already has a Doxygen comment (updated above) */
static void *
efreet_desktop_command_execs_process(Efreet_Desktop_Command *command, Eina_List *execs)
{
    Eina_List *l;
    char *exec;
    int num;
    void *ret = NULL;

    num = eina_list_count(execs);
    EINA_LIST_FOREACH(execs, l, exec)
    {
        ret = command->cb_command(command->data, command->desktop, exec, --num);
    }
    return ret;
}


/* efreet_desktop_command_build already has a Doxygen comment (updated above) */
static Eina_List *
efreet_desktop_command_build(Efreet_Desktop_Command *command)
{
    Eina_List *execs = NULL;
    const Eina_List *l;
    char *exec;

    /* if the Exec field appends multiple, that will run the list to the end,
     * causing this loop to only run once. otherwise, this loop will generate a
     * command for each file in the list. if the list is empty, this
     * will run once, removing any file field codes */
    l = command->files;
    do
    {
        const char *p;
        int len = 0;
        int size = PATH_MAX;
        int file_added = 0;
        Efreet_Desktop_Command_File *file = eina_list_data_get(l);
       int single;

        exec = malloc(size);
        if (!exec) goto error;
        p = command->desktop->exec;
        len = 0;

        single = 0;
        while (*p)
        {
            if (len >= size - 1)
            {
                char *tmp;

                size = len + 1024;
                tmp = realloc(exec, size);
                if (!tmp) goto error;
                exec = tmp;
            }

            /* XXX handle fields inside quotes? */
            if (*p == '%')
            {
                p++;
                switch (*p)
                {
                    case 'f':
                    case 'u':
                    case 'd':
                    case 'n':
                        if (file)
                        {
                            exec = efreet_desktop_command_append_single(exec, &size,
                                    &len, file, *p);
                            if (!exec) goto error;
                            file_added = 1;
                            single = 1;
                        }
                        break;
                    case 'F':
                    case 'U':
                    case 'D':
                    case 'N':
                        if (file)
                        {
                            exec = efreet_desktop_command_append_multiple(exec, &size,
                                    &len, command, *p);
                            if (!exec) goto error;
                            file_added = 1;
                            /* Set l to NULL to break the loop, since we parse all command->files
                             * in efreet_desktop_command_append_multiple */
                            l = NULL;
                        }
                        break;
                    case 'i':
                        exec = efreet_desktop_command_append_icon(exec, &size, &len,
                                command->desktop);
                        if (!exec) goto error;
                        break;
                    case 'c':
                        exec = efreet_desktop_command_append_quoted(exec, &size, &len,
                                command->desktop->name);
                        if (!exec) goto error;
                        break;
                    case 'k':
                        exec = efreet_desktop_command_append_quoted(exec, &size, &len,
                                command->desktop->orig_path);
                        if (!exec) goto error;
                        break;
                    case 'v':
                    case 'm':
                        WRN("Deprecated conversion char: '%c' in file '%s'",
                                *p, command->desktop->orig_path);
                        break;
                    case '%':
                        exec[len++] = *p;
                        break;
                    default:
#ifdef STRICT_SPEC
                        WRN("Unknown conversion character: '%c' in file '%s'", *p, command->desktop->orig_path);
#endif
                        break;
                }
            }
            else exec[len++] = *p;
            p++;
        }

#ifdef SLOPPY_SPEC
        /* NON-SPEC!!! this is to work around LOTS of 'broken' .desktop files that
         * do not specify %U/%u, %F/F etc. etc. at all. just a command. this is
         * unlikely to be fixed in distributions etc. in the long run as gnome/kde
         * seem to have workarounds too so no one notices.
         */
        if ((file) && (!file_added))
        {
            WRN("Efreet_desktop: %s\n"
                "  command: %s\n"
                "  has no file path/uri spec info for executing this app WITH a\n"
                "  file/uri as a parameter. This is unlikely to be the intent.\n"
                "  please check the .desktop file and fix it by adding a %%U or %%F\n"
                "  or something appropriate.",
                command->desktop->orig_path, command->desktop->exec);
            if (len >= size - 1)
            {
                char *tmp;
                size = len + 1024;
                tmp = realloc(exec, size);
                if (!tmp) goto error;
                exec = tmp;
            }
            exec[len++] = ' ';
            exec = efreet_desktop_command_append_multiple(exec, &size,
                    &len, command, 'F');
            if (!exec) goto error;
            file_added = 1;
        }
#endif
        exec[len++] = '\0';

       if ((single) || (!execs))
         {
            execs = eina_list_append(execs, exec);
            exec = NULL;
         }

        /* If no file was added, then the Exec field doesn't contain any file
         * fields (fFuUdDnN). We only want to run the app once in this case. */
        if (!file_added) break;
    }
    while ((l = eina_list_next(l)));

    IF_FREE(exec);
    return execs;
error:
    IF_FREE(exec);
    EINA_LIST_FREE(execs, exec)
        free(exec);
    return NULL;
}

/* efreet_desktop_command_free already has a Doxygen comment (updated above) */
static void
efreet_desktop_command_free(Efreet_Desktop_Command *command)
{
    Efreet_Desktop_Command_File *dcf;

    if (!command) return;

    while (command->files)
    {
        dcf = eina_list_data_get(command->files);
        efreet_desktop_command_file_free(dcf);
        command->files = eina_list_remove_list(command->files,
                                               command->files);
    }
    FREE(command);
}

/* efreet_desktop_command_append_quoted already has a Doxygen comment (updated above) */
static char *
efreet_desktop_command_append_quoted(char *dest, int *size, int *len, char *src)
{
    if (!src) return dest;
    dest = efreet_string_append(dest, size, len, "'");
    if (!dest) return NULL;

    /* single quotes in src need to be escaped */
    if (strchr(src, '\''))
    {
        char *p;
        p = src;
        while (*p)
        {
            if (*p == '\'')
            {
                dest = efreet_string_append(dest, size, len, "\'\\\'");
                if (!dest) return NULL;
            }

            dest = efreet_string_append_char(dest, size, len, *p);
            if (!dest) return NULL;
            p++;
        }
    }
    else
    {
        dest = efreet_string_append(dest, size, len, src);
        if (!dest) return NULL;
    }

    dest = efreet_string_append(dest, size, len, "'");
    if (!dest) return NULL;

    return dest;
}

/* efreet_desktop_command_append_multiple already has a Doxygen comment (updated above) */
static char *
efreet_desktop_command_append_multiple(char *dest, int *size, int *len,
                                        Efreet_Desktop_Command *command,
                                        char type)
{
    Efreet_Desktop_Command_File *file;
    Eina_List *l;
    int first = 1;

    if (!command->files) return dest;

    EINA_LIST_FOREACH(command->files, l, file)
    {
        if (first)
            first = 0;
        else
        {
            dest = efreet_string_append_char(dest, size, len, ' ');
            if (!dest) return NULL;
        }

        dest = efreet_desktop_command_append_single(dest, size, len,
                                                    file, tolower(type));
        if (!dest) return NULL;
    }

    return dest;
}

/* efreet_desktop_command_append_single already has a Doxygen comment (updated above) */
static char *
efreet_desktop_command_append_single(char *dest, int *size, int *len,
                                        Efreet_Desktop_Command_File *file,
                                        char type)
{
    char *str;
    switch(type)
    {
        case 'f':
            str = file->fullpath;
            break;
        case 'u':
            str = file->uri;
            break;
        case 'd':
            str = file->dir;
            break;
        case 'n':
            str = file->file;
            break;
        default:
            ERR("Invalid type passed to efreet_desktop_command_append_single:"
                                                                " '%c'", type);
            return dest;
    }

    if (!str) return dest;

    dest = efreet_desktop_command_append_quoted(dest, size, len, str);
    if (!dest) return NULL;

    return dest;
}

/* efreet_desktop_command_append_icon already has a Doxygen comment (updated above) */
static char *
efreet_desktop_command_append_icon(char *dest, int *size, int *len,
                                            Efreet_Desktop *desktop)
{
    if (!desktop->icon || !desktop->icon[0]) return dest;

    dest = efreet_string_append(dest, size, len, "--icon ");
    if (!dest) return NULL;
    dest = efreet_desktop_command_append_quoted(dest, size, len, desktop->icon);
    if (!dest) return NULL;

    return dest;
}

/**
 * @internal
 * @brief Checks if a given path string starts with a protocol scheme (e.g., "http:", "ftp:").
 * A protocol is identified by a colon appearing before any slash.
 * @param path The path string to check.
 * @return EINA_TRUE if a protocol is detected, EINA_FALSE otherwise.
 */
static Eina_Bool
_is_protocol(const char *path)
{
    Eina_Bool nonlocal = EINA_FALSE;
    char *p = (char*)path;
    while (!nonlocal && *p && *p != '/')
    {
       nonlocal = (*p == ':');
       p++;
    }
   return nonlocal;
}

/* efreet_desktop_command_file_process already has a Doxygen comment (updated above) */
static Efreet_Desktop_Command_File *
efreet_desktop_command_file_process(Efreet_Desktop_Command *command, const char *file)
{
    Efreet_Desktop_Command_File *f;
    const char *uri, *base;
    int nonlocal = 0;
    f = NEW(Efreet_Desktop_Command_File, 1);
    if (!f) return NULL;

    f->command = command;

    /* handle uris */
    if (!strncmp(file, "file:", 5))
    {
        file = efreet_desktop_command_file_uri_process(file);
        if (!file)
        {
            efreet_desktop_command_file_free(f);
            return NULL;
        }
    }
    else if (_is_protocol(file))
    {
        uri = file;
        base = ecore_file_file_get(file);

        nonlocal = 1;
    }

    if (nonlocal)
    {
        /* process non-local uri */
        if (command->flags & EFREET_DESKTOP_EXEC_FLAG_FULLPATH)
        {
            char buf[PATH_MAX];
            Eina_Tmpstr *dest;
            int fd;

            snprintf(buf, sizeof(buf), "%s_XXXXXX", base);
            fd = eina_file_mkstemp(buf, &dest);
            if (fd >= 0)
            {
                close(fd);
                f->fullpath = strdup(dest);
                f->pending = 1;
                eina_tmpstr_del(dest);

                ecore_file_download(uri, f->fullpath, efreet_desktop_cb_download_complete,
                                    efreet_desktop_cb_download_progress, f, NULL);
            }
        }

        if (command->flags & EFREET_DESKTOP_EXEC_FLAG_URI)
            f->uri = strdup(uri);
    }
    else
    {
        char *absol = efreet_desktop_command_path_absolute(file);
        if (!absol) goto error;
        /* process local uri/path */
        if (command->flags & EFREET_DESKTOP_EXEC_FLAG_FULLPATH)
            f->fullpath = strdup(absol);

        if (command->flags & EFREET_DESKTOP_EXEC_FLAG_URI)
        {
            const char *buf;
            Efreet_Uri ef_uri;
            ef_uri.protocol = "file";
            ef_uri.hostname = "";
            ef_uri.path = absol;
            buf = efreet_uri_encode(&ef_uri);

            f->uri = strdup(buf);

            eina_stringshare_del(buf);
        }

        free(absol);
    }
    return f;
error:
    IF_FREE(f);
    return NULL;
}

/* efreet_desktop_command_file_uri_process already has a Doxygen comment (updated above) */
static const char *
efreet_desktop_command_file_uri_process(const char *uri)
{
    const char *path = NULL;
    int len = strlen(uri);

    /* uri:foo/bar => relative path foo/bar*/
    if (len >= 4 && uri[5] != '/')
        path = uri + strlen("file:");

    /* uri:/foo/bar => absolute path /foo/bar */
    else if (len >= 5 && uri[6] != '/')
        path = uri + strlen("file:");

    /* uri://foo/bar => absolute path /bar on machine foo */
    else if (len >= 6 && uri[7] != '/')
    {
        char *tmp, *p;
        char hostname[PATH_MAX];
        size_t len2;

        len2 = strlen(uri + 7) + 1;
        tmp = alloca(len2);
        memcpy(tmp, uri + 7, len2);
        p = strchr(tmp, '/');
        if (p)
        {
            *p = '\0';
            if (!strcmp(tmp, "localhost"))
                path = uri + strlen("file://localhost");
            else
            {
                int ret;

                ret = gethostname(hostname, PATH_MAX);
                if ((ret == 0) && !strcmp(tmp, hostname))
                    path = uri + strlen("file://") + strlen(hostname);
            }
        }
    }

    /* uri:///foo/bar => absolute path /foo/bar on local machine */
    else if (len >= 7)
        path = uri + strlen("file://");

    return path;
}

/* efreet_desktop_command_file_free already has a Doxygen comment (updated above) */
static void
efreet_desktop_command_file_free(Efreet_Desktop_Command_File *file)
{
    if (!file) return;

    IF_FREE(file->fullpath);
    IF_FREE(file->uri);
    IF_FREE(file->dir);
    IF_FREE(file->file);

    FREE(file);
}


/* efreet_desktop_cb_download_complete already has a Doxygen comment (updated above) */
static void
efreet_desktop_cb_download_complete(void *data, const char *file EINA_UNUSED,
                                                        int status EINA_UNUSED)
{
    Efreet_Desktop_Command_File *f;

    f = data;

    /* XXX check status... error handling, etc */
    f->pending = 0;
    f->command->num_pending--;

    if (f->command->num_pending <= 0)
    {
        Eina_List *execs;

        execs = efreet_desktop_command_build(f->command);
        if (execs)
        {
            /* TODO: Need to handle the return value from efreet_desktop_command_execs_process */
            efreet_desktop_command_execs_process(f->command, execs);
            eina_list_free(execs);
        }
        efreet_desktop_command_free(f->command);
    }
}

/* efreet_desktop_cb_download_progress already has a Doxygen comment (updated above) */
static int
efreet_desktop_cb_download_progress(void *data,
                                    const char *file EINA_UNUSED,
                                    long int dltotal, long int dlnow,
                                    long int ultotal EINA_UNUSED,
                                    long int ulnow EINA_UNUSED)
{
    Efreet_Desktop_Command_File *dcf;

    dcf = data;
    if (dcf->command->cb_progress)
        return dcf->command->cb_progress(dcf->command->data,
                                        dcf->command->desktop,
                                        dcf->uri, dltotal, dlnow);

    return 0;
}

/* efreet_desktop_command_path_absolute already has a Doxygen comment (updated above) */
static char *
efreet_desktop_command_path_absolute(const char *path)
{
    char *buf;
    int size = PATH_MAX;
    int len = 0;

    /* relative url */
    if (eina_file_path_relative(path))
    {
        if (!(buf = malloc(size))) return NULL;
        if (!getcwd(buf, size))
        {
            FREE(buf);
            return NULL;
        }
        len = strlen(buf);

        if (buf[len-1] != '/') buf = efreet_string_append(buf, &size, &len, "/");
        if (!buf) return NULL;
        buf = efreet_string_append(buf, &size, &len, path);
        if (!buf) return NULL;

        return buf;
    }

    /* just dup an already absolute buffer */
    return strdup(path);
}

/* efreet_string_append already has a Doxygen comment (updated above) */
static char *
efreet_string_append(char *dest, int *size, int *len, const char *src)
{
   int append_len = strlen(src);

   if ((*len + append_len + 1) > *size)
     {
        char *dest2 = realloc(dest, *size + append_len + 1024);
        if (!dest2) return NULL;

        dest = dest2;
        *size += append_len + 1024;
     }
   strcpy(dest + *len, src);
   *len += append_len;
   return dest;
}

/* efreet_string_append_char already has a Doxygen comment (updated above) */
static char *
efreet_string_append_char(char *dest, int *size, int *len, char c)
{
   char str[2];

   str[0] = c;
   str[1] = 0;
   return efreet_string_append(dest, size, len, str);
}

