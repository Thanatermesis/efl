#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif

#include <Eo.h>
#include "ecore_audio_private.h"
#include <sndfile.h>

#define MY_CLASS ECORE_AUDIO_IN_SNDFILE_CLASS
#define MY_CLASS_NAME "Ecore_Audio_In_Sndfile"

extern SF_VIRTUAL_IO vio_wrapper;

/**
 * @brief Private data for the Ecore_Audio_In_Sndfile object.
 *
 * This structure holds the sndfile library specific data, including the
 * file handle and information about the audio file.
 */
struct _Ecore_Audio_In_Sndfile_Data
{
  SNDFILE *handle; /**< The sndfile handle for the opened audio file. */
  SF_INFO sfinfo;  /**< Structure containing information about the sound file (samplerate, channels, format, etc.). */
};

typedef struct _Ecore_Audio_In_Sndfile_Data Ecore_Audio_In_Sndfile_Data;

/**
 * @brief Reads audio data from the sndfile input.
 *
 * This function is the internal implementation for reading audio data.
 * It uses sf_read_float to read data as float samples.
 *
 * @param[in] eo_obj The Efl_Object instance.
 * @param[in,out] obj The private data of the Ecore_Audio_In_Sndfile object.
 * @param[out] data Buffer to store the read audio data.
 * @param[in] len The maximum number of bytes to read. This is expected to be a multiple of 4 (sizeof(float)).
 *
 * @return The number of bytes read, or 0 on error or if sndfile is not loaded.
 *         The return value is also a multiple of 4.
 */
EOLIAN static ssize_t
_ecore_audio_in_sndfile_ecore_audio_in_read_internal(Eo *eo_obj EINA_UNUSED, Ecore_Audio_In_Sndfile_Data *obj, void *data, size_t len)
{
  if (!ESF_LOAD()) return 0;
  return ESF_CALL(sf_read_float)(obj->handle, data, len/4)*4;
}

/**
 * @brief Seeks to a position in the audio stream.
 *
 * This function uses sf_seek to change the current read/write position in the
 * audio file. The offset is specified in seconds.
 *
 * @param[in] eo_obj The Efl_Object instance.
 * @param[in,out] obj The private data of the Ecore_Audio_In_Sndfile object.
 * @param[in] offs The offset in seconds from the position specified by 'mode'.
 * @param[in] mode The seeking mode, typically one of SEEK_SET, SEEK_CUR, SEEK_END
 *                 as defined in sndfile.h (e.g., SF_SEEK_SET, SF_SEEK_CUR, SF_SEEK_END).
 *
 * @return The new position in seconds from the beginning of the file, or 0.0 on error
 *         or if sndfile is not loaded.
 */
EOLIAN static double
_ecore_audio_in_sndfile_ecore_audio_in_seek(Eo *eo_obj EINA_UNUSED, Ecore_Audio_In_Sndfile_Data *obj, double offs, int mode)
{
  sf_count_t count, pos;

  if (!ESF_LOAD()) return 0.0;
  count = offs * obj->sfinfo.samplerate;
  pos = ESF_CALL(sf_seek)(obj->handle, count, mode);

  return (double)pos / obj->sfinfo.samplerate;
}

/**
 * @brief Sets the audio source (file path) for the input.
 *
 * This function opens the specified audio file using sndfile. If a file is
 * already open, it will be closed first. It updates various properties of the
 * input object like length, samplerate, channels, and format based on the
 * opened file.
 *
 * @param[in] eo_obj The Efl_Object instance.
 * @param[in,out] obj The private data of the Ecore_Audio_In_Sndfile object.
 * @param[in] source The file path of the audio source to open.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., file not found,
 *         unsupported format, sndfile not loaded).
 */
