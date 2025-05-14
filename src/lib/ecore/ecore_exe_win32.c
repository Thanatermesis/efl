#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <process.h>

#include <evil_private.h> /* evil_last_error_get */

#include "Ecore.h"
#include "ecore_private.h"

#include "ecore_exe_private.h"

/*
 * TESTS
 *
 * [X] add event
 * [X] data event
 * [X] error event
 * [X] data event buffered
 * [X] del event
 * [X] batch files (not recommended by MSDN)
 * [X] shebang
 * [X] exit code
 * [X] inherited env var
 */

/*
 * FIXME :
 *
 * [ ] child program with ecore main loop does not exit and nothing is sent
 * [X] ecore_exe_send fails (race condition ? same problem as above ?)
 */

#define ECORE_EXE_WIN32_TIMEOUT 3000

/** @internal
 * @brief Default priority for running processes.
 * This can be changed by ecore_exe_run_priority_set().
 */
static int run_pri = NORMAL_PRIORITY_CLASS;

/**
 * @internal
 * @brief Terminates the I/O polling thread associated with an Ecore_Exe object.
 *
 * This function cancels the thread and waits for a short period for it to exit.
 *
 * @param obj The Ecore_Exe object whose I/O thread is to be terminated.
 */
static void
_ecore_exe_threads_terminate(Ecore_Exe *obj)
{
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, ECORE_EXE_CLASS);

   if (!exe) return;
   if (!exe->th) return;
   ecore_thread_cancel(exe->th);
   ecore_thread_wait(exe->th, 0.3);
   exe->th = NULL;
}

/**
 * @internal
 * @brief Callback function invoked when a child process handle is signaled (e.g., process termination).
 *
 * This function retrieves the exit code of the process and adds an ECORE_EXE_EVENT_DEL
 * event to the Ecore event queue.
 *
 * @param data Pointer to the Ecore_Exe object.
 * @param wh Unused Ecore_Win32_Handler.
 * @return EINA_FALSE to indicate the handler should not be called again (it will be deleted).
 */
static Eina_Bool
_ecore_exe_close_cb(void *data,
                    Ecore_Win32_Handler *wh EINA_UNUSED)
{
   Ecore_Exe_Event_Del *e;
   Ecore_Exe *obj = data;
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, ECORE_EXE_CLASS);
   DWORD exit_code = 0;

   if (!exe) return 0;

   e = calloc(1, sizeof(Ecore_Exe_Event_Del));
   if (!e) return 0;

   /* FIXME : manage the STILL_ACTIVE returned error */
   if (!GetExitCodeProcess(exe->process, &exit_code))
     DBG("%s", evil_last_error_get());

   e->exit_code = exit_code;
   e->exited = 1;
   e->pid = exe->pid;
   e->exe = obj;
   exe->h_close = NULL; // It's going to get deleted in the next callback.

   ecore_event_add(ECORE_EXE_EVENT_DEL, e, _ecore_exe_event_del_free, NULL);

   DBG("Exiting process %s with exit code %d\n", exe->cmd, e->exit_code);
   return 0;
}

/**
 * @internal
 * @brief Structure to hold data for the I/O polling thread.
 */
typedef struct
{
   Ecore_Exe  *obj;        /**< The Ecore_Exe object associated with this thread. */
   HANDLE      read_pipe;  /**< Handle to the child's stdout pipe. */
   HANDLE      error_pipe; /**< Handle to the child's stderr pipe. */
   Eina_Bool   read : 1;   /**< Flag indicating if stdout should be polled. */
   Eina_Bool   error : 1;  /**< Flag indicating if stderr should be polled. */
} Threaddata;

/**
 * @internal
 * @brief Structure to hold data passed from the I/O polling thread to the main thread.
 */
typedef struct
{
   Ecore_Exe     *obj;        /**< The Ecore_Exe object. */
   unsigned char *buf;        /**< Buffer containing data read from the pipe. */
   int            buf_size; /**< Size of the data in the buffer. */
   Eina_Bool      read : 1;   /**< Flag indicating if data is from stdout. */
   Eina_Bool      error : 1;  /**< Flag indicating if data is from stderr. */
} Threadreply;

/**
 * @internal
 * @brief I/O polling thread function for reading data from child process pipes.
 *
 * This thread continuously polls the stdout and stderr pipes of the child process.
 * When data is available, it reads the data and sends it to the main thread
 * via ecore_thread_feedback().
 *
 * @param data Pointer to a Threaddata structure.
 * @param th The Ecore_Thread this function is running in.
 */
