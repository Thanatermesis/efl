#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Ecore.h"
#include "ecore_private.h"
#include "Ecore_Audio.h"
#include "ecore_audio_private.h"

static Ecore_Audio_Module *module = NULL;

/**
 * @brief Creates a new custom input object.
 * @param input The Ecore_Audio_Object to initialize as a custom input.
 * @return The initialized Ecore_Audio_Object, or NULL on failure.
 *
 * This function initializes the custom input, setting default samplerate and channels,
 * and allocating memory for callback data.
 */
static Ecore_Audio_Object *
_custom_input_new(Ecore_Audio_Object *input)
{
   Ecore_Audio_Input *in = (Ecore_Audio_Input *)input;
   struct _Ecore_Audio_Callback *cb = calloc(1, sizeof(struct _Ecore_Audio_Callback));

   in->samplerate = 44100;
   in->channels = 2;

   in->module_data = cb;

   return (Ecore_Audio_Object *)in;
}

/**
 * @brief Deletes a custom input object.
 * @param input The Ecore_Audio_Object representing the custom input to delete.
 *
 * This function frees the resources associated with the custom input,
 * specifically the callback data.
 */
static void
_custom_input_del(Ecore_Audio_Object *input)
{
   Ecore_Audio_Input *in = (Ecore_Audio_Input *)input;
   free(in->module_data);
}

/**
 * @brief Reads audio data from the custom input.
 * @param input The Ecore_Audio_Object representing the custom input.
 * @param data Buffer to store the read audio data.
 * @param len The maximum number of bytes to read.
 * @return The number of bytes read, or 0 if paused or at end of stream.
 *
 * If the input is paused, this function will fill the buffer with zeros.
 * Otherwise, it calls the registered read callback to get audio data.
 */
static int
_custom_input_read(Ecore_Audio_Object *input, void *data, int len)
{
   Ecore_Audio_Input *in = (Ecore_Audio_Input *)input;
   struct _Ecore_Audio_Callback *cb = in->module_data;

   if (in->paused)
     {
        memset(data, 0, len);
        return len;
     }

   return cb->read_cb(cb->data, data, len);
}

/**
 * @brief Seeks to a position in the custom input stream.
 * @param input The Ecore_Audio_Object representing the custom input (unused).
 * @param offs The offset to seek to (unused).
 * @param mode The seeking mode (unused).
 * @return Always returns 0 as seeking is not supported for custom inputs.
 */
static double
_custom_input_seek(Ecore_Audio_Object *input EINA_UNUSED, double offs EINA_UNUSED, int mode EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Input operations API structure for custom inputs.
 *
 * This structure holds pointers to the functions that implement
 * the input operations for the custom audio module.
 */
static struct input_api inops = {
   .input_new = _custom_input_new,
   .input_del = _custom_input_del,
   .input_read = _custom_input_read,
   .input_seek = _custom_input_seek,
};

/**
 * @brief Initialize the Ecore_Audio sndfile module
 *
 * @return the initialized module on success, NULL on error
 */
Ecore_Audio_Module *
ecore_audio_custom_init(void)
{
   module = calloc(1, sizeof(Ecore_Audio_Module));
   if (!module)
     {
        ERR("Could not allocate memory for module.");
        return NULL;
     }

   ECORE_MAGIC_SET(module, ECORE_MAGIC_AUDIO_MODULE);
   module->type = ECORE_AUDIO_TYPE_CUSTOM;
   module->name = "custom";
   module->inputs = NULL;
   module->outputs = NULL;
   module->in_ops = &inops;
   module->out_ops = NULL;

   DBG("Initialized");
   return module;
}

/**
 * @brief Shut down the Ecore_Audio custom module
 */
void
ecore_audio_custom_shutdown(void)
{
   free(module);
   module = NULL;

   DBG("Shutting down");
}
