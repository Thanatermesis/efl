#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "emotion_gstreamer.h"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE("sink",
                                                                   GST_PAD_SINK, GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE("{ I420, YV12, YUY2, NV12, BGRx, BGR, BGRA }")));

GST_DEBUG_CATEGORY_STATIC(emotion_video_sink_debug);
#define GST_CAT_DEFAULT emotion_video_sink_debug

enum {
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_EMOTION_OBJECT,
  PROP_LAST
};

#define _do_init                                        \
  GST_DEBUG_CATEGORY_INIT(emotion_video_sink_debug,        \
                          "emotion-sink",		\
                          0,                            \
                          "emotion video sink")

#define parent_class emotion_video_sink_parent_class
G_DEFINE_TYPE_WITH_CODE(EmotionVideoSink,
                     emotion_video_sink,
                     GST_TYPE_VIDEO_SINK,
                     G_ADD_PRIVATE(EmotionVideoSink)
                     _do_init);


static void unlock_buffer_mutex(EmotionVideoSinkPrivate* priv);
static void emotion_video_sink_main_render(void *data);

/**
 * @brief Initializes a new EmotionVideoSink instance.
 * @param sink The EmotionVideoSink instance to initialize.
 *
 * This function is called when a new instance of the sink is created.
 * It allocates and initializes the private data structure, including
 * a mutex and condition variable for thread synchronization.
 */
static void
emotion_video_sink_init(EmotionVideoSink* sink)
{
   EmotionVideoSinkPrivate* priv;

   INF("sink init");
   sink->priv = priv = emotion_video_sink_get_instance_private(sink);
   gst_video_info_init (&priv->info);
   priv->eheight = 0;
   priv->func = NULL;
   priv->eformat = EVAS_COLORSPACE_ARGB8888;
   eina_lock_new(&priv->m);
   eina_condition_new(&priv->c, &priv->m);
   priv->unlocked = EINA_FALSE;
}

/**** Object methods ****/
/**
 * @brief Callback function to clear the Evas object reference when it's deleted.
 * @param data The private data of the EmotionVideoSink.
 * @param e The Evas canvas.
 * @param obj The Evas object that was deleted.
 * @param event_info Event-specific data.
 *
 * This function is registered as a callback for the `EVAS_CALLBACK_DEL` event
 * on the Evas object. It ensures that the sink does not hold a dangling pointer
 * to the Evas object after it has been deleted.
 */
static void
_cleanup_priv(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   EmotionVideoSinkPrivate* priv;

   priv = data;

   eina_lock_take(&priv->m);
   if (priv->evas_object == obj)
     priv->evas_object = NULL;
   eina_lock_release(&priv->m);
}

/**
 * @brief Sets a property on the EmotionVideoSink object.
 * @param object A GObject instance.
 * @param prop_id The ID of the property to set.
 * @param value The value to set for the property.
 * @param pspec The GParamSpec for the property.
 *
 * This function handles setting GObject properties. It's primarily used
 * to set the "emotion-object" property, which links this GStreamer sink
 * to the Emotion object that will display the video.
 */
static void
emotion_video_sink_set_property(GObject * object, guint prop_id,
                             const GValue * value, GParamSpec * pspec)
{
   EmotionVideoSink* sink;
   EmotionVideoSinkPrivate* priv;

   sink = EMOTION_VIDEO_SINK (object);
   priv = sink->priv;

   switch (prop_id) {
    case PROP_EMOTION_OBJECT:
       eina_lock_take(&priv->m);
       if (priv->evas_object)
         evas_object_event_callback_del(priv->evas_object, EVAS_CALLBACK_DEL, _cleanup_priv);
       priv->emotion_object = g_value_get_pointer (value);
       INF("sink set Emotion object %p", priv->emotion_object);
       if (priv->emotion_object)
         {
            priv->evas_object = emotion_object_image_get(priv->emotion_object);
            if (priv->evas_object)
              {
                 evas_object_event_callback_add(priv->evas_object, EVAS_CALLBACK_DEL, _cleanup_priv, priv);
                 evas_object_image_pixels_get_callback_set(priv->evas_object, NULL, NULL);
              }
         }
       eina_lock_release(&priv->m);
       break;
    default:
       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
       ERR("invalid property");
       break;
   }
}

