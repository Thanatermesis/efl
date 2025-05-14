/**
 * @file
 * @brief These routines are used for controlling external processes.
 */
#ifndef _ECORE_EXE_EO_H_
#define _ECORE_EXE_EO_H_

#ifndef _ECORE_EXE_EO_CLASS_TYPE
#define _ECORE_EXE_EO_CLASS_TYPE

/**
 * @struct Ecore_Exe
 * Opaque handle to manage Ecore Exe objects.
 * @ingroup Ecore_Exe
 */
typedef Eo Ecore_Exe;

#endif

#ifndef _ECORE_EXE_EO_TYPES
#define _ECORE_EXE_EO_TYPES

/**
 * @brief A structure which stores information on lines data from a child process.
 *
 * This structure is used when line-buffered reading is enabled for stdout or stderr.
 * Each instance represents a single line of text.
 *
 * @ingroup Ecore_Exe_Event_Data
 */
typedef struct _Ecore_Exe_Event_Data_Line
{
  char *line; /**< The null-terminated string for the line of buffered data. This memory is owned by Ecore and should not be freed by the user. It is valid until the event callback returns. */
  int size; /**< The size of the line buffer in bytes, including the null terminator. */
} Ecore_Exe_Event_Data_Line;

/**
 * @brief Ecore exe event data structure.
 *
 * This structure is passed as event information for data and error events from
 * a spawned process.
 *
 * @ingroup Ecore_Exe
 */
typedef struct _Ecore_Exe_Event_Data
{
  Efl_Object *exe; /**< The handle to the process. FIXME: should actually be Ecore.Exe, workaround cyclic dependency. This is the Ecore_Exe object that spawned the process. */
  void *data; /**< The raw binary data from the child process received. This is @c NULL if line buffering is enabled. This memory is owned by Ecore and should not be freed by the user. It is valid until the event callback returns. */
  int size; /**< The size of this data in bytes. If line buffering is enabled, this indicates the number of lines in the @c lines array. */
  Ecore_Exe_Event_Data_Line *lines; /**< An array of line data if line buffered.
                                     * This is @c NULL if line buffering is not enabled.
                                     * The array is terminated by an entry where the @c line member is @c NULL.
                                     * For example:
                                     * @code
                                     * Ecore_Exe_Event_Data_Line *line_ptr;
                                     * for (line_ptr = event_info->lines; line_ptr->line; line_ptr++)
                                     *   {
                                     *      printf("Line: %s\n", line_ptr->line);
                                     *   }
                                     * @endcode
                                     * This memory is owned by Ecore and should not be freed by the user. It is valid until the event callback returns.
                                     */
} Ecore_Exe_Event_Data;

/**
 * @brief Flags for executing a child process.
 *
 * These flags control how a child process is spawned and how its I/O streams
 * are handled. They can be bitwise-ORed together.
 *
 * @ingroup Ecore
 */
typedef enum
{
  ECORE_EXE_NONE = 0, /**< No special flags. Standard behavior. */
  ECORE_EXE_PIPE_READ = 1, /**< Pipe the child process's stdout to the parent. Data is received via the #ECORE_EXE_EVENT_DATA_GET event. */
  ECORE_EXE_PIPE_WRITE = 2, /**< Pipe the parent's output to the child process's stdin. Data can be sent using ecore_exe_send(). */
  ECORE_EXE_PIPE_ERROR = 4, /**< Pipe the child process's stderr to the parent. Data is received via the #ECORE_EXE_EVENT_DATA_ERROR event. */
  ECORE_EXE_PIPE_READ_LINE_BUFFERED = 8, /**< Buffer stdout data from the child process until a newline character is encountered.
                                          * Each line is then sent as a separate #Ecore_Exe_Event_Data_Line within the #ECORE_EXE_EVENT_DATA_GET event.
                                          * This flag implies #ECORE_EXE_PIPE_READ. */
  ECORE_EXE_PIPE_ERROR_LINE_BUFFERED = 16, /**< Buffer stderr data from the child process until a newline character is encountered.
                                            * Each line is then sent as a separate #Ecore_Exe_Event_Data_Line within the #ECORE_EXE_EVENT_DATA_ERROR event.
                                            * This flag implies #ECORE_EXE_PIPE_ERROR. */
  ECORE_EXE_PIPE_AUTO = 32, /**< Automatically buffer stdout and stderr from the child process.
                             * This is a convenience flag equivalent to (#ECORE_EXE_PIPE_READ | #ECORE_EXE_PIPE_ERROR | #ECORE_EXE_PIPE_READ_LINE_BUFFERED | #ECORE_EXE_PIPE_ERROR_LINE_BUFFERED). */
  ECORE_EXE_RESPAWN = 64, /**< FIXME: This flag is intended to make Ecore automatically restart the child process if it exits. Currently not fully implemented or reliable. */
  ECORE_EXE_USE_SH = 128, /**< Execute the command using `/bin/sh -c "command"`.
                           * This allows shell interpretation of the command string (e.g., wildcards, pipelines).
                           * This flag is unused on Windows platforms. */
  ECORE_EXE_NOT_LEADER = 256, /**< Do not call `setsid()` for the child process.
                               * By default, Ecore makes the child process a new session leader.
                               * This flag prevents that, keeping the child in the same session as the parent. */
  ECORE_EXE_TERM_WITH_PARENT = 512, /**< Attempt to ensure the child process receives a SIGTERM signal when its parent process exits.
                                     * The exact mechanism may vary by platform (e.g., `prctl(PR_SET_PDEATHSIG, SIGTERM)` on Linux). */
  ECORE_EXE_ISOLATE_IO = 1024 /**< Try to isolate the stdin, stdout, and stderr of the child process from the parent.
                               * This is particularly relevant if not using pipes, to prevent the child from inheriting or interfering with the parent's standard I/O. */
} Ecore_Exe_Flags;


