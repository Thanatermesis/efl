#include "private.h"

/**
 * @brief Structure defining the parameters for an Elm_Check widget.
 *
 * This structure holds all the configurable parameters for an Elm_Check widget,
 * including its label, icon, and state. It is used when creating or updating
 * the widget through the external parameter API.
 */
typedef struct _Elm_Params_Check
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   const char *label; /**< The text label to display on the check widget. */
   Evas_Object *icon; /**< An Evas_Object to use as an icon for the check widget. */
   Eina_Bool state:1; /**< The boolean state of the check (EINA_TRUE for checked, EINA_FALSE for unchecked). */
   Eina_Bool state_exists:1; /**< Flag indicating if the state parameter was provided. */
} Elm_Params_Check;

/**
 * @brief Sets the state of an Elm_Check widget based on external parameters.
 *
 * This function is a callback used by the Edje external system to update
 * the check widget's visual state (label, icon, and checked state)
 * during animations or transitions.
 *
 * @param data Unused user data.
 * @param obj The Elm_Check Evas_Object to modify.
 * @param from_params The parameters defining the starting state (can be NULL).
 * @param to_params The parameters defining the target state (can be NULL).
 * @param pos The position in the transition (unused).
 */
static void
external_check_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                         const void *from_params, const void *to_params,
                         float pos EINA_UNUSED)
{
   const Elm_Params_Check *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label)
     elm_object_text_set(obj, p->label);
   if (p->icon)
     elm_object_part_content_set(obj, "icon", p->icon);
   if (p->state_exists)
     elm_check_state_set(obj, p->state);
}

/**
 * @brief Sets a specific parameter for an Elm_Check widget.
 *
 * This function is a callback used by the Edje external system to set
 * individual properties of an Elm_Check widget, such as its label, icon, or state.
 *
 * @param data Unused user data.
 * @param obj The Elm_Check Evas_Object to modify.
 * @param param A pointer to an Edje_External_Param structure containing the
 *              name and value of the parameter to set.
 *              Supported parameters:
 *              - "label" (string): Sets the text of the check.
 *              - "icon" (string): Sets the icon of the check. The string is
 *                                 typically a file path or a standard icon name.
 *              - "state" (bool): Sets the checked state (0 for unchecked, 1 for checked).
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_check_param_set(void *data EINA_UNUSED, Evas_Object *obj,
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
   else if (!strcmp(param->name, "state"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_check_state_set(obj, param->i);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific parameter from an Elm_Check widget.
 *
 * This function is a callback used by the Edje external system to retrieve
 * individual properties of an Elm_Check widget.
 *
 * @param data Unused user data.
 * @param obj The Elm_Check Evas_Object to query.
 * @param param A pointer to an Edje_External_Param structure. The `name` field
 *              should be set to the parameter to retrieve. The `type` field
 *              should also be set to the expected type. The retrieved value
 *              will be stored in the appropriate field (e.g., `s` for string,
 *              `i` for integer/boolean).
 *              Supported parameters:
 *              - "label" (string): Gets the text of the check.
 *              - "icon" (string): Attempting to get the icon is not fully supported
 *                                 and will likely return EINA_FALSE.
 *              - "state" (bool): Gets the checked state.
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_check_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
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
   else if (!strcmp(param->name, "state"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_check_state_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje_External_Param to create an Elm_Params_Check structure.
 *
 * This function is called by the Edje external system to convert a list of
 * parameters (e.g., from an EDC file) into a structured Elm_Params_Check object.
 * This object is then used to initialize or update an Elm_Check widget.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object this parse is associated with (used for icon parsing).
 * @param params A list of Edje_External_Param structures to parse.
 *               Example of `params` list elements:
 *               - Edje_External_Param { .name = "label", .type = EDJE_EXTERNAL_PARAM_TYPE_STRING, .s = "My Checkbox" }
 *               - Edje_External_Param { .name = "icon", .type = EDJE_EXTERNAL_PARAM_TYPE_STRING, .s = "path/to/icon.png" }
 *               - Edje_External_Param { .name = "state", .type = EDJE_EXTERNAL_PARAM_TYPE_BOOL, .i = 1 }
 * @return A pointer to a newly allocated Elm_Params_Check structure, or NULL on failure.
 *         The caller is responsible for freeing this structure using external_check_params_free().
 */
static void *
external_check_params_parse(void *data EINA_UNUSED, Evas_Object *obj,
                            const Eina_List *params)
{
   Elm_Params_Check *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Check));
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "state"))
          {
             mem->state = !!param->i;
             mem->state_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "label"))
          mem->label = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Retrieves content from an Elm_Check widget.
 *
 * This function is a callback for the Edje external system. Elm_Check widgets
 * do not support named content parts in the same way other widgets might (e.g., a button's label
 * is text, not a swallable content object). Therefore, this function currently
 * indicates that no content is available.
 *
 * @param data Unused user data.
 * @param obj The Elm_Check Evas_Object (unused).
 * @param content The name of the content part to retrieve (unused).
 * @return Always returns NULL, as Elm_Check does not expose content this way.
 */
static Evas_Object *external_check_content_get(void *data EINA_UNUSED,
                                               const Evas_Object *obj EINA_UNUSED,
                                               const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees an Elm_Params_Check structure.
 *
 * This function is used to release the memory allocated by
 * external_check_params_parse(). It ensures that any dynamically
 * allocated members within the Elm_Params_Check structure (like shared strings)
 * are properly deallocated.
 *
 * @param params A pointer to the Elm_Params_Check structure to free.
 */
static void
external_check_params_free(void *params)
{
   Elm_Params_Check *mem = params;
   if (mem->label)
     eina_stringshare_del(mem->label);
   free(params);
}

/**
 * @brief Array defining the external parameters for Elm_Check widgets.
 *
 * This array provides metadata about the parameters that can be used to
 * configure an Elm_Check widget from an Edje EDC file or through the
 * Edje external API. It includes common parameters (like "disabled")
 * and check-specific parameters like "label", "icon", and "state".
 *
 * The structure of elements is defined by Edje_External_Param_Info.
 * For example:
 * - {"label", EDJE_EXTERNAL_PARAM_TYPE_STRING, ...}
 * - {"icon", EDJE_EXTERNAL_PARAM_TYPE_STRING, ...}
 * - {"state", EDJE_EXTERNAL_PARAM_TYPE_BOOL, .spec.bool_str = {"unchecked", "checked"}, ...}
 */
static Edje_External_Param_Info external_check_params[] = {
     DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "disabled", "visible", etc. */
     EDJE_EXTERNAL_PARAM_INFO_STRING("label"), /**< Parameter for the check's text label. */
     EDJE_EXTERNAL_PARAM_INFO_STRING("icon"), /**< Parameter for the check's icon. */
     EDJE_EXTERNAL_PARAM_INFO_BOOL_FULL("state", 0, "unchecked", "checked"), /**< Parameter for the check's boolean state (0 or 1), with string representations. */
     EDJE_EXTERNAL_PARAM_INFO_SENTINEL /**< Marks the end of the parameter list. */
};

DEFINE_EXTERNAL_ICON_ADD(check, "check");
DEFINE_EXTERNAL_TYPE_SIMPLE(check, "Check");
