#define EFL_NET_SERVER_FD_PROTECTED 1
#define EFL_NET_SERVER_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

#include <fcntl.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#define MY_CLASS EFL_NET_SERVER_FD_CLASS

/**
 * @brief Private data for the Efl_Net_Server_Fd class.
 *
 * This structure holds all the internal state for a server that operates
 * on an existing file descriptor.
 */
typedef struct _Efl_Net_Server_Fd_Data
{
   Eina_Stringshare *address; /**< The address this server is associated with (if any, usually for systemd activation). */
   int family; /**< The socket family (e.g., AF_INET, AF_INET6, AF_UNIX). Set before fd_set. */
   unsigned int clients_count; /**< Current number of connected clients. */
   unsigned int clients_limit; /**< Maximum number of clients allowed. 0 means no limit. */
   Eina_Bool clients_reject_excess; /**< If EINA_TRUE, reject new clients when limit is reached. Otherwise, stop accepting. */
   Eina_Bool waiting_accept; /**< If EINA_TRUE, the server is actively listening for read events to accept new clients. */
   Eina_Bool serving; /**< If EINA_TRUE, the server is considered to be actively serving. */
   Eina_Bool close_on_exec; /**< If EINA_TRUE, the server socket will be closed on exec(). */
   Eina_Bool reuse_address; /**< If EINA_TRUE, SO_REUSEADDR is enabled on the socket. */
   Eina_Bool reuse_port; /**< If EINA_TRUE, SO_REUSEPORT is enabled on the socket. */
} Efl_Net_Server_Fd_Data;

/**
 * @brief Wrapper around accept() or accept4() to handle client connections.
 *
 * This function accepts a new connection on the given server socket.
 * It uses accept4() if available to set SOCK_CLOEXEC atomically.
 * Otherwise, it falls back to accept() and then fcntl() to set FD_CLOEXEC.
 *
 * @param fd The listening server socket descriptor.
 * @param addr Pointer to a sockaddr structure to receive the client address.
 * @param addrlen Pointer to a socklen_t to store the client address length.
 * @param close_on_exec If EINA_TRUE, the new client socket will be set to close-on-exec.
 * @return The new client socket descriptor on success, or INVALID_SOCKET on error.
 */
static SOCKET
efl_net_accept4(SOCKET fd, struct sockaddr *addr, socklen_t *addrlen, Eina_Bool close_on_exec)
{
#ifdef HAVE_ACCEPT4
   int flags = 0;
   if (close_on_exec) flags |= SOCK_CLOEXEC;
   return accept4(fd, addr, addrlen, flags);
#else
   SOCKET client = accept(fd, addr, addrlen);
   if (client == INVALID_SOCKET) return client;

#ifdef FD_CLOEXEC
   if (close_on_exec)
     {
        if (!eina_file_close_on_exec(client, EINA_TRUE))
          {
             int errno_bkp = errno;
             ERR("fcntl(" SOCKET_FMT ", F_SETFD, FD_CLOEXEC): %s", client, eina_error_msg_get(errno));
             closesocket(client);
             errno = errno_bkp;
             return INVALID_SOCKET;
          }
     }
#else
   (void)close_on_exec;
#endif

   return client;
#endif
}

/**
 * @brief Event callback for readable server file descriptor.
 *
 * This function is called when the server's listening socket has pending
 * incoming connections (is readable). It triggers processing of this data.
 *
 * @param data User data (unused).
 * @param event The event information. The event object is the server itself.
 */
static void
_efl_net_server_fd_event_read(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_net_server_fd_process_incoming_data(event->object);
}

/**
 * @brief Event callback for an error on the server file descriptor.
 *
 * This function is called when an error occurs on the server's listening socket.
 * It sets the server to not serving and emits a server_error event.
 *
 * @param data User data (unused).
 * @param event The event information. The event object is the server itself.
 */
static void
_efl_net_server_fd_event_error(void *data EINA_UNUSED, const Efl_Event *event)
{
   Eo *o = event->object;
   Eina_Error err = EBADF;

   efl_net_server_serving_set(o, EINA_FALSE);
   efl_event_callback_call(o, EFL_NET_SERVER_EVENT_SERVER_ERROR, &err);
}

