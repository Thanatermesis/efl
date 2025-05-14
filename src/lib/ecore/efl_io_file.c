#define EFL_IO_READER_PROTECTED 1
#define EFL_IO_WRITER_PROTECTED 1
#define EFL_IO_READER_FD_PROTECTED 1
#define EFL_IO_WRITER_FD_PROTECTED 1
#define EFL_IO_CLOSER_FD_PROTECTED 1
#define EFL_IO_SIZER_FD_PROTECTED 1
#define EFL_IO_POSITIONER_FD_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef O_CLOEXEC
/* If a platform doesn't define O_CLOEXEC, then use 0 as we'll "| 0"
 * and "& ~0", which have no effect.
 *
 * This should be the case on _WIN32.
 */
#define O_CLOEXEC (0)
#endif

#define MY_CLASS EFL_IO_FILE_CLASS

/**
 * @brief Private data structure for Efl_Io_File.
 */
typedef struct _Efl_Io_File_Data
{
   uint32_t flags;          //!< File open flags (e.g., O_RDONLY, O_WRONLY, O_RDWR, O_CLOEXEC).
   uint32_t mode;           //!< File creation mode (e.g., S_IRUSR, S_IWUSR), used if O_CREAT is in flags.
   uint64_t last_position;  //!< Stores the last known file position to detect changes.
   // TODO: monitor reader.can_read,changed/writer.can_write,changed events in order to dynamically connect to Loop_Fd events.
} Efl_Io_File_Data;

/**
 * @brief Updates the read/write/eos state of the Efl_Io_File object.
 *
 * This function is called after operations that might change the file state,
 * such as read, write, seek, or resize. It checks the current position and size
 * to determine if the file can be read from, written to, or if EOF has been reached.
 * It also emits the "position,changed" event if the position has changed.
 *
 * @param o The Efl_Io_File object.
 * @param pd The private data of the Efl_Io_File object.
 */
static void
_efl_io_file_state_update(Eo *o, Efl_Io_File_Data *pd)
{
   uint64_t pos = efl_io_positioner_position_get(o);
   uint64_t size = efl_io_sizer_size_get(o);
   uint32_t flags = pd->flags & O_ACCMODE;

   if ((flags == O_RDWR) || (flags == O_RDONLY))
     {
        efl_io_reader_can_read_set(o, pos < size);
        efl_io_reader_eos_set(o, pos >= size);
     }

   if ((flags == O_RDWR) || (flags == O_WRONLY))
     efl_io_writer_can_write_set(o, EINA_TRUE);

   if (pd->last_position != pos)
     {
        pd->last_position = pos;
        efl_event_callback_call(o, EFL_IO_POSITIONER_EVENT_POSITION_CHANGED, NULL);
     }
}

/**
 * @brief Sets the file descriptor for the Efl_Io_File object and updates related properties.
 *
 * This function not only sets the file descriptor for the underlying Efl_Loop_Fd
 * but also propagates this fd to the Efl_Io_Positioner, Efl_Io_Sizer,
 * Efl_Io_Reader, Efl_Io_Writer, and Efl_Io_Closer interfaces.
 * If a valid fd (>= 0) is provided, it also triggers an update of the file state.
 *
 * @param o The Efl_Io_File object.
 * @param pd The private data of the Efl_Io_File object.
 * @param fd The file descriptor to set.
 */
EOLIAN static void
_efl_io_file_efl_loop_fd_fd_file_set(Eo *o, Efl_Io_File_Data *pd, int fd)
{
   efl_loop_fd_file_set(efl_super(o, MY_CLASS), fd);
   efl_io_positioner_fd_set(o, fd);
   efl_io_sizer_fd_set(o, fd);
   efl_io_reader_fd_set(o, fd);
   efl_io_writer_fd_set(o, fd);
   efl_io_closer_fd_set(o, fd);
   if (fd >= 0) _efl_io_file_state_update(o, pd);
}

/**
 * @brief Sets the file open flags for the Efl_Io_File object.
 *
 * These flags (e.g., O_RDONLY, O_WRONLY, O_APPEND, O_CLOEXEC) are used when
 * the file is opened during finalization if no file descriptor was set beforehand.
 * This function also synchronizes the O_CLOEXEC flag with the
 * Efl_Io_Closer.close_on_exec property.
 *
 * @param o The Efl_Io_File object.
 * @param pd The private data of the Efl_Io_File object.
 * @param flags The file open flags to set.
 */
EOLIAN static void
_efl_io_file_flags_set(Eo *o, Efl_Io_File_Data *pd, uint32_t flags)
{
   Eina_Bool close_on_exec;

   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(o));

   pd->flags = flags;

   close_on_exec = !!(flags & O_CLOEXEC);
   if (close_on_exec != efl_io_closer_close_on_exec_get(o))
     efl_io_closer_close_on_exec_set(o, close_on_exec);
}

