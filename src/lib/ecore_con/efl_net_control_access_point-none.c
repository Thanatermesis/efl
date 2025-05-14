#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

/**
 * @brief Private data for the Efl_Net_Control_Access_Point_None class.
 * @details This structure is currently empty as the "none" implementation
 *          does not require any specific data.
 */
typedef struct
{

} Efl_Net_Control_Access_Point_Data;

/**
 * @brief Destructor for the Efl_Net_Control_Access_Point_None object.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 */
EOLIAN static void
_efl_net_control_access_point_efl_object_destructor(Eo *obj, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, EFL_NET_CONTROL_ACCESS_POINT_CLASS));
}

/**
 * @brief Gets the state of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns 0 (EFL_NET_CONTROL_ACCESS_POINT_STATE_UNKNOWN or equivalent)
 *         as this is a stub implementation.
 */
EOLIAN static Efl_Net_Control_Access_Point_State
_efl_net_control_access_point_state_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Gets the last error of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns 0 (EFL_NET_CONTROL_ACCESS_POINT_ERROR_NONE or equivalent)
 *         as this is a stub implementation.
 */
EOLIAN static Efl_Net_Control_Access_Point_Error
_efl_net_control_access_point_error_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Gets the SSID of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns NULL as this is a stub implementation.
 */
EOLIAN static const char *
_efl_net_control_access_point_ssid_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Sets the priority of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[in] priority The priority to set.
 * @details This is a no-op in the "none" implementation.
 */
EOLIAN static void
_efl_net_control_access_point_priority_set(Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, unsigned int priority EINA_UNUSED)
{
}

/**
 * @brief Gets the priority of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns 0 as this is a stub implementation.
 */
EOLIAN static unsigned int
_efl_net_control_access_point_priority_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Gets the technology of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns NULL as this is a stub implementation.
 */
EOLIAN static Efl_Net_Control_Technology *
_efl_net_control_access_point_technology_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Gets the strength of the access point signal.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns 0 as this is a stub implementation.
 */
EOLIAN static uint8_t
_efl_net_control_access_point_strength_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Gets the roaming status of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns EINA_FALSE as this is a stub implementation.
 */
EOLIAN static Eina_Bool
_efl_net_control_access_point_roaming_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Sets the auto-connect behavior for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[in] auto_connect The auto-connect flag.
 * @details This is a no-op in the "none" implementation.
 */
EOLIAN static void
_efl_net_control_access_point_auto_connect_set(Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, Eina_Bool auto_connect EINA_UNUSED)
{
}

/**
 * @brief Gets the auto-connect behavior for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns EINA_FALSE as this is a stub implementation.
 */
EOLIAN static Eina_Bool
_efl_net_control_access_point_auto_connect_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Gets whether the access point is remembered.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns EINA_FALSE as this is a stub implementation.
 */
EOLIAN static Eina_Bool
_efl_net_control_access_point_remembered_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Gets whether the access point configuration is immutable.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns EINA_FALSE as this is a stub implementation.
 */
EOLIAN static Eina_Bool
_efl_net_control_access_point_immutable_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Gets the security type of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns 0 (EFL_NET_CONTROL_ACCESS_POINT_SECURITY_NONE or equivalent)
 *         as this is a stub implementation.
 */
EOLIAN static Efl_Net_Control_Access_Point_Security
_efl_net_control_access_point_security_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Gets the name servers for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns NULL as this is a stub implementation.
 *         The iterator would contain strings (const char *) if implemented.
 */
EOLIAN static Eina_Iterator *
_efl_net_control_access_point_name_servers_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Gets the time servers for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns NULL as this is a stub implementation.
 *         The iterator would contain strings (const char *) if implemented.
 */
EOLIAN static Eina_Iterator *
_efl_net_control_access_point_time_servers_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Gets the domains for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns NULL as this is a stub implementation.
 *         The iterator would contain strings (const char *) if implemented.
 */
EOLIAN static Eina_Iterator *
_efl_net_control_access_point_domains_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Gets the IPv4 configuration of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[out] method Pointer to store the IPv4 method.
 * @param[out] address Pointer to store the IPv4 address.
 * @param[out] netmask Pointer to store the IPv4 netmask.
 * @param[out] gateway Pointer to store the IPv4 gateway.
 * @details This is a no-op in the "none" implementation. Output parameters are not modified.
 */
