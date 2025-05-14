#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "Eina.h"

/**
 * @file
 * @brief This file implements a helper process for EFL applications to query
 *        proxy settings using libproxy. It communicates with the main
 *        application via stdin/stdout.
 */

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(EINA_LOG_DOMAIN_GLOBAL, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(EINA_LOG_DOMAIN_GLOBAL, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(EINA_LOG_DOMAIN_GLOBAL, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(EINA_LOG_DOMAIN_GLOBAL, __VA_ARGS__)

#ifdef CRI
# undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(EINA_LOG_DOMAIN_GLOBAL, __VA_ARGS__)

typedef struct pxProxyFactory_  pxProxyFactory; /**< Opaque type for libproxy's proxy factory. */

/**
 * @brief Structure to hold libproxy related data and function pointers.
 */
typedef struct _Libproxy
{
   pxProxyFactory    *factory; /**< Pointer to the libproxy factory instance. */
   char           **(*px_proxy_factory_get_proxies) (pxProxyFactory *factory, const char *url); /**< Function pointer to get proxies for a URL. */
   void            *(*px_proxy_factory_new)         (void); /**< Function pointer to create a new proxy factory. */
   void             (*px_proxy_factory_free)        (pxProxyFactory *); /**< Function pointer to free a proxy factory. */
   Eina_Module       *mod; /**< Eina_Module handle for the loaded libproxy library. */
} Libproxy;

static Libproxy _libproxy = { 0 }; /**< Global instance of the Libproxy structure. */

static Eina_Spinlock pending_lock; /**< Spinlock to protect access to pending and opcount. */
static int pending = 0; /**< Counter for the number of currently active proxy lookup threads. */
static int opcount = 0; /**< Counter for the total number of proxy lookup operations initiated. */
static Eina_List *join_list = NULL; /**< List of threads that have finished their work and are waiting to be joined. */

/**
 * @brief Initializes libproxy by loading the library and its symbols.
 *
 * This function attempts to load the libproxy shared library (e.g., libproxy.so.1)
 * and resolve necessary function symbols (px_proxy_factory_new,
 * px_proxy_factory_free, px_proxy_factory_get_proxies).
 * If successful, it creates a new proxy factory.
 *
 * @return @c EINA_TRUE on successful initialization, @c EINA_FALSE otherwise.
 */
static Eina_Bool
init(void)
{
   if (!_libproxy.mod)
     {
#define LOAD(x) \
   if (!_libproxy.mod) { \
      _libproxy.mod = eina_module_new(x); \
      if (_libproxy.mod) { \
         if (!eina_module_load(_libproxy.mod)) { \
            eina_module_free(_libproxy.mod); \
            _libproxy.mod = NULL; \
         } \
      } \
   }
#if defined(_WIN32) || defined(__CYGWIN__)
        LOAD("libproxy-1.dll");
        LOAD("libproxy.dll");
#elif defined(__APPLE__) && defined(__MACH__)
        LOAD("libproxy.1.dylib");
        LOAD("libproxy.dylib");
#else
        LOAD("libproxy.so.1");
        LOAD("libproxy.so");
#endif
#undef LOAD
        if (!_libproxy.mod)
          {
             DBG("Couldn't find libproxy in your system. Continue without it");
             return EINA_FALSE;
          }

#define SYM(x) \
   if ((_libproxy.x = eina_module_symbol_get(_libproxy.mod, #x)) == NULL) { \
      ERR("libproxy (%s) missing symbol %s", \
          eina_module_file_get(_libproxy.mod), #x); \
      eina_module_free(_libproxy.mod); \
      _libproxy.mod = NULL; \
      return EINA_FALSE; \
   }

        SYM(px_proxy_factory_new);
        SYM(px_proxy_factory_free);
        SYM(px_proxy_factory_get_proxies);
#undef SYM
        DBG("using libproxy=%s", eina_module_file_get(_libproxy.mod));
     }

   if (!_libproxy.factory)
     _libproxy.factory = _libproxy.px_proxy_factory_new();

   return !!_libproxy.factory;
}

/**
 * @brief Shuts down libproxy by freeing the factory and unloading the module.
 *
 * This function frees the libproxy factory if it exists and then unloads
 * the libproxy shared library module.
 */
static void
shutdown(void)
{
   if (_libproxy.factory)
     {
        _libproxy.px_proxy_factory_free(_libproxy.factory);
        _libproxy.factory = NULL;
     }
   if (_libproxy.mod)
     {
        eina_module_free(_libproxy.mod);
        _libproxy.mod = NULL;
     }
}

/**
 * @brief Thread function to perform a proxy lookup for a given URL.
 *
 * This function is executed in a separate thread. It parses the command,
 * calls libproxy to get the proxies for the URL, and prints the results
 * to stdout.
 * The format of the output for each proxy is "P <id> P <proxy_string>\n".
 * After all proxies are printed, it prints "P <id> E\n" to signify the end.
 *
 * Example `cmd` (data): "P 1234 http://example.com"
 * Example output:
 * P 1234 P direct://
 * P 1234 P http://proxy.example.com:8080
 * P 1234 E
 *
 * @param data A string containing the command. The format is "P <id> <URL>".
 *             The function takes ownership of this string and frees it.
 * @param t The Eina_Thread handle of the current thread. Used for potential
 *          re-queuing if the thread pool is busy.
 * @return NULL.
 */
static void *
proxy_lookup(void *data, Eina_Thread t)
{
   char *cmd = data;
   char **proxies, **itr;
   const char *p, *url;
   int id = atoi(cmd + 2);
   int pending_local, opcount_prev;

   if (id > 0)
     {
        for (p = cmd + 2; *p && (*p != ' '); p++);
        if (*p == ' ')
          {
             url = p + 1;
             proxies = _libproxy.px_proxy_factory_get_proxies
               (_libproxy.factory, url);
             if (proxies)
               {
                  for (itr = proxies; *itr != NULL; itr++)
                    {
                       fprintf(stdout, "P %i P %s\n", id, *itr);
                       free(*itr);
                    }
                  free(proxies);
               }
             fprintf(stdout, "P %i E\n", id);
             fflush(stdout);
          }
     }
   free(cmd);

   eina_spinlock_take(&pending_lock);
     {
        pending--;
        pending_local = pending;
        opcount_prev = opcount;
     }
   eina_spinlock_release(&pending_lock);
   // if there are no more pending threads doing work - sleep for the
   // timeout then check if we still are and if so - exit;
   if (pending_local == 0) sleep(10);
   eina_spinlock_take(&pending_lock);
     {
        Eina_Thread *tt;

        if ((pending == 0) & (opcount == opcount_prev)) exit(0);
        tt = calloc(1, sizeof(Eina_Thread));
        if (tt)
          {
             *tt = t;
             join_list = eina_list_append(join_list, tt);
          }
     }
   eina_spinlock_release(&pending_lock);
   return NULL;
}

/**
 * @brief Handles a command received from stdin.
 *
 * Currently, it only supports the 'P' command for proxy lookup.
 * "P <id> <URL>" - Initiates a proxy lookup for the given URL with a specific ID.
 * The lookup is performed in a new background thread.
 *
 * @param cmd The command string received from stdin.
 *            Example: "P 1234 http://www.example.com"
 */
static void
handle(const char *cmd)
{
   // "P 1234 URL" -> Get Proxy, id=1234, url=URL
   if ((cmd[0] == 'P') && (cmd[1] == ' '))
     {
        char *dup = strdup(cmd);

        if (dup)
          {
             Eina_Thread t;

             eina_spinlock_take(&pending_lock);
               {
                  pending++;
                  opcount++;
               }
             eina_spinlock_release(&pending_lock);
             if (!eina_thread_create(&t, EINA_THREAD_BACKGROUND, -1,
                                     proxy_lookup, dup))
               {
                  abort();
               }
          }
        return;
     }
}

/**
 * @brief Joins completed threads.
 *
 * This function iterates through the `join_list` (populated by
 * `proxy_lookup` when threads finish) and joins each thread.
 * This is necessary to clean up thread resources.
 * It is called after handling each command in the main loop.
 */
static void
clean_threads(void)
{
   eina_spinlock_take(&pending_lock);
     {
        Eina_Thread *t;

        EINA_LIST_FREE(join_list, t)
          {
             eina_thread_join(*t);
             free(t);
          }
     }
   eina_spinlock_release(&pending_lock);
}

/**
 * @brief Main function of the efl_net_proxy_helper.
 *
 * Initializes Eina and libproxy. Then enters a loop, reading commands
 * from stdin, one per line. Each command is processed by `handle()`.
 * After processing each command, `clean_threads()` is called to join
 * any completed worker threads.
 * If libproxy initialization fails, it prints "F\n" to stdout and enters
 * an infinite sleep loop.
 *
 * Input commands from stdin:
 *   - "P <id> <URL>": Request proxy information for the given URL.
 *     Example: "P 1 http://example.com"
 *
 * Output to stdout:
 *   - "P <id> P <proxy_info>": A proxy found for the request.
 *     Example: "P 1 P direct://"
 *              "P 1 P socks5://localhost:1080"
 *   - "P <id> E": End of proxy information for the request.
 *     Example: "P 1 E"
 *   - "F\n": Sent if libproxy initialization fails.
 *
 * @param argc Argument count (unused).
 * @param argv Argument vector (unused).
 * @return 0 on successful completion, though it typically runs indefinitely
 *         or exits via `exit(0)` in `proxy_lookup` under certain conditions.
 */
int
main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   char inbuf[8192];
   eina_init();
   if (init())
     {
        eina_spinlock_new(&pending_lock);
        // 1 command per stdin line
        while (fgets(inbuf, sizeof(inbuf) - 1, stdin))
          {
             // strip off newline and ensure the string is 0 terminated
             int len = strlen(inbuf);
             if (len > 0)
               {
                  if (inbuf[len -1 ] == '\n') inbuf[len - 1] = 0;
                  else inbuf[len] = 0;
                  handle(inbuf);
               }
             clean_threads();
          }
        eina_spinlock_free(&pending_lock);
        shutdown();
     }
   else
     {
        // Failed to init libproxy so report this before exit
        fprintf(stdout, "F\n");
        fflush(stdout);
        for (;;) sleep(60 * 60 * 24);
     }
   eina_shutdown();
   return 0;
}
