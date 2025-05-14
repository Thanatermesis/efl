#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <Eina.h>
#include <Ecore.h>
#include <Eeze_Sensor.h>
#include "eeze_sensor_private.h"
#include "../../static_libs/buildsystem/buildsystem.h"

/** @file
 * @brief This file implements the Eeze sensor API.
 */

/** @brief Event type for accelerometer sensor data. */
EAPI int EEZE_SENSOR_EVENT_ACCELEROMETER;
/** @brief Event type for gravity sensor data. */
EAPI int EEZE_SENSOR_EVENT_GRAVITY;
/** @brief Event type for linear acceleration sensor data. */
EAPI int EEZE_SENSOR_EVENT_LINEAR_ACCELERATION;
/** @brief Event type for device orientation sensor data. */
EAPI int EEZE_SENSOR_EVENT_DEVICE_ORIENTATION;
/** @brief Event type for magnetic field sensor data. */
EAPI int EEZE_SENSOR_EVENT_MAGNETIC;
/** @brief Event type for orientation sensor data. */
EAPI int EEZE_SENSOR_EVENT_ORIENTATION;
/** @brief Event type for gyroscope sensor data. */
EAPI int EEZE_SENSOR_EVENT_GYROSCOPE;
/** @brief Event type for light sensor data. */
EAPI int EEZE_SENSOR_EVENT_LIGHT;
/** @brief Event type for proximity sensor data. */
EAPI int EEZE_SENSOR_EVENT_PROXIMITY;
/** @brief Event type for snap gesture sensor data. */
EAPI int EEZE_SENSOR_EVENT_SNAP;
/** @brief Event type for shake gesture sensor data. */
EAPI int EEZE_SENSOR_EVENT_SHAKE;
/** @brief Event type for double tap gesture sensor data. */
EAPI int EEZE_SENSOR_EVENT_DOUBLETAP;
/** @brief Event type for panning gesture sensor data. */
EAPI int EEZE_SENSOR_EVENT_PANNING;
/** @brief Event type for panning browse gesture sensor data. */
EAPI int EEZE_SENSOR_EVENT_PANNING_BROWSE;
/** @brief Event type for tilt gesture sensor data. */
EAPI int EEZE_SENSOR_EVENT_TILT;
/** @brief Event type for facedown gesture sensor data. */
EAPI int EEZE_SENSOR_EVENT_FACEDOWN;
/** @brief Event type for direct call gesture sensor data. */
EAPI int EEZE_SENSOR_EVENT_DIRECT_CALL;
/** @brief Event type for smart alert gesture sensor data. */
EAPI int EEZE_SENSOR_EVENT_SMART_ALERT;
/** @brief Event type for no move gesture sensor data. */
EAPI int EEZE_SENSOR_EVENT_NO_MOVE;
/** @brief Event type for barometer sensor data. */
EAPI int EEZE_SENSOR_EVENT_BAROMETER;
/** @brief Event type for temperature sensor data. */
EAPI int EEZE_SENSOR_EVENT_TEMPERATURE;

/** @internal Global handle for Eeze_Sensor library. */
static Eeze_Sensor *g_handle;
/** @internal Eina_Prefix for library path handling. */
static Eina_Prefix *pfx;

/* Priority order for modules. The one with the highest order of the available
 * ones will be used. This in good enough for now as we only have three modules
 * and one is a test harness anyway. If the number of modules grows we might
 * re-think the priority handling, but we should do this when the need arise.
 */
/** @internal Array defining the priority order of sensor modules.
 * Modules are checked in this order, and the first one found is used.
 * Example: {"fake", "udev", NULL} means "fake" is checked first, then "udev".
 */
static const char *_module_priority[] = {
   "fake",
   "udev",
   NULL
};

/* Search through the list of loaded module and return the one with the highest
 * priority.
 */
