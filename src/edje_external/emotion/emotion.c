#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Edje.h>

#include "Emotion.h"

typedef struct _External_Emotion_Params External_Emotion_Params;
typedef struct _External_Emotion_Signals_Proxy_Context External_Emotion_Signals_Proxy_Context;

/**
 * @brief Structure to hold parameters for an external Emotion object.
 * This structure is used to parse and store parameters from Edje
 * data collections, which are then applied to an Emotion object.
 * The macros _STR, _BOOL, _INT, _DOUBLE are helpers to define
 * members and their existence flags.
 */
struct _External_Emotion_Params
{
#define _STR(M) const char *M
#define _BOOL(M) Eina_Bool M:1; Eina_Bool M##_exists:1
#define _INT(M) int M; Eina_Bool M##_exists:1
#define _DOUBLE(M) double M; Eina_Bool M##_exists:1
   _STR(file);
   _BOOL(play);
   _DOUBLE(position);
   _BOOL(smooth_scale);
   _DOUBLE(audio_volume);
   _BOOL(audio_mute);
   _INT(audio_channel);
   _BOOL(video_mute);
   _INT(video_channel);
   _BOOL(spu_mute);
   _INT(spu_channel);
   _INT(chapter);
   _DOUBLE(play_speed);
   _DOUBLE(play_length);
   //_INT(vis);
#undef _STR
#undef _BOOL
#undef _INT
#undef _DOUBLE
};

/**
 * @brief Context for proxying Emotion object signals to Edje object signals.
 * This structure holds the necessary information to forward a signal
 * from an Emotion object to its corresponding Edje object, allowing
 * Edje themes to react to Emotion events.
 */
struct _External_Emotion_Signals_Proxy_Context
{
   const char *emission; /**< The signal name to be emitted by the Edje object. */
   const char *source;   /**< The source part name for the Edje signal. */
   Evas_Object *edje;    /**< The Edje object that will emit the signal. */
};

static int _log_dom = -1;
#define CRI(...) EINA_LOG_DOM_CRIT(_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_log_dom, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_log_dom, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_log_dom, __VA_ARGS__)
#define DBG(...) EINA_LOG_DOM_DBG(_log_dom, __VA_ARGS__)

static const char *_external_emotion_engines[] = {
#if EMOTION_BUILD_GSTREAMER1
  "gstreamer1",
#endif
  NULL,
};

static const char _external_emotion_engine_def[] =
#if defined(EMOTION_BUILD_GSTREAMER1)
  "gstreamer1";
#else
  "impossible";
#endif

/**
 * @brief Frees the context used for signal proxying.
 * This function is called when the Emotion object is deleted.
 * @param data The context to free (External_Emotion_Signals_Proxy_Context).
 * @param e Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
static void
_external_emotion_signal_proxy_free_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   External_Emotion_Signals_Proxy_Context *ctxt = data;
   free(ctxt);
}

/**
 * @brief Callback for Emotion object signals that proxies them to an Edje object.
 * When an Emotion object emits a signal, this function is called,
 * and it, in turn, emits a corresponding signal on the associated Edje object.
 * @param data The context containing emission, source, and Edje object (External_Emotion_Signals_Proxy_Context).
 * @param obj Unused.
 * @param event_info Unused.
 */
static void
_external_emotion_signal_proxy_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   External_Emotion_Signals_Proxy_Context *ctxt = data;
   // TODO: Is it worth to check Evas_Smart_Cb_Description and do something
   // TODO: with event_info given its description?
   edje_object_signal_emit(ctxt->edje, ctxt->emission, ctxt->source);
}

/**
 * @brief Creates and initializes an Emotion object for use as an Edje external object.
 * This function is called by Edje when an "emotion" external part is encountered.
 * It creates an Emotion object, initializes it with the specified engine, and
 * sets up signal proxying so that Emotion signals are forwarded to the Edje object.
 * @param data Unused.
 * @param evas The Evas canvas.
 * @param edje The Edje object that contains this external part.
 * @param params A list of Edje_External_Param defining initial properties.
 *               Example: A list containing an Edje_External_Param with name "engine" and value "gstreamer1".
 * @param part_name The name of the Edje part this Emotion object is associated with.
 * @return A new Evas_Object (Emotion object) on success, or NULL on failure.
 */
