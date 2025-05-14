#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <Ecore.h>
#include <Eeze.h>
#include <Eeze_Disk.h>

#include "eeze_udev_private.h"
#include "eeze_disk_private.h"

#define EEZE_MOUNT_DEFAULT_OPTS "noexec,nosuid,utf8"

/** @brief Event ID for disk mount success */
EAPI int EEZE_EVENT_DISK_MOUNT = 0;
/** @brief Event ID for disk unmount success */
EAPI int EEZE_EVENT_DISK_UNMOUNT = 0;
/** @brief Event ID for disk eject success */
EAPI int EEZE_EVENT_DISK_EJECT = 0;
/** @brief Event ID for disk operation error */
EAPI int EEZE_EVENT_DISK_ERROR = 0;
/** @internal @brief Ecore event handler for mount/unmount/eject results. */
static Ecore_Event_Handler *_mount_handler = NULL;
/** @internal @brief List of Eeze_Disk objects currently undergoing an operation. */
Eina_List *eeze_events = NULL;

/*
 *
 * PRIVATE
 *
 */

/**
 * @internal
 * @brief Frees an Eeze_Event_Disk_Error structure.
 * This function is used as a callback for ecore_event_add when an error event is emitted.
 * @param data Unused.
 * @param de The Eeze_Event_Disk_Error structure to free.
 */
static void
_eeze_disk_mount_error_free(void *data EINA_UNUSED, Eeze_Event_Disk_Error *de)
{
   if (!de)
     return;

   eina_stringshare_del(de->message);
   free(de);
}

/**
 * @internal
 * @brief Handles errors during disk operations and emits an EEZE_EVENT_DISK_ERROR.
 * @param disk The Eeze_Disk associated with the error.
 * @param error A string describing the error.
 */
static void
_eeze_disk_mount_error_handler(Eeze_Disk *disk, const char *error)
{
   Eeze_Event_Disk_Error *de;

   ERR("%s", error);
   if (!(de = calloc(1, sizeof(Eeze_Event_Disk_Error))))
     return;

   de->disk = disk;
   de->message = eina_stringshare_add(error);
   /* FIXME: placeholder since currently there are only mount-type errors */
   ecore_event_add(EEZE_EVENT_DISK_ERROR, de, (Ecore_End_Cb)_eeze_disk_mount_error_free, NULL);
}

