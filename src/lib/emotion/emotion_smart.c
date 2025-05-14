#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define EFL_CANVAS_OBJECT_PROTECTED
#define EFL_CANVAS_GROUP_PROTECTED

#include <Evas.h>
#include <Ecore.h>

#ifdef HAVE_EIO
# include <math.h>
# include <Eio.h>
#endif

#define EFL_INTERNAL_UNSTABLE
#include <Evas_Internal.h>

#include "Emotion.h"
#include "emotion_private.h"

#include "canvas/evas_canvas_eo.h"

#ifdef _WIN32
# define FMT_UCHAR "%c"
#else
# define FMT_UCHAR "%hhu"
#endif

#define E_SMART_OBJ_GET(smart, o, type) \
     { \
        if (!o) return; \
        if (!efl_isa(o, MY_CLASS)) { \
             ERR("Tried calling on a non-emotion object."); \
             return; \
        } \
        smart = efl_data_scope_get(o, MY_CLASS); \
        if (!smart) return; \
     }

#define E_SMART_OBJ_GET_RETURN(smart, o, type, ret) \
   { \
      if (!o) return ret; \
      if (!efl_isa(o, MY_CLASS)) { \
           ERR("Tried calling on a non-emotion object."); \
           return ret; \
      } \
      smart = efl_data_scope_get(o, MY_CLASS); \
      if (!smart) return ret; \
   }

#define E_OBJ_NAME "efl_canvas_video"

#ifdef MY_CLASS
# undef MY_CLASS
#endif

#define MY_CLASS EFL_CANVAS_VIDEO_CLASS

/**
 * @brief Private data structure for the Efl_Canvas_Video object.
 *
 * This structure holds all the internal state of an Emotion video object,
 * including engine details, file information, playback state, and rendering
 * properties.
 */
typedef struct _Efl_Canvas_Video_Data Efl_Canvas_Video_Data;

/**
 * @brief Private data structure for managing extended attributes (xattr) of a video file.
 *
 * This structure is used to store and manage file-specific metadata, such as
 * the last playback position, primarily for features like resuming playback.
 * It includes a reference count for managing its lifecycle and Eio_File handles
 * for asynchronous xattr operations if HAVE_EIO is defined.
 */
typedef struct _Emotion_Xattr_Data Emotion_Xattr_Data;

struct _Efl_Canvas_Video_Data
{
   Emotion_Engine_Instance *engine_instance; /**< Instance of the Emotion video engine. */

   const char    *engine; /**< Name of the video engine being used (e.g., "gstreamer1"). */
   const char    *file; /**< Path to the video file. */
   Evas_Object   *obj; /**< The Evas image object used for displaying video frames. */
   Evas_Object   *bg; /**< Background Evas object. */

   Ecore_Job     *job; /**< Ecore job for deferred position setting. */

   Emotion_Xattr_Data *xattr; /**< Extended attributes data for the video file. */

   const char *title; /**< Title of the video. */

   struct {
      const char *info; /**< Progress information string. */
      double  stat; /**< Progress status (0.0 to 1.0). */
   } progress; /**< Playback progress data. */
   struct {
      const char *file; /**< Referenced file path. */
      int   num; /**< Reference number. */
   } ref; /**< Reference file data. */
   struct {
      int button_num; /**< Number of SPU buttons. */
      int button; /**< Current SPU button. */
   } spu; /**< SPU (Subpicture Unit) data. */
   struct {
      int l; /**< Left crop value. */
      int r; /**< Right crop value. */
      int t; /**< Top crop value. */
      int b; /**< Bottom crop value. */
      Evas_Object *clipper; /**< Evas object used for cropping. */
   } crop; /**< Video cropping data. */

   struct {
      int         w, h; /**< Width and height of the video stream. */
   } video; /**< Video stream dimensions. */
   struct {
      double      w, h; /**< Fill width and height factors. */
   } fill; /**< Video fill mode data. */

   double         ratio; /**< Aspect ratio of the video. */
   double         pos; /**< Current playback position in seconds. */
   double         remember_jump; /**< Position to jump to after opening. */
   double         seek_pos; /**< Target position for seeking. */
   double         len; /**< Total length of the video in seconds. */

   Emotion_Module_Options module_options; /**< Options for the Emotion module. */

   Emotion_Suspend state; /**< Current suspend state of the video object. */
   Emotion_Aspect aspect; /**< Aspect ratio handling mode. */

   Ecore_Animator *anim; /**< Ecore animator for frame updates. */

   Eina_Bool open : 1; /**< Flag indicating if the video file is open. */
   Eina_Bool play : 1; /**< Flag indicating if the video is playing. */
   Eina_Bool pause : 1; /**< Flag indicating if the video is paused. */
   Eina_Bool remember_play : 1; /**< Flag to remember play state before opening. */
   Eina_Bool seek : 1; /**< Flag indicating a seek operation is pending. */
   Eina_Bool seeking : 1; /**< Flag indicating a seek operation is in progress. */
   Eina_Bool loaded : 1; /**< Flag indicating if the video file is loaded. */
};

struct _Emotion_Xattr_Data
{
   EINA_REFCOUNT; /**< Reference count for the xattr data. */
   Eo       *obj_wref; /**< Weak reference to the Emotion object. */
#ifdef HAVE_EIO
   Eio_File *load; /**< Eio handle for loading xattr. */
   Eio_File *save; /**< Eio handle for saving xattr. */
#endif
};

static void _mouse_move(void *data, Evas *ev, Evas_Object *obj, void *event_info);
static void _mouse_down(void *data, Evas *ev, Evas_Object *obj, void *event_info);
static void _pos_set_job(void *data);
static void _pixels_get(void *data, Evas_Object *obj);

/**
 * @brief Initializes the Emotion engine instance if not already initialized.
 * @param obj The Emotion Evas object.
 * @param sd Pointer to the private data of the Emotion object.
 */
static void
_engine_init(Eo *obj, Efl_Canvas_Video_Data *sd)
{
   if (sd->engine_instance) return;
   sd->engine_instance = emotion_engine_instance_new(sd->engine, obj,
                                                     &(sd->module_options));
}

/**
 * @brief Fills the image data buffer of an Evas image object with zeros.
 *
 * This is used to clear the video display area, for example, when a video
 * is closed or before a new frame is decoded into a differently sized buffer.
 * It handles different colorspaces (ARGB8888, YCbCr planar formats).
 *
 * @param img The Evas image object whose data is to be zeroed.
 */
static void
_emotion_image_data_zero(Evas_Object *img)
{
   void *data = NULL;

   data = evas_object_image_data_get(img, 1);
   if (data)
     {
        int w, h, sz = 0;
        Evas_Colorspace cs;

        evas_object_image_size_get(img, &w, &h);
        cs = evas_object_image_colorspace_get(img);
        if (cs == EVAS_COLORSPACE_ARGB8888)
           sz = w * h * 4;
        if ((cs == EVAS_COLORSPACE_YCBCR422P601_PL) ||
            (cs == EVAS_COLORSPACE_YCBCR422P709_PL))
           sz = h * 2 * sizeof(unsigned char *);
        if (sz != 0) memset(data, 0, sz);
     }
   evas_object_image_data_set(img, data);
}

/**
 * @brief Cancels ongoing EIO operations for extended attributes.
 *
 * If EIO is enabled and there are active load or save operations for
 * file extended attributes (like last playback position), this function
 * cancels them.
 *
 * @param xattr Pointer to the Emotion_Xattr_Data structure.
 */
static void
_xattr_data_cancel(Emotion_Xattr_Data *xattr)
{
   (void) xattr;
#ifdef HAVE_EIO
   /* Only cancel the load_xattr or we will loose ref to time_seek stringshare */
   if (xattr->load) eio_file_cancel(xattr->load);
   xattr->load = NULL;
   if (xattr->save) eio_file_cancel(xattr->save);
   xattr->save = NULL;
#endif
}

/**
 * @brief Decrements the reference count of Emotion_Xattr_Data and frees it if count reaches zero.
 *
 * Also cancels any pending EIO operations and removes the weak reference to the object.
 *
 * @param xattr Pointer to the Emotion_Xattr_Data structure to unreference.
 */
static void
_xattr_data_unref(Emotion_Xattr_Data *xattr)
{
   EINA_REFCOUNT_UNREF(xattr) {} else return;

   _xattr_data_cancel(xattr);
   efl_wref_del_safe(&xattr->obj_wref);
   free(xattr);
}

/**
 * @brief Updates the position and size of the video image and its clipper.
 *
 * This function is responsible for scaling and positioning the actual video
 * image (`sd->obj`) within the bounds of the Emotion object, taking into
 * account any cropping (`sd->crop`) and fill settings (`sd->fill`).
 * If cropping is active, it also manages the clipper Evas object.
 *
 * @param obj The Emotion Evas object.
 * @param x The target x-coordinate for the Emotion object.
 * @param y The target y-coordinate for the Emotion object.
 * @param w The target width for the Emotion object.
 * @param h The target height for the Emotion object.
 * @param vid_w The native width of the video stream.
 * @param vid_h The native height of the video stream.
 */
static void
_clipper_position_size_update(Evas_Object *obj, int x, int y, int w, int h, int vid_w, int vid_h)
{
   Efl_Canvas_Video_Data *sd;
   double scale_w, scale_h;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);

   if (vid_w == 0 || vid_h == 0)
     {
       evas_object_image_fill_set(sd->obj, 0, 0, 0, 0);
       evas_object_move(sd->obj, x, y);
       evas_object_resize(sd->obj, 0, 0);
       if (!sd->crop.clipper) return;
       evas_object_move(sd->crop.clipper, x, y);
       evas_object_resize(sd->crop.clipper, 0, 0);
     }
   else
     {
       scale_w = (double)w / (double)(vid_w - sd->crop.l - sd->crop.r);
       scale_h = (double)h / (double)(vid_h - sd->crop.t - sd->crop.b);

       if (sd->fill.w < 0 && sd->fill.h < 0)
         evas_object_image_fill_set(sd->obj, 0, 0, vid_w * scale_w, vid_h * scale_h);
       else
         evas_object_image_fill_set(sd->obj, 0, 0, sd->fill.w * w, sd->fill.h * h);
       evas_object_resize(sd->obj, vid_w * scale_w, vid_h * scale_h);
       evas_object_move(sd->obj, x - sd->crop.l * scale_w, y - sd->crop.t * scale_h);
       if (!sd->crop.clipper) return;
       evas_object_move(sd->crop.clipper, x, y);
       evas_object_resize(sd->crop.clipper, w, h);
     }
}

/*******************************/
/* Externally accessible calls */
/*******************************/


/**
 * @brief Adds a new Emotion video object to the given Evas canvas.
 *
 * This is the primary way to create an Emotion object.
 *
 * @param evas The Evas canvas to add the object to.
 * @return A new Emotion Evas_Object on success, or @c NULL on failure.
 *
 * @ingroup Emotion_Group_Basic
 */
EMOTION_API Evas_Object *
emotion_object_add(Evas *evas)
{
   evas = evas_find(evas);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(evas, EVAS_CANVAS_CLASS), NULL);
   return efl_add(MY_CLASS, evas, efl_canvas_object_legacy_ctor(efl_added));
}

EOLIAN static Eo *
_efl_canvas_video_efl_object_constructor(Eo *obj, Efl_Canvas_Video_Data *pd)
{
   efl_canvas_group_clipped_set(obj, EINA_TRUE);
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, E_OBJ_NAME);

   eina_stringshare_replace(&(pd->engine), "gstreamer1");
   pd->spu.button = -1;
   pd->ratio = 1.0;
   _engine_init(obj, pd);

   return obj;
}

/**
 * @brief Gets the underlying Evas image object used by Emotion for video display.
 *
 * This function returns the Evas_Object that Emotion uses internally to render
 * video frames. This can be useful for advanced manipulation or inspection, but
 * direct modification of this object is generally not recommended as it may
 * interfere with Emotion's operations.
 *
 * @param obj The Emotion object.
 * @return The internal Evas image object, or @c NULL if @p obj is invalid.
 *
 * @ingroup Emotion_Group_Advanced
 */
EMOTION_API Evas_Object *
emotion_object_image_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd = efl_data_scope_safe_get(obj, MY_CLASS);
   if (!sd) return NULL;
   return sd->obj;
}

EOLIAN static void
_efl_canvas_video_option_set(Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *pd, const char *opt, const char *val)
{
   Efl_Canvas_Video_Data *sd = pd;

   if ((!opt) || (!val)) return;

   if (strcmp(opt, "video") == 0)
     {
        if (strcmp(val, "off") == 0)
          sd->module_options.no_video = EINA_TRUE;
        else if (strcmp(val, "on") == 0)
          sd->module_options.no_video = EINA_FALSE;
        else
          sd->module_options.no_video = !!atoi(val);

        ERR("Deprecated. Use emotion_object_video_mute_set()");
     }
   else if (strcmp(opt, "audio") == 0)
     {
        if (strcmp(val, "off") == 0)
          sd->module_options.no_audio = EINA_TRUE;
        else if (strcmp(val, "on") == 0)
          sd->module_options.no_audio = EINA_FALSE;
        else
          sd->module_options.no_audio = !!atoi(val);

        ERR("Deprecated. Use emotion_object_audio_mute_set()");
     }
   else
     ERR("Unsupported %s=%s", opt, val);
}

