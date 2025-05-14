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

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif

#ifdef _WIN32
# include <ws2tcpip.h>
#endif

#include "Ecore.h"
#include "ecore_private.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

#define ECORE_CON_SOCKS_VERSION_CHECK(X)             do {    \
       if (!(X) || ((X)->version < 4) || ((X)->version > 5)) \
         return;                                             \
  } while (0)
#define ECORE_CON_SOCKS_VERSION_CHECK_RETURN(X, ret) do {    \
       if (!(X) || ((X)->version < 4) || ((X)->version > 5)) \
         return (ret);                                       \
  } while (0)

/**
 * @internal
 * @brief List of all configured SOCKS proxies.
 * This list stores pointers to Ecore_Con_Socks (or its versioned variants like
 * Ecore_Con_Socks_v5) structures.
 */
static Eina_List *ecore_con_socks_proxies = NULL;

/**
 * @internal
 * @brief Finds an existing SOCKS proxy configuration.
 *
 * This function searches the internal list of SOCKS proxies for a match
 * based on version, IP, port, username, and password (for SOCKSv5).
 *
 * @param version The SOCKS protocol version (4 or 5).
 * @param ip The IP address of the SOCKS proxy server.
 * @param port The port number of the SOCKS proxy server. Can be -1 to match any port.
 * @param username The username for authentication (if any).
 * @param ulen The length of the username.
 * @param password The password for SOCKSv5 authentication (if any).
 * @param plen The length of the password.
 * @return A pointer to the found Ecore_Con_Socks structure, or NULL if not found.
 */
static Ecore_Con_Socks *
_ecore_con_socks_find(unsigned char version, const char *ip, int port, const char *username, size_t ulen, const char *password, size_t plen)
{
   Eina_List *l;
   Ecore_Con_Socks_v5 *ecs;

   if (!ecore_con_socks_proxies) return NULL;

   EINA_LIST_FOREACH(ecore_con_socks_proxies, l, ecs)
     {
        if (ecs->version != version) continue;
        if (strcmp(ecs->ip, ip)) continue;
        if ((port != -1) && (port != ecs->port)) continue;
        if (ulen != ecs->ulen) continue;
        if (username && strcmp(ecs->username, username)) continue;
        if (version == 5)
          {
             if (plen != ecs->plen) continue;
             if (password && strcmp(ecs->password, password)) continue;
          }
        return (Ecore_Con_Socks *)ecs;
     }
   return NULL;
}

/**
 * @internal
 * @brief Frees a SOCKS proxy configuration.
 *
 * This function releases the memory associated with an Ecore_Con_Socks
 * structure, including its stringshared members. It also ensures that
 * if this proxy was set as a one-time or global proxy, those references
 * are cleared.
 *
 * @param ecs The SOCKS proxy configuration to free.
 */
static void
_ecore_con_socks_free(Ecore_Con_Socks *ecs)
{
   ECORE_CON_SOCKS_VERSION_CHECK(ecs);

   if (_ecore_con_proxy_once == ecs) _ecore_con_proxy_once = NULL;
   if (_ecore_con_proxy_global == ecs) _ecore_con_proxy_global = NULL;
   eina_stringshare_del(ecs->ip);
   eina_stringshare_del(ecs->username);
   free(ecs);
}

/**
 * @brief Shuts down the SOCKS proxy subsystem.
 *
 * This function frees all configured SOCKS proxies and resets any
 * global or one-time proxy settings. It should be called during
 * Ecore_Con shutdown.
 */
void
ecore_con_socks_shutdown(void)
{
   Ecore_Con_Socks *ecs;
   EINA_LIST_FREE(ecore_con_socks_proxies, ecs)
     _ecore_con_socks_free(ecs);
   _ecore_con_proxy_once = NULL;
   _ecore_con_proxy_global = NULL;
}

/**
 * @brief Initializes the SOCKS proxy subsystem.
 *
 * This function attempts to read SOCKS proxy configuration from
 * environment variables (ECORE_CON_SOCKS_V4 or ECORE_CON_SOCKS_V5).
 * If a valid configuration is found, it adds and applies it as a global proxy.
 * The environment variable format is:
 * ECORE_CON_SOCKS_V4=[user@]host-port:[1|0]
 * ECORE_CON_SOCKS_V5=[user@]host-port:[1|0]
 * where the final [1|0] indicates whether DNS lookups should be performed
 * by the proxy (1) or locally (0).
 *
 * This function is typically called during Ecore_Con initialization.
 */