/**
 * @internal
 * @brief Handles the result of mount, unmount, or eject operations.
 * This function is an Ecore_Event_Handler_Cb for ECORE_EXE_EVENT_DEL events,
 * which are triggered when an ecore_exe_run process finishes.
 * It determines the success or failure of the operation and emits the
 * appropriate Eeze event (EEZE_EVENT_DISK_MOUNT, EEZE_EVENT_DISK_UNMOUNT,
 * EEZE_EVENT_DISK_EJECT, or EEZE_EVENT_DISK_ERROR).
 *
 * @param data Unused.
 * @param type Unused.
 * @param ev The Ecore_Exe_Event_Del event data.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_eeze_disk_mount_result_handler(void *data EINA_UNUSED, int type EINA_UNUSED, Ecore_Exe_Event_Del *ev)
{
   Eeze_Disk *disk;
   Eina_List *l;
   Eeze_Event_Disk_Mount *e;

   if ((!ev) || (!ev->exe))
     return ECORE_CALLBACK_RENEW;
   disk = ecore_exe_data_get(ev->exe);

   if ((!disk) || (!eeze_events) || (!(l = eina_list_data_find_list(eeze_events, disk))))
     return ECORE_CALLBACK_RENEW;

   eeze_events = eina_list_remove_list(eeze_events, l);
   if (!disk->mounter) /* killed */
     {
        disk->mount_status = EEZE_DISK_NULL;
        return ECORE_CALLBACK_RENEW;
     }
   if (disk->mount_status == EEZE_DISK_MOUNTING)
     {
        disk->mounter = NULL;
        if (!ev->exit_code)
          {
              disk->mounted = EINA_TRUE;
              e = malloc(sizeof(Eeze_Event_Disk_Mount));
              EINA_SAFETY_ON_NULL_RETURN_VAL(e, ECORE_CALLBACK_RENEW);
              e->disk = disk;
              ecore_event_add(EEZE_EVENT_DISK_MOUNT, e, NULL, NULL);
          }
        else if (ev->exit_code & 2)
           _eeze_disk_mount_error_handler(disk, "system error (out of memory, cannot fork, no more loop devices)");
        else if (ev->exit_code & 4)
           _eeze_disk_mount_error_handler(disk, "internal mount bug");
        else if (ev->exit_code & 8)
           _eeze_disk_mount_error_handler(disk, "user interrupt");
        else if (ev->exit_code & 16)
           _eeze_disk_mount_error_handler(disk, "problems writing or locking /etc/mtab");
        else if (ev->exit_code & 32)
           _eeze_disk_mount_error_handler(disk, "mount failure");
        else if (ev->exit_code & 64)
           _eeze_disk_mount_error_handler(disk, "some mount succeeded");
        else
           _eeze_disk_mount_error_handler(disk, "incorrect invocation or permissions");
     }
   else if (disk->mount_status == EEZE_DISK_UNMOUNTING)
     switch (ev->exit_code)
       {
        case 0:
          e = malloc(sizeof(Eeze_Event_Disk_Unmount));
          EINA_SAFETY_ON_NULL_RETURN_VAL(e, ECORE_CALLBACK_RENEW);
          e->disk = disk;
          disk->mounter = NULL;
          disk->mounted = EINA_FALSE;
          ecore_event_add(EEZE_EVENT_DISK_UNMOUNT, e, NULL, NULL);
          break;

        default:
          if (disk->mount_fail_count++ < 3)
            {
               INF("Could not unmount disk, retrying");
               disk->mounter = ecore_exe_run(eina_strbuf_string_get(disk->unmount_cmd), disk);
               eeze_events = eina_list_append(eeze_events, disk);
            }
          else
            {
               disk->mount_fail_count = 0;
               _eeze_disk_mount_error_handler(disk, "Maximum number of mount-related failures reached");
            }
          return ECORE_CALLBACK_RENEW;
       }
     else
       switch (ev->exit_code)
         {
          case 0:
            e = malloc(sizeof(Eeze_Event_Disk_Eject));
            EINA_SAFETY_ON_NULL_RETURN_VAL(e, ECORE_CALLBACK_RENEW);
            e->disk = disk;
            disk->mounter = NULL;
            if (disk->mount_status & EEZE_DISK_UNMOUNTING)
              {
                 disk->mount_status |= EEZE_DISK_UNMOUNTING;
                 disk->mounted = EINA_FALSE;
                 ecore_event_add(EEZE_EVENT_DISK_UNMOUNT, e, NULL, NULL);
                 eeze_disk_eject(disk);
              }
            else
              ecore_event_add(EEZE_EVENT_DISK_EJECT, e, NULL, NULL);
            break;

          default:
            if (disk->mount_fail_count++ < 3)
              {
                 INF("Could not eject disk, retrying");
                 if (disk->mount_status & EEZE_DISK_UNMOUNTING)
                   disk->mounter = ecore_exe_run(eina_strbuf_string_get(disk->unmount_cmd), disk);
                 else
                   disk->mounter = ecore_exe_run(eina_strbuf_string_get(disk->eject_cmd), disk);
                 eeze_events = eina_list_append(eeze_events, disk);
              }
           else
             {
                disk->mount_fail_count = 0;
                _eeze_disk_mount_error_handler(disk, "Maximum number of mount-related failures reached");
             }
            return ECORE_CALLBACK_RENEW;
         }
   return ECORE_CALLBACK_RENEW;
}

/*
 *
 * INVISIBLE
 *
 */

/**
 * @internal
 * @brief Initializes the eeze disk mounting subsystem.
 * This function registers new ecore event types for disk operations
 * and sets up an event handler for mount/unmount/eject results.
 * It also initializes the underlying libmount integration.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool
eeze_mount_init(void)
{
   EEZE_EVENT_DISK_MOUNT = ecore_event_type_new();
   EEZE_EVENT_DISK_UNMOUNT = ecore_event_type_new();
   EEZE_EVENT_DISK_EJECT = ecore_event_type_new();
   EEZE_EVENT_DISK_ERROR = ecore_event_type_new();
   _mount_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                                           (Ecore_Event_Handler_Cb)_eeze_disk_mount_result_handler, NULL);
   return eeze_libmount_init();
}

/**
 * @internal
 * @brief Shuts down the eeze disk mounting subsystem.
 * Flushes any pending disk operation events, shuts down libmount integration,
 * and removes the event handler.
 */
