/* EINA - EFL data type library
 * Copyright (C) 2015 Carsten Haitzler
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

#include <Eina.h>
#include <Ecore.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define SWAP_64(x) x
#define SWAP_32(x) x
#define SWAP_16(x) x
#else
#define SWAP_64(x) eina_swap64(x)
#define SWAP_32(x) eina_swap32(x)
#define SWAP_16(x) eina_swap16(x)
#endif

#define EXTRACT(_buf, pval, sz) \
{ \
   memcpy(pval, _buf, sz); \
   _buf += sz; \
}
#define _EVLOG_INTERVAL 0.2 //!< Interval in seconds for fetching event logs.

static int               _evlog_max_times = 0; //!< Maximum number of times to fetch event logs.
static Ecore_Timer      *_evlog_fetch_timer = NULL; //!< Timer for periodically fetching event logs.
static FILE             *_evlog_file = NULL; //!< File pointer for storing event log data.

static int _cl_stat_reg_opcode = EINA_DEBUG_OPCODE_INVALID; //!< Opcode for registering a client status observer.
static int _cid_from_pid_opcode = EINA_DEBUG_OPCODE_INVALID; //!< Opcode for getting client ID from process ID.
static int _prof_on_opcode = EINA_DEBUG_OPCODE_INVALID; //!< Opcode for turning profiler on.
static int _prof_off_opcode = EINA_DEBUG_OPCODE_INVALID; //!< Opcode for turning profiler off.
static int _cpufreq_on_opcode = EINA_DEBUG_OPCODE_INVALID; //!< Opcode for turning CPU frequency scaling on.
static int _cpufreq_off_opcode = EINA_DEBUG_OPCODE_INVALID; //!< Opcode for turning CPU frequency scaling off.
static int _evlog_get_opcode = EINA_DEBUG_OPCODE_INVALID; //!< Opcode for getting event log data.

static Eina_Debug_Session *_session = NULL; //!< The main debug session.

static int _cid = 0; //!< Stores the client ID of the process being debugged.

static int my_argc = 0; //!< Stores the argument count from main().
static char **my_argv = NULL; //!< Stores the argument vector from main().

/**
 * @brief Callback function invoked when event log data is received from the debugged client.
 *
 * This function processes the received event log data. It writes the data to
 * the _evlog_file if it's open and the maximum number of receive times
 * (_evlog_max_times) has not been reached. If the maximum number of times is
 * reached, it closes the file and quits the main loop.
 *
 * The buffer format is expected to be:
 * - First 4 bytes: unsigned int overflow_count
 * - Remaining bytes: event log data
 *
 * The data written to the file is prefixed with a header:
 * - 0xffee211 (magic number)
 * - blocksize (size of the event log data)
 * - overflow_count
 *
 * @param session The debug session (unused).
 * @param src The source client ID (unused).
 * @param buffer Pointer to the buffer containing the event log data.
 * @param size Size of the buffer.
 * @return EINA_TRUE to keep the handler, EINA_FALSE to remove it.
 */
static Eina_Bool
_evlog_get_cb(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED, void *buffer, int size)
{
   static int received_times = 0;
   unsigned char *d = buffer;
   unsigned int *overflow = (unsigned int *)(d + 0);
   unsigned char *p = d + 4;
   unsigned int blocksize = size - 4;

   if(++received_times <= _evlog_max_times)
     {
        if ((_evlog_file) && (blocksize > 0))
          {
             unsigned int header[3];

             header[0] = 0xffee211;
             header[1] = SWAP_32(blocksize);
             header[2] = *overflow;
             if (fwrite(header, 1, 12, _evlog_file) < 12 ||
                   fwrite(p, 1, blocksize, _evlog_file) < blocksize)
                printf("Error writing bytes to evlog file\n");
          }
     }

   if(received_times == _evlog_max_times)
     {
        printf("Received last evlog response\n");
        if (_evlog_file) fclose(_evlog_file);
        _evlog_file = NULL;
        ecore_main_loop_quit();
     }

   return EINA_TRUE;
}

