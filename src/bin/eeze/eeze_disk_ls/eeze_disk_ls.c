#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include <Ecore.h>
#include <Eeze.h>
#include <Eeze_Disk.h>

/**
 * @file
 * @brief A simple application to list available disks and their properties.
 *
 * This program utilizes the Eeze library to find and display information
 * about mountable, removable, and internal drives, as well as GPIO devices.
 * For each device, it typically prints its system path, device path, and
 * mount point (if applicable).
 */

/* simple app to print disks and their mount points */

/**
 * @brief Main function for the eeze_disk_ls application.
 *
 * Initializes Eeze, then queries and prints information about various
 * types of disks and GPIO devices.
 * @return 0 on successful execution.
 */
int
main(void)
{
   Eina_List *disks;
   const char *syspath;

   eeze_init();
   eeze_disk_function();

   /* Find and print all mountable disks */
   disks = eeze_udev_find_by_type(EEZE_UDEV_TYPE_DRIVE_MOUNTABLE, NULL);
   printf("Found the following mountable disks:\n");
   EINA_LIST_FREE(disks, syspath)
     {
        Eeze_Disk *disk;

        disk = eeze_disk_new(syspath);
        printf("\t%s - %s:%s\n", syspath, eeze_disk_devpath_get(disk), eeze_disk_mount_point_get(disk));
        eeze_disk_free(disk);
        eina_stringshare_del(syspath);
     }

   /* Find and print all removable drives */
   disks = eeze_udev_find_by_type(EEZE_UDEV_TYPE_DRIVE_REMOVABLE, NULL);
   printf("Found the following removable drives:\n");
   EINA_LIST_FREE(disks, syspath)
     {
        Eeze_Disk *disk;

        disk = eeze_disk_new(syspath);
        printf("\t%s - %s:%s\n", syspath, eeze_disk_devpath_get(disk), eeze_disk_mount_point_get(disk));
        eeze_disk_free(disk);
        eina_stringshare_del(syspath);
     }

   /* Find and print all internal drives */
   disks = eeze_udev_find_by_type(EEZE_UDEV_TYPE_DRIVE_INTERNAL, NULL);
   printf("Found the following internal drives:\n");
   EINA_LIST_FREE(disks, syspath)
     {
        Eeze_Disk *disk;

        disk = eeze_disk_new(syspath);
        printf("\t%s - %s\n", syspath, eeze_disk_devpath_get(disk));
        eeze_disk_free(disk);
        eina_stringshare_del(syspath);
     }

   /* Find and print all GPIO devices (though typically not disks, Eeze can list them) */
   disks = eeze_udev_find_by_type(EEZE_UDEV_TYPE_GPIO, NULL);
   printf("Found the following GPIO(s):\n");
   EINA_LIST_FREE(disks, syspath)
     {
        Eeze_Disk *disk;

        disk = eeze_disk_new(syspath);
        printf("\t%s - %s:%s\n", syspath, eeze_disk_devpath_get(disk), eeze_disk_mount_point_get(disk));
        eeze_disk_free(disk);
        eina_stringshare_del(syspath);
     }

   return 0;
}