void
eeze_mount_shutdown(void)
{
   ecore_event_type_flush(EEZE_EVENT_DISK_MOUNT,
                          EEZE_EVENT_DISK_UNMOUNT,
                          EEZE_EVENT_DISK_EJECT,
                          EEZE_EVENT_DISK_ERROR);
   eeze_libmount_shutdown();
   ecore_event_handler_del(_mount_handler);
   _mount_handler = NULL;
}

/*
 *
 * API
 *
 */

/**
 * @brief Checks if a disk is currently mounted.
 * This function queries the system (via libmount) to determine if the specified disk
 * is mounted.
 * @param disk The disk to check.
 * @return EINA_TRUE if the disk is mounted, EINA_FALSE otherwise or on error.
 */
EAPI Eina_Bool
eeze_disk_mounted_get(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, EINA_FALSE);

   return eeze_disk_libmount_mounted_get(disk);
}

/**
 * @brief Sets the mount options for a disk.
 * These options will be used when @ref eeze_disk_mount is called.
 * Setting new options marks the internal mount command as changed, so it will
 * be regenerated on the next mount attempt.
 * If EEZE_DISK_MOUNTOPT_UID is set, the current user's UID will be stored
 * for use in the mount command (e.g., "uid=<uid>").
 *
 * @param disk The disk for which to set mount options.
 * @param opts A bitmask of Eeze_Disk_Mount_Opts flags.
 *        Example: EEZE_DISK_MOUNTOPT_NOEXEC | EEZE_DISK_MOUNTOPT_NOSUID
 * @return EINA_TRUE on success, EINA_FALSE if disk is NULL.
 */
EAPI Eina_Bool
eeze_disk_mountopts_set(Eeze_Disk *disk, unsigned long opts)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, EINA_FALSE);
   if (opts != disk->mount_opts)
     disk->mount_cmd_changed = EINA_TRUE;
   disk->mount_opts = opts;
   if (opts & EEZE_DISK_MOUNTOPT_UID)
     disk->uid = getuid();
   return EINA_TRUE;
}

/**
 * @brief Gets the currently set mount options for a disk.
 * @param disk The disk to query.
 * @return A bitmask of Eeze_Disk_Mount_Opts flags, or 0 if disk is NULL.
 */
EAPI unsigned long
eeze_disk_mountopts_get(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, 0);
   return disk->mount_opts;
}

/**
 * @brief Sets a wrapper command to be prepended to mount/unmount/eject commands.
 * This can be used, for example, to execute commands via `sudo` or a custom
 * helper utility. The wrapper string will be prepended to the command line
 * followed by a space.
 *
 * @param disk The disk for which to set the wrapper.
 * @param wrapper The wrapper command string (e.g., "sudo").
 *        Pass NULL to remove an existing wrapper.
 * @return EINA_TRUE on success, EINA_FALSE if disk is NULL or wrapper is an empty string.
 */
EAPI Eina_Bool
eeze_disk_mount_wrapper_set(Eeze_Disk *disk, const char *wrapper)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, EINA_FALSE);
   if (wrapper) EINA_SAFETY_ON_TRUE_RETURN_VAL(!*wrapper, EINA_FALSE);
   else
     {
        eina_stringshare_del(disk->mount_wrapper);
        disk->mount_wrapper = NULL;
        return EINA_TRUE;
     }
   eina_stringshare_replace(&disk->mount_wrapper, wrapper);
   return EINA_TRUE;
}

/**
 * @brief Gets the currently set mount wrapper command for a disk.
 * @param disk The disk to query.
 * @return The wrapper command string, or NULL if no wrapper is set or disk is NULL.
 *         The returned string is an Eina_Stringshare, do not free it.
 */
EAPI const char *
eeze_disk_mount_wrapper_get(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);
   return disk->mount_wrapper;
}

