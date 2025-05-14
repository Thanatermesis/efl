#define EFL_NET_SOCKET_FD_PROTECTED 1
#define EFL_LOOP_FD_PROTECTED 1
#define EFL_IO_READER_FD_PROTECTED 1
#define EFL_IO_WRITER_FD_PROTECTED 1
#define EFL_IO_CLOSER_FD_PROTECTED 1
#define EFL_IO_READER_PROTECTED 1
#define EFL_IO_WRITER_PROTECTED 1
#define EFL_IO_CLOSER_PROTECTED 1
#define EFL_NET_SOCKET_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

#include <fcntl.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#define MY_CLASS EFL_NET_SOCKET_FD_CLASS

/**
 * @brief Private data for the Efl_Net_Socket_Fd class.
 *
 * This structure holds the local and remote addresses as stringshares
 * and the socket family (e.g., AF_INET, AF_INET6).
 */
typedef struct _Efl_Net_Socket_Fd_Data
{
   Eina_Stringshare *address_local; /**< The local socket address string. */
   Eina_Stringshare *address_remote; /**< The remote socket address string. */
   int family; /**< The socket address family. */
} Efl_Net_Socket_Fd_Data;

/**
 * @brief Event callback for readable file descriptor.
 *
 * This function is called when the underlying file descriptor becomes readable.
 * It sets the can_read property to EINA_TRUE if the socket is not closed.
 *
 * @param data User data (unused).
 * @param event The Efl_Event structure.
 */
static void
_efl_net_socket_fd_event_read(void *data EINA_UNUSED, const Efl_Event *event)
{
   if (efl_io_closer_closed_get(event->object))
     return;
   efl_io_reader_can_read_set(event->object, EINA_TRUE);
}

/**
 * @brief Event callback for writable file descriptor.
 *
 * This function is called when the underlying file descriptor becomes writable.
 * It sets the can_write property to EINA_TRUE if the socket is not closed.
 *
 * @param data User data (unused).
 * @param event The Efl_Event structure.
 */
static void
_efl_net_socket_fd_event_write(void *data EINA_UNUSED, const Efl_Event *event)
{
   if (efl_io_closer_closed_get(event->object))
     return;
   efl_io_writer_can_write_set(event->object, EINA_TRUE);
}

/**
 * @brief Event callback for file descriptor error.
 *
 * This function is called when an error occurs on the underlying file descriptor.
 * It sets can_write and can_read to EINA_FALSE and eos to EINA_TRUE if the socket is not closed.
 *
 * @param data User data (unused).
 * @param event The Efl_Event structure.
 */
static void
_efl_net_socket_fd_event_error(void *data EINA_UNUSED, const Efl_Event *event)
{
   if (efl_io_closer_closed_get(event->object))
     return;
   efl_io_writer_can_write_set(event->object, EINA_FALSE);
   efl_io_reader_can_read_set(event->object, EINA_FALSE);
   efl_io_reader_eos_set(event->object, EINA_TRUE);
}

/**
 * @brief Finalizes the Efl_Net_Socket_Fd object.
 *
 * This function is called during the finalization phase of object construction.
 * It registers event callbacks for read, write, and error events on the file descriptor.
 *
 * @param o The Efl_Net_Socket_Fd object.
 * @param pd Private data (unused).
 * @return The finalized Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_net_socket_fd_efl_object_finalize(Eo *o, Efl_Net_Socket_Fd_Data *pd EINA_UNUSED)
{
   o = efl_finalize(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   efl_event_callback_add(o, EFL_LOOP_FD_EVENT_WRITE, _efl_net_socket_fd_event_write, NULL);
   efl_event_callback_add(o, EFL_LOOP_FD_EVENT_READ, _efl_net_socket_fd_event_read, NULL);
   efl_event_callback_add(o, EFL_LOOP_FD_EVENT_ERROR, _efl_net_socket_fd_event_error, NULL);
   return o;
}

/**
 * @brief Constructs the Efl_Net_Socket_Fd object.
 *
 * This function is called during the construction phase of the object.
 * It initializes the socket family to AF_UNSPEC and sets default
 * properties for closing behavior and file descriptors.
 *
 * @param o The Efl_Net_Socket_Fd object.
 * @param pd Private data for the object.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_net_socket_fd_efl_object_constructor(Eo *o, Efl_Net_Socket_Fd_Data *pd)
{
   pd->family = AF_UNSPEC;
   o = efl_constructor(efl_super(o, MY_CLASS));

   efl_io_closer_close_on_exec_set(o, EINA_TRUE);
   efl_io_closer_close_on_invalidate_set(o, EINA_TRUE);
   efl_io_reader_fd_set(o, SOCKET_TO_LOOP_FD(INVALID_SOCKET));
   efl_io_writer_fd_set(o, SOCKET_TO_LOOP_FD(INVALID_SOCKET));
   efl_io_closer_fd_set(o, SOCKET_TO_LOOP_FD(INVALID_SOCKET));

   return o;
}

/**
 * @brief Invalidates the Efl_Net_Socket_Fd object.
 *
 * This function is called when the object is being invalidated.
 * If close_on_invalidate is set and the socket is not already closed,
 * it closes the socket.
 *
 * @param o The Efl_Net_Socket_Fd object.
 * @param pd Private data (unused).
 */
