#include "eldbus_private.h"
#include "eldbus_private_types.h"
#include <dbus/dbus.h>

/* TODO: mempool of Eldbus_Signal_Handler */

#define ELDBUS_SIGNAL_HANDLER_CHECK(handler)                        \
  do                                                               \
    {                                                              \
       EINA_SAFETY_ON_NULL_RETURN(handler);                        \
       if (!EINA_MAGIC_CHECK(handler, ELDBUS_SIGNAL_HANDLER_MAGIC)) \
         {                                                         \
            EINA_MAGIC_FAIL(handler, ELDBUS_SIGNAL_HANDLER_MAGIC);  \
            return;                                                \
         }                                                         \
    }                                                              \
  while (0)

#define ELDBUS_SIGNAL_HANDLER_CHECK_RETVAL(handler, retval)         \
  do                                                               \
    {                                                              \
       EINA_SAFETY_ON_NULL_RETURN_VAL(handler, retval);            \
       if (!EINA_MAGIC_CHECK(handler, ELDBUS_SIGNAL_HANDLER_MAGIC)) \
         {                                                         \
            EINA_MAGIC_FAIL(handler, ELDBUS_SIGNAL_HANDLER_MAGIC);  \
            return retval;                                         \
         }                                                         \
    }                                                              \
  while (0)

/**
 * @internal
 * @brief Internal function to delete a signal handler and free its resources.
 * This function is called when the reference count of the signal handler reaches zero.
 * It dispatches free callbacks, removes the handler from the connection,
 * and frees all associated memory.
 *
 * @param handler The signal handler to delete.
 */
static void _eldbus_signal_handler_del(Eldbus_Signal_Handler *handler);

/**
 * @internal
 * @brief Internal function to clean up a signal handler's resources.
 * This function is called before a signal handler is deleted or when it's explicitly
 * stopped via eldbus_signal_handler_del(). It removes the D-Bus match rule
 * and marks the handler as dangling.
 *
 * @param handler The signal handler to clean.
 */
static void _eldbus_signal_handler_clean(Eldbus_Signal_Handler *handler);

Eina_Bool
eldbus_signal_handler_init(void)
{
   // TODO: Initialize mempools if ELDBUS_SIGNAL_HANDLER_MEMPOOL is enabled.
   return EINA_TRUE;
}

void
eldbus_signal_handler_shutdown(void)
{
   // TODO: Shutdown mempools if ELDBUS_SIGNAL_HANDLER_MEMPOOL is enabled.
}

/**
 * @internal
 * @brief Appends a key-value pair to a D-Bus match rule string.
 * Ensures that the resulting match rule does not exceed the maximum allowed length.
 *
 * @param match The Eina_Strbuf to append to.
 * @param key The key of the match rule component (e.g., "sender", "path").
 * @param value The value for the key. If NULL, nothing is appended.
 */
static void
_match_append(Eina_Strbuf *match, const char *key, const char *value)
{
   if (!value) return;

   if ((eina_strbuf_length_get(match) + strlen(",=''") + strlen(key) + strlen(value))
       >= DBUS_MAXIMUM_MATCH_RULE_LENGTH)
     {
        ERR("cannot add match %s='%s' to %s: too long!", key, value,
            eina_strbuf_string_get(match));
        return;
     }

   eina_strbuf_append_printf(match, ",%s='%s'", key, value);
}

/**
 * @internal
 * @brief Comparator function for sorting Signal_Argument structures by their index.
 * Used with eina_inlist_sorted_state_insert.
 *
 * @param d1 Pointer to the first Signal_Argument.
 * @param d2 Pointer to the second Signal_Argument.
 * @return Negative if arg1->index < arg2->index,
 *         zero if arg1->index == arg2->index,
 *         positive if arg1->index > arg2->index.
 */
static int
_sort_arg(const void *d1, const void *d2)
{
   const Signal_Argument *arg1, *arg2;
   arg1 = d1;
   arg2 = d2;
   return arg1->index - arg2->index;
}