/**
 * @internal
 * @brief Finalizes the Efl_Net_Server_Fd object.
 *
 * Sets up event callbacks for read and error events on the server's fd.
 * This is called after the object is fully constructed.
 *
 * @param o The Efl_Net_Server_Fd object.
 * @param pd The private data of the object.
 * @return The finalized object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_net_server_fd_efl_object_finalize(Eo *o, Efl_Net_Server_Fd_Data *pd)
{
   o = efl_finalize(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   pd->waiting_accept = EINA_TRUE;
   efl_event_callback_add(o, EFL_LOOP_FD_EVENT_READ, _efl_net_server_fd_event_read, NULL);
   efl_event_callback_add(o, EFL_LOOP_FD_EVENT_ERROR, _efl_net_server_fd_event_error, NULL);
   return o;
}

/**
 * @internal
 * @brief Constructs the Efl_Net_Server_Fd object.
 *
 * Initializes default values for the server, such as address family and
 * close_on_exec behavior.
 *
 * @param o The Efl_Net_Server_Fd object.
 * @param pd The private data of the object.
 * @return The constructed object.
 */
EOLIAN static Efl_Object *
_efl_net_server_fd_efl_object_constructor(Eo *o, Efl_Net_Server_Fd_Data *pd)
{
   pd->family = AF_UNSPEC; // Must be set by user via family_set before fd_set
   pd->close_on_exec = EINA_TRUE;
   return efl_constructor(efl_super(o, MY_CLASS));
}

/**
 * @internal
 * @brief Destroys the Efl_Net_Server_Fd object.
 *
 * Cleans up resources, including closing the server socket if it's open
 * and freeing the stored address.
 *
 * @param o The Efl_Net_Server_Fd object.
 * @param pd The private data of the object.
 */
EOLIAN static void
_efl_net_server_fd_efl_object_destructor(Eo *o, Efl_Net_Server_Fd_Data *pd)
{
   SOCKET fd = efl_loop_fd_get(o);

   if (fd != INVALID_SOCKET)
     {
        efl_loop_fd_set(o, SOCKET_TO_LOOP_FD(INVALID_SOCKET));
        closesocket(fd);
     }

   efl_destructor(efl_super(o, MY_CLASS));

   eina_stringshare_replace(&pd->address, NULL);
}

/**
 * @internal
 * @brief Sets the file descriptor for the server.
 *
 * This function is called when the underlying file descriptor for the server
 * is set. It applies any postponed settings like close_on_exec, reuse_address,
 * and reuse_port. It also checks if the socket family has been set.
 * If an invalid FD is set, the server address is cleared.
 *
 * @param o The Efl_Net_Server_Fd object.
 * @param pd The private data of the object.
 * @param pfd The new file descriptor (cast to int).
 */
EOLIAN static void
_efl_net_server_fd_efl_loop_fd_fd_set(Eo *o, Efl_Net_Server_Fd_Data *pd, int pfd)
{
   SOCKET fd = (SOCKET)pfd;

   efl_loop_fd_set(efl_super(o, MY_CLASS), pfd);

   if (fd != INVALID_SOCKET)
     {
        /* apply postponed values */
        efl_net_server_fd_close_on_exec_set(o, pd->close_on_exec);
        efl_net_server_fd_reuse_address_set(o, pd->reuse_address);
        efl_net_server_fd_reuse_port_set(o, pd->reuse_port);

        if (pd->family == AF_UNSPEC)
          {
             ERR("efl_loop_fd_set() must be called after efl_net_server_fd_family_set()");
             return;
          }
     }
   else
     {
        efl_net_server_address_set(o, NULL);
     }
}

EOLIAN static void
_efl_net_server_fd_efl_net_server_address_set(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd, const char *address)
{
   eina_stringshare_replace(&pd->address, address);
}

EOLIAN static const char *
_efl_net_server_fd_efl_net_server_address_get(const Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd)
{
   return pd->address;
}

EOLIAN static unsigned int
_efl_net_server_fd_efl_net_server_clients_count_get(const Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd)
{
   return pd->clients_count;
}

EOLIAN static void
_efl_net_server_fd_efl_net_server_clients_count_set(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd, unsigned int count)
{
   pd->clients_count = count;
   // If there's no client limit, or the current count is below the limit,
   // ensure we are waiting for new connections.
   if ((pd->clients_limit == 0) || (pd->clients_limit > count))
     {
        if (!pd->waiting_accept)
          {
             pd->waiting_accept = EINA_TRUE;
             efl_event_callback_add(o, EFL_LOOP_FD_EVENT_READ, _efl_net_server_fd_event_read, NULL);
          }
     }
}

/**
 * @internal
 * @brief Sets the client limit and rejection policy.
 *
 * @param o The Efl_Net_Server_Fd object (unused).
 * @param pd The private data of the object.
 * @param limit The maximum number of clients. 0 for unlimited.
 * @param reject_excess If EINA_TRUE, new connections are actively rejected
 *        (socket created and immediately closed) when the limit is reached.
 *        If EINA_FALSE, the server stops accepting new connections.
 */
