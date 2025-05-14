#define EFL_NET_SERVER_UNIX_PROTECTED 1
#define EFL_NET_SERVER_FD_PROTECTED 1
#define EFL_NET_SERVER_PROTECTED 1
#define EFL_LOOP_FD_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

// BSD workaround - unable to reproduce.... but we seem to
// get a blocking bind here if 2 processes fight over the same
// socket where one of them loses out by sitting here and
// blockign forever - as i can't reproduce in the freebsd vm
// i have, so i'm limited in what to do so this is a
// workaround to try mitigate this
#if defined (__FreeBSD__)
# define BIND_HANG_WORKAROUND 1
#else
// only need on freebsd
//# define BIND_HANG_WORKAROUND 1
#endif

#ifdef BIND_HANG_WORKAROUND
# include <sys/file.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
#endif

/* no include EVIL as it's not supposed to be compiled on Windows */

#define MY_CLASS EFL_NET_SERVER_UNIX_CLASS

/**
 * @brief Private data for the Efl_Net_Server_Unix class.
 */
typedef struct _Efl_Net_Server_Unix_Data
{
   unsigned int leading_directories_create_mode; /**< Permissions mode for creating leading directories. Example: 0700 */
#ifdef BIND_HANG_WORKAROUND
   int lock_fd; /**< File descriptor for the bind hang workaround lock file. */
   Eina_Bool have_lock_fd : 1; /**< Flag indicating if the lock_fd is valid. */
#endif
   Eina_Bool leading_directories_create : 1; /**< If EINA_TRUE, create leading directories for the socket path. */
   Eina_Bool unlink_before_bind : 1; /**< If EINA_TRUE, unlink the socket path before binding. */
} Efl_Net_Server_Unix_Data;

#ifdef BIND_HANG_WORKAROUND
/**
 * @brief Workaround for a potential bind hang issue on FreeBSD.
 *
 * This function attempts to acquire or release a lock file associated with the
 * given socket address. This is used to prevent multiple processes from
 * attempting to bind to the same socket simultaneously, which can cause a hang
 * on some systems.
 *
 * @param address The socket address (path) for which to manage the lock.
 *                Example: "/tmp/my_server.sock"
 * @param lock If EINA_TRUE, acquire the lock. If EINA_FALSE, release the lock.
 * @param lockfile_fd The file descriptor of an existing lock file if releasing.
 *                    If acquiring, this is ignored and a new fd is created.
 * @return The file descriptor of the lock file if acquired successfully,
 *         -1 otherwise. When releasing, it also returns -1.
 */
static int
_efl_net_server_unix_bind_hang_lock_workaround(const char *address, Eina_Bool lock, int lockfile_fd)
{
   size_t addrlen;
   char *lockfile;
   int ret;

   addrlen = strlen(address);
   lockfile = malloc(addrlen + 1 + 5);
   if (!lockfile) return -1;

   strcpy(lockfile, address);
   strncpy(lockfile + addrlen, ".lock", 6);
   if (lock)
     {
#ifdef HAVE_OPEN_CLOEXEC
        lockfile_fd = open(lockfile, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC,
                           S_IRUSR | S_IWUSR);
        if (lockfile_fd < 0) return -1;
#else
        lockfile_fd = open(lockfile, O_RDWR | O_CREAT | O_TRUNC,
                           S_IRUSR | S_IWUSR);
        if (lockfile_fd < 0) return -1;
        eina_file_close_on_exec(lockfile_fd, EINA_TRUE);
#endif
     }

   errno = 0;
   if (lock)
     {
        ret = flock(lockfile_fd, LOCK_EX | LOCK_NB);
        if ((ret != 0) && (errno == EWOULDBLOCK))
          {
             close(lockfile_fd);
             lockfile_fd = -1;
          }
     }
   else
     {
        flock(lockfile_fd, LOCK_UN | LOCK_NB);
        unlink(lockfile);
        close(lockfile_fd);
        lockfile_fd = -1;
     }
   free(lockfile);
   return lockfile_fd;
}
#endif