EOLIAN static Eina_Bool
_efl_canvas_video_engine_set(Eo *obj, Efl_Canvas_Video_Data *pd, const char *engine)
{
   Efl_Canvas_Video_Data *sd = pd;
   const char *file;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);

   if (!engine) engine = "gstreamer1";
   if (!strcmp(engine, sd->engine)) return EINA_TRUE;

   eina_stringshare_replace(&(sd->engine), engine);

   file = sd->file;
   sd->file = NULL;

   eina_stringshare_del(sd->title);
   sd->title = NULL;
   eina_stringshare_del(sd->progress.info);
   sd->progress.info = NULL;
   sd->progress.stat = 0.0;
   eina_stringshare_del(sd->ref.file);
   sd->ref.file = NULL;
   sd->ref.num = 0;
   sd->spu.button_num = 0;
   sd->spu.button = -1;
   sd->ratio = 1.0;
   sd->pos = 0;
   sd->remember_jump = 0;
   sd->seek_pos = 0;
   sd->len = 0;
   sd->remember_play = 0;

   if (sd->anim) ecore_animator_del(sd->anim);
   sd->anim = NULL;

   if (sd->engine_instance) emotion_engine_instance_del(sd->engine_instance);
   sd->engine_instance = NULL;
   _engine_init(obj, sd);
   if (!sd->engine_instance)
     {
        sd->file = file;
        return EINA_FALSE;
     }

   if (file)
     {
        emotion_object_file_set(obj, file);
        eina_stringshare_del(file);
     }

   return EINA_TRUE;
}

/**
 * @brief Sets the media file to be played by the Emotion object.
 *
 * This function tells Emotion which file (or URI) to load and play.
 * The actual loading might be deferred.
 *
 * @param obj The Emotion object.
 * @param file The path or URI of the media file. For example, "/path/to/video.mp4" or "file:///path/to/video.ogv".
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 *
 * @ingroup Emotion_Group_File
 */
EMOTION_API Eina_Bool
emotion_object_file_set(Evas_Object *obj, const char *file)
{
   return efl_file_simple_load(obj, file, NULL);
}

/**
 * @brief Implements Efl.File.file_set for the Emotion object.
 * @internal
 *
 * Sets the file path for the video. This updates the internal file path
 * stringshare and marks the video as not loaded. It then calls the parent
 * class's implementation.
 *
 * @param obj The Emotion Eo object.
 * @param sd Pointer to the private data of the Emotion object.
 * @param file The file path to set.
 * @return An Eina_Error code, typically 0 on success.
 */
EOLIAN static Eina_Error
_efl_canvas_video_efl_file_file_set(Eo *obj, Efl_Canvas_Video_Data *sd, const char *file)
{
   DBG("file=%s", file);

   eina_stringshare_replace(&sd->file, file);
   sd->loaded = 0;
   return efl_file_set(efl_super(obj, MY_CLASS), file);
}

EOLIAN static Eina_Bool
_efl_canvas_video_efl_file_loaded_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   return sd->open && sd->loaded;
}

EOLIAN static void
_efl_canvas_video_efl_file_unload(Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
    if (sd->engine_instance) emotion_engine_instance_file_close(sd->engine_instance);
    sd->engine_instance = NULL;
    evas_object_image_data_set(sd->obj, NULL);
    evas_object_image_size_set(sd->obj, 1, 1);
    _emotion_image_data_zero(sd->obj);

   if (sd->anim) ecore_animator_del(sd->anim);
   sd->anim = NULL;

   _xattr_data_cancel(sd->xattr);
   sd->loaded = 0;
}

EOLIAN static Eina_Error
_efl_canvas_video_efl_file_load(Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   const char *file = sd->file;
   if (!sd->engine_instance) _engine_init(obj, sd);
   if (!sd->engine_instance)
     {
        WRN("No engine chosen. Please set an engine.");
        return EFL_GFX_IMAGE_LOAD_ERROR_GENERIC;
     }

   sd->video.w = 0;
   sd->video.h = 0;
   if ((file) && (file[0] != 0))
     {
        char *file2 = NULL;

        emotion_engine_instance_file_close(sd->engine_instance);
        evas_object_image_data_set(sd->obj, NULL);
        evas_object_image_size_set(sd->obj, 1, 1);
        _emotion_image_data_zero(sd->obj);
        sd->open = 0;

        if (file)
          {
             file2 = eina_vpath_resolve(file);
          }

        if (!emotion_engine_instance_file_open(sd->engine_instance, file2))
          {
             WRN("Couldn't open file=%s", sd->file);
             return EFL_GFX_IMAGE_LOAD_ERROR_GENERIC;
          }
        free(file2);
        DBG("successfully opened file=%s", sd->file);
        sd->pos = 0.0;
        if (sd->play) emotion_engine_instance_play(sd->engine_instance, 0.0);
     }
   else
     {
        emotion_engine_instance_file_close(sd->engine_instance);
        evas_object_image_data_set(sd->obj, NULL);
        evas_object_image_size_set(sd->obj, 1, 1);
        _emotion_image_data_zero(sd->obj);
     }

   if (sd->anim) ecore_animator_del(sd->anim);
   sd->anim = NULL;

   _xattr_data_cancel(sd->xattr);
   sd->loaded = 1;

   return 0;
}

/**
 * @brief Gets the media file currently set for the Emotion object.
 *
 * @param obj The Emotion object.
 * @return The path or URI of the current media file as a stringshared string,
 *         or @c NULL if no file is set or @p obj is invalid. The returned string
 *         should not be freed by the caller.
 *
 * @ingroup Emotion_Group_File
 */
EMOTION_API const char *
emotion_object_file_get(const Evas_Object *obj)
{
   return efl_file_get(obj);
}

/**
 * @brief Applies aspect ratio and border settings to the video display.
 * @internal
 *
 * This function is called when the object's geometry or aspect/border
 * settings change. It configures the clipper object if borders are used,
 * and then calls _clipper_position_size_update to adjust the video image.
 *
 * @param obj The Emotion Evas object.
 * @param sd Pointer to the private data of the Emotion object.
 * @param w The current width of the Emotion object.
 * @param h The current height of the Emotion object.
 * @param iw The native width of the video stream.
 * @param ih The native height of the video stream.
 */
static void
_emotion_aspect_borders_apply(Evas_Object *obj, Efl_Canvas_Video_Data *sd, int w, int h, int iw, int ih)
{
   int x, y;

   evas_object_geometry_get(obj, &x, &y, NULL, NULL);

   /* applying calculated borders */
   if ((sd->crop.l == 0) && (sd->crop.r == 0) &&
       (sd->crop.t == 0) && (sd->crop.b == 0))
     {
        Evas_Object *old_clipper;
        if (sd->crop.clipper)
          {
             old_clipper = evas_object_clip_get(sd->crop.clipper);
             evas_object_clip_unset(sd->obj);
             evas_object_clip_set(sd->obj, old_clipper);
             evas_object_del(sd->crop.clipper);
             sd->crop.clipper = NULL;
          }
     }
   else
     {
        if (!sd->crop.clipper)
          {
             Evas_Object *old_clipper;
             sd->crop.clipper = evas_object_rectangle_add
               (evas_object_evas_get(obj));
             evas_object_smart_member_add(sd->crop.clipper, obj);
             old_clipper = evas_object_clip_get(sd->obj);
             evas_object_clip_set(sd->obj, sd->crop.clipper);
             evas_object_clip_set(sd->crop.clipper, old_clipper);
             evas_object_show(sd->crop.clipper);
          }
     }
   _clipper_position_size_update(obj, x, y, w, h, iw, ih);
}

/**
 * @brief Calculates and applies borders based on aspect ratio settings.
 * @internal
 *
 * This function determines the necessary cropping borders (sd->crop.l, .r, .t, .b)
 * based on the current aspect ratio mode (sd->aspect), the object's dimensions (w, h),
 * and the video's native dimensions and aspect ratio. It then calls
 * _emotion_aspect_borders_apply to effect these changes.
 *
 * @param obj The Emotion Evas object.
 * @param sd Pointer to the private data of the Emotion object.
 * @param w The width of the Emotion object.
 * @param h The height of the Emotion object.
 */
static void
_efl_canvas_video_aspect_border_apply(Evas_Object *obj, Efl_Canvas_Video_Data *sd, int w, int h)
{
   int iw, ih;
   double ir;
   double r;

   int aspect_opt = 0;

   /* Prefer (if available) the video aspect ratio to calculate the sizes */
   if (sd->ratio > 0.0)
     {
        ir = sd->ratio;
        ih = sd->video.h;
        iw = (double)ih * ir;
     }
   else
     {
        iw = sd->video.w;
        ih = sd->video.h;
        ir = (double)iw / ih;
     }

   r = (double)w / h;

   /* First check if we should fit the width or height of the video inside the
    * width/height of the object.  This check takes into account the original
    * aspect ratio and the object aspect ratio, if we are keeping both sizes or
    * cropping the exceding area.
    */
   if (sd->aspect == EMOTION_ASPECT_KEEP_NONE)
     {
        sd->crop.l = 0;
        sd->crop.r = 0;
        sd->crop.t = 0;
        sd->crop.b = 0;
        aspect_opt = 0; // just ignore keep_aspect
     }
   else if (sd->aspect == EMOTION_ASPECT_KEEP_WIDTH)
     {
        aspect_opt = 1;
     }
   else if (sd->aspect == EMOTION_ASPECT_KEEP_HEIGHT)
     {
        aspect_opt = 2;
     }
   else if (sd->aspect == EMOTION_ASPECT_KEEP_BOTH)
     {
        if (ir > r) aspect_opt = 1;
        else aspect_opt = 2;
     }
   else if (sd->aspect == EMOTION_ASPECT_CROP)
     {
        if (ir > r) aspect_opt = 2;
        else aspect_opt = 1;
     }
   else if (sd->aspect == EMOTION_ASPECT_CUSTOM)
     {
        // nothing to do, just respect the border settings
        aspect_opt = 0;
     }

   /* updating borders based on keep_aspect settings */
   if (aspect_opt == 1) // keep width
     {
        int th, dh;
        double scale;

        sd->crop.l = 0;
        sd->crop.r = 0;
        scale = (double)iw / w;
        th = h * scale;
        dh = ih - th;
        sd->crop.t = sd->crop.b = dh / 2;
     }
   else if (aspect_opt == 2) // keep height
     {
        int tw, dw;
        double scale;

        sd->crop.t = 0;
        sd->crop.b = 0;
        scale = (double)ih / h;
        tw = w * scale;
        dw = iw - tw;
        sd->crop.l = sd->crop.r = dw / 2;
     }

   _emotion_aspect_borders_apply(obj, sd, w, h, iw, ih);
}

/**
 * @brief Sets custom borders for the video display.
 *
 * This function allows specifying how many pixels to crop from each side of
 * the video. Positive values crop from the edge, effectively zooming in.
 * Using this function sets the aspect mode to #EMOTION_ASPECT_CUSTOM.
 *
 * @param obj The Emotion object.
 * @param l Pixels to crop from the left.
 * @param r Pixels to crop from the right.
 * @param t Pixels to crop from the top.
 * @param b Pixels to crop from the bottom.
 *
 * @ingroup Emotion_Group_Aspect
 */
EMOTION_API void
emotion_object_border_set(Evas_Object *obj, int l, int r, int t, int b)
{
   Efl_Canvas_Video_Data *sd;
   int w, h;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);

   sd->aspect = EMOTION_ASPECT_CUSTOM;
   sd->crop.l = -l;
   sd->crop.r = -r;
   sd->crop.t = -t;
   sd->crop.b = -b;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   _efl_canvas_video_aspect_border_apply(obj, sd, w, h);
}

/**
 * @brief Gets the current custom borders for the video display.
 *
 * Retrieves the border values previously set by emotion_object_border_set().
 * Note that these values are the negative of the internal crop values.
 *
 * @param obj The Emotion object.
 * @param[out] l Pointer to store the left border value.
 * @param[out] r Pointer to store the right border value.
 * @param[out] t Pointer to store the top border value.
 * @param[out] b Pointer to store the bottom border value.
 *
 * @ingroup Emotion_Group_Aspect
 */
EMOTION_API void
emotion_object_border_get(const Evas_Object *obj, int *l, int *r, int *t, int *b)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   *l = -sd->crop.l;
   *r = -sd->crop.r;
   *t = -sd->crop.t;
   *b = -sd->crop.b;
}

/**
 * @brief Sets the background color of the Emotion object.
 *
 * This color is visible in areas not covered by the video, for example,
 * if the video's aspect ratio does not match the object's aspect ratio
 * and "letterboxing" or "pillarboxing" occurs.
 *
 * @param obj The Emotion object.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 *
 * @ingroup Emotion_Group_Display
 */
EMOTION_API void
emotion_object_bg_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   evas_object_color_set(sd->bg, r, g, b, a);
}

