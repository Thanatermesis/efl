#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#include <Eina.h>

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"
#include "Ecore_Con_Eet.h"

#define ECORE_CON_EET_RAW_MAGIC 0xDEAD007
#define ECORE_CON_EET_DATA_KEY "ecore_con_eet_data_key"

/**
 * @brief Opaque type for base Ecore_Con_Eet data.
 */
typedef struct _Ecore_Con_Eet_Base_Data Ecore_Con_Eet_Base_Data;
/**
 * @brief Opaque type for Ecore_Con_Eet server object data.
 */
typedef struct _Ecore_Con_Eet_Server_Obj_Data Ecore_Con_Eet_Server_Obj_Data;
/**
 * @brief Opaque type for Ecore_Con_Eet client object data.
 */
typedef struct _Ecore_Con_Eet_Client_Obj_Data Ecore_Con_Eet_Client_Obj_Data;
/**
 * @internal
 * @brief Structure to hold Ecore_Con_Eet data callback information.
 */
typedef struct _Ecore_Con_Eet_Data     Ecore_Con_Eet_Data;
/**
 * @internal
 * @brief Structure to hold Ecore_Con_Eet raw data callback information.
 */
typedef struct _Ecore_Con_Eet_Raw_Data Ecore_Con_Eet_Raw_Data;
/**
 * @brief Structure to hold Ecore_Con_Eet client callback information.
 */
typedef struct _Ecore_Con_Eet_Client   Ecore_Con_Eet_Client;
/**
 * @brief Structure to hold Ecore_Con_Eet server callback information.
 */
typedef struct _Ecore_Con_Eet_Server   Ecore_Con_Eet_Server;

/**
 * @internal
 * @brief Internal data structure for an Ecore_Con_Eet server object.
 * This structure holds lists of client connections and callbacks for client
 * connect/disconnect events, along with event handlers for these events.
 */
struct _Ecore_Con_Eet_Server_Obj_Data
{
   Eina_List *connections;
   Eina_List *client_connect_callbacks;
   Eina_List *client_disconnect_callbacks;

   Ecore_Event_Handler *handler_add; /**< Event handler for client add events. */
   Ecore_Event_Handler *handler_del; /**< Event handler for client delete events. */
   Ecore_Event_Handler *handler_data; /**< Event handler for client data events. */
};

/**
 * @internal
 * @brief Internal data structure for an Ecore_Con_Eet client object.
 * This structure holds the connection reply, callbacks for server
 * connect/disconnect events, and event handlers for these events.
 */
struct _Ecore_Con_Eet_Client_Obj_Data
{
   Ecore_Con_Reply *r; /**< The connection reply object. */
   Eina_List       *server_connect_callbacks;
   Eina_List       *server_disconnect_callbacks;

   Ecore_Event_Handler *handler_add; /**< Event handler for server add events. */
   Ecore_Event_Handler *handler_del; /**< Event handler for server delete events. */
   Ecore_Event_Handler *handler_data; /**< Event handler for server data events. */
};

/**
 * @internal
 * @brief Represents a reply context for an Ecore_Con_Eet connection.
 * This structure manages the state of an Eet connection, including buffering
 * for raw data and handling of incoming/outgoing Eet messages.
 */
struct _Ecore_Con_Reply
{
   Ecore_Con_Eet          *ece; /**< The Ecore_Con_Eet object associated with this reply. */
   Ecore_Con_Client       *client; /**< The Ecore_Con_Client if this is a server-side reply, NULL otherwise. */

   Eet_Connection         *econn; /**< The Eet connection object. */

   char                   *buffer_section; /**< Name of the current section for raw data buffering. */
   unsigned char          *buffer; /**< Buffer for incoming raw data. */
   unsigned int            buffer_length; /**< Total length of the raw data buffer. */
   unsigned int            buffer_current; /**< Current amount of data in the raw data buffer. */
   Ecore_Con_Eet_Raw_Data *buffer_handler; /**< Handler for the currently buffered raw data. */
};

/**
 * @internal
 * @brief Holds information for a registered Eet data callback.
 */
struct _Ecore_Con_Eet_Data
{
   Ecore_Con_Eet_Data_Cb func; /**< The callback function to be called when data of 'name' type is received. */
   const char           *name; /**< The name (type) of the data this callback handles. */
   const void           *data; /**< User data to be passed to the callback function. */
};

/**
 * @internal
 * @brief Holds information for a registered raw Eet data callback.
 */
struct _Ecore_Con_Eet_Raw_Data
{
   Ecore_Con_Eet_Raw_Data_Cb func; /**< The callback function for raw data. */
   const char               *name; /**< The protocol name for this raw data callback. */
   const void               *data; /**< User data for the raw data callback. */
};

/**
 * @internal
 * @brief Holds information for a client connect/disconnect callback.
 */
struct _Ecore_Con_Eet_Client
{
   Ecore_Con_Eet_Client_Cb func; /**< The callback function. */
   const void             *data; /**< User data for the callback. */
};

/**
 * @internal
 * @brief Holds information for a server connect/disconnect callback.
 */
struct _Ecore_Con_Eet_Server
{
   Ecore_Con_Eet_Server_Cb func; /**< The callback function. */
   const void             *data; /**< User data for the callback. */
};

/**
 * @internal
 * @brief Base data structure for Ecore_Con_Eet objects.
 * This structure contains the Ecore_Con_Server instance, Eet data descriptors,
 * and hash tables for managing data and raw data callbacks.
 */
struct _Ecore_Con_Eet_Base_Data
{
   Ecore_Con_Server    *server; /**< The Ecore_Con_Server this Eet connection is associated with. */

   Eet_Data_Descriptor *edd; /**< Main Eet data descriptor for encoding/decoding Ecore_Con_Eet_Protocol. */
   Eet_Data_Descriptor *matching; /**< Eet data descriptor for matching specific data types within the protocol. */

   Eina_Hash           *data_callbacks; /**< Hash table of registered data callbacks (Ecore_Con_Eet_Data). Key: stringshared name. */
   Eina_Hash           *raw_data_callbacks; /**< Hash table of registered raw data callbacks (Ecore_Con_Eet_Raw_Data). Key: string name. */
};

/**
 * @internal
 * @brief Frees an Ecore_Con_Eet_Data structure.
 * Used as a callback for eina_hash_free.
 * @param data Pointer to the Ecore_Con_Eet_Data to free.
 */
static void
_ecore_con_eet_data_free(void *data)
{
   Ecore_Con_Eet_Data *eced = data;

   eina_stringshare_del(eced->name);
   free(eced);
}

/**
 * @internal
 * @brief Frees an Ecore_Con_Eet_Raw_Data structure.
 * Used as a callback for eina_hash_free.
 * @param data Pointer to the Ecore_Con_Eet_Raw_Data to free.
 */
static void
_ecore_con_eet_raw_data_free(void *data)
{
   Ecore_Con_Eet_Raw_Data *eced = data;

   eina_stringshare_del(eced->name);
   free(eced);
}

/**
 * @internal
 * @brief Cleans up resources associated with an Ecore_Con_Reply's buffer.
 * This function frees the buffer and resets buffer-related fields if a
 * buffer_handler was active. It also frees the buffer_section string.
 * @param n The Ecore_Con_Reply to clean up.
 */
static void
_ecore_con_eet_reply_cleanup(Ecore_Con_Reply *n)
{
   if (n->buffer_handler) free(n->buffer);
   n->buffer = NULL;
   n->buffer_handler = NULL;
   free(n->buffer_section);
   n->buffer_section = NULL;
}