void
ecore_con_socks_init(void)
{
   const char *socks = NULL;
   char *h, *p, *l, *u = NULL;
   char buf[512];
   int port, lookup = 0;
   Eina_Bool v5 = EINA_FALSE;
   Ecore_Con_Socks *ecs;
   unsigned char addr[sizeof(struct in_addr)];
#ifdef HAVE_IPV6
   unsigned char addr6[sizeof(struct in6_addr)];
#endif

#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   if (getuid() == geteuid())
#endif
     {
        /* ECORE_CON_SOCKS_V4=[user@]host-port:[1|0] */
        socks = getenv("ECORE_CON_SOCKS_V4");
        if (!socks)
          {
             /* ECORE_CON_SOCKS_V5=[user@]host-port:[1|0] */
             socks = getenv("ECORE_CON_SOCKS_V5");
             v5 = EINA_TRUE;
          }
     }
   if ((!socks) || (!socks[0]) || (strlen(socks) + 1 > 512)) return;
   memcpy(buf, socks, strlen(socks) + 1);
   h = strchr(buf, '@');
   /* username */
   if (h && (h - buf > 0)) *h++ = 0, u = buf;
   else h = buf;

   /* host ip; I ain't resolvin shit here */
   p = strchr(h, '-');
   if (!p) return;
   *p++ = 0;
   if (!inet_pton(AF_INET, h, addr))
#ifdef HAVE_IPV6
     {
        if (!v5) return;
        if (!inet_pton(AF_INET6, h, addr6))
          return;
     }
#else
     return;
#endif

   errno = 0;
   port = strtol(p, &l, 10);
   if (errno || (port < 0) || (port > 65535)) return;
   if (l && (l[0] == ':'))
     lookup = (l[1] == '1');
   if (v5)
     ecs = ecore_con_socks5_remote_add(h, port, u, NULL);
   else
     ecs = ecore_con_socks4_remote_add(h, port, u);
   if (!ecs) return;
   ecore_con_socks_lookup_set(ecs, lookup);
   ecore_con_socks_apply_always(ecs);
   INF("Added global proxy server %s%s%s:%d - DNS lookup %s",
       u ? : "", u ? "@" : "", h, port, lookup ? "ENABLED" : "DISABLED");
}

/////////////////////////////////////////////////////////////////////////////////////

/*
 * General Socks API.
 */

/**
 * @brief Adds a SOCKSv4 proxy server configuration.
 *
 * If a proxy with the same IP, port, and username already exists,
 * a pointer to the existing configuration is returned. Otherwise, a new
 * configuration is created and added to the internal list.
 *
 * @param ip The IP address of the SOCKSv4 proxy server. Must not be NULL or empty.
 * @param port The port number of the SOCKSv4 proxy server (0-65535).
 * @param username Optional username for SOCKSv4 authentication (max 255 chars).
 *                 Can be NULL if no username is required.
 * @return A pointer to the Ecore_Con_Socks structure representing the proxy,
 *         or NULL on failure (e.g., invalid parameters).
 */
ECORE_CON_API Ecore_Con_Socks *
ecore_con_socks4_remote_add(const char *ip, int port, const char *username)
{
   Ecore_Con_Socks *ecs;
   size_t ulen = 0;

   if ((!ip) || (!ip[0]) || (port < 0) || (port > 65535)) return NULL;

   if (username)
     {
        ulen = strlen(username);
        /* max length for protocol */
        if ((!ulen) || (ulen > 255)) return NULL;
     }
   ecs = _ecore_con_socks_find(4, ip, port, username, ulen, NULL, 0);
   if (ecs) return ecs;

   ecs = calloc(1, sizeof(Ecore_Con_Socks_v4));
   if (!ecs) return NULL;

   ecs->version = 4;
   ecs->ip = eina_stringshare_add(ip);
   ecs->port = port;
   ecs->username = eina_stringshare_add(username);
   ecs->ulen = ulen;
   ecore_con_socks_proxies = eina_list_append(ecore_con_socks_proxies, ecs);
   return ecs;
}

