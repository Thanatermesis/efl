/**
 * @file
 *
 * Copyright (C) 2009 by ProFUSION embedded systems
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 *
 * @author Rafael Antognolli <antognolli@profusion.mobi>
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <Eina.h>
#include <Ecore_Getopt.h>
#include <Ecore.h>
#include <Eldbus.h>
#include <Ethumb.h>
#include <Ethumb_Client.h>

#include "ethumbd_private.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_ID 2000000
#define DAEMON "daemon"
#define ODATA "odata"

#define DBG(...) EINA_LOG_DOM_DBG(_log_domain, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_log_domain, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_log_domain, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_log_domain, __VA_ARGS__)
#define CRI(...) EINA_LOG_DOM_CRIT(_log_domain, __VA_ARGS__)

static const char _ethumb_dbus_bus_name[] = "org.enlightenment.Ethumb";
static const char _ethumb_dbus_interface[] = "org.enlightenment.Ethumb";
static const char _ethumb_dbus_objects_interface[] = "org.enlightenment.Ethumb.objects";
static const char _ethumb_dbus_path[] = "/org/enlightenment/Ethumb";

static Eina_Prefix *_pfx = NULL;
static int _log_domain = -1;

typedef struct _Ethumbd_Setup Ethumbd_Setup;
typedef struct _Ethumbd_Slave Ethumbd_Slave;
typedef struct _Ethumbd_Request Ethumbd_Request;
typedef struct _Ethumbd_Queue Ethumbd_Queue;
typedef struct _Ethumbd_Object Ethumbd_Object;
typedef struct _Ethumbd_Object_Data Ethumbd_Object_Data;
typedef struct _Ethumbd Ethumbd;

struct _Ethumbd_Setup
{
   struct
   {
      Eina_Bool fdo : 1;
      Eina_Bool size : 1;
      Eina_Bool format : 1;
      Eina_Bool aspect : 1;
      Eina_Bool orientation: 1;
      Eina_Bool crop : 1;
      Eina_Bool quality : 1;
      Eina_Bool compress : 1;
      Eina_Bool directory : 1;
      Eina_Bool category : 1;
      Eina_Bool frame : 1;
      Eina_Bool video_time : 1;
      Eina_Bool video_start : 1;
      Eina_Bool video_interval : 1;
      Eina_Bool video_ntimes : 1;
      Eina_Bool video_fps : 1;
      Eina_Bool document_page : 1;
   } flags;
   int fdo;
   int tw, th;
   int format;
   int aspect;
   int orientation;
   float cx, cy;
   int quality;
   int compress;
   const char *directory;
   const char *category;
   const char *theme_file;
   const char *group;
   const char *swallow;
   float video_time;
   float video_start;
   float video_interval;
   unsigned int video_ntimes;
   unsigned int video_fps;
   unsigned int document_page;
};

struct _Ethumbd_Request
{
   int id;
   const char *file, *key;
   const char *thumb, *thumb_key;
   Ethumbd_Setup setup;
};

struct _Ethumbd_Object
{
   int used;
   const char *path;
   const char *client;
   Eina_List *queue;
   int nqueue;
   int id_count;
   int max_id;
   int min_id;
   Eldbus_Service_Interface *iface;
};

struct _Ethumbd_Queue
{
   int count;
   int max_count;
   int nqueue;
   int last;
   int current;
   Ethumbd_Object *table;
   int *list;
};

struct _Ethumbd_Slave
{
   Ecore_Exe *exe;
   char *bufcmd; // buffer to read commands from slave
   int scmd; // size of command to read
   int pcmd; // position in the command buffer
};

struct _Ethumbd
{
   Eldbus_Connection *conn;
   Ecore_Idle_Enterer *idle_enterer;
   Ethumbd_Request *processing;
   Ethumbd_Queue queue;
   double timeout;
   Ecore_Timer *timeout_timer;
   Ecore_Timer *hang_timer;
   Ethumbd_Slave slave;

   Ecore_Event_Handler *data_cb;
   Ecore_Event_Handler *del_cb;
};

struct _Ethumbd_Object_Data
{
   int idx;
   Ethumbd *ed;
};

const Ecore_Getopt optdesc = {
  "ethumbd",
  NULL,
  PACKAGE_VERSION,
  "(C) 2009 - ProFUSION embedded systems",
  "LGPL v2.1 - GNU Lesser General Public License",
  "Ethumb daemon.\n"
  "\n"
  "ethumbd uses the Ethumb library to create thumbnails for any "
  "program that requests it (now just by dbus).\n",
  0,
  {
    ECORE_GETOPT_STORE_DOUBLE
    ('t', "timeout", "finish ethumbd after <timeout> seconds of no activity."),
    ECORE_GETOPT_LICENSE('L', "license"),
    ECORE_GETOPT_COPYRIGHT('C', "copyright"),
    ECORE_GETOPT_VERSION('V', "version"),
    ECORE_GETOPT_HELP('h', "help"),
    ECORE_GETOPT_SENTINEL
  }
};

static Eldbus_Message *_ethumb_dbus_queue_add_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_ethumb_dbus_queue_remove_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_ethumb_dbus_queue_clear_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_ethumb_dbus_ethumb_setup_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_ethumb_dbus_delete_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);

static const Eldbus_Method _ethumb_dbus_objects_methods[] = {
  {
   "queue_add",
   ELDBUS_ARGS({"i", "id"}, {"ay", "file"}, {"ay", "key"}, {"ay", "thumb"}, {"ay", "thumb_key"}),
   ELDBUS_ARGS({"i", "queue_id"}), _ethumb_dbus_queue_add_cb, 0
  },
  {
   "queue_remove", ELDBUS_ARGS({"i", "queue_id"}), ELDBUS_ARGS({"b", "result"}),
   _ethumb_dbus_queue_remove_cb, 0
  },
  {
   "clear_queue", NULL, NULL, _ethumb_dbus_queue_clear_cb, 0
  },
  {
   "ethumb_setup", ELDBUS_ARGS({"a{sv}", "array"}), ELDBUS_ARGS({"b", "result"}),
   _ethumb_dbus_ethumb_setup_cb, 0
  },
  {
   "delete", NULL, NULL, _ethumb_dbus_delete_cb, 0
  },
  { }
};

enum
{
   ETHUMB_DBUS_OBJECTS_SIGNAL_GENERATED = 0,
};

static const Eldbus_Signal _ethumb_dbus_objects_signals[] = {
  [ETHUMB_DBUS_OBJECTS_SIGNAL_GENERATED] = { "generated",
       ELDBUS_ARGS({ "i", "id" }, { "ay", "paths" }, { "ay", "keys" },
                  { "b", "success" }), 0 },
  { }
};

static void _ethumb_dbus_generated_signal(Ethumbd *ed, int *id, const char *thumb_path, const char *thumb_key, Eina_Bool success);
static Eina_Bool _ethumbd_slave_spawn(Ethumbd_Slave *slave, Ethumbd *ed);

/**
 * @brief Ecore timer callback to quit the main loop on inactivity.
 *
 * This function is called when the inactivity timeout is reached. It quits the
 * main loop, causing the daemon to exit gracefully.
 * @param data The Ethumbd context.
 * @return EINA_FALSE to stop the timer from recurring.
 */
static Eina_Bool
_ethumbd_timeout_cb(void *data)
{
   Ethumbd *ed = data;

   ecore_main_loop_quit();
   ed->timeout_timer = NULL;
   return EINA_FALSE;
}

/**
 * @brief Starts the inactivity timeout timer.
 *
 * If the timeout value is non-negative and the timer is not already running,
 * this function starts a timer that will call _ethumbd_timeout_cb after
 * the specified duration of inactivity.
 * @param ed The Ethumbd context.
 */
static void
_ethumbd_timeout_start(Ethumbd *ed)
{
   if (ed->timeout < 0) return;
   if (!ed->timeout_timer)
     ed->timeout_timer = ecore_timer_add(ed->timeout, _ethumbd_timeout_cb, ed);
}

/**
 * @brief Stops the inactivity timeout timer.
 *
 * This is called when there is activity, to prevent the daemon from
 * shutting down.
 * @param ed The Ethumbd context.
 */
static void
_ethumbd_timeout_stop(Ethumbd *ed)
{
   if (!ed->timeout_timer) return;
   ecore_timer_del(ed->timeout_timer);
   ed->timeout_timer = NULL;
}

/**
 * @brief Resets the inactivity timeout timer.
 *
 * This is a convenience function to stop and then start the timer again,
 * effectively resetting the inactivity period.
 * @param ed The Ethumbd context.
 */