/**
 * @internal
 * @brief Retrieves the loaded sensor module with the highest priority.
 *
 * This function iterates through the `_module_priority` list and returns the
 * first module found in the `g_handle->modules` hash.
 *
 * @return A pointer to the highest priority Eeze_Sensor_Module, or NULL if no
 *         suitable module is found.
 */
Eeze_Sensor_Module *
_highest_priority_module_get(void)
{
   Eeze_Sensor_Module *module = NULL;
   int i = 0;

   while (_module_priority[i] != NULL)
     {
        module = eina_hash_find(g_handle->modules, _module_priority[i]);
        if (module) return module;
        i++;
     }
   return NULL;
}

/* Utility function to take the given sensor type and get the matching sensor
 * object from the highest priority module.
 */
/**
 * @brief Retrieves a sensor object for a given sensor type from the
 *        highest priority module.
 *
 * This function first determines the highest priority module available.
 * Then, it searches within that module's sensor list for a sensor matching
 * the specified `sensor_type`. If found, a copy of the sensor object is
 * allocated and returned. The caller is responsible for freeing this object
 * using `eeze_sensor_free()`.
 *
 * @param sensor_type The type of sensor to retrieve (e.g., EEZE_SENSOR_TYPE_ACCELEROMETER).
 * @return A newly allocated Eeze_Sensor_Obj for the requested sensor type,
 *         or NULL if the sensor type is not available or an error occurs.
 */
EAPI Eeze_Sensor_Obj *
eeze_sensor_obj_get(Eeze_Sensor_Type sensor_type)
{
   Eina_List *l;
   Eeze_Sensor_Obj *obj, *sens;
   Eeze_Sensor_Module *module;

   module = _highest_priority_module_get();

   if (!module) return NULL;

   EINA_LIST_FOREACH(module->sensor_list, l, obj)
     {
        if (obj->type == sensor_type)
          {
             sens = calloc(1, sizeof(Eeze_Sensor_Obj));
             if (!sens) return NULL;

             memcpy(sens, obj, sizeof(Eeze_Sensor_Obj));

             return sens;
          }
     }
   return NULL;
}

/**
 * @internal
 * @brief Loads available Eeze sensor modules.
 *
 * This function searches for sensor modules in predefined locations.
 * It prioritizes modules in the build directory if the `EFL_RUN_IN_TREE`
 * environment variable is set and effective UIDs match. Otherwise, it looks
 * in the system library directory.
 * All found modules are added to `g_handle->modules_array` and then loaded.
 */
static void
eeze_sensor_modules_load(void)
{
   char buf[PATH_MAX];

   /* Check for available runtime modules and load them. In some cases the
    * un-installed modules to be used from the local build dir. Coverage check
    * is one of these items. We do load the modules from the builddir if the
    * environment is set. Normal case is to use installed modules from system
    */
#ifdef NEED_RUN_IN_TREE
   if (
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
       (getuid() == geteuid()) &&
#endif
       (getenv("EFL_RUN_IN_TREE")))
     {
        const char **itr;

        for (itr = _module_priority; *itr != NULL; itr++)
          {
             bs_mod_dir_get(buf, sizeof(buf), "eeze/sensor", *itr);
             g_handle->modules_array = eina_module_list_get(
                g_handle->modules_array, buf, EINA_FALSE, NULL, NULL);
          }
     }
   else
#endif
     {
        snprintf(buf, sizeof(buf), "%s/eeze/modules/sensor",
                 eina_prefix_lib_get(pfx));
        g_handle->modules_array = eina_module_arch_list_get(NULL, buf,
                                                            MODULE_ARCH);
     }

   if (!g_handle->modules_array)
     {
        ERR("No modules found!");
        return;
     }
   // XXX: MODFIX: do not list ALL modules and load them ALL! this is
   // this will, for example, load both udev AND fake modules - run
   // their init funcs etc. etc. why? no!
   eina_module_list_load(g_handle->modules_array);
}

