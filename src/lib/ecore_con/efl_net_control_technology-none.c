#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

typedef struct
{

} Efl_Net_Control_Technology_Data;

/**
 * @brief Destructor for the Efl_Net_Control_Technology object.
 *
 * This function is called when the Efl_Net_Control_Technology object is being destroyed.
 * It ensures that the parent class's destructor is called.
 *
 * @param obj The Efl_Net_Control_Technology object.
 * @param pd Private data for the Efl_Net_Control_Technology object (unused).
 */
EOLIAN static void
_efl_net_control_technology_efl_object_destructor(Eo *obj, Efl_Net_Control_Technology_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, EFL_NET_CONTROL_TECHNOLOGY_CLASS));
}

/**
 * @brief Sets the powered state of the technology.
 *
 * This is a stub implementation and does nothing.
 *
 * @param obj The Efl_Net_Control_Technology object (unused).
 * @param pd Private data for the Efl_Net_Control_Technology object (unused).
 * @param powered The desired powered state (unused).
 */
EOLIAN static void
_efl_net_control_technology_powered_set(Eo *obj EINA_UNUSED, Efl_Net_Control_Technology_Data *pd EINA_UNUSED, Eina_Bool powered EINA_UNUSED)
{
}

/**
 * @brief Gets the powered state of the technology.
 *
 * This is a stub implementation and always returns EINA_FALSE.
 *
 * @param obj The Efl_Net_Control_Technology object (unused).
 * @param pd Private data for the Efl_Net_Control_Technology object (unused).
 * @return EINA_FALSE, indicating the technology is not powered.
 */
EOLIAN static Eina_Bool
_efl_net_control_technology_powered_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Technology_Data *pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Sets the tethering state of the technology.
 *
 * This is a stub implementation and does nothing.
 *
 * @param obj The Efl_Net_Control_Technology object (unused).
 * @param pd Private data for the Efl_Net_Control_Technology object (unused).
 * @param enabled The desired tethering state (unused).
 * @param identifier The identifier for tethering (unused).
 * @param passphrase The passphrase for tethering (unused).
 */
EOLIAN static void
_efl_net_control_technology_tethering_set(Eo *obj EINA_UNUSED, Efl_Net_Control_Technology_Data *pd EINA_UNUSED, Eina_Bool enabled EINA_UNUSED, const char *identifier EINA_UNUSED, const char *passphrase EINA_UNUSED)
{
}

/**
 * @brief Gets the tethering state of the technology.
 *
 * This is a stub implementation and does not set any output parameters.
 *
 * @param obj The Efl_Net_Control_Technology object (unused).
 * @param pd Private data for the Efl_Net_Control_Technology object (unused).
 * @param enabled Pointer to store the tethering state (unused).
 * @param identifier Pointer to store the tethering identifier (unused).
 * @param passphrase Pointer to store the tethering passphrase (unused).
 */
EOLIAN static void
_efl_net_control_technology_tethering_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Technology_Data *pd EINA_UNUSED, Eina_Bool *enabled EINA_UNUSED, const char **identifier EINA_UNUSED, const char **passphrase EINA_UNUSED)
{
}

/**
 * @brief Gets the connected state of the technology.
 *
 * This is a stub implementation and always returns EINA_FALSE.
 *
 * @param obj The Efl_Net_Control_Technology object (unused).
 * @param pd Private data for the Efl_Net_Control_Technology object (unused).
 * @return EINA_FALSE, indicating the technology is not connected.
 */
EOLIAN static Eina_Bool
_efl_net_control_technology_connected_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Technology_Data *pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Gets the name of the technology object.
 *
 * This is a stub implementation and always returns NULL.
 *
 * @param obj The Efl_Net_Control_Technology object (unused).
 * @param pd Private data for the Efl_Net_Control_Technology object (unused).
 * @return NULL.
 */
EOLIAN static const char *
_efl_net_control_technology_efl_object_name_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Technology_Data *pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Gets the type of the technology.
 *
 * This is a stub implementation and always returns 0.
 *
 * @param obj The Efl_Net_Control_Technology object (unused).
 * @param pd Private data for the Efl_Net_Control_Technology object (unused).
 * @return 0, representing an undefined or default technology type.
 */
EOLIAN static Efl_Net_Control_Technology_Type
_efl_net_control_technology_type_get(const Eo *obj EINA_UNUSED, Efl_Net_Control_Technology_Data *pd EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Initiates a scan for networks using this technology.
 *
 * This is a stub implementation and always returns a rejected future
 * with the error EINA_ERROR_NOT_IMPLEMENTED.
 *
 * @param obj The Efl_Net_Control_Technology object.
 * @param pd Private data for the Efl_Net_Control_Technology object (unused).
 * @return A future that is immediately rejected with EINA_ERROR_NOT_IMPLEMENTED.
 */
EOLIAN static Eina_Future *
_efl_net_control_technology_scan(Eo *obj, Efl_Net_Control_Technology_Data *pd EINA_UNUSED)
{
   return efl_loop_future_rejected(obj,
                               EINA_ERROR_NOT_IMPLEMENTED);
}

#include "efl_net_control_technology.eo.c"
