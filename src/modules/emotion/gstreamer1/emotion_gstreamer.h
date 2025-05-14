/**
 * @file emotion_gstreamer.h
 * @brief Header file for the Emotion GStreamer module.
 *
 * This file defines the structures, types, and functions used by the
 * GStreamer backend for the Emotion video player library. It handles
 * the integration of GStreamer pipelines with Evas objects for video
 * rendering and playback control.
 */
#ifndef __EMOTION_GSTREAMER_H__
#define __EMOTION_GSTREAMER_H__

#include <glib.h>
#include <gst/gst.h>
#include <glib-object.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>
#include <gst/video/navigation.h>
#include <gst/audio/audio.h>
#include <gst/tag/tag.h>
#include <gst/pbutils/pbutils.h>

#include <unistd.h>
#include <fcntl.h>

#include <Eina.h>
#include <Evas.h>
#include <Ecore.h>

#include "emotion_modules.h"

/**
 * @struct _Emotion_Convert_Info
 * @brief Holds information for video frame conversion.
 *
 * This structure contains details about the video planes, such as
 * bits per pixel (bpp), stride, and pointers to the plane data.
 * It is used by the Evas_Video_Convert_Cb callback.
 */
typedef struct _Emotion_Convert_Info Emotion_Convert_Info;

/**
 * @typedef Evas_Video_Convert_Cb
 * @brief Callback function type for converting GStreamer video frames to Evas format.
 *
 * @param evas_data Pointer to the destination buffer for Evas image data.
 * @param gst_data Pointer to the source GStreamer video frame data.
 * @param w Width of the video frame.
 * @param h Height of the video frame.
 * @param output_height The height of the output buffer, which might be different from h (e.g., for specific YUV formats).
 * @param info Pointer to an Emotion_Convert_Info structure containing plane details.
 */
typedef void (*Evas_Video_Convert_Cb)(unsigned char *evas_data,
                                      const unsigned char *gst_data,
                                      unsigned int w,
                                      unsigned int h,
                                      unsigned int output_height,
                                      Emotion_Convert_Info *info);

/**
 * @struct _EmotionVideoSinkPrivate
 * @brief Private data for the EmotionVideoSink GStreamer element.
 *
 * This structure holds private members for the custom GStreamer video sink,
 * including references to Evas objects, video format information,
 * synchronization primitives, and frame handling data.
 */
typedef struct _EmotionVideoSinkPrivate EmotionVideoSinkPrivate;
/**
 * @struct _EmotionVideoSink
 * @brief Custom GStreamer video sink element for Emotion.
 *
 * This structure represents the GStreamer video sink that integrates
 * with Evas for rendering video frames.
 */
typedef struct _EmotionVideoSink        EmotionVideoSink;
/**
 * @struct _EmotionVideoSinkClass
 * @brief Class structure for the EmotionVideoSink GStreamer element.
 *
 * This structure holds the class definition for the custom GStreamer
 * video sink.
 */
typedef struct _EmotionVideoSinkClass   EmotionVideoSinkClass;
/**
 * @struct _Emotion_Gstreamer
 * @brief Main structure for an Emotion GStreamer instance.
 *
 * This structure encapsulates all the data associated with a single
 * instance of an Emotion GStreamer player, including the GStreamer pipeline,
 * Evas object, playback state, and metadata.
 */
typedef struct _Emotion_Gstreamer Emotion_Gstreamer;
/**
 * @struct _Emotion_Gstreamer_Metadata
 * @brief Stores metadata extracted from a media stream.
 *
 * This structure holds common metadata fields like title, artist, album, etc.
 */
typedef struct _Emotion_Gstreamer_Metadata Emotion_Gstreamer_Metadata;
/**
 * @struct _Emotion_Gstreamer_Buffer
 * @brief Represents a GStreamer buffer to be processed or rendered.
 *
 * This structure is used to pass GStreamer buffer data along with
 * conversion information to the rendering parts of Emotion.
 */
typedef struct _Emotion_Gstreamer_Buffer Emotion_Gstreamer_Buffer;
/**
 * @struct _Emotion_Gstreamer_Message
 * @brief Wrapper for GStreamer messages to be handled by Emotion.
 *
 * This structure is used to queue GStreamer messages for processing
 * on the main loop.
 */
typedef struct _Emotion_Gstreamer_Message Emotion_Gstreamer_Message;

