#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_ACCESS_WIDGET_ACTION_PROTECTED


#include "elm_priv.h"
#include "elm_widget_layout.h"
#include "efl_ui_video_private.h"

/* TODO: add buffering support to Emotion and display buffering
 * progress in the theme when needed */

#define MY_CLASS EFL_UI_VIDEO_CLASS
#define MY_CLASS_NAME "Efl.Ui.Video"

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_LAYOUT_FOCUSED, ""}, /**< handled by elm_layout */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< handled by elm_layout */
   {NULL, NULL}
};


static void
_on_open_done(void *data, const Efl_Event *event);
static void
_on_playback_started(void *data, const Efl_Event *event);
static void
_on_playback_finished(void *data, const Efl_Event *event);
static void
_on_aspect_ratio_updated(void *data, const Efl_Event *event);
static void
_on_title_changed(void *data, const Efl_Event *event);
static void
_on_audio_level_changed(void *data, const Efl_Event *event);

static Eina_Bool _key_action_move(Evas_Object *obj, const char *params);
static Eina_Bool _key_action_play(Evas_Object *obj, const char *params);

static const Elm_Action key_actions[] = {
   {"move", _key_action_move},
   {"play", _key_action_play},
   {NULL, NULL}
};

EFL_CALLBACKS_ARRAY_DEFINE(_video_cb,
   { EFL_CANVAS_VIDEO_EVENT_OPEN_DONE, _on_open_done },
   { EFL_CANVAS_VIDEO_EVENT_PLAYBACK_START, _on_playback_started },
   { EFL_CANVAS_VIDEO_EVENT_PLAYBACK_STOP, _on_playback_finished },
   { EFL_CANVAS_VIDEO_EVENT_FRAME_RESIZE, _on_aspect_ratio_updated },
   { EFL_CANVAS_VIDEO_EVENT_TITLE_CHANGE, _on_title_changed },
   { EFL_CANVAS_VIDEO_EVENT_VOLUME_CHANGE, _on_audio_level_changed }
);

/**
 * @internal
 * @brief Handles key actions for seeking in the video.
 *
 * It seeks forward for "right" and backward for "left". The seek step is
 * 1/100th of the total length.
 *
 * @param obj The video object.
 * @param params The direction of movement, "left" or "right".
 * @return EINA_TRUE if the action was handled, EINA_FALSE otherwise.
 */
