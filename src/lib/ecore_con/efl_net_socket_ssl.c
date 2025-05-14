#define EFL_NET_SOCKET_SSL_PROTECTED 1
#define EFL_IO_READER_PROTECTED 1
#define EFL_IO_WRITER_PROTECTED 1
#define EFL_IO_CLOSER_PROTECTED 1
#define EFL_NET_SOCKET_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

typedef struct _Efl_Net_Ssl_Conn Efl_Net_Ssl_Conn;

/**
 * Setups the SSL context
 *
 * Update the given lists, removing invalid entries. If all entries
 * failed in a list, return EINVAL.
 *
 * @internal
 */
static Eina_Error efl_net_ssl_conn_setup(Efl_Net_Ssl_Conn *conn, Eina_Bool is_dialer, Efl_Net_Socket *sock, Efl_Net_Ssl_Context *context);

/**
 * Cleans up the SSL associated to this context.
 * @internal
 */
static void efl_net_ssl_conn_teardown(Efl_Net_Ssl_Conn *conn);

/**
 * Send data to remote peer.
 *
 * This should be called once handshake is finished, otherwise it may
 * lead the handshake to fail.
 *
 * @param slice[inout] takes the amount of bytes to write and
 *        source memory, will store written length in slice->len.
 *
 * @internal
 */
static Eina_Error efl_net_ssl_conn_write(Efl_Net_Ssl_Conn *conn, Eina_Slice *slice);

/**
 * Receive data from remote peer.
 *
 * This should be called once handshake is finished, otherwise it may
 * lead the handshake to fail.
 *
 * Note that even if the socket 'can_read', eventually it couldn't
 * decipher a byte and it will return slice->len == 0 with EAGAIN as
 * error.
 *
 * @param slice[inout] takes the amount of bytes to read and
 *        destination memory, will store read length in slice->len.
 *
 * @internal
 */
static Eina_Error efl_net_ssl_conn_read(Efl_Net_Ssl_Conn *conn, Eina_Rw_Slice *slice);

/**
 * Attempt to finish the handshake.
 *
 * This should not block, if it's not finished yet, just set done =
 * false.
 *
 * Errors, such as failed handshake, should be returned as Eina_Error.
 *
 * @internal
 */
static Eina_Error efl_net_ssl_conn_handshake(Efl_Net_Ssl_Conn *conn, Eina_Bool *done);

/**
 * Configure how to verify peer.
 *
 * @internal
 */
static Eina_Error efl_net_ssl_conn_verify_mode_set(Efl_Net_Ssl_Conn *conn, Efl_Net_Ssl_Verify_Mode verify_mode);

/**
 * Configure whenever to check for hostname.
 *
 * @internal
 */
static Eina_Error efl_net_ssl_conn_hostname_verify_set(Efl_Net_Ssl_Conn *conn, Eina_Bool hostname_verify);

/**
 * Overrides the hostname to use.
 *
 * @note duplicate hostname if needed!
 *
 * @internal
 */
static Eina_Error efl_net_ssl_conn_hostname_override_set(Efl_Net_Ssl_Conn *conn, const char *hostname);

#if HAVE_OPENSSL
# include "efl_net_ssl_conn-openssl.c"
#else
# include "efl_net_ssl_conn-none.c"
#endif

#define MY_CLASS EFL_NET_SOCKET_SSL_CLASS

typedef struct _Efl_Net_Socket_Ssl_Data
{
   Eo *sock;
   Efl_Net_Ssl_Context *context;
   const char *hostname_override;
   Efl_Net_Ssl_Conn ssl_conn;
   Efl_Net_Ssl_Verify_Mode verify_mode;
   Eina_Bool hostname_verify;
   Eina_Bool did_handshake;
   Eina_Bool torndown;
   Eina_Bool can_read;
   Eina_Bool eos;
   Eina_Bool can_write;
} Efl_Net_Socket_Ssl_Data;

/**
 * @brief Callback for the underlying socket's EOS (End Of Stream) event.
 *
 * When the adopted socket signals EOS, this function sets the EOS state
 * on the SSL socket object itself.
 *
 * @param data The SSL socket object (Eo *o).
 * @param event The event information (unused).
 */
static void
efl_net_socket_ssl_sock_eos(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   efl_io_reader_eos_set(o, EINA_TRUE);
}

/**
 * @brief Attempts to perform or continue the SSL handshake.
 *
 * This function is called when the underlying socket becomes readable or writable,
 * or after a connection is established, if the SSL handshake has not yet completed.
 * If the handshake completes successfully, it updates the can_read and can_write
 * properties of the SSL socket and emits the EFL_NET_SOCKET_SSL_EVENT_SSL_READY event.
 * If the handshake fails, it emits EFL_NET_SOCKET_SSL_EVENT_SSL_ERROR and closes
 * the SSL socket.
 *
 * @param o The SSL socket object.
 * @param pd The private data of the SSL socket object.
 */