static void
_ecore_exe_win32_io_poll_thread(void *data, Ecore_Thread *th)
{
   Threaddata *tdat = data;
   Threadreply *trep;
   Eina_Bool data_read, data_error;
   char buf[4096];
   DWORD size, current_size;
   BOOL res;

   while (EINA_TRUE)
     {
        data_read = EINA_FALSE;
        data_error = EINA_FALSE;

        if (tdat->read)
          {
             res = PeekNamedPipe(tdat->read_pipe, buf, sizeof(buf),
                                 &size, &current_size, NULL);
             if (res && (size != 0))
               {
                  trep = calloc(1, sizeof(Threadreply));
                  if (trep)
                    {
                       trep->obj = tdat->obj;
                       trep->buf = malloc(current_size);
                       if (trep->buf)
                         {
                            res = ReadFile(tdat->read_pipe, trep->buf,
                                           current_size, &size, NULL);
                            if (!res || (size == 0))
                              {
                                 free(trep->buf);
                                 free(trep);
                              }
                            trep->buf_size = size;
                            trep->read = EINA_TRUE;
                            ecore_thread_feedback(th, trep);
                            data_read = EINA_TRUE;
                         }
                    }
               }
          }
        if (tdat->error)
          {
             res = PeekNamedPipe(tdat->error_pipe, buf, sizeof(buf),
                                 &size, &current_size, NULL);
             if (res && (size != 0))
               {
                  trep = calloc(1, sizeof(Threadreply));
                  if (trep)
                    {
                       trep->obj = tdat->obj;
                       trep->buf = malloc(current_size);
                       if (trep->buf)
                         {
                            res = ReadFile(tdat->error_pipe, trep->buf,
                                           current_size, &size, NULL);
                            if (!res || (size == 0))
                              {
                                 free(trep->buf);
                                 free(trep);
                              }
                            trep->buf_size = size;
                            trep->error = EINA_TRUE;
                            ecore_thread_feedback(th, trep);
                            data_error = EINA_TRUE;
                         }
                    }
               }
          }
        if (ecore_thread_check(th)) break;
        if (!(data_read || data_error)) Sleep(100);
        else if (ecore_thread_check(th)) break;
     }
   free(tdat);
}

/**
 * @internal
 * @brief Notification callback executed in the main thread when data is received from the I/O polling thread.
 *
 * This function processes the data received from the child process's pipes.
 * It appends the new data to existing buffers and triggers ECORE_EXE_EVENT_DATA
 * or ECORE_EXE_EVENT_ERROR events.
 *
 * @param data Unused user data.
 * @param th Unused Ecore_Thread.
 * @param msg Pointer to a Threadreply structure containing the received data.
 */
static void
_ecore_exe_win32_io_poll_notify(void *data EINA_UNUSED,
                                Ecore_Thread *th EINA_UNUSED, void *msg)
{
   Threadreply *trep = msg;
   Ecore_Exe *obj = trep->obj;
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, ECORE_EXE_CLASS);
   unsigned char *b;

   if (exe)
     {
        Ecore_Exe_Event_Data *event_data;

        if (trep->read)
          {
             if (!exe->pipe_read.data_buf)
               {
                  exe->pipe_read.data_buf = trep->buf;
                  exe->pipe_read.data_size = trep->buf_size;
                  trep->buf = NULL;
               }
             else
               {
                  b = realloc(exe->pipe_read.data_buf,
                              exe->pipe_read.data_size + trep->buf_size);
                  if (b)
                    {
                       memcpy(b + exe->pipe_read.data_size,
                              trep->buf, trep->buf_size);
                       exe->pipe_read.data_buf = b;
                       exe->pipe_read.data_size += trep->buf_size;
                    }
                  else ERR("Out of memory in reading exe pipe data");
               }
             event_data = ecore_exe_event_data_get(obj, ECORE_EXE_PIPE_READ);
             if (event_data)
               {
                  ecore_event_add(ECORE_EXE_EVENT_DATA, event_data,
                                  _ecore_exe_event_exe_data_free, NULL);
                  efl_event_callback_call(obj, ECORE_EXE_EVENT_DATA_GET,
                                          event_data);
               }
          }
        else if (trep->error)
          {
             if (!exe->pipe_error.data_buf)
               {
                  exe->pipe_error.data_buf = trep->buf;
                  exe->pipe_error.data_size = trep->buf_size;
                  trep->buf = NULL;
               }
             else
               {
                  b = realloc(exe->pipe_error.data_buf,
                              exe->pipe_error.data_size + trep->buf_size);
                  if (b)
                    {
                       memcpy(b + exe->pipe_error.data_size,
                              trep->buf, trep->buf_size);
                       exe->pipe_error.data_buf = b;
                       exe->pipe_error.data_size += trep->buf_size;
                    }
                  else ERR("Out of memory in reading exe pipe data");
               }
             event_data = ecore_exe_event_data_get(obj, ECORE_EXE_PIPE_ERROR);
             if (event_data)
               {
                  ecore_event_add(ECORE_EXE_EVENT_ERROR, event_data,
                                  _ecore_exe_event_exe_data_free, NULL);
                  efl_event_callback_call(obj, ECORE_EXE_EVENT_DATA_ERROR,
                                          event_data);
               }
          }
     }
   free(trep->buf);
   free(trep);
}

/**
 * @internal
 * @brief Thread procedure used by CreateRemoteThread to send Ctrl+C/Ctrl+Break events.
 *
 * This function is injected into the target process to simulate console control events.
 *
 * @param data Unused.
 * @return Always 1.
 */
static DWORD WINAPI
_ecore_exe_thread_procedure(LPVOID data EINA_UNUSED)
{
   GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
   GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0);
   return 1;
}

/**
 * @internal
 * @brief Thread procedure used by CreateRemoteThread to call ExitProcess in the target process.
 *
 * This function is injected into the target process to force it to exit with a specific code.
 *
 * @param data Pointer to a UINT containing the exit code.
 * @return Always 1 (though ExitProcess should prevent this from being reached).
 */
static DWORD __stdcall
_ecore_exe_exit_process(void *data)
{
   UINT *code;
   code = (UINT *)data;
   ExitProcess(*code);
   return 1;
}

