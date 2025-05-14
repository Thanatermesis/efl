#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#ifdef HAVE_IEEEFP_H
# include <ieeefp.h> /* for Solaris */
#endif

#ifdef HAVE_ISFINITE
# define ECORE_FINITE(t) isfinite(t)
#else
# define ECORE_FINITE(t) finite(t)
#endif

#define FIX_HZ 1

#ifdef FIX_HZ
# include <sys/param.h>
# ifndef HZ
#  define HZ 100
# endif
#endif

/*
 * On Windows, pipe() is implemented with sockets.
 * Contrary to Linux, Windows uses different functions
 * for sockets and fd's: write() is for fd's and send
 * is for sockets. So I need to put some win32 code
 * here. I can't think of a solution where the win32
 * code is in Evil and not here.
 */

#define PIPE_FD_INVALID -1

#ifdef _WIN32
# include <winsock2.h>
# include <evil_private.h> /* pipe fcntl */
# define pipe_write(fd, buffer, size) send((fd), (char *)(buffer), size, 0)
# define pipe_read(fd, buffer, size)  recv((fd), (char *)(buffer), size, 0)
# define pipe_close(fd)               closesocket(fd)
# define PIPE_FD_ERROR   SOCKET_ERROR
#else
# ifdef HAVE_SYS_EPOLL_H
#  include <sys/epoll.h>
# endif /* HAVE_SYS_EPOLL_H */
# ifdef HAVE_SYS_TIMERFD_H
#  include <sys/timerfd.h>
# endif
# include <unistd.h>
# include <fcntl.h>
# define pipe_write(fd, buffer, size) write((fd), buffer, size)
# define pipe_read(fd, buffer, size)  read((fd), buffer, size)
# define pipe_close(fd)               close(fd)
# define PIPE_FD_ERROR   -1
#endif /* ! _WIN32 */

#include "Ecore.h"
#include "ecore_private.h"

// How of then we should retry to write to the pipe
#define ECORE_PIPE_WRITE_RETRY 6

/**
 * @brief Structure representing an Ecore_Pipe.
 * @internal
 */
struct _Ecore_Pipe
{
   ECORE_MAGIC; /**< Magic number for type checking. */
   int               fd_read; /**< File descriptor for reading from the pipe. */
   int               fd_write; /**< File descriptor for writing to the pipe. */
   Ecore_Fd_Handler *fd_handler; /**< Fd handler for the read end of the pipe. */
   const void       *data; /**< User data associated with the pipe. */
   Ecore_Pipe_Cb     handler; /**< Callback function to be called when data is read. */
   unsigned int      len; /**< Length of the current message being read. */
   int               handling; /**< Counter to track if the pipe is currently being handled (e.g., in a callback). */
   unsigned int      already_read; /**< Number of bytes already read for the current message. */
   void             *passed_data; /**< Buffer to store incoming data. */
   int               message; /**< Counter for messages processed by _ecore_pipe_wait. */
#ifndef _WIN32
   int               pollfd; /**< epoll instance for _ecore_pipe_wait (Linux specific). */
   int               timerfd; /**< timerfd for _ecore_pipe_wait (Linux specific). */
#endif
   Eina_Bool         delete_me : 1; /**< Flag indicating if the pipe is marked for deletion. */
};
GENERIC_ALLOC_SIZE_DECLARE(Ecore_Pipe);

static Eina_Bool _ecore_pipe_read(void             *data,
                                  Ecore_Fd_Handler *fd_handler);

/**
 * @brief Creates a new pipe.
 *
 * This function creates a new pipe and sets up a handler for the readable
 * end of the pipe.
 *
 * @param handler The function to call when data is available on the pipe.
 * @param data User data to pass to the handler function.
 * @return A new Ecore_Pipe object on success, @c NULL on failure.
 */
EAPI Ecore_Pipe *
ecore_pipe_add(Ecore_Pipe_Cb handler,
               const void   *data)
{
   return _ecore_pipe_add(handler, data);
}