/**
 * @brief Gets the background color of the Emotion object.
 *
 * @param obj The Emotion object.
 * @param[out] r Pointer to store the red component.
 * @param[out] g Pointer to store the green component.
 * @param[out] b Pointer to store the blue component.
 * @param[out] a Pointer to store the alpha component.
 *
 * @ingroup Emotion_Group_Display
 */
EMOTION_API void
emotion_object_bg_color_get(const Evas_Object *obj, int *r, int *g, int *b, int *a)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   evas_object_color_get(sd->bg, r, g, b, a);
}

/**
 * @brief Sets the aspect ratio handling mode for the video.
 *
 * This determines how the video is scaled and displayed within the
 * Emotion object's bounds if their aspect ratios differ.
 *
 * @param obj The Emotion object.
 * @param a The desired #Emotion_Aspect mode.
 *
 * @ingroup Emotion_Group_Aspect
 */
EMOTION_API void
emotion_object_keep_aspect_set(Evas_Object *obj, Emotion_Aspect a)
{
   Efl_Canvas_Video_Data *sd;
   int w, h;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (a == sd->aspect) return;

   sd->aspect = a;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   _efl_canvas_video_aspect_border_apply(obj, sd, w, h);
}

/**
 * @brief Gets the current aspect ratio handling mode.
 *
 * @param obj The Emotion object.
 * @return The current #Emotion_Aspect mode.
 *
 * @ingroup Emotion_Group_Aspect
 */
EMOTION_API Emotion_Aspect
emotion_object_keep_aspect_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, EMOTION_ASPECT_KEEP_NONE);
   return sd->aspect;
}

/**
 * @brief Starts or stops video playback.
 *
 * This is a convenience function. Setting @p play to @c EINA_TRUE
 * is equivalent to `efl_player_playing_set(obj, EINA_TRUE)` followed by
 * `efl_player_paused_set(obj, EINA_FALSE)`.
 * Setting @p play to @c EINA_FALSE is equivalent to
 * `efl_player_playing_set(obj, EINA_FALSE)` (which also implies unpausing)
 * and then `efl_player_paused_set(obj, EINA_TRUE)` to effectively stop.
 *
 * @param obj The Emotion object.
 * @param play @c EINA_TRUE to start playback, @c EINA_FALSE to stop.
 *
 * @ingroup Emotion_Group_Playback
 */
EMOTION_API void
emotion_object_play_set(Evas_Object *obj, Eina_Bool play)
{
   /* avoid calling playback_position_set(0) for legacy */
   if (play)
     efl_player_playing_set(obj, EINA_TRUE);
   efl_player_paused_set(obj, !play);
}

/**
 * @brief Implements Efl.Player.playing_set for the Emotion object.
 * @internal
 *
 * Sets the playing state of the video. If play is true, it starts playback.
 * If play is false, it stops playback and resets the position to 0.
 * Handles remembering the play state if the file is not yet open.
 *
 * @param obj The Emotion Eo object.
 * @param sd Pointer to the private data of the Emotion object.
 * @param play The desired playing state.
 * @return @c EINA_TRUE if the state was successfully set or queued, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_canvas_video_efl_player_playing_set(Eo *obj, Efl_Canvas_Video_Data *sd, Eina_Bool play)
{
   play = !!play;
   DBG("play=" FMT_UCHAR ", was=" FMT_UCHAR, play, sd->play);
   if (!sd->engine_instance) return EINA_FALSE;
   /* always unset pause if playing is false */
   if (!play) sd->pause = EINA_FALSE;
   if (!sd->open)
     {
        sd->remember_play = play;
        return EINA_TRUE;
     }
   if (play == sd->play) return EINA_TRUE;
   sd->play = play;
   sd->remember_play = play;
   if (sd->state != EMOTION_WAKEUP) emotion_object_suspend_set(obj, EMOTION_WAKEUP);
   if (sd->play) emotion_engine_instance_play(sd->engine_instance, 0.0);
   else
     {
        emotion_engine_instance_stop(sd->engine_instance);
        efl_player_playback_position_set(obj, 0.0);
     }
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_canvas_video_efl_player_paused_set(Eo *obj, Efl_Canvas_Video_Data *sd, Eina_Bool paused)
{
   paused = !!paused;
   DBG("paused=" FMT_UCHAR ", was=" FMT_UCHAR, paused, sd->pause);
   if (!sd->engine_instance) return EINA_FALSE;
   if (!sd->open)
     {
        /* queue pause */
        if (sd->remember_play)
          sd->pause = paused;
        return sd->remember_play;
     }
   if (!sd->play) return EINA_FALSE;
   if (paused == sd->pause) return EINA_TRUE;
   sd->pause = paused;
   if (sd->pause)
     emotion_engine_instance_stop(sd->engine_instance);
   else
     {
        if (sd->state != EMOTION_WAKEUP) emotion_object_suspend_set(obj, EMOTION_WAKEUP);
        emotion_engine_instance_play(sd->engine_instance, sd->pos);
     }
   return EINA_TRUE;
}

/**
 * @brief Gets the current playback state of the video.
 *
 * @param obj The Emotion object.
 * @return @c EINA_TRUE if the video is currently playing (and not paused),
 *         @c EINA_FALSE otherwise.
 *
 * @ingroup Emotion_Group_Playback
 */
EMOTION_API Eina_Bool
emotion_object_play_get(const Evas_Object *obj)
{
   return efl_player_playing_get(obj) && !efl_player_paused_get(obj);
}

/**
 * @brief Implements Efl.Player.playing_get for the Emotion object.
 * @internal
 *
 * Returns the current playing state (sd->play).
 *
 * @param obj The Emotion Eo object (unused).
 * @param sd Pointer to the private data of the Emotion object.
 * @return @c EINA_TRUE if playing, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_canvas_video_efl_player_playing_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   if (!sd->engine_instance) return EINA_FALSE;
   return sd->play;
}

EOLIAN static Eina_Bool
_efl_canvas_video_efl_player_paused_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   if (!sd->engine_instance) return EINA_FALSE;
   if (!sd->play) return EINA_FALSE;
   return sd->pause;
}

/**
 * @brief Sets the current playback position of the video.
 *
 * Jumps to the specified time in the video.
 *
 * @param obj The Emotion object.
 * @param sec The desired position in seconds from the beginning of the video.
 *            Example: 120.5 for 2 minutes and 0.5 seconds.
 *
 * @ingroup Emotion_Group_Playback
 */
EMOTION_API void
emotion_object_position_set(Evas_Object *obj, double sec)
{
   efl_player_playback_position_set(obj, sec);
}

/**
 * @brief Implements Efl.Player.playback_position_set for the Emotion object.
 * @internal
 *
 * Sets the playback position. If the file is not open, remembers the jump position.
 * Otherwise, schedules a job to perform the seek operation.
 *
 * @param obj The Emotion Eo object.
 * @param sd Pointer to the private data of the Emotion object.
 * @param sec The target position in seconds.
 */
EOLIAN static void
_efl_canvas_video_efl_player_playback_position_set(Eo *obj, Efl_Canvas_Video_Data *sd, double sec)
{
   DBG("sec=%f", sec);
   if (!sd->engine_instance) return;
   if (sec < 0.0) sec = 0.0;
   if (!sd->open)
     {
        sd->remember_jump = sec;
        return;
     }
   sd->remember_jump = 0;
   sd->seek_pos = sec;
   sd->seek = 1;
   sd->pos = sd->seek_pos;
   if (sd->job) ecore_job_del(sd->job);
   sd->job = ecore_job_add(_pos_set_job, obj);
}

/**
 * @brief Gets the current playback position of the video.
 *
 * @param obj The Emotion object.
 * @return The current position in seconds from the beginning of the video.
 *         Example: 60.0 for 1 minute.
 *
 * @ingroup Emotion_Group_Playback
 */
EMOTION_API double
emotion_object_position_get(const Evas_Object *obj)
{
   return efl_player_playback_position_get(obj);
}

/**
 * @brief Implements Efl.Player.playback_position_get for the Emotion object.
 * @internal
 *
 * Retrieves the current playback position from the engine instance.
 *
 * @param obj The Emotion Eo object (unused).
 * @param sd Pointer to the private data of the Emotion object.
 * @return The current playback position in seconds.
 */
EOLIAN static double
_efl_canvas_video_efl_player_playback_position_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   if (!sd->engine_instance) return 0.0;
   sd->pos = emotion_engine_instance_pos_get(sd->engine_instance);
   return sd->pos;
}

/**
 * @brief Gets the current buffer fill status.
 *
 * This indicates how much of the media stream is buffered by the underlying
 * engine. A value of 1.0 means the buffer is full or buffering is complete
 * for the current segment.
 *
 * @param obj The Emotion object.
 * @return The buffer size as a fraction (0.0 to 1.0). Returns 0.0 if no
 *         engine instance is available.
 *
 * @ingroup Emotion_Group_Info
 */
EMOTION_API double
emotion_object_buffer_size_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 1.0);
   if (!sd->engine_instance) return 0.0;
   return emotion_engine_instance_buffer_size_get(sd->engine_instance);
}

/**
 * @brief Checks if the current media is seekable.
 *
 * @param obj The Emotion object.
 * @return @c EINA_TRUE if the media is seekable, @c EINA_FALSE otherwise
 *         (e.g., for a live stream or if no engine is active).
 *
 * @ingroup Emotion_Group_Info
 */
EMOTION_API Eina_Bool
emotion_object_seekable_get(const Evas_Object *obj)
{
   return efl_playable_seekable_get(obj);
}

/**
 * @brief Checks if the video stream is being handled by the current engine.
 *
 * Some media files might be audio-only, or the engine might not support
 * the video codec.
 *
 * @param obj The Emotion object.
 * @return @c EINA_TRUE if video is handled, @c EINA_FALSE otherwise or if no
 *         engine instance is available.
 *
 * @ingroup Emotion_Group_Info
 */
EMOTION_API Eina_Bool
emotion_object_video_handled_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->engine_instance) return EINA_FALSE;
   return emotion_engine_instance_video_handled(sd->engine_instance);
}

/**
 * @brief Checks if the audio stream is being handled by the current engine.
 *
 * Some media files might be video-only, or the engine might not support
 * the audio codec.
 *
 * @param obj The Emotion object.
 * @return @c EINA_TRUE if audio is handled, @c EINA_FALSE otherwise or if no
 *         engine instance is available.
 *
 * @ingroup Emotion_Group_Info
 */
EMOTION_API Eina_Bool
emotion_object_audio_handled_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->engine_instance) return EINA_FALSE;
   return emotion_engine_instance_audio_handled(sd->engine_instance);
}

/**
 * @brief Gets the total playback length (duration) of the video.
 *
 * @param obj The Emotion object.
 * @return The total length in seconds. Example: 300.0 for a 5-minute video.
 *         Returns 0.0 if the length is unknown or no engine is active.
 *
 * @ingroup Emotion_Group_Info
 */
EMOTION_API double
emotion_object_play_length_get(const Evas_Object *obj)
{
   return efl_playable_length_get(obj);
}

/**
 * @brief Gets the native size (resolution) of the video stream.
 *
 * @param obj The Emotion object.
 * @param[out] iw Pointer to store the native width of the video in pixels. Can be @c NULL.
 * @param[out] ih Pointer to store the native height of the video in pixels. Can be @c NULL.
 *
 * @ingroup Emotion_Group_Info
 */
EMOTION_API void
emotion_object_size_get(const Evas_Object *obj, int *iw, int *ih)
{
   Eina_Size2D sz;

   sz = efl_gfx_image_load_controller_load_size_get(obj);
   if (iw) *iw = sz.w;
   if (ih) *ih = sz.h;
}

/**
 * @brief Implements Efl.Gfx.ImageLoadController.load_size_get for Emotion.
 * @internal
 *
 * Returns the native video dimensions.
 *
 * @param obj The Emotion Eo object (unused).
 * @param sd Pointer to the private data of the Emotion object.
 * @return Eina_Size2D struct containing the video width and height.
 */
EOLIAN static Eina_Size2D
_efl_canvas_video_efl_gfx_image_load_controller_load_size_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   // FIXME: Shouldn't this be efl_gfx_view_size instead?
   return EINA_SIZE2D(sd->video.w, sd->video.h);
}

/**
 * @brief Enables or disables smooth scaling for the video.
 *
 * Smooth scaling typically provides better visual quality when the video is
 * scaled up or down, but may incur a performance cost.
 *
 * @param obj The Emotion object.
 * @param smooth @c EINA_TRUE to enable smooth scaling, @c EINA_FALSE to disable.
 *
 * @ingroup Emotion_Group_Display
 */
EMOTION_API void
emotion_object_smooth_scale_set(Evas_Object *obj, Eina_Bool smooth)
{
   efl_gfx_image_smooth_scale_set(obj, smooth);
}

/**
 * @brief Implements Efl.Gfx.Image.smooth_scale_set for Emotion.
 * @internal
 *
 * Sets the smooth scaling property on the internal Evas image object.
 *
 * @param obj The Emotion Eo object (unused).
 * @param sd Pointer to the private data of the Emotion object.
 * @param smooth The desired smooth scaling state.
 */
EOLIAN static void
_efl_canvas_video_efl_gfx_image_smooth_scale_set(Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd, Eina_Bool smooth)
{
   evas_object_image_smooth_scale_set(sd->obj, smooth);
}

