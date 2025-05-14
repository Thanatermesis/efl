/**
 * @file
 *
 * This is the client-server thumbnail library, see @ref
 * tutorial_ethumb_client.
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
 * @author Gustavo Sverzut Barbieri <barbieri@profusion.mobi>
 */

/**
 * @page tutorial_ethumb_client Client-Server Thumbnailing Tutorial
 *
 * @section tutorial_ethumb_client_intro Introduction
 *
 * Ethumb provides both in process and client-server generation
 * methods. The advantage of the client-server method is that current
 * process will not do the heavy operations that may block, stopping
 * animations and other user interactions. Instead the client library
 * will configure a local #Ethumb instance and mirrors/controls a
 * remote process using DBus. The simple operations like most setters
 * and getters as well as checking for thumbnail existence
 * (ethumb_client_thumb_exists()) is done locally, while expensive
 * (ethumb_client_generate()) are done on server and then reported
 * back to application when it is finished (both success or failure).
 *
 * @section tutorial_ethumb_client_connect Connecting to Server
 *
 * TODO
 *
 * @section tutorial_ethumb_client_generate Requesting Thumbnail Generation
 *
 * TODO
 *
 * @section tutorial_ethumb_client_setup Setup Extra Thumbnail Parameters
 *
 * TODO
 *
 * @section tutorial_ethumb_client_server_died Handle Server Disconnection
 *
 * TODO
 */

/**
 * @cond LOCAL
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <stdbool.h>

#include <Eina.h>
#include <eina_safety_checks.h>
#include <Eldbus.h>
#include <Ethumb.h>
#include <Ecore.h>

#include "Ethumb_Client.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_ID   2000000

static int _log_dom = -1;
#define DBG(...)      EINA_LOG_DOM_DBG(_log_dom, __VA_ARGS__)
#define INF(...)      EINA_LOG_DOM_INFO(_log_dom, __VA_ARGS__)
#define WRN(...)      EINA_LOG_DOM_WARN(_log_dom, __VA_ARGS__)
#define ERR(...)      EINA_LOG_DOM_ERR(_log_dom, __VA_ARGS__)
#define CRI(...)      EINA_LOG_DOM_CRIT(_log_dom, __VA_ARGS__)

/**
 * @brief Represents an Ethumb client instance.
 * This structure holds all the necessary information for a client
 * to communicate with the Ethumb server.
 */
struct _Ethumb_Client
{
   Ethumb                *ethumb; /**< Local Ethumb instance for configuration. */
   int                    id_count; /**< Counter for generating unique request IDs. */
   Ethumb                *old_ethumb_conf; /**< Stores the previous Ethumb configuration to detect changes. */
   Eldbus_Connection      *conn; /**< Eldbus connection to the D-Bus session bus. */
   struct
   {
      Ethumb_Client_Connect_Cb cb; /**< Callback function for connection status. */
      void                    *data; /**< User data for the connection callback. */
      Eina_Free_Cb             free_data; /**< Function to free user data for connection callback. */
   } connect; /**< Connection related callbacks and data. */
   Eina_List             *pending_add; /**< List of pending thumbnail generation requests (_ethumb_pending_add). */
   Eina_List             *pending_remove; /**< List of pending thumbnail cancellation requests (_ethumb_pending_remove). */
   Eina_List             *pending_gen; /**< List of generation requests sent to the server, awaiting 'generated' signal (_ethumb_pending_gen). */
   Eina_List             *dbus_pending; /**< List of pending D-Bus calls (Eldbus_Pending). */
   struct
   {
      Ethumb_Client_Die_Cb cb; /**< Callback function for server disconnection. */
      void                *data; /**< User data for the server disconnection callback. */
      Eina_Free_Cb         free_data; /**< Function to free user data for server disconnection callback. */
   } die; /**< Server disconnection related callbacks and data. */
   Eldbus_Proxy           *proxy; /**< Eldbus proxy for the remote Ethumb object. */
   Eldbus_Signal_Handler  *generated_sig_handler; /**< Eldbus signal handler for 'generated' signal. */
   EINA_REFCOUNT; /**< Reference count for managing the lifecycle of the client object. */
   Eina_Bool              connected : 1; /**< Flag indicating if the client is connected to the server. */
   Eina_Bool              server_started : 1; /**< Flag indicating if the server was started by this client. (Currently unused) */
   Eina_Bool              invalid : 1; /**< Flag indicating if the client object is in an invalid state (e.g., during freeing). */
};

/**
 * @brief Represents a pending request to add a thumbnail generation task to the server's queue.
 * This structure holds information about a thumbnail generation request before it's fully processed
 * by the server and moved to the `pending_gen` list.
 */
struct _ethumb_pending_add
{
   int32_t                   id; /**< Unique ID for this generation request. */
   const char               *file; /**< Path to the original file. (stringshared) */
   const char               *key; /**< Optional key within the file (e.g., for EET files). (stringshared) */
   const char               *thumb; /**< Path where the thumbnail will be stored. (stringshared) */
   const char               *thumb_key; /**< Optional key for the thumbnail file. (stringshared) */
   Ethumb_Client_Generate_Cb generated_cb; /**< Callback for when generation is complete. */
   void                     *data; /**< User data for the `generated_cb`. */
   Eina_Free_Cb              free_data; /**< Function to free `data`. */
   Eldbus_Pending           *pending_call; /**< The D-Bus pending call for the `queue_add` method. */
   Ethumb_Client            *client; /**< Pointer back to the client instance. */
};

/**
 * @brief Represents a pending request to remove/cancel a thumbnail generation task.
 */
struct _ethumb_pending_remove
{
   int32_t                          id; /**< ID of the generation request to cancel. */
   Ethumb_Client_Generate_Cancel_Cb cancel_cb; /**< Callback for when cancellation is complete. */
   void                            *data; /**< User data for the `cancel_cb`. */
   Eina_Free_Cb                     free_data; /**< Function to free `data`. */
   Eldbus_Pending                  *pending_call; /**< The D-Bus pending call for the `queue_remove` method. */
   Ethumb_Client                   *client; /**< Pointer back to the client instance. */
};

/**
 * @brief Represents a thumbnail generation task that has been sent to the server and is awaiting the 'generated' signal.
 * Once the `queue_add` D-Bus call returns successfully, the corresponding `_ethumb_pending_add`
 * is converted into this structure and added to the `client->pending_gen` list.
 */
struct _ethumb_pending_gen
{
   int32_t                   id; /**< Unique ID for this generation request, matches the one in `_ethumb_pending_add`. */
   const char               *file; /**< Path to the original file. (stringshared) */
   const char               *key; /**< Optional key within the file. (stringshared) */
   const char               *thumb; /**< Path where the thumbnail will be stored. (stringshared) */
   const char               *thumb_key; /**< Optional key for the thumbnail file. (stringshared) */
   Ethumb_Client_Generate_Cb generated_cb; /**< Callback for when generation is complete. */
   void                     *data; /**< User data for the `generated_cb`. */
   Eina_Free_Cb              free_data; /**< Function to free `data`. */
};

/**
 * @brief Forward declaration for Ethumb_Async_Exists.
 */
typedef struct _Ethumb_Async_Exists Ethumb_Async_Exists;

/**
 * @brief Represents an asynchronous request to check if a thumbnail exists.
 * This structure manages the state of an asynchronous thumbnail existence check,
 * which is performed in a separate thread.
 */
struct _Ethumb_Async_Exists
{
   const char   *path; /**< Path of the file for which thumbnail existence is checked. (stringshared) */

   Ethumb       *dup; /**< A duplicate of the Ethumb configuration at the time of the request.
                           This is used by the worker thread to prevent race conditions and locking issues
                           with the main client's Ethumb instance. */

   Eina_List    *callbacks; /**< List of `_Ethumb_Exists` structures (callbacks) associated with this async request. */
   Ecore_Thread *thread; /**< The worker thread performing the existence check. */
};

/**
 * @brief Represents a specific callback and its context for an asynchronous thumbnail existence check.
 * Multiple `_Ethumb_Exists` can be associated with a single `_Ethumb_Async_Exists` if multiple
 * requests for the same file are made concurrently.
 */
struct _Ethumb_Exists
{
   Ethumb_Async_Exists          *parent; /**< Pointer to the parent asynchronous request. */
   Ethumb_Client                *client; /**< The client instance that initiated this request. */
   Ethumb                       *dup; /**< A duplicate of the client's Ethumb settings at the time of the request.
                                           This ensures that parameters used for the existence check are consistent
                                           with what the caller expected, even if the main client's Ethumb
                                           settings change later. */

   Ethumb_Client_Thumb_Exists_Cb exists_cb; /**< Callback function to be invoked when the existence check is complete. */
   const void                   *data; /**< User data for the `exists_cb`. */
};

/** @brief D-Bus service name for Ethumb. */
static const char _ethumb_dbus_bus_name[] = "org.enlightenment.Ethumb";
/** @brief Main D-Bus interface for Ethumb service. */
static const char _ethumb_dbus_interface[] = "org.enlightenment.Ethumb";
/** @brief D-Bus interface for individual Ethumb objects created by the server. */
static const char _ethumb_dbus_objects_interface[] = "org.enlightenment.Ethumb.objects";
/** @brief D-Bus object path for the Ethumb service. */
static const char _ethumb_dbus_path[] = "/org/enlightenment/Ethumb";

/** @brief Initialization counter for the library. */
static int _initcount = 0;
/** @brief Hash table to store active asynchronous thumbnail existence requests (_Ethumb_Async_Exists).
 * The key is the file path (stringshared), and the value is the _Ethumb_Async_Exists structure.
 * This helps to coalesce multiple requests for the same file path into a single worker thread.
 */
static Eina_Hash *_exists_request = NULL;

/** @brief Callback for the "generated" D-Bus signal from the server. */
static void _ethumb_client_generated_cb(void *data, const Eldbus_Message *msg);
/** @brief Initiates the D-Bus call to the server's "new" method to create a new Ethumb object on the server. */
static void _ethumb_client_call_new(Ethumb_Client *client);
/** @brief Callback for D-Bus name owner changes, used to detect server connection and disconnection. */
static void _ethumb_client_name_owner_changed(void *context, const char *bus, const char *old_id, const char *new_id);