/**
 * @brief Mounts a disk.
 * This function asynchronously attempts to mount the specified disk.
 * It first checks if a mount point is set for the disk. If not, it tries to
 * determine one using libmount (looking up by UUID, then devpath).
 * If the mount point directory does not exist, it attempts to create it.
 * The actual mount command is constructed based on the disk's properties
 * (devpath/UUID, mount point, mount options, wrapper).
 * An ECORE_EXE_EVENT_DEL event will be triggered upon completion (or failure),
 * which is handled by _eeze_disk_mount_result_handler to emit the appropriate
 * Eeze event (EEZE_EVENT_DISK_MOUNT or EEZE_EVENT_DISK_ERROR).
 *
 * @param disk The disk to mount.
 * @return EINA_TRUE if the mount process was successfully initiated, EINA_FALSE on error
 *         (e.g., disk is NULL, no mount point, already mounted, failed to start command).
 */
EAPI Eina_Bool
eeze_disk_mount(Eeze_Disk *disk)
{
   struct stat st;
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, EINA_FALSE);

   if (!disk->mount_point)
     return EINA_FALSE;
   if (eeze_disk_libmount_mounted_get(disk))
     return EINA_FALSE;

   if (!disk->mount_cmd)
     disk->mount_cmd = eina_strbuf_new();

   if (disk->mount_cmd_changed)
     {
        const char *dev, *str;
        eina_strbuf_string_free(disk->mount_cmd);
        dev = eeze_disk_uuid_get(disk);
        if (dev) str = "UUID=";
        else
          {
             dev = eeze_disk_devpath_get(disk);
             str = NULL;
          }

        if (!disk->mount_point)
          {
             const char *mp;
             /* here we attempt to guess the mount point using libmount */
             mp = eeze_disk_libmount_mp_lookup_by_uuid(disk->cache.uuid);
             if (!mp)
               {
                  const char *devpath;

                  devpath = eeze_disk_devpath_get(disk);
                  if (devpath)
                    {
                       mp = eeze_disk_libmount_mp_lookup_by_devpath(devpath);
                       eina_stringshare_del(devpath);
                    }
               }
             if (!eeze_disk_mount_point_set(disk, mp))
               /* sometimes we fail */
               return EINA_FALSE;
          }

        if ((!disk->mount_point) || (!disk->mount_point[0])) return EINA_FALSE;
        if (disk->mount_wrapper)
          eina_strbuf_append_printf(disk->mount_cmd, "%s ", disk->mount_wrapper);
        if (disk->mount_opts == EEZE_DISK_MOUNTOPT_DEFAULTS)
          eina_strbuf_append_printf(disk->mount_cmd, EEZE_MOUNT_BIN" -o "EEZE_MOUNT_DEFAULT_OPTS" %s%s %s", str ? str : "", dev, disk->mount_point);
        else if (!disk->mount_opts)
          eina_strbuf_append_printf(disk->mount_cmd, EEZE_MOUNT_BIN" %s%s %s", str ? str : "", dev, disk->mount_point);
        else
          {
             eina_strbuf_append(disk->mount_cmd, EEZE_MOUNT_BIN" -o ");
             /* trailing commas are okay */
             if (disk->mount_opts & EEZE_DISK_MOUNTOPT_LOOP)
               eina_strbuf_append(disk->mount_cmd, "loop,");
             if (disk->mount_opts & EEZE_DISK_MOUNTOPT_UTF8)
               {
                  const char *fstype;
                  eina_strbuf_append(disk->mount_cmd, "utf8,");
                  fstype = eeze_disk_fstype_get(disk);
                  if (fstype && (!strcmp(fstype, "jfs")))
                    eina_strbuf_append(disk->mount_cmd, "iocharset=utf8,");
               }
             if (disk->mount_opts & EEZE_DISK_MOUNTOPT_NOEXEC)
               eina_strbuf_append(disk->mount_cmd, "noexec,");
             if (disk->mount_opts & EEZE_DISK_MOUNTOPT_NODEV)
               eina_strbuf_append(disk->mount_cmd, "nodev,");
             if (disk->mount_opts & EEZE_DISK_MOUNTOPT_NOSUID)
               eina_strbuf_append(disk->mount_cmd, "nosuid,");
             if (disk->mount_opts & EEZE_DISK_MOUNTOPT_REMOUNT)
               eina_strbuf_append(disk->mount_cmd, "remount,");
             if (disk->mount_opts & EEZE_DISK_MOUNTOPT_UID)
               eina_strbuf_append_printf(disk->mount_cmd, "uid=%i,", (int)disk->uid);
             eina_strbuf_append_printf(disk->mount_cmd, " %s%s %s", str ? str : "", dev, disk->mount_point);
          }
        disk->mount_cmd_changed = EINA_FALSE;
     }

   if (stat(disk->mount_point, &st))
     {
        INF("Creating not-existing mount point directory '%s'", disk->mount_point);
        if (mkdir(disk->mount_point, S_IROTH | S_IWOTH | S_IXOTH))
          ERR("Could not create directory: %s; hopefully this is handled by your mounter!", strerror(errno));
     }
   else if (!S_ISDIR(st.st_mode))
     {
        ERR("%s is not a directory!", disk->mount_point);
        return EINA_FALSE;
     }
   INF("Mounting: %s", eina_strbuf_string_get(disk->mount_cmd));
   disk->mounter = ecore_exe_run(eina_strbuf_string_get(disk->mount_cmd), disk);
   if (!disk->mounter)
     return EINA_FALSE;
   eeze_events = eina_list_append(eeze_events, disk);
   disk->mount_status = EEZE_DISK_MOUNTING;

   return EINA_TRUE;
}

