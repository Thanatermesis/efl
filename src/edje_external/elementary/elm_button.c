#include "private.h"

/**
 * @brief Structure defining the parameters for an Elm_Button widget.
 *
 * This structure holds all configurable parameters for a button,
 * including its label, icon, and autorepeat behavior.
 */
typedef struct _Elm_Params_Button
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   const char *label; /**< The text label to display on the button. */
   Evas_Object *icon; /**< The icon object to display on the button. */
   double autorepeat_initial; /**< The initial timeout before autorepeat starts (in seconds). */
   double autorepeat_gap; /**< The gap timeout between autorepeat events (in seconds). */
   Eina_Bool autorepeat:1; /**< Flag indicating if autorepeat is enabled. */
   Eina_Bool autorepeat_exists:1; /**< Flag indicating if the autorepeat parameter was provided. */
   Eina_Bool autorepeat_gap_exists:1; /**< Flag indicating if the autorepeat_gap parameter was provided. */
   Eina_Bool autorepeat_initial_exists:1; /**< Flag indicating if the autorepeat_initial parameter was provided. */
} Elm_Params_Button;

/**
 * @brief Sets the state of an external button widget.
 *
 * This function is called to apply a set of parameters (either `to_params`
 * or `from_params`) to the given Evas_Object, which is expected to be an
 * Elm_Button. It updates the button's label, icon, and autorepeat properties.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (Elm_Button) to modify.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The new state parameters to apply (can be NULL).
 * @param pos Unused position value for animations/transitions.
 */
static void
external_button_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                          const void *from_params, const void *to_params,
                          float pos EINA_UNUSED)
{
   const Elm_Params_Button *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label)
     elm_object_text_set(obj, p->label);
   if (p->icon)
     elm_object_part_content_set(obj, "icon", p->icon);
   if (p->autorepeat_gap_exists)
     elm_button_autorepeat_gap_timeout_set(obj, p->autorepeat_gap);
   if (p->autorepeat_initial_exists)
     elm_button_autorepeat_initial_timeout_set(obj, p->autorepeat_initial);
   if (p->autorepeat_exists)
     elm_button_autorepeat_set(obj, p->autorepeat);
}

/**
 * @brief Sets a single external parameter for a button widget.
 *
 * This function is called by Edje to set a specific parameter on the
 * Elm_Button widget. It handles parameters like "label", "icon",
 * "autorepeat_initial", "autorepeat_gap", and "autorepeat".
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (Elm_Button) to modify.
 * @param param The Edje_External_Param to apply.
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_button_param_set(void *data EINA_UNUSED, Evas_Object *obj,
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
   else if (!strcmp(param->name, "autorepeat_initial"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             elm_button_autorepeat_initial_timeout_set(obj, param->d);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "autorepeat_gap"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             elm_button_autorepeat_gap_timeout_set(obj, param->d);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "autorepeat"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_button_autorepeat_set(obj, param->i);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a single external parameter from a button widget.
 *
 * This function is called by Edje to retrieve the value of a specific
 * parameter from the Elm_Button widget. It handles parameters like "label",
 * "autorepeat_initial", "autorepeat_gap", and "autorepeat".
 * Note: Retrieving the "icon" parameter is not supported directly.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (Elm_Button) to query.
 * @param param The Edje_External_Param to fill with the retrieved value.
 *              The `name` field indicates which parameter to get, and `type`
 *              indicates the expected type. The value is stored in the
 *              appropriate union member (e.g., `param->s`, `param->d`, `param->i`).
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_button_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
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
   else if (!strcmp(param->name, "autorepeat_initial"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             param->d = elm_button_autorepeat_initial_timeout_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "autorepeat_gap"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             param->d = elm_button_autorepeat_gap_timeout_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "autorepeat"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_button_autorepeat_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje_External_Param objects and creates an Elm_Params_Button structure.
 *
 * This function iterates through a list of external parameters (typically from an
 * Edje theme file) and populates an Elm_Params_Button structure. This structure
 * can then be used to set the initial state of an Elm_Button.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (Elm_Button) this parse is associated with (used for icon parsing).
 * @param params A list of Edje_External_Param objects to parse.
 *               Example of `params` structure:
 *               Eina_List containing Edje_External_Param elements.
 *               Each Edje_External_Param might look like:
 *               - { .name = "label", .type = EDJE_EXTERNAL_PARAM_TYPE_STRING, .s = "Click Me" }
 *               - { .name = "icon", .type = EDJE_EXTERNAL_PARAM_TYPE_STRING, .s = "home" }
 *               - { .name = "autorepeat", .type = EDJE_EXTERNAL_PARAM_TYPE_BOOL, .i = 1 }
 *               - { .name = "autorepeat_initial", .type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, .d = 0.5 }
 *               - { .name = "autorepeat_gap", .type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, .d = 0.2 }
 * @return A pointer to a newly allocated Elm_Params_Button structure, or NULL on failure.
 *         The caller is responsible for freeing this memory using external_button_params_free().
 */
static void *
external_button_params_parse(void *data EINA_UNUSED, Evas_Object *obj,
                             const Eina_List *params)
{
   Elm_Params_Button *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = ELM_NEW(Elm_Params_Button);
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "autorepeat_initial"))
          {
             mem->autorepeat_initial = param->d;
             mem->autorepeat_initial_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "autorepeat_gap"))
          {
             mem->autorepeat_gap = param->d;
             mem->autorepeat_gap_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "autorepeat"))
          {
             mem->autorepeat = !!param->i;
             mem->autorepeat_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "label"))
          mem->label = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Retrieves content from an external button widget.
 *
 * This function is intended to get a named content part from the button.
 * However, for Elm_Button, it currently does not support retrieving content
 * this way and will always log an error and return NULL.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param content Unused name of the content part to retrieve.
 * @return Always NULL for Elm_Button.
 */
static Evas_Object *external_button_content_get(void *data EINA_UNUSED,
                                                const Evas_Object *obj EINA_UNUSED,
                                                const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Button.
 *
 * This function releases the resources held by an Elm_Params_Button
 * structure, including any stringshared label.
 *
 * @param params A pointer to the Elm_Params_Button structure to free.
 */
static void
external_button_params_free(void *params)
{
   Elm_Params_Button *mem = params;
   if (mem->label)
     eina_stringshare_del(mem->label);
   free(params);
}

/**
 * @brief Array defining the external parameters supported by Elm_Button.
 *
 * This array provides metadata about the parameters that can be set on an
 * Elm_Button from an Edje theme or externally. It includes the parameter
 * name and its type.
 */
static Edje_External_Param_Info external_button_params[] =
{
   DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "swallow" */
   EDJE_EXTERNAL_PARAM_INFO_STRING("label"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("icon"),
   EDJE_EXTERNAL_PARAM_INFO_DOUBLE("autorepeat_initial"),
   EDJE_EXTERNAL_PARAM_INFO_DOUBLE("autorepeat_gap"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("autorepeat"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(button, "button");
DEFINE_EXTERNAL_TYPE_SIMPLE(button, "Button");