/**
 * @internal
 * @brief Unloads all loaded Eeze sensor modules.
 *
 * This function unloads modules listed in `g_handle->modules_array`,
 * frees the list, and clears the array.
 */
static void
eeze_sensor_modules_unload(void)
{
   if (!g_handle->modules_array) return;
   eina_module_list_unload(g_handle->modules_array);
   eina_module_list_free(g_handle->modules_array);
   eina_array_free(g_handle->modules_array);
   g_handle->modules_array = NULL;
}

/* This function is offered to the modules to register itself after they have
 * been loaded in initialized. They stay in the hash function until they
 * unregister themself.
 */
/**
 * @brief Registers a sensor module with the Eeze sensor system.
 *
 * This function is called by sensor modules themselves after they have been
 * loaded and initialized. The module is added to a hash table for later lookup.
 * The module's `init` function is called as part of the registration.
 *
 * @param name The name of the module to register (e.g., "udev", "fake").
 * @param mod A pointer to the Eeze_Sensor_Module structure representing the module.
 * @return EINA_TRUE on successful registration, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
eeze_sensor_module_register(const char *name, Eeze_Sensor_Module *mod)
{
   Eeze_Sensor_Module *module = NULL;

   if (!mod) return EINA_FALSE;

   module = mod;

   if (!module->init) return EINA_FALSE;
   if (!(module->init())) return EINA_FALSE;

   INF("Registered module %s", name);

   return eina_hash_add(g_handle->modules, name, module);
}

/* This function is offered to the modules to unregsiter itself. When requested
 * we remove them safely from the hash.
 */
/**
 * @brief Unregisters a sensor module from the Eeze sensor system.
 *
 * This function is called by sensor modules when they are being unloaded.
 * It removes the module from the internal hash table.
 * The module's `shutdown` function is called before removal if it exists.
 *
 * @param name The name of the module to unregister.
 * @return EINA_TRUE on successful unregistration, EINA_FALSE if the module
 *         was not found or an error occurred.
 */
EAPI Eina_Bool
eeze_sensor_module_unregister(const char *name)
{
   DBG("Unregister module %s", name);

   Eeze_Sensor_Module *module = NULL;

   module = eina_hash_find(g_handle->modules, name);
   if (!module) return EINA_FALSE;

   if (module->shutdown)
     module->shutdown();

   return eina_hash_del(g_handle->modules, name, NULL);
}

/* Create a new sensor object for a given sensor type. This functions allocates
 * the needed memory and links it with the matching sensor from the loaded
 * modules. It also does an initial synchronous read to fill the sensor object
 * with values.
 * Make sure to use the eeze_sensor_free function to remove this sensor object
 * when it is no longer needed.
 */
/**
 * @brief Creates a new sensor object for a given sensor type.
 *
 * This function allocates memory for a new Eeze_Sensor_Obj and links it
 * with the corresponding sensor from the highest priority loaded module.
 * It performs an initial synchronous read to populate the sensor object
 * with current values.
 *
 * @param type The type of sensor to create (e.g., EEZE_SENSOR_TYPE_ACCELEROMETER).
 * @return A pointer to the newly created Eeze_Sensor_Obj, or NULL if the
 *         sensor type is not available, no module provides it, or an error occurs.
 *         The returned object must be freed using `eeze_sensor_free()`.
 */
EAPI Eeze_Sensor_Obj *
eeze_sensor_new(Eeze_Sensor_Type type)
{
   Eeze_Sensor_Obj *sens;
   Eeze_Sensor_Module *module = NULL;

   sens = eeze_sensor_obj_get(type);
   if (!sens) return NULL;

   module = _highest_priority_module_get();
   if (!module)
     {
      free(sens);
      return NULL;
     }

   if (!module->read)
     {
      free(sens);
      return NULL;
     }

   /* The read is asynchronous here as we want to make sure that the sensor
    * object has valid data when created. As we give back cached values we
    * have a race condition when we do a asynchronous read here and the
    * application asks for cached data before the reply came in. This logic has
    * the downside that the sensor creation takes longer. But that is only a
    *initial cost.
    */
   if (module->read(sens))
      return sens;

   free(sens);
   return NULL;
}