/**
 * @brief Frees an Ethumb_Client instance and all associated resources.
 * This function is typically called when the reference count of the client drops to zero.
 * It cancels pending D-Bus calls, frees pending request lists, and releases D-Bus resources.
 * @param client The Ethumb_Client instance to free.
 */
static void
_ethumb_client_free(Ethumb_Client *client)
{
   void *data;
   Eldbus_Object *obj;

   if (client->invalid)
      return;

   if (client->dbus_pending)
     {
        Eldbus_Pending *pending;
        EINA_LIST_FREE(client->dbus_pending, pending)
           eldbus_pending_cancel(pending);
     }

   client->invalid = EINA_TRUE;
   EINA_LIST_FREE(client->pending_add, data)
     {
        struct _ethumb_pending_add *pending = data;
        if (pending->pending_call)
          {
             Eldbus_Pending *call = pending->pending_call;

             pending->pending_call = NULL;
             pending->client = NULL;
             eldbus_pending_cancel(call);
          }
        else
          {
             pending->client = NULL;
             free(pending);
          }
     }

   EINA_LIST_FREE(client->pending_gen, data)
     {
        struct _ethumb_pending_gen *pending = data;
        eina_stringshare_del(pending->file);
        eina_stringshare_del(pending->key);
        eina_stringshare_del(pending->thumb);
        eina_stringshare_del(pending->thumb_key);
        if (pending->free_data)
          pending->free_data(pending->data);
        free(pending);
     }

   EINA_LIST_FREE(client->pending_remove, data)
     {
        struct _ethumb_pending_remove *pending = data;
        if (pending->free_data)
          pending->free_data(pending->data);
        if (pending->pending_call)
          {
             Eldbus_Pending *call = pending->pending_call;

             pending->pending_call = NULL;
             pending->client = NULL;
             eldbus_pending_cancel(call);
          }
        else
          {
             pending->client = NULL;
             free(pending);
          }
     }

   if (client->old_ethumb_conf)
     {
        ethumb_free(client->old_ethumb_conf);
        client->old_ethumb_conf = NULL;
     }

   if (client->ethumb)
     {
        ethumb_free(client->ethumb);
        client->ethumb = NULL;
     }

   if (client->conn)
     eldbus_name_owner_changed_callback_del(client->conn,
                                            _ethumb_dbus_bus_name,
                                            _ethumb_client_name_owner_changed,
                                            client);
   if (client->generated_sig_handler)
     {
        eldbus_signal_handler_del(client->generated_sig_handler);
        client->generated_sig_handler = NULL;
     }
   if (client->proxy)
     {
        obj = eldbus_proxy_object_get(client->proxy);
        eldbus_proxy_unref(client->proxy);
        client->proxy = NULL;
        if (obj) eldbus_object_unref(obj);
     }
   if (client->conn)
     {
        eldbus_connection_unref(client->conn);
        client->conn = NULL;
     }

   if (client->connect.free_data)
     client->connect.free_data(client->connect.data);
   if (client->die.free_data)
     client->die.free_data(client->die.data);

   free(client);
}

/**
 * @brief Frees an Ethumb_Async_Exists structure.
 * This function is used as a callback for eina_hash_free when removing entries
 * from the `_exists_request` hash. It ensures that associated resources like
 * the duplicated Ethumb instance and the path stringshare are released.
 * @param data Pointer to the Ethumb_Async_Exists structure to be freed.
 */
static void
_ethumb_async_delete(void *data)
{
   Ethumb_Async_Exists *async = data;

   EINA_SAFETY_ON_FALSE_RETURN(async->callbacks == NULL);
   EINA_SAFETY_ON_FALSE_RETURN(async->thread == NULL);

   ethumb_free(async->dup);
   eina_stringshare_del(async->path);

   free(async);
}

/**
 * @brief Handles D-Bus name owner changes for the Ethumb service.
 * This function is called by Eldbus when the owner of the Ethumb D-Bus name changes.
 * It's used to detect when the Ethumb server connects (new_id is non-empty) or
 * disconnects (new_id is empty).
 * @param context The Ethumb_Client instance.
 * @param bus The D-Bus name that changed owner (unused).
 * @param old_id The old owner of the D-Bus name.
 * @param new_id The new owner of the D-Bus name. If empty, the name has no owner (server disconnected).
 */
static void
_ethumb_client_name_owner_changed(void *context, const char *bus EINA_UNUSED, const char *old_id, const char *new_id)
{
   Ethumb_Client *client = context;

   DBG("NameOwnerChanged from=[%s] to=[%s]", old_id, new_id);
   if (new_id[0])
     {
        if (client->connected)
          return;

        client->connected = EINA_TRUE;
        INF("Server connected");
        _ethumb_client_call_new(client);
        return;
     }
   INF("Server disconnected");
   EINA_REFCOUNT_REF(client);
   client->connected = EINA_FALSE;
   if (client->die.cb)
     {
        client->die.cb(client->die.data, client);
        client->die.cb = NULL;
     }
   if (client->die.free_data)
     {
        client->die.free_data(client->die.data);
        client->die.free_data = NULL;
        client->die.data = NULL;
     }
   EINA_REFCOUNT_UNREF(client) _ethumb_client_free(client);
}

/**
 * @brief Reports the connection status to the user via the connection callback.
 * This function invokes the `connect.cb` callback provided by the user during
 * `ethumb_client_connect()`. It also handles freeing associated user data.
 * @param client The Ethumb_Client instance.
 * @param success EINA_TRUE if connection was successful, EINA_FALSE otherwise.
 */
static void
_ethumb_client_report_connect(Ethumb_Client *client, Eina_Bool success)
{
   if (!client->connect.cb)
     {
        //ERR("already called?!");
        return;
     }

   EINA_REFCOUNT_REF(client);
   if (success)
     INF("Success connecting to Ethumb server.");
   else
     ERR("Could not connect to Ethumb server.");

   client->connect.cb(client->connect.data, client, success);
   if (client->connect.free_data)
     {
        client->connect.free_data(client->connect.data);
        client->connect.free_data = NULL;
     }
   client->connect.cb = NULL;
   client->connect.data = NULL;
   EINA_REFCOUNT_UNREF(client) _ethumb_client_free(client);
}

/**
 * @brief Callback for the D-Bus "new" method call.
 * This function is invoked when the Ethumb server responds to the "new" method call,
 * which requests the creation of a new Ethumb object instance on the server.
 * On success, it retrieves the object path of the newly created server-side Ethumb object,
 * creates a proxy for it, and sets up a signal handler for the "generated" signal.
 * @param data The Ethumb_Client instance.
 * @param msg The D-Bus reply message.
 * @param pending The Eldbus_Pending object for this call.
 */
static void
_ethumb_client_new_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   const char *errname, *errmsg;
   const char *opath;
   Ethumb_Client *client = data;
   Eldbus_Object *obj;

   client->dbus_pending = eina_list_remove(client->dbus_pending, pending);
   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Error: %s %s", errname, errmsg);
        _ethumb_client_report_connect(client, 0);
        return;
     }

   if (!eldbus_message_arguments_get(msg, "o", &opath))
     {
        ERR("Error: could not get entry contents");
        _ethumb_client_report_connect(client, 0);
        return;
     }

   if (client->generated_sig_handler)
     {
        eldbus_signal_handler_del(client->generated_sig_handler);
        client->generated_sig_handler = NULL;
     }
   if (!client->proxy)
     {
        obj = eldbus_object_get(client->conn, _ethumb_dbus_bus_name, opath);
        client->proxy = eldbus_proxy_get(obj, _ethumb_dbus_objects_interface);
     }
   client->generated_sig_handler =
     eldbus_proxy_signal_handler_add(client->proxy, "generated",
                                     _ethumb_client_generated_cb, client);
   _ethumb_client_report_connect(client, 1);
}

/**
 * @brief Initiates a D-Bus method call to "new" on the Ethumb server.
 * This function sends a request to the Ethumb server to create a new
 * server-side Ethumb object instance that this client will interact with.
 * @param client The Ethumb_Client instance.
 */
static void
_ethumb_client_call_new(Ethumb_Client *client)
{
   Eldbus_Message *msg;
   Eldbus_Pending *pending;
   msg = eldbus_message_method_call_new(_ethumb_dbus_bus_name,
                                       _ethumb_dbus_path,
                                       _ethumb_dbus_interface, "new");
   pending = eldbus_connection_send(client->conn, msg,
                                    _ethumb_client_new_cb, client, -1);
   if (pending)
     client->dbus_pending = eina_list_append(client->dbus_pending, pending);
}

/**
 * @brief Performs the potentially blocking part of the thumbnail existence check.
 * This function is executed in a separate Ecore_Thread. It calls `ethumb_thumb_hash()`
 * on a duplicated Ethumb instance to calculate the thumbnail path and check for
 * its existence without blocking the main loop.
 * @param data Pointer to the Ethumb_Async_Exists structure.
 * @param thread The Ecore_Thread executing this function (unused).
 */
static void
_ethumb_client_exists_heavy(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Ethumb_Async_Exists *async = data;

   ethumb_thumb_hash(async->dup);
}

/**
 * @brief Handles the completion of the asynchronous thumbnail existence check.
 * This function is called in the main loop when the Ecore_Thread finishes
 * `_ethumb_client_exists_heavy`. It iterates through all registered callbacks
 * for this specific file path and invokes them with the result of the existence check.
 * Finally, it cleans up the Ethumb_Async_Exists structure from the `_exists_request` hash.
 * @param data Pointer to the Ethumb_Async_Exists structure.
 * @param thread The Ecore_Thread that finished (unused).
 */
static void
_ethumb_client_exists_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Ethumb_Async_Exists *async = data;
   Ethumb_Exists *cb;

   EINA_LIST_FREE(async->callbacks, cb)
     {
        Ethumb *tmp;

        ethumb_thumb_hash_copy(cb->dup, async->dup);
        tmp = cb->client->ethumb;
        cb->client->ethumb = cb->dup;

        cb->exists_cb((void *)cb->data,
                      cb->client, cb,
                      ethumb_exists(cb->client->ethumb));

        cb->client->ethumb = tmp;
        EINA_REFCOUNT_UNREF(cb->client) _ethumb_client_free(cb->client);
        ethumb_free(cb->dup);
        free(cb);
     }

   async->thread = NULL;

   eina_hash_del(_exists_request, async->path, async);
}