EOLIAN static Eina_Bool
_ecore_audio_in_sndfile_ecore_audio_source_set(Eo *eo_obj, Ecore_Audio_In_Sndfile_Data *obj, const char *source)
{
  Ecore_Audio_Object *ea_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);
  Ecore_Audio_Input *in_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_IN_CLASS);

  if (!ESF_LOAD()) return EINA_FALSE;
  if (obj->handle) {
    ESF_CALL(sf_close)(obj->handle);
    obj->handle = NULL;
  }

  eina_stringshare_replace(&ea_obj->source, source);

  if (!ea_obj->source)
    return EINA_FALSE;

  obj->handle = ESF_CALL(sf_open)(ea_obj->source, SFM_READ, &obj->sfinfo);

  if (!obj->handle) {
    eina_stringshare_del(ea_obj->source);
    ea_obj->source = NULL;
    return EINA_FALSE;
  }

  in_obj->seekable = EINA_TRUE;
  in_obj->length = (double)obj->sfinfo.frames / obj->sfinfo.samplerate;

  in_obj->samplerate =  obj->sfinfo.samplerate;
  in_obj->channels =  obj->sfinfo.channels;

  if (obj->sfinfo.format& SF_FORMAT_WAV)
    ea_obj->format = ECORE_AUDIO_FORMAT_WAV;
  else if (obj->sfinfo.format& SF_FORMAT_OGG)
    ea_obj->format = ECORE_AUDIO_FORMAT_OGG;
  else if (obj->sfinfo.format& SF_FORMAT_FLAC)
    ea_obj->format = ECORE_AUDIO_FORMAT_FLAC;
  else
    ea_obj->format = ECORE_AUDIO_FORMAT_AUTO;

   return EINA_TRUE;
}

/**
 * @brief Gets the current audio source (file path).
 *
 * @param[in] eo_obj The Efl_Object instance.
 * @param[in] _pd The private data of the Ecore_Audio_In_Sndfile object (unused).
 *
 * @return The current audio source file path as a stringshare, or @c NULL if not set.
 */
EOLIAN static const char*
_ecore_audio_in_sndfile_ecore_audio_source_get(const Eo *eo_obj, Ecore_Audio_In_Sndfile_Data *_pd EINA_UNUSED)
{
  Ecore_Audio_Object *obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);
  return obj->source;
}

/**
 * @brief Sets the desired audio format for the input.
 *
 * This function allows specifying the audio format (e.g., WAV, OGG, FLAC)
 * to be used by sndfile when opening a source. This setting is typically
 * used before a source is set, or when using virtual I/O where format
 * detection might be ambiguous. It does not change the format of an
 * already opened file.
 *
 * @param[in] eo_obj The Efl_Object instance.
 * @param[in,out] obj The private data of the Ecore_Audio_In_Sndfile object.
 * @param[in] format The desired Ecore_Audio_Format.
 *
 * @return @c EINA_TRUE if the format was successfully set (or is supported),
 *         @c EINA_FALSE if the input is already open or the format is not supported.
 */
EOLIAN static Eina_Bool
_ecore_audio_in_sndfile_ecore_audio_format_set(Eo *eo_obj, Ecore_Audio_In_Sndfile_Data *obj, Ecore_Audio_Format format)
{
  Ecore_Audio_Object *ea_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);

  if (ea_obj->source) {
      ERR("Input is already open - cannot change format");
      return EINA_FALSE;
  }

  switch (format) {
    case ECORE_AUDIO_FORMAT_AUTO:
      obj->sfinfo.format = 0;
      break;
    case ECORE_AUDIO_FORMAT_WAV:
      obj->sfinfo.format = SF_FORMAT_WAV|SF_FORMAT_PCM_16;
      break;
    case ECORE_AUDIO_FORMAT_OGG:
      obj->sfinfo.format = SF_FORMAT_OGG|SF_FORMAT_VORBIS;
      break;
    case ECORE_AUDIO_FORMAT_FLAC:
      obj->sfinfo.format = SF_FORMAT_FLAC;
      break;
    default:
      ERR("Format not supported!");
      return EINA_FALSE;
  }
  ea_obj->format = format;

  return EINA_TRUE;
}

/**
 * @brief Gets the current audio format of the input.
 *
 * This format is either auto-detected when a source is set, or it's the
 * format explicitly set via ecore_audio_format_set().
 *
 * @param[in] eo_obj The Efl_Object instance.
 * @param[in] _pd The private data of the Ecore_Audio_In_Sndfile object (unused).
 *
 * @return The current Ecore_Audio_Format.
 */
