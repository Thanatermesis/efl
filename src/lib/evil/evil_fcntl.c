#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <sys/locking.h>

#include <winsock2.h> /* for ioctlsocket */
#include <io.h>

#include "evil_private.h"

/**
 * @internal
 * @brief Checks if a given file descriptor is a socket.
 *
 * This function uses select() with a very short timeout to determine if the
 * descriptor behaves like a socket. It checks if select() reports readability
 * without error.
 *
 * @param s The file descriptor (cast to SOCKET) to check.
 * @return Non-zero if @p s is a socket, 0 otherwise. SOCKET_ERROR from select
 *         is treated as not a socket.
 */
/* SOCKET is defined as a uintptr_t, so passing a fd (int) is not a problem */
static int
_is_socket(SOCKET s)
{
   fd_set rfds;
   struct timeval tv;

   tv.tv_sec = 0;
   tv.tv_usec = 100;
   FD_ZERO(&rfds);
   FD_SET(s, &rfds);

   return select(1, &rfds, NULL, NULL, &tv) != SOCKET_ERROR;
}


/*
 * port of fcntl function
 *
 */

EVIL_API int
fcntl(int fd, int cmd, ...)
{
   va_list va;
   int     res = -1;

   va_start (va, cmd);

   /*
    * F_GETFD: Get file descriptor flags.
    * Retrieves the close-on-exec flag (FD_CLOEXEC) for the file descriptor.
    */
   if (cmd == F_GETFD)
     {
        HANDLE  h;
        DWORD flag;

        h = _is_socket(fd) ? (HANDLE)(uintptr_t)fd : (HANDLE)_get_osfhandle(fd);
        if (h == INVALID_HANDLE_VALUE)
          return -1;

	if (GetHandleInformation(h, &flag))
          {
             if (flag == HANDLE_FLAG_INHERIT)
               return FD_CLOEXEC;

             return 0;
          }

	return -1;
     }

   /*
    * F_SETFD: Set file descriptor flags.
    * Sets the close-on-exec flag (FD_CLOEXEC) for the file descriptor.
    * Expects a long argument after cmd, which should be FD_CLOEXEC.
    */
   if (cmd == F_SETFD)
     {
        HANDLE  h;
        long flag;

        h = _is_socket(fd) ? (HANDLE)(uintptr_t)fd : (HANDLE)_get_osfhandle(fd);
        if (h == INVALID_HANDLE_VALUE)
          return -1;

        flag = va_arg(va, long);
        if (flag == FD_CLOEXEC)
          {
             if (SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0))
               return 0;
          }
     }
   /*
    * F_GETFL: Get file status flags.
    * This implementation currently does nothing for F_GETFL.
    */
   else if (cmd == F_GETFL)
     {
        /* does nothing*/
     }
   /*
    * F_SETFL: Set file status flags.
    * Currently, only supports setting O_NONBLOCK for sockets.
    * Expects a long argument after cmd containing the flags.
    */
   else if (cmd == F_SETFL)
     {
        long flag;

        flag = va_arg(va, long);
        /* Check if O_NONBLOCK is being set */
        if (flag & O_NONBLOCK)
          {
             u_long arg = 1; /* Argument for ioctlsocket to enable non-blocking mode */
             int    type;
             int    len;
             int    ret;

             len = (int)sizeof(int);
             ret = getsockopt((SOCKET)fd, SOL_SOCKET, SO_TYPE, (char *)&type, &len);
             if (!ret && (type == SOCK_STREAM))
               {
                  if (ioctlsocket((SOCKET)fd, FIONBIO, &arg) != SOCKET_ERROR)
                    res = 0;
               }
          }
     }
   /*
    * F_SETLK & F_SETLKW: Set or release a file lock.
    * F_SETLK attempts to lock and returns immediately if it fails.
    * F_SETLKW waits until the lock can be acquired.
    * Expects a 'struct flock *' argument after cmd.
    */
   else if ((cmd == F_SETLK) || (cmd == F_SETLKW))
     {
        struct flock *fl;
        off_t        length = 0;
        long         pos;

        fl = va_arg(va, struct flock *);

        /* If l_len is 0, it means lock until the end of the file.
         * We need to determine the file size to set l_len appropriately. */
        if (fl->l_len == 0)
          {
             length = _lseek(fd, 0L, SEEK_END); /* Get current file size */
             if (length != -1L)
               res = 0; /* Tentatively set success if lseek works */
             else
               res = -1; /* lseek failed, cannot determine length */
          }
        /* If res is -1 from previous block, or l_len was non-zero,
         * fl->l_len is either user-provided or needs to be calculated.
         * If length was determined (l_len was 0 initially), calculate actual length.
         * Otherwise, use user-provided fl->l_len.
         */
        if (res != -1) { // Only proceed if length determination was okay or not needed
            if (fl->l_len == 0) /* This means it was 0 initially and length was found */
                fl->l_len = length - fl->l_start;
            /* else: fl->l_len was non-zero, use as is. */

            pos = _lseek(fd, fl->l_start, fl->l_whence); /* Position to the start of the lock region */
            if (pos != -1L)
              res = 0; /* Tentatively set success if lseek works */
            else
              res = -1; /* lseek failed */

            if (res != -1) { // Only proceed if positioning was okay
                if ((fl->l_type == F_RDLCK) || (fl->l_type == F_WRLCK))
                  {
                     /* Apply a read or write lock */
                     if (cmd == F_SETLK)
                       res = _locking(fd, _LK_NBLCK, fl->l_len); /* Non-blocking lock attempt */
                     else /* F_SETLKW */
                       res = _locking(fd, _LK_LOCK, fl->l_len); /* Blocking lock attempt */
                  }
                else if (fl->l_type == F_UNLCK)
                  {
                     /* Release a lock */
                     res = _locking(fd, _LK_UNLCK, fl->l_len);
                  }
                else
                  {
                    /* Invalid lock type */
                    res = -1;
                  }
            }
        }
     }

   va_end(va);

   return res;
}