static void
efl_net_socket_ssl_handshake_try(Eo *o, Efl_Net_Socket_Ssl_Data *pd)
{
   Eina_Error err;

   if (pd->torndown) return;

   DBG("SSL=%p handshake...", o);

   err = efl_net_ssl_conn_handshake(&pd->ssl_conn, &pd->did_handshake);
   if (err)
     {
        WRN("SSL=%p failed handshake: %s", o, eina_error_msg_get(err));
        efl_event_callback_call(o, EFL_NET_SOCKET_SSL_EVENT_SSL_ERROR, &err);
        efl_io_closer_close(o);
        return;
     }
   if (!pd->did_handshake) return;

   DBG("SSL=%p finished handshake", o);
   efl_io_reader_can_read_set(o, efl_io_reader_can_read_get(pd->sock));
   efl_io_writer_can_write_set(o, efl_io_writer_can_write_get(pd->sock));

   efl_event_callback_call(o, EFL_NET_SOCKET_SSL_EVENT_SSL_READY, NULL);
}

/**
 * @brief Callback for the underlying socket's 'can_read_changed' event.
 *
 * If the underlying socket becomes readable:
 * - If the SSL handshake is already done, it sets the SSL socket as readable.
 * - If the SSL handshake is not done, it attempts the handshake.
 * The object `o` is reffed during this operation because callbacks might be emitted,
 * and unreffed before the function returns.
 *
 * @param data The SSL socket object (Eo *o).
 * @param event The event information (unused).
 */
static void
efl_net_socket_ssl_sock_can_read_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Net_Socket_Ssl_Data *pd = efl_data_scope_get(o, MY_CLASS);

   efl_ref(o); /* we're emitting callbacks then continuing the workflow */

   if (!efl_io_reader_can_read_get(pd->sock))
     goto end;

   if (pd->did_handshake)
     efl_io_reader_can_read_set(o, EINA_TRUE);
   else
     efl_net_socket_ssl_handshake_try(o, pd);

 end:
   efl_unref(o);
}

/**
 * @brief Callback for the underlying socket's 'can_write_changed' event.
 *
 * If the underlying socket becomes writable:
 * - If the SSL handshake is already done, it sets the SSL socket as writable.
 * - If the SSL handshake is not done, it attempts the handshake.
 * The object `o` is reffed during this operation because callbacks might be emitted,
 * and unreffed before the function returns.
 *
 * @param data The SSL socket object (Eo *o).
 * @param event The event information (unused).
 */
static void
efl_net_socket_ssl_sock_can_write_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Net_Socket_Ssl_Data *pd = efl_data_scope_get(o, MY_CLASS);

   efl_ref(o); /* we're emitting callbacks then continuing the workflow */

   if (!efl_io_writer_can_write_get(pd->sock))
     goto end;

   if (pd->did_handshake)
     efl_io_writer_can_write_set(o, EINA_TRUE);
   else
     efl_net_socket_ssl_handshake_try(o, pd);

 end:
   efl_unref(o);
}

/**
 * @brief Callback for the underlying socket's 'closed' event.
 *
 * Propagates the 'closed' event from the adopted socket to this SSL socket object.
 *
 * @param data The SSL socket object (Eo *o).
 * @param event The event information (unused).
 */
static void
efl_net_socket_ssl_sock_closed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   efl_event_callback_call(o, EFL_IO_CLOSER_EVENT_CLOSED, NULL);
}

/**
 * @brief Callback for the underlying socket's 'del' (deletion) event.
 *
 * When the adopted socket is deleted, this function nullifies the reference
 * to it within the SSL socket's private data, marks the SSL connection as
 * torn down, and cleans up associated SSL resources.
 *
 * @param data The SSL socket object (Eo *o).
 * @param event The event information (unused).
 */
static void
efl_net_socket_ssl_sock_del(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Net_Socket_Ssl_Data *pd = efl_data_scope_get(o, MY_CLASS);
   pd->sock = NULL;
   pd->torndown = EINA_TRUE;
   efl_net_ssl_conn_teardown(&pd->ssl_conn);
}

EFL_CALLBACKS_ARRAY_DEFINE(efl_net_socket_ssl_sock_cbs,
                           {EFL_IO_READER_EVENT_EOS, efl_net_socket_ssl_sock_eos},
                           {EFL_IO_READER_EVENT_CAN_READ_CHANGED, efl_net_socket_ssl_sock_can_read_changed},
                           {EFL_IO_WRITER_EVENT_CAN_WRITE_CHANGED, efl_net_socket_ssl_sock_can_write_changed},
                           {EFL_IO_CLOSER_EVENT_CLOSED, efl_net_socket_ssl_sock_closed},
                           {EFL_EVENT_DEL, efl_net_socket_ssl_sock_del});

/**
 * @brief Callback for the underlying dialer socket's 'resolved' event.
 *
 * Propagates the 'dialer_resolved' event from the adopted dialer socket
 * to this SSL socket object, if it's not already torn down.
 *
 * @param data The SSL socket object (Eo *o).
 * @param event The event information (unused).
 */