static Eina_Bool
_key_action_move(Evas_Object *obj, const char *params)
{
   const char *dir = params;

   _elm_widget_focus_auto_show(obj);
   if (!strcmp(dir, "left"))
     {
        double current, last;

        current = elm_video_play_position_get(obj);
        last = elm_video_play_length_get(obj);

        if (current < last)
          {
             current += last / 100;
             elm_video_play_position_set(obj, current);
          }
     }
   else if (!strcmp(dir, "right"))
     {
        double current, last;

        current = elm_video_play_position_get(obj);
        last = elm_video_play_length_get(obj);

        if (current > 0)
          {
             current -= last / 100;
             if (current < 0) current = 0;
             elm_video_play_position_set(obj, current);
          }
     }
   else return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Handles key action for toggling play/pause.
 *
 * @param obj The video object.
 * @param params EINA_UNUSED.
 * @return EINA_TRUE always.
 */
static Eina_Bool
_key_action_play(Evas_Object *obj, const char *params EINA_UNUSED)
{
   if (!efl_player_paused_get(obj))
     elm_video_pause(obj);
   else
     elm_video_play(obj);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Calculates the minimum size of the video widget.
 *
 * This function is called when the layout needs to be recalculated. It takes the
 * minimum size from the emotion object (the video itself) and uses it to calculate
 * the minimum size of the whole widget, also considering the theme.
 * It also sets the aspect ratio hint to maintain the video's aspect.
 *
 * @param obj The video widget object.
 * @param sd The private data of the video widget.
 */
EOLIAN static void
_efl_ui_video_efl_canvas_group_group_calculate(Eo *obj, Efl_Ui_Video_Data *sd)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   Evas_Coord minw = 0, minh = 0;
   Evas_Coord w = 0, h = 0;

   efl_canvas_group_need_recalculate_set(obj, EINA_FALSE);
   evas_object_size_hint_request_get(sd->emotion, &minw, &minh);
   if (minw && minh)
     evas_object_size_hint_aspect_set
       (sd->emotion, EVAS_ASPECT_CONTROL_BOTH, minw, minh);
   edje_object_size_min_calc(wd->resize_obj, &w, &h);

   if (w != 0 && h != 0)
     {
        minw = w;
        minh = h;
        evas_object_size_hint_aspect_set(obj, EVAS_ASPECT_CONTROL_BOTH, minw, minh);
     }
}

/**
 * @internal
 * @brief Callback function for when the size hints of the object change.
 *
 * This triggers a recalculation of the container canvas group.
 *
 * @param data EINA_UNUSED.
 * @param e EINA_UNUSED.
 * @param obj The object that changed.
 * @param event_info EINA_UNUSED.
 */
static void
_on_size_hints_changed(void *data EINA_UNUSED,
                       Evas *e EINA_UNUSED,
                       Evas_Object *obj,
                       void *event_info EINA_UNUSED)
{
   efl_canvas_group_change(obj);
}

/**
 * @internal
 * @brief Callback for the "open,done" signal from the emotion object.
 *
 * Emits a "video,open" signal to the edje theme. The signal name depends
 * on whether it's a legacy widget ("elm,video,open") or not ("efl,video,open").
 *
 * @param data The video object.
 * @param event EINA_UNUSED.
 */
static void
_on_open_done(void *data, const Efl_Event *event EINA_UNUSED)
{
   if(elm_widget_is_legacy(data))
     elm_layout_signal_emit(data, "elm,video,open", "elm");
   else
     elm_layout_signal_emit(data, "efl,video,open", "efl");
}

/**
 * @internal
 * @brief Callback for the "playback,start" event from the emotion object.
 *
 * Emits a "video,play" signal to the edje theme. The signal name depends
 * on whether it's a legacy widget ("elm,video,play") or not ("efl,video,play").
 *
 * @param data The video object.
 * @param event EINA_UNUSED.
 */
static void
_on_playback_started(void *data, const Efl_Event *event EINA_UNUSED)
{
   if(elm_widget_is_legacy(data))
     elm_layout_signal_emit(data, "elm,video,play", "elm");
   else
     elm_layout_signal_emit(data, "efl,video,play", "efl");

   return;

}

/**
 * @internal
 * @brief Callback for the "playback,stop" event from the emotion object.
 *
 * This function is called when the video playback naturally finishes.
 * It ensures the emotion object's play property is set to false and
 * emits a "video,end" signal to the edje theme.
 *
 * @param data The video object.
 * @param event EINA_UNUSED.
 */
static void
_on_playback_finished(void *data, const Efl_Event *event EINA_UNUSED)
{
   EFL_UI_VIDEO_DATA_GET(data, sd);
   emotion_object_play_set(sd->emotion, EINA_FALSE);

   if(elm_widget_is_legacy(data))
     elm_layout_signal_emit(data, "elm,video,end", "elm");
   else
     elm_layout_signal_emit(data, "efl,video,end", "efl");
}

/**
 * @internal
 * @brief Callback for the "frame,resize" event from the emotion object.
 *
 * This event indicates that the video's aspect ratio has changed.
 * It triggers a recalculation of the widget's size.
 *
 * @param data The video object.
 * @param event EINA_UNUSED.
 */
static void
_on_aspect_ratio_updated(void *data, const Efl_Event *event EINA_UNUSED)
{
   efl_canvas_group_change(data);
}

/**
 * @internal
 * @brief Callback for the "title,change" event from the emotion object.
 *
 * Updates the "title" text part in the theme and emits a "video,title"
 * signal to the edje theme.
 *
 * @param data The video object.
 * @param event EINA_UNUSED.
 */
static void
_on_title_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   const char *title;

   EFL_UI_VIDEO_DATA_GET(data, sd);

   title = emotion_object_title_get(sd->emotion);

   if(elm_widget_is_legacy(data))
     {
        elm_layout_text_set(data, "elm,title", title);
        elm_layout_signal_emit(data, "elm,video,title", "elm");
     }
   else
     {
        elm_layout_text_set(data, "efl,title", title);
        elm_layout_signal_emit(data, "efl,video,title", "efl");
     }
}

/**
 * @internal
 * @brief Callback for the "volume,change" event from the emotion object.
 *
 * Currently, this function does nothing.
 *
 * @param data The video object.
 * @param event EINA_UNUSED.
 */
static void
_on_audio_level_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   (void)data;
}

/**
 * @internal
 * @brief Timer callback to progressively suspend the video object.
 *
 * This function is called periodically when the video is paused. It increases
 * the suspend level of the emotion object over time to save resources.
 * The progression is SLEEP -> DEEP_SLEEP -> HIBERNATE. Once in HIBERNATE,
 * the timer is cancelled.
 *
 * @param data The video object.
 * @return ECORE_CALLBACK_RENEW to continue the timer, or ECORE_CALLBACK_CANCEL to stop it.
 */
