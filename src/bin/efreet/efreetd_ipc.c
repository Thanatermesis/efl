#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include <Ecore_Ipc.h>

#include "efreetd.h"
#include "efreetd_cache.h"

extern FILE *efreetd_log_file;

static int init = 0; /**< Initialization counter for IPC. */
static Ecore_Ipc_Server *ipc = NULL; /**< The Ecore IPC server instance. */
static Ecore_Event_Handler *hnd_add = NULL; /**< Event handler for client connect. */
static Ecore_Event_Handler *hnd_del = NULL; /**< Event handler for client disconnect. */
static Ecore_Event_Handler *hnd_data = NULL; /**< Event handler for client data. */
static int clients = 0; /**< Number of currently connected clients. */
static Ecore_Timer *quit_timer_start = NULL; /**< Timer to start the quit countdown when no clients are initially connected. */
static Ecore_Timer *quit_timer = NULL; /**< Timer to quit efreetd when no clients are connected for a period. */

/**
 * @brief Callback function for the quit timer.
 *
 * This function is called when the quit_timer expires, indicating that
 * efreetd should shut down due to inactivity.
 * @param data Unused.
 * @return EINA_FALSE to stop the timer.
 */
static Eina_Bool
_cb_quit_timer(void *data EINA_UNUSED)
{
   quit_timer = NULL;
   quit();
   return EINA_FALSE;
}

/**
 * @brief Callback function for the initial quit timer.
 *
 * This function is called when efreetd starts and no clients connect
 * within a certain timeframe. It then starts the final quit_timer.
 * @param data Unused.
 * @return EINA_FALSE to stop the timer.
 */
static Eina_Bool
_cb_quit_timer_start(void *data EINA_UNUSED)
{
   quit_timer_start = NULL;
   if (quit_timer) ecore_timer_del(quit_timer);
   quit_timer = ecore_timer_add(10.0, _cb_quit_timer, NULL);
   return EINA_FALSE;
}

/**
 * @brief Broadcasts a message to all connected IPC clients.
 * @param svr The IPC server.
 * @param major The major opcode of the message.
 * @param minor The minor opcode of the message.
 * @param data The data payload of the message.
 * @param size The size of the data payload.
 */
static void
_broadcast(Ecore_Ipc_Server *svr, int major, int minor, void *data, int size)
{
   Eina_List *ipc_clients = ecore_ipc_server_clients_get(svr);
   Eina_List *l;
   Ecore_Ipc_Client *cl;

   EINA_LIST_FOREACH(ipc_clients, l, cl)
     {
        fprintf(efreetd_log_file, "[%09.3f] Client broadcast %i.%i\n", ecore_time_get(), major, minor);
        fflush(efreetd_log_file);
        ecore_ipc_client_send(cl, major, minor, 0, 0, 0, data, size);
     }
}

/**
 * @brief Parses a single null-terminated string from raw data.
 * @param data Pointer to the raw data.
 * @param size Size of the raw data.
 * @return A newly allocated string, or NULL on failure. The caller must free the returned string.
 * @note The input data is expected to contain a single string.
 */
static char *
_parse_str(void *data, int size)
{
   char *str = malloc(size + 1);
   if (!str) return NULL;
   memcpy(str, data, size);
   str[size] = 0;
   return str;
}

/**
 * @brief Parses a list of null-terminated strings from raw data.
 *
 * The raw data is expected to be a sequence of null-terminated strings,
 * concatenated together. For example: "string1\0string2\0string3\0".
 * @param data Pointer to the raw data.
 * @param size Size of the raw data.
 * @return A list (Eina_List) of newly allocated strings, or NULL on failure.
 *         The caller must free the list and its string elements.
 */
static Eina_List *
_parse_strs(void *data, int size)
{
   Eina_List *list = NULL;
   char *p, *p0 = NULL, *p1 = NULL, *e = (char *)data + size;

   for (p = data; p < e; p++)
     {
        if (!p0)
          {
             if (*p)
               {
                  p0 = p;
                  p1 = e;
               }
          }
        if ((!*p) && (p0))
          {
             p1 = strdup(p0);
             if (p1) list = eina_list_append(list, p1);
             p0 = NULL;
          }
     }
   if (p0)
     {
        p = malloc(p1 - p0 + 1);
        if (p)
          {
             memcpy(p, p0, p1 - p0);
             p[p1 - p0] = 0;
             list = eina_list_append(list, p);
          }
     }
   return list;
}

