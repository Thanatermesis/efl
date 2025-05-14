#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif

#include <Eo.h>

#include "ecore_audio_private.h"
#include <sndfile.h>

/* Virtual IO wrapper functions */

/**
 * @brief Wrapper for the get_filelen VIO callback.
 *
 * This function is called by libsndfile to get the total length of the virtual file.
 * It retrieves the Ecore_Audio_Object associated with the Eo object and calls
 * the user-provided get_length function.
 *
 * @param data Pointer to the Eo object, passed by libsndfile.
 * @return The length of the file in bytes, or -1 on error (e.g., VIO not set up).
 */
static sf_count_t _wrap_get_filelen(void *data)
{
  Eo *eo_obj = data;
  Ecore_Audio_Object *ea_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);

  if (!ea_obj->vio->vio)
    goto error;

  if (ea_obj->vio->vio->get_length)
    return ea_obj->vio->vio->get_length(ea_obj->vio->data, eo_obj);

error:
  return -1;
}

/**
 * @brief Wrapper for the seek VIO callback.
 *
 * This function is called by libsndfile to change the current read/write position
 * in the virtual file. It retrieves the Ecore_Audio_Object and calls the
 * user-provided seek function.
 *
 * @param offset The offset to seek to, relative to whence.
 * @param whence The position from where offset is added. It can be SEEK_SET,
 *               SEEK_CUR, or SEEK_END.
 * @param data Pointer to the Eo object, passed by libsndfile.
 * @return The new offset from the beginning of the file, or -1 on error.
 */
static sf_count_t _wrap_seek(sf_count_t offset, int whence, void *data)
{
  Eo *eo_obj = data;
  Ecore_Audio_Object *ea_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);

  if (!ea_obj->vio->vio)
    goto error;

  if (ea_obj->vio->vio->seek)
    return ea_obj->vio->vio->seek(ea_obj->vio->data, eo_obj, offset, whence);

error:
  return -1;
}

/**
 * @brief Wrapper for the read VIO callback.
 *
 * This function is called by libsndfile to read data from the virtual file.
 * It retrieves the Ecore_Audio_Object and calls the user-provided read function.
 *
 * @param buffer Pointer to the buffer where the read data will be stored.
 * @param count The number of bytes to read.
 * @param data Pointer to the Eo object, passed by libsndfile.
 * @return The number of bytes actually read. This may be less than count if
 *         the end of the file is reached or an error occurs. Returns 0 on error
 *         if VIO is not set up.
 */
static sf_count_t _wrap_read(void *buffer, sf_count_t count, void *data)
{
  Eo *eo_obj = data;
  Ecore_Audio_Object *ea_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);

  if (!ea_obj->vio->vio)
    goto error;

  if (ea_obj->vio->vio->read)
    return ea_obj->vio->vio->read(ea_obj->vio->data, eo_obj, buffer, count);

error:
  return 0;
}

/**
 * @brief Wrapper for the write VIO callback.
 *
 * This function is called by libsndfile to write data to the virtual file.
 * It retrieves the Ecore_Audio_Object and calls the user-provided write function.
 *
 * @param buffer Pointer to the buffer containing the data to be written.
 * @param count The number of bytes to write.
 * @param data Pointer to the Eo object, passed by libsndfile.
 * @return The number of bytes actually written. This may be less than count if
 *         an error occurs. Returns 0 on error if VIO is not set up.
 */
static sf_count_t _wrap_write(const void *buffer, sf_count_t count, void *data)
{
  Eo *eo_obj = data;
  Ecore_Audio_Object *ea_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);

  if (!ea_obj->vio->vio)
    goto error;

  if (ea_obj->vio->vio->write)
    return ea_obj->vio->vio->write(ea_obj->vio->data, eo_obj, buffer, count);

error:
  return 0;
}

/**
 * @brief Wrapper for the tell VIO callback.
 *
 * This function is called by libsndfile to get the current read/write position
 * in the virtual file. It retrieves the Ecore_Audio_Object and calls the
 * user-provided tell function.
 *
 * @param data Pointer to the Eo object, passed by libsndfile.
 * @return The current file offset in bytes from the beginning of the file,
 *         or -1 on error.
 */
static sf_count_t _wrap_tell(void *data)
{
  Eo *eo_obj = data;
  Ecore_Audio_Object *ea_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);

  if (!ea_obj->vio->vio)
    goto error;

  if (ea_obj->vio->vio->tell)
    return ea_obj->vio->vio->tell(ea_obj->vio->data, eo_obj);

error:
  return -1;
}

/**
 * @brief Structure defining the virtual I/O functions for libsndfile.
 *
 * This structure maps the standard VIO operations (get_filelen, seek, read,
 * write, tell) to their respective wrapper functions defined in this file.
 * An instance of this structure is passed to libsndfile when opening a sound
 * file with virtual I/O.
 */
SF_VIRTUAL_IO vio_wrapper = {
    .get_filelen = _wrap_get_filelen, /**< Function to get file length. */
    .seek = _wrap_seek, /**< Function to seek within the file. */
    .read = _wrap_read, /**< Function to read from the file. */
    .write = _wrap_write, /**< Function to write to the file. */
    .tell = _wrap_tell, /**< Function to get the current file position. */
};

/* End virtual IO wrapper functions */