/**
 * @brief Deletes an Ecore_Pipe.
 *
 * This function closes and frees an Ecore_Pipe. If the pipe is currently
 * being handled (e.g., its callback is running), it will be marked for
 * deletion and freed later.
 *
 * @param p The Ecore_Pipe to delete.
 * @return The data pointer originally passed to ecore_pipe_add() or
 *         ecore_pipe_full_add(). @c NULL if p is @c NULL.
 */
EAPI void *
ecore_pipe_del(Ecore_Pipe *p)
{
   if (!p) return NULL;
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   return _ecore_pipe_del(p);
}

/**
 * @brief Closes the read end of an Ecore_Pipe.
 *
 * This function closes the file descriptor used for reading from the pipe
 * and removes the associated fd handler.
 *
 * @param p The Ecore_Pipe whose read end should be closed.
 */
EAPI void
ecore_pipe_read_close(Ecore_Pipe *p)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   if (!ECORE_MAGIC_CHECK(p, ECORE_MAGIC_PIPE))
     {
        ECORE_MAGIC_FAIL(p, ECORE_MAGIC_PIPE, "ecore_pipe_read_close");
        return;
     }
   if (p->fd_handler)
     {
        _ecore_main_fd_handler_del(ML_OBJ, ML_DAT, p->fd_handler);
        p->fd_handler = NULL;
     }
   if (p->fd_read != PIPE_FD_INVALID)
     {
        pipe_close(p->fd_read);
        p->fd_read = PIPE_FD_INVALID;
     }
}

/**
 * @brief Gets the read file descriptor of an Ecore_Pipe.
 *
 * @param p The Ecore_Pipe.
 * @return The file descriptor for reading, or @c PIPE_FD_INVALID on error or if p is @c NULL.
 */
EAPI int
ecore_pipe_read_fd(Ecore_Pipe *p)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(PIPE_FD_INVALID);
   if (!p) return PIPE_FD_INVALID;
   return p->fd_read;
}

/**
 * @brief Freezes an Ecore_Pipe.
 *
 * This function temporarily stops the Ecore_Pipe from listening for read events
 * by deleting its fd handler.
 *
 * @param p The Ecore_Pipe to freeze.
 */
EAPI void
ecore_pipe_freeze(Ecore_Pipe *p)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   if (!ECORE_MAGIC_CHECK(p, ECORE_MAGIC_PIPE))
     {
        ECORE_MAGIC_FAIL(p, ECORE_MAGIC_PIPE, "ecore_pipe_read_freeze");
        return;
     }
   if (p->fd_handler)
     {
        _ecore_main_fd_handler_del(ML_OBJ, ML_DAT, p->fd_handler);
        p->fd_handler = NULL;
     }
}

/**
 * @brief Thaws an Ecore_Pipe.
 *
 * This function resumes listening for read events on an Ecore_Pipe that was
 * previously frozen by ecore_pipe_freeze(). It re-adds the fd handler.
 *
 * @param p The Ecore_Pipe to thaw.
 */
EAPI void
ecore_pipe_thaw(Ecore_Pipe *p)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   if (!ECORE_MAGIC_CHECK(p, ECORE_MAGIC_PIPE))
     {
        ECORE_MAGIC_FAIL(p, ECORE_MAGIC_PIPE, "ecore_pipe_read_thaw");
        return;
     }
   if ((!p->fd_handler) && (p->fd_read != PIPE_FD_INVALID))
     p->fd_handler = ecore_main_fd_handler_add(p->fd_read, ECORE_FD_READ,
                                               _ecore_pipe_read, p,
                                               NULL, NULL);
}

/**
 * @brief Waits for data on an Ecore_Pipe.
 *
 * This function blocks until a specified number of messages are read from
 * the pipe or a timeout occurs.
 *
 * @param p The Ecore_Pipe to wait on.
 * @param message_count The number of messages to wait for.
 * @param wait The maximum time in seconds to wait. A negative value means wait indefinitely.
 * @return The number of messages read, or -1 on error.
 */