struct _Emotion_Convert_Info
{
   unsigned int bpp[4]; /**< Bits per pixel for each plane. Example: {8, 8, 8, 0} for YUV. */
   unsigned int stride[4]; /**< Stride (bytes per row) for each plane. Example: {width, width/2, width/2, 0}. */
   unsigned char *plane_ptr[4]; /**< Pointers to the start of each plane's data. */
};

struct _Emotion_Gstreamer_Metadata
{
   char *title; /**< Title of the media. */
   char *album; /**< Album name. */
   char *artist; /**< Artist name. */
   char *genre; /**< Genre of the media. */
   char *comment; /**< Comments associated with the media. */
   char *year; /**< Year of release or recording. */
   char *count; /**< Track number or count. */
   char *disc_id; /**< Disc identifier (e.g., for CDDB). */
};

struct _Emotion_Gstreamer
{
   const Emotion_Engine *api; /**< Pointer to the Emotion engine API. */

   volatile int     ref_count; /**< Reference count for the GStreamer instance. */

   const char       *subtitle; /**< Path to the subtitle file, if any. */
   /* Gstreamer elements */
   GstElement       *pipeline; /**< The main GStreamer pipeline element (playbin). */
   GstElement       *vsink;    /**< The custom Emotion video sink element. */

   Eina_List        *threads;  /**< List of Ecore_Thread instances used for asynchronous operations. */

   /* Evas object */
   Evas_Object      *obj;      /**< The Evas object associated with this Emotion instance. */

   gulong            audio_buffer_probe; /**< ID of the audio buffer probe, if active. */
   GstPad           *audio_buffer_probe_pad; /**< GStreamer pad where the audio buffer probe is attached. */
   gint              audio_buffer_probe_pending; /**< Flag to prevent queuing too many audio probe callbacks. */

   /* Characteristics of stream */
   double            position; /**< Current playback position in seconds. */
   double            volume;   /**< Current volume level (0.0 to 1.0+). */

   Emotion_Gstreamer_Metadata *metadata; /**< Pointer to the extracted media metadata. */

   Emotion_Vis       vis; /**< Selected audio visualization. */

   Eina_Bool         play         : 1; /**< EINA_TRUE if playback is active, EINA_FALSE otherwise. */
   Eina_Bool         video_mute   : 1; /**< EINA_TRUE if video is muted, EINA_FALSE otherwise. */
   Eina_Bool         audio_mute   : 1; /**< EINA_TRUE if audio is muted, EINA_FALSE otherwise. */
   Eina_Bool         spu_mute     : 1; /**< EINA_TRUE if SPU (subtitles) are muted, EINA_FALSE otherwise. */
   Eina_Bool         ready        : 1; /**< EINA_TRUE if the pipeline is ready for playback, EINA_FALSE otherwise. */
   Eina_Bool         live         : 1; /**< EINA_TRUE if the stream is a live stream, EINA_FALSE otherwise. */
   Eina_Bool         buffering    : 1; /**< EINA_TRUE if the stream is currently buffering, EINA_FALSE otherwise. */
   Eina_Bool         shutdown     : 1; /**< EINA_TRUE if the instance is shutting down, EINA_FALSE otherwise. */
};

struct _EmotionVideoSink {
    /*< private >*/
    GstVideoSink parent; /**< Parent GstVideoSink instance. */
    EmotionVideoSinkPrivate *priv; /**< Pointer to private data. */
};

struct _EmotionVideoSinkClass {
    /*< private >*/
    GstVideoSinkClass parent_class; /**< Parent GstVideoSinkClass. */
};

struct _EmotionVideoSinkPrivate {
   Evas_Object *emotion_object; /**< The main Emotion Evas object. */
   Evas_Object *evas_object;    /**< The Evas image object used for rendering video frames. */

   GstVideoInfo info;           /**< GStreamer video information (format, width, height, etc.). */
   unsigned int eheight;        /**< Height of the Evas image object. */
   Evas_Colorspace eformat;     /**< Evas colorspace format for the video. */
   Evas_Video_Convert_Cb func; /**< Conversion function for video frames. */

   Eina_Lock m;                /**< Mutex for thread synchronization. */
   Eina_Condition c;           /**< Condition variable for thread synchronization. */

   Emotion_Gstreamer_Buffer *send; /**< Buffer currently being sent/processed. */

    /* We need to keep a copy of the last inserted buffer as evas doesn't copy YUV data around */
   GstBuffer        *last_buffer; /**< The last GStreamer buffer received. */
   GstMapInfo        map_info;    /**< Mapping information for the last_buffer. */