/* Free sensor object created with eeze_sensor_new */
/**
 * @brief Frees a sensor object created with `eeze_sensor_new()`.
 *
 * @param sens The sensor object to free. If NULL, the function does nothing.
 */
EAPI void
eeze_sensor_free(Eeze_Sensor_Obj *sens)
{
   if (!sens) return;
   free(sens);
}

/* All of the below getter function do access the cached data from the last
 * sensor read. It is way faster this way but also means that the timestamp
 * should be checked to ensure recent data if needed.
 */
/**
 * @brief Gets the accuracy level of the sensor reading.
 *
 * This function retrieves the cached accuracy value from the sensor object.
 *
 * @param sens The sensor object.
 * @param[out] accuracy Pointer to an integer where the accuracy level will be stored.
 *                      The meaning of this value is sensor-dependent but generally
 *                      higher values mean better accuracy.
 * @return EINA_TRUE on success, EINA_FALSE if `sens` is NULL.
 */
EAPI Eina_Bool
eeze_sensor_accuracy_get(Eeze_Sensor_Obj *sens, int *accuracy)
{
   if (!sens) return EINA_FALSE;

   *accuracy = sens->accuracy;
   return EINA_TRUE;
}

/**
 * @brief Gets the three-axis (X, Y, Z) sensor data.
 *
 * This function retrieves cached X, Y, and Z values from the sensor object.
 * Applicable to sensors like accelerometers, gyroscopes, magnetometers.
 *
 * @param sens The sensor object.
 * @param[out] x Pointer to a float where the X-axis value will be stored.
 * @param[out] y Pointer to a float where the Y-axis value will be stored.
 * @param[out] z Pointer to a float where the Z-axis value will be stored.
 * @return EINA_TRUE on success, EINA_FALSE if `sens` is NULL.
 */
EAPI Eina_Bool
eeze_sensor_xyz_get(Eeze_Sensor_Obj *sens, float *x, float *y, float *z)
{
   if (!sens) return EINA_FALSE;

   *x = sens->data[0];
   *y = sens->data[1];
   *z = sens->data[2];
   return EINA_TRUE;
}

/**
 * @brief Gets the two-axis (X, Y) sensor data.
 *
 * This function retrieves cached X and Y values from the sensor object.
 *
 * @param sens The sensor object.
 * @param[out] x Pointer to a float where the X-axis value will be stored.
 * @param[out] y Pointer to a float where the Y-axis value will be stored.
 * @return EINA_TRUE on success, EINA_FALSE if `sens` is NULL.
 */
EAPI Eina_Bool
eeze_sensor_xy_get(Eeze_Sensor_Obj *sens, float *x, float *y)
{
   if (!sens) return EINA_FALSE;

   *x = sens->data[0];
   *y = sens->data[1];
   return EINA_TRUE;
}

/**
 * @brief Gets single-axis (X) sensor data.
 *
 * This function retrieves the cached X value (data[0]) from the sensor object.
 * Applicable to sensors like light, proximity, temperature, barometer.
 *
 * @param sens The sensor object.
 * @param[out] x Pointer to a float where the sensor value will be stored.
 * @return EINA_TRUE on success, EINA_FALSE if `sens` is NULL.
 */
EAPI Eina_Bool
eeze_sensor_x_get(Eeze_Sensor_Obj *sens, float *x)
{
   if (!sens) return EINA_FALSE;

   *x = sens->data[0];
   return EINA_TRUE;
}

