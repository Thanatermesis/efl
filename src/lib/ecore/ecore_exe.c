#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Efl.h>

#include "Ecore.h"
#include "ecore_private.h"

#define MY_CLASS ECORE_EXE_CLASS

#include "ecore_exe_private.h"

/* TODO: Something to let people build a command line and does auto escaping -
 *
 * ecore_exe_snprintf()
 *
 *   OR
 *
 * cmd = ecore_exe_comand_parameter_append(cmd, "firefox");
 * cmd = ecore_exe_comand_parameter_append(cmd, "http://www.foo.com/bar.html?baz=yes");
 * each parameter appended is one argument, and it gets escaped, quoted, and
 * appended with a preceding space.  The first is the command off course.
 */

struct _ecore_exe_dead_exe
{
   pid_t pid;
   char *cmd;
};

#ifdef _WIN32
/*
 * this job is used to close child processes when parent one is closed
 * see https://stackoverflow.com/a/53214/688348
 */
HANDLE _ecore_exe_win32_job = NULL;
#endif

/**
 * @brief Event type for when a new Ecore_Exe process is started.
 * @ingroup Ecore_Exe_Group
 */
EAPI int ECORE_EXE_EVENT_ADD = 0;
/**
 * @brief Event type for when an Ecore_Exe process terminates.
 * @ingroup Ecore_Exe_Group
 */
EAPI int ECORE_EXE_EVENT_DEL = 0;
/**
 * @brief Event type for when an Ecore_Exe process sends data (stdout/stderr).
 * @ingroup Ecore_Exe_Group
 */
EAPI int ECORE_EXE_EVENT_DATA = 0;
/**
 * @brief Event type for when an Ecore_Exe process sends error data (stderr, if separated).
 * @ingroup Ecore_Exe_Group
 */
EAPI int ECORE_EXE_EVENT_ERROR = 0;

/**
 * @brief Sets the priority for processes run by ecore_exe_run() or ecore_exe_pipe_run().
 *
 * @param pri The priority to set. This value is typically platform-dependent.
 *            For POSIX systems, this might correspond to nice levels.
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_run_priority_set(int pri)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   _impl_ecore_exe_run_priority_set(pri);
}

/**
 * @brief Gets the current priority set for processes run by ecore_exe.
 *
 * @return The current priority value.
 * @ingroup Ecore_Exe_Group
 */
EAPI int
ecore_exe_run_priority_get(void)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(0);
   return _impl_ecore_exe_run_priority_get();
}

/**
 * @brief Runs the given command.
 *
 * This function forks and runs the command @p exe_cmd.
 *
 * @param exe_cmd The command to run with all its arguments.
 *                Example: "ls -l /tmp"
 * @param data User data to associate with this Ecore_Exe instance.
 * @return A new Ecore_Exe object if successful, @c NULL otherwise.
 * @ingroup Ecore_Exe_Group
 */
EAPI Ecore_Exe *
ecore_exe_run(const char *exe_cmd,
              const void *data)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   return ecore_exe_pipe_run(exe_cmd, 0, data);
}

/**
 * @brief Runs the given command with specified pipe behavior.
 *
 * This function is a more advanced version of ecore_exe_run() that allows
 * specifying how the child process's standard I/O streams (stdin, stdout, stderr)
 * should be handled using @p flags.
 *
 * @param exe_cmd The command to run with all its arguments.
 *                Example: "my_program --input /dev/null"
 * @param flags Flags to control pipe behavior (e.g., ECORE_EXE_PIPE_READ, ECORE_EXE_PIPE_WRITE).
 * @param data User data to associate with this Ecore_Exe instance.
 * @return A new Ecore_Exe object if successful, @c NULL otherwise.
 * @ingroup Ecore_Exe_Group
 */
EAPI Ecore_Exe *
ecore_exe_pipe_run(const char      *exe_cmd,
                   Ecore_Exe_Flags  flags,
                   const void      *data)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   Ecore_Exe *ret = efl_add(MY_CLASS, efl_main_loop_get(),
                            ecore_obj_exe_command_set(efl_added, exe_cmd,
                                                      flags));
   if (ret)
     {
        Ecore_Exe_Data *pd = efl_data_scope_get(ret, MY_CLASS);
        pd->data = (void *) data;
     }
   return ret;
}

