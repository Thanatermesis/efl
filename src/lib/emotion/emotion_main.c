#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#include <stdio.h>

#include <Ecore.h>

#include "emotion_private.h"

/** @internal
 * @brief Stores the library version information.
 */
static Emotion_Version _version = { VMAJ, VMIN, VMIC, VREV };
/** @internal
 * @brief Counter for pending Emotion objects.
 * Used to track active objects to ensure proper shutdown.
 */
static int emotion_pending_objects = 0;
/** @internal
 * @brief Lock for synchronizing access to pending object and event counters.
 */
static Eina_Lock emotion_pending_lock;
/**
 * @brief Pointer to the Emotion library version information.
 *
 * This variable provides runtime access to the version of the Emotion
 * library being used. The version is defined by VMAJ, VMIN, VMIC, VREV
 * macros.
 */
EMOTION_API Emotion_Version *emotion_version = &_version;

/** @internal
 * @brief Prefix for Emotion library paths.
 * Used to locate Emotion-specific resources like modules and configuration files.
 */
Eina_Prefix *_emotion_pfx = NULL;
/** @internal
 * @brief Log domain for Emotion library messages.
 */
int _emotion_log_domain = -1;

/** @internal
 * @brief Eet file handle for the Emotion configuration.
 */
static Eet_File *_emotion_config_file = NULL;

/** @internal
 * @brief Structure to hold a file extension and its length.
 * Used for quick matching of playable file types.
 */
struct ext_match_s
{
   unsigned int length;
   const char *extension;
};

#define MATCHING(Ext)                           \
  { sizeof (Ext), Ext }

static const struct ext_match_s matchs[] =
{ /* map extensions to know if it's a emotion playable content for good first-guess tries */
   MATCHING(".264"),
   MATCHING(".3g2"),
   MATCHING(".3gp"),
   MATCHING(".3gp2"),
   MATCHING(".3gpp"),
   MATCHING(".3gpp2"),
   MATCHING(".3p2"),
   MATCHING(".aac"),
   MATCHING(".asf"),
   MATCHING(".avi"),
   MATCHING(".bdm"),
   MATCHING(".bdmv"),
   MATCHING(".clpi"),
   MATCHING(".clp"),
   MATCHING(".fla"),
   MATCHING(".flac"),
   MATCHING(".flv"),
   MATCHING(".m1v"),
   MATCHING(".m2v"),
   MATCHING(".m2t"),
   MATCHING(".m4a"),
   MATCHING(".m4v"),
   MATCHING(".mkv"),
   MATCHING(".mov"),
   MATCHING(".mp2"),
   MATCHING(".mp2ts"),
   MATCHING(".mp3"),
   MATCHING(".mp4"),
   MATCHING(".mpe"),
   MATCHING(".mpeg"),
   MATCHING(".mpg"),
   MATCHING(".mpl"),
   MATCHING(".mpls"),
   MATCHING(".mts"),
   MATCHING(".mxf"),
   MATCHING(".nut"),
   MATCHING(".nuv"),
   MATCHING(".ogg"),
   MATCHING(".ogm"),
   MATCHING(".ogv"),
   MATCHING(".opus"),
   MATCHING(".rm"),
   MATCHING(".rmj"),
   MATCHING(".rmm"),
   MATCHING(".rms"),
   MATCHING(".rmx"),
   MATCHING(".rmvb"),
   MATCHING(".swf"),
   MATCHING(".ts"),
   MATCHING(".wav"),
   MATCHING(".weba"),
   MATCHING(".webm"),
   MATCHING(".wma"),
   MATCHING(".wmv")
};

/**
 * @internal
 * @brief Generic function to check if a file extension is likely playable by Emotion.
 *
 * This function compares the extension of the given file path against a predefined
 * list of known playable extensions. It's a "first-guess" mechanism.
 *
 * @param data Unused data pointer, kept for compatibility or future use.
 * @param file The full path or filename to check.
 * @return @c EINA_TRUE if the extension matches a known playable type,
 *         @c EINA_FALSE otherwise or if the file string is too short.
 */
