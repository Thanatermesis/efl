#include "elput_private.h"

/**
 * @internal
 * @brief Checks the return status of a libinput configuration function.
 * @param ret The return code from a libinput_device_config_* function.
 * @return EINA_TRUE if the status is LIBINPUT_CONFIG_STATUS_SUCCESS, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_check_status(int ret)
{
   if (ret == LIBINPUT_CONFIG_STATUS_SUCCESS)
     return EINA_TRUE;
   return EINA_FALSE;
}

/**
 * @brief Enables or disables tap-and-drag for a touch device.
 *
 * When enabled, a tap followed by a finger down within a timeout will
 * start a drag. The drag terminates when the finger is lifted.
 *
 * @param device The Elput device.
 * @param enabled EINA_TRUE to enable tap-and-drag, EINA_FALSE to disable.
 * @return EINA_TRUE on success, EINA_FALSE on failure or if the device is NULL.
 */
EAPI Eina_Bool
elput_touch_drag_enabled_set(Elput_Device *device, Eina_Bool enabled)
{
   int ret = -1;

   EINA_SAFETY_ON_NULL_RETURN_VAL(device, EINA_FALSE);

   if (enabled)
     {
        ret =
          libinput_device_config_tap_set_drag_enabled(device->device,
                                                      LIBINPUT_CONFIG_DRAG_ENABLED);
     }
   else
     {
        ret =
          libinput_device_config_tap_set_drag_enabled(device->device,
                                                      LIBINPUT_CONFIG_DRAG_DISABLED);
     }

   return _check_status(ret);
}

/**
 * @brief Gets the current tap-and-drag enabled state for a touch device.
 * @param device The Elput device.
 * @return EINA_TRUE if tap-and-drag is enabled, EINA_FALSE if disabled or on error.
 */
EAPI Eina_Bool
elput_touch_drag_enabled_get(Elput_Device *device)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(device, EINA_FALSE);

   return libinput_device_config_tap_get_drag_enabled(device->device);
}

/**
 * @brief Enables or disables drag lock for tap-and-drag on a touch device.
 *
 * When enabled, a tap-and-drag operation may be "locked". A locked drag
 * does not terminate when the finger is lifted but requires a second tap
 * to release the drag.
 *
 * @param device The Elput device.
 * @param enabled EINA_TRUE to enable drag lock, EINA_FALSE to disable.
 * @return EINA_TRUE on success, EINA_FALSE on failure or if the device is NULL.
 */
EAPI Eina_Bool
elput_touch_drag_lock_enabled_set(Elput_Device *device, Eina_Bool enabled)
{
   int ret = -1;

   EINA_SAFETY_ON_NULL_RETURN_VAL(device, EINA_FALSE);

   if (enabled)
     {
        ret =
          libinput_device_config_tap_set_drag_lock_enabled(device->device,
                                                           LIBINPUT_CONFIG_DRAG_LOCK_ENABLED);
     }
   else
     {
        ret =
          libinput_device_config_tap_set_drag_lock_enabled(device->device,
                                                           LIBINPUT_CONFIG_DRAG_LOCK_DISABLED);
     }

   return _check_status(ret);
}

/**
 * @brief Gets the current drag lock enabled state for a touch device.
 * @param device The Elput device.
 * @return EINA_TRUE if drag lock is enabled, EINA_FALSE if disabled or on error.
 */
EAPI Eina_Bool
elput_touch_drag_lock_enabled_get(Elput_Device *device)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(device, EINA_FALSE);

   return libinput_device_config_tap_get_drag_lock_enabled(device->device);
}

/**
 * @brief Enables or disables "disable while typing" (DWT) for a touchpad.
 *
 * If DWT is enabled, the touchpad is disabled while typing and for a
 * short period after the last key press.
 *
 * @param device The Elput device.
 * @param enabled EINA_TRUE to enable DWT, EINA_FALSE to disable.
 * @return EINA_TRUE on success, EINA_FALSE on failure, if the device is NULL,
 *         or if DWT is not available for the device.
 */
EAPI Eina_Bool
elput_touch_dwt_enabled_set(Elput_Device *device, Eina_Bool enabled)
{
   int ret = -1;

   EINA_SAFETY_ON_NULL_RETURN_VAL(device, EINA_FALSE);

   if (!libinput_device_config_dwt_is_available(device->device))
     return EINA_FALSE;

   if (enabled)
     {
        ret =
          libinput_device_config_dwt_set_enabled(device->device,
                                                 LIBINPUT_CONFIG_DWT_ENABLED);
     }
   else
     {
        ret =
          libinput_device_config_dwt_set_enabled(device->device,
                                                 LIBINPUT_CONFIG_DWT_DISABLED);
     }

   return _check_status(ret);
}

