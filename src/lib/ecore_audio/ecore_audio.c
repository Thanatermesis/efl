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

/**
 * @internal
 * @brief Log domain for Ecore_Audio.
 */
int _ecore_audio_log_dom = -1;
/**
 * @internal
 * @brief Initialization counter for Ecore_Audio.
 *
 * Incremented by ecore_audio_init() and decremented by ecore_audio_shutdown().
 * Ecore_Audio is initialized when the counter goes from 0 to 1,
 * and shut down when it goes from 1 to 0.
 */
static int _ecore_audio_init_count = 0;
/**
 * @internal
 * @brief List of loaded Ecore_Audio modules.
 *
 * This list is currently not used for managing modules directly but could be
 * intended for future enhancements or tracking.
 */
Eina_List *ecore_audio_modules;

#ifdef HAVE_PULSE
/**
 * @internal
 * @brief Global structure holding function pointers for the PulseAudio library.
 *
 * This structure is populated by ecore_audio_pulse_lib_load() if PulseAudio
 * support is compiled in and the library can be dynamically loaded.
 */
Ecore_Audio_Lib_Pulse *ecore_audio_pulse_lib = NULL;
#endif /* HAVE_PULSE */
#ifdef HAVE_SNDFILE
/**
 * @internal
 * @brief Global structure holding function pointers for the Sndfile library.
 *
 * This structure is populated by ecore_audio_sndfile_lib_load() if Sndfile
 * support is compiled in and the library can be dynamically loaded.
 */
Ecore_Audio_Lib_Sndfile *ecore_audio_sndfile_lib = NULL;
#endif /* HAVE_SNDFILE */

/* externally accessible functions */

/**
 * @brief Initializes the Ecore_Audio library.
 *
 * This function initializes all the necessary subsystems for Ecore_Audio to work.
 * It increments a counter, and if the counter was 0, it initializes Ecore,
 * Efl_Object, and registers a log domain for Ecore_Audio.
 *
 * @return The new initialization count. 0 or negative on failure.
 *
 * @see ecore_audio_shutdown()
 */
ECORE_AUDIO_API int
ecore_audio_init(void)
{

   if (++_ecore_audio_init_count != 1)
     return _ecore_audio_init_count;

   if (!ecore_init())
     return --_ecore_audio_init_count;

   if (!efl_object_init()) {
     ecore_shutdown();
     return --_ecore_audio_init_count;
   }

   _ecore_audio_log_dom = eina_log_domain_register("ecore_audio", ECORE_AUDIO_DEFAULT_LOG_COLOR);
   if (_ecore_audio_log_dom < 0)
     {
        EINA_LOG_ERR("Impossible to create a log domain for the ecore audio module.");
        return --_ecore_audio_init_count;
     }

   DBG("Ecore_Audio init");
   ecore_audio_modules = NULL;


   eina_log_timing(_ecore_audio_log_dom,
		   EINA_LOG_STATE_STOP,
		   EINA_LOG_STATE_INIT);

   return _ecore_audio_init_count;
}

/**
 * @brief Shuts down the Ecore_Audio library.
 *
 * This function shuts down all the subsystems used by Ecore_Audio.
 * It decrements a counter, and if the counter becomes 0, it unregisters
 * the log domain, frees the list of modules, and shuts down Efl_Object and Ecore.
 *
 * @note Library unloading for PulseAudio and Sndfile is explicitly disabled
 *       to prevent crashes if objects still exist and try to access them.
 *
 * @return The new initialization count.
 *
 * @see ecore_audio_init()
 */
ECORE_AUDIO_API int
ecore_audio_shutdown(void)
{
   DBG("Ecore_Audio shutdown");
   if (--_ecore_audio_init_count != 0)
     return _ecore_audio_init_count;

#ifdef HAVE_SNDFILE
// explicitly disabled - yes, we know to "fix a leak" you unload here, but
// objects may still exist at this point and may access functions/symbols
// from sndfile
//   ecore_audio_sndfile_lib_unload();
#endif /* HAVE_SNDFILE */
#ifdef HAVE_PULSE
// explicitly disabled - yes, we know to "fix a leak" you unload here, but
// objects may still exist at this point and may access functions/symbols
// from pulseaudio
//   ecore_audio_pulse_lib_unload();
#endif /* HAVE_PULSE */

   /* FIXME: Shutdown all the inputs and outputs first */
   eina_log_timing(_ecore_audio_log_dom,
		   EINA_LOG_STATE_START,
		   EINA_LOG_STATE_SHUTDOWN);


   eina_list_free(ecore_audio_modules);

   eina_log_domain_unregister(_ecore_audio_log_dom);
   _ecore_audio_log_dom = -1;

   efl_object_shutdown();
   ecore_shutdown();

   return _ecore_audio_init_count;
}