EOLIAN static void
_efl_net_socket_fd_efl_object_invalidate(Eo *o, Efl_Net_Socket_Fd_Data *pd EINA_UNUSED)
{
   if (efl_io_closer_close_on_invalidate_get(o) &&
       (!efl_io_closer_closed_get(o)))
     {
        efl_event_freeze(o);
        efl_io_closer_close(o);
        efl_event_thaw(o);
     }

   efl_invalidate(efl_super(o, MY_CLASS));
}

/**
 * @brief Destroys the Efl_Net_Socket_Fd object.
 *
 * This function is called when the object is being destroyed.
 * It releases the stringshare for local and remote addresses.
 *
 * @param o The Efl_Net_Socket_Fd object.
 * @param pd Private data for the object.
 */
EOLIAN static void
_efl_net_socket_fd_efl_object_destructor(Eo *o, Efl_Net_Socket_Fd_Data *pd)
{
   efl_destructor(efl_super(o, MY_CLASS));

   eina_stringshare_replace(&pd->address_local, NULL);
   eina_stringshare_replace(&pd->address_remote, NULL);
}

/**
 * @brief Configures the socket with a new file descriptor.
 *
 * This internal helper function sets the given file descriptor for reading,
 * writing, and closing operations on the socket object. It also applies
 * any postponed settings like close_on_exec. It ensures that the socket
 * family is set before proceeding.
 *
 * @param o The Efl_Net_Socket_Fd object.
 * @param pd Private data for the object.
 * @param fd The socket file descriptor to set.
 */
static void
_efl_net_socket_fd_set(Eo *o, Efl_Net_Socket_Fd_Data *pd, SOCKET fd)
{
   Eina_Bool close_on_exec = efl_io_closer_close_on_exec_get(o); /* get cached value, otherwise will query from set fd */
   efl_io_reader_fd_set(o, fd);
   efl_io_writer_fd_set(o, fd);
   efl_io_closer_fd_set(o, fd);

   /* apply postponed values */
   efl_io_closer_close_on_exec_set(o, close_on_exec);
   if (pd->family == AF_UNSPEC)
     {
        ERR("efl_loop_fd_set() must be called after efl_net_server_fd_family_set()");
        return;
     }
}

/**
 * @brief Resets the socket's file descriptor and addresses.
 *
 * This internal helper function sets the file descriptor for reading, writing,
 * and closing to INVALID_SOCKET. It also clears the local and remote
 * socket addresses.
 *
 * @param o The Efl_Net_Socket_Fd object.
 */
static void
_efl_net_socket_fd_unset(Eo *o)
{
   efl_io_reader_fd_set(o, SOCKET_TO_LOOP_FD(INVALID_SOCKET));
   efl_io_writer_fd_set(o, SOCKET_TO_LOOP_FD(INVALID_SOCKET));
   efl_io_closer_fd_set(o, SOCKET_TO_LOOP_FD(INVALID_SOCKET));

   efl_net_socket_address_local_set(o, NULL);
   efl_net_socket_address_remote_set(o, NULL);
}

/**
 * @brief Sets the file descriptor for the Efl_Loop_Fd interface.
 *
 * This function handles setting the underlying file descriptor for the socket.
 * If the socket family is not yet determined and a valid fd is provided,
 * it attempts to determine the family using getsockname().
 * It then calls the superclass's fd_set method and either configures
 * or unsets the socket based on whether the fd is valid.
 *
 * @param o The Efl_Net_Socket_Fd object.
 * @param pd Private data for the object.
 * @param pfd The platform-specific file descriptor.
 */