EAPI int
ecore_pipe_wait(Ecore_Pipe *p,
                int         message_count,
                double      wait)
{
   return _ecore_pipe_wait(p, message_count, wait);
}

/**
 * @brief Closes the write end of an Ecore_Pipe.
 *
 * @param p The Ecore_Pipe whose write end should be closed.
 */
EAPI void
ecore_pipe_write_close(Ecore_Pipe *p)
{
   if (!ECORE_MAGIC_CHECK(p, ECORE_MAGIC_PIPE))
     {
        ECORE_MAGIC_FAIL(p, ECORE_MAGIC_PIPE, "ecore_pipe_write_close");
        return;
     }
   if (p->fd_write != PIPE_FD_INVALID)
     {
        pipe_close(p->fd_write);
        p->fd_write = PIPE_FD_INVALID;
     }
}

/**
 * @brief Gets the write file descriptor of an Ecore_Pipe.
 *
 * @param p The Ecore_Pipe.
 * @return The file descriptor for writing, or @c PIPE_FD_INVALID on error or if p is @c NULL.
 */
EAPI int
ecore_pipe_write_fd(Ecore_Pipe *p)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(PIPE_FD_INVALID);
   if (!p) return PIPE_FD_INVALID;
   return p->fd_write;
}

/**
 * @brief Writes data to an Ecore_Pipe.
 *
 * This function writes a specified number of bytes from a buffer to the pipe.
 * It first writes the size of the data, then the data itself.
 * It retries writing a few times in case of transient errors.
 *
 * @param p The Ecore_Pipe to write to.
 * @param buffer The buffer containing the data to write.
 * @param nbytes The number of bytes to write from the buffer.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
ecore_pipe_write(Ecore_Pipe  *p,
                 const void  *buffer,
                 unsigned int nbytes)
{
   ssize_t ret;
   size_t already_written = 0;
   int retry = ECORE_PIPE_WRITE_RETRY;
   Eina_Bool ok = EINA_FALSE;
   unsigned int bytes = nbytes;

   if (!ECORE_MAGIC_CHECK(p, ECORE_MAGIC_PIPE))
     {
        ECORE_MAGIC_FAIL(p, ECORE_MAGIC_PIPE, "ecore_pipe_write");
        goto out;
     }

   if (p->delete_me) goto out;

   if (p->fd_write == PIPE_FD_INVALID) goto out;

   do // First write the len into the pipe
     {
        ret = pipe_write(p->fd_write, &bytes, sizeof(bytes));
        if (ret == sizeof(nbytes))
          {
             retry = ECORE_PIPE_WRITE_RETRY;
             break;
          }
        else if (ret > 0)
          {
             // XXX What should we do here?
             ERR("The length of the data was not written complete"
                 " to the pipe");
             goto out;
          }
        else if ((ret == PIPE_FD_ERROR) && (errno == EPIPE))
          {
             pipe_close(p->fd_write);
             p->fd_write = PIPE_FD_INVALID;
             goto out;
          }
        else if ((ret == PIPE_FD_ERROR) && (errno == EINTR))
          // try it again
          ;
        else
          {
             ERR("An unhandled error (ret: %zd errno: %d)"
                 "occurred while writing to the pipe the length",
                 ret, errno);
          }
     }
   while (retry--);

   if (retry != ECORE_PIPE_WRITE_RETRY) goto out;

   do // and now pass the data to the pipe
     {
        ret = pipe_write(p->fd_write,
                         ((unsigned char *)buffer) + already_written,
                         nbytes - already_written);

        if (ret == (ssize_t)(nbytes - already_written))
          {
             ok = EINA_TRUE;
             goto out;
          }
        else if (ret >= 0)
          {
             already_written -= ret;
             continue;
          }
        else if ((ret == PIPE_FD_ERROR) && (errno == EPIPE))
          {
             pipe_close(p->fd_write);
             p->fd_write = PIPE_FD_INVALID;
             goto out;
          }
        else if ((ret == PIPE_FD_ERROR) && (errno == EINTR))
          // try it again
          ;
        else
          {
             ERR("An unhandled error (ret: %zd errno: %d)"
                 "occurred while writing to the pipe the length",
                 ret, errno);
          }
     }
   while (retry--);

out:
   return ok;
}

/**
 * @brief Creates a new pipe with more control over file descriptors.
 *
 * This function creates a new Ecore_Pipe, allowing the use of existing
 * file descriptors or creating new ones. It also allows specifying whether
 * the read and write ends of the pipe should remain open after a fork().
 *
 * @param handler The function to call when data is available on the pipe.
 * @param data User data to pass to the handler function.
 * @param fd_read The file descriptor to use for reading. If -1, a new one is created.
 * @param fd_write The file descriptor to use for writing. If -1, a new one is created.
 * @param read_survive_fork If @c EINA_TRUE, the read fd will not be closed on exec.
 * @param write_survive_fork If @c EINA_TRUE, the write fd will not be closed on exec.
 * @return A new Ecore_Pipe object on success, @c NULL on failure.
 */
