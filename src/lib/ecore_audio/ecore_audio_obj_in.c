#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif
#include <ctype.h>
#include <errno.h>

#include "ecore_audio_private.h"

#define MY_CLASS ECORE_AUDIO_IN_CLASS
#define MY_CLASS_NAME "Ecore_Audio_In"

/**
 * @brief Sets the playback speed of the input.
 *
 * The speed is clamped between 0.2 and 5.0.
 * Setting the speed will trigger the ECORE_AUDIO_IN_EVENT_IN_SAMPLERATE_CHANGED event.
 *
 * @param[in] obj The Ecore_Audio_Input object data.
 * @param[in] speed The desired playback speed. 1.0 is normal speed.
 */
EOLIAN static void
_ecore_audio_in_speed_set(Eo *eo_obj EINA_UNUSED, Ecore_Audio_Input *obj, double speed)
{
  if (speed < 0.2)
    speed = 0.2;
  if (speed > 5.0)
    speed = 5.0;

  obj->speed = speed;

  efl_event_callback_call(eo_obj, ECORE_AUDIO_IN_EVENT_IN_SAMPLERATE_CHANGED, NULL);
}

/**
 * @brief Gets the playback speed of the input.
 *
 * @param[in] obj The Ecore_Audio_Input object data.
 * @return The current playback speed.
 */
EOLIAN static double
_ecore_audio_in_speed_get(const Eo *eo_obj EINA_UNUSED, Ecore_Audio_Input *obj)
{
  return obj->speed;
}

/**
 * @brief Sets the samplerate of the input.
 *
 * Setting the samplerate will trigger the ECORE_AUDIO_IN_EVENT_IN_SAMPLERATE_CHANGED event.
 *
 * @param[in] obj The Ecore_Audio_Input object data.
 * @param[in] samplerate The desired samplerate in Hz (e.g., 44100).
 */
EOLIAN static void
_ecore_audio_in_samplerate_set(Eo *eo_obj EINA_UNUSED, Ecore_Audio_Input *obj, int samplerate)
{
  obj->samplerate = samplerate;

  efl_event_callback_call(eo_obj, ECORE_AUDIO_IN_EVENT_IN_SAMPLERATE_CHANGED, NULL);
}

/**
 * @brief Gets the samplerate of the input.
 *
 * @param[in] obj The Ecore_Audio_Input object data.
 * @return The current samplerate in Hz.
 */
EOLIAN static int
_ecore_audio_in_samplerate_get(const Eo *eo_obj EINA_UNUSED, Ecore_Audio_Input *obj)
{
  return obj->samplerate;;
}

/**
 * @brief Sets the number of channels for the input.
 *
 * @param[in] obj The Ecore_Audio_Input object data.
 * @param[in] channels The number of audio channels (e.g., 1 for mono, 2 for stereo).
 */
EOLIAN static void
_ecore_audio_in_channels_set(Eo *eo_obj EINA_UNUSED, Ecore_Audio_Input *obj, int channels)
{
  obj->channels = channels;

  /* TODO: Notify output */

}

/**
 * @brief Gets the number of channels for the input.
 *
 * @param[in] obj The Ecore_Audio_Input object data.
 * @return The current number of audio channels.
 */
EOLIAN static int
_ecore_audio_in_channels_get(const Eo *eo_obj EINA_UNUSED, Ecore_Audio_Input *obj)
{
  return obj->channels;
}

/**
 * @brief Sets whether the input should loop.
 *
 * @param[in] obj The Ecore_Audio_Input object data.
 * @param[in] looped EINA_TRUE to enable looping, EINA_FALSE otherwise.
 */
EOLIAN static void
_ecore_audio_in_looped_set(Eo *eo_obj EINA_UNUSED, Ecore_Audio_Input *obj, Eina_Bool looped)
{
  obj->looped = looped;
}

/**
 * @brief Gets whether the input is set to loop.
 *
 * @param[in] obj The Ecore_Audio_Input object data.
 * @return EINA_TRUE if looping is enabled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_ecore_audio_in_looped_get(const Eo *eo_obj EINA_UNUSED, Ecore_Audio_Input *obj)
{
  return obj->looped;
}

/**
 * @brief Gets the total length of the input audio.
 *
 * @param[in] obj The Ecore_Audio_Input object data.
 * @return The length of the audio in seconds.
 */
EOLIAN static double
_ecore_audio_in_length_get(const Eo *eo_obj EINA_UNUSED, Ecore_Audio_Input *obj)
{
  return obj->length;;
}

/**
 * @brief Gets the remaining playback time of the input audio.
 *
 * This function relies on the input being seekable.
 *
 * @param[in] eo_obj The Ecore_Audio_Input Eo object.
 * @param[in] obj The Ecore_Audio_Input object data.
 * @return The remaining time in seconds, or -1 if the input is not seekable.
 */
EOLIAN static double
_ecore_audio_in_remaining_get(const Eo *eo_obj, Ecore_Audio_Input *obj)
{
  if (!obj->seekable) return -1;
  else {
       double ret = 0.0;
       /* XXX const */
       ret = ecore_audio_obj_in_seek((Eo *)eo_obj, 0, SEEK_CUR);
       return obj->length - ret;
  }
}

/**
 * @brief Reads audio data from the input.
 *
 * This function handles pausing, looping, and end-of-stream events.
 * If paused, it fills the buffer with zeros.
 * If looping is enabled and the end of stream is reached, it seeks to the beginning
 * and continues reading.
 *
 * @param[in] eo_obj The Ecore_Audio_Input Eo object.
 * @param[in] obj The Ecore_Audio_Input object data.
 * @param[out] buf The buffer to store the read audio data.
 * @param[in] len The maximum number of bytes to read.
 * @return The number of bytes read, or 0 on end of stream (if not looping), or -1 on error.
 */
