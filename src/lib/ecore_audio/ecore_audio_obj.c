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

#include <Eo.h>
#include "ecore_audio_private.h"

#define MY_CLASS ECORE_AUDIO_CLASS
#define MY_CLASS_NAME "Ecore_Audio"

/**
 * @brief Sets the paused state of the audio object.
 *
 * @param[in] eo_obj The Ecore_Audio object.
 * @param[in] obj The internal Ecore_Audio_Object data.
 * @param[in] paused EINA_TRUE to pause, EINA_FALSE to unpause.
 */
EOLIAN static void
_ecore_audio_paused_set(Eo *eo_obj EINA_UNUSED, Ecore_Audio_Object *obj, Eina_Bool paused)
{
  obj->paused = paused;
}

/**
 * @brief Gets the paused state of the audio object.
 *
 * @param[in] eo_obj The Ecore_Audio object.
 * @param[in] obj The internal Ecore_Audio_Object data.
 * @return EINA_TRUE if paused, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_ecore_audio_paused_get(const Eo *eo_obj EINA_UNUSED, Ecore_Audio_Object *obj)
{
  return obj->paused;
}

/**
 * @brief Sets the volume of the audio object.
 *
 * @param[in] eo_obj The Ecore_Audio object.
 * @param[in] obj The internal Ecore_Audio_Object data.
 * @param[in] volume The volume level (0.0 to 1.0).
 */
EOLIAN static void
_ecore_audio_volume_set(Eo *eo_obj EINA_UNUSED, Ecore_Audio_Object *obj, double volume)
{
  obj->volume = volume;
}

/**
 * @brief Gets the volume of the audio object.
 *
 * @param[in] eo_obj The Ecore_Audio object.
 * @param[in] obj The internal Ecore_Audio_Object data.
 * @return The current volume level (0.0 to 1.0).
 */
EOLIAN static double
_ecore_audio_volume_get(const Eo *eo_obj EINA_UNUSED, Ecore_Audio_Object *obj)
{
  return obj->volume;
}

/**
 * @brief Constructor for the Ecore_Audio object.
 *
 * Initializes the Ecore_Audio object, setting the default volume to 1.0.
 *
 * @param[in] eo_obj The Ecore_Audio object being constructed.
 * @param[in] obj The internal Ecore_Audio_Object data.
 * @return The constructed Efl_Object.
 */
EOLIAN static Eo *
_ecore_audio_efl_object_constructor(Eo *eo_obj, Ecore_Audio_Object *obj)
{
  obj->volume = 1.0;
  return efl_constructor(efl_super(eo_obj, MY_CLASS));
}

#include "ecore_audio.eo.c"