static void
efl_net_socket_ssl_sock_resolved(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Net_Socket_Ssl_Data *pd = efl_data_scope_get(o, MY_CLASS);

   if (pd->torndown) return;

   efl_event_callback_call(o, EFL_NET_DIALER_EVENT_DIALER_RESOLVED, NULL);
}

/**
 * @brief Callback for the underlying dialer socket's 'connected' event.
 *
 * When the adopted dialer socket connects, this function attempts the SSL handshake.
 * If the handshake fails, the SSL socket is closed.
 * The object `o` is reffed during this operation because callbacks might be emitted,
 * and unreffed before the function returns.
 *
 * @param data The SSL socket object (Eo *o).
 * @param event The event information (unused).
 */
static void
efl_net_socket_ssl_sock_connected(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Net_Socket_Ssl_Data *pd = efl_data_scope_get(o, MY_CLASS);
   Eina_Error err;

   if (pd->torndown) return;

   efl_ref(o); /* we're emitting callbacks then continuing the workflow */

   err = efl_net_ssl_conn_handshake(&pd->ssl_conn, &pd->did_handshake);
   if (err)
     {
        WRN("SSL=%p failed handshake: %s", o, eina_error_msg_get(err));
        efl_io_closer_close(o);
     }

   efl_unref(o);
}

EFL_CALLBACKS_ARRAY_DEFINE(efl_net_socket_ssl_sock_dialer_cbs,
                           {EFL_NET_DIALER_EVENT_DIALER_RESOLVED, efl_net_socket_ssl_sock_resolved},
                           {EFL_NET_DIALER_EVENT_DIALER_CONNECTED, efl_net_socket_ssl_sock_connected});

/**
 * @brief Callback for the SSL context's 'del' (deletion) event.
 *
 * When the associated SSL context is deleted, this function nullifies
 * the reference to it in the SSL socket's private data.
 *
 * @param data The SSL socket object (Eo *o).
 * @param event The event information (unused).
 */
static void
_efl_net_socket_ssl_context_del(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Net_Socket_Ssl_Data *pd = efl_data_scope_get(o, MY_CLASS);
   pd->context = NULL;
}

/**
 * @brief Implements efl_net_socket_ssl_adopt.
 *
 * Initializes the SSL connection using the provided socket and SSL context.
 * It sets up SSL parameters, including hostname for verification, based on
 * the context or derived from the socket's address. It also registers
 * callbacks on the adopted socket to monitor its state (readable, writable, closed, etc.)
 * and on the context for its deletion.
 *
 * @param o The SSL socket object.
 * @param pd The private data of the SSL socket object.
 * @param sock The Efl_Net_Socket to adopt. Must implement Efl_Net_Socket.
 * @param context The Efl_Net_Ssl_Context to use for SSL configuration.
 */
EOLIAN static void
_efl_net_socket_ssl_adopt(Eo *o, Efl_Net_Socket_Ssl_Data *pd, Efl_Net_Socket *sock, Efl_Net_Ssl_Context *context)
{
   Eina_Error err;
   char *tmp = NULL;
   const char *hostname;
   Eina_Bool is_dialer;

   EINA_SAFETY_ON_TRUE_RETURN(pd->sock != NULL);
   EINA_SAFETY_ON_FALSE_RETURN(efl_isa(sock, EFL_NET_SOCKET_INTERFACE));
   EINA_SAFETY_ON_FALSE_RETURN(efl_isa(context, EFL_NET_SSL_CONTEXT_CLASS));
   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(o));

   is_dialer = efl_isa(sock, EFL_NET_DIALER_INTERFACE);
   err = efl_net_ssl_conn_setup(&pd->ssl_conn, is_dialer, sock, context);
   if (err)
     {
        ERR("ssl=%p failed to adopt socket (is_dialer=%d) sock=%p", o, is_dialer, sock);
        return;
     }

   pd->context = efl_ref(context);
   efl_event_callback_add(context, EFL_EVENT_DEL, _efl_net_socket_ssl_context_del, o);

   DBG("ssl=%p adopted socket (is_dialer=%d) sock=%p, ssl_conn=%p", o, is_dialer, sock, &pd->ssl_conn);

   if (pd->hostname_verify == 0xff)
     pd->hostname_verify = efl_net_ssl_context_hostname_verify_get(context);

   hostname = pd->hostname_override;
   if (!hostname)
     hostname = pd->hostname_override = eina_stringshare_ref(efl_net_ssl_context_hostname_get(context));
   if (!hostname)
     {
        const char *remote_address = (is_dialer ?
                                      efl_net_dialer_address_dial_get(sock) :
                                      efl_net_socket_address_remote_get(sock));
        if (remote_address)
          {
             const char *host, *port;

             tmp = strdup(remote_address);
             EINA_SAFETY_ON_NULL_RETURN(tmp);
             if (efl_net_ip_port_split(tmp, &host, &port))
               hostname = host;
          }
     }

   if ((uint8_t)pd->verify_mode == 0xff) pd->verify_mode = efl_net_ssl_context_verify_mode_get(context);
   efl_net_ssl_conn_verify_mode_set(&pd->ssl_conn, pd->verify_mode);
   efl_net_ssl_conn_hostname_verify_set(&pd->ssl_conn, pd->hostname_verify);
   efl_net_ssl_conn_hostname_override_set(&pd->ssl_conn, hostname);
   free(tmp);

   pd->sock = efl_ref(sock);
   efl_event_callback_array_add(sock, efl_net_socket_ssl_sock_cbs(), o);

   if (efl_isa(sock, EFL_NET_DIALER_INTERFACE))
     efl_event_callback_array_add(sock, efl_net_socket_ssl_sock_dialer_cbs(), o);

   efl_net_socket_ssl_sock_can_read_changed(o, NULL);
   efl_net_socket_ssl_sock_can_write_changed(o, NULL);
   if (efl_io_closer_closed_get(sock))
     efl_event_callback_call(o, EFL_IO_CLOSER_EVENT_CLOSED, NULL);
}