/**
 * @brief Destructor for the Efl_Net_Server_Unix object.
 *
 * Cleans up resources, including unlinking the socket file if it's not
 * an abstract socket and releasing any bind hang workaround locks.
 *
 * @param o The Efl_Net_Server_Unix object.
 * @param pd The private data for the object.
 */
EOLIAN static void
_efl_net_server_unix_efl_object_destructor(Eo *o, Efl_Net_Server_Unix_Data *pd EINA_UNUSED)
{
   SOCKET fd = efl_loop_fd_get(o);
   const char *address = efl_net_server_address_get(o);

   if (fd != INVALID_SOCKET)
     {
        if ((address) &&
            (strncmp(address, "abstract:", strlen("abstract:")) != 0))
          unlink(address);
     }
#ifdef BIND_HANG_WORKAROUND
   if ((address) && (pd->have_lock_fd) && (pd->lock_fd >= 0))
     {
        _efl_net_server_unix_bind_hang_lock_workaround
          (address, EINA_FALSE, pd->lock_fd);
        pd->lock_fd = -1;
        pd->have_lock_fd = EINA_FALSE;
     }
#endif

   efl_destructor(efl_super(o, MY_CLASS));
}

/**
 * @brief Binds the server to the specified Unix domain socket address.
 *
 * This function handles the creation of the socket, setting up the address
 * structure (including abstract namespace sockets), optionally creating
 * leading directories, and unlinking an existing socket file before binding.
 * It also incorporates a workaround for potential bind hangs on FreeBSD.
 *
 * @param o The Efl_Net_Server_Unix object.
 * @param pd The private data for the object.
 * @return 0 on success, or a standard errno value on failure.
 *         Example return values: 0 (success), EADDRINUSE, ENOENT.
 */