EOLIAN static void
_efl_net_socket_fd_efl_loop_fd_fd_set(Eo *o, Efl_Net_Socket_Fd_Data *pd, int pfd)
{
   SOCKET fd = (SOCKET)pfd;

   if ((pd->family == AF_UNSPEC) && (fd != INVALID_SOCKET))
     {
        struct sockaddr_storage addr;
        socklen_t addrlen = sizeof(addr);
        if (getsockname(fd, (struct sockaddr *)&addr, &addrlen) != 0)
          ERR("getsockname(" SOCKET_FMT "): %s", fd, eina_error_msg_get(efl_net_socket_error_get()));
        else
          efl_net_socket_fd_family_set(o, addr.ss_family);
     }

   efl_loop_fd_set(efl_super(o, MY_CLASS), fd);

   if (fd != INVALID_SOCKET) _efl_net_socket_fd_set(o, pd, fd);
   else _efl_net_socket_fd_unset(o);
}

/**
 * @brief Closes the socket.
 *
 * Implements the Efl.Io.Closer.close interface.
 * It sets can_write and can_read to false, and eos to true.
 * It then closes the underlying socket file descriptor and emits the "closed" event.
 * Finally, it cleans up by unsetting the socket's fd and addresses.
 *
 * @param o The Efl_Net_Socket_Fd object.
 * @param pd Private data (unused).
 * @return 0 on success, or an error code on failure.
 */
EOLIAN static Eina_Error
_efl_net_socket_fd_efl_io_closer_close(Eo *o, Efl_Net_Socket_Fd_Data *pd EINA_UNUSED)
{
   SOCKET fd = efl_io_closer_fd_get(o);
   Eina_Error ret = 0;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_io_closer_closed_get(o), EBADF);

   efl_io_writer_can_write_set(o, EINA_FALSE);
   efl_io_reader_can_read_set(o, EINA_FALSE);
   efl_io_reader_eos_set(o, EINA_TRUE);

   /* skip _efl_net_socket_fd_efl_loop_fd_fd_set() since we want to
    * retain efl_io_closer_fd_get() so close(super()) works
    * and we emit the events with proper addresses.
    */
   efl_loop_fd_set(efl_super(o, MY_CLASS), SOCKET_TO_LOOP_FD(INVALID_SOCKET));

   efl_io_closer_fd_set(o, SOCKET_TO_LOOP_FD(INVALID_SOCKET));
   if (!((pd->family == AF_UNSPEC) && (fd == 0))) /* if nothing is set, fds are all zero, avoid closing STDOUT */
     if (closesocket(fd) != 0) ret = efl_net_socket_error_get();
   efl_event_callback_call(o, EFL_IO_CLOSER_EVENT_CLOSED, NULL);

   /* do the cleanup our _efl_net_socket_fd_efl_loop_fd_fd_set() would do */
   _efl_net_socket_fd_unset(o);

   return ret;
}

/**
 * @brief Checks if the socket is closed.
 *
 * Implements the Efl.Io.Closer.closed_get interface.
 * The socket is considered closed if its family is specified and
 * the underlying file descriptor is INVALID_SOCKET.
 *
 * @param o The Efl_Net_Socket_Fd object.
 * @param pd Private data for the object.
 * @return EINA_TRUE if closed, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_socket_fd_efl_io_closer_closed_get(const Eo *o, Efl_Net_Socket_Fd_Data *pd)
{
   if (pd->family == AF_UNSPEC) return EINA_FALSE;
   return (SOCKET)efl_io_closer_fd_get(o) == INVALID_SOCKET;
}

/**
 * @brief Reads data from the socket.
 *
 * Implements the Efl.Io.Reader.read interface.
 * It attempts to receive data into the provided Eina_Rw_Slice.
 * Handles EINTR errors by retrying the recv call.
 * Sets can_read to false after a successful read or error,
 * and sets eos to true if recv returns 0 (connection closed by peer).
 *
 * @param o The Efl_Net_Socket_Fd object.
 * @param pd Private data (unused).
 * @param rw_slice The read-write slice to store the received data.
 *                 On success, rw_slice->len is updated with the number of bytes read.
 *                 On error or EOF, rw_slice->len is 0 and rw_slice->mem is NULL.
 * @return 0 on success, or an error code on failure.
 */
