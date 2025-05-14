#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "emotion_gstreamer.h"

/**
 * @brief Allocates and initializes an Emotion_Gstreamer_Buffer.
 *
 * This function creates a new Emotion_Gstreamer_Buffer, which is used to
 * hold video frame data from GStreamer for processing by Emotion.
 * It references the provided GstBuffer and maps the video frame if possible.
 *
 * @param sink The EmotionVideoSink associated with this buffer.
 * @param buffer The GStreamer buffer containing the video frame data.
 * @param info The GstVideoInfo describing the video frame.
 * @param eformat The Evas_Colorspace format of the video frame.
 * @param eheight The height of the video frame in Evas units.
 * @param func The Evas_Video_Convert_Cb callback function for video conversion.
 * @return A pointer to the newly allocated Emotion_Gstreamer_Buffer, or NULL on failure.
 */
Emotion_Gstreamer_Buffer *
emotion_gstreamer_buffer_alloc(EmotionVideoSink *sink,
                               GstBuffer *buffer,
                               GstVideoInfo *info,
                               Evas_Colorspace eformat,
                               int eheight,
                               Evas_Video_Convert_Cb func)
{
   Emotion_Gstreamer_Buffer *send;

   if (!sink->priv->emotion_object) return NULL;

   send = calloc(1, sizeof(Emotion_Gstreamer_Buffer));
   if (!send) return NULL;

   send->sink = gst_object_ref(sink);
   send->frame = gst_buffer_ref(buffer);
   send->info = *info;
   if (gst_video_frame_map(&(send->vframe), info, buffer, GST_MAP_READ))
     send->vfmapped = EINA_TRUE;
   else
     send->vfmapped = EINA_FALSE;
   send->eformat = eformat;
   send->eheight = eheight;
   send->func = func;
   return send;
}

/**
 * @brief Frees an Emotion_Gstreamer_Buffer.
 *
 * This function releases the resources associated with an Emotion_Gstreamer_Buffer,
 * including unreferencing the GStreamer objects and freeing the memory.
 *
 * @param send The Emotion_Gstreamer_Buffer to free.
 */
void
emotion_gstreamer_buffer_free(Emotion_Gstreamer_Buffer *send)
{
   gst_object_unref(send->sink);
   gst_buffer_replace(&send->frame, NULL);
   free(send);
}

/**
 * @brief Allocates and initializes an Emotion_Gstreamer_Message.
 *
 * This function creates a new Emotion_Gstreamer_Message, which is used to
 * wrap a GstMessage for processing within the Emotion GStreamer integration.
 * It references the provided Emotion_Gstreamer instance and GstMessage.
 *
 * @param ev The Emotion_Gstreamer instance associated with this message.
 * @param msg The GStreamer message to wrap.
 * @return A pointer to the newly allocated Emotion_Gstreamer_Message, or NULL on failure.
 */
Emotion_Gstreamer_Message *
emotion_gstreamer_message_alloc(Emotion_Gstreamer *ev,
                                GstMessage *msg)
{
   Emotion_Gstreamer_Message *send;

   if (!ev) return NULL;

   send = malloc(sizeof (Emotion_Gstreamer_Message));
   if (!send) return NULL;

   send->ev = emotion_gstreamer_ref(ev);
   send->msg = gst_message_ref(msg);

   return send;
}

/**
 * @brief Frees an Emotion_Gstreamer_Message.
 *
 * This function releases the resources associated with an Emotion_Gstreamer_Message,
 * including unreferencing the Emotion_Gstreamer instance and the GstMessage,
 * and freeing the memory.
 *
 * @param send The Emotion_Gstreamer_Message to free.
 */
void
emotion_gstreamer_message_free(Emotion_Gstreamer_Message *send)
{
   emotion_gstreamer_unref(send->ev);
   gst_message_unref(send->msg);
   free(send);
}
