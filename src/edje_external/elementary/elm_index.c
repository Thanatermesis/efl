#include "private.h"

/**
 * @brief Structure for holding Elm_Index parameters.
 *
 * This structure extends Elm_Params with specific properties for an index widget,
 * primarily to manage its 'active' state, which relates to autohide behavior.
 */
typedef struct _Elm_Params_Index
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   Eina_Bool active:1; /**< Boolean flag indicating if autohide is disabled (TRUE) or enabled (FALSE). */
   Eina_Bool active_exists:1; /**< Boolean flag indicating if the 'active' parameter was provided. */

} Elm_Params_Index;

/**
 * @brief Sets the state of an external index widget.
 *
 * This function is called to apply parameters to an Evas_Object representing
 * an index widget. It primarily handles the 'active' state, which controls
 * whether the index autohide feature is disabled.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (index widget) to modify.
 * @param from_params The previous set of parameters (can be NULL).
 * @param to_params The new set of parameters to apply (can be NULL).
 * @param pos The transition position (0.0 to 1.0), unused in this function.
 */
static void
external_index_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                         const void *from_params, const void *to_params,
                         float pos EINA_UNUSED)
{
   const Elm_Params_Index *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->active_exists)
     elm_index_autohide_disabled_set(obj, p->active);
}

/**
 * @brief Sets a specific external parameter for an index widget.
 *
 * This function is called by Edje to set a parameter on the index widget.
 * It currently supports the "active" boolean parameter, which controls
 * the autohide_disabled state.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (index widget) to modify.
 * @param param The Edje_External_Param to set.
 *              Example for 'active':
 *              param->name = "active"
 *              param->type = EDJE_EXTERNAL_PARAM_TYPE_BOOL
 *              param->i = 1 (to disable autohide) or 0 (to enable autohide)
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_index_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                         const Edje_External_Param *param)
{
   if (!strcmp(param->name, "active"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_index_autohide_disabled_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific external parameter from an index widget.
 *
 * This function is called by Edje to retrieve a parameter's value from the
 * index widget. It currently supports the "active" boolean parameter,
 * which reflects the autohide_disabled state.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (index widget) to query.
 * @param param An Edje_External_Param structure to fill with the parameter's value.
 *              Example for 'active':
 *              param->name = "active"
 *              param->type = EDJE_EXTERNAL_PARAM_TYPE_BOOL
 *              (param->i will be set to 1 if autohide is disabled, 0 otherwise)
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_index_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                         Edje_External_Param *param)
{
   if (!strcmp(param->name, "active"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_index_autohide_disabled_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters and stores them.
 *
 * This function allocates an Elm_Params_Index structure and populates it
 * based on the provided list of parameters. It specifically looks for the
 * "active" parameter.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object, unused in this function.
 * @param params A list (Eina_List) of Edje_External_Param structures.
 *               Example of params list structure:
 *               Eina_List* containing Edje_External_Param* elements.
 *               For an "active" parameter:
 *               element->name = "active"
 *               element->type = EDJE_EXTERNAL_PARAM_TYPE_BOOL
 *               element->i = 0 or 1
 * @return A pointer to the newly allocated and populated Elm_Params_Index structure,
 *         or NULL on failure.
 */
static void *
external_index_params_parse(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                            const Eina_List *params)
{
   Elm_Params_Index *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Index));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "active"))
          {
             mem->active = !!param->i;
             mem->active_exists = EINA_TRUE;
          }
     }

   return mem;
}

/**
 * @brief Retrieves content from an external index widget.
 *
 * This function is intended to get a named content part from the widget.
 * However, index widgets currently do not support named content parts.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (index widget), unused.
 * @param content The name of the content part to retrieve, unused.
 * @return Always NULL, as index widgets do not provide content this way.
 */
static Evas_Object *external_index_content_get(void *data EINA_UNUSED,
                                               const Evas_Object *obj EINA_UNUSED,
                                               const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Index.
 *
 * This function is called to release the Elm_Params_Index structure
 * previously allocated by external_index_params_parse.
 *
 * @param params A pointer to the Elm_Params_Index structure to free.
 */
static void
external_index_params_free(void *params)
{
   Elm_Params_Index *mem = params;
   free(mem);
}

static Edje_External_Param_Info external_index_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
    EDJE_EXTERNAL_PARAM_INFO_BOOL("active"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(index, "index");
DEFINE_EXTERNAL_TYPE_SIMPLE(index, "Index");
