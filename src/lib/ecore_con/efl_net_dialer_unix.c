#define EFL_NET_DIALER_UNIX_PROTECTED 1
#define EFL_NET_DIALER_PROTECTED 1
#define EFL_NET_SOCKET_FD_PROTECTED 1
#define EFL_NET_SOCKET_PROTECTED 1
#define EFL_IO_READER_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#define MY_CLASS EFL_NET_DIALER_UNIX_CLASS

/**
 * @brief Private data for the Efl_Net_Dialer_Unix class.
 */
typedef struct _Efl_Net_Dialer_Unix_Data
{
   struct {
      Ecore_Thread *thread; /**< Thread used for asynchronous connection attempts. */
      Eina_Future *timeout; /**< Future for handling connection timeouts. */
   } connect; /**< Connection-related data. */
   Eina_Stringshare *address_dial; /**< The Unix domain socket address to dial. Can be a path or an abstract address prefixed with "abstract:". */
   Eina_Bool connected; /**< Flag indicating if the dialer is currently connected. */
   double timeout_dial; /**< Timeout in seconds for the dial operation. */
} Efl_Net_Dialer_Unix_Data;

EOLIAN static Eo*
_efl_net_dialer_unix_efl_object_constructor(Eo *o, Efl_Net_Dialer_Unix_Data *pd EINA_UNUSED)
{
   o = efl_constructor(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   efl_net_dialer_timeout_dial_set(o, 30.0);
   return o;
}

/**
 * @brief Handles object invalidation.
 *
 * Cleans up resources, such as closing the connection if configured to do so
 * and cancelling any ongoing connection threads.
 */
EOLIAN static void
_efl_net_dialer_unix_efl_object_invalidate(Eo *o, Efl_Net_Dialer_Unix_Data *pd)
{
   if (efl_io_closer_close_on_invalidate_get(o) &&
       (!efl_io_closer_closed_get(o)))
     {
        efl_event_freeze(o);
        efl_io_closer_close(o);
        efl_event_thaw(o);
     }

   if (pd->connect.thread)
     {
        ecore_thread_cancel(pd->connect.thread);
        pd->connect.thread = NULL;
     }

   efl_invalidate(efl_super(o, MY_CLASS));
}

/**
 * @brief Handles object destruction.
 *
 * Frees allocated resources, such as the stored dial address.
 */
EOLIAN static void
_efl_net_dialer_unix_efl_object_destructor(Eo *o, Efl_Net_Dialer_Unix_Data *pd)
{
   efl_destructor(efl_super(o, MY_CLASS));

   eina_stringshare_replace(&pd->address_dial, NULL);
}

/**
 * @brief Callback function executed when a connection attempt times out.
 *
 * This function is triggered by an Eina_Future. It cancels any ongoing
 * connection thread, sets the End-Of-Stream (EOS) flag, and emits a
 * 'dialer,error' event with ETIMEDOUT.
 *
 * @param o The Efl_Net_Dialer_Unix object.
 * @param data User data (unused in this context).
 * @param v The Eina_Value associated with the future (unused in this context).
 * @return The input Eina_Value v.
 */
static Eina_Value
_efl_net_dialer_unix_connect_timeout(Eo *o, void *data EINA_UNUSED, const Eina_Value v)
{
   Efl_Net_Dialer_Unix_Data *pd = efl_data_scope_get(o, MY_CLASS);
   Eina_Error err = ETIMEDOUT;

   if (pd->connect.thread)
     {
        ecore_thread_cancel(pd->connect.thread);
        pd->connect.thread = NULL;
     }

   efl_ref(o);
   efl_io_reader_eos_set(o, EINA_TRUE);
   efl_event_callback_call(o, EFL_NET_DIALER_EVENT_DIALER_ERROR, &err);
   efl_unref(o);
   return v;
}

/**
 * @brief Callback function executed when an asynchronous connection attempt completes.
 *
 * This function is called by the efl_net_connect_async_new thread.
 * It handles both successful connections and errors. On success, it sets up
 * the socket, emits 'dialer,resolved' and 'dialer,connected' events.
 * On error, it cleans up and emits a 'dialer,error' event.
 *
 * @param data The Efl_Net_Dialer_Unix object, passed as user data.
 * @param addr The socket address of the connected peer.
 * @param addrlen The length of the socket address.
 * @param sockfd The file descriptor of the connected socket.
 * @param err An Eina_Error code indicating the result of the connection attempt.
 *            0 on success, or an error code otherwise.
 */
static void
_efl_net_dialer_unix_connected(void *data, const struct sockaddr *addr, socklen_t addrlen EINA_UNUSED, SOCKET sockfd, Eina_Error err)
{
   Eo *o = data;
   Efl_Net_Dialer_Unix_Data *pd = efl_data_scope_get(o, MY_CLASS);
   if (!pd) return;

   pd->connect.thread = NULL;

   efl_ref(o); /* we're emitting callbacks then continuing the workflow */

   if (err) goto error;

   efl_net_socket_fd_family_set(o, addr->sa_family);
   efl_loop_fd_set(o, sockfd);
   if (efl_net_socket_address_remote_get(o))
     {
        efl_event_callback_call(o, EFL_NET_DIALER_EVENT_DIALER_RESOLVED, NULL);
        efl_net_dialer_connected_set(o, EINA_TRUE);
     }
   else
     {
        err = EFL_NET_DIALER_ERROR_COULDNT_CONNECT;
        efl_loop_fd_set(o, SOCKET_TO_LOOP_FD(INVALID_SOCKET));
        closesocket(sockfd);
        goto error;
     }

 error:
   if (err)
     {
        efl_io_reader_eos_set(o, EINA_TRUE);
        efl_event_callback_call(o, EFL_NET_DIALER_EVENT_DIALER_ERROR, &err);
     }

   efl_unref(o);
}

/**
 * @brief Schedules a connection timeout.
 *
 * If a timeout value is set (pd->timeout_dial > 0), this function
 * creates an Eina_Future that will trigger the
 * _efl_net_dialer_unix_connect_timeout callback after the specified delay.
 *
 * @param o The Efl_Net_Dialer_Unix object.
 * @param pd The private data for the object.
 */
static void
_timeout_schedule(Eo *o, Efl_Net_Dialer_Unix_Data *pd)
{
   efl_future_then(o, efl_loop_timeout(efl_loop_get(o), pd->timeout_dial),
                   .success = _efl_net_dialer_unix_connect_timeout,
                   .storage = &pd->connect.timeout);
}

/**
 * @brief Initiates a connection to a Unix domain socket.
 *
 * This function attempts to connect to the specified Unix domain socket address.
 * The address can be a filesystem path or an abstract namespace path
 * (prefixed with "abstract:").
 * The connection is performed asynchronously.
 *
 * @param o The Efl_Net_Dialer_Unix object.
 * @param pd The private data for the object.
 * @param address The Unix domain socket address to connect to.
 *                Examples: "/tmp/mysocket", "abstract:my_abstract_socket".
 * @return 0 on success, or an Eina_Error code on failure.
 *         Possible errors include EINVAL for invalid arguments,
 *         EISCONN if already connected, EBADF if closed,
 *         EALREADY if a connection is in progress, or
 *         EFL_NET_ERROR_COULDNT_RESOLVE_HOST if the path is too long.
 */
EOLIAN static Eina_Error
_efl_net_dialer_unix_efl_net_dialer_dial(Eo *o, Efl_Net_Dialer_Unix_Data *pd, const char *address)
{
   struct sockaddr_un addr = { .sun_family = AF_UNIX };
   socklen_t addrlen;

   EINA_SAFETY_ON_NULL_RETURN_VAL(address, EINVAL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(address[0] == '\0', EINVAL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_net_dialer_connected_get(o), EISCONN);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_io_closer_closed_get(o), EBADF);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_loop_fd_get(o) >= 0, EALREADY);

   if (pd->connect.thread)
     {
        ecore_thread_cancel(pd->connect.thread);
        pd->connect.thread = NULL;
     }

   if (strncmp(address, "abstract:", strlen("abstract:")) == 0)
     {
        const char *path = address + strlen("abstract:");
        if (strlen(path) + 2 > sizeof(addr.sun_path))
          {
             ERR("abstract path is too long: %s", path);
             return EFL_NET_ERROR_COULDNT_RESOLVE_HOST;
          }
        addr.sun_path[0] = '\0';
        memcpy(addr.sun_path + 1, path, strlen(path) + 1);
        addrlen = strlen(path) + 2 + offsetof(struct sockaddr_un, sun_path);
     }
   else
     {
        const char *path = address;
        if (strlen(path) + 1 > sizeof(addr.sun_path))
          {
             ERR("path is too long: %s", path);
             return EFL_NET_ERROR_COULDNT_RESOLVE_HOST;
          }
        memcpy(addr.sun_path, path, strlen(path) + 1);
        addrlen = strlen(path) + 1 + offsetof(struct sockaddr_un, sun_path);
     }

   pd->connect.thread = efl_net_connect_async_new((const struct sockaddr *)&addr, addrlen, SOCK_STREAM, 0,
                                                  efl_io_closer_close_on_exec_get(o),
                                                  _efl_net_dialer_unix_connected, o);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->connect.thread, EINVAL);

   efl_net_dialer_address_dial_set(o, address);

   if (pd->connect.timeout) eina_future_cancel(pd->connect.timeout);
   if (pd->timeout_dial > 0.0) _timeout_schedule(o, pd);

   return 0;
}

