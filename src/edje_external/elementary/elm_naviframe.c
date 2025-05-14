#include "private.h"

/**
 * @brief Structure to hold parameters for the Naviframe widget.
 *
 * This structure is used to parse and store parameters from an Edje external
 * definition for a Naviframe widget.
 */
typedef struct _Elm_Params_Naviframe
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   Eina_Bool preserve_on_pop:1; /**< If EINA_TRUE, content will be preserved when popped. */
   Eina_Bool preserve_on_pop_exists:1; /**< Flag indicating if preserve_on_pop was set. */
   Eina_Bool prev_btn_auto_push:1; /**< If EINA_TRUE, the previous button will be automatically pushed. */
   Eina_Bool prev_btn_auto_push_exists:1; /**< Flag indicating if prev_btn_auto_push was set. */
} Elm_Params_Naviframe;

/**
 * @brief Sets the state of the Naviframe widget based on external parameters.
 *
 * This function is called by Edje to apply state changes to the Naviframe
 * widget. It uses either the `to_params` or `from_params` to update
 * widget properties like content preservation on pop and previous button
 * auto push behavior.
 *
 * @param data Unused user data.
 * @param obj The Naviframe Evas_Object to modify.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The new state parameters (can be NULL).
 * @param pos Unused position value.
 */
static void
external_naviframe_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                             const void *from_params, const void *to_params,
                             float pos EINA_UNUSED)
{
   const Elm_Params_Naviframe *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->preserve_on_pop_exists)
     elm_naviframe_content_preserve_on_pop_set(obj, p->preserve_on_pop);
   if (p->prev_btn_auto_push_exists)
     elm_naviframe_prev_btn_auto_pushed_set(obj, p->prev_btn_auto_push);
}

/**
 * @brief Sets a specific external parameter for the Naviframe widget.
 *
 * This function is called by Edje to set individual parameters on the
 * Naviframe widget. It handles "preserve on pop" and "prev btn auto push"
 * boolean parameters.
 *
 * @param data Unused user data.
 * @param obj The Naviframe Evas_Object to modify.
 * @param param The Edje_External_Param to set.
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_naviframe_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                             const Edje_External_Param *param)
{
   if (!strcmp(param->name, "preserve on pop"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_naviframe_content_preserve_on_pop_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "prev btn auto push"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_naviframe_prev_btn_auto_pushed_set(obj, param->i);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific external parameter from the Naviframe widget.
 *
 * This function is called by Edje to retrieve the value of individual
 * parameters from the Naviframe widget. It handles "preserve on pop" and
 * "prev btn auto push" boolean parameters.
 *
 * @param data Unused user data.
 * @param obj The Naviframe Evas_Object to query.
 * @param param The Edje_External_Param to fill with the retrieved value.
 *              The `name` field indicates which parameter to get.
 *              The `type` field indicates the expected type.
 *              The value will be stored in the appropriate field (e.g., `i` for bool).
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_naviframe_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                             Edje_External_Param *param)
{
   if (!strcmp(param->name, "preserve on pop"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_naviframe_content_preserve_on_pop_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "prev btn auto push"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_naviframe_prev_btn_auto_pushed_get(obj);
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
 * This function is called by Edje to convert a list of raw external
 * parameters into a structured format (`Elm_Params_Naviframe`) for easier use.
 * It allocates memory for the structure and populates it based on the
 * provided parameter list.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param params A list of Edje_External_Param objects to parse.
 *               Example of `params` structure:
 *               Eina_List containing Edje_External_Param elements.
 *               Each Edje_External_Param might look like:
 *               - { .name = "preserve on pop", .type = EDJE_EXTERNAL_PARAM_TYPE_BOOL, .i = 1 }
 *               - { .name = "prev btn auto push", .type = EDJE_EXTERNAL_PARAM_TYPE_BOOL, .i = 0 }
 * @return A pointer to the newly allocated Elm_Params_Naviframe structure,
 *         or NULL on failure. The caller is responsible for freeing this memory
 *         using `external_naviframe_params_free`.
 */
static void *
external_naviframe_params_parse(void *data EINA_UNUSED,
                                Evas_Object *obj EINA_UNUSED,
                                const Eina_List *params)
{
   Elm_Params_Naviframe *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = ELM_NEW(Elm_Params_Naviframe);
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "preserve on pop"))
          {
             mem->preserve_on_pop = !!param->i;
             mem->preserve_on_pop_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "prev btn auto push"))
          {
             mem->prev_btn_auto_push = !!param->i;
             mem->prev_btn_auto_push_exists = EINA_TRUE;
          }
     }

   return mem;
}

/**
 * @brief Retrieves content from the Naviframe widget. (Currently not implemented)
 *
 * This function is intended to allow Edje to get content parts from the
 * Naviframe. However, it currently always returns NULL and logs an error,
 * indicating that this functionality is not supported for Naviframe via
 * external_content_get.
 *
 * @param data Unused user data.
 * @param obj Unused Naviframe Evas_Object.
 * @param content Unused name of the content part to get.
 * @return Always NULL.
 */
static Evas_Object *external_naviframe_content_get(void *data EINA_UNUSED,
      const Evas_Object *obj EINA_UNUSED, const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Naviframe parameters.
 *
 * This function is called by Edje to release the memory previously
 * allocated by `external_naviframe_params_parse`.
 *
 * @param params A pointer to the Elm_Params_Naviframe structure to free.
 */
static void
external_naviframe_params_free(void *params)
{
   Elm_Params_Naviframe *mem = params;
   free(mem);
}

/**
 * @brief Defines the external parameters available for the Naviframe widget.
 *
 * This array provides metadata about the parameters that can be set on a
 * Naviframe widget from an Edje file. It includes common parameters and
 * Naviframe-specific ones like "preserve on pop" and "prev btn auto push".
 */
static Edje_External_Param_Info external_naviframe_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_BOOL("preserve on pop"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("prev btn auto push"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(naviframe, "naviframe");
DEFINE_EXTERNAL_TYPE_SIMPLE(naviframe, "Naviframe");
