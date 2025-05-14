#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/**
 * @file
 * @brief This file implements a helper mechanism to resolve proxy URIs using an
 * external process. This is typically used when libproxy is not available or
 * not working correctly. It communicates with a helper executable
 * (efl_net_proxy_helper) via stdin/stdout.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "Ecore.h"
#include "ecore_private.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"
#include "../../static_libs/buildsystem/buildsystem.h"

/**
 * @brief Represents a request to the proxy helper.
 */
typedef struct {
   Eina_Thread_Queue  *thq;      /**< Thread queue for communication back to the requesting thread. */
   char               *str;      /**< The string sent to the helper process (e.g., "P ID URL\n"). */
   char              **proxies;  /**< Array of proxy strings returned by the helper. NULL-terminated.
                                   * Example: {"http://proxy.example.com:8080", "socks5://localhost:1080", NULL} */
   int                 id;       /**< Unique identifier for this request. */
   int                 busy;     /**< Counter indicating if the request is being processed (e.g., waiting for thread queue). */
   int                 fails;    /**< Number of times this request has failed (e.g., due to helper process restart). */
} Efl_Net_Proxy_Helper_Req;

/**
 * @brief Message structure for the thread queue.
 */
typedef struct {
   Eina_Thread_Queue_Msg   head;     /**< Standard thread queue message header. */
   char                  **proxies;  /**< Array of proxy strings. Ownership is transferred.
                                      * Example: {"http://proxy.example.com:8080", NULL} */
} Efl_Net_Proxy_Helper_Thq_Msg;

static Eina_Bool            _efl_net_proxy_helper_works            = EINA_TRUE; /**< Flag indicating if the helper mechanism is functional. Set to EINA_FALSE on fatal errors. */
static Ecore_Exe           *_efl_net_proxy_helper_exe              = NULL; /**< Handle for the running helper executable. */
static Eina_Prefix         *_efl_net_proxy_helper_prefix           = NULL; /**< Prefix for finding helper executable and other resources. */
static Eina_Spinlock        _efl_net_proxy_helper_queue_lock; /**< Spinlock to protect access to the request queue. */
static int                  _efl_net_proxy_helper_req_id           = 0; /**< Counter for generating unique request IDs. */
static Eina_List           *_efl_net_proxy_helper_queue            = NULL; /**< List of pending Efl_Net_Proxy_Helper_Req. */
static Ecore_Event_Handler *_efl_net_proxy_helper_handler_exe_del  = NULL; /**< Event handler for helper process deletion. */
static Ecore_Event_Handler *_efl_net_proxy_helper_handler_exe_data = NULL; /**< Event handler for data received from helper process. */
static Eina_Bool            _efl_net_proxy_helper_queue_lock_init  = EINA_FALSE; /**< Flag indicating if the queue lock has been initialized. */
static int                  _efl_net_proxy_helper_init_num         = 0; /**< Initialization counter for the helper system. */

static int locks = 0; /**< Debugging counter for spinlock usage, potentially for detecting unbalanced lock/unlock. */

#ifdef _WIN32
# define HELPER_EXT ".exe"
#else
# define HELPER_EXT
#endif

/**
 * @brief Callback invoked when the helper Ecore_Exe object is deleted.
 *
 * This can happen if the helper process exits unexpectedly or is killed.
 * It sets _efl_net_proxy_helper_exe to NULL.
 *
 * @param data User data (unused).
 * @param ev The EFL event information.
 */
static void
_efl_net_proxy_helper_delete_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   if (ev->object == _efl_net_proxy_helper_exe)
     {
        INF("HTTP proxy helper object died before the process exited.");
        _efl_net_proxy_helper_exe = NULL;
     }
}

/**
 * @brief Spawns the external proxy helper executable.
 *
 * If the helper is already running or the system is marked as not working,
 * this function does nothing. It constructs the path to the helper
 * executable and launches it with piped stdin/stdout.
 * Existing requests in the queue are resent to the new helper instance.
 */
