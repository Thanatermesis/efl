#define EFL_NET_DIALER_SSL_PROTECTED 1
#define EFL_NET_SOCKET_SSL_PROTECTED 1
#define EFL_NET_DIALER_PROTECTED 1
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
#ifdef HAVE_NETINET_SSL_H
# include <netinet/ssl.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#define MY_CLASS EFL_NET_DIALER_SSL_CLASS

/**
 * @brief Private data for the Efl_Net_Dialer_Ssl class.
 */
typedef struct _Efl_Net_Dialer_Ssl_Data
{
   Eo *sock; /**< The underlying TCP socket dialer or adopted socket. */
   Eo *ssl_ctx; /**< The SSL context to be used for the connection. */
   Eina_Future *connect_timeout; /**< Future for handling connection timeout. */
   Eina_Bool connected; /**< Flag indicating if the dialer is connected. */
} Efl_Net_Dialer_Ssl_Data;

/**
 * @brief Callback invoked when the SSL handshake is successfully completed.
 *
 * This function sets the dialer's connected state to EINA_TRUE.
 *
 * @param data User data, unused in this callback.
 * @param event The event information.
 */
static void
_efl_net_dialer_ssl_ready(void *data EINA_UNUSED, const Efl_Event *event)
{
   Eo *o = event->object;
   efl_net_dialer_connected_set(o, EINA_TRUE);
}

/**
 * @brief Callback invoked when an error occurs during the SSL handshake.
 *
 * This function forwards the error as an EFL_NET_DIALER_EVENT_DIALER_ERROR event.
 *
 * @param data User data, unused in this callback.
 * @param event The event information, containing the Eina_Error.
 */
static void
_efl_net_dialer_ssl_error(void *data EINA_UNUSED, const Efl_Event *event)
{
   Eo *o = event->object;
   Eina_Error *perr = event->info;
   efl_event_callback_call(o, EFL_NET_DIALER_EVENT_DIALER_ERROR, perr);
}

EFL_CALLBACKS_ARRAY_DEFINE(_efl_net_dialer_ssl_cbs,
                           { EFL_NET_SOCKET_SSL_EVENT_SSL_READY, _efl_net_dialer_ssl_ready },
                           { EFL_NET_SOCKET_SSL_EVENT_SSL_ERROR, _efl_net_dialer_ssl_error });

/**
 * @brief Constructor for the Efl_Net_Dialer_Ssl object.
 *
 * Initializes the dialer, creates an underlying TCP dialer, and sets up
 * event callbacks for SSL events.
 *
 * @param o The Efl_Net_Dialer_Ssl object.
 * @param pd The private data for the object.
 * @return The constructed Efl_Net_Dialer_Ssl object, or NULL on failure.
 */
EOLIAN static Eo*
_efl_net_dialer_ssl_efl_object_constructor(Eo *o, Efl_Net_Dialer_Ssl_Data *pd)
{
   o = efl_constructor(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   pd->sock = efl_add_ref(EFL_NET_DIALER_TCP_CLASS, o);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->sock, NULL);

   efl_event_callback_array_add(o, _efl_net_dialer_ssl_cbs(), o);

   return o;
}

/**
 * @brief Finalizer for the Efl_Net_Dialer_Ssl object.
 *
 * Handles adoption of an existing socket or sets up the default SSL context
 * if no socket is adopted.
 *
 * @param o The Efl_Net_Dialer_Ssl object.
 * @param pd The private data for the object.
 * @return The finalized Efl_Net_Dialer_Ssl object.
 */
EOLIAN static Eo*
_efl_net_dialer_ssl_efl_object_finalize(Eo *o, Efl_Net_Dialer_Ssl_Data *pd)
{
   Eo *a_sock, *a_ctx;

   if (efl_net_socket_ssl_adopted_get(o, &a_sock, &a_ctx))
     {
        efl_replace(&pd->sock, a_sock); /* stub TCP we created */
        efl_replace(&pd->ssl_ctx, a_ctx);
     }
   else
     {
        if (!pd->ssl_ctx)
          pd->ssl_ctx = efl_ref(efl_net_ssl_context_default_dialer_get());

        efl_net_socket_ssl_adopt(o, pd->sock, pd->ssl_ctx);
     }

   return efl_finalize(efl_super(o, MY_CLASS));
}