EOLIAN static ssize_t
_ecore_audio_in_read(Eo *eo_obj, Ecore_Audio_Input *obj, void *buf, size_t len)
{
  ssize_t len_read = 0;
  const Ecore_Audio_Object *ea_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);

  if (ea_obj->paused) {
    memset(buf, 0, len);
    len_read = len;
  } else {
      len_read = ecore_audio_obj_in_read_internal(eo_obj, buf, len);
      if (len_read == 0) {
          if (!obj->looped || !obj->seekable) {
              efl_event_callback_call(eo_obj, ECORE_AUDIO_IN_EVENT_IN_STOPPED, NULL);
          } else {
              ecore_audio_obj_in_seek(eo_obj, 0, SEEK_SET);
              len_read = ecore_audio_obj_in_read_internal(eo_obj, buf, len);
              efl_event_callback_call(eo_obj, ECORE_AUDIO_IN_EVENT_IN_LOOPED, NULL);
          }
      }

  }

  return len_read;
}

/**
 * @brief Internal function to read audio data using the VIO interface.
 *
 * This function is called by _ecore_audio_in_read to perform the actual read operation.
 *
 * @param[in] eo_obj The Ecore_Audio_Input Eo object.
 * @param[in] _pd The Ecore_Audio_Input object data (unused).
 * @param[out] buf The buffer to store the read audio data.
 * @param[in] len The maximum number of bytes to read.
 * @return The number of bytes read by the VIO's read function.
 */
EOLIAN static ssize_t
_ecore_audio_in_read_internal(Eo *eo_obj, Ecore_Audio_Input *_pd EINA_UNUSED, void *buf, size_t len)
{
  ssize_t len_read = 0;
  const Ecore_Audio_Object *ea_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);

  if (ea_obj->vio && ea_obj->vio->vio->read) {
      len_read = ea_obj->vio->vio->read(ea_obj->vio->data, eo_obj, buf, len);
  }

  return len_read;
}

/**
 * @brief Gets the output object connected to this input.
 *
 * @param[in] obj The Ecore_Audio_Input object data.
 * @return The connected Ecore_Audio_Output Eo object, or NULL if not connected.
 */
EOLIAN static Eo*
_ecore_audio_in_output_get(const Eo *eo_obj EINA_UNUSED, Ecore_Audio_Input *obj)
{
   return obj->output;
}

/**
 * @brief Frees the VIO (Virtual I/O) structure.
 *
 * If a free function was provided when setting the VIO, it will be called.
 *
 * @param[in,out] ea_obj The Ecore_Audio_Object data containing the VIO.
 */
static void _free_vio(Ecore_Audio_Object *ea_obj)
{
  if (ea_obj->vio->free_func)
    ea_obj->vio->free_func(ea_obj->vio->data);

  free(ea_obj->vio);
  ea_obj->vio = NULL;
}

/**
 * @brief Sets the VIO (Virtual I/O) interface for the input.
 *
 * This allows custom data sources to be used with Ecore_Audio.
 * If a VIO is already set, it will be freed first.
 *
 * @param[in] eo_obj The Ecore_Audio_Input Eo object.
 * @param[in,out] obj The Ecore_Audio_Input object data.
 * @param[in] vio The Ecore_Audio_Vio structure with I/O function pointers.
 * @param[in] data User data to be passed to the VIO functions.
 * @param[in] free_func Function to free the user data when the VIO is unset or the object is destroyed.
 */
EOLIAN static void
_ecore_audio_in_ecore_audio_vio_set(Eo *eo_obj, Ecore_Audio_Input *obj, Ecore_Audio_Vio *vio, void *data, efl_key_data_free_func free_func)
{
  Ecore_Audio_Object *ea_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);

  if (ea_obj->vio)
    {
       ERR("VIO already set!");
       _free_vio(ea_obj);
    }

  obj->seekable = obj->seekable_prev;

  if (!vio)
    return;

  ea_obj->vio = calloc(1, sizeof(Ecore_Audio_Vio_Internal));
  ea_obj->vio->vio = vio;
  ea_obj->vio->data = data;
  ea_obj->vio->free_func = free_func;

  obj->seekable_prev = obj->seekable;
  obj->seekable = (vio->seek != NULL);
}

/**
 * @brief Constructor for the Ecore_Audio_Input object.
 *
 * Initializes the default speed to 1.0.
 *
 * @param[in] eo_obj The Ecore_Audio_Input Eo object being constructed.
 * @param[in,out] obj The Ecore_Audio_Input object data.
 * @return The constructed Eo object.
 */
EOLIAN static Eo *
_ecore_audio_in_efl_object_constructor(Eo *eo_obj, Ecore_Audio_Input *obj)
{
  eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

  obj->speed = 1.0;

  return eo_obj;
}

/**
 * @brief Destructor for the Ecore_Audio_Input object.
 *
 * Detaches from any connected output and frees the VIO if set.
 *
 * @param[in] eo_obj The Ecore_Audio_Input Eo object being destructed.
 * @param[in] obj The Ecore_Audio_Input object data.
 */
EOLIAN static void
_ecore_audio_in_efl_object_destructor(Eo *eo_obj, Ecore_Audio_Input *obj)
{
  Ecore_Audio_Object *ea_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);
  if(obj->output)
    {
       if (!ecore_audio_obj_out_input_detach(obj->output, eo_obj))
         ERR("Failed to detach output %p!", obj->output);
    }

  if (ea_obj->vio)
    _free_vio(ea_obj);
  efl_destructor(efl_super(eo_obj, MY_CLASS));
}

#include "ecore_audio_in.eo.c"
