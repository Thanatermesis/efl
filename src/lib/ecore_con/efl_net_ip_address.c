/**
 * @file
 * @brief This file implements the Efl.Net.Ip_Address interface, providing
 *        functionality for handling IP addresses (both IPv4 and IPv6).
 *        It includes operations such as creating, parsing, formatting,
 *        and resolving IP addresses.
 */

#define EFL_NET_IP_ADDRESS_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

/**
 * @brief Private data structure for Efl_Net_Ip_Address objects.
 *
 * This structure holds the internal representation of an IP address,
 * including its string form, the sockaddr union for IPv4/IPv6,
 * and a slice pointing to the raw address bytes.
 */
typedef struct _Efl_Net_Ip_Address_Data {
   char string[INET6_ADDRSTRLEN + sizeof("[]:65536")]; /**< String representation of the IP address (e.g., "192.168.1.1:80", "[::1]:8080"). */
   union {
      struct sockaddr addr; /**< Generic socket address structure. */
      struct sockaddr_in ipv4;  /**< IPv4 socket address structure. */
      struct sockaddr_in6 ipv6; /**< IPv6 socket address structure. */
   };
   Eina_Slice addr_slice; /**< Slice pointing to the raw IP address bytes (either ipv4.sin_addr or ipv6.sin6_addr). */
} Efl_Net_Ip_Address_Data;

/**
 * @brief Structure holding the result of an IP address resolution operation.
 *
 * This structure is used to return the results from Efl.Net.Ip_Address.resolve.
 * It contains the original requested address, the canonical name (if requested),
 * and an array of resolved Efl_Net_Ip_Address objects.
 */
typedef struct _Efl_Net_Ip_Address_Resolve_Value
{
   Eina_Stringshare *request_address; /**< The 'address' argument given to Efl.Net.Ip_Address.resolve (e.g., "example.com:80"). */
   Eina_Stringshare *canonical_name; /**< The canonical name, if it was requested in flags (e.g., "server.example.com"). */
   const Eina_Value_Array results;  /**< An Eina_Value_Array of Efl_Net_Ip_Address objects.
                                     * Example structure:
                                     * [
                                     *   (Efl_Net_Ip_Address *) "192.0.2.1:80",
                                     *   (Efl_Net_Ip_Address *) "[2001:db8::1]:80"
                                     * ]
                                     * Do not modify this array but you can keep reference
                                     * to elements using efl_ref() and efl_unref(). */
} Efl_Net_Ip_Address_Resolve_Value;

#define MY_CLASS EFL_NET_IP_ADDRESS_CLASS

/**
 * @internal
 * @brief Finalizes the Efl_Net_Ip_Address object.
 *
 * This function is called when the object is being finalized. It formats
 * the IP address into its string representation and performs safety checks.
 * If the port is 0, it removes the ":0" suffix from the string.
 *
 * @param o The Efl_Net_Ip_Address object.
 * @param pd The private data of the object.
 * @return The finalized object, or NULL on failure.
 */
