#include "private.h"

/**
 * @brief Structure to hold parameters for an Elm_Slider widget.
 *
 * This structure is used to store and apply a set of configuration
 * parameters to an Elm_Slider widget, often during its creation or
 * state transition.
 */
typedef struct _Elm_Params_Slider
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   const char *label; /**< The text label to set on the slider. */
   Evas_Object *icon; /**< An Evas_Object to use as an icon for the slider. */
   const char *indicator; /**< Format string for the indicator label (e.g., "%.2f"). */
   const char *unit; /**< Format string for the unit label (e.g., "%s units"). */
   double min; /**< The minimum value of the slider. */
   double max; /**< The maximum value of the slider. */
   double value; /**< The current value of the slider. */
   Evas_Coord span; /**< The span of the slider on the canvas. */
   Eina_Bool min_exists:1; /**< Flag indicating if 'min' value is set. */
   Eina_Bool max_exists:1; /**< Flag indicating if 'max' value is set. */
   Eina_Bool value_exists:1; /**< Flag indicating if 'value' is set. */
   Eina_Bool inverted:1; /**< Flag indicating if the slider is inverted. */
   Eina_Bool inverted_exists:1; /**< Flag indicating if 'inverted' state is set. */
   Eina_Bool span_exists:1; /**< Flag indicating if 'span' value is set. */
   Eina_Bool horizontal:1; /**< Flag indicating if the slider is horizontal. */
   Eina_Bool horizontal_exists:1; /**< Flag indicating if 'horizontal' state is set. */
} Elm_Params_Slider;

/**
 * @brief Sets the state of an external slider widget.
 *
 * This function applies parameters to an Elm_Slider object, typically
 * during animations or state transitions managed by Edje.
 *
 * @param data Unused.
 * @param obj The Elm_Slider Evas_Object to modify.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The target state parameters (can be NULL).
 * @param pos Unused.
 */
static void
external_slider_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                          const void *from_params, const void *to_params,
                          float pos EINA_UNUSED)
{
   const Elm_Params_Slider *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label)
     elm_object_text_set(obj, p->label);
   if (p->icon)
     elm_object_part_content_set(obj, "icon", p->icon);
   if (p->span_exists)
     elm_slider_span_size_set(obj, p->span);
   if ((p->min_exists) && (p->max_exists))
     elm_slider_min_max_set(obj, p->min, p->max);
   else if ((p->min_exists) || (p->max_exists))
     {
        double min, max;
        elm_slider_min_max_get(obj, &min, &max);
        if (p->min_exists)
          elm_slider_min_max_set(obj, p->min, max);
        else
          elm_slider_min_max_set(obj, min, p->max);
     }
   if (p->value_exists)
     elm_slider_value_set(obj, p->value);
   if (p->inverted_exists)
     elm_slider_inverted_set(obj, p->inverted);
   if (p->horizontal_exists)
     elm_slider_horizontal_set(obj, p->horizontal);
   if (p->indicator)
     elm_slider_indicator_format_set(obj, p->indicator);
   if (p->unit)
     elm_slider_unit_format_set(obj, p->unit);
}

/**
 * @brief Sets a specific parameter for an external slider widget.
 *
 * This function is called by Edje to set individual properties of an
 * Elm_Slider widget based on external parameters defined in an Edje theme.
 *
 * @param data Unused.
 * @param obj The Elm_Slider Evas_Object to modify.
 * @param param The Edje_External_Param describing the property to set.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_slider_param_set(void *data EINA_UNUSED, Evas_Object *obj,
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
   else if (!strcmp(param->name, "min"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             double min, max;
             elm_slider_min_max_get(obj, &min, &max);
             elm_slider_min_max_set(obj, param->d, max);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "max"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             double min, max;
             elm_slider_min_max_get(obj, &min, &max);
             elm_slider_min_max_set(obj, min, param->d);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "value"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             elm_slider_value_set(obj, param->d);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_slider_horizontal_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "inverted"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_slider_inverted_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "span"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             elm_slider_span_size_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "unit format"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_slider_unit_format_set(obj, param->s);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "indicator format"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_slider_indicator_format_set(obj, param->s);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific parameter from an external slider widget.
 *
 * This function is called by Edje to retrieve individual properties of an
 * Elm_Slider widget for use in external parameter definitions.
 *
 * @param data Unused.
 * @param obj The Elm_Slider Evas_Object to query.
 * @param param The Edje_External_Param to fill with the property value.
 *              The `name` field indicates which parameter to get.
 *              The `type` field indicates the expected type.
 *              The corresponding value field (e.g., `s`, `i`, `d`) will be set.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_slider_param_get(void *data EINA_UNUSED, const Evas_Object *obj, Edje_External_Param *param)
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
   else if (!strcmp(param->name, "min"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             double min, max;
             elm_slider_min_max_get(obj, &min, &max);
             param->d = min;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "max"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             double min, max;
             elm_slider_min_max_get(obj, &min, &max);
             param->d = max;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "value"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             param->d = elm_slider_value_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_slider_horizontal_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "inverted"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_slider_inverted_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "span"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             param->i = elm_slider_span_size_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "unit format"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_slider_unit_format_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "indicator format"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_slider_indicator_format_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters and creates an Elm_Params_Slider structure.
 *
 * This function converts a list of Edje_External_Param objects into a
 * more usable Elm_Params_Slider structure, allocating memory for it.
 * The caller is responsible for freeing this memory using external_slider_params_free().
 *
 * @param data Unused.
 * @param obj Unused.
 * @param params A list of Edje_External_Param objects to parse.
 *               Example of params list structure:
 *               - param1: Edje_External_Param { name="label", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="My Slider" }
 *               - param2: Edje_External_Param { name="min", type=EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, d=0.0 }
 *               - param3: Edje_External_Param { name="max", type=EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, d=100.0 }
 * @return A pointer to a newly allocated Elm_Params_Slider structure, or NULL on failure.
 */