static void
_ethumbd_timeout_redo(Ethumbd *ed)
{
   _ethumbd_timeout_stop(ed);
   _ethumbd_timeout_start(ed);
}

/**
 * @brief Ecore timer callback to detect and handle a hung slave process.
 *
 * If a thumbnail request is being processed and takes too long, this function
 * is called. It logs an error and kills the slave process, which will then
 * be respawned by the _ethumbd_slave_del_cb handler.
 * @param data The Ethumbd context.
 * @return EINA_FALSE to stop the timer from recurring.
 */
static Eina_Bool
_ethumbd_hang_cb(void *data)
{
   Ethumbd *ed = data;

   ed->hang_timer = NULL;
   if (ed->processing)
     {
        ERR("timeout while processing thumb");
        if (ed->slave.exe) ecore_exe_kill(ed->slave.exe);
     }
   return EINA_FALSE;
}

/**
 * @brief Starts the hang detection timer.
 *
 * This timer is started when a file is sent to the slave for processing.
 * If the slave doesn't respond within a certain time, it's considered hung.
 * The timeout is a fraction of the main inactivity timeout, with a cap.
 * @param ed The Ethumbd context.
 */
static void
_ethumbd_hang_start(Ethumbd *ed)
{
   double tim = ed->timeout;

   if (tim < 0) tim = 10.0;
   else
     {
        tim = tim / 3.0;
        if (tim > 10.0) tim = 10.0;
     }
   if (!ed->hang_timer)
     ed->hang_timer = ecore_timer_add(tim, _ethumbd_hang_cb, ed);
}

/**
 * @brief Stops the hang detection timer.
 *
 * This should be called when the slave process completes a task.
 * @param ed The Ethumbd context.
 */
static void
_ethumbd_hang_stop(Ethumbd *ed)
{
   if (!ed->hang_timer) return;
   ecore_timer_del(ed->hang_timer);
   ed->hang_timer = NULL;
}

/**
 * @brief Resets the hang detection timer.
 *
 * This is a convenience function to stop and then start the timer again.
 * @param ed The Ethumbd context.
 */
static void
_ethumbd_hang_redo(Ethumbd *ed)
{
   _ethumbd_hang_stop(ed);
   _ethumbd_hang_start(ed);
}

/**
 * @brief Checks if a request ID is valid for a given client object.
 *
 * A request ID is considered invalid if it falls within the range of
 * IDs currently in the queue for that client object. This prevents
 * duplicate request IDs from being added. The range can wrap around MAX_ID.
 *
 * @param eobject The client object context.
 * @param id The request ID to check.
 * @return 0 if the ID is valid (not in the queue), 1 if it is invalid.
 */
static int
_ethumb_dbus_check_id(Ethumbd_Object *eobject, int id)
{
   if (id < 0 || id > MAX_ID)
     return 0;

   if (eobject->min_id < eobject->max_id)
     return id < eobject->min_id || id > eobject->max_id;
   else if (eobject->min_id > eobject->max_id)
     return id < eobject->min_id && id > eobject->max_id;
   else
     return id != eobject->max_id;
}

/**
 * @brief Updates the maximum request ID for a client object.
 *
 * This is called when a new request is added to the queue. It is used
 * by _ethumb_dbus_check_id to manage the range of in-use IDs.
 * @param eobject The client object context.
 * @param id The new maximum ID.
 */
static void
_ethumb_dbus_inc_max_id(Ethumbd_Object *eobject, int id)
{
   if (eobject->min_id < 0 && eobject->max_id < 0)
     eobject->min_id = id;

   eobject->max_id = id;
}

/**
 * @brief Updates the minimum request ID for a client object.
 *
 * This is called after a request has been processed. It finds the next
 * request in the queue and sets its ID as the new minimum. If the queue
 * is empty, the min/max range is reset.
 * @param eobject The client object context.
 */
static void
_ethumb_dbus_inc_min_id(Ethumbd_Object *eobject)
{
   Eina_List *l;

   l = eobject->queue;
   while (l)
     {
	Ethumbd_Request *request = l->data;
	if (request->id >= 0)
	  {
	     eobject->min_id = request->id;
	     break;
	  }

	l = l->next;
     }

   if (!l)
     {
	eobject->min_id = -1;
	eobject->max_id = -1;
     }
}

/**
 * @brief Safely writes data to the slave process pipe.
 *
 * This function wraps ecore_exe_send to provide a check that the slave
 * process is running before attempting to send data.
 * @param slave The slave context.
 * @param buf The data buffer to send.
 * @param size The number of bytes to send.
 * @return 1 on success, 0 on failure (if slave is not running).
 */
int
_ethumbd_write_safe(Ethumbd_Slave *slave, const void *buf, ssize_t size)
{

   if (!slave->exe)
     {
	ERR("slave process isn't running.");
	return 0;
     }

   ecore_exe_send(slave->exe, buf, size);
   return 1;
}

/**
 * @brief Sends an OP_NEW command to the slave.
 *
 * This informs the slave process that a new client object has been created,
 * allowing it to allocate any necessary per-client resources.
 * @param slave The slave context.
 * @param idx The index of the new client object.
 */
static void
_ethumbd_child_write_op_new(Ethumbd_Slave *slave, int idx)
{
   int id = ETHUMBD_OP_NEW;
   _ethumbd_write_safe(slave, &id, sizeof(id));
   _ethumbd_write_safe(slave, &idx, sizeof(idx));
}

/**
 * @brief Sends an OP_DEL command to the slave.
 *
 * This informs the slave process that a client object has been deleted,
 * allowing it to free associated resources.
 * @param slave The slave context.
 * @param idx The index of the deleted client object.
 */
static void
_ethumbd_child_write_op_del(Ethumbd_Slave *slave, int idx)
{
   int id = ETHUMBD_OP_DEL;
   _ethumbd_write_safe(slave, &id, sizeof(id));
   _ethumbd_write_safe(slave, &idx, sizeof(idx));
}

/**
 * @brief Writes a length-prefixed string to the slave process.
 *
 * This serialization format allows the slave to know how many bytes to read
 * for the string.
 * @param slave The slave context.
 * @param str The null-terminated string to write. Can be NULL.
 */
static void
_ethumbd_pipe_str_write(Ethumbd_Slave *slave, const char *str)
{
   int len;

   if (str)
     len = strlen(str) + 1;
   else
     len = 0;

   _ethumbd_write_safe(slave, &len, sizeof(len));
   _ethumbd_write_safe(slave, str, len);
}

/**
 * @brief Sends an OP_GENERATE command to the slave.
 *
 * This command instructs the slave to generate a thumbnail for the given file.
 * All string parameters are sent using the length-prefixed format.
 *
 * @param slave The slave context.
 * @param idx The index of the client object this request belongs to.
 * @param path The path to the source file.
 * @param key A key associated with the source file (e.g., for Evas Eet files).
 * @param thumb_path The destination path for the thumbnail file.
 * @param thumb_key The key for the thumbnail file.
 */
static void
_ethumbd_child_write_op_generate(Ethumbd_Slave *slave,
				 int idx, const char *path, const char *key,
				 const char *thumb_path, const char *thumb_key)
{
   int id = ETHUMBD_OP_GENERATE;

   _ethumbd_write_safe(slave, &id, sizeof(id));
   _ethumbd_write_safe(slave, &idx, sizeof(idx));

   _ethumbd_pipe_str_write(slave, path);
   _ethumbd_pipe_str_write(slave, key);
   _ethumbd_pipe_str_write(slave, thumb_path);
   _ethumbd_pipe_str_write(slave, thumb_key);
}

/**
 * @brief Callback invoked when a thumbnail generation is complete.
 *
 * This function is called after the slave process signals completion of a
 * thumbnail request. It sends the "generated" D-Bus signal to the client,
 * cleans up the processed request object, and resets the inactivity and
 * hang timers.
 *
 * @param ed The main daemon context.
 * @param success EINA_TRUE if generation was successful, EINA_FALSE otherwise.
 * @param thumb_path The path to the generated thumbnail file.
 * @param thumb_key The key of the generated thumbnail.
 */
static void
_generated_cb(Ethumbd *ed, Eina_Bool success, const char *thumb_path, const char *thumb_key)
{
   int i = ed->queue.current;

   DBG("thumbnail ready at: \"%s:%s\"", thumb_path, thumb_key);

   if (ed->queue.table[i].used)
     _ethumb_dbus_generated_signal
       (ed, &ed->processing->id, thumb_path, thumb_key, success);
   eina_stringshare_del(ed->processing->file);
   eina_stringshare_del(ed->processing->key);
   eina_stringshare_del(ed->processing->thumb);
   eina_stringshare_del(ed->processing->thumb_key);
   free(ed->processing);
   ed->processing = NULL;
   _ethumbd_timeout_redo(ed);
   _ethumbd_hang_stop(ed);
}