/**
 * @def IPC_HEAD(_type)
 * @brief Macro to boilerplate check if an IPC event is for our server.
 *
 * This macro retrieves the event structure and checks if the client's
 * server matches the current IPC server instance. If not, it passes
 * the event on.
 * @param _type The type of the Ecore_Ipc_Event_Client (e.g., Add, Del, Data).
 */
#define IPC_HEAD(_type) \
   Ecore_Ipc_Event_Client_##_type *e = event; \
   if (ecore_ipc_client_server_get(e->client) != ipc) \
     return ECORE_CALLBACK_PASS_ON

/**
 * @brief Callback for when a new client connects to the IPC server.
 * @param data Unused.
 * @param type Unused.
 * @param event The Ecore_Ipc_Event_Client_Add event data.
 * @return ECORE_CALLBACK_DONE to indicate the event was handled.
 */
static Eina_Bool
_cb_client_add(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   IPC_HEAD(Add);
   if (quit_timer)
     {
        ecore_timer_del(quit_timer);
        quit_timer = NULL;
     }
   if (quit_timer_start)
     {
        ecore_timer_del(quit_timer_start);
        quit_timer_start = NULL;
     }
   clients++;
   fprintf(efreetd_log_file, "[%09.3f] Add client (count=%i)\n", ecore_time_get(),
           clients);
   fflush(efreetd_log_file);
   return ECORE_CALLBACK_DONE;
}

/**
 * @brief Callback for when a client disconnects from the IPC server.
 * @param data Unused.
 * @param type Unused.
 * @param event The Ecore_Ipc_Event_Client_Del event data.
 * @return ECORE_CALLBACK_DONE to indicate the event was handled.
 */
static Eina_Bool
_cb_client_del(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   IPC_HEAD(Del);
   clients--;
   fprintf(efreetd_log_file, "[%09.3f] Del client (count=%i)\n", ecore_time_get(),
           clients);
   fflush(efreetd_log_file);
   if (clients == 0)
     {
        if (quit_timer) ecore_timer_del(quit_timer);
        quit_timer = ecore_timer_add(2.0, _cb_quit_timer, NULL);
     }
   return ECORE_CALLBACK_DONE;
}

/**
 * @brief Callback for when a client sends data to the IPC server.
 *
 * This function handles various client requests based on the major opcode
 * in the event data.
 * - Major 1: Register language. Client sends its LANG setting.
 *            Server replies with whether the desktop cache exists.
 * - Major 2: Add desktop directories. Client sends a list of directories.
 * - Major 3: Build desktop cache. Client may send its LANG setting.
 * - Major 4: Add icon directories. Client sends a list of directories.
 * - Major 5: Add icon extensions. Client sends a list of extensions.
 *
 * @param data Unused.
 * @param type Unused.
 * @param event The Ecore_Ipc_Event_Client_Data event data.
 * @return ECORE_CALLBACK_DONE to indicate the event was handled.
 */
