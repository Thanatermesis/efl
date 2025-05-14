#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define EFL_BETA_API_SUPPORT
#endif

#include "Ethumb.h"
#include "Ethumb_Plugin.h"

#define EDJE_EDIT_IS_UNSTABLE_AND_I_KNOW_ABOUT_IT 1

#include <stdio.h>
#include <stdlib.h>
#include <Eo.h>
#include <Eina.h>
#include <Eet.h>
#include <Ecore_File.h>
#include <Evas.h>
#include <Ecore.h>
#include <Edje.h>
#include <Edje_Edit.h>
#include <Emotion.h>

static Eina_Prefix *_pfx = NULL;
static int _init_count = 0;
static int _log_dom = -1;
#define DBG(...) EINA_LOG_DOM_DBG(_log_dom, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_log_dom, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_log_dom, __VA_ARGS__)

/**
 * @brief Private data for the emotion thumbnailer plugin.
 *
 * This structure holds all the state required for generating a thumbnail from a
 * video file using Emotion.
 */
struct _emotion_plugin
{
   unsigned int fps; /**< Frames per second (not currently used). */
   double ptotal; /**< Total time for one segment of an animated thumbnail. */
   double len; /**< Total length of the video in seconds. */
   double pi; /**< Current position to seek to in the video. */
   double total_time; /**< Total accumulated time for the animated thumbnail. */
   double tmp_time; /**< Time of the current segment being processed. */
   unsigned int pcount; /**< Number of segments processed for animated thumbnail. */
   unsigned int frnum; /**< Frame number for animated thumbnail. */
   unsigned int okfr; /**< Counter for decoded frames to ensure seeking is complete. */
   Eina_Bool first; /**< Flag to indicate if it is the first frame grab. */
   Eet_File *ef; /**< Eet file handle for animated thumbnails. */
   Evas_Object *video; /**< Emotion video object. */
   Evas_Object *edje_frame; /**< Edje object for framing the video. */
   Ethumb *e; /**< Ethumb context. */
   int w, h; /**< Thumbnail width and height. */
};

static Eina_Bool _frame_grab(void *data);
static Eina_Bool _frame_grab_single(void *data);

/**
 * @brief Resizes the video object based on thumbnail parameters.
 *
 * This function calculates the aspect ratio and fill for the thumbnail and
 * resizes the Emotion video object and its containing frame accordingly.
 * It also mutes the audio of the video.
 *
 * @param _plugin The emotion plugin private data.
 */
static void
_resize_movie(struct _emotion_plugin *_plugin)
{
   Ethumb *e = _plugin->e;
   double ratio;
   int w, h;
   int fx, fy, fw, fh;

   ratio = emotion_object_ratio_get(_plugin->video);
   ethumb_calculate_aspect_from_ratio(e, ratio, &w, &h);
   ethumb_calculate_fill_from_ratio(e, ratio, &fx, &fy, &fw, &fh);
   DBG("size: w=%d, h=%d fill: x=%d, y=%d, w=%d, h=%d", w, h, fx, fy, fw, fh);

   _plugin->w = w;
   _plugin->h = h;

   ethumb_plugin_image_resize(e, _plugin->w, _plugin->h);

   if (_plugin->edje_frame)
     {
        evas_object_geometry_set(_plugin->edje_frame, fx, fy, fw, fh);
     }
   else
     {
        evas_object_geometry_set(_plugin->video, fx, fy, fw, fh);
     }
   emotion_object_audio_mute_set(_plugin->video, 1);
}

/**
 * @brief Callback for when a video frame is decoded.
 *
 * This callback is triggered by Emotion whenever a new frame is decoded. It
 * then calls the appropriate frame grabbing function depending on whether an
 * animated or static thumbnail is being generated.
 *
 * @param data The emotion plugin private data.
 * @param event The EFL event data (unused).
 */
static void
_frame_decode_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   struct _emotion_plugin *_plugin = data;

   if (_plugin->ef)
     _frame_grab(data);
   else
     _frame_grab_single(data);

   return;
 }

/**
 * @brief Callback for when the video is resized.
 *
 * @param data The emotion plugin private data.
 * @param event The EFL event data (unused).
 */
static void
_frame_resized_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   _resize_movie(data);
}

/**
 * @brief Callback for when video playback stops.
 *
 * Resets state related to frame grabbing and time accumulation when the
 * video playback stops unexpectedly or at the end of a segment.
 *
 * @param data The emotion plugin private data.
 * @param event The EFL event data (unused).
 */