EAPI Ecore_Pipe *
ecore_pipe_full_add(Ecore_Pipe_Cb handler,
                    const void   *data,
                    int fd_read,
                    int fd_write,
                    Eina_Bool read_survive_fork,
                    Eina_Bool write_survive_fork)
{
   Ecore_Pipe *p = NULL;
   int fds[2];

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   if (!handler) return NULL;

   p = ecore_pipe_calloc(1);
   if (!p) return NULL;

   if ((fd_read == -1) && (fd_write == -1))
     {
        if (pipe(fds))
          {
             ecore_pipe_mp_free(p);
             return NULL;
          }
        fd_read = fds[0];
        fd_write = fds[1];
     }
   else
     {
        fd_read = (fd_read == -1) ? PIPE_FD_INVALID : fd_read;
        fd_write = (fd_write == -1) ? PIPE_FD_INVALID : fd_write;
     }

   ECORE_MAGIC_SET(p, ECORE_MAGIC_PIPE);
   p->fd_read = fd_read;
   p->fd_write = fd_write;
   p->handler = handler;
   p->data = data;

   if (!read_survive_fork) eina_file_close_on_exec(fd_read, EINA_TRUE);
   if (!write_survive_fork) eina_file_close_on_exec(fd_write, EINA_TRUE);

#if defined(HAVE_SYS_EPOLL_H) && defined(HAVE_SYS_TIMERFD_H)
   struct epoll_event pollev = { 0 };
   p->pollfd = epoll_create(1);
   p->timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
   eina_file_close_on_exec(p->pollfd, EINA_TRUE);

   pollev.data.ptr = &(p->fd_read);
   pollev.events = EPOLLIN;
   epoll_ctl(p->pollfd, EPOLL_CTL_ADD, p->fd_read, &pollev);

   pollev.data.ptr = &(p->timerfd);
   pollev.events = EPOLLIN;
   epoll_ctl(p->pollfd, EPOLL_CTL_ADD, p->timerfd, &pollev);
#endif

   if (fcntl(p->fd_read, F_SETFL, O_NONBLOCK) < 0)
     ERR("can't set pipe to NONBLOCK");
   p->fd_handler = ecore_main_fd_handler_add(p->fd_read, ECORE_FD_READ,
                                             _ecore_pipe_read, p, NULL, NULL);
   return p;
}

// Private functions

/**
 * @internal
 * @brief Internal implementation for ecore_pipe_add.
 *
 * Calls ecore_pipe_full_add with default parameters (new pipe fds,
 * close on fork).
 *
 * @param handler The callback function.
 * @param data User data for the callback.
 * @return A new Ecore_Pipe or @c NULL on error.
 */
Ecore_Pipe *
_ecore_pipe_add(Ecore_Pipe_Cb handler,
                const void   *data)
{
   return ecore_pipe_full_add(handler, data, -1, -1, EINA_FALSE, EINA_FALSE);
}