EOLIAN static void
_ecore_exe_command_set(Eo *obj EINA_UNUSED, Ecore_Exe_Data *pd, const char *cmd, Ecore_Exe_Flags flags)
{
   if (!cmd) return;
   pd->cmd = strdup(cmd);
   pd->flags = flags;
}

EOLIAN static void
_ecore_exe_command_get(const Eo *obj EINA_UNUSED, Ecore_Exe_Data *pd, const char **cmd, Ecore_Exe_Flags *flags)
{
   if (cmd) *cmd = pd->cmd;
   if (flags) *flags = pd->flags;
}

EOLIAN static Eo *
_ecore_exe_efl_object_finalize(Eo *obj, Ecore_Exe_Data *exe)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   obj = efl_finalize(efl_super(obj, MY_CLASS));
   if (!obj) return obj;
   return _impl_ecore_exe_efl_object_finalize(obj, exe);
}

/**
 * @brief Sets a callback function to be called just before an Ecore_Exe object is freed.
 *
 * This can be used for cleaning up any resources associated with the Ecore_Exe
 * that are managed outside of the Ecore_Exe itself.
 *
 * @param obj The Ecore_Exe object.
 * @param func The callback function to set.
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_callback_pre_free_set(Ecore_Exe   *obj,
                                Ecore_Exe_Cb func)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return;
   exe->pre_free_cb = func;
}

/**
 * @brief Sends data to the stdin of the running process.
 *
 * The Ecore_Exe must have been started with ECORE_EXE_PIPE_WRITE flag for this
 * function to work.
 *
 * @param obj The Ecore_Exe object.
 * @param data The data to send.
 * @param size The size of the data in bytes.
 * @return @c EINA_TRUE if the send was successful or queued, @c EINA_FALSE on error
 *         (e.g., stdin is not piped or is closed).
 * @ingroup Ecore_Exe_Group
 */
EAPI Eina_Bool
ecore_exe_send(Ecore_Exe  *obj,
               const void *data,
               int         size)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(EINA_FALSE);
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return EINA_FALSE;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(size == 0, EINA_TRUE);
   if (exe->close_stdin)
     {
        ERR("Ecore_Exe %p stdin is closed! Cannot send %d bytes from %p",
            exe, size, data);
        return EINA_FALSE;
     }
   return _impl_ecore_exe_send(obj, exe, data, size);
}

/**
 * @brief Closes the stdin pipe of the running process.
 *
 * After calling this, no more data can be sent to the process using ecore_exe_send().
 * This is typically used to signal EOF to the child process's stdin.
 *
 * @param obj The Ecore_Exe object.
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_close_stdin(Ecore_Exe *obj)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return;
   exe->close_stdin = 1;
}

/**
 * @brief Sets the limits for auto-buffering of stdout/stderr data.
 *
 * Ecore_Exe can automatically buffer data read from the child process's
 * stdout and stderr. These parameters control the size of those buffers.
 *
 * @param obj The Ecore_Exe object.
 * @param start_bytes The initial size of the byte buffer.
 * @param end_bytes The maximum size the byte buffer can grow to.
 * @param start_lines The initial number of lines in the line buffer.
 * @param end_lines The maximum number of lines the line buffer can grow to.
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_auto_limits_set(Ecore_Exe *obj,
                          int        start_bytes,
                          int        end_bytes,
                          int        start_lines,
                          int        end_lines)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return;
   _impl_ecore_exe_auto_limits_set(obj, exe, start_bytes, end_bytes,
                                   start_lines, end_lines);
}

/**
 * @brief Retrieves buffered data from the process's stdout or stderr.
 *
 * This function allows polling for data that has been buffered by Ecore_Exe.
 * The data is returned in an Ecore_Exe_Event_Data structure, which must be
 * freed using ecore_exe_event_data_free() when no longer needed.
 *
 * @param obj The Ecore_Exe object.
 * @param flags Specifies whether to get data from stdout (ECORE_EXE_PIPE_READ)
 *              or stderr (ECORE_EXE_PIPE_ERROR).
 * @return An Ecore_Exe_Event_Data structure containing the buffered data,
 *         or @c NULL if no data is available or on error.
 * @ingroup Ecore_Exe_Group
 */
EAPI Ecore_Exe_Event_Data *
ecore_exe_event_data_get(Ecore_Exe      *obj,
                         Ecore_Exe_Flags flags)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return NULL;
   return _impl_ecore_exe_event_data_get(obj, exe, flags);
}