   GstVideoFrame last_vframe;   /**< The last GStreamer video frame processed. */

   int frames;                  /**< Counter for rendered frames (used for FPS calculation). */
   int flapse;                  /**< Frame counter at the last FPS calculation point. */
   double rtime;                 /**< Real time at the last FPS calculation point. */
   double rlapse;                /**< Real time duration since the last FPS calculation. */

   // If this is TRUE all processing should finish ASAP
   // This is necessary because there could be a race between
   // unlock() and render(), where unlock() wins, signals the
   // GCond, then render() tries to render a frame although
   // everything else isn't running anymore. This will lead
   // to deadlocks because render() holds the stream lock.
   //
   // Protected by the buffer mutex
   Eina_Bool unlocked : 1;      /**< EINA_TRUE if the sink is unlocked and should stop processing. */
   Eina_Bool mapped : 1;        /**< EINA_TRUE if last_buffer is currently mapped. */
   Eina_Bool vfmapped : 1;      /**< EINA_TRUE if last_vframe is currently mapped. */
};

struct _Emotion_Gstreamer_Buffer
{
   GstVideoFrame vframe;      /**< GStreamer video frame. */
   EmotionVideoSink *sink;   /**< Pointer to the EmotionVideoSink instance. */
   GstBuffer *frame;         /**< GStreamer buffer containing the video frame data. */
   GstVideoInfo info;        /**< GStreamer video information for this buffer. */
   Evas_Video_Convert_Cb func; /**< Conversion function to use for this buffer. */
   Evas_Colorspace eformat;  /**< Target Evas colorspace for this buffer. */
   int eheight;              /**< Target height for Evas image. */
   Eina_Bool vfmapped : 1;   /**< EINA_TRUE if vframe is mapped. */
};

struct _Emotion_Gstreamer_Message
{
   Emotion_Gstreamer *ev; /**< Pointer to the Emotion_Gstreamer instance this message is for. */

   GstMessage *msg;       /**< The GStreamer message. */
};

extern int _emotion_gstreamer_log_domain; /**< Log domain for Emotion GStreamer module. */
extern Eina_Bool debug_fps; /**< Flag to enable FPS debugging output. */

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_emotion_gstreamer_log_domain, __VA_ARGS__)

#ifdef INF
#undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_emotion_gstreamer_log_domain, __VA_ARGS__)

#ifdef WRN
#undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_emotion_gstreamer_log_domain, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_emotion_gstreamer_log_domain, __VA_ARGS__)

#ifdef CRI
#undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_emotion_gstreamer_log_domain, __VA_ARGS__)

#define EMOTION_TYPE_VIDEO_SINK emotion_video_sink_get_type()

#define EMOTION_VIDEO_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    EMOTION_TYPE_VIDEO_SINK, EmotionVideoSink))

#define EMOTION_VIDEO_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), \
    EMOTION_TYPE_VIDEO_SINK, EmotionVideoSinkClass))

#define EMOTION_IS_VIDEO_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    EMOTION_TYPE_VIDEO_SINK))

#define EMOTION_IS_VIDEO_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), \
    EMOTION_TYPE_VIDEO_SINK))

#define EMOTION_VIDEO_SINK_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), \
    EMOTION_TYPE_VIDEO_SINK, EmotionVideoSinkClass))

/**
 * @brief Initializes the GStreamer plugin for Emotion.
 * @param plugin The GStreamer plugin object.
 * @return TRUE on success, FALSE on failure.
 *
 * This function is called by GStreamer to initialize the emotion-sink plugin.
 */
gboolean    gstreamer_plugin_init(GstPlugin *plugin);

/**
 * @brief Allocates an Emotion_Gstreamer_Buffer structure.
 * @param sink The EmotionVideoSink instance.
 * @param buffer The GStreamer buffer.
 * @param info GStreamer video information.
 * @param eformat Target Evas colorspace.
 * @param eheight Target Evas image height.
 * @param func Conversion function to use.
 * @return A newly allocated Emotion_Gstreamer_Buffer, or NULL on failure.
 */
Emotion_Gstreamer_Buffer *emotion_gstreamer_buffer_alloc(EmotionVideoSink *sink,
                                                         GstBuffer *buffer,
                                                         GstVideoInfo *info,
                                                         Evas_Colorspace eformat,
                                                         int eheight,
                                                         Evas_Video_Convert_Cb func);