/**
 * @internal
 * @brief Internal implementation for ecore_pipe_del.
 *
 * Handles the actual deletion of the pipe, including cleaning up
 * fd handlers and closing file descriptors. If the pipe is marked
 * as being handled, deletion is deferred.
 *
 * @param p The Ecore_Pipe to delete.
 * @return The user data associated with the pipe.
 */
void *
_ecore_pipe_del(Ecore_Pipe *p)
{
   void *data = NULL;

   if (!ECORE_MAGIC_CHECK(p, ECORE_MAGIC_PIPE))
     {
        ECORE_MAGIC_FAIL(p, ECORE_MAGIC_PIPE, "ecore_pipe_del");
        return NULL;
     }
#if defined(HAVE_SYS_EPOLL_H) && defined(HAVE_SYS_TIMERFD_H)
   epoll_ctl(p->pollfd, EPOLL_CTL_DEL, p->fd_read, NULL);
   epoll_ctl(p->pollfd, EPOLL_CTL_DEL, p->timerfd, NULL);
   if (p->timerfd >= 0) close(p->timerfd);
   if (p->pollfd >= 0) close(p->pollfd);
   p->timerfd = PIPE_FD_INVALID;
   p->pollfd = PIPE_FD_INVALID;
#endif
   p->delete_me = EINA_TRUE;
   if (p->handling > 0) return (void *)p->data;
   if (p->fd_handler) _ecore_main_fd_handler_del(ML_OBJ, ML_DAT,
                                                 p->fd_handler);
   if (p->fd_read != PIPE_FD_INVALID) pipe_close(p->fd_read);
   if (p->fd_write != PIPE_FD_INVALID) pipe_close(p->fd_write);
   p->fd_handler = NULL;
   p->fd_read = PIPE_FD_INVALID;
   p->fd_write = PIPE_FD_INVALID;
   data = (void *)p->data;
   ecore_pipe_mp_free(p);
   return data;
}

/**
 * @internal
 * @brief Decrements the handling counter and deletes the pipe if marked.
 *
 * This function is called after a pipe operation (like a read callback)
 * is finished. It decrements the `handling` counter. If the pipe was
 * marked for deletion (`delete_me` is true) and `handling` drops to 0,
 * it calls `_ecore_pipe_del` to perform the actual deletion.
 *
 * @param p The Ecore_Pipe.
 */
static void
_ecore_pipe_unhandle(Ecore_Pipe *p)
{
   p->handling--;
   if (p->delete_me) _ecore_pipe_del(p);
}

#if ! defined(HAVE_SYS_EPOLL_H) || ! defined(HAVE_SYS_TIMERFD_H)
/**
 * @internal
 * @brief Waits for messages on a pipe using select().
 *
 * This is the implementation of ecore_pipe_wait for systems that
 * do not have epoll and timerfd. It uses select() to monitor the
 * pipe's read file descriptor.
 *
 * @param p The Ecore_Pipe to wait on.
 * @param message_count The number of messages to wait for.
 * @param wait The maximum time in seconds to wait.
 * @return The number of messages read, or -1 on error.
 */
