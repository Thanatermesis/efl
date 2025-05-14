
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_ALSA

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif
#include <ctype.h>
#include <errno.h>

#include <alsa/asoundlib.h>

#include "ecore_audio_private.h"

static Ecore_Audio_Module *alsa_module = NULL;

/**
 * @brief Creates a new ALSA output object.
 *
 * This function initializes an ALSA playback stream and sets its parameters.
 *
 * @param output The Ecore_Audio_Object to initialize as an ALSA output.
 * @return The initialized Ecore_Audio_Object on success, NULL on failure.
 */
static Ecore_Audio_Object *
_alsa_output_new(Ecore_Audio_Object *output)
{
   int ret;
   Ecore_Audio_Output *out = (Ecore_Audio_Output *)output;
   struct _Ecore_Audio_Alsa *alsa;

   alsa = calloc(1, sizeof(struct _Ecore_Audio_Alsa));
   if (!alsa)
     {
        ERR("Could not allocate memory for private structure.");
        free(out);
        return NULL;
     }

   out->module_data = alsa;

   ret = snd_pcm_open(&alsa->handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
   if (ret < 0)
     {
        ERR("Could not open playback: %s", snd_strerror(ret));
        goto err;
     }

   /* XXX: Make configurable */
   ret = snd_pcm_set_params(alsa->handle, SND_PCM_FORMAT_FLOAT, SND_PCM_ACCESS_RW_INTERLEAVED, 2, 44100, 1, 500000);
   if (ret < 0)
     {
        ERR("Could not set parameters: %s", snd_strerror(ret));
        goto err;
     }

   return output;

err:
   free(out->module_data);
   return NULL;
}

/**
 * @brief Deletes an ALSA output object.
 *
 * Frees the private ALSA data associated with the output object.
 *
 * @param output The Ecore_Audio_Object to delete.
 */
static void
_alsa_output_del(Ecore_Audio_Object *output)
{
   free(output->module_data);
}

/**
 * @brief Asynchronous callback for ALSA output.
 *
 * This function is called periodically by a timer to read audio data from
 * the input, and write it to the ALSA playback device. It handles
 * buffer availability and recovers from ALSA errors.
 *
 * @param output The Ecore_Audio_Object representing the ALSA output.
 * @return EINA_TRUE to continue the timer, EINA_FALSE to stop.
 */
static Eina_Bool
_alsa_output_async_cb(Ecore_Audio_Object *output)
{
   unsigned char *data;
   int size, ret;
   snd_pcm_sframes_t avail;
   Ecore_Audio_Output *out = output;
   Ecore_Audio_Input *in = eina_list_data_get(out->inputs);
   struct _Ecore_Audio_Alsa *alsa = (struct _Ecore_Audio_Alsa *)output->module_data;

   /* XXX: Error handling! */
   avail = snd_pcm_avail_update(alsa->handle);
   ERR("Aval: %i", avail);
   if (!avail)
     return EINA_TRUE;
   if (avail < 0)
     goto recover;

   data = calloc(1, sizeof(float) * avail * 8);

   size = in->module->in_ops->input_read((Ecore_Audio_Object *)in, data, avail*8);
   ERR("Read: %i", size);

   ret = snd_pcm_writei(alsa->handle, data, size/8);
   ERR("Written: %i", ret);
   if (ret < 0) {
recover:
       ERR(snd_strerror(ret));
     snd_pcm_recover(alsa->handle, ret, 0);
   }

   return EINA_TRUE;
}

/**
 * @brief Adds an input to an ALSA output.
 *
 * This function currently starts a timer that periodically calls
 * _alsa_output_async_cb to process audio data.
 *
 * @param output The Ecore_Audio_Object representing the ALSA output.
 * @param input The Ecore_Audio_Object representing the input to add (currently unused in this function).
 */
static void
_alsa_output_add_input(Ecore_Audio_Object *output, Ecore_Audio_Object *input EINA_UNUSED)
{
   // TODO: The 'input' parameter is not used. It might be intended for future use
   // where multiple inputs could be mixed or selected.
   // For now, the callback _alsa_output_async_cb fetches the first input
   // from out->inputs.
   ecore_timer_add(0.3, _alsa_output_async_cb, output);
}

static struct input_api inops = {
};

static struct output_api outops = {
    .output_new = _alsa_output_new,
    .output_del = _alsa_output_del,
//    .output_volume_set = _alsa_output_volume_set,
    .output_add_input = _alsa_output_add_input,
//    .output_del_input = _alsa_output_del_input,
//    .output_update_input_format = _alsa_output_update_input_format,
};

/* externally accessible functions */

/**
 * @addtogroup Ecore_Audio_Group Ecore_Audio_alsa - Ecore internal ALSA audio functions
 *
 * @{
 */

/**
 * @brief Initialize the Ecore_Audio ALSA module.
 *
 * @return the initialized module success, NULL on error.
 *
 */
Ecore_Audio_Module *
ecore_audio_alsa_init(void)
{
   alsa_module = calloc(1, sizeof(Ecore_Audio_Module));
   if (!alsa_module)
     {
        ERR("Could not allocate memory for the module.");
        return NULL;
     }

   ECORE_MAGIC_SET(alsa_module, ECORE_MAGIC_AUDIO_MODULE);
   alsa_module->type = ECORE_AUDIO_TYPE_ALSA;
   alsa_module->name = "alsa";
   alsa_module->inputs = NULL;
   alsa_module->outputs = NULL;
   alsa_module->in_ops = &inops;
   alsa_module->out_ops = &outops;

   return alsa_module;
}

/**
 * @brief Shut down the Ecore_Audio ALSA module
 */
void
ecore_audio_alsa_shutdown(void)
{
   free(alsa_module);
   alsa_module = NULL;
}

/**
 * @}
 */

#endif /* HAVE_ALSA */