static Evas_Object *
_external_emotion_add(void *data EINA_UNUSED, Evas *evas, Evas_Object *edje EINA_UNUSED, const Eina_List *params, const char *part_name)
{
   const Evas_Smart_Cb_Description **cls_descs, **inst_descs;
   unsigned int cls_count, inst_count, total;
   External_Emotion_Signals_Proxy_Context *ctxt;
   Evas_Object *obj;
   const char *engine;

   if (!edje_external_param_choice_get(params, "engine", &engine))
     engine = NULL;
   if (!engine) engine = _external_emotion_engine_def;

   obj = emotion_object_add(evas);
   if (!emotion_object_init(obj, engine))
     {
	ERR("failed to initialize emotion with engine '%s'.", engine);
	return NULL;
     }

   evas_object_smart_callbacks_descriptions_get
     (obj, &cls_descs, &cls_count, &inst_descs, &inst_count);

   total = cls_count + inst_count;
   if (!total) goto end;
   ctxt = malloc(sizeof(External_Emotion_Signals_Proxy_Context) * total);
   if (!ctxt) goto end;
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_DEL, _external_emotion_signal_proxy_free_cb, ctxt);

   for (; cls_count > 0; cls_count--,  cls_descs++, ctxt++)
     {
	const Evas_Smart_Cb_Description *d = *cls_descs;
	ctxt->emission = d->name;
	ctxt->source = part_name;
	ctxt->edje = edje;
	evas_object_smart_callback_add
	  (obj, d->name, _external_emotion_signal_proxy_cb, ctxt);
     }

   for (; inst_count > 0; inst_count--,  inst_descs++, ctxt++)
     {
	const Evas_Smart_Cb_Description *d = *inst_descs;
	ctxt->emission = d->name;
	ctxt->source = part_name;
	ctxt->edje = edje;
	evas_object_smart_callback_add
	  (obj, d->name, _external_emotion_signal_proxy_cb, ctxt);
     }

 end:
   return obj;
}

/**
 * @brief Handles signals emitted from Edje to the Emotion external object.
 * This function is called when Edje emits a signal targeted at this external part.
 * Currently, it only logs the received signal.
 * @param data Unused.
 * @param obj The Emotion object.
 * @param signal The signal string (e.g., "play", "stop").
 * @param source The source string of the signal.
 */
static void
_external_emotion_signal(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *signal, const char *source)
{
   DBG("External Signal received: '%s' '%s'", signal, source);
}

/**
 * @brief Sets the state of the Emotion object based on Edje state descriptions.
 * This function is called by Edje to apply a state (defined in the EDC) to
 * the Emotion object. It parses parameters from `from_params` or `to_params`
 * and applies them to the Emotion object.
 * @param data Unused.
 * @param obj The Emotion object.
 * @param from_params The parameters of the starting state (External_Emotion_Params).
 * @param to_params The parameters of the target state (External_Emotion_Params).
 * @param pos Unused (represents the position in a transition, not used here).
 */
static void
_external_emotion_state_set(void *data EINA_UNUSED, Evas_Object *obj, const void *from_params, const void *to_params, float pos EINA_UNUSED)
{
   const External_Emotion_Params *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

#define _STR(M) if (p->M) emotion_object_##M##_set(obj, p->M)
#define _BOOL(M) if (p->M##_exists) emotion_object_##M##_set(obj, p->M)
#define _INT(M) if (p->M##_exists) emotion_object_##M##_set(obj, p->M)
#define _DOUBLE(M) if (p->M##_exists) emotion_object_##M##_set(obj, p->M)
   _STR(file);
   _BOOL(play);
   //_DOUBLE(position);
   if (p->position_exists)
     WRN("position should not be set from state description! Ignored.");
   _BOOL(smooth_scale);
   _DOUBLE(audio_volume);
   _BOOL(audio_mute);
   _INT(audio_channel);
   _BOOL(video_mute);
   _INT(video_channel);
   _BOOL(spu_mute);
   _INT(spu_channel);
   _INT(chapter);
   _DOUBLE(play_speed);
   if (p->play_length_exists) ERR("play_length is read-only");
   //_INT(vis);
#undef _STR
#undef _BOOL
#undef _INT
#undef _DOUBLE
}