/**
 * @brief Timer callback function to periodically request event log data.
 *
 * This function is called by an Ecore_Timer. It sends a request for event log
 * data to the debugged client (_cid) using the _evlog_get_opcode.
 * If the maximum number of send attempts (_evlog_max_times) is reached,
 * it sends a command to turn off CPU frequency scaling and deletes the timer.
 *
 * @param data User data associated with the timer (unused).
 * @return ECORE_CALLBACK_RENEW to continue the timer, ECORE_CALLBACK_CANCEL to stop it.
 */
static Eina_Bool
_cb_evlog(void *data EINA_UNUSED)
{
   static int sent_times = 0;
   Eina_Bool ret = ECORE_CALLBACK_RENEW;
   if(++sent_times <= _evlog_max_times)
         eina_debug_session_send(_session, _cid, _evlog_get_opcode, NULL, 0);

   if(sent_times == _evlog_max_times)
     {
        eina_debug_session_send(_session, _cid, _cpufreq_off_opcode, NULL, 0);
        ecore_timer_del(_evlog_fetch_timer);
        _evlog_fetch_timer = NULL;
        ret = ECORE_CALLBACK_CANCEL;
     }

   return ret;
}

/**
 * @brief Callback function invoked when the client ID (CID) is received.
 *
 * This function is called after requesting the CID for a given PID.
 * It stores the received CID in the global `_cid` variable.
 * Based on the command-line arguments (`my_argv`), it then sends appropriate
 * debug commands to the client, such as enabling/disabling profiling or
 * event logging.
 *
 * For "evlogon", it calculates `_evlog_max_times` based on the duration
 * provided in `my_argv[3]` and the `_EVLOG_INTERVAL`. It then sets up
 * the event log file and starts a timer (`_evlog_fetch_timer`) to periodically
 * request event logs.
 *
 * @param session The debug session (unused).
 * @param cid The client ID of the sender (unused, the CID of interest is in the buffer).
 * @param buffer Pointer to the buffer containing the integer client ID.
 * @param size Size of the buffer (unused, expected to be sizeof(int)).
 * @return EINA_TRUE to keep the handler.
 */
static Eina_Bool
_cid_get_cb(Eina_Debug_Session *session EINA_UNUSED, int cid EINA_UNUSED, void *buffer, int size EINA_UNUSED)
{
   _cid = *(int *)buffer;

   const char *op_str = my_argv[1];
   Eina_Bool quit = EINA_TRUE;

   if ((!strcmp(op_str, "pon")) && (3 <= (my_argc - 1)))
     {
        int freq = SWAP_32(atoi(my_argv[3]));
        eina_debug_session_send(_session, _cid, _prof_on_opcode, &freq, sizeof(int));
     }
   else if (!strcmp(op_str, "poff"))
      eina_debug_session_send(_session, _cid, _prof_off_opcode,  NULL, 0);
   else if (!strcmp(op_str, "evlogon") && (3 <= (my_argc - 1)))
     {
        double max_time;
        sscanf(my_argv[3], "%lf", &max_time);
        _evlog_max_times = max_time > 0 ? (max_time/_EVLOG_INTERVAL+1) : 1;
        eina_debug_session_send(_session, 0, _cl_stat_reg_opcode, NULL, 0);
        printf("Evlog request will be sent %d times\n", _evlog_max_times);
        eina_debug_session_send(_session, _cid, _cpufreq_on_opcode,  NULL, 0);

        /* Creating the evlog file and setting the timer */
        char path[4096];
        int pid = atoi(my_argv[2]);
        snprintf(path, sizeof(path), "%s/efl_debug_evlog-%ld.log",
              getenv("HOME"), (long)pid);
        _evlog_file = fopen(path, "wb");
        _evlog_fetch_timer = ecore_timer_add(_EVLOG_INTERVAL, _cb_evlog, NULL);

        quit = EINA_FALSE;
     }
   else if (!strcmp(op_str, "evlogoff"))
        eina_debug_session_send(_session, _cid, _cpufreq_off_opcode,  NULL, 0);

   if(quit)
        ecore_main_loop_quit();

   return EINA_TRUE;
}