static Eina_Bool
_suspend_cb(void *data)
{
   double interval;

   EFL_UI_VIDEO_DATA_GET(data, sd);

   interval = ecore_timer_interval_get(sd->timer);
   if (interval <= 20)
     emotion_object_suspend_set(sd->emotion, EMOTION_SLEEP);
   else if (interval <= 30)
     emotion_object_suspend_set(sd->emotion, EMOTION_DEEP_SLEEP);
   else
     {
        emotion_object_suspend_set(sd->emotion, EMOTION_HIBERNATE);
        sd->timer = NULL;

        return ECORE_CALLBACK_CANCEL;
     }

   ecore_timer_interval_set(sd->timer, interval + 10);
   return ECORE_CALLBACK_RENEW;
}

/**
 * @internal
 * @brief Checks if an object is a valid Efl_Ui_Video widget.
 *
 * This is an internal helper function.
 *
 * @param video The object to check.
 * @return EINA_TRUE if the object is a valid video widget, EINA_FALSE otherwise.
 */
Eina_Bool
_elm_video_check(Evas_Object *video)
{
   EFL_UI_VIDEO_CHECK(video) EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Adds the Efl_Ui_Video object to the canvas.
 *
 * This function is the Evas smart 'add' implementation for the video widget.
 * It sets up the theme, creates and initializes the internal emotion object,
 * swallows the emotion object into the layout, and sets up various event
 * callbacks for video events and size changes. It also starts a timer for
 * resource-saving suspend modes.
 *
 * @param obj The video widget object.
 * @param priv The private data for the video widget.
 */
EOLIAN static void
_efl_ui_video_efl_canvas_group_group_add(Eo *obj, Efl_Ui_Video_Data *priv)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "video");
   efl_canvas_group_add(efl_super(obj, MY_CLASS));
   elm_widget_can_focus_set(obj, EINA_TRUE);

   priv->emotion = emotion_object_add(evas_object_evas_get(obj));
   if (!emotion_object_init(priv->emotion, NULL))
     CRI("Failed to init emotion object");

   if (elm_widget_theme_object_set(obj, wd->resize_obj,
                                       elm_widget_theme_klass_get(obj),
                                       elm_widget_theme_element_get(obj),
                                       elm_widget_theme_style_get(obj)) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     CRI("Failed to set layout!");

   if (elm_widget_is_legacy(obj))
     elm_layout_content_set(obj, "elm.swallow.video", priv->emotion);
   else
     elm_layout_content_set(obj, "efl.video", priv->emotion);

   efl_event_callback_array_add(priv->emotion, _video_cb(), obj);

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _on_size_hints_changed, NULL);
   efl_composite_attach(obj, priv->emotion);

   priv->timer = ecore_timer_add(20.0, _suspend_cb, obj);
}

/**
 * @internal
 * @brief Deletes the Efl_Ui_Video object from the canvas.
 *
 * This function is the Evas smart 'del' implementation for the video widget.
 * It cleans up resources, including deleting the suspend timer and, if enabled,
 * saving the last playback position.
 *
 * @param obj The video widget object.
 * @param sd The private data for the video widget.
 */
EOLIAN static void
_efl_ui_video_efl_canvas_group_group_del(Eo *obj, Efl_Ui_Video_Data *sd)
{
   ecore_timer_del(sd->timer);
   if (sd->remember) emotion_object_last_position_save(sd->emotion);

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Constructor for the Efl_Ui_Video object.
 *
 * Initializes the object by calling the parent constructor, setting up
 * smart callback descriptions, and setting the accessibility role.
 *
 * @param obj The object being constructed.
 * @param _pd EINA_UNUSED.
 * @return The constructed object.
 */
EOLIAN static Eo *
_efl_ui_video_efl_object_constructor(Eo *obj, Efl_Ui_Video_Data *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_ANIMATION);

   return obj;
}

EOLIAN static void
_efl_ui_video_efl_file_unload(Eo *obj, Efl_Ui_Video_Data *sd)
{
   if (sd->remember) emotion_object_last_position_save(sd->emotion);
   sd->stop = EINA_FALSE;
   efl_file_unload(sd->emotion);

   if(elm_widget_is_legacy(obj))
     elm_layout_signal_emit(obj, "elm,video,stop", "elm");
   else
     elm_layout_signal_emit(obj, "efl,video,stop", "efl");
}

