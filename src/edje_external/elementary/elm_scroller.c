#include "private.h"
#include <assert.h>

typedef struct _Elm_Params_Scroller Elm_Params_Scroller;

/**
 * @brief Structure to hold parameters for an external scroller object.
 *
 * This structure extends Elm_Params with scroller-specific parameters.
 */
struct _Elm_Params_Scroller
{
   Elm_Params base; /**< Base parameters common to all Elm objects. */
   Evas_Object *content; /**< The content object to be scrolled. This is typically a layout or another widget. */
};

/**
 * @brief Sets the state of an external scroller object.
 *
 * This function is called when the state of the scroller needs to be updated,
 * for example, during animations or transitions. It applies parameters
 * (like the content object) from either `to_params` or `from_params`.
 *
 * @param data Unused user data.
 * @param obj The scroller Evas_Object to modify.
 * @param from_params The parameters representing the starting state.
 * @param to_params The parameters representing the target state.
 * @param pos The position in the transition (0.0 to 1.0), unused in this function.
 */
static void external_scroller_state_set(void *data EINA_UNUSED,
                                        Evas_Object *obj,
                                        const void *from_params,
                                        const void *to_params,
                                        float pos EINA_UNUSED)
{
   const Elm_Params_Scroller *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->content)
     elm_object_content_set(obj, p->content);
}

/**
 * @brief Sets a specific parameter for an external scroller object.
 *
 * This function is called by Edje to set a parameter on the scroller
 * object based on external definitions. It currently supports setting
 * the "content" of the scroller.
 *
 * @param data Unused user data.
 * @param obj The scroller Evas_Object to modify.
 * @param param The Edje_External_Param to apply.
 *              Example for 'content': param->name = "content",
 *                                     param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING,
 *                                     param->s = "name_of_content_object_in_layout"
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool external_scroller_param_set(void *data EINA_UNUSED,
                                             Evas_Object *obj,
                                             const Edje_External_Param *param)
{
   if (!strcmp(param->name, "content")
       && param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
     {
        Evas_Object *content = external_common_param_elm_layout_get(obj, param);
        if ((strcmp(param->s, "")) && (!content))
          return EINA_FALSE;
        elm_object_content_set(obj, content);
        return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific parameter from an external scroller object.
 *
 * This function is called by Edje to retrieve a parameter value from the
 * scroller object. Currently, it does not support getting the "content"
 * parameter effectively.
 *
 * @param data Unused user data.
 * @param obj The scroller Evas_Object to query (unused).
 * @param param The Edje_External_Param to fill with the retrieved value.
 *              Example for 'content': param->name = "content"
 * @return EINA_FALSE, as getting content name is not straightforward.
 */
static Eina_Bool external_scroller_param_get(void *data EINA_UNUSED,
                                             const Evas_Object *obj EINA_UNUSED,
                                             Edje_External_Param *param)
{
   if (!strcmp(param->name, "content"))
     {
        /* not easy to get content name back from live object */
        return EINA_FALSE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters and creates an Elm_Params_Scroller structure.
 *
 * This function iterates through a list of parameters (e.g., from an Edje theme)
 * and populates an Elm_Params_Scroller structure. It specifically looks for
 * the "content" parameter to identify the scroller's content object.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (scroller) to which these parameters will apply.
 * @param params A list of Edje_External_Param structures.
 *               Example structure of elements in `params` list:
 *               [
 *                 { name = "content", type = EDJE_EXTERNAL_PARAM_TYPE_STRING, s = "my_content_object" },
 *                 ... (other common params)
 *               ]
 * @return A pointer to the newly allocated and populated Elm_Params_Scroller structure,
 *         or NULL on failure.
 */
static void * external_scroller_params_parse(void *data EINA_UNUSED,
                                             Evas_Object *obj,
                                             const Eina_List *params)
{
   Elm_Params_Scroller *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = ELM_NEW(Elm_Params_Scroller);
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "content"))
          mem->content = external_common_param_elm_layout_get(obj, param);
     }

   return mem;
}

/**
 * @brief Retrieves a specific content part from the external scroller object.
 *
 * This function is used to get a named content object from the scroller.
 * It currently supports retrieving the main "content" of the scroller.
 *
 * @param data Unused user data.
 * @param obj The scroller Evas_Object from which to get the content.
 * @param content A string identifying the content part to retrieve.
 *                Example: "content"
 * @return The Evas_Object representing the requested content, or NULL if not found
 *         or if the content name is unknown.
 */
static Evas_Object *external_scroller_content_get(void *data EINA_UNUSED,
                                                  const Evas_Object *obj,
                                                  const char *content)
{
   if (!strcmp(content, "content"))
     return elm_object_content_get(obj);

   ERR("unknown content '%s'", content);
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Scroller.
 *
 * This function is responsible for releasing resources associated with
 * the scroller parameters, typically an Elm_Params_Scroller structure.
 * It calls the common parameter freeing function.
 *
 * @param params A pointer to the Elm_Params_Scroller structure to free.
 */
static void external_scroller_params_free(void *params)
{
   external_common_params_free(params);
}

/**
 * @brief Array defining the external parameters for a scroller.
 *
 * This array provides metadata about the parameters that can be
 * configured for an external scroller object via Edje. It includes
 * common parameters and the scroller-specific "content" parameter.
 */
static Edje_External_Param_Info external_scroller_params[] =
{
   DEFINE_EXTERNAL_COMMON_PARAMS, /**< Macro defining common external parameters like "disabled", "visible", etc. */
   EDJE_EXTERNAL_PARAM_INFO_STRING("content"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(scroller, "scroller");
DEFINE_EXTERNAL_TYPE_SIMPLE(scroller, "Scroller");