/**
 * @brief Invalidator for the Efl_Net_Dialer_Ssl object.
 *
 * Cleans up resources, closes the connection if configured to do so,
 * and releases references to the socket and SSL context.
 *
 * @param o The Efl_Net_Dialer_Ssl object.
 * @param pd The private data for the object.
 */
EOLIAN static void
_efl_net_dialer_ssl_efl_object_invalidate(Eo *o, Efl_Net_Dialer_Ssl_Data *pd)
{
   pd->sock = NULL;

   if (efl_io_closer_close_on_invalidate_get(o) &&
       (!efl_io_closer_closed_get(o)))
     {
        efl_event_freeze(o);
        efl_io_closer_close(o);
        efl_event_thaw(o);
     }

   efl_replace(&pd->ssl_ctx, NULL);
   efl_replace(&pd->sock, NULL);

   efl_invalidate(efl_super(o, MY_CLASS));
}

/**
 * @brief Sets the SSL context for the dialer.
 *
 * This function must be called before the object is finalized.
 *
 * @param o The Efl_Net_Dialer_Ssl object.
 * @param pd The private data for the object.
 * @param ssl_ctx The SSL context to set. Must be an EFL_NET_SSL_CONTEXT_CLASS object.
 */
EOLIAN static void
_efl_net_dialer_ssl_ssl_context_set(Eo *o, Efl_Net_Dialer_Ssl_Data *pd, Eo *ssl_ctx)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(o));
   EINA_SAFETY_ON_FALSE_RETURN(efl_isa(ssl_ctx, EFL_NET_SSL_CONTEXT_CLASS));

   if (pd->ssl_ctx == ssl_ctx) return;
   efl_replace(&pd->ssl_ctx, ssl_ctx);
}

/**
 * @brief Gets the SSL context used by the dialer.
 *
 * @param o The Efl_Net_Dialer_Ssl object, unused.
 * @param pd The private data for the object.
 * @return The current SSL context.
 */
EOLIAN static Eo *
_efl_net_dialer_ssl_ssl_context_get(const Eo *o EINA_UNUSED, Efl_Net_Dialer_Ssl_Data *pd)
{
   return pd->ssl_ctx;
}

/**
 * @brief Callback function executed when the connection attempt times out.
 *
 * This function sets the End-Of-Stream (EOS) flag and emits a
 * EFL_NET_DIALER_EVENT_DIALER_ERROR event with ETIMEDOUT.
 *
 * @param o The Efl_Net_Dialer_Ssl object.
 * @param data User data, unused in this callback.
 * @param v The Eina_Value associated with the future, unused.
 * @return The input Eina_Value v.
 */
static Eina_Value
_efl_net_dialer_ssl_connect_timeout(Eo *o, void *data EINA_UNUSED, const Eina_Value v)
{
   Eina_Error err = ETIMEDOUT;

   efl_ref(o);
   efl_io_reader_eos_set(o, EINA_TRUE);
   efl_event_callback_call(o, EFL_NET_DIALER_EVENT_DIALER_ERROR, &err);
   efl_unref(o);
   return v;
}

/**
 * @brief Schedules a connection timeout.
 *
 * If a timeout is specified, this function sets up a future that will
 * trigger `_efl_net_dialer_ssl_connect_timeout` if the connection
 * does not complete within the given time.
 *
 * @param o The Efl_Net_Dialer_Ssl object.
 * @param pd The private data for the object.
 * @param timeout The timeout duration in seconds.
 */
static void
_timeout_schedule(Eo *o, Efl_Net_Dialer_Ssl_Data *pd, double timeout)
{
   efl_future_then(o, efl_loop_timeout(efl_loop_get(o), timeout),
                   .success = _efl_net_dialer_ssl_connect_timeout,
                   .storage = &pd->connect_timeout);
}