/**
 * @brief Processes a complete command received from the slave process.
 *
 * Once a full command message has been read from the slave's stdout, this
 * function is called to parse the message and act on it. The message
 * format is a success flag, followed by two length-prefixed strings for
 * thumb_path and thumb_key.
 *
 * @param ed The main daemon context.
 */
static void
_ethumbd_slave_cmd_ready(Ethumbd *ed)
{
   const char *bufcmd = ed->slave.bufcmd;
   Eina_Bool success;
   const char *thumb_path = NULL;
   const char *thumb_key = NULL;
   int size_path, size_key;

   /* NOTE: accessing values directly on bufcmd breaks alignment
    * as the first item is an Eina_Bool (size 1) and second is
    * an integer (size 4, alignment 4).
    * Thus copy to stack values before using them, to have proper alignment.
    */
#define READVAL(dst)				\
   memcpy(&dst, bufcmd, sizeof(dst));		\
   bufcmd += sizeof(dst);

   READVAL(success);

   READVAL(size_path);
   if (size_path)
     {
	thumb_path = bufcmd;
	bufcmd += size_path;
     }

   READVAL(size_key);
   if (size_key) thumb_key = bufcmd;

#undef READVAL

   _generated_cb(ed, success, thumb_path, thumb_key);

   free(ed->slave.bufcmd);
   ed->slave.bufcmd = NULL;
   ed->slave.scmd = 0;
}

/**
 * @brief Allocates memory for an incoming command from the slave.
 *
 * The slave sends messages prefixed with their total size. This function reads
 * that size and allocates a buffer to hold the rest of the message.
 *
 * @param ed The main daemon context.
 * @param ssize The size of the available data chunk from the slave.
 * @param sdata Pointer to the available data.
 * @return The number of bytes consumed (the size of the size prefix), or 0 on error.
 */
static int
_ethumbd_slave_alloc_cmd(Ethumbd *ed, int ssize, char *sdata)
{
   int *scmd;

   if (ed->slave.bufcmd)
     return 0;

   scmd = (int *)sdata;
   if (ssize < (int)sizeof(*scmd)) {
	ERR("could not read size of command.");
	return 0;
   }
   ed->slave.bufcmd = malloc(*scmd);
   ed->slave.scmd = *scmd;
   ed->slave.pcmd = 0;

   return sizeof(*scmd);
}

/**
 * @brief Ecore event handler for data received from the slave process.
 *
 * This function is called whenever the slave process writes to its stdout.
 * It handles the asynchronous and potentially fragmented nature of pipe I/O
 * by assembling the data into complete command messages.
 *
 * @param data The Ethumbd context.
 * @param type The event type (unused).
 * @param event The Ecore_Exe_Event_Data event structure.
 * @return ECORE_CALLBACK_PASS_ON (1).
 */
static Eina_Bool
_ethumbd_slave_data_read_cb(void *data, int type EINA_UNUSED, void *event)
{
   Ethumbd *ed = data;
   Ecore_Exe_Event_Data *ev = event;
   int ssize;
   char *sdata;

   if (ev->exe != ed->slave.exe)
     {
	ERR("PARENT ERROR: slave != ev->exe");
	return 0;
     }

   ssize = ev->size;
   sdata = ev->data;

   while (ssize > 0)
     {
	if (!ed->slave.bufcmd)
	  {
	     int n;
	     n = _ethumbd_slave_alloc_cmd(ed, ssize, sdata);
	     ssize -= n;
	     sdata += n;
	  }
	else
	  {
	     char *bdata;
	     int nbytes;
	     bdata = ed->slave.bufcmd + ed->slave.pcmd;
	     nbytes = ed->slave.scmd - ed->slave.pcmd;
	     nbytes = ssize < nbytes ? ssize : nbytes;
	     memcpy(bdata, sdata, nbytes);
	     sdata += nbytes;
	     ssize -= nbytes;
	     ed->slave.pcmd += nbytes;

	     if (ed->slave.pcmd == ed->slave.scmd)
	       _ethumbd_slave_cmd_ready(ed);
	  }
     }
   _ethumbd_timeout_redo(ed);
   return 1;
}

/**
 * @brief Ecore event handler for slave process deletion/exit.
 *
 * This function is called when the slave process terminates for any reason.
 * It handles cleanup, logs errors, and if a request was being processed, it
 * sends a failure signal. It then attempts to respawn the slave and
 * re-initialize it with all existing client contexts.
 *
 * @param data The Ethumbd context.
 * @param type The event type (unused).
 * @param event The Ecore_Exe_Event_Del event structure.
 * @return ECORE_CALLBACK_RENEW (EINA_TRUE) to keep the handler, or ECORE_CALLBACK_CANCEL (EINA_FALSE) on failure.
 */
static Eina_Bool
_ethumbd_slave_del_cb(void *data, int type EINA_UNUSED, void *event)
{
   Ethumbd *ed = data;
   Ecore_Exe_Event_Del *ev = event;
   int i;

   if (ev->exe != ed->slave.exe)
     return 1;

   _ethumbd_hang_stop(ed);

   if (ev->exited)
     ERR("slave exited with code: %d", ev->exit_code);
   else if (ev->signalled)
     ERR("slave exited by signal: %d", ev->exit_signal);

   if (!ed->processing)
     goto end;

   i = ed->queue.current;
   ERR("failed to generate thumbnail for: \"%s:%s\"",
       ed->processing->file, ed->processing->key);

   if (ed->queue.table[i].used)
     _ethumb_dbus_generated_signal
       (ed, &ed->processing->id, NULL, NULL, EINA_FALSE);
   eina_stringshare_del(ed->processing->file);
   eina_stringshare_del(ed->processing->key);
   eina_stringshare_del(ed->processing->thumb);
   eina_stringshare_del(ed->processing->thumb_key);
   free(ed->processing);
   ed->processing = NULL;

end:
   ed->slave.exe = NULL;
   if (ed->slave.bufcmd)
     free(ed->slave.bufcmd);

   if (!_ethumbd_slave_spawn(&ed->slave, ed))
     return EINA_FALSE;

   /* restart all queue */
   for (i = 0; i < ed->queue.count; ++i)
     _ethumbd_child_write_op_new(&ed->slave, ed->queue.list[i]);

   return EINA_TRUE;
}

/**
 * @brief Writes a single setup parameter to the slave process.
 *
 * This function serializes a setup option (e.g., size, quality) and sends
 * it to the slave. The data is sent as a type identifier followed by the
 * value, which can be an integer, a float, or a length-prefixed string.
 *
 * @param slave The slave context.
 * @param type The Ethumbd_Op type for the setting.
 * @param data A pointer to the value for the setting.
 */
static void
_ethumbd_pipe_write_setup(Ethumbd_Slave *slave, int type, const void *data)
{
   const int *i_value;
   const float *f_value;

   _ethumbd_write_safe(slave, &type, sizeof(type));

   switch (type)
     {
      case ETHUMBD_FDO:
      case ETHUMBD_FORMAT:
      case ETHUMBD_ASPECT:
      case ETHUMBD_ORIENTATION:
      case ETHUMBD_QUALITY:
      case ETHUMBD_COMPRESS:
      case ETHUMBD_SIZE_W:
      case ETHUMBD_SIZE_H:
      case ETHUMBD_DOCUMENT_PAGE:
      case ETHUMBD_VIDEO_NTIMES:
      case ETHUMBD_VIDEO_FPS:
	 i_value = data;
	 _ethumbd_write_safe(slave, i_value, sizeof(*i_value));
	 break;
      case ETHUMBD_CROP_X:
      case ETHUMBD_CROP_Y:
      case ETHUMBD_VIDEO_TIME:
      case ETHUMBD_VIDEO_START:
      case ETHUMBD_VIDEO_INTERVAL:
	 f_value = data;
	 _ethumbd_write_safe(slave, f_value, sizeof(*f_value));
	 break;
      case ETHUMBD_DIRECTORY:
      case ETHUMBD_CATEGORY:
      case ETHUMBD_FRAME_FILE:
      case ETHUMBD_FRAME_GROUP:
      case ETHUMBD_FRAME_SWALLOW:
	 _ethumbd_pipe_str_write(slave, data);
	 break;
      case ETHUMBD_SETUP_FINISHED:
	 break;
      default:
	 ERR("wrong ethumb setup parameter.");
     }
}

