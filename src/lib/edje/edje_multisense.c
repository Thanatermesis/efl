#include "edje_private.h"

#ifdef ENABLE_MULTISENSE
#include "Ecore_Audio.h"

static Eo *out = NULL;
static int outs = 0;
static Eina_List *ins = NULL;
static Eina_Bool outfail = EINA_FALSE;

/**
 * @brief Callback function invoked when audio playback finishes.
 *
 * This function is registered as an event handler for ECORE_AUDIO_IN_EVENT_IN_STOPPED.
 * It cleans up resources associated with the finished audio input stream by removing
 * it from the list of active inputs and decrementing its reference count.
 *
 * @param data Custom data pointer (unused).
 * @param event The EFL event information. The event object is the audio input that stopped.
 */
static void
_play_finished(void *data EINA_UNUSED, const Efl_Event *event)
{
   ins = eina_list_remove(ins, event->object);
   efl_unref(event->object);
}

#if defined(_WIN32) || defined(HAVE_PULSE)
/**
 * @brief Callback function for audio output failure.
 *
 * This function is registered as an event handler for context failure events
 * from the audio output backend (WASAPI or PulseAudio). It marks the output as failed,
 * preventing further playback attempts, and releases the output object.
 *
 * @param data Custom data pointer (unused).
 * @param event The EFL event information. The event object is the audio output that failed.
 */
static void
_out_fail(void *data EINA_UNUSED, const Efl_Event *event)
{
   outfail = EINA_TRUE;
   efl_unref(event->object);
   out = NULL;
}
#endif

/**
 * @brief Manages audio data read from an Eet file for Ecore_Audio's VIO.
 *
 * This structure holds all necessary information to stream audio data directly
 * from an Edje file's Eet archive without loading the entire sample into a
 * separate memory buffer. It is used with the Ecore_Audio VIO (Virtual I/O) interface.
 */
struct _edje_multisense_eet_data
{
   unsigned int    offset, length; /**< Current read offset and total length of the audio data. */
   Eet_File       *ef; /**< A handle to the open Eet file, kept to ensure the mmap is valid. */
   const char     *data; /**< Direct pointer to the audio data within the mmap'd Eet file. */
   Ecore_Audio_Vio vio; /**< The VIO function table for Ecore_Audio. */
};

/**
 * @brief VIO callback to get the total length of the audio data.
 * @param data A pointer to a struct _edje_multisense_eet_data.
 * @param eo_obj The Ecore_Audio object (unused).
 * @return The total length of the audio data in bytes.
 */
static int
eet_snd_file_get_length(void *data, Eo *eo_obj EINA_UNUSED)
{
   struct _edje_multisense_eet_data *vf = data;
   return vf->length;
}

/**
 * @brief VIO callback to seek to a position in the audio data.
 * @param data A pointer to a struct _edje_multisense_eet_data.
 * @param eo_obj The Ecore_Audio object (unused).
 * @param offset The offset to seek to.
 * @param whence The positioning mode (SEEK_SET, SEEK_CUR, SEEK_END).
 * @return The new offset from the beginning of the data.
 */
static int
eet_snd_file_seek(void *data, Eo *eo_obj EINA_UNUSED, int offset, int whence)
{
   struct _edje_multisense_eet_data *vf = data;

   switch (whence)
     {
      case SEEK_SET:
        vf->offset = offset;
        break;

      case SEEK_CUR:
        vf->offset += offset;
        break;

      case SEEK_END:
        vf->offset = vf->length + offset;
        break;

      default:
        break;
     }
   return vf->offset;
}

/**
 * @brief VIO callback to read a chunk of audio data.
 * @param data A pointer to a struct _edje_multisense_eet_data.
 * @param eo_obj The Ecore_Audio object (unused).
 * @param buffer The destination buffer to copy data into.
 * @param count The number of bytes to read.
 * @return The number of bytes actually read.
 */
static int
eet_snd_file_read(void *data, Eo *eo_obj EINA_UNUSED, void *buffer, int count)
{
   struct _edje_multisense_eet_data *vf = data;

   if ((vf->offset + count) > vf->length)
     count = vf->length - vf->offset;
   memcpy(buffer, vf->data + vf->offset, count);
   vf->offset += count;
   return count;
}