/**
 * @brief Initiates a connection to the given address.
 *
 * This function delegates the actual dialing to the underlying TCP dialer
 * and schedules a timeout if one is configured.
 *
 * @param o The Efl_Net_Dialer_Ssl object.
 * @param pd The private data for the object.
 * @param address The address to connect to (e.g., "example.com:443").
 * @return 0 on success, or a POSIX error code on failure.
 *         Possible errors include EINVAL (null address), EISCONN (already connected),
 *         EBADF (closed).
 */
EOLIAN static Eina_Error
_efl_net_dialer_ssl_efl_net_dialer_dial(Eo *o, Efl_Net_Dialer_Ssl_Data *pd, const char *address)
{
   double timeout;

   EINA_SAFETY_ON_NULL_RETURN_VAL(address, EINVAL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_net_dialer_connected_get(o), EISCONN);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_io_closer_closed_get(o), EBADF);

   if (pd->connect_timeout) eina_future_cancel(pd->connect_timeout);

   timeout = efl_net_dialer_timeout_dial_get(pd->sock);
   if (timeout > 0.0) _timeout_schedule(o, pd, timeout);

   return efl_net_dialer_dial(pd->sock, address);
}

/**
 * @brief Gets the address the dialer is configured to connect to.
 *
 * @param o The Efl_Net_Dialer_Ssl object, unused.
 * @param pd The private data for the object.
 * @return The dial address string.
 */
EOLIAN static const char *
_efl_net_dialer_ssl_efl_net_dialer_address_dial_get(const Eo *o EINA_UNUSED, Efl_Net_Dialer_Ssl_Data *pd)
{
   return efl_net_dialer_address_dial_get(pd->sock);
}

/**
 * @brief Sets the proxy URL for the underlying TCP dialer.
 *
 * @param o The Efl_Net_Dialer_Ssl object, unused.
 * @param pd The private data for the object.
 * @param proxy_url The proxy URL string (e.g., "socks5://user:password@host:port").
 */
EOLIAN static void
_efl_net_dialer_ssl_efl_net_dialer_proxy_set(Eo *o EINA_UNUSED, Efl_Net_Dialer_Ssl_Data *pd, const char *proxy_url)
{
   efl_net_dialer_proxy_set(pd->sock, proxy_url);
}

/**
 * @brief Gets the proxy URL from the underlying TCP dialer.
 *
 * @param o The Efl_Net_Dialer_Ssl object, unused.
 * @param pd The private data for the object.
 * @return The proxy URL string.
 */
EOLIAN static const char *
_efl_net_dialer_ssl_efl_net_dialer_proxy_get(const Eo *o EINA_UNUSED, Efl_Net_Dialer_Ssl_Data *pd)
{
   return efl_net_dialer_proxy_get(pd->sock);
}

/**
 * @brief Sets the dial timeout for the connection.
 *
 * If a connection is already in progress and not yet connected, this
 * function will cancel the existing timeout and schedule a new one.
 *
 * @param o The Efl_Net_Dialer_Ssl object.
 * @param pd The private data for the object.
 * @param seconds The timeout duration in seconds. A value <= 0.0 disables the timeout.
 */
EOLIAN static void
_efl_net_dialer_ssl_efl_net_dialer_timeout_dial_set(Eo *o, Efl_Net_Dialer_Ssl_Data *pd, double seconds)
{
   efl_net_dialer_timeout_dial_set(pd->sock, seconds);

   if (pd->connect_timeout) eina_future_cancel(pd->connect_timeout);

   if ((seconds > 0.0) && (!pd->connected)) _timeout_schedule(o, pd, seconds);
}

/**
 * @brief Gets the dial timeout for the connection.
 *
 * @param o The Efl_Net_Dialer_Ssl object, unused.
 * @param pd The private data for the object.
 * @return The timeout duration in seconds.
 */
EOLIAN static double
_efl_net_dialer_ssl_efl_net_dialer_timeout_dial_get(const Eo *o EINA_UNUSED, Efl_Net_Dialer_Ssl_Data *pd)
{
   return efl_net_dialer_timeout_dial_get(pd->sock);
}

