#include "private.h"

/**
 * @brief Structure to hold the parameters for a slideshow object.
 * This structure is used to parse and store parameters from an Edje external definition.
 */
typedef struct _Elm_Params_Slideshow
{
   Elm_Params base; /**< Base parameters, inherits from Elm_Params */
   double timeout; /**< The timeout in seconds for transitioning between slides. */
   const char *transition; /**< The name of the transition effect to use (e.g., "fade", "horizontal"). */
   const char *layout; /**< The layout mode for the slideshow (e.g., "fullscreen"). */
   Eina_Bool loop:1; /**< Boolean flag indicating if the slideshow should loop. */
   Eina_Bool timeout_exists:1; /**< Flag to indicate if the timeout parameter was provided. */
   Eina_Bool loop_exists:1; /**< Flag to indicate if the loop parameter was provided. */
} Elm_Params_Slideshow;

/**
 * @brief Array of available transition names for the slideshow.
 * The last element must be NULL to mark the end of the array.
 * Example: {"fade", "black_fade", "horizontal", "vertical", "square", NULL}
 */
static const char *transitions[] =
{
   "fade", "black_fade", "horizontal", "vertical", "square", NULL
};
/**
 * @brief Array of available layout names for the slideshow.
 * The last element must be NULL to mark the end of the array.
 * Example: {"fullscreen", "not_fullscreen", NULL}
 */
static const char *layout[] = { "fullscreen", "not_fullscreen", NULL };

/**
 * @brief Sets the state of the slideshow object based on parameters.
 * This function is called by Edje to apply parameters to the slideshow object,
 * typically during state transitions or initialization.
 *
 * @param data User data, not used in this function.
 * @param obj The Evas_Object (slideshow) to set the state for.
 * @param from_params The previous state's parameters (can be NULL).
 * @param to_params The new state's parameters (can be NULL).
 * @param pos The position in the transition (not used in this function).
 */
static void
external_slideshow_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                             const void *from_params, const void *to_params,
                             float pos EINA_UNUSED)
{
   const Elm_Params_Slideshow *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->timeout_exists)
     elm_slideshow_timeout_set(obj , p->timeout);
   if (p->loop_exists)
     elm_slideshow_loop_set(obj, p->loop);
   if (p->transition)
     elm_slideshow_transition_set(obj, p->transition);
   if (p->layout)
     elm_slideshow_layout_set(obj, p->layout);
}

/**
 * @brief Sets a specific external parameter for the slideshow object.
 * This function is called by Edje when an external parameter is set
 * directly on the slideshow object.
 *
 * @param data User data, not used in this function.
 * @param obj The Evas_Object (slideshow) to set the parameter for.
 * @param param The Edje_External_Param to set.
 *              Example for 'timeout': param->name = "timeout", param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, param->d = 5.0
 *              Example for 'loop': param->name = "loop", param->type = EDJE_EXTERNAL_PARAM_TYPE_BOOL, param->i = EINA_TRUE
 *              Example for 'transition': param->name = "transition", param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING, param->s = "fade"
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_slideshow_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                             const Edje_External_Param *param)
{
   if (!strcmp(param->name, "timeout"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             elm_slideshow_timeout_set(obj, param->d);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "loop"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_slideshow_loop_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "transition"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_slideshow_transition_set(obj, param->s);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "layout"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_slideshow_layout_set(obj, param->s);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific external parameter from the slideshow object.
 * This function is called by Edje to retrieve the current value of an
 * external parameter from the slideshow object.
 *
 * @param data User data, not used in this function.
 * @param obj The Evas_Object (slideshow) to get the parameter from.
 * @param param The Edje_External_Param to fill with the parameter's value.
 *              The `param->name` field specifies which parameter to get.
 *              The `param->type` field specifies the expected type.
 *              Example for 'timeout': param->name = "timeout", param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE. param->d will be filled.
 *              Example for 'loop': param->name = "loop", param->type = EDJE_EXTERNAL_PARAM_TYPE_BOOL. param->i will be filled.
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_slideshow_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                             Edje_External_Param *param)
{
   if (!strcmp(param->name, "timeout"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             param->d = elm_slideshow_timeout_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "loop"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_slideshow_loop_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "transition"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_slideshow_transition_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "layout"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_slideshow_layout_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters and stores them in an Elm_Params_Slideshow structure.
 * This function is called by Edje to convert a list of parameters from an Edje
 * definition into a custom structure that can be used by `external_slideshow_state_set`.
 *
 * @param data User data, not used in this function.
 * @param obj The Evas_Object, not used in this function.
 * @param params A list of Edje_External_Param structures to parse.
 *               Example structure of an element in `params` list:
 *               Edje_External_Param {
 *                 name = "timeout",
 *                 type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE,
 *                 d = 5.0
 *               }
 * @return A pointer to a newly allocated Elm_Params_Slideshow structure filled
 *         with the parsed parameters, or NULL on failure. The caller is
 *         responsible for freeing this memory using `external_slideshow_params_free`.
 */