/**
 * @brief Implements efl_net_socket_ssl_adopted_get.
 *
 * Retrieves the adopted socket and SSL context.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @param[out] sock Pointer to store the adopted Efl_Net_Socket, or NULL.
 * @param[out] context Pointer to store the Efl_Net_Ssl_Context, or NULL.
 * @return EINA_TRUE if a socket is adopted, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_socket_ssl_adopted_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd, Efl_Net_Socket **sock, Efl_Net_Ssl_Context **context)
{
   if (sock) *sock = pd->sock;
   if (context) *context = pd->context;
   return !!pd->sock;
}

/**
 * @brief Implements efl_net_socket_ssl_verify_mode_get.
 *
 * Gets the SSL peer verification mode.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @return The current Efl_Net_Ssl_Verify_Mode.
 */
static Efl_Net_Ssl_Verify_Mode
_efl_net_socket_ssl_verify_mode_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd)
{
   return pd->verify_mode;
}

/**
 * @brief Implements efl_net_socket_ssl_verify_mode_set.
 *
 * Sets the SSL peer verification mode. If the object is finalized and not
 * torn down, this updates the underlying SSL connection's verification mode.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @param verify_mode The Efl_Net_Ssl_Verify_Mode to set.
 */
static void
_efl_net_socket_ssl_verify_mode_set(Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd, Efl_Net_Ssl_Verify_Mode verify_mode)
{
   pd->verify_mode = verify_mode;
   if (!efl_finalized_get(o)) return;
   EINA_SAFETY_ON_TRUE_RETURN(pd->torndown);

   efl_net_ssl_conn_verify_mode_set(&pd->ssl_conn, pd->verify_mode);
}

/**
 * @brief Implements efl_net_socket_ssl_hostname_verify_get.
 *
 * Gets whether hostname verification is enabled.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @return EINA_TRUE if hostname verification is enabled, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_net_socket_ssl_hostname_verify_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd)
{
   return pd->hostname_verify;
}

/**
 * @brief Implements efl_net_socket_ssl_hostname_verify_set.
 *
 * Sets whether to enable SSL hostname verification. If the object is finalized
 * and not torn down, this updates the underlying SSL connection.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @param hostname_verify EINA_TRUE to enable hostname verification, EINA_FALSE otherwise.
 */
static void
_efl_net_socket_ssl_hostname_verify_set(Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd, Eina_Bool hostname_verify)
{
   pd->hostname_verify = hostname_verify;
   if (!efl_finalized_get(o)) return;
   EINA_SAFETY_ON_TRUE_RETURN(pd->torndown);

   efl_net_ssl_conn_hostname_verify_set(&pd->ssl_conn, pd->hostname_verify);
}

/**
 * @brief Implements efl_net_socket_ssl_hostname_override_get.
 *
 * Gets the overridden hostname used for verification.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @return The overridden hostname (stringshared), or NULL if not set.
 */
static const char *
_efl_net_socket_ssl_hostname_override_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd)
{
   return pd->hostname_override;
}

/**
 * @brief Implements efl_net_socket_ssl_hostname_override_set.
 *
 * Overrides the hostname used for SSL verification. If the object is finalized
 * and not torn down, this updates the underlying SSL connection. If the provided
 * `hostname_override` is NULL, it attempts to derive the hostname from the
 * adopted socket's remote address. The provided string is stringshared.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @param hostname_override The hostname to use for verification, or NULL to
 *        derive it or use the context's default.
 */
static void
_efl_net_socket_ssl_hostname_override_set(Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd, const char* hostname_override)
{
   char *tmp = NULL;
   const char *hostname;

   eina_stringshare_replace(&pd->hostname_override, hostname_override);

   if (!efl_finalized_get(o)) return;
   EINA_SAFETY_ON_TRUE_RETURN(pd->torndown);

   hostname = pd->hostname_override;
   if (!hostname)
     {
        const char *remote_address = (efl_isa(pd->sock, EFL_NET_DIALER_INTERFACE) ?
                                      efl_net_dialer_address_dial_get(pd->sock) :
                                      efl_net_socket_address_remote_get(pd->sock));
        if (remote_address)
          {
             const char *host, *port;

             tmp = strdup(remote_address);
             EINA_SAFETY_ON_NULL_RETURN(tmp);
             if (efl_net_ip_port_split(tmp, &host, &port))
               hostname = host;
          }
     }

   efl_net_ssl_conn_hostname_override_set(&pd->ssl_conn, hostname);
   free(tmp);
}