static void
_efl_net_proxy_helper_spawn(void)
{
   char buf[PATH_MAX];
   Eina_List *l;
   Efl_Net_Proxy_Helper_Req *req;
#ifdef NEED_RUN_IN_TREE
   static int run_in_tree = -1;
#endif

   if (!_efl_net_proxy_helper_works) return;
   if (_efl_net_proxy_helper_exe) return;
#ifdef NEED_RUN_IN_TREE
   if (run_in_tree == -1)
     {
        run_in_tree = 0;
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
        if (getuid() == geteuid())
#endif
          {
             if (getenv("EFL_RUN_IN_TREE")) run_in_tree = 1;
          }
     }
   // find binary location path
   if (run_in_tree == 1)
     bs_binary_get(buf, sizeof(buf), "ecore_con", "efl_net_proxy_helper");
   else
#endif
     snprintf
       (buf, sizeof(buf),
        "%s/ecore_con/utils/"MODULE_ARCH"/efl_net_proxy_helper"HELPER_EXT,
        eina_prefix_lib_get(_efl_net_proxy_helper_prefix));
  // run it with stdin/out piped line buffered with events
   _efl_net_proxy_helper_exe = ecore_exe_pipe_run
     (buf,
      ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_WRITE |
      ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_TERM_WITH_PARENT |
      ECORE_EXE_NOT_LEADER, NULL);
   // resend unhandled requests
   eina_spinlock_take(&_efl_net_proxy_helper_queue_lock);
     {
        locks++;
        EINA_LIST_FOREACH(_efl_net_proxy_helper_queue, l, req)
          ecore_exe_send(_efl_net_proxy_helper_exe,
                         req->str, strlen(req->str));
        locks--;
     }
   eina_spinlock_release(&_efl_net_proxy_helper_queue_lock);
   efl_event_callback_add(_efl_net_proxy_helper_exe, EFL_EVENT_DEL,
                          _efl_net_proxy_helper_delete_cb, NULL);
}

/**
 * @brief Kills the running proxy helper executable.
 *
 * If no helper is running or if there are pending requests in the queue,
 * this function does nothing. Otherwise, it terminates and frees the
 * helper process.
 */
static void
_efl_net_proxy_helper_kill(void)
{
   if (!_efl_net_proxy_helper_exe) return;
   // don't exit if anything is pending
   if (_efl_net_proxy_helper_queue) return;
   ecore_exe_kill(_efl_net_proxy_helper_exe);
   ecore_exe_free(_efl_net_proxy_helper_exe);
   _efl_net_proxy_helper_exe = NULL;
}

/**
 * @brief Cancels all pending proxy requests.
 *
 * This function iterates through the request queue, sends a NULL proxy list
 * back to each waiting thread (signifying cancellation or failure), and
 * cleans up the request data.
 */
static void
_efl_net_proxy_helper_cancel(void)
{
   eina_spinlock_take(&_efl_net_proxy_helper_queue_lock);
     {
        locks++;
        Efl_Net_Proxy_Helper_Req *req;

        EINA_LIST_FREE(_efl_net_proxy_helper_queue, req)
          {
             Efl_Net_Proxy_Helper_Thq_Msg *msg;
             void *ref;

             msg = eina_thread_queue_send
               (req->thq, sizeof(Efl_Net_Proxy_Helper_Thq_Msg), &ref);
             msg->proxies = NULL;
             eina_thread_queue_send_done(req->thq, ref);
             if (!req->busy)
               {
                  free(req->str);
                  ecore_con_libproxy_proxies_free(req->proxies);
                  eina_thread_queue_free(req->thq);
                  free(req);
               }
          }
        locks--;
     }
   eina_spinlock_release(&_efl_net_proxy_helper_queue_lock);
}

/**
 * @brief Adds a resolved proxy URL to a specific request or finalizes the request.
 *
 * This function is called when the helper process sends back a proxy URL
 * or an end-of-proxies marker for a given request ID.
 *
 * @param id The ID of the request to update.
 * @param url The proxy URL string (e.g., "http://proxy.example.com:8080").
 *            If NULL, it signifies the end of proxies for this request, and
 *            the accumulated list is sent back to the waiting thread.
 */
static void
_efl_net_proxy_helper_proxy_add(int id, const char *url)
{
   Eina_List *l;
   Efl_Net_Proxy_Helper_Req *req;

   eina_spinlock_take(&_efl_net_proxy_helper_queue_lock);
     {
        locks++;
        EINA_LIST_FOREACH(_efl_net_proxy_helper_queue, l, req)
          {
             if (req->id == id) break;
             req = NULL;
          }
        if (req)
          {
             if (url)
               {
                  char **proxies = req->proxies;
                  int n = 0;

                  if (proxies)
                    {
                       for (n = 0; proxies[n]; n++);
                    }
                  n++;
                  proxies = realloc(proxies, sizeof(char *) * (n + 1));
                  if (proxies)
                    {
                       req->proxies = proxies;
                       proxies[n - 1] = strdup(url);
                       proxies[n] = NULL;
                    }
                  else
                    {
                       ERR("Out of memory allocating proxies in helper");
                       goto err;
                    }
               }
             else
               {
                  Efl_Net_Proxy_Helper_Thq_Msg *msg;
                  void *ref;

                  msg = eina_thread_queue_send
                    (req->thq, sizeof(Efl_Net_Proxy_Helper_Thq_Msg), &ref);
                  msg->proxies =  req->proxies;
                  req->proxies = NULL;
                  eina_thread_queue_send_done(req->thq, ref);
               }
          }
err:
        locks--;
     }
   eina_spinlock_release(&_efl_net_proxy_helper_queue_lock);
}

