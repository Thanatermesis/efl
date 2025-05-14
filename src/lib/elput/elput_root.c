#include "elput_private.h"
#include <grp.h>
#include <sys/types.h>
#include <pwd.h>

# ifdef major
#  define MAJOR(x) major(x)
# else
#  define MAJOR(x) ((((x) >> 8) & 0xfff) | (((x) >> 32) & ~0xfff))
# endif

/**
 * @brief Checks if the current user is a member of the "input" group.
 *
 * This function is crucial for security, ensuring that only users with
 * appropriate permissions (i.e., part of the "input" group) can
 * access input devices.
 *
 * @return EINA_TRUE if the user is part of the "input" group,
 *         EINA_FALSE otherwise or if an error occurs (e.g., user or group
 *         not found, memory allocation failure).
 */
static Eina_Bool
_user_part_of_input(void)
{
   uid_t user;
   struct passwd *user_pw;
   gid_t *gids = NULL;
   int number_of_groups = 0;
   struct group *input_group;

   user = getuid();
   user_pw = getpwuid(user);
   input_group = getgrnam("input");

   EINA_SAFETY_ON_NULL_RETURN_VAL(user_pw, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(input_group, EINA_FALSE);

   if (getgrouplist(user_pw->pw_name, getgid(), NULL, &number_of_groups) != -1)
     {
        ERR("Failed to enumerate groups of user");
        return EINA_FALSE;
     }
   number_of_groups ++;
   gids = alloca((number_of_groups) * sizeof(gid_t));
   if (getgrouplist(user_pw->pw_name, getgid(), gids, &number_of_groups) == -1)
     {
        ERR("Failed to get groups of user");
        return EINA_FALSE;
     }

   for (int i = 0; i < number_of_groups; ++i)
     {
        if (gids[i] == input_group->gr_gid)
          return EINA_TRUE;
     }
   return EINA_FALSE;
}

/**
 * @brief Connects and initializes the Elput manager for root operations.
 *
 * This function allocates and sets up an Elput_Manager structure. It also
 * verifies that the current user is part of the "input" group before
 * proceeding.
 *
 * @param[out] manager Pointer to a location where the newly created
 *                     Elput_Manager pointer will be stored.
 * @param[in] seat The seat identifier string (e.g., "seat0"). This is
 *                 stored in the Elput_Manager.
 * @param[in] tty The TTY number. This parameter is currently unused in this
 *                function.
 * @return EINA_TRUE on successful connection and initialization,
 *         EINA_FALSE on failure (e.g., memory allocation error, user not
 *         in "input" group).
 */
static Eina_Bool
_root_connect(Elput_Manager **manager EINA_UNUSED, const char *seat EINA_UNUSED, unsigned int tty EINA_UNUSED)
{
   Elput_Manager *em;

   em = calloc(1, sizeof(Elput_Manager));
   if (!em) return EINA_FALSE;

   em->interface = &_root_interface;
   em->seat = eina_stringshare_add(seat);

   if (!_user_part_of_input())
     {
        free(em);
        return EINA_FALSE;
     }
   *manager = em;
   return EINA_TRUE;
}

/**
 * @brief Disconnects the Elput manager for root operations.
 *
 * Currently, this function is a no-op as there are no specific resources
 * allocated by _root_connect that need explicit freeing here beyond what
 * higher-level management might handle.
 *
 * @param[in] em Pointer to the Elput_Manager to disconnect. This parameter
 *               is currently unused.
 */
static void
_root_disconnect(Elput_Manager *em EINA_UNUSED)
{
   //Nothing to do here, there is no data to free
}

/**
 * @brief Opens a device file with specified flags when running as root.
 *
 * This function performs necessary checks (e.g., if the path is a character
 * device) before opening the file. It also tracks the number of opened DRM
 * devices.
 *
 * @param[in,out] em Pointer to the Elput_Manager. Used to increment
 *                   `drm_opens` if a DRM device is opened.
 * @param[in] path The file system path to the device to open (e.g.,
 *                 "/dev/input/event0", "/dev/dri/card0").
 * @param[in] flags The flags to use when opening the file (e.g., O_RDWR,
 *                  O_NONBLOCK).
 * @return The file descriptor on success, or -1 on error (e.g., path not
 *         found, not a character device, open failed).
 */
static int
_root_open(Elput_Manager *em EINA_UNUSED, const char *path, int flags)
{
   struct stat st;
   int ret, fd = -1;
   int fl;

   ret = stat(path, &st);
   if (ret < 0) return -1;

   if (!S_ISCHR(st.st_mode)) return -1;

   fd = open(path, flags);
   if (fd < 0) return fd;

   if (MAJOR(st.st_rdev) == 226) //DRM_MAJOR
     em->drm_opens++;

   fl = fcntl(fd, F_GETFL);
   if (fl < 0) goto err;

   if (flags & O_NONBLOCK)
     fl |= O_NONBLOCK;

   ret = fcntl(fd, F_SETFL, fl);
   if (ret < 0) goto err;

   return fd;
err:
   close(fd);
   return -1;
}

/**
 * @brief Opens a device file asynchronously and sends the fd via a pipe.
 *
 * This function calls _root_open to open the device and then writes the
 * resulting file descriptor to a pipe specified in `em->input.pipe`.
 * This is typically used when the open operation might block or needs to be
 * handled in a non-blocking fashion by another part of the system.
 *
 * @param[in] em Pointer to the Elput_Manager. `em->input.pipe` is used to
 *               send the opened file descriptor.
 * @param[in] path The file system path to the device to open.
 * @param[in] flags The flags to use when opening the file.
 */
static void
_root_open_async(Elput_Manager *em, const char *path, int flags)
{
   int fd, ret;

   fd = _root_open(em, path, flags);
   while (1)
     {
        ret = write(em->input.pipe, &fd, sizeof(int));
        if (ret < 0)
          {
             if ((errno == EAGAIN) || (errno == EINTR))
               continue;
             WRN("Failed to write to input pipe");
          }
        break;
     }
   close(em->input.pipe);
   em->input.pipe = -1;
}

/**
 * @brief Closes a file descriptor.
 *
 * @param[in] em Pointer to the Elput_Manager. This parameter is currently
 *               unused.
 * @param[in] fd The file descriptor to close.
 */
static void
_root_close(Elput_Manager *em EINA_UNUSED, int fd)
{
   close(fd);
}

/**
 * @brief Sets the active virtual terminal (VT).
 *
 * This function is intended for operations that require changing the VT.
 * Currently, it's a no-op and always returns success.
 *
 * @param[in] em Pointer to the Elput_Manager. This parameter is currently
 *               unused.
 * @param[in] vt The virtual terminal number to switch to. This parameter is
 *               currently unused.
 * @return EINA_TRUE, as the operation is currently a no-op.
 */
static Eina_Bool
_root_vt_set(Elput_Manager *em EINA_UNUSED, int vt EINA_UNUSED)
{
   //Nothing to do here
   return EINA_TRUE;
}

/**
 * @brief Interface for Elput operations when running with root privileges.
 *
 * This structure maps generic Elput operations to their root-specific
 * implementations.
 */
Elput_Interface _root_interface =
{
   _root_connect,    /**< Function to connect and initialize the manager. */
   _root_disconnect, /**< Function to disconnect the manager. */
   _root_open,       /**< Function to open a device file. */
   _root_open_async, /**< Function to open a device file asynchronously. */
   _root_close,      /**< Function to close a file descriptor. */
   _root_vt_set,     /**< Function to set the virtual terminal. */
};