/**
 * @brief Implements efl_object_finalize for Efl_Net_Socket_Ssl.
 *
 * Finalizes the SSL socket object. This involves:
 * 1. Finalizing the parent class.
 * 2. Checking if a base socket (`pd->sock`) has been adopted. If not, finalization fails.
 * 3. If the adopted socket is a dialer and not yet connected, finalization succeeds without attempting handshake immediately.
 * 4. Otherwise, attempts the SSL handshake. If handshake fails, finalization fails.
 *
 * @param o The SSL socket object.
 * @param pd The private data of the SSL socket object (marked EINA_UNUSED in signature, but used in implementation).
 * @return The finalized Eo object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_net_socket_ssl_efl_object_finalize(Eo *o, Efl_Net_Socket_Ssl_Data *pd EINA_UNUSED)
{
   Eina_Error err;

   o = efl_finalize(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   if (!pd->sock)
     {
        ERR("no Efl.Net.Socket was adopted by this SSL=%p", o);
        return NULL;
     }

   if (efl_isa(pd->sock, EFL_NET_DIALER_INTERFACE))
     {
        if (!efl_net_dialer_connected_get(pd->sock))
          return o;
     }

   err = efl_net_ssl_conn_handshake(&pd->ssl_conn, &pd->did_handshake);
   if (err)
     {
        WRN("SSL=%p failed handshake", o);
        return NULL;
     }

   return o;
}

/**
 * @brief Implements efl_object_constructor for Efl_Net_Socket_Ssl.
 *
 * Initializes default values for `hostname_verify` and `verify_mode` in the
 * private data structure. These special values (0xff) indicate that the actual settings
 * should be inherited from the SSL context upon adoption or from global defaults.
 * Then calls the parent class's constructor.
 *
 * @param o The SSL socket object.
 * @param pd The private data of the SSL socket object.
 * @return The constructed Eo object from the superclass.
 */
EOLIAN static Eo *
_efl_net_socket_ssl_efl_object_constructor(Eo *o, Efl_Net_Socket_Ssl_Data *pd)
{
   pd->hostname_verify = 0xff;
   pd->verify_mode = 0xff;
   return efl_constructor(efl_super(o, MY_CLASS));
}

/**
 * @brief Implements efl_object_invalidate for Efl_Net_Socket_Ssl.
 *
 * Invalidates the SSL socket object. This function performs cleanup:
 * 1. Closes the socket if `close_on_invalidate` is set and it's not already closed.
 *    Event emission is frozen and thawed around the close operation.
 * 2. Marks the connection as torn down and cleans up SSL resources via `efl_net_ssl_conn_teardown`.
 * 3. Removes event callbacks from and unrefs the adopted socket (`pd->sock`), if any.
 * 4. Removes event callbacks from and unrefs the SSL context (`pd->context`), if any.
 * 5. Calls the parent class's invalidate method.
 *
 * @param o The SSL socket object.
 * @param pd The private data of the SSL socket object (marked EINA_UNUSED in signature, but used in implementation).
 */
EOLIAN static void
_efl_net_socket_ssl_efl_object_invalidate(Eo *o, Efl_Net_Socket_Ssl_Data *pd EINA_UNUSED)
{
   if (efl_io_closer_close_on_invalidate_get(o) &&
       (!efl_io_closer_closed_get(o)))
     {
        efl_event_freeze(o);
        efl_io_closer_close(o);
        efl_event_thaw(o);
     }

   pd->torndown = EINA_TRUE;
   efl_net_ssl_conn_teardown(&pd->ssl_conn);
   if (pd->sock)
     {
        efl_event_callback_array_del(pd->sock, efl_net_socket_ssl_sock_cbs(), o);
        if (efl_isa(pd->sock, EFL_NET_DIALER_INTERFACE))
          efl_event_callback_array_del(pd->sock, efl_net_socket_ssl_sock_dialer_cbs(), o);
        efl_unref(pd->sock);
        pd->sock = NULL;
     }

   if (pd->context)
     {
        efl_event_callback_del(pd->context, EFL_EVENT_DEL, _efl_net_socket_ssl_context_del, o);
        efl_unref(pd->context);
        pd->context = NULL;
     }

   efl_invalidate(efl_super(o, MY_CLASS));
}

/**
 * @brief Implements efl_object_destructor for Efl_Net_Socket_Ssl.
 *
 * Cleans up resources during destruction. Specifically, it releases the
 * stringshared `hostname_override` if it was set.
 * Then calls the parent class's destructor.
 *
 * @param o The SSL socket object.
 * @param pd The private data of the SSL socket object.
 */