Eina_Bool
_emotion_object_extension_can_play_generic_get(const void *data EINA_UNUSED, const char *file)
{
   unsigned int length;
   unsigned int i;

   length = eina_stringshare_strlen(file) + 1;
   if (length < 5) return EINA_FALSE;

   for (i = 0; i < sizeof (matchs) / sizeof (struct ext_match_s); ++i)
     {
        if (matchs[i].length > length) continue;

        if (!strcasecmp(matchs[i].extension,
                        file + length - matchs[i].length))
          return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Quickly checks if a file extension suggests it might be playable by Emotion.
 *
 * This function performs a quick check based on the file extension against a
 * predefined list of common multimedia extensions. It does not involve
 * eina_stringshare operations, making it faster for immediate checks where
 * the filename string's lifetime is managed externally or is a literal.
 *
 * @param file The filename or path to check. Must not be NULL.
 * @return @c EINA_TRUE if the file extension is in the known list,
 *         @c EINA_FALSE otherwise.
 * @see emotion_object_extension_may_play_get()
 */
EMOTION_API Eina_Bool
emotion_object_extension_may_play_fast_get(const char *file)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(file, EINA_FALSE);
   return _emotion_object_extension_can_play_generic_get(NULL, file);
}

/**
 * @brief Checks if a file extension suggests it might be playable by Emotion.
 *
 * This function checks the file extension against a predefined list of common
 * multimedia extensions. It uses eina_stringshare for managing the filename
 * string, which might involve a slight overhead compared to
 * emotion_object_extension_may_play_fast_get().
 *
 * @param file The filename or path to check. Must not be NULL.
 * @return @c EINA_TRUE if the file extension is in the known list,
 *         @c EINA_FALSE otherwise.
 * @see emotion_object_extension_may_play_fast_get()
 */
EMOTION_API Eina_Bool
emotion_object_extension_may_play_get(const char *file)
{
   const char *tmp;
   Eina_Bool result;

   EINA_SAFETY_ON_NULL_RETURN_VAL(file, EINA_FALSE);
   tmp = eina_stringshare_add(file);
   result = emotion_object_extension_may_play_fast_get(tmp);
   eina_stringshare_del(tmp);

   return result;
}

/** @internal
 * @brief Initialization counter for the Emotion library.
 * Ensures that init/shutdown calls are balanced.
 */
static int _emotion_init_count = 0;

/**
 * @brief Initializes the Emotion library.
 *
 * This function sets up Emotion and its dependencies, including Eina, Ecore,
 * and Eet. It registers a log domain, sets up path prefixes, initializes
 * webcam support, and loads Emotion modules.
 *
 * This function increments an internal counter. emotion_shutdown() must be
 * called a corresponding number of times to fully shut down the library.
 *
 * @return @c EINA_TRUE on successful initialization, @c EINA_FALSE on failure.
 * @see emotion_shutdown()
 */
EMOTION_API Eina_Bool
emotion_init(void)
{
   char buffer[PATH_MAX];

   if (_emotion_init_count > 0)
     {
        _emotion_init_count++;
        return EINA_TRUE;
     }

   eina_init();

   _emotion_log_domain = eina_log_domain_register("emotion", EINA_COLOR_LIGHTCYAN);
   if (_emotion_log_domain < 0)
     {
        EINA_LOG_CRIT("Could not register log domain 'emotion'");
        eina_shutdown();
        return EINA_FALSE;
     }

   _emotion_pfx = eina_prefix_new(NULL, emotion_init,
                                  "EMOTION", "emotion", "checkme",
                                  PACKAGE_BIN_DIR, PACKAGE_LIB_DIR,
                                  PACKAGE_DATA_DIR, PACKAGE_DATA_DIR);
   EINA_SAFETY_ON_NULL_GOTO(_emotion_pfx, error);

   ecore_init();
   eet_init();

   snprintf(buffer, sizeof(buffer), "%s/emotion.cfg",
            eina_prefix_data_get(_emotion_pfx));
   _emotion_config_file = eet_open(buffer, EET_FILE_MODE_READ);

   if (!emotion_webcam_init()) goto error_webcam;

   if (!emotion_modules_init()) goto error_modules;

   _emotion_init_count = 1;
   return EINA_TRUE;

 error_modules:
   emotion_webcam_shutdown();

 error_webcam:
   eina_prefix_free(_emotion_pfx);
   _emotion_pfx = NULL;

 error:
   eina_log_domain_unregister(_emotion_log_domain);
   _emotion_log_domain = -1;

   eina_shutdown();
   return EINA_FALSE;
}

/** @internal
 * @brief Counter for pending Ecore events related to Emotion.
 * Used to ensure all events are processed before shutdown.
 */
static int emotion_pendig_events = 0;

/**
 * @internal
 * @brief Marks the beginning of an Ecore event processing sequence.
 *
 * This function increments a counter for pending Ecore events. It should be
 * called before an operation that might generate Ecore events that Emotion
 * needs to track for graceful shutdown.
 * Access to the counter is synchronized with emotion_pending_lock.
 *
 * @see _emotion_pending_ecore_end()
 */
EMOTION_API void
_emotion_pending_ecore_begin(void)
{
   eina_lock_take(&emotion_pending_lock);
   emotion_pendig_events++;
   eina_lock_release(&emotion_pending_lock);
}

/**
 * @internal
 * @brief Marks the end of an Ecore event processing sequence.
 *
 * This function decrements a counter for pending Ecore events. It should be
 * called after an operation that might generate Ecore events has completed.
 * Access to the counter is synchronized with emotion_pending_lock.
 *
 * @see _emotion_pending_ecore_begin()
 */
EMOTION_API void
_emotion_pending_ecore_end(void)
{
   eina_lock_take(&emotion_pending_lock);
   emotion_pendig_events--;
   eina_lock_release(&emotion_pending_lock);
}

/**
 * @brief Shuts down the Emotion library.
 *
 * This function cleans up resources used by Emotion. It decrements an
 * internal initialization counter. The library is fully shut down only when
 * this counter reaches zero.
 *
 * It waits for a short period for pending objects and events to complete
 * before proceeding with the shutdown of modules, webcam support, and
 * other subsystems like Eet, Ecore, and Eina.
 *
 * @return @c EINA_TRUE if shutdown is proceeding or complete,
 *         @c EINA_FALSE if the init count was already zero (indicating an error).
 * @see emotion_init()
 */
EMOTION_API Eina_Bool
emotion_shutdown(void)
{
   double start;

   if (_emotion_init_count <= 0)
     {
        ERR("Init count not greater than 0 in emotion shutdown.");
        return EINA_FALSE;
     }
   if (--_emotion_init_count) return EINA_TRUE;

   eina_lock_take(&emotion_pending_lock);
   start = ecore_time_get();
   while (((emotion_pending_objects > 0) ||
           (emotion_pendig_events > 0)) &&
          ((ecore_time_get() - start) < 0.5))
     {
        eina_lock_release(&emotion_pending_lock);
        ecore_main_loop_iterate();
        eina_lock_take(&emotion_pending_lock);
     }

   if (emotion_pending_objects > 0)
     {
        ERR("There is still %i Emotion pipeline running", emotion_pending_objects);
     }
   if (emotion_pendig_events > 0)
     {
        ERR("There is still %i Emotion events queued", emotion_pendig_events);
     }
   eina_lock_release(&emotion_pending_lock);

   eina_lock_free(&emotion_pending_lock);

   emotion_modules_shutdown();

   emotion_webcam_shutdown();

   if (_emotion_config_file)
     {
        /* As long as there is no one reference any pointer, you are safe */
        eet_close(_emotion_config_file);
        _emotion_config_file = NULL;
     }

   eet_shutdown();
   ecore_shutdown();

   eina_prefix_free(_emotion_pfx);
   _emotion_pfx = NULL;

   eina_log_domain_unregister(_emotion_log_domain);
   _emotion_log_domain = -1;

   eina_shutdown();

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Increments the count of pending Emotion objects.
 *
 * This function is called when an Emotion object (e.g., a media player instance)
 * is created or becomes active, to ensure the library doesn't shut down
 * prematurely while objects are still in use.
 * Access to the counter is synchronized with emotion_pending_lock.
 *
 * @see _emotion_pending_object_unref()
 */
EMOTION_API void
_emotion_pending_object_ref(void)
{
   eina_lock_take(&emotion_pending_lock);
   emotion_pending_objects++;
   eina_lock_release(&emotion_pending_lock);
}

/**
 * @internal
 * @brief Decrements the count of pending Emotion objects.
 *
 * This function is called when an Emotion object is destroyed or is no longer
 * active.
 * Access to the counter is synchronized with emotion_pending_lock.
 *
 * @see _emotion_pending_object_ref()
 */
EMOTION_API void
_emotion_pending_object_unref(void)
{
   eina_lock_take(&emotion_pending_lock);
   emotion_pending_objects--;
   eina_lock_release(&emotion_pending_lock);
}