/**
 * @brief Checks if a SOCKSv4 proxy server configuration exists.
 *
 * @param ip The IP address of the SOCKSv4 proxy server. Must not be NULL or empty.
 * @param port The port number of the SOCKSv4 proxy server.
 *             Use -1 to match any port for the given IP and username.
 * @param username Optional username for SOCKSv4 authentication.
 *                 If NULL, checks for proxies without a username.
 *                 If an empty string, it's considered an invalid parameter.
 * @return EINA_TRUE if the proxy configuration exists, EINA_FALSE otherwise or on invalid input.
 */
ECORE_CON_API Eina_Bool
ecore_con_socks4_remote_exists(const char *ip, int port, const char *username)
{
   if ((!ip) || (!ip[0]) || (port < -1) || (port > 65535) || (username && (!username[0])))
     return EINA_FALSE;
   return !!_ecore_con_socks_find(4, ip, port, username, username ? strlen(username) : 0, NULL, 0);
}

/**
 * @brief Deletes a SOCKSv4 proxy server configuration.
 *
 * Removes the specified SOCKSv4 proxy from the internal list and frees
 * its associated resources.
 *
 * @param ip The IP address of the SOCKSv4 proxy server. Must not be NULL or empty.
 * @param port The port number of the SOCKSv4 proxy server.
 *             Use -1 to match any port for the given IP and username.
 * @param username Optional username for SOCKSv4 authentication.
 *                 If NULL, matches proxies without a username.
 *                 If an empty string, it's considered an invalid parameter and the function returns.
 */
ECORE_CON_API void
ecore_con_socks4_remote_del(const char *ip, int port, const char *username)
{
   Ecore_Con_Socks_v4 *v4;

   if ((!ip) || (!ip[0]) || (port < -1) || (port > 65535) || (username && (!username[0]))) return;
   if (!ecore_con_socks_proxies) return;

   v4 = (Ecore_Con_Socks_v4 *)_ecore_con_socks_find(4, ip, port, username, username ? strlen(username) : 0, NULL, 0);
   if (!v4) return;
   ecore_con_socks_proxies = eina_list_remove(ecore_con_socks_proxies, v4);
   _ecore_con_socks_free((Ecore_Con_Socks *)v4);
}

/**
 * @brief Adds a SOCKSv5 proxy server configuration.
 *
 * If a proxy with the same IP, port, username, and password already exists,
 * a pointer to the existing configuration is returned. Otherwise, a new
 * configuration is created and added to the internal list.
 *
 * @param ip The IP address of the SOCKSv5 proxy server. Must not be NULL or empty.
 * @param port The port number of the SOCKSv5 proxy server (0-65535).
 * @param username Optional username for SOCKSv5 authentication (max 255 chars).
 *                 Can be NULL if no username is required.
 * @param password Optional password for SOCKSv5 authentication (max 255 chars).
 *                 Can be NULL if no password is required.
 *                 Required if username is provided.
 * @return A pointer to the Ecore_Con_Socks structure (cast from Ecore_Con_Socks_v5)
 *         representing the proxy, or NULL on failure (e.g., invalid parameters).
 */
ECORE_CON_API Ecore_Con_Socks *
ecore_con_socks5_remote_add(const char *ip, int port, const char *username, const char *password)
{
   Ecore_Con_Socks_v5 *ecs5;
   size_t ulen = 0, plen = 0;

   if ((!ip) || (!ip[0]) || (port < 0) || (port > 65535)) return NULL;

   if (username)
     {
        ulen = strlen(username);
        /* max length for protocol */
        if ((!ulen) || (ulen > 255)) return NULL;
     }
   if (password)
     {
        plen = strlen(password);
        /* max length for protocol */
        if ((!plen) || (plen > 255)) return NULL;
     }
   ecs5 = (Ecore_Con_Socks_v5 *)_ecore_con_socks_find(5, ip, port, username, ulen, password, plen);
   if (ecs5) return (Ecore_Con_Socks *)ecs5;

   ecs5 = calloc(1, sizeof(Ecore_Con_Socks_v5));
   if (!ecs5) return NULL;

   ecs5->version = 5;
   ecs5->ip = eina_stringshare_add(ip);
   ecs5->port = port;
   ecs5->username = eina_stringshare_add(username);
   ecs5->ulen = ulen;
   ecs5->password = eina_stringshare_add(password);
   ecs5->plen = plen;
   ecore_con_socks_proxies = eina_list_append(ecore_con_socks_proxies, ecs5);
   return (Ecore_Con_Socks *)ecs5;
}