static void
_video_stopped_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   struct _emotion_plugin *_plugin = data;

   _plugin->okfr = 0;
   _plugin->pi = 0;
   _plugin->ptotal = 0;
   _plugin->first = EINA_FALSE;
   _plugin->total_time = _plugin->tmp_time;
}

/**
 * @brief Sets the video position for the next frame grab.
 *
 * Calculates the time (in seconds) to seek to in the video for the next
 * thumbnail frame. This depends on whether it's the first frame for a static
 * thumbnail or one of many for an animated one.
 *
 * For static thumbnails, it uses `ethumb_video_start_get()`. If not set, it
 * defaults to 10% into the video.
 * For animated thumbnails, it calculates positions based on `ethumb_video_interval_get()`.
 *
 * @param _plugin The emotion plugin private data.
 */
static void
_video_pos_set(struct _emotion_plugin *_plugin)
{
   double pos;
   double interval;

   pos = ethumb_video_start_get(_plugin->e);
   interval = ethumb_video_interval_get(_plugin->e);
   _plugin->len = emotion_object_play_length_get(_plugin->video);

   if (_plugin->len > 0)
     _plugin->first = EINA_TRUE;

   if ((pos <= 0) || (pos >= 1))
     _plugin->pi = (0.1 * _plugin->len) +
     (_plugin->pcount * _plugin->len * interval);
   else
     _plugin->pi = (pos * _plugin->len) +
     (_plugin->pcount * _plugin->len * interval);

   emotion_object_position_set(_plugin->video, _plugin->pi);
}

/**
 * @brief Configures the Edje file for an animated thumbnail.
 *
 * After all frames have been generated and stored in an Eet file (which is
 * also the thumbnail Edje file), this function updates the Edje file. It adds
 * references to the generated images and sets up the "animate" and "animate_loop"
 * programs to create the animation. It also sets the group's min/max dimensions.
 *
 * @param _plugin The emotion plugin private data.
 * @return 1 on success, 0 on failure.
 */
static int
_setup_thumbnail(struct _emotion_plugin *_plugin)
{
   char buf[4096];
   Evas *evas;
   Evas_Object *edje;
   unsigned int i;
   const char *thumb_path;

   ethumb_thumb_path_get(_plugin->e, &thumb_path, NULL);
   evas = ethumb_evas_get(_plugin->e);

   if (!edje_file_group_exists(thumb_path, "movie/thumb"))
     {
        ERR("no group 'movie/thumb' found in file=%s", thumb_path);
        goto exit_error;
     }

   edje = edje_edit_object_add(evas);
   edje_object_file_set(edje, thumb_path, "movie/thumb");
   if (!edje_object_part_exists(edje, "image"))
     {
        ERR("no 'image' part found in file=%s, group=movie/thumb", thumb_path);
        evas_object_del(edje);
        goto exit_error;
     }
   if (!edje_edit_program_exist(edje, "animate"))
     {
        ERR("no 'animate' program found in file=%s, group=movie/thumb",
            thumb_path);
        evas_object_del(edje);
        goto exit_error;
     }

   for (i = 0; i < _plugin->frnum; i++)
     {
	snprintf(buf, sizeof(buf), "images/%u", i);
	edje_edit_image_data_add(edje, buf, i);
	if (i == 0)
	  edje_edit_state_image_set(edje, "image", "default", 0.00, buf);
	else
	  edje_edit_state_tween_add(edje, "image", "default", 0.00, buf);
     }

   edje_edit_program_transition_time_set(edje, "animate",
					 _plugin->total_time);
   edje_edit_program_transition_time_set(edje, "animate_loop",
					 _plugin->total_time);
   edje_edit_group_min_w_set(edje, _plugin->w);
   edje_edit_group_max_w_set(edje, _plugin->w);
   edje_edit_group_min_h_set(edje, _plugin->h);
   edje_edit_group_max_h_set(edje, _plugin->h);
   edje_edit_save(edje);

   evas_object_del(edje);

   return 1;

exit_error:
   return 0;
}

/**
 * @brief Cleans up thumbnailing objects.
 *
 * This function is called as an Ecore job to delete the Emotion video object
 * and the Edje frame object, and to free the plugin data structure. This is
 * done in a job to avoid issues with object deletion from within an event
 * callback.
 *
 * @param data The emotion plugin private data.
 */
static void
_finish_thumb_obj(void *data)
{
   struct _emotion_plugin *_plugin = data;
   evas_object_del(_plugin->video);
   if (_plugin->edje_frame) evas_object_del(_plugin->edje_frame);
   free(_plugin);
}

