#define EFL_NET_SOCKET_TCP_PROTECTED 1
#define EFL_NET_SOCKET_FD_PROTECTED 1
#define EFL_LOOP_FD_PROTECTED 1
#define EFL_IO_READER_FD_PROTECTED 1
#define EFL_IO_WRITER_FD_PROTECTED 1
#define EFL_IO_CLOSER_FD_PROTECTED 1
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

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#define MY_CLASS EFL_NET_SOCKET_TCP_CLASS

/**
 * @brief Private data for the Efl_Net_Socket_Tcp class.
 */
typedef struct _Efl_Net_Socket_Tcp_Data
{
   Eina_Bool keep_alive; /**< Whether SO_KEEPALIVE is enabled. */
   Eina_Bool no_delay;   /**< Whether TCP_NODELAY is enabled (Nagle's algorithm disabled). */
   Eina_Bool cork;       /**< Whether TCP_CORK or TCP_NOPUSH is enabled. */
} Efl_Net_Socket_Tcp_Data;

/**
 * @brief Sets the file descriptor for the TCP socket and applies pending options.
 *
 * This function is called when the underlying file descriptor for the socket
 * is set. It then proceeds to apply any socket options (like keep_alive,
 * no_delay, cork) that were configured before the fd was available.
 * It also retrieves and sets the local and remote socket addresses.
 *
 * @param o The Efl_Net_Socket_Tcp object.
 * @param pd Private data for the Efl_Net_Socket_Tcp object.
 * @param pfd The new file descriptor.
 */
EOLIAN static void
_efl_net_socket_tcp_efl_loop_fd_fd_set(Eo *o, Efl_Net_Socket_Tcp_Data *pd, int pfd)
{
   SOCKET fd = (SOCKET)pfd;

   efl_loop_fd_set(efl_super(o, MY_CLASS), pfd);

   if (fd != INVALID_SOCKET)
     {
        struct sockaddr_storage addr;
        socklen_t addrlen;
        int family;

        /* apply postponed values */
        efl_net_socket_tcp_keep_alive_set(o, pd->keep_alive);
        efl_net_socket_tcp_no_delay_set(o, pd->no_delay);
        efl_net_socket_tcp_cork_set(o, pd->cork);

        family = efl_net_socket_fd_family_get(o);
        if (family == AF_UNSPEC) return;

        addrlen = sizeof(addr);
        if (getsockname(fd, (struct sockaddr *)&addr, &addrlen) != 0)
          ERR("getsockname(" SOCKET_FMT "): %s", fd, eina_error_msg_get(efl_net_socket_error_get()));
        else
          {
             char str[INET6_ADDRSTRLEN + sizeof("[]:65536")];
             if (efl_net_ip_port_fmt(str, sizeof(str), (struct sockaddr *)&addr))
               efl_net_socket_address_local_set(o, str);
          }

        addrlen = sizeof(addr);
        if (getpeername(fd, (struct sockaddr *)&addr, &addrlen) != 0)
          ERR("getpeername(" SOCKET_FMT "): %s", fd, eina_error_msg_get(efl_net_socket_error_get()));
        else
          {
             char str[INET6_ADDRSTRLEN + sizeof("[]:65536")];
             if (efl_net_ip_port_fmt(str, sizeof(str), (struct sockaddr *)&addr))
               efl_net_socket_address_remote_set(o, str);
          }
     }
}