int
_ecore_pipe_wait(Ecore_Pipe *p,
                 int         message_count,
                 double      wait)
{
   struct timeval tv, *t;
   fd_set rset, wset, exset;
   double end = 0.0;
   double timeout;
   int ret;
   int total = 0;

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(-1);
   if (p->fd_read == PIPE_FD_INVALID) return -1;

   FD_ZERO(&rset);
   FD_ZERO(&wset);
   FD_ZERO(&exset);
   FD_SET(p->fd_read, &rset);

   if (wait >= 0.0) end = ecore_time_get() + wait;
   timeout = wait;

   while ((message_count > 0) && ((timeout > 0.0) || (wait <= 0.0)))
     {
        if (wait >= 0.0)
          {
             // finite() tests for NaN, too big, too small, and infinity.
             if ((!ECORE_FINITE(timeout)) || (EINA_DBL_EQ(timeout, 0.0)))
               {
                  tv.tv_sec = 0;
                  tv.tv_usec = 0;
               }
             else if (timeout > 0.0)
               {
                  int sec, usec;
#ifdef FIX_HZ
                  timeout += (0.5 / HZ);
#endif
                  sec = (int)timeout;
                  usec = (int)((timeout - (double)sec) * 1000000);
                  tv.tv_sec = sec;
                  tv.tv_usec = usec;
               }
             t = &tv;
          }
        else t = NULL;

#ifdef _WIN32
        ret = ecore_main_win32_select(p->fd_read + 1, &rset, &wset, &exset, t);
#else
        ret = select(p->fd_read + 1, &rset, &wset, &exset, t);
#endif

        if (ret > 0)
          {
             p->handling++;
             _ecore_pipe_read(p, NULL);
             message_count -= p->message;
             total += p->message;
             p->message = 0;
             _ecore_pipe_unhandle(p);
          }
        else if (ret == 0) break;
        else if (errno != EINTR)
          {
             if (p->fd_read != PIPE_FD_INVALID)
               {
                  close(p->fd_read);
                  p->fd_read = PIPE_FD_INVALID;
               }
             break;
          }

        if (wait >= 0.0) timeout = end - ecore_time_get();
     }

   return total;
}

#else
/**
 * @internal
 * @brief Waits for messages on a pipe using epoll() and timerfd().
 *
 * This is the implementation of ecore_pipe_wait for systems that
 * support epoll and timerfd (typically Linux). It uses epoll to
 * monitor both the pipe's read file descriptor and a timerfd for timeouts.
 *
 * @param p The Ecore_Pipe to wait on.
 * @param message_count The number of messages to wait for.
 * @param wait The maximum time in seconds to wait.
 * @return The number of messages read, or -1 on error or if epoll_wait fails.
 */
int
_ecore_pipe_wait(Ecore_Pipe *p,
                 int         message_count,
                 double      wait)
{
   int64_t timerfdbuf;
   struct epoll_event pollincoming[2];
   double timeout;
   int ret = 0;
   int total = 0;
   int time_exit = -1;
   Eina_Bool fd_read_found;
   Eina_Bool fd_timer_found;
   struct itimerspec tspec_new;

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(-1);
   if (p->fd_read == PIPE_FD_INVALID) return -1;

   timeout = wait;
   int sec, usec;
   if (wait >= 0.0)
     {
        if ((!ECORE_FINITE(timeout)) || (EINA_DBL_EQ(timeout, 0.0)))
          {
             tspec_new.it_value.tv_sec = 0;
             tspec_new.it_value.tv_nsec = 0;
             tspec_new.it_interval.tv_sec = 0;
             tspec_new.it_interval.tv_nsec = 0;
             time_exit = 0;
          }
        else
          {
#ifdef FIX_HZ
             timeout += (0.5 / HZ);
#endif
             sec = (int)timeout;
             usec = (int)((timeout - (double)sec) * 1000000000);
             tspec_new.it_value.tv_sec = sec;
             tspec_new.it_value.tv_nsec = (int)(usec) % 1000000000;
             tspec_new.it_interval.tv_sec = 0;
             tspec_new.it_interval.tv_nsec = 0;
             timerfd_settime(p->timerfd, 0, &tspec_new, NULL);
          }
     }

   while ((p->pollfd != PIPE_FD_INVALID) && (ret = epoll_wait(p->pollfd, pollincoming, 2, time_exit)) > 0)
     {
        fd_read_found  = EINA_FALSE;
        fd_timer_found = EINA_FALSE;

        for (int i = 0; i < ret;i++)
          {
             if ((&p->fd_read == pollincoming[i].data.ptr))
               fd_read_found  = EINA_TRUE;
             if ((&p->timerfd == pollincoming[i].data.ptr))
               fd_timer_found = EINA_TRUE;
          }

        p->handling++;
        if (fd_read_found)
          {
             _ecore_pipe_read(p, NULL);
             message_count -= p->message;
             total += p->message;
             p->message = 0;
             if (message_count <= 0)
               {
                  _ecore_pipe_unhandle(p);
                  break;
               }
          }

        if ((fd_timer_found) && (p->timerfd != PIPE_FD_INVALID))
          {
             if (pipe_read(p->timerfd, &timerfdbuf, sizeof(timerfdbuf)) <
                 (int)sizeof(int64_t))
               WRN("Could not read timerfd data");
             _ecore_pipe_unhandle(p);
             break;
          }
        _ecore_pipe_unhandle(p);
     }
   if (ret < 0)
     {
        if (errno != EBADF)
          WRN("epoll file descriptor is not a valid");
        else if (errno != EINVAL)
          WRN("epoll file descriptor is not an epoll file descriptor, or maxevents is less than or equal to zero.");
        else if (errno != EFAULT)
          WRN("The memory area pointed to by epoll_event is not accessible with write permissions.");
        else if (errno != EINTR)
          WRN("The call was interrupted by a signal handler before any of the requested epoll_event "
              "occurred or the timeout expired; see signal(7).");
     }
   return total;
}