EOLIAN static void
_efl_net_server_fd_efl_net_server_clients_limit_set(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd, unsigned int limit, Eina_Bool reject_excess)
{
   pd->clients_limit = limit;
   pd->clients_reject_excess = reject_excess;
}

EOLIAN static void
_efl_net_server_fd_efl_net_server_clients_limit_get(const Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd, unsigned int *limit, Eina_Bool *reject_excess)
{
   if (limit) *limit = pd->clients_limit;
   if (reject_excess) *reject_excess = pd->clients_reject_excess;
}

EOLIAN static void
_efl_net_server_fd_efl_net_server_serving_set(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd, Eina_Bool serving)
{
   if (pd->serving == serving) return;
   pd->serving = serving;
   if (serving)
     efl_event_callback_call(o, EFL_NET_SERVER_EVENT_SERVING, NULL); // Notify that serving state changed.
}

/**
 * @internal
 * @brief Implements the Efl.Net.Server.serve method.
 *
 * For Efl_Net_Server_Fd, this is a no-op as serving is determined by the
 * presence of a valid file descriptor and the serving_set property.
 * The address parameter is typically used by subclasses that create their own sockets.
 *
 * @param o The Efl_Net_Server_Fd object (unused).
 * @param pd The private data of the object (unused).
 * @param address The address to serve on (ignored).
 * @return Always 0 (success), as this operation doesn't perform actions
 *         that can fail in this context.
 */
EOLIAN static Eina_Error
_efl_net_server_fd_efl_net_server_serve(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd EINA_UNUSED, const char *address)
{
   DBG("address=%s", address);
   return 0;
}

EOLIAN static Eina_Bool
_efl_net_server_fd_efl_net_server_serving_get(const Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd)
{
   return pd->serving;
}

EOLIAN static Eina_Error
_efl_net_server_fd_socket_activate(Eo *o, Efl_Net_Server_Fd_Data *pd EINA_UNUSED, const char *address)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL((SOCKET)efl_loop_fd_get(o) != INVALID_SOCKET, EALREADY); // Already has an FD
   EINA_SAFETY_ON_NULL_RETURN_VAL(address, EINVAL); // Address is not used by this function but API requires it.

#ifndef HAVE_SYSTEMD
   DBG("systemd support is disabled");
   return ENOENT;
#else
   if (!sd_fd_max)
     {
        DBG("This service was not socket-activated, no $LISTEN_FDS");
        return ENOENT;
     }
   else if (sd_fd_index >= sd_fd_max)
     {
        WRN("No more systemd sockets available. Configuration mismatch?");
        return ENOENT;
     }
   else
     {
        SOCKET fd = /*SD_LISTEN_FDS_START*/3 + sd_fd_index;
        int family;
        socklen_t len = sizeof(family);

        if (getsockopt(fd, SOL_SOCKET, SO_DOMAIN, (char *)&family, &len) != 0)
          {
             WRN("socket " SOCKET_FMT " failed to return family: %s", fd, eina_error_msg_get(efl_net_socket_error_get()));
             return EINVAL;
          }

        sd_fd_index++;
        efl_net_server_fd_family_set(o, family);
        efl_loop_fd_set(o, fd);
        if ((SOCKET)efl_loop_fd_get(o) == INVALID_SOCKET)
          {
             sd_fd_index--;
             WRN("socket " SOCKET_FMT " could not be used by %p (%s)",
                 fd, o, efl_class_name_get(efl_class_get(o)));
             return EINVAL;
          }

        /* by default they all come with close_on_exec set
         * and we must apply our local conf.
         */
        efl_net_server_fd_close_on_exec_set(o, pd->close_on_exec);
        return 0;
     }
#endif
}