#endif
/**
 * @brief Ecore.Exe is responsible for managing portable process spawning.
 *
 * With this module you are able to spawn, pause, and quit spawned processes.
 * Interaction between your process and those spawned is possible using pipes
 * for I/O or signals for process control.
 *
 * @ingroup Ecore_Exe
 */
#define ECORE_EXE_CLASS ecore_exe_class_get() /**< Macro to get the Ecore_Exe class. @see ecore_exe_class_get */

/**
 * @brief Retrieves the Ecore_Exe Efl_Class.
 *
 * @return The Efl_Class for Ecore_Exe.
 * @ingroup Ecore_Exe
 */
EWAPI const Efl_Class *ecore_exe_class_get(void) EINA_CONST;

/**
 * @brief Sets the command string and execution flags for an Ecore_Exe object.
 *
 * This function configures the command to be executed and how it should be run.
 * This function should be called before the object is finalized or the process is started.
 *
 * @param[in] obj The Ecore_Exe object.
 * @param[in] exe_cmd The command string to execute (e.g., "/bin/ls -l", "my_script.sh").
 *                    If #ECORE_EXE_USE_SH is set in @p flags, this command is passed to `/bin/sh -c`.
 * @param[in] flags A bitmask of #Ecore_Exe_Flags to control execution behavior.
 *                  For example, to pipe stdout and stderr with line buffering:
 *                  `ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR_LINE_BUFFERED`.
 *
 * @ingroup Ecore_Exe
 */
EOAPI void ecore_obj_exe_command_set(Eo *obj, const char *exe_cmd, Ecore_Exe_Flags flags);

/**
 * @brief Retrieves the command string and execution flags for an Ecore_Exe object.
 *
 * This function allows introspection of the currently configured command and flags.
 *
 * @param[in] obj The Ecore_Exe object.
 * @param[out] exe_cmd A pointer to a character string where the command will be stored.
 *                     The lifetime of this string is managed by the Ecore_Exe object. Do not free it.
 * @param[out] flags A pointer to an #Ecore_Exe_Flags variable where the current flags will be stored.
 *
 * @ingroup Ecore_Exe
 */
EOAPI void ecore_obj_exe_command_get(const Eo *obj, const char **exe_cmd, Ecore_Exe_Flags *flags);

EWAPI extern const Efl_Event_Description _ECORE_EXE_EVENT_DATA_GET;

/**
 * @brief Event triggered when data is received from the child process's stdout.
 *
 * The event_info field of the Efl_Event will be a pointer to an #Ecore_Exe_Event_Data structure.
 * This event is only triggered if #ECORE_EXE_PIPE_READ or related flags (like #ECORE_EXE_PIPE_READ_LINE_BUFFERED)
 * were set when the command was configured.
 *
 * Example usage:
 * @code
 * static void
 * _exe_data_cb(void *data EINA_UNUSED, const Efl_Event *event)
 * {
 *    Ecore_Exe_Event_Data *ev = event->info;
 *    if (ev->lines) // Line buffered
 *    {
 *       Ecore_Exe_Event_Data_Line *line_ptr;
 *       for (line_ptr = ev->lines; line_ptr->line; line_ptr++)
 *         printf("STDOUT Line: %.*s\n", line_ptr->size -1, line_ptr->line); // -1 to exclude null terminator from printf precision
 *    }
 *    else if (ev->data && ev->size > 0) // Raw data
 *    {
 *       printf("STDOUT Raw: %.*s\n", ev->size, (char*)ev->data);
 *    }
 * }
 *
 * efl_event_callback_add(exe, ECORE_EXE_EVENT_DATA_GET, _exe_data_cb, NULL);
 * @endcode
 *
 * @return A pointer to an #Ecore_Exe_Event_Data structure containing the received data.
 * @ingroup Ecore_Exe
 */
#define ECORE_EXE_EVENT_DATA_GET (&(_ECORE_EXE_EVENT_DATA_GET))

EWAPI extern const Efl_Event_Description _ECORE_EXE_EVENT_DATA_ERROR;

/**
 * @brief Event triggered when data is received from the child process's stderr.
 *
 * The event_info field of the Efl_Event will be a pointer to an #Ecore_Exe_Event_Data structure.
 * This event is only triggered if #ECORE_EXE_PIPE_ERROR or related flags (like #ECORE_EXE_PIPE_ERROR_LINE_BUFFERED)
 * were set when the command was configured.
 *
 * The usage is analogous to #ECORE_EXE_EVENT_DATA_GET.
 *
 * @return A pointer to an #Ecore_Exe_Event_Data structure containing the received error data.
 * @ingroup Ecore_Exe
 */
#define ECORE_EXE_EVENT_DATA_ERROR (&(_ECORE_EXE_EVENT_DATA_ERROR))

#endif