EOLIAN static Ecore_Audio_Format
_ecore_audio_in_sndfile_ecore_audio_format_get(const Eo *eo_obj, Ecore_Audio_In_Sndfile_Data *_pd EINA_UNUSED)
{
  Ecore_Audio_Object *obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);
  return obj->format;;
}

/**
 * @brief Sets up virtual I/O for the sndfile input.
 *
 * This allows sndfile to read audio data from a custom source (e.g., memory buffer)
 * instead of a regular file. It uses the provided Ecore_Audio_Vio callbacks.
 * If a file handle is already open, it's closed.
 * After successfully opening via VIO, it updates audio properties like length,
 * samplerate, channels, and format.
 *
 * @param[in] eo_obj The Efl_Object instance.
 * @param[in,out] obj The private data of the Ecore_Audio_In_Sndfile object.
 * @param[in] vio Pointer to an Ecore_Audio_Vio structure with callback functions.
 *                If NULL, VIO is unset.
 * @param[in] data User data to be passed to the VIO callbacks.
 * @param[in] free_func Function to free the user data when the object is destroyed or VIO is unset.
 */
EOLIAN static void
_ecore_audio_in_sndfile_ecore_audio_vio_set(Eo *eo_obj, Ecore_Audio_In_Sndfile_Data *obj, Ecore_Audio_Vio *vio, void *data, efl_key_data_free_func free_func)
{
  Ecore_Audio_Object *ea_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);
  Ecore_Audio_Input *in_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_IN_CLASS);

  if (!ESF_LOAD()) return;
  if (obj->handle) {
    ESF_CALL(sf_close)(obj->handle);
    obj->handle = NULL;
  }

  if (vio)
    eina_stringshare_replace(&ea_obj->source, "VIO");
  else
    eina_stringshare_replace(&ea_obj->source, NULL);

  in_obj->seekable = EINA_FALSE;
  ecore_audio_obj_vio_set(efl_super(eo_obj, MY_CLASS), vio, data, free_func);

  if (!vio)
    return;
  in_obj->seekable = (vio->seek != NULL);

  obj->handle = ESF_CALL(sf_open_virtual)(&vio_wrapper, SFM_READ, &obj->sfinfo, eo_obj);

  if (!obj->handle) {
    if (ea_obj->vio->free_func)
      ea_obj->vio->free_func(ea_obj->vio->data);
    free(ea_obj->vio);
    ea_obj->vio = NULL;
    eina_stringshare_del(ea_obj->source);
    ea_obj->source = NULL;
    return;
  }

  in_obj->seekable = EINA_TRUE;
  in_obj->length = (double)obj->sfinfo.frames / obj->sfinfo.samplerate;

  in_obj->samplerate =  obj->sfinfo.samplerate;
  in_obj->channels =  obj->sfinfo.channels;

  if (obj->sfinfo.format& SF_FORMAT_WAV)
    ea_obj->format = ECORE_AUDIO_FORMAT_WAV;
  else if (obj->sfinfo.format& SF_FORMAT_OGG)
    ea_obj->format = ECORE_AUDIO_FORMAT_OGG;
  else if (obj->sfinfo.format& SF_FORMAT_FLAC)
    ea_obj->format = ECORE_AUDIO_FORMAT_FLAC;
  else
    ea_obj->format = ECORE_AUDIO_FORMAT_AUTO;
}

/**
 * @brief Destructor for the Ecore_Audio_In_Sndfile object.
 *
 * This function is called when the Efl_Object is being destroyed.
 * It ensures that the sndfile handle is closed if it's open, releasing
 * any associated resources.
 *
 * @param[in] eo_obj The Efl_Object instance being destroyed.
 * @param[in,out] obj The private data of the Ecore_Audio_In_Sndfile object.
 */
EOLIAN static void
_ecore_audio_in_sndfile_efl_object_destructor(Eo *eo_obj, Ecore_Audio_In_Sndfile_Data *obj)
{
  if (obj->handle)
    ESF_CALL(sf_close)(obj->handle);

  efl_destructor(efl_super(eo_obj, MY_CLASS));
}

#include "ecore_audio_in_sndfile.eo.c"