EOLIAN static void
_efl_net_socket_ssl_efl_object_destructor(Eo *o, Efl_Net_Socket_Ssl_Data *pd)
{
   eina_stringshare_replace(&pd->hostname_override, NULL);

   efl_destructor(efl_super(o, MY_CLASS));
}

/**
 * @brief Implements efl_io_closer_close for Efl_Net_Socket_Ssl.
 *
 * Closes the SSL socket. This involves:
 * 1. Setting `can_read` to false and `eos` to true on the SSL socket object.
 * 2. Marking the connection as torn down (`pd->torndown = EINA_TRUE`).
 * 3. Tearing down the SSL connection context via `efl_net_ssl_conn_teardown`.
 * 4. If the underlying adopted socket is not already closed, its `efl_io_closer_close`
 *    method is called.
 *
 * @param o The SSL socket object.
 * @param pd The private data of the SSL socket object.
 * @return 0 on success, or an Eina_Error code on failure (e.g., from closing the underlying socket).
 *         Returns EBADF if no socket is adopted (`pd->sock` is NULL).
 */
EOLIAN static Eina_Error
_efl_net_socket_ssl_efl_io_closer_close(Eo *o, Efl_Net_Socket_Ssl_Data *pd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->sock, EBADF);

   efl_io_reader_can_read_set(o, EINA_FALSE);
   efl_io_reader_eos_set(o, EINA_TRUE);
   pd->torndown = EINA_TRUE;
   efl_net_ssl_conn_teardown(&pd->ssl_conn);
   if (efl_io_closer_closed_get(pd->sock))
     return 0;
   return efl_io_closer_close(pd->sock);
}

/**
 * @brief Implements efl_io_closer_closed_get for Efl_Net_Socket_Ssl.
 *
 * Checks if the SSL socket is closed. This is true if no underlying socket
 * is adopted (`pd->sock` is NULL) or if the adopted socket itself is closed.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @return EINA_TRUE if the socket is closed, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_socket_ssl_efl_io_closer_closed_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd)
{
   return (!pd->sock) || efl_io_closer_closed_get(pd->sock);
}

/**
 * @brief Implements efl_io_reader_read for Efl_Net_Socket_Ssl.
 *
 * Reads data from the SSL socket.
 * - If the SSL handshake (`pd->did_handshake`) is not yet complete, it returns EAGAIN.
 * - If torn down (`pd->torndown`), it returns EBADF.
 * - Otherwise, it attempts to read decrypted data via `efl_net_ssl_conn_read`.
 * - If the read results in 0 bytes and no error, `can_read` is set to false for this
 *   object, and if the error was 0 (indicating clean SSL-level EOF), `eos` is set to true.
 *
 * @param o The SSL socket object.
 * @param pd The private data of the SSL socket object.
 * @param rw_slice Slice indicating buffer to read into and its size.
 *                 On success, `rw_slice->len` contains the number of bytes read.
 *                 On EAGAIN due to pending handshake, `rw_slice->mem` is set to NULL
 *                 and `rw_slice->len` to 0.
 * @return 0 on success (even if 0 bytes read, if not an error), or an Eina_Error code on failure.
 *         Common errors: EAGAIN (operation would block or handshake pending),
 *         EBADF (socket torn down), EINVAL (rw_slice is NULL).
 */