EOLIAN static Eina_Bool
_efl_net_server_fd_close_on_exec_set(Eo *o, Efl_Net_Server_Fd_Data *pd, Eina_Bool close_on_exec)
{
#ifdef FD_CLOEXEC
   SOCKET fd;
   Eina_Bool old = pd->close_on_exec;
#endif

   pd->close_on_exec = close_on_exec; // Store the desired state.

#ifdef FD_CLOEXEC
   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return EINA_TRUE; /* postpone application until fd_set() */

   // Try to apply the setting immediately if FD is valid.
   if (!eina_file_close_on_exec(fd, close_on_exec))
     {
        ERR("fcntl(" SOCKET_FMT ", F_SETFD,): %s", fd, eina_error_msg_get(errno));
        pd->close_on_exec = old; // Revert to old state on failure.
        return EINA_FALSE;
     }
#else
   DBG("close on exec is not supported on your platform");
   (void)close_on_exec;
   (void)o;
#endif

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_net_server_fd_close_on_exec_get(const Eo *o, Efl_Net_Server_Fd_Data *pd)
{
#ifdef FD_CLOEXEC
   SOCKET fd;
   int flags;

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return pd->close_on_exec;

   /* if there is a fd, always query it directly as it may be modified
    * elsewhere by nasty users.
    */
   flags = fcntl(fd, F_GETFD);
   if (flags < 0)
     {
        ERR("fcntl(" SOCKET_FMT ", F_GETFD): %s", fd, eina_error_msg_get(errno));
        return EINA_FALSE;
     }

   pd->close_on_exec = !!(flags & FD_CLOEXEC); /* sync */
#else
   (void)o;
#endif
   return pd->close_on_exec;
}

EOLIAN static Eina_Bool
_efl_net_server_fd_reuse_address_set(Eo *o, Efl_Net_Server_Fd_Data *pd, Eina_Bool reuse_address)
{
   SOCKET fd;
   int value;
   Eina_Bool old = pd->reuse_address;

   pd->reuse_address = reuse_address; // Store the desired state.

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return EINA_TRUE; /* postpone application until fd_set() */

   value = reuse_address;
   // Try to apply the setting immediately if FD is valid.
   if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&value, sizeof(value)) != 0)
     {
        ERR("setsockopt(" SOCKET_FMT ", SOL_SOCKET, SO_REUSEADDR, %d): %s",
            fd, value, eina_error_msg_get(efl_net_socket_error_get()));
        pd->reuse_address = old; // Revert to old state on failure.
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_net_server_fd_reuse_address_get(const Eo *o, Efl_Net_Server_Fd_Data *pd)
{
   SOCKET fd;
   int value = 0;
   socklen_t valuelen;

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return pd->reuse_address;

   /* if there is a fd, always query it directly as it may be modified
    * elsewhere by nasty users.
    */
   valuelen = sizeof(value);
   if (getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&value, &valuelen) != 0)
     {
        ERR("getsockopt(" SOCKET_FMT ", SOL_SOCKET, SO_REUSEADDR): %s",
            fd, eina_error_msg_get(efl_net_socket_error_get()));
        return EINA_FALSE;
     }

   pd->reuse_address = !!value; /* sync */
   return pd->reuse_address;
}

EOLIAN static Eina_Bool
_efl_net_server_fd_reuse_port_set(Eo *o, Efl_Net_Server_Fd_Data *pd, Eina_Bool reuse_port)
{
#ifdef SO_REUSEPORT
   SOCKET fd;
   int value;
   Eina_Bool old = pd->reuse_port;
#endif

   pd->reuse_port = reuse_port; // Store the desired state.

#ifdef SO_REUSEPORT
   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return EINA_TRUE; /* postpone application until fd_set() */

   value = reuse_port;
   // Try to apply the setting immediately if FD is valid.
   if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const char *)&value, sizeof(value)) != 0)
     {
        ERR("setsockopt(" SOCKET_FMT ", SOL_SOCKET, SO_REUSEPORT, %d): %s",
            fd, value, eina_error_msg_get(efl_net_socket_error_get()));
        pd->reuse_port = old; // Revert to old state on failure.
        return EINA_FALSE;
     }
#else
   (void)o;
#endif

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_net_server_fd_reuse_port_get(const Eo *o, Efl_Net_Server_Fd_Data *pd)
{
#ifdef SO_REUSEPORT
   SOCKET fd;
   int value = 0;
   socklen_t valuelen;

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return pd->reuse_port;

   /* if there is a fd, always query it directly as it may be modified
    * elsewhere by nasty users.
    */
   valuelen = sizeof(value);
   if (getsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *)&value, &valuelen) != 0)
     {
        ERR("getsockopt(" SOCKET_FMT ", SOL_SOCKET, SO_REUSEPORT): %s",
            fd, eina_error_msg_get(efl_net_socket_error_get()));
        return EINA_FALSE;
     }

   pd->reuse_port = !!value; /* sync */
#else
   (void)o;
#endif

   return pd->reuse_port;
}

EOLIAN static void
_efl_net_server_fd_family_set(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd, int family)
{
   pd->family = family;
}

EOLIAN static int
_efl_net_server_fd_family_get(const Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd)
{
   return pd->family;
}

