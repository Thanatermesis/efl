#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef _WIN32
# include <ws2tcpip.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include "Ecore.h"
#include "ecore_private.h"

#ifdef HAVE_GLIB
# include <glib.h>

static Eina_Bool _ecore_glib_active = EINA_FALSE; /**< Tracks if GLib integration is active */
static Ecore_Select_Function _ecore_glib_select_original; /**< Stores the original Ecore select function */
static GPollFD *_ecore_glib_fds = NULL; /**< Array of GLib poll file descriptors */
static size_t _ecore_glib_fds_size = 0; /**< Current allocated size of _ecore_glib_fds array */
static const size_t ECORE_GLIB_FDS_INITIAL = 128; /**< Initial size for _ecore_glib_fds */
static const size_t ECORE_GLIB_FDS_STEP = 8; /**< Step size for increasing _ecore_glib_fds */
static const size_t ECORE_GLIB_FDS_MAX_FREE = 256; /**< Maximum free slots before shrinking _ecore_glib_fds */
#if GLIB_CHECK_VERSION(2,32,0)
static GRecMutex *_ecore_glib_select_lock; /**< Mutex to protect select operations in GLib >= 2.32 */
#else
static GStaticRecMutex *_ecore_glib_select_lock; /**< Mutex to protect select operations in GLib < 2.32 */
#endif

/**
 * @brief Resizes the internal array of GLib poll file descriptors.
 *
 * @param size The new desired size of the array.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., realloc failed).
 */
static Eina_Bool
_ecore_glib_fds_resize(size_t size)
{
   void *tmp = realloc(_ecore_glib_fds, sizeof(GPollFD) * size);

   if (!tmp)
     {
        ERR("Could not realloc from %zu to %zu buckets.",
            _ecore_glib_fds_size, size);
        return EINA_FALSE;
     }

   _ecore_glib_fds = tmp;
   _ecore_glib_fds_size = size;
   return EINA_TRUE;
}

/**
 * @brief Queries the GLib main context for file descriptors and timeout.
 *
 * This function wraps g_main_context_query, handling resizing of the
 * internal _ecore_glib_fds array as needed. It also implements a strategy
 * to shrink the array if it becomes too large compared to the required
 * number of file descriptors.
 *
 * @param ctx The GLib main context to query.
 * @param priority The maximum priority of sources to check.
 * @param p_timer Pointer to an integer where the timeout value (in milliseconds)
 *                will be stored.
 * @return The number of file descriptors ready, or -1 on error (e.g., resize failed).
 */
static int
_ecore_glib_context_query(GMainContext *ctx,
                          int           priority,
                          int          *p_timer)
{
   int reqfds;

   if (_ecore_glib_fds_size == 0)
     {
        if (!_ecore_glib_fds_resize(ECORE_GLIB_FDS_INITIAL)) return -1;
     }

   while (1)
     {
        size_t size;

        reqfds = g_main_context_query
            (ctx, priority, p_timer, _ecore_glib_fds, _ecore_glib_fds_size);
        if (reqfds <= (int)_ecore_glib_fds_size) break;

        size = (1 + reqfds / ECORE_GLIB_FDS_STEP) * ECORE_GLIB_FDS_STEP;
        if (!_ecore_glib_fds_resize(size)) return -1;
     }

   if (reqfds + ECORE_GLIB_FDS_MAX_FREE < _ecore_glib_fds_size)
     {
        size_t size;

        size = (1 + reqfds / ECORE_GLIB_FDS_MAX_FREE) * ECORE_GLIB_FDS_MAX_FREE;
        _ecore_glib_fds_resize(size);
     }

   return reqfds;
}

/**
 * @brief Populates Ecore's fd_sets based on GLib's GPollFD array.
 *
 * This function iterates through the GPollFD array (populated by
 * g_main_context_query) and sets the corresponding file descriptors
 * in Ecore's read, write, and error fd_sets.
 *
 * @param pfds Array of GPollFD structures from GLib.
 * @param count The number of elements in the pfds array.
 * @param rfds Pointer to Ecore's read file descriptor set.
 * @param wfds Pointer to Ecore's write file descriptor set.
 * @param efds Pointer to Ecore's error file descriptor set.
 * @return The highest file descriptor number encountered plus one,
 *         suitable for use as the first argument to select().
 */
static int
_ecore_glib_context_poll_from(const GPollFD *pfds,
                              int            count,
                              fd_set        *rfds,
                              fd_set        *wfds,
                              fd_set        *efds)
{
   const GPollFD *itr = pfds, *itr_end = pfds + count;
   int glib_fds = -1;

   for (; itr < itr_end; itr++)
     {
        if (glib_fds < itr->fd)
          glib_fds = itr->fd;

        if (itr->events & G_IO_IN)
          FD_SET(itr->fd, rfds);
        if (itr->events & G_IO_OUT)
          FD_SET(itr->fd, wfds);
        if (itr->events & (G_IO_HUP | G_IO_ERR))
          FD_SET(itr->fd, efds);
     }

   return glib_fds + 1;
}