#ifdef HAVE_PULSE
/**
 * @internal
 * @brief Dynamically loads the PulseAudio library and resolves its symbols.
 *
 * If PulseAudio support is compiled in, this function attempts to load the
 * PulseAudio shared library (e.g., libpulse.so.0, libpulse-0.dll) and
 * retrieve pointers to necessary functions. These pointers are stored in
 * the global `ecore_audio_pulse_lib` structure.
 *
 * @return @c EINA_TRUE if the library was successfully loaded (or already loaded)
 *         and all required symbols were found, @c EINA_FALSE otherwise.
 */
Eina_Bool
ecore_audio_pulse_lib_load(void)
{
   if (ecore_audio_pulse_lib)
     {
        if (!ecore_audio_pulse_lib->mod) return EINA_FALSE;
        return EINA_TRUE;
     }

   ecore_audio_pulse_lib = calloc(1, sizeof(Ecore_Audio_Lib_Pulse));
   if (!ecore_audio_pulse_lib) return EINA_FALSE;
# define LOAD(x)                                               \
   if (!ecore_audio_pulse_lib->mod) {                          \
      if ((ecore_audio_pulse_lib->mod = eina_module_new(x))) { \
         if (!eina_module_load(ecore_audio_pulse_lib->mod)) {  \
            eina_module_free(ecore_audio_pulse_lib->mod);      \
            ecore_audio_pulse_lib->mod = NULL;                 \
         }                                                     \
      }                                                        \
   }
# if defined(_WIN32) || defined(__CYGWIN__)
   LOAD("libpulse-0.dll");
   LOAD("libpulse.dll");
   LOAD("pulse.dll");
   if (!ecore_audio_pulse_lib->mod)
     ERR("Could not find libpulse-0.dll, libpulse.dll, pulse.dll");
# elif defined(__APPLE__) && defined(__MACH__)
   LOAD("libpulse.0.dylib");
   LOAD("libpulse.0.so");
   LOAD("libpulse.so.0");
   if (!ecore_audio_pulse_lib->mod)
     ERR("Could not find libpulse.0.dylib, libpulse.0.so, libpulse.so.0");
# else
   LOAD("libpulse.so.0");
   if (!ecore_audio_pulse_lib->mod)
     ERR("Could not find libpulse.so.0");
# endif
# undef LOAD
   if (!ecore_audio_pulse_lib->mod) return EINA_FALSE;

#define SYM(x) \
   if (!(ecore_audio_pulse_lib->x = eina_module_symbol_get(ecore_audio_pulse_lib->mod, #x))) { \
      ERR("Cannot find symbol '%s' in'%s", #x, eina_module_file_get(ecore_audio_pulse_lib->mod)); \
      goto err; \
   }
   SYM(pa_context_new);
   SYM(pa_context_unref);
   SYM(pa_context_connect);
   SYM(pa_context_set_sink_input_volume);
   SYM(pa_context_get_state);
   SYM(pa_context_set_state_callback);
   SYM(pa_operation_unref);
   SYM(pa_cvolume_set);
   SYM(pa_stream_new);
   SYM(pa_stream_unref);
   SYM(pa_stream_connect_playback);
   SYM(pa_stream_disconnect);
   SYM(pa_stream_drain);
   SYM(pa_stream_flush);
   SYM(pa_stream_cork);
   SYM(pa_stream_write);
   SYM(pa_stream_begin_write);
   SYM(pa_stream_set_write_callback);
   SYM(pa_stream_trigger);
   SYM(pa_stream_update_sample_rate);
   SYM(pa_stream_get_index);
#undef SYM
   return EINA_TRUE;
err:
   if (ecore_audio_pulse_lib->mod)
     {
        eina_module_free(ecore_audio_pulse_lib->mod);
        ecore_audio_pulse_lib->mod = NULL;
        ERR("Cannot find libpulse at runtime!");
     }
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Unloads the PulseAudio library and frees associated resources.
 *
 * This function releases the dynamically loaded PulseAudio library and
 * frees the memory allocated for the `ecore_audio_pulse_lib` structure.
 * @note This function is currently not called during `ecore_audio_shutdown`
 *       to prevent potential issues with lingering objects.
 */
void
ecore_audio_pulse_lib_unload(void)
{
   if (ecore_audio_pulse_lib)
     {
        if (ecore_audio_pulse_lib->mod)
          eina_module_free(ecore_audio_pulse_lib->mod);
        free(ecore_audio_pulse_lib);
        ecore_audio_pulse_lib = NULL;
     }
}
#endif /* HAVE_PULSE */

#ifdef HAVE_SNDFILE
/**
 * @internal
 * @brief Dynamically loads the Sndfile library and resolves its symbols.
 *
 * If Sndfile support is compiled in, this function attempts to load the
 * Sndfile shared library (e.g., libsndfile.so.1, libsndfile-1.dll) and
 * retrieve pointers to necessary functions. These pointers are stored in
 * the global `ecore_audio_sndfile_lib` structure.
 *
 * @return @c EINA_TRUE if the library was successfully loaded (or already loaded)
 *         and all required symbols were found, @c EINA_FALSE otherwise.
 */
Eina_Bool
ecore_audio_sndfile_lib_load(void)
{
   if (ecore_audio_sndfile_lib)
     {
        if (!ecore_audio_sndfile_lib->mod) return EINA_FALSE;
        return EINA_TRUE;
     }

   ecore_audio_sndfile_lib = calloc(1, sizeof(Ecore_Audio_Lib_Sndfile));
   if (!ecore_audio_sndfile_lib) return EINA_FALSE;
# define LOAD(x)                                                 \
   if (!ecore_audio_sndfile_lib->mod) {                          \
      if ((ecore_audio_sndfile_lib->mod = eina_module_new(x))) { \
         if (!eina_module_load(ecore_audio_sndfile_lib->mod)) {  \
            eina_module_free(ecore_audio_sndfile_lib->mod);      \
            ecore_audio_sndfile_lib->mod = NULL;                 \
         }                                                       \
      }                                                          \
   }
# if defined(_WIN32) || defined(__CYGWIN__)
   LOAD("libsndfile-1.dll");
   LOAD("libsndfile.dll");
   LOAD("sndfile.dll");
# elif defined(__APPLE__) && defined(__MACH__)
   LOAD("libsndfile.1.dylib");
   LOAD("libsndfile.1.so");
   LOAD("libsndfile.so.1");
# else
   LOAD("libsndfile.so.1");
# endif
# undef LOAD
   if (!ecore_audio_sndfile_lib->mod) return EINA_FALSE;

#define SYM(x) \
   if (!(ecore_audio_sndfile_lib->x = eina_module_symbol_get(ecore_audio_sndfile_lib->mod, #x))) { \
      ERR("Cannot find symbol '%s' in'%s", #x, eina_module_file_get(ecore_audio_sndfile_lib->mod)); \
      goto err; \
   }
   SYM(sf_open);
   SYM(sf_open_virtual);
   SYM(sf_close);
   SYM(sf_read_float);
   SYM(sf_write_float);
   SYM(sf_write_sync);
   SYM(sf_seek);
   SYM(sf_strerror);
#undef SYM
   return EINA_TRUE;
err:
   if (ecore_audio_sndfile_lib->mod)
     {
        eina_module_free(ecore_audio_sndfile_lib->mod);
        ecore_audio_sndfile_lib->mod = NULL;
        ERR("Cannot find libsndfile at runtime!");
     }
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Unloads the Sndfile library and frees associated resources.
 *
 * This function releases the dynamically loaded Sndfile library and
 * frees the memory allocated for the `ecore_audio_sndfile_lib` structure.
 * @note This function is currently not called during `ecore_audio_shutdown`
 *       to prevent potential issues with lingering objects.
 */
void
ecore_audio_sndfile_lib_unload(void)
{
   if (ecore_audio_sndfile_lib)
     {
        if (ecore_audio_sndfile_lib->mod)
          eina_module_free(ecore_audio_sndfile_lib->mod);
        free(ecore_audio_sndfile_lib);
        ecore_audio_sndfile_lib = NULL;
     }
}
#endif /* HAVE_SNDFILE */

/**
 * @brief Gets the name of an Ecore_Audio object.
 *
 * This function retrieves the name associated with an Efl_Object, which
 * is the base type for Ecore_Audio objects like inputs and outputs.
 *
 * @param obj The Efl_Object whose name is to be retrieved.
 * @return The name of the object, or @c NULL if no name is set or on error.
 *         The returned string is an interned string or owned by the object,
 *         and should not be modified or freed by the caller.
 *
 * @see ecore_audio_obj_name_set()
 */
ECORE_AUDIO_API const char*
ecore_audio_obj_name_get(const Efl_Object* obj)
{
   return efl_name_get(obj);
}

/**
 * @brief Sets the name of an Ecore_Audio object.
 *
 * This function assigns a name to an Efl_Object. This name can be used
 * for identification or debugging purposes.
 *
 * @param obj The Efl_Object whose name is to be set.
 * @param name The name to set for the object. The string is copied.
 *
 * @see ecore_audio_obj_name_get()
 */
ECORE_AUDIO_API void
ecore_audio_obj_name_set(Efl_Object* obj, const char *name)
{
   efl_name_set(obj, name);
}


/**
 * @}
 */