/**
 * @brief Gets the current smooth scaling state for the video.
 *
 * @param obj The Emotion object.
 * @return @c EINA_TRUE if smooth scaling is enabled, @c EINA_FALSE otherwise.
 *
 * @ingroup Emotion_Group_Display
 */
EMOTION_API Eina_Bool
emotion_object_smooth_scale_get(const Evas_Object *obj)
{
   return efl_gfx_image_smooth_scale_get(obj);
}

/**
 * @brief Implements Efl.Gfx.Image.smooth_scale_get for Emotion.
 * @internal
 *
 * Gets the smooth scaling property from the internal Evas image object.
 *
 * @param obj The Emotion Eo object (unused).
 * @param sd Pointer to the private data of the Emotion object.
 * @return The current smooth scaling state.
 */
EOLIAN static Eina_Bool
_efl_canvas_video_efl_gfx_image_smooth_scale_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   return evas_object_image_smooth_scale_get(sd->obj);
}

/**
 * @brief Gets the pixel aspect ratio of the video.
 *
 * This is the ratio of width to height of a single pixel in the video stream.
 * For most modern digital video, this is 1.0 (square pixels).
 *
 * @param obj The Emotion object.
 * @return The pixel aspect ratio (width/height). Returns 0.0 if no engine
 *         instance is available.
 *
 * @ingroup Emotion_Group_Info
 */
EMOTION_API double
emotion_object_ratio_get(const Evas_Object *obj)
{
   return efl_gfx_image_ratio_get(obj);
}

/**
 * @brief Implements Efl.Gfx.Image.ratio_get for Emotion.
 * @internal
 *
 * Returns the stored pixel aspect ratio (sd->ratio).
 *
 * @param obj The Emotion Eo object (unused).
 * @param sd Pointer to the private data of the Emotion object.
 * @return The pixel aspect ratio.
 */
EOLIAN static double
_efl_canvas_video_efl_gfx_image_ratio_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   if (!sd->engine_instance) return 0.0;
   return sd->ratio;
}

/**
 * @brief Sends a simple event to the media player.
 *
 * This is typically used for DVD-like navigation events (e.g., menu, next, prev).
 * The available events and their effects depend on the underlying media and engine.
 *
 * @param obj The Emotion object.
 * @param ev The #Emotion_Event to send.
 *           Example: #EMOTION_EVENT_PREV for previous chapter/track.
 *
 * @ingroup Emotion_Group_Control
 */
EMOTION_API void
emotion_object_event_simple_send(Evas_Object *obj, Emotion_Event ev)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (!sd->engine_instance) return;
   emotion_engine_instance_event_feed(sd->engine_instance, ev);
}

/**
 * @brief Sets the audio volume.
 *
 * @param obj The Emotion object.
 * @param vol The desired volume level, from 0.0 (mute) to 1.0 (full volume).
 *            Values outside this range may be clamped by the engine.
 *            Example: 0.5 for 50% volume.
 *
 * @ingroup Emotion_Group_Audio
 */
EMOTION_API void
emotion_object_audio_volume_set(Evas_Object *obj, double vol)
{
   efl_audio_control_volume_set(obj, vol);
}

/**
 * @brief Implements Efl.Audio.Control.volume_set for Emotion.
 * @internal
 *
 * Sets the audio volume on the engine instance.
 *
 * @param obj The Emotion Eo object (unused).
 * @param sd Pointer to the private data of the Emotion object.
 * @param vol The volume level (0.0 to 1.0).
 */
EOLIAN static void
_efl_canvas_video_efl_audio_control_volume_set(Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd, double vol)
{
   DBG("vol=%f", vol);
   if (!sd->engine_instance) return;
   emotion_engine_instance_audio_channel_volume_set(sd->engine_instance, vol);
}

/**
 * @brief Gets the current audio volume.
 *
 * @param obj The Emotion object.
 * @return The current volume level (0.0 to 1.0).
 *
 * @ingroup Emotion_Group_Audio
 */
EMOTION_API double
emotion_object_audio_volume_get(const Evas_Object *obj)
{
   return efl_audio_control_volume_get(obj);
}

/**
 * @brief Implements Efl.Audio.Control.volume_get for Emotion.
 * @internal
 *
 * Gets the audio volume from the engine instance.
 *
 * @param obj The Emotion Eo object (unused).
 * @param sd Pointer to the private data of the Emotion object.
 * @return The current volume level.
 */
EOLIAN static double
_efl_canvas_video_efl_audio_control_volume_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   if (!sd->engine_instance) return 0.0;
   return emotion_engine_instance_audio_channel_volume_get(sd->engine_instance);
}

/**
 * @brief Sets the audio mute state.
 *
 * @param obj The Emotion object.
 * @param mute @c EINA_TRUE to mute audio, @c EINA_FALSE to unmute.
 *
 * @ingroup Emotion_Group_Audio
 */
EMOTION_API void
emotion_object_audio_mute_set(Evas_Object *obj, Eina_Bool mute)
{
   efl_audio_control_mute_set(obj, mute);
}

/**
 * @brief Implements Efl.Audio.Control.mute_set for Emotion.
 * @internal
 *
 * Sets the audio mute state on the engine instance.
 *
 * @param obj The Emotion Eo object (unused).
 * @param sd Pointer to the private data of the Emotion object.
 * @param mute The desired mute state.
 */
EOLIAN static void
_efl_canvas_video_efl_audio_control_mute_set(Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd, Eina_Bool mute)
{
   DBG("mute=" FMT_UCHAR, mute);
   if (!sd->engine_instance) return;
   emotion_engine_instance_audio_channel_mute_set(sd->engine_instance, mute);
}

/**
 * @brief Gets the current audio mute state.
 *
 * @param obj The Emotion object.
 * @return @c EINA_TRUE if audio is muted, @c EINA_FALSE otherwise.
 *
 * @ingroup Emotion_Group_Audio
 */
EMOTION_API Eina_Bool
emotion_object_audio_mute_get(const Evas_Object *obj)
{
   return efl_audio_control_mute_get(obj);
}

/**
 * @brief Implements Efl.Audio.Control.mute_get for Emotion.
 * @internal
 *
 * Gets the audio mute state from the engine instance.
 *
 * @param obj The Emotion Eo object (unused).
 * @param sd Pointer to the private data of the Emotion object.
 * @return The current mute state.
 */
EOLIAN static Eina_Bool
_efl_canvas_video_efl_audio_control_mute_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   if (!sd->engine_instance) return EINA_FALSE;
   return emotion_engine_instance_audio_channel_mute_get(sd->engine_instance);
}

/**
 * @brief Gets the number of available audio channels (tracks).
 *
 * Some media files may contain multiple audio tracks (e.g., different languages).
 *
 * @param obj The Emotion object.
 * @return The number of audio channels. Returns 0 if no engine instance or
 *         if channel information is unavailable.
 *
 * @ingroup Emotion_Group_Audio
 */
EMOTION_API int
emotion_object_audio_channel_count(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->engine_instance) return 0;
   return emotion_engine_instance_audio_channel_count(sd->engine_instance);
}

/**
 * @brief Gets the name of a specific audio channel (track).
 *
 * @param obj The Emotion object.
 * @param channel The index of the audio channel (0 to count-1).
 *                Example: 0 for the first audio track.
 * @return The name of the audio channel (e.g., "English", "Stereo"), or @c NULL
 *         if the channel index is invalid, no engine instance, or name is unavailable.
 *         The returned string is managed by Emotion and should not be freed.
 *
 * @ingroup Emotion_Group_Audio
 */
EMOTION_API const char *
emotion_object_audio_channel_name_get(const Evas_Object *obj, int channel)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   if (!sd->engine_instance) return NULL;
   return emotion_engine_instance_audio_channel_name_get(sd->engine_instance, channel);
}

/**
 * @brief Sets the currently active audio channel (track).
 *
 * @param obj The Emotion object.
 * @param channel The index of the audio channel to activate.
 *                Example: 1 to switch to the second audio track.
 *
 * @ingroup Emotion_Group_Audio
 */
EMOTION_API void
emotion_object_audio_channel_set(Evas_Object *obj, int channel)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("channel=%d", channel);
   if (!sd->engine_instance) return;
   emotion_engine_instance_audio_channel_set(sd->engine_instance, channel);
}

/**
 * @brief Gets the currently active audio channel (track) index.
 *
 * @param obj The Emotion object.
 * @return The index of the current audio channel. Returns 0 if no engine
 *         instance or if the current channel cannot be determined.
 *
 * @ingroup Emotion_Group_Audio
 */
EMOTION_API int
emotion_object_audio_channel_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->engine_instance) return 0;
   return emotion_engine_instance_audio_channel_get(sd->engine_instance);
}

/**
 * @brief Sets the video mute state.
 *
 * Muting video typically means the video track is still processed (e.g., for timing)
 * but not rendered to the screen. This can be used to make a video player "audio-only".
 *
 * @param obj The Emotion object.
 * @param mute @c EINA_TRUE to mute video, @c EINA_FALSE to unmute.
 *
 * @ingroup Emotion_Group_Video
 */
EMOTION_API void
emotion_object_video_mute_set(Evas_Object *obj, Eina_Bool mute)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("mute=" FMT_UCHAR, mute);
   if (!sd->engine_instance) return;
   emotion_engine_instance_video_channel_mute_set(sd->engine_instance, mute);
}

/**
 * @brief Gets the current video mute state.
 *
 * @param obj The Emotion object.
 * @return @c EINA_TRUE if video is muted, @c EINA_FALSE otherwise.
 *
 * @ingroup Emotion_Group_Video
 */
EMOTION_API Eina_Bool
emotion_object_video_mute_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->engine_instance) return EINA_FALSE;
   return emotion_engine_instance_video_channel_mute_get(sd->engine_instance);
}

/**
 * @brief Sets the subtitle file to be used for the current video.
 *
 * @param obj The Emotion object.
 * @param filepath Path to the subtitle file (e.g., ".srt", ".ssa").
 *                 Set to @c NULL to disable external subtitles.
 *                 Example: "/path/to/subtitles.srt".
 *
 * @ingroup Emotion_Group_Subtitle
 */
EMOTION_API void
emotion_object_video_subtitle_file_set(Evas_Object *obj, const char *filepath)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("subtitle=%s", filepath);
   if (!sd->engine_instance) _engine_init(obj, sd);
   if (!sd->engine_instance) return;
   emotion_engine_instance_video_subtitle_file_set(sd->engine_instance, filepath);
}

/**
 * @brief Gets the path of the currently set subtitle file.
 *
 * @param obj The Emotion object.
 * @return The path to the subtitle file, or @c NULL if none is set or
 *         no engine instance is available. The returned string is managed by
 *         Emotion and should not be freed.
 *
 * @ingroup Emotion_Group_Subtitle
 */
EMOTION_API const char *
emotion_object_video_subtitle_file_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->engine_instance) return NULL;
   return emotion_engine_instance_video_subtitle_file_get(sd->engine_instance);
}

/**
 * @brief Gets the number of available video channels (tracks).
 *
 * While less common than multiple audio tracks, some media might offer
 * multiple video angles or versions.
 *
 * @param obj The Emotion object.
 * @return The number of video channels. Returns 0 if no engine instance or
 *         if channel information is unavailable.
 *
 * @ingroup Emotion_Group_Video
 */
EMOTION_API int
emotion_object_video_channel_count(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->engine_instance) return 0;
   return emotion_engine_instance_video_channel_count(sd->engine_instance);
}

/**
 * @brief Gets the name of a specific video channel (track).
 *
 * @param obj The Emotion object.
 * @param channel The index of the video channel (0 to count-1).
 * @return The name of the video channel, or @c NULL if the index is invalid,
 *         no engine instance, or name is unavailable. The returned string is
 *         managed by Emotion and should not be freed.
 *
 * @ingroup Emotion_Group_Video
 */
EMOTION_API const char *
emotion_object_video_channel_name_get(const Evas_Object *obj, int channel)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   if (!sd->engine_instance) return NULL;
   return emotion_engine_instance_video_channel_name_get(sd->engine_instance, channel);
}

/**
 * @brief Sets the currently active video channel (track).
 *
 * @param obj The Emotion object.
 * @param channel The index of the video channel to activate.
 *
 * @ingroup Emotion_Group_Video
 */
EMOTION_API void
emotion_object_video_channel_set(Evas_Object *obj, int channel)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("channel=%d", channel);
   if (!sd->engine_instance) return;
   emotion_engine_instance_video_channel_set(sd->engine_instance, channel);
}

/**
 * @brief Gets the currently active video channel (track) index.
 *
 * @param obj The Emotion object.
 * @return The index of the current video channel. Returns 0 if no engine
 *         instance or if the current channel cannot be determined.
 *
 * @ingroup Emotion_Group_Video
 */
EMOTION_API int
emotion_object_video_channel_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->engine_instance) return 0;
   return emotion_engine_instance_video_channel_get(sd->engine_instance);
}

/**
 * @brief Sets the mute state for SPU (Subpicture Unit / Subtitles).
 *
 * This controls the visibility of embedded subtitles or DVD subpictures.
 *
 * @param obj The Emotion object.
 * @param mute @c EINA_TRUE to mute (hide) SPU, @c EINA_FALSE to unmute (show).
 *
 * @ingroup Emotion_Group_Subtitle
 */