/**
 * @brief Gets the current "disable while typing" (DWT) enabled state for a touchpad.
 * @param device The Elput device.
 * @return EINA_TRUE if DWT is enabled, EINA_FALSE if disabled, on error,
 *         or if DWT is not available for the device.
 */
EAPI Eina_Bool
elput_touch_dwt_enabled_get(Elput_Device *device)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(device, EINA_FALSE);

   if (!libinput_device_config_dwt_is_available(device->device))
     return EINA_FALSE;

   return libinput_device_config_dwt_get_enabled(device->device);
}

/**
 * @brief Sets the scroll method for a touch device.
 *
 * Available methods are device-dependent. Common methods include
 * two-finger scrolling and edge scrolling.
 * See `enum libinput_config_scroll_method` for possible values.
 * Example: `LIBINPUT_CONFIG_SCROLL_2FG` for two-finger scrolling.
 *
 * @param device The Elput device.
 * @param method The scroll method to set.
 * @return EINA_TRUE on success, EINA_FALSE on failure or if the device is NULL.
 */
EAPI Eina_Bool
elput_touch_scroll_method_set(Elput_Device *device, int method)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(device, EINA_FALSE);

   if (libinput_device_config_scroll_set_method(device->device, method) ==
       LIBINPUT_CONFIG_STATUS_SUCCESS)
     return EINA_TRUE;

   return EINA_FALSE;
}

/**
 * @brief Gets the current scroll method for a touch device.
 * @param device The Elput device.
 * @return The current scroll method (e.g., `LIBINPUT_CONFIG_SCROLL_2FG`),
 *         or -1 on error or if the device is NULL.
 *         See `enum libinput_config_scroll_method`.
 */
EAPI int
elput_touch_scroll_method_get(Elput_Device *device)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(device, -1);

   return libinput_device_config_scroll_get_method(device->device);
}

/**
 * @brief Sets the click method for a touch device (usually a clickpad).
 *
 * Click methods determine how button clicks are emulated on a button-less
 * touchpad.
 * See `enum libinput_config_click_method` for possible values.
 * Example: `LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS` or `LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER`.
 *
 * @param device The Elput device.
 * @param method The click method to set.
 * @return EINA_TRUE on success, EINA_FALSE on failure or if the device is NULL.
 */
EAPI Eina_Bool
elput_touch_click_method_set(Elput_Device *device, int method)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(device, EINA_FALSE);

   if (libinput_device_config_click_set_method(device->device, method) ==
       LIBINPUT_CONFIG_STATUS_SUCCESS)
     return EINA_TRUE;

   return EINA_FALSE;
}

/**
 * @brief Gets the current click method for a touch device.
 * @param device The Elput device.
 * @return The current click method (e.g., `LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS`),
 *         or -1 on error or if the device is NULL.
 *         See `enum libinput_config_click_method`.
 */
EAPI int
elput_touch_click_method_get(Elput_Device *device)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(device, -1);

   return libinput_device_config_click_get_method(device->device);
}

/**
 * @brief Enables or disables tap-to-click for a touch device.
 *
 * When enabled, a short touch on the surface is interpreted as a button click.
 *
 * @param device The Elput device.
 * @param enabled EINA_TRUE to enable tap-to-click, EINA_FALSE to disable.
 * @return EINA_TRUE on success, EINA_FALSE on failure or if the device is NULL.
 */
EAPI Eina_Bool
elput_touch_tap_enabled_set(Elput_Device *device, Eina_Bool enabled)
{
   int ret = -1;

   EINA_SAFETY_ON_NULL_RETURN_VAL(device, EINA_FALSE);

   if (enabled)
     {
        ret =
          libinput_device_config_tap_set_enabled(device->device,
                                                 LIBINPUT_CONFIG_TAP_ENABLED);
     }
   else
     {
        ret =
          libinput_device_config_tap_set_enabled(device->device,
                                                 LIBINPUT_CONFIG_TAP_DISABLED);
     }

   return _check_status(ret);
}

/**
 * @brief Gets the current tap-to-click enabled state for a touch device.
 * @param device The Elput device.
 * @return EINA_TRUE if tap-to-click is enabled, EINA_FALSE if disabled or on error.
 */
EAPI Eina_Bool
elput_touch_tap_enabled_get(Elput_Device *device)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(device, EINA_FALSE);

   return libinput_device_config_tap_get_enabled(device->device);
}