/**
 * @endcond
 */

/**
 * @brief Initialize the Ethumb_Client library.
 *
 * @return 1 or greater on success, 0 on error.
 *
 * This function sets up all the Ethumb_Client module dependencies. It
 * returns 0 on failure (that is, when one of the dependency fails to
 * initialize), otherwise it returns the number of times it has
 * already been called.
 *
 * When Ethumb_Client is not used anymore, call
 * ethumb_client_shutdown() to shut down the Ethumb_Client library.
 *
 * @see ethumb_client_shutdown()
 * @see ethumb_client_connect()
 * @see @ref tutorial_ethumb_client
 */
EAPI int
ethumb_client_init(void)
{
   if (_initcount)
     return ++_initcount;

   if (!eina_init())
     {
        fprintf(stderr, "ERROR: Could not initialize log module.\n");
        return 0;
     }
   _log_dom = eina_log_domain_register("ethumb_client", EINA_COLOR_YELLOW);
   if (_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: ethumb_client");
        eina_shutdown();
        return 0;
     }

   ethumb_init();
   eldbus_init();

   _exists_request = eina_hash_stringshared_new(_ethumb_async_delete);

   return ++_initcount;
}

/**
 * @brief Shut down the Ethumb_Client library.
 *
 * @return 0 when everything is shut down, 1 or greater if there are
 *         other users of the Ethumb_Client library pending shutdown.
 *
 * This function shuts down the Ethumb_Client library. It returns 0
 * when it has been called the same number of times than
 * ethumb_client_init(). In that case it shut down all the
 * Ethumb_Client modules dependencies.
 *
 * Once this function succeeds (that is, @c 0 is returned), you must
 * not call any of the Eina function anymore. You must call
 * ethumb_client_init() again to use the Ethumb_Client functions
 * again.
 */
EAPI int
ethumb_client_shutdown(void)
{
   _initcount--;
   if (_initcount > 0)
     return _initcount;

   /* should find a non racy solution to closing all pending exists request */
   eina_hash_free(_exists_request);
   _exists_request = NULL;

   eldbus_shutdown();
   ethumb_shutdown();
   eina_log_domain_unregister(_log_dom);
   _log_dom = -1;
   eina_shutdown();
   return _initcount;
}

/**
 * @brief Callback for the eldbus_name_start D-Bus call.
 * This function is invoked when the attempt to start the Ethumb D-Bus service
 * (if it wasn't already running) completes. It logs any errors encountered
 * during the service startup.
 * @param data User data (unused).
 * @param msg The D-Bus reply message.
 * @param pending The Eldbus_Pending object for this call (unused).
 */
static void
_name_start(void *data EINA_UNUSED, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
   const char *name, *text;
   if (eldbus_message_error_get(msg, &name, &text))
     {
        ERR("Starting ethumb failed %s %s", name, text);
        return;
     }
}

/**
 * Connects to Ethumb server and return the client instance.
 *
 * This is the "constructor" of Ethumb_Client, where everything
 * starts.
 *
 * If server was down, it is tried to start it using DBus activation,
 * then the connection is retried.
 *
 * This call is asynchronous and will not block, instead it will be in
 * "not connected" state until @a connect_cb is called with either
 * success or failure. On failure, then no methods should be
 * called. On success you're now able to setup and then ask generation
 * of thumbnails.
 *
 * Usually you should listen for server death/disconenction with
 * ethumb_client_on_server_die_callback_set().
 *
 * @param connect_cb function to call to report connection success or
 *        failure. Do not call any other ethumb_client method until
 *        this function returns. The first received parameter is the
 *        given argument @a data. Must @b not be @c NULL. This
 *        function will not be called if user explicitly calls
 *        ethumb_client_disconnect().
 * @param data context to give back to @a connect_cb. May be @c NULL.
 * @param free_data function used to release @a data resources, if
 *        any. May be @c NULL. If this function exists, it will be
 *        called immediately after @a connect_cb is called or if user
 *        explicitly calls ethumb_client_disconnect() before such
 *        (that is, don't rely on @a data after @a connect_cb was
 *        called!)
 *
 * @return client instance or NULL if failed. If @a connect_cb is
 *         missing it returns @c NULL. If it fail for other
 *         conditions, @c NULL is also returned and @a connect_cb is
 *         called with @c success=EINA_FALSE. The client instance is
 *         not ready to be used until @a connect_cb is called.
 */
EAPI Ethumb_Client *
ethumb_client_connect(Ethumb_Client_Connect_Cb connect_cb, const void *data, Eina_Free_Cb free_data)
{
   Ethumb_Client *eclient;

   EINA_SAFETY_ON_NULL_RETURN_VAL(connect_cb, NULL);

   eclient = calloc(1, sizeof(*eclient));
   if (!eclient)
     {
        ERR("could not allocate Ethumb_Client structure.");
        goto err;
     }

   eclient->old_ethumb_conf = NULL;
   eclient->connect.cb = connect_cb;
   eclient->connect.data = (void *)data;
   eclient->connect.free_data = free_data;

   eclient->ethumb = ethumb_new();
   if (!eclient->ethumb)
     {
        ERR("could not create ethumb handler.");
        goto ethumb_new_err;
     }

   eclient->conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   if (!eclient->conn)
     {
        ERR("could not connect to session bus.");
        goto connection_err;
     }

   if (!eldbus_name_start(eclient->conn, _ethumb_dbus_bus_name, 0, _name_start, NULL))
     {
        ERR("Failed to start ethumb bus");
        goto connection_err;
     }

   eldbus_name_owner_changed_callback_add(eclient->conn, _ethumb_dbus_bus_name,
                                         _ethumb_client_name_owner_changed,
                                         eclient, EINA_TRUE);
   EINA_REFCOUNT_INIT(eclient);

   return eclient;

connection_err:
   ethumb_free(eclient->ethumb);
ethumb_new_err:
   free(eclient);
err:
   connect_cb((void *)data, NULL, EINA_FALSE);
   if (free_data)
     free_data((void *)data);
   return NULL;
}

/**
 * Disconnect the client, releasing all client resources.
 *
 * This is the destructor of Ethumb_Client, after it's disconnected
 * the client handle is now gone and should not be used.
 *
 * @param client client instance to be destroyed. Must @b not be @c
 *        NULL.
 */
EAPI void
ethumb_client_disconnect(Ethumb_Client *client)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   EINA_REFCOUNT_UNREF(client) _ethumb_client_free(client);
}

/**
 * Sets the callback to report server died.
 *
 * When server dies there is nothing you can do, just release
 * resources with ethumb_client_disconnect() and probably try to
 * connect again.
 *
 * Usually you should set this callback and handle this case, it does
 * happen!
 *
 * @param client the client instance to monitor. Must @b not be @c
 *        NULL.
 * @param server_die_cb function to call back when server dies. The
 *        first parameter will be the argument @a data. May be @c
 *        NULL.
 * @param data context to give back to @a server_die_cb. May be @c
 *        NULL.
 * @param free_data used to release @a data resources after @a
 *        server_die_cb is called or user calls
 *        ethumb_client_disconnect().
 */
EAPI void
ethumb_client_on_server_die_callback_set(Ethumb_Client *client, Ethumb_Client_Die_Cb server_die_cb, const void *data, Eina_Free_Cb free_data)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   if (client->die.free_data)
     client->die.free_data(client->die.data);

   client->die.cb = server_die_cb;
   client->die.data = (void *)data;
   client->die.free_data = free_data;
}

/**
 * @cond LOCAL
 */

static void
_ethumb_client_ethumb_setup_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   const char *errname, *errmsg;
   Eina_Bool result = 0;
   Ethumb_Client *client = data;

   client->dbus_pending = eina_list_remove(client->dbus_pending, pending);

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Error: %s %s", errname, errmsg);
        return;
     }

   if (!eldbus_message_arguments_get(msg, "b", &result))
     {
        ERR("Error getting arguments");
        return;
     }
   EINA_SAFETY_ON_FALSE_RETURN(result);
}

/**
 * @brief Reads a D-Bus byte array (ay) and converts it to a stringshared C string.
 * D-Bus strings are often sent as byte arrays. This helper function reads such an array
 * from a D-Bus message iterator and returns it as a null-terminated, stringshared C string.
 * @param array The Eldbus_Message_Iter positioned at the byte array.
 * @return A new eina_stringshare instance containing the string, or NULL on error.
 *         The caller is responsible for freeing the returned stringshare.
 */
static const char *
_ethumb_client_dbus_get_bytearray(Eldbus_Message_Iter *array)
{
   int length;
   const char *result;

   if (eldbus_message_iter_fixed_array_get(array, 'y', &result, &length))
     return eina_stringshare_add_length(result, length);
   else
     {
        ERR("Not byte array. Signature: %s",
            eldbus_message_iter_signature_get(array));
        return NULL;
     }
}

/**
 * @brief Appends a C string to a D-Bus message iterator as a byte array (ay).
 * This helper function takes a null-terminated C string and appends it to a D-Bus
 * message as a byte array, including the null terminator.
 * @param parent The parent Eldbus_Message_Iter to which the byte array container will be added.
 * @param string The C string to append. If NULL, an empty string (single null byte) is appended.
 */
static void
_ethumb_client_dbus_append_bytearray(Eldbus_Message_Iter *parent, const char *string)
{
   int i, size;
   Eldbus_Message_Iter *array;

   if (!string)
     string = "";

   array = eldbus_message_iter_container_new(parent, 'a', "y");
   size = strlen(string) + 1;
   for (i = 0; i < size; i++)
     eldbus_message_iter_basic_append(array, 'y', string[i]);
   eldbus_message_iter_container_close(parent, array);
}

/**
 * @endcond
 */

