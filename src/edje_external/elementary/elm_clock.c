#include "private.h"

/**
 * @brief Parameters for the clock widget.
 *
 * This structure holds all the parameters that can be used to configure
 * a clock widget from an Edje theme. It includes time components (hours,
 * minutes, seconds), flags to indicate which components are set, and
 * display options like edit mode, AM/PM format, and seconds visibility.
 */
typedef struct _Elm_Params_Clock
{
   Elm_Params base;
   int hrs, min, sec;
   Eina_Bool hrs_exists:1;
   Eina_Bool min_exists:1;
   Eina_Bool sec_exists:1;
   Eina_Bool edit:1;
   Eina_Bool ampm:1;
   Eina_Bool seconds:1;
} Elm_Params_Clock;

/**
 * @brief Set the state of the clock object.
 *
 * This function is called by Edje to apply a state to the clock object.
 * It takes parameter sets for the start and end states and applies
 * properties to the clock widget. If only one set of parameters is
 * provided, it's used as the target state.
 *
 * @param data Unused.
 * @param obj The clock Evas_Object to modify.
 * @param from_params The parameters for the starting state.
 * @param to_params The parameters for the ending state.
 * @param pos Unused.
 */
static void
external_clock_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                         const void *from_params, const void *to_params,
                         float pos EINA_UNUSED)
{
   const Elm_Params_Clock *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if ((p->hrs_exists) && (p->min_exists) && (p->sec_exists))
     elm_clock_time_set(obj, p->hrs, p->min, p->sec);
   else if ((p->hrs_exists) || (p->min_exists) || (p->sec_exists))
     {
        int hrs, min, sec;
        elm_clock_time_get(obj, &hrs, &min, &sec);
        if (p->hrs_exists)
          hrs = p->hrs;
        if (p->min_exists)
          min = p->min;
        if (p->sec_exists)
          sec = p->sec;
        elm_clock_time_set(obj, hrs, min, sec);
     }
   if (p->edit)
     elm_clock_edit_set(obj, p->edit);
   if (p->ampm)
     elm_clock_show_am_pm_set(obj, p->ampm);
   if (p->seconds)
     elm_clock_show_seconds_set(obj, p->seconds);
}

/**
 * @brief Set a specific parameter on the clock object.
 *
 * This function is called by Edje to set a single parameter on the clock
 * object. It handles parameters like "hours", "minutes", "seconds",
 * "editable", "am/pm", and "show seconds".
 *
 * @param data Unused.
 * @param obj The clock Evas_Object to modify.
 * @param param The parameter to set.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_clock_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                         const Edje_External_Param *param)
{
   if (!strcmp(param->name, "hours"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             int hrs, min, sec;
             elm_clock_time_get(obj, &hrs, &min, &sec);
             elm_clock_time_set(obj, param->i, min, sec);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "minutes"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             int hrs, min, sec;
             elm_clock_time_get(obj, &hrs, &min, &sec);
             elm_clock_time_set(obj, hrs, param->i, sec);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "seconds"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             int hrs, min, sec;
             elm_clock_time_get(obj, &hrs, &min, &sec);
             elm_clock_time_set(obj, hrs, min, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "editable"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_clock_edit_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "am/pm"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_clock_show_am_pm_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "show seconds"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_clock_show_seconds_set(obj, param->i);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Get a specific parameter from the clock object.
 *
 * This function is called by Edje to retrieve the value of a single
 * parameter from the clock object.
 *
 * @param data Unused.
 * @param obj The clock Evas_Object to query.
 * @param param The parameter to get, its value will be filled in.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_clock_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                         Edje_External_Param *param)
{
   if (!strcmp(param->name, "hours"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             int hrs, min, sec;
             elm_clock_time_get(obj, &hrs, &min, &sec);
             param->i = hrs;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "minutes"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             int hrs, min, sec;
             elm_clock_time_get(obj, &hrs, &min, &sec);
             param->i = min;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "seconds"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             int hrs, min, sec;
             elm_clock_time_get(obj, &hrs, &min, &sec);
             param->i = sec;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "editable"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_clock_edit_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "am/pm"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_clock_show_am_pm_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "show seconds"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_clock_show_seconds_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parse a list of external parameters.
 *
 * This function converts an Eina_List of Edje_External_Param structures
 * into an Elm_Params_Clock structure. This is used by Edje to prepare
 * the parameters for a state change.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param params A list of Edje_External_Param to parse.
 * @return A newly allocated Elm_Params_Clock structure, or NULL on failure.
 *         The caller is responsible for freeing the returned structure.
 */
static void *
external_clock_params_parse(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                            const Eina_List *params)
{
   Elm_Params_Clock *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Clock));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "hours"))
          {
             mem->hrs = param->i;
             mem->hrs_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "minutes"))
          {
             mem->min = param->i;
             mem->min_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "seconds"))
          {
             mem->sec = param->i;
             mem->sec_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "editable"))
          mem->edit = !!param->i;
        else if (!strcmp(param->name, "am/pm"))
          mem->ampm = !!param->i;
        else if (!strcmp(param->name, "show seconds"))
          mem->seconds = !!param->i;
     }

   return mem;
}

/**
 * @brief Get content from the clock object.
 *
 * This clock widget does not support content objects. This function
 * will always return NULL and print an error.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param content Unused.
 * @return Always NULL.
 */
static Evas_Object *external_clock_content_get(void *data EINA_UNUSED,
                                               const Evas_Object *obj EINA_UNUSED, const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Free the parsed parameters structure.
 *
 * This function is called by Edje to free the memory allocated by
 * external_clock_params_parse().
 *
 * @param params The Elm_Params_Clock structure to free.
 */
static void
external_clock_params_free(void *params)
{
   Elm_Params_Clock *mem = params;

   free(mem);
}

static Edje_External_Param_Info external_clock_params[] = {
     DEFINE_EXTERNAL_COMMON_PARAMS,
     EDJE_EXTERNAL_PARAM_INFO_INT("hours"),
     EDJE_EXTERNAL_PARAM_INFO_INT("minutes"),
     EDJE_EXTERNAL_PARAM_INFO_INT("seconds"),
     EDJE_EXTERNAL_PARAM_INFO_BOOL("editable"),
     EDJE_EXTERNAL_PARAM_INFO_BOOL("am/pm"),
     EDJE_EXTERNAL_PARAM_INFO_BOOL("show seconds"),
     EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(clock, "clock");
DEFINE_EXTERNAL_TYPE_SIMPLE(clock, "Clock");