/**
 * @internal
 * @brief Internal structure used for Eet serialization of typed data.
 * This structure wraps the actual data with its type name, allowing for
 * dynamic dispatch of data handling based on the type.
 */
typedef struct _Ecore_Con_Eet_Protocol Ecore_Con_Eet_Protocol;
/**
 * @internal
 * @brief Defines the protocol structure for Eet communication.
 * It contains the type of the data and a pointer to the data itself.
 */
struct _Ecore_Con_Eet_Protocol
{
   const char *type; /**< The string identifier for the data type. */
   void       *data; /**< Pointer to the actual data. */
};

/**
 * @internal
 * @brief Eet callback to get the type name from an Ecore_Con_Eet_Protocol structure.
 * @param data Pointer to the Ecore_Con_Eet_Protocol structure.
 * @param unknow EINA_UNUSED.
 * @return The type string.
 */
static const char *
_ecore_con_eet_data_type_get(const void *data, Eina_Bool *unknow EINA_UNUSED)
{
   const Ecore_Con_Eet_Protocol *p = data;

   return p->type;
}

/**
 * @internal
 * @brief Eet callback to set the type name in an Ecore_Con_Eet_Protocol structure.
 * @param type The type string to set.
 * @param data Pointer to the Ecore_Con_Eet_Protocol structure.
 * @param unknow EINA_UNUSED.
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_ecore_con_eet_data_type_set(const char *type, void *data, Eina_Bool unknow EINA_UNUSED)
{
   Ecore_Con_Eet_Protocol *p = data;

   p->type = type;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Sets up the Eet data descriptors for Ecore_Con_Eet_Protocol.
 * This initializes `ece->edd` for the base protocol structure and `ece->matching`
 * for handling variant data types within the protocol.
 * @param ece Pointer to the Ecore_Con_Eet_Base_Data containing the descriptors.
 */
static void
_ecore_con_eet_data_descriptor_setup(Ecore_Con_Eet_Base_Data *ece)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ecore_Con_Eet_Protocol);
   ece->edd = eet_data_descriptor_stream_new(&eddc);

   eddc.version = EET_DATA_DESCRIPTOR_CLASS_VERSION;
   eddc.func.type_get = _ecore_con_eet_data_type_get;
   eddc.func.type_set = _ecore_con_eet_data_type_set;
   ece->matching = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_VARIANT(ece->edd, Ecore_Con_Eet_Protocol, "data", data, type, ece->matching);
}

/* Dealing with a server listening to connection */

/**
 * @internal
 * @brief Eet connection callback for reading data.
 * This function is called by the Eet library when data is received. It decodes
 * the data using the Eet descriptor, finds the appropriate callback based on
 * the data type, and invokes it.
 * @param eet_data Pointer to the received Eet data.
 * @param size Size of the received data.
 * @param user_data User data, expected to be an Ecore_Con_Reply pointer.
 * @return EINA_TRUE to continue processing, EINA_FALSE on critical error (not used here).
 */
