#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

/**
 * @brief Private data for the Efl_Net_Control_Manager_Data class.
 * @since 1.26
 *
 * @note This is a "none" implementation, so the data structure is empty
 * as no actual network management is performed.
 */
typedef struct
{

} Efl_Net_Control_Manager_Data;

/**
 * @brief Destructor for the Efl_Net_Control_Manager object.
 *
 * @param obj The Efl_Net_Control_Manager object.
 * @param pd Private data for the object.
 */
EOLIAN static void
_efl_net_control_manager_efl_object_destructor(Eo *obj, Efl_Net_Control_Manager_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, EFL_NET_CONTROL_MANAGER_CLASS));
}

/**
 * @brief Constructor for the Efl_Net_Control_Manager object.
 *
 * Logs an informational message indicating that EFL was compiled with
 * network control disabled.
 *
 * @param obj The Efl_Net_Control_Manager object to construct.
 * @param pd Private data for the object.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_net_control_manager_efl_object_constructor(Eo *obj, Efl_Net_Control_Manager_Data *pd EINA_UNUSED)
{
   INF("EFL compiled with --with-net-control=none");
   return efl_constructor(efl_super(obj, EFL_NET_CONTROL_MANAGER_CLASS));
}

/**
 * @brief Finalizer for the Efl_Net_Control_Manager object.
 *
 * Emits the EFL_NET_CONTROL_MANAGER_EVENT_STATE_CHANGED event.
 *
 * @param obj The Efl_Net_Control_Manager object to finalize.
 * @param pd Private data for the object.
 * @return The finalized Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_net_control_manager_efl_object_finalize(Eo *obj, Efl_Net_Control_Manager_Data *pd EINA_UNUSED)
{
   obj = efl_finalize(efl_super(obj, EFL_NET_CONTROL_MANAGER_CLASS));
   efl_event_callback_call(obj, EFL_NET_CONTROL_MANAGER_EVENT_STATE_CHANGED, NULL);
   return obj;
}

/**
 * @brief Sets the offline mode for radios.
 * @note This is a "none" implementation and has no effect.
 *
 * @param obj The Efl_Net_Control_Manager object.
 * @param pd Private data for the object.
 * @param radios_offline EINA_TRUE to set radios to offline, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_net_control_manager_radios_offline_set(Eo *obj EINA_UNUSED, Efl_Net_Control_Manager_Data *pd EINA_UNUSED, Eina_Bool radios_offline EINA_UNUSED)
{
}

/**
 * @brief Gets the offline mode for radios.
 * @note This is a "none" implementation and always returns EINA_FALSE.
 *
 * @param obj The Efl_Net_Control_Manager object.
 * @param pd Private data for the object.
 * @return Always EINA_FALSE.
 */
EOLIAN static Eina_Bool
_efl_net_control_manager_radios_offline_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Manager_Data *pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Gets the current network state.
 * @note This is a "none" implementation and always returns EFL_NET_CONTROL_STATE_ONLINE
 *       as a best guess for unsupported systems.
 *
 * @param obj The Efl_Net_Control_Manager object.
 * @param pd Private data for the object.
 * @return Always EFL_NET_CONTROL_STATE_ONLINE.
 */
EOLIAN static Efl_Net_Control_State
_efl_net_control_manager_state_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Manager_Data *pd EINA_UNUSED)
{
   return EFL_NET_CONTROL_STATE_ONLINE; /* best default for unsupported, hope we're online */
}

/**
 * @brief Gets an iterator for available network access points.
 * @note This is a "none" implementation and always returns an empty iterator.
 *
 * @param obj The Efl_Net_Control_Manager object.
 * @param pd Private data for the object.
 * @return An empty Eina_Iterator.
 */
EOLIAN static Eina_Iterator *
_efl_net_control_manager_access_points_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Manager_Data *pd EINA_UNUSED)
{
   return eina_list_iterator_new(NULL);
}

/**
 * @brief Gets an iterator for available network technologies.
 * @note This is a "none" implementation and always returns an empty iterator.
 *
 * @param obj The Efl_Net_Control_Manager object.
 * @param pd Private data for the object.
 * @return An empty Eina_Iterator.
 */
EOLIAN static Eina_Iterator *
_efl_net_control_manager_technologies_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Manager_Data *pd EINA_UNUSED)
{
   return eina_list_iterator_new(NULL);
}

/**
 * @brief Sets whether the network agent is enabled.
 * @note This is a "none" implementation and has no effect.
 *
 * @param obj The Efl_Net_Control_Manager object.
 * @param pd Private data for the object.
 * @param agent_enabled EINA_TRUE to enable the agent, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_net_control_manager_agent_enabled_set(Eo *obj EINA_UNUSED, Efl_Net_Control_Manager_Data *pd EINA_UNUSED, Eina_Bool agent_enabled EINA_UNUSED)
{
}

/**
 * @brief Gets whether the network agent is enabled.
 * @note This is a "none" implementation and always returns EINA_FALSE.
 *
 * @param obj The Efl_Net_Control_Manager object.
 * @param pd Private data for the object.
 * @return Always EINA_FALSE.
 */
EOLIAN static Eina_Bool
_efl_net_control_manager_agent_enabled_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Manager_Data *pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Replies to an agent request.
 * @note This is a "none" implementation and has no effect.
 *
 * @param obj The Efl_Net_Control_Manager object.
 * @param pd Private data for the object.
 * @param name The name associated with the request.
 * @param ssid The SSID for the network.
 * @param username The username for authentication.
 * @param passphrase The passphrase for authentication.
 * @param wps WPS pin or an empty string.
 */
EOLIAN static void
_efl_net_control_manager_agent_reply(Eo *obj EINA_UNUSED, Efl_Net_Control_Manager_Data *pd EINA_UNUSED, const char *name EINA_UNUSED, const Eina_Slice *ssid EINA_UNUSED, const char *username EINA_UNUSED, const char *passphrase EINA_UNUSED, const char *wps EINA_UNUSED)
{
}

#include "efl_net_control_manager.eo.c"
