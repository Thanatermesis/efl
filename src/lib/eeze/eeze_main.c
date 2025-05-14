#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Ecore.h>
#include <Eeze.h>
#include <Eeze_Disk.h>
#include <Eeze_Sensor.h>
#include "eeze_udev_private.h"
#include "eeze_net_private.h"
#include "eeze_disk_private.h"
#include "eeze_sensor_private.h"

/**
 * @internal
 * @brief Global udev context.
 * This variable holds the global udev context, initialized by udev_new()
 * during eeze_init() and released by udev_unref() during eeze_shutdown().
 * It is used by various Eeze functions to interact with the udev system.
 */
_udev *udev;

/**
 * @internal
 * @brief Log domain for Eeze udev operations.
 */
int _eeze_udev_log_dom = -1;
/**
 * @internal
 * @brief Log domain for Eeze net operations.
 */
int _eeze_net_log_dom = -1;
/**
 * @internal
 * @brief Log domain for Eeze sensor operations.
 */
int _eeze_sensor_log_dom = -1;
/**
 * @internal
 * @brief Initialization counter for the Eeze library.
 * This counter tracks the number of times eeze_init() has been called.
 * The library is initialized on the first call and shut down when the
 * counter reaches zero after corresponding eeze_shutdown() calls.
 */
int _eeze_init_count = 0;

/**
 * @internal
 * @brief Internal static Eeze library version information.
 */
static Eeze_Version _version = { VMAJ, VMIN, VMIC, VREV };
/**
 * @brief Eeze library version information.
 *
 * Use this to check the version of Eeze your application is linked against.
 * Example:
 * @code
 * const Eeze_Version *version = eeze_version;
 * printf("Eeze version: %d.%d.%d.%d\n",
 *        version->major, version->minor,
 *        version->micro, version->revision);
 * @endcode
 */
EAPI Eeze_Version *eeze_version = &_version;

/**
 * @brief Initialize the Eeze library.
 *
 * This function initializes all the necessary Eeze subsystems. It increments
 * an internal counter, and if the counter is 1, it proceeds with full
 * initialization. This includes initializing Eina, Ecore, registering log
 * domains, initializing udev, and Eeze-specific modules like disk, net,
 * and sensor.
 *
 * @return The new init count, or 0 on failure.
 *
 * @see eeze_shutdown()
 */
EAPI int
eeze_init(void)
{
   if (++_eeze_init_count != 1)
     return _eeze_init_count;

   if (!eina_init())
     return --_eeze_init_count;

   _eeze_udev_log_dom = eina_log_domain_register("eeze_udev", EINA_COLOR_CYAN);
   if (_eeze_udev_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register 'eeze_udev' log domain.");
        goto eina_fail;
     }
   _eeze_net_log_dom = eina_log_domain_register("eeze_net", EINA_COLOR_GREEN);
   if (_eeze_net_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register 'eeze_net' log domain.");
        goto eina_net_fail;
     }

   _eeze_sensor_log_dom = eina_log_domain_register("eeze_sensor", EINA_COLOR_BLUE);
   if (_eeze_sensor_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register 'eeze_sensor' log domain.");
        goto eina_sensor_fail;
     }

   if (!ecore_init())
     goto ecore_fail;
#ifdef HAVE_EEZE_MOUNT
   if (!eeze_disk_init())
     goto eeze_fail;
#endif
   if (!(udev = udev_new()))
     {
        EINA_LOG_ERR("Could not initialize udev library!");
        goto fail;
     }
   if (!eeze_net_init())
     {
        EINA_LOG_ERR("Error initializing eeze_net subsystems!");
        goto net_fail;
     }
   if (!eeze_sensor_init())
     {
        EINA_LOG_ERR("Error initializing eeze_sensor subsystems!");
        goto sensor_fail;
     }

   return _eeze_init_count;

sensor_fail:
   eeze_net_shutdown();
net_fail:
   udev_unref(udev);
fail:
#ifdef HAVE_EEZE_MOUNT
   eeze_disk_shutdown();
eeze_fail:
#endif
   ecore_shutdown();
ecore_fail:
   eina_log_domain_unregister(_eeze_sensor_log_dom);
   _eeze_sensor_log_dom = -1;
eina_sensor_fail:
   eina_log_domain_unregister(_eeze_net_log_dom);
   _eeze_net_log_dom = -1;
eina_net_fail:
   eina_log_domain_unregister(_eeze_udev_log_dom);
   _eeze_udev_log_dom = -1;
eina_fail:
   eina_shutdown();
   return --_eeze_init_count;
}

/**
 * @brief Shut down the Eeze library.
 *
 * This function shuts down all Eeze subsystems. It decrements an internal
 * counter, and if the counter reaches 0, it proceeds with full shutdown.
 * This includes unreferencing the udev context, shutting down Eeze-specific
 * modules, Ecore, and unregistering log domains, and finally shutting down Eina.
 *
 * @return The new init count. If the count reaches 0, it means a full
 *         shutdown was performed. Returns 0 if called when init count
 *         is already zero or less.
 *
 * @see eeze_init()
 */
EAPI int
eeze_shutdown(void)
{
   if (_eeze_init_count <= 0)
     {
        EINA_LOG_ERR("Init count not greater than 0 in shutdown.");
        return 0;
     }
   if (--_eeze_init_count != 0)
     return _eeze_init_count;

   udev_unref(udev);
#ifdef HAVE_EEZE_MOUNT
   eeze_disk_shutdown();
#endif
   eeze_sensor_shutdown();
   eeze_net_shutdown();
   ecore_shutdown();
   eina_log_domain_unregister(_eeze_udev_log_dom);
   _eeze_udev_log_dom = -1;
   eina_log_domain_unregister(_eeze_net_log_dom);
   _eeze_net_log_dom = -1;
   eina_log_domain_unregister(_eeze_sensor_log_dom);
   _eeze_sensor_log_dom = -1;
   eina_shutdown();
   return _eeze_init_count;
}

/**
 * @brief Get the global udev context.
 *
 * This function returns a pointer to the global udev context that Eeze uses.
 * This can be useful for applications that need to perform custom udev
 * operations alongside Eeze. The udev context is managed by Eeze and should
 * not be unreferenced by the caller.
 *
 * @return A pointer to the global _udev context, or NULL if Eeze is not
 *         initialized or udev initialization failed.
 *
 * @warning Do not call udev_unref() on the returned pointer.
 *
 * Example:
 * @code
 * struct udev *ctx = eeze_udev_get();
 * if (ctx)
 *   {
 *      // Use ctx for udev operations
 *   }
 * @endcode
 */
EAPI void *
eeze_udev_get(void)
{
   return udev;
}