#define ARGX "arg"
EAPI Eina_Bool
eldbus_signal_handler_match_extra_vset(Eldbus_Signal_Handler *sh, va_list ap)
{
   const char *key = NULL, *read;
   DBusError err;

   ELDBUS_SIGNAL_HANDLER_CHECK_RETVAL(sh, EINA_FALSE);

   dbus_error_init(&err);
   dbus_bus_remove_match(sh->conn->dbus_conn,
                         eina_strbuf_string_get(sh->match), NULL);

   for (read = va_arg(ap, char *); read; read = va_arg(ap, char *))
     {
        Signal_Argument *arg;

        if (!key)
          {
             key = read;
             continue;
          }
        arg = calloc(1, sizeof(Signal_Argument));
        EINA_SAFETY_ON_NULL_GOTO(arg, error);
        if (!strncmp(key, ARGX, strlen(ARGX)))
          {
             int id = atoi(key + strlen(ARGX));
             arg->index = (unsigned short) id;
             arg->value = eina_stringshare_add(read);
             sh->args = eina_inlist_sorted_state_insert(sh->args,
                                                        EINA_INLIST_GET(arg),
                                                        _sort_arg,
                                                        sh->state_args);
             _match_append(sh->match, key, read);
          }
        else
          {
             ERR("%s not supported", key);
             free(arg);
          }
        key = NULL;
     }

   dbus_bus_add_match(sh->conn->dbus_conn,
                      eina_strbuf_string_get(sh->match), NULL);
   return EINA_TRUE;

error:
   dbus_bus_add_match(sh->conn->dbus_conn,
                      eina_strbuf_string_get(sh->match), NULL);
   return EINA_FALSE;
}

EAPI Eina_Bool
eldbus_signal_handler_match_extra_set(Eldbus_Signal_Handler *sh, ...)
{
   Eina_Bool ret;
   va_list ap;

   va_start(ap, sh);
   ret = eldbus_signal_handler_match_extra_vset(sh, ap);
   va_end(ap);
   return ret;
}

/**
 * @internal
 * @brief Forward declaration for the callback used when a connection (associated with a signal handler) is freed.
 */
static void _on_handler_of_conn_free(void *data, const void *dead_pointer);

/**
 * @internal
 * @brief Callback invoked when the Eldbus_Connection associated with a signal handler is freed.
 * This function ensures that the signal handler is also cleaned up (deleted)
 * and removes the corresponding free callback from the signal handler itself to prevent
 * circular dependencies or double-frees.
 *
 * @param data The Eldbus_Signal_Handler instance.
 * @param dead_pointer The Eldbus_Connection that is being freed (unused in this function).
 */
static void
_on_connection_free(void *data, const void *dead_pointer EINA_UNUSED)
{
   Eldbus_Signal_Handler *sh = data;
   // Remove the callback that would be called if the signal handler was freed first
   eldbus_signal_handler_free_cb_del(sh, _on_handler_of_conn_free, sh->conn);
   // Delete the signal handler as its connection is gone
   eldbus_signal_handler_del(sh);
}

/**
 * @internal
 * @brief Callback invoked when an Eldbus_Signal_Handler is freed.
 * This function removes the corresponding free callback from the associated Eldbus_Connection
 * to prevent attempts to use the now-freed signal handler.
 *
 * @param data The Eldbus_Connection instance.
 * @param dead_pointer The Eldbus_Signal_Handler that is being freed.
 */
static void
_on_handler_of_conn_free(void *data, const void *dead_pointer)
{
   Eldbus_Connection *conn = data;
   // Remove the callback that would be called if the connection was freed first
   eldbus_connection_free_cb_del(conn, _on_connection_free, dead_pointer);
}

EAPI Eldbus_Signal_Handler *
eldbus_signal_handler_add(Eldbus_Connection *conn, const char *sender, const char *path, const char *interface, const char *member, Eldbus_Signal_Cb cb, const void *cb_data)
{
   Eldbus_Signal_Handler *sh;
   sh = _eldbus_signal_handler_add(conn, sender, path, interface, member, cb, cb_data);
   EINA_SAFETY_ON_NULL_RETURN_VAL(sh, NULL);
   eldbus_connection_free_cb_add(conn, _on_connection_free, sh);
   eldbus_signal_handler_free_cb_add(sh, _on_handler_of_conn_free, conn);
   return sh;
}