/**
 * @brief Sets the Unix domain socket address to be dialed.
 *
 * @param o The Efl_Net_Dialer_Unix object (unused).
 * @param pd The private data for the object.
 * @param address The Unix domain socket address.
 *                Example: "/tmp/mysocket" or "abstract:my_abstract_socket".
 */
EOLIAN static void
_efl_net_dialer_unix_efl_net_dialer_address_dial_set(Eo *o EINA_UNUSED, Efl_Net_Dialer_Unix_Data *pd, const char *address)
{
   eina_stringshare_replace(&pd->address_dial, address);
}

/**
 * @brief Gets the Unix domain socket address to be dialed.
 *
 * @param o The Efl_Net_Dialer_Unix object (unused).
 * @param pd The private data for the object.
 * @return The currently set Unix domain socket address.
 */
EOLIAN static const char *
_efl_net_dialer_unix_efl_net_dialer_address_dial_get(const Eo *o EINA_UNUSED, Efl_Net_Dialer_Unix_Data *pd)
{
   return pd->address_dial;
}

/**
 * @brief Sets the timeout for the dial operation.
 *
 * @param o The Efl_Net_Dialer_Unix object.
 * @param pd The private data for the object.
 * @param seconds The timeout duration in seconds. A value of 0.0 or less
 *                disables the timeout.
 */