/**
 * @brief Sets a specific parameter on the Emotion object.
 * This function is called by Edje to set a single parameter on the Emotion object,
 * typically from an EDC script action like `param_set`.
 * @param data Unused.
 * @param obj The Emotion object.
 * @param param The parameter to set (Edje_External_Param).
 *              Example: param->name = "file", param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING, param->s = "/path/to/video.mp4".
 *              Example: param->name = "play", param->type = EDJE_EXTERNAL_PARAM_TYPE_BOOL, param->i = EINA_TRUE.
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
_external_emotion_param_set(void *data EINA_UNUSED, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "engine"))
     {
	// TODO
	WRN("engine is a property that can be set only at object creation!");
	return EINA_FALSE;
     }

#define _STR(M)						\
   else if (!strcmp(param->name, #M))				\
     {								\
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)	\
	  {							\
	     emotion_object_##M##_set(obj, param->s);		\
	     return EINA_TRUE;					\
	  }							\
     }
#define _BOOL(M)						\
   else if (!strcmp(param->name, #M))				\
     {								\
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)	\
	  {							\
	     emotion_object_##M##_set(obj, param->i);		\
	     return EINA_TRUE;					\
	  }							\
     }
#define _INT(M)						\
   else if (!strcmp(param->name, #M))				\
     {								\
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)	\
	  {							\
	     emotion_object_##M##_set(obj, param->i);		\
	     return EINA_TRUE;					\
	  }							\
     }
#define _DOUBLE(M)						\
   else if (!strcmp(param->name, #M))				\
     {								\
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)	\
	  {							\
	     emotion_object_##M##_set(obj, param->d);		\
	     return EINA_TRUE;					\
	  }							\
     }

   if (0) {} // so else if works...
   _STR(file)
   _BOOL(play)
   _DOUBLE(position)
   _BOOL(smooth_scale)
   _DOUBLE(audio_volume)
   _BOOL(audio_mute)
   _INT(audio_channel)
   _BOOL(video_mute)
   _INT(video_channel)
   _BOOL(spu_mute)
   _INT(spu_channel)
   _INT(chapter)
   _DOUBLE(play_speed)
   else if (!strcmp(param->name, "play_length"))
     {
        ERR("play_length is read-only");
        return EINA_FALSE;
     }
   //_INT(vis);
#undef _STR
#undef _BOOL
#undef _INT
#undef _DOUBLE

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific parameter from the Emotion object.
 * This function is called by Edje to retrieve the value of a single parameter
 * from the Emotion object.
 * @param data Unused.
 * @param obj The Emotion object (const, as this is a get operation).
 * @param param An Edje_External_Param structure to be filled with the parameter's value.
 *              The `name` field of `param` indicates which parameter to get.
 *              Example: param->name = "file", param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING.
 *                       After call, param->s will contain the current file path.
 *              Example: param->name = "play_length", param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE.
 *                       After call, param->d will contain the video duration.
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
_external_emotion_param_get(void *data EINA_UNUSED, const Evas_Object *obj, Edje_External_Param *param)
{
#define _STR(M)						\
   else if (!strcmp(param->name, #M))				\
     {								\
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)	\
	  {							\
	     param->s = emotion_object_##M##_get(obj);		\
	     return EINA_TRUE;					\
	  }							\
     }
#define _BOOL(M)						\
   else if (!strcmp(param->name, #M))				\
     {								\
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)	\
	  {							\
	     param->i = emotion_object_##M##_get(obj);		\
	     return EINA_TRUE;					\
	  }							\
     }
#define _INT(M)						\
   else if (!strcmp(param->name, #M))				\
     {								\
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)	\
	  {							\
	     param->i = emotion_object_##M##_get(obj);		\
	     return EINA_TRUE;					\
	  }							\
     }
#define _DOUBLE(M)						\
   else if (!strcmp(param->name, #M))				\
     {								\
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)	\
	  {							\
	     param->d = emotion_object_##M##_get(obj);		\
	     return EINA_TRUE;					\
	  }							\
     }

   if (0) {} // so else if works...
   _STR(file)
   _BOOL(play)
   _DOUBLE(position)
   _BOOL(smooth_scale)
   _DOUBLE(audio_volume)
   _BOOL(audio_mute)
   _INT(audio_channel)
   _BOOL(video_mute)
   _INT(video_channel)
   _BOOL(spu_mute)
   _INT(spu_channel)
   _INT(chapter)
   _DOUBLE(play_speed)
   _DOUBLE(play_length)
   //_INT(vis)
#undef _STR
#undef _BOOL
#undef _INT
#undef _DOUBLE

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters into an External_Emotion_Params structure.
 * This function is called by Edje to convert a list of parameters (typically from
 * an EDC state description) into a custom structure that can be used by
 * `_external_emotion_state_set`.
 * @param data Unused.
 * @param obj Unused.
 * @param params A list of Edje_External_Param.
 *               Example: A list containing parameters like:
 *               - {name="file", type=STRING, s="video.ogv"}
 *               - {name="play", type=BOOL, i=EINA_TRUE}
 *               - {name="audio_volume", type=DOUBLE, d=0.5}
 * @return A pointer to a newly allocated External_Emotion_Params structure filled
 *         with values from `params`, or NULL on allocation failure.
 */