EOLIAN static Eina_Error
_efl_net_socket_fd_efl_io_reader_read(Eo *o, Efl_Net_Socket_Fd_Data *pd EINA_UNUSED, Eina_Rw_Slice *rw_slice)
{
   SOCKET fd = efl_io_reader_fd_get(o);
   ssize_t r;

   EINA_SAFETY_ON_NULL_RETURN_VAL(rw_slice, EINVAL);
   if (fd == INVALID_SOCKET) goto error;
   do
     {
        r = recv(fd, rw_slice->mem, rw_slice->len, 0);
        if (r == SOCKET_ERROR)
          {
             Eina_Error err = efl_net_socket_error_get();

             if (err == EINTR) continue;

             rw_slice->len = 0;
             rw_slice->mem = NULL;

             efl_io_reader_can_read_set(o, EINA_FALSE);
             return err;
          }
     }
   while (r == SOCKET_ERROR);

   rw_slice->len = r;
   efl_io_reader_can_read_set(o, EINA_FALSE); /* wait Efl.Loop.Fd "read" */
   if (r == 0)
     efl_io_reader_eos_set(o, EINA_TRUE);

   return 0;

 error:
   rw_slice->len = 0;
   rw_slice->mem = NULL;
   efl_io_reader_can_read_set(o, EINA_FALSE);
   return EINVAL;
}

/**
 * @brief Sets the can_read property and manages read event monitoring.
 *
 * Implements the Efl.Io.Reader.can_read_set interface.
 * When can_read is set to true (meaning data is available or an event occurred),
 * it stops monitoring the EFL_LOOP_FD_EVENT_READ event, as the user is expected
 * to call efl_io_reader_read().
 * When can_read is set to false (either after a read or if no data is available),
 * it resumes monitoring the EFL_LOOP_FD_EVENT_READ event.
 *
 * @param o The Efl_Net_Socket_Fd object.
 * @param pd Private data (unused).
 * @param value The new value for the can_read property.
 */
EOLIAN static void
_efl_net_socket_fd_efl_io_reader_can_read_set(Eo *o, Efl_Net_Socket_Fd_Data *pd EINA_UNUSED, Eina_Bool value)
{
   Eina_Bool old = efl_io_reader_can_read_get(o);
   if (old == value) return;

   efl_io_reader_can_read_set(efl_super(o, MY_CLASS), value);

   if (value)
     {
        /* stop monitoring the FD, we need to wait the user to read and clear the kernel flag */
        efl_event_callback_del(o, EFL_LOOP_FD_EVENT_READ, _efl_net_socket_fd_event_read, NULL);
     }
   else
     {
        /* kernel flag is clear, resume monitoring the FD */
        efl_event_callback_add(o, EFL_LOOP_FD_EVENT_READ, _efl_net_socket_fd_event_read, NULL);
     }
}

EOLIAN static void
_efl_net_socket_fd_efl_io_reader_eos_set(Eo *o, Efl_Net_Socket_Fd_Data *pd EINA_UNUSED, Eina_Bool value)
{
   Eina_Bool old = efl_io_reader_eos_get(o);
   if (old == value) return;

   efl_io_reader_eos_set(efl_super(o, MY_CLASS), value);

   if (!value) return;

   /* stop monitoring the FD, it's closed or an error occurred */
   efl_event_callback_del(o, EFL_LOOP_FD_EVENT_READ, _efl_net_socket_fd_event_read, NULL);
   efl_event_callback_del(o, EFL_LOOP_FD_EVENT_WRITE, _efl_net_socket_fd_event_write, NULL);
}

/**
 * @brief Writes data to the socket.
 *
 * Implements the Efl.Io.Writer.write interface.
 * It attempts to send data from the provided Eina_Slice.
 * Handles EINTR errors by retrying the send call.
 * Sets can_write to false after a successful write or error.
 * If `remaining` is provided, it will be updated with any data that was not sent.
 *
 * @param o The Efl_Net_Socket_Fd object.
 * @param pd Private data (unused).
 * @param ro_slice The slice containing data to write. On success, ro_slice->len
 *                 is updated with the number of bytes written.
 * @param remaining Optional slice to store any unsent data.
 *                  Example: If ro_slice has 100 bytes and only 60 are sent,
 *                           remaining will point to the last 40 bytes of the original
 *                           buffer and its len will be 40.
 * @return 0 on success, or an error code on failure.
 */
