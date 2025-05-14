#include "private.h"

/**
 * @brief Structure to hold parameters for the combobox widget.
 *
 * This structure extends Elm_Params and adds a guide text specific to the combobox.
 */
typedef struct
{
   Elm_Params base; /**< Base Elementary parameters */
   Eina_Stringshare *guide; /**< The guide text for the combobox (e.g., "Click to select an item") */
} Elm_Params_Combobox;


/**
 * @brief Sets the state of the external combobox widget.
 *
 * This function is called when the state of the combobox (e.g., guide text)
 * needs to be updated based on provided parameters.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (combobox widget) whose state is to be set.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The new state parameters to apply.
 * @param pos The position for animations/transitions, unused in this function.
 */
static void
external_combobox_state_set(void        *data        EINA_UNUSED,
                            Evas_Object *obj,
                            const void  *from_params,
                            const void  *to_params,
                            float        pos         EINA_UNUSED)
{
   const Elm_Params_Combobox *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->guide) elm_object_part_text_set(obj, "guide", p->guide);
}

/**
 * @brief Sets a specific external parameter for the combobox widget.
 *
 * This function handles setting individual parameters like "guide" text.
 * It's typically called by the Edje theme engine.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (combobox widget) to modify.
 * @param param The Edje_External_Param to set.
 *              Example: param->name = "guide", param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING, param->s = "Select an option"
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_combobox_param_set(void                      *data  EINA_UNUSED,
                            Evas_Object               *obj,
                            const Edje_External_Param *param)
{
   if (!strcmp(param->name, "guide"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_object_part_text_set(obj, "guide", param->s);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'", param->name,
       edje_external_param_type_str(param->type));
   return EINA_FALSE;
}

/**
 * @brief Gets a specific external parameter from the combobox widget.
 *
 * This function retrieves the value of parameters like "guide" text.
 * It's typically called by the Edje theme engine.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (combobox widget) to query.
 * @param param An Edje_External_Param structure to fill with the parameter's value.
 *              Example: param->name = "guide", param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING.
 *                       After the call, param->s will contain the guide text.
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_combobox_param_get(void                *data  EINA_UNUSED,
                            const Evas_Object   *obj,
                            Edje_External_Param *param)
{
   if (!strcmp(param->name, "guide"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_object_part_text_get(obj, "guide");
             return EINA_TRUE;
          }
     }

   ERR("Unknown parameter '%s' f type %s", param->name,
       edje_external_param_type_str(param->type));
   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje_External_Param and creates an Elm_Params_Combobox structure.
 *
 * This function is used to convert a list of parameters (typically from an EDC file)
 * into a structured format (Elm_Params_Combobox) for easier use.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (combobox widget), unused in this function.
 * @param params A list of Edje_External_Param to parse.
 *               Example: A list containing one Edje_External_Param where:
 *                        param->name = "guide",
 *                        param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING,
 *                        param->s = "Choose one..."
 * @return A pointer to a newly allocated Elm_Params_Combobox structure, or NULL on failure.
 *         The caller is responsible for freeing this memory using external_combobox_params_free().
 */
static void *
external_combobox_params_parse(void            *data   EINA_UNUSED,
                               Evas_Object     *obj    EINA_UNUSED,
                               const Eina_List *params)
{
   Elm_Params_Combobox *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(*mem));
   if (EINA_UNLIKELY(!mem))
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "guide"))
          {
             mem->guide = eina_stringshare_add(param->s);
          }
     }

   return mem;
}

/**
 * @brief Retrieves a content part of the combobox widget.
 *
 * For combobox, this function currently does not support named content parts
 * and will always return NULL with an error message.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (combobox widget), unused in this function.
 * @param content The name of the content part to retrieve, unused in this function.
 * @return Always NULL for combobox, as it does not expose named content parts this way.
 */
static Evas_Object *
external_combobox_content_get(void              *data    EINA_UNUSED,
                              const Evas_Object *obj     EINA_UNUSED,
                              const char        *content EINA_UNUSED)
{
   ERR("No content for combobox");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Combobox.
 *
 * This function should be called to release the resources
 * allocated by external_combobox_params_parse().
 *
 * @param params A pointer to the Elm_Params_Combobox structure to free.
 */
static void
external_combobox_params_free(void *params)
{
   Elm_Params_Combobox *const mem = params;

   if (mem->guide) eina_stringshare_del(mem->guide);
   free(mem);
}

/**
 * @brief Defines the external parameters supported by the combobox widget.
 *
 * This array lists the parameters that can be set or retrieved for the combobox
 * via the Edje external interface. It includes common parameters and a specific
 * "guide" string parameter.
 */
static Edje_External_Param_Info external_combobox_params[] = {
     DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "disabled", "visible", etc. */
     EDJE_EXTERNAL_PARAM_INFO_STRING("guide"), /**< Parameter for the guide text. Example: "guide: \"Select an item\";" in EDC. */
     EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(combobox, "combobox");
DEFINE_EXTERNAL_TYPE_SIMPLE(combobox, "Combobox");