/**
 * @brief Gets the timestamp of the last sensor reading.
 *
 * This function retrieves the cached timestamp from the sensor object.
 * The timestamp typically represents time in seconds since the Epoch or
 * system boot, depending on the underlying module.
 *
 * @param sens The sensor object.
 * @param[out] timestamp Pointer to a double where the timestamp will be stored.
 * @return EINA_TRUE on success, EINA_FALSE if `sens` is NULL.
 */
EAPI Eina_Bool
eeze_sensor_timestamp_get(Eeze_Sensor_Obj *sens, double *timestamp)
{
   if (!sens) return EINA_FALSE;

   *timestamp = sens->timestamp;
   return EINA_TRUE;
}

/* Synchronous read. Blocked until the data was readout from the hardware
 * sensor
 */
/**
 * @brief Performs a synchronous read of the sensor data.
 *
 * This function blocks until the data has been read from the hardware sensor.
 * The sensor object's cached values are updated upon successful read.
 *
 * @param sens The sensor object to read data for.
 * @return EINA_TRUE if the read was successful, EINA_FALSE otherwise (e.g.,
 *         `sens` is NULL, no module found, or module's read function fails).
 */
EAPI Eina_Bool
eeze_sensor_read(Eeze_Sensor_Obj *sens)
{
   Eeze_Sensor_Module *module = NULL;

   if (!sens) return EINA_FALSE;

   module = _highest_priority_module_get();
   if (!module) return EINA_FALSE;

   if (module->read)
     return module->read(sens);

   return EINA_FALSE;
}

/* Asynchronous read. Schedule a new read out that will update the cached values
 * as soon as it arrives.
 */
/**
 * @brief Performs an asynchronous read of the sensor data.
 *
 * This function schedules a new readout from the hardware sensor.
 * The sensor object's cached values will be updated when the data arrives.
 * An event (specific to the sensor type) is typically generated upon completion.
 *
 * @param sens The sensor object to read data for.
 * @param user_data User-defined data to be passed to the completion callback
 *                  or event, if applicable by the module.
 * @return EINA_TRUE if the asynchronous read was successfully initiated,
 *         EINA_FALSE otherwise (e.g., `sens` is NULL, no module found, or
 *         module's async_read function is not implemented or fails).
 */
EAPI Eina_Bool
eeze_sensor_async_read(Eeze_Sensor_Obj *sens, void *user_data)
{
   Eeze_Sensor_Module *module = NULL;

   if (!sens) return EINA_FALSE;

   module = _highest_priority_module_get();
   if (!module) return EINA_FALSE;
   if (module->async_read)
     return module->async_read(sens, user_data);

   return EINA_FALSE;
}

/**
 * @brief Shuts down the Eeze sensor system.
 *
 * This function performs necessary cleanup for the Eeze sensor library.
 * It flushes all sensor-related ecore events, unloads sensor modules,
 * frees global handles and the Eina_Prefix.
 * It also calls `eina_shutdown()`.
 */
void
eeze_sensor_shutdown(void)
{
   ecore_event_type_flush(EEZE_SENSOR_EVENT_ACCELEROMETER,
                          EEZE_SENSOR_EVENT_GRAVITY,
                          EEZE_SENSOR_EVENT_LINEAR_ACCELERATION,
                          EEZE_SENSOR_EVENT_DEVICE_ORIENTATION,
                          EEZE_SENSOR_EVENT_MAGNETIC,
                          EEZE_SENSOR_EVENT_ORIENTATION,
                          EEZE_SENSOR_EVENT_GYROSCOPE,
                          EEZE_SENSOR_EVENT_LIGHT,
                          EEZE_SENSOR_EVENT_PROXIMITY,
                          EEZE_SENSOR_EVENT_SNAP,
                          EEZE_SENSOR_EVENT_SHAKE,
                          EEZE_SENSOR_EVENT_DOUBLETAP,
                          EEZE_SENSOR_EVENT_PANNING,
                          EEZE_SENSOR_EVENT_PANNING_BROWSE,
                          EEZE_SENSOR_EVENT_TILT,
                          EEZE_SENSOR_EVENT_FACEDOWN,
                          EEZE_SENSOR_EVENT_DIRECT_CALL,
                          EEZE_SENSOR_EVENT_SMART_ALERT,
                          EEZE_SENSOR_EVENT_NO_MOVE,
                          EEZE_SENSOR_EVENT_BAROMETER,
                          EEZE_SENSOR_EVENT_TEMPERATURE);
   eeze_sensor_modules_unload();
   eina_hash_free(g_handle->modules);
   g_handle->modules = NULL;
   free(g_handle);
   g_handle = NULL;

   eina_prefix_free(pfx);
   pfx = NULL;

   eina_shutdown();
}