/**
 * @brief Gets a property from the EmotionVideoSink object.
 * @param object A GObject instance.
 * @param prop_id The ID of the property to get.
 * @param value A GValue to store the property value.
 * @param pspec The GParamSpec for the property.
 *
 * This function handles getting GObject properties. It's used to retrieve
 * the "emotion-object" property.
 */
static void
emotion_video_sink_get_property(GObject * object, guint prop_id,
                             GValue * value, GParamSpec * pspec)
{
   EmotionVideoSink* sink;
   EmotionVideoSinkPrivate* priv;

   sink = EMOTION_VIDEO_SINK (object);
   priv = sink->priv;

   switch (prop_id) {
    case PROP_EMOTION_OBJECT:
       INF("sink get property.");
       eina_lock_take(&priv->m);
       g_value_set_pointer(value, priv->emotion_object);
       eina_lock_release(&priv->m);
       break;
    default:
       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
       ERR("invalid property");
       break;
   }
}

/**
 * @brief Disposes of the EmotionVideoSink instance.
 * @param object The EmotionVideoSink GObject to dispose.
 *
 * This is called when the object's refcount drops to zero. It releases
 * all resources held by the sink, such as unmapping any mapped video frames,
 * unreferencing buffers, and freeing the mutex and condition variable.
 */
static void
emotion_video_sink_dispose(GObject* object)
{
   EmotionVideoSink* sink;
   EmotionVideoSinkPrivate* priv;

   INF("dispose.");

   sink = EMOTION_VIDEO_SINK(object);
   priv = sink->priv;

   eina_lock_take(&priv->m);
   if (priv->vfmapped)
     {
        if (priv->evas_object)
          {
             evas_object_image_size_set(priv->evas_object, 1, 1);
             evas_object_image_data_set(priv->evas_object, NULL);
          }
        gst_video_frame_unmap(&(priv->last_vframe));
        priv->vfmapped = EINA_FALSE;
     }
   else
     {
        if ((priv->mapped) && (priv->last_buffer))
          {
             if (priv->evas_object)
               {
                  evas_object_image_size_set(priv->evas_object, 1, 1);
                  evas_object_image_data_set(priv->evas_object, NULL);
               }
             gst_buffer_unmap(priv->last_buffer, &(priv->map_info));
             priv->mapped = EINA_FALSE;
          }
     }
   if (priv->last_buffer)
     {
        gst_buffer_unref(priv->last_buffer);
        priv->last_buffer = NULL;
     }

   eina_lock_release(&priv->m);
   eina_lock_free(&priv->m);
   eina_condition_free(&priv->c);

   G_OBJECT_CLASS(parent_class)->dispose(object);
}


/**** BaseSink methods ****/

/**
 * @brief Sets the capabilities of the sink.
 * @param bsink The GstBaseSink instance.
 * @param caps The GstCaps to set.
 * @return TRUE on success, FALSE on failure.
 *
 * This function is called when the sink pad's caps are configured. It
 * parses the video format from the caps and looks up a suitable color
 * space converter function to convert the input video format to a format
 * that Evas can render.
 */
gboolean emotion_video_sink_set_caps(GstBaseSink *bsink, GstCaps *caps)
{
   EmotionVideoSink* sink;
   EmotionVideoSinkPrivate* priv;
   GstVideoInfo info;
   unsigned int i;

   sink = EMOTION_VIDEO_SINK(bsink);
   priv = sink->priv;

   if (!gst_video_info_from_caps(&info, caps))
     {
        ERR("Unable to parse caps.");
        return FALSE;
     }

   priv->info = info;
   priv->eheight = info.height;

   for (i = 0; colorspace_format_conversion[i].name; i++)
     {
        if ((info.finfo->format == colorspace_format_conversion[i].format) &&
            ((colorspace_format_conversion[i].colormatrix == GST_VIDEO_COLOR_MATRIX_UNKNOWN) ||
             (colorspace_format_conversion[i].colormatrix == info.colorimetry.matrix)))
          {
             DBG("Found '%s'", colorspace_format_conversion[i].name);
             priv->eformat = colorspace_format_conversion[i].eformat;
             priv->func = colorspace_format_conversion[i].func;
             if (colorspace_format_conversion[i].force_height)
               {
                  priv->eheight = (priv->eheight >> 1) << 1;
               }
             return TRUE;
          }
       }

   ERR("unsupported : %s\n", gst_video_format_to_string(info.finfo->format));
   return FALSE;
}

