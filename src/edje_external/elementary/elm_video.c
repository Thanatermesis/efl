#include "private.h"

/**
 * @brief Structure to hold parameters for the video widget.
 * This structure is used to pass parameters when creating or updating
 * a video widget through the Edje external interface.
 */
typedef struct _Elm_Params_Video
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets */
   const char *file; /**< Path to the video file to be played. Mutually exclusive with uri. */
   const char *uri; /**< URI of the video stream to be played. Mutually exclusive with file. */
   Eina_Bool play:1; /**< If true, start playing the video. */
   Eina_Bool play_exists:1; /**< Internal flag to check if 'play' parameter was set. */
   Eina_Bool pause:1; /**< If true, pause the video. */
   Eina_Bool pause_exists:1; /**< Internal flag to check if 'pause' parameter was set. */
   Eina_Bool stop:1; /**< If true, stop the video. */
   Eina_Bool stop_exists:1; /**< Internal flag to check if 'stop' parameter was set. */
   Eina_Bool audio_mute:1; /**< If true, mute the audio. */
   Eina_Bool audio_mute_exists:1; /**< Internal flag to check if 'audio_mute' parameter was set. */
   double audio_level; /**< Audio volume level (0.0 to 1.0). */
   Eina_Bool audio_level_exists:1; /**< Internal flag to check if 'audio_level' parameter was set. */
   double play_position; /**< Playback position in seconds. */
   Eina_Bool play_position_exists:1; /**< Internal flag to check if 'play_position' parameter was set. */
   Eina_Bool remember_position:1; /**< If true, remember the last playback position. */
   Eina_Bool remember_position_exists:1; /**< Internal flag to check if 'remember_position' parameter was set. */
} Elm_Params_Video;

/**
 * @brief Sets the state of the video object based on parameters.
 * This function is called by Edje to apply a new state to the video object,
 * typically during animations or state transitions.
 *
 * @param data Unused.
 * @param obj The Evas_Object (video widget) to modify.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The new state parameters to apply (can be NULL).
 * @param pos Unused.
 */
static void
external_video_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                         const void *from_params, const void *to_params,
                         float pos EINA_UNUSED)
{
   const Elm_Params_Video *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->file) elm_video_file_set(obj, p->file);
   if (p->uri) elm_video_file_set(obj, p->uri);
   if (p->play_exists && p->play) elm_video_play(obj);
   if (p->pause_exists && p->pause) elm_video_pause(obj);
   if (p->stop_exists && p->stop) elm_video_stop(obj);
   if (p->audio_mute_exists) elm_video_audio_mute_set(obj, p->audio_mute);
   if (p->audio_level_exists) elm_video_audio_level_set(obj, p->audio_level);
   if (p->play_position_exists)
     elm_video_play_position_set(obj, p->play_position);
   if (p->remember_position_exists)
     elm_video_remember_position_set(obj, p->remember_position);
}

/**
 * @brief Sets a specific parameter on the video object.
 * This function is called by Edje to set individual parameters on the
 * video object.
 *
 * @param data Unused.
 * @param obj The Evas_Object (video widget) to modify.
 * @param param The parameter to set, including its name, type, and value.
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_video_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                         const Edje_External_Param *param)
{
   if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
       && (!strcmp(param->name, "file")))
     {
        elm_video_file_set(obj, param->s);
        return EINA_TRUE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
       && (!strcmp(param->name, "uri")))
     {
        elm_video_file_set(obj, param->s);
        return EINA_TRUE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
       && (!strcmp(param->name, "play")))
     {
        if (param->i)
          elm_video_play(obj);
        return EINA_TRUE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
       && (!strcmp(param->name, "pause")))
     {
        if (param->i)
          elm_video_pause(obj);
        return EINA_TRUE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
       && (!strcmp(param->name, "stop")))
     {
        if (param->i)
          elm_video_stop(obj);
        return EINA_TRUE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
       && (!strcmp(param->name, "audio mute")))
     {
        elm_video_audio_mute_set(obj, param->i);
        return EINA_TRUE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
       && (!strcmp(param->name, "audio level")))
     {
        elm_video_audio_level_set(obj, param->d);
        return EINA_TRUE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
       && (!strcmp(param->name, "play position")))
     {
        elm_video_play_position_set(obj, param->d);
        return EINA_TRUE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
       && (!strcmp(param->name, "remember position")))
     {
        elm_video_remember_position_set(obj, param->i);
        return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific parameter from the video object.
 * This function is called by Edje to retrieve individual parameters from the
 * video object.
 *
 * @param data Unused.
 * @param obj The Evas_Object (video widget) to query.
 * @param param The parameter to get. The name and type are inputs, and the
 *              value (e.g., param->i, param->s, param->d) is an output.
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_video_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                         Edje_External_Param *param)
{
   if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
       && (!strcmp(param->name, "file")))
     {
        //        param->s = elm_video_file_get(obj);
        //        return EINA_TRUE;
        return EINA_FALSE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
                && (!strcmp(param->name, "uri")))
     {
        //        elm_video_uri_get(obj, param->s);
        //        return EINA_TRUE;
        return EINA_FALSE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
            && (!strcmp(param->name, "play")))
     {
        //        param->i = elm_video_play_get(obj); return EINA_TRUE;
        return EINA_FALSE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
            && (!strcmp(param->name, "pause")))
     {
             //        param->i = elm_video_pause_get(obj); return EINA_TRUE;
        return EINA_FALSE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
            && (!strcmp(param->name, "stop")))
     {
        //        param->i = elm_video_stop_get(obj); return EINA_TRUE;
        return EINA_FALSE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
            && (!strcmp(param->name, "audio mute")))
     {
        param->i = elm_video_audio_mute_get(obj);
        return EINA_TRUE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE) &&
            (!strcmp(param->name, "audio level")))
     {
        param->d = elm_video_audio_level_get(obj);
        return EINA_TRUE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
            && (!strcmp(param->name, "play position")))
     {
        param->d = elm_video_play_position_get(obj);
        return EINA_TRUE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
            && (!strcmp(param->name, "play length")))
     {
        param->d = elm_video_play_length_get(obj);
        return EINA_TRUE;
     }
   else if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
            && (!strcmp(param->name, "remember position")))
     {
        param->i = elm_video_remember_position_get(obj);
        return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'", param->name,
       edje_external_param_type_str(param->type));

   return EINA_FALSE; }

/**
 * @brief Parses a list of Edje_External_Param into an Elm_Params_Video structure.
 * This function is called by Edje to convert a list of parameters (e.g., from an
 * EDC file) into a structured format that can be used by external_video_state_set.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param params A list of Edje_External_Param structures to parse.
 *               Example of params list elements:
 *               - Edje_External_Param { name="file", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="/path/to/video.mp4" }
 *               - Edje_External_Param { name="play", type=EDJE_EXTERNAL_PARAM_TYPE_BOOL, i=1 }
 *               - Edje_External_Param { name="audio_level", type=EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, d=0.8 }
 * @return A pointer to a newly allocated Elm_Params_Video structure, or NULL on failure.
 *         The caller is responsible for freeing this memory using external_video_params_free.
 */