EOLIAN static void
_efl_net_dialer_unix_efl_net_dialer_timeout_dial_set(Eo *o, Efl_Net_Dialer_Unix_Data *pd, double seconds)
{
   pd->timeout_dial = seconds;

   if (pd->connect.timeout) eina_future_cancel(pd->connect.timeout);
   if ((pd->timeout_dial > 0.0) && (pd->connect.thread)) _timeout_schedule(o, pd);
}

/**
 * @brief Gets the timeout for the dial operation.
 *
 * @param o The Efl_Net_Dialer_Unix object (unused).
 * @param pd The private data for the object.
 * @return The timeout duration in seconds.
 */
EOLIAN static double
_efl_net_dialer_unix_efl_net_dialer_timeout_dial_get(const Eo *o EINA_UNUSED, Efl_Net_Dialer_Unix_Data *pd)
{
   return pd->timeout_dial;
}

/**
 * @brief Sets the connected state of the dialer.
 *
 * This function updates the internal connected flag and emits the
 * 'dialer,connected' event if the state changes to connected.
 * It also cancels any pending connection timeout.
 *
 * @param o The Efl_Net_Dialer_Unix object.
 * @param pd The private data for the object.
 * @param connected EINA_TRUE if connected, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_net_dialer_unix_efl_net_dialer_connected_set(Eo *o, Efl_Net_Dialer_Unix_Data *pd, Eina_Bool connected)
{
   if (pd->connect.timeout) eina_future_cancel(pd->connect.timeout);
   if (pd->connected == connected) return;
   pd->connected = connected;
   if (connected) efl_event_callback_call(o, EFL_NET_DIALER_EVENT_DIALER_CONNECTED, NULL);
}

/**
 * @brief Gets the connected state of the dialer.
 *
 * @param o The Efl_Net_Dialer_Unix object (unused).
 * @param pd The private data for the object.
 * @return EINA_TRUE if connected, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_dialer_unix_efl_net_dialer_connected_get(const Eo *o EINA_UNUSED, Efl_Net_Dialer_Unix_Data *pd)
{
   return pd->connected;
}

/**
 * @brief Closes the connection.
 *
 * This function sets the connected state to EINA_FALSE and then calls the
 * parent class's close method.
 *
 * @param o The Efl_Net_Dialer_Unix object.
 * @param pd The private data for the object (unused).
 * @return 0 on success, or an Eina_Error code on failure.
 */
EOLIAN static Eina_Error
_efl_net_dialer_unix_efl_io_closer_close(Eo *o, Efl_Net_Dialer_Unix_Data *pd EINA_UNUSED)
{
   efl_net_dialer_connected_set(o, EINA_FALSE);
   return efl_io_closer_close(efl_super(o, MY_CLASS));
}

#include "efl_net_dialer_unix.eo.c"