/**
 * @brief Processes a setup request by sending it to the slave.
 *
 * A setup request is a special request with a negative ID that contains
 * configuration for subsequent thumbnail generations for a specific client.
 * This function iterates over the flags in the request's setup struct and
 * sends each configured option to the slave.
 *
 * @param ed The main daemon context.
 */
static void
_process_setup(Ethumbd *ed)
{
   int op_id = ETHUMBD_OP_SETUP;
   int idx = ed->queue.current;

   Ethumbd_Setup *setup = &ed->processing->setup;
   Ethumbd_Slave *slave = &ed->slave;

   _ethumbd_write_safe(slave, &op_id, sizeof(op_id));
   _ethumbd_write_safe(slave, &idx, sizeof(idx));

   if (setup->flags.fdo)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_FDO, &setup->fdo);
   if (setup->flags.size)
     {
	_ethumbd_pipe_write_setup(slave, ETHUMBD_SIZE_W, &setup->tw);
	_ethumbd_pipe_write_setup(slave, ETHUMBD_SIZE_H, &setup->th);
     }
   if (setup->flags.format)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_FORMAT, &setup->format);
   if (setup->flags.aspect)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_ASPECT, &setup->aspect);
   if (setup->flags.orientation)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_ORIENTATION, &setup->orientation);
   if (setup->flags.crop)
     {
	_ethumbd_pipe_write_setup(slave, ETHUMBD_CROP_X, &setup->cx);
	_ethumbd_pipe_write_setup(slave, ETHUMBD_CROP_Y, &setup->cy);
     }
   if (setup->flags.quality)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_QUALITY, &setup->quality);
   if (setup->flags.compress)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_COMPRESS, &setup->compress);
   if (setup->flags.directory)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_DIRECTORY, setup->directory);
   if (setup->flags.category)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_CATEGORY, setup->category);
   if (setup->flags.frame)
     {
	_ethumbd_pipe_write_setup(slave, ETHUMBD_FRAME_FILE, setup->theme_file);
	_ethumbd_pipe_write_setup(slave, ETHUMBD_FRAME_GROUP, setup->group);
	_ethumbd_pipe_write_setup(slave, ETHUMBD_FRAME_SWALLOW, setup->swallow);
     }
   if (setup->flags.video_time)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_VIDEO_TIME, &setup->video_time);
   if (setup->flags.video_start)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_VIDEO_START, &setup->video_start);
   if (setup->flags.video_interval)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_VIDEO_INTERVAL,
			       &setup->video_interval);
   if (setup->flags.video_ntimes)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_VIDEO_NTIMES, &setup->video_ntimes);
   if (setup->flags.video_fps)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_VIDEO_FPS, &setup->video_fps);
   if (setup->flags.document_page)
     _ethumbd_pipe_write_setup(slave, ETHUMBD_DOCUMENT_PAGE,
			       &setup->document_page);
   _ethumbd_pipe_write_setup(slave, ETHUMBD_SETUP_FINISHED, NULL);


   if (setup->directory) eina_stringshare_del(setup->directory);
   if (setup->category) eina_stringshare_del(setup->category);
   if (setup->theme_file) eina_stringshare_del(setup->theme_file);
   if (setup->group) eina_stringshare_del(setup->group);
   if (setup->swallow) eina_stringshare_del(setup->swallow);

   free(ed->processing);
   ed->processing = NULL;
}

/**
 * @brief Processes a file generation request by sending it to the slave.
 *
 * This function takes the currently processing request and sends an
 * OP_GENERATE command to the slave. It also starts the hang detection timer.
 * @param ed The main daemon context.
 */
static void
_process_file(Ethumbd *ed)
{
   _ethumbd_hang_redo(ed);
   _ethumbd_child_write_op_generate
     (&ed->slave, ed->queue.current, ed->processing->file,
      ed->processing->key, ed->processing->thumb, ed->processing->thumb_key);
}

/**
 * @brief Selects the next client object to process a request from.
 *
 * This function implements a simple round-robin scheduling algorithm. It
 * iterates through the list of active client objects, starting from the one
 * after the last one serviced, and returns the index of the first one found
 * with a non-empty request queue.
 *
 * @param queue The main request queue.
 * @return The index of the next client object to service.
 */
static int
_get_next_on_queue(Ethumbd_Queue *queue)
{
   int i, idx;
   Ethumbd_Object *eobject;

   i = queue->last;
   i++;
   if (i >= queue->count)
     i = 0;

   idx = queue->list[i];
   eobject = &(queue->table[idx]);
   while (!eobject->nqueue)
     {
	i = (i + 1) % queue->count;

	idx = queue->list[i];
	eobject = &(queue->table[idx]);
     }

   return queue->list[i];
}

/**
 * @brief Ecore idle enterer callback to process the thumbnail request queue.
 *
 * This function runs when the main loop is idle. It pulls one request from the
 * queue (using round-robin scheduling across clients), and sends it to the
 * slave process for generation or configuration. It ensures only one request
 * is processing at a time.
 *
 * @param data The main daemon context.
 * @return ECORE_CALLBACK_RENEW (1) to continue processing, or ECORE_CALLBACK_CANCEL (0) if queue is empty.
 */
static Eina_Bool
_process_queue_cb(void *data)
{
   Ethumbd_Object *eobject;
   int i;
   Ethumbd *ed = data;
   Ethumbd_Queue *queue = &ed->queue;
   Ethumbd_Request *request;

   if (ed->processing)
     return 1;

   if (!queue->nqueue)
     {
	ed->idle_enterer = NULL;
        _ethumbd_timeout_redo(ed);
	ed->idle_enterer = NULL;
	return 0;
     }

   i = _get_next_on_queue(queue);
   eobject = &(queue->table[i]);

   request = eina_list_data_get(eobject->queue);
   eobject->queue = eina_list_remove_list(eobject->queue, eobject->queue);
   ed->queue.current = i;
   DBG("processing file: \"%s:%s\"...", request->file,
       request->key);
   ed->processing = request;

   if (request->id < 0)
     _process_setup(ed);
   else
     {
	_process_file(ed);
	_ethumb_dbus_inc_min_id(eobject);
     }
   eobject->nqueue--;
   queue->nqueue--;

   queue->last = i;

   return 1;
}

/**
 * @brief Starts the queue processing idle enterer.
 *
 * If the idle enterer is not already running, this function adds it.
 * This is typically called when a new request is added to an empty queue.
 * @param ed The main daemon context.
 */
static void
_process_queue_start(Ethumbd *ed)
{
   if (!ed->idle_enterer)
     ed->idle_enterer = ecore_idle_enterer_add(_process_queue_cb, ed);
}

/**
 * @brief Stops the queue processing idle enterer.
 *
 * This is not heavily used, but could be called to pause queue processing.
 * The enterer typically removes itself when the queue becomes empty.
 * @param ed The main daemon context.
 */
static void
_process_queue_stop(Ethumbd *ed)
{
   if (ed->idle_enterer)
     {
	ecore_idle_enterer_del(ed->idle_enterer);
	ed->idle_enterer = NULL;
     }
}

/**
 * @brief Appends a new client object to the table.
 *
 * Finds a free slot in the client object table (reallocating if necessary)
 * and initializes it for a new client.
 *
 * @param ed The main daemon context.
 * @return The index of the newly created object, or -1 on failure.
 */
static int
_ethumb_table_append(Ethumbd *ed)
{
   int i;
   char buf[1024];
   Ethumbd_Queue *q = &ed->queue;

   if (q->count == q->max_count)
     {
	int new_max = q->max_count + 5;
	int start, size;
	void *tmp;

	start = q->max_count;
	size = new_max - q->max_count;

	tmp = realloc(q->table, new_max * sizeof(Ethumbd_Object));
	if (!tmp)
	  {
	     CRI("could not realloc q->table to %zd bytes: %s",
		  new_max * sizeof(Ethumbd_Object), strerror(errno));
	     return -1;
	  }
	q->table = tmp;
	memset(&q->table[start], 0, size * sizeof(Ethumbd_Object));

	tmp = realloc(q->list, new_max * sizeof(int));
	if (!tmp)
	  {
	     CRI("could not realloc q->list to %zd bytes: %s",
		  new_max * sizeof(int), strerror(errno));
	     return -1;
	  }
	q->list = tmp;

	q->max_count = new_max;
     }

   for (i = 0; i < q->max_count; i++)
     {
	if (!q->table[i].used)
	  break;
     }

   if (i >= q->max_count)
     {
        CRI("cannot find free table slot in table of %i", q->max_count);
        return -1;
     }

   snprintf(buf, sizeof(buf), "%s/%d", _ethumb_dbus_path, i);
   q->table[i].used = 1;
   q->table[i].path = eina_stringshare_add(buf);
   q->table[i].max_id = -1;
   q->table[i].min_id = -1;
   q->list[q->count] = i;
   q->count++;
   DBG("new object: %s, idx = %d, count = %d", buf, i, q->count);

   return i;
}

