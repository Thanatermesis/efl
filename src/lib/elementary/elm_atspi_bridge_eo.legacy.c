/**
 * @brief Implements the EAPI function to get the AT-SPI2 connection status.
 * @param obj The Elm_Atspi_Bridge object.
 * @return @c EINA_TRUE if connected, @c EINA_FALSE otherwise.
 * @see elm_atspi_bridge_connected_get() in the header for detailed documentation.
 */
EAPI Eina_Bool
elm_atspi_bridge_connected_get(const Elm_Atspi_Bridge *obj)
{
   return elm_obj_atspi_bridge_connected_get(obj);
}
