#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <string.h>
#include <stdlib.h>

#include <Eina.h>

#include "Eet.h"
#include "Eet_private.h"

/* max message size: 1Gb - raised from original 64Kb */
#define MAX_MSG_SIZE (1024 * 1024 * 1024)
#define MAGIC_EET_DATA_PACKET 0x4270ACE1

/**
 * @internal
 * @brief Structure to manage a connection for sending and receiving Eet data.
 *
 * This structure holds the state of an Eet connection, including callbacks
 * for reading and writing data, a buffer for incoming partial data, and
 * user-defined data.
 */
struct _Eet_Connection
{
   Eet_Read_Cb  *eet_read_cb; /**< Callback function to be called when data is decoded. */
   Eet_Write_Cb *eet_write_cb; /**< Callback function to send encoded data over the connection. */
   void         *user_data; /**< User-specific data passed to callbacks. */

   size_t        allocated; /**< Current allocated size of the buffer. */
   size_t        size; /**< Expected size of the current incoming packet. 0 if no packet is being assembled. */
   size_t        received; /**< Amount of data received for the current packet. */

   void         *buffer; /**< Buffer to store partial incoming data. */
};

/**
 * @brief Creates a new Eet connection object.
 *
 * This function initializes an Eet_Connection structure with the provided
 * read and write callbacks and user data. These callbacks are essential for
 * handling the actual data transmission and reception.
 *
 * @param eet_read_cb The callback function to be invoked when a complete
 *        Eet data packet has been received and decoded.
 *        The callback signature is:
 *        `Eina_Bool (*Eet_Read_Cb)(const void *data, size_t size, void *user_data)`
 *        - `data`: Pointer to the decoded data.
 *        - `size`: Size of the decoded data.
 *        - `user_data`: The user_data pointer provided to eet_connection_new().
 *        It should return EINA_TRUE on success, EINA_FALSE on failure.
 * @param eet_write_cb The callback function used to send encoded Eet data
 *        over the underlying transport.
 *        The callback signature is:
 *        `int (*Eet_Write_Cb)(const void *data, size_t size, void *user_data)`
 *        - `data`: Pointer to the data to be written.
 *        - `size`: Size of the data to be written.
 *        - `user_data`: The user_data pointer provided to eet_connection_new().
 *        It should return the number of bytes written, or -1 on error.
 * @param user_data A pointer to user-specific data that will be passed
 *        to the read and write callbacks.
 * @return A pointer to the newly created Eet_Connection object, or NULL
 *         if memory allocation fails or if callbacks are invalid.
 */
EAPI Eet_Connection *
eet_connection_new(Eet_Read_Cb  *eet_read_cb,
                   Eet_Write_Cb *eet_write_cb,
                   const void   *user_data)
{
   Eet_Connection *conn;

   if ((!eet_read_cb) || (!eet_write_cb)) return NULL;

   conn = calloc(1, sizeof (Eet_Connection));
   if (!conn) return NULL;
   conn->eet_read_cb = eet_read_cb;
   conn->eet_write_cb = eet_write_cb;
   conn->user_data = (void *)user_data;
   return conn;
}

/**
 * @brief Processes incoming raw data for an Eet connection.
 *
 * This function takes a chunk of raw data received from the underlying
 * transport (e.g., a socket) and processes it. It handles packet
 * framing, reassembly of fragmented packets, and then invokes the
 * `eet_read_cb` when a complete Eet message is available.
 *
 * Eet messages are framed with a magic number and a size prefix.
 * This function reads this header to determine message boundaries.
 * If a message is larger than the `MAX_MSG_SIZE` or has an invalid
 * magic number, it's considered an error.
 *
 * The function can handle multiple Eet messages within a single `data`
 * buffer, or partial messages that will be completed by subsequent calls.
 *
 * @param conn The Eet_Connection object.
 * @param data Pointer to the raw data received.
 * @param size The size of the raw data in bytes.
 * @return The number of bytes remaining in the `data` buffer that were
 *         not processed. This can happen if an error occurs or if a
 *         partial message remains at the end of the buffer. If all data
 *         is processed successfully, it returns 0. If invalid parameters
 *         are passed (e.g., NULL conn, data, or zero size), it returns
 *         the original `size` indicating no processing was done.
 */