static void _name_owner_changed_cb(void *context, const char *bus, const char *old_id, const char *new_id);

/**
 * @brief Deletes a client object from the table and cleans up its resources.
 *
 * This function frees all memory associated with a client object, including
 * its request queue. It also unregisters its D-Bus object and informs the
 * slave process that the client is gone.
 *
 * @param ed The main daemon context.
 * @param i The index of the client object to delete.
 */
static void
_ethumb_table_del(Ethumbd *ed, int i)
{
   int j;
   Eina_List *l;
   Ethumbd_Queue *q = &ed->queue;
   Ethumbd_Object_Data *odata;

   eina_stringshare_del(q->table[i].path);

   l = q->table[i].queue;
   while (l)
     {
	Ethumbd_Request *request = l->data;
	eina_stringshare_del(request->file);
	eina_stringshare_del(request->key);
	eina_stringshare_del(request->thumb);
	eina_stringshare_del(request->thumb_key);
	free(request);
	l = eina_list_remove_list(l, l);
     }
   q->nqueue -= q->table[i].nqueue;

   odata = eldbus_service_object_data_del(q->table[i].iface, ODATA);
   eldbus_name_owner_changed_callback_del(ed->conn, ed->queue.table[i].client,
                                         _name_owner_changed_cb, odata);
   //this string was not been freed previously
   eina_stringshare_del(ed->queue.table[i].client);
   free(odata);
   eldbus_service_object_unregister(q->table[i].iface);

   memset(&(q->table[i]), 0, sizeof(Ethumbd_Object));
   for (j = 0; j < q->count; j++)
     {
	if (q->list[j] == i)
	  q->list[j] = q->list[q->count - 1];
     }

   q->count--;
   _ethumbd_child_write_op_del(&ed->slave, i);
   if (!q->count && !ed->processing)
     _ethumbd_timeout_redo(ed);
}

/**
 * @brief Clears all client objects from the table.
 *
 * This is called during shutdown to ensure all client resources are released.
 * @param ed The main daemon context.
 */
static void
_ethumb_table_clear(Ethumbd *ed)
{
   int i;

   for (i = 0; i < ed->queue.max_count; i++)
     if (ed->queue.table[i].used)
       _ethumb_table_del(ed, i);
}

/**
 * @brief D-Bus callback for when a client's connection is lost.
 *
 * This function is registered to be called by Eldbus when a client that has
 * created a thumbnailing object disconnects from the D-Bus. It triggers the
 * deletion of the corresponding client object and its resources.
 *
 * @param context The Ethumbd_Object_Data associated with the client.
 * @param bus The name of the bus the client was on.
 * @param old_id The old owner ID of the client name.
 * @param new_id The new owner ID of the client name (empty on disconnect).
 */
static void
_name_owner_changed_cb(void *context, const char *bus, const char *old_id, const char *new_id)
{
   Ethumbd_Object_Data *odata = context;
   Ethumbd *ed = odata->ed;

   DBG("NameOwnerChanged: name = %s, from = %s, to = %s", bus, old_id, new_id);
   if (new_id[0])
     return;
   _ethumb_table_del(ed, odata->idx);
}

static const Eldbus_Service_Interface_Desc client_desc = {
   _ethumb_dbus_objects_interface, _ethumb_dbus_objects_methods,
   _ethumb_dbus_objects_signals, NULL, NULL, NULL
};

/**
 * @brief D-Bus method handler for creating a new thumbnailing session object.
 *
 * When a client calls the "new" method, this function is invoked. It creates
 * a new client object, registers a new D-Bus object path for it (e.g.,
 * /org/enlightenment/Ethumb/0), and returns this path to the client. The client
 * then uses this object path for all subsequent communication for that session.
 *
 * @param interface The D-Bus service interface.
 * @param msg The incoming D-Bus message.
 * @return A reply message containing the object path of the new session object.
 */
static Eldbus_Message *
_ethumb_dbus_ethumb_new_cb(const Eldbus_Service_Interface *interface, const Eldbus_Message *msg)
{
   Eldbus_Message *reply;
   Eldbus_Service_Interface *iface;
   Ethumbd_Object_Data *odata;
   int i;
   const char *return_path = "";
   const char *client;
   Ethumbd *ed;

   ed = eldbus_service_object_data_get(interface, DAEMON);
   if (!ed) goto end_new;
   client = eldbus_message_sender_get(msg);
   if (!client)
     goto end_new;

   i = _ethumb_table_append(ed);
   if (i < 0)
     goto end_new;

   odata = calloc(1, sizeof(*odata));
   odata->idx = i;
   odata->ed = ed;

   ed->queue.table[i].client = eina_stringshare_add(client);
   return_path = ed->queue.table[i].path;

   iface = eldbus_service_interface_register(ed->conn, return_path, &client_desc);
   if (!iface)
     {
	ERR("could not create dbus_object.");
	free(odata);
	return_path = "";
	goto end_new;
     }
   eldbus_service_object_data_set(iface, ODATA, odata);
   ed->queue.table[i].iface = iface;
   eldbus_name_owner_changed_callback_add(ed->conn, client,
                                         _name_owner_changed_cb, odata,
                                         EINA_TRUE);
   _ethumbd_child_write_op_new(&ed->slave, i);
   _ethumbd_timeout_redo(ed);
   _ethumbd_hang_redo(ed);

 end_new:
   reply = eldbus_message_method_return_new(msg);
   eldbus_message_arguments_append(reply, "o", return_path);
   return reply;
}

static const Eldbus_Method _ethumb_dbus_methods[] = {
    {
     "new", NULL, ELDBUS_ARGS({"o", "path"}), _ethumb_dbus_ethumb_new_cb, 0
    },
    { }
};

/**
 * @brief Helper function to extract a string from a D-Bus byte array ('ay').
 *
 * D-Bus strings are often sent as byte arrays. This function reads an 'ay'
 * iterator, extracts the content, and returns it as a new eina_stringshare
 * instance. It handles empty or NULL strings correctly.
 *
 * @param iter The D-Bus message iterator positioned at the byte array.
 * @return A new stringshared string, or NULL if the array is empty or invalid.
 * The caller is responsible for freeing the returned stringshare.
 */
static const char *
_ethumb_dbus_get_bytearray(Eldbus_Message_Iter *iter)
{
   int length;
   const char *result;

   if (!eldbus_message_iter_fixed_array_get(iter, 'y', &result,
                                           &length))
     {
        ERR("not byte array element. Signature: %s",
            eldbus_message_iter_signature_get(iter));
        return NULL;
     }

   if ((length == 0) || (result[0] == '\0'))
     return NULL;
   else
     return eina_stringshare_add_length(result, length);
}

/**
 * @brief Helper function to append a string as a D-Bus byte array ('ay').
 *
 * This function takes a C string and appends it to a D-Bus message as a
 * byte array, including the null terminator.
 *
 * @param parent The parent message iterator.
 * @param array The iterator for the byte array container.
 * @param string The string to append. An empty string is used if NULL.
 */
static void
_ethumb_dbus_append_bytearray(Eldbus_Message_Iter *parent, Eldbus_Message_Iter *array, const char *string)
{
   int i, size;

   if (!string)
     string = "";

   size = strlen(string) + 1;
   for (i = 0; i < size; i++)
     eldbus_message_iter_basic_append(array, 'y', string[i]);
   eldbus_message_iter_container_close(parent, array);
}

/**
 * @brief D-Bus method handler for adding a file to the thumbnail queue.
 *
 * Parses the arguments for a new thumbnail request (ID, file, key, etc.)
 * from the D-Bus message, creates a request object, and adds it to the
 * appropriate client's queue.
 *
 * @param iface The D-Bus service interface for the client object.
 * @param msg The incoming D-Bus message.
 * @return A reply message containing the ID of the queued request.
 */