/**
 * @brief Starts the sink element.
 * @param base_sink The GstBaseSink instance.
 * @return TRUE if start is successful, FALSE otherwise.
 *
 * This function is called when the pipeline goes from READY to PAUSED state.
 * It checks if the emotion object has been set, which is required for rendering.
 */
static gboolean
emotion_video_sink_start(GstBaseSink* base_sink)
{
   EmotionVideoSinkPrivate* priv;
   gboolean res = TRUE;

   INF("sink start");

   priv = EMOTION_VIDEO_SINK(base_sink)->priv;
   eina_lock_take(&priv->m);
   if (!priv->emotion_object)
     res = FALSE;
   else
     priv->unlocked = EINA_FALSE;
   eina_lock_release(&priv->m);

   priv->frames = priv->rlapse = priv->flapse = 0;

   return res;
}

/**
 * @brief Stops the sink element.
 * @param base_sink The GstBaseSink instance.
 * @return Always returns TRUE.
 *
 * This function is called when the pipeline goes from PAUSED to READY state.
 * It cleans up any held resources, like video frames and buffers, to ensure
 * a clean state. It also signals any waiting threads to unblock.
 */
static gboolean
emotion_video_sink_stop(GstBaseSink* base_sink)
{
   EmotionVideoSinkPrivate* priv = EMOTION_VIDEO_SINK(base_sink)->priv;

   INF("sink stop");

   eina_lock_take(&priv->m);
   if (priv->vfmapped)
     {
        if (priv->evas_object)
          {
             evas_object_image_size_set(priv->evas_object, 1, 1);
             evas_object_image_data_set(priv->evas_object, NULL);
          }
        gst_video_frame_unmap(&(priv->last_vframe));
        priv->vfmapped = EINA_FALSE;
     }
   if (priv->last_buffer)
     {
        if (priv->evas_object)
          {
             evas_object_image_size_set(priv->evas_object, 1, 1);
             evas_object_image_data_set(priv->evas_object, NULL);
          }
        if (priv->mapped)
          gst_buffer_unmap(priv->last_buffer, &(priv->map_info));
        priv->mapped = EINA_FALSE;
        gst_buffer_unref(priv->last_buffer);
        priv->last_buffer = NULL;
     }

   /* If there still is a pending frame, neutralize it */
   if (priv->send)
     {
        gst_buffer_replace(&priv->send->frame, NULL);
        priv->send = NULL;
     }

   unlock_buffer_mutex(priv);
   eina_lock_release(&priv->m);
   return TRUE;
}

/**
 * @brief Prepares the sink for a state change.
 * @param object The GstBaseSink instance.
 * @return The result of the parent class's unlock function.
 *
 * This is called before a state change to PAUSED or READY. It marks the sink
 * as unlocked, causing subsequent buffer submissions in show_frame() to be
 * rejected.
 */
static gboolean
emotion_video_sink_unlock(GstBaseSink* object)
{
   EmotionVideoSink* sink;

   INF("sink unlock");

   sink = EMOTION_VIDEO_SINK(object);

   eina_lock_take(&sink->priv->m);
   unlock_buffer_mutex(sink->priv);
   eina_lock_release(&sink->priv->m);

   return GST_CALL_PARENT_WITH_DEFAULT(GST_BASE_SINK_CLASS, unlock,
                                       (object), TRUE);
}

/**
 * @brief Re-enables buffer processing after a state change.
 * @param object The GstBaseSink instance.
 * @return The result of the parent class's unlock_stop function.
 *
 * This is called after a state change to re-enable buffer processing. It
 * resets the `unlocked` flag, allowing show_frame() to process buffers again.
 */
static gboolean
emotion_video_sink_unlock_stop(GstBaseSink* object)
{
   EmotionVideoSink* sink;
   EmotionVideoSinkPrivate* priv;

   sink = EMOTION_VIDEO_SINK(object);
   priv = sink->priv;

   INF("sink unlock stop");

   eina_lock_take(&priv->m);
   priv->unlocked = FALSE;
   eina_lock_release(&priv->m);

   return GST_CALL_PARENT_WITH_DEFAULT(GST_BASE_SINK_CLASS, unlock_stop,
                                       (object), TRUE);
}

/**
 * @brief Renders a video frame.
 * @param vsink The GstVideoSink instance.
 * @param buffer The GstBuffer containing the video frame to render.
 * @return A GstFlowReturn code indicating success or failure.
 *
 * This function is the core of the video sink. It is called by GStreamer
 * for each video frame that needs to be displayed. It prepares the buffer
 * for rendering, dispatches it to the main thread via an async ecore call
 * (`emotion_video_sink_main_render`), and then waits on a condition
 * variable for the rendering to complete. This synchronization ensures that
 * the streaming thread doesn't get ahead of the rendering.
 */