/**
 * @brief Sets a string tag for an Ecore_Exe object.
 *
 * This tag can be used to identify or categorize Ecore_Exe instances.
 * The provided string is duplicated by the function.
 *
 * @param obj The Ecore_Exe object.
 * @param tag The string tag to set. If @c NULL, any existing tag is removed.
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_tag_set(Ecore_Exe  *obj,
                  const char *tag)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return;
   IF_FREE(exe->tag);
   if (tag) exe->tag = strdup(tag);
   else exe->tag = NULL;
}

/**
 * @brief Gets the string tag associated with an Ecore_Exe object.
 *
 * @param obj The Ecore_Exe object.
 * @return The string tag, or @c NULL if no tag is set or on error.
 *         The returned string is an internal pointer and should not be modified or freed.
 * @ingroup Ecore_Exe_Group
 */
EAPI const char *
ecore_exe_tag_get(const Ecore_Exe *obj)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return NULL;
   return exe->tag;
}

/**
 * @brief Frees an Ecore_Exe object.
 *
 * This function will terminate the running process if it's still active,
 * close any open pipes, and free all resources associated with the Ecore_Exe.
 * The user data associated with the Ecore_Exe (set by ecore_exe_run() or
 * ecore_exe_data_set()) is returned.
 *
 * @param obj The Ecore_Exe object to free.
 * @return The user data associated with the Ecore_Exe.
 * @ingroup Ecore_Exe_Group
 */
EAPI void *
ecore_exe_free(Ecore_Exe *obj)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return NULL;
   void *data = exe->data;
   efl_del(obj);
   return data;
}

EOLIAN static void
_ecore_exe_efl_object_destructor(Eo *obj, Ecore_Exe_Data *exe)
{
   efl_destructor(efl_super(obj, ECORE_EXE_CLASS));
   _impl_ecore_exe_efl_object_destructor(obj, exe);
}

/**
 * @brief Frees an Ecore_Exe_Event_Data structure.
 *
 * This function should be called on Ecore_Exe_Event_Data structures received
 * from ECORE_EXE_EVENT_DATA events or ecore_exe_event_data_get().
 *
 * @param e The Ecore_Exe_Event_Data structure to free.
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_event_data_free(Ecore_Exe_Event_Data *e)
{
   if (!e) return;
   IF_FREE(e->lines);
   IF_FREE(e->data);
   free(e);
}

/**
 * @brief Gets the process ID (PID) of the running executable.
 *
 * @param obj The Ecore_Exe object.
 * @return The PID of the child process, or -1 on error or if the process
 *         has not been started or has already exited.
 * @ingroup Ecore_Exe_Group
 */
EAPI pid_t
ecore_exe_pid_get(const Ecore_Exe *obj)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(0);
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return -1;
   return exe->pid;
}

/**
 * @brief Gets the command string that was used to start the Ecore_Exe.
 *
 * @param obj The Ecore_Exe object.
 * @return The command string. This string is an internal pointer and should not
 *         be modified or freed. It remains valid as long as @p obj is valid.
 * @ingroup Ecore_Exe_Group
 */
EAPI const char *
ecore_exe_cmd_get(const Ecore_Exe *obj)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   const char *ret = NULL;
   ecore_obj_exe_command_get(obj, &ret, NULL);
   return ret;
}

/**
 * @brief Gets the user data associated with an Ecore_Exe object.
 *
 * This is the data pointer provided when calling ecore_exe_run(),
 * ecore_exe_pipe_run(), or subsequently set by ecore_exe_data_set().
 *
 * @param obj The Ecore_Exe object.
 * @return The user data pointer, or @c NULL if no data is associated or on error.
 * @ingroup Ecore_Exe_Group
 */
EAPI void *
ecore_exe_data_get(const Ecore_Exe *obj)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return NULL;
   return exe->data;
}

/**
 * @brief Sets the user data associated with an Ecore_Exe object.
 *
 * This function allows changing the user data pointer associated with an
 * Ecore_Exe instance after it has been created.
 *
 * @param obj The Ecore_Exe object.
 * @param data The new user data pointer to associate.
 * @return The previously associated user data pointer.
 * @ingroup Ecore_Exe_Group
 */
EAPI void *
ecore_exe_data_set(Ecore_Exe *obj,
                   void      *data)
{
   void *ret;
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return NULL;
   ret = exe->data;
   exe->data = data;
   return ret;
}