static Eldbus_Message *
_ethumb_dbus_queue_add_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Eldbus_Message *reply;
   const char *file, *key, *thumb, *thumb_key;
   Ethumbd_Object_Data *odata;
   Ethumbd_Object *eobject;
   Ethumbd *ed;
   Ethumbd_Request *request;
   int id = -1;
   Eldbus_Message_Iter *file_iter, *key_iter, *thumb_iter, *thumb_key_iter;

   if (!eldbus_message_arguments_get(msg, "iayayayay", &id, &file_iter,
                                    &key_iter, &thumb_iter, &thumb_key_iter))
     {
        ERR("Error getting arguments.");
        goto end;
     }
   file = _ethumb_dbus_get_bytearray(file_iter);
   key = _ethumb_dbus_get_bytearray(key_iter);
   thumb = _ethumb_dbus_get_bytearray(thumb_iter);
   thumb_key = _ethumb_dbus_get_bytearray(thumb_key_iter);

   if (!file)
     {
        eina_stringshare_del(key);
        eina_stringshare_del(thumb);
        eina_stringshare_del(thumb_key);
	ERR("no filename given.");
	goto end;
     }

   odata = eldbus_service_object_data_get(iface, ODATA);
   if (!odata)
     {
        eina_stringshare_del(file);
        eina_stringshare_del(key);
        eina_stringshare_del(thumb);
        eina_stringshare_del(thumb_key);
	ERR("could not get dbus_object data.");
	goto end;
     }

   ed = odata->ed;
   eobject = &(ed->queue.table[odata->idx]);
   if (!_ethumb_dbus_check_id(eobject, id))
     goto end;
   request = calloc(1, sizeof(*request));
   request->id = id;
   request->file = file;
   request->key = key;
   request->thumb = thumb;
   request->thumb_key = thumb_key;
   eobject->queue = eina_list_append(eobject->queue, request);
   eobject->nqueue++;
   ed->queue.nqueue++;
   _ethumb_dbus_inc_max_id(eobject, id);

   _process_queue_start(ed);

end:
   reply = eldbus_message_method_return_new(msg);
   eldbus_message_arguments_append(reply, "i", id);
   return reply;
}

/**
 * @brief D-Bus method handler for removing a request from the queue.
 *
 * Finds and removes a pending request from a client's queue, identified by its ID.
 *
 * @param iface The D-Bus service interface for the client object.
 * @param msg The incoming D-Bus message containing the request ID.
 * @return A reply message with a boolean indicating success.
 */
static Eldbus_Message *
_ethumb_dbus_queue_remove_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Eldbus_Message *reply;
   int id;
   Ethumbd_Object_Data *odata;
   Ethumbd_Object *eobject;
   Ethumbd_Request *request;
   Ethumbd *ed;
   Eina_Bool r = EINA_FALSE;
   Eina_List *l;

   if (!eldbus_message_arguments_get(msg, "i", &id))
     {
        ERR("Error getting arguments.");
        goto end;
     }
   odata = eldbus_service_object_data_get(iface, ODATA);
   if (!odata)
     {
	ERR("could not get dbus_object data.");
	goto end;
     }

   ed = odata->ed;
   eobject = &ed->queue.table[odata->idx];
   l = eobject->queue;
   while (l)
     {
	request = l->data;
	if (id == request->id)
	  break;
	l = l->next;
     }

   if (l)
     {
	r = EINA_TRUE;
	eina_stringshare_del(request->file);
	eina_stringshare_del(request->key);
	eina_stringshare_del(request->thumb);
	eina_stringshare_del(request->thumb_key);
	free(request);
	eobject->queue = eina_list_remove_list(eobject->queue, l);
	eobject->nqueue--;
	ed->queue.nqueue--;
	_ethumb_dbus_inc_min_id(eobject);
     }

 end:
   reply = eldbus_message_method_return_new(msg);
   eldbus_message_arguments_append(reply, "b", r);
   return reply;
}

/**
 * @brief D-Bus method handler for clearing a client's entire request queue.
 *
 * Removes all pending requests for a specific client.
 *
 * @param iface The D-Bus service interface for the client object.
 * @param msg The incoming D-Bus message.
 * @return An empty reply message.
 */
static Eldbus_Message *
_ethumb_dbus_queue_clear_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Ethumbd_Object_Data *odata;
   Ethumbd_Object *eobject;
   Ethumbd *ed;
   Eina_List *l;

   odata = eldbus_service_object_data_get(iface, ODATA);
   if (!odata)
     {
	ERR("could not get dbus_object data.");
	goto end;
     }

   ed = odata->ed;
   eobject = &ed->queue.table[odata->idx];
   l = eobject->queue;
   while (l)
     {
	Ethumbd_Request *request = l->data;
	eina_stringshare_del(request->file);
	eina_stringshare_del(request->key);
	eina_stringshare_del(request->thumb);
	eina_stringshare_del(request->thumb_key);
	free(request);
	l = eina_list_remove_list(l, l);
     }
   ed->queue.nqueue -= eobject->nqueue;
   eobject->nqueue = 0;

 end:
   return eldbus_message_method_return_new(msg);
}

/**
 * @brief D-Bus method handler for deleting a thumbnailing session object.
 *
 * This is called when the client explicitly wants to end its session.
 * It cleans up and removes the client object.
 *
 * @param iface The D-Bus service interface for the client object.
 * @param msg The incoming D-Bus message.
 * @return An empty reply message.
 */
static Eldbus_Message *
_ethumb_dbus_delete_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Eldbus_Message *reply;
   Ethumbd_Object_Data *odata;
   Ethumbd *ed;

   reply = eldbus_message_method_return_new(msg);

   odata = eldbus_service_object_data_get(iface, ODATA);
   if (!odata)
     {
	ERR("could not get dbus_object data for del_cb.");
	return reply;
     }
   ed = odata->ed;
   _ethumb_table_del(ed, odata->idx);
   free(odata);

   return reply;
}

static int
_ethumb_dbus_fdo_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   int fdo;

   if (!eldbus_message_iter_arguments_get(variant, "i", &fdo))
     {
	ERR("invalid param for fdo_set.");
	return 0;
     }

   DBG("setting fdo to: %d", fdo);
   request->setup.flags.fdo = 1;
   request->setup.fdo = fdo;

   return 1;
}

static int
_ethumb_dbus_size_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   Eldbus_Message_Iter *st;
   int w, h;

   if (!eldbus_message_iter_arguments_get(variant, "(ii)", &st))
     {
	ERR("invalid param for size_set.");
	return 0;
     }

   if (!eldbus_message_iter_arguments_get(st, "ii", &w, &h)) return 0;
   DBG("setting size to: %dx%d", w, h);
   request->setup.flags.size = 1;
   request->setup.tw = w;
   request->setup.th = h;

   return 1;
}

static int
_ethumb_dbus_format_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   int format;

   if (!eldbus_message_iter_arguments_get(variant, "i", &format))
     {
	ERR("invalid param for format_set.");
	return 0;
     }

   DBG("setting format to: %d", format);
   request->setup.flags.format = 1;
   request->setup.format = format;

   return 1;
}

static int
_ethumb_dbus_aspect_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   int aspect;

   if (!eldbus_message_iter_arguments_get(variant, "i", &aspect))
     {
	ERR("invalid param for aspect_set.");
	return 0;
     }

   DBG("setting aspect to: %d", aspect);
   request->setup.flags.aspect = 1;
   request->setup.aspect = aspect;

   return 1;
}

static int
_ethumb_dbus_orientation_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   int orientation;

   if (!eldbus_message_iter_arguments_get(variant, "i", &orientation))
     {
	ERR("invalid param for orientation_set.");
	return 0;
     }

   DBG("setting orientation to: %d", orientation);
   request->setup.flags.orientation = 1;
   request->setup.orientation = orientation;

   return 1;
}

static int
_ethumb_dbus_crop_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   Eldbus_Message_Iter *st;
   double x, y;

   if (!eldbus_message_iter_arguments_get(variant, "(dd)", &st))
     {
	ERR("invalid param for crop_set.");
	return 0;
     }

   if (!eldbus_message_iter_arguments_get(st, "dd", &x, &y)) return 0;
   DBG("setting crop to: %3.2f,%3.2f", x, y);
   request->setup.flags.crop = 1;
   request->setup.cx = x;
   request->setup.cy = y;

   return 1;
}

static int
_ethumb_dbus_quality_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   int quality;

   if (!eldbus_message_iter_arguments_get(variant, "i", &quality))
     {
	ERR("invalid param for quality_set.");
	return 0;
     }

   DBG("setting quality to: %d", quality);
   request->setup.flags.quality = 1;
   request->setup.quality = quality;

   return 1;
}