static GstFlowReturn
emotion_video_sink_show_frame(GstVideoSink* vsink, GstBuffer* buffer)
{
   Emotion_Gstreamer_Buffer *send;
   EmotionVideoSinkPrivate *priv;
   EmotionVideoSink *sink;

   INF("sink render %p", buffer);

   sink = EMOTION_VIDEO_SINK(vsink);
   priv = sink->priv;

   eina_lock_take(&priv->m);
   if (priv->unlocked) {
      ERR("LOCKED");
      eina_lock_release(&priv->m);
      return GST_FLOW_FLUSHING;
   }

   send = emotion_gstreamer_buffer_alloc(sink, buffer, &priv->info, priv->eformat, priv->eheight, priv->func);

   /* If there still is a pending frame, neutralize it */
   if (priv->send)
     {
        gst_buffer_replace(&priv->send->frame, NULL);
     }
   priv->send = send;

   if (!send) {
      eina_lock_release(&priv->m);
      return GST_FLOW_ERROR;
   }

   _emotion_pending_ecore_begin();
   ecore_main_loop_thread_safe_call_async(emotion_video_sink_main_render, send);

   eina_condition_wait(&priv->c);
   eina_lock_release(&priv->m);

   return GST_FLOW_OK;
}

/**
 * @brief Updates and logs the frame rate.
 * @param priv The private data of the EmotionVideoSink.
 *
 * This is a helper function for debugging. If FPS debugging is enabled,
 * it calculates and potentially prints the current frames per second.
 */
static void
_update_emotion_fps(EmotionVideoSinkPrivate *priv)
{
   double tim;

   if (!debug_fps) return;

   tim = ecore_time_get();
   priv->frames++;

   if (EINA_DBL_EQ(priv->rlapse, 0.0))
     {
        priv->rlapse = tim;
        priv->flapse = priv->frames;
     }
   else if ((tim - priv->rlapse) >= 0.5)
     {
        priv->rlapse = tim;
        priv->flapse = priv->frames;
     }
}

/**
 * @brief Performs the actual rendering of a video frame on the main thread.
 * @param data A pointer to an Emotion_Gstreamer_Buffer structure.
 *
 * This function is called asynchronously on the main (Ecore) thread to
 * avoid threading issues with Evas. It takes a prepared video buffer,
 * converts its pixel data if necessary, and sets it on the Evas image
 * object for display. After rendering, it signals the streaming thread
 * (waiting in `emotion_video_sink_show_frame`) to continue.
 */
