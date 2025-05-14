#include "private.h"

/**
 * @brief Structure to hold the parameters for an Elm_Spinner widget.
 * This structure is used to pass parameters when creating or updating
 * a spinner widget externally, for example, from an Edje theme.
 */
typedef struct _Elm_Params_Spinner
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   const char *label_format; /**< The format string for the label. e.g., "%.2f units" */
   double min; /**< The minimum value for the spinner. */
   double max; /**< The maximum value for the spinner. */
   double step; /**< The step increment/decrement value. */
   double value; /**< The current value of the spinner. */
   Eina_Bool min_exists:1; /**< Flag indicating if 'min' is set. */
   Eina_Bool max_exists:1; /**< Flag indicating if 'max' is set. */
   Eina_Bool step_exists:1; /**< Flag indicating if 'step' is set. */
   Eina_Bool value_exists:1; /**< Flag indicating if 'value' is set. */
   Eina_Bool wrap_exists:1; /**< Flag indicating if 'wrap' mode is set. */
   Eina_Bool wrap:1; /**< The wrap mode. If EINA_TRUE, the spinner wraps around when reaching min/max. */
} Elm_Params_Spinner;

/**
 * @brief Sets the state of an external spinner widget.
 *
 * This function is called to apply a set of parameters (either `from_params` or
 * `to_params`) to the spinner object `obj`. It's typically used in animations
 * or transitions where the spinner's properties are changed over time.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (spinner) to set the state for.
 * @param from_params The initial state parameters (can be NULL).
 * @param to_params The target state parameters (can be NULL).
 * @param pos The position in the transition (0.0 to 1.0), unused in this function.
 */
static void
external_spinner_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                           const void *from_params, const void *to_params,
                           float pos EINA_UNUSED)
{
   const Elm_Params_Spinner *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label_format)
     elm_spinner_label_format_set(obj, p->label_format);
   if ((p->min_exists) && (p->max_exists))
     elm_spinner_min_max_set(obj, p->min, p->max);
   else if ((p->min_exists) || (p->max_exists))
     {
        double min, max;
        elm_spinner_min_max_get(obj, &min, &max);
        if (p->min_exists)
          elm_spinner_min_max_set(obj, p->min, max);
        else
          elm_spinner_min_max_set(obj, min, p->max);
     }
   if (p->step_exists)
     elm_spinner_step_set(obj, p->step);
   if (p->value_exists)
     elm_spinner_value_set(obj, p->value);
   if (p->wrap_exists)
     elm_spinner_wrap_set(obj, p->wrap);
}

/**
 * @brief Sets a specific external parameter for the spinner widget.
 *
 * This function is called by Edje to set a single parameter on the spinner
 * object. It handles various parameter types like string, double, and boolean.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (spinner) to set the parameter on.
 * @param param A pointer to an Edje_External_Param structure containing the
 *              parameter name, type, and value to be set.
 *              Example for param->name: "label format", "min", "max", "step", "value", "wrap".
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_spinner_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                           const Edje_External_Param *param)
{
   if (!strcmp(param->name, "label format"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_spinner_label_format_set(obj, param->s);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "min"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             double min, max;
             elm_spinner_min_max_get(obj, &min, &max);
             elm_spinner_min_max_set(obj, param->d, max);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "max"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             double min, max;
             elm_spinner_min_max_get(obj, &min, &max);
             elm_spinner_min_max_set(obj, min, param->d);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "step"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             elm_spinner_step_set(obj, param->d);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "value"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             elm_spinner_value_set(obj, param->d);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "wrap"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_spinner_wrap_set(obj, param->i);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific external parameter from the spinner widget.
 *
 * This function is called by Edje to retrieve the current value of a single
 * parameter from the spinner object.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (spinner) to get the parameter from.
 * @param param A pointer to an Edje_External_Param structure where the
 *              parameter's current value will be stored. The `name` and `type`
 *              fields of this struct indicate which parameter to retrieve.
 *              Example for param->name: "label format", "min", "max", "step", "value", "wrap".
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_spinner_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                           Edje_External_Param *param)
{
   if (!strcmp(param->name, "label format"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_spinner_label_format_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "min"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             double min, max;
             elm_spinner_min_max_get(obj, &min, &max);
             param->d = min;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "max"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             double min, max;
             elm_spinner_min_max_get(obj, &min, &max);
             param->d = max;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "step"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             param->d = elm_spinner_step_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "value"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             param->d = elm_spinner_value_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "wrap"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_spinner_value_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of external parameters and stores them.
 *
 * This function is called to convert a list of Edje_External_Param structures
 * into an Elm_Params_Spinner structure. This parsed structure can then be used
 * by `external_spinner_state_set` to apply the parameters.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object, unused in this function.
 * @param params A list (Eina_List) of Edje_External_Param structures to parse.
 *               Each element in the list is an Edje_External_Param.
 *               Example structure of params list elements:
 *               - param1: {name="label format", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="Value: %f"}
 *               - param2: {name="min", type=EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, d=0.0}
 *               - param3: {name="max", type=EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, d=100.0}
 * @return A pointer to a newly allocated Elm_Params_Spinner structure containing
 *         the parsed parameters, or NULL on failure. The caller is responsible
 *         for freeing this memory using `external_spinner_params_free`.
 */