#endif
/**
 * @internal
 * @brief Calls the user-provided handler for a received pipe message.
 *
 * This function is responsible for invoking the callback function
 * associated with the Ecore_Pipe when a complete message has been read.
 * It resets pipe state related to the current message before calling
 * the handler and frees the buffer after the handler returns.
 *
 * @param p The Ecore_Pipe.
 * @param buf The buffer containing the received message data. Can be @c NULL if len is 0.
 * @param len The length of the received message in bytes.
 */
static void
_ecore_pipe_handler_call(Ecore_Pipe *p,
                         unsigned char *buf,
                         size_t len)
{
   void *data = (void*) p->data;

   // clear all values of pipe first.
   p->passed_data = NULL;
   p->already_read = 0;
   p->len = 0;
   p->message++;

   if (!p->delete_me) p->handler(data, buf, len);

   // free p->passed_data
   free(buf);
}

/**
 * @internal
 * @brief Reads data from the pipe and calls the handler.
 *
 * This function is called by the fd handler when the read end of the pipe
 * is readable. It attempts to read the message length, then the message
 * data. Once a complete message is read, it calls _ecore_pipe_handler_call.
 * It handles partial reads and various error conditions.
 *
 * @param data The Ecore_Pipe structure.
 * @param fd_handler The Ecore_Fd_Handler that triggered this call (unused).
 * @return ECORE_CALLBACK_RENEW to keep the fd handler active,
 *         ECORE_CALLBACK_CANCEL to remove it (e.g., on error or pipe close).
 */