EAPI int
eet_connection_received(Eet_Connection *conn,
                        const void     *data,
                        size_t          size)
{
   if ((!conn) || (!data) || (!size)) return size;
   do
     {
        size_t copy_size;

        if (conn->size == 0)
          {
             const int *msg;
             size_t packet_size;

             if (size < (sizeof(int) * 2)) break;

             msg = data;
             /* Check the magic */
             if (eina_ntohl(msg[0]) != MAGIC_EET_DATA_PACKET) break;

             packet_size = eina_ntohl(msg[1]);
             /* Message should always be under MAX_MSG_SIZE */
             if (packet_size > MAX_MSG_SIZE) break;

             data = (void *)(msg + 2);
             size -= sizeof(int) * 2;
             if ((size_t)packet_size <= size)
               {
                  /* Not a partial receive, go the quick way. */
                  if (!conn->eet_read_cb(data, packet_size, conn->user_data))
                    break;

                  data = (void *)((char *)data + packet_size);
                  size -= packet_size;
                  conn->received = 0;
                  continue;
               }
             conn->size = packet_size;
             if (conn->allocated < conn->size)
               {
                  void *tmp;

                  tmp = realloc(conn->buffer, conn->size);
                  if (!tmp) break;
                  conn->buffer = tmp;
                  conn->allocated = conn->size;
               }
          }

        /* Partial receive */
        copy_size =
          (conn->size - conn->received >=
              size) ? size : conn->size - conn->received;
        memcpy((char *)conn->buffer + conn->received, data, copy_size);

        conn->received += copy_size;
        data = (void *)((char *)data + copy_size);
        size -= copy_size;

        if (conn->received == conn->size)
          {
             size_t data_size;

             data_size = conn->size;
             conn->size = 0;
             conn->received = 0;
             /* Completed a packet. */
             if (!conn->eet_read_cb(conn->buffer, data_size, conn->user_data))
               {
                  /* Something goes wrong. Stop now. */
                  size += data_size;
                  break;
               }
          }
     }
   while (size > 0);

   return size;
}

/**
 * @internal
 * @brief Sends raw data with Eet framing over the connection.
 *
 * This function prepends the Eet magic number and data size to the
 * provided data buffer and then calls the `eet_write_cb` to send it.
 *
 * @param conn The Eet_Connection object.
 * @param data Pointer to the raw data to be sent.
 * @param data_size The size of the raw data in bytes.
 * @return EINA_TRUE if the data was successfully passed to the write callback,
 *         EINA_FALSE otherwise (e.g., if data_size exceeds MAX_MSG_SIZE or
 *         memory allocation fails).
 */
static Eina_Bool
_eet_connection_raw_send(Eet_Connection *conn,
                         void           *data,
                         int             data_size)
{
   int *message;

   /* Message should always be under MAX_MSG_SIZE */
   if (data_size > MAX_MSG_SIZE) return EINA_FALSE;
   message = malloc(data_size + (sizeof(int) * 2));
   message[0] = eina_htonl(MAGIC_EET_DATA_PACKET);
   message[1] = eina_htonl(data_size);
   memcpy(message + 2, data, data_size);
   conn->eet_write_cb(message,
                      data_size + (sizeof(int) * 2),
                      conn->user_data);

   free(message);
   return EINA_TRUE;
}

