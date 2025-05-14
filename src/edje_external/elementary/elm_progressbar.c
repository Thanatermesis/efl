#include "private.h"

/**
 * @brief Structure to hold parameters for an Elm_Progressbar widget.
 *
 * This structure is used to store and apply a set of configuration
 * parameters to a progressbar widget, often during its creation or
 * state transition.
 */
typedef struct _Elm_Params_Progressbar
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   const char *label; /**< The text label to display on the progressbar. */
   Evas_Object *icon; /**< An icon object to display on the progressbar. */
   const char *unit; /**< The unit format string (e.g., "%.2f %%"). */
   double value; /**< The current progress value (typically between 0.0 and 1.0). */
   Evas_Coord span; /**< The span of the progressbar, affecting its visual length. */
   Eina_Bool value_exists:1; /**< Flag indicating if 'value' is set. */
   Eina_Bool span_exists:1; /**< Flag indicating if 'span' is set. */
   Eina_Bool inverted:1; /**< Flag indicating if the progressbar is inverted. */
   Eina_Bool inverted_exists:1; /**< Flag indicating if 'inverted' is set. */
   Eina_Bool horizontal:1; /**< Flag indicating if the progressbar is horizontal. */
   Eina_Bool horizontal_exists:1; /**< Flag indicating if 'horizontal' is set. */
   Eina_Bool pulse:1; /**< Flag indicating if pulse mode is enabled. */
   Eina_Bool pulse_exists:1; /**< Flag indicating if 'pulse' is set. */
   Eina_Bool pulsing:1; /**< Flag indicating if the progressbar is currently pulsing. */
   Eina_Bool pulsing_exists:1; /**< Flag indicating if 'pulsing' is set. */
} Elm_Params_Progressbar;

/**
 * @brief Sets the state of an external progressbar widget.
 *
 * This function applies parameters to the progressbar object, typically
 * during animations or state transitions. It uses either `to_params` or
 * `from_params` to determine the state to apply.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (progressbar) to modify.
 * @param from_params The source state parameters (used if to_params is NULL).
 * @param to_params The target state parameters.
 * @param pos Unused position value for transitions.
 */
static void
external_progressbar_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                               const void *from_params, const void *to_params,
                               float pos EINA_UNUSED)
{
   const Elm_Params_Progressbar *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label)
     elm_object_text_set(obj, p->label);
   if (p->icon)
     elm_object_part_content_set(obj, "icon", p->icon);
   if (p->span_exists)
     elm_progressbar_span_size_set(obj, p->span);
   if (p->value_exists)
     elm_progressbar_value_set(obj, p->value);
   if (p->inverted_exists)
     elm_progressbar_inverted_set(obj, p->inverted);
   if (p->horizontal_exists)
     elm_progressbar_horizontal_set(obj, p->horizontal);
   if (p->unit)
     elm_progressbar_unit_format_set(obj, p->unit);
   if (p->pulse_exists)
     elm_progressbar_pulse_set(obj, p->pulse);
   if  (p->pulsing_exists)
     elm_progressbar_pulse(obj, p->pulsing);
}

/**
 * @brief Sets a specific parameter for an external progressbar widget.
 *
 * This function is called by Edje to set individual parameters of the
 * progressbar widget based on external definitions.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (progressbar) to modify.
 * @param param The Edje_External_Param describing the parameter to set.
 *              Example: param->name = "value", param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, param->d = 0.5
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter or wrong type).
 */
