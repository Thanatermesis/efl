/**
 * @brief Legacy wrapper for ecore_con_eet_base_register().
 * @deprecated This function is a legacy wrapper. Prefer using
 *             ecore_con_eet_base_register() directly with an Eo object that
 *             inherits from Ecore_Con_Eet_Base.
 *
 * This function provides a compatibility layer for older code by forwarding
 * the call to ecore_con_eet_base_register(). It is intended for use
 * with the old #Ecore_Con_Eet_Base typedef.
 *
 * @param[in] obj The #Ecore_Con_Eet_Base object (legacy typedef).
 * @param[in] name The name of the Eet stream.
 * @param[in] edd The #Eet_Data_Descriptor to be registered.
 */
ECORE_CON_API void
ecore_con_eet(Ecore_Con_Eet_Base *obj, const char *name, Eet_Data_Descriptor *edd)
{
   ecore_con_eet_base_register(obj, name, edd);
}