/**
 * @brief Finalizes the thumbnail generation process.
 *
 * This function is called when thumbnail generation is complete (either
 * successfully or with an error). It unregisters event callbacks, closes the
 * Eet file for animated thumbnails, sets up the final Edje file via
 * _setup_thumbnail(), calls the ethumb finished callback, and schedules the
 * cleanup of plugin resources.
 *
 * @param _plugin The emotion plugin private data.
 * @param success A boolean indicating if the thumbnail was generated successfully.
 */
static void
_finish_thumb_generation(struct _emotion_plugin *_plugin, int success)
{
   int r = 0;

   efl_event_callback_del(_plugin->video, EFL_CANVAS_VIDEO_EVENT_FRAME_RESIZE, _frame_resized_cb, _plugin);
   efl_event_callback_del(_plugin->video, EFL_CANVAS_VIDEO_EVENT_FRAME_DECODE, _frame_decode_cb, _plugin);
   efl_event_callback_del(_plugin->video, EFL_CANVAS_VIDEO_EVENT_PLAYBACK_STOP, _video_stopped_cb, _plugin);

   emotion_object_play_set(_plugin->video, 0);

   if (_plugin->ef)
     {
        Eet_Error err = eet_close(_plugin->ef);
        if (err != EET_ERROR_NONE)
          {
             ERR("Error writing Eet thumbnail file: %d", err);
             success = EINA_FALSE;
          }
     }

   if (success)
     r = _setup_thumbnail(_plugin);

   ethumb_finished_callback_call(_plugin->e, r);

   ecore_job_add(_finish_thumb_obj, _plugin);
}

/**
 * @brief Grabs a single frame for a static thumbnail.
 *
 * This function is called repeatedly from the frame decode callback. It waits
 * for a few frames to be decoded (`okfr < 5`) to ensure the video has properly
 * seeked to the desired position. Once ready, it saves the current frame as
 * the thumbnail image and finalizes the process.
 *
 * @param data The emotion plugin private data.
 * @return EINA_TRUE to continue being called, EINA_FALSE to stop.
 */
static Eina_Bool
_frame_grab_single(void *data)
{
   struct _emotion_plugin *_plugin = data;
   Ethumb *e = _plugin->e;
   double p;

   if (_plugin->len <= 0)
     {
        _video_pos_set(_plugin);
        return EINA_TRUE;
     }

   p = emotion_object_position_get(_plugin->video);
   _plugin->okfr++;
   if (_plugin->okfr < 5)
     return EINA_TRUE;

   DBG("saving static thumbnail at position=%f (intended=%f)", p, _plugin->pi);

   ethumb_image_save(e);

   efl_event_callback_del(_plugin->video, EFL_CANVAS_VIDEO_EVENT_FRAME_RESIZE, _frame_resized_cb, _plugin);

   emotion_object_play_set(_plugin->video, 0);

   evas_object_del(_plugin->video);
   if (_plugin->edje_frame) evas_object_del(_plugin->edje_frame);
   free(_plugin);

   ethumb_finished_callback_call(e, 1);

   return EINA_FALSE;
}

/**
 * @brief Grabs a frame for an animated thumbnail.
 *
 * Called from the frame decode callback. This function checks if the video
 * has reached the target position for the current frame grab. If so, it extracts
 * the pixel data and writes it as a compressed image into the Eet file.
 *
 * It manages multiple segments of video for creating the animated thumbnail,
 * advancing through segments and seeking to new positions as needed until the
 * required number of frames (`ethumb_video_ntimes_get()`) is generated.
 *
 * @param data The emotion plugin private data.
 * @return EINA_TRUE to continue grabbing frames, EINA_FALSE when done.
 */
static Eina_Bool
_frame_grab(void *data)
{
   struct _emotion_plugin *_plugin = data;
   Ethumb *e = _plugin->e;
   char buf[4096];
   const void *pixels;
   double p;

   if (_plugin->len <= 0)
     {
	_video_pos_set(_plugin);
	return EINA_TRUE;
     }

   p = emotion_object_position_get(_plugin->video);
   if (p < _plugin->pi)
     return EINA_TRUE;

   if (_plugin->first)
     {
	_plugin->pi = p;
	_plugin->first = EINA_FALSE;
     }

   if (p > _plugin->pi + _plugin->ptotal)
     {
	_plugin->total_time += _plugin->tmp_time;
	if (_plugin->pcount >= ethumb_video_ntimes_get(e))
	  {
	     _finish_thumb_generation(_plugin, EINA_TRUE);
	     return EINA_FALSE;
	  }
	else
	  {
	     _plugin->pcount++;
	     _video_pos_set(_plugin);
	     return EINA_TRUE;
	  }
     }

   _plugin->tmp_time = p - _plugin->pi;

   if (_plugin->ef)
     {
	Ecore_Evas *ee = ethumb_ecore_evas_get(e);
	int quality, compress;

	quality = ethumb_thumb_quality_get(e);
	compress = ethumb_thumb_compress_get(e);

	pixels = ecore_evas_buffer_pixels_get(ee);
	snprintf(buf, sizeof(buf), "images/%d", _plugin->frnum);
        if (!eet_data_image_write(_plugin->ef, buf, pixels, _plugin->w, _plugin->h,
                                  0, compress, quality, quality))
           return EINA_TRUE;
	_plugin->frnum++;
     }

   return EINA_TRUE;
}

