#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Eeze.h>
#include <Eeze_Disk.h>

#include "eeze_udev_private.h"
#include "eeze_disk_private.h"

/**
 * @brief Initializes the libmount dummy interface.
 * @return EINA_TRUE always, as this is a dummy implementation.
 */
Eina_Bool
eeze_libmount_init(void)
{
   return EINA_TRUE;
}

/**
 * @brief Shuts down the libmount dummy interface.
 * This function does nothing in the dummy implementation.
 */
void
eeze_libmount_shutdown(void)
{
}

/**
 * @brief Gets the libmount options for a disk (dummy).
 * @param disk The Eeze_Disk object (unused in dummy).
 * @return 0 always, as this is a dummy implementation.
 */
unsigned long
eeze_disk_libmount_opts_get(Eeze_Disk *disk EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Checks if a disk is mounted (dummy).
 * @param disk The Eeze_Disk object (unused in dummy).
 * @return EINA_FALSE always, as this is a dummy implementation.
 */
Eina_Bool
eeze_disk_libmount_mounted_get(Eeze_Disk *disk EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Finds the source device for a given mount point (dummy).
 * @param mount_point The mount point path (unused in dummy).
 * @return NULL always, as this is a dummy implementation.
 */
const char *
eeze_disk_libmount_mp_find_source(const char *mount_point EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Looks up a mount point by UUID (dummy).
 * @param uuid The UUID string (unused in dummy).
 * @return NULL always, as this is a dummy implementation.
 */
const char *
eeze_disk_libmount_mp_lookup_by_uuid(const char *uuid EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Looks up a mount point by label (dummy).
 * @param label The label string (unused in dummy).
 * @return NULL always, as this is a dummy implementation.
 */
const char *
eeze_disk_libmount_mp_lookup_by_label(const char *label EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Looks up a mount point by device path (dummy).
 * @param devpath The device path string (unused in dummy).
 * @return NULL always, as this is a dummy implementation.
 */
const char *
eeze_disk_libmount_mp_lookup_by_devpath(const char *devpath EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Watches mount tabs for changes (dummy).
 * @return EINA_FALSE always, as this is a dummy implementation and prints an error.
 */
EAPI Eina_Bool
eeze_mount_tabs_watch(void)
{
   ERR("Dummy backend no watching code provided !");
   return EINA_FALSE;
}

/**
 * @brief Unwatches mount tabs (dummy).
 * This function does nothing in the dummy implementation.
 */
EAPI void
eeze_mount_tabs_unwatch(void)
{
}

/**
 * @brief Scans the mtab file (dummy).
 * @return EINA_FALSE always, as this is a dummy implementation.
 */
EAPI Eina_Bool
eeze_mount_mtab_scan(void)
{
   return EINA_FALSE;
}

/**
 * @brief Scans the fstab file (dummy).
 * @return EINA_FALSE always, as this is a dummy implementation.
 */
EAPI Eina_Bool
eeze_mount_fstab_scan(void)
{
   return EINA_FALSE;
}

