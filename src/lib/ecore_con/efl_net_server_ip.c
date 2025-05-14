#define EFL_NET_SERVER_FD_PROTECTED 1
#define EFL_NET_SERVER_PROTECTED 1
#define EFL_LOOP_FD_PROTECTED 1

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

#define MY_CLASS EFL_NET_SERVER_IP_CLASS

/**
 * @brief Data structure for Efl_Net_Server_Ip.
 * @since 1.21
 */
typedef struct _Efl_Net_Server_Ip_Data
{
   Eina_Bool ipv6_only; /**< If true, the server will only accept IPv6 connections. Set to 0xff (EINA_TRUE) initially to indicate it hasn't been explicitly set by the user or successfully queried from the OS. */
} Efl_Net_Server_Ip_Data;

/**
 * @brief Constructor for Efl_Net_Server_Ip.
 *
 * @param o The Eolian object.
 * @param pd The private data for the Eolian object.
 * @return The constructed Eolian object.
 *
 * @internal
 */
EOLIAN Efl_Object *
_efl_net_server_ip_efl_object_constructor(Eo *o, Efl_Net_Server_Ip_Data *pd)
{
   pd->ipv6_only = 0xff;
   return efl_constructor(efl_super(o, MY_CLASS));
}

/**
 * @brief Sets whether the server should only accept IPv6 connections.
 *
 * This function attempts to set the IPV6_V6ONLY socket option.
 * If the socket is not valid or not an IPv6 socket, this function does nothing.
 * After attempting to set the option, it queries the socket to update the
 * internal state.
 *
 * @param o The Eolian object.
 * @param pd The private data for the Eolian object.
 * @param ipv6_only EINA_TRUE to enable IPv6 only, EINA_FALSE otherwise.
 *
 * @internal
 */
EOLIAN void
_efl_net_server_ip_ipv6_only_set(Eo *o, Efl_Net_Server_Ip_Data *pd, Eina_Bool ipv6_only)
{
#ifdef IPV6_V6ONLY
   SOCKET fd = efl_loop_fd_get(o);
#ifdef _WIN32
   DWORD value = ipv6_only;
   int valuelen;
#else
   int value = ipv6_only;
   socklen_t valuelen;
#endif
#endif

#ifdef IPV6_V6ONLY
   if (fd == INVALID_SOCKET) return;
   if (efl_net_server_fd_family_get(o) != AF_INET6) return;

   if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&value, sizeof(value)) != 0)
     {
        ERR("could not set socket=" SOCKET_FMT " IPV6_V6ONLY=%d: %s", fd, (int)value, eina_error_msg_get(efl_net_socket_error_get()));
     }

   valuelen = sizeof(value);
   if (getsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&value, &valuelen) != 0)
     {
        WRN("getsockopt(" SOCKET_FMT ", IPPROTO_IPV6, IPV6_V6ONLY): %s", fd, eina_error_msg_get(efl_net_socket_error_get()));
        return ;
     }
   pd->ipv6_only = !!value;
#endif
}

/**
 * @brief Gets whether the server is set to only accept IPv6 connections.
 *
 * @param o The Eolian object (unused).
 * @param pd The private data for the Eolian object.
 * @return EINA_TRUE if IPv6 only is enabled, EINA_FALSE otherwise.
 *         Returns 0xff (EINA_TRUE) if the value has not been successfully set or queried.
 *
 * @internal
 */
EOLIAN Eina_Bool
_efl_net_server_ip_ipv6_only_get(const Eo *o EINA_UNUSED, Efl_Net_Server_Ip_Data *pd)
{
   return pd->ipv6_only;
}

#include "efl_net_server_ip.eo.c"
