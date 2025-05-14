#include "private.h"
#include <assert.h>

typedef struct _Elm_Params_Notify Elm_Params_Notify;

/**
 * @brief Structure holding the parameters for a notify widget.
 * @details This structure is used to parse and store parameters from an EDC
 * file or other external sources, which can then be applied to a notify widget.
 */
struct _Elm_Params_Notify
{
   Elm_Params   base; /**< Inherited base parameters. */
   Evas_Object *content;      /**< The content object to be set in the notify widget. */ /* part name whose obj is to be set as content */
   Eina_Bool    allow_events_exists; /**< Flag indicating if allow_events is set. */
   Eina_Bool    allow_events; /**< If EINA_TRUE, events are passed to lower layers. */
   Eina_Bool    timeout_exists; /**< Flag indicating if timeout is set. */
   double       timeout; /**< Timeout in seconds before the notify disappears. */

   const char  *orient; /**< String representation of the orientation. */
};

/**
 * @brief Array of strings representing the possible orientations.
 * @details The index of each string corresponds to the Elm_Notify_Orient enum value.
 * For example, orients[ELM_NOTIFY_ORIENT_TOP] is "top".
 * The array must be terminated with NULL.
 */
static const char *orients[] = {
   "top",
   "center",
   "bottom",
   "left",
   "right",
   "top_left",
   "top_right",
   "bottom_left",
   "bottom_right",
   NULL
};

/**
 * @brief Get the orientation of the notify object.
 * @details This function is a compatibility wrapper to maintain the old external
 * orientation API. It translates the notify object's alignment values (horizontal
 * and vertical) into an Elm_Notify_Orient enum value.
 * @param obj The notify object.
 * @return The orientation of the notify object.
 */
/* keeping old externals orient api for notify, but taking away the
 * introduced deprecation warning by copying the deprecated code
 * here */