/**
 * @brief Callback for ECORE_EXE_EVENT_DEL events.
 *
 * Handles the unexpected termination of the helper executable.
 * It may attempt to respawn the helper if requests are pending,
 * unless it has failed too many times, in which case it cancels all requests.
 *
 * @param data User data (unused).
 * @param type The event type (ECORE_EXE_EVENT_DEL).
 * @param info Event-specific information (Ecore_Exe_Event_Del).
 * @return ECORE_CALLBACK_PASS_ON or ECORE_CALLBACK_DONE.
 */
static Eina_Bool
_efl_net_proxy_helper_cb_exe_del(void *data EINA_UNUSED, int type EINA_UNUSED, void *info)
{
   Ecore_Exe_Event_Del *event = info;
   int min_fails = 0;

   if (!_efl_net_proxy_helper_exe) return EINA_TRUE;
   if (event->exe == _efl_net_proxy_helper_exe)
     {
        static double last_respawn = 0.0;
        double t;
        Eina_Bool respawn = EINA_FALSE;

        t = ecore_time_get();
        _efl_net_proxy_helper_exe = NULL;
        eina_spinlock_take(&_efl_net_proxy_helper_queue_lock);
          {
             locks++;
             if (_efl_net_proxy_helper_queue)
               {
                  if ((t - last_respawn) > 5.0) respawn = EINA_TRUE;
               }
             if (respawn)
               {
                  Eina_List *l;
                  Efl_Net_Proxy_Helper_Req *req;

                  EINA_LIST_FOREACH(_efl_net_proxy_helper_queue, l, req)
                    {
                       req->fails++;
                       if (req->fails > min_fails) min_fails = req->fails;
                    }
               }
             locks--;
          }
        eina_spinlock_release(&_efl_net_proxy_helper_queue_lock);
        if (min_fails >= 5) _efl_net_proxy_helper_cancel();
        else if (respawn)
          {
             last_respawn = t;
             _efl_net_proxy_helper_spawn();
          }
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/**
 * @brief Callback for ECORE_EXE_EVENT_DATA events.
 *
 * Processes data received from the stdout of the helper executable.
 * Lines are parsed to extract proxy information or failure notifications.
 * - 'F': Fatal error, helper system is disabled.
 * - 'P ID P PROXY_URL': A proxy URL for the given ID.
 * - 'P ID E': End of proxies for the given ID.
 *
 * @param data User data (unused).
 * @param type The event type (ECORE_EXE_EVENT_DATA).
 * @param info Event-specific information (Ecore_Exe_Event_Data).
 * @return ECORE_CALLBACK_PASS_ON or ECORE_CALLBACK_DONE.
 */
static Eina_Bool
_efl_net_proxy_helper_cb_exe_data(void *data EINA_UNUSED, int type EINA_UNUSED, void *info)
{
   Ecore_Exe_Event_Data *event = info;

   if (!_efl_net_proxy_helper_exe) return EINA_TRUE;
   if (event->exe == _efl_net_proxy_helper_exe)
     {
        if (event->lines)
          {
             int i;

             for (i = 0; event->lines[i].line; i++)
               {
                  char *line = event->lines[i].line;

                  if (line[0] == 'F') // failure
                    {
                       _efl_net_proxy_helper_works = EINA_FALSE;
                       _efl_net_proxy_helper_cancel();
                       _efl_net_proxy_helper_kill();
                    }
                  else if ((line[0] == 'P') && (line[1] == ' ')) // proxy
                    {
                       int id = atoi(line + 2);
                       char *p, *url = NULL;

                       for (p = line + 2; *p && (*p != ' '); p++);
                       if ((p[0] == ' ') && (p[1] == 'P') && (p[2] == ' '))
                         url = p + 3;
                       else if ((p[0] == ' ') && (p[1] == 'E'))
                         url = NULL;
                       _efl_net_proxy_helper_proxy_add(id, url);
                    }
               }
          }
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/**
 * @brief Checks if the proxy helper system is currently considered functional.
 * @return EINA_TRUE if the helper can be used, EINA_FALSE otherwise.
 */
Eina_Bool
_efl_net_proxy_helper_can_do(void)
{
   return _efl_net_proxy_helper_works;
}

/**
 * @brief Sends a string to the helper process's stdin.
 *
 * This function is typically called from the main loop thread via
 * ecore_main_loop_thread_safe_call_async. It ensures the helper
 * process is spawned if not already running.
 *
 * @param data The string to send (e.g., "P ID URL\n"). This string is freed
 *             by the function.
 */
static void
_efl_net_proxy_helper_cb_send_do(void *data)
{
   char *str = data;
   if (!str) return;
   // spawn exe if needed
   if (!_efl_net_proxy_helper_exe) _efl_net_proxy_helper_spawn();
   if (_efl_net_proxy_helper_exe)
     ecore_exe_send(_efl_net_proxy_helper_exe, str, strlen(str));
   free(str);
}

/**
 * @brief Sends a URL to the proxy helper to resolve proxies.
 *
 * This function is called from a worker thread. It queues a request and
 * sends a message to the helper process (via the main loop) to look up
 * proxies for the given URL.
 *
 * @param url The URL for which to find proxies (e.g., "http://example.com").
 * @param eth The Ecore_Thread from which this function is called. Used for
 *            sanity checks.
 * @return A unique request ID if successful, or -1 on failure. This ID is
 *         used with _efl_net_proxy_helper_url_wait().
 */
int
_efl_net_proxy_helper_url_req_send(const char *url, Ecore_Thread *eth)
{
   char *buf;
   int id = -1;
   Efl_Net_Proxy_Helper_Req *req;

   if (!_efl_net_proxy_helper_works) return -1;
   eina_spinlock_take(&_efl_net_proxy_helper_queue_lock);
     {
        locks++;
        // new id - just an int that eventually loops.
        _efl_net_proxy_helper_req_id++;
        if (_efl_net_proxy_helper_req_id >= ((1 << 30) - 1))
          _efl_net_proxy_helper_req_id = 1;
        id = _efl_net_proxy_helper_req_id;
        locks--;
     }
   eina_spinlock_release(&_efl_net_proxy_helper_queue_lock);
   if (ecore_thread_check(eth)) return -1;
   // create request to quque up to look up responses for
   req = calloc(1, sizeof(Efl_Net_Proxy_Helper_Req));
   if (!req) return -1;
   req->id = id;
   req->thq = eina_thread_queue_new();
   if (!req->thq)
     {
        free(req);
        return -1;
     }
   buf = alloca(strlen(url) + 256);
   sprintf(buf, "P %i %s\n", req->id, url);
   req->str = strdup(buf);
   if ((!req->str) || ecore_thread_check(eth))
     {
        eina_thread_queue_free(req->thq);
        free(req->str);
        free(req);
        return -1;
     }
   eina_spinlock_take(&_efl_net_proxy_helper_queue_lock);
     {
        locks++;
        _efl_net_proxy_helper_queue =
          eina_list_append(_efl_net_proxy_helper_queue, req);
        locks--;
     }
   eina_spinlock_release(&_efl_net_proxy_helper_queue_lock);
   // actually send the req now i'ts queued
   ecore_main_loop_thread_safe_call_async
     (_efl_net_proxy_helper_cb_send_do, strdup(buf));
   return id;
}

/**
 * @brief Waits for the result of a proxy lookup request.
 *
 * This function is called from the same worker thread that initiated the
 * request with _efl_net_proxy_helper_url_req_send(). It blocks until the
 * helper process responds or the request is cancelled.
 *
 * @param id The request ID returned by _efl_net_proxy_helper_url_req_send().
 * @return A NULL-terminated array of proxy strings (e.g.,
 *         `{"direct://", "http://proxy.example.com:8080", NULL}`).
 *         The caller is responsible for freeing this array and its contents
 *         using ecore_con_libproxy_proxies_free(). Returns NULL on failure,
 *         if the ID is invalid, or if the request was cancelled.
 *         Example of a returned array:
 *         char **proxies = _efl_net_proxy_helper_url_wait(req_id);
 *         if (proxies) {
 *           for (int i = 0; proxies[i]; i++) {
 *             printf("Proxy: %s\n", proxies[i]);
 *           }
 *           // Assuming ecore_con_libproxy_proxies_free iterates and frees strings
 *           // and then the array itself.
 *           ecore_con_libproxy_proxies_free(proxies);
 *         }
 */
char **
_efl_net_proxy_helper_url_wait(int id)
{
   Eina_List *l;
   Efl_Net_Proxy_Helper_Req *req;
   Efl_Net_Proxy_Helper_Thq_Msg *msg;
   void *ref;
   char **ret = NULL;

   if (id < 0) return NULL;
   if (!_efl_net_proxy_helper_exe) return NULL;
   eina_spinlock_take(&_efl_net_proxy_helper_queue_lock);
     {
        locks++;
        EINA_LIST_FOREACH(_efl_net_proxy_helper_queue, l, req)
          {
             if (req->id == id) break;
             req = NULL;
          }
        if (!req) goto end;
        req->busy++;
        locks--;
     }
   eina_spinlock_release(&_efl_net_proxy_helper_queue_lock);

   msg = eina_thread_queue_wait(req->thq, &ref);
   if (!msg) return NULL;
   ret = msg->proxies;
   msg->proxies = NULL;
   eina_thread_queue_wait_done(req->thq, ref);

   eina_spinlock_take(&_efl_net_proxy_helper_queue_lock);
     {
        locks++;
        free(req->str);
        ecore_con_libproxy_proxies_free(req->proxies);
        eina_thread_queue_free(req->thq);
        _efl_net_proxy_helper_queue =
          eina_list_remove(_efl_net_proxy_helper_queue, req);
        free(req);
     }
end:
   locks--;
   eina_spinlock_release(&_efl_net_proxy_helper_queue_lock);
   return ret;
}

/**
 * @brief Initializes the proxy helper system.
 *
 * This function sets up necessary resources like prefixes for finding the
 * helper executable, spinlocks, and event handlers. It uses a reference
 * counter (_efl_net_proxy_helper_init_num) to allow multiple initializations.
 */
void
_efl_net_proxy_helper_init(void)
{
   _efl_net_proxy_helper_init_num++;
   if (_efl_net_proxy_helper_init_num > 1) return;
   if (_efl_net_proxy_helper_prefix) return;
   _efl_net_proxy_helper_prefix = eina_prefix_new
     (NULL, ecore_con_init, "ECORE_CON", "ecore_con", "checkme",
      PACKAGE_BIN_DIR, PACKAGE_LIB_DIR, PACKAGE_DATA_DIR, PACKAGE_DATA_DIR);
   if (!_efl_net_proxy_helper_queue_lock_init)
     {
        eina_spinlock_new(&_efl_net_proxy_helper_queue_lock);
        _efl_net_proxy_helper_queue_lock_init = EINA_TRUE;
     }
   _efl_net_proxy_helper_handler_exe_del =
     ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                             _efl_net_proxy_helper_cb_exe_del, NULL);
   _efl_net_proxy_helper_handler_exe_data =
     ecore_event_handler_add(ECORE_EXE_EVENT_DATA,
                             _efl_net_proxy_helper_cb_exe_data, NULL);
}

/**
 * @brief Shuts down the proxy helper system.
 *
 * This function cleans up resources allocated by _efl_net_proxy_helper_init().
 * It uses a reference counter to ensure resources are freed only when the
 * last user shuts down. If the helper executable is running, it's cancelled
 * and killed.
 */
void
_efl_net_proxy_helper_shutdown(void)
{
   _efl_net_proxy_helper_init_num--;
   if (_efl_net_proxy_helper_init_num > 0) return;
   if (!_efl_net_proxy_helper_prefix) return;
   if (_efl_net_proxy_helper_exe)
     {
        _efl_net_proxy_helper_cancel();
        _efl_net_proxy_helper_kill();
     }
   eina_spinlock_take(&_efl_net_proxy_helper_queue_lock);
   eina_spinlock_release(&_efl_net_proxy_helper_queue_lock);
   eina_prefix_free(_efl_net_proxy_helper_prefix);
   _efl_net_proxy_helper_prefix = NULL;
   ecore_event_handler_del(_efl_net_proxy_helper_handler_exe_del);
   _efl_net_proxy_helper_handler_exe_del = NULL;
   ecore_event_handler_del(_efl_net_proxy_helper_handler_exe_data);
   _efl_net_proxy_helper_handler_exe_data = NULL;
}