/**
 * @brief Opens a new dictionary entry (a{sv}) in a D-Bus message iterator for Ethumb setup.
 * This is a helper function for constructing the D-Bus message in `ethumb_client_ethumb_setup`.
 * It appends a struct containing a string key and a variant value.
 *
 * Example of D-Bus structure created:
 * @code
 * // For key "size", type "(ii)"
 * {
 *   "size", // string key
 *   variant ( // variant 'v'
 *     (int32, int32) // actual type, e.g., (128, 128)
 *   )
 * }
 * @endcode
 *
 * @param array The main D-Bus array iterator (a{sv}) to append to.
 * @param[out] entry Pointer to store the iterator for the created struct '{sv}'.
 * @param key The string key for the dictionary entry (e.g., "size", "format").
 * @param type The D-Bus signature string for the variant's content type (e.g., "(ii)", "i").
 * @return Eldbus_Message_Iter* Iterator for the variant 'v', ready for appending the actual value.
 */
static Eldbus_Message_Iter *
_setup_iterator_open(Eldbus_Message_Iter *array, Eldbus_Message_Iter **entry, const char *key, const char *type)
{
   Eldbus_Message_Iter *variant, *_struct;
   eldbus_message_iter_arguments_append(array, "{sv}", &_struct);
   eldbus_message_iter_basic_append(_struct, 's', key);
   variant = eldbus_message_iter_container_new(_struct, 'v', type);

   *entry = _struct;
   return variant;
}

/**
 * @brief Closes a dictionary entry opened by `_setup_iterator_open`.
 * This helper function closes the variant container and then the struct container
 * in the D-Bus message iterator.
 * @param array The main D-Bus array iterator (a{sv}) (unused in current impl, but good for context).
 * @param entry The iterator for the struct '{sv}' to be closed.
 * @param variant The iterator for the variant 'v' to be closed.
 */
static void
_setup_iterator_close(Eldbus_Message_Iter *array EINA_UNUSED, Eldbus_Message_Iter *entry, Eldbus_Message_Iter *variant)
{
   eldbus_message_iter_container_close(entry, variant);
   eldbus_message_iter_container_close(array, entry);
}

/**
 * Send setup to server.
 *
 * This method is called automatically by ethumb_client_generate() if
 * any property was changed. No need to call it manually.
 *
 * @param client client instance. Must @b not be @c NULL and client
 *        must be connected (after connected_cb is called).
 */
EAPI void
ethumb_client_ethumb_setup(Ethumb_Client *client)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *array, *main_iter;
   Eldbus_Message_Iter *entry, *variant;
   Eldbus_Message_Iter *sub_struct;
   Ethumb *e = client->ethumb;
   int tw, th, format, aspect, orientation, quality, compress;
   float cx, cy;
   const char *theme_file, *group, *swallow;
   const char *directory, *category;
   double video_time, video_start, video_interval;
   unsigned int video_ntimes, video_fps, document_page;
   Eldbus_Pending *pending;

   EINA_SAFETY_ON_NULL_RETURN(client);
   EINA_SAFETY_ON_FALSE_RETURN(client->connected);

   msg = eldbus_proxy_method_call_new(client->proxy, "ethumb_setup");
   main_iter = eldbus_message_iter_get(msg);
   eldbus_message_iter_arguments_append(main_iter, "a{sv}", &array);

   /* starting array elements */
   variant = _setup_iterator_open(array, &entry, "size", "(ii)");
   eldbus_message_iter_arguments_append(variant, "(ii)", &sub_struct);
   ethumb_thumb_size_get(e, &tw, &th);
   eldbus_message_iter_arguments_append(sub_struct, "ii", tw, th);
   eldbus_message_iter_container_close(variant, sub_struct);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "format", "i");
   format = ethumb_thumb_format_get(e);
   eldbus_message_iter_arguments_append(variant, "i", format);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "aspect", "i");
   aspect = ethumb_thumb_aspect_get(e);
   eldbus_message_iter_arguments_append(variant, "i", aspect);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "orientation", "i");
   orientation = ethumb_thumb_orientation_get(e);
   eldbus_message_iter_arguments_append(variant, "i", orientation);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "crop", "(dd)");
   eldbus_message_iter_arguments_append(variant, "(dd)", &sub_struct);
   ethumb_thumb_crop_align_get(e, &cx, &cy);
   eldbus_message_iter_arguments_append(sub_struct, "dd", (double)cx, (double)cy);
   eldbus_message_iter_container_close(variant, sub_struct);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "quality", "i");
   quality = ethumb_thumb_quality_get(e);
   eldbus_message_iter_arguments_append(variant, "i", quality);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "compress", "i");
   compress = ethumb_thumb_compress_get(e);
   eldbus_message_iter_arguments_append(variant, "i", compress);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "frame", "(ayayay)");
   eldbus_message_iter_arguments_append(variant, "(ayayay)", &sub_struct);
   ethumb_frame_get(e, &theme_file, &group, &swallow);
   _ethumb_client_dbus_append_bytearray(sub_struct, theme_file);
   _ethumb_client_dbus_append_bytearray(sub_struct, group);
   _ethumb_client_dbus_append_bytearray(sub_struct, swallow);
   eldbus_message_iter_container_close(variant, sub_struct);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "directory", "ay");
   directory = ethumb_thumb_dir_path_get(e);
   _ethumb_client_dbus_append_bytearray(variant, directory);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "category", "ay");
   category = ethumb_thumb_category_get(e);
   _ethumb_client_dbus_append_bytearray(variant, category);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "video_time", "d");
   video_time = ethumb_video_time_get(e);
   eldbus_message_iter_arguments_append(variant, "d", video_time);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "video_start", "d");
   video_start = ethumb_video_start_get(e);
   eldbus_message_iter_arguments_append(variant, "d", video_start);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "video_interval", "d");
   video_interval = ethumb_video_interval_get(e);
   eldbus_message_iter_arguments_append(variant, "d", video_interval);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "video_ntimes", "u");
   video_ntimes = ethumb_video_ntimes_get(e);
   eldbus_message_iter_arguments_append(variant, "u", video_ntimes);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "video_fps", "u");
   video_fps = ethumb_video_fps_get(e);
   eldbus_message_iter_arguments_append(variant, "u", video_fps);
   _setup_iterator_close(array, entry, variant);

   variant = _setup_iterator_open(array, &entry, "document_page", "u");
   document_page = ethumb_document_page_get(e);
   eldbus_message_iter_arguments_append(variant, "u", document_page);
   _setup_iterator_close(array, entry, variant);

   eldbus_message_iter_container_close(main_iter, array);

   pending = eldbus_proxy_send(client->proxy, msg,
                               _ethumb_client_ethumb_setup_cb, client, -1);
   if (pending)
     client->dbus_pending = eina_list_append(client->dbus_pending, pending);
}

/**
 * @cond LOCAL
 */
/**
 * @brief Callback for the "generated" D-Bus signal from the Ethumb server.
 * This function is invoked when the server emits the "generated" signal, indicating
 * that a thumbnail generation task has completed (either successfully or with failure).
 * It finds the corresponding pending generation request in `client->pending_gen`,
 * invokes the user's callback, and cleans up the request.
 *
 * The "generated" signal has the signature "iayayb":
 * - `i`: The ID of the completed thumbnail request.
 * - `ay`: The path to the generated thumbnail file (as a byte array).
 * - `ay`: The key for the generated thumbnail file (as a byte array).
 * - `b`: A boolean indicating success (EINA_TRUE) or failure (EINA_FALSE).
 *
 * @param data The Ethumb_Client instance.
 * @param msg The D-Bus signal message.
 */
static void
_ethumb_client_generated_cb(void *data, const Eldbus_Message *msg)
{
   int id = -1;
   Ethumb_Client *client = data;
   Eldbus_Message_Iter *thumb_iter;
   Eldbus_Message_Iter *thumb_key_iter;
   Eina_Bool success;
   int found;
   struct _ethumb_pending_gen *pending;
   Eina_List *l;

   if (!client) return;
   if (!eldbus_message_arguments_get(msg, "iayayb", &id, &thumb_iter,
                                    &thumb_key_iter, &success))
     {
        ERR("Error getting data from signal.");
        return;
     }

   found = 0;
   l = client->pending_gen;
   while (l)
     {
        pending = l->data;
        if (pending->id == id)
          {
             found = 1;
             break;
          }
        l = l->next;
     }

   if (found)
     {
        const char *thumb = _ethumb_client_dbus_get_bytearray(thumb_iter);
        const char *thumb_key = _ethumb_client_dbus_get_bytearray(thumb_key_iter);

        client->pending_gen = eina_list_remove_list(client->pending_gen, l);
        if (pending->generated_cb)
          pending->generated_cb(pending->data, client, id,
                                pending->file, pending->key,
                                thumb, thumb_key,
                                success);
        if (pending->free_data)
          pending->free_data(pending->data);
        eina_stringshare_del(pending->file);
        eina_stringshare_del(pending->key);
        eina_stringshare_del(pending->thumb);
        eina_stringshare_del(pending->thumb_key);
        eina_stringshare_del(thumb);
        eina_stringshare_del(thumb_key);
        free(pending);
     }
}

/**
 * @brief Callback for the "queue_add" D-Bus method call.
 * This function is invoked when the Ethumb server responds to the "queue_add" request.
 * On success, the server returns the ID assigned to this generation task. This function
 * then moves the request from `client->pending_add` to `client->pending_gen`,
 * awaiting the "generated" signal.
 *
 * The "queue_add" method returns an integer `i` (the ID).
 *
 * @param data Pointer to the struct _ethumb_pending_add for this request.
 * @param msg The D-Bus reply message.
 * @param eldbus_pending The Eldbus_Pending object for this call (unused).
 */
static void
_ethumb_client_queue_add_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *eldbus_pending EINA_UNUSED)
{
   const char *errname, *errmsg;
   int32_t id;
   struct _ethumb_pending_add *pending = data;
   struct _ethumb_pending_gen *generating;
   Ethumb_Client *client = pending->client;

   pending->pending_call = NULL;
   if (!client) goto end;
   client->pending_add = eina_list_remove(client->pending_add, pending);

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Error: %s %s", errname, errmsg);
        goto end;
     }

   if (!eldbus_message_arguments_get(msg, "i", &id))
     {
        ERR("Error getting arguments.");
        goto end;
     }


   generating = calloc(1, sizeof(*generating));
   generating->id = id;
   generating->file = pending->file;
   generating->key = pending->key;
   generating->thumb = pending->thumb;
   generating->thumb_key = pending->thumb_key;
   generating->generated_cb = pending->generated_cb;
   generating->data = pending->data;
   generating->free_data = pending->free_data;
   client->pending_gen = eina_list_append(client->pending_gen, generating);

   free(pending);
   return;