static void *
external_slider_params_parse(void *data EINA_UNUSED,
                             Evas_Object *obj EINA_UNUSED,
                             const Eina_List *params)
{
   Elm_Params_Slider *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Slider));
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
        else if (!strcmp(param->name, "min"))
          {
             mem->min = param->d;
             mem->min_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "max"))
          {
             mem->max = param->d;
             mem->max_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "value"))
          {
             mem->value = param->d;
             mem->value_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "inverted"))
          {
             mem->inverted = param->i;
             mem->inverted_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "horizontal"))
          {
             mem->horizontal = param->i;
             mem->horizontal_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "unit format"))
          mem->unit = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "indicator format"))
          mem->indicator = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "label"))
          mem->label = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Retrieves content from an external slider widget.
 *
 * Currently, sliders do not support named content parts beyond "icon"
 * (handled by elm_object_part_content_set/get), so this function
 * always returns NULL and logs an error.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param content Unused.
 * @return Always NULL.
 */
static Evas_Object *external_slider_content_get(void *data EINA_UNUSED,
                                                const Evas_Object *obj EINA_UNUSED,
                                                const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Slider.
 *
 * This function releases the resources held by an Elm_Params_Slider
 * structure, including any shared strings.
 *
 * @param params A pointer to the Elm_Params_Slider structure to free.
 */
static void
external_slider_params_free(void *params)
{
   Elm_Params_Slider *mem = params;

   if (mem->unit)
     eina_stringshare_del(mem->unit);
   if (mem->indicator)
     eina_stringshare_del(mem->indicator);
   if (mem->label)
     eina_stringshare_del(mem->label);
   free(params);
}

/**
 * @brief Array defining the external parameters available for Elm_Slider.
 *
 * This array is used by Edje to understand what parameters can be
 * set or get from an Elm_Slider widget. Each entry defines the name,
 * type, and optionally default values for a parameter.
 */
static Edje_External_Param_Info external_slider_params[] = {
     DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "disabled", "visible". */
     EDJE_EXTERNAL_PARAM_INFO_STRING("label"), /**< The text label of the slider. */
     EDJE_EXTERNAL_PARAM_INFO_STRING("icon"), /**< The icon for the slider (filename or group name). */
     EDJE_EXTERNAL_PARAM_INFO_DOUBLE("min"), /**< The minimum value of the slider. */
     EDJE_EXTERNAL_PARAM_INFO_DOUBLE_DEFAULT("max", 10.0), /**< The maximum value of the slider, defaults to 10.0. */
     EDJE_EXTERNAL_PARAM_INFO_DOUBLE("value"), /**< The current value of the slider. */
     EDJE_EXTERNAL_PARAM_INFO_BOOL("horizontal"), /**< Whether the slider is horizontal (true) or vertical (false). */
     EDJE_EXTERNAL_PARAM_INFO_BOOL("inverted"), /**< Whether the slider's direction is inverted. */
     EDJE_EXTERNAL_PARAM_INFO_INT("span"), /**< The pixel span of the slider on the canvas. */
     EDJE_EXTERNAL_PARAM_INFO_STRING_DEFAULT("unit format", "%1.2f"), /**< Format string for the unit label (e.g., "%.2f units"). */
     EDJE_EXTERNAL_PARAM_INFO_STRING_DEFAULT("indicator format", "%1.2f"), /**< Format string for the indicator label (e.g., "Value: %.2f"). */
     EDJE_EXTERNAL_PARAM_INFO_SENTINEL /**< Marks the end of the parameter list. */
};

DEFINE_EXTERNAL_ICON_ADD(slider, "slider");
DEFINE_EXTERNAL_TYPE_SIMPLE(slider, "Slider");