EOLIAN static void
_efl_net_control_access_point_ipv4_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, Efl_Net_Control_Access_Point_Ipv4_Method *method EINA_UNUSED, const char **address EINA_UNUSED, const char **netmask EINA_UNUSED, const char **gateway EINA_UNUSED)
{
}

/**
 * @brief Gets the IPv6 configuration of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[out] method Pointer to store the IPv6 method.
 * @param[out] address Pointer to store the IPv6 address.
 * @param[out] prefix_length Pointer to store the IPv6 prefix length.
 * @param[out] netmask Pointer to store the IPv6 netmask.
 * @param[out] gateway Pointer to store the IPv6 gateway.
 * @details This is a no-op in the "none" implementation. Output parameters are not modified.
 */
EOLIAN static void
_efl_net_control_access_point_ipv6_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, Efl_Net_Control_Access_Point_Ipv6_Method *method EINA_UNUSED, const char **address EINA_UNUSED, uint8_t *prefix_length EINA_UNUSED, const char **netmask EINA_UNUSED, const char **gateway EINA_UNUSED)
{
}

/**
 * @brief Gets the proxy configuration of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[out] method Pointer to store the proxy method.
 * @param[out] url Pointer to store the proxy URL.
 * @param[out] servers Pointer to store an iterator of proxy servers.
 * @param[out] excludes Pointer to store an iterator of proxy excludes.
 * @details This is a no-op in the "none" implementation. Output parameters are not modified.
 *          Iterators for servers and excludes would contain strings (const char *) if implemented.
 */
EOLIAN static void
_efl_net_control_access_point_proxy_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, Efl_Net_Control_Access_Point_Proxy_Method *method EINA_UNUSED, const char **url EINA_UNUSED, Eina_Iterator **servers EINA_UNUSED, Eina_Iterator **excludes EINA_UNUSED)
{
}

/**
 * @brief Sets the configured name servers for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[in] name_servers An iterator of name server strings (const char *).
 * @details This is a no-op in the "none" implementation.
 */
EOLIAN static void
_efl_net_control_access_point_configuration_name_servers_set(Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, Eina_Iterator *name_servers EINA_UNUSED)
{
}

/**
 * @brief Gets the configured name servers for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns NULL as this is a stub implementation.
 *         The iterator would contain strings (const char *) if implemented.
 */
EOLIAN static Eina_Iterator *
_efl_net_control_access_point_configuration_name_servers_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Sets the configured time servers for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[in] time_servers An iterator of time server strings (const char *).
 * @details This is a no-op in the "none" implementation.
 */
EOLIAN static void
_efl_net_control_access_point_configuration_time_servers_set(Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, Eina_Iterator *time_servers EINA_UNUSED)
{
}

/**
 * @brief Gets the configured time servers for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns NULL as this is a stub implementation.
 *         The iterator would contain strings (const char *) if implemented.
 */
EOLIAN static Eina_Iterator *
_efl_net_control_access_point_configuration_time_servers_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Sets the configured domains for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[in] domains An iterator of domain strings (const char *).
 * @details This is a no-op in the "none" implementation.
 */
EOLIAN static void
_efl_net_control_access_point_configuration_domains_set(Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, Eina_Iterator *domains EINA_UNUSED)
{
}

/**
 * @brief Gets the configured domains for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns NULL as this is a stub implementation.
 *         The iterator would contain strings (const char *) if implemented.
 */
EOLIAN static Eina_Iterator *
_efl_net_control_access_point_configuration_domains_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Sets the configured IPv4 settings for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[in] method The IPv4 configuration method.
 * @param[in] address The IPv4 address string.
 * @param[in] netmask The IPv4 netmask string.
 * @param[in] gateway The IPv4 gateway string.
 * @details This is a no-op in the "none" implementation.
 */
EOLIAN static void
_efl_net_control_access_point_configuration_ipv4_set(Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, Efl_Net_Control_Access_Point_Ipv4_Method method EINA_UNUSED, const char *address EINA_UNUSED, const char *netmask EINA_UNUSED, const char *gateway EINA_UNUSED)
{
}

/**
 * @brief Gets the configured IPv4 settings of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[out] method Pointer to store the IPv4 method.
 * @param[out] address Pointer to store the IPv4 address.
 * @param[out] netmask Pointer to store the IPv4 netmask.
 * @param[out] gateway Pointer to store the IPv4 gateway.
 * @details This is a no-op in the "none" implementation. Output parameters are not modified.
 */
