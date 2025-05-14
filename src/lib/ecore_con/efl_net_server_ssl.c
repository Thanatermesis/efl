#define EFL_NET_SERVER_SSL_PROTECTED 1
#define EFL_NET_SERVER_FD_PROTECTED 1
#define EFL_NET_SERVER_PROTECTED 1
#define EFL_LOOP_FD_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

#define MY_CLASS EFL_NET_SERVER_SSL_CLASS

/**
 * @brief Private data for the Efl_Net_Server_Ssl class.
 */
typedef struct _Efl_Net_Server_Ssl_Data
{
   Eo *server; /**< The underlying TCP server object. */
   Eo *ssl_ctx; /**< The SSL context to be used for new connections. */
} Efl_Net_Server_Ssl_Data;

/**
 * @brief Callback for when a client connection is closed.
 *
 * This function is registered as an event callback for the EFL_IO_CLOSER_EVENT_CLOSED
 * event on each client. It ensures proper cleanup when a client disconnects.
 *
 * @param data The server object (Eo *).
 * @param event The event information.
 */
static void
_efl_net_server_ssl_client_event_closed(void *data, const Efl_Event *event)
{
   Eo *server = data;
   Eo *client = event->object;

   efl_event_callback_del(client, EFL_IO_CLOSER_EVENT_CLOSED, _efl_net_server_ssl_client_event_closed, server);
   if (efl_parent_get(client) == server)
     efl_parent_set(client, NULL);

   /* do NOT change count as we're using the underlying server's count */
   //efl_net_server_clients_count_set(server, efl_net_server_clients_count_get(server) - 1);
}

/**
 * @brief Announces a new client to the server.
 *
 * This function is called by the underlying TCP server when a new client
 * has been wrapped with SSL. It performs safety checks, emits the
 * EFL_NET_SERVER_EVENT_CLIENT_ADD event, and sets up a callback for when
 * the client closes.
 *
 * @param o The server object.
 * @param pd The private data of the server.
 * @param client The newly connected and SSL-wrapped client socket.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., client was reparented,
 *         not handled, or closed immediately).
 */
static Eina_Bool
_efl_net_server_ssl_efl_net_server_client_announce(Eo *o, Efl_Net_Server_Ssl_Data *pd EINA_UNUSED, Eo *client)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_GOTO(efl_isa(client, EFL_NET_SOCKET_SSL_CLASS), wrong_type);
   EINA_SAFETY_ON_FALSE_GOTO(efl_parent_get(client) == o, wrong_parent);

   efl_event_callback_call(o, EFL_NET_SERVER_EVENT_CLIENT_ADD, client);

   if (efl_parent_get(client) != o)
     {
        DBG("client %s was reparented! Ignoring it...",
            efl_net_socket_address_remote_get(client));
        return EINA_TRUE;
     }

   if (efl_ref_count(client) == 1) /* users must take a reference themselves */
     {
        DBG("client %s was not handled, closing it...",
            efl_net_socket_address_remote_get(client));
        efl_del(client);
        return EINA_FALSE;
     }
   else if (efl_io_closer_closed_get(client))
     {
        DBG("client %s was closed from 'client,add', delete it...",
            efl_net_socket_address_remote_get(client));
        efl_del(client);
        return EINA_FALSE;
     }

   /* do NOT change count as we're using the underlying server's count */
   //efl_net_server_clients_count_set(o, efl_net_server_clients_count_get(o) + 1);
   efl_event_callback_add(client, EFL_IO_CLOSER_EVENT_CLOSED, _efl_net_server_ssl_client_event_closed, o);
   return EINA_TRUE;

 wrong_type:
   ERR("%p client %p (%s) doesn't implement Efl.Net.Socket.Ssl class, deleting it.", o, client, efl_class_name_get(efl_class_get(client)));
   efl_del(client);
   return EINA_FALSE;

 wrong_parent:
   ERR("%p client %p (%s) parent=%p is not our child, deleting it.", o, client, efl_class_name_get(efl_class_get(client)), efl_parent_get(client));
   efl_del(client);
   return EINA_FALSE;
}

/**
 * @brief Handles a new client connection accepted via a file descriptor.
 *
 * This function is called when the underlying server (which could be a
 * Efl_Net_Server_Fd) accepts a new client connection represented by a
 * file descriptor. It creates a TCP socket from the fd, then wraps it
 * with an SSL socket using the server's SSL context.
 *
 * @param o The server object.
 * @param pd The private data of the server.
 * @param client_fd The file descriptor of the newly accepted client.
 */