/**
 * @brief Checks if a SOCKSv5 proxy server configuration exists.
 *
 * @param ip The IP address of the SOCKSv5 proxy server. Must not be NULL or empty.
 * @param port The port number of the SOCKSv5 proxy server.
 *             Use -1 to match any port for the given IP, username, and password.
 * @param username Optional username for SOCKSv5 authentication.
 *                 If NULL, checks for proxies without authentication.
 *                 If an empty string, it's considered an invalid parameter.
 * @param password Optional password for SOCKSv5 authentication.
 *                 If NULL, checks for proxies without a password (relevant if username is also NULL).
 *                 If an empty string, it's considered an invalid parameter.
 * @return EINA_TRUE if the proxy configuration exists, EINA_FALSE otherwise or on invalid input.
 */
ECORE_CON_API Eina_Bool
ecore_con_socks5_remote_exists(const char *ip, int port, const char *username, const char *password)
{
   if ((!ip) || (!ip[0]) || (port < -1) || (port > 65535) || (username && (!username[0])) || (password && (!password[0])))
     return EINA_FALSE;
   return !!_ecore_con_socks_find(5, ip, port, username, username ? strlen(username) : 0, password, password ? strlen(password) : 0);
}

/**
 * @brief Deletes a SOCKSv5 proxy server configuration.
 *
 * Removes the specified SOCKSv5 proxy from the internal list and frees
 * its associated resources.
 *
 * @param ip The IP address of the SOCKSv5 proxy server. Must not be NULL or empty.
 * @param port The port number of the SOCKSv5 proxy server.
 *             Use -1 to match any port for the given IP, username, and password.
 * @param username Optional username for SOCKSv5 authentication.
 *                 If NULL, matches proxies without authentication.
 *                 If an empty string, it's considered an invalid parameter and the function returns.
 * @param password Optional password for SOCKSv5 authentication.
 *                 If NULL, matches proxies without a password (relevant if username is also NULL).
 *                 If an empty string, it's considered an invalid parameter and the function returns.
 */
ECORE_CON_API void
ecore_con_socks5_remote_del(const char *ip, int port, const char *username, const char *password)
{
   Ecore_Con_Socks_v5 *v5;

   if ((!ip) || (!ip[0]) || (port < -1) || (port > 65535) || (username && (!username[0])) || (password && (!password[0])))
     return;
   if (!ecore_con_socks_proxies) return;

   v5 = (Ecore_Con_Socks_v5 *)_ecore_con_socks_find(5, ip, port, username, username ? strlen(username) : 0, password, password ? strlen(password) : 0);
   if (!v5) return;
   ecore_con_socks_proxies = eina_list_remove(ecore_con_socks_proxies, v5);
   _ecore_con_socks_free((Ecore_Con_Socks *)v5);
}

/**
 * @brief Sets whether DNS lookups should be performed by the SOCKS proxy.
 *
 * For SOCKSv4a and SOCKSv5, the proxy can resolve hostnames.
 * If enabled, hostnames are sent to the proxy for resolution.
 * If disabled, Ecore_Con resolves hostnames locally before connecting to the proxy.
 *
 * @param ecs The SOCKS proxy configuration.
 * @param enable EINA_TRUE to enable proxy-side DNS lookup, EINA_FALSE for local lookup.
 */
ECORE_CON_API void
ecore_con_socks_lookup_set(Ecore_Con_Socks *ecs, Eina_Bool enable)
{
   ECORE_CON_SOCKS_VERSION_CHECK(ecs);
   ecs->lookup = !!enable;
}

