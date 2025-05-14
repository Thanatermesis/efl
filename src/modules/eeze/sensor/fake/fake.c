#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <sys/time.h>

#include <Eina.h>
#include <Ecore.h>
#include <Eeze_Sensor.h>
#include "eeze_sensor_private.h"

/**
 * @file
 * @brief This Eeze_Sensor module serves as a test harness for development.
 *
 * It simulates sensor data without interacting with actual hardware.
 * It provides fixed data values but uses accurate timestamps.
 * This is useful for testing applications that consume sensor data
 * without requiring physical sensors.
 */

/* This small Eeze_Sensor module is meant to be used as a test harness for
 * developing. It does not gather any real data from hardware sensors. It uses
 * fixed values for the data, but provides the correct timestamp value.
 */

static int _eeze_sensor_fake_log_dom = -1; /**< Log domain for the fake sensor module. */

#ifdef ERR
#undef ERR
#endif
#define ERR(...)  EINA_LOG_DOM_ERR(_eeze_sensor_fake_log_dom, __VA_ARGS__)

static Eeze_Sensor_Module *esensor_module; /**< Pointer to the Eeze_Sensor_Module structure for this fake module. */

/**
 * @brief A dummy free function for ecore events.
 *
 * This function is used as a callback for ecore_event_add. It intentionally
 * does nothing, preventing the event data (Eeze_Sensor_Obj) from being freed
 * by the event system. The module manages the lifecycle of Eeze_Sensor_Obj
 * instances itself.
 *
 * @param user_data Unused.
 * @param func_data Unused.
 */
static void
_dummy_free(void *user_data EINA_UNUSED, void *func_data EINA_UNUSED)
{
/* Don't free the event data after dispatching the event. We keep track of
 * it on our own
 */
}

/**
 * @brief Initializes the fake sensor module.
 *
 * This function is called by the Eeze sensor core when the module is loaded.
 * It populates the sensor list with all potential sensor types, even if
 * they are not actively providing data. Each sensor object is allocated
 * and its type is set.
 *
 * @return EINA_TRUE on successful initialization, EINA_FALSE otherwise.
 */
static Eina_Bool
fake_init(void)
{
   /* For the fake module we prepare a list with all potential sensors. Even if
    * we only have a small subset at the moment.
    */
   Eeze_Sensor_Type type;

   for (type = 0; type <= EEZE_SENSOR_TYPE_LAST; type++)
     {
        Eeze_Sensor_Obj *obj = calloc(1, sizeof(Eeze_Sensor_Obj));
        obj->type = type;
        esensor_module->sensor_list = eina_list_append(esensor_module->sensor_list, obj);
     }

   return EINA_TRUE;
}

/* We don't have anything to clear when we get unregistered from the core here.
 * This is different in other modules.
 */
/**
 * @brief Shuts down the fake sensor module.
 *
 * This function is called by the Eeze sensor core when the module is unloaded.
 * For this fake module, no specific cleanup is required beyond what the core
 * and the EINA_MODULE_SHUTDOWN macro handle for the sensor_list.
 *
 * @return EINA_TRUE always, as shutdown is trivial.
 */
static Eina_Bool
fake_shutdown(void)
{
   return EINA_TRUE;
}

/**
 * @brief Reads data from a simulated sensor synchronously.
 *
 * Populates the given Eeze_Sensor_Obj with fixed, hardcoded data based on the
 * sensor type. The timestamp is set to the current time.
 *
 * @param obj Pointer to the Eeze_Sensor_Obj to populate with data.
 *            The `obj->type` field determines the kind of data generated.
 *            The `obj->data` array will be filled, e.g.:
 *            - For ACCELEROMETER: `obj->data[0]=x, obj->data[1]=y, obj->data[2]=z`
 *            - For LIGHT: `obj->data[0]=lux_value`
 * @return EINA_TRUE if data was successfully "read" (i.e., generated),
 *         EINA_FALSE if the sensor type is unknown or unsupported.
 */
