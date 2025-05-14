#include "private.h"

/**
 * @brief Structure to hold parameters for an Elm_Frame widget.
 *
 * This structure extends Elm_Params and includes specific parameters
 * for a frame, such as its label and content object.
 */
typedef struct _Elm_Params_Frame
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   const char *label; /**< The text label to display on the frame. */
   Evas_Object *content; /**< The Evas_Object to be set as the frame's content. This refers to an object identified by a part name. */
} Elm_Params_Frame;

/**
 * @brief Sets the state of an external frame widget.
 *
 * This function is called to apply a new state (defined by Elm_Params_Frame)
 * to the frame object. It updates the label and content based on the
 * provided parameters.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object representing the frame.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The new state parameters to apply (can be NULL).
 * @param pos The position in the transition (unused).
 */
static void
external_frame_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                         const void *from_params, const void *to_params,
                         float pos EINA_UNUSED)
{
   const Elm_Params_Frame *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label) elm_object_text_set(obj, p->label);
   if (p->content) elm_object_content_set(obj, p->content);
}

/**
 * @brief Sets a specific external parameter for the frame widget.
 *
 * This function handles setting individual parameters like "label" or "content"
 * on the frame object. It's used by the Edje external interface.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object representing the frame.
 * @param param The Edje_External_Param to set.
 *              Example for "label": param->name = "label", param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING, param->s = "My Frame"
 *              Example for "content": param->name = "content", param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING, param->s = "my_content_object_name"
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter or type mismatch).
 */
static Eina_Bool
external_frame_param_set(void *data EINA_UNUSED, Evas_Object *obj,
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
   else if (!strcmp(param->name, "content"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             Evas_Object *content =
                external_common_param_elm_layout_get(obj,param);
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
 * @brief Gets a specific external parameter from the frame widget.
 *
 * This function retrieves the value of parameters like "label" from the frame object.
 * It's used by the Edje external interface. Getting the "content" parameter
 * is not straightforward as it involves resolving a part name.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object representing the frame.
 * @param param An Edje_External_Param structure to fill with the parameter's value.
 *              Example for "label": param->name = "label", param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING. param->s will be set.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter or type mismatch).
 */
static Eina_Bool
external_frame_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
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
 * @brief Parses a list of Edje_External_Param to create an Elm_Params_Frame structure.
 *
 * This function converts a list of raw parameters (typically from an Edje theme)
 * into a structured Elm_Params_Frame object that can be used to configure a frame.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object representing the frame, used for context (e.g., resolving content part names).
 * @param params A list of Edje_External_Param structures.
 *               Example `params` list structure:
 *               [
 *                 { name = "label", type = EDJE_EXTERNAL_PARAM_TYPE_STRING, s = "Frame Title" },
 *                 { name = "content", type = EDJE_EXTERNAL_PARAM_TYPE_STRING, s = "content_part_name" }
 *               ]
 * @return A newly allocated Elm_Params_Frame structure, or NULL on failure.
 *         The caller is responsible for freeing this structure using external_frame_params_free().
 */
static void *
external_frame_params_parse(void *data EINA_UNUSED, Evas_Object *obj,
                            const Eina_List *params)
{
   Elm_Params_Frame *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Frame));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "content"))
          mem->content = external_common_param_elm_layout_get(obj, param);
        else if (!strcmp(param->name, "label"))
          mem->label = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Retrieves a content object from the frame by its part name.
 *
 * This function is used to get a specific content part of the frame.
 * For Elm_Frame, it typically returns the main content object if the
 * requested part name is "content".
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object representing the frame.
 * @param content The name of the content part to retrieve (e.g., "content").
 * @return The Evas_Object for the specified content part, or NULL if not found or unknown.
 */
static Evas_Object *external_frame_content_get(void *data EINA_UNUSED,
                                               const Evas_Object *obj EINA_UNUSED,
                                               const char *content EINA_UNUSED)
{
   if (!strcmp(content, "content"))
     return elm_object_content_get(obj);

   ERR("unknown content '%s'", content);
   return NULL;
}

/**
 * @brief Frees an Elm_Params_Frame structure.
 *
 * This function releases the memory allocated for an Elm_Params_Frame,
 * including any stringshared labels.
 *
 * @param params The Elm_Params_Frame structure to free.
 */
static void
external_frame_params_free(void *params)
{
   Elm_Params_Frame *mem = params;

   if (mem->label)
      eina_stringshare_del(mem->label);
   free(params);
}

static Edje_External_Param_Info external_frame_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("label"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("content"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(frame, "frame");
DEFINE_EXTERNAL_TYPE_SIMPLE(frame, "Frame");