/**
 * @brief Gets the file open flags for the Efl_Io_File object.
 *
 * @param o The Efl_Io_File object (unused).
 * @param pd The private data of the Efl_Io_File object.
 * @return The currently set file open flags.
 * @note TODO: query from fd? This currently returns the stored flags,
 *       which might not reflect the actual flags if the fd was set externally
 *       and then flags were changed.
 */
EOLIAN static uint32_t
_efl_io_file_flags_get(const Eo *o EINA_UNUSED, Efl_Io_File_Data *pd)
{
   return pd->flags; // TODO: query from fd?
}

/**
 * @brief Sets the file creation mode for the Efl_Io_File object.
 *
 * This mode (e.g., S_IRUSR | S_IWUSR) is used when the file is created
 * (i.e., if O_CREAT is part of the flags) during finalization.
 *
 * @param o The Efl_Io_File object.
 * @param pd The private data of the Efl_Io_File object.
 * @param mode The file creation mode to set.
 */
EOLIAN static void
_efl_io_file_mode_set(Eo *o, Efl_Io_File_Data *pd, uint32_t mode)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(o));
   pd->mode = mode;
}

/**
 * @brief Gets the file creation mode for the Efl_Io_File object.
 *
 * @param o The Efl_Io_File object (unused).
 * @param pd The private data of the Efl_Io_File object.
 * @return The currently set file creation mode.
 */
EOLIAN static uint32_t
_efl_io_file_mode_get(const Eo *o EINA_UNUSED, Efl_Io_File_Data *pd)
{
   return pd->mode;
}

/**
 * @brief Constructor for the Efl_Io_File object.
 *
 * Initializes the private data with default flags (O_RDONLY | O_CLOEXEC)
 * and sets default properties for the Efl_Io_Closer, Efl_Io_Positioner,
 * Efl_Io_Sizer, Efl_Io_Reader, and Efl_Io_Writer interfaces (typically
 * setting their associated fd to -1 initially).
 *
 * @param o The Efl_Io_File object being constructed.
 * @param pd The private data of the Efl_Io_File object.
 * @return The constructed Efl_Io_File object.
 */
EOLIAN static Eo *
_efl_io_file_efl_object_constructor(Eo *o, Efl_Io_File_Data *pd)
{
   pd->flags = O_RDONLY | O_CLOEXEC;

   o = efl_constructor(efl_super(o, MY_CLASS));

   efl_io_closer_close_on_exec_set(o, EINA_TRUE);
   efl_io_closer_close_on_invalidate_set(o, EINA_TRUE);
   efl_io_positioner_fd_set(o, -1);
   efl_io_sizer_fd_set(o, -1);
   efl_io_reader_fd_set(o, -1);
   efl_io_writer_fd_set(o, -1);
   efl_io_closer_fd_set(o, -1);

   return o;
}

/**
 * @brief Destructor for the Efl_Io_File object.
 *
 * If the `close_on_invalidate` property is set and the file has not been
 * explicitly closed, this function will close the file. It ensures that
 * events are frozen during the close operation to prevent unexpected callbacks.
 *
 * @param o The Efl_Io_File object being destructed.
 * @param pd The private data of the Efl_Io_File object (unused).
 */
EOLIAN static void
_efl_io_file_efl_object_destructor(Eo *o, Efl_Io_File_Data *pd EINA_UNUSED)
{
   if (efl_io_closer_close_on_invalidate_get(o) &&
       (!efl_io_closer_closed_get(o)))
     {
        efl_event_freeze(o);
        efl_io_closer_close(o);
        efl_event_thaw(o);
     }

   efl_destructor(efl_super(o, MY_CLASS));
}

/**
 * @brief Finalizes the Efl_Io_File object.
 *
 * If a file descriptor has not already been set (fd < 0), this function
 * attempts to open the file specified by the `efl_file_path_get()` property
 * using the flags and mode stored in the private data.
 * If opening the file fails, an error is logged, and NULL is returned.
 * Otherwise, the obtained file descriptor is set using `efl_loop_fd_file_set()`.
 *
 * @param o The Efl_Io_File object being finalized.
 * @param pd The private data of the Efl_Io_File object.
 * @return The finalized Efl_Io_File object, or NULL on failure to open the file.
 */
EOLIAN static Efl_Object *
_efl_io_file_efl_object_finalize(Eo *o, Efl_Io_File_Data *pd)
{
   int fd = efl_loop_fd_file_get(o);
   if (fd < 0)
     {
        const char *path = efl_file_get(o);
        EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);

        if (pd->mode)
          fd = open(path, pd->flags, pd->mode);
        else
          fd = open(path, pd->flags);

        if (fd < 0)
          {
             eina_error_set(errno);
             ERR("Could not open file '%s': %s", path, strerror(errno));
             return NULL;
          }

        efl_loop_fd_file_set(o, fd);
     }

   return efl_finalize(efl_super(o, MY_CLASS));
}

/**
 * @brief Reads data from the file.
 *
 * This function calls the parent class's read implementation and then
 * updates the file state (can_read, eos, position).
 *
 * @param o The Efl_Io_File object.
 * @param pd The private data of the Efl_Io_File object.
 * @param rw_slice The slice to read data into. Upon success, rw_slice.len
 *                 will be updated to the number of bytes read.
 * @return 0 on success, or an Eina_Error code on failure.
 */