/**
 * @brief Gets whether DNS lookups are performed by the SOCKS proxy.
 *
 * @param ecs The SOCKS proxy configuration.
 * @return EINA_TRUE if proxy-side DNS lookup is enabled, EINA_FALSE otherwise.
 *         Returns EINA_FALSE if ecs is NULL or invalid.
 */
ECORE_CON_API Eina_Bool
ecore_con_socks_lookup_get(Ecore_Con_Socks *ecs)
{
   ECORE_CON_SOCKS_VERSION_CHECK_RETURN(ecs, EINA_FALSE);
   return ecs->lookup;
}

/**
 * @brief Sets whether the SOCKS connection is for a BIND operation.
 *
 * SOCKS protocol supports a BIND command, which is used for scenarios
 * like FTP where the server connects back to the client.
 *
 * @param ecs The SOCKS proxy configuration.
 * @param is_bind EINA_TRUE if this is for a BIND operation, EINA_FALSE for CONNECT.
 */
ECORE_CON_API void
ecore_con_socks_bind_set(Ecore_Con_Socks *ecs, Eina_Bool is_bind)
{
   EINA_SAFETY_ON_NULL_RETURN(ecs);
   ECORE_CON_SOCKS_VERSION_CHECK(ecs);
   ecs->bind = !!is_bind;
}

/**
 * @brief Gets whether the SOCKS connection is for a BIND operation.
 *
 * @param ecs The SOCKS proxy configuration.
 * @return EINA_TRUE if this is for a BIND operation, EINA_FALSE otherwise.
 *         Returns EINA_FALSE if ecs is NULL or invalid.
 */
ECORE_CON_API Eina_Bool
ecore_con_socks_bind_get(Ecore_Con_Socks *ecs)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ecs, EINA_FALSE);
   ECORE_CON_SOCKS_VERSION_CHECK_RETURN(ecs, EINA_FALSE);
   return ecs->bind;
}

/**
 * @brief Gets the SOCKS protocol version of the proxy configuration.
 *
 * @param ecs The SOCKS proxy configuration.
 * @return The SOCKS protocol version (4 or 5), or 0 if ecs is NULL or invalid.
 */
ECORE_CON_API unsigned int
ecore_con_socks_version_get(Ecore_Con_Socks *ecs)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ecs, 0);
   ECORE_CON_SOCKS_VERSION_CHECK_RETURN(ecs, 0);
   return ecs->version;
}

/**
 * @brief Deletes a SOCKS proxy server configuration using its handle.
 *
 * Removes the specified SOCKS proxy from the internal list and frees
 * its associated resources. This is an alternative to deleting by
 * IP/port/credentials.
 *
 * @param ecs The SOCKS proxy configuration handle to delete.
 */
ECORE_CON_API void
ecore_con_socks_remote_del(Ecore_Con_Socks *ecs)
{
   EINA_SAFETY_ON_NULL_RETURN(ecs);
   if (!ecore_con_socks_proxies) return;

   ecore_con_socks_proxies = eina_list_remove(ecore_con_socks_proxies, ecs);
   _ecore_con_socks_free(ecs);
}

/**
 * @brief Applies a SOCKS proxy configuration for the next connection only.
 *
 * This sets a one-time SOCKS proxy. The next Ecore_Con connection attempt
 * will use this proxy, and then this setting will be cleared.
 * If a global proxy is also set, this one-time proxy takes precedence.
 *
 * @param ecs The SOCKS proxy configuration to apply once. Can be NULL to clear
 *            a previously set one-time proxy.
 */
ECORE_CON_API void
ecore_con_socks_apply_once(Ecore_Con_Socks *ecs)
{
   _ecore_con_proxy_once = ecs;
}

/**
 * @brief Applies a SOCKS proxy configuration globally for all subsequent connections.
 *
 * This sets a global SOCKS proxy. All subsequent Ecore_Con connection
 * attempts will use this proxy unless a one-time proxy is also set
 * (which would take precedence for that single connection).
 *
 * @param ecs The SOCKS proxy configuration to apply globally. Can be NULL to
 *            disable the global proxy.
 */
ECORE_CON_API void
ecore_con_socks_apply_always(Ecore_Con_Socks *ecs)
{
   _ecore_con_proxy_global = ecs;
}

