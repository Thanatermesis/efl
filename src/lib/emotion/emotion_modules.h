/**
 * @file
 * @brief Emotion module internal C أبي.
 *
 * This header defines the structures and function prototypes used internally
 * by Emotion to interact with its various playback engine modules.
 * It includes the definition of the Emotion_Engine API structure that
 * each module must implement.
 */
#ifndef _EMOTION_MODULE_H_
#define _EMOTION_MODULE_H_ 1

#include "Emotion.h"

#include <emotion_api.h>

/** @name Metadata Keys
 *  @{
 */
#define META_TRACK_TITLE 1    /**< Metadata key for track title. */
#define META_TRACK_ARTIST 2   /**< Metadata key for track artist. */
#define META_TRACK_GENRE 3    /**< Metadata key for track genre. */
#define META_TRACK_COMMENT 4  /**< Metadata key for track comment. */
#define META_TRACK_ALBUM 5    /**< Metadata key for track album. */
#define META_TRACK_YEAR 6     /**< Metadata key for track year. */
#define META_TRACK_DISCID 7   /**< Metadata key for disc ID. */
#define META_TRACK_COUNT 8    /**< Metadata key for track count in album. */
/** @} */

typedef enum _Emotion_Format Emotion_Format;             /**< Typedef for enum _Emotion_Format. */
typedef struct _Emotion_Engine Emotion_Engine;           /**< Typedef for struct _Emotion_Engine. */
typedef struct _Emotion_Module_Options Emotion_Module_Options; /**< Typedef for struct _Emotion_Module_Options. */
typedef struct _Eina_Emotion_Plugins Eina_Emotion_Plugins; /**< Typedef for struct _Eina_Emotion_Plugins (definition likely elsewhere). */

/**
 * @brief Enumerates pixel formats used by Emotion.
 */
enum _Emotion_Format
{
   EMOTION_FORMAT_NONE, /**< No specific format / unknown. */
   EMOTION_FORMAT_I420, /**< YUV 4:2:0 planar format. */
   EMOTION_FORMAT_YV12, /**< YVU 4:2:0 planar format (like I420 but V and U planes swapped). */
   EMOTION_FORMAT_YUY2, /**< YUV 4:2:2 packed format. Currently unused as Evas doesn't support it directly. */
   EMOTION_FORMAT_BGRA  /**< 32-bit BGRA pixel format. */
};

/**
 * @brief Options for initializing an Emotion module instance.
 *
 * These options can be passed when creating a new engine instance to
 * hint at desired behavior (e.g., disabling video or audio processing).
 */
struct _Emotion_Module_Options
{
   Eina_Bool no_video : 1; /**< If EINA_TRUE, request the engine to disable video processing. */
   Eina_Bool no_audio : 1; /**< If EINA_TRUE, request the engine to disable audio processing. */
};

/**
 * @brief Defines the API structure for an Emotion playback engine.
 *
 * Each Emotion module (e.g., GStreamer, VLC backend) must provide an
 * implementation of this structure. It contains function pointers for all
 * operations Emotion can perform on a media object.
 */
struct _Emotion_Engine
{
/**
 * @def EMOTION_ENGINE_API_VERSION
 * @brief The API version that modules must match.
 *
 * This is used to ensure compatibility between Emotion core and its modules.
 * If a module's API version doesn't match this, it won't be loaded.
 */
#define EMOTION_ENGINE_API_VERSION (1U)
   unsigned       version; /**< API version implemented by the module. Must match EMOTION_ENGINE_API_VERSION. */

/**
 * @def EMOTION_ENGINE_PRIORITY_DEFAULT
 * @brief Default priority for an engine if not otherwise specified.
 *
 * Engines with higher priority values are preferred when Emotion selects
 * an engine automatically. This can be overridden by user configuration.
 * Typical range is 0-100.
 */
#define EMOTION_ENGINE_PRIORITY_DEFAULT (50)
   int            priority; /**< Default priority of this engine. Higher is better. */

   const char    *name;   /**< Unique name of the engine (e.g., "gstreamer1"). */
   /**
    * @brief Creates a new instance of this engine.
    * @param api Pointer to this engine's API structure.
    * @param obj The Evas_Object this instance will be associated with.
    * @param opts Module options for this instance.
    * @return A pointer to engine-specific instance data, or NULL on failure.
    */
   void          *(*add)(const Emotion_Engine *api, Evas_Object *obj, const Emotion_Module_Options *opts);
   /**
    * @brief Deletes an engine instance.
    * @param ef Engine-specific instance data (returned by `add`).
    */
   void           (*del)(void *ef);