/**
 * @brief Gets the flags used to start the Ecore_Exe.
 *
 * These are the flags originally passed to ecore_exe_pipe_run().
 *
 * @param obj The Ecore_Exe object.
 * @return The Ecore_Exe_Flags used for this instance.
 * @ingroup Ecore_Exe_Group
 */
EAPI Ecore_Exe_Flags
ecore_exe_flags_get(const Ecore_Exe *obj)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(0);
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return 0;
   return exe->flags;
}

/**
 * @brief Pauses a running Ecore_Exe process.
 *
 * This typically sends a SIGSTOP signal on POSIX systems.
 *
 * @param obj The Ecore_Exe object.
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_pause(Ecore_Exe *obj)
{
   efl_control_suspend_set(obj, EINA_TRUE);
}

/**
 * @brief Resumes a paused Ecore_Exe process.
 *
 * This typically sends a SIGCONT signal on POSIX systems.
 *
 * @param obj The Ecore_Exe object.
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_continue(Ecore_Exe *obj)
{
   efl_control_suspend_set(obj, EINA_FALSE);
}

EOLIAN static void
_ecore_exe_efl_control_suspend_set(Eo *obj EINA_UNUSED, Ecore_Exe_Data *exe, Eina_Bool suspend)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   if (suspend) _impl_ecore_exe_pause(obj, exe);
   else _impl_ecore_exe_continue(obj, exe);
}

/**
 * @brief Sends an interrupt signal (SIGINT) to the Ecore_Exe process.
 *
 * @param obj The Ecore_Exe object.
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_interrupt(Ecore_Exe *obj)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return;
   _impl_ecore_exe_interrupt(obj, exe);
}

/**
 * @brief Sends a quit signal (SIGQUIT) to the Ecore_Exe process.
 *
 * @param obj The Ecore_Exe object.
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_quit(Ecore_Exe *obj)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return;
   _impl_ecore_exe_quit(obj, exe);
}

/**
 * @brief Sends a terminate signal (SIGTERM) to the Ecore_Exe process.
 *
 * This is generally a polite request for the process to exit.
 *
 * @param obj The Ecore_Exe object.
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_terminate(Ecore_Exe *obj)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return;
   _impl_ecore_exe_terminate(obj, exe);
}

/**
 * @brief Sends a kill signal (SIGKILL) to the Ecore_Exe process.
 *
 * This is a forceful termination of the process.
 *
 * @param obj The Ecore_Exe object.
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_kill(Ecore_Exe *obj)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return;
   _impl_ecore_exe_kill(obj, exe);
}

/**
 * @brief Sends a specific signal to the Ecore_Exe process.
 *
 * @param obj The Ecore_Exe object.
 * @param num The signal number to send (e.g., SIGUSR1, SIGUSR2).
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_signal(Ecore_Exe *obj,
                 int        num)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return;
   _impl_ecore_exe_signal(obj, exe, num);
}

/**
 * @brief Sends a hangup signal (SIGHUP) to the Ecore_Exe process.
 *
 * @param obj The Ecore_Exe object.
 * @ingroup Ecore_Exe_Group
 */
EAPI void
ecore_exe_hup(Ecore_Exe *obj)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
   if (!efl_isa(obj, MY_CLASS)) return;
   _impl_ecore_exe_hup(obj, exe);
}

/**
 * @internal
 * @brief Initializes the Ecore_Exe subsystem.
 *
 * This function sets up the event types used by Ecore_Exe and performs
 * any platform-specific initialization (like creating a Job Object on Windows
 * to manage child processes).
 * It is called by ecore_init().
 */
void
_ecore_exe_init(void)
{
#ifdef _WIN32
   _ecore_exe_win32_job = CreateJobObject( NULL, NULL);
   if (_ecore_exe_win32_job)
     {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;

        memset (&jeli, 0, sizeof(jeli));
        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(_ecore_exe_win32_job, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)))
          {
             CloseHandle(_ecore_exe_win32_job);
             _ecore_exe_win32_job = NULL;
          }
     }
#endif
   ECORE_EXE_EVENT_ADD = ecore_event_type_new();
   ECORE_EXE_EVENT_DEL = ecore_event_type_new();
   ECORE_EXE_EVENT_DATA = ecore_event_type_new();
   ECORE_EXE_EVENT_ERROR = ecore_event_type_new();
}