/**
 * @brief VIO callback to get the current read position.
 * @param data A pointer to a struct _edje_multisense_eet_data.
 * @param eo_obj The Ecore_Audio object (unused).
 * @return The current offset in the audio data.
 */
static int
eet_snd_file_tell(void *data, Eo *eo_obj EINA_UNUSED)
{
   struct _edje_multisense_eet_data *vf = data;

   return vf->offset;
}

/**
 * @brief Frees the resources associated with an Eet-based audio stream.
 *
 * This function is registered as a cleanup callback for the VIO data.
 * It closes the Eet file handle, which is necessary because the audio data
 * was accessed via a direct mmap pointer (`eet_read_direct`), and frees the
 * _edje_multisense_eet_data container.
 *
 * @param data A pointer to a struct _edje_multisense_eet_data to be freed.
 */
static void
_free(void *data)
{
   struct _edje_multisense_eet_data *eet_data = data;

   if (eet_data->ef) eet_close(eet_data->ef);
// don't free if eet_data->data  comes from eet_read_direct
//  free(eet_data->data);
   free(data);
   outs--;
}

static Eina_Bool _channel_mute_states[8] = { 0 };

/**
 * @brief Checks if a specific audio channel is muted.
 *
 * This function checks both the per-channel mute state and a global mute flag.
 * Channel 7 is treated as a "master mute" switch.
 *
 * @param ed The Edje object (unused).
 * @param channel The audio channel to check (0-6 for specific channels, 7 for master).
 * @return @c EINA_TRUE if the channel is muted, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_channel_mute(Edje *ed EINA_UNUSED, int channel)
{
   // ed lets use set mute per object... but for now no api's for this
   // if all are muted ... then all!
   if (_channel_mute_states[7]) return EINA_TRUE;
   if ((channel < 0) || (channel > 7)) return EINA_FALSE;
   return _channel_mute_states[channel];
   return EINA_FALSE;
}

#endif

/**
 * @brief Sets the mute state for a specific audio channel.
 *
 * @param channel The channel to modify. Can be one of EDJE_CHANNEL_MUSIC,
 *        EDJE_CHANNEL_EFFECT, etc., up to EDJE_CHANNEL_ALL.
 *        EDJE_CHANNEL_ALL (7) acts as a master mute for all channels.
 * @param mute EINA_TRUE to mute the channel, EINA_FALSE to unmute.
 *
 * @ingroup Edje_Audio
 */
EAPI void
edje_audio_channel_mute_set(Edje_Channel channel, Eina_Bool mute)
{
#ifdef ENABLE_MULTISENSE
   if ((unsigned)channel > 7) return;
   _channel_mute_states[channel] = mute;
#else
   (void)channel;
   (void)mute;
#endif
}

/**
 * @brief Gets the mute state of a specific audio channel.
 *
 * @param channel The channel to check.
 * @return @c EINA_TRUE if the channel is muted, @c EINA_FALSE otherwise.
 *
 * @ingroup Edje_Audio
 */
EAPI Eina_Bool
edje_audio_channel_mute_get(Edje_Channel channel)
{
#ifdef ENABLE_MULTISENSE
   if ((unsigned)channel > 7) return EINA_FALSE;
   return _channel_mute_states[channel];
#else
   (void)channel;
   return EINA_TRUE;
#endif
}