EOLIAN static void
_efl_net_control_access_point_configuration_ipv4_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, Efl_Net_Control_Access_Point_Ipv4_Method *method EINA_UNUSED, const char **address EINA_UNUSED, const char **netmask EINA_UNUSED, const char **gateway EINA_UNUSED)
{
}

/**
 * @brief Sets the configured IPv6 settings for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[in] method The IPv6 configuration method.
 * @param[in] address The IPv6 address string.
 * @param[in] prefix_length The IPv6 prefix length.
 * @param[in] netmask The IPv6 netmask string.
 * @param[in] gateway The IPv6 gateway string.
 * @details This is a no-op in the "none" implementation.
 */
EOLIAN static void
_efl_net_control_access_point_configuration_ipv6_set(Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, Efl_Net_Control_Access_Point_Ipv6_Method method EINA_UNUSED, const char *address EINA_UNUSED, uint8_t prefix_length EINA_UNUSED, const char *netmask EINA_UNUSED, const char *gateway EINA_UNUSED)
{
}

/**
 * @brief Gets the configured IPv6 settings of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[out] method Pointer to store the IPv6 method.
 * @param[out] address Pointer to store the IPv6 address.
 * @param[out] prefix_length Pointer to store the IPv6 prefix length.
 * @param[out] netmask Pointer to store the IPv6 netmask.
 * @param[out] gateway Pointer to store the IPv6 gateway.
 * @details This is a no-op in the "none" implementation. Output parameters are not modified.
 */
EOLIAN static void
_efl_net_control_access_point_configuration_ipv6_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, Efl_Net_Control_Access_Point_Ipv6_Method *method EINA_UNUSED, const char **address EINA_UNUSED, uint8_t *prefix_length EINA_UNUSED, const char **netmask EINA_UNUSED, const char **gateway EINA_UNUSED)
{
}

/**
 * @brief Sets the configured proxy settings for the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[in] method The proxy configuration method.
 * @param[in] url The proxy URL string.
 * @param[in] servers An iterator of proxy server strings (const char *).
 * @param[in] excludes An iterator of proxy exclusion strings (const char *).
 * @details This is a no-op in the "none" implementation.
 */
EOLIAN static void
_efl_net_control_access_point_configuration_proxy_set(Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, Efl_Net_Control_Access_Point_Proxy_Method method EINA_UNUSED, const char *url EINA_UNUSED, Eina_Iterator *servers EINA_UNUSED, Eina_Iterator *excludes EINA_UNUSED)
{
}

/**
 * @brief Gets the configured proxy settings of the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @param[out] method Pointer to store the proxy method.
 * @param[out] url Pointer to store the proxy URL.
 * @param[out] servers Pointer to store an iterator of proxy servers.
 * @param[out] excludes Pointer to store an iterator of proxy excludes.
 * @details This is a no-op in the "none" implementation. Output parameters are not modified.
 *          Iterators for servers and excludes would contain strings (const char *) if implemented.
 */
EOLIAN static void
_efl_net_control_access_point_configuration_proxy_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED, Efl_Net_Control_Access_Point_Proxy_Method *method EINA_UNUSED, const char **url EINA_UNUSED, Eina_Iterator **servers EINA_UNUSED, Eina_Iterator **excludes EINA_UNUSED)
{
}

/**
 * @brief Connects to the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @return Always returns a future rejected with EINA_ERROR_NOT_IMPLEMENTED
 *         as this is a stub implementation.
 */
EOLIAN static Eina_Future *
_efl_net_control_access_point_connect(Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
   return efl_loop_future_rejected(obj,
                               EINA_ERROR_NOT_IMPLEMENTED);
}

/**
 * @brief Disconnects from the access point.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @details This is a no-op in the "none" implementation.
 */
EOLIAN static void
_efl_net_control_access_point_disconnect(Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
}

/**
 * @brief Forgets the access point configuration.
 * @param[in] obj The Efl_Net_Control_Access_Point_None object.
 * @param[in] pd The private data associated with the object.
 * @details This is a no-op in the "none" implementation.
 */
EOLIAN static void
_efl_net_control_access_point_forget(Eo *obj EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd EINA_UNUSED)
{
}

#include "efl_net_control_access_point.eo.c"
