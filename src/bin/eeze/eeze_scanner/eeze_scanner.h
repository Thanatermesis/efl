#ifndef EEZE_SCANNER_H
#define EEZE_SCANNER_H

#include <Eeze.h>

/**
 * @brief Macro to set up an Eet_Data_Descriptor for Eeze_Scanner_Event.
 *
 * This macro simplifies the creation of an Eet_Data_Descriptor for serializing
 * and deserializing Eeze_Scanner_Event structures.
 *
 * @param edd The Eet_Data_Descriptor to set up.
 */
#define EEZE_SCANNER_EDD_SETUP(edd) \
  EET_DATA_DESCRIPTOR_ADD_BASIC((edd), Eeze_Scanner_Event, "device", device, EET_T_INLINED_STRING); \
  EET_DATA_DESCRIPTOR_ADD_BASIC((edd), Eeze_Scanner_Event, "type", type, EET_T_UINT); \
  EET_DATA_DESCRIPTOR_ADD_BASIC((edd), Eeze_Scanner_Event, "volume", volume, EET_T_UCHAR)

/**
 * @brief Defines the types of events that the eeze scanner can report.
 *
 * These event types correspond to udev events for device changes.
 */
typedef enum
{
   EEZE_SCANNER_EVENT_TYPE_NONE, /**< No specific event type. */
   EEZE_SCANNER_EVENT_TYPE_ADD = EEZE_UDEV_EVENT_ADD, /**< A device has been added. */
   EEZE_SCANNER_EVENT_TYPE_REMOVE = EEZE_UDEV_EVENT_REMOVE, /**< A device has been removed. */
   EEZE_SCANNER_EVENT_TYPE_CHANGE = EEZE_UDEV_EVENT_CHANGE /**< A device has changed state. */
} Eeze_Scanner_Event_Type;

/**
 * @brief Structure representing a scanner event.
 *
 * This structure is used to communicate device events (add, remove, change)
 * to clients.
 */
typedef struct
{
   const char *device; /**< The syspath of the device related to the event. */
   Eeze_Scanner_Event_Type type; /**< The type of the event. */
   Eina_Bool volume; /**< EINA_TRUE if the event is for a volume, EINA_FALSE for a storage device. */
} Eeze_Scanner_Event;

/**
 * @brief Structure representing a scanned device, particularly for CD-ROMs.
 *
 * This structure holds information about a device being monitored,
 * including a poller for CD-ROM status checks.
 */
typedef struct
{
   Ecore_Poller *poller; /**< Poller used for CD-ROM devices to check for media changes. NULL for other devices. */
   const char *device; /**< The syspath of the device. */
   Eina_Bool mounted; /**< EINA_TRUE if the device (typically a CD-ROM) is currently considered mounted or has media. */
} Eeze_Scanner_Device;

#endif