EOLIAN static Eina_Error
_efl_io_file_efl_io_reader_read(Eo *o, Efl_Io_File_Data *pd, Eina_Rw_Slice *rw_slice)
{
   Eina_Error err = efl_io_reader_read(efl_super(o, MY_CLASS), rw_slice);
   if (err) return err;
   _efl_io_file_state_update(o, pd);
   return 0;
}

/**
 * @brief Writes data to the file.
 *
 * This function calls the parent class's write implementation and then
 * updates the file state (can_read, eos, position).
 *
 * @param o The Efl_Io_File object.
 * @param pd The private data of the Efl_Io_File object.
 * @param slice The slice of data to write.
 * @param remaining Optional slice that, if provided, will be filled with
 *                  any data that could not be written.
 * @return 0 on success, or an Eina_Error code on failure.
 */
EOLIAN static Eina_Error
_efl_io_file_efl_io_writer_write(Eo *o, Efl_Io_File_Data *pd, Eina_Slice *slice, Eina_Slice *remaining)
{
   Eina_Error err = efl_io_writer_write(efl_super(o, MY_CLASS), slice, remaining);
   if (err) return err;
   _efl_io_file_state_update(o, pd);
   return 0;
}

/**
 * @brief Closes the file.
 *
 * This function updates the reader and writer states to indicate that
 * reading and writing are no longer possible. It then calls the parent
 * class's close implementation and resets the internal file descriptor
 * via `efl_loop_fd_file_set(o, -1)`.
 *
 * @param o The Efl_Io_File object.
 * @param pd The private data of the Efl_Io_File object (unused).
 * @return 0 on success, or an Eina_Error code on failure.
 */
EOLIAN static Eina_Error
_efl_io_file_efl_io_closer_close(Eo *o, Efl_Io_File_Data *pd EINA_UNUSED)
{
   Eina_Error ret;
   efl_io_reader_can_read_set(o, EINA_FALSE);
   efl_io_reader_eos_set(o, EINA_TRUE);
   efl_io_writer_can_write_set(o, EINA_FALSE);

   ret = efl_io_closer_close(efl_super(o, MY_CLASS));

   efl_loop_fd_file_set(o, -1);

   return ret;
}

/**
 * @brief Resizes the file.
 *
 * This function calls the parent class's resize implementation and then
 * updates the file state (can_read, eos, position).
 *
 * @param o The Efl_Io_File object.
 * @param pd The private data of the Efl_Io_File object.
 * @param size The new desired size of the file.
 * @return 0 on success, or an Eina_Error code on failure.
 */
EOLIAN static Eina_Error
_efl_io_file_efl_io_sizer_resize(Eo *o, Efl_Io_File_Data *pd, uint64_t size)
{
   Eina_Error err = efl_io_sizer_resize(efl_super(o, MY_CLASS), size);
   if (err) return err;
   _efl_io_file_state_update(o, pd);
   return 0;
}

/**
 * @brief Seeks to a new position in the file.
 *
 * This function calls the parent class's seek implementation and then
 * updates the file state (can_read, eos, position).
 *
 * @param o The Efl_Io_File object.
 * @param pd The private data of the Efl_Io_File object.
 * @param offset The offset to seek to.
 * @param whence The reference point for the seek (EFL_IO_POSITIONER_WHENCE_START,
 *               EFL_IO_POSITIONER_WHENCE_CURRENT, or EFL_IO_POSITIONER_WHENCE_END).
 * @return 0 on success, or an Eina_Error code on failure.
 */
EOLIAN static Eina_Error
_efl_io_file_efl_io_positioner_seek(Eo *o, Efl_Io_File_Data *pd, int64_t offset, Efl_Io_Positioner_Whence whence)
{
   Eina_Error err = efl_io_positioner_seek(efl_super(o, MY_CLASS), offset, whence);
   if (err) return err;
   _efl_io_file_state_update(o, pd);
   return 0;
}

/**
 * @brief Sets whether the file should be closed when exec() is called.
 *
 * This function updates the internal `O_CLOEXEC` flag in `pd->flags`
 * based on the `close_on_exec` parameter and then calls the parent
 * class's `efl_io_closer_close_on_exec_set` implementation.
 *
 * @param o The Efl_Io_File object.
 * @param pd The private data of the Efl_Io_File object.
 * @param close_on_exec If EINA_TRUE, the file will be closed on exec.
 *                      If EINA_FALSE, it will remain open.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_io_file_efl_io_closer_close_on_exec_set(Eo *o, Efl_Io_File_Data *pd, Eina_Bool close_on_exec)
{
   if (close_on_exec)
     pd->flags |= O_CLOEXEC;
   else
     pd->flags &= (~O_CLOEXEC);

   return efl_io_closer_close_on_exec_set(efl_super(o, MY_CLASS), close_on_exec);
}

#include "efl_io_file.eo.c"