end:
   eina_stringshare_del(pending->file);
   eina_stringshare_del(pending->key);
   eina_stringshare_del(pending->thumb);
   eina_stringshare_del(pending->thumb_key);
   if (pending->free_data)
     pending->free_data(pending->data);
   free(pending);
}

/**
 * @brief Sends a "queue_add" D-Bus request to the Ethumb server.
 * This function constructs and sends a D-Bus message to the server to add a new
 * thumbnail generation task to its queue. It creates a `_ethumb_pending_add` structure
 * to track this request.
 *
 * The "queue_add" D-Bus method expects parameters with signature "iayayayay":
 * - `i`: Client-generated ID for the request.
 * - `ay`: Path to the original file.
 * - `ay`: Key within the original file.
 * - `ay`: Path for the thumbnail.
 * - `ay`: Key for the thumbnail.
 *
 * @param client The Ethumb_Client instance.
 * @param file Path to the original file.
 * @param key Optional key within the original file.
 * @param thumb Path where the thumbnail should be stored.
 * @param thumb_key Optional key for the thumbnail file.
 * @param generated_cb Callback for when generation is complete.
 * @param data User data for `generated_cb`.
 * @param free_data Function to free `data`.
 * @return The client-side generated ID for this request.
 */
static int
_ethumb_client_queue_add(Ethumb_Client *client, const char *file, const char *key, const char *thumb, const char *thumb_key, Ethumb_Client_Generate_Cb generated_cb, const void *data, Eina_Free_Cb free_data)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *main_itr;
   struct _ethumb_pending_add *pending;

   pending = calloc(1, sizeof(*pending));
   pending->id = client->id_count;
   pending->file = eina_stringshare_add(file);
   pending->key = eina_stringshare_add(key);
   pending->thumb = eina_stringshare_add(thumb);
   pending->thumb_key = eina_stringshare_add(thumb_key);
   pending->generated_cb = generated_cb;
   pending->data = (void *)data;
   pending->free_data = free_data;
   pending->client = client;

   client->id_count = (client->id_count + 1) % MAX_ID;

   msg = eldbus_proxy_method_call_new(client->proxy, "queue_add");
   main_itr = eldbus_message_iter_get(msg);
   eldbus_message_iter_basic_append(main_itr, 'i', pending->id);
   _ethumb_client_dbus_append_bytearray(main_itr, file);
   _ethumb_client_dbus_append_bytearray(main_itr, key);
   _ethumb_client_dbus_append_bytearray(main_itr, thumb);
   _ethumb_client_dbus_append_bytearray(main_itr, thumb_key);

   client->pending_add = eina_list_append(client->pending_add, pending);

   pending->pending_call = eldbus_proxy_send(client->proxy, msg,
                                             _ethumb_client_queue_add_cb,
                                             pending, -1);

   return pending->id;
}

/**
 * @brief Callback for the "queue_remove" D-Bus method call.
 * This function is invoked when the Ethumb server responds to a "queue_remove" request,
 * which is used to cancel a pending thumbnail generation. It calls the user's
 * cancellation callback with the success status.
 *
 * The "queue_remove" method returns a boolean `b` indicating success.
 *
 * @param data Pointer to the struct _ethumb_pending_remove for this request.
 * @param msg The D-Bus reply message.
 * @param eldbus_pending The Eldbus_Pending object for this call (unused).
 */
static void
_ethumb_client_queue_remove_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *eldbus_pending EINA_UNUSED)
{
   Eina_Bool success = EINA_FALSE;
   struct _ethumb_pending_remove *pending = data;
   Ethumb_Client *client = pending->client;
   const char *errname, *errmsg;

   pending->pending_call = NULL;
   if (!client) goto end;
   client->pending_remove = eina_list_remove(client->pending_remove, pending);

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Error: %s %s", errname, errmsg);
        goto end;
     }

   if (!eldbus_message_arguments_get(msg, "b", &success))
     {
        ERR("Error getting arguments.");
        goto end;
     }

end:
   if (pending->cancel_cb)
     pending->cancel_cb(pending->data, success);
   if (pending->free_data)
     pending->free_data(pending->data);
   free(pending);
}

/**
 * @endcond
 */

/**
 * Ask server to cancel generation of thumbnail.
 *
 * @param client client instance. Must @b not be @c NULL and client
 *        must be connected (after connected_cb is called).
 * @param id valid id returned by ethumb_client_generate()
 * @param cancel_cb function to report cancellation results.
 * @param data context argument to give back to @a cancel_cb. May be
 *        @c NULL.
 * @param data context to give back to @a cancel_cb. May be @c
 *        NULL.
 * @param free_data used to release @a data resources after @a
 *        cancel_cb is called or user calls
 *        ethumb_client_disconnect().
 */
EAPI void
ethumb_client_generate_cancel(Ethumb_Client *client, int id, Ethumb_Client_Generate_Cancel_Cb cancel_cb, const void *data, Eina_Free_Cb free_data)
{
   struct _ethumb_pending_remove *pending;
   Eina_List *l;
   int found;
   int32_t id32 = id;
   EINA_SAFETY_ON_NULL_RETURN(client);
   EINA_SAFETY_ON_FALSE_RETURN(id >= 0);

   pending = calloc(1, sizeof(*pending));
   pending->id = id;
   pending->cancel_cb = cancel_cb;
   pending->data = (void *)data;
   pending->free_data = free_data;
   pending->client = client;

   pending->pending_call =
     eldbus_proxy_call(client->proxy, "queue_remove",
                       _ethumb_client_queue_remove_cb, pending, -1,
                       "i", pending->id);
   client->pending_remove = eina_list_append(client->pending_remove, pending);

   /*
    * Check if answer was not received yet cancel it
    * callback of queue_add will be called with a error msg
    * and data will be freed
    */
   found = 0;
   l = client->pending_add;
   while (l)
     {
        struct _ethumb_pending_add *pending_add = l->data;
        if (pending_add->id != id32)
          {
             l = l->next;
             continue;
          }
        if (pending_add->pending_call)
          {
             Eldbus_Pending *call = pending_add->pending_call;

             pending_add->pending_call = NULL;
             eldbus_pending_cancel(call);
          }
        found = 1;
        break;
     }

   if (found)
     return;

   //if already received answer only free memory
   l = client->pending_gen;
   while (l)
     {
        struct _ethumb_pending_gen *pending_gen = l->data;
        if (pending_gen->id != id32)
          {
             l = l->next;
             continue;
          }
        client->pending_gen = eina_list_remove_list(client->pending_gen, l);
        eina_stringshare_del(pending_gen->file);
        eina_stringshare_del(pending_gen->key);
        eina_stringshare_del(pending_gen->thumb);
        eina_stringshare_del(pending_gen->thumb_key);
        if (pending_gen->free_data)
          pending_gen->free_data(pending_gen->data);
        free(pending_gen);
        break;
     }
}

/**
 * Ask server to cancel generation of all thumbnails.
 *
 * @param client client instance. Must @b not be @c NULL and client
 *        must be connected (after connected_cb is called).
 *
 * @see ethumb_client_generate_cancel()
 */
EAPI void
ethumb_client_generate_cancel_all(Ethumb_Client *client)
{
   void *data;
   EINA_SAFETY_ON_NULL_RETURN(client);

   EINA_LIST_FREE(client->pending_add, data)
     {
        struct _ethumb_pending_add *pending = data;
        if (pending->pending_call)
          eldbus_pending_cancel(pending->pending_call);
        pending->pending_call = NULL;
        pending->client = NULL;
        free(pending);
     }

   EINA_LIST_FREE(client->pending_gen, data)
     {
        struct _ethumb_pending_gen *pending = data;
        eina_stringshare_del(pending->file);
        eina_stringshare_del(pending->key);
        eina_stringshare_del(pending->thumb);
        eina_stringshare_del(pending->thumb_key);
        if (pending->free_data)
          pending->free_data(pending->data);
        free(pending);
     }

   eldbus_proxy_call(client->proxy, "queue_clear", NULL, NULL, -1, "");
}

/**
 * Configure future requests to use FreeDesktop.Org preset.
 *
 * This is a preset to provide freedesktop.org (fdo) standard
 * compliant thumbnails. That is, files are stored as JPEG under
 * ~/.thumbnails/SIZE, with size being either normal (128x128) or
 * large (256x256).
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param s size identifier, either #ETHUMB_THUMB_NORMAL (0) or
 *        #ETHUMB_THUMB_LARGE (1).
 *
 * @see ethumb_client_size_set()
 * @see ethumb_client_aspect_set()
 * @see ethumb_client_crop_align_set()
 * @see ethumb_client_category_set()
 * @see ethumb_client_dir_path_set()
 */
EAPI void
ethumb_client_fdo_set(Ethumb_Client *client, Ethumb_Thumb_FDO_Size s)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_thumb_fdo_set(client->ethumb, s);
}

/**
 * Configure future request to use custom size.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param tw width, default is 128.
 * @param th height, default is 128.
 */
EAPI void
ethumb_client_size_set(Ethumb_Client *client, int tw, int th)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_thumb_size_set(client->ethumb, tw, th);
}

/**
 * Retrieve future request to use custom size.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param tw where to return width. May be @c NULL.
 * @param th where to return height. May be @c NULL.
 */
EAPI void
ethumb_client_size_get(const Ethumb_Client *client, int *tw, int *th)
{
   if (tw) *tw = 0;
   if (th) *th = 0;
   EINA_SAFETY_ON_NULL_RETURN(client);

   ethumb_thumb_size_get(client->ethumb, tw, th);
}

/**
 * Configure format to use for future requests.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param f format identifier to use, either #ETHUMB_THUMB_FDO (0),
 *        #ETHUMB_THUMB_JPEG (1) or #ETHUMB_THUMB_EET (2). Default is FDO.
 */
EAPI void
ethumb_client_format_set(Ethumb_Client *client, Ethumb_Thumb_Format f)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_thumb_format_set(client->ethumb, f);
}

/**
 * Retrieve format to use for future requests.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 *
 * @return format identifier to use, either #ETHUMB_THUMB_FDO (0),
 *         #ETHUMB_THUMB_JPEG (1) or #ETHUMB_THUMB_EET (2).
 */