/**
 * @brief Prepares for animated thumbnail generation.
 *
 * For animated thumbnails, this function prepares the target thumbnail file.
 * It copies a template Edje file to the final thumbnail location and opens it
 * as an Eet file in read/write mode. This file will be populated with image
 * data by `_frame_grab`.
 *
 * @param _plugin The emotion plugin private data.
 */
static void
_generate_animated_thumb(struct _emotion_plugin *_plugin)
{
   const char *thumb_path;
   char *thumb_dir;
   char buf[4096];
   Ethumb *e = _plugin->e;

   snprintf(buf, sizeof(buf),
            "%s/ethumb/modules/emotion/" MODULE_ARCH "/template.edj",
            eina_prefix_lib_get(_pfx));
   ethumb_thumb_path_get(e, &thumb_path, NULL);
   thumb_dir = ecore_file_dir_get(thumb_path);
   ecore_file_mkpath(thumb_dir);
   free(thumb_dir);
   if (!eina_file_copy(buf, thumb_path, 0, NULL, NULL))
     {
        ERR("Couldn't copy file '%s' to '%s'", buf, thumb_path);
        ERR("could not open '%s'", thumb_path);
        _finish_thumb_generation(_plugin, 0);
        return;
     }
   _plugin->ef = eet_open(thumb_path, EET_FILE_MODE_READ_WRITE);
   if (!_plugin->ef)
     {
        ERR("could not open '%s'", thumb_path);
        _finish_thumb_generation(_plugin, 0);
     }
}

/**
 * @brief The main thumbnail generation function for this plugin.
 *
 * This is the entry point for the plugin, called by Ethumb to start
 * generating a thumbnail for a video file. It creates and initializes an
 * Emotion object, sets up necessary callbacks, determines if a static or
 * animated thumbnail is requested, and starts video playback to begin the
 * frame grabbing process.
 *
 * If a custom frame is specified via `ethumb_frame_get()`, it will be used
 * to wrap the video content.
 *
 * @param e The Ethumb context for this thumbnail request.
 * @return A pointer to the private plugin data on success, NULL on failure.
 */
static void *
_thumb_generate(Ethumb *e)
{
   Evas_Object *o;
   int r;
   const char *file;
   Ethumb_Thumb_Format f;
   double dv;
   struct _emotion_plugin *_plugin = calloc(1, sizeof(struct _emotion_plugin));
   const char *ffile, *fgroup, *fswallow;

   o = emotion_object_add(ethumb_evas_get(e));
   r = emotion_object_init(o, NULL);
   if (!r)
     {
        ERR("Could not initialize emotion object.");
        evas_object_del(o);
        ethumb_finished_callback_call(e, 0);
        free(_plugin);
        return NULL;
     }

   _plugin->video = o;

   ethumb_file_get(e, &file, NULL);
   f = ethumb_thumb_format_get(e);

   emotion_object_file_set(o, file);
   emotion_object_audio_mute_set(o, EINA_TRUE);

   _plugin->video = o;
   _plugin->e = e;

   dv = ethumb_video_ntimes_get(e);
   if (dv > 0.0) _plugin->ptotal = ethumb_video_time_get(e) / dv;
   else _plugin->ptotal = 0.0;
   _plugin->pcount = 1;

   _resize_movie(_plugin);
   efl_event_callback_add
     (o, EFL_CANVAS_VIDEO_EVENT_FRAME_DECODE, _frame_decode_cb, _plugin);
   efl_event_callback_add
     (o, EFL_CANVAS_VIDEO_EVENT_FRAME_RESIZE, _frame_resized_cb, _plugin);
   efl_event_callback_add
     (o, EFL_CANVAS_VIDEO_EVENT_PLAYBACK_STOP, _video_stopped_cb, _plugin);

   if (f == ETHUMB_THUMB_EET)
     {
        _generate_animated_thumb(_plugin);
     }

   _video_pos_set(_plugin);
   emotion_object_play_set(o, 1);
   evas_object_show(o);

   ethumb_frame_get(e, &ffile, &fgroup, &fswallow);
   if (ffile && fgroup && fswallow)
     {
        Evas_Object *ed = edje_object_add(ethumb_evas_get(e));
        if (!ed)
          {
             ERR("could not create edje frame object.");
             return _plugin;
          }
        if (!edje_object_file_set(ed, ffile, fgroup))
          {
             ERR("could not load frame theme.");
             evas_object_del(ed);
             return _plugin;
          }
        edje_object_part_swallow(ed, fswallow, o);
        if (!edje_object_part_swallow_get(ed, fswallow))
          {
             ERR("could not swallow video to edje frame.");
             evas_object_del(ed);
             return _plugin;
          }
        evas_object_show(ed);
        _plugin->edje_frame = ed;
     }

   return _plugin;
}

