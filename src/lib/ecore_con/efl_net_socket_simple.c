#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

typedef struct
{

} Efl_Net_Socket_Simple_Data;

#define MY_CLASS EFL_NET_SOCKET_SIMPLE_CLASS

/**
 * @internal
 * @brief Sets the inner I/O object for the buffered stream.
 *
 * This function is an EOLIAN implementation for setting the underlying
 * I/O object that this socket simple object will use for its operations.
 * The provided @p io object must implement the EFL_NET_SOCKET_INTERFACE.
 *
 * @param[in] o The Eolian object.
 * @param[in] pd Private data for this object type (unused).
 * @param[in] io The inner I/O object to set. Must be a valid Efl_Net_Socket.
 */
EOLIAN static void
_efl_net_socket_simple_efl_io_buffered_stream_inner_io_set(Eo *o, Efl_Net_Socket_Simple_Data *pd EINA_UNUSED, Efl_Object *io)
{
   EINA_SAFETY_ON_FALSE_RETURN(efl_isa(io, EFL_NET_SOCKET_INTERFACE));
   efl_io_buffered_stream_inner_io_set(efl_super(o, MY_CLASS), io);
}

/**
 * @internal
 * @brief Gets the local socket address.
 *
 * This function is an EOLIAN implementation for retrieving the local
 * address of the underlying socket. It delegates the call to the
 * inner I/O object.
 *
 * @param[in] o The Eolian object.
 * @param[in] pd Private data for this object type (unused).
 *
 * @return The local socket address as a string, or @c NULL on failure.
 *         The string is owned by the inner I/O object and should not be freed.
 *         Example: "127.0.0.1:12345" or "/tmp/socket_file.sock"
 */
EOLIAN static const char *
_efl_net_socket_simple_efl_net_socket_address_local_get(const Eo *o, Efl_Net_Socket_Simple_Data *pd EINA_UNUSED)
{
   return efl_net_socket_address_local_get(efl_io_buffered_stream_inner_io_get(o));
}

/**
 * @internal
 * @brief Gets the remote socket address.
 *
 * This function is an EOLIAN implementation for retrieving the remote
 * address of the underlying socket. It delegates the call to the
 * inner I/O object.
 *
 * @param[in] o The Eolian object.
 * @param[in] pd Private data for this object type (unused).
 *
 * @return The remote socket address as a string, or @c NULL on failure or if not connected.
 *         The string is owned by the inner I/O object and should not be freed.
 *         Example: "192.168.1.100:8080"
 */
EOLIAN static const char *
_efl_net_socket_simple_efl_net_socket_address_remote_get(const Eo *o, Efl_Net_Socket_Simple_Data *pd EINA_UNUSED)
{
   return efl_net_socket_address_remote_get(efl_io_buffered_stream_inner_io_get(o));
}

#include "efl_net_socket_simple.eo.c"