static int
_ethumb_dbus_compress_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   int compress;

   if (!eldbus_message_iter_arguments_get(variant, "i", &compress))
     {
	ERR("invalid param for compress_set.");
	return 0;
     }

   DBG("setting compress to: %d", compress);
   request->setup.flags.compress = 1;
   request->setup.compress = compress;

   return 1;
}

static int
_ethumb_dbus_frame_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   Eldbus_Message_Iter *_struct, *file_iter, *group_iter, *swallow_iter;
   const char *file, *group, *swallow;

   if (!eldbus_message_iter_arguments_get(variant, "(ayayay)", &_struct))
     {
	ERR("invalid param for frame_set.");
	return 0;
     }

   if (!eldbus_message_iter_arguments_get(_struct, "ayayay", &file_iter, &group_iter, &swallow_iter))
     return 0;

   file = _ethumb_dbus_get_bytearray(file_iter);
   group = _ethumb_dbus_get_bytearray(group_iter);
   swallow = _ethumb_dbus_get_bytearray(swallow_iter);
   DBG("setting frame to \"%s:%s:%s\"", file, group, swallow);
   request->setup.flags.frame = 1;
   request->setup.theme_file = file;
   request->setup.group = group;
   request->setup.swallow = swallow;

   return 1;
}

static int
_ethumb_dbus_directory_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   const char *directory;
   Eldbus_Message_Iter *array;

   if (!eldbus_message_iter_arguments_get(variant, "ay", &array))
     {
	ERR("invalid param for dir_path_set.");
	return 0;
     }

   directory = _ethumb_dbus_get_bytearray(array);
   DBG("setting directory to: %s", directory);
   request->setup.flags.directory = 1;
   request->setup.directory = directory;

   return 1;
}

static int
_ethumb_dbus_category_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   const char *category;
   Eldbus_Message_Iter *array;

   if (!eldbus_message_iter_arguments_get(variant, "ay", &array))
     {
	ERR("invalid param for category.");
	return 0;
     }

   category = _ethumb_dbus_get_bytearray(array);
   DBG("setting category to: %s", category);
   request->setup.flags.category = 1;
   request->setup.category = category;

   return 1;
}

static int
_ethumb_dbus_video_time_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   double video_time;

   if (!eldbus_message_iter_arguments_get(variant, "d", &video_time))
     {
	ERR("invalid param for video_time_set.");
	return 0;
     }

   DBG("setting video_time to: %3.2f", video_time);
   request->setup.flags.video_time = 1;
   request->setup.video_time = video_time;

   return 1;
}

static int
_ethumb_dbus_video_start_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   double video_start;

   if (!eldbus_message_iter_arguments_get(variant, "d", &video_start))
     {
	ERR("invalid param for video_start_set.");
	return 0;
     }

   DBG("setting video_start to: %3.2f", video_start);
   request->setup.flags.video_start = 1;
   request->setup.video_start = video_start;

   return 1;
}

static int
_ethumb_dbus_video_interval_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   double video_interval;

   if (!eldbus_message_iter_arguments_get(variant, "d", &video_interval))
     {
        ERR("invalid param for video_interval_set.");
        return 0;
     }
   DBG("setting video_interval to: %3.2f", video_interval);
   request->setup.flags.video_interval = 1;
   request->setup.video_interval = video_interval;

   return 1;
}

static int
_ethumb_dbus_video_ntimes_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   unsigned int video_ntimes;

   if (!eldbus_message_iter_arguments_get(variant, "u", &video_ntimes))
     {
	ERR("invalid param for video_ntimes_set.");
	return 0;
     }

   DBG("setting video_ntimes to: %3.2d", video_ntimes);
   request->setup.flags.video_ntimes = 1;
   request->setup.video_ntimes = video_ntimes;

   return 1;
}

static int
_ethumb_dbus_video_fps_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   unsigned int video_fps;

   if (!eldbus_message_iter_arguments_get(variant, "u", &video_fps))
     {
	ERR("invalid param for video_fps_set.");
	return 0;
     }

   DBG("setting video_fps to: %3.2d", video_fps);
   request->setup.flags.video_fps = 1;
   request->setup.video_fps = video_fps;

   return 1;
}

static int
_ethumb_dbus_document_page_set(Ethumbd_Object *eobject EINA_UNUSED, Eldbus_Message_Iter *variant, Ethumbd_Request *request)
{
   unsigned int document_page;

   if (!eldbus_message_iter_arguments_get(variant, "u", &document_page))
     {
	ERR("invalid param for document_page_set.");
	return 0;
     }

   DBG("setting document_page to: %d", document_page);
   request->setup.flags.document_page = 1;
   request->setup.document_page = document_page;

   return 1;
}

static struct
{
   const char *option;
   int (*setup_func)(Ethumbd_Object *eobject, Eldbus_Message_Iter *variant, Ethumbd_Request *request);
} _option_cbs[] = {
  {"fdo", _ethumb_dbus_fdo_set},
  {"size", _ethumb_dbus_size_set},
  {"format", _ethumb_dbus_format_set},
  {"aspect", _ethumb_dbus_aspect_set},
  {"orientation", _ethumb_dbus_orientation_set},
  {"crop", _ethumb_dbus_crop_set},
  {"quality", _ethumb_dbus_quality_set},
  {"compress", _ethumb_dbus_compress_set},
  {"frame", _ethumb_dbus_frame_set},
  {"directory", _ethumb_dbus_directory_set},
  {"category", _ethumb_dbus_category_set},
  {"video_time", _ethumb_dbus_video_time_set},
  {"video_start", _ethumb_dbus_video_start_set},
  {"video_interval", _ethumb_dbus_video_interval_set},
  {"video_ntimes", _ethumb_dbus_video_ntimes_set},
  {"video_fps", _ethumb_dbus_video_fps_set},
  {"document_page", _ethumb_dbus_document_page_set},
  {NULL, NULL}
};

/**
 * @brief Parses a single element from the setup options array.
 *
 * The setup options are passed as an array of dictionary entries ('a{sv}').
 * This function parses one entry, which consists of a string key (the option
 * name) and a variant value. It looks up the option name and calls the
 * corresponding `_ethumb_dbus_*_set` function to handle the value.
 *
 * @param eobject The client object.
 * @param data An iterator for a dictionary entry ('e') containing 'sv'.
 * @param request The request object to populate.
 * @return 1 on success, 0 on failure.
 */
static int
_ethumb_dbus_ethumb_setup_parse_element(Ethumbd_Object *eobject, Eldbus_Message_Iter *data, Ethumbd_Request *request)
{
   Eldbus_Message_Iter *variant;
   const char *option;
   int i, r;

   if (!eldbus_message_iter_arguments_get(data, "sv", &option, &variant))
     return 0;

   r = 0;
   for (i = 0; _option_cbs[i].option; i++)
     if (!strcmp(_option_cbs[i].option, option))
       {
	  r = 1;
	  break;
       }

   if (!r)
     {
	ERR("ethumb_setup invalid option: %s", option);
	return 0;
     }

   return _option_cbs[i].setup_func(eobject, variant, request);
}

/**
 * @brief D-Bus method handler for setting up thumbnail generation options.
 *
 * Clients call this method with an array of key-value pairs to configure
 * how subsequent thumbnails should be generated (e.g., size, quality, etc.).
 * This function creates a special "setup" request and adds it to the queue.
 *
 * The array is of type 'a{sv}', an array of dictionary entries where 's' is
 * the option name and 'v' is the variant value.
 * Example structure:
 * [
 *   {"size", (128, 128)},
 *   {"quality", 80}
 * ]
 *
 * @param iface The D-Bus service interface for the client object.
 * @param msg The incoming D-Bus message.
 * @return A reply message with a boolean indicating success.
 */
static Eldbus_Message *
_ethumb_dbus_ethumb_setup_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Eldbus_Message *reply;
   Ethumbd_Object_Data *odata;
   Ethumbd *ed;
   Ethumbd_Object *eobject;
   Ethumbd_Request *request;
   Eina_Bool r = EINA_FALSE;
   Eldbus_Message_Iter *array;
   Eldbus_Message_Iter *data;

   if (!eldbus_message_arguments_get(msg, "a{sv}", &array))
     {
        ERR("could not get dbus_object data for setup_cb.");
        goto end;
     }

   odata = eldbus_service_object_data_get(iface, ODATA);
   if (!odata)
     {
	ERR("could not get dbus_object data for setup_cb.");
	goto end;
     }

   ed = odata->ed;
   eobject = &ed->queue.table[odata->idx];

   request = calloc(1, sizeof(*request));
   request->id = -1;

   r = EINA_TRUE;
   while (eldbus_message_iter_get_and_next(array, 'e', &data) && r)
     {
        if (!_ethumb_dbus_ethumb_setup_parse_element(eobject, data, request))
          r = EINA_FALSE;
     }

   eobject->queue = eina_list_append(eobject->queue, request);
   eobject->nqueue++;
   ed->queue.nqueue++;