/**
 * @brief Unmounts a disk.
 * This function asynchronously attempts to unmount the specified disk.
 * It first checks if the disk is actually mounted.
 * The unmount command is constructed based on the disk's devpath and any
 * configured wrapper.
 * An ECORE_EXE_EVENT_DEL event will be triggered upon completion (or failure),
 * which is handled by _eeze_disk_mount_result_handler to emit the appropriate
 * Eeze event (EEZE_EVENT_DISK_UNMOUNT or EEZE_EVENT_DISK_ERROR).
 * If unmounting fails, it will retry up to 3 times.
 *
 * @param disk The disk to unmount.
 * @return EINA_TRUE if the unmount process was successfully initiated or if the disk
 *         was not mounted, EINA_FALSE on other errors (e.g., disk is NULL,
 *         failed to start command).
 */
EAPI Eina_Bool
eeze_disk_unmount(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, EINA_FALSE);

   if (!eeze_disk_libmount_mounted_get(disk))
     return EINA_TRUE;

   if (!disk->unmount_cmd)
     disk->unmount_cmd = eina_strbuf_new();

   if (disk->unmount_cmd_changed)
     {
        eina_strbuf_string_free(disk->unmount_cmd);
        if (disk->mount_wrapper)
          eina_strbuf_append_printf(disk->unmount_cmd, "%s ", disk->mount_wrapper);
        eina_strbuf_append_printf(disk->unmount_cmd, EEZE_UNMOUNT_BIN" %s", eeze_disk_devpath_get(disk));
        disk->unmount_cmd_changed = EINA_FALSE;
     }

   INF("Unmounting: %s", eina_strbuf_string_get(disk->unmount_cmd));
   disk->mounter = ecore_exe_run(eina_strbuf_string_get(disk->unmount_cmd), disk);
   if (!disk->mounter)
     return EINA_FALSE;

   eeze_events = eina_list_append(eeze_events, disk);
   disk->mount_status = EEZE_DISK_UNMOUNTING;
   return EINA_TRUE;
}

/**
 * @brief Ejects a disk.
 * This function asynchronously attempts to eject the specified disk.
 * If the disk is currently mounted, it will first attempt to unmount it.
 * The eject command is constructed based on the disk's devpath and any
 * configured wrapper.
 * An ECORE_EXE_EVENT_DEL event will be triggered upon completion (or failure),
 * which is handled by _eeze_disk_mount_result_handler to emit the appropriate
 * Eeze event (EEZE_EVENT_DISK_EJECT or EEZE_EVENT_DISK_ERROR).
 * If ejecting fails (or unmounting prior to ejecting fails), it will retry up to 3 times.
 *
 * @param disk The disk to eject.
 * @return EINA_TRUE if the eject (or pre-eject unmount) process was successfully
 *         initiated, EINA_FALSE on error (e.g., disk is NULL, failed to start command).
 */