/**
 * @internal
 * @brief Callback procedure for EnumWindows, used to send signals/messages to a child process's windows.
 *
 * This function iterates through top-level windows. If a window belongs to the
 * target child process (identified by thread ID), it attempts to terminate or
 * signal the process using various methods (Ctrl+C, WM_CLOSE, WM_QUIT, TerminateProcess).
 *
 * @param window Handle to a top-level window.
 * @param data LPARAM, cast to Ecore_Exe* representing the target child process.
 * @return EINA_TRUE to continue enumeration, EINA_FALSE to stop.
 */
static BOOL CALLBACK
_ecore_exe_enum_windows_procedure(HWND window,
                                  LPARAM data)
{
   Ecore_Exe *obj = (Ecore_Exe *) data;
   Ecore_Exe_Data *exe = efl_data_scope_get(obj, ECORE_EXE_CLASS);
   DWORD thread_id;
   UINT code = 0;

   if (!exe) return EINA_FALSE;
   thread_id = GetWindowThreadProcessId(window, NULL);

   if (thread_id == exe->thread_id)
     {
        /* Ctrl-C or Ctrl-Break */
        if (CreateRemoteThread(exe->process, NULL, 0,
                               (LPTHREAD_START_ROUTINE)_ecore_exe_thread_procedure, NULL,
                                0, NULL))
          return EINA_FALSE;

        if ((exe->sig == ECORE_EXE_WIN32_SIGINT) ||
            (exe->sig == ECORE_EXE_WIN32_SIGQUIT))
          return EINA_FALSE;

        /* WM_CLOSE message */
        PostMessage(window, WM_CLOSE, 0, 0);
        if (WaitForSingleObject(exe->process, ECORE_EXE_WIN32_TIMEOUT) == WAIT_OBJECT_0)
          return EINA_FALSE;

        /* WM_QUIT message */
        PostMessage(window, WM_QUIT, 0, 0);
        if (WaitForSingleObject(exe->process, ECORE_EXE_WIN32_TIMEOUT) == WAIT_OBJECT_0)
          return EINA_FALSE;

        /* Exit process */
        if (CreateRemoteThread(exe->process, NULL, 0,
                               (LPTHREAD_START_ROUTINE)_ecore_exe_exit_process,
                               &code,
                               0, NULL))
          return EINA_FALSE;

        if (exe->sig == ECORE_EXE_WIN32_SIGTERM)
          return EINA_FALSE;

        TerminateProcess(exe->process, 0);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Sets the default priority for new processes created by ecore_exe_run.
 *
 * @param pri The desired priority class, one of ECORE_EXE_WIN32_PRIORITY_*.
 * @see _impl_ecore_exe_run_priority_get()
 */
void
_impl_ecore_exe_run_priority_set(int pri)
{
   switch (pri)
     {
      case ECORE_EXE_WIN32_PRIORITY_IDLE:
        run_pri = IDLE_PRIORITY_CLASS;
        break;

      case ECORE_EXE_WIN32_PRIORITY_BELOW_NORMAL:
        run_pri = BELOW_NORMAL_PRIORITY_CLASS;
        break;

      case ECORE_EXE_WIN32_PRIORITY_NORMAL:
        run_pri = NORMAL_PRIORITY_CLASS;
        break;

      case ECORE_EXE_WIN32_PRIORITY_ABOVE_NORMAL:
        run_pri = ABOVE_NORMAL_PRIORITY_CLASS;
        break;

      case ECORE_EXE_WIN32_PRIORITY_HIGH:
        run_pri = HIGH_PRIORITY_CLASS;
        break;

      case ECORE_EXE_WIN32_PRIORITY_REALTIME:
        run_pri = REALTIME_PRIORITY_CLASS;
        break;

      default:
        break;
     }
}

/**
 * @internal
 * @brief Gets the current default priority for new processes.
 *
 * @return The current priority class, one of ECORE_EXE_WIN32_PRIORITY_*.
 * @see _impl_ecore_exe_run_priority_set()
 */
int
_impl_ecore_exe_run_priority_get(void)
{
   switch (run_pri)
     {
      case IDLE_PRIORITY_CLASS:
        return ECORE_EXE_WIN32_PRIORITY_IDLE;

      case BELOW_NORMAL_PRIORITY_CLASS:
        return ECORE_EXE_WIN32_PRIORITY_BELOW_NORMAL;

      case NORMAL_PRIORITY_CLASS:
        return ECORE_EXE_WIN32_PRIORITY_NORMAL;

      case ABOVE_NORMAL_PRIORITY_CLASS:
        return ECORE_EXE_WIN32_PRIORITY_ABOVE_NORMAL;

      case HIGH_PRIORITY_CLASS:
        return ECORE_EXE_WIN32_PRIORITY_HIGH;

      case REALTIME_PRIORITY_CLASS:
        return ECORE_EXE_WIN32_PRIORITY_REALTIME;

      /* default should not be reached */
      default:
        return ECORE_EXE_WIN32_PRIORITY_NORMAL;
     }
}

/**
 * @internal
 * @brief Extracts the child executable name from a command string.
 *
 * This function parses the command string to find the first token, which is
 * assumed to be the executable. It handles quoted paths.
 *
 * @param cmd The full command string.
 * @return A newly allocated string containing the child executable name, or NULL on failure.
 *         The caller is responsible for freeing the returned string.
 * @note Example: "C:\\Path\\To\\program.exe" -arg1 -> "C:\\Path\\To\\program.exe"
 * @note Example: program.exe -arg1 -> "program.exe"
 */
static char *
_ecore_exe_win32_child_get(const char *cmd)
{
   char *child;
   const char *iter = cmd;
   const char *iter2;

   while (*iter == ' ')
     iter++;

   if (*iter == '\"')
     {
        iter++;
        iter2 = iter;
        while (*iter2)
          {
             if (*iter2 == '\"' && *(iter2 - 1) != '\\')
               break;

             iter2++;
          }

        if (*iter2 == 0) /* unbalanced " */
          return NULL;
     }
   else
     {
        iter2 = iter + 1;

        while (*iter2 && *iter2 != ' ')
          iter2++;
     }

   child = calloc(iter2 - iter + 1, sizeof(char));
   if (!child)
     return NULL;

   memcpy(child, iter, iter2 - iter);

   return child;
}

/**
 * @internal
 * @brief Reads a file to find a shebang ("#!") line and extracts the interpreter.
 *
 * This function opens the specified file, reads the first line, and if it's a
 * shebang line, parses the interpreter. It handles optional "/usr/bin/env"
 * and quotes the extracted interpreter.
 *
 * @param child Path to the script file.
 * @return A newly allocated string containing the quoted interpreter (e.g., "\"python.exe\""),
 *         or NULL if no shebang is found, the file cannot be opened, or on error.
 *         The caller is responsible for freeing the returned string.
 */
static char *
_ecore_exe_win32_shebang_interpreter_get(const char *child)
{
   Eina_File *file;
   Eina_Iterator *it;
   Eina_File_Line *line;
   const char *iter;
   char *interpreter = NULL;
   size_t sz;

   file = eina_file_open(child, EINA_FALSE);
   if (!file)
     return NULL;

   it = eina_file_map_lines(file);
   if (!it)
     goto close_file;

   if (!eina_iterator_next(it, (void **)(void *)&line))
     goto free_iterator;

   /*
    * shebang spec:
    * #!
    * optional spaces
    * optional /usr/bin/env
    * at least one space
    * interpreter
    */

   if (line->length < 2)
     goto free_iterator;

   iter = line->start;
   if ((iter[0] != '#') && (iter[1] != '!'))
     goto free_iterator;

   iter += 2;

   /* skip possible spaces after #! */
   while (iter < line->end)
     {
        if (*iter != ' ')
          break;
        iter++;
     }

   /*
    * Check if "/usr/bin/env" is used.
    * If so, skip it (useless on Windows as the command is searched in PATH).
    * Note that length of "/usr/bin/env" is 12.
    */
   if ((line->end - iter >= 12) &&
       (strncmp(iter, "/usr/bin/env", 12) == 0))
     iter += 12;

   /* skip possible spaces after #! */
   while (iter < line->end)
     {
        if (*iter != ' ')
          break;
        iter++;
     }

   if (*iter == '/')
     {
        const char *i;

        /* get the last '/' (parse from last char) */
        i = line->end;
        while (i != iter)
          {
             if (*i == '/')
               break;
             i--;
          }

        if (i == iter)
          {
             goto free_iterator;
          }

        i++;
        if (i > line->end)
          goto free_iterator;

        iter = i;
     }

   sz = line->end - iter;
   /* quote it --> + 2 for the 2 " */
   interpreter = malloc(sz + 3);
   if (!interpreter)
     goto free_iterator;

   interpreter[0] = '\"';
   memcpy(interpreter + 1, iter, sz);
   interpreter[sz + 1] = '\"';
   interpreter[sz + 2] = '\0';

 free_iterator:
   eina_iterator_free(it);
 close_file:
   eina_file_close(file);

   return interpreter;
}

/**
 * @internal
 * @brief Constructs a command string to execute a batch file using "cmd.exe /C".
 *
 * @param exe_cmd The original command string (path to the .bat file and its arguments).
 * @return A newly allocated string formatted as "\"cmd.exe\" \"/C\" original_command",
 *         or NULL on allocation failure. The caller must free the returned string.
 * @note Example: mybatch.bat arg1 -> "\"cmd.exe\" \"/C\" mybatch.bat arg1"
 */
static char *
_ecore_exe_win32_batch_cmd_get(const char *exe_cmd)
{
   char *cmd = NULL;
   size_t len;

   if (!exe_cmd || !*exe_cmd)
     return NULL;

   len = strlen(exe_cmd);
   cmd = (char *)calloc(len + 16, sizeof(char));
   if (cmd)
     {
        char *iter = cmd;

        memcpy(iter, "\"cmd.exe\" \"/C\" ", 15);
        iter += 15;
        memcpy (iter, exe_cmd, len + 1);

        DBG("Batch cmd: '%s'", cmd);
     }

   return cmd;
}

/**
 * @internal
 * @brief Constructs a command string to execute a script using its shebang interpreter.
 *
 * This function gets the interpreter from the shebang line of the `child` script
 * and prepends it to the `exe_cmd`.
 *
 * @param exe_cmd The original command string (script path and its arguments).
 * @param child Path to the script file (used to read the shebang).
 * @return A newly allocated string formatted as "interpreter original_command",
 *         or NULL if the interpreter cannot be determined or on allocation failure.
 *         The caller must free the returned string.
 * @note Example: if myscript.sh has "#!/usr/bin/python.exe", and exe_cmd is "myscript.sh -v",
 *       this returns "\"python.exe\" myscript.sh -v".
 */
static char *
_ecore_exe_win32_shebang_cmd_get(const char *exe_cmd, const char *child)
{
   char *cmd = NULL;
   char *interpreter;
   char *iter;
   size_t len;
   size_t len2;

   if (!exe_cmd || !*exe_cmd || !child || !*child)
     return NULL;

   interpreter = _ecore_exe_win32_shebang_interpreter_get(child);
   if (!interpreter)
     return NULL;

   len = strlen(exe_cmd);
   len2 = strlen(interpreter);
   cmd = (char *)calloc(len + len2 + 2, sizeof(char));
   if (!cmd)
     {
        free(interpreter);
        return NULL;
     }

   iter = cmd;
   memcpy(iter, interpreter, len2);
   free(interpreter);
   iter += len2;
   *iter = ' ';
   iter++;
   memcpy (iter, exe_cmd, len + 1);

   DBG("Shebang cmd: '%s'", cmd);

   return cmd;
}

/**
 * @internal
 * @brief Finalizes the Ecore_Exe object, creating and starting the child process.
 *
 * This function is called when the Ecore_Exe object is finalized. It sets up
 * pipes for stdin, stdout, and stderr as specified by the flags, then creates
 * the child process using CreateProcess. It handles .bat files and shebangs
 * by modifying the command line appropriately. It also sets up a handler to
 * detect process termination.
 *
 * @param obj The Ecore_Exe object being finalized.
 * @param exe The private data associated with the Ecore_Exe object.
 * @return The finalized Ecore_Exe object (obj) on success, or NULL on failure.
 */
Eo *
_impl_ecore_exe_efl_object_finalize(Eo *obj, Ecore_Exe_Data *exe)
{
   SECURITY_ATTRIBUTES sa;
   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   char *cmd = NULL;
   char *child = NULL;
   HANDLE child_pipe_read = NULL;
   HANDLE child_pipe_error = NULL;
   Ecore_Exe_Event_Add *e;
   Ecore_Exe_Flags flags;
   BOOL ret;

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);

   flags = exe->flags;
   DBG("Creating process %s with flags %d", exe->cmd, flags);
   if (!exe->cmd) goto error;

   if ((flags & ECORE_EXE_PIPE_AUTO) && (!(flags & ECORE_EXE_PIPE_ERROR)) &&
       (!(flags & ECORE_EXE_PIPE_READ)))
     /* We need something to auto pipe. */
     flags |= ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_ERROR;

   /* stdout, stderr and stdin pipes */
   sa.nLength = sizeof(SECURITY_ATTRIBUTES);
   sa.bInheritHandle = EINA_TRUE;
   sa.lpSecurityDescriptor = NULL;

   if ((exe->flags & ECORE_EXE_PIPE_READ) ||
       (exe->flags & ECORE_EXE_PIPE_ERROR))
     {
        Threaddata *tdat;

        tdat = calloc(1, sizeof(Threaddata));
        if (tdat)
          {
             tdat->obj = obj;
             /* stdout pipe */
             if (exe->flags & ECORE_EXE_PIPE_READ)
               {
                  if (!CreatePipe(&exe->pipe_read.child_pipe,
                                  &child_pipe_read, &sa, 0))
                    goto error;
                  if (!SetHandleInformation(exe->pipe_read.child_pipe,
                                            HANDLE_FLAG_INHERIT, 0))
                    goto error;
                  tdat->read = EINA_TRUE;
                  tdat->read_pipe = exe->pipe_read.child_pipe;
               }
             /* stderr pipe */
             if (exe->flags & ECORE_EXE_PIPE_ERROR)
               {
                  if (!CreatePipe(&exe->pipe_error.child_pipe,
                                  &child_pipe_error, &sa, 0))
                    goto error;
                  if (!SetHandleInformation(exe->pipe_error.child_pipe,
                                            HANDLE_FLAG_INHERIT, 0))
                    goto error;
                  tdat->error = EINA_TRUE;
                  tdat->error_pipe = exe->pipe_error.child_pipe;
               }
             exe->th = ecore_thread_feedback_run
               (_ecore_exe_win32_io_poll_thread,
                _ecore_exe_win32_io_poll_notify,
                NULL, NULL, tdat, EINA_TRUE);
             if (!exe->th)
               {
                  free(tdat);
                  goto error;
               }
          }
     }

   /* stdin pipe */
   if (exe->flags & ECORE_EXE_PIPE_WRITE)
     {
        if (!CreatePipe(&exe->pipe_write.child_pipe,
                        &exe->pipe_write.child_pipe_x, &sa, 0))
          goto error;
        if (!SetHandleInformation(exe->pipe_write.child_pipe_x,
                                  HANDLE_FLAG_INHERIT, 0))
          goto error;
     }

   /* create child process */

   ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

   ZeroMemory(&si, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   si.hStdOutput = child_pipe_read;
   si.hStdInput = exe->pipe_write.child_pipe;
   si.hStdError = child_pipe_error;
   si.dwFlags |= STARTF_USESTDHANDLES;

   /*
    * Creation of command line.
    * Don't use an Eina_Strbuf as CreateProcess() can modify the string
    * and eina_strbuf_string_get() returns a const string
    */

   /* Try first with cmd if the extension is .bat */
   child = _ecore_exe_win32_child_get(exe->cmd);
   if (eina_str_has_extension(child, ".bat"))
     {
        cmd = _ecore_exe_win32_batch_cmd_get(exe->cmd);
        if (!cmd) goto error;

        ret = CreateProcess(NULL, cmd,
                            NULL, NULL, EINA_TRUE,
                            run_pri | CREATE_SUSPENDED, NULL, NULL,
                            &si, &pi);
     }
   else if ((cmd = _ecore_exe_win32_shebang_cmd_get(exe->cmd, child)))
     {
        ret = CreateProcess(NULL, cmd,
                            NULL, NULL, EINA_TRUE,
                            run_pri | CREATE_SUSPENDED, NULL, NULL,
                            &si, &pi);
     }
   else
     {
        size_t len = strlen(exe->cmd);

        cmd = (char *)calloc(len + 1, sizeof(char));
        if (!cmd) goto error;

        memcpy(cmd, exe->cmd, len + 1);
        DBG("CreateProcess: child: '%s'", cmd);
        ret = CreateProcess(NULL, cmd,
                            NULL, NULL, EINA_TRUE,
                            run_pri | CREATE_SUSPENDED, NULL, NULL,
                            &si, &pi);
     }

   if (!ret)
     {
        WRN("Failed to create process '%s': %s",
            cmd, evil_last_error_get());
        goto error;
     }

   if ((flags & ECORE_EXE_TERM_WITH_PARENT) && _ecore_exe_win32_job)
     {
        if (!AssignProcessToJobObject(_ecore_exe_win32_job, pi.hProcess))
          WRN("AssignProcessToJobObject failed (job: %p, process: %p",
              _ecore_exe_win32_job, pi.hProcess);
     }

   /*
    * Close pipe handles (do not continue to modify the parent).
    * We need to make sure that no handles to the write end of the
    * output and error pipes are maintained in this process or else
    * the pipe will not close when the child process exits and the
    * ReadFile will hang.
    */
   IF_FN_DEL(CloseHandle, child_pipe_read);
   IF_FN_DEL(CloseHandle, child_pipe_error);

   /* be sure that the child process is running */
   /* FIXME: This does not work if the child is an EFL-based app */
   /* if (WaitForInputIdle(pi.hProcess, INFINITE) == WAIT_FAILED) */
   /*   goto error; */
   exe->process = pi.hProcess;
   exe->process_thread = pi.hThread;
   exe->pid = pi.dwProcessId;
   exe->thread_id = pi.dwThreadId;

   exe->h_close = ecore_main_win32_handler_add(exe->process,
                                               _ecore_exe_close_cb, obj);
   if (!exe->h_close) goto error;

   if (ResumeThread(exe->process_thread) == ((DWORD)-1))
     {
        ERR("Could not resume process");
        goto error;
     }

   exe->loop = efl_provider_find(obj, EFL_LOOP_CLASS);
   Efl_Loop_Data *loop = efl_data_scope_get(exe->loop, EFL_LOOP_CLASS);
   if (loop) loop->exes = eina_list_append(loop->exes, obj);

   e = calloc(1, sizeof(Ecore_Exe_Event_Add));
   if (!e) goto error;
   e->exe = obj;
   ecore_event_add(ECORE_EXE_EVENT_ADD, e, _ecore_exe_event_add_free, NULL);
   return obj;

error:
   free(child);
   free(cmd);
   return NULL;
}

/**
 * @internal
 * @brief Sends data to the stdin of the child process.
 *
 * @param obj The Ecore_Exe object.
 * @param exe The private data of the Ecore_Exe object.
 * @param data Pointer to the data to send.
 * @param size The size of the data in bytes.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., pipe closed or write error).
 */
Eina_Bool
_impl_ecore_exe_send(Ecore_Exe  *obj,
                     Ecore_Exe_Data *exe,
                     const void *data,
                     int         size)
{
   DWORD num_exe;
   BOOL res;

   res = WriteFile(exe->pipe_write.child_pipe_x, data, size, &num_exe, NULL);
   if (!res || num_exe == 0)
     {
        ERR("Ecore_Exe %p stdin is closed! Cannot send %d bytes from %p",
            obj, size, data);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Retrieves data from the internal buffers for stdout or stderr and prepares an event.
 *
 * This function is called to construct an Ecore_Exe_Event_Data structure when
 * data has been read from the child process's stdout or stderr pipes.
 * It handles line buffering if enabled for the respective pipe.
 *
 * @param obj The Ecore_Exe object.
 * @param exe The private data of the Ecore_Exe object.
 * @param flags Indicates whether to get data for ECORE_EXE_PIPE_READ or ECORE_EXE_PIPE_ERROR.
 * @return A newly allocated Ecore_Exe_Event_Data structure, or NULL if no complete lines
 *         are available (for line-buffered mode) or on allocation failure.
 *         The caller is responsible for the event data if not NULL.
 *         The `data` field within the returned struct points to the buffered data.
 *         If line-buffered, `lines` array is populated:
 *         Example `e->lines` structure:
 *         `e->lines[0] = { .line = "First line data", .size = 15 }`
 *         `e->lines[1] = { .line = "Second line data", .size = 16 }`
 *         `...`
 *         `e->lines[n] = { .line = NULL, .size = 0 }` (terminator)
 */
Ecore_Exe_Event_Data *
_impl_ecore_exe_event_data_get(Ecore_Exe      *obj,
                               Ecore_Exe_Data *exe,
                               Ecore_Exe_Flags flags)
{
   Ecore_Exe_Event_Data *e = NULL;
   unsigned char *inbuf;
   DWORD inbuf_num;
   Eina_Bool is_buffered = EINA_FALSE;

   /* Sort out what sort of event we are. */
   if (flags & ECORE_EXE_PIPE_READ)
     {
        flags = ECORE_EXE_PIPE_READ;
        if (exe->flags & ECORE_EXE_PIPE_READ_LINE_BUFFERED)
          is_buffered = EINA_TRUE;
     }
   else
     {
        flags = ECORE_EXE_PIPE_ERROR;
        if (exe->flags & ECORE_EXE_PIPE_ERROR_LINE_BUFFERED)
          is_buffered = EINA_TRUE;
     }

   /* Get the data. */
   if (flags & ECORE_EXE_PIPE_READ)
     {
        inbuf = exe->pipe_read.data_buf;
        inbuf_num = exe->pipe_read.data_size;
        exe->pipe_read.data_buf = NULL;
        exe->pipe_read.data_size = 0;
     }
   else
     {
        inbuf = exe->pipe_error.data_buf;
        inbuf_num = exe->pipe_error.data_size;
        exe->pipe_error.data_buf = NULL;
        exe->pipe_error.data_size = 0;
     }

   e = calloc(1, sizeof(Ecore_Exe_Event_Data));
   if (e)
     {
        e->exe = obj;
        e->data = inbuf;
        e->size = inbuf_num;

        if (is_buffered) /* Deal with line buffering. */
          {
             char *c;
             DWORD i;
             DWORD max = 0;
             DWORD count = 0;
             DWORD last = 0;

             c = (char *)inbuf;
             for (i = 0; i < inbuf_num; i++)
               {
                  if (inbuf[i] == '\n')
                    {
                       int end;

                       if (count >= max)
                         {
                            Ecore_Exe_Event_Data_Line *lines;

                            max += 10;
                            lines = realloc (e->lines,
                                             sizeof(Ecore_Exe_Event_Data_Line) * (max + 1));
                            if (lines) e->lines = lines;
                            else
                              {
                                 ERR("Out of memory in allocating exe lines");
                                 break;
                              }
                         }

                       if ((i >= 1) && (inbuf[i - 1] == '\r')) end = i - 1;
                       else end = i;
                       inbuf[end] = '\0';
                       e->lines[count].line = c;
                       e->lines[count].size = end - last;
                       last = i + 1;
                       c = (char *)&inbuf[last];
                       count++;
                    }
               }
             if (i > last) /* Partial line left over, save it for next time. */
               {
                  if (count != 0) e->size = last;
                  if (flags & ECORE_EXE_PIPE_READ)
                    {
                       exe->pipe_read.data_size = i - last;
                       exe->pipe_read.data_buf = malloc(exe->pipe_read.data_size);
                       if (exe->pipe_read.data_buf)
                         memcpy(exe->pipe_read.data_buf, c, exe->pipe_read.data_size);
                       else
                         {
                            exe->pipe_read.data_size = 0;
                            ERR("Out of memory in allocating exe pipe data");
                         }
                    }
                  else
                    {
                       exe->pipe_error.data_size = i - last;
                       exe->pipe_error.data_buf = malloc(exe->pipe_error.data_size);
                       if (exe->pipe_error.data_buf)
                         memcpy(exe->pipe_error.data_buf, c, exe->pipe_error.data_size);
                       else
                         {
                            exe->pipe_error.data_size = 0;
                            ERR("Out of memory in allocating exe pipe data");
                         }
                    }
               }
             if (count == 0) /* No lines to send, cancel the event. */
               {
                  _ecore_exe_event_exe_data_free(NULL, e);
                  e = NULL;
               }
             else /* NULL terminate the array, so that people know where the end is. */
               {
                  e->lines[count].line = NULL;
                  e->lines[count].size = 0;
               }
          }
     }
   return e;
}

/**
 * @internal
 * @brief Destructor for the Ecore_Exe object.
 *
 * Cleans up resources associated with the Ecore_Exe, including terminating
 * I/O threads, closing handles (process, thread, pipes), and freeing allocated memory.
 *
 * @param obj The Ecore_Exe object being destructed.
 * @param exe The private data of the Ecore_Exe object.
 */
void
_impl_ecore_exe_efl_object_destructor(Eo *obj, Ecore_Exe_Data *exe)
{
   void *data;

   _ecore_exe_threads_terminate(obj);

   data = exe->data;
   if (exe->pre_free_cb) exe->pre_free_cb(data, obj);

   IF_FN_DEL(ecore_main_win32_handler_del, exe->h_close);

   IF_FN_DEL(CloseHandle, exe->process_thread);
   IF_FN_DEL(CloseHandle, exe->process);
   IF_FN_DEL(CloseHandle, exe->pipe_write.child_pipe);
   IF_FN_DEL(CloseHandle, exe->pipe_write.child_pipe_x);
   IF_FN_DEL(CloseHandle, exe->pipe_error.child_pipe);
   IF_FN_DEL(CloseHandle, exe->pipe_read.child_pipe);

   IF_FREE(exe->cmd);

   Efl_Loop_Data *loop = efl_data_scope_get(exe->loop, EFL_LOOP_CLASS);
   if (loop) loop->exes = eina_list_remove(loop->exes, obj);
   IF_FREE(exe->tag);
}

/**
 * @internal
 * @brief Pauses (suspends) the child process.
 *
 * @param obj Unused Ecore_Exe object.
 * @param exe The private data of the Ecore_Exe object.
 */
void
_impl_ecore_exe_pause(Ecore_Exe *obj EINA_UNUSED, Ecore_Exe_Data *exe)
{
   if (exe->is_suspended) return;
   if (SuspendThread(exe->process_thread) != (DWORD)-1) exe->is_suspended = 1;
}

/**
 * @internal
 * @brief Resumes a paused (suspended) child process.
 *
 * @param obj Unused Ecore_Exe object.
 * @param exe The private data of the Ecore_Exe object.
 */
void
_impl_ecore_exe_continue(Ecore_Exe *obj EINA_UNUSED, Ecore_Exe_Data *exe)
{
   if (!exe->is_suspended) return;
   if (ResumeThread(exe->process_thread) != (DWORD)-1) exe->is_suspended = 0;
}

/**
 * @internal
 * @brief Sends an interrupt signal (simulates Ctrl+C) to the child process.
 *
 * This attempts to send a Ctrl+C event to the console window of the process.
 * It uses EnumWindows to find windows belonging to the child process.
 *
 * @param obj The Ecore_Exe object.
 * @param exe The private data of the Ecore_Exe object.
 */
void
_impl_ecore_exe_interrupt(Ecore_Exe *obj, Ecore_Exe_Data *exe)
{
   CloseHandle(exe->process_thread);
   exe->process_thread = NULL;
   CloseHandle(exe->process);
   exe->process = NULL;
   exe->sig = ECORE_EXE_WIN32_SIGINT;
   EnumWindows(_ecore_exe_enum_windows_procedure, (LPARAM)obj);
}

/**
 * @internal
 * @brief Sends a quit signal (simulates Ctrl+Break or WM_CLOSE/WM_QUIT) to the child process.
 *
 * This attempts to gracefully terminate the child process by sending
 * Ctrl+Break, WM_CLOSE, and WM_QUIT messages to its windows.
 *
 * @param obj The Ecore_Exe object.
 * @param exe The private data of the Ecore_Exe object.
 */
void
_impl_ecore_exe_quit(Ecore_Exe *obj, Ecore_Exe_Data *exe)
{
   CloseHandle(exe->process_thread);
   exe->process_thread = NULL;
   CloseHandle(exe->process);
   exe->process = NULL;
   exe->sig = ECORE_EXE_WIN32_SIGQUIT;
   EnumWindows(_ecore_exe_enum_windows_procedure, (LPARAM)obj);
}

/**
 * @internal
 * @brief Terminates the child process forcefully (similar to SIGTERM).
 *
 * This function attempts to terminate the child process, potentially using
 * TerminateProcess as a last resort via _ecore_exe_enum_windows_procedure.
 *
 * @param obj The Ecore_Exe object.
 * @param exe The private data of the Ecore_Exe object.
 */
void
_impl_ecore_exe_terminate(Ecore_Exe *obj, Ecore_Exe_Data *exe)
{
/*    CloseHandle(exe->thread); */
   CloseHandle(exe->process);
   exe->process = NULL;
   exe->sig = ECORE_EXE_WIN32_SIGTERM;
   while (EnumWindows(_ecore_exe_enum_windows_procedure, (LPARAM)obj));
}

/**
 * @internal
 * @brief Kills the child process immediately (similar to SIGKILL).
 *
 * This function forcefully terminates the child process using TerminateProcess
 * via _ecore_exe_enum_windows_procedure.
 *
 * @param obj The Ecore_Exe object.
 * @param exe The private data of the Ecore_Exe object.
 */
void
_impl_ecore_exe_kill(Ecore_Exe *obj, Ecore_Exe_Data *exe)
{
   CloseHandle(exe->process_thread);
   exe->process_thread = NULL;
   CloseHandle(exe->process);
   exe->process = NULL;
   exe->sig = ECORE_EXE_WIN32_SIGKILL;
   while (EnumWindows(_ecore_exe_enum_windows_procedure, (LPARAM)obj));
}

/**
 * @internal
 * @brief Sets auto-flushing limits for pipe data (Not implemented on Windows).
 *
 * @param obj Unused.
 * @param exe Unused.
 * @param start_bytes Unused.
 * @param end_bytes Unused.
 * @param start_lines Unused.
 * @param end_lines Unused.
 */
void
_impl_ecore_exe_auto_limits_set(Ecore_Exe *obj EINA_UNUSED,
                                Ecore_Exe_Data *exe EINA_UNUSED,
                                int        start_bytes EINA_UNUSED,
                                int        end_bytes EINA_UNUSED,
                                int        start_lines EINA_UNUSED,
                                int        end_lines EINA_UNUSED)
{
   ERR("Not implemented on windows!");
}

/**
 * @internal
 * @brief Sends a signal to the child process (Not implemented on Windows).
 *
 * @param obj Unused.
 * @param exe Unused.
 * @param num Unused.
 */
void
_impl_ecore_exe_signal(Ecore_Exe *obj EINA_UNUSED,
                       Ecore_Exe_Data *exe EINA_UNUSED,
                       int num EINA_UNUSED)
{
   ERR("Not implemented on windows!");
}

/**
 * @internal
 * @brief Sends a SIGHUP signal to the child process (Not implemented on Windows).
 *
 * @param obj Unused.
 * @param exe Unused.
 */
void
_impl_ecore_exe_hup(Ecore_Exe *obj EINA_UNUSED,
                    Ecore_Exe_Data *exe EINA_UNUSED)
{
   ERR("Not implemented on windows!");
}