/**
 * @brief Callback function invoked when information about new clients is received.
 *
 * This function is called when the debug daemon reports that new clients
 * have connected. It parses the buffer to extract client information
 * (CID, PID, name) and prints it to the console, unless event logging
 * is active (`_evlog_fetch_timer` is not NULL).
 *
 * The buffer contains a series of client records. Each record has:
 * - int cid: Client ID (32-bit integer, needs byte swapping on big-endian)
 * - int pid: Process ID (32-bit integer, needs byte swapping on big-endian)
 * - char name[]: Null-terminated client name string
 *
 * @param session The debug session (unused).
 * @param src The source client ID (unused).
 * @param buffer Pointer to the buffer containing information about added clients.
 * @param size Size of the buffer.
 * @return EINA_TRUE to keep the handler.
 */
static Eina_Bool
_clients_info_added_cb(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED, void *buffer, int size)
{
   char *buf = buffer;
   while(size)
     {
        int cid, pid, len;
        EXTRACT(buf, &cid, sizeof(int));
        EXTRACT(buf, &pid, sizeof(int));
        cid = SWAP_32(cid);
        pid = SWAP_32(pid);
        /* We dont need client notifications on evlog */
        if(!_evlog_fetch_timer)
           printf("Added: CID: %d - PID: %d - Name: %s\n", cid, pid, buf);
        len = strlen(buf) + 1;
        buf += len;
        size -= (2 * sizeof(int) + len);
     }
   return EINA_TRUE;
}

/**
 * @brief Callback function invoked when information about deleted clients is received.
 *
 * This function is called when the debug daemon reports that clients
 * have disconnected. It parses the buffer to extract the CIDs of deleted clients.
 *
 * If event logging is active (`_evlog_fetch_timer` is not NULL) and the
 * deleted client is the one being monitored for event logs (its CID matches `_cid`),
 * this function stops the event logging process: it deletes the timer,
 * closes the event log file, and quits the main loop.
 * Otherwise, it just prints the CID of the deleted client.
 *
 * The buffer contains a series of client records. Each record has:
 * - int cid: Client ID (32-bit integer, needs byte swapping on big-endian)
 *
 * @param session The debug session (unused).
 * @param src The source client ID (unused).
 * @param buffer Pointer to the buffer containing CIDs of deleted clients.
 * @param size Size of the buffer.
 * @return EINA_TRUE to keep the handler.
 */
static Eina_Bool
_clients_info_deleted_cb(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED, void *buffer, int size)
{
   char *buf = buffer;
   while(size)
     {
        int cid;
        EXTRACT(buf, &cid, sizeof(int));
        cid = SWAP_32(cid);
        size -= sizeof(int);

        /* If client deleted dont send anymore evlog requests */
        if(_evlog_fetch_timer)
          {
             if(_cid == cid)
               {
                  printf("Evlog debugged App closed (CID: %d), stopping evlog\n", cid);
                  ecore_timer_del(_evlog_fetch_timer);
                  _evlog_fetch_timer = NULL;
                  fclose(_evlog_file);
                  _evlog_file = NULL;
                  ecore_main_loop_quit();
               }
          }
        else
           printf("Deleted: CID: %d\n", cid);
     }
   return EINA_TRUE;
}

/**
 * @brief Dispatches debug messages in the main Ecore thread.
 *
 * This function is called via `ecore_main_loop_thread_safe_call_async`
 * to ensure that `eina_debug_dispatch` is executed in the context of
 * the main Ecore loop, making it thread-safe.
 *
 * @param data The buffer data to be dispatched by `eina_debug_dispatch`.
 *             This data is typically passed from `_disp_cb`.
 */
static void
_ecore_thread_dispatcher(void *data)
{
   eina_debug_dispatch(_session, data);
}