/**
 * @brief Initializes the Eeze sensor system.
 *
 * This function initializes necessary components for the Eeze sensor library.
 * It initializes Eina, sets up an Eina_Prefix for path resolution,
 * allocates global handles, creates ecore event types for various sensors,
 * and loads available sensor modules.
 * This must be called before any other eeze_sensor_* functions.
 *
 * @return EINA_TRUE on successful initialization, EINA_FALSE otherwise.
 */
Eina_Bool
eeze_sensor_init(void)
{
   if (!eina_init()) return EINA_FALSE;

   pfx = eina_prefix_new(NULL, eeze_sensor_init, "EEZE", "eeze", "checkme",
                         PACKAGE_BIN_DIR, PACKAGE_LIB_DIR,
                         PACKAGE_DATA_DIR, PACKAGE_DATA_DIR);

   g_handle = calloc(1, sizeof(Eeze_Sensor));
   if (!g_handle) return EINA_FALSE;

   g_handle->modules_array = NULL;
   g_handle->modules = eina_hash_string_small_new(NULL);
   if (!g_handle->modules) return EINA_FALSE;

   /* Make sure we create new ecore event types before using them */
   EEZE_SENSOR_EVENT_ACCELEROMETER = ecore_event_type_new();
   EEZE_SENSOR_EVENT_GRAVITY = ecore_event_type_new();
   EEZE_SENSOR_EVENT_LINEAR_ACCELERATION = ecore_event_type_new();
   EEZE_SENSOR_EVENT_DEVICE_ORIENTATION = ecore_event_type_new();
   EEZE_SENSOR_EVENT_MAGNETIC = ecore_event_type_new();
   EEZE_SENSOR_EVENT_ORIENTATION = ecore_event_type_new();
   EEZE_SENSOR_EVENT_GYROSCOPE = ecore_event_type_new();
   EEZE_SENSOR_EVENT_LIGHT = ecore_event_type_new();
   EEZE_SENSOR_EVENT_PROXIMITY = ecore_event_type_new();
   EEZE_SENSOR_EVENT_SNAP = ecore_event_type_new();
   EEZE_SENSOR_EVENT_SHAKE = ecore_event_type_new();
   EEZE_SENSOR_EVENT_DOUBLETAP = ecore_event_type_new();
   EEZE_SENSOR_EVENT_PANNING = ecore_event_type_new();
   EEZE_SENSOR_EVENT_PANNING_BROWSE = ecore_event_type_new();
   EEZE_SENSOR_EVENT_TILT = ecore_event_type_new();
   EEZE_SENSOR_EVENT_FACEDOWN = ecore_event_type_new();
   EEZE_SENSOR_EVENT_DIRECT_CALL = ecore_event_type_new();
   EEZE_SENSOR_EVENT_SMART_ALERT = ecore_event_type_new();
   EEZE_SENSOR_EVENT_NO_MOVE = ecore_event_type_new();
   EEZE_SENSOR_EVENT_BAROMETER = ecore_event_type_new();
   EEZE_SENSOR_EVENT_TEMPERATURE = ecore_event_type_new();

   /* Core is ready so we can load the modules from disk now */
   eeze_sensor_modules_load();

   return EINA_TRUE;
}