/**
 * @brief Sets the SO_KEEPALIVE socket option.
 *
 * If the socket fd is not yet set, the value is stored and applied later
 * when the fd becomes available.
 *
 * @param o The Efl_Net_Socket_Tcp object.
 * @param pd Private data for the Efl_Net_Socket_Tcp object.
 * @param keep_alive EINA_TRUE to enable keep-alive, EINA_FALSE to disable.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_net_socket_tcp_keep_alive_set(Eo *o, Efl_Net_Socket_Tcp_Data *pd, Eina_Bool keep_alive)
{
   SOCKET fd;
   Eina_Bool old = pd->keep_alive;
#ifdef _WIN32
   DWORD value;
#else
   int value;
#endif

   pd->keep_alive = keep_alive;

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return EINA_TRUE; /* postpone until fd_set() */

   value = keep_alive;
   if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&value, sizeof(value)) != 0)
     {
        ERR("setsockopt(" SOCKET_FMT ", SOL_SOCKET, SO_KEEPALIVE, %d): %s",
            fd, (int)value, eina_error_msg_get(efl_net_socket_error_get()));
        pd->keep_alive = old;
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Gets the SO_KEEPALIVE socket option.
 *
 * If the socket fd is not yet set, the stored value is returned. Otherwise,
 * the current value of the option is queried from the system.
 *
 * @param o The Efl_Net_Socket_Tcp object.
 * @param pd Private data for the Efl_Net_Socket_Tcp object.
 * @return EINA_TRUE if keep-alive is enabled, EINA_FALSE otherwise or on error.
 */
EOLIAN static Eina_Bool
_efl_net_socket_tcp_keep_alive_get(const Eo *o, Efl_Net_Socket_Tcp_Data *pd)
{
   SOCKET fd;
#ifdef _WIN32
   DWORD value = 0;
   int valuelen;
#else
   int value = 0;
   socklen_t valuelen;
#endif

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return pd->keep_alive;

   /* if there is a fd, always query it directly as it may be modified
    * elsewhere by nasty users.
    */
   valuelen = sizeof(value);
   if (getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&value, &valuelen) != 0)
     {
        ERR("getsockopt(" SOCKET_FMT ", SOL_SOCKET, SO_KEEPALIVE): %s",
            fd, eina_error_msg_get(efl_net_socket_error_get()));
        return EINA_FALSE;
     }

   pd->keep_alive = !!value; /* sync */
   return pd->keep_alive;
}

/**
 * @brief Sets the TCP_NODELAY socket option.
 *
 * Enabling TCP_NODELAY disables Nagle's algorithm.
 * If the socket fd is not yet set, the value is stored and applied later
 * when the fd becomes available.
 *
 * @param o The Efl_Net_Socket_Tcp object.
 * @param pd Private data for the Efl_Net_Socket_Tcp object.
 * @param no_delay EINA_TRUE to enable TCP_NODELAY (disable Nagle's), EINA_FALSE to disable.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_net_socket_tcp_no_delay_set(Eo *o, Efl_Net_Socket_Tcp_Data *pd, Eina_Bool no_delay)
{
   SOCKET fd;
   Eina_Bool old = pd->no_delay;
#ifdef _WIN32
   BOOL value;
#else
   int value;
#endif

   pd->no_delay = no_delay;

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return EINA_TRUE; /* postpone until fd_set() */

   value = no_delay;
   if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&value, sizeof(value)) != 0)
     {
        ERR("setsockopt(" SOCKET_FMT ", IPPROTO_TCP, TCP_NODELAY, %d): %s",
            fd, value, eina_error_msg_get(efl_net_socket_error_get()));
        pd->no_delay = old;
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Gets the TCP_NODELAY socket option.
 *
 * If the socket fd is not yet set, the stored value is returned. Otherwise,
 * the current value of the option is queried from the system.
 *
 * @param o The Efl_Net_Socket_Tcp object.
 * @param pd Private data for the Efl_Net_Socket_Tcp object.
 * @return EINA_TRUE if TCP_NODELAY is enabled, EINA_FALSE otherwise or on error.
 */
EOLIAN static Eina_Bool
_efl_net_socket_tcp_no_delay_get(const Eo *o, Efl_Net_Socket_Tcp_Data *pd)
{
   SOCKET fd;
#ifdef _WIN32
   BOOL value;
   int valuelen;
#else
   int value;
   socklen_t valuelen;
#endif

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return pd->no_delay;

   /* if there is a fd, always query it directly as it may be modified
    * elsewhere by nasty users.
    */
   valuelen = sizeof(value);
   if (getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&value, &valuelen) != 0)
     {
        ERR("getsockopt(" SOCKET_FMT ", IPPROTO_TCP, TCP_NODELAY): %s",
            fd, eina_error_msg_get(efl_net_socket_error_get()));
        return EINA_FALSE;
     }

   pd->no_delay = !!value; /* sync */
   return pd->no_delay;
}