static Eina_Error
_efl_net_server_unix_bind(Eo *o, Efl_Net_Server_Unix_Data *pd)
{
   const char *address = efl_net_server_address_get(o);
   struct sockaddr_un addr = { .sun_family = AF_UNIX };
   socklen_t addrlen;
   SOCKET fd = INVALID_SOCKET;
   Eina_Error err = 0;
   int r;

   efl_net_server_fd_family_set(o, AF_UNIX);

   if ((pd->leading_directories_create) &&
       (strncmp(address, "abstract:", strlen("abstract:")) != 0))
     _ecore_con_local_mkpath(address, pd->leading_directories_create_mode);

   do
     {
        fd = efl_net_socket4(AF_UNIX, SOCK_STREAM, 0,
                             efl_net_server_fd_close_on_exec_get(o));
        if (fd == INVALID_SOCKET)
          {
             err = efl_net_socket_error_get();
             ERR("socket(AF_UNIX, SOCK_STREAM, 0): %s", eina_error_msg_get(err));
             goto error;
          }

        if (strncmp(address, "abstract:", strlen("abstract:")) == 0)
          {
             const char *path = address + strlen("abstract:");
             if (strlen(path) + 2 > sizeof(addr.sun_path))
               {
                  ERR("abstract path is too long: %s", path);
                  err = EFL_NET_ERROR_COULDNT_RESOLVE_HOST;
               }
             addr.sun_path[0] = '\0';
             memcpy(addr.sun_path + 1, path, strlen(path) + 1);
             addrlen = strlen(path) + 2 + offsetof(struct sockaddr_un, sun_path);
          }
        else
          {
             const char *path = address;
             if (strlen(path) + 1 > sizeof(addr.sun_path))
               {
                  ERR("path is too long: %s", path);
                  err = EFL_NET_ERROR_COULDNT_RESOLVE_HOST;
               }
             memcpy(addr.sun_path, path, strlen(path) + 1);
             addrlen = strlen(path) + 1 + offsetof(struct sockaddr_un, sun_path);
          }

        if ((pd->unlink_before_bind) && (addr.sun_path[0] != '\0'))
          {
             DBG("unlinking AF_UNIX path '%s'", addr.sun_path);
             unlink(addr.sun_path);
          }

#ifdef BIND_HANG_WORKAROUND
        if (addr.sun_path[0] != '\0')
          {
             pd->lock_fd = _efl_net_server_unix_bind_hang_lock_workaround
               (addr.sun_path, EINA_TRUE, -1);
             if (pd->lock_fd < 0)
               {
                  err = EADDRINUSE;
                  goto error;
               }
             pd->have_lock_fd = EINA_TRUE;
             unlink(addr.sun_path);
          }
#endif
        r = bind(fd, (struct sockaddr *)&addr, addrlen);
        if (r != 0)
          {
             err = efl_net_socket_error_get();
             if ((err == EADDRINUSE) && (pd->unlink_before_bind) && (addr.sun_path[0] != '\0'))
               {
                  closesocket(fd);
                  fd = INVALID_SOCKET;
                  err = 0;
                  continue;
               }
             if ((err == EADDRINUSE) && (addr.sun_path[0] != '\0') &&
                 (connect(fd, (struct sockaddr *)&addr, addrlen) != 0))
               {
                  DBG("bind(" SOCKET_FMT ", %s): failed with EADDRINUSE but connect also failed, so unlink socket file and try again", fd, address);
                  closesocket(fd);
                  unlink(addr.sun_path);
                  fd = INVALID_SOCKET;
                  err = 0;
                  continue;
               }
             DBG("bind(" SOCKET_FMT ", %s): %s", fd, address, eina_error_msg_get(err));
             goto error;
          }
        break;
     }
   while (pd->unlink_before_bind);

   if (fd == INVALID_SOCKET) goto error;
   efl_loop_fd_set(o, fd);

   r = listen(fd, 0);
   if (r != 0)
     {
        err = efl_net_socket_error_get();
        DBG("listen(" SOCKET_FMT "): %s", fd, eina_error_msg_get(err));
        goto error;
     }

   addrlen = sizeof(addr);
   if (getsockname(fd, (struct sockaddr *)&addr, &addrlen) != 0)
     {
        err = efl_net_socket_error_get();
        ERR("getsockname(" SOCKET_FMT "): %s", fd, eina_error_msg_get(err));
        goto error;
     }
   else
     {
        char str[sizeof(addr) + sizeof("abstract:")];
        if (!efl_net_unix_fmt(str, sizeof(str), fd, &addr, addrlen))
          ERR("could not format unix address");
        else
          {
             efl_net_server_address_set(o, str);
             address = efl_net_server_address_get(o);
          }
     }

   DBG("fd=" SOCKET_FMT " serving at %s", fd, address);
   efl_net_server_serving_set(o, EINA_TRUE);

 error:
   if (err)
     {
#ifdef BIND_HANG_WORKAROUND
        if ((pd->have_lock_fd) && (pd->lock_fd >= 0))
          {
             pd->lock_fd = _efl_net_server_unix_bind_hang_lock_workaround
               (addr.sun_path, EINA_FALSE, -1);
             pd->have_lock_fd = EINA_TRUE;
          }
#endif
        efl_event_callback_call(o, EFL_NET_SERVER_EVENT_SERVER_ERROR, &err);
        if (fd != INVALID_SOCKET) closesocket(fd);
        efl_loop_fd_set(o, SOCKET_TO_LOOP_FD(INVALID_SOCKET));
     }
   return err;
}

/**
 * @brief Activates the server using a pre-existing socket (e.g., from systemd socket activation).
 *
 * This function checks if a socket matching the given address is already
 * available (typically via systemd). If so, it uses that socket.
 * Otherwise, it falls back to the standard server activation.
 * It ensures the socket is listening and updates the server's address.
 *
 * @param o The Efl_Net_Server_Unix object.
 * @param pd The private data for the object.
 * @param address The address to serve on. Example: "/tmp/my_server.sock" or "abstract:my_abstract_socket"
 * @return 0 on success, or a standard errno value on failure.
 */
