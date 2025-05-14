#include "private.h"

/**
 * @brief Structure holding the parameters for an Elm_Label widget.
 *
 * This structure is used to pass parameters when creating or updating
 * an Elm_Label widget through the Edje external interface.
 */
typedef struct _Elm_Params_Label
{
   Elm_Params base; /**< Base parameters common to all Elm widgets. */
   const char* label; /**< The text to display on the label. This string is shared. */
} Elm_Params_Label;

/**
 * @brief Sets the state of the label widget.
 *
 * This function is called by Edje to update the widget's state based on
 * parameters. It primarily sets the text of the label.
 *
 * @param data User data, not used in this function.
 * @param obj The Evas_Object (label widget) to modify.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The new state parameters to apply.
 * @param pos The transition position (0.0 to 1.0), not used here.
 */
static void
external_label_state_set(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                         const void *from_params, const void *to_params,
                         float pos EINA_UNUSED)
{
   const Elm_Params_Label *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label) elm_object_text_set(obj, p->label);
}

/**
 * @brief Sets a specific external parameter for the label widget.
 *
 * This function is called by Edje to set a single parameter on the widget.
 * It handles the "label" parameter to update the widget's text.
 *
 * @param data User data, not used in this function.
 * @param obj The Evas_Object (label widget) to modify.
 * @param param The Edje_External_Param to set.
 *              Example for param:
 *              param->name = "label"
 *              param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING
 *              param->s = "New Label Text"
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_label_param_set(void *data EINA_UNUSED, Evas_Object *obj,
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

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific external parameter from the label widget.
 *
 * This function is called by Edje to retrieve the value of a single parameter
 * from the widget. It handles the "label" parameter to get the widget's current text.
 *
 * @param data User data, not used in this function.
 * @param obj The Evas_Object (label widget) to query.
 * @param param The Edje_External_Param to fill with the retrieved value.
 *              Example for param (input):
 *              param->name = "label"
 *              param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING
 *              (output if successful):
 *              param->s will point to the current label text (e.g., "Current Label")
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_label_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
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

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters into an Elm_Params_Label structure.
 *
 * This function is called by Edje to convert a list of raw parameters
 * (e.g., from an EDC file) into a structured format that the widget can use.
 * It specifically looks for the "label" parameter.
 *
 * @param data User data, not used in this function.
 * @param obj The Evas_Object (label widget), not used in this function.
 * @param params A list of Edje_External_Param structures to parse.
 *               Example for params (a list containing one parameter):
 *               element 0: Edje_External_Param {
 *                            name = "label",
 *                            type = EDJE_EXTERNAL_PARAM_TYPE_STRING,
 *                            s = "Initial Label"
 *                          }
 * @return A pointer to a newly allocated Elm_Params_Label structure filled
 *         with the parsed parameters, or NULL on failure. The caller is
 *         responsible for freeing this memory using external_label_params_free().
 */
static void *
external_label_params_parse(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                            const Eina_List *params EINA_UNUSED)
{
   Elm_Params_Label *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = ELM_NEW(Elm_Params_Label);
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "label"))
          mem->label = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Retrieves a content part of the label widget.
 *
 * Labels typically do not have named content parts that can be retrieved
 * in this manner (e.g., like an icon in a button). This function
 * currently indicates that no content parts are available.
 *
 * @param data User data, not used in this function.
 * @param obj The Evas_Object (label widget), not used in this function.
 * @param content The name of the content part to retrieve (e.g., "icon").
 * @return NULL, as labels do not expose content parts this way.
 */
static Evas_Object *external_label_content_get(void *data EINA_UNUSED,
                                               const Evas_Object *obj EINA_UNUSED,
                                               const char *content EINA_UNUSED)
{
   ERR("no content");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Label.
 *
 * This function is responsible for releasing the resources held by an
 * Elm_Params_Label structure, including any shared strings.
 *
 * @param params A pointer to the Elm_Params_Label structure to free.
 */
static void
external_label_params_free(void *params)
{
   Elm_Params_Label *mem = params;
   if (mem->label)
     eina_stringshare_del(mem->label);
   free(params);
}

/**
 * @brief Defines the external parameters supported by the Elm_Label widget.
 *
 * This array describes the parameters that can be used in an EDC file
 * or via Edje external API to configure an Elm_Label.
 * It includes common parameters and a specific "label" string parameter.
 */
static Edje_External_Param_Info external_label_params[] = {
     DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "disabled", "visible", etc. */
     EDJE_EXTERNAL_PARAM_INFO_STRING("label"), /**< Parameter to set the label's text. Expects a string value. */
     EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(label, "label");
DEFINE_EXTERNAL_TYPE_SIMPLE(label, "Label");