EOLIAN static Eina_Error
_efl_net_socket_ssl_efl_io_reader_read(Eo *o, Efl_Net_Socket_Ssl_Data *pd, Eina_Rw_Slice *rw_slice)
{
   Eina_Error err;

   EINA_SAFETY_ON_NULL_RETURN_VAL(rw_slice, EINVAL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(pd->torndown, EBADF);

   if (!pd->did_handshake)
     {
        rw_slice->mem = NULL;
        rw_slice->len = 0;
        return EAGAIN;
     }

   err = efl_net_ssl_conn_read(&pd->ssl_conn, rw_slice);

   if (rw_slice->len == 0)
     {
        efl_io_reader_can_read_set(o, EINA_FALSE);
        if (err == 0)
          efl_io_reader_eos_set(o, EINA_TRUE);
     }

   return err;
}

/**
 * @brief Implements efl_io_reader_can_read_set for Efl_Net_Socket_Ssl.
 *
 * Sets the `can_read` property of the SSL socket. If the state changes from its
 * current value (`pd->can_read`), it updates `pd->can_read` and emits the
 * EFL_IO_READER_EVENT_CAN_READ_CHANGED event with the new state.
 * This is typically called internally based on SSL state or underlying socket readiness.
 *
 * @param o The SSL socket object.
 * @param pd The private data of the SSL socket object.
 * @param can_read EINA_TRUE if the socket should be marked as readable, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_net_socket_ssl_efl_io_reader_can_read_set(Eo *o, Efl_Net_Socket_Ssl_Data *pd, Eina_Bool can_read)
{
   EINA_SAFETY_ON_NULL_RETURN(pd->sock);
   if (pd->can_read == can_read) return;
   pd->can_read = can_read;
   efl_event_callback_call(o, EFL_IO_READER_EVENT_CAN_READ_CHANGED, &can_read);
}

/**
 * @brief Implements efl_io_reader_can_read_get for Efl_Net_Socket_Ssl.
 *
 * Gets the `can_read` property of the SSL socket, which indicates if data
 * is expected to be available for reading.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @return EINA_TRUE if the socket is marked as readable, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_socket_ssl_efl_io_reader_can_read_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd)
{
   return pd->can_read;
}

/**
 * @brief Implements efl_io_reader_eos_get for Efl_Net_Socket_Ssl.
 *
 * Gets the End-Of-Stream (EOS) state of the SSL socket.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @return EINA_TRUE if EOS has been reached, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_socket_ssl_efl_io_reader_eos_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd)
{
   return pd->eos;
}

/**
 * @brief Implements efl_io_reader_eos_set for Efl_Net_Socket_Ssl.
 *
 * Sets the End-Of-Stream (EOS) state of the SSL socket. If the state changes
 * from its current value (`pd->eos`) to true, it updates `pd->eos` and emits
 * the EFL_IO_READER_EVENT_EOS event.
 * This is typically called internally when the SSL layer or underlying socket
 * signals that no more data will be received.
 *
 * @param o The SSL socket object.
 * @param pd The private data of the SSL socket object.
 * @param is_eos EINA_TRUE to set EOS state, EINA_FALSE to clear it (though clearing EOS is unusual).
 */
EOLIAN static void
_efl_net_socket_ssl_efl_io_reader_eos_set(Eo *o, Efl_Net_Socket_Ssl_Data *pd, Eina_Bool is_eos)
{
   EINA_SAFETY_ON_NULL_RETURN(pd->sock);
   if (pd->eos == is_eos) return;
   pd->eos = is_eos;
   if (is_eos)
     efl_event_callback_call(o, EFL_IO_READER_EVENT_EOS, NULL);
}

/**
 * @brief Implements efl_io_writer_write for Efl_Net_Socket_Ssl.
 *
 * Writes data to the SSL socket.
 * - If the SSL handshake (`pd->did_handshake`) is not yet complete, it returns EAGAIN.
 *   In this case, if `remaining` is provided, it's set to `*ro_slice`; `ro_slice->len` is set to 0.
 * - If torn down (`pd->torndown`), it returns EBADF.
 * - Otherwise, it attempts to write encrypted data via `efl_net_ssl_conn_write`.
 * - The `ro_slice->len` is updated with the number of bytes actually consumed from the input.
 * - If `remaining` is provided, it's adjusted to reflect the unwritten part of the original `ro_slice`.
 * - If the write consumes 0 bytes from `ro_slice` (and no error occurred), `can_write` is set to false.
 *
 * @param o The SSL socket object.
 * @param pd The private data of the SSL socket object.
 * @param ro_slice Slice containing the data to write. On return, `ro_slice->len`
 *                 is updated to the number of bytes actually consumed/written from this slice.
 * @param[out] remaining Optional. If not NULL, this slice is updated to represent the
 *                       portion of the input `ro_slice` that was not written.
 *                       It's initialized with `*ro_slice` at the start of the SSL write operation.
 * @return 0 on success (even if 0 bytes written, if not an error), or an Eina_Error code on failure.
 *         Common errors: EAGAIN (operation would block or handshake pending),
 *         EBADF (socket torn down), EINVAL (ro_slice is NULL).
 */
EOLIAN static Eina_Error
_efl_net_socket_ssl_efl_io_writer_write(Eo *o, Efl_Net_Socket_Ssl_Data *pd, Eina_Slice *ro_slice, Eina_Slice *remaining)
{
   Eina_Error err;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ro_slice, EINVAL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(pd->torndown, EBADF);

   if (!pd->did_handshake)
     {
        if (remaining) *remaining = *ro_slice;
        ro_slice->mem = NULL;
        ro_slice->len = 0;
        return EAGAIN;
     }

   if (remaining) *remaining = *ro_slice;

   err = efl_net_ssl_conn_write(&pd->ssl_conn, ro_slice);

   if (remaining)
     {
        remaining->bytes += ro_slice->len;
        remaining->len -= ro_slice->len;
     }

   if (ro_slice->len == 0)
     efl_io_writer_can_write_set(o, EINA_FALSE);

   return err;
}

/**
 * @brief Implements efl_io_writer_can_write_set for Efl_Net_Socket_Ssl.
 *
 * Sets the `can_write` property of the SSL socket. If the state changes from its
 * current value (`pd->can_write`), it updates `pd->can_write` and emits the
 * EFL_IO_WRITER_EVENT_CAN_WRITE_CHANGED event with the new state.
 * This is typically called internally based on SSL state or underlying socket readiness.
 *
 * @param o The SSL socket object.
 * @param pd The private data of the SSL socket object.
 * @param can_write EINA_TRUE if the socket should be marked as writable, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_net_socket_ssl_efl_io_writer_can_write_set(Eo *o, Efl_Net_Socket_Ssl_Data *pd, Eina_Bool can_write)
{
   EINA_SAFETY_ON_NULL_RETURN(pd->sock);
   if (pd->can_write == can_write) return;
   pd->can_write = can_write;
   efl_event_callback_call(o, EFL_IO_WRITER_EVENT_CAN_WRITE_CHANGED, &can_write);
}

/**
 * @brief Implements efl_io_writer_can_write_get for Efl_Net_Socket_Ssl.
 *
 * Gets the `can_write` property of the SSL socket, which indicates if the socket
 * is ready to accept data for writing.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @return EINA_TRUE if the socket is marked as writable, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_socket_ssl_efl_io_writer_can_write_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd)
{
   return pd->can_write;
}

/**
 * @brief Implements efl_io_closer_close_on_exec_set for Efl_Net_Socket_Ssl.
 *
 * Sets the close-on-exec flag for the underlying adopted socket, if one exists.
 * This flag determines if the socket's file descriptor will be closed automatically
 * when a new program is executed via one of the `exec` family of functions.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @param close_on_exec EINA_TRUE to enable close-on-exec, EINA_FALSE to disable.
 * @return EINA_TRUE (the underlying efl_io_closer_close_on_exec_set might return Eina_Bool,
 *          but this wrapper always returns EINA_TRUE).
 */
EOLIAN static Eina_Bool
_efl_net_socket_ssl_efl_io_closer_close_on_exec_set(Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd, Eina_Bool close_on_exec)
{
   if (pd->sock) efl_io_closer_close_on_exec_set(pd->sock, close_on_exec);
   return EINA_TRUE;
}

/**
 * @brief Implements efl_io_closer_close_on_exec_get for Efl_Net_Socket_Ssl.
 *
 * Gets the close-on-exec flag from the underlying adopted socket.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @return EINA_TRUE if close-on-exec is set on the underlying socket,
 *         EINA_FALSE otherwise (including if no socket is adopted).
 */
EOLIAN static Eina_Bool
_efl_net_socket_ssl_efl_io_closer_close_on_exec_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd)
{
   return pd->sock && efl_io_closer_close_on_exec_get(pd->sock);
}

