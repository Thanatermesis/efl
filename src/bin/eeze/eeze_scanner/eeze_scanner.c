#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eet.h>
#include <Eeze.h>
#include <Ecore_Con.h>
#include <Eeze_Disk.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "eeze_scanner.h"

#define DBG(...)            EINA_LOG_DOM_DBG(es_log_dom, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(es_log_dom, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(es_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(es_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(es_log_dom, __VA_ARGS__)

#ifndef O_BINARY
# define O_BINARY 0
#endif

static int es_log_dom = -1; /**< Log domain for eeze_scanner. */
static int retval = EXIT_SUCCESS; /**< Program exit value. */
static Eet_Data_Descriptor *es_edd = NULL; /**< Eet Data Descriptor for Eeze_Scanner_Event. */
static Eina_List *clients = NULL; /**< List of connected Eet_Connection clients. */

static Eina_List *storage_devices = NULL; /**< List of syspaths for non-CDROM storage devices. */
static Eina_List *storage_cdrom = NULL; /**< List of Eeze_Scanner_Device for CDROMs. */

static Eina_List *volume_cdrom = NULL; /**< List of Eeze_Scanner_Device for CDROM volumes (media inserted). */
static Eina_List *volume_devices = NULL; /**< List of syspaths for non-CDROM mountable volumes. */

/**
 * @brief Sends an Eeze_Scanner_Event to all connected clients.
 *
 * Serializes and sends the event data over Eet_Connection.
 *
 * @param device The syspath of the device.
 * @param type The type of event (add, remove, change).
 * @param volume EINA_TRUE if it's a volume event, EINA_FALSE otherwise.
 */
static void
event_send(const char *device, Eeze_Scanner_Event_Type type, Eina_Bool volume)
{
   Eeze_Scanner_Event ev;
   Eet_Connection *ec;
   const Eina_List *n;


   ev.device = device;
   ev.type = type;
   ev.volume = volume;
   EINA_LIST_FOREACH(clients, n, ec)
     {
        const char *ts;
        const char *vol = volume ? " (volume)" : "";
        switch (type)
          {
           case EEZE_SCANNER_EVENT_TYPE_ADD: ts = "ADD"; break;
           case EEZE_SCANNER_EVENT_TYPE_REMOVE: ts = "REMOVE"; break;
           case EEZE_SCANNER_EVENT_TYPE_CHANGE: ts = "CHANGE"; break;
           default: ts = "?";
          }
        INF("Serializing event %s '%s'%s for client %p...", ts, device, vol, ec);
        eet_connection_send(ec, es_edd, &ev, NULL);
     }
}

/**
 * @brief Callback for Eeze disk mount/unmount events.
 *
 * Specifically tracks the mounted state of CD-ROM devices.
 *
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param disk The Eeze_Disk object associated with the event.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
disk_mount(void *data EINA_UNUSED, int type EINA_UNUSED, Eeze_Disk *disk)
{
   Eina_List *l;
   Eeze_Scanner_Device *d;
   if (eeze_disk_type_get(disk) != EEZE_DISK_TYPE_CDROM) return ECORE_CALLBACK_RENEW;

   EINA_LIST_FOREACH(storage_cdrom, l, d)
     {
        if (d->device == eeze_disk_syspath_get(disk))
          {
             d->mounted = !d->mounted;
             break;
          }
     }
   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Sends initial device and volume state to a newly connected client.
 *
 * Iterates through known storage devices, CD-ROMs, and volumes, sending
 * ADD events for each to the new client.
 *
 * @param cl The client object (Eo *).
 * @param ec The Eet_Connection for the client.
 */