/**
 * @brief Cancels an in-progress thumbnail generation.
 *
 * Called by Ethumb when a thumbnail request is cancelled. It cleans up all
 * resources associated with the generation process.
 *
 * @param e The Ethumb context (unused).
 * @param data The private plugin data.
 */
static void
_thumb_cancel(Ethumb *e EINA_UNUSED, void *data)
{
   struct _emotion_plugin *_plugin = data;

   if (_plugin->ef) eet_close(_plugin->ef);
   evas_object_del(_plugin->video);
   if (_plugin->edje_frame) evas_object_del(_plugin->edje_frame);
   free(_plugin);
}

static const char *extensions[] = { /* based on emotion's list */
  "264",
  "3g2",
  "3gp",
  "3gp2",
  "3gpp",
  "3gpp2",
  "3p2",
  "asf",
  "avi",
  "bdm",
  "bdmv",
  "clpi",
  "clp",
  "fla",
  "flv",
  "m1v",
  "m2v",
  "m2t",
  "m4v",
  "mkv",
  "mov",
  "mp2",
  "mp2ts",
  "mp4",
  "mpe",
  "mpeg",
  "mpg",
  "mpl",
  "mpls",
  "mts",
  "mxf",
  "nut",
  "nuv",
  "ogg",
  "ogm",
  "ogv",
  "rm",
  "rmj",
  "rmm",
  "rms",
  "rmx",
  "rmvb",
  "swf",
  "ts",
  "weba",
  "webm",
  "wmv",
  NULL
};
static const Ethumb_Plugin plugin =
  {
    ETHUMB_PLUGIN_API_VERSION,
    "emotion",
    extensions,
    _thumb_generate,
    _thumb_cancel
};

/**
 * @brief Initializes the emotion thumbnailer module.
 *
 * This function is called by Eina when the module is loaded. It initializes
 * Emotion, registers a log domain, and registers the plugin with Ethumb.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_module_init(void)
{
   if (_init_count > 0)
     {
        _init_count++;
        return EINA_TRUE;
     }

   _log_dom = eina_log_domain_register("ethumb_emotion", EINA_COLOR_GREEN);
   if (_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: ethumb_emotion");
        goto error_log;
     }

   _pfx = eina_prefix_new(NULL, ethumb_init,
                          "ETHUMB", "ethumb", "checkme",
                          PACKAGE_BIN_DIR, PACKAGE_LIB_DIR,
                          PACKAGE_DATA_DIR, PACKAGE_DATA_DIR);
   if (!_pfx)
     {
        ERR("Could not get ethumb installation prefix.");
        goto error_pfx;
     }

   emotion_init();

   ethumb_plugin_register(&plugin);

   _init_count = 1;
   return EINA_TRUE;

 error_pfx:
   eina_log_domain_unregister(_log_dom);
   _log_dom = -1;

 error_log:
   return EINA_FALSE;
}

/**
 * @brief Shuts down the emotion thumbnailer module.
 *
 * This function is called by Eina when the module is unloaded. It unregisters
 * the plugin from Ethumb, shuts down Emotion, and cleans up resources.
 */
static void
_module_shutdown(void)
{
   if (_init_count <= 0)
     {
        EINA_LOG_ERR("Init count not greater than 0 in shutdown.");
        return;
     }
   _init_count--;
   if (_init_count > 0) return;

   ethumb_plugin_unregister(&plugin);

   emotion_shutdown();

   eina_prefix_free(_pfx);
   _pfx = NULL;
   eina_log_domain_unregister(_log_dom);
   _log_dom = -1;
}

EINA_MODULE_INIT(_module_init);
EINA_MODULE_SHUTDOWN(_module_shutdown);