EAPI Ethumb_Thumb_Format
ethumb_client_format_get(const Ethumb_Client *client)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, 0);

   return ethumb_thumb_format_get(client->ethumb);
}

/**
 * Configure aspect mode to use.
 *
 * If aspect is kept (#ETHUMB_THUMB_KEEP_ASPECT), then image will be
 * rescaled so the largest dimension is not bigger than it's specified
 * size (see ethumb_client_size_get()) and the other dimension is
 * resized in the same proportion. Example: size is 256x256, image is
 * 1000x500, resulting thumbnail is 256x128.
 *
 * If aspect is ignored (#ETHUMB_THUMB_IGNORE_ASPECT), then image will
 * be distorted to match required thumbnail size. Example: size is
 * 256x256, image is 1000x500, resulting thumbnail is 256x256.
 *
 * If crop is required (#ETHUMB_THUMB_CROP), then image will be
 * cropped so the smallest dimension is not bigger than its specified
 * size (see ethumb_client_size_get()) and the other dimension will
 * overflow, not being visible in the final image. How it will
 * overflow is speficied by ethumb_client_crop_align_set()
 * alignment. Example: size is 256x256, image is 1000x500, crop
 * alignment is 0.5, 0.5, resulting thumbnail is 256x256 with 250
 * pixels from left and 250 pixels from right being lost, that is just
 * the 500x500 central pixels of image will be considered for scaling.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param a aspect mode identifier, either #ETHUMB_THUMB_KEEP_ASPECT (0),
 *        #ETHUMB_THUMB_IGNORE_ASPECT (1) or #ETHUMB_THUMB_CROP (2).
 */
EAPI void
ethumb_client_aspect_set(Ethumb_Client *client, Ethumb_Thumb_Aspect a)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_thumb_aspect_set(client->ethumb, a);
}

/**
 * Get current aspect in use for requests.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 *
 * @return aspect in use for future requests.
 */
EAPI Ethumb_Thumb_Aspect
ethumb_client_aspect_get(const Ethumb_Client *client)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, 0);

   return ethumb_thumb_aspect_get(client->ethumb);
}

/**
 * Configure orientation to use for future requests.
 *
 * Default value is #ETHUMB_THUMB_ORIENT_ORIGINAL: metadata from the file
 * will be used to orient pixel data.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param o format identifier to use, either #ETHUMB_THUMB_ORIENT_NONE (0),
 *        #ETHUMB_THUMB_ROTATE_90_CW (1), #ETHUMB_THUMB_ROTATE_180 (2),
 *        #ETHUMB_THUMB_ROTATE_90_CCW (3), #ETHUMB_THUMB_FLIP_HORIZONTAL (4),
 *        #ETHUMB_THUMB_FLIP_VERTICAL (5), #ETHUMB_THUMB_FLIP_TRANSPOSE (6),
 *        #ETHUMB_THUMB_FLIP_TRANSVERSE (7) or #ETHUMB_THUMB_ORIENT_ORIGINAL
 *        (8). Default is ORIGINAL.
 */
EAPI void
ethumb_client_orientation_set(Ethumb_Client *client, Ethumb_Thumb_Orientation o)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_thumb_orientation_set(client->ethumb, o);
}

/**
 * Get current orientation in use for requests.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 *
 * @return orientation in use for future requests.
 */
EAPI Ethumb_Thumb_Orientation
ethumb_client_orientation_get(const Ethumb_Client *client)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, 0);

   return ethumb_thumb_orientation_get(client->ethumb);
}

/**
 * Configure crop alignment in use for future requests.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param x horizontal alignment. 0.0 means left side will be visible
 *        or right side is being lost. 1.0 means right side will be
 *        visible or left side is being lost. 0.5 means just center is
 *        visible, both sides will be lost.  Default is 0.5.
 * @param y vertical alignment. 0.0 is top visible, 1.0 is bottom
 *        visible, 0.5 is center visible. Default is 0.5
 */
EAPI void
ethumb_client_crop_align_set(Ethumb_Client *client, float x, float y)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_thumb_crop_align_set(client->ethumb, x, y);
}

/**
 * Get current crop alignment in use for requests.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param x where to return horizontal alignment. May be @c NULL.
 * @param y where to return vertical alignment. May be @c NULL.
 */
EAPI void
ethumb_client_crop_align_get(const Ethumb_Client *client, float *x, float *y)
{
   if (x) *x = 0.0;
   if (y) *y = 0.0;
   EINA_SAFETY_ON_NULL_RETURN(client);

   ethumb_thumb_crop_align_get(client->ethumb, x, y);
}

/**
 * Configure quality to be used in thumbnails.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param quality value from 0 to 100, default is 80. The effect
 *        depends on the format being used, PNG will not use it.
 */
EAPI void
ethumb_client_quality_set(Ethumb_Client *client, int quality)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   ethumb_thumb_quality_set(client->ethumb, quality);
}

/**
 * Get quality to be used in thumbnails.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 *
 * @return quality value from 0 to 100, default is 80. The effect
 *         depends on the format being used, PNG will not use it.
 */
EAPI int
ethumb_client_quality_get(const Ethumb_Client *client)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, 0);

   return ethumb_thumb_quality_get(client->ethumb);
}

/**
 * Configure compression level used in requests.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param compress value from 0 to 9, default is 9. The effect
 *        depends on the format being used, JPEG will not use it.
 */
EAPI void
ethumb_client_compress_set(Ethumb_Client *client, int compress)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   ethumb_thumb_compress_set(client->ethumb, compress);
}

/**
 * Get compression level used in requests.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 *
 * @return compress value from 0 to 9, default is 9. The effect
 *         depends on the format being used, JPEG will not use it.
 */
EAPI int
ethumb_client_compress_get(const Ethumb_Client *client)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, 0);

   return ethumb_thumb_compress_get(client->ethumb);
}

/**
 * Set frame to apply to future thumbnails.
 *
 * This will create an edje object that will have image swallowed
 * in. This can be used to simulate Polaroid or wood frames in the
 * generated image. Remeber it is bad to modify the original contents
 * of thumbnails, but sometimes it's useful to have it composited and
 * avoid runtime overhead.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param file file path to edje.
 * @param group group inside edje to use.
 * @param swallow name of swallow part.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
ethumb_client_frame_set(Ethumb_Client *client, const char *file, const char *group, const char *swallow)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, 0);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   return ethumb_frame_set(client->ethumb, file, group, swallow);
}

/**
 * Configure where to store thumbnails in future requests.
 *
 * This value will be used to generate thumbnail paths, that is, it
 * will be used when ethumb_client_thumb_path_set() was not called
 * after last ethumb_client_file_set().
 *
 * Note that this is the base, a category is added to this path as a
 * sub directory. This is not the final directory where files are
 * stored, the thumbnail system will account @b category as well, see
 * ethumb_client_category_set().
 *
 * As other options, this value will only be applied to future
 * requests.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param path base directory where to store thumbnails. Default is
 *        ~/.thumbnails
 *
 * @see ethumb_client_category_set()
 */
EAPI void
ethumb_client_dir_path_set(Ethumb_Client *client, const char *path)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_thumb_dir_path_set(client->ethumb, path);
}

/**
 * Get base directory path where to store thumbnails.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 *
 * @return pointer to internal string with current path. This string
 *         should not be modified or freed.
 *
 * @see ethumb_client_dir_path_set()
 */
EAPI const char *
ethumb_client_dir_path_get(const Ethumb_Client *client)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, NULL);

   return ethumb_thumb_dir_path_get(client->ethumb);
}

/**
 * Category directory to store thumbnails.
 *
 * This value will be used to generate thumbnail paths, that is, it
 * will be used when ethumb_client_thumb_path_set() was not called
 * after last ethumb_client_file_set().
 *
 * This is a sub-directory inside base directory
 * (ethumb_client_dir_path_set()) that creates a namespace to avoid
 * different options resulting in the same file.
 *
 * As other options, this value will only be applied to future
 * requests.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param category category sub directory to store thumbnail. Default
 *        is either "normal" or "large" for FDO compliant thumbnails
 *        or WIDTHxHEIGHT-ASPECT[-FRAMED]-FORMAT. It can be a string
 *        or @c NULL to use auto generated names.
 *
 * @see ethumb_client_dir_path_set()
 */
EAPI void
ethumb_client_category_set(Ethumb_Client *client, const char *category)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_thumb_category_set(client->ethumb, category);
}

/**
 * Get category sub-directory  where to store thumbnails.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 *
 * @return pointer to internal string with current path. This string
 *         should not be modified or freed.
 *
 * @see ethumb_client_category_set()
 */
EAPI const char *
ethumb_client_category_get(const Ethumb_Client *client)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, NULL);

   return ethumb_thumb_category_get(client->ethumb);
}

/**
 * Set the video time (duration) in seconds.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param t duration (in seconds). Defaults to 3 seconds.
 */
EAPI void
ethumb_client_video_time_set(Ethumb_Client *client, float t)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_video_time_set(client->ethumb, t);
}

/**
 * Set initial video position to start thumbnailing, in percentage.
 *
 * This is useful to avoid thumbnailing the company/producer logo or
 * movie opening.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param start initial video positon to thumbnail, in percentage (0.0
 *        to 1.0, inclusive). Defaults to 10% (0.1).
 */
EAPI void
ethumb_client_video_start_set(Ethumb_Client *client, float start)
{
   EINA_SAFETY_ON_NULL_RETURN(client);
   EINA_SAFETY_ON_FALSE_RETURN(start >= 0.0);
   EINA_SAFETY_ON_FALSE_RETURN(start <= 1.0);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_video_start_set(client->ethumb, start);
}

/**
 * Set the video frame interval, in seconds.
 *
 * This is useful for animated thumbnail and will define skip time
 * before going to the next frame. Note that video backends might not
 * be able to precisely skip that amount as it will depend on various
 * factors, including video encoding.
 *
 * Although this seems similar to ethumb_client_video_fps_set(), this
 * one is the time that will be used to seek. The math is simple, for
 * each new frame the video position will be set to:
 * ((video_length * start_time) + (interval * current_frame_number)).
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param interval time between frames, in seconds. Defaults to 0.05
 *        seconds.
 */