EOLIAN static void
_efl_net_server_fd_process_incoming_data(Eo *o, Efl_Net_Server_Fd_Data *pd)
{
   Eina_Bool do_reject = EINA_FALSE;
   struct sockaddr_storage addr; // To store client address information.
   SOCKET client, fd;
   socklen_t addrlen;

   // Check if client limit is reached.
   if ((pd->clients_limit > 0) && (pd->clients_count >= pd->clients_limit))
     {
        if (!pd->clients_reject_excess) // If not rejecting, just stop listening for new connections.
          {
             if (pd->waiting_accept) // If we were waiting, stop.
               {
                  pd->waiting_accept = EINA_FALSE;
                  efl_event_callback_del(o, EFL_LOOP_FD_EVENT_READ, _efl_net_server_fd_event_read, NULL);
               }
             return; // Do not accept new clients.
          }
        do_reject = EINA_TRUE; // Mark that this client should be rejected after accept.
     }

   fd = efl_loop_fd_get(o);

   addrlen = sizeof(addr);
   client = efl_net_accept4(fd, (struct sockaddr *)&addr, &addrlen,
                            efl_net_server_fd_close_on_exec_get(o));
   if (client == INVALID_SOCKET)
     {
        Eina_Error err = efl_net_socket_error_get();
        ERR("accept(" SOCKET_FMT "): %s", fd, eina_error_msg_get(err));
        efl_event_callback_call(o, EFL_NET_SERVER_EVENT_SERVER_ERROR, &err);
        return;
     }

   if (do_reject)
     efl_net_server_fd_client_reject(o, client);
   else
     efl_net_server_fd_client_add(o, client); // Add the client (implementation specific, usually creates an Efl.Net.Socket).
}

/**
 * @brief Event callback for when a client connection is closed.
 *
 * This function is called when a client that was previously accepted by this
 * server emits the EFL_IO_CLOSER_EVENT_CLOSED event. It decrements the
 * server's client count and unparents the client object if it was parented
 * to the server.
 *
 * @param data The server object (Eo *).
 * @param event The event information. The event object is the client.
 */
static void
_efl_net_server_fd_client_event_closed(void *data, const Efl_Event *event)
{
   Eo *server = data;
   Eo *client = event->object;

   // Clean up callback and parent relationship.
   efl_event_callback_del(client, EFL_IO_CLOSER_EVENT_CLOSED, _efl_net_server_fd_client_event_closed, server);
   if (efl_parent_get(client) == server)
     efl_parent_set(client, NULL);

   efl_net_server_clients_count_set(server, efl_net_server_clients_count_get(server) - 1);
}

/**
 * @internal
 * @brief Announces a new client to the server.
 *
 * This function is called by subclasses (like Efl.Net.Server.Simple) after
 * they have created a client object (e.g., Efl.Net.Socket) for a new connection.
 * It performs safety checks, emits the "client,add" event, and manages the
 * client's lifecycle based on user interaction with the event.
 * If the client is not referenced or is closed during the event, it's deleted.
 * Otherwise, its client count is incremented and a "closed" event handler is attached.
 *
 * @param o The Efl_Net_Server_Fd object (server).
 * @param pd The private data of the server (unused).
 * @param client The newly created client object. Must implement Efl.Net.Socket.
 * @return EINA_TRUE if the client was successfully announced and handled,
 *         EINA_FALSE otherwise (e.g., wrong type, wrong parent, or closed/unhandled).
 */
static Eina_Bool
_efl_net_server_fd_efl_net_server_client_announce(Eo *o, Efl_Net_Server_Fd_Data *pd EINA_UNUSED, Eo *client)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_GOTO(efl_isa(client, EFL_NET_SOCKET_INTERFACE), wrong_type);
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

   efl_net_server_clients_count_set(o, efl_net_server_clients_count_get(o) + 1);
   efl_event_callback_add(client, EFL_IO_CLOSER_EVENT_CLOSED, _efl_net_server_fd_client_event_closed, o);
   return EINA_TRUE;

 wrong_type:
   ERR("%p client %p (%s) doesn't implement Efl.Net.Socket interface, deleting it.", o, client, efl_class_name_get(efl_class_get(client)));
   efl_io_closer_close(client);
   efl_del(client);
   return EINA_FALSE;

 wrong_parent:
   ERR("%p client %p (%s) parent=%p is not our child, deleting it.", o, client, efl_class_name_get(efl_class_get(client)), efl_parent_get(client));
   efl_io_closer_close(client);
   efl_del(client);
   return EINA_FALSE;
}

#include "efl_net_server_fd.eo.c"
