#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

typedef struct
{

} Efl_Net_Session_Data;

/**
 * @brief Destructor for the Efl_Net_Session object.
 *
 * This function is called when the Efl_Net_Session object is being destroyed.
 * It cleans up any resources allocated by the object.
 *
 * @param obj The Efl_Net_Session object.
 * @param pd Private data for the Efl_Net_Session object.
 */
EOLIAN static void
_efl_net_session_efl_object_destructor(Eo *obj, Efl_Net_Session_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, EFL_NET_SESSION_CLASS));
}

/**
 * @brief Constructor for the Efl_Net_Session object.
 *
 * This function is called when the Efl_Net_Session object is being created.
 * It initializes the object. In this "none" implementation, it logs that
 * network control is disabled.
 *
 * @param obj The Efl_Net_Session object.
 * @param pd Private data for the Efl_Net_Session object.
 * @return The constructed Efl_Net_Session object.
 */
EOLIAN static Efl_Object *
_efl_net_session_efl_object_constructor(Eo *obj, Efl_Net_Session_Data *pd EINA_UNUSED)
{
   INF("EFL compiled with --with-net-control=none");
   return efl_constructor(efl_super(obj, EFL_NET_SESSION_CLASS));
}

/**
 * @brief Finalizer for the Efl_Net_Session object.
 *
 * This function is called when the Efl_Net_Session object is being finalized.
 * It emits an EFL_NET_SESSION_EVENT_CHANGED event.
 *
 * @param obj The Efl_Net_Session object.
 * @param pd Private data for the Efl_Net_Session object.
 * @return The finalized Efl_Net_Session object.
 */
EOLIAN static Efl_Object *
_efl_net_session_efl_object_finalize(Eo *obj, Efl_Net_Session_Data *pd EINA_UNUSED)
{
   obj = efl_finalize(efl_super(obj, EFL_NET_SESSION_CLASS));
   efl_event_callback_call(obj, EFL_NET_SESSION_EVENT_CHANGED, NULL);
   return obj;
}

/**
 * @brief Gets the current network name (SSID for Wi-Fi).
 *
 * In this "none" implementation, network control is disabled, so this function
 * always returns NULL.
 *
 * @param obj The Efl_Net_Session object.
 * @param pd Private data for the Efl_Net_Session object.
 * @return Always NULL in this implementation.
 */
EOLIAN static const char *
_efl_net_session_network_name_get(const Eo *obj EINA_UNUSED, Efl_Net_Session_Data *pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Gets the current network state.
 *
 * In this "none" implementation, network control is disabled. It defaults to
 * EFL_NET_SESSION_STATE_ONLINE as a best guess for unsupported systems.
 *
 * @param obj The Efl_Net_Session object.
 * @param pd Private data for the Efl_Net_Session object.
 * @return Always EFL_NET_SESSION_STATE_ONLINE in this implementation.
 */
EOLIAN static Efl_Net_Session_State
_efl_net_session_state_get(const Eo *obj EINA_UNUSED, Efl_Net_Session_Data *pd EINA_UNUSED)
{
   return EFL_NET_SESSION_STATE_ONLINE; /* best default for unsupported, hope we're online */
}

/**
 * @brief Gets the current network technology.
 *
 * In this "none" implementation, network control is disabled. It returns
 * EFL_NET_SESSION_TECHNOLOGY_UNKNOWN.
 *
 * @param obj The Efl_Net_Session object.
 * @param pd Private data for the Efl_Net_Session object.
 * @return Always EFL_NET_SESSION_TECHNOLOGY_UNKNOWN in this implementation.
 */
EOLIAN static Efl_Net_Session_Technology
_efl_net_session_technology_get(const Eo *obj EINA_UNUSED, Efl_Net_Session_Data *pd EINA_UNUSED)
{
   return EFL_NET_SESSION_TECHNOLOGY_UNKNOWN;
}

/**
 * @brief Gets the current network interface name.
 *
 * In this "none" implementation, network control is disabled, so this function
 * always returns NULL.
 *
 * @param obj The Efl_Net_Session object.
 * @param pd Private data for the Efl_Net_Session object.
 * @return Always NULL in this implementation.
 */
EOLIAN static const char *
_efl_net_session_interface_get(const Eo *obj EINA_UNUSED, Efl_Net_Session_Data *pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Gets the IPv4 configuration.
 *
 * In this "none" implementation, network control is disabled. All output
 * parameters will be set to NULL.
 *
 * @param obj The Efl_Net_Session object.
 * @param pd Private data for the Efl_Net_Session object.
 * @param address Pointer to store the IPv4 address (e.g., "192.168.1.100").
 * @param netmask Pointer to store the IPv4 netmask (e.g., "255.255.255.0").
 * @param gateway Pointer to store the IPv4 gateway (e.g., "192.168.1.1").
 */
EOLIAN static void
_efl_net_session_ipv4_get(const Eo *obj EINA_UNUSED, Efl_Net_Session_Data *pd EINA_UNUSED, const char **address, const char **netmask, const char **gateway)
{
   if (address) *address = NULL;
   if (netmask) *netmask = NULL;
   if (gateway) *gateway = NULL;
}

/**
 * @brief Gets the IPv6 configuration.
 *
 * In this "none" implementation, network control is disabled. All output
 * parameters will be set to NULL or 0.
 *
 * @param obj The Efl_Net_Session object.
 * @param pd Private data for the Efl_Net_Session object.
 * @param address Pointer to store the IPv6 address (e.g., "2001:0db8:85a3:0000:0000:8a2e:0370:7334").
 * @param prefix_length Pointer to store the IPv6 prefix length (e.g., 64).
 * @param netmask Pointer to store the IPv6 netmask.
 * @param gateway Pointer to store the IPv6 gateway.
 */
EOLIAN static void
_efl_net_session_ipv6_get(const Eo *obj EINA_UNUSED, Efl_Net_Session_Data *pd EINA_UNUSED, const char **address, uint8_t *prefix_length, const char **netmask, const char **gateway)
{
   if (address) *address = NULL;
   if (prefix_length) *prefix_length = 0;
   if (netmask) *netmask = NULL;
   if (gateway) *gateway = NULL;
}

/**
 * @brief Connects to a network.
 *
 * In this "none" implementation, network control is disabled. This function
 * logs a message indicating that connection is not possible.
 *
 * @param obj The Efl_Net_Session object.
 * @param pd Private data for the Efl_Net_Session object.
 * @param online_required Whether an online connection is required.
 * @param technologies_allowed Allowed network technologies.
 */
EOLIAN static void
_efl_net_session_connect(Eo *obj EINA_UNUSED, Efl_Net_Session_Data *pd EINA_UNUSED, Eina_Bool online_required EINA_UNUSED, Efl_Net_Session_Technology technologies_allowed EINA_UNUSED)
{
   INF("EFL compiled with --with-net-control=none, cannot connect.");
}

/**
 * @brief Disconnects from the current network.
 *
 * In this "none" implementation, network control is disabled. This function
 * logs a message indicating that disconnection is not possible.
 *
 * @param obj The Efl_Net_Session object.
 * @param pd Private data for the Efl_Net_Session object.
 */
EOLIAN static void
_efl_net_session_disconnect(Eo *obj EINA_UNUSED, Efl_Net_Session_Data *pd EINA_UNUSED)
{
   INF("EFL compiled with --with-net-control=none, cannot disconnect.");
}

#include "efl_net_session.eo.c"
