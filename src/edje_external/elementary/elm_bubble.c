#include "private.h"

/**
 * @brief Structure to hold parameters for an Elm_Bubble widget.
 *
 * This structure is used to pass parameters when creating or configuring
 * an Elm_Bubble widget through external Edje interfaces.
 */
typedef struct _Elm_Params_Bubble
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   const char *label; /**< The main text label of the bubble. */
   Evas_Object *icon; /**< An Evas_Object to be used as an icon. */
   const char *info; /**< Additional informational text for the bubble. */
   Evas_Object *content; /**< An Evas_Object to be set as the main content. */
} Elm_Params_Bubble;

/**
 * @brief Sets the state of the bubble widget based on provided parameters.
 *
 * This function is called by the Edje external interface to apply a set of
 * parameters (either `from_params` or `to_params`) to the bubble object `obj`.
 * It typically handles transitions or initial state settings.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (Elm_Bubble) to modify.
 * @param from_params The source state parameters (can be NULL).
 * @param to_params The target state parameters (can be NULL).
 * @param pos The position in the transition (unused in this function).
 */
static void
external_bubble_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                          const void *from_params, const void *to_params,
                          float pos EINA_UNUSED)
{
   const Elm_Params_Bubble *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label) elm_object_text_set(obj, p->label);
   if (p->icon)
     elm_object_part_content_set(obj, "icon", p->icon);
   if (p->info) elm_object_part_text_set(obj, "info", p->info);
   if (p->content) elm_object_content_set(obj, p->content);
}

/**
 * @brief Sets a specific parameter on the bubble widget.
 *
 * This function is called by the Edje external interface to set a single
 * parameter (e.g., "label", "icon") on the bubble object `obj`.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (Elm_Bubble) to modify.
 * @param param The Edje_External_Param to apply.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter,
 *         wrong type, or error during setting).
 */
static Eina_Bool
external_bubble_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                          const Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_object_text_set(obj, param->s);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "icon"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             Evas_Object *icon = external_common_param_icon_get(obj, param);
             if ((strcmp(param->s, "")) && (!icon)) return EINA_FALSE;
             elm_object_part_content_set(obj, "icon", icon);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "info"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_object_part_text_set(obj, "info", param->s);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "content"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             Evas_Object *content = \
                                    external_common_param_elm_layout_get(obj, param);
             if ((strcmp(param->s, "")) && (!content)) return EINA_FALSE;
             elm_object_content_set(obj, content);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific parameter from the bubble widget.
 *
 * This function is called by the Edje external interface to retrieve the value
 * of a single parameter (e.g., "label", "info") from the bubble object `obj`.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (Elm_Bubble) to query.
 * @param param An Edje_External_Param structure to fill with the parameter's
 *              value. The `name` field indicates which parameter to get.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter,
 *         parameter type mismatch, or parameter not gettable).
 */
static Eina_Bool
external_bubble_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                          Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_object_text_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "icon"))
     {
        /* not easy to get icon name back from live object */
        return EINA_FALSE;
     }
   else if (!strcmp(param->name, "info"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_object_part_text_get(obj, "info");
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "content"))
     {
        /* not easy to get content name back from live object */
        return EINA_FALSE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters for a bubble widget.
 *
 * This function allocates and populates an Elm_Params_Bubble structure
 * based on a list of Edje_External_Param provided. This structure can then
 * be used, for example, by external_bubble_state_set.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (Elm_Bubble) context, used for resolving
 *            relative paths or creating associated objects (e.g., icons).
 * @param params A list of Edje_External_Param to parse.
 * @return A pointer to a newly allocated Elm_Params_Bubble structure, or NULL
 *         on allocation failure. The caller is responsible for freeing this
 *         memory using external_bubble_params_free.
 */
static void *
external_bubble_params_parse(void *data EINA_UNUSED, Evas_Object *obj,
                             const Eina_List *params)
{
   Elm_Params_Bubble *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Bubble));
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "info"))
          mem->info = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "content"))
          mem->content = external_common_param_elm_layout_get(obj, param);
        else if (!strcmp(param->name, "label"))
          mem->label = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Retrieves a named content part from the bubble widget.
 *
 * This function is used by the Edje external interface to get a specific
 * content object associated with the bubble. For example, the main content area.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (Elm_Bubble) from which to get content.
 * @param content A string identifying the content part to retrieve (e.g., "content").
 * @return The Evas_Object corresponding to the named content part, or NULL if
 *         the content part name is unknown or not set.
 */
static Evas_Object *external_bubble_content_get(void *data EINA_UNUSED,
                                                const Evas_Object *obj EINA_UNUSED,
                                                const char *content EINA_UNUSED)
{
   if (!strcmp(content, "content"))
     return elm_object_content_get(obj);
   ERR("unknown content '%s'", content);
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Bubble.
 *
 * This function is responsible for releasing the resources held by an
 * Elm_Params_Bubble structure, including any stringshared strings.
 *
 * @param params A pointer to the Elm_Params_Bubble structure to free.
 */
static void
external_bubble_params_free(void *params)
{
   Elm_Params_Bubble *mem = params;

   if (mem->info)
     eina_stringshare_del(mem->info);
   if (mem->label)
      eina_stringshare_del(mem->label);
   free(params);
}

/**
 * @brief Defines the external parameters available for the Elm_Bubble widget.
 *
 * This array provides metadata about the parameters that can be set or get
 * on an Elm_Bubble widget via the Edje external interface. It includes common
 * parameters and bubble-specific ones like "label", "icon", "info", and "content".
 */
static Edje_External_Param_Info external_bubble_params[] =
{
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("label"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("icon"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("info"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("content"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(bubble, "bubble");
DEFINE_EXTERNAL_TYPE_SIMPLE(bubble, "Bubble");