static void
emotion_video_sink_main_render(void *data)
{
   Emotion_Gstreamer_Buffer *send;
   EmotionVideoSinkPrivate *priv;
   GstBuffer *buffer = NULL;
   GstMapInfo map;
   unsigned char *evas_data;
   double ratio;
   Emotion_Convert_Info info;

   send = data;

   priv = send->sink->priv;

   eina_lock_take(&priv->m);
   /* Sink was shut down already or this is a stale
    * frame */
   if (priv->send != send)
     goto exit_point;
   if (!send->frame)
     goto exit_point;

   priv->send = NULL;

   if (!priv->emotion_object || priv->unlocked)
     goto exit_point;

   /* Can happen if cleanup_priv was called */
   if (!priv->evas_object)
     {
        priv->evas_object = emotion_object_image_get(priv->emotion_object);
        if (priv->evas_object)
          {
             evas_object_event_callback_add(priv->evas_object, EVAS_CALLBACK_DEL, _cleanup_priv, priv);
             evas_object_image_pixels_get_callback_set(priv->evas_object, NULL, NULL);
          }
     }

   if (!priv->evas_object)
     goto exit_point;

   buffer = gst_buffer_ref(send->frame);

   if (!send->vfmapped)
     {
        if (!gst_buffer_map(buffer, &map, GST_MAP_READ))
          {
             gst_buffer_unref(buffer);
             ERR("Cannot map video buffer for read.\n");
             goto exit_point;
          }
     }

   INF("sink main render [%i, %i] (source height: %i)", send->info.width, send->eheight, send->info.height);

   evas_object_image_alpha_set(priv->evas_object, 0);
   evas_object_image_colorspace_set(priv->evas_object, send->eformat);
   evas_object_image_size_set(priv->evas_object, send->info.width, send->eheight);

   evas_data = evas_object_image_data_get(priv->evas_object, 1);
   if (!evas_data)
     {
        if (!send->vfmapped)
          {
             gst_buffer_unmap(buffer, &map);
             priv->mapped = EINA_FALSE;
          }
        else
          {
             gst_video_frame_unmap(&(send->vframe));
             priv->vfmapped = EINA_FALSE;
          }
        gst_buffer_unref(buffer);
        goto exit_point;
     }

// XXX: need to handle GstVideoCropMeta to get video cropping right
// XXX: can't get crop meta from buffer (always null)
//   GstVideoCropMeta *meta;
//   meta = gst_buffer_get_video_crop_meta(buffer);
//   printf("META: %p\n", meta);

/* this just is a demo of broken vaapi back-end values for stride and
 * plane offset - the below is what i needed to fix them up for a few videos
 */
/*
   info.stride[0] = 64 * ((send->info.stride[0] + 63) / 64);
   info.stride[1] = 64 * ((send->info.stride[1] + 63) / 64);
   info.stride[2] = 64 * ((send->info.stride[2] + 63) / 64);
   info.stride[3] = 64 * ((send->info.stride[3] + 63) / 64);
   info.plane_offset[0] = send->info.offset[0];
   info.plane_offset[1] = (((send->info.height + 15) / 16) * 16) * info.stride[1];
   info.plane_offset[2] = send->info.offset[2];
   info.plane_offset[3] = send->info.offset[3];
 */
   if (send->vfmapped)
     {
        GstVideoFrame *vframe = &(send->vframe);

        map.data = GST_VIDEO_FRAME_PLANE_DATA(vframe, 0);
        info.bpp[0] = GST_VIDEO_FRAME_COMP_PSTRIDE(vframe, 0);
        info.bpp[1] = GST_VIDEO_FRAME_COMP_PSTRIDE(vframe, 1);
        info.bpp[2] = GST_VIDEO_FRAME_COMP_PSTRIDE(vframe, 2);
        info.bpp[3] = GST_VIDEO_FRAME_COMP_PSTRIDE(vframe, 3);
        info.stride[0] = GST_VIDEO_FRAME_COMP_STRIDE(vframe, 0);
        info.stride[1] = GST_VIDEO_FRAME_COMP_STRIDE(vframe, 1);
        info.stride[2] = GST_VIDEO_FRAME_COMP_STRIDE(vframe, 2);
        info.stride[3] = GST_VIDEO_FRAME_COMP_STRIDE(vframe, 3);
        info.plane_ptr[0] = GST_VIDEO_FRAME_PLANE_DATA(vframe, 0);
        info.plane_ptr[1] = GST_VIDEO_FRAME_PLANE_DATA(vframe, 1);
        info.plane_ptr[2] = GST_VIDEO_FRAME_PLANE_DATA(vframe, 2);
        info.plane_ptr[3] = GST_VIDEO_FRAME_PLANE_DATA(vframe, 3);
     }
   else
     {
        info.bpp[0] = 1;
        info.bpp[1] = 1;
        info.bpp[2] = 1;
        info.bpp[3] = 1;
        info.stride[0] = send->info.stride[0];
        info.stride[1] = send->info.stride[1];
        info.stride[2] = send->info.stride[2];
        info.stride[3] = send->info.stride[3];
        info.plane_ptr[0] = ((unsigned char *)map.data) + send->info.offset[0];
        info.plane_ptr[1] = ((unsigned char *)map.data) + send->info.offset[1];
        info.plane_ptr[2] = ((unsigned char *)map.data) + send->info.offset[2];
        info.plane_ptr[3] = ((unsigned char *)map.data) + send->info.offset[3];
     }

   if (send->func)
     send->func(evas_data, map.data, send->info.width, send->info.height, send->eheight, &info);
   else
     WRN("No way to decode %x colorspace !", send->eformat);

   evas_object_image_data_set(priv->evas_object, evas_data);
   evas_object_image_data_update_add(priv->evas_object, 0, 0, send->info.width, send->eheight);
   evas_object_image_pixels_dirty_set(priv->evas_object, 0);

   _update_emotion_fps(priv);

   ratio = (double) send->info.width / (double) send->eheight;
   ratio *= (double) send->info.par_n / (double) send->info.par_d;

   _emotion_frame_resize(priv->emotion_object, send->info.width, send->eheight, ratio);

   if (priv->vfmapped)
     {
        gst_video_frame_unmap(&(priv->last_vframe));
     }
   else
     {
        if ((priv->mapped) && (priv->last_buffer))
          gst_buffer_unmap(priv->last_buffer, &(priv->map_info));
     }
   if (send->vfmapped)
     {
        priv->last_vframe = send->vframe;
        priv->vfmapped = EINA_TRUE;
     }
   else
     {
        priv->vfmapped = EINA_FALSE;
        priv->map_info = map;
        priv->mapped = EINA_TRUE;
     }

   if (priv->last_buffer) gst_buffer_unref(priv->last_buffer);
   priv->last_buffer = buffer;

   _emotion_frame_new(priv->emotion_object);

 exit_point:
   if (!priv->unlocked)
     eina_condition_signal(&priv->c);

   eina_lock_release(&priv->m);

   emotion_gstreamer_buffer_free(send);

   _emotion_pending_ecore_end();
}