/**
 * @brief Checks if the connection's internal receive buffer is empty.
 *
 * This function can be used to determine if there is any partially
 * received packet data pending in the connection's buffer.
 *
 * @param conn The Eet_Connection object.
 * @return EINA_TRUE if there is no partial packet being assembled (i.e.,
 *         `conn->size` is 0), EINA_FALSE otherwise. Also returns EINA_TRUE
 *         if `conn` is NULL.
 */
EAPI Eina_Bool
eet_connection_empty(Eet_Connection *conn)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, EINA_TRUE);
   return conn->size ? EINA_FALSE : EINA_TRUE;
}

/**
 * @brief Encodes data using an Eet_Data_Descriptor and sends it.
 *
 * This function takes a data structure (`data_in`), encodes it into a
 * flat byte stream using the provided Eet_Data_Descriptor (`edd`),
 * optionally encrypts it if `cipher_key` is provided, and then sends
 * it over the connection.
 *
 * @param conn The Eet_Connection object.
 * @param edd The Eet_Data_Descriptor describing the structure of `data_in`.
 * @param data_in Pointer to the data structure to be encoded and sent.
 * @param cipher_key Optional key for encrypting the data. If NULL,
 *        no encryption is performed.
 * @return EINA_TRUE if the data was successfully encoded and passed to
 *         the write callback, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
eet_connection_send(Eet_Connection      *conn,
                    Eet_Data_Descriptor *edd,
                    const void          *data_in,
                    const char          *cipher_key)
{
   void *flat_data;
   int data_size;
   Eina_Bool ret = EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, EINA_FALSE);

   flat_data = eet_data_descriptor_encode_cipher(edd,
                                                 data_in,
                                                 cipher_key,
                                                 &data_size);
   if (!flat_data) return EINA_FALSE;
   if (_eet_connection_raw_send(conn, flat_data, data_size)) ret = EINA_TRUE;
   free(flat_data);
   return ret;
}

/**
 * @brief Encodes data from an Eet_Node and sends it.
 *
 * This function takes an Eet_Node, encodes it into a flat byte stream,
 * optionally encrypts it if `cipher_key` is provided, and then sends
 * it over the connection. This is useful for sending more complex or
 * dynamically structured data that is represented by an Eet_Node graph.
 *
 * @param conn The Eet_Connection object.
 * @param node The Eet_Node to be encoded and sent.
 * @param cipher_key Optional key for encrypting the data. If NULL,
 *        no encryption is performed.
 * @return EINA_TRUE if the data was successfully encoded and passed to
 *         the write callback, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
eet_connection_node_send(Eet_Connection *conn,
                         Eet_Node       *node,
                         const char     *cipher_key)
{
   void *data;
   int data_size;
   Eina_Bool ret = EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, EINA_FALSE);

   data = eet_data_node_encode_cipher(node, cipher_key, &data_size);
   if (!data) return EINA_FALSE;
   if (_eet_connection_raw_send(conn, data, data_size))
     ret = EINA_TRUE;
   free(data);
   return ret;
}

/**
 * @brief Closes an Eet connection and frees associated resources.
 *
 * This function cleans up the Eet_Connection object, freeing its internal
 * buffer. It also allows checking if there was any unprocessed data
 * remaining in the receive buffer.
 *
 * @param conn The Eet_Connection object to close.
 * @param on_going Optional output parameter. If not NULL, it will be set
 *        to EINA_TRUE if there was partially received data in the
 *        connection's buffer (`conn->received != 0`), indicating an
 *        incomplete packet. Otherwise, it's set to EINA_FALSE.
 * @return The user_data pointer that was originally passed to
 *         eet_connection_new(). Returns NULL if `conn` is NULL.
 */
EAPI void *
eet_connection_close(Eet_Connection *conn,
                     Eina_Bool      *on_going)
{
   void *user_data;

   if (!conn) return NULL;
   if (on_going) *on_going = conn->received == 0 ? EINA_FALSE : EINA_TRUE;
   user_data = conn->user_data;
   free(conn->buffer);
   free(conn);
   return user_data;
}