/**
 * @internal
 * @brief Core logic for adding a new signal handler.
 * This function creates and initializes an Eldbus_Signal_Handler structure,
 * constructs the D-Bus match rule, and registers it with the D-Bus daemon.
 * It does not set up the cross-referencing free callbacks between the
 * connection and the signal handler; that is handled by the public API
 * eldbus_signal_handler_add().
 *
 * @param conn The Eldbus_Connection.
 * @param sender The sender's bus name (or NULL for any).
 * @param path The object path (or NULL for any).
 * @param interface The interface name (or NULL for any).
 * @param member The signal name (or NULL for any).
 * @param cb The callback function to invoke when the signal is received.
 * @param cb_data User data to pass to the callback.
 * @return A new Eldbus_Signal_Handler on success, or NULL on failure.
 */
Eldbus_Signal_Handler *
_eldbus_signal_handler_add(Eldbus_Connection *conn, const char *sender, const char *path, const char *interface, const char *member, Eldbus_Signal_Cb cb, const void *cb_data)
{
   Eldbus_Signal_Handler *sh;
   Eina_Strbuf *match;

   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);
   DBG("conn=%p, sender=%s, path=%s, interface=%s, member=%s, cb=%p %p",
       conn, sender, path, interface, member, cb, cb_data);

   sh = calloc(1, sizeof(Eldbus_Signal_Handler));
   EINA_SAFETY_ON_NULL_RETURN_VAL(sh, NULL);

   match = eina_strbuf_new();
   EINA_SAFETY_ON_NULL_GOTO(match, cleanup_create_strbuf);
   eina_strbuf_append(match, "type='signal'");
   _match_append(match, "sender", sender);
   _match_append(match, "path", path);
   _match_append(match, "interface", interface);
   _match_append(match, "member", member);

   dbus_bus_add_match(conn->dbus_conn, eina_strbuf_string_get(match), NULL);

   if (sender)
     {
        sh->bus = eldbus_connection_name_get(conn, sender);
        if (!sh->bus) goto cleanup;
        eldbus_connection_name_ref(sh->bus);
     }

   sh->cb = cb;
   sh->cb_data = cb_data;
   sh->conn = conn;
   sh->interface = eina_stringshare_add(interface);
   sh->member = eina_stringshare_add(member);
   sh->path = eina_stringshare_add(path);
   sh->sender = eina_stringshare_add(sender);
   sh->match = match;
   sh->refcount = 1;
   sh->dangling = EINA_FALSE;
   sh->state_args = eina_inlist_sorted_state_new();
   EINA_MAGIC_SET(sh, ELDBUS_SIGNAL_HANDLER_MAGIC);

   eldbus_connection_signal_handler_add(conn, sh);
   return sh;

cleanup:
   eina_strbuf_free(match);
cleanup_create_strbuf:
   free(sh);

   return NULL;
}

static void
_eldbus_signal_handler_clean(Eldbus_Signal_Handler *handler)
{
   DBusError err;

   if (handler->dangling) return;
   DBG("clean handler=%p path=%p cb=%p", handler, handler->path, handler->cb);
   dbus_error_init(&err);
   dbus_bus_remove_match(handler->conn->dbus_conn,
                         eina_strbuf_string_get(handler->match), NULL);
   handler->dangling = EINA_TRUE;
}