static Eina_Bool
_ecore_pipe_read(void             *data,
                 Ecore_Fd_Handler *fd_handler EINA_UNUSED)
{
   Ecore_Pipe *p = (Ecore_Pipe *)data;
   int i;

   p->handling++;
   for (i = 0; i < 16; i++)
     {
        ssize_t ret;

        // if we already have read some data we don't need to read the len
        // but to finish the already started job
        if (p->len == 0)
          {
             // read the len of the passed data
             ret = pipe_read(p->fd_read, &p->len, sizeof(p->len));

             // catch the non error case first
             // read amount ok - nothing more to do
             if (ret == sizeof(p->len))
               ;
             else if (ret > 0)
               {
                  // we got more data than we asked for - definite error
                  ERR("Only read %i bytes from the pipe, although"
                      " we need to read %i bytes.",
                      (int)ret, (int)sizeof(p->len));
                  _ecore_pipe_unhandle(p);
                  return ECORE_CALLBACK_CANCEL;
               }
             else if (ret == 0)
               {
                  // we got no data
                  if (i == 0)
                    {
                       // no data on first try through means an error
                       _ecore_pipe_handler_call(p, NULL, 0);
                       pipe_close(p->fd_read);
                       p->fd_read = PIPE_FD_INVALID;
                       p->fd_handler = NULL;
                       _ecore_pipe_unhandle(p);
                       return ECORE_CALLBACK_CANCEL;
                    }
                  else
                    {
                       // no data after first loop try is ok
                       _ecore_pipe_unhandle(p);
                       return ECORE_CALLBACK_RENEW;
                    }
               }
#ifndef _WIN32
             else if ((ret == PIPE_FD_ERROR) &&
                      ((errno == EINTR) || (errno == EAGAIN)))
                {
                   _ecore_pipe_unhandle(p);
                   return ECORE_CALLBACK_RENEW;
                }
             else
               {
                  ERR("An unhandled error (ret: %i errno: %i [%s])"
                      "occurred while reading from the pipe the length",
                      (int)ret, errno, strerror(errno));
                  _ecore_pipe_unhandle(p);
                  return ECORE_CALLBACK_RENEW;
               }
#else
             else // ret == PIPE_FD_ERROR is the only other case on Windows
                {
                   if (WSAGetLastError() != WSAEWOULDBLOCK)
                     {
                        _ecore_pipe_handler_call(p, NULL, 0);
                        pipe_close(p->fd_read);
                        p->fd_read = PIPE_FD_INVALID;
                        p->fd_handler = NULL;
                        _ecore_pipe_unhandle(p);
                        return ECORE_CALLBACK_CANCEL;
                     }
                }
#endif
          }
        // if somehow we got less than or equal to 0 we got an errnoneous
        // messages so call callback with null and len we got. this case should
        // never happen
        if (p->len == 0)
          {
             _ecore_pipe_handler_call(p, NULL, 0);
             _ecore_pipe_unhandle(p);
             return ECORE_CALLBACK_RENEW;
          }

        // we dont have a buffer to hold the data, so alloc it
        if (!p->passed_data)
          {
             p->passed_data = malloc(p->len);
             // alloc failed - error case
             if (!p->passed_data)
               {
                  _ecore_pipe_handler_call(p, NULL, 0);
                  // close the pipe
                  pipe_close(p->fd_read);
                  p->fd_read = PIPE_FD_INVALID;
                  p->fd_handler = NULL;
                  _ecore_pipe_unhandle(p);
                  return ECORE_CALLBACK_CANCEL;
               }
          }

        // and read the passed data
        ret = pipe_read(p->fd_read,
                        ((unsigned char *)p->passed_data) + p->already_read,
                        p->len - p->already_read);

        // catch the non error case first
        // if we read enough data to finish the message/buffer
        if (ret == (ssize_t)(p->len - p->already_read))
          _ecore_pipe_handler_call(p, p->passed_data, p->len);
        else if (ret > 0)
          {
             // more data left to read
             p->already_read += (unsigned int)ret;
             _ecore_pipe_unhandle(p);
             return ECORE_CALLBACK_RENEW;
          }
        else if (ret == 0)
          {
             // 0 bytes to read - could be more to read next select wake up
             _ecore_pipe_unhandle(p);
             return ECORE_CALLBACK_RENEW;
          }
#ifndef _WIN32
        else if ((ret == PIPE_FD_ERROR) &&
                 ((errno == EINTR) || (errno == EAGAIN)))
          {
             _ecore_pipe_unhandle(p);
             return ECORE_CALLBACK_RENEW;
          }
        else
          {
             ERR("An unhandled error (ret: %zd errno: %d)"
                 "occurred while reading from the pipe the data",
                 ret, errno);
             _ecore_pipe_unhandle(p);
             return ECORE_CALLBACK_RENEW;
          }
#else
        else // ret == PIPE_FD_ERROR is the only other case on Windows
          {
             if (WSAGetLastError() != WSAEWOULDBLOCK)
               {
                  _ecore_pipe_handler_call(p, NULL, 0);
                  pipe_close(p->fd_read);
                  p->fd_read = PIPE_FD_INVALID;
                  p->fd_handler = NULL;
                  _ecore_pipe_unhandle(p);
                  return ECORE_CALLBACK_CANCEL;
               }
             else break;
          }
#endif
     }

   _ecore_pipe_unhandle(p);
   return ECORE_CALLBACK_RENEW;
}