EOLIAN static Eo *
_efl_net_ip_address_efl_object_finalize(Eo *o, Efl_Net_Ip_Address_Data *pd)
{
   const uint16_t *pport;

   o = efl_finalize(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(pd->addr.sa_family == 0, NULL);

   if (!efl_net_ip_port_fmt(pd->string, sizeof(pd->string), &pd->addr))
     {
        ERR("Could not format address!");
        return NULL;
     }

   if (pd->addr.sa_family == AF_INET6)
     pport = &pd->ipv6.sin6_port;
   else
     pport = &pd->ipv4.sin_port;

   if (*pport == 0) /* port == 0, no ":0" in the string */
     {
        char *d = strrchr(pd->string, ':');
        EINA_SAFETY_ON_NULL_RETURN_VAL(d, NULL);
        *d = '\0';
     }

   return o;
}

/**
 * @internal
 * @brief Gets the string representation of the IP address.
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return A pointer to the string representation (e.g., "192.168.1.1:80").
 */
EOLIAN static const char *
_efl_net_ip_address_string_get(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   return pd->string;
}

/**
 * @internal
 * @brief Sets the address family (AF_INET or AF_INET6).
 *
 * This function initializes the address family and sets up the addr_slice
 * to point to the correct address part of the sockaddr union.
 * It can only be called once when the family is not yet set.
 *
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @param family The address family to set (AF_INET or AF_INET6).
 */
EOLIAN static void
_efl_net_ip_address_family_set(Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd, int family)
{
   if (pd->addr.sa_family == family) return;
   EINA_SAFETY_ON_TRUE_RETURN(pd->addr.sa_family != 0);
   EINA_SAFETY_ON_TRUE_RETURN((family != AF_INET) && (family != AF_INET6));
   pd->addr.sa_family = family;
   if (family == AF_INET6)
     {
        pd->addr_slice.mem = &pd->ipv6.sin6_addr;
        pd->addr_slice.len = sizeof(pd->ipv6.sin6_addr);
     }
   else
     {
        pd->addr_slice.mem = &pd->ipv4.sin_addr;
        pd->addr_slice.len = sizeof(pd->ipv4.sin_addr);
     }
}

/**
 * @internal
 * @brief Gets the address family.
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return The address family (AF_INET or AF_INET6), or 0 if not set.
 */
EOLIAN static int
_efl_net_ip_address_family_get(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   return pd->addr.sa_family;
}

/**
 * @internal
 * @brief Sets the port number for the IP address.
 *
 * The port is converted to network byte order. This function ensures
 * that the address family is already set and that the port is not
 * set multiple times.
 *
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @param port The port number to set (e.g., 80, 443).
 */
EOLIAN static void
_efl_net_ip_address_port_set(Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd, uint16_t port)
{
   uint16_t *pport, nport = eina_htons(port);

   EINA_SAFETY_ON_TRUE_RETURN(pd->addr.sa_family == 0);
   if (pd->addr.sa_family == AF_INET6)
     pport = &pd->ipv6.sin6_port;
   else
     pport = &pd->ipv4.sin_port;

   if (*pport == nport) return;
   if (*pport)
     {
        ERR("port already set to %hu, new %hu", eina_ntohs(*pport), port);
        return;
     }

   *pport = nport;
}

/**
 * @internal
 * @brief Gets the port number of the IP address.
 *
 * The port is converted from network byte order to host byte order.
 *
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return The port number (e.g., 80, 443), or 0 if not set or family is invalid.
 */
EOLIAN static uint16_t
_efl_net_ip_address_port_get(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   const uint16_t *pport;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(pd->addr.sa_family == 0, 0);
   if (pd->addr.sa_family == AF_INET6)
     pport = &pd->ipv6.sin6_port;
   else
     pport = &pd->ipv4.sin_port;

   return eina_ntohs(*pport);
}

/**
 * @internal
 * @brief Sets the raw IP address bytes.
 *
 * This function copies the provided address bytes into the internal
 * sockaddr structure. It ensures that the address family is set,
 * the length of the provided slice matches the family, and that the
 * address is not set multiple times to a different value.
 *
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @param address An Eina_Slice containing the raw IP address bytes.
 *                For IPv4, this should be 4 bytes (e.g., `{ 0xC0, 0xA8, 0x01, 0x01 }` for 192.168.1.1).
 *                For IPv6, this should be 16 bytes.
 */
EOLIAN static void
_efl_net_ip_address_address_set(Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd, Eina_Slice address)
{
   Eina_Rw_Slice rw_slice;
   size_t i;

   EINA_SAFETY_ON_TRUE_RETURN(pd->addr.sa_family == 0);

   rw_slice.mem = (void *)pd->addr_slice.mem;
   rw_slice.len = pd->addr_slice.len;

   EINA_SAFETY_ON_TRUE_RETURN(rw_slice.len != address.len);

   if (eina_slice_compare(eina_rw_slice_slice_get(rw_slice), address) == 0)
     return;

   for (i = 0; i < rw_slice.len; i++)
     {
        if (rw_slice.bytes[i])
          {
             char old_str[INET6_ADDRSTRLEN] = "";
             char new_str[INET6_ADDRSTRLEN] = "";

             if (!inet_ntop(pd->addr.sa_family, rw_slice.mem, old_str, sizeof(old_str)))
               {
                  old_str[0] = '?';
                  old_str[1] = '\0';
               }
             if (!inet_ntop(pd->addr.sa_family, address.mem, new_str, sizeof(new_str)))
               {
                  new_str[0] = '?';
                  new_str[1] = '\0';
               }
             ERR("address already set to %s, new %s", old_str, new_str);
             return;
          }
     }

   eina_rw_slice_copy(rw_slice, address);
}

/**
 * @internal
 * @brief Gets the raw IP address bytes as an Eina_Slice.
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return An Eina_Slice pointing to the raw IP address bytes.
 *         The slice will have a length of 4 for IPv4 or 16 for IPv6.
 */
EOLIAN static Eina_Slice
_efl_net_ip_address_address_get(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   return pd->addr_slice;
}

/**
 * @internal
 * @brief Sets the IP address from a sockaddr structure.
 *
 * This function copies the data from the provided sockaddr structure
 * into the internal representation. It also sets the address family
 * and the addr_slice accordingly. It can only be called once when
 * the family is not yet set.
 *
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @param ptr A pointer to a `struct sockaddr` (either `struct sockaddr_in` or `struct sockaddr_in6`).
 */
EOLIAN static void
_efl_net_ip_address_sockaddr_set(Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd, const void *ptr)
{
   const struct sockaddr *sockaddr = ptr;

   EINA_SAFETY_ON_TRUE_RETURN(pd->addr.sa_family != 0);
   EINA_SAFETY_ON_NULL_RETURN(sockaddr);

   EINA_SAFETY_ON_TRUE_RETURN((sockaddr->sa_family != AF_INET) && (sockaddr->sa_family != AF_INET6));

   if (sockaddr->sa_family == AF_INET6)
     {
        memcpy(&pd->ipv6, sockaddr, sizeof(pd->ipv6));
        pd->addr_slice.mem = &pd->ipv6.sin6_addr;
        pd->addr_slice.len = sizeof(pd->ipv6.sin6_addr);
     }
   else
     {
        memcpy(&pd->ipv4, sockaddr, sizeof(pd->ipv4));
        pd->addr_slice.mem = &pd->ipv4.sin_addr;
        pd->addr_slice.len = sizeof(pd->ipv4.sin_addr);
     }
}

/**
 * @internal
 * @brief Gets a pointer to the internal sockaddr structure.
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return A const pointer to the `struct sockaddr` representing the IP address.
 *         This can be cast to `struct sockaddr_in` or `struct sockaddr_in6`
 *         depending on the address family.
 */
EOLIAN static const void *
_efl_net_ip_address_sockaddr_get(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   return &pd->addr;
}

/** Helper macro to get IPv4 address in host byte order. */
#define IPV4_ADDR_GET(pd) eina_ntohl(pd->ipv4.sin_addr.s_addr)

/**
 * @internal
 * @brief Checks if the IPv4 address is Class A.
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return EINA_TRUE if it's an IPv4 Class A address, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_ip_address_ipv4_class_a_check(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   return (pd->addr.sa_family == AF_INET) && IN_CLASSA(IPV4_ADDR_GET(pd));
}

/**
 * @internal
 * @brief Checks if the IPv4 address is Class B.
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return EINA_TRUE if it's an IPv4 Class B address, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_ip_address_ipv4_class_b_check(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   return (pd->addr.sa_family == AF_INET) && IN_CLASSB(IPV4_ADDR_GET(pd));
}

/**
 * @internal
 * @brief Checks if the IPv4 address is Class C.
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return EINA_TRUE if it's an IPv4 Class C address, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_ip_address_ipv4_class_c_check(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   return (pd->addr.sa_family == AF_INET) && IN_CLASSC(IPV4_ADDR_GET(pd));
}

/**
 * @internal
 * @brief Checks if the IPv4 address is Class D (multicast).
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return EINA_TRUE if it's an IPv4 Class D address, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_ip_address_ipv4_class_d_check(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   return (pd->addr.sa_family == AF_INET) && IN_CLASSD(IPV4_ADDR_GET(pd));
}

/**
 * @internal
 * @brief Checks if the IPv6 address is an IPv4-mapped address.
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return EINA_TRUE if it's an IPv4-mapped IPv6 address, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_ip_address_ipv6_v4mapped_check(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   return (pd->addr.sa_family == AF_INET6) &&
     IN6_IS_ADDR_V4MAPPED(&pd->ipv6.sin6_addr);
}

/**
 * @internal
 * @brief Checks if the IPv6 address is an IPv4-compatible address.
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return EINA_TRUE if it's an IPv4-compatible IPv6 address, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_ip_address_ipv6_v4compat_check(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   return (pd->addr.sa_family == AF_INET6) &&
     IN6_IS_ADDR_V4COMPAT(&pd->ipv6.sin6_addr);
}

/**
 * @internal
 * @brief Checks if the IPv6 address is a link-local address.
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return EINA_TRUE if it's a link-local IPv6 address, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_ip_address_ipv6_local_link_check(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   return (pd->addr.sa_family == AF_INET6) &&
     IN6_IS_ADDR_LINKLOCAL(&pd->ipv6.sin6_addr);
}

/**
 * @internal
 * @brief Checks if the IPv6 address is a site-local address.
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return EINA_TRUE if it's a site-local IPv6 address, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_ip_address_ipv6_local_site_check(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   return (pd->addr.sa_family == AF_INET6) &&
     IN6_IS_ADDR_SITELOCAL(&pd->ipv6.sin6_addr);
}

/**
 * @internal
 * @brief Checks if the IP address is a multicast address.
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return EINA_TRUE if it's a multicast address (IPv4 or IPv6), EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_ip_address_multicast_check(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   if (pd->addr.sa_family == AF_INET6)
     return IN6_IS_ADDR_MULTICAST(&pd->ipv6.sin6_addr);
   else
     return IN_MULTICAST(IPV4_ADDR_GET(pd));
}

/**
 * @internal
 * @brief Checks if the IP address is a loopback address.
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return EINA_TRUE if it's a loopback address (e.g., 127.0.0.1 or ::1), EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_ip_address_loopback_check(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   if (pd->addr.sa_family == AF_INET6)
     return IN6_IS_ADDR_LOOPBACK(&pd->ipv6.sin6_addr);
   else
     return IPV4_ADDR_GET(pd) == INADDR_LOOPBACK;
}

/**
 * @internal
 * @brief Checks if the IP address is an "any" address (0.0.0.0 or ::).
 *
 * This means all bytes of the address are zero.
 *
 * @param o The Efl_Net_Ip_Address object (unused).
 * @param pd The private data of the object.
 * @return EINA_TRUE if it's an "any" address, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_ip_address_any_check(const Eo *o EINA_UNUSED, Efl_Net_Ip_Address_Data *pd)
{
   size_t i;

   for (i = 0; i < pd->addr_slice.len; i++)
     {
        if (pd->addr_slice.bytes[i])
          return EINA_FALSE;
     }

   return i > 0;
}

/**
 * @internal
 * @brief Creates an Efl_Net_Ip_Address object from raw address bytes and port.
 *
 * @param port The port number (e.g., 80).
 * @param address An Eina_Slice containing the raw IP address bytes.
 *                Must be 4 bytes for IPv4 or 16 bytes for IPv6.
 *                Example IPv4: `{ 0xC0, 0xA8, 0x01, 0x01 }` for 192.168.1.1.
 * @return A new, referenced Efl_Net_Ip_Address object, or NULL on failure.
 */
EOLIAN static Efl_Net_Ip_Address *
_efl_net_ip_address_create(uint16_t port, const Eina_Slice address)
{
   int family;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(address.len != 4 && address.len != 16, NULL);

   if (address.len == 16)
     family = AF_INET6;
   else
     family = AF_INET;

   return efl_add_ref(EFL_NET_IP_ADDRESS_CLASS, efl_main_loop_get(),
                  efl_net_ip_address_family_set(efl_added, family),
                  efl_net_ip_address_port_set(efl_added, port),
                  efl_net_ip_address_set(efl_added, address));
}

/**
 * @internal
 * @brief Creates an Efl_Net_Ip_Address object from a sockaddr structure.
 *
 * @param ptr A pointer to a `struct sockaddr` (either `struct sockaddr_in` or `struct sockaddr_in6`).
 * @return A new, referenced Efl_Net_Ip_Address object, or NULL on failure.
 */
EOLIAN static Efl_Net_Ip_Address *
_efl_net_ip_address_create_sockaddr(const void *ptr)
{
   const struct sockaddr *sockaddr = ptr;

   EINA_SAFETY_ON_NULL_RETURN_VAL(sockaddr, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((sockaddr->sa_family != AF_INET) && (sockaddr->sa_family != AF_INET6), NULL);

   return efl_add_ref(EFL_NET_IP_ADDRESS_CLASS, efl_main_loop_get(),
                  efl_net_ip_address_sockaddr_set(efl_added, sockaddr));
}

EOLIAN static Efl_Net_Ip_Address *
_efl_net_ip_address_parse(const char *numeric_address)
{
   struct sockaddr_storage ss;
   Eina_Bool r;
   const char *address = numeric_address;
   char *tmp = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(numeric_address, NULL);

   if (numeric_address[0] != '[')
     {
        const char *p = strchr(numeric_address, ':');
        if (p)
          {
             p = strchr(p + 1, ':');
             if (p)
               {
                  size_t len = strlen(numeric_address);
                  /* IPv6 no braces: ::1, etc... no port, add braces */
                  tmp = malloc(len + sizeof("[]"));
                  EINA_SAFETY_ON_NULL_RETURN_VAL(tmp, NULL);
                  tmp[0] = '[';
                  memcpy(tmp + 1, numeric_address, len);
                  tmp[1 + len] = ']';
                  tmp[1 + len + 1] = '\0';
                  address = tmp;
               }
          }
     }

   r = efl_net_ip_port_parse(address, &ss);
   free(tmp);
   if (!r)
     {
        DBG("could not parse numeric address: %s", numeric_address);
        return NULL;
     }

   return efl_add_ref(EFL_NET_IP_ADDRESS_CLASS, efl_main_loop_get(),
                  efl_net_ip_address_sockaddr_set(efl_added, &ss));
}

/**
 * @brief Context structure for asynchronous IP address resolution.
 *
 * Holds the state required for an ongoing resolution operation, including
 * the requested address string, the worker thread, and the promise for the result.
 */
typedef struct _Efl_Net_Ip_Address_Resolve_Context {
   Eina_Stringshare *request_address; /**< The original address string being resolved (e.g., "example.com:80"). */
   Ecore_Thread *thread;             /**< The worker thread performing the resolution. */
   Eina_Promise *promise;            /**< The promise to be fulfilled with the resolution result or error. */
} Efl_Net_Ip_Address_Resolve_Context;

/**
 * @internal
 * @brief Gets the Eina_Value structure descriptor for Efl_Net_Ip_Address_Resolve_Value.
 *
 * This is used for handling Efl_Net_Ip_Address_Resolve_Value with Eina_Value.
 * It defines the members and their types.
 *
 * @return A pointer to the static Eina_Value_Struct_Desc.
 */
static Eina_Value_Struct_Desc *
_efl_net_ip_address_resolve_value_desc_get(void)
{
   static Eina_Value_Struct_Member struct_members[] = {
     // no eina_value_type as they are not constant initializers, see below.
     EINA_VALUE_STRUCT_MEMBER(NULL, Efl_Net_Ip_Address_Resolve_Value, canonical_name),
     EINA_VALUE_STRUCT_MEMBER(NULL, Efl_Net_Ip_Address_Resolve_Value, request_address),
     EINA_VALUE_STRUCT_MEMBER(NULL, Efl_Net_Ip_Address_Resolve_Value, results)
   };
   static Eina_Value_Struct_Desc struct_desc = {
      EINA_VALUE_STRUCT_DESC_VERSION,
      NULL,
      struct_members,
      EINA_C_ARRAY_LENGTH(struct_members),
      sizeof (Efl_Net_Ip_Address_Resolve_Value)
   };
   // Types are set here because EINA_VALUE_TYPE_* are not constant initializers.
   struct_members[0].type = EINA_VALUE_TYPE_STRINGSHARE;
   struct_members[1].type = EINA_VALUE_TYPE_STRINGSHARE;
   struct_members[2].type = EINA_VALUE_TYPE_ARRAY;
   struct_desc.ops = EINA_VALUE_STRUCT_OPERATIONS_BINSEARCH;

   return &struct_desc;
}

/**
 * @internal
 * @brief Cleans up the resolution context when a promise is cancelled or finished.
 *
 * This function is called when the promise associated with a resolution
 * operation is deleted (e.g., due to cancellation or completion).
 * It ensures that resources like the stringshare, thread, and context
 * itself are properly freed.
 *
 * @param data The Efl_Net_Ip_Address_Resolve_Context to clean up.
 * @param dead_promise The promise that is being deleted (unused).
 */
static void
_efl_net_ip_address_resolve_del(void *data,
                                const Eina_Promise *dead_promise EINA_UNUSED)
{
   Efl_Net_Ip_Address_Resolve_Context *ctx = data;

   ctx->promise = NULL;

   eina_stringshare_replace(&ctx->request_address, NULL);

   if (ctx->thread)
     {
        ecore_thread_cancel(ctx->thread);
        ecore_thread_wait(ctx->thread, 1);
        ctx->thread = NULL;
     }

   free(ctx);
}

/**
 * @internal
 * @brief Searches for an IP address (sockaddr) within an Eina_Value array of Efl_Net_Ip_Address objects.
 *
 * This function iterates through the array and compares the sockaddr
 * of each Efl_Net_Ip_Address object with the provided sockaddr.
 * It is used to avoid adding duplicate addresses to the resolution results.
 *
 * @param array An Eina_Value of type EINA_VALUE_TYPE_ARRAY, where each element
 *              is an Efl_Net_Ip_Address object.
 * @param addr The `struct sockaddr` to search for.
 * @return The index of the found address in the array, or -1 if not found.
 */
static inline int
_efl_net_ip_address_find(const Eina_Value *array, const struct sockaddr *addr)
{
   const Efl_Net_Ip_Address *o;
   unsigned int i, len;

   EINA_VALUE_ARRAY_FOREACH(array, len, i, o)
     {
        const struct sockaddr *other = efl_net_ip_address_sockaddr_get(o);

        if (addr->sa_family == AF_INET6)
          {
             if (other->sa_family == AF_INET6)
               {
                  if (memcmp(other,  addr, sizeof(struct sockaddr_in6)) == 0)
                    return (int)i;
               }
          }
        else
          {
             if (other->sa_family == AF_INET)
               {
                  if (memcmp(other,  addr, sizeof(struct sockaddr_in)) == 0)
                    return (int)i;
               }
          }
     }
   return -1;
}

/**
 * @internal
 * @brief Callback function invoked when asynchronous IP address resolution is complete.
 *
 * This function is called by the ecore_con dns lookup mechanism once the
 * getaddrinfo call in the worker thread finishes. It processes the results,
 * creates Efl_Net_Ip_Address objects, and resolves or rejects the associated promise.
 *
 * @param data The Efl_Net_Ip_Address_Resolve_Context.
 * @param host The hostname that was resolved (unused by this function directly, but part of ecore_con_dns_lookup_done_cb signature).
 * @param port The port string that was used (unused by this function directly).
 * @param hints The addrinfo hints used for resolution (unused).
 * @param result A linked list of `struct addrinfo` containing the resolved addresses.
 * @param gai_error An error code from getaddrinfo (0 on success).
 */
static void
_efl_net_ip_address_resolve_done(void *data,
                                 const char *host, const char *port,
                                 const struct addrinfo *hints EINA_UNUSED,
                                 struct addrinfo *result,
                                 int gai_error)
{
   Efl_Net_Ip_Address_Resolve_Context *ctx = data;
   Eina_Value_Array desc = { 0 };
   Eina_Value s = EINA_VALUE_EMPTY;
   Eina_Value r = EINA_VALUE_EMPTY;
   Eina_Error err = EFL_NET_ERROR_COULDNT_RESOLVE_HOST;
   const struct addrinfo *a;

   DBG("done resolving '%s' (host='%s', port='%s'): %s",
       ctx->request_address, host, port,
       gai_error ? gai_strerror(gai_error) : "success");

   ctx->thread = NULL;

   if (gai_error)
     {
        if (gai_error == EAI_SYSTEM)
          err = errno;

        goto on_error;
     }

   err = ENOMEM;
   if (!eina_value_array_setup(&r, EINA_VALUE_TYPE_OBJECT, 1))
     goto on_error;

   if (!eina_value_struct_setup(&s, _efl_net_ip_address_resolve_value_desc_get()))
     goto on_error;

   eina_value_struct_set(&s, "request_address", ctx->request_address);

   for (a = result; a != NULL; a = a->ai_next)
     {
        Eina_Stringshare *canonical_name = NULL;
        Eo *o;

        eina_value_struct_get(&s, "canonical_name", &canonical_name);
        if (EINA_UNLIKELY((canonical_name == NULL) &&
                          (a->ai_canonname != NULL)))
          {
             canonical_name = eina_stringshare_add(a->ai_canonname);
             eina_value_struct_set(&s, "canonical_name", canonical_name);
             eina_stringshare_del(canonical_name);
          }

        /* some addresses get duplicated with different options that we
         * do not care, so check for duplicates.
         */
        if (EINA_UNLIKELY(_efl_net_ip_address_find(&r, a->ai_addr) >= 0))
          continue;

        o = efl_net_ip_address_create_sockaddr(a->ai_addr);
        if (!o) continue ;

        eina_value_array_append(&r, o);
        efl_unref(o);
     }
   freeaddrinfo(result);

   if (!eina_value_pget(&r, &desc)) goto on_error;
   if (!eina_value_struct_pset(&s, "results", &desc)) goto on_error;
   eina_value_flush(&r);

   eina_promise_resolve(ctx->promise, s);

   eina_stringshare_replace(&ctx->request_address, NULL);
   free(ctx);

   return ;

 on_error:
   eina_promise_reject(ctx->promise, err);

   eina_stringshare_replace(&ctx->request_address, NULL);
   free(ctx);
}

/**
 * @internal
 * @brief Initiates asynchronous resolution of a host address string.
 *
 * This function resolves a given address string (which can be a hostname,
 * an IP literal, and an optional port) into one or more Efl_Net_Ip_Address objects.
 * The resolution is performed asynchronously in a separate thread.
 *
 * @param address The address string to resolve (e.g., "example.com", "192.168.1.1:80", "[::1]:http").
 * @param family The desired address family (AF_INET, AF_INET6, or AF_UNSPEC).
 *               If 0, AF_UNSPEC is used.
 * @param flags Flags for getaddrinfo (e.g., AI_CANONNAME).
 *              If 0, `AI_ADDRCONFIG | AI_V4MAPPED | AI_CANONNAME` is used.
 * @return An Eina_Future that will be fulfilled with an Eina_Value of type
 *         Efl_Net_Ip_Address_Resolve_Value on success, or rejected with an
 *         Eina_Error on failure. Returns NULL if immediate setup fails.
 *         The Efl_Net_Ip_Address_Resolve_Value contains:
 *         - `request_address`: The original input address string.
 *         - `canonical_name`: The canonical name if AI_CANONNAME was used and a CNAME record exists.
 *         - `results`: An Eina_Value_Array of Efl_Net_Ip_Address objects.
 *           Example `results` array:
 *           `[ (Efl_Net_Ip_Address*)"198.51.100.1:80", (Efl_Net_Ip_Address*)"[2001:db8::a]:80" ]`
 */
EOLIAN static Eina_Future *
_efl_net_ip_address_resolve(const char *address, int family, int flags)
{
   Efl_Net_Ip_Address_Resolve_Context *ctx;
   struct addrinfo hints = { };
   const char *host = NULL, *port = NULL;
   Eina_Bool r;
   char *str;

   EINA_SAFETY_ON_NULL_RETURN_VAL(address, NULL);

   if (family == 0) family = AF_UNSPEC;
   EINA_SAFETY_ON_TRUE_RETURN_VAL((family != AF_UNSPEC) && (family != AF_INET) && (family != AF_INET6), NULL);

   if (flags == 0) flags = AI_ADDRCONFIG | AI_V4MAPPED | AI_CANONNAME;
   hints.ai_family = family;
   hints.ai_flags = flags;

   str = strdup(address);
   EINA_SAFETY_ON_NULL_RETURN_VAL(str, NULL);

   r = efl_net_ip_port_split(str, &host, &port);
   if ((!r) || (!host) || (host[0] == '\0'))
     {
        host = address;
        port = "0";
     }
   if (!port) port = "0";

   ctx = calloc(1, sizeof(Efl_Net_Ip_Address_Resolve_Context));
   EINA_SAFETY_ON_NULL_GOTO(ctx, error_ctx);

   ctx->request_address = eina_stringshare_add(address);
   EINA_SAFETY_ON_NULL_GOTO(ctx->request_address, error_result_address);

   ctx->thread = efl_net_ip_resolve_async_new(host, port, &hints, _efl_net_ip_address_resolve_done, ctx);
   EINA_SAFETY_ON_NULL_GOTO(ctx->thread, error_thread);

   ctx->promise = eina_promise_new(efl_loop_future_scheduler_get(efl_main_loop_get()), _efl_net_ip_address_resolve_del, ctx);
   EINA_SAFETY_ON_NULL_GOTO(ctx->promise, error_promise);

   free(str);
   return eina_future_new(ctx->promise);

 error_promise:
   ecore_thread_cancel(ctx->thread);
 error_thread:
   eina_stringshare_del(ctx->request_address);
 error_result_address:
   free(ctx);
 error_ctx:
   free(str);
   return NULL;
}

#include "efl_net_ip_address.eo.c"
