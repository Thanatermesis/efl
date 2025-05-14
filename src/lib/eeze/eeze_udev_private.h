#ifndef EEZE_UDEV_PRIVATE_H
#define EEZE_UDEV_PRIVATE_H
#include <Eeze.h>

#define LIBUDEV_I_KNOW_THE_API_IS_SUBJECT_TO_CHANGE 1
#include <libudev.h>

#ifndef EEZE_UDEV_COLOR_DEFAULT
#define EEZE_UDEV_COLOR_DEFAULT EINA_COLOR_CYAN
#endif
extern int _eeze_udev_log_dom;
#ifdef ERR
#undef ERR
#endif
#ifdef INF
#undef INF
#endif
#ifdef WARN
#undef WARN
#endif
#ifdef DBG
#undef DBG
#endif

#define DBG(...)   EINA_LOG_DOM_DBG(_eeze_udev_log_dom, __VA_ARGS__)
#define INF(...)    EINA_LOG_DOM_INFO(_eeze_udev_log_dom, __VA_ARGS__)
#define WARN(...) EINA_LOG_DOM_WARN(_eeze_udev_log_dom, __VA_ARGS__)
#define ERR(...)   EINA_LOG_DOM_ERR(_eeze_udev_log_dom, __VA_ARGS__)

/* typedefs because I'm lazy */
typedef struct udev _udev;
typedef struct udev_list_entry _udev_list_entry;
typedef struct udev_device _udev_device;
typedef struct udev_enumerate _udev_enumerate;
typedef struct udev_monitor _udev_monitor;

extern _udev *udev; /**< Global udev library context */

/**
 * @brief Creates a new udev device object from a syspath.
 * @param syspath The sysfs path of the device (e.g., "/sys/class/power_supply/BAT0").
 * @return A new udev_device object, or NULL on failure.
 */
_udev_device *_new_device(const char *syspath);

/**
 * @brief Walks down the device tree from a given syspath to find a child device
 *        that has a specific sysattr or property.
 * @param syspath The starting sysfs path.
 * @param sysattr The name of the sysfs attribute or udev property to look for.
 * @param subsystem Optional: Filter children by subsystem (e.g., "power_supply").
 * @param property If EINA_TRUE, sysattr is treated as a udev property name;
 *                 otherwise, it's treated as a sysfs attribute name.
 * @return The stringshared value of the found attribute/property, or NULL if not found.
 *         The caller is responsible for stringsharing this value if needed further.
 */
const char *_walk_children_get_attr(const char *syspath, const char *sysattr, const char *subsystem, Eina_Bool property);

/**
 * @brief Walks up the device tree from a given device to check if any parent
 *        (or the device itself) has a specific sysfs attribute, optionally matching a value.
 * @param device The udev_device object to start from.
 * @param sysattr The name of the sysfs attribute to test.
 * @param value Optional: The value the sysfs attribute should have. If NULL,
 *              only the existence of the attribute is checked.
 * @return EINA_TRUE if the attribute (and optionally value) is found, EINA_FALSE otherwise.
 */
Eina_Bool _walk_parents_test_attr(_udev_device *device, const char *sysattr, const char* value);

/**
 * @brief Walks up the device tree from a given device to get the value of a
 *        sysfs attribute or udev property from the first parent (or device itself) that has it.
 * @param device The udev_device object to start from.
 * @param sysattr The name of the sysfs attribute or udev property.
 * @param property If EINA_TRUE, sysattr is treated as a udev property name;
 *                 otherwise, it's treated as a sysfs attribute name.
 * @return The stringshared value of the found attribute/property, or NULL if not found.
 *         The caller is responsible for stringsharing this value if needed further.
 */
const char *_walk_parents_get_attr(_udev_device *device, const char *sysattr, Eina_Bool property);

/**
 * @brief Traverses up the parent chain of a given udev device and adds the syspaths
 *        of parents to the provided list if they are not already present and share
 *        similar vendor/model identifiers.
 * @param list An Eina_List of stringshared syspaths. This list will be modified.
 * @param device The udev_device to start traversing from.
 * @return The (potentially modified) Eina_List containing stringshared syspaths.
 *         The list elements are stringshared syspaths of the parents.
 *         Example: If device is a USB mouse, this might add its USB controller parent.
 */
Eina_List *_get_unlisted_parents(Eina_List *list, _udev_device *device);

/**
 * @brief Creates a reference (increments refcount) to an existing udev_device.
 * @param device The udev_device object to reference.
 * @return A new reference to the udev_device.
 */
_udev_device *_copy_device(_udev_device *device);

#endif