static Eina_Bool
_ecore_con_eet_read_cb(const void *eet_data, size_t size, void *user_data)
{
   Ecore_Con_Reply *n = user_data;
   Ecore_Con_Eet_Protocol *protocol;
   Ecore_Con_Eet_Data *cb;
   Ecore_Con_Eet_Base_Data *ece_data = efl_data_scope_get(n->ece, ECORE_CON_EET_BASE_CLASS);

   protocol = eet_data_descriptor_decode(ece_data->edd, eet_data, size);
   if (!protocol) return EINA_TRUE;

   cb = eina_hash_find(ece_data->data_callbacks, protocol->type);
   if (!cb) return EINA_TRUE;  /* Should I report unknow protocol communication ? */

   cb->func((void *)cb->data, n, cb->name, protocol->data);

   eina_stringshare_del(protocol->type);
   free(protocol);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Eet connection callback for writing data from the server to a client.
 * This function is called by the Eet library when it needs to send data.
 * It uses ecore_con_client_send to transmit the data.
 * @param data Pointer to the data to send.
 * @param size Size of the data to send.
 * @param user_data User data, expected to be an Ecore_Con_Reply pointer.
 * @return EINA_TRUE if send was successful, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_con_eet_server_write_cb(const void *data, size_t size, void *user_data)
{
   Ecore_Con_Reply *n = user_data;

   if (ecore_con_client_send(n->client, data, size) != (int)size)
     return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Eet connection callback for writing data from the client to a server.
 * This function is called by the Eet library when it needs to send data.
 * It uses ecore_con_server_send to transmit the data.
 * @param data Pointer to the data to send.
 * @param size Size of the data to send.
 * @param user_data User data, expected to be an Ecore_Con_Reply pointer.
 * @return EINA_TRUE if send was successful, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_con_eet_client_write_cb(const void *data, size_t size, void *user_data)
{
   Ecore_Con_Reply *n = user_data;
   Ecore_Con_Eet_Base_Data *ece_data = efl_data_scope_get(n->ece, ECORE_CON_EET_BASE_CLASS);

   if (ecore_con_server_send(ece_data->server, data, size) != (int)size)
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Event handler for ECORE_CON_EVENT_CLIENT_ADD on the server side.
 * This function is called when a new client connects to the server. It sets up
 * an Ecore_Con_Reply and an Eet_Connection for the new client, and calls
 * registered client connect callbacks.
 * @param data User data, expected to be an Ecore_Con_Eet object (server instance).
 * @param type EINA_UNUSED, the type of the event.
 * @param ev The Ecore_Con_Event_Client_Add event structure.
 * @return ECORE_CALLBACK_PASS_ON (EINA_TRUE) to continue event processing.
 */
static Eina_Bool
_ecore_con_eet_server_connected(void *data, int type EINA_UNUSED, Ecore_Con_Event_Client_Add *ev)
{
   Ecore_Con_Eet_Client *ecec;
   Eina_List *ll;
   Ecore_Con_Reply *n;
   Ecore_Con_Eet *ece_obj = data;
   Ecore_Con_Eet_Base_Data *base_data = efl_data_scope_get(ece_obj, ECORE_CON_EET_BASE_CLASS);
   Ecore_Con_Eet_Server_Obj_Data *r = efl_data_scope_get(ece_obj, ECORE_CON_EET_SERVER_OBJ_CLASS);

   if (ecore_con_client_server_get(ev->client) != base_data->server)
     return EINA_TRUE;

   n = calloc(1, sizeof (Ecore_Con_Reply));
   if (!n) return EINA_TRUE;

   n->client = ev->client;
   n->ece = ece_obj;
   n->econn = eet_connection_new(_ecore_con_eet_read_cb, _ecore_con_eet_server_write_cb, n);
   ecore_con_client_data_set(n->client, n);

   EINA_LIST_FOREACH(r->client_connect_callbacks, ll, ecec)
     if (!ecec->func((void *)ecec->data, n, n->client))
       {
          eet_connection_close(n->econn, NULL);
          free(n);
          return EINA_TRUE;
       }

   r->connections = eina_list_append(r->connections, n);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Event handler for ECORE_CON_EVENT_CLIENT_DEL on the server side.
 * This function is called when a client disconnects from the server. It cleans
 * up the Ecore_Con_Reply and Eet_Connection associated with the client, and
 * calls registered client disconnect callbacks.
 * @param data User data, expected to be an Ecore_Con_Eet object (server instance).
 * @param type EINA_UNUSED, the type of the event.
 * @param ev The Ecore_Con_Event_Client_Del event structure.
 * @return ECORE_CALLBACK_PASS_ON (EINA_TRUE) to continue event processing.
 */
static Eina_Bool
_ecore_con_eet_server_disconnected(void *data, int type EINA_UNUSED, Ecore_Con_Event_Client_Del *ev)
{
   Ecore_Con_Eet *ece_obj = data;
   Ecore_Con_Eet_Base_Data *base_data = efl_data_scope_get(ece_obj, ECORE_CON_EET_BASE_CLASS);
   Ecore_Con_Eet_Server_Obj_Data *r = efl_data_scope_get(ece_obj, ECORE_CON_EET_SERVER_OBJ_CLASS);
   Ecore_Con_Reply *n;
   Eina_List *l;

   if (ecore_con_client_server_get(ev->client) != base_data->server)
     return EINA_TRUE;

   EINA_LIST_FOREACH(r->connections, l, n)
     if (n->client == ev->client)
       {
          Ecore_Con_Eet_Client *ecec;
          Eina_List *ll;

          EINA_LIST_FOREACH(r->client_disconnect_callbacks, ll, ecec)
            ecec->func((void *)ecec->data, n, n->client);

          eet_connection_close(n->econn, NULL);
          free(n);
          r->connections = eina_list_remove_list(r->connections, l);
          return EINA_TRUE;
       }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Appends incoming raw data to the reply's buffer and processes it when complete.
 * If the buffer is full after appending, it calls the registered raw data handler.
 * @param n The Ecore_Con_Reply context.
 * @param data Pointer to the chunk of raw data received.
 * @param size Size of the data chunk.
 */
static void
_ecore_con_eet_raw_data_push(Ecore_Con_Reply *n, void *data, int size)
{
   if (n->buffer_handler)
     memcpy(n->buffer + n->buffer_current, data, size);
   n->buffer_current += size;

   if (n->buffer_current == n->buffer_length)
     {
        if (n->buffer_handler)
          n->buffer_handler->func((void *)n->buffer_handler->data, n, n->buffer_handler->name, n->buffer_section, n->buffer, n->buffer_length);
        _ecore_con_eet_reply_cleanup(n);
     }
}

/**
 * @internal
 * @brief Processes incoming data for an Eet connection.
 * This function handles both Eet-formatted messages and raw data streams.
 * For raw data, it parses a header (magic number, lengths) to determine
 * the protocol, section, and total data length, then buffers the incoming
 * data until complete. For Eet messages, it passes them to eet_connection_received.
 * @param n The Ecore_Con_Reply context.
 * @param data Pointer to the received data.
 * @param size Size of the received data.
 */
static void
_ecore_con_eet_data(Ecore_Con_Reply *n, void *data, unsigned int size)
{
   /* FIXME: Enforce detection of attack and kill connection on that case */
   if (n->buffer)
     {
        if (n->buffer_current + size > n->buffer_length)
          {
             _ecore_con_eet_reply_cleanup(n);
             return;
          }

        _ecore_con_eet_raw_data_push(n, data, size);
        return;
     }
   else if (eet_connection_empty(n->econn) && size > (int)(4 * sizeof (unsigned int) + 2))
     {
        unsigned int *tmp = data;
        size -= 4 * sizeof (unsigned int);

        if (eina_ntohl(tmp[0]) == ECORE_CON_EET_RAW_MAGIC)
          {
             unsigned int protocol_length = eina_ntohl(tmp[1]);
             unsigned int section_length = eina_ntohl(tmp[2]);
             unsigned int data_length = eina_ntohl(tmp[3]);

             if (protocol_length > 1 && section_length > 1 && protocol_length + section_length <= size && data_length < 10 * 1024 * 1024)
               {
                  char *buffer = (char *)&tmp[4];
                  char *protocol;
                  char *section;
                  Ecore_Con_Eet_Base_Data *eceb_data = efl_data_scope_get(n->ece,ECORE_CON_EET_BASE_CLASS);

                  protocol = buffer;
                  section = buffer + protocol_length;

                  if (protocol[protocol_length - 1] == '\0' &&
                      section[section_length - 1] == '\0')
                    {
                       size -= protocol_length + section_length;
                       buffer = section + section_length;

                       n->buffer_handler = eina_hash_find(eceb_data->raw_data_callbacks, protocol);
                       n->buffer_section = strdup(section);
                       n->buffer_length = data_length;
                       n->buffer_current = 0;
                       if (n->buffer_handler)
                         n->buffer = malloc(sizeof (unsigned char) * data_length);
                       else
                         n->buffer = (void *)1;
                       if (n->buffer)
                         {
                            _ecore_con_eet_raw_data_push(n, buffer, size);
                            return;
                         }
                       _ecore_con_eet_reply_cleanup(n);

                       size += protocol_length + section_length;
                    }
               }
          }

        size += 4 * sizeof (unsigned int);
     }

   eet_connection_received(n->econn, data, size);
}

/**
 * @internal
 * @brief Event handler for ECORE_CON_EVENT_CLIENT_DATA on the server side.
 * This function is called when data is received from a client. It retrieves
 * the Ecore_Con_Reply associated with the client and passes the data to
 * _ecore_con_eet_data for processing.
 * @param data User data, expected to be an Ecore_Con_Eet object (server instance).
 * @param type EINA_UNUSED, the type of the event.
 * @param ev The Ecore_Con_Event_Client_Data event structure.
 * @return ECORE_CALLBACK_PASS_ON (EINA_TRUE) to continue event processing.
 */
static Eina_Bool
_ecore_con_eet_server_data(void *data, int type EINA_UNUSED, Ecore_Con_Event_Client_Data *ev)
{
   Ecore_Con_Eet *ece_obj = data;
   Ecore_Con_Eet_Base_Data *r = efl_data_scope_get(ece_obj, ECORE_CON_EET_BASE_CLASS);
   Ecore_Con_Reply *n;

   if (ecore_con_client_server_get(ev->client) != r->server)
     return EINA_TRUE;

   n = ecore_con_client_data_get(ev->client);

   efl_ref(ece_obj);
   _ecore_con_eet_data(n, ev->data, ev->size);
   efl_unref(ece_obj);

   return EINA_TRUE;
}

/* Dealing connection to a server */

/**
 * @internal
 * @brief Event handler for ECORE_CON_EVENT_SERVER_ADD on the client side.
 * This function is called when the client successfully connects to a server.
 * It sets up an Ecore_Con_Reply and an Eet_Connection for communication with
 * the server, and calls registered server connect callbacks.
 * @param data User data, expected to be an Ecore_Con_Eet object (client instance).
 * @param type EINA_UNUSED, the type of the event.
 * @param ev The Ecore_Con_Event_Server_Add event structure.
 * @return ECORE_CALLBACK_PASS_ON (EINA_TRUE) to continue event processing.
 */
static Eina_Bool
_ecore_con_eet_client_connected(void *data, int type EINA_UNUSED, Ecore_Con_Event_Server_Add *ev)
{
   Ecore_Con_Eet_Server *eces;
   Ecore_Con_Eet *ece_obj = data;
   Ecore_Con_Eet_Base_Data *base_data = efl_data_scope_get(ece_obj, ECORE_CON_EET_BASE_CLASS);
   Ecore_Con_Eet_Client_Obj_Data *r = efl_data_scope_get(ece_obj, ECORE_CON_EET_CLIENT_OBJ_CLASS);
   Ecore_Con_Reply *n;
   Eina_List *ll;

   /* Client did connect */
   if (base_data->server != ev->server) return EINA_TRUE;
   if (r->r) return EINA_TRUE;

   n = calloc(1, sizeof (Ecore_Con_Reply));
   if (!n) return EINA_TRUE;

   n->client = NULL;
   n->ece = ece_obj;
   n->econn = eet_connection_new(_ecore_con_eet_read_cb, _ecore_con_eet_client_write_cb, n);

   EINA_LIST_FOREACH(r->server_connect_callbacks, ll, eces)
     {
        Ecore_Con_Eet_Base_Data *temp = efl_data_scope_get(n->ece, ECORE_CON_EET_BASE_CLASS);
        if (!eces->func((void *)eces->data, n, temp->server))
          {
             eet_connection_close(n->econn, NULL);
             free(n);
             return EINA_TRUE;
          }
     }

   r->r = n;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Event handler for ECORE_CON_EVENT_SERVER_DEL on the client side.
 * This function is called when the client is disconnected from the server.
 * It cleans up the Ecore_Con_Reply and Eet_Connection, and calls registered
 * server disconnect callbacks.
 * @param data User data, expected to be an Ecore_Con_Eet object (client instance).
 * @param type EINA_UNUSED, the type of the event.
 * @param ev The Ecore_Con_Event_Server_Del event structure.
 * @return ECORE_CALLBACK_PASS_ON (EINA_TRUE) to continue event processing.
 */
static Eina_Bool
_ecore_con_eet_client_disconnected(void *data, int type EINA_UNUSED, Ecore_Con_Event_Server_Del *ev)
{
   Ecore_Con_Eet *ece_obj = data;
   Ecore_Con_Eet_Base_Data *base_data = efl_data_scope_get(ece_obj, ECORE_CON_EET_BASE_CLASS);
   Ecore_Con_Eet_Client_Obj_Data *r = efl_data_scope_get(ece_obj, ECORE_CON_EET_CLIENT_OBJ_CLASS);
   Ecore_Con_Eet_Server *eces;
   Eina_List *ll;

   if (base_data->server != ev->server) return EINA_TRUE;
   if (!r->r) return EINA_TRUE;

   /* Client disconnected */
   EINA_LIST_FOREACH(r->server_disconnect_callbacks, ll, eces)
     eces->func((void *)eces->data, r->r, base_data->server);

   eet_connection_close(r->r->econn, NULL);
   free(r->r);
   r->r = NULL;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Event handler for ECORE_CON_EVENT_SERVER_DATA on the client side.
 * This function is called when data is received from the server. It retrieves
 * the Ecore_Con_Reply and passes the data to _ecore_con_eet_data for processing.
 * @param data User data, expected to be an Ecore_Con_Eet object (client instance).
 * @param type EINA_UNUSED, the type of the event.
 * @param ev The Ecore_Con_Event_Server_Data event structure.
 * @return ECORE_CALLBACK_PASS_ON (EINA_TRUE) to continue event processing.
 */
static Eina_Bool
_ecore_con_eet_client_data(void *data, int type EINA_UNUSED, Ecore_Con_Event_Server_Data *ev)
{
   Ecore_Con_Eet *ece_obj = data;
   Ecore_Con_Eet_Base_Data *base_data = efl_data_scope_get(ece_obj, ECORE_CON_EET_BASE_CLASS);
   Ecore_Con_Eet_Client_Obj_Data *r = efl_data_scope_get(ece_obj, ECORE_CON_EET_CLIENT_OBJ_CLASS);

   if (base_data->server != ev->server) return EINA_TRUE;
   if (!r->r) return EINA_TRUE;

   /* Got some data */
   efl_ref(ece_obj);
   _ecore_con_eet_data(r->r, ev->data, ev->size);
   efl_unref(ece_obj);

   return EINA_TRUE;
}

/*************
 * Generated API
 */

/**
 * @internal
 * @brief Implements ecore_con_eet_base_data_callback_set.
 * Registers a callback function to be invoked when Eet data of a specific type (name) is received.
 * @param obj The Ecore_Con_Eet object.
 * @param pd The private data of the Ecore_Con_Eet_Base object.
 * @param name The name identifying the data type for this callback.
 * @param func The callback function to execute.
 * @param data User-provided data to pass to the callback function.
 */
EOLIAN static void
_ecore_con_eet_base_data_callback_set(Eo *obj EINA_UNUSED, Ecore_Con_Eet_Base_Data *pd, const char *name, Ecore_Con_Eet_Data_Cb func, const void *data)
{
   Ecore_Con_Eet_Data *eced;

   eced = calloc(1, sizeof (Ecore_Con_Eet_Data));
   if (!eced) return;

   eced->func = func;
   eced->data = data;
   eced->name = eina_stringshare_add(name);

   eina_hash_direct_add(pd->data_callbacks, eced->name, eced);
}

/**
 * @internal
 * @brief Implements ecore_con_eet_base_raw_data_callback_set.
 * Registers a callback function for handling raw data streams associated with a protocol name.
 * @param obj The Ecore_Con_Eet object.
 * @param pd The private data of the Ecore_Con_Eet_Base object.
 * @param name The protocol name for which this raw data callback is registered.
 * @param func The callback function to execute.
 * @param data User-provided data to pass to the callback function.
 */
EOLIAN static void
_ecore_con_eet_base_raw_data_callback_set(Eo *obj EINA_UNUSED, Ecore_Con_Eet_Base_Data *pd, const char *name, Ecore_Con_Eet_Raw_Data_Cb func, const void *data)
{
   Ecore_Con_Eet_Raw_Data *ecerd;

   ecerd = calloc(1, sizeof (Ecore_Con_Eet_Raw_Data));
   if (!ecerd) return;

   ecerd->func = func;
   ecerd->data = data;
   ecerd->name = eina_stringshare_add(name);

   eina_hash_direct_add(pd->raw_data_callbacks, ecerd->name, ecerd);
}

/**
 * @internal
 * @brief Implements ecore_con_eet_base_data_callback_del.
 * Deletes a previously registered data callback.
 * @param obj The Ecore_Con_Eet object.
 * @param pd The private data of the Ecore_Con_Eet_Base object.
 * @param name The name of the data callback to delete.
 */
EOLIAN static void
_ecore_con_eet_base_data_callback_del(Eo *obj EINA_UNUSED, Ecore_Con_Eet_Base_Data *pd, const char *name)
{
   eina_hash_del(pd->data_callbacks, name, NULL);
}

/**
 * @internal
 * @brief Implements ecore_con_eet_base_raw_data_callback_del.
 * Deletes a previously registered raw data callback. If this callback is currently
 * handling an in-progress raw data transfer on a client, it cleans up the buffer.
 * @param obj The Ecore_Con_Eet object.
 * @param pd The private data of the Ecore_Con_Eet_Base object.
 * @param name The name of the raw data callback to delete.
 */
EOLIAN static void
_ecore_con_eet_base_raw_data_callback_del(Eo *obj, Ecore_Con_Eet_Base_Data *pd, const char *name)
{
   Ecore_Con_Eet_Client_Obj_Data *eced = efl_data_scope_get(obj, ECORE_CON_EET_CLIENT_OBJ_CLASS);

   if (efl_isa(obj, ECORE_CON_EET_CLIENT_OBJ_CLASS) &&
       eced->r->buffer_handler &&
       !strcmp(eced->r->buffer_handler->name, name))
     {
        eced->r->buffer_handler = NULL;
        free(eced->r->buffer);
        eced->r->buffer = (void *)1;
     }
   eina_hash_del(pd->raw_data_callbacks, name, NULL);
}

/**
 * @internal
 * @brief Implements ecore_con_eet_base_send.
 * Sends Eet-encoded data over the connection associated with the reply.
 * @param obj The Ecore_Con_Eet object.
 * @param pd The private data of the Ecore_Con_Eet_Base object.
 * @param reply The Ecore_Con_Reply context for the connection.
 * @param name The type name of the data being sent.
 * @param value Pointer to the data structure to send.
 */
EOLIAN static void
_ecore_con_eet_base_send(Eo *obj EINA_UNUSED, Ecore_Con_Eet_Base_Data *pd, Ecore_Con_Reply *reply, const char *name, void *value)
{
   Ecore_Con_Eet_Protocol protocol;

   if (!reply) return;

   protocol.type = name;
   protocol.data = value;

   eet_connection_send(reply->econn, pd->edd, &protocol, NULL);
}

/**
 * @internal
 * @brief Implements ecore_con_eet_base_raw_send.
 * Sends raw data over the connection. This involves sending a header packet
 * (magic, protocol name length, section name length, data length) followed by
 * the protocol name, section name, and then the actual binary data.
 * @param obj The Ecore_Con_Eet object.
 * @param pd The private data of the Ecore_Con_Eet_Base object.
 * @param reply The Ecore_Con_Reply context for the connection.
 * @param protocol_name Name of the protocol for this raw data.
 * @param section Name of the section within the protocol.
 * @param section_data Eina_Binbuf containing the raw data to send.
 */
EOLIAN static void
_ecore_con_eet_base_raw_send(Eo *obj EINA_UNUSED, Ecore_Con_Eet_Base_Data *pd, Ecore_Con_Reply *reply, const char *protocol_name, const char *section, Eina_Binbuf *section_data)
{
   unsigned int protocol[4];
   unsigned int protocol_length;
   unsigned int section_length;
   unsigned int size;
   unsigned int length = 0;
   const void *value = NULL;
   char *tmp;

   if (!reply) return;
   if (!protocol_name) return;
   if (!section) return;

   if (section_data)
     {
        length = eina_binbuf_length_get(section_data);
        value = eina_binbuf_string_get(section_data);
     }

   protocol_length = strlen(protocol_name) + 1;
   if (protocol_length == 1) return;
   section_length = strlen(section) + 1;

   protocol[0] = eina_htonl(ECORE_CON_EET_RAW_MAGIC);
   protocol[1] = eina_htonl(protocol_length);
   protocol[2] = eina_htonl(section_length);
   protocol[3] = eina_htonl(length);

   size = sizeof (protocol) + protocol_length + section_length;
   tmp = alloca(size);
   memcpy(tmp, protocol, sizeof (protocol));
   memcpy(tmp + sizeof (protocol), protocol_name, protocol_length);
   memcpy(tmp + sizeof (protocol) + protocol_length, section, section_length);

   if (reply->client)
     {
        ecore_con_client_send(reply->client, tmp, size);
        ecore_con_client_send(reply->client, value, length);
     }
   else
     {
        ecore_con_server_send(pd->server, tmp, size);
        ecore_con_server_send(pd->server, value, length);
     }
}

/**
 * @internal
 * @brief Implements ecore_con_eet_base_register.
 * Registers an Eet_Data_Descriptor for a specific data type name. This allows
 * Eet to encode/decode structures of this type when they are part of an
 * Ecore_Con_Eet_Protocol message.
 * @param obj The Ecore_Con_Eet object.
 * @param pd The private data of the Ecore_Con_Eet_Base object.
 * @param name The type name to associate with the Eet_Data_Descriptor.
 * @param edd The Eet_Data_Descriptor for the data type.
 */
EOLIAN static void
_ecore_con_eet_base_register(Eo *obj EINA_UNUSED, Ecore_Con_Eet_Base_Data *pd, const char *name, Eet_Data_Descriptor *edd)
{
   EET_DATA_DESCRIPTOR_ADD_MAPPING(pd->matching, name, edd);
}

/**
 * @internal
 * @brief Constructor for Ecore_Con_Eet_Server_Obj.
 * Initializes the object and sets up event handlers for client connect,
 * disconnect, and data events.
 * @param obj The Eo object to construct.
 * @param pd Private data for the Ecore_Con_Eet_Server_Obj.
 * @return The constructed Eo object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_ecore_con_eet_server_obj_efl_object_constructor(Eo *obj, Ecore_Con_Eet_Server_Obj_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, ECORE_CON_EET_SERVER_OBJ_CLASS));

   if (!obj) return NULL;

   pd->handler_add = ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_ADD,
                                             (Ecore_Event_Handler_Cb)_ecore_con_eet_server_connected, obj);
   pd->handler_del = ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DEL,
                                             (Ecore_Event_Handler_Cb)_ecore_con_eet_server_disconnected, obj);
   pd->handler_data = ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DATA,
                                              (Ecore_Event_Handler_Cb)_ecore_con_eet_server_data, obj);

   return obj;
}

/**
 * @internal
 * @brief Destructor for Ecore_Con_Eet_Server_Obj.
 * Cleans up resources, including freeing connection lists, callback lists,
 * and deleting event handlers.
 * @param obj The Eo object to destruct.
 * @param pd Private data for the Ecore_Con_Eet_Server_Obj.
 */
EOLIAN static void
_ecore_con_eet_server_obj_efl_object_destructor(Eo *obj, Ecore_Con_Eet_Server_Obj_Data *pd EINA_UNUSED)
{
   Ecore_Con_Reply *n;
   Ecore_Con_Eet_Client *c;

   EINA_LIST_FREE(pd->connections, n)
     {
        _ecore_con_eet_reply_cleanup(n);
        eet_connection_close(n->econn, NULL);
        free(n);
     }
   EINA_LIST_FREE(pd->client_connect_callbacks, c)
     free(c);
   EINA_LIST_FREE(pd->client_disconnect_callbacks, c)
     free(c);

   ecore_event_handler_del(pd->handler_add);
   ecore_event_handler_del(pd->handler_del);
   ecore_event_handler_del(pd->handler_data);

   efl_destructor(efl_super(obj, ECORE_CON_EET_SERVER_OBJ_CLASS));
}

/**
 * @internal
 * @brief Constructor for Ecore_Con_Eet_Client_Obj.
 * Initializes the object and sets up event handlers for server connect,
 * disconnect, and data events.
 * @param obj The Eo object to construct.
 * @param pd Private data for the Ecore_Con_Eet_Client_Obj.
 * @return The constructed Eo object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_ecore_con_eet_client_obj_efl_object_constructor(Eo *obj, Ecore_Con_Eet_Client_Obj_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, ECORE_CON_EET_CLIENT_OBJ_CLASS));

   if (!obj) return NULL;

   pd->handler_add = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ADD,
                                             (Ecore_Event_Handler_Cb)_ecore_con_eet_client_connected, obj);
   pd->handler_del = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL,
                                             (Ecore_Event_Handler_Cb)_ecore_con_eet_client_disconnected, obj);
   pd->handler_data = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA,
                                              (Ecore_Event_Handler_Cb)_ecore_con_eet_client_data, obj);

   return obj;
}

/**
 * @internal
 * @brief Destructor for Ecore_Con_Eet_Client_Obj.
 * Cleans up resources, including freeing the Ecore_Con_Reply, callback lists,
 * and deleting event handlers.
 * @param obj The Eo object to destruct.
 * @param pd Private data for the Ecore_Con_Eet_Client_Obj.
 */
EOLIAN static void
_ecore_con_eet_client_obj_efl_object_destructor(Eo *obj, Ecore_Con_Eet_Client_Obj_Data *pd EINA_UNUSED)
{
   Ecore_Con_Eet_Server *s;

   if (pd->r)
     {
        _ecore_con_eet_reply_cleanup(pd->r);
        eet_connection_close(pd->r->econn, NULL);
     }
   EINA_LIST_FREE(pd->server_connect_callbacks, s)
     free(s);
   EINA_LIST_FREE(pd->server_disconnect_callbacks, s)
     free(s);

   ecore_event_handler_del(pd->handler_add);
   ecore_event_handler_del(pd->handler_del);
   ecore_event_handler_del(pd->handler_data);

   efl_destructor(efl_super(obj, ECORE_CON_EET_CLIENT_OBJ_CLASS));
}

/**
 * @internal
 * @brief Constructor for Ecore_Con_Eet_Base.
 * Initializes hash tables for data and raw_data callbacks and sets up
 * the Eet data descriptors.
 * @param obj The Eo object to construct.
 * @param pd Private data for the Ecore_Con_Eet_Base.
 * @return The constructed Eo object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_ecore_con_eet_base_efl_object_constructor(Eo *obj, Ecore_Con_Eet_Base_Data *pd)
{
   obj = efl_constructor(efl_super(obj, ECORE_CON_EET_BASE_CLASS));

   if (!obj) return NULL;

   pd->data_callbacks = eina_hash_stringshared_new(_ecore_con_eet_data_free);
   pd->raw_data_callbacks = eina_hash_string_superfast_new(_ecore_con_eet_raw_data_free);

   _ecore_con_eet_data_descriptor_setup(pd);

   return obj;
}

/**
 * @internal
 * @brief Destructor for Ecore_Con_Eet_Base.
 * Frees Eet data descriptors and hash tables for callbacks.
 * @param obj The Eo object to destruct.
 * @param pd Private data for the Ecore_Con_Eet_Base.
 */
EOLIAN static void
_ecore_con_eet_base_efl_object_destructor(Eo *obj, Ecore_Con_Eet_Base_Data *pd)
{
   eet_data_descriptor_free(pd->edd);
   eet_data_descriptor_free(pd->matching);
   eina_hash_free(pd->data_callbacks);
   eina_hash_free(pd->raw_data_callbacks);

   efl_destructor(efl_super(obj, ECORE_CON_EET_BASE_CLASS));
}

/**
 * @internal
 * @brief Finalizer for Ecore_Con_Eet_Base.
 * Ensures that an Ecore_Con_Server has been set before finalizing.
 * @param obj The Eo object to finalize.
 * @param pd Private data for the Ecore_Con_Eet_Base.
 * @return The finalized Eo object, or NULL if server is not set.
 */
EOLIAN static Efl_Object *
_ecore_con_eet_base_efl_object_finalize(Eo *obj, Ecore_Con_Eet_Base_Data *pd)
{
   if (!pd->server) return NULL;

   return efl_finalize(efl_super(obj, ECORE_CON_EET_BASE_CLASS));
}

/**
 * @internal
 * @brief Implements ecore_con_eet_base_server_set.
 * Sets the Ecore_Con_Server associated with this Ecore_Con_Eet instance.
 * @param obj The Ecore_Con_Eet object.
 * @param pd The private data of the Ecore_Con_Eet_Base object.
 * @param data The Ecore_Con_Server to associate.
 */
EOLIAN static void
_ecore_con_eet_base_server_set(Eo *obj EINA_UNUSED, Ecore_Con_Eet_Base_Data *pd, Ecore_Con_Server *data)
{
   if (!ecore_con_server_check(data))
     return;

   pd->server = data;
}

/**
 * @internal
 * @brief Implements ecore_con_eet_base_server_get.
 * Gets the Ecore_Con_Server associated with this Ecore_Con_Eet instance.
 * @param obj The Ecore_Con_Eet object.
 * @param pd The private data of the Ecore_Con_Eet_Base object.
 * @return The associated Ecore_Con_Server.
 */
EOLIAN static Ecore_Con_Server *
_ecore_con_eet_base_server_get(const Eo *obj EINA_UNUSED, Ecore_Con_Eet_Base_Data *pd)
{
   return pd->server;
}

/**************
* Global API *
**************/

/**
 * @brief Creates a new Ecore_Con_Eet server instance.
 * This function initializes an Ecore_Con_Eet object that will listen for
 * incoming client connections on the provided Ecore_Con_Server.
 * @param server The Ecore_Con_Server object to use for listening.
 *               Must be a valid server (e.g., created with ecore_con_server_add()).
 * @return A new Ecore_Con_Eet object on success, or NULL on failure.
 *         The returned object should be freed with ecore_con_eet_server_free()
 *         or efl_unref() when no longer needed.
 * @see ecore_con_eet_server_free()
 * @see ecore_con_server_add()
 */
ECORE_CON_API Ecore_Con_Eet *
ecore_con_eet_server_new(Ecore_Con_Server *server)
{
   Ecore_Con_Eet *ece_obj;

   if (!server) return NULL;

   ece_obj = efl_add_ref(ECORE_CON_EET_SERVER_OBJ_CLASS, NULL, ecore_con_eet_base_server_set(efl_added, server));

   return ece_obj;
}

/**
 * @brief Creates a new Ecore_Con_Eet client instance.
 * This function initializes an Ecore_Con_Eet object that will attempt to
 * connect to a server specified by the Ecore_Con_Server object.
 * @param server The Ecore_Con_Server object representing the server to connect to.
 *               Must be a valid client connection target (e.g., created with ecore_con_server_connect()).
 * @return A new Ecore_Con_Eet object on success, or NULL on failure.
 *         The returned object should be freed with ecore_con_eet_server_free()
 *         (despite the name, it's used for both client and server Eet objects)
 *         or efl_unref() when no longer needed.
 * @see ecore_con_eet_server_free()
 * @see ecore_con_server_connect()
 */
ECORE_CON_API Ecore_Con_Eet *
ecore_con_eet_client_new(Ecore_Con_Server *server)
{
   Ecore_Con_Eet *ece_obj;

   if (!server) return NULL;

   ece_obj = efl_add_ref(ECORE_CON_EET_CLIENT_OBJ_CLASS, NULL, ecore_con_eet_base_server_set(efl_added, server));

   return ece_obj;
}

/**
 * @brief Frees an Ecore_Con_Eet instance (server or client).
 * This function decrements the reference count of the Ecore_Con_Eet object.
 * If the reference count reaches zero, the object and its associated resources
 * will be deallocated.
 * @param server The Ecore_Con_Eet object to free.
 * @note Despite the name, this function is used for both server and client Eet objects.
 */
ECORE_CON_API void
ecore_con_eet_server_free(Ecore_Con_Eet *server)
{
   efl_unref(server);
}

/**
 * @brief Registers an Eet_Data_Descriptor for a specific data type name.
 * This allows Eet to encode/decode structures of this type when they are
 * part of an Ecore_Con_Eet_Protocol message. This is necessary to send
 * custom data structures over an Eet connection.
 * @param ece The Ecore_Con_Eet object.
 * @param name The type name to associate with the Eet_Data_Descriptor.
 *             Example: "My_Custom_Data_Type".
 * @param edd The Eet_Data_Descriptor for the data type.
 * @see ecore_con_eet_send()
 */
ECORE_CON_API void
ecore_con_eet_register(Ecore_Con_Eet *ece, const char *name, Eet_Data_Descriptor *edd)
{
   ecore_con_eet_base_register(ece, name, edd);
}

/**
 * @brief Adds a callback for handling specific Eet data types.
 * When data identified by 'name' is received, 'func' will be called.
 * @param ece The Ecore_Con_Eet object.
 * @param name The name of the data type this callback handles. This should match
 *             the 'name' used in ecore_con_eet_send().
 * @param func The callback function to execute upon receiving the data.
 * @param data User-defined data to be passed to the callback function.
 * @see ecore_con_eet_data_callback_del()
 * @see ecore_con_eet_send()
 */
ECORE_CON_API void
ecore_con_eet_data_callback_add(Ecore_Con_Eet *ece, const char *name, Ecore_Con_Eet_Data_Cb func, const void *data)
{
   ecore_con_eet_base_data_callback_set(ece, name, func, data);
}

/**
 * @brief Deletes a data callback.
 * Removes a previously added callback for a specific data type name.
 * @param ece The Ecore_Con_Eet object.
 * @param name The name of the data type whose callback should be removed.
 * @see ecore_con_eet_data_callback_add()
 */
ECORE_CON_API void
ecore_con_eet_data_callback_del(Ecore_Con_Eet *ece, const char *name)
{
   ecore_con_eet_base_data_callback_del(ece, name);
}

/**
 * @brief Adds a callback for handling raw data streams.
 * When a raw data stream identified by 'name' (protocol name) is received,
 * 'func' will be called with chunks of data.
 * @param ece The Ecore_Con_Eet object.
 * @param name The protocol name for this raw data stream. This should match
 *             the 'protocol_name' used in ecore_con_eet_raw_send().
 * @param func The callback function to execute.
 * @param data User-defined data to be passed to the callback function.
 * @see ecore_con_eet_raw_data_callback_del()
 * @see ecore_con_eet_raw_send()
 */
ECORE_CON_API void
ecore_con_eet_raw_data_callback_add(Ecore_Con_Eet *ece, const char *name, Ecore_Con_Eet_Raw_Data_Cb func, const void *data)
{
   ecore_con_eet_base_raw_data_callback_set(ece, name, func, data);
}

/**
 * @brief Deletes a raw data callback.
 * Removes a previously added callback for a specific raw data protocol name.
 * @param ece The Ecore_Con_Eet object.
 * @param name The protocol name of the raw data callback to remove.
 * @see ecore_con_eet_raw_data_callback_add()
 */
ECORE_CON_API void
ecore_con_eet_raw_data_callback_del(Ecore_Con_Eet *ece, const char *name)
{
   ecore_con_eet_base_raw_data_callback_del(ece, name);
}

/**
 * @brief Adds a callback for client connection events (server-side).
 * This function is used on an Ecore_Con_Eet server instance. The callback 'func'
 * will be invoked when a new client connects to the server.
 * @param ece The Ecore_Con_Eet server object.
 * @param func The callback function to execute when a client connects.
 *             The function should return EINA_TRUE to accept the connection,
 *             or EINA_FALSE to reject and close it.
 * @param data User-defined data to be passed to the callback function.
 * @see ecore_con_eet_client_connect_callback_del()
 */
ECORE_CON_API void
ecore_con_eet_client_connect_callback_add(Ecore_Con_Eet *ece, Ecore_Con_Eet_Client_Cb func, const void *data)
{
   Ecore_Con_Eet_Server_Obj_Data *eces = efl_data_scope_get(ece, ECORE_CON_EET_SERVER_OBJ_CLASS);
   Ecore_Con_Eet_Client *c;

   if (!eces || !func) return;

   c = calloc(1, sizeof (Ecore_Con_Eet_Client));
   if (!c) return;

   c->func = func;
   c->data = data;

   eces->client_connect_callbacks = eina_list_append(eces->client_connect_callbacks, c);
}

/**
 * @brief Deletes a client connection callback (server-side).
 * Removes a previously added callback for client connection events.
 * @param ece The Ecore_Con_Eet server object.
 * @param func The callback function to remove.
 * @param data The user-defined data that was passed when adding the callback.
 *             Both 'func' and 'data' must match for the callback to be removed.
 * @see ecore_con_eet_client_connect_callback_add()
 */
ECORE_CON_API void
ecore_con_eet_client_connect_callback_del(Ecore_Con_Eet *ece, Ecore_Con_Eet_Client_Cb func, const void *data)
{
   Ecore_Con_Eet_Server_Obj_Data *eces = efl_data_scope_get(ece, ECORE_CON_EET_SERVER_OBJ_CLASS);
   Ecore_Con_Eet_Client *c;
   Eina_List *l;

   if (!eces || !func) return;

   EINA_LIST_FOREACH(eces->client_connect_callbacks, l, c)
     if (c->func == func && c->data == data)
       {
          eces->client_connect_callbacks = eina_list_remove_list(eces->client_connect_callbacks, l);
          free(c);
          return;
       }
}

/**
 * @brief Adds a callback for client disconnection events (server-side).
 * This function is used on an Ecore_Con_Eet server instance. The callback 'func'
 * will be invoked when a client disconnects from the server.
 * @param ece The Ecore_Con_Eet server object.
 * @param func The callback function to execute when a client disconnects.
 * @param data User-defined data to be passed to the callback function.
 * @see ecore_con_eet_client_disconnect_callback_del()
 */
ECORE_CON_API void
ecore_con_eet_client_disconnect_callback_add(Ecore_Con_Eet *ece, Ecore_Con_Eet_Client_Cb func, const void *data)
{
   Ecore_Con_Eet_Server_Obj_Data *eces = efl_data_scope_get(ece, ECORE_CON_EET_SERVER_OBJ_CLASS);
   Ecore_Con_Eet_Client *c;

   if (!eces || !func) return;

   c = calloc(1, sizeof (Ecore_Con_Eet_Client));
   if (!c) return;

   c->func = func;
   c->data = data;

   eces->client_disconnect_callbacks = eina_list_append(eces->client_disconnect_callbacks, c);
}

/**
 * @brief Deletes a client disconnection callback (server-side).
 * Removes a previously added callback for client disconnection events.
 * @param ece The Ecore_Con_Eet server object.
 * @param func The callback function to remove.
 * @param data The user-defined data that was passed when adding the callback.
 *             Both 'func' and 'data' must match for the callback to be removed.
 * @see ecore_con_eet_client_disconnect_callback_add()
 */
ECORE_CON_API void
ecore_con_eet_client_disconnect_callback_del(Ecore_Con_Eet *ece, Ecore_Con_Eet_Client_Cb func, const void *data)
{
   Ecore_Con_Eet_Server_Obj_Data *eced = efl_data_scope_get(ece, ECORE_CON_EET_SERVER_OBJ_CLASS);
   Ecore_Con_Eet_Client *c;
   Eina_List *l;

   if (!eced || !func) return;

   EINA_LIST_FOREACH(eced->client_disconnect_callbacks, l, c)
     if (c->func == func && c->data == data)
       {
          eced->client_disconnect_callbacks = eina_list_remove_list(eced->client_disconnect_callbacks, l);
          free(c);
          return;
       }
}

/**
 * @brief Adds a callback for server connection events (client-side).
 * This function is used on an Ecore_Con_Eet client instance. The callback 'func'
 * will be invoked when the client successfully connects to the server.
 * @param ece The Ecore_Con_Eet client object.
 * @param func The callback function to execute when connected to the server.
 *             The function should return EINA_TRUE to proceed with the connection,
 *             or EINA_FALSE to immediately disconnect.
 * @param data User-defined data to be passed to the callback function.
 * @see ecore_con_eet_server_connect_callback_del()
 */
ECORE_CON_API void
ecore_con_eet_server_connect_callback_add(Ecore_Con_Eet *ece, Ecore_Con_Eet_Server_Cb func, const void *data)
{
   Ecore_Con_Eet_Client_Obj_Data *eced = efl_data_scope_get(ece, ECORE_CON_EET_CLIENT_OBJ_CLASS);
   Ecore_Con_Eet_Server *s;

   if (!eced || !func) return;

   s = calloc(1, sizeof (Ecore_Con_Eet_Server));
   if (!s) return;

   s->func = func;
   s->data = data;

   eced->server_connect_callbacks = eina_list_append(eced->server_connect_callbacks, s);
}

/**
 * @brief Deletes a server connection callback (client-side).
 * Removes a previously added callback for server connection events.
 * @param ece The Ecore_Con_Eet client object.
 * @param func The callback function to remove.
 * @param data The user-defined data that was passed when adding the callback.
 *             Both 'func' and 'data' must match for the callback to be removed.
 * @see ecore_con_eet_server_connect_callback_add()
 */
ECORE_CON_API void
ecore_con_eet_server_connect_callback_del(Ecore_Con_Eet *ece, Ecore_Con_Eet_Server_Cb func, const void *data)
{
   Ecore_Con_Eet_Client_Obj_Data *eced = efl_data_scope_get(ece, ECORE_CON_EET_CLIENT_OBJ_CLASS);
   Ecore_Con_Eet_Server *s;
   Eina_List *l;

   if (!eced || !func) return;

   EINA_LIST_FOREACH(eced->server_connect_callbacks, l, s)
     if (s->func == func && s->data == data)
       {
          eced->server_connect_callbacks = eina_list_remove_list(eced->server_connect_callbacks, l);
          free(s);
          return;
       }
}

/**
 * @brief Adds a callback for server disconnection events (client-side).
 * This function is used on an Ecore_Con_Eet client instance. The callback 'func'
 * will be invoked when the client is disconnected from the server.
 * @param ece The Ecore_Con_Eet client object.
 * @param func The callback function to execute upon disconnection from the server.
 * @param data User-defined data to be passed to the callback function.
 * @see ecore_con_eet_server_disconnect_callback_del()
 */
ECORE_CON_API void
ecore_con_eet_server_disconnect_callback_add(Ecore_Con_Eet *ece, Ecore_Con_Eet_Server_Cb func, const void *data)
{
   Ecore_Con_Eet_Client_Obj_Data *eced = efl_data_scope_get(ece, ECORE_CON_EET_CLIENT_OBJ_CLASS);
   Ecore_Con_Eet_Server *s;

   if (!eced || !func) return;

   s = calloc(1, sizeof (Ecore_Con_Eet_Server));
   if (!s) return;

   s->func = func;
   s->data = data;

   eced->server_disconnect_callbacks = eina_list_append(eced->server_disconnect_callbacks, s);
}

/**
 * @brief Deletes a server disconnection callback (client-side).
 * Removes a previously added callback for server disconnection events.
 * @param ece The Ecore_Con_Eet client object.
 * @param func The callback function to remove.
 * @param data The user-defined data that was passed when adding the callback.
 *             Both 'func' and 'data' must match for the callback to be removed.
 * @see ecore_con_eet_server_disconnect_callback_add()
 */
ECORE_CON_API void
ecore_con_eet_server_disconnect_callback_del(Ecore_Con_Eet *ece, Ecore_Con_Eet_Server_Cb func, const void *data)
{
   Ecore_Con_Eet_Client_Obj_Data *eced = efl_data_scope_get(ece, ECORE_CON_EET_CLIENT_OBJ_CLASS);
   Ecore_Con_Eet_Server *s;
   Eina_List *l;

   if (!eced || !func) return;

   EINA_LIST_FOREACH(eced->server_disconnect_callbacks, l, s)
     if (s->func == func && s->data == data)
       {
          eced->server_disconnect_callbacks = eina_list_remove_list(eced->server_disconnect_callbacks, l);
          free(s);
          return;
       }
}

/**
 * @brief Sets user-defined data associated with an Ecore_Con_Eet object.
 * This allows arbitrary data to be attached to an Eet connection object.
 * @param ece The Ecore_Con_Eet object.
 * @param data Pointer to the user data.
 * @see ecore_con_eet_data_get()
 */
ECORE_CON_API void
ecore_con_eet_data_set(Ecore_Con_Eet *ece, const void *data)
{
   efl_key_data_set(ece, ECORE_CON_EET_DATA_KEY, data);
}

/**
 * @brief Gets user-defined data associated with an Ecore_Con_Eet object.
 * Retrieves data previously set by ecore_con_eet_data_set().
 * @param ece The Ecore_Con_Eet object.
 * @return Pointer to the user data, or NULL if no data was set.
 * @see ecore_con_eet_data_set()
 */
ECORE_CON_API const void *
ecore_con_eet_data_get(Ecore_Con_Eet *ece)
{
   return efl_key_data_get(ece, ECORE_CON_EET_DATA_KEY);
}

/**
 * @brief Retrieves the Ecore_Con_Eet object associated with an Ecore_Con_Reply.
 * This is useful in callbacks that receive an Ecore_Con_Reply to get back
 * to the main Eet connection object.
 * @param reply The Ecore_Con_Reply object.
 * @return The associated Ecore_Con_Eet object, or NULL if reply is NULL.
 */
ECORE_CON_API Ecore_Con_Eet *
ecore_con_eet_reply(Ecore_Con_Reply *reply)
{
   if (!reply) return NULL;
   return reply->ece;
}

/**
 * @brief Sends Eet-encoded data.
 * The data 'value' will be encoded using the Eet_Data_Descriptor previously
 * registered for 'name' via ecore_con_eet_register().
 * @param reply The Ecore_Con_Reply object representing the connection to send data over.
 *              This is typically obtained in a connection callback.
 * @param name The type name of the data being sent. This must match a name
 *             previously registered with ecore_con_eet_register().
 *             Example: "My_Custom_Data_Type".
 * @param value Pointer to the data structure to send. The structure must match
 *              the Eet_Data_Descriptor registered for 'name'.
 * @see ecore_con_eet_register()
 * @see ecore_con_eet_data_callback_add()
 */
ECORE_CON_API void
ecore_con_eet_send(Ecore_Con_Reply *reply, const char *name, void *value)
{
   ecore_con_eet_base_send(reply->ece, reply, name, value);
}

/**
 * @brief Sends raw binary data over the connection.
 * This function sends data as a raw byte stream, preceded by a header
 * containing the protocol name, section name, and data length.
 * @param reply The Ecore_Con_Reply object for the connection.
 * @param protocol_name A string identifying the protocol of this raw data.
 *                      Example: "FileTransferProtocol".
 * @param section A string identifying a subsection or type within the protocol.
 *                Example: "ChunkData" or "FileHeader".
 * @param value Pointer to the binary data to send.
 * @param length The length of the binary data in bytes.
 * @see ecore_con_eet_raw_data_callback_add()
 */
ECORE_CON_API void
ecore_con_eet_raw_send(Ecore_Con_Reply *reply, const char *protocol_name, const char *section, void *value, unsigned int length)
{
   Eina_Binbuf *buf = eina_binbuf_manage_new(value, length, 1);
   ecore_con_eet_base_raw_send(reply->ece, reply, protocol_name, section, buf);
   eina_binbuf_free(buf);
}

#include "ecore_con_eet_base_eo.c"
#include "ecore_con_eet_server_obj_eo.c"
#include "ecore_con_eet_client_obj_eo.c"