/**
 * @brief Gets the appropriate socket option value for TCP_CORK or TCP_NOPUSH.
 *
 * This function checks for the availability of TCP_CORK (Linux) or
 * TCP_NOPUSH (BSD/macOS) and returns the corresponding integer value
 * for use with setsockopt/getsockopt.
 *
 * @return The socket option value (e.g., TCP_CORK, TCP_NOPUSH) or -1 if neither is available.
 */
static inline int
_cork_option_get(void)
{
#if defined(HAVE_TCP_CORK)
   return TCP_CORK;
#elif defined(HAVE_TCP_NOPUSH)
   return TCP_NOPUSH;
#else
   return -1;
#endif
}

/**
 * @brief Sets the TCP_CORK or TCP_NOPUSH socket option.
 *
 * This option prevents partial frames from being sent, gathering small packets
 * into a single larger packet before transmission.
 * If the socket fd is not yet set, the value is stored and applied later
 * when the fd becomes available.
 *
 * @param o The Efl_Net_Socket_Tcp object.
 * @param pd Private data for the Efl_Net_Socket_Tcp object.
 * @param cork EINA_TRUE to enable corking/nopush, EINA_FALSE to disable.
 * @return EINA_TRUE on success, EINA_FALSE on failure or if the option is not supported.
 */
EOLIAN static Eina_Bool
_efl_net_socket_tcp_cork_set(Eo *o, Efl_Net_Socket_Tcp_Data *pd, Eina_Bool cork)
{
   SOCKET fd;
   int value, option;
   Eina_Bool old = pd->cork;

   option = _cork_option_get();
   if (EINA_UNLIKELY(option < 0))
     {
        if (cork)
          ERR("Could not find a TCP_CORK equivalent on your system");
        return EINA_FALSE;
     }

   pd->cork = cork;

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return EINA_TRUE; /* postpone until fd_set() */

   value = cork;
   if (setsockopt(fd, IPPROTO_TCP, option, (const char *)&value, sizeof(value)) != 0)
     {
        ERR("setsockopt(" SOCKET_FMT ", IPPROTO_TCP, 0x%x, %d): %s",
            fd, option, value, eina_error_msg_get(efl_net_socket_error_get()));
        pd->cork = old;
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Gets the TCP_CORK or TCP_NOPUSH socket option.
 *
 * If the socket fd is not yet set, the stored value is returned. Otherwise,
 * the current value of the option is queried from the system.
 *
 * @param o The Efl_Net_Socket_Tcp object.
 * @param pd Private data for the Efl_Net_Socket_Tcp object.
 * @return EINA_TRUE if corking/nopush is enabled, EINA_FALSE otherwise, on error, or if not supported.
 */
EOLIAN static Eina_Bool
_efl_net_socket_tcp_cork_get(const Eo *o, Efl_Net_Socket_Tcp_Data *pd)
{
   SOCKET fd;
   int value = 0;
   socklen_t valuelen;
   int option;

   option = _cork_option_get();
   if (EINA_UNLIKELY(option < 0))
     {
        WRN("Could not find a TCP_CORK equivalent on your system");
        return EINA_FALSE;
     }

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return pd->cork;

   /* if there is a fd, always query it directly as it may be modified
    * elsewhere by nasty users.
    */
   valuelen = sizeof(value);
   if (getsockopt(fd, IPPROTO_TCP, option, (char *)&value, &valuelen) != 0)
     {
        ERR("getsockopt(" SOCKET_FMT ", IPPROTO_TCP, 0x%x): %s",
            fd, option, eina_error_msg_get(efl_net_socket_error_get()));
        return EINA_FALSE;
     }

   pd->cork = !!value; /* sync */
   return pd->cork;
}

#include "efl_net_socket_tcp.eo.c"
