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
#include <math.h>

#define MY_CLASS ECORE_AUDIO_IN_TONE_CLASS
#define MY_CLASS_NAME "Ecore_Audio_In_Tone"

/**
 * @brief Private data for the Ecore_Audio_In_Tone object.
 */
struct _Ecore_Audio_In_Tone_Data
{
  int freq;   /**< Frequency of the tone in Hz */
  int phase;  /**< Current phase of the tone, used for generating the waveform */
};

typedef struct _Ecore_Audio_In_Tone_Data Ecore_Audio_In_Tone_Data;

/**
 * @brief Reads audio data from the tone generator.
 *
 * This function generates a sine wave based on the configured frequency
 * and current phase.
 *
 * @param eo_obj The Ecore_Audio_In_Tone object.
 * @param obj The private data of the Ecore_Audio_In_Tone object.
 * @param data Buffer to store the generated audio data (float samples).
 * @param len Length of the buffer in bytes.
 * @return The number of bytes read, or -1 on error.
 */
EOLIAN static ssize_t
_ecore_audio_in_tone_ecore_audio_in_read_internal(Eo *eo_obj, Ecore_Audio_In_Tone_Data *obj, void *data, size_t len)
{
  size_t i, remain;
  Ecore_Audio_Input *in_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_IN_CLASS);

  float *val = data;

  remain = in_obj->length * in_obj->samplerate * 4 - obj->phase * 4;
  if (remain > len)
    remain = len;

  for (i=0; i<remain/4; i++) {
      val[i] = sin(2* M_PI * obj->freq * (obj->phase + i) / in_obj->samplerate);
  }

  obj->phase += i;

  return remain;
}

/**
 * @brief Seeks to a specific position in the tone stream.
 *
 * @param eo_obj The Ecore_Audio_In_Tone object.
 * @param obj The private data of the Ecore_Audio_In_Tone object.
 * @param offs Offset in seconds.
 * @param mode Seek mode (SEEK_SET, SEEK_CUR, SEEK_END).
 * @return The new position in seconds from the beginning, or -1.0 on error.
 */
EOLIAN static double
_ecore_audio_in_tone_ecore_audio_in_seek(Eo *eo_obj, Ecore_Audio_In_Tone_Data *obj, double offs, int mode)
{
  int tmp;
  Ecore_Audio_Input *in_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_IN_CLASS);

  switch (mode) {
    case SEEK_SET:
      tmp = offs * in_obj->samplerate;
      break;
    case SEEK_CUR:
      tmp = obj->phase + offs * in_obj->samplerate;
      break;
    case SEEK_END:
      tmp = (in_obj->length + offs) * in_obj->samplerate;
      break;
    default:
      goto err;
  }
  if ((tmp < 0) || (tmp > in_obj->length * in_obj->samplerate))
    goto err;

  obj->phase = tmp;

  return (double)obj->phase / in_obj->samplerate;
err:
  return -1.0;
}

/**
 * @brief Sets the length of the tone.
 *
 * @param eo_obj The Ecore_Audio_In_Tone object.
 * @param _pd The private data of the Ecore_Audio_In_Tone object (unused).
 * @param length The new length in seconds.
 */
EOLIAN static void
_ecore_audio_in_tone_ecore_audio_in_length_set(Eo *eo_obj, Ecore_Audio_In_Tone_Data *_pd EINA_UNUSED, double length)
{
  Ecore_Audio_Input *in_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_IN_CLASS);
  in_obj->length = length;
}

/**
 * @brief Sets key-value data on the Ecore_Audio_In_Tone object.
 *
 * This function is used to set attributes like the tone frequency
 * using ECORE_AUDIO_ATTR_TONE_FREQ.
 *
 * @param eo_obj The Ecore_Audio_In_Tone object.
 * @param obj The private data of the Ecore_Audio_In_Tone object.
 * @param key The key of the data to set (e.g., ECORE_AUDIO_ATTR_TONE_FREQ).
 * @param val Pointer to the value to set. For ECORE_AUDIO_ATTR_TONE_FREQ, this should be a pointer to an int.
 */
EOLIAN static void
_ecore_audio_in_tone_efl_object_key_data_set(Eo *eo_obj, Ecore_Audio_In_Tone_Data *obj, const char *key, const void *val)
{
  if (!key) return;

  if (!strcmp(key, ECORE_AUDIO_ATTR_TONE_FREQ)) {
      obj->freq = *(int *)val;
  } else {
      efl_key_data_set(efl_super(eo_obj, MY_CLASS), key, val);
  }

}

/**
 * @brief Gets key-value data from the Ecore_Audio_In_Tone object.
 *
 * This function is used to retrieve attributes like the tone frequency
 * using ECORE_AUDIO_ATTR_TONE_FREQ.
 *
 * @param eo_obj The Ecore_Audio_In_Tone object.
 * @param obj The private data of the Ecore_Audio_In_Tone object.
 * @param key The key of the data to get (e.g., ECORE_AUDIO_ATTR_TONE_FREQ).
 * @return Pointer to the value. For ECORE_AUDIO_ATTR_TONE_FREQ, this will be the frequency value cast to a void pointer.
 *         Returns NULL if the key is not found by this object and defers to superclass.
 */
EOLIAN static void *
_ecore_audio_in_tone_efl_object_key_data_get(Eo *eo_obj, Ecore_Audio_In_Tone_Data *obj, const char *key)
{
  if (!strcmp(key, ECORE_AUDIO_ATTR_TONE_FREQ)) {
      return (void *) (intptr_t) obj->freq;
  } else {
      return efl_key_data_get(efl_super(eo_obj, MY_CLASS), key);
  }
}

/**
 * @brief Constructor for the Ecore_Audio_In_Tone object.
 *
 * Initializes the tone generator with default values:
 * - Channels: 1
 * - Samplerate: 44100 Hz
 * - Length: 1 second
 * - Seekable: True
 * - Frequency: 1000 Hz
 *
 * @param eo_obj The Ecore_Audio_In_Tone object being constructed.
 * @param obj The private data of the Ecore_Audio_In_Tone object.
 * @return The constructed Eo object.
 */
EOLIAN static Eo *
_ecore_audio_in_tone_efl_object_constructor(Eo *eo_obj, Ecore_Audio_In_Tone_Data *obj)
{
  Ecore_Audio_Input *in_obj = efl_data_scope_get(eo_obj, ECORE_AUDIO_IN_CLASS);

  eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

  in_obj->channels = 1;
  in_obj->samplerate = 44100;
  in_obj->length = 1;
  in_obj->seekable = EINA_TRUE;

  obj->freq = 1000;

  return eo_obj;
}

#define ECORE_AUDIO_IN_TONE_EXTRA_OPS \
   EFL_OBJECT_OP_FUNC(efl_key_data_set, _ecore_audio_in_tone_efl_object_key_data_set), \
   EFL_OBJECT_OP_FUNC(efl_key_data_get, _ecore_audio_in_tone_efl_object_key_data_get)

#include "ecore_audio_in_tone.eo.c"