static void *
external_spinner_params_parse(void *data EINA_UNUSED,
                              Evas_Object *obj EINA_UNUSED,
                              const Eina_List *params)
{
   Elm_Params_Spinner *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Spinner));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "label format"))
          mem->label_format = eina_stringshare_add(param->s);
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
        else if (!strcmp(param->name, "step"))
          {
             mem->step = param->d;
             mem->step_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "value"))
          {
             mem->value = param->d;
             mem->value_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "wrap"))
          {
             mem->wrap = param->i;
             mem->wrap_exists = EINA_TRUE;
          }
     }

   return mem;
}

/**
 * @brief Retrieves a content part of the spinner widget.
 *
 * Spinners typically do not have named content parts that can be retrieved
 * in this manner, so this function currently returns NULL and logs an error.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (spinner), unused.
 * @param content The name of the content part to retrieve, unused.
 * @return Always NULL for spinner, as it does not support named content parts.
 */
static Evas_Object *external_spinner_content_get(void *data EINA_UNUSED,
                                                 const Evas_Object *obj EINA_UNUSED,
                                                 const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for parsed spinner parameters.
 *
 * This function is used to release the Elm_Params_Spinner structure that was
 * allocated by `external_spinner_params_parse`.
 *
 * @param params A pointer to the Elm_Params_Spinner structure to free.
 */
static void
external_spinner_params_free(void *params)
{
   Elm_Params_Spinner *mem = params;

   if (mem->label_format)
     eina_stringshare_del(mem->label_format);
   free(mem);
}

/**
 * @brief Defines the external parameters available for the spinner widget.
 *
 * This array provides metadata about the parameters that can be set or get
 * on a spinner widget externally (e.g., via an Edje theme). It includes
 * parameter names, types, and default values.
 */
static Edje_External_Param_Info external_spinner_params[] = {
     DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "disabled". */
     EDJE_EXTERNAL_PARAM_INFO_STRING_DEFAULT("label format", "%1.2f"), /**< Format string for the spinner's label. */
     EDJE_EXTERNAL_PARAM_INFO_DOUBLE("min"), /**< Minimum value of the spinner. */
     EDJE_EXTERNAL_PARAM_INFO_DOUBLE_DEFAULT("max", 100.0), /**< Maximum value of the spinner, defaults to 100.0. */
     EDJE_EXTERNAL_PARAM_INFO_DOUBLE_DEFAULT("step", 1.0), /**< Step increment/decrement value, defaults to 1.0. */
     EDJE_EXTERNAL_PARAM_INFO_DOUBLE("value"), /**< Current value of the spinner. */
     EDJE_EXTERNAL_PARAM_INFO_BOOL("wrap"), /**< Boolean to enable/disable wrapping. */
     EDJE_EXTERNAL_PARAM_INFO_SENTINEL /**< Marks the end of the parameter list. */
};

DEFINE_EXTERNAL_ICON_ADD(spinner, "spinner");
DEFINE_EXTERNAL_TYPE_SIMPLE(spinner, "Spinner");