static void * external_video_params_parse(void *data EINA_UNUSED,
                                          Evas_Object *obj EINA_UNUSED,
                                          const Eina_List *params)
{
   Elm_Params_Video *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Video));
   if (!mem) return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "file"))
          mem->file = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "uri"))
          mem->uri = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "play"))
          {
             mem->play = param->i;
             mem->play_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "pause"))
          {
             mem->pause = param->i;
             mem->pause_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "stop"))
          {
             mem->stop = param->i;
             mem->stop_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "audio mute"))
          {
             mem->audio_mute = param->i;
             mem->audio_mute_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "audio level"))
          {
             mem->audio_level = param->d;
             mem->audio_level_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "play position"))
          {
             mem->play_position = param->d;
             mem->play_position_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "remember position"))
          {
             mem->remember_position = param->i;
             mem->remember_position_exists = EINA_TRUE;
          }
     }
   return mem;
}

/**
 * @brief Retrieves a specific content part from the video object.
 * For the video widget, this function is not implemented and always returns NULL,
 * as video objects typically do not have named sub-content parts accessible
 * this way.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param content Unused.
 * @return Always NULL.
 */
static Evas_Object *external_video_content_get(void *data EINA_UNUSED,
                                               const Evas_Object *obj EINA_UNUSED,
                                               const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Video.
 * This function is called by Edje to release the parameter structure
 * previously allocated by external_video_params_parse.
 *
 * @param params A pointer to the Elm_Params_Video structure to free.
 */
static void external_video_params_free(void *params)
{
   Elm_Params_Video *mem = params;

   if (mem->file) eina_stringshare_del(mem->file);
   if (mem->uri) eina_stringshare_del(mem->uri);
   free(params);
}

/**
 * @brief Array describing the parameters supported by the video external type.
 * This array provides metadata about each parameter, including its name and type.
 * It is used by Edje to validate and handle parameters.
 *
 * Example structure of elements in this array:
 * - { "file", EDJE_EXTERNAL_PARAM_TYPE_STRING, ... }
 * - { "play", EDJE_EXTERNAL_PARAM_TYPE_BOOL, ... }
 * - { "audio_level", EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, ... }
 */
static Edje_External_Param_Info external_video_params[] = {
     DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "id", "class", etc. */
     EDJE_EXTERNAL_PARAM_INFO_STRING("file"), /**< Parameter for setting the video file path. */
     EDJE_EXTERNAL_PARAM_INFO_STRING("uri"), /**< Parameter for setting the video URI. */
     EDJE_EXTERNAL_PARAM_INFO_BOOL("play"), /**< Parameter to start video playback. */
     EDJE_EXTERNAL_PARAM_INFO_BOOL("pause"), /**< Parameter to pause video playback. */
     EDJE_EXTERNAL_PARAM_INFO_BOOL("stop"), /**< Parameter to stop video playback. */
     EDJE_EXTERNAL_PARAM_INFO_BOOL("audio mute"), /**< Parameter to mute/unmute audio. */
     EDJE_EXTERNAL_PARAM_INFO_DOUBLE("audio level"), /**< Parameter to set audio volume. */
     EDJE_EXTERNAL_PARAM_INFO_DOUBLE("play position"), /**< Parameter to set playback position. */
     EDJE_EXTERNAL_PARAM_INFO_DOUBLE("play length"), /**< Parameter to get video duration (read-only). */
     EDJE_EXTERNAL_PARAM_INFO_BOOL("remember position"), /**< Parameter to enable/disable remembering playback position. */
     EDJE_EXTERNAL_PARAM_INFO_SENTINEL /**< Marks the end of the parameter list. */
};

DEFINE_EXTERNAL_ICON_ADD(video, "video");
DEFINE_EXTERNAL_TYPE_SIMPLE(video, "Video");