/**
 * @brief Signals the buffer condition variable and marks the sink as unlocked.
 * @param priv The private data of the EmotionVideoSink.
 *
 * This function must be called with the private data's mutex locked.
 * It sets the `unlocked` flag to `EINA_TRUE` and signals the condition
 * variable `c`, which typically unblocks a waiting thread in
 * emotion_video_sink_show_frame(). This is used to handle state changes
 * like stopping or flushing.
 */
/* Must be called with priv->m taken */
static void
unlock_buffer_mutex(EmotionVideoSinkPrivate* priv)
{
   priv->unlocked = EINA_TRUE;

   eina_condition_signal(&priv->c);
}

/**
 * @brief Initializes the EmotionVideoSinkClass.
 * @param klass The EmotionVideoSinkClass to initialize.
 *
 * This function sets up the GObject class, including installing properties,
 * setting up pad templates, and overriding virtual methods from parent
 * classes (like GstBaseSink and GstVideoSink) with the sink's specific
 * implementations.
 */
static void
emotion_video_sink_class_init(EmotionVideoSinkClass* klass)
{
   GObjectClass* gobject_class;
   GstElementClass* gstelement_class;
   GstBaseSinkClass* gstbase_sink_class;
   GstVideoSinkClass* gstvideo_sink_class;

   gobject_class = G_OBJECT_CLASS(klass);
   gstelement_class = GST_ELEMENT_CLASS(klass);
   gstbase_sink_class = GST_BASE_SINK_CLASS(klass);
   gstvideo_sink_class = GST_VIDEO_SINK_CLASS(klass);

   gobject_class->set_property = emotion_video_sink_set_property;
   gobject_class->get_property = emotion_video_sink_get_property;

   g_object_class_install_property (gobject_class, PROP_EMOTION_OBJECT,
                                    g_param_spec_pointer ("emotion-object", "Emotion Object",
                                                          "The Emotion object where the display of the video will be done",
                                                          G_PARAM_READWRITE));

   gobject_class->dispose = emotion_video_sink_dispose;

   gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&sinktemplate));
   gst_element_class_set_static_metadata(gstelement_class, "Emotion video sink",
                                        "Sink/Video", "Sends video data from a GStreamer pipeline to an Emotion object",
                                        "Vincent Torri <vtorri@univ-evry.fr>");

   gstbase_sink_class->set_caps = emotion_video_sink_set_caps;
   gstbase_sink_class->stop = emotion_video_sink_stop;
   gstbase_sink_class->start = emotion_video_sink_start;
   gstbase_sink_class->unlock = emotion_video_sink_unlock;
   gstbase_sink_class->unlock_stop = emotion_video_sink_unlock_stop;
   gstvideo_sink_class->show_frame = emotion_video_sink_show_frame;
}

/**
 * @brief GStreamer plugin initialization function.
 * @param plugin The GstPlugin to initialize.
 * @return TRUE on successful registration, FALSE otherwise.
 *
 * This is the entry point for the GStreamer plugin. It registers the
 * emotion-sink element with the GStreamer framework, making it available
 * for use in pipelines.
 */
gboolean
gstreamer_plugin_init (GstPlugin * plugin)
{
   return gst_element_register (plugin,
                                "emotion-sink",
                                GST_RANK_NONE,
                                EMOTION_TYPE_VIDEO_SINK);
}