/**
 * @brief Sets the connected state of the dialer.
 *
 * This function cancels any pending connection timeout and, if the state
 * changes to connected, emits the EFL_NET_DIALER_EVENT_DIALER_CONNECTED event.
 *
 * @param o The Efl_Net_Dialer_Ssl object.
 * @param pd The private data for the object.
 * @param connected The new connected state.
 */
EOLIAN static void
_efl_net_dialer_ssl_efl_net_dialer_connected_set(Eo *o, Efl_Net_Dialer_Ssl_Data *pd, Eina_Bool connected)
{
   if (pd->connect_timeout)
     eina_future_cancel(pd->connect_timeout);
   if (pd->connected == connected) return;
   pd->connected = connected;
   if (connected) efl_event_callback_call(o, EFL_NET_DIALER_EVENT_DIALER_CONNECTED, NULL);
}

/**
 * @brief Gets the connected state of the dialer.
 *
 * @param o The Efl_Net_Dialer_Ssl object, unused.
 * @param pd The private data for the object.
 * @return EINA_TRUE if connected, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_dialer_ssl_efl_net_dialer_connected_get(const Eo *o EINA_UNUSED, Efl_Net_Dialer_Ssl_Data *pd)
{
   return pd->connected;
}

/**
 * @brief Closes the connection.
 *
 * This function sets the connected state to EINA_FALSE and then calls the
 * parent class's close method.
 *
 * @param o The Efl_Net_Dialer_Ssl object.
 * @param pd The private data for the object, unused.
 * @return 0 on success, or a POSIX error code on failure.
 */
EOLIAN static Eina_Error
_efl_net_dialer_ssl_efl_io_closer_close(Eo *o, Efl_Net_Dialer_Ssl_Data *pd EINA_UNUSED)
{
   efl_net_dialer_connected_set(o, EINA_FALSE);
   return efl_io_closer_close(efl_super(o, MY_CLASS));
}

/**
 * @brief Sets the TCP keep-alive option for the underlying socket.
 *
 * @param o The Efl_Net_Dialer_Ssl object, unused.
 * @param pd The private data for the object.
 * @param keep_alive EINA_TRUE to enable keep-alive, EINA_FALSE to disable.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_net_dialer_ssl_keep_alive_set(Eo *o EINA_UNUSED, Efl_Net_Dialer_Ssl_Data *pd, Eina_Bool keep_alive)
{
   return efl_net_socket_tcp_keep_alive_set(pd->sock, keep_alive);
}

/**
 * @brief Gets the TCP keep-alive option for the underlying socket.
 *
 * @param o The Efl_Net_Dialer_Ssl object, unused.
 * @param pd The private data for the object.
 * @return EINA_TRUE if keep-alive is enabled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_dialer_ssl_keep_alive_get(const Eo *o EINA_UNUSED, Efl_Net_Dialer_Ssl_Data *pd)
{
   return efl_net_socket_tcp_keep_alive_get(pd->sock);
}

/**
 * @brief Sets the TCP no-delay (Nagle's algorithm) option for the underlying socket.
 *
 * @param o The Efl_Net_Dialer_Ssl object, unused.
 * @param pd The private data for the object.
 * @param no_delay EINA_TRUE to disable Nagle's algorithm (enable no-delay), EINA_FALSE to enable it.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_net_dialer_ssl_no_delay_set(Eo *o EINA_UNUSED, Efl_Net_Dialer_Ssl_Data *pd, Eina_Bool no_delay)
{
   return efl_net_socket_tcp_no_delay_set(pd->sock, no_delay);
}

/**
 * @brief Gets the TCP no-delay (Nagle's algorithm) option for the underlying socket.
 *
 * @param o The Efl_Net_Dialer_Ssl object, unused.
 * @param pd The private data for the object.
 * @return EINA_TRUE if no-delay is enabled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_dialer_ssl_no_delay_get(const Eo *o EINA_UNUSED, Efl_Net_Dialer_Ssl_Data *pd)
{
   return efl_net_socket_tcp_no_delay_get(pd->sock);
}

#include "efl_net_dialer_ssl.eo.c"