/**
 * @brief Updates GLib's GPollFD array based on the results from select().
 *
 * This function iterates through the GPollFD array and sets the `revents`
 * field for each descriptor based on whether it's present in Ecore's
 * read, write, or error fd_sets after the select() call.
 * It also performs a check for sockets that might have been closed by the
 * peer, marking them with G_IO_ERR if getpeername fails.
 *
 * @param pfds Array of GPollFD structures to update.
 * @param count The number of elements in the pfds array.
 * @param rfds Pointer to Ecore's read file descriptor set (after select).
 * @param wfds Pointer to Ecore's write file descriptor set (after select).
 * @param efds Pointer to Ecore's error file descriptor set (after select).
 * @param ready The number of file descriptors reported ready by select().
 * @return The number of remaining ready file descriptors not consumed by GLib.
 */
static int
_ecore_glib_context_poll_to(GPollFD      *pfds,
                            int           count,
                            const fd_set *rfds,
                            const fd_set *wfds,
                            const fd_set *efds,
                            int           ready)
{
   GPollFD *itr = pfds, *itr_end = pfds + count;
   struct stat st;

   for (; (itr < itr_end) && (ready > 0); itr++)
     {
        itr->revents = 0;
        if (FD_ISSET(itr->fd, rfds) && (itr->events & G_IO_IN))
          {
             itr->revents |= G_IO_IN;
             ready--;
          }
        if (FD_ISSET(itr->fd, wfds) && (itr->events & G_IO_OUT))
          {
             itr->revents |= G_IO_OUT;
             ready--;
             if (!fstat(itr->fd, &st))
               {
                  if (S_ISSOCK(st.st_mode))
                    {
                       struct sockaddr_in peer;
                       socklen_t length = sizeof(peer);

                       memset(&peer, 0, sizeof(peer));
                       if (getpeername(itr->fd, (struct sockaddr *)&peer,
                                       &length))
                         itr->revents |= G_IO_ERR;
                    }
               }
          }
        if (FD_ISSET(itr->fd, efds) && (itr->events & (G_IO_HUP | G_IO_ERR)))
          {
             itr->revents |= G_IO_ERR;
             ready--;
          }
     }
   return ready;
}

/**
 * @brief Core select logic, integrating Ecore and GLib event sources.
 *
 * This function is called with the _ecore_glib_select_lock held.
 * It prepares the GLib main context, queries it for FDs and timeout,
 * merges GLib FDs into Ecore's fd_sets, determines the overall timeout,
 * calls the original Ecore select function, updates GLib's GPollFDs
 * with the results, and dispatches any pending GLib events.
 *
 * @param ctx The GLib main context.
 * @param ecore_fds The highest file descriptor number from Ecore sources.
 * @param rfds Pointer to Ecore's read file descriptor set.
 * @param wfds Pointer to Ecore's write file descriptor set.
 * @param efds Pointer to Ecore's error file descriptor set.
 * @param ecore_timeout The timeout requested by Ecore.
 * @return The number of file descriptors ready, or -1 on error.
 */
static int
_ecore_glib_select__locked(GMainContext   *ctx,
                           int             ecore_fds,
                           fd_set         *rfds,
                           fd_set         *wfds,
                           fd_set         *efds,
                           struct timeval *ecore_timeout)
{
   int priority, maxfds, glib_fds, reqfds, reqtimeout, ret;
   struct timeval *timeout, glib_timeout;

   g_main_context_prepare(ctx, &priority);
   reqfds = _ecore_glib_context_query(ctx, priority, &reqtimeout);
   if (reqfds < 0) goto error;

   glib_fds = _ecore_glib_context_poll_from
       (_ecore_glib_fds, reqfds, rfds, wfds, efds);

   if (reqtimeout == -1)
     timeout = ecore_timeout;
   else
     {
        glib_timeout.tv_sec = reqtimeout / 1000;
        glib_timeout.tv_usec = (reqtimeout % 1000) * 1000;

        if (!ecore_timeout || timercmp(ecore_timeout, &glib_timeout, >))
          timeout = &glib_timeout;
        else
          timeout = ecore_timeout;
     }

   maxfds = (ecore_fds >= glib_fds) ? ecore_fds : glib_fds;
   ret = _ecore_glib_select_original(maxfds, rfds, wfds, efds, timeout);

   ret = _ecore_glib_context_poll_to
       (_ecore_glib_fds, reqfds, rfds, wfds, efds, ret);

   if (g_main_context_check(ctx, priority, _ecore_glib_fds, reqfds))
     g_main_context_dispatch(ctx);

   return ret;

error:
   return _ecore_glib_select_original
            (ecore_fds, rfds, wfds, efds, ecore_timeout);
}

static int
_ecore_glib_select(int             ecore_fds,
                   fd_set         *rfds,
                   fd_set         *wfds,
                   fd_set         *efds,
                   struct timeval *ecore_timeout)
{
   GMainContext *ctx;
   int ret;

   ctx = g_main_context_default(); // Get the default GLib main context.