static void
cl_setup(Eo *cl, Eet_Connection *ec)
{
   Eina_List *l;
   Eeze_Scanner_Device *dev;
   Eeze_Scanner_Event ev;
   const char *sys;

   INF("Sending initial events to new client %p (ec=%p)", cl, ec);
   EINA_LIST_FOREACH(storage_devices, l, sys)
     {
        ev.device = sys;
        ev.type = EEZE_SCANNER_EVENT_TYPE_ADD;
        ev.volume = EINA_FALSE;
        eet_connection_send(ec, es_edd, &ev, NULL);
     }
   EINA_LIST_FOREACH(storage_cdrom, l, dev)
     {
        ev.device = dev->device;
        ev.type = EEZE_SCANNER_EVENT_TYPE_ADD;
        ev.volume = EINA_FALSE;
        eet_connection_send(ec, es_edd, &ev, NULL);
     }
   EINA_LIST_FOREACH(volume_devices, l, sys)
     {
        ev.device = sys;
        ev.type = EEZE_SCANNER_EVENT_TYPE_ADD;
        ev.volume = EINA_TRUE;
        eet_connection_send(ec, es_edd, &ev, NULL);
     }
   EINA_LIST_FOREACH(volume_cdrom, l, dev)
     {
        ev.device = dev->device;
        ev.type = EEZE_SCANNER_EVENT_TYPE_ADD;
        ev.volume = EINA_TRUE;
        eet_connection_send(ec, es_edd, &ev, NULL);
     }
}

/**
 * @brief Eet_Connection callback for writing data to a client.
 *
 * This function is called by Eet when there is data to be sent to a client.
 * It uses efl_io_writer_write to send the data.
 *
 * @param data Pointer to the data to write.
 * @param size Size of the data to write.
 * @param user_data The client object (Eo *) associated with this connection.
 * @return EINA_TRUE on successful write, EINA_FALSE on failure.
 */