static Eina_Bool
external_progressbar_param_set(void *data EINA_UNUSED, Evas_Object *obj,
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
   else if (!strcmp(param->name, "value"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             elm_progressbar_value_set(obj, param->d);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_progressbar_horizontal_set(obj, param->i);
             return EINA_TRUE;
          }
     }
  else if (!strcmp(param->name, "pulse"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_progressbar_pulse_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "pulsing"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_progressbar_pulse(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "inverted"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_progressbar_inverted_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "span"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             elm_progressbar_span_size_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "unit format"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_progressbar_unit_format_set(obj, param->s);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific parameter from an external progressbar widget.
 *
 * This function is called by Edje to retrieve the current value of
 * individual parameters of the progressbar widget.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (progressbar) to query.
 * @param param The Edje_External_Param structure to fill with the parameter's value.
 *              The `param->name` field indicates which parameter to get.
 *              Example: param->name = "value", param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE
 *                       On return, param->d will contain the current value.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter or wrong type).
 */
static Eina_Bool
external_progressbar_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
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
   else if (!strcmp(param->name, "value"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             param->d = elm_progressbar_value_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_progressbar_horizontal_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "pulse"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_progressbar_pulse_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "pulsing"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_progressbar_is_pulsing_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "inverted"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_progressbar_inverted_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "span"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             param->i = elm_progressbar_span_size_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "unit format"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_progressbar_unit_format_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters and creates an Elm_Params_Progressbar structure.
 *
 * This function converts a list of generic Edje parameters into a
 * progressbar-specific parameter structure. This structure can then be used
 * to configure a progressbar widget.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param params A list of Edje_External_Param structures to parse.
 *               Example: params might contain Edje_External_Param elements like:
 *               - { name="label", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="Loading..." }
 *               - { name="value", type=EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, d=0.75 }
 *               - { name="horizontal", type=EDJE_EXTERNAL_PARAM_TYPE_BOOL, i=1 }
 * @return A pointer to a newly allocated Elm_Params_Progressbar structure, or NULL on failure.
 *         The caller is responsible for freeing this structure using external_progressbar_params_free().
 */
static void *
external_progressbar_params_parse(void *data EINA_UNUSED,
                                  Evas_Object *obj EINA_UNUSED,
                                  const Eina_List *params)
{
   Elm_Params_Progressbar *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Progressbar));
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "span"))
          {
             mem->span = param->i;
             mem->span_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "value"))
          {
             mem->value = param->d;
             mem->value_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "inverted"))
          {
             mem->inverted = !!param->i;
             mem->inverted_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "horizontal"))
          {
             mem->horizontal = !!param->i;
             mem->horizontal_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "pulse"))
          {
             mem->pulse = !!param->i;
             mem->pulse_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "pulsing"))
          {
             mem->pulsing = !!param->i;
             mem->pulsing_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "unit format"))
          mem->unit = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "label"))
          mem->label = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Retrieves content from an external progressbar widget.
 *
 * Currently, progressbars do not support named content parts beyond "icon"
 * (handled by elm_object_part_content_set/get), so this function
 * indicates that no other content is available.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param content Unused content part name.
 * @return Always NULL, as progressbars don't have other named content parts.
 */
static Evas_Object *external_progressbar_content_get(void *data EINA_UNUSED,
                                                     const Evas_Object *obj EINA_UNUSED,
                                                     const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees an Elm_Params_Progressbar structure.
 *
 * This function releases the memory allocated for an Elm_Params_Progressbar
 * structure, including any stringshared members.
 *
 * @param params A pointer to the Elm_Params_Progressbar structure to free.
 */
static void
external_progressbar_params_free(void *params)
{
   Elm_Params_Progressbar *mem = params;

   if (mem->unit)
     eina_stringshare_del(mem->unit);
   if (mem->label)
     eina_stringshare_del(mem->label);
   free(params);
}

/**
 * @brief Array defining the external parameters for a progressbar widget.
 *
 * This array provides metadata about the parameters that can be set or
 * retrieved for a progressbar widget via Edje's external interface.
 * Each entry defines the parameter's name and type.
 *
 * Example structure of elements:
 * - { "label", EDJE_EXTERNAL_PARAM_TYPE_STRING, ... }
 * - { "value", EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, ... }
 * - { "horizontal", EDJE_EXTERNAL_PARAM_TYPE_BOOL, ... }
 */
static Edje_External_Param_Info external_progressbar_params[] = {
     DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "disabled". */
     EDJE_EXTERNAL_PARAM_INFO_STRING("label"), /**< Parameter for the progressbar text label. */
     EDJE_EXTERNAL_PARAM_INFO_STRING("icon"), /**< Parameter for the progressbar icon. */
     EDJE_EXTERNAL_PARAM_INFO_DOUBLE("value"), /**< Parameter for the progress value. */
     EDJE_EXTERNAL_PARAM_INFO_BOOL("horizontal"), /**< Parameter for horizontal orientation. */
     EDJE_EXTERNAL_PARAM_INFO_BOOL("pulse"), /**< Parameter to enable pulse mode. */
     EDJE_EXTERNAL_PARAM_INFO_BOOL("pulsing"), /**< Parameter to control pulsing state. */
     EDJE_EXTERNAL_PARAM_INFO_BOOL("inverted"), /**< Parameter for inverted display. */
     EDJE_EXTERNAL_PARAM_INFO_INT("span"), /**< Parameter for the progressbar span size. */
     EDJE_EXTERNAL_PARAM_INFO_STRING_DEFAULT("unit format", "%1.2f"), /**< Parameter for the unit format string, defaults to "%.2f". */
     EDJE_EXTERNAL_PARAM_INFO_SENTINEL /**< Marks the end of the parameter list. */
};

DEFINE_EXTERNAL_ICON_ADD(progressbar, "progressbar");
DEFINE_EXTERNAL_TYPE_SIMPLE(progressbar, "Progressbar");