static void
_eldbus_signal_handler_del(Eldbus_Signal_Handler *handler)
{
   Eina_Inlist *list;
   Signal_Argument *arg;
   DBG("handler %p, refcount=%d, conn=%p %s",
       handler, handler->refcount, handler->conn, handler->sender);
   eldbus_cbs_free_dispatch(&(handler->cbs_free), handler);
   eldbus_connection_signal_handler_del(handler->conn, handler);
   EINA_MAGIC_SET(handler, EINA_MAGIC_NONE);

   /* after cbs_free dispatch these shouldn't exit, error if they do */

   eina_stringshare_replace(&handler->sender, NULL);
   eina_stringshare_replace(&handler->path, NULL);
   eina_stringshare_replace(&handler->interface, NULL);
   eina_stringshare_replace(&handler->member, NULL);
   eina_strbuf_free(handler->match);
   EINA_INLIST_FOREACH_SAFE(handler->args, list, arg)
     {
        eina_stringshare_replace(&arg->value, NULL);
        free(arg);
     }
   eina_inlist_sorted_state_free(handler->state_args);

   if (handler->bus)
     eldbus_connection_name_unref(handler->conn, handler->bus);
   free(handler);
}

EAPI Eldbus_Signal_Handler *
eldbus_signal_handler_ref(Eldbus_Signal_Handler *handler)
{
   ELDBUS_SIGNAL_HANDLER_CHECK_RETVAL(handler, NULL);
   DBG("handler=%p, pre-refcount=%d, match=%s",
       handler, handler->refcount, eina_strbuf_string_get(handler->match));
   handler->refcount++;
   return handler;
}

EAPI void
eldbus_signal_handler_unref(Eldbus_Signal_Handler *handler)
{
   ELDBUS_SIGNAL_HANDLER_CHECK(handler);
   DBG("handler=%p, pre-refcount=%d, match=%s",
       handler, handler->refcount, eina_strbuf_string_get(handler->match));
   handler->refcount--;
   if (handler->refcount > 0) return;

   _eldbus_signal_handler_clean(handler);
   _eldbus_signal_handler_del(handler);
}

EAPI void
eldbus_signal_handler_del(Eldbus_Signal_Handler *handler)
{
   ELDBUS_SIGNAL_HANDLER_CHECK(handler);
   _eldbus_signal_handler_clean(handler);
   eldbus_signal_handler_unref(handler);
}

EAPI void
eldbus_signal_handler_free_cb_add(Eldbus_Signal_Handler *handler, Eldbus_Free_Cb cb, const void *data)
{
   ELDBUS_SIGNAL_HANDLER_CHECK(handler);
   EINA_SAFETY_ON_NULL_RETURN(cb);
   handler->cbs_free = eldbus_cbs_free_add(handler->cbs_free, cb, data);
}

EAPI void
eldbus_signal_handler_free_cb_del(Eldbus_Signal_Handler *handler, Eldbus_Free_Cb cb, const void *data)
{
   ELDBUS_SIGNAL_HANDLER_CHECK(handler);
   EINA_SAFETY_ON_NULL_RETURN(cb);
   handler->cbs_free = eldbus_cbs_free_del(handler->cbs_free, cb, data);
}

EAPI const char *
eldbus_signal_handler_sender_get(const Eldbus_Signal_Handler *handler)
{
   ELDBUS_SIGNAL_HANDLER_CHECK_RETVAL(handler, NULL);
   return handler->sender;
}

EAPI const char *
eldbus_signal_handler_path_get(const Eldbus_Signal_Handler *handler)
{
   ELDBUS_SIGNAL_HANDLER_CHECK_RETVAL(handler, NULL);
   return handler->path;
}

EAPI const char *
eldbus_signal_handler_interface_get(const Eldbus_Signal_Handler *handler)
{
   ELDBUS_SIGNAL_HANDLER_CHECK_RETVAL(handler, NULL);
   return handler->interface;
}

EAPI const char *
eldbus_signal_handler_member_get(const Eldbus_Signal_Handler *handler)
{
   ELDBUS_SIGNAL_HANDLER_CHECK_RETVAL(handler, NULL);
   return handler->member;
}

EAPI const char *
eldbus_signal_handler_match_get(const Eldbus_Signal_Handler *handler)
{
   ELDBUS_SIGNAL_HANDLER_CHECK_RETVAL(handler, NULL);
   return eina_strbuf_string_get(handler->match);
}

EAPI Eldbus_Connection *
eldbus_signal_handler_connection_get(const Eldbus_Signal_Handler *handler)
{
   ELDBUS_SIGNAL_HANDLER_CHECK_RETVAL(handler, NULL);
   return handler->conn;
}
