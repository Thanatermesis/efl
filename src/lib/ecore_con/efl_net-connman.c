#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "efl_net-connman.h"

/** @internal Singleton proxy for net.connman.Manager */
static Eldbus_Proxy *_efl_net_connman_manager;
/** @internal Singleton DBus connection for ConnMan */
static Eldbus_Connection *_efl_net_connman_conn;
/** @internal Reference counter for efl_net_connman_init() and efl_net_connman_shutdown() */
static int _efl_net_connman_init_count;

/**
 * @internal
 * @brief Callback invoked when the _efl_net_connman_manager proxy is freed.
 *
 * This function is registered with eldbus_proxy_free_cb_add() to ensure
 * that the global _efl_net_connman_manager pointer is set to NULL when
 * the proxy object is destroyed. This helps prevent use-after-free errors,
 * especially in scenarios involving multiple unreferences.
 *
 * @param data User data passed to eldbus_proxy_free_cb_add (unused here).
 * @param dead_ptr The proxy object that was freed.
 */
static void
_efl_net_connman_manager_free(void *data EINA_UNUSED, const void *dead_ptr)
{
   DBG("ConnMan Manager was freed");

   /* handle programming errors where 'too many unrefs' happen */
   if (dead_ptr == _efl_net_connman_manager)
     _efl_net_connman_manager = NULL;
}

Eina_Bool
efl_net_connman_init(void)
{
   Eldbus_Object *obj;

   /*
    * Increment the initialization counter. If this is not the first call,
    * it means ConnMan support has already been initialized, so return success.
    * This allows multiple parts of the application to initialize ConnMan
    * support without issues.
    */
   _efl_net_connman_init_count++;
   if (_efl_net_connman_init_count > 1) return EINA_TRUE;

   if (!eldbus_init())
     {
        ERR("could not init eldbus");
        goto error;
     }

   _efl_net_connman_conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SYSTEM);
   if (!_efl_net_connman_conn)
     {
        ERR("could not get DBus system connection");
        goto error;
     }

   DBG("DBus unique name %s", eldbus_connection_unique_name_get(_efl_net_connman_conn));

   obj = eldbus_object_get(_efl_net_connman_conn, "net.connman", "/");
   if (!obj)
     {
        ERR("could not create DBus object for name='net.connman', path='/'");
        goto error_obj;
     }

   _efl_net_connman_manager = eldbus_proxy_get(obj, "net.connman.Manager");
   if (!_efl_net_connman_manager)
     {
        ERR("could not create DBus proxy for interface='net.connman.Manager', name='net.connman', path='/'");
        goto error_proxy;
     }
   eldbus_proxy_free_cb_add(_efl_net_connman_manager, _efl_net_connman_manager_free, NULL);

   DBG("ConnMan support initialized");
   return EINA_TRUE;

 error_proxy:
   eldbus_object_unref(obj);

 error_obj:
   eldbus_connection_unref(_efl_net_connman_conn);
   _efl_net_connman_conn = NULL;

 error:
   _efl_net_connman_init_count = 0;
   return EINA_FALSE;
}

void
efl_net_connman_shutdown(void)
{
   /*
    * Decrement the initialization counter. If the counter is still greater
    * than zero, it means other parts of the application are still using
    * ConnMan support, so do not shut down yet.
    */
   _efl_net_connman_init_count--;
   if (_efl_net_connman_init_count > 0) return;

   DBG("ConnMan support shutting down");

   if (_efl_net_connman_manager)
     {
        Eldbus_Object *obj = eldbus_proxy_object_get(_efl_net_connman_manager);
        eldbus_proxy_unref(_efl_net_connman_manager);
        _efl_net_connman_manager = NULL;
        eldbus_object_unref(obj);
     }

   if (_efl_net_connman_conn)
     {
        eldbus_connection_unref(_efl_net_connman_conn);
        _efl_net_connman_conn = NULL;
     }

   eldbus_shutdown();
}

Eldbus_Connection *
efl_net_connman_connection_get(void)
{
   return _efl_net_connman_conn;
}

Eldbus_Proxy *
efl_net_connman_manager_get(void)
{
   return _efl_net_connman_manager;
}