   /** @brief Opens a media file. @param ef Instance data. @param file Path to file. @return EINA_TRUE on success. */
   Eina_Bool      (*file_open) (void *ef, const char *file);
   /** @brief Closes the current media file. @param ef Instance data. */
   void           (*file_close) (void *ef);
   /** @brief Starts or resumes playback. @param ef Instance data. @param pos Start position in seconds. */
   void           (*play) (void *ef, double pos);
   /** @brief Stops playback. @param ef Instance data. */
   void           (*stop) (void *ef);
   /** @brief Gets video dimensions. @param ef Instance data. @param[out] w Width. @param[out] h Height. */
   void           (*size_get) (void *ef, int *w, int *h);
   /** @brief Sets playback position. @param ef Instance data. @param pos Position in seconds. */
   void           (*pos_set) (void *ef, double pos);
   /** @brief Gets media length. @param ef Instance data. @return Length in seconds. */
   double         (*len_get) (void *ef);
   /** @brief Gets buffer fill status. @param ef Instance data. @return Buffer size (0.0-1.0). */
   double         (*buffer_size_get) (void *ef);
   /** @brief Gets FPS numerator. @param ef Instance data. @return FPS numerator. */
   int            (*fps_num_get) (void *ef);
   /** @brief Gets FPS denominator. @param ef Instance data. @return FPS denominator. */
   int            (*fps_den_get) (void *ef);
   /** @brief Gets FPS. @param ef Instance data. @return FPS as double. */
   double         (*fps_get) (void *ef);
   /** @brief Gets current playback position. @param ef Instance data. @return Position in seconds. */
   double         (*pos_get) (void *ef);
   /** @brief Sets visualization type. @param ef Instance data. @param vis Visualization type. */
   void           (*vis_set) (void *ef, Emotion_Vis vis);
   /** @brief Gets current visualization. @param ef Instance data. @return Current Emotion_Vis. */
   Emotion_Vis    (*vis_get) (void *ef);
   /** @brief Checks if visualization is supported. @param ef Instance data. @param vis Visualization to check. @return EINA_TRUE if supported. */
   Eina_Bool      (*vis_supported) (void *ef, Emotion_Vis vis);
   /** @brief Gets video aspect ratio. @param ef Instance data. @return Aspect ratio. */
   double         (*ratio_get) (void *ef);
   /** @brief Checks if video is handled. @param ef Instance data. @return Non-zero if video is handled. */
   int            (*video_handled) (void *ef);
   /** @brief Checks if audio is handled. @param ef Instance data. @return Non-zero if audio is handled. */
   int            (*audio_handled) (void *ef);
   /** @brief Checks if media is seekable. @param ef Instance data. @return Non-zero if seekable. */
   int            (*seekable) (void *ef);
   /** @brief Called when a video frame is done processing/displaying. @param ef Instance data. */
   void           (*frame_done) (void *ef);
   /** @brief Gets video pixel format. @param ef Instance data. @return Emotion_Format. */
   Emotion_Format (*format_get) (void *ef);
   /** @brief Gets raw video data dimensions. @param ef Instance data. @param[out] w Width. @param[out] h Height. */
   void           (*video_data_size_get) (void *ef, int *w, int *h);
   /** @brief Gets YUV plane data. @param ef Instance data. @param w Width. @param h Height. @param[out] yrows Y plane. @param[out] urows U plane. @param[out] vrows V plane. @return Non-zero on success. */
   int            (*yuv_rows_get) (void *ef, int w, int h, unsigned char **yrows, unsigned char **urows, unsigned char **vrows);
   /** @brief Gets BGRA pixel data. @param ef Instance data. @param[out] bgra_data Pointer to BGRA data. @return Non-zero on success. */
   int            (*bgra_data_get) (void *ef, unsigned char **bgra_data);
   /** @brief Feeds a generic event to the engine. @param ef Instance data. @param event Event code. */
   void           (*event_feed) (void *ef, int event);
   /** @brief Feeds mouse button event. @param ef Instance data. @param button Button number. @param x X-coord. @param y Y-coord. */
   void           (*event_mouse_button_feed) (void *ef, int button, int x, int y);
   /** @brief Feeds mouse move event. @param ef Instance data. @param x X-coord. @param y Y-coord. */
   void           (*event_mouse_move_feed) (void *ef, int x, int y);
   /** @brief Gets video channel count. @param ef Instance data. @return Number of video channels. */
   int            (*video_channel_count) (void *ef);
   /** @brief Sets current video channel. @param ef Instance data. @param channel Channel index. */
   void           (*video_channel_set) (void *ef, int channel);
   /** @brief Gets current video channel. @param ef Instance data. @return Current video channel index. */
   int            (*video_channel_get) (void *ef);
   /** @brief Sets subtitle file. @param ef Instance data. @param filepath Path to subtitle file. */
   void           (*video_subtitle_file_set) (void *ef, const char *filepath);
   /** @brief Gets subtitle file. @param ef Instance data. @return Path to subtitle file, or NULL. */
   const char *   (*video_subtitle_file_get) (void *ef);
   /** @brief Gets name of a video channel. @param ef Instance data. @param channel Channel index. @return Channel name, or NULL. */
   const char *   (*video_channel_name_get) (void *ef, int channel);
   /** @brief Sets video channel mute state. @param ef Instance data. @param mute Mute state (0 or 1). */
   void           (*video_channel_mute_set) (void *ef, int mute);
   /** @brief Gets video channel mute state. @param ef Instance data. @return Mute state (0 or 1). */
   int            (*video_channel_mute_get) (void *ef);
   /** @brief Gets audio channel count. @param ef Instance data. @return Number of audio channels. */
   int            (*audio_channel_count) (void *ef);
   /** @brief Sets current audio channel. @param ef Instance data. @param channel Channel index. */
   void           (*audio_channel_set) (void *ef, int channel);
   /** @brief Gets current audio channel. @param ef Instance data. @return Current audio channel index. */
   int            (*audio_channel_get) (void *ef);
   /** @brief Gets name of an audio channel. @param ef Instance data. @param channel Channel index. @return Channel name, or NULL. */
   const char *   (*audio_channel_name_get) (void *ef, int channel);
   /** @brief Sets audio channel mute state. @param ef Instance data. @param mute Mute state (0 or 1). */
   void           (*audio_channel_mute_set) (void *ef, int mute);
   /** @brief Gets audio channel mute state. @param ef Instance data. @return Mute state (0 or 1). */
   int            (*audio_channel_mute_get) (void *ef);
   /** @brief Sets audio channel volume. @param ef Instance data. @param vol Volume (0.0-1.0). */
   void           (*audio_channel_volume_set) (void *ef, double vol);
   /** @brief Gets audio channel volume. @param ef Instance data. @return Volume (0.0-1.0). */
   double         (*audio_channel_volume_get) (void *ef);
   /** @brief Gets SPU (subtitle/overlay) channel count. @param ef Instance data. @return Number of SPU channels. */
   int            (*spu_channel_count) (void *ef);
   /** @brief Sets current SPU channel. @param ef Instance data. @param channel Channel index. */
   void           (*spu_channel_set) (void *ef, int channel);
   /** @brief Gets current SPU channel. @param ef Instance data. @return Current SPU channel index. */
   int            (*spu_channel_get) (void *ef);
   /** @brief Gets name of an SPU channel. @param ef Instance data. @param channel Channel index. @return Channel name, or NULL. */
   const char *   (*spu_channel_name_get) (void *ef, int channel);
   /** @brief Sets SPU channel mute state. @param ef Instance data. @param mute Mute state (0 or 1). */
   void           (*spu_channel_mute_set) (void *ef, int mute);
   /** @brief Gets SPU channel mute state. @param ef Instance data. @return Mute state (0 or 1). */
   int            (*spu_channel_mute_get) (void *ef);
   /** @brief Gets chapter count. @param ef Instance data. @return Number of chapters. */
   int            (*chapter_count) (void *ef);
   /** @brief Sets current chapter. @param ef Instance data. @param chapter Chapter index. */
   void           (*chapter_set) (void *ef, int chapter);
   /** @brief Gets current chapter. @param ef Instance data. @return Current chapter index. */
   int            (*chapter_get) (void *ef);
   /** @brief Gets name of a chapter. @param ef Instance data. @param chapter Chapter index. @return Chapter name, or NULL. */
   const char *   (*chapter_name_get) (void *ef, int chapter);
   /** @brief Sets playback speed. @param ef Instance data. @param speed Playback speed (1.0 is normal). */
   void           (*speed_set) (void *ef, double speed);
   /** @brief Gets playback speed. @param ef Instance data. @return Playback speed. */
   double         (*speed_get) (void *ef);
   /** @brief Ejects media (e.g., DVD). @param ef Instance data. @return Non-zero on success. */
   int            (*eject) (void *ef);
   /** @brief Gets metadata. @param ef Instance data. @param meta Metadata key (META_TRACK_*). @return Metadata string, or NULL. Caller does not free. */
   const char *   (*meta_get) (void *ef, int meta);
   /** @brief Sets engine priority. @param ef Instance data. @param priority EINA_TRUE for high priority. */
   void           (*priority_set) (void *ef, Eina_Bool priority);
   /** @brief Gets engine priority. @param ef Instance data. @return EINA_TRUE if high priority. */
   Eina_Bool      (*priority_get) (void *ef);
   /**
    * @brief Retrieves artwork associated with the media.
    * @param ef Engine-specific instance data.
    * @param img An Evas_Object (image) to load the artwork into.
    * @param path A path or identifier for the artwork (engine-specific).
    * @param type The type of artwork requested (e.g., album cover).
    * @return Engine-specific data related to the artwork operation, or NULL on failure.
    */
   void       *   (*meta_artwork_get)(void *ef, Evas_Object *img, const char *path, Emotion_Artwork_Info type);
};

