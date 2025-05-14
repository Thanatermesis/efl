#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <direct.h>
# include <sys/time.h>

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#undef WIN32_LEAN_AND_MEAN

#include "evil_private.h"


LONGLONG _evil_time_freq;
LONGLONG _evil_time_count;

/*
 * Time related functions
 *
 */

/**
 * @brief Return the time spent since the Evil library has been initialized.
 * @return The time spent in seconds.
 *
 * This function returns the time spent since the Evil library has
 * been initialized. It uses a high-resolution timer (QueryPerformanceCounter)
 * and then can have a precision up to the nano-second. The precision is
 * processor dependant. This function can be used to benchmark parts of code
 * with high precision.
 *
 * @see _evil_time_count
 * @see _evil_time_freq
 */
EVIL_API double
evil_time_get(void)
{
   LARGE_INTEGER count;

   QueryPerformanceCounter(&count);

   return (double)(count.QuadPart - _evil_time_count)/ (double)_evil_time_freq;
}


/*
 * Sockets and pipe related functions
 *
 */

/**
 * @brief Initiates the use of Windows sockets (Winsock).
 * @return 1 on success, 0 otherwise.
 *
 * This function calls WSAStartup to initialize the Winsock library.
 * It requests version 2.2 of Winsock. If initialization is successful
 * and the correct version is supported, it returns 1. Otherwise, it
 * calls WSACleanup (if WSAStartup succeeded but version is wrong) and
 * returns 0.
 */
EVIL_API int
evil_sockets_init(void)
{
   WSADATA wsa_data;
   WORD version;

   version = MAKEWORD(2, 2);
   if (WSAStartup(version, &wsa_data) == 0)
     {
        if ((LOBYTE(wsa_data.wVersion) == 2) &&
            (HIBYTE(wsa_data.wVersion) == 2))
          return 1;
        else
          {
             WSACleanup();
             return 0;
          }
     }

   return 0;
}

/**
 * @brief Shuts down the Windows socket system.
 *
 * This function calls WSACleanup to terminate the use of the
 * Winsock library.
 */
EVIL_API void
evil_sockets_shutdown(void)
{
   WSACleanup();
}

/*
 * The code of the following functions has been kindly offered
 * by Tor Lillqvist.
 */

/**
 * @brief Create a pair of connected sockets.
 * @param[out] fds A pointer to an integer array of size 2.
 *                 On success, fds[0] will be the read end and
 *                 fds[1] will be the write end of the pipe.
 *                 Example: int sockets[2]; evil_pipe(sockets);
 * @return 0 on success, -1 on error.
 *
 * This function creates a pair of connected sockets that emulate a Unix pipe.
 * It works by creating a listening socket on the loopback interface,
 * then connecting a client socket to it, and finally accepting the
 * connection to get the second socket.
 * The sockets are set to blocking mode after establishment.
 * If any step fails, previously created sockets are closed, and fds[0]
 * and fds[1] are set to -1.
 */
EVIL_API int
evil_pipe(int *fds)
{
   struct sockaddr_in saddr;
   SOCKET             temp;
   SOCKET             socket1 = INVALID_SOCKET;
   SOCKET             socket2 = INVALID_SOCKET;
   u_long             arg;
   fd_set             read_set;
   fd_set             write_set;
   int                len;

   temp = socket (AF_INET, SOCK_STREAM, 0);

   if (temp == INVALID_SOCKET)
     goto out0;

   arg = 1;
   if (ioctlsocket (temp, FIONBIO, &arg) == SOCKET_ERROR)
     goto out0;

   memset (&saddr, 0, sizeof (saddr));
   saddr.sin_family = AF_INET;
   saddr.sin_port = 0;
   saddr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

   if (bind (temp, (struct sockaddr *)&saddr, sizeof (saddr)))
     goto out0;

   if (listen (temp, 1) == SOCKET_ERROR)
     goto out0;

   len = sizeof (saddr);
   if (getsockname (temp, (struct sockaddr *)&saddr, &len))
     goto out0;

   socket1 = socket (AF_INET, SOCK_STREAM, 0);

   if (socket1 == INVALID_SOCKET)
     goto out0;

   arg = 1;
   if (ioctlsocket (socket1, FIONBIO, &arg) == SOCKET_ERROR)
      goto out1;

   if ((connect (socket1, (struct sockaddr  *)&saddr, len) == SOCKET_ERROR) &&
       (WSAGetLastError () != WSAEWOULDBLOCK))
     goto out1;

   FD_ZERO (&read_set);
   FD_SET (temp, &read_set);

   if (select (0, &read_set, NULL, NULL, NULL) == SOCKET_ERROR)
     goto out1;

   if (!FD_ISSET (temp, &read_set))
     goto out1;

   socket2 = accept (temp, (struct sockaddr *) &saddr, &len);
   if (socket2 == INVALID_SOCKET)
     goto out1;

   FD_ZERO (&write_set);
   FD_SET (socket1, &write_set);

   if (select (0, NULL, &write_set, NULL, NULL) == SOCKET_ERROR)
     goto out2;

   if (!FD_ISSET (socket1, &write_set))
     goto out2;

   arg = 0;
   if (ioctlsocket (socket1, FIONBIO, &arg) == SOCKET_ERROR)
     goto out2;

   arg = 0;
   if (ioctlsocket (socket2, FIONBIO, &arg) == SOCKET_ERROR)
     goto out2;

   fds[0] = socket1;
   fds[1] = socket2;

   closesocket (temp);

   return 0;

 out2:
   closesocket (socket2);
 out1:
   closesocket (socket1);
 out0:
   closesocket (temp);

   fds[0] = -1;
   fds[1] = -1;

   return -1;
}