EOLIAN static Eina_Error
_efl_net_socket_fd_efl_io_writer_write(Eo *o, Efl_Net_Socket_Fd_Data *pd EINA_UNUSED, Eina_Slice *ro_slice, Eina_Slice *remaining)
{
   SOCKET fd = efl_io_writer_fd_get(o);
   ssize_t r;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ro_slice, EINVAL);
   if (fd == INVALID_SOCKET) goto error;

   do
     {
        r = send(fd, ro_slice->mem, ro_slice->len, 0);
        if (r == SOCKET_ERROR)
          {
             Eina_Error err = efl_net_socket_error_get();

             if (err == EINTR) continue;

             if (remaining) *remaining = *ro_slice;
             ro_slice->len = 0;
             ro_slice->mem = NULL;
             efl_io_writer_can_write_set(o, EINA_FALSE);
             return err;
          }
     }
   while (r == SOCKET_ERROR);

   if (remaining)
     {
        remaining->len = ro_slice->len - r;
        remaining->bytes = ro_slice->bytes + r;
     }
   ro_slice->len = r;
   efl_io_writer_can_write_set(o, EINA_FALSE); /* wait Efl.Loop.Fd "write" */

   return 0;

 error:
   if (remaining) *remaining = *ro_slice;
   ro_slice->len = 0;
   ro_slice->mem = NULL;
   efl_io_writer_can_write_set(o, EINA_FALSE);
   return EINVAL;
}

/**
 * @brief Sets the can_write property and manages write event monitoring.
 *
 * Implements the Efl.Io.Writer.can_write_set interface.
 * When can_write is set to true (meaning the socket is ready for writing or an event occurred),
 * it stops monitoring the EFL_LOOP_FD_EVENT_WRITE event, as the user is expected
 * to call efl_io_writer_write().
 * When can_write is set to false (either after a write or if the socket is not ready),
 * it resumes monitoring the EFL_LOOP_FD_EVENT_WRITE event.
 *
 * @param o The Efl_Net_Socket_Fd object.
 * @param pd Private data (unused).
 * @param value The new value for the can_write property.
 */
EOLIAN static void
_efl_net_socket_fd_efl_io_writer_can_write_set(Eo *o, Efl_Net_Socket_Fd_Data *pd EINA_UNUSED, Eina_Bool value)
{
   Eina_Bool old = efl_io_writer_can_write_get(o);
   if (old == value) return;

   efl_io_writer_can_write_set(efl_super(o, MY_CLASS), value);

   if (value)
     {
        /* stop monitoring the FD, we need to wait the user to write and clear the kernel flag */
        efl_event_callback_del(o, EFL_LOOP_FD_EVENT_WRITE, _efl_net_socket_fd_event_write, NULL);
     }
   else
     {
        /* kernel flag is clear, resume monitoring the FD */
        efl_event_callback_add(o, EFL_LOOP_FD_EVENT_WRITE, _efl_net_socket_fd_event_write, NULL);
     }
}

EOLIAN static void
_efl_net_socket_fd_efl_net_socket_address_local_set(Eo *o EINA_UNUSED, Efl_Net_Socket_Fd_Data *pd, const char *address)
{
   eina_stringshare_replace(&pd->address_local, address);
}

EOLIAN static const char *
_efl_net_socket_fd_efl_net_socket_address_local_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Fd_Data *pd)
{
   return pd->address_local;
}

EOLIAN static void
_efl_net_socket_fd_efl_net_socket_address_remote_set(Eo *o EINA_UNUSED, Efl_Net_Socket_Fd_Data *pd, const char *address)
{
   eina_stringshare_replace(&pd->address_remote, address);
}

EOLIAN static const char *
_efl_net_socket_fd_efl_net_socket_address_remote_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Fd_Data *pd)
{
   return pd->address_remote;
}

EOLIAN static void
_efl_net_socket_fd_family_set(Eo *o EINA_UNUSED, Efl_Net_Socket_Fd_Data *pd, int family)
{
   pd->family = family;
}

EOLIAN static int
_efl_net_socket_fd_family_get(const Eo *o EINA_UNUSED, Efl_Net_Socket_Fd_Data *pd)
{
   return pd->family;
}

#include "efl_net_socket_fd.eo.c"