/**
 * @internal
 * @brief Plays a sound sample from an Edje file.
 *
 * This is the internal implementation for playing a sound specified in the
 * "sounds" block of an EDC file. It finds the sample by name, sets up
 * a VIO stream to read it directly from the Eet-archived Edje file,
 * and plays it through the Ecore_Audio system.
 *
 * @param ed The Edje object containing the sound.
 * @param sample_name The name of the sample to play.
 * @param speed The playback speed modifier (1.0 for normal).
 * @param channel The audio channel to play on.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
Eina_Bool
_edje_multisense_internal_sound_sample_play(Edje *ed, const char *sample_name, const double speed, int channel)
{
#ifdef ENABLE_MULTISENSE
   Eo *in;
   Edje_Sound_Sample *sample;
   char snd_id_str[255];
   int i;
   Eina_Bool ret = EINA_FALSE;

   if (_channel_mute(ed, channel)) return EINA_FALSE;

   if (outfail) return EINA_FALSE;

   if (!sample_name)
     {
        ERR("Given Sample Name is NULL\n");
        return EINA_FALSE;
     }

   if ((!ed) || (!ed->file) || (!ed->file->sound_dir))
     return EINA_FALSE;

   for (i = 0; i < (int)ed->file->sound_dir->samples_count; i++)
     {
        sample = &ed->file->sound_dir->samples[i];
        if (!strcmp(sample->name, sample_name))
          {
             struct _edje_multisense_eet_data *eet_data;
             int len;

             snprintf(snd_id_str, sizeof(snd_id_str), "edje/sounds/%i", sample->id);

             eet_data = calloc(1, sizeof(struct _edje_multisense_eet_data));
             if (!eet_data)
               {
                  ERR("Out of memory in allocating multisense sample info");
                  return EINA_FALSE;
               }
             // open eet file again to esnure we have  reference because we
             // use eet_read_direct to avoid duplicating/copying into memory
             // by relying on a direct mmap, but this means we need to close
             // the eet file handle instead of freeing data
             eet_data->ef = eet_mmap(ed->file->f);
             if (!eet_data->ef)
               {
                  ERR("Cannot open edje file '%s' for samples", ed->path);
                  free(eet_data);
                  return EINA_FALSE;
               }
             eet_data->data = eet_read_direct(eet_data->ef, snd_id_str, &len);
             if (len <= 0)
               {
                  ERR("Sample form edj file '%s' is 0 length", ed->path);
                  eet_close(eet_data->ef);
                  free(eet_data);
                  return EINA_FALSE;
               }
             eet_data->length = len;
             /* action->speed */

             eet_data->vio.get_length = eet_snd_file_get_length;
             eet_data->vio.seek = eet_snd_file_seek;
             eet_data->vio.read = eet_snd_file_read;
             eet_data->vio.tell = eet_snd_file_tell;
             eet_data->offset = 0;

             in = efl_add_ref(ECORE_AUDIO_IN_SNDFILE_CLASS, NULL, efl_name_set(efl_added, snd_id_str), ecore_audio_obj_in_speed_set(efl_added, speed), ecore_audio_obj_vio_set(efl_added, &eet_data->vio, eet_data, _free), efl_event_callback_add(efl_added, ECORE_AUDIO_IN_EVENT_IN_STOPPED, _play_finished, NULL));
             if (!out)
               {

# ifdef _WIN32
                  out = efl_add_ref(ECORE_AUDIO_OUT_WASAPI_CLASS, NULL, efl_event_callback_add(efl_added, ECORE_AUDIO_OUT_WASAPI_EVENT_CONTEXT_FAIL, _out_fail, NULL));
# else
#  ifdef HAVE_PULSE
                  out = efl_add_ref(ECORE_AUDIO_OUT_PULSE_CLASS, NULL, efl_event_callback_add(efl_added, ECORE_AUDIO_OUT_PULSE_EVENT_CONTEXT_FAIL, _out_fail, NULL));
#  endif
# endif
                  if (out) outs++;
               }
             if (!out)
               {
                  static Eina_Bool complained = EINA_FALSE;

                  if (!complained)
                    {
                       complained = EINA_TRUE;
# ifdef _WIN32
                       ERR("Could not create multisense audio out (wasapi)");
# else
#  ifdef HAVE_PULSE
                       ERR("Could not create multisense audio out (pulse)");
#  endif
# endif
                    }
                  efl_unref(in);
                  return EINA_FALSE;
               }
             ret = ecore_audio_obj_out_input_attach(out, in);
             if (!ret)
               {
                  static Eina_Bool complained = EINA_FALSE;

                  if (!complained)
                    {
                       complained = EINA_TRUE;
                       ERR("Could not attach input");
                    }
                  efl_unref(in);
                  return EINA_FALSE;
               }
             ins = eina_list_append(ins, in);
          }
     }
   return EINA_TRUE;
#else
   // warning shh
   (void)ed;
   (void)sample_name;
   (void)speed;
   (void)channel;
   return EINA_FALSE;
#endif
}