EAPI void
ethumb_client_video_interval_set(Ethumb_Client *client, float interval)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_video_interval_set(client->ethumb, interval);
}

/**
 * Set the number of frames to thumbnail.
 *
 * This is useful for animated thumbnail and will define how many
 * frames the generated file will have.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param ntimes number of times, must be greater than zero.
 *        Defaults to 3.
 */
EAPI void
ethumb_client_video_ntimes_set(Ethumb_Client *client, unsigned int ntimes)
{
   EINA_SAFETY_ON_NULL_RETURN(client);
   EINA_SAFETY_ON_FALSE_RETURN(ntimes > 0);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_video_ntimes_set(client->ethumb, ntimes);
}

/**
 * Set the number of frames per second to thumbnail the video.
 *
 * This configures the number of times per seconds the thumbnail will
 * use to create thumbnails.
 *
 * Although this is similar to ethumb_client_video_interval_set(), it
 * is the delay used between calling functions thata generates frames,
 * while the other is the time used to skip inside the video.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param fps number of frames per second to thumbnail. Must be greater
 *        than zero. Defaults to 10.
 */
EAPI void
ethumb_client_video_fps_set(Ethumb_Client *client, unsigned int fps)
{
   EINA_SAFETY_ON_NULL_RETURN(client);
   EINA_SAFETY_ON_FALSE_RETURN(fps > 0);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_video_fps_set(client->ethumb, fps);
}

/**
 * Set the page number to thumbnail in paged documents.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param page page number, defaults to 0 (first).
 */
EAPI void
ethumb_client_document_page_set(Ethumb_Client *client, unsigned int page)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   if (!client->old_ethumb_conf)
     client->old_ethumb_conf = ethumb_dup(client->ethumb);
   ethumb_document_page_set(client->ethumb, page);
}

/**
 * Set source file to be thumbnailed.
 *
 * Calling this function has the side effect of resetting values set
 * with ethumb_client_thumb_path_set() or auto-generated with
 * ethumb_client_thumb_exists().
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param path the filesystem path to use. May be @c NULL.
 * @param key the extra argument/key inside @a path to read image
 *        from. This is only used for formats that allow multiple
 *        resources in one file, like EET or Edje (group name).
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
ethumb_client_file_set(Ethumb_Client *client, const char *path, const char *key)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, 0);

   return ethumb_file_set(client->ethumb, path, key);
}

/**
 * Get values set with ethumb_client_file_get()
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param path where to return configured path. May be @c NULL.  If
 *        not @c NULL, then it will be a pointer to a stringshared
 *        instance, but @b no references are added (do it with
 *        eina_stringshare_ref())!
 * @param key where to return configured key. May be @c NULL.If not @c
 *        NULL, then it will be a pointer to a stringshared instance,
 *        but @b no references are added (do it with
 *        eina_stringshare_ref())!
 */
EAPI void
ethumb_client_file_get(Ethumb_Client *client, const char **path, const char **key)
{
   if (path) *path = NULL;
   if (key) *key = NULL;
   EINA_SAFETY_ON_NULL_RETURN(client);

   ethumb_file_get(client->ethumb, path, key);
}

/**
 * Reset previously set file to @c NULL.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 */
EAPI void
ethumb_client_file_free(Ethumb_Client *client)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   ethumb_file_free(client->ethumb);
}

/**
 * Set a defined path and key to store the thumbnail.
 *
 * If not explicitly given, the thumbnail path will be auto-generated
 * by ethumb_client_thumb_exists() or server using configured
 * parameters like size, aspect and category.
 *
 * Set these to @c NULL to forget previously given values. After
 * ethumb_client_file_set() these values will be reset to @c NULL.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param path force generated thumbnail to the exact given path. If
 *        @c NULL, then reverts back to auto-generation.
 * @param key force generated thumbnail to the exact given key. If
 *        @c NULL, then reverts back to auto-generation.
 */
EAPI void
ethumb_client_thumb_path_set(Ethumb_Client *client, const char *path, const char *key)
{
   EINA_SAFETY_ON_NULL_RETURN(client);

   ethumb_thumb_path_set(client->ethumb, path, key);
}

/**
 * Get the configured thumbnail path.
 *
 * This returns the value set with ethumb_client_thumb_path_set() or
 * auto-generated by ethumb_client_thumb_exists() if it was not set.
 *
 * @param client the client instance to use. Must @b not be @c
 *        NULL. May be pending connected (can be called before @c
 *        connected_cb)
 * @param path where to return configured path. May be @c NULL.  If
 *        there was no path configured with
 *        ethumb_client_thumb_path_set() and
 *        ethumb_client_thumb_exists() was not called, then it will
 *        probably return @c NULL. If not @c NULL, then it will be a
 *        pointer to a stringshared instance, but @b no references are
 *        added (do it with eina_stringshare_ref())!
 * @param key where to return configured key. May be @c NULL.  If
 *        there was no key configured with
 *        ethumb_client_thumb_key_set() and
 *        ethumb_client_thumb_exists() was not called, then it will
 *        probably return @c NULL. If not @c NULL, then it will be a
 *        pointer to a stringshared instance, but @b no references are
 *        added (do it with eina_stringshare_ref())!
 */
EAPI void
ethumb_client_thumb_path_get(Ethumb_Client *client, const char **path, const char **key)
{
   if (path) *path = NULL;
   if (key) *key = NULL;
   EINA_SAFETY_ON_NULL_RETURN(client);

   ethumb_thumb_path_get(client->ethumb, path, key);
}

/**
 * Checks whenever file already exists (locally!)
 *
 * This will check locally (not calling server) if thumbnail already
 * exists or not, also calculating the thumbnail path. See
 * ethumb_client_thumb_path_get(). Path must be configured with
 * ethumb_client_file_set() before using it and the last set file will
 * be used!
 *
 * @param client client instance. Must @b not be @c NULL and client
 *        must be configured with ethumb_client_file_set().
 *
 * @return @c NULL on failure, a valid Ethumb_Exists pointer otherwise
 */
EAPI Ethumb_Exists *
ethumb_client_thumb_exists(Ethumb_Client *client, Ethumb_Client_Thumb_Exists_Cb exists_cb, const void *data)
{
   const char *path = NULL;
   Ethumb_Async_Exists *async = NULL;
   Ethumb_Exists *cb = NULL;
   Ecore_Thread *t;

   EINA_SAFETY_ON_NULL_RETURN_VAL(client, NULL);

   ethumb_file_get(client->ethumb, &path, NULL);
   if (!path) goto on_error;

   async = eina_hash_find(_exists_request, path);
   if (!async)
     {
        async = malloc(sizeof (Ethumb_Async_Exists));
        if (!async) goto on_error;

        async->path = eina_stringshare_ref(path);
        async->callbacks = NULL;
        async->dup = ethumb_dup(client->ethumb);

        if (!async->dup) goto on_error;

        cb = malloc(sizeof (Ethumb_Exists));
        if (!cb) goto on_error;

        EINA_REFCOUNT_REF(client);
        cb->client = client;
        cb->dup = ethumb_dup(client->ethumb);
        cb->exists_cb = exists_cb;
        cb->data = data;
        cb->parent = async;

        async->callbacks = eina_list_append(async->callbacks, cb);

        /* spawn a thread here */
        t = ecore_thread_run(_ethumb_client_exists_heavy,
                             _ethumb_client_exists_end,
                             _ethumb_client_exists_end,
                             async);
        if (!t) return NULL;
        async->thread = t;

        eina_hash_direct_add(_exists_request, async->path, async);

        return cb;
     }

   cb = malloc(sizeof (Ethumb_Exists));
   if (!cb)
     {
        async = NULL;
        goto on_error;
     }

   EINA_REFCOUNT_REF(client);
   cb->client = client;
   cb->dup = ethumb_dup(client->ethumb);
   cb->exists_cb = exists_cb;
   cb->data = data;
   cb->parent = async;

   async->callbacks = eina_list_append(async->callbacks, cb);

   return cb;

on_error:
   exists_cb((void *)data, client, NULL, EINA_FALSE);

   if (async)
     {
        eina_stringshare_del(async->path);
        if (async->dup) ethumb_free(async->dup);
        free(async);
     }
   return NULL;
}

/**
 * Cancel an ongoing exists request.
 *
 * @param exists the request to cancel.
 */
EAPI void
ethumb_client_thumb_exists_cancel(Ethumb_Exists *exists)
{
   Ethumb_Async_Exists *async = exists->parent;

   async->callbacks = eina_list_remove(async->callbacks, exists);
   if (eina_list_count(async->callbacks) <= 0)
     ecore_thread_cancel(async->thread);

   ethumb_free(exists->dup);
   EINA_REFCOUNT_UNREF(exists->client) _ethumb_client_free(exists->client);
   free(exists);
}

/**
 * Check if an exists request was cancelled.
 *
 * @param exists the request to check.
 * @result return EINA_TRUE if the request was cancelled.
 */
EAPI Eina_Bool
ethumb_client_thumb_exists_check(Ethumb_Exists *exists)
{
   Ethumb_Async_Exists *async = exists->parent;

   if (!async) return EINA_TRUE;

   if (async->callbacks) return EINA_FALSE;

   return ecore_thread_check(async->thread);
}

/**
 * Ask server to generate thumbnail.
 *
 * This process is asynchronous and will report back from main loop
 * using @a generated_cb. One can cancel this request by calling
 * ethumb_client_generate_cancel() or
 * ethumb_client_generate_cancel_all(), but not that request might be
 * processed by server already and no generated files will be removed
 * if that is the case.
 *
 * This will not check if file already exists, this should be done by
 * explicitly calling ethumb_client_thumb_exists(). That is, this
 * function will override any existing thumbnail.
 *
 * @param client client instance. Must @b not be @c NULL and client
 *        must be connected (after connected_cb is called).
 * @param generated_cb function to report generation results.
 * @param data context argument to give back to @a generated_cb. May
 *        be @c NULL.
 * @param data context to give back to @a generate_cb. May be @c
 *        NULL.
 * @param free_data used to release @a data resources after @a
 *        generated_cb is called or user calls
 *        ethumb_client_disconnect().
 *
 * @return identifier or -1 on error. If -1 is returned (error) then
 *         @a free_data is @b not called!
 *
 * @see ethumb_client_connect()
 * @see ethumb_client_file_set()
 * @see ethumb_client_thumb_exists()
 * @see ethumb_client_generate_cancel()
 * @see ethumb_client_generate_cancel_all()
 */