EMOTION_API void
emotion_object_spu_mute_set(Evas_Object *obj, Eina_Bool mute)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("mute=" FMT_UCHAR, mute);
   if (!sd->engine_instance) return;
   emotion_engine_instance_spu_channel_mute_set(sd->engine_instance, mute);
}

/**
 * @brief Gets the current mute state for SPU (Subpicture Unit / Subtitles).
 *
 * @param obj The Emotion object.
 * @return @c EINA_TRUE if SPU is muted, @c EINA_FALSE otherwise.
 *
 * @ingroup Emotion_Group_Subtitle
 */
EMOTION_API Eina_Bool
emotion_object_spu_mute_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->engine_instance) return EINA_FALSE;
   return emotion_engine_instance_spu_channel_mute_get(sd->engine_instance);
}

/**
 * @brief Gets the number of available SPU (subtitle) channels.
 *
 * Media like DVDs can have multiple subtitle tracks.
 *
 * @param obj The Emotion object.
 * @return The number of SPU channels. Returns 0 if no engine instance or
 *         if channel information is unavailable.
 *
 * @ingroup Emotion_Group_Subtitle
 */
EMOTION_API int
emotion_object_spu_channel_count(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->engine_instance) return 0;
   return emotion_engine_instance_spu_channel_count(sd->engine_instance);
}

/**
 * @brief Gets the name of a specific SPU (subtitle) channel.
 *
 * @param obj The Emotion object.
 * @param channel The index of the SPU channel (0 to count-1).
 * @return The name of the SPU channel (e.g., "English subtitles"), or @c NULL
 *         if the index is invalid, no engine instance, or name is unavailable.
 *         The returned string is managed by Emotion and should not be freed.
 *
 * @ingroup Emotion_Group_Subtitle
 */
EMOTION_API const char *
emotion_object_spu_channel_name_get(const Evas_Object *obj, int channel)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   if (!sd->engine_instance) return NULL;
   return emotion_engine_instance_spu_channel_name_get(sd->engine_instance, channel);
}

/**
 * @brief Sets the currently active SPU (subtitle) channel.
 *
 * @param obj The Emotion object.
 * @param channel The index of the SPU channel to activate.
 *                Example: 0 to select the first subtitle track.
 *
 * @ingroup Emotion_Group_Subtitle
 */
EMOTION_API void
emotion_object_spu_channel_set(Evas_Object *obj, int channel)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("channel=%d", channel);
   if (!sd->engine_instance) return;
   emotion_engine_instance_spu_channel_set(sd->engine_instance, channel);
}

/**
 * @brief Gets the currently active SPU (subtitle) channel index.
 *
 * @param obj The Emotion object.
 * @return The index of the current SPU channel. Returns 0 if no engine
 *         instance or if the current channel cannot be determined.
 *
 * @ingroup Emotion_Group_Subtitle
 */
EMOTION_API int
emotion_object_spu_channel_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->engine_instance) return 0;
   return emotion_engine_instance_spu_channel_get(sd->engine_instance);
}

/**
 * @brief Gets the number of chapters in the current media.
 *
 * This is common for DVDs or media files with chapter markers.
 *
 * @param obj The Emotion object.
 * @return The number of chapters. Returns 0 if no engine instance or
 *         if chapter information is unavailable.
 *
 * @ingroup Emotion_Group_Chapter
 */
EMOTION_API int
emotion_object_chapter_count(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->engine_instance) return 0;
   return emotion_engine_instance_chapter_count(sd->engine_instance);
}

/**
 * @brief Sets the current chapter.
 *
 * Playback will jump to the beginning of the specified chapter.
 *
 * @param obj The Emotion object.
 * @param chapter The chapter number to set (usually 0-indexed or 1-indexed
 *                depending on the engine/media).
 *                Example: 0 for the first chapter.
 *
 * @ingroup Emotion_Group_Chapter
 */
EMOTION_API void
emotion_object_chapter_set(Evas_Object *obj, int chapter)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("chapter=%d", chapter);
   if (!sd->engine_instance) return;
   emotion_engine_instance_chapter_set(sd->engine_instance, chapter);
}

/**
 * @brief Gets the current chapter number.
 *
 * @param obj The Emotion object.
 * @return The current chapter number. Returns 0 if no engine instance or
 *         if the current chapter cannot be determined.
 *
 * @ingroup Emotion_Group_Chapter
 */
EMOTION_API int
emotion_object_chapter_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   if (!sd->engine_instance) return 0;
   return emotion_engine_instance_chapter_get(sd->engine_instance);
}

/**
 * @brief Gets the name of a specific chapter.
 *
 * @param obj The Emotion object.
 * @param chapter The chapter number.
 * @return The name of the chapter, or @c NULL if the chapter number is invalid,
 *         no engine instance, or name is unavailable. The returned string is
 *         managed by Emotion and should not be freed.
 *
 * @ingroup Emotion_Group_Chapter
 */
EMOTION_API const char *
emotion_object_chapter_name_get(const Evas_Object *obj, int chapter)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   if (!sd->engine_instance) return NULL;
   return emotion_engine_instance_chapter_name_get(sd->engine_instance, chapter);
}

/**
 * @brief Sets the playback speed.
 *
 * @param obj The Emotion object.
 * @param speed The desired playback speed. 1.0 is normal speed.
 *              Values > 1.0 for faster playback, < 1.0 for slower.
 *              Negative values might be supported for reverse playback by some engines.
 *              Example: 2.0 for double speed, 0.5 for half speed.
 *
 * @ingroup Emotion_Group_Playback
 */
EMOTION_API void
emotion_object_play_speed_set(Evas_Object *obj, double speed)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("speed=%f", speed);
   if (!sd->engine_instance) return;
   emotion_engine_instance_speed_set(sd->engine_instance, speed);
}

/**
 * @brief Gets the current playback speed.
 *
 * @param obj The Emotion object.
 * @return The current playback speed. 1.0 is normal.
 *         Returns 0.0 if no engine instance.
 *
 * @ingroup Emotion_Group_Playback
 */
EMOTION_API double
emotion_object_play_speed_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0.0);
   if (!sd->engine_instance) return 0.0;
   return emotion_engine_instance_speed_get(sd->engine_instance);
}

/**
 * @brief Ejects the current media.
 *
 * This is typically applicable to physical media like DVDs or CDs.
 * The effect on file-based playback may vary by engine.
 *
 * @param obj The Emotion object.
 *
 * @ingroup Emotion_Group_Control
 */
EMOTION_API void
emotion_object_eject(Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (!sd->engine_instance) return;
   emotion_engine_instance_eject(sd->engine_instance);
}

/**
 * @brief Gets the title of the current media.
 *
 * This is usually read from the media's metadata.
 *
 * @param obj The Emotion object.
 * @return The title string, or @c NULL if not available or no object.
 *         The returned string is stringshared and should not be freed.
 *
 * @ingroup Emotion_Group_Meta
 */
EMOTION_API const char *
emotion_object_title_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   return sd->title;
}

/**
 * @brief Gets a human-readable string describing the current progress.
 *
 * This might be something like "Buffering 50%" or "Downloading chapter 2".
 * The format and content depend on the engine.
 *
 * @param obj The Emotion object.
 * @return The progress information string, or @c NULL if not available.
 *         The returned string is stringshared and should not be freed.
 *
 * @ingroup Emotion_Group_Info
 */
EMOTION_API const char *
emotion_object_progress_info_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   return sd->progress.info;
}

/**
 * @brief Gets the numerical status of the current progress.
 *
 * This is typically a value from 0.0 to 1.0, where 1.0 means completion.
 * The exact meaning (e.g., buffering progress, download progress) depends
 * on the engine and the current operation.
 *
 * @param obj The Emotion object.
 * @return The progress status (0.0 to 1.0).
 *
 * @ingroup Emotion_Group_Info
 */
EMOTION_API double
emotion_object_progress_status_get(const Evas_Object *obj)
{
   return efl_player_playback_progress_get(obj);
}

/**
 * @brief Implements Efl.Player.playback_progress_get for Emotion.
 * @internal
 *
 * Returns the stored progress status (sd->progress.stat).
 *
 * @param obj The Emotion Eo object (unused).
 * @param sd Pointer to the private data of the Emotion object.
 * @return The progress status.
 */
EOLIAN static double
_efl_canvas_video_efl_player_playback_progress_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   return sd->progress.stat;
}

/**
 * @brief Implements Efl.Player.playback_progress_set for Emotion.
 * @internal
 *
 * This is called by the Efl.Player interface when progress is updated.
 * It internally calls _emotion_progress_set to update the smart data and
 * emit signals.
 *
 * @param obj The Emotion Eo object.
 * @param sd Pointer to the private data of the Emotion object (unused here, but part of signature).
 * @param progress The new progress value.
 */
EOLIAN static void
_efl_canvas_video_efl_player_playback_progress_set(Eo *obj, Efl_Canvas_Video_Data *sd EINA_UNUSED, double progress)
{
   const char *info = emotion_object_progress_info_get((const Evas_Object*)obj);
   _emotion_progress_set(obj, (char*)info, progress);
}

/**
 * @brief Implements Efl.Playable.length_get for Emotion.
 * @internal
 *
 * Retrieves the media length from the engine instance and updates sd->len.
 *
 * @param obj The Emotion Eo object (unused).
 * @param sd Pointer to the private data of the Emotion object.
 * @return The length of the media in seconds.
 */
EOLIAN static double
_efl_canvas_video_efl_playable_length_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   if (!sd->engine_instance) return 0.0;
   sd->len = emotion_engine_instance_len_get(sd->engine_instance);
   return sd->len;
}

/**
 * @brief Implements Efl.Playable.seekable_get for Emotion.
 * @internal
 *
 * Checks if the current media is seekable via the engine instance.
 *
 * @param obj The Emotion Eo object (unused).
 * @param sd Pointer to the private data of the Emotion object.
 * @return @c EINA_TRUE if seekable, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_canvas_video_efl_playable_seekable_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   if (!sd->engine_instance) return EINA_FALSE;
   return emotion_engine_instance_seekable(sd->engine_instance);
}

/**
 * @brief Gets the referenced file path, if any.
 *
 * Some media (e.g., playlists, streaming manifests) might refer to other files.
 * This function retrieves the path of such a referenced file.
 *
 * @param obj The Emotion object.
 * @return The path of the referenced file, or @c NULL if none.
 *         The returned string is stringshared and should not be freed.
 *
 * @ingroup Emotion_Group_Info
 */
EMOTION_API const char *
emotion_object_ref_file_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   return sd->ref.file;
}

/**
 * @brief Gets the reference number associated with a referenced file.
 *
 * This is often used in conjunction with emotion_object_ref_file_get()
 * to identify a specific item in a list of referenced files (e.g., track
 * number in a playlist).
 *
 * @param obj The Emotion object.
 * @return The reference number.
 *
 * @ingroup Emotion_Group_Info
 */
EMOTION_API int
emotion_object_ref_num_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   return sd->ref.num;
}

/**
 * @brief Gets the number of SPU (Subpicture Unit) buttons available.
 *
 * This is relevant for interactive DVD menus where subpictures act as buttons.
 *
 * @param obj The Emotion object.
 * @return The number of SPU buttons.
 *
 * @ingroup Emotion_Group_DVD
 */
EMOTION_API int
emotion_object_spu_button_count_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   return sd->spu.button_num;
}

/**
 * @brief Gets the currently highlighted or active SPU button.
 *
 * This is relevant for interactive DVD menus.
 *
 * @param obj The Emotion object.
 * @return The index of the current SPU button, or -1 if none.
 *
 * @ingroup Emotion_Group_DVD
 */
EMOTION_API int
emotion_object_spu_button_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, 0);
   return sd->spu.button;
}

/**
 * @brief Gets various metadata information about the current media.
 *
 * @param obj The Emotion object.
 * @param meta The type of #Emotion_Meta_Info to retrieve.
 *             Example: #EMOTION_META_INFO_TRACK_ARTIST to get the artist name.
 * @return A string containing the requested metadata, or @c NULL if not available
 *         or an invalid meta type is requested. The returned string is managed
 *         by Emotion and should not be freed.
 *
 * @ingroup Emotion_Group_Meta
 */
EMOTION_API const char *
emotion_object_meta_info_get(const Evas_Object *obj, Emotion_Meta_Info meta)
{
   Efl_Canvas_Video_Data *sd;
   int id;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   if (!sd->engine_instance) return NULL;
   switch (meta)
     {
      case EMOTION_META_INFO_TRACK_TITLE:
         id = META_TRACK_TITLE;
         break;
      case EMOTION_META_INFO_TRACK_ARTIST:
         id = META_TRACK_ARTIST;
         break;
      case EMOTION_META_INFO_TRACK_ALBUM:
         id = META_TRACK_ALBUM;
         break;
      case EMOTION_META_INFO_TRACK_YEAR:
         id = META_TRACK_YEAR;
         break;
      case EMOTION_META_INFO_TRACK_GENRE:
         id = META_TRACK_GENRE;
         break;
      case EMOTION_META_INFO_TRACK_COMMENT:
         id = META_TRACK_COMMENT;
         break;
      case EMOTION_META_INFO_TRACK_DISC_ID:
         id = META_TRACK_DISCID;
        break;
      default:
         ERR("Unknown meta info id: %d", meta);
         return NULL;
     }

   return emotion_engine_instance_meta_get(sd->engine_instance, id);
}

