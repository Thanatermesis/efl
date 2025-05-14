#include "private.h"

/**
 * @brief Structure to hold parameters for the multibuttonentry widget.
 *
 * This structure is used to store the label and guide text
 * for an Elm_Multibuttonentry widget when it's created or modified
 * through external parameters (e.g., from an Edje theme).
 */
typedef struct _Elm_Params_Multibuttonentry
{
   const char *label; /**< The main text label of the multibuttonentry. */
   const char *guide_text; /**< The guide text displayed when the entry is empty. */
} Elm_Params_Multibuttonentry;

/**
 * @brief Sets the state of the multibuttonentry widget.
 *
 * This function is called by Edje to apply state parameters (label, guide text)
 * to the multibuttonentry widget. It can transition from one state to another.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (multibuttonentry widget) to modify.
 * @param from_params The parameters of the previous state (can be NULL).
 * @param to_params The parameters of the new state (can be NULL).
 * @param pos The position in the transition (unused).
 */
static void
external_multibuttonentry_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                                    const void *from_params,
                                    const void *to_params,
                                    float pos EINA_UNUSED)
{
   const Elm_Params_Multibuttonentry *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label)
     elm_object_text_set(obj, p->label);
   if (p->guide_text)
     elm_object_part_text_set(obj, "guide", p->guide_text);
}

/**
 * @brief Sets a specific external parameter for the multibuttonentry widget.
 *
 * This function is called by Edje to set individual parameters like "label"
 * or "guide text" on the multibuttonentry widget.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (multibuttonentry widget) to modify.
 * @param param The Edje_External_Param to apply.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter).
 */
static Eina_Bool
external_multibuttonentry_param_set(void *data EINA_UNUSED, Evas_Object *obj,
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
   else if (!strcmp(param->name, "guide text"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_object_part_text_set(obj, "guide", param->s);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific external parameter from the multibuttonentry widget.
 *
 * This function is called by Edje to retrieve the current value of parameters
 * like "label" or "guide text" from the multibuttonentry widget.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (multibuttonentry widget) to query.
 * @param param A pointer to an Edje_External_Param structure to fill with the value.
 *              The `name` field of this struct indicates which parameter to get.
 *              The `type` field indicates the expected type.
 *              The corresponding value field (e.g., `s` for string) will be set.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter).
 */
static Eina_Bool
external_multibuttonentry_param_get(void *data EINA_UNUSED,
                                    const Evas_Object *obj,
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
   else if (!strcmp(param->name, "guide text"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_object_part_text_get(obj, "guide");
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of external parameters and creates a parameter structure.
 *
 * This function is called by Edje to convert a list of Edje_External_Param
 * into a custom Elm_Params_Multibuttonentry structure. This structure is then
 * typically used by external_multibuttonentry_state_set.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param params An Eina_List of Edje_External_Param objects to parse.
 *               Example of params list structure:
 *               [
 *                 { name="label", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="My Label" },
 *                 { name="guide text", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="Enter text here..." }
 *               ]
 * @return A pointer to a newly allocated Elm_Params_Multibuttonentry structure
 *         filled with parsed values, or NULL on failure. The caller is
 *         responsible for freeing this structure using external_multibuttonentry_params_free.
 */
static void *
external_multibuttonentry_params_parse(void *data EINA_UNUSED,
                                       Evas_Object *obj EINA_UNUSED,
                                       const Eina_List *params)
{
   Elm_Params_Multibuttonentry *mem = NULL;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Multibuttonentry));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "label"))
          mem->label = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "guide text"))
          mem->guide_text = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Gets a content part of the multibuttonentry widget.
 *
 * This function is intended to retrieve specific content Evas_Objects
 * from the multibuttonentry, if it supported named content parts.
 * Currently, it's a stub and returns NULL.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object (the multibuttonentry widget).
 * @param content Unused name of the content part to retrieve.
 * @return NULL, as content parts are not supported by this external interface.
 */
static Evas_Object *external_multibuttonentry_content_get(void *data EINA_UNUSED,
                                                          const Evas_Object *obj EINA_UNUSED,
                                                          const char *content EINA_UNUSED)
{
   ERR("so content");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Multibuttonentry.
 *
 * This function is responsible for releasing the resources held by an
 * Elm_Params_Multibuttonentry structure, including any stringshared strings.
 * It also calls external_common_params_free for any common parameters.
 *
 * @param params A pointer to the Elm_Params_Multibuttonentry structure to free.
 */
static void
external_multibuttonentry_params_free(void *params)
{
   Elm_Params_Multibuttonentry *mem = params;

   if (mem->label)
     eina_stringshare_del(mem->label);
   if (mem->guide_text)
     eina_stringshare_del(mem->guide_text);
   external_common_params_free(params);
}

static Edje_External_Param_Info external_multibuttonentry_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("label"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("guide text"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(multibuttonentry, "multibuttonentry");
DEFINE_EXTERNAL_TYPE_SIMPLE(multibuttonentry, "Multibuttonentry");