EOLIAN static Eina_Error
_efl_ui_video_efl_file_load(Eo *obj, Efl_Ui_Video_Data *sd)
{
   const char *file = efl_file_get(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(file, EFL_GFX_IMAGE_LOAD_ERROR_DOES_NOT_EXIST);

   if (eina_streq(file, emotion_object_file_get(sd->emotion)) && efl_file_loaded_get(sd->emotion)) return 0;

   if (sd->remember) emotion_object_last_position_save(sd->emotion);
   sd->stop = EINA_FALSE;
   if (!emotion_object_file_set(sd->emotion, file)) return EFL_GFX_IMAGE_LOAD_ERROR_DOES_NOT_EXIST;

   if (file && ((!strncmp(file, "file://", 7)) || (!strstr(file, "://"))))
     emotion_object_last_position_load(sd->emotion);

   if(elm_widget_is_legacy(obj))
     elm_layout_signal_emit(obj, "elm,video,load", "elm");
   else
     elm_layout_signal_emit(obj, "efl,video,load", "efl");

   return 0;
}

EOLIAN static Evas_Object*
_efl_ui_video_emotion_get(const Eo *obj EINA_UNUSED, Efl_Ui_Video_Data *sd)
{
   return sd->emotion;
}

EOLIAN static Eina_Bool
_efl_ui_video_efl_player_paused_set(Eo *obj, Efl_Ui_Video_Data *sd, Eina_Bool paused)
{
   paused = !!paused;
   /* can't pause if we're stopped */
   if (sd->stop) return EINA_FALSE;
   if (emotion_object_play_get(sd->emotion) == !paused) return EINA_TRUE;

   if (!paused)
     {
        ELM_SAFE_FREE(sd->timer, ecore_timer_del);
        emotion_object_play_set(sd->emotion, EINA_TRUE);

        if(elm_widget_is_legacy(obj))
          elm_layout_signal_emit(obj, "elm,video,play", "elm");
        else
          elm_layout_signal_emit(obj, "efl,video,play", "efl");
     }
   else
     {
        /* FIXME: pause will setup timer and go into sleep or
         * hibernate after a while without activity.
         */
        if (!sd->timer) sd->timer = ecore_timer_add(20.0, _suspend_cb, obj);
        emotion_object_play_set(sd->emotion, EINA_FALSE);

        if(elm_widget_is_legacy(obj))
          elm_layout_signal_emit(obj, "elm,video,pause", "elm");
        else
          elm_layout_signal_emit(obj, "efl,video,pause", "efl");
     }
   return EINA_TRUE;
}

/* FIXME: stop should go into hibernate state directly.
 */
EOLIAN static Eina_Bool
_efl_ui_video_efl_player_playing_set(Eo *obj, Efl_Ui_Video_Data *sd, Eina_Bool playing)
{
   playing = !!playing;
   if (playing && emotion_object_play_get(sd->emotion)) return EINA_TRUE;
   if ((!playing) && sd->stop) return EINA_TRUE;
   ELM_SAFE_FREE(sd->timer, ecore_timer_del);
   sd->stop = !playing;
   if (playing)
     {
        emotion_object_play_set(sd->emotion, EINA_TRUE);

        if(elm_widget_is_legacy(obj))
          elm_layout_signal_emit(obj, "elm,video,play", "elm");
        else
          elm_layout_signal_emit(obj, "efl,video,play", "efl");
        return EINA_TRUE;
     }
   efl_player_playback_position_set(obj, 0.0);
   emotion_object_play_set(sd->emotion, EINA_FALSE);

   if(elm_widget_is_legacy(obj))
     elm_layout_signal_emit(obj, "elm,video,stop", "elm");
   else
     elm_layout_signal_emit(obj, "efl,video,stop", "efl");

   emotion_object_suspend_set(sd->emotion, EMOTION_HIBERNATE);
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_ui_video_efl_player_playing_get(const Eo *obj EINA_UNUSED, Efl_Ui_Video_Data *sd)
{
   return emotion_object_play_get(sd->emotion);
}

EOLIAN static Eina_Bool
_efl_ui_video_efl_player_paused_get(const Eo *obj EINA_UNUSED, Efl_Ui_Video_Data *sd)
{
   /* pause is when !playing and !stopped */
   return !emotion_object_play_get(sd->emotion) && !sd->stop;
}

EOLIAN static const char*
_efl_ui_video_title_get(const Eo *obj EINA_UNUSED, Efl_Ui_Video_Data *sd)
{
   return emotion_object_title_get(sd->emotion);
}

EOLIAN static void
_efl_ui_video_remember_position_set(Eo *obj EINA_UNUSED, Efl_Ui_Video_Data *sd, Eina_Bool remember)
{
   sd->remember = remember;
}

EOLIAN static Eina_Bool
_efl_ui_video_remember_position_get(const Eo *obj EINA_UNUSED, Efl_Ui_Video_Data *sd)
{
   return sd->remember;
}

/**
 * @internal
 * @brief Get the accessibility actions available for the video widget.
 *
 * Provides a list of actions that can be performed on the widget through
 * accessibility APIs. The returned array contains structures with the
 * following format:
 * @code
 * { "action_name", "action_type", "action_params", function_pointer }
 * @endcode
 * The available actions are:
 * - @c { "move,left", "move", "left", _key_action_move}
 * - @c { "move,right", "move", "right", _key_action_move}
 * - @c { "play", "play", NULL, _key_action_play}
 *
 * @param obj EINA_UNUSED.
 * @param pd EINA_UNUSED.
 * @return A static, null-terminated array of Efl_Access_Action_Data describing the actions.
 */
EOLIAN const Efl_Access_Action_Data *
_efl_ui_video_efl_access_widget_action_elm_actions_get(const Eo *obj EINA_UNUSED, Efl_Ui_Video_Data *pd EINA_UNUSED)
{
   static Efl_Access_Action_Data atspi_actions[] = {
          { "move,left", "move", "left", _key_action_move},
          { "move,right", "move", "right", _key_action_move},
          { "play", "play", NULL, _key_action_play},
          { NULL, NULL, NULL, NULL}
   };
   return &atspi_actions[0];
}
/* Internal EO APIs and hidden overrides */

ELM_WIDGET_KEY_DOWN_DEFAULT_IMPLEMENT(efl_ui_video, Efl_Ui_Video_Data)

/* Internal EO APIs and hidden overrides */

#define EFL_UI_VIDEO_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(efl_ui_video)

#include "efl_ui_video.eo.c"
#include "efl_ui_video_eo.legacy.c"

#include "efl_ui_video_legacy_eo.h"

#define MY_CLASS_NAME_LEGACY "elm_video"

static void
_efl_ui_video_legacy_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

EOLIAN static Eo *
_efl_ui_video_legacy_efl_object_constructor(Eo *obj, void *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, EFL_UI_VIDEO_LEGACY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   legacy_object_focus_handle(obj);

   return obj;
}

EAPI Evas_Object *
elm_video_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(EFL_UI_VIDEO_LEGACY_CLASS, parent);
}

EAPI Eina_Bool
elm_video_file_set(Eo *obj, const char *filename)
{
   return efl_file_simple_load((Eo *) obj, filename, NULL);
}

EAPI void
elm_video_file_get(Eo *obj, const char **filename)
{
   efl_file_simple_get((Eo *) obj, filename, NULL);
}

EAPI void
elm_video_audio_level_set(Evas_Object *obj, double volume)
{
   efl_audio_control_volume_set(obj, volume);
}

EAPI double
elm_video_audio_level_get(const Evas_Object *obj)
{
   return efl_audio_control_volume_get(obj);
}

EAPI void
elm_video_audio_mute_set(Evas_Object *obj, Eina_Bool mute)
{
   efl_audio_control_mute_set(obj, mute);
}

EAPI Eina_Bool
elm_video_audio_mute_get(const Evas_Object *obj)
{
   return efl_audio_control_mute_get(obj);
}

EAPI double
elm_video_play_length_get(const Evas_Object *obj)
{
   return efl_playable_length_get(obj);
}

EAPI Eina_Bool
elm_video_is_seekable_get(const Evas_Object *obj)
{
   return efl_playable_seekable_get(obj);
}

EAPI void
elm_video_play_position_set(Evas_Object *obj, double position)
{
   efl_player_playback_position_set(obj, position);
}

EAPI double
elm_video_play_position_get(const Evas_Object *obj)
{
   return efl_player_playback_position_get(obj);
}

EAPI Eina_Bool
elm_video_is_playing_get(Evas_Object *obj)
{
   return efl_player_playing_get(obj) && !efl_player_paused_get(obj);
}

EAPI void
elm_video_play(Evas_Object *obj)
{
   if (efl_player_playing_get(obj))
     efl_player_paused_set(obj, EINA_FALSE);
   else
     efl_player_playing_set(obj, EINA_TRUE);
}

EAPI void
elm_video_stop(Evas_Object *obj)
{
   efl_player_playing_set(obj, EINA_FALSE);
}

EAPI void
elm_video_pause(Evas_Object *obj)
{
   efl_player_paused_set(obj, EINA_TRUE);
}

#include "efl_ui_video_legacy_eo.c"