Eina_Bool
_disp_cb(Eina_Debug_Session *session EINA_UNUSED, void *buffer)
{
   ecore_main_loop_thread_safe_call_async(_ecore_thread_dispatcher, buffer);
   return EINA_TRUE;
}

/**
 * @brief Handles command-line arguments after the debug opcodes are registered.
 *
 * This function is called as a callback after `eina_debug_opcodes_register`
 * completes. It processes the command-line arguments stored in `my_argv`.
 * If the `flag` is false, it exits the program.
 * It overrides the default dispatch mechanism to use `_disp_cb` for thread safety.
 *
 * - If the command is "list", it requests a list of all connected clients.
 * - If other commands requiring a PID are given (e.g., "pon", "poff", "evlogon"),
 *   it extracts the PID from `my_argv[2]` and sends a request to get the
 *   corresponding client ID (CID) using `_cid_from_pid_opcode`. The actual
 *   command (like "pon") will be processed in `_cid_get_cb` once the CID is known.
 *
 * @param data User data associated with the callback (unused).
 * @param flag A boolean flag indicating success or failure of opcode registration.
 *             If EINA_FALSE, the program exits.
 */
static void
_args_handle(void *data EINA_UNUSED, Eina_Bool flag)
{
   if (!flag) exit(0);
   eina_debug_session_dispatch_override(_session, _disp_cb);;

   const char *op_str = my_argv[1];
   if (op_str && !strcmp(op_str, "list"))
     {
        eina_debug_session_send(_session, 0, _cl_stat_reg_opcode, NULL, 0);
     }
   else if (2 <= my_argc - 1)
     {
        int pid = atoi(my_argv[2]);
        eina_debug_session_send(_session, 0, _cid_from_pid_opcode, &pid, sizeof(int));
     }
}

EINA_DEBUG_OPCODES_ARRAY_DEFINE(ops,
      {"Daemon/Client/register_observer",  &_cl_stat_reg_opcode,   NULL},
      {"Daemon/Client/added",              NULL,                   &_clients_info_added_cb},
      {"Daemon/Client/deleted",            NULL,                   &_clients_info_deleted_cb},
      {"Daemon/Client/cid_from_pid",       &_cid_from_pid_opcode,  &_cid_get_cb},
      {"Profiler/on",                      &_prof_on_opcode,       NULL},
      {"Profiler/off",                     &_prof_off_opcode,      NULL},
      {"CPU/Freq/on",                      &_cpufreq_on_opcode,    NULL},
      {"CPU/Freq/off",                     &_cpufreq_off_opcode,   NULL},
      {"EvLog/get",                        &_evlog_get_opcode,     _evlog_get_cb},
      {NULL, NULL, NULL}
);

/**
 * @brief Main entry point for the efl_debug utility.
 *
 * Initializes Ecore, stores command-line arguments, connects to the local
 * Eina debug daemon, registers opcodes and their handlers, and starts the
 * Ecore main loop.
 *
 * Command-line usage examples:
 * - `efl_debug list`: Lists all EFL applications connected to the debug daemon.
 * - `efl_debug pon <pid> <freq>`: Turns on profiling for the application with PID <pid> at frequency <freq>.
 * - `efl_debug poff <pid>`: Turns off profiling for the application with PID <pid>.
 * - `efl_debug evlogon <pid> <duration_sec>`: Starts event logging for application <pid> for <duration_sec> seconds.
 * - `efl_debug evlogoff <pid>`: (Effectively) Stops event logging by turning off CPU frequency (which was turned on by evlogon).
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on successful execution, -1 on failure to connect to the debug daemon.
 */
int
main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   ecore_init();

   my_argc = argc;
   my_argv = argv;

   _session = eina_debug_local_connect(EINA_TRUE);
   if (!_session)
     {
        fprintf(stderr, "ERROR: Cannot connect to debug daemon.\n");
        return -1;
     }
   eina_debug_opcodes_register(_session, ops(), _args_handle, NULL);

   ecore_main_loop_begin();

   ecore_shutdown();

   return 0;
}