/**
 * @brief Retrieves artwork associated with a media file.
 *
 * This function attempts to extract artwork (e.g., album cover) from the
 * media file specified by @p path. The type of artwork to retrieve is
 * specified by @p type.
 *
 * @param obj The Emotion object (used to get the Evas canvas).
 * @param path The path to the media file from which to extract artwork.
 *             Example: "/path/to/music.mp3".
 * @param type The #Emotion_Artwork_Info type of artwork to retrieve (e.g., front cover).
 * @return A new Evas_Object (image) containing the artwork on success,
 *         or @c NULL on failure (e.g., artwork not found, load error).
 *         The caller is responsible for deleting the returned Evas_Object
 *         when no longer needed.
 *
 * @ingroup Emotion_Group_Meta
 */
EMOTION_API Evas_Object *
emotion_file_meta_artwork_get(const Evas_Object *obj, const char *path, Emotion_Artwork_Info type)
{
   Efl_Canvas_Video_Data *sd;
   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   if (!sd->engine_instance) return NULL;

   Evas *ev = evas_object_evas_get(obj);
   Evas_Object *artwork = evas_object_image_add(ev);

   Evas_Object *result = emotion_engine_instance_meta_artwork_get(sd->engine_instance, artwork, path, type);
   if (!result) return NULL;

   Evas_Load_Error _error = evas_object_image_load_error_get(result);
   if (_error != EVAS_LOAD_ERROR_NONE) return NULL;

   return result;
}

/**
 * @brief Sets the audio visualization to be used.
 *
 * When playing audio-only content or when video is muted, Emotion can
 * display visualizations (e.g., spectrum analyzer) if supported by the engine.
 *
 * @param obj The Emotion object.
 * @param visualization The #Emotion_Vis type to set.
 *                      Use #EMOTION_VIS_NONE to disable visualization.
 *
 * @ingroup Emotion_Group_Vis
 */
EMOTION_API void
emotion_object_vis_set(Evas_Object *obj, Emotion_Vis visualization)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   DBG("visualization=%d", visualization);
   if (!sd->engine_instance) return;
   emotion_engine_instance_vis_set(sd->engine_instance, visualization);
}

/**
 * @brief Gets the currently active audio visualization.
 *
 * @param obj The Emotion object.
 * @return The current #Emotion_Vis type, or #EMOTION_VIS_NONE if disabled
 *         or no engine instance.
 *
 * @ingroup Emotion_Group_Vis
 */
EMOTION_API Emotion_Vis
emotion_object_vis_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, EMOTION_VIS_NONE);
   if (!sd->engine_instance) return EMOTION_VIS_NONE;
   return emotion_engine_instance_vis_get(sd->engine_instance);
}

/**
 * @brief Checks if a specific audio visualization is supported by the current engine.
 *
 * @param obj The Emotion object.
 * @param visualization The #Emotion_Vis type to check.
 * @return @c EINA_TRUE if the visualization is supported, @c EINA_FALSE otherwise
 *         or if no engine instance.
 *
 * @ingroup Emotion_Group_Vis
 */
EMOTION_API Eina_Bool
emotion_object_vis_supported(const Evas_Object *obj, Emotion_Vis visualization)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, EINA_FALSE);
   if (!sd->engine_instance) return EINA_FALSE;
   return emotion_engine_instance_vis_supported(sd->engine_instance, visualization);
}

/**
 * @brief Sets the priority of the Emotion object for resource allocation.
 *
 * If @p priority is set to @c EINA_TRUE, the underlying engine may try to
 * allocate more resources or give higher scheduling priority to this object,
 * potentially at the expense of other, lower-priority Emotion objects or
 * applications. The exact behavior is engine-dependent.
 *
 * @param obj The Emotion object.
 * @param priority @c EINA_TRUE to set high priority, @c EINA_FALSE for normal.
 *
 * @ingroup Emotion_Group_Advanced
 */
EMOTION_API void
emotion_object_priority_set(Evas_Object *obj, Eina_Bool priority)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (!sd->engine_instance) return;
   emotion_engine_instance_priority_set(sd->engine_instance, priority);
}

/**
 * @brief Gets the current priority setting of the Emotion object.
 *
 * @param obj The Emotion object.
 * @return @c EINA_TRUE if high priority is set, @c EINA_FALSE otherwise.
 *
 * @ingroup Emotion_Group_Advanced
 */
EMOTION_API Eina_Bool
emotion_object_priority_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, EINA_FALSE);
   if (!sd->engine_instance) return EINA_FALSE;
   return emotion_engine_instance_priority_get(sd->engine_instance);
}

#ifdef HAVE_EIO
/**
 * @brief Cleans up resources after an EIO xattr load operation.
 * @internal
 *
 * Nullifies the Eio_File handle in the xattr data and unreferences the xattr data.
 *
 * @param xattr The Emotion_Xattr_Data associated with the operation.
 * @param handler The Eio_File handle for the completed operation.
 */
static void
_eio_load_xattr_cleanup(Emotion_Xattr_Data *xattr, Eio_File *handler)
{
   if (handler == xattr->load) xattr->load = NULL;
   _xattr_data_unref(xattr);
}

/**
 * @brief Callback for successful EIO xattr load operation.
 * @internal
 *
 * Sets the object's position to the loaded xattr value (last saved position)
 * and emits success signals/callbacks. Then cleans up.
 *
 * @param data User data (Emotion_Xattr_Data pointer).
 * @param handler The Eio_File handle for the operation.
 * @param xattr_double The double value read from the extended attribute.
 */
static void
_eio_load_xattr_done(void *data, Eio_File *handler, double xattr_double)
{
   Emotion_Xattr_Data *xattr = data;

   emotion_object_position_set(evas_object_smart_parent_get(xattr->obj_wref), xattr_double);
   efl_event_callback_call(evas_object_smart_parent_get(xattr->obj_wref), EFL_CANVAS_VIDEO_EVENT_POSITION_LOAD_DONE, NULL);
   evas_object_smart_callback_call(evas_object_smart_parent_get(xattr->obj_wref), "position_load,succeed", NULL);
   _eio_load_xattr_cleanup(xattr, handler);
}

/**
 * @brief Callback for failed EIO xattr load operation.
 * @internal
 *
 * Emits failure signals/callbacks and cleans up.
 *
 * @param data User data (Emotion_Xattr_Data pointer).
 * @param handler The Eio_File handle for the operation.
 * @param err The error code from EIO (unused in this function).
 */
static void
_eio_load_xattr_error(void *data, Eio_File *handler, int err EINA_UNUSED)
{
   Emotion_Xattr_Data *xattr = data;

   efl_event_callback_call(evas_object_smart_parent_get(xattr->obj_wref), EFL_CANVAS_VIDEO_EVENT_POSITION_LOAD_FAIL, NULL);
   evas_object_smart_callback_call(evas_object_smart_parent_get(xattr->obj_wref), "position_load,failed", NULL);
   _eio_load_xattr_cleanup(xattr, handler);
}
#endif

/**
 * @brief Loads the last saved playback position for the current file.
 *
 * This function attempts to read an extended attribute ("user.e.time_seek")
 * from the media file, which should contain the last playback position.
 * If successful, it sets the video to this position.
 * This uses EIO for asynchronous operation if HAVE_EIO is defined, otherwise
 * it's synchronous.
 *
 * Signals "position_load,succeed" or "position_load,failed" are emitted.
 * EFL events EFL_CANVAS_VIDEO_EVENT_POSITION_LOAD_DONE or
 * EFL_CANVAS_VIDEO_EVENT_POSITION_LOAD_FAIL are called.
 *
 * @param obj The Emotion object.
 *
 * @ingroup Emotion_Group_Persistence
 */
EMOTION_API void
emotion_object_last_position_load(Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;
   const char *tmp;
#ifndef HAVE_EIO
   double xattr;
#endif

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (!sd->file) return;

   if (!strncmp(sd->file, "file://", 7)) tmp = sd->file + 7;
   else if (!strstr(sd->file, "://")) tmp = sd->file;
   else return;

#ifdef HAVE_EIO
   Emotion_Xattr_Data *xattr = sd->xattr;

   if (xattr->load) return;
   EINA_REFCOUNT_REF(xattr);

   xattr->load = eio_file_xattr_double_get(tmp,
                                           "user.e.time_seek",
                                           _eio_load_xattr_done,
                                           _eio_load_xattr_error,
                                           xattr);
#else
   if (eina_xattr_double_get(tmp, "user.e.time_seek", &xattr))
     {
        emotion_object_position_set(obj, xattr);
        efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_POSITION_LOAD_DONE, NULL);
        evas_object_smart_callback_call(obj, "position_load,succeed", NULL);
     }
   else
     {
        efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_POSITION_LOAD_FAIL, NULL);
        evas_object_smart_callback_call(obj, "position_load,failed", NULL);
     }
#endif
}

#ifdef HAVE_EIO
/**
 * @brief Cleans up resources after an EIO xattr save operation.
 * @internal
 *
 * Nullifies the Eio_File handle in the xattr data and unreferences the xattr data.
 *
 * @param xattr The Emotion_Xattr_Data associated with the operation.
 * @param handler The Eio_File handle for the completed operation.
 */
static void
_eio_save_xattr_cleanup(Emotion_Xattr_Data *xattr, Eio_File *handler)
{
   if (handler == xattr->save) xattr->save = NULL;
   _xattr_data_unref(xattr);
}

/**
 * @brief Callback for successful EIO xattr save operation.
 * @internal
 *
 * Emits success signals/callbacks and cleans up.
 *
 * @param data User data (Emotion_Xattr_Data pointer).
 * @param handler The Eio_File handle for the operation.
 */
static void
_eio_save_xattr_done(void *data, Eio_File *handler)
{
   Emotion_Xattr_Data *xattr = data;

   efl_event_callback_call(xattr->obj_wref, EFL_CANVAS_VIDEO_EVENT_POSITION_SAVE_DONE, NULL);
   evas_object_smart_callback_call(xattr->obj_wref, "position_save,succeed", NULL);
   _eio_save_xattr_cleanup(xattr, handler);
}

/**
 * @brief Callback for failed EIO xattr save operation.
 * @internal
 *
 * Emits failure signals/callbacks and cleans up.
 *
 * @param data User data (Emotion_Xattr_Data pointer).
 * @param handler The Eio_File handle for the operation.
 * @param err The error code from EIO (unused in this function).
 */
static void
_eio_save_xattr_error(void *data, Eio_File *handler, int err EINA_UNUSED)
{
   Emotion_Xattr_Data *xattr = data;

   efl_event_callback_call(xattr->obj_wref, EFL_CANVAS_VIDEO_EVENT_POSITION_SAVE_FAIL, NULL);
   evas_object_smart_callback_call(xattr->obj_wref, "position_save,failed", NULL);
   _eio_save_xattr_cleanup(xattr, handler);
}
#endif

/**
 * @brief Saves the current playback position to the media file's extended attributes.
 *
 * This function writes the current playback position (obtained via
 * emotion_object_position_get()) to an extended attribute named "user.e.time_seek"
 * on the media file. This allows the position to be restored later using
 * emotion_object_last_position_load().
 * This uses EIO for asynchronous operation if HAVE_EIO is defined, otherwise
 * it's synchronous.
 *
 * Signals "position_save,succeed" or "position_save,failed" are emitted.
 * EFL events EFL_CANVAS_VIDEO_EVENT_POSITION_SAVE_DONE or
 * EFL_CANVAS_VIDEO_EVENT_POSITION_SAVE_FAIL are called.
 *
 * @param obj The Emotion object.
 *
 * @ingroup Emotion_Group_Persistence
 */
EMOTION_API void
emotion_object_last_position_save(Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;
   const char *tmp;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (!sd->file) return;

   if (!strncmp(sd->file, "file://", 7)) tmp = sd->file + 7;
   else if (!strstr(sd->file, "://")) tmp = sd->file;
   else return;
#ifdef HAVE_EIO
   Emotion_Xattr_Data *xattr = sd->xattr;

   if (xattr->save) return;
   EINA_REFCOUNT_REF(xattr);

   xattr->save = eio_file_xattr_double_set(tmp,
                                           "user.e.time_seek",
                                           emotion_object_position_get(obj),
                                           0,
                                           _eio_save_xattr_done,
                                           _eio_save_xattr_error,
                                           xattr);
#else
   if (eina_xattr_double_set(tmp, "user.e.time_seek", emotion_object_position_get(obj), 0))
     {
        efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_POSITION_SAVE_DONE, NULL);
        evas_object_smart_callback_call(obj, "position_save,succeed", NULL);
     }
   else
     {
        efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_POSITION_SAVE_FAIL, NULL);
        evas_object_smart_callback_call(obj, "position_save,failed", NULL);
     }
#endif
}