/**
 * @internal
 * @brief Plays a synthesized tone.
 *
 * This is the internal implementation for playing a tone specified in the
 * "sounds" block of an EDC file. It finds the tone definition by name
 * and uses Ecore_Audio to generate and play a sine wave of the specified
 * frequency and duration.
 *
 * @param ed The Edje object containing the tone definition.
 * @param tone_name The name of the tone to play.
 * @param duration The duration of the tone in seconds.
 * @param channel The audio channel to play on.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
Eina_Bool
_edje_multisense_internal_sound_tone_play(Edje *ed, const char *tone_name, const double duration, int channel)
{
#ifdef ENABLE_MULTISENSE
   unsigned int i;
   Edje_Sound_Tone *tone;
   Eina_Bool ret = EINA_FALSE;

   Eo *in;
   if (!tone_name)
     {
        ERR("Given Tone Name is NULL");
        return EINA_FALSE;
     }

   if (_channel_mute(ed, channel)) return EINA_FALSE;

   if (outfail) return EINA_FALSE;

   if ((!ed) || (!ed->file) || (!ed->file->sound_dir))
     return EINA_FALSE;

   for (i = 0; i < ed->file->sound_dir->tones_count; i++)
     {
        tone = &ed->file->sound_dir->tones[i];
        if (!strcmp(tone->name, tone_name))
          {
             in = efl_add_ref(ECORE_AUDIO_IN_TONE_CLASS, NULL);
             efl_name_set(in, "tone");
             efl_key_data_set(in, ECORE_AUDIO_ATTR_TONE_FREQ, &tone->value);
             ecore_audio_obj_in_length_set(in, duration);
             efl_event_callback_add(in, ECORE_AUDIO_IN_EVENT_IN_STOPPED, _play_finished, NULL);

             if (!out)
               {
# ifdef _WIN32
                  out = efl_add_ref(ECORE_AUDIO_OUT_WASAPI_CLASS, NULL, efl_event_callback_add(efl_added, ECORE_AUDIO_OUT_WASAPI_EVENT_CONTEXT_FAIL, _out_fail, NULL));
# else
#  ifdef HAVE_PULSE
                  out = efl_add_ref(ECORE_AUDIO_OUT_PULSE_CLASS, NULL, efl_event_callback_add(efl_added, ECORE_AUDIO_OUT_PULSE_EVENT_CONTEXT_FAIL, _out_fail, NULL));
#  endif
# endif
                  if (out) outs++;
               }

             ret = ecore_audio_obj_out_input_attach(out, in);
             if (!ret)
               {
                  ERR("Could not attach input");
                  efl_unref(in);
                  return EINA_FALSE;
               }
          }
     }
   return EINA_TRUE;
#else
   // warning shh
   (void)ed;
   (void)duration;
   (void)tone_name;
   (void)channel;
   return EINA_FALSE;
#endif
}

/**
 * @internal
 * @brief Plays a vibration pattern.
 *
 * @note This function is a stub and is not yet implemented.
 *
 * @param ed The Edje object.
 * @param sample_name The name of the vibration sample.
 * @param repeat The number of times to repeat the vibration.
 * @return Currently always @c EINA_FALSE.
 */
Eina_Bool
_edje_multisense_internal_vibration_sample_play(Edje *ed EINA_UNUSED, const char *sample_name EINA_UNUSED, int repeat EINA_UNUSED)
{
#ifdef ENABLE_MULTISENSE
   ERR("Vibration is not supported yet, name:%s, repeat:%d", sample_name, repeat);
   return EINA_FALSE;
#else
   (void)ed;
   (void)repeat;
   return EINA_FALSE;
#endif
}

/**
 * @internal
 * @brief Initializes the multisense (audio) subsystem.
 *
 * This function must be called before any other multisense functions.
 * It initializes the underlying Ecore_Audio library.
 */
void
_edje_multisense_init(void)
{
#ifdef ENABLE_MULTISENSE
   ecore_audio_init();
#endif
}

/**
 * @internal
 * @brief Shuts down the multisense (audio) subsystem.
 *
 * This function cleans up all resources used by the audio system. It stops
 * any playing sounds, destroys the audio output and input objects, and shuts
 * down the Ecore_Audio library. It should be called on application exit.
 */
void
_edje_multisense_shutdown(void)
{
#ifdef ENABLE_MULTISENSE
   Eo *in;
   if (outs > 0)
     {
        WRN("Shutting down audio while samples still playing");
     }
   if (out)
     {
        efl_unref(out);
        out = NULL;
        outs = 0;
     }
   EINA_LIST_FREE(ins, in)
     efl_unref(in);
   ecore_audio_shutdown();
#endif
}