end:
   reply = eldbus_message_method_return_new(msg);
   eldbus_message_arguments_append(reply, "b", r);
   return reply;
}

/**
 * @brief Sends the "generated" D-Bus signal to a client.
 *
 * This signal informs the client that a thumbnail request has been completed.
 * It includes the original request ID, the path and key of the generated
 * thumbnail, and a success flag.
 *
 * @param ed The main daemon context.
 * @param id The ID of the completed request.
 * @param thumb_path The path to the generated thumbnail file (or NULL on failure).
 * @param thumb_key The key for the thumbnail (or NULL on failure).
 * @param success EINA_TRUE if generation was successful.
 */
static void
_ethumb_dbus_generated_signal(Ethumbd *ed, int *id, const char *thumb_path, const char *thumb_key, Eina_Bool success)
{
   Eldbus_Message *sig;
   Eldbus_Service_Interface *iface;
   Eldbus_Message_Iter *iter, *iter_path, *iter_key;
   int id32;

   id32 = *id;

   iface = ed->queue.table[ed->queue.current].iface;
   sig = eldbus_service_signal_new(iface, ETHUMB_DBUS_OBJECTS_SIGNAL_GENERATED);

   iter = eldbus_message_iter_get(sig);
   eldbus_message_iter_arguments_append(iter, "iay", id32, &iter_path);
   _ethumb_dbus_append_bytearray(iter, iter_path, thumb_path);
   eldbus_message_iter_arguments_append(iter, "ay", &iter_key);
   _ethumb_dbus_append_bytearray(iter, iter_key, thumb_key);
   eldbus_message_iter_arguments_append(iter, "b", success);

   eldbus_service_signal_send(iface, sig);
}

static const Eldbus_Service_Interface_Desc server_desc = {
   _ethumb_dbus_interface, _ethumb_dbus_methods, NULL, NULL, NULL, NULL
};

/**
 * @brief Callback for the D-Bus name request.
 *
 * This function is called after attempting to acquire the service name
 * "org.enlightenment.Ethumb". If successful, it proceeds to register the main
 * D-Bus service object at "/org/enlightenment/Ethumb".
 *
 * @param data The Ethumbd context.
 * @param msg The reply message from the D-Bus daemon.
 * @param pending The pending call object (unused).
 */
static void
_ethumb_dbus_request_name_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
   Eldbus_Service_Interface *iface;
   const char *errname, *errmsg;
   Ethumbd *ed = data;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("request name error: %s %s", errname, errmsg);
        eldbus_connection_unref(ed->conn);
        return;
     }

   iface = eldbus_service_interface_register(ed->conn, _ethumb_dbus_path,
                                            &server_desc);
   EINA_SAFETY_ON_NULL_RETURN(iface);

   eldbus_service_object_data_set(iface, DAEMON, ed);

   _ethumbd_timeout_redo(ed);
}

/**
 * @brief Initializes the D-Bus service.
 *
 * This function starts the D-Bus integration by requesting the well-known
 * service name on the session bus.
 *
 * @param ed The main daemon context.
 * @return 1 on success.
 */
static int
_ethumb_dbus_setup(Ethumbd *ed)
{
   eldbus_name_request(ed->conn, _ethumb_dbus_bus_name, 0,
                      _ethumb_dbus_request_name_cb, ed);
   return 1;
}

/**
 * @brief Shuts down the D-Bus service.
 *
 * This function cleans up all D-Bus related resources, including unregistering
 * all client objects and closing the connection.
 *
 * @param ed The main daemon context.
 */
static void
_ethumb_dbus_finish(Ethumbd *ed)
{
   _process_queue_stop(ed);
   _ethumb_table_clear(ed);
   eldbus_connection_unref(ed->conn);
   free(ed->queue.table);
   free(ed->queue.list);
}

/**
 * @brief Spawns the ethumbd_slave helper process.
 *
 * This function locates and executes the slave process, setting up pipes
 * for inter-process communication. The slave process does the actual
 * thumbnail generation work in a separate address space to isolate the
 * main daemon from potential crashes in image loading libraries.
 *
 * @param slave The slave context to initialize.
 * @param ed The main daemon context, passed as user data to the Ecore_Exe.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_ethumbd_slave_spawn(Ethumbd_Slave *slave, Ethumbd *ed)
{
   char buf[PATH_MAX];

   slave->bufcmd = NULL;
   slave->scmd = 0;

   snprintf(buf, sizeof(buf),
            "%s/ethumb_client/utils/"MODULE_ARCH"/ethumbd_slave",
            eina_prefix_lib_get(_pfx));

   slave->exe = ecore_exe_pipe_run(buf,
      ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_WRITE, ed);
   if (!slave->exe)
     {
	ERR("could not create slave.");
	return 0;
     }

   return 1;
}

/**
 * @brief Main function of the ethumbd daemon.
 *
 * Initializes libraries, parses command line options, sets up the D-Bus
 * connection, spawns the slave process, and enters the Ecore main loop.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Exit code.
 */
int
main(int argc, char *argv[])
{
   Eina_Bool quit_option = 0;
   int exit_value = 0;
   int arg_idx;
   Ethumbd ed;
   int child;
   double timeout = 30.0;

#ifdef HAVE_SYS_RESOURCE_H
   setpriority(PRIO_PROCESS, 0, 19);
#elif _WIN32
   SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
#endif

   memset(&ed, 0, sizeof(ed));
   ecore_init();
   ethumb_init();

   if (_log_domain < 0)
     {
	_log_domain = eina_log_domain_register("ethumbd", NULL);
	if (_log_domain < 0)
	  {
	     EINA_LOG_CRIT("could not register log domain 'ethumbd'");
	     exit_value = -8;
	     goto finish;
	  }
     }

   _pfx = eina_prefix_new(argv[0], ethumb_client_init,
                          "ETHUMB_CLIENT", "ethumb_client", "checkme",
                          PACKAGE_BIN_DIR, PACKAGE_LIB_DIR,
                          PACKAGE_DATA_DIR, PACKAGE_DATA_DIR);
   if (!_pfx)
     {
        ERR("Could not get ethumb_client installation prefix.");
        exit_value = -7;
        goto finish;
     }

   ed.data_cb = ecore_event_handler_add(ECORE_EXE_EVENT_DATA,
					_ethumbd_slave_data_read_cb, &ed);
   ed.del_cb = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
				       _ethumbd_slave_del_cb, &ed);

   child = _ethumbd_slave_spawn(&ed.slave, &ed);
   if (!child)
     {
	exit_value = -6;
	goto finish;
     }

   if (child == 2)
     {
	exit_value = 0;
	goto finish;
     }

   if (!eldbus_init())
     {
	ERR("could not init eldbus.");
	exit_value = -1;
	goto finish;
     }

   Ecore_Getopt_Value values[] = {
     ECORE_GETOPT_VALUE_DOUBLE(timeout),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_NONE
   };

   arg_idx = ecore_getopt_parse(&optdesc, values, argc, argv);
   if (arg_idx < 0)
     {
	ERR("Could not parse arguments.");
	exit_value = -2;
	goto finish;
     }

   if (quit_option)
     goto finish;

   ed.conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   if (!ed.conn)
     {
	ERR("could not connect to session bus.");
	exit_value = -3;
	goto finish_eldbus;
     }

   ed.timeout = timeout;

   if (!_ethumb_dbus_setup(&ed))
     {
	eldbus_connection_unref(ed.conn);
	ERR("could not setup dbus connection.");
	exit_value = -5;
	goto finish_eldbus;
     }

   ecore_main_loop_begin();
   _ethumb_dbus_finish(&ed);

 finish_eldbus:
   if (_log_domain >= 0)
     {
	eina_log_domain_unregister(_log_domain);
	_log_domain = -1;
     }

   eldbus_shutdown();
 finish:
   if (ed.slave.exe)
     ecore_exe_quit(ed.slave.exe);

   if (_pfx) eina_prefix_free(_pfx);
   ethumb_shutdown();
   ecore_shutdown();
   return exit_value;
}
