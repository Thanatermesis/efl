#include "private.h"

/**
 * @brief Structure to hold the parameters for an Elm_Actionslider widget.
 *
 * This structure extends Elm_Params and adds a label specific to the actionslider.
 */
typedef struct _Elm_Params_Actionslider
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   const char *label; /**< The text label to display on the actionslider. */
} Elm_Params_Actionslider;

/**
 * @brief Sets the state of the actionslider widget.
 *
 * This function is called by Edje to update the widget's state based on
 * external parameters. It primarily sets the label of the actionslider.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (actionslider widget) whose state is to be set.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The new state parameters.
 * @param pos Unused position value.
 */
static void
external_actionslider_state_set(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                                const void *from_params, const void *to_params,
                                float pos EINA_UNUSED)
{
   const Elm_Params_Actionslider *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label)
     elm_object_text_set(obj, p->label);
}

/**
 * @brief Sets a specific external parameter for the actionslider widget.
 *
 * This function is called by Edje to set a single parameter. It handles
 * the "label" parameter, updating the actionslider's text.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (actionslider widget) to modify.
 * @param param The Edje_External_Param to set. Expected to be "label" of type STRING.
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_actionslider_param_set(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                                const Edje_External_Param *param)
{
   if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
       && (!strcmp(param->name, "label")))
     {
        elm_object_text_set(obj, param->s);
        return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific external parameter from the actionslider widget.
 *
 * This function is called by Edje to retrieve a single parameter's value.
 * It handles the "label" parameter, returning the actionslider's current text.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (actionslider widget) to query.
 * @param param The Edje_External_Param to fill. Expected to be "label" of type STRING.
 *              The `param->s` field will be set to the current label.
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_actionslider_param_get(void *data EINA_UNUSED, const Evas_Object *obj EINA_UNUSED,
                                Edje_External_Param *param)
{
   if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
       && (!strcmp(param->name, "label")))
     {
        param->s = elm_object_text_get(obj);
        return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje_External_Param and creates an Elm_Params_Actionslider structure.
 *
 * This function is called by Edje to convert a list of parameters from an EDC file
 * into a widget-specific parameter structure. It looks for the "label" parameter.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param params A list of Edje_External_Param structures to parse.
 *               Example of params list structure:
 *               params = [
 *                 (Edje_External_Param){ .name = "label", .type = EDJE_EXTERNAL_PARAM_TYPE_STRING, .s = "My Label" },
 *                 ...
 *               ]
 * @return A pointer to a newly allocated Elm_Params_Actionslider structure,
 *         or NULL on failure. The caller is responsible for freeing this memory
 *         using external_actionslider_params_free().
 */
static void *
external_actionslider_params_parse(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                                   const Eina_List *params)
{
   Elm_Params_Actionslider *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = ELM_NEW(Elm_Params_Actionslider);
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "label"))
          {
             mem->label = eina_stringshare_add(param->s);
             break;
          }
     }

   return mem;
}

/**
 * @brief Retrieves content from the actionslider widget.
 *
 * This function is intended to get content parts from the widget.
 * For actionslider, it currently does not support any content parts and
 * will always return NULL and log an error.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object (the actionslider widget).
 * @param content Unused name of the content part to retrieve.
 * @return Always NULL for actionslider.
 */
static Evas_Object *external_actionslider_content_get(void *data EINA_UNUSED,
		const Evas_Object *obj EINA_UNUSED, const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Actionslider.
 *
 * This function is called by Edje to release the parameter structure
 * previously created by external_actionslider_params_parse().
 *
 * @param params A pointer to the Elm_Params_Actionslider structure to free.
 */
static void
external_actionslider_params_free(void *params)
{
   Elm_Params_Actionslider *mem = params;
   if (mem->label)
     eina_stringshare_del(mem->label);
   free(mem);
}

/**
 * @brief Array describing the external parameters supported by the actionslider widget.
 *
 * This array is used by Edje to understand the parameters that can be
 * passed to the actionslider from an EDC file.
 * It includes common parameters and the "label" string parameter.
 */
static Edje_External_Param_Info external_actionslider_params[] =
{
   DEFINE_EXTERNAL_COMMON_PARAMS, /**< Macro defining common parameters like "disabled". */
   EDJE_EXTERNAL_PARAM_INFO_STRING("label"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(actionslider, "actionslider");
DEFINE_EXTERNAL_TYPE_SIMPLE(actionslider, "Actionslider");