EOLIAN static Eina_Error
_efl_net_server_unix_efl_net_server_fd_socket_activate(Eo *o, Efl_Net_Server_Unix_Data *pd EINA_UNUSED, const char *address)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL((SOCKET)efl_loop_fd_get(o) != INVALID_SOCKET, EALREADY);
   EINA_SAFETY_ON_NULL_RETURN_VAL(address, EINVAL);

#ifndef HAVE_SYSTEMD
   return efl_net_server_fd_socket_activate(efl_super(o, MY_CLASS), address);
#else
   {
      char buf[INET6_ADDRSTRLEN + sizeof("[]:65536")];
      Eina_Bool listening;
      Eina_Error err;
      struct sockaddr_storage addr;
      socklen_t addrlen;
      SOCKET fd;

      err = efl_net_ip_socket_activate_check(address, AF_UNIX, SOCK_STREAM, &listening);
      if (err) return err;

      err = efl_net_server_fd_socket_activate(efl_super(o, MY_CLASS), address);
      if (err) return err;

      fd = efl_loop_fd_get(o);

      if (!listening)
        {
           if (listen(fd, 0) != 0)
             {
                err = efl_net_socket_error_get();
                DBG("listen(" SOCKET_FMT "): %s", fd, eina_error_msg_get(err));
                goto error;
             }
        }

      addrlen = sizeof(addr);
      if (getsockname(fd, (struct sockaddr *)&addr, &addrlen) != 0)
        {
           err = efl_net_socket_error_get();
           ERR("getsockname(" SOCKET_FMT "): %s", fd, eina_error_msg_get(err));
           goto error;
        }
      else if (efl_net_ip_port_fmt(buf, sizeof(buf), (struct sockaddr *)&addr))
        efl_net_server_address_set(o, buf);

      DBG("fd=" SOCKET_FMT " serving at %s", fd, address);
      efl_net_server_serving_set(o, EINA_TRUE);
      return 0;

   error:
      efl_net_server_fd_family_set(o, AF_UNSPEC);
      efl_loop_fd_set(o, SOCKET_TO_LOOP_FD(INVALID_SOCKET));
      closesocket(fd);
      return err;
   }
#endif
}

/**
 * @brief Starts serving on the given Unix domain socket address.
 *
 * Sets the server's address and then calls the internal bind function.
 *
 * @param o The Efl_Net_Server_Unix object.
 * @param pd The private data for the object.
 * @param address The address to serve on. Example: "/tmp/my_server.sock" or "abstract:my_abstract_socket"
 * @return 0 on success, or a standard errno value on failure from _efl_net_server_unix_bind().
 */
EOLIAN static Eina_Error
_efl_net_server_unix_efl_net_server_serve(Eo *o, Efl_Net_Server_Unix_Data *pd, const char *address)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(address, EINVAL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(address[0] == '\0', EINVAL);

   efl_net_server_address_set(o, address);

   return _efl_net_server_unix_bind(o, pd);
}

/**
 * @brief Handles a new accepted client connection.
 *
 * Creates an Efl_Net_Socket_Unix object for the new client, sets its properties
 * (close_on_exec, close_on_invalidate, fd), and announces the new client
 * to the server.
 *
 * @param o The Efl_Net_Server_Unix object (server).
 * @param pd The private data for the server object.
 * @param client_fd The file descriptor of the accepted client socket.
 */
static void
_efl_net_server_unix_efl_net_server_fd_client_add(Eo *o, Efl_Net_Server_Unix_Data *pd EINA_UNUSED, int client_fd)
{
   Eo *client = efl_add(EFL_NET_SOCKET_UNIX_CLASS, o,
                        efl_io_closer_close_on_exec_set(efl_added, efl_net_server_fd_close_on_exec_get(o)),
                        efl_io_closer_close_on_invalidate_set(efl_added, EINA_TRUE),
                        efl_loop_fd_set(efl_added, client_fd));
   if (!client)
     {
        ERR("could not create client object fd=" SOCKET_FMT, (SOCKET)client_fd);
        closesocket(client_fd);
        return;
     }

   efl_net_server_client_announce(o, client);
}