static void *
_external_emotion_params_parse(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const Eina_List *params)
{
   const Edje_External_Param *param;
   const Eina_List *l;
   External_Emotion_Params *p = calloc(1, sizeof(External_Emotion_Params));
   if (!p) return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
#define _STR(M)					\
	if (!strcmp(param->name, #M)) p->M = eina_stringshare_add(param->s)
#define _BOOL(M)					\
	if (!strcmp(param->name, #M))			\
	  {						\
	     p->M = param->i;				\
	     p->M##_exists = EINA_TRUE;		\
	  }
#define _INT(M)					\
	if (!strcmp(param->name, #M))			\
	  {						\
	     p->M = param->i;				\
	     p->M##_exists = EINA_TRUE;		\
	  }
#define _DOUBLE(M)				\
	if (!strcmp(param->name, #M))			\
	  {						\
	     p->M = param->d;				\
	     p->M##_exists = EINA_TRUE;		\
	  }

	_STR(file);
	_BOOL(play);
	_DOUBLE(position);
	_BOOL(smooth_scale);
	_DOUBLE(audio_volume);
	_BOOL(audio_mute);
	_INT(audio_channel);
	_BOOL(video_mute);
	_INT(video_channel);
	_BOOL(spu_mute);
	_INT(spu_channel);
	_INT(chapter);
	_DOUBLE(play_speed);
	_DOUBLE(play_length);
	//_INT(vis);
#undef _STR
#undef _BOOL
#undef _INT
#undef _DOUBLE
     }

   return p;
}

/**
 * @brief Frees an External_Emotion_Params structure.
 * This function is called by Edje to release the memory allocated by
 * `_external_emotion_params_parse`.
 * @param params A pointer to the External_Emotion_Params structure to free.
 */
static void
_external_emotion_params_free(void *params)
{
   External_Emotion_Params *p = params;

#define _STR(M) eina_stringshare_del(p->M)
#define _BOOL(M) do {} while (0)
#define _INT(M) do {} while (0)
#define _DOUBLE(M) do {} while (0)
   _STR(file);
   _BOOL(play);
   _DOUBLE(position);
   _BOOL(smooth_scale);
   _DOUBLE(audio_volume);
   _BOOL(audio_mute);
   _INT(audio_channel);
   _BOOL(video_mute);
   _INT(video_channel);
   _BOOL(spu_mute);
   _INT(spu_channel);
   _INT(chapter);
   _DOUBLE(play_speed);
   _DOUBLE(play_length);
   //_INT(vis);
#undef _STR
#undef _BOOL
#undef _INT
#undef _DOUBLE
   free(p);
}

/**
 * @brief Gets the human-readable label for this external type.
 * This label is used in UI elements, like an editor.
 * @param data Unused.
 * @return A string literal "Emotion".
 */
static const char *
_external_emotion_label_get(void *data EINA_UNUSED)
{
    return "Emotion";
}

/**
 * @brief Creates an icon for this external type.
 * This icon is used in UI elements, like an editor, to represent the Emotion external object.
 * @param data Unused.
 * @param e The Evas canvas on which to create the icon.
 * @return An Evas_Object (Edje object) representing the icon, or NULL on failure.
 */
static Evas_Object *
_external_emotion_icon_add(void *data EINA_UNUSED, Evas *e)
{
   Evas_Object *ic;
   int w = 0, h = 0;

   ic = edje_object_add(e);
   edje_object_file_set(ic, PACKAGE_DATA_DIR"/data/icon.edj", "icon");
   edje_object_size_min_get(ic, &w, &h);
   if (w < 1) w = 20;
   if (h < 1) h = 10;
   evas_object_size_hint_min_set(ic, w, h);
   evas_object_size_hint_max_set(ic, w, h);

   return ic;
}

/**
 * @brief Translates a string related to this external type.
 * This function is intended for internationalization of parameter names or descriptions.
 * Currently, it returns the original string.
 * @param data Unused.
 * @param orig The original string to translate.
 * @return The translated string (or `orig` if no translation is available).
 */
static const char *
_external_emotion_translate(void *data EINA_UNUSED, const char *orig)
{
   // in future, mark all params as translatable and use dgettext()
   // with "emotion" text domain here.
   return orig;
}

/**
 * @brief Array describing the parameters supported by the Emotion external type.
 * This information is used by tools like Edje editors to provide a UI
 * for configuring Emotion objects. Each entry defines a parameter's name,
 * type, default value, and possible choices (for choice types).
 *
 * Example structure of elements:
 * - EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("engine", _external_emotion_engine_def, _external_emotion_engines)
 *   Defines a "choice" parameter named "engine".
 *   _external_emotion_engine_def is the default value (e.g., "gstreamer1").
 *   _external_emotion_engines is an array of const char* for available choices (e.g., {"gstreamer1", NULL}).
 * - EDJE_EXTERNAL_PARAM_INFO_STRING("file")
 *   Defines a "string" parameter named "file".
 * - EDJE_EXTERNAL_PARAM_INFO_BOOL_DEFAULT("play", EINA_FALSE)
 *   Defines a "boolean" parameter named "play" with a default value of EINA_FALSE.
 * - EDJE_EXTERNAL_PARAM_INFO_DOUBLE("position")
 *   Defines a "double" parameter named "position".
 * - EDJE_EXTERNAL_PARAM_INFO_INT_DEFAULT("audio_channel", 0)
 *   Defines an "integer" parameter named "audio_channel" with a default value of 0.
 */
static Edje_External_Param_Info _external_emotion_params[] = {
  EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL
  ("engine", _external_emotion_engine_def, _external_emotion_engines),
  EDJE_EXTERNAL_PARAM_INFO_STRING("file"),
  EDJE_EXTERNAL_PARAM_INFO_BOOL_DEFAULT("play", EINA_FALSE),
  EDJE_EXTERNAL_PARAM_INFO_DOUBLE("position"),
  EDJE_EXTERNAL_PARAM_INFO_BOOL_DEFAULT("smooth_scale", EINA_FALSE),
  EDJE_EXTERNAL_PARAM_INFO_DOUBLE_DEFAULT("audio_volume", 0.9),
  EDJE_EXTERNAL_PARAM_INFO_BOOL_DEFAULT("audio_mute", EINA_FALSE),
  EDJE_EXTERNAL_PARAM_INFO_INT_DEFAULT("audio_channel", 0),
  EDJE_EXTERNAL_PARAM_INFO_BOOL_DEFAULT("video_mute", EINA_FALSE),
  EDJE_EXTERNAL_PARAM_INFO_INT_DEFAULT("video_channel", 0),
  EDJE_EXTERNAL_PARAM_INFO_BOOL_DEFAULT("spu_mute", EINA_FALSE),
  EDJE_EXTERNAL_PARAM_INFO_INT_DEFAULT("spu_channel", 0),
  EDJE_EXTERNAL_PARAM_INFO_INT("chapter"),
  EDJE_EXTERNAL_PARAM_INFO_DOUBLE_DEFAULT("play_speed", 1.0),
  EDJE_EXTERNAL_PARAM_INFO_DOUBLE("play_length"),
  //EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("vis", ...),
  EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

static const Edje_External_Type _external_emotion_type = {
    .abi_version = EDJE_EXTERNAL_TYPE_ABI_VERSION,
    .module = "emotion",
    .module_name = "Emotion",
    .add = _external_emotion_add,
    .state_set = _external_emotion_state_set,
    .signal_emit = _external_emotion_signal,
    .param_set = _external_emotion_param_set,
    .param_get = _external_emotion_param_get,
    .params_parse = _external_emotion_params_parse,
    .params_free = _external_emotion_params_free,
    .label_get = _external_emotion_label_get,
    .description_get = NULL,
    .icon_add = _external_emotion_icon_add,
    .preview_add = NULL,
    .translate = _external_emotion_translate,
    .parameters_info = _external_emotion_params,
    .data = NULL
};

static Edje_External_Type_Info _external_emotion_types[] =
{
  {"emotion", &_external_emotion_type},
  {NULL, NULL}
};

/**
 * @brief Initializes the Emotion external module.
 * This function is called when the module is loaded. It registers the
 * "emotion-externals" log domain and registers the Emotion external type
 * with Edje.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
external_emotion_mod_init(void)
{
   _log_dom = eina_log_domain_register
     ("emotion-externals", EINA_COLOR_LIGHTBLUE);
   edje_external_type_array_register(_external_emotion_types);
   return EINA_TRUE;
}

/**
 * @brief Shuts down the Emotion external module.
 * This function is called when the module is unloaded. It unregisters
 * the Emotion external type from Edje and unregisters the log domain.
 */
static void
external_emotion_mod_shutdown(void)
{
   edje_external_type_array_unregister(_external_emotion_types);
   eina_log_domain_unregister(_log_dom);
   _log_dom = -1;
}

EINA_MODULE_INIT(external_emotion_mod_init);
EINA_MODULE_SHUTDOWN(external_emotion_mod_shutdown);