/**
 * @brief Frees an Emotion_Gstreamer_Buffer structure.
 * @param send The buffer to free.
 */
void emotion_gstreamer_buffer_free(Emotion_Gstreamer_Buffer *send);

/**
 * @brief Allocates an Emotion_Gstreamer_Message structure.
 * @param ev The Emotion_Gstreamer instance.
 * @param msg The GStreamer message to wrap.
 * @return A newly allocated Emotion_Gstreamer_Message, or NULL on failure.
 */
Emotion_Gstreamer_Message *emotion_gstreamer_message_alloc(Emotion_Gstreamer *ev,
                                                           GstMessage *msg);
/**
 * @brief Frees an Emotion_Gstreamer_Message structure.
 * @param send The message wrapper to free.
 */
void emotion_gstreamer_message_free(Emotion_Gstreamer_Message *send);

/**
 * @brief Increments the reference count of an Emotion_Gstreamer instance.
 * @param ev The Emotion_Gstreamer instance.
 * @return The same Emotion_Gstreamer instance.
 */
Emotion_Gstreamer * emotion_gstreamer_ref (Emotion_Gstreamer *ev);

/**
 * @brief Decrements the reference count of an Emotion_Gstreamer instance.
 * If the reference count reaches zero, the instance is freed.
 * @param ev The Emotion_Gstreamer instance.
 */
void emotion_gstreamer_unref (Emotion_Gstreamer *ev);

/**
 * @struct _ColorSpace_Format_Conversion
 * @brief Defines a mapping between GStreamer video formats and Evas colorspaces.
 *
 * This structure is used to look up the appropriate Evas colorspace and
 * conversion function for a given GStreamer video format and color matrix.
 */
typedef struct _ColorSpace_Format_Conversion ColorSpace_Format_Conversion;

struct _ColorSpace_Format_Conversion
{
   const char *name; /**< Descriptive name for the conversion. */
   GstVideoFormat format; /**< GStreamer video format. */
   GstVideoColorMatrix colormatrix; /**< GStreamer color matrix. */
   Evas_Colorspace eformat; /**< Corresponding Evas colorspace. */
   Evas_Video_Convert_Cb func; /**< Conversion function for this format. */
   Eina_Bool force_height; /**< If EINA_TRUE, forces the output height to a specific value (e.g., for planar YUV). */
};

/**
 * @var colorspace_format_conversion
 * @brief Array of ColorSpace_Format_Conversion structures.
 *
 * This array defines the supported conversions from GStreamer video formats
 * to Evas colorspaces.
 * Example entry:
 * @code
 * { "I420", GST_VIDEO_FORMAT_I420, GST_VIDEO_COLOR_MATRIX_UNKNOWN, EVAS_COLORSPACE_YCBCR420P_PL, _evas_video_convert_i420, EINA_TRUE }
 * @endcode
 */
extern const ColorSpace_Format_Conversion colorspace_format_conversion[];

/* From gst-plugins-base/gst/playback */
/**
 * @enum GstPlayFlags
 * @brief Flags used by the GStreamer playbin element to control playback aspects.
 *
 * These flags are typically used with playbin's "flags" property.
 * Copied from gst-plugins-base/gst/playback/gstplay-enum.h for reference
 * as playbin uses these internally.
 */
typedef enum {
  GST_PLAY_FLAG_VIDEO         = (1 << 0), /**< Enable video playback. */
  GST_PLAY_FLAG_AUDIO         = (1 << 1), /**< Enable audio playback. */
  GST_PLAY_FLAG_TEXT          = (1 << 2), /**< Enable text subtitle playback. */
  GST_PLAY_FLAG_VIS           = (1 << 3), /**< Enable audio visualization when no video. */
  GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4), /**< Force software volume control. */
  GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5), /**< Output native audio. Only useful for A/V sync. */
  GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6), /**< Output native video. Only useful for A/V sync. */
  GST_PLAY_FLAG_DOWNLOAD      = (1 << 7), /**< Attempt to download stream if 'download' interface is available. */
  GST_PLAY_FLAG_BUFFERING     = (1 << 8), /**< Enable buffering. */
  GST_PLAY_FLAG_DEINTERLACE   = (1 << 9), /**< Force deinterlacing. */
  GST_PLAY_FLAG_SOFT_COLORBALANCE = (1 << 10) /**< Force software color balance. */
} GstPlayFlags;

#endif /* __EMOTION_GSTREAMER_H__ */