static Eina_Bool
ec_write(const void *data, size_t size, void *user_data)
{
   Eo *client = user_data;
   Eina_Slice slice = { .mem = data, .len = size };
   Eina_Error err;
   DBG("Write " EINA_SLICE_FMT " to %p", EINA_SLICE_PRINT(slice), client);
   err = efl_io_writer_write(client, &slice, NULL);
   if (err)
     {
        DBG("Failed write " EINA_SLICE_FMT " to %p: %s", EINA_SLICE_PRINT(slice), client, eina_error_msg_get(err));
        if (!efl_io_closer_closed_get(client))
          efl_io_closer_close(client);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/**
 * @brief Eet_Connection callback for reading data from a client.
 *
 * This function is called by Eet when data is received from a client.
 * Currently, it's a no-op as eeze_scanner only sends data.
 *
 * @param eet_data Pointer to the received data (unused).
 * @param size Size of the received data (unused).
 * @param user_data The client object (Eo *) (unused).
 * @return EINA_TRUE.
 */
static Eina_Bool
ec_read(const void *eet_data EINA_UNUSED, size_t size EINA_UNUSED, void *user_data EINA_UNUSED)
{
   return EINA_TRUE;
}

/**
 * @brief Callback for EFL_IO_BUFFERED_STREAM_EVENT_READ_FINISHED event on a client.
 *
 * This indicates that the read operation on the client's buffered stream has finished.
 * The client connection is closed if not already.
 *
 * @param data The Eet_Connection associated with the client.
 * @param event The EFL event data.
 */
static void
cl_finished(void *data, const Efl_Event *event)
{
   Eo *client = event->object;
   Eet_Connection *ec = data;

   DBG("Finished %p (ec=%p)", client, ec);

   if (!efl_io_closer_closed_get(client))
     efl_io_closer_close(client);
}

/**
 * @brief Callback for EFL_IO_BUFFERED_STREAM_EVENT_ERROR event on a client.
 *
 * Handles errors occurring on the client's buffered stream.
 * The client connection is closed if not already.
 *
 * @param data The Eet_Connection associated with the client.
 * @param event The EFL event data, containing the Eina_Error.
 */
static void
cl_error(void *data, const Efl_Event *event)
{
   Eo *client = event->object;
   Eina_Error *perr = event->info;
   Eet_Connection *ec = data;

   WRN("client %p (ec=%p) error: %s", client, ec, eina_error_msg_get(*perr));

   if (!efl_io_closer_closed_get(client))
     efl_io_closer_close(client);
}

static Efl_Callback_Array_Item *cl_cbs(void);

/**
 * @brief Callback for EFL_IO_CLOSER_EVENT_CLOSED event on a client.
 *
 * This is called when a client connection is fully closed.
 * It cleans up resources associated with the client, including removing it
 * from the list of active clients and freeing the Eet_Connection.
 *
 * @param data The Eet_Connection associated with the client.
 * @param event The EFL event data.
 */
static void
cl_closed(void *data, const Efl_Event *event)
{
   Eo *client = event->object;
   Eet_Connection *ec = data;

   INF("Removed client %p (ec=%p)", client, ec);
   clients = eina_list_remove(clients, ec);
   efl_event_callback_array_del(client, cl_cbs(), ec);
   efl_unref(client);
   eet_connection_close(ec, NULL);
}

EFL_CALLBACKS_ARRAY_DEFINE(cl_cbs,
                           { EFL_IO_BUFFERED_STREAM_EVENT_READ_FINISHED, cl_finished },
                           { EFL_IO_BUFFERED_STREAM_EVENT_ERROR, cl_error },
                           { EFL_IO_CLOSER_EVENT_CLOSED, cl_closed });

/**
 * @brief Callback for EFL_NET_SERVER_EVENT_CLIENT_ADD event.
 *
 * This function is called when a new client connects to the server.
 * It sets up an Eet_Connection for the new client, adds it to the list
 * of clients, and sends the initial state of devices and volumes.
 *
 * @param data User data (unused).
 * @param event The EFL event data, where event->info is the new client (Eo *).
 */
static void
cl_add(void *data EINA_UNUSED, const Efl_Event *event)
{
   Eo *client = event->info;
   Eet_Connection *ec;

   INF("Added client %p", client);
   ec = eet_connection_new(ec_read, ec_write, client);
   if (!ec)
     {
        ERR("Could not create eet serializer! Lost client %p!", client);
        return;
     }

   efl_ref(client);
   efl_event_callback_array_add(client, cl_cbs(), ec);
   clients = eina_list_append(clients, ec);
   cl_setup(client, ec);
}

/**
 * @brief Initializes the Eet data descriptor for Eeze_Scanner_Event.
 *
 * Sets up the `es_edd` global variable which is used for serializing
 * event data to clients.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
eet_setup(void)
{
   Eet_Data_Descriptor_Class eddc;

   if (!eet_eina_stream_data_descriptor_class_set(&eddc, sizeof(eddc), "eeze_scanner_event", sizeof(Eeze_Scanner_Event)))
     {
        CRI("Could not create eet data descriptor!");
        return EINA_FALSE;
     }

   es_edd = eet_data_descriptor_stream_new(&eddc);
#define DAT(MEMBER, TYPE) EET_DATA_DESCRIPTOR_ADD_BASIC(es_edd, Eeze_Scanner_Event, #MEMBER, MEMBER, EET_T_##TYPE)
   DAT(device, INLINED_STRING);
   DAT(type, UINT);
   DAT(volume, UCHAR);
#undef DAT
   return EINA_TRUE;
}

/**
 * @brief Poller callback for CD-ROM devices.
 *
 * This function is periodically called for each CD-ROM device to check
 * if media has been inserted or removed, as CD-ROMs might not generate
 * standard udev events for this in all cases. It attempts to open the
 * device node; success implies media is present.
 *
 * @param dev The Eeze_Scanner_Device representing the CD-ROM.
 * @return EINA_TRUE to continue polling.
 */
static Eina_Bool
cdrom_timer(Eeze_Scanner_Device *dev)
{
   const char *devpath;
   int fd;

   /* cdrom already mounted, no need to poll */
   if (dev->mounted) return EINA_TRUE;
   devpath = eeze_udev_syspath_get_devpath(dev->device);
   fd = open(devpath, O_RDONLY | O_BINARY);
   if (fd < 0)
     {
        Eina_List *l;

        l = eina_list_data_find_list(volume_cdrom, dev);
        if (l)
          {
             /* disc removed, delete volume */
             INF("Removed cdrom '%s'", dev->device);
             volume_cdrom = eina_list_remove_list(volume_cdrom, l);
             event_send(dev->device, EEZE_SCANNER_EVENT_TYPE_CHANGE, EINA_TRUE);
          }
        /* just in case */
        dev->mounted = EINA_FALSE;
     }
   else
     {
        if (!eina_list_data_find(volume_cdrom, dev))
          {
             INF("Added cdrom '%s'", dev->device);
             volume_cdrom = eina_list_append(volume_cdrom, dev);
             event_send(dev->device, EEZE_SCANNER_EVENT_TYPE_CHANGE, EINA_TRUE);
          }
        close(fd);
     }
   eina_stringshare_del(devpath);
   return EINA_TRUE;
}

/**
 * @brief Initializes the lists of storage devices and volumes at startup.
 *
 * Queries eeze for internal, removable, and CD-ROM drives, as well as
 * mountable volumes. Sets up pollers for CD-ROM devices.
 *
 * @return EINA_TRUE on success, EINA_FALSE if no initial storage devices are found.
 */
static Eina_Bool
storage_setup(void)
{
   Eina_List *l, *ll;
   const char *sys;

   storage_devices = eeze_udev_find_by_type(EEZE_UDEV_TYPE_DRIVE_INTERNAL, NULL);
   if (!storage_devices)
     {
        ERR("No storage devices found! This is not supposed to happen!");
        return EINA_FALSE;
     }

   ll = eeze_udev_find_by_type(EEZE_UDEV_TYPE_DRIVE_REMOVABLE, NULL);
   EINA_LIST_FREE(ll, sys)
     {
        storage_devices = eina_list_append(storage_devices, sys);
     }

   l = eeze_udev_find_by_type(EEZE_UDEV_TYPE_DRIVE_CDROM, NULL);
   EINA_LIST_FREE(l, sys)
     {
        Eeze_Scanner_Device *dev;
        Eeze_Disk *disk;

        dev = calloc(1, sizeof(Eeze_Scanner_Device));
        if (!dev)
          {
             ERR("Lost cdrom device '%s'!", sys);
             eina_stringshare_del(sys);
             continue;
          }
        disk = eeze_disk_new(sys);
        if (!disk)
          {
             ERR("Lost cdrom device '%s'!", sys);
             eina_stringshare_del(sys);
             free(dev);
             continue;
          }
        dev->device = sys;
        dev->mounted = eeze_disk_mounted_get(disk);
        eeze_disk_free(disk);
        dev->poller = ecore_poller_add(ECORE_POLLER_CORE, 32, (Ecore_Task_Cb)cdrom_timer, dev);
        storage_cdrom = eina_list_append(storage_cdrom, dev);
     }
   volume_devices = eeze_udev_find_by_type(EEZE_UDEV_TYPE_DRIVE_MOUNTABLE, NULL);
   EINA_LIST_FOREACH_SAFE(volume_devices, l, ll, sys)
     {
        Eina_List *c;
        Eeze_Scanner_Device *dev;

        EINA_LIST_FOREACH(storage_cdrom, c, dev)
          if (sys == dev->device)
            {
               eina_stringshare_del(sys);
               volume_devices = eina_list_remove_list(volume_devices, l);
               volume_cdrom = eina_list_append(volume_cdrom, dev);
               l = NULL;
               break;
            }
     }

   return EINA_TRUE;
}

/**
 * @brief Callback for udev events related to mountable volumes.
 *
 * Handles add, remove, and change events for volumes (partitions, etc.).
 * Updates internal lists and sends events to clients.
 * Distinguishes between regular volumes and CD-ROM volumes.
 *
 * @param device The syspath of the device that changed.
 * @param ev The type of udev event.
 * @param data User data (unused).
 * @param watch The Eeze_Udev_Watch triggering this callback (unused).
 */
static void
cb_vol_chg(const char *device, Eeze_Udev_Event ev, void *data EINA_UNUSED, Eeze_Udev_Watch *watch EINA_UNUSED)
{
   Eina_List *l;
   Eeze_Scanner_Device *dev;

   DBG("device='%s'", device);

   if (ev == EEZE_UDEV_EVENT_ONLINE) ev = EEZE_UDEV_EVENT_ADD;
   else if (ev == EEZE_UDEV_EVENT_OFFLINE) ev = EEZE_UDEV_EVENT_REMOVE;

   event_send(device, (Eeze_Scanner_Event_Type)ev, EINA_TRUE);
   switch (ev)
     {
        case EEZE_UDEV_EVENT_ADD:
        case EEZE_UDEV_EVENT_ONLINE:
          INF("Added volume '%s'", device);
          EINA_LIST_FOREACH(storage_cdrom, l, dev)
            if (device == dev->device)
              {
                 volume_cdrom = eina_list_append(volume_cdrom, dev);
                 return;
              }
          volume_devices = eina_list_append(volume_devices, eina_stringshare_add(device));
          break;
        case EEZE_UDEV_EVENT_REMOVE:
        case EEZE_UDEV_EVENT_OFFLINE:
          INF("Removed volume '%s'", device);
          EINA_LIST_FOREACH(volume_cdrom, l, dev)
            if (device == dev->device)
              {
                 volume_cdrom = eina_list_remove_list(volume_cdrom, l);
                 return;
              }
          volume_devices = eina_list_remove(volume_devices, device);
          eina_stringshare_del(device);
          break;
        default:
          INF("Changed volume '%s'", device);
          break;
     }
}

/**
 * @brief Callback for udev events related to storage devices (disks).
 *
 * Handles add, remove, and change events for storage devices like HDDs, SSDs,
 * and CD-ROM drives. Updates internal lists and sends events to clients.
 * For CD-ROMs, it also sets up or tears down the media polling mechanism.
 *
 * @param device The syspath of the device that changed.
 * @param ev The type of udev event.
 * @param data User data (unused).
 * @param watch The Eeze_Udev_Watch triggering this callback (unused).
 */
static void
cb_stor_chg(const char *device, Eeze_Udev_Event ev, void *data EINA_UNUSED, Eeze_Udev_Watch *watch EINA_UNUSED)
{
   Eina_List *l;
   Eeze_Scanner_Device *dev = NULL;
   const char *str;


   DBG("device='%s'", device);   
   switch (ev)
     {
        case EEZE_UDEV_EVENT_ADD:
        case EEZE_UDEV_EVENT_ONLINE:
          INF("Added device '%s'", device);
          event_send(device, (Eeze_Scanner_Event_Type)ev, EINA_FALSE);
          str = eeze_udev_syspath_get_property(device, "ID_CDROM");
          if (!str)
            {
               storage_devices = eina_list_append(storage_devices, eina_stringshare_add(device));
               return;
            }
          eina_stringshare_del(str);
          dev = calloc(1, sizeof(Eeze_Scanner_Device));
          if (!dev) return;
          dev->device = eina_stringshare_add(device);
          dev->poller = ecore_poller_add(ECORE_POLLER_CORE, 32,
                                         (Ecore_Task_Cb)cdrom_timer, dev);
          storage_cdrom = eina_list_append(storage_cdrom, dev);
          break;
        case EEZE_UDEV_EVENT_REMOVE:
        case EEZE_UDEV_EVENT_OFFLINE:
          if (!eina_list_data_find(storage_devices, device))
            {
               EINA_LIST_FOREACH(storage_cdrom, l, dev)
                 if (dev->device == device) break;
               if ((!dev) || (dev->device != device)) return;
            }
          INF("Removed device '%s'", device);
          event_send(device, (Eeze_Scanner_Event_Type)ev, EINA_FALSE);
          EINA_LIST_FOREACH(storage_cdrom, l, dev)
            if (device == dev->device)
              {
                 if (dev->poller) ecore_poller_del(dev->poller);
                 storage_cdrom = eina_list_remove_list(storage_cdrom, l);
                 eina_stringshare_del(dev->device);
                 free(dev);
                 return;
              }
          storage_devices = eina_list_remove(storage_devices, device);
          eina_stringshare_del(device);
          break;
        default:
          INF("Changed device '%s'", device);
          break;
     }
}

/**
 * @brief Callback for EFL_NET_SERVER_EVENT_SERVER_ERROR event.
 *
 * Handles errors reported by the EFL network server.
 * Logs the error, sets the program's retval to failure, and quits the main loop.
 *
 * @param data User data (unused).
 * @param event The EFL event data, where event->info is an Eina_Error.
 */
static void
server_error(void *data EINA_UNUSED, const Efl_Event *event)
{
   Eina_Error *perr = event->info;

   ERR("server error: %s", eina_error_msg_get(*perr));
   retval = EXIT_FAILURE;
   ecore_main_loop_quit();
}

/**
 * @brief Main function for the eeze_scanner daemon.
 *
 * Initializes Efl, Eeze, Eet, and Ecore_Con. Sets up logging,
 * data serialization, initial storage device scan, udev event watches,
 * and the IPC server for clients to connect to. Then enters the
 * Ecore main loop.
 *
 * @return EXIT_SUCCESS on normal termination, EXIT_FAILURE on error.
 */
int
main(void)
{
   Eo *server = NULL, *loop;
   char *path = NULL;
   Ecore_Event_Handler *eh;
   Eina_List *ehl = NULL;
   Eeze_Udev_Watch *uw;
   Eina_List *uwl = NULL;
   Eina_Error err;

   ecore_app_no_system_modules();

   eina_init();
   ecore_init();
   eet_init();
   ecore_con_init();
   eeze_init();
   eeze_disk_function();
   eeze_mount_tabs_watch();

   es_log_dom = eina_log_domain_register("eeze_scanner", EINA_COLOR_CYAN);

   if (!eet_setup())
     goto error_setup;

   if (!storage_setup())
     goto error_setup;

#define ADD_HANDLER(type, cb) \
   ehl = eina_list_append(ehl, ecore_event_handler_add(type, (Ecore_Event_Handler_Cb)cb, NULL))
   ADD_HANDLER(EEZE_EVENT_DISK_UNMOUNT, disk_mount);
   ADD_HANDLER(EEZE_EVENT_DISK_MOUNT, disk_mount);
#undef ADD_HANDLER

#define ADD_WATCH(type, cb) \
   uwl = eina_list_append(uwl, eeze_udev_watch_add(type, EEZE_UDEV_EVENT_NONE, cb, NULL))
   ADD_WATCH(EEZE_UDEV_TYPE_DRIVE_INTERNAL, cb_stor_chg);
   ADD_WATCH(EEZE_UDEV_TYPE_DRIVE_REMOVABLE, cb_stor_chg);
   ADD_WATCH(EEZE_UDEV_TYPE_DRIVE_CDROM, cb_stor_chg);
   ADD_WATCH(EEZE_UDEV_TYPE_DRIVE_MOUNTABLE, cb_vol_chg);
#undef ADD_WATCH

   path = ecore_con_local_path_new(EINA_TRUE, "eeze_scanner", 0);
   if (!path)
     {
        fprintf(stderr, "ERROR: could not get local communication path\n");
        retval = EXIT_FAILURE;
        goto end;
     }

   loop = efl_main_loop_get();

#ifdef EFL_NET_SERVER_UNIX_CLASS
   server = efl_add(EFL_NET_SERVER_SIMPLE_CLASS, loop,
                    efl_net_server_simple_inner_class_set(efl_added, EFL_NET_SERVER_UNIX_CLASS));
#else
   /* TODO: maybe start a TCP using locahost:12345?
    * Right now eina_debug_monitor is only for AF_UNIX, so not an issue.
    */
   fprintf(stderr, "ERROR: your platform doesn't support Efl.Net.Server.Unix\n");
#endif
   if (!server)
     {
        fprintf(stderr, "ERROR: could not create communication server\n");
        retval = EXIT_FAILURE;
        goto end;
     }

   efl_event_callback_add(server, EFL_NET_SERVER_EVENT_CLIENT_ADD, cl_add, NULL);
   efl_event_callback_add(server, EFL_NET_SERVER_EVENT_SERVER_ERROR, server_error, NULL);

#ifdef EFL_NET_SERVER_UNIX_CLASS
   {
      Eo *inner_server = efl_net_server_simple_inner_server_get(server);
      efl_net_server_unix_unlink_before_bind_set(inner_server, EINA_TRUE);
      efl_net_server_unix_leading_directories_create_set(inner_server, EINA_TRUE, 0700);
   }
#endif

   err = efl_net_server_serve(server, path);
   if (err)
     {
        fprintf(stderr, "ERROR: could not serve '%s': %s\n", path, eina_error_msg_get(err));
        retval = EXIT_FAILURE;
        goto end;
     }

   ecore_main_loop_begin();

 end:
   efl_del(server);
   server = NULL;

   free(path);
   path = NULL;

   EINA_LIST_FREE(ehl, eh) ecore_event_handler_del(eh);
   EINA_LIST_FREE(uwl, uw) eeze_udev_watch_del(uw);

 error_setup:
   eeze_shutdown();
   ecore_con_shutdown();
   eet_shutdown();
   ecore_shutdown();
   eina_shutdown();

   return retval;
}