/**
 * @internal
 * @brief Shuts down the Ecore_Exe subsystem.
 *
 * This function frees any running Ecore_Exe instances, flushes associated
 * event types, and performs platform-specific cleanup (like closing the
 * Windows Job Object).
 * It is called by ecore_shutdown().
 */
void
_ecore_exe_shutdown(void)
{
   Ecore_Exe *exe = NULL;
   Eina_List *l1, *l2;
   Efl_Loop_Data *loop = EFL_LOOP_DATA;

   EINA_LIST_FOREACH_SAFE(loop->exes, l1, l2, exe)
      ecore_exe_free(exe);

   ecore_event_type_flush(ECORE_EXE_EVENT_ADD,
                          ECORE_EXE_EVENT_DEL,
                          ECORE_EXE_EVENT_DATA,
                          ECORE_EXE_EVENT_ERROR);

#ifdef _WIN32
   if (_ecore_exe_win32_job)
     CloseHandle(_ecore_exe_win32_job);
#endif
}

/**
 * @internal
 * @brief Finds an Ecore_Exe instance by its process ID (PID).
 *
 * @param pid The process ID to search for.
 * @return The Ecore_Exe object if found, @c NULL otherwise.
 */
Ecore_Exe *
_ecore_exe_find(pid_t pid)
{
   Eina_List *itr;
   Ecore_Exe *obj;
   Efl_Loop_Data *loop = EFL_LOOP_DATA;

   EINA_LIST_FOREACH(loop->exes, itr, obj)
     {
        Ecore_Exe_Data *exe = efl_data_scope_get(obj, MY_CLASS);
        if (exe->pid == pid) return obj;
     }
   return NULL;
}

/**
 * @internal
 * @brief Allocates and initializes a new Ecore_Exe_Event_Del structure.
 *
 * This structure is used for ECORE_EXE_EVENT_DEL events.
 *
 * @return A pointer to the newly allocated Ecore_Exe_Event_Del structure.
 */
void *
_ecore_exe_event_del_new(void)
{
   Ecore_Exe_Event_Del *e = calloc(1, sizeof(Ecore_Exe_Event_Del));
   return e;
}

/**
 * @internal
 * @brief Frees an Ecore_Exe_Event_Del structure.
 *
 * If the event structure contains a reference to an Ecore_Exe object
 * (e->exe), that object is also freed. This is typically used as the
 * free function for ECORE_EXE_EVENT_DEL events.
 *
 * @param data User data (unused).
 * @param ev Pointer to the Ecore_Exe_Event_Del structure to free.
 */
void
_ecore_exe_event_del_free(void *data EINA_UNUSED,
                          void *ev)
{
   Ecore_Exe_Event_Del *e = ev;
   if (e->exe) ecore_exe_free(e->exe);
   free(e);
}

/**
 * @internal
 * @brief Frees an Ecore_Exe_Event_Data structure.
 *
 * This function is a wrapper around ecore_exe_event_data_free() and is
 * typically used as the free function for ECORE_EXE_EVENT_DATA and
 * ECORE_EXE_EVENT_ERROR events.
 *
 * @param data User data (unused).
 * @param ev Pointer to the Ecore_Exe_Event_Data structure to free.
 */
void
_ecore_exe_event_exe_data_free(void *data EINA_UNUSED,
                               void *ev)
{
   Ecore_Exe_Event_Data *e = ev;
   ecore_exe_event_data_free(e);
}

/**
 * @internal
 * @brief Allocates and initializes a new Ecore_Exe_Event_Add structure.
 *
 * This structure is used for ECORE_EXE_EVENT_ADD events.
 *
 * @return A pointer to the newly allocated Ecore_Exe_Event_Add structure.
 */
Ecore_Exe_Event_Add *
_ecore_exe_event_add_new(void)
{
   Ecore_Exe_Event_Add *e = calloc(1, sizeof(Ecore_Exe_Event_Add));
   return e;
}

/**
 * @internal
 * @brief Frees an Ecore_Exe_Event_Add structure.
 *
 * This is typically used as the free function for ECORE_EXE_EVENT_ADD events.
 *
 * @param data User data (unused).
 * @param ev Pointer to the Ecore_Exe_Event_Add structure to free.
 */
void
_ecore_exe_event_add_free(void *data EINA_UNUSED,
                          void *ev)
{
   Ecore_Exe_Event_Add *e = ev;
   free(e);
}

#include "ecore_exe_eo.c"
