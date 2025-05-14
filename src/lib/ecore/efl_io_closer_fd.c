#define EFL_IO_CLOSER_FD_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include <fcntl.h>
#include "ecore_private.h"

#define MY_CLASS EFL_IO_CLOSER_FD_MIXIN

/**
 * @brief Private data for the Efl_Io_Closer_Fd mixin.
 *
 * This structure holds the file descriptor and flags related to its closing behavior.
 */
typedef struct _Efl_Io_Closer_Fd_Data
{
   int fd; /**< The file descriptor to be managed. -1 if not set or closed. */

   Eina_Bool close_on_exec; /**< If true, the file descriptor will be closed on exec. */
   Eina_Bool close_on_invalidate; /**< If true, the file descriptor will be closed when the object is invalidated. */
} Efl_Io_Closer_Fd_Data;

/**
 * @internal
 * @brief Constructor for Efl_Io_Closer_Fd.
 *
 * Initializes the file descriptor to -1 (invalid/not set).
 *
 * @param obj The Efl_Io_Closer_Fd object.
 * @param pd The private data for the object.
 * @return The constructed Efl_Object.
 */
static Efl_Object *
_efl_io_closer_fd_efl_object_constructor(Eo *obj, Efl_Io_Closer_Fd_Data *pd)
{
   pd->fd = -1;

   return efl_constructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Sets the file descriptor to be managed.
 * @param o The Efl_Io_Closer_Fd object (unused).
 * @param pd The private data for the object.
 * @param fd The file descriptor to set.
 */
EOLIAN static void
_efl_io_closer_fd_closer_fd_set(Eo *o EINA_UNUSED, Efl_Io_Closer_Fd_Data *pd, int fd)
{
   pd->fd = fd;
}

/**
 * @internal
 * @brief Gets the managed file descriptor.
 * @param o The Efl_Io_Closer_Fd object (unused).
 * @param pd The private data for the object.
 * @return The current file descriptor, or -1 if not set or closed.
 */
EOLIAN static int
_efl_io_closer_fd_closer_fd_get(const Eo *o EINA_UNUSED, Efl_Io_Closer_Fd_Data *pd)
{
   return pd->fd;
}

/**
 * @internal
 * @brief Closes the managed file descriptor.
 *
 * If the file descriptor is valid (>= 0), it attempts to close it.
 * After attempting to close, it sets the internal fd to -1 and
 * emits the "closed" event.
 *
 * @param o The Efl_Io_Closer_Fd object.
 * @param pd The private data for the object (unused, fd is retrieved via getter).
 * @return 0 on success, or an error code (errno) on failure.
 *         Returns EBADF if the fd was already < 0.
 */
EOLIAN static Eina_Error
_efl_io_closer_fd_efl_io_closer_close(Eo *o, Efl_Io_Closer_Fd_Data *pd EINA_UNUSED)
{
   int fd;
   Eina_Error err = 0;

   fd = efl_io_closer_fd_get(o);

   EINA_SAFETY_ON_TRUE_RETURN_VAL(fd < 0, EBADF);

   efl_io_closer_fd_set(o, -1);
   if (close(fd) < 0) err = errno;
   efl_event_callback_call(o, EFL_IO_CLOSER_EVENT_CLOSED, NULL);
   return err;
}

/**
 * @internal
 * @brief Checks if the managed file descriptor is closed.
 *
 * The fd is considered closed if its value is less than 0.
 *
 * @param o The Efl_Io_Closer_Fd object.
 * @param pd The private data for the object (unused, fd is retrieved via getter).
 * @return @c EINA_TRUE if the file descriptor is closed, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_io_closer_fd_efl_io_closer_closed_get(const Eo *o, Efl_Io_Closer_Fd_Data *pd EINA_UNUSED)
{
   return efl_io_closer_fd_get(o) < 0;
}

/**
 * @internal
 * @brief Sets whether the file descriptor should be closed on exec.
 *
 * This function attempts to set the FD_CLOEXEC flag on the managed
 * file descriptor. If the fd is not currently set (is < 0), the flag
 * is stored and applied when fd_set() is called.
 * On Windows, this feature is not supported and the function will log a debug message
 * and return EINA_FALSE, though it will store the desired state.
 *
 * @param o The Efl_Io_Closer_Fd object.
 * @param pd The private data for the object.
 * @param close_on_exec @c EINA_TRUE to close on exec, @c EINA_FALSE otherwise.
 * @return @c EINA_TRUE on success or if fd is not yet set, @c EINA_FALSE on failure to set the flag.
 */
EOLIAN static Eina_Bool
_efl_io_closer_fd_efl_io_closer_close_on_exec_set(Eo *o, Efl_Io_Closer_Fd_Data *pd, Eina_Bool close_on_exec)
{
#ifdef _WIN32
   DBG("close on exec is not supported on windows");
   pd->close_on_exec = close_on_exec;
   return EINA_FALSE;
   (void)o;
#else
   int fd;
   Eina_Bool old = pd->close_on_exec;

   pd->close_on_exec = close_on_exec;

   fd = efl_io_closer_fd_get(o);
   if (fd < 0) return EINA_TRUE; /* postpone until fd_set(), users
                                  * must apply MANUALLY if it's not
                                  * already set!
                                  */

   if (!eina_file_close_on_exec(fd, close_on_exec))
     {
        ERR("eina_file_close_on_exec(%d) failed", fd);
        pd->close_on_exec = old;
        return EINA_FALSE;
     }

   return EINA_TRUE;
#endif
}

/**
 * @internal
 * @brief Gets whether the file descriptor is set to close on exec.
 *
 * If a valid file descriptor is set, this function queries its FD_CLOEXEC flag directly.
 * Otherwise, it returns the stored desired state.
 * On Windows, this returns the stored state as the feature is not supported.
 *
 * @param o The Efl_Io_Closer_Fd object.
 * @param pd The private data for the object.
 * @return @c EINA_TRUE if close on exec is set or intended, @c EINA_FALSE otherwise or on error.
 */
EOLIAN static Eina_Bool
_efl_io_closer_fd_efl_io_closer_close_on_exec_get(const Eo *o, Efl_Io_Closer_Fd_Data *pd)
{
#ifdef _WIN32
   return pd->close_on_exec;
   (void)o;
#else
   int flags, fd;

   fd = efl_io_closer_fd_get(o);
   if (fd < 0) return pd->close_on_exec;

   /* if there is a fd, always query it directly as it may be modified
    * elsewhere by nasty users.
    */
   flags = fcntl(fd, F_GETFD);
   if (flags < 0)
     {
        ERR("fcntl(%d, F_GETFD): %s", fd, strerror(errno));
        return EINA_FALSE;
     }

   pd->close_on_exec = !!(flags & FD_CLOEXEC); /* sync */
   return pd->close_on_exec;
#endif
}

/**
 * @internal
 * @brief Sets whether the file descriptor should be closed when the Efl_Object is invalidated.
 *
 * @param o The Efl_Io_Closer_Fd object (unused).
 * @param pd The private data for the object.
 * @param close_on_invalidate @c EINA_TRUE to close on invalidate, @c EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_io_closer_fd_efl_io_closer_close_on_invalidate_set(Eo *o EINA_UNUSED, Efl_Io_Closer_Fd_Data *pd, Eina_Bool close_on_invalidate)
{
   pd->close_on_invalidate = close_on_invalidate;
}

/**
 * @internal
 * @brief Gets whether the file descriptor is set to close when the Efl_Object is invalidated.
 *
 * @param o The Efl_Io_Closer_Fd object (unused).
 * @param pd The private data for the object.
 * @return @c EINA_TRUE if close on invalidate is set, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_io_closer_fd_efl_io_closer_close_on_invalidate_get(const Eo *o EINA_UNUSED, Efl_Io_Closer_Fd_Data *pd)
{
   return pd->close_on_invalidate;
}

#include "efl_io_closer_fd.eo.c"