/**
 * @brief Sets the suspend state of the Emotion object.
 *
 * This function controls how the Emotion object behaves when it's not
 * actively being used, allowing for different levels of resource saving.
 * - #EMOTION_WAKEUP: Normal operation.
 * - #EMOTION_SLEEP: May destroy some rendering parts.
 * - #EMOTION_DEEP_SLEEP: Destroys most rendering parts, keeps last frame.
 * - #EMOTION_HIBERNATE: Destroys rendering, keeps small thumbnail.
 *
 * The exact behavior can be engine-dependent.
 *
 * @param obj The Emotion object.
 * @param state The desired #Emotion_Suspend state.
 *
 * @ingroup Emotion_Group_Advanced
 */
EMOTION_API void
emotion_object_suspend_set(Evas_Object *obj, Emotion_Suspend state)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   switch (state)
     {
      case EMOTION_WAKEUP:
         /* Restore the rendering pipeline, offset and everything back to play again (this will be called automatically by play_set) */
      case EMOTION_SLEEP:
         /* This destroy some part of the rendering pipeline */
      case EMOTION_DEEP_SLEEP:
         /* This destroy all the rendering pipeline and just keep the last rendered image (fullscreen) */
      case EMOTION_HIBERNATE:
         /* This destroy all the rendering pipeline and keep 1/4 of the last rendered image */
      default:
         break;
     }
   sd->state = state;
}

/**
 * @brief Gets the current suspend state of the Emotion object.
 *
 * @param obj The Emotion object.
 * @return The current #Emotion_Suspend state.
 *
 * @ingroup Emotion_Group_Advanced
 */
EMOTION_API Emotion_Suspend
emotion_object_suspend_get(Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, EMOTION_WAKEUP);
   return sd->state;
}

/*****************************/
/* Utility calls for modules */
/*****************************/

/**
 * @internal
 * @brief Gets the engine-specific data associated with the Emotion object.
 *
 * This function is intended for use by Emotion engine modules to access
 * their private data structures.
 *
 * @param obj The Emotion object.
 * @return A pointer to the engine-specific data, or @c NULL on failure.
 */
EMOTION_API void *
_emotion_video_get(const Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, NULL);
   return emotion_engine_instance_data_get(sd->engine_instance);
}

/**
 * @internal
 * @brief Animator callback for processing a new video frame.
 *
 * This function is called by an Ecore_Animator when it's time to update
 * the video display with a new frame. It marks the Evas image as dirty,
 * updates position information, and emits frame decode signals/callbacks.
 *
 * @param data The Emotion Evas object.
 * @return EINA_FALSE to indicate the animator should not run again automatically
 *         (it will be re-added by _emotion_frame_new when needed).
 */
static Eina_Bool
_emotion_frame_anim(void *data)
{
   Evas_Object *obj = data;
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET_RETURN(sd, obj, E_OBJ_NAME, EINA_FALSE);

   sd->anim = NULL;
   evas_object_image_pixels_dirty_set(sd->obj, 1);
   _emotion_video_pos_update(obj,
                             emotion_engine_instance_pos_get(sd->engine_instance),
                             emotion_engine_instance_len_get(sd->engine_instance));
   efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_FRAME_DECODE, NULL);
   evas_object_smart_callback_call(obj, "frame_decode", NULL);
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Signals that a new video frame is available from the engine.
 *
 * This function is called by engine modules when they have a new decoded
 * video frame ready for display. It schedules an animator (_emotion_frame_anim)
 * to handle the actual update on the Evas canvas in the next rendering cycle.
 *
 * @param obj The Emotion Evas object.
 */
EMOTION_API void
_emotion_frame_new(Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (!sd->anim)
     sd->anim = ecore_evas_animator_add(obj, _emotion_frame_anim, obj);
}

/**
 * @internal
 * @brief Updates the playback position and length, emitting signals if changed.
 *
 * Called by engine modules to inform the Emotion object about changes in
 * the current playback time or total duration of the media.
 *
 * @param obj The Emotion Evas object.
 * @param pos The new current playback position in seconds.
 * @param len The new total length of the media in seconds.
 */
EMOTION_API void
_emotion_video_pos_update(Evas_Object *obj, double pos, double len)
{
   Efl_Canvas_Video_Data *sd;
   int npos = 0, nlen = 0;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (!EINA_DBL_EQ(pos, sd->pos)) npos = 1;
   if (!EINA_DBL_EQ(len, sd->len)) nlen = 1;
   sd->pos = pos;
   sd->len = len;
   if (npos)
     {
        efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_POSITION_CHANGE, NULL);
        evas_object_smart_callback_call(obj, "position_update", NULL);
     }
   if (nlen)
     {
        efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_LENGTH_CHANGE, NULL);
        evas_object_smart_callback_call(obj, "length_change", NULL);
     }
}

/**
 * @internal
 * @brief Handles a change in the video stream's dimensions or aspect ratio.
 *
 * Called by engine modules when the native resolution or pixel aspect ratio
 * of the video stream changes (e.g., mid-stream format change).
 * It updates internal data, emits signals, and reapplies aspect/border settings.
 *
 * @param obj The Emotion Evas object.
 * @param w The new native width of the video stream.
 * @param h The new native height of the video stream.
 * @param ratio The new pixel aspect ratio of the video stream.
 */
EMOTION_API void
_emotion_frame_resize(Evas_Object *obj, int w, int h, double ratio)
{
   Efl_Canvas_Video_Data *sd;
   double tmp;
   int changed = 0;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if ((w != sd->video.w) || (h != sd->video.h))
     {
        sd->video.w = w;
        sd->video.h = h;
        _emotion_image_data_zero(sd->obj);
        changed = 1;
     }
   if (h > 0) tmp  = (double)w / (double)h;
   else tmp = 1.0;
   if (!EINA_DBL_EQ(ratio, tmp)) tmp = ratio;
   if (!EINA_DBL_EQ(tmp, sd->ratio))
     {
        sd->ratio = tmp;
        changed = 1;
     }
   if (changed)
     {
        evas_object_size_hint_request_set(obj, w, h);
        efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_FRAME_RESIZE, NULL);
        evas_object_smart_callback_call(obj, "frame_resize", NULL);
        evas_object_geometry_get(obj, NULL, NULL, &w, &h);
        _efl_canvas_video_aspect_border_apply(obj, sd, w, h);
     }
}

/**
 * @internal
 * @brief Resets the content of the Evas image object used for video display.
 *
 * This function calls _emotion_image_data_zero to clear the image buffer.
 * It's typically used by engine modules when the video stream stops or
 * the underlying image data becomes invalid.
 *
 * @param obj The Emotion Evas object.
 */
EMOTION_API void
_emotion_image_reset(Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   _emotion_image_data_zero(sd->obj);
}

/**
 * @internal
 * @brief Signals that video decoding has stopped.
 *
 * Called by engine modules when they stop decoding video frames (e.g., at
 * end of stream, or due to an error). It updates the play state and emits
 * a "decode_stop" smart callback.
 *
 * @param obj The Emotion Evas object.
 */
EMOTION_API void
_emotion_decode_stop(Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (sd->play)
     {
        sd->play = 0;
        evas_object_smart_callback_call(obj, "decode_stop", NULL);
     }
}

/**
 * @internal
 * @brief Signals that the media file has been successfully opened by the engine.
 *
 * Called by engine modules after they have successfully opened and initialized
 * the media file. This function updates the open state, applies any remembered
 * jump position or play state, and emits "open_done" signals/callbacks.
 *
 * @param obj The Emotion Evas object.
 */
EMOTION_API void
_emotion_open_done(Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   sd->open = 1;

   if (!EINA_DBL_EQ(sd->remember_jump, 0.0))
     emotion_object_position_set(obj, sd->remember_jump);
   if (sd->remember_play != sd->play)
     {
        if (sd->pause)
          sd->play = sd->remember_play;
        else
          emotion_object_play_set(obj, sd->remember_play);
     }
   efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_OPEN_DONE, NULL);
   evas_object_smart_callback_call(obj, "open_done", NULL);
}

/**
 * @internal
 * @brief Signals that playback has started.
 *
 * Called by engine modules when media playback actually begins.
 * Emits "playback_started" signals/callbacks.
 *
 * @param obj The Emotion Evas object.
 */
EMOTION_API void
_emotion_playback_started(Evas_Object *obj)
{
   efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_PLAYBACK_START, NULL);
   evas_object_smart_callback_call(obj, "playback_started", NULL);
}

/**
 * @internal
 * @brief Signals that playback has finished.
 *
 * Called by engine modules when media playback reaches the end or is otherwise
 * considered finished. Emits "playback_finished" signals/callbacks.
 *
 * @param obj The Emotion Evas object.
 */
EMOTION_API void
_emotion_playback_finished(Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_PLAYBACK_STOP, NULL);
   evas_object_smart_callback_call(obj, "playback_finished", NULL);
}

/**
 * @internal
 * @brief Signals that the audio level (volume or mute state) has changed.
 *
 * Called by engine modules when the audio volume or mute status is changed
 * externally to the Emotion object (e.g., by the engine itself or system).
 * Emits "audio_level_change" signals/callbacks.
 *
 * @param obj The Emotion Evas object.
 */
EMOTION_API void
_emotion_audio_level_change(Evas_Object *obj)
{
   efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_VOLUME_CHANGE, NULL);
   evas_object_smart_callback_call(obj, "audio_level_change", NULL);
}

/**
 * @internal
 * @brief Signals that the available audio/video/SPU channels have changed.
 *
 * Called by engine modules when the number or properties of available
 * media tracks (audio, video, subtitles) change.
 * Emits "channels_change" signals/callbacks.
 *
 * @param obj The Emotion Evas object.
 */
EMOTION_API void
_emotion_channels_change(Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_CHANNELS_CHANGE, NULL);
   evas_object_smart_callback_call(obj, "channels_change", NULL);
}

/**
 * @internal
 * @brief Sets the title of the media and signals the change.
 *
 * Called by engine modules when the title of the currently playing media
 * is determined or changes. Updates the internal title stringshare and
 * emits "title_change" signals/callbacks.
 *
 * @param obj The Emotion Evas object.
 * @param title The new title string. This string will be stringshared.
 */
EMOTION_API void
_emotion_title_set(Evas_Object *obj, char *title)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   eina_stringshare_replace(&sd->title, title);
   efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_TITLE_CHANGE, NULL);
   evas_object_smart_callback_call(obj, "title_change", NULL);
}

/**
 * @internal
 * @brief Sets the progress information and status, signaling the change.
 *
 * Called by engine modules to update progress information (e.g., buffering status).
 * Updates internal progress data and emits "progress_change" signals/callbacks.
 *
 * @param obj The Emotion Evas object.
 * @param info A string describing the progress. This string will be stringshared.
 * @param st A numerical status of the progress (typically 0.0 to 1.0).
 */
EMOTION_API void
_emotion_progress_set(Evas_Object *obj, char *info, double st)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   eina_stringshare_replace(&sd->progress.info, info);
   sd->progress.stat = st;
   efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_PROGRESS_CHANGE, NULL);
   evas_object_smart_callback_call(obj, "progress_change", NULL);
}

/**
 * @internal
 * @brief Sets information about a referenced file and signals the change.
 *
 * Called by engine modules when the media refers to another file (e.g.,
 * an item in a playlist). Updates internal reference data and emits
 * "ref_change" signals/callbacks.
 *
 * @param obj The Emotion Evas object.
 * @param file The path/URI of the referenced file. This string will be stringshared.
 * @param num A number associated with the reference (e.g., track number).
 */
EMOTION_API void
_emotion_file_ref_set(Evas_Object *obj, const char *file, int num)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   eina_stringshare_replace(&sd->ref.file, file);
   sd->ref.num = num;
   efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_REF_CHANGE, NULL);
   evas_object_smart_callback_call(obj, "ref_change", NULL);
}

/**
 * @internal
 * @brief Sets the number of SPU (DVD menu) buttons and signals the change.
 *
 * Called by engine modules (typically DVD engines) when the number of
 * available SPU buttons changes. Updates internal SPU data and emits
 * "button_num_change" signals/callbacks.
 *
 * @param obj The Emotion Evas object.
 * @param num The new number of SPU buttons.
 */
EMOTION_API void
_emotion_spu_button_num_set(Evas_Object *obj, int num)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   sd->spu.button_num = num;
   efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_BUTTON_NUM_CHANGE, NULL);
   evas_object_smart_callback_call(obj, "button_num_change", NULL);
}

/**
 * @internal
 * @brief Sets the currently highlighted SPU (DVD menu) button and signals the change.
 *
 * Called by engine modules when the highlighted SPU button changes.
 * Updates internal SPU data and emits "button_change" signals/callbacks.
 *
 * @param obj The Emotion Evas object.
 * @param button The index of the new highlighted SPU button.
 */
EMOTION_API void
_emotion_spu_button_set(Evas_Object *obj, int button)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   sd->spu.button = button;
   efl_event_callback_call(obj, EFL_CANVAS_VIDEO_EVENT_BUTTON_CHANGE, NULL);
   evas_object_smart_callback_call(obj, "button_change", NULL);
}