EAPI Eina_Bool
eeze_disk_eject(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, EINA_FALSE);

   if (!disk->eject_cmd)
     {
        disk->eject_cmd = eina_strbuf_new();
        if (disk->mount_wrapper)
          eina_strbuf_append_printf(disk->eject_cmd, "%s ", disk->mount_wrapper);
        eina_strbuf_append_printf(disk->eject_cmd, EEZE_EJECT_BIN" %s", eeze_disk_devpath_get(disk));
     }

   INF("Ejecting: %s", eina_strbuf_string_get(disk->eject_cmd));
   if (eeze_disk_libmount_mounted_get(disk))
     {
        Eina_Bool ret;

        ret = eeze_disk_unmount(disk);
        if (ret) disk->mount_status |= EEZE_DISK_EJECTING;
        return ret;
     }
   disk->mounter = ecore_exe_run(eina_strbuf_string_get(disk->eject_cmd), disk);
   if (!disk->mounter)
     return EINA_FALSE;

   eeze_events = eina_list_append(eeze_events, disk);
   disk->mount_status = EEZE_DISK_EJECTING;
   return EINA_TRUE;
}

/**
 * @brief Cancels an ongoing mount, unmount, or eject operation for a disk.
 * If there is an active operation (mounter process) for the disk, this function
 * will kill the process.
 *
 * @param disk The disk whose operation should be canceled.
 */
EAPI void
eeze_disk_cancel(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN(disk);
   if ((!disk->mount_status) || (!disk->mounter)) return;
   disk->mount_status = EEZE_DISK_NULL;
   ecore_exe_kill(disk->mounter);
   disk->mounter = NULL;
}

/**
 * @brief Gets the mount point for a disk.
 * If a mount point has been explicitly set using @ref eeze_disk_mount_point_set,
 * that value is returned.
 * Otherwise, it attempts to look up the mount point using libmount by:
 * 1. Device path (e.g., /dev/sdb1)
 * 2. UUID
 * 3. Filesystem label
 * If a mount point is found via libmount, it is cached on the Eeze_Disk object
 * for future calls.
 *
 * @param disk The disk to query.
 * @return The mount point string (e.g., "/media/usb_drive") if found,
 *         otherwise NULL. The returned string is an Eina_Stringshare, do not free it.
 *         Returns NULL if disk is NULL.
 */
EAPI const char *
eeze_disk_mount_point_get(Eeze_Disk *disk)
{
   const char *mp;
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);

   if (disk->mount_point)
     return disk->mount_point;

   mp = eeze_disk_libmount_mp_lookup_by_devpath(eeze_disk_devpath_get(disk));
   if (mp)
     {
        disk->mount_point = eina_stringshare_add(mp);
        return disk->mount_point;
     }
   mp = eeze_disk_libmount_mp_lookup_by_uuid(eeze_disk_uuid_get(disk));
   if (mp)
     {
        disk->mount_point = eina_stringshare_add(mp);
        return disk->mount_point;
     }
   mp = eeze_disk_libmount_mp_lookup_by_label(eeze_disk_label_get(disk));
   if (mp)
     {
        disk->mount_point = eina_stringshare_add(mp);
        return disk->mount_point;
     }
   return NULL;
}

/**
 * @brief Sets the mount point for a disk.
 * This function allows explicitly setting the directory where the disk should be mounted.
 * Setting a new mount point marks the internal mount and unmount commands as changed,
 * so they will be regenerated on the next mount/unmount attempt.
 *
 * @param disk The disk for which to set the mount point.
 * @param mount_point The desired mount point path (e.g., "/mnt/my_disk").
 *        Can be NULL to clear a previously set mount point, though this might
 *        cause issues if a mount point cannot be automatically determined later.
 * @return EINA_TRUE on success, EINA_FALSE if disk is NULL.
 */
EAPI Eina_Bool
eeze_disk_mount_point_set(Eeze_Disk *disk, const char *mount_point)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, EINA_FALSE);

   eina_stringshare_replace(&disk->mount_point, mount_point);
   disk->mount_cmd_changed = EINA_TRUE;
   disk->unmount_cmd_changed = EINA_TRUE;
   return EINA_TRUE;
}