static Eina_Bool
fake_read(Eeze_Sensor_Obj *obj)
{
   switch (obj->type)
     {
      case EEZE_SENSOR_TYPE_ACCELEROMETER:
      case EEZE_SENSOR_TYPE_GRAVITY:
      case EEZE_SENSOR_TYPE_LINEAR_ACCELERATION:
      case EEZE_SENSOR_TYPE_DEVICE_ORIENTATION:
      case EEZE_SENSOR_TYPE_MAGNETIC:
      case EEZE_SENSOR_TYPE_ORIENTATION:
      case EEZE_SENSOR_TYPE_GYROSCOPE:
        /* This is only a test harness so we supply hardcoded values here */
        obj->accuracy = -1;
        obj->data[0] = 7;
        obj->data[1] = 23;
        obj->data[2] = 42;
        obj->timestamp = ecore_time_get();
        break;

      case EEZE_SENSOR_TYPE_LIGHT:
      case EEZE_SENSOR_TYPE_PROXIMITY:
      case EEZE_SENSOR_TYPE_BAROMETER:
      case EEZE_SENSOR_TYPE_TEMPERATURE:
        obj->accuracy = -1;
        obj->data[0] = 7;
        obj->timestamp = ecore_time_get();
        break;

      default:
        ERR("Not possible to read from this sensor type.");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Initiates an asynchronous "read" from a simulated sensor.
 *
 * This function simulates an asynchronous sensor data update. It populates
 * the Eeze_Sensor_Obj with fixed data and then posts an ecore event
 * corresponding to the sensor type. The actual data is sent via this event.
 *
 * @param obj Pointer to the Eeze_Sensor_Obj representing the sensor.
 *            The `obj->type` field determines the event type and data structure.
 *            The `obj->data` array will be filled with hardcoded values, e.g.:
 *            - For GYROSCOPE: `obj->data[0]=x_rate, obj->data[1]=y_rate, obj->data[2]=z_rate`
 *            - For PROXIMITY: `obj->data[0]=distance`
 * @param user_data Custom data to be associated with the sensor object,
 *                  stored in `obj->user_data`.
 * @return EINA_TRUE if the asynchronous read was successfully initiated,
 *         EINA_FALSE if the sensor type is unknown or unsupported.
 */
static Eina_Bool
fake_async_read(Eeze_Sensor_Obj *obj, void *user_data)
{
   if (user_data)
     obj->user_data = user_data;

   /* Default values for sensor objects with three data points */
   obj->accuracy = -1;
   obj->data[0] = 7;
   obj->data[1] = 23;
   obj->data[2] = 42;
   obj->timestamp = ecore_time_get();

   switch (obj->type)
     {
      case EEZE_SENSOR_TYPE_ACCELEROMETER:
        ecore_event_add(EEZE_SENSOR_EVENT_ACCELEROMETER, obj, _dummy_free, NULL);
        break;
      case EEZE_SENSOR_TYPE_GRAVITY:
        ecore_event_add(EEZE_SENSOR_EVENT_GRAVITY, obj, _dummy_free, NULL);
        break;
      case EEZE_SENSOR_TYPE_LINEAR_ACCELERATION:
        ecore_event_add(EEZE_SENSOR_EVENT_LINEAR_ACCELERATION, obj, _dummy_free, NULL);
        break;
      case EEZE_SENSOR_TYPE_DEVICE_ORIENTATION:
        ecore_event_add(EEZE_SENSOR_EVENT_DEVICE_ORIENTATION, obj, _dummy_free, NULL);
        break;
      case EEZE_SENSOR_TYPE_MAGNETIC:
        ecore_event_add(EEZE_SENSOR_EVENT_MAGNETIC, obj, _dummy_free, NULL);
        break;
      case EEZE_SENSOR_TYPE_ORIENTATION:
        ecore_event_add(EEZE_SENSOR_EVENT_ORIENTATION, obj, _dummy_free, NULL);
        break;
      case EEZE_SENSOR_TYPE_GYROSCOPE:
        ecore_event_add(EEZE_SENSOR_EVENT_GYROSCOPE, obj, _dummy_free, NULL);
        break;
      case EEZE_SENSOR_TYPE_LIGHT:
        /* Reset values that are not used for sensor object with one data point */
        obj->data[1] = 0;
        obj->data[2] = 0;
        ecore_event_add(EEZE_SENSOR_EVENT_LIGHT, obj, _dummy_free, NULL);
        break;
      case EEZE_SENSOR_TYPE_PROXIMITY:
        obj->data[1] = 0;
        obj->data[2] = 0;
        ecore_event_add(EEZE_SENSOR_EVENT_PROXIMITY, obj, _dummy_free, NULL);
        break;
      case EEZE_SENSOR_TYPE_BAROMETER:
        obj->data[1] = 0;
        obj->data[2] = 0;
        ecore_event_add(EEZE_SENSOR_EVENT_BAROMETER, obj, _dummy_free, NULL);
        break;
      case EEZE_SENSOR_TYPE_TEMPERATURE:
        obj->data[1] = 0;
        obj->data[2] = 0;
        ecore_event_add(EEZE_SENSOR_EVENT_TEMPERATURE, obj, _dummy_free, NULL);
        break;

      default:
        ERR("Not possible to read from this sensor type.");
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/* This function gets called when the module is loaded from the disk. Its the
 * entry point to anything in this module. After setting ourself up we register
 * into the core of eeze sensor to make our functionality available.
 */
/**
 * @brief Module initialization function, called when the module is loaded.
 *
 * This is the entry point for the fake sensor module. It performs the
 * following actions:
 * 1. Registers a log domain for the module.
 * 2. Allocates and initializes the Eeze_Sensor_Module structure.
 * 3. Sets up function pointers for init, shutdown, read, and async_read
 *    to point to the local `fake_*` implementations.
 * 4. Registers this module with the Eeze sensor core under the name "fake".
 *
 * @return EINA_TRUE on successful initialization and registration,
 *         EINA_FALSE otherwise.
 */
static Eina_Bool
sensor_fake_init(void)
{

   _eeze_sensor_fake_log_dom = eina_log_domain_register("eeze_sensor_fake", EINA_COLOR_BLUE);
   if (_eeze_sensor_fake_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register 'eeze_sensor_fake' log domain.");
        return EINA_FALSE;
     }

   /* Check to avoid multi-init */
   if (esensor_module) return EINA_FALSE;

   /* Set module function pointer to allow calls into the module */
   esensor_module = calloc(1, sizeof(Eeze_Sensor_Module));
   if (!esensor_module) return EINA_FALSE;

   /* Setup our function pointers to allow the core accessing this modules
    * functions
    */
   esensor_module->init = fake_init;
   esensor_module->shutdown = fake_shutdown;
   esensor_module->read = fake_read;
   esensor_module->async_read = fake_async_read;

   if (!eeze_sensor_module_register("fake", esensor_module))
     {
        ERR("Failed to register fake module.");
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/* Cleanup when the module gets unloaded. Unregister ourself from the core to
 * avoid calls into a not loaded module.
 */
/**
 * @brief Module shutdown function, called when the module is unloaded.
 *
 * This function performs cleanup tasks for the fake sensor module:
 * 1. Unregisters the module from the Eeze sensor core.
 * 2. Frees all Eeze_Sensor_Obj instances stored in the `sensor_list`.
 * 3. Unregisters the log domain.
 * 4. Frees the Eeze_Sensor_Module structure.
 * 5. Resets static global variables.
 */
static void
sensor_fake_shutdown(void)
{
   Eeze_Sensor_Obj *sens;

   eeze_sensor_module_unregister("fake");
   EINA_LIST_FREE(esensor_module->sensor_list, sens)
      free(sens);

   eina_log_domain_unregister(_eeze_sensor_fake_log_dom);

   free(esensor_module);
   esensor_module = NULL;

   _eeze_sensor_fake_log_dom = -1;
}

EINA_MODULE_INIT(sensor_fake_init);
EINA_MODULE_SHUTDOWN(sensor_fake_shutdown);