/**
 * @internal
 * @brief Signals that a seek operation has completed.
 *
 * Called by engine modules after a requested seek operation is finished.
 * If another seek was queued (sd->seek is true), it initiates that seek.
 *
 * @param obj The Emotion Evas object.
 */
EMOTION_API void
_emotion_seek_done(Evas_Object *obj)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if (sd->seeking)
     {
        sd->seeking = 0;
        if (sd->seek) emotion_object_position_set(obj, sd->seek_pos);
     }
}

/**
 * @internal
 * @brief Adjusts the fill parameters of the Evas image object.
 *
 * This function is called to change how the video frame is scaled within
 * the Evas image object, potentially overscanning or underscanning relative
 * to the object's actual size. This is different from aspect ratio handling
 * and cropping, as it directly manipulates the `evas_object_image_fill_set`
 * properties.
 *
 * @param obj The Emotion Evas object.
 * @param w The desired fill width factor. If <= 0, resets to default fill behavior.
 *          A value of 1.0 means the source image width will fill the target width.
 *          A value of 2.0 means the source image width will be twice the target width (zoomed in).
 * @param h The desired fill height factor. If <= 0, resets to default fill behavior.
 *          Similar to @p w for height.
 */
EMOTION_API void
_emotion_frame_refill(Evas_Object *obj, double w, double h)
{
   Efl_Canvas_Video_Data *sd;

   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   if ((!EINA_DBL_EQ(sd->fill.w, w)) ||
       (!EINA_DBL_EQ(sd->fill.h, h)))
     {
        Evas_Coord ow, oh;

        evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
        if ((w <= 0) || (h <= 0))
          {
             double scale_w, scale_h;

             sd->fill.w = -1;
             sd->fill.h = -1;

             scale_w = (double)ow / (double)(sd->video.w - sd->crop.l - sd->crop.r);
             scale_h = (double)oh / (double)(sd->video.h - sd->crop.t - sd->crop.b);
             evas_object_image_fill_set(sd->obj, 0, 0, scale_w * sd->video.w, scale_h * sd->video.h);
          }
        else
          {
             sd->fill.w = w;
             sd->fill.h = h;
             evas_object_image_fill_set(sd->obj, 0, 0, w * ow, h * oh);
          }
     }
}

/****************************/
/* Internal object routines */
/****************************/

/**
 * @internal
 * @brief Handles mouse move events on the video object.
 *
 * Converts canvas coordinates to video-relative coordinates and feeds them
 * to the engine instance. This is used for features like interactive DVD menus.
 *
 * @param data User data (Efl_Canvas_Video_Data pointer).
 * @param ev The Evas canvas (unused).
 * @param obj The Evas object that received the event (the internal image object).
 * @param event_info Pointer to Evas_Event_Mouse_Move structure.
 */
static void
_mouse_move(void *data, Evas *ev EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *e;
   Efl_Canvas_Video_Data *sd;
   int x, y, iw, ih;
   Evas_Coord ox, oy, ow, oh;

   e = event_info;
   sd = data;
   if (!sd->engine_instance) return;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   evas_object_image_size_get(obj, &iw, &ih);
   if ((iw < 1) || (ih < 1)) return;
   x = (((int)e->cur.canvas.x - ox) * iw) / ow;
   y = (((int)e->cur.canvas.y - oy) * ih) / oh;
   emotion_engine_instance_event_mouse_move_feed(sd->engine_instance, x, y);
}

/**
 * @internal
 * @brief Handles mouse down events on the video object.
 *
 * Converts canvas coordinates to video-relative coordinates and feeds them
 * (along with button 1, assuming left-click) to the engine instance.
 * Used for features like interactive DVD menus.
 *
 * @param data User data (Efl_Canvas_Video_Data pointer).
 * @param ev The Evas canvas (unused).
 * @param obj The Evas object that received the event (the internal image object).
 * @param event_info Pointer to Evas_Event_Mouse_Down structure.
 */
static void
_mouse_down(void *data, Evas *ev EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *e;
   Efl_Canvas_Video_Data *sd;
   int x, y, iw, ih;
   Evas_Coord ox, oy, ow, oh;

   e = event_info;
   sd = data;
   if (!sd->engine_instance) return;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   evas_object_image_size_get(obj, &iw, &ih);
   if ((iw < 1) || (ih < 1)) return;
   x = (((int)e->canvas.x - ox) * iw) / ow;
   y = (((int)e->canvas.y - oy) * ih) / oh;
   emotion_engine_instance_event_mouse_button_feed(sd->engine_instance, 1, x, y);
}

/**
 * @internal
 * @brief Ecore_Job callback to perform a deferred seek operation.
 *
 * This job is scheduled when emotion_object_position_set() is called.
 * It ensures that seek operations are not performed too rapidly and allows
 * for coalescing multiple seek requests.
 *
 * @param data User data (the Emotion Evas_Object).
 */
static void
_pos_set_job(void *data)
{
   Evas_Object *obj;
   Efl_Canvas_Video_Data *sd;

   obj = data;
   E_SMART_OBJ_GET(sd, obj, E_OBJ_NAME);
   sd->job = NULL;
   if (!sd->engine_instance) return;
   if (sd->seeking) return;
   if (sd->seek)
     {
        sd->seeking = 1;
        emotion_engine_instance_pos_set(sd->engine_instance, sd->seek_pos);
        sd->seek = 0;
     }
}

/**
 * @internal
 * @brief Evas image pixels_get_callback.
 *
 * This function is called by Evas when it needs pixel data for the internal
 * image object (`sd->obj`) used to display video. It retrieves the latest
 * video frame data from the engine instance (in YUV or BGRA format) and
 * provides it to Evas.
 *
 * @param data User data (Efl_Canvas_Video_Data pointer).
 * @param obj The Evas image object for which pixels are needed.
 */
static void
_pixels_get(void *data, Evas_Object *obj)
{
   int iw, ih, w, h;
   Efl_Canvas_Video_Data *sd;
   Emotion_Format format;
   unsigned char *bgra_data;

   sd = data;
   if (!sd->engine_instance) return;
   emotion_engine_instance_video_data_size_get(sd->engine_instance, &w, &h);
   w = (w >> 1) << 1;
   h = (h >> 1) << 1;

   evas_object_image_colorspace_set(obj, EVAS_COLORSPACE_YCBCR422P601_PL);
   evas_object_image_alpha_set(obj, 0);
   evas_object_image_size_set(obj, w, h);
   iw = w;
   ih = h;

   if ((iw <= 1) || (ih <= 1))
     {
        _emotion_image_data_zero(sd->obj);
        evas_object_image_pixels_dirty_set(obj, 0);
     }
   else
     {
        format = emotion_engine_instance_format_get(sd->engine_instance);
        if ((format == EMOTION_FORMAT_YV12) || (format == EMOTION_FORMAT_I420))
          {
             unsigned char **rows;

             evas_object_image_colorspace_set(obj, EVAS_COLORSPACE_YCBCR422P601_PL);
             rows = evas_object_image_data_get(obj, 1);
             if (rows)
               {
                  if (emotion_engine_instance_yuv_rows_get(sd->engine_instance, iw, ih,
                                                           rows,
                                                           &rows[ih],
                                                           &rows[ih + (ih / 2)]))
                  evas_object_image_data_update_add(obj, 0, 0, iw, ih);
               }
             evas_object_image_data_set(obj, rows);
             evas_object_image_pixels_dirty_set(obj, 0);
          }
        else if (format == EMOTION_FORMAT_BGRA)
          {
             evas_object_image_colorspace_set(obj, EVAS_COLORSPACE_ARGB8888);
             if (emotion_engine_instance_bgra_data_get(sd->engine_instance, &bgra_data))
               {
                  evas_object_image_data_set(obj, bgra_data);
                  evas_object_image_pixels_dirty_set(obj, 0);
               }
          }
     }
}

/*******************************************/
/* Internal smart object required routines */
/*******************************************/

EOLIAN static void
_efl_canvas_video_efl_canvas_group_group_add(Evas_Object *obj, Efl_Canvas_Video_Data *sd)
{
   Emotion_Xattr_Data *xattr;
   unsigned int *pixel;

   /* TODO: remove legacy: emotion used to have no init, call automatically */
   emotion_init();

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   sd->state = EMOTION_WAKEUP;
   sd->obj = evas_object_image_add(evas_object_evas_get(obj));
   sd->bg = evas_object_rectangle_add(evas_object_evas_get(obj));
   sd->engine = eina_stringshare_add("gstreamer1");
   evas_object_color_set(sd->bg, 0, 0, 0, 0);
   evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move, sd);
   evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, sd);
   evas_object_image_pixels_get_callback_set(sd->obj, _pixels_get, sd);
   evas_object_smart_member_add(sd->obj, obj);
   evas_object_smart_member_add(sd->bg, obj);
   evas_object_lower(sd->bg);
   sd->ratio = 1.0;
   sd->spu.button = -1;
   sd->fill.w = -1;
   sd->fill.h = -1;
   evas_object_image_alpha_set(sd->obj, 0);
   pixel = evas_object_image_data_get(sd->obj, 1);
   if (pixel)
     {
        *pixel = 0xff000000;
        evas_object_image_data_set(obj, pixel);
     }
   evas_object_show(sd->obj);
   evas_object_show(sd->bg);

   xattr = calloc(1, sizeof(*xattr));
   EINA_REFCOUNT_INIT(xattr);
   efl_wref_add(obj, &xattr->obj_wref);
   sd->xattr = xattr;
}

/**
 * @internal
 * @brief Implements Efl.Canvas.Group.group_del for Emotion. (Smart object destructor)
 *
 * Cleans up all resources associated with the Emotion object, including:
 * - Closing and deleting the engine instance.
 * - Deleting Ecore jobs and animators.
 * - Freeing stringshared data (file path, progress info, etc.).
 * - Unreferencing xattr data.
 * - Calling the parent class's group_del.
 * - Shutting down Emotion (if this was the last instance).
 *
 * @param obj The Emotion Eo object (unused here, but part of signature).
 * @param sd Pointer to the private data of the Emotion object.
 */
EOLIAN static void
_efl_canvas_video_efl_canvas_group_group_del(Evas_Object *obj EINA_UNUSED, Efl_Canvas_Video_Data *sd)
{
   if (sd->engine_instance)
     {
        emotion_engine_instance_file_close(sd->engine_instance);
        emotion_engine_instance_del(sd->engine_instance);
     }
   sd->engine_instance = NULL;
   if (sd->job) ecore_job_del(sd->job);
   sd->job = NULL;
   if (sd->anim) ecore_animator_del(sd->anim);
   sd->anim = NULL;
   eina_stringshare_del(sd->file);
   eina_stringshare_del(sd->progress.info);
   eina_stringshare_del(sd->ref.file);
   sd->file = NULL;
   sd->progress.info = NULL;
   sd->ref.file = NULL;
   _xattr_data_unref(sd->xattr);
   efl_canvas_group_del(efl_super(obj, MY_CLASS));
   emotion_shutdown();
}

/**
 * @internal
 * @brief Implements Efl.Gfx.Entity.position_set for Emotion.
 *
 * Handles setting the position of the Emotion object. After calling the
 * parent's implementation, it updates the position of the internal video
 * image and clipper via _clipper_position_size_update().
 *
 * @param obj The Emotion Eo object.
 * @param sd Pointer to the private data of the Emotion object.
 * @param pos The new Eina_Position2D for the object.
 */
EOLIAN static void
_efl_canvas_video_efl_gfx_entity_position_set(Evas_Object *obj, Efl_Canvas_Video_Data *sd, Eina_Position2D pos)
{
   Eina_Size2D sz;

   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_MOVE, 0, pos.x, pos.y))
     return;

   efl_gfx_entity_position_set(efl_super(obj, MY_CLASS), pos);

   sz = efl_gfx_entity_size_get(obj);
   _clipper_position_size_update(obj, pos.x, pos.y, sz.w, sz.h, sd->video.w, sd->video.h);
}

/**
 * @internal
 * @brief Implements Efl.Gfx.Entity.size_set for Emotion.
 *
 * Handles setting the size of the Emotion object. After calling the
 * parent's implementation, it applies aspect/border rules via
 * _efl_canvas_video_aspect_border_apply() and resizes the background object.
 *
 * @param obj The Emotion Eo object.
 * @param sd Pointer to the private data of the Emotion object.
 * @param sz The new Eina_Size2D for the object.
 */
EOLIAN static void
_efl_canvas_video_efl_gfx_entity_size_set(Evas_Object *obj, Efl_Canvas_Video_Data *sd, Eina_Size2D sz)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_RESIZE, 0, sz.w, sz.h))
     return;

   efl_gfx_entity_size_set(efl_super(obj, MY_CLASS), sz);

   _efl_canvas_video_aspect_border_apply(obj, sd, sz.w, sz.h);
   evas_object_resize(sd->bg, sz.w, sz.h);
}

/* Internal EO APIs and hidden overrides */

#define EFL_CANVAS_VIDEO_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(efl_canvas_video) /**< Macro defining extra operations for the Efl_Canvas_Video class, likely related to group add/delete. */


#include "efl_canvas_video.eo.c"
#include "efl_canvas_video_eo.legacy.c"