/**
 * @brief Implements efl_io_closer_close_on_invalidate_set for Efl_Net_Socket_Ssl.
 *
 * Sets the close-on-invalidate flag for the underlying adopted socket, if one exists.
 * This flag determines if `efl_io_closer_close()` should be automatically
 * called on the socket when its Efl_Object is invalidated.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @param close_on_invalidate EINA_TRUE to enable auto-close on invalidate, EINA_FALSE to disable.
 */
EOLIAN static void
_efl_net_socket_ssl_efl_io_closer_close_on_invalidate_set(Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd, Eina_Bool close_on_invalidate)
{
   if (pd->sock) efl_io_closer_close_on_invalidate_set(pd->sock, close_on_invalidate);
}

/**
 * @brief Implements efl_io_closer_close_on_invalidate_get for Efl_Net_Socket_Ssl.
 *
 * Gets the close-on-invalidate flag from the underlying adopted socket.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @return EINA_TRUE if close-on-invalidate is set on the underlying socket,
 *         EINA_FALSE otherwise (including if no socket is adopted).
 */
EOLIAN static Eina_Bool
_efl_net_socket_ssl_efl_io_closer_close_on_invalidate_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd)
{
   return pd->sock && efl_io_closer_close_on_invalidate_get(pd->sock);
}

/**
 * @brief Implements efl_net_socket_address_local_get for Efl_Net_Socket_Ssl.
 *
 * Gets the local address string from the underlying adopted socket.
 * The returned string is typically in "IPv4:PORT" or "[IPv6]:PORT" format.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @return The local address string (stringshared, owned by the underlying socket),
 *         or a static string "unbound" if no socket is adopted or it's not bound.
 */
EOLIAN static const char *
_efl_net_socket_ssl_efl_net_socket_address_local_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd)
{
   if (!pd->sock) return "unbound";
   return efl_net_socket_address_local_get(pd->sock);
}

/**
 * @brief Implements efl_net_socket_address_remote_get for Efl_Net_Socket_Ssl.
 *
 * Gets the remote address string from the underlying adopted socket.
 * The returned string is typically in "IPv4:PORT" or "[IPv6]:PORT" format.
 *
 * @param o The SSL socket object (unused).
 * @param pd The private data of the SSL socket object.
 * @return The remote address string (stringshared, owned by the underlying socket),
 *         or a static string "unbound" if no socket is adopted or it's not connected.
 */
EOLIAN static const char *
_efl_net_socket_ssl_efl_net_socket_address_remote_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Ssl_Data *pd)
{
   if (!pd->sock) return "unbound";
   return efl_net_socket_address_remote_get(pd->sock);
}

#include "efl_net_socket_ssl.eo.c"