static Eina_Bool
_cb_client_data(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Eina_List *strs;
   char *s;
   IPC_HEAD(Data);
   if (e->major == 1) // register lang
     { // input: str -> lang
        fprintf(efreetd_log_file, "[%09.3f] Client register lang\n", ecore_time_get());
        fflush(efreetd_log_file);
        if ((s = _parse_str(e->data, e->size)))
          {
             char envlang[128], *env;

             env = getenv("LANG");
             if (!((env) && (!strcmp(env, s))))
               {
                  snprintf(envlang, sizeof(envlang), "LANG=%s", s);
                  env = strdup(envlang);
                  putenv(env);
                  /* leak env intentionnally */
               }
             free(s);
          }
        // return if desktop cache exists (bool as minor)
        ecore_ipc_client_send(e->client, 1 /* register reply */,
                              cache_desktop_exists(), 0, 0, 0, NULL, 0);
     }
   else if (e->major == 2) // add desktop dirs
     { // input: array of str -> dirs
        fprintf(efreetd_log_file, "[%09.3f] Client add desktop dirs\n", ecore_time_get());
        fflush(efreetd_log_file);
        strs = _parse_strs(e->data, e->size);
        EINA_LIST_FREE(strs, s)
          {
             cache_desktop_dir_add(s);
             free(s);
          }
     }
   else if (e->major == 3) // build desktop cache
     { // input: str -> lang
        fprintf(efreetd_log_file, "[%09.3f] Client update desktop cache\n", ecore_time_get());
        fflush(efreetd_log_file);
        if ((s = _parse_str(e->data, e->size)))
          {
             char envlang[128], *env;

             env = getenv("LANG");
             if (!((env) && (!strcmp(env, s))))
               {
                  snprintf(envlang, sizeof(envlang), "LANG=%s", s);
                  env = strdup(envlang);
                  putenv(env);
                  /* leak env intentionnally */
               }
             free(s);
          }
        cache_desktop_update();
     }
   else if (e->major == 4) // add icon dirs
     { // input: array of str -> dirs
        fprintf(efreetd_log_file, "[%09.3f] Client add icon dirs\n", ecore_time_get());
        fflush(efreetd_log_file);
        strs = _parse_strs(e->data, e->size);
        EINA_LIST_FREE(strs, s)
          {
             cache_icon_dir_add(s);
             free(s);
          }
     }
   else if (e->major == 5) // add icon exts
     { // input: array of str -> exts
        fprintf(efreetd_log_file, "[%09.3f] Client add icon exts\n", ecore_time_get());
        fflush(efreetd_log_file);
        strs = _parse_strs(e->data, e->size);
        EINA_LIST_FREE(strs, s)
          {
             cache_icon_ext_add(s);
             free(s);
          }
     }
   return ECORE_CALLBACK_DONE;
}

///////////////////////////////////////////////////////////////////////////

void
send_signal_icon_cache_update(Eina_Bool update)
{
   _broadcast(ipc, 2 /* icon cache update */, update, NULL, 0);
}

void
send_signal_desktop_cache_update(Eina_Bool update)
{
   _broadcast(ipc, 3 /* desktop cache update */, update, NULL, 0);
}

void
send_signal_desktop_cache_build(void)
{
   _broadcast(ipc, 1 /* desktop cache build */, 1, NULL, 0);
}

void
send_signal_mime_cache_build(void)
{
   _broadcast(ipc, 4 /* mime cache build */, 1, NULL, 0);
}

Eina_Bool
ipc_init(void)
{
   if (init > 0) return EINA_TRUE;
   if (!ecore_ipc_init()) return EINA_FALSE;
   ipc = ecore_ipc_server_add(ECORE_IPC_LOCAL_USER, "efreetd", 0, NULL);
   if (!ipc)
     {
        ecore_ipc_shutdown();
        return EINA_FALSE;
     }
   quit_timer_start = ecore_timer_add(10.0, _cb_quit_timer_start, NULL);
   hnd_add = ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_ADD,
                                     _cb_client_add, NULL);
   hnd_del = ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DEL,
                                     _cb_client_del, NULL);
   hnd_data = ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DATA,
                                      _cb_client_data, NULL);
   init++;
   return EINA_TRUE;
}

Eina_Bool
ipc_shutdown(void)
{
   if (init <= 0) return EINA_TRUE;
   init--;
   if (init > 0) return EINA_TRUE;
   if (quit_timer) ecore_timer_del(quit_timer);
   if (quit_timer_start) ecore_timer_del(quit_timer_start);
   quit_timer = NULL;
   quit_timer_start = NULL;
   ecore_ipc_server_del(ipc);
   ecore_event_handler_del(hnd_add);
   ecore_event_handler_del(hnd_del);
   ecore_event_handler_del(hnd_data);
   ipc = NULL;
   hnd_add = NULL;
   hnd_del = NULL;
   hnd_data = NULL;
   ecore_ipc_shutdown();
   return EINA_TRUE;
}