static void
_efl_net_server_ssl_efl_net_server_fd_client_add(Eo *o, Efl_Net_Server_Ssl_Data *pd, int client_fd)
{
   Eo *client_ssl;
   Eo *client_tcp;
   const char *addr;

   client_tcp = efl_add(EFL_NET_SOCKET_TCP_CLASS, o,
                        efl_io_closer_close_on_exec_set(efl_added, efl_net_server_fd_close_on_exec_get(o)),
                        efl_io_closer_close_on_invalidate_set(efl_added, EINA_TRUE),
                        efl_loop_fd_set(efl_added, client_fd));
   if (!client_tcp)
     {
        ERR("could not create client object fd=" SOCKET_FMT, (SOCKET)client_fd);
        closesocket(client_fd);
        return;
     }

   addr = efl_net_socket_address_remote_get(client_tcp);
   if (!pd->ssl_ctx)
     {
        ERR("ssl server %p rejecting client %p '%s' since no SSL context was set!", o, client_tcp, addr);
        efl_event_callback_call(o, EFL_NET_SERVER_EVENT_CLIENT_REJECTED, (void *)addr);
        efl_del(client_tcp);
        return;
     }

   client_ssl = efl_add(EFL_NET_SOCKET_SSL_CLASS, o,
                        efl_net_socket_ssl_adopt(efl_added, client_tcp, pd->ssl_ctx));
   if (!client_ssl)
     {
        ERR("ssl server %p could not wrap client %p '%s' using context=%p", o, client_tcp, addr, pd->ssl_ctx);
        efl_event_callback_call(o, EFL_NET_SERVER_EVENT_CLIENT_REJECTED, (void *)addr);
        efl_del(client_tcp);
        return;
     }

   efl_net_server_client_announce(o, client_ssl);
}

/**
 * @brief Constructor for the Efl_Net_Server_Ssl object.
 *
 * Initializes the SSL server by creating an underlying TCP server and
 * compositing it.
 *
 * @param o The object being constructed.
 * @param pd The private data for the object.
 * @return The constructed object, or NULL on failure.
 */
EOLIAN Efl_Object *
_efl_net_server_ssl_efl_object_constructor(Eo *o, Efl_Net_Server_Ssl_Data *pd)
{
   o = efl_constructor(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   pd->server = efl_add(EFL_NET_SERVER_TCP_CLASS, o);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->server, NULL);
   if (!efl_composite_attach(o, pd->server))
     goto on_error;

   return o;

 on_error:
   ERR("Failed to composite the object\n");
   efl_del(pd->server);
   efl_del(o);
   return NULL;
}

/**
 * @brief Callback for when the associated SSL context is deleted.
 *
 * This function is registered as an event callback for the EFL_EVENT_DEL
 * event on the SSL context. It ensures that the server's reference to the
 * SSL context is cleared if the context is deleted externally.
 *
 * @param data The server object (Eo *).
 * @param event The event information (unused).
 */
static void
_efl_net_server_ssl_ctx_del(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Net_Server_Ssl_Data *pd = efl_data_scope_get(o, MY_CLASS);
   pd->ssl_ctx = NULL;
}

/**
 * @brief Invalidates the Efl_Net_Server_Ssl object.
 *
 * Called when the object is being invalidated. It clears the reference
 * to the underlying TCP server.
 *
 * @param o The object being invalidated.
 * @param pd The private data for the object.
 */
EOLIAN void
_efl_net_server_ssl_efl_object_invalidate(Eo *o, Efl_Net_Server_Ssl_Data *pd)
{
   pd->server = NULL;

   efl_invalidate(efl_super(o, MY_CLASS));
}

/**
 * @brief Destructor for the Efl_Net_Server_Ssl object.
 *
 * Cleans up resources, specifically unreferencing the SSL context and
 * removing the EFL_EVENT_DEL callback from it.
 *
 * @param o The object being destructed.
 * @param pd The private data for the object.
 */
EOLIAN void
_efl_net_server_ssl_efl_object_destructor(Eo *o, Efl_Net_Server_Ssl_Data *pd)
{
   if (pd->ssl_ctx)
     {
        efl_event_callback_del(pd->ssl_ctx, EFL_EVENT_DEL, _efl_net_server_ssl_ctx_del, o);
        efl_unref(pd->ssl_ctx);
        pd->ssl_ctx = NULL;
     }

   efl_destructor(efl_super(o, MY_CLASS));
}

/**
 * @brief Sets the SSL context for the server.
 *
 * This context will be used for all new client connections.
 *
 * @param o The server object (unused).
 * @param pd The private data of the server.
 * @param ssl_ctx The SSL context to use. Must be an Efl_Net_Ssl_Context object.
 */
EOLIAN static void
_efl_net_server_ssl_ssl_context_set(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd, Eo *ssl_ctx)
{
   EINA_SAFETY_ON_FALSE_RETURN(efl_isa(ssl_ctx, EFL_NET_SSL_CONTEXT_CLASS));

   if (pd->ssl_ctx == ssl_ctx) return;
   efl_unref(pd->ssl_ctx);
   pd->ssl_ctx = efl_ref(ssl_ctx);
   if (ssl_ctx)
     efl_event_callback_add(ssl_ctx, EFL_EVENT_DEL, _efl_net_server_ssl_ctx_del, o);
}

/**
 * @brief Gets the SSL context currently used by the server.
 *
 * @param o The server object (unused).
 * @param pd The private data of the server.
 * @return The current SSL context, or NULL if none is set.
 */
EOLIAN static Eo *
_efl_net_server_ssl_ssl_context_get(const Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd)
{
   return pd->ssl_ctx;
}

#include "efl_net_server_ssl.eo.c"