/**
 * @brief Handles a client connection that is rejected.
 *
 * This function is called when a client connection is accepted by the underlying
 * system but is then rejected by the server logic (e.g., due to connection limits).
 * It attempts to get the peer name of the rejected client for logging/event purposes,
 * then closes the client socket and emits a CLIENT_REJECTED event.
 *
 * @param o The Efl_Net_Server_Unix object (server).
 * @param pd The private data for the server object.
 * @param client_fd The file descriptor of the rejected client socket.
 */
static void
_efl_net_server_unix_efl_net_server_fd_client_reject(Eo *o, Efl_Net_Server_Unix_Data *pd EINA_UNUSED, int client_fd)
{
   struct sockaddr_un addr;
   socklen_t addrlen;
   char str[sizeof(addr) + sizeof("abstract:")] = "";

   addrlen = sizeof(addr);
   if (getpeername(client_fd, (struct sockaddr *)&addr, &addrlen) != 0)
     ERR("getpeername(%d): %s", client_fd, eina_error_msg_get(efl_net_socket_error_get()));
   else
     {
        if (!efl_net_unix_fmt(str, sizeof(str), client_fd, &addr, addrlen))
          ERR("could not format rejected client unix address fd=" SOCKET_FMT, (SOCKET)client_fd);
     }

   closesocket(client_fd);
   efl_event_callback_call(o, EFL_NET_SERVER_EVENT_CLIENT_REJECTED, str);
}

/**
 * @brief Sets whether to unlink the socket file before binding.
 *
 * @param o The Efl_Net_Server_Unix object.
 * @param pd The private data for the object.
 * @param unlink_before_bind If EINA_TRUE, unlink before binding.
 */
static void
_efl_net_server_unix_unlink_before_bind_set(Eo *o EINA_UNUSED, Efl_Net_Server_Unix_Data *pd, Eina_Bool unlink_before_bind)
{
   pd->unlink_before_bind = unlink_before_bind;
}

/**
 * @brief Gets whether to unlink the socket file before binding.
 *
 * @param o The Efl_Net_Server_Unix object.
 * @param pd The private data for the object.
 * @return EINA_TRUE if unlinking before binding is enabled, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_net_server_unix_unlink_before_bind_get(const Eo *o EINA_UNUSED, Efl_Net_Server_Unix_Data *pd)
{
   return pd->unlink_before_bind;
}

/**
 * @brief Sets whether to create leading directories for the socket path and the mode for creation.
 *
 * @param o The Efl_Net_Server_Unix object.
 * @param pd The private data for the object.
 * @param do_it If EINA_TRUE, create leading directories.
 * @param mode The file system mode (permissions) to use when creating directories. Example: 0755
 */
static void
_efl_net_server_unix_leading_directories_create_set(Eo *o EINA_UNUSED, Efl_Net_Server_Unix_Data *pd, Eina_Bool do_it, unsigned int mode)
{
   pd->leading_directories_create = do_it;
   pd->leading_directories_create_mode = mode;
}

/**
 * @brief Gets whether leading directories will be created and the mode for creation.
 *
 * @param o The Efl_Net_Server_Unix object.
 * @param pd The private data for the object.
 * @param do_it Pointer to store Eina_Bool indicating if creation is enabled. Can be NULL.
 * @param mode Pointer to store the unsigned int mode for directory creation. Can be NULL.
 */
static void
_efl_net_server_unix_leading_directories_create_get(const Eo *o EINA_UNUSED, Efl_Net_Server_Unix_Data *pd, Eina_Bool *do_it, unsigned int *mode)
{
   if (do_it) *do_it = pd->leading_directories_create;
   if (mode) *mode = pd->leading_directories_create_mode;
}

#include "efl_net_server_unix.eo.c"