EAPI int
ethumb_client_generate(Ethumb_Client *client, Ethumb_Client_Generate_Cb generated_cb, const void *data, Eina_Free_Cb free_data)
{
   const char *file, *key, *thumb, *thumb_key;
   int id;
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, -1);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(client->connected, -1);

   ethumb_file_get(client->ethumb, &file, &key);
   if (!file)
     {
        ERR("no file set.");
        return -1;
     }

   ethumb_thumb_path_get(client->ethumb, &thumb, &thumb_key);

   if (client->old_ethumb_conf &&
       ethumb_cmp(client->old_ethumb_conf, client->ethumb))
     {
        ethumb_client_ethumb_setup(client);
        ethumb_free(client->old_ethumb_conf);
        client->old_ethumb_conf = NULL;
     }
   id = _ethumb_client_queue_add(client, file, key, thumb, thumb_key,
                                 generated_cb, data, free_data);
   return id;
}

struct _Ethumb_Client_Async
{
   Ethumb_Exists               *exists;
   Ethumb_Client               *client;
   Ethumb                      *dup;

   Ethumb_Client_Async_Done_Cb  done; /**< Callback for successful completion (thumbnail exists or generated). */
   Ethumb_Client_Async_Error_Cb error; /**< Callback for errors during the process. */
   const void                  *data; /**< User data for the callbacks. */

   int                          id; /**< ID of the generation request if one was made, -1 otherwise. */
};

/** @brief Array of Ecore_Idler pointers for managing batched operations.
 * idler[0] is for batching `ethumb_client_thumb_exists` calls.
 * idler[1] is for batching `ethumb_client_generate` calls.
 */
static Ecore_Idler *idler[2] = { NULL, NULL };
/** @brief List of active `Ethumb_Client_Async` requests that are currently being processed
 * (either waiting for `ethumb_client_thumb_exists` callback or `_ethumb_client_thumb_finish` callback).
 */
static Eina_List *pending = NULL;
/** @brief Array of Eina_List pointers for tasks to be processed by idlers.
 * idle_tasks[0] stores `Ethumb_Client_Async` requests waiting to call `ethumb_client_thumb_exists`.
 * idle_tasks[1] stores `Ethumb_Client_Async` requests waiting to call `ethumb_client_generate`.
 */
static Eina_List *idle_tasks[2] = { NULL, NULL };

/**
 * @brief Frees an Ethumb_Client_Async structure and its associated resources.
 * This includes freeing the duplicated Ethumb instance and unreferencing the client.
 * @param async The Ethumb_Client_Async structure to free.
 */
static void
_ethumb_client_async_free(Ethumb_Client_Async *async)
{
   Ethumb_Client *client = async->client;
   ethumb_free(async->dup);
   free(async);
   if (client)
     {
        EINA_REFCOUNT_UNREF(client) _ethumb_client_free(client);
     }
}

static void
_ethumb_client_thumb_finish(void *data,
                            Ethumb_Client *client, int id,
                            const char *file EINA_UNUSED, const char *key EINA_UNUSED,
                            const char *thumb_path, const char *thumb_key,
                            Eina_Bool success)
{
   Ethumb_Client_Async *async = data;

   EINA_SAFETY_ON_FALSE_RETURN(async->id == id);

   if (success)
     {
        async->done(client, thumb_path, thumb_key, (void *)async->data);
     }
   else
     {
        async->error(client, (void *)async->data);
     }

   pending = eina_list_remove(pending, async);
   _ethumb_client_async_free(async);
}

/**
 * @brief Ecore_Idler callback to process thumbnail generation requests in batches.
 * This idler iterates through `idle_tasks[1]` (tasks waiting for generation).
 * For each task, it calls `ethumb_client_generate`.
 * It processes tasks until a certain time slice is consumed to avoid blocking the main loop.
 * @param data User data for the idler (unused).
 * @return EINA_TRUE if there are more tasks to process, EINA_FALSE otherwise (idler will be removed).
 */
static Eina_Bool
_ethumb_client_thumb_generate_idler(void *data EINA_UNUSED)
{
   Ethumb_Client_Async *async;
   Eina_List *l1, *l2;

   EINA_LIST_FOREACH_SAFE (idle_tasks[1], l1, l2, async)
     {
        Ethumb *tmp;

        idle_tasks[1] = eina_list_remove_list(idle_tasks[1], l1);

        tmp = async->client->ethumb;
        async->client->ethumb = async->dup;

        async->id = ethumb_client_generate(async->client, _ethumb_client_thumb_finish, async, NULL);
        if (async->id == -1)
          {
             async->error(async->client, (void *)async->data);
             async->client->ethumb = tmp;
             _ethumb_client_async_free(async);
             async = NULL;
          }
        else
          {
             async->client->ethumb = tmp;
          }

        if (async)
          pending = eina_list_append(pending, async);

        if (ecore_time_get() - ecore_loop_time_get() > ecore_animator_frametime_get() * 0.5)
          return EINA_TRUE;
     }

   idler[1] = NULL;
   return EINA_FALSE;
}

/**
 * @brief Callback for `ethumb_client_thumb_exists` used by the asynchronous helper.
 * This function is called when the local existence check for a thumbnail completes.
 * If the thumbnail exists, it calls the `done` callback of the `Ethumb_Client_Async` request.
 * If it doesn't exist, it queues the request for generation by adding it to `idle_tasks[1]`
 * and ensures the generation idler (`_ethumb_client_thumb_generate_idler`) is running.
 * @param data Pointer to the `Ethumb_Client_Async` structure.
 * @param client The Ethumb_Client instance.
 * @param request The `Ethumb_Exists` request object.
 * @param exists EINA_TRUE if the thumbnail exists locally, EINA_FALSE otherwise.
 */
static void
_ethumb_client_thumb_exists(void *data, Ethumb_Client *client, Ethumb_Exists *request, Eina_Bool exists)
{
   Ethumb_Client_Async *async = data;

   if (request == NULL)
     return;

   EINA_SAFETY_ON_FALSE_RETURN(async->exists == request);

   async->exists = NULL;
   pending = eina_list_remove(pending, async);

   if (exists)
     {
        const char *thumb_path;
        const char *thumb_key;

        ethumb_client_thumb_path_get(client, &thumb_path, &thumb_key);
        async->done(client, thumb_path, thumb_key, (void *)async->data);
        _ethumb_client_async_free(async);
     }
   else
     {
        idle_tasks[1] = eina_list_append(idle_tasks[1], async);

        if (!idler[1])
          idler[1] = ecore_idler_add(_ethumb_client_thumb_generate_idler, NULL);
     }
}

/**
 * @brief Ecore_Idler callback to process thumbnail existence check requests in batches.
 * This idler iterates through `idle_tasks[0]` (tasks waiting for existence check).
 * For each task, it calls `ethumb_client_thumb_exists`.
 * It processes tasks until a certain time slice is consumed to avoid blocking the main loop.
 * @param data User data for the idler (unused).
 * @return EINA_TRUE if there are more tasks to process, EINA_FALSE otherwise (idler will be removed).
 */
static Eina_Bool
_ethumb_client_thumb_exists_idler(void *data EINA_UNUSED)
{
   Ethumb_Client_Async *async;
   Eina_List *l1, *l2;

   EINA_LIST_FOREACH_SAFE (idle_tasks[0], l1, l2, async)
     {
        Ethumb *tmp;

        idle_tasks[0] = eina_list_remove_list(idle_tasks[0], l1);

        tmp = async->client->ethumb;
        async->client->ethumb = async->dup;

        async->exists = ethumb_client_thumb_exists(async->client, _ethumb_client_thumb_exists, async);
        if (!async->exists)
          {
             async->error(async->client, (void *)async->data);
             async->client->ethumb = tmp;
             _ethumb_client_async_free(async);
             continue;
          }

        async->client->ethumb = tmp;

        pending = eina_list_append(pending, async);

        if (ecore_time_get() - ecore_loop_time_get() > ecore_animator_frametime_get() * 0.5)
          return EINA_TRUE;
     }

   idler[0] = NULL;
   return EINA_FALSE;
}

EAPI Ethumb_Client_Async *
ethumb_client_thumb_async_get(Ethumb_Client *client,
                              Ethumb_Client_Async_Done_Cb done,
                              Ethumb_Client_Async_Error_Cb error,
                              const void *data)
{
   Ethumb_Client_Async *async; /**< The asynchronous request object to be returned. */

   EINA_SAFETY_ON_NULL_RETURN_VAL(client, NULL);

   async = malloc(sizeof (Ethumb_Client_Async));
   if (!async)
     {
        error(client, (void *)data);
        return NULL;
     }

   EINA_REFCOUNT_REF(client);
   async->client = client;
   async->dup = ethumb_dup(client->ethumb);
   async->done = done;
   async->error = error;
   async->data = data;
   async->exists = NULL;
   async->id = -1;

   idle_tasks[0] = eina_list_append(idle_tasks[0], async);

   if (!idler[0])
     idler[0] = ecore_idler_add(_ethumb_client_thumb_exists_idler, NULL);

   return async;
}

EAPI void
ethumb_client_thumb_async_cancel(Ethumb_Client *client, Ethumb_Client_Async *request)
{
   const char *path;

   EINA_SAFETY_ON_NULL_RETURN(client);
   EINA_SAFETY_ON_NULL_RETURN(request);

   ethumb_file_get(request->dup, &path, NULL);

   if (request->exists)
     {
        ethumb_client_thumb_exists_cancel(request->exists);
        request->exists = NULL;

        pending = eina_list_remove(pending, request);
     }
   else if (request->id != -1)
     {
        Ethumb *tmp = request->client->ethumb;
        request->client->ethumb = request->dup;

        ethumb_client_generate_cancel(request->client, request->id, NULL, NULL, NULL);

        request->client->ethumb = tmp;

        pending = eina_list_remove(pending, request);
     }
   else
     {
        idle_tasks[0] = eina_list_remove(idle_tasks[0], request);
        idle_tasks[1] = eina_list_remove(idle_tasks[1], request);
     }

   _ethumb_client_async_free(request);
}