static Elm_Notify_Orient
_elm_notify_orient_get(const Evas_Object *obj)
{
   Elm_Notify_Orient orient;
   double horizontal, vertical;

   elm_notify_align_get(obj, &horizontal, &vertical);

   if (EINA_DBL_EQ(horizontal, 0.5) && EINA_DBL_EQ(vertical, 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP;
   else if (EINA_DBL_EQ(horizontal, 0.5) && EINA_DBL_EQ(vertical, 0.5))
     orient = ELM_NOTIFY_ORIENT_CENTER;
   else if (EINA_DBL_EQ(horizontal, 0.5) && EINA_DBL_EQ(vertical, 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM;
   else if (EINA_DBL_EQ(horizontal, 0.0) && EINA_DBL_EQ(vertical, 0.5))
     orient = ELM_NOTIFY_ORIENT_LEFT;
   else if (EINA_DBL_EQ(horizontal, 1.0) && EINA_DBL_EQ(vertical, 0.5))
     orient = ELM_NOTIFY_ORIENT_RIGHT;
   else if (EINA_DBL_EQ(horizontal, 0.0) && EINA_DBL_EQ(vertical, 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP_LEFT;
   else if (EINA_DBL_EQ(horizontal, 1.0) && EINA_DBL_EQ(vertical, 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP_RIGHT;
   else if (EINA_DBL_EQ(horizontal, 0.0) && EINA_DBL_EQ(vertical, 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM_LEFT;
   else if (EINA_DBL_EQ(horizontal, 1.0) && EINA_DBL_EQ(vertical, 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM_RIGHT;
   else
     orient = ELM_NOTIFY_ORIENT_TOP;
   return orient;
}

/**
 * @brief Set the orientation of the notify object.
 * @details This function is a compatibility wrapper that sets the notify object's
 * alignment based on an Elm_Notify_Orient enum value. It translates the
 * orientation into horizontal and vertical alignment values.
 * @param obj The notify object.
 * @param orient The orientation to set.
 */
static void
_elm_notify_orient_set(Evas_Object *obj,
                       Elm_Notify_Orient orient)
{
   double horizontal = 0, vertical = 0;

   switch (orient)
     {
      case ELM_NOTIFY_ORIENT_TOP:
         horizontal = 0.5; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_CENTER:
         horizontal = 0.5; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM:
         horizontal = 0.5; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_LEFT:
         horizontal = 0.0; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_RIGHT:
         horizontal = 1.0; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_TOP_LEFT:
         horizontal = 0.0; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_TOP_RIGHT:
         horizontal = 1.0; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM_LEFT:
         horizontal = 0.0; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM_RIGHT:
         horizontal = 1.0; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_LAST:
        break;
     }

   elm_notify_align_set(obj, horizontal, vertical);
}

/**
 * @brief Get the orientation enum value from a string.
 * @details This function converts a string representation of an orientation
 * (e.g., "top", "center") into the corresponding Elm_Notify_Orient enum value.
 * @param orient The string representation of the orientation.
 * @return The Elm_Notify_Orient enum value, or ELM_NOTIFY_ORIENT_LAST if not found.
 */
static Elm_Notify_Orient
_orient_get(const char *orient)
{
   unsigned int i;

   assert(sizeof(orients) / sizeof(orients[0]) ==
          ELM_NOTIFY_ORIENT_LAST + 1);

   for (i = 0; i < ELM_NOTIFY_ORIENT_LAST; i++)
     if (!strcmp(orient, orients[i])) return i;

   return ELM_NOTIFY_ORIENT_LAST;
}

/**
 * @brief Set the state of the notify object from parameters.
 * @details This function is used to set the visual state of the notify object,
 * typically during a state transition. It applies parameters from either
 * `to_params` or `from_params`.
 * @param data Unused.
 * @param obj The notify object to modify.
 * @param from_params The starting state parameters.
 * @param to_params The ending state parameters.
 * @param pos Unused.
 */
static void
external_notify_state_set(void *data EINA_UNUSED,
                          Evas_Object *obj, const void *from_params,
                          const void *to_params, float pos EINA_UNUSED)
{
   const Elm_Params_Notify *p;

   if (to_params) p = to_params;
   else if (from_params)
     p = from_params;
   else return;

   if (p->content)
     {
        elm_object_content_set(obj, p->content);
     }
   if (p->allow_events_exists)
     elm_notify_allow_events_set(obj, p->allow_events);
   if (p->timeout_exists)
     elm_notify_timeout_set(obj, p->timeout);
   if (p->orient)
     {
        Elm_Notify_Orient set = _orient_get(p->orient);
        if (set == ELM_NOTIFY_ORIENT_LAST) return;
        _elm_notify_orient_set(obj, set);
     }
}

/**
 * @brief Set a parameter for the notify object.
 * @details This function is a callback for the Edje external interface to set
 * a specific parameter on the notify object. It handles parameters like
 * "content", "allow_events", "timeout", and "orient".
 * @param data Unused.
 * @param obj The notify object.
 * @param param The parameter to set.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_notify_param_set(void *data EINA_UNUSED,
                          Evas_Object *obj, const Edje_External_Param *param)
{
   if ((!strcmp(param->name, "content"))
       && (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING))
     {
        Evas_Object *content =
          external_common_param_elm_layout_get(obj, param);
        if ((strcmp(param->s, "")) && (!content))
          return EINA_FALSE;
        elm_object_content_set(obj, content);
        return EINA_TRUE;
     }
   else if ((!strcmp(param->name, "allow_events"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL))
     {
        elm_notify_allow_events_set(obj, param->i);
        return EINA_TRUE;
     }
   else if ((!strcmp(param->name, "timeout"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE))
     {
        elm_notify_timeout_set(obj, param->d);
        return EINA_TRUE;
     }
   else if ((!strcmp(param->name, "orient"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE))
     {
        Elm_Notify_Orient set = _orient_get(param->s);
        if (set == ELM_NOTIFY_ORIENT_LAST) return EINA_FALSE;
        _elm_notify_orient_set(obj, set);
        return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Get a parameter from the notify object.
 * @details This function is a callback for the Edje external interface to get
 * a specific parameter's value from the notify object. It handles parameters
 * like "allow_events", "timeout", and "orient". Getting "content" is not
 * supported.
 * @param data Unused.
 * @param obj The notify object.
 * @param param A pointer to an Edje_External_Param struct to be filled.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_notify_param_get(void *data EINA_UNUSED,
                          const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "content"))
     {
        /* not easy to get content name back from live object */
        return EINA_FALSE;
     }
   else if ((!strcmp(param->name, "allow_events"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL))
     {
        param->i = elm_notify_allow_events_get(obj);
        return EINA_TRUE;
     }
   else if ((!strcmp(param->name, "timeout"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE))
     {
        param->d = elm_notify_timeout_get(obj);
        return EINA_TRUE;
     }
   else if ((!strcmp(param->name, "orient"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE))
     {
        Elm_Notify_Orient set = _elm_notify_orient_get(obj);
        if (set == ELM_NOTIFY_ORIENT_LAST) return EINA_FALSE;
        param->s = orients[set];
        return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parse a list of external parameters for the notify object.
 * @details This function allocates and populates an Elm_Params_Notify structure
 * from a list of Edje_External_Param. This structure is then typically used
 * in state transitions.
 * @param data Unused.
 * @param obj The notify object.
 * @param params A list of Edje_External_Param to parse.
 * @return A newly allocated Elm_Params_Notify structure, or NULL on failure.
 */
static void *
external_notify_params_parse(void *data EINA_UNUSED, Evas_Object *obj,
                             const Eina_List *params)
{
   Elm_Params_Notify *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Notify));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "content"))
          mem->content = external_common_param_elm_layout_get(obj, param);
        else if (!strcmp(param->name, "timeout"))
          {
             mem->timeout = param->d;
             mem->timeout_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "allow_events"))
          {
             mem->allow_events = param->i;
             mem->allow_events_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "orient"))
          mem->orient = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Get a content object from the notify widget.
 * @details This function is a callback for the Edje external interface to
 * retrieve a content part from the notify object. It only supports the "content"
 * part name.
 * @param data Unused.
 * @param obj The notify object.
 * @param content The name of the content part to get (e.g., "content").
 * @return The content Evas_Object, or NULL if not found.
 */
static Evas_Object *
external_notify_content_get(void *data EINA_UNUSED,
                            const Evas_Object *obj, const char *content)
{
   if (!strcmp(content, "content"))
     return elm_object_content_get(obj);

   ERR("unknown content '%s'", content);
   return NULL;
}

/**
 * @brief Free the notify parameters structure.
 * @details This function frees the memory allocated for an Elm_Params_Notify
 * structure, which was created by external_notify_params_parse().
 * @param params The parameter structure to free.
 */
static void
external_notify_params_free(void *params)
{
   free(params);
}

static Edje_External_Param_Info external_notify_params[] =
{
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("content"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("allow_events"),
   EDJE_EXTERNAL_PARAM_INFO_DOUBLE("timeout"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(notify, "notify");
DEFINE_EXTERNAL_TYPE_SIMPLE(notify, "Notify");