static void *
external_slideshow_params_parse(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const Eina_List *params)
{
   Elm_Params_Slideshow *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Slideshow));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "timeout"))
          {
             mem->timeout = param->d;
             mem->timeout_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "loop"))
          {
             mem->loop = param->i;
             mem->loop_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "transition"))
          {
             mem->transition = param->s;
          }
        else if (!strcmp(param->name, "layout"))
          {
             mem->layout = param->s;
          }
     }

   return mem;
}

/**
 * @brief Retrieves content from the slideshow.
 * This function is intended to allow Edje to get content parts from the
 * external object. For slideshow, this is not implemented and always returns NULL.
 *
 * @param data User data, not used in this function.
 * @param obj The Evas_Object (slideshow), not used in this function.
 * @param content The name of the content part to retrieve, not used.
 * @return Always NULL for slideshow, as it does not provide named content parts this way.
 */
static Evas_Object *external_slideshow_content_get(void *data EINA_UNUSED,
                                                   const Evas_Object *obj EINA_UNUSED,
                                                   const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for slideshow parameters.
 * This function is called by Edje to free the structure returned by
 * `external_slideshow_params_parse`.
 *
 * @param params A pointer to the Elm_Params_Slideshow structure to free.
 *               Currently, this function is a no-op as `external_slideshow_params_parse`
 *               allocates memory with `calloc` but the strings `transition` and `layout`
 *               point to string literals or Edje-managed strings, not heap-allocated copies.
 *               If `Elm_Params_Slideshow` were to store heap-allocated strings,
 *               they would need to be freed here.
 */
static void
external_slideshow_params_free(void *params EINA_UNUSED)
{
   // If mem->transition or mem->layout were strdup'd in _parse,
   // they would be free'd here.
   // free(params) is done by Edje itself if this function is non-NULL.
   // Currently, params is just calloc'd, so Edje will free it.
   return;
}

/**
 * @brief Defines the external parameters available for the slideshow object.
 * This array provides metadata about the parameters that can be set on a slideshow
 * from an Edje file or through API calls.
 * - DEFINE_EXTERNAL_COMMON_PARAMS: Includes common parameters like "id", "class", etc.
 * - EDJE_EXTERNAL_PARAM_INFO_DOUBLE("timeout"): Defines a parameter named "timeout" of type double.
 * - EDJE_EXTERNAL_PARAM_INFO_BOOL("loop"): Defines a parameter named "loop" of type boolean.
 * - EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("transition", "fade", transitions): Defines a choice parameter
 *   named "transition", with a default value "fade", and possible values from the `transitions` array.
 * - EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("layout", "fullscreen", layout): Defines a choice parameter
 *   named "layout", with a default value "fullscreen", and possible values from the `layout` array.
 * - EDJE_EXTERNAL_PARAM_INFO_SENTINEL: Marks the end of the parameter list.
 */
static Edje_External_Param_Info external_slideshow_params[] = {
     DEFINE_EXTERNAL_COMMON_PARAMS,
     EDJE_EXTERNAL_PARAM_INFO_DOUBLE("timeout"),
     EDJE_EXTERNAL_PARAM_INFO_BOOL("loop"),
     EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("transition", "fade", transitions),
     EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("layout", "fullscreen", layout),
     EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(slideshow, "slideshow");
DEFINE_EXTERNAL_TYPE_SIMPLE(slideshow, "Slideshow");