/** @brief Retrieves the internal video data associated with an Evas_Object. For internal use. */
EMOTION_API void *_emotion_video_get(const Evas_Object *obj);
/** @brief Signals that a new video frame is available. For internal use by engines. */
EMOTION_API void  _emotion_frame_new(Evas_Object *obj);
/** @brief Updates video position and length. For internal use by engines. */
EMOTION_API void  _emotion_video_pos_update(Evas_Object *obj, double pos, double len);
/** @brief Signals a video frame resize. For internal use by engines. */
EMOTION_API void  _emotion_frame_resize(Evas_Object *obj, int w, int h, double ratio);
/** @brief Signals a video frame refill. For internal use by engines. */
EMOTION_API void  _emotion_frame_refill(Evas_Object *obj, double w, double h);
/** @brief Signals that decoding should stop. For internal use by engines. */
EMOTION_API void  _emotion_decode_stop(Evas_Object *obj);
/** @brief Signals that file opening is complete. For internal use by engines. */
EMOTION_API void  _emotion_open_done(Evas_Object *obj);
/** @brief Signals that playback has started. For internal use by engines. */
EMOTION_API void  _emotion_playback_started(Evas_Object *obj);
/** @brief Signals that playback has finished. For internal use by engines. */
EMOTION_API void  _emotion_playback_finished(Evas_Object *obj);
/** @brief Signals an audio level change. For internal use by engines. */
EMOTION_API void  _emotion_audio_level_change(Evas_Object *obj);
/** @brief Signals a change in available channels (audio/video/spu). For internal use by engines. */
EMOTION_API void  _emotion_channels_change(Evas_Object *obj);
/** @brief Sets the title of the media. For internal use by engines. */
EMOTION_API void  _emotion_title_set(Evas_Object *obj, char *title);
/** @brief Sets progress information. For internal use by engines. */
EMOTION_API void  _emotion_progress_set(Evas_Object *obj, char *info, double stat);
/** @brief Sets file reference information. For internal use by engines. */
EMOTION_API void  _emotion_file_ref_set(Evas_Object *obj, const char *file, int num);
/** @brief Sets the number of SPU buttons. For internal use by engines. */
EMOTION_API void  _emotion_spu_button_num_set(Evas_Object *obj, int num);
/** @brief Sets the current SPU button. For internal use by engines. */
EMOTION_API void  _emotion_spu_button_set(Evas_Object *obj, int button);
/** @brief Signals that a seek operation is complete. For internal use by engines. */
EMOTION_API void  _emotion_seek_done(Evas_Object *obj);
/** @brief Resets the image data of the object. For internal use. */
EMOTION_API void  _emotion_image_reset(Evas_Object *obj);

/** @brief Increments pending object reference count. For internal use. */
EMOTION_API void _emotion_pending_object_ref(void);
/** @brief Decrements pending object reference count. For internal use. */
EMOTION_API void _emotion_pending_object_unref(void);

/** @brief Marks beginning of a pending ecore operation. For internal use. */
EMOTION_API void _emotion_pending_ecore_begin(void);
/** @brief Marks end of a pending ecore operation. For internal use. */
EMOTION_API void _emotion_pending_ecore_end(void);

/** @brief Gets custom webcam device string. For internal use. */
EMOTION_API const char *emotion_webcam_custom_get(const char *device);

/**
 * @brief Registers an Emotion engine module.
 * @param api Pointer to the Emotion_Engine API structure.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see _emotion_module_register() in emotion_modules.c
 */
EMOTION_API Eina_Bool _emotion_module_register(const Emotion_Engine *api);

/**
 * @brief Unregisters an Emotion engine module.
 * @param api Pointer to the Emotion_Engine API structure.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see _emotion_module_unregister() in emotion_modules.c
 */
EMOTION_API Eina_Bool _emotion_module_unregister(const Emotion_Engine *api);

#endif