   while (!g_main_context_acquire(ctx))
     g_thread_yield();

#if GLIB_CHECK_VERSION(2,32,0)
   g_rec_mutex_lock(_ecore_glib_select_lock);
#else
   g_static_rec_mutex_lock(_ecore_glib_select_lock);
#endif

   ret = _ecore_glib_select__locked
       (ctx, ecore_fds, rfds, wfds, efds, ecore_timeout);

#if GLIB_CHECK_VERSION(2,32,0)
   g_rec_mutex_unlock(_ecore_glib_select_lock);
#else
   g_static_rec_mutex_unlock(_ecore_glib_select_lock);
#endif
   g_main_context_release(ctx);

   return ret;
}

#endif

/**
 * @internal
 * @brief Initializes GLib integration specific resources.
 *
 * This function initializes the mutex used for synchronizing access to
 * GLib's main context operations. It handles different GLib versions
 * for mutex initialization. This is typically called when GLib integration
 * is first activated.
 */
void
_ecore_glib_init(void)
{
#ifdef HAVE_GLIB
#if GLIB_CHECK_VERSION(2,32,0)
   _ecore_glib_select_lock = malloc(sizeof(GRecMutex));
   g_rec_mutex_init(_ecore_glib_select_lock);
#else
   if (!g_thread_get_initialized()) g_thread_init(NULL);
   _ecore_glib_select_lock = malloc(sizeof(GStaticRecMutex));
   g_static_rec_mutex_init(_ecore_glib_select_lock);
#endif
#endif
}

/**
 * @internal
 * @brief Shuts down GLib integration and cleans up resources.
 *
 * This function deactivates GLib integration, restores the original Ecore
 * select function, frees allocated memory for GLib poll file descriptors,
 * and cleans up the synchronization mutex.
 */
void
_ecore_glib_shutdown(void)
{
#ifdef HAVE_GLIB
   if (!_ecore_glib_active) return;
   _ecore_glib_active = EINA_FALSE;

   if (ecore_main_loop_select_func_get() == _ecore_glib_select)
     ecore_main_loop_select_func_set(_ecore_glib_select_original);

   if (_ecore_glib_fds)
     {
        free(_ecore_glib_fds);
        _ecore_glib_fds = NULL;
     }
   _ecore_glib_fds_size = 0;

#if GLIB_CHECK_VERSION(2,32,0)
   g_rec_mutex_clear(_ecore_glib_select_lock);
   free(_ecore_glib_select_lock);
   _ecore_glib_select_lock = NULL;
#else
   g_static_rec_mutex_free(_ecore_glib_select_lock);
   _ecore_glib_select_lock = NULL;
#endif
#endif
}

/**
 * @brief Integrates the GLib main loop with the Ecore main loop.
 *
 * After calling this function, Ecore's main loop will also process
 * GLib events. This is achieved by replacing Ecore's default select
 * function with a wrapper (`_ecore_glib_select`) that polls both
 * Ecore and GLib event sources.
 *
 * @return EINA_TRUE if integration was successful or already active.
 *         EINA_FALSE if GLib support is not compiled in.
 * @see ecore_main_loop_select_func_set()
 * @see _ecore_glib_select()
 */
EAPI Eina_Bool
ecore_main_loop_glib_integrate(void)
{
#ifdef HAVE_GLIB
   void *func;

   if (_ecore_glib_active) return EINA_TRUE; // Already active
   func = ecore_main_loop_select_func_get();
   if (func == _ecore_glib_select) return EINA_TRUE; // Already integrated (e.g. by another call)

   _ecore_glib_select_original = func; // Store the original select function
   ecore_main_loop_select_func_set(_ecore_glib_select); // Set our wrapper
   _ecore_glib_active = EINA_TRUE;

   /* Init GLib specific parts only when integration is explicitly requested */
   _ecore_glib_init();
   return EINA_TRUE;
#else
   ERR("No glib support");
   return EINA_FALSE;
#endif
}

/**
 * @brief Flag to control automatic GLib integration at Ecore initialization.
 * @since 1.2
 *
 * If true (default), Ecore will attempt to integrate with GLib automatically
 * during its initialization if GLib is present and Ecore was compiled with
 * GLib support. Setting this to false via
 * ecore_main_loop_glib_always_integrate_disable() prevents this automatic
 * integration, requiring an explicit call to ecore_main_loop_glib_integrate().
 */
Eina_Bool _ecore_glib_always_integrate = 1;

/**
 * @brief Disables the automatic integration of the GLib main loop at Ecore initialization.
 * @since 1.2
 *
 * By default, Ecore attempts to integrate with GLib automatically if available.
 * Calling this function prevents that automatic integration, allowing for manual
 * control via ecore_main_loop_glib_integrate().
 *
 * @see ecore_main_loop_glib_integrate()
 * @see _ecore_glib_always_integrate
 */
EAPI void
ecore_main_loop_glib_always_integrate_disable(void)
{
   _ecore_glib_always_integrate = 0;
}
