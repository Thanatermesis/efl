#include <assert.h>
#include "private.h"

/**
 * @brief Parameters for the external calendar widget.
 *
 * This structure holds all the parameters that can be configured for an
 * external calendar widget. It extends the base Elm_Params structure.
 */
typedef struct _Elm_Params_Calendar
{
   Elm_Params base; /**< Base parameters */
   int year_min; /**< The minimum year to display. 0 if not set. */
   int year_max; /**< The maximum year to display. 0 if not set. */
   const char *select_mode; /**< The selection mode as a string. e.g., "default", "always", "none", "ondemand" */

} Elm_Params_Calendar;

#define SELECT_MODE_GET(CHOICES, STR)                                \
   unsigned int i;                                              \
   for (i = 0; i < (sizeof(CHOICES) / sizeof(CHOICES[0])); ++i) \
     if (!strcmp(STR, CHOICES[i]))                              \
       return i;

/**
 * @brief String representations of calendar selection modes.
 *
 * This array maps the Elm_Calendar_Select_Mode enum values to their
 * string equivalents. The order must match the enum definition.
 * The `NULL` at the end is a sentinel.
 * - "default": The default selection behavior.
 * - "always": Selection is always visible.
 * - "none": No selection is allowed.
 * - "ondemand": Selection is shown only when needed.
 */
static const char *_calendar_select_modes[] =
{
   "default", "always", "none", "ondemand", NULL
};

/**
 * @brief Converts a selection mode string to its corresponding enum value.
 *
 * @param select_mode The string representation of the selection mode.
 * @return The Elm_Calendar_Select_Mode enum value, or -1 if not found.
 */
static Elm_Calendar_Select_Mode
_calendar_select_mode_get(const char *select_mode)
{
   assert(sizeof(_calendar_select_modes) /
          sizeof(_calendar_select_modes[0])
          == ELM_CALENDAR_SELECT_MODE_ONDEMAND + 2);
   SELECT_MODE_GET(_calendar_select_modes, select_mode);
   return -1;
}

/**
 * @brief Sets the state of the calendar widget.
 *
 * This function is called by Edje to apply a state to the external
 * calendar object. It interpolates between `from_params` and `to_params`
 * but in this case, it just applies the target state (`to_params` if it
 * exists, otherwise `from_params`).
 *
 * @param data Unused.
 * @param obj The calendar widget object.
 * @param from_params The starting state parameters.
 * @param to_params The ending state parameters.
 * @param pos Unused.
 */
static void
external_calendar_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                            const void *from_params, const void *to_params,
                            float pos EINA_UNUSED)
{
   const Elm_Params_Calendar *p;
   Elm_Calendar_Select_Mode select_mode;
   int min,max;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->year_min)
     {
        elm_calendar_min_max_year_get(obj, NULL, &max);
        elm_calendar_min_max_year_set(obj, p->year_min, max);
     }
   if (p->year_max)
     {
        elm_calendar_min_max_year_get(obj, &min, NULL);
        elm_calendar_min_max_year_set(obj, min, p->year_max);
     }
   if (p->select_mode)
     {
        select_mode = _calendar_select_mode_get(p->select_mode);
        elm_calendar_select_mode_set(obj, select_mode);
     }
}

/**
 * @brief Sets a specific parameter on the calendar widget.
 *
 * This function is a callback used by Edje to set a single parameter on the
 * external calendar widget. It handles parameters like "year_min", "year_max",
 * and "select_mode".
 *
 * @param data Unused.
 * @param obj The calendar widget object.
 * @param param The parameter to set.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_calendar_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                            const Edje_External_Param *param)
{
   int min,max;

   if (!strcmp(param->name, "year_min"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             elm_calendar_min_max_year_get(obj, NULL, &max);
             elm_calendar_min_max_year_set(obj, param->i, max);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "year_max"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             elm_calendar_min_max_year_get(obj, &min, NULL);
             elm_calendar_min_max_year_set(obj, min,param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "select_mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             Elm_Calendar_Select_Mode select_mode;
             select_mode = _calendar_select_mode_get(param->s);
             elm_calendar_select_mode_set(obj, select_mode);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific parameter from the calendar widget.
 *
 * This function is a callback used by Edje to retrieve the value of a single
 * parameter from the external calendar widget.
 *
 * @param data Unused.
 * @param obj The calendar widget object.
 * @param param A pointer to an Edje_External_Param structure to be filled.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_calendar_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                            Edje_External_Param *param)
{
   int min, max;

   if (!strcmp(param->name, "year_min"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             elm_calendar_min_max_year_get(obj, &(param->i) ,&max);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "year_max"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             elm_calendar_min_max_year_get(obj, &min,&(param->i));
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "select_mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             Elm_Calendar_Select_Mode mode;
             mode = elm_calendar_select_mode_get(obj);
             param->s = _calendar_select_modes[mode];
             return EINA_TRUE;
          }
     }


   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of external parameters and creates a parameter struct.
 *
 * This function is called by Edje to parse a list of parameters from the
 * EDC theme file and create an Elm_Params_Calendar structure that holds them.
 * This structure is later used by `external_calendar_state_set`.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param params A list of Edje_External_Param to parse.
 * @return A newly allocated Elm_Params_Calendar structure, or NULL on failure.
 *         The caller is responsible for freeing this memory.
 */
static void *
external_calendar_params_parse(void *data EINA_UNUSED,
                               Evas_Object *obj EINA_UNUSED,
                               const Eina_List *params)
{
   Elm_Params_Calendar *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Calendar));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "year_min"))
          mem->year_min = param->i;

        else if (!strcmp(param->name, "year_max"))
          mem->year_max = param->i;

        else if (!strcmp(param->name, "select_mode"))
          mem->select_mode = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Gets a content object from the calendar.
 *
 * This function is supposed to return a swallowable content object. The
 * calendar widget does not support this, so it always returns NULL and
 * logs an error.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param content Unused.
 * @return Always returns NULL.
 */
static Evas_Object *
external_calendar_content_get(void *data EINA_UNUSED,
                              const Evas_Object *obj EINA_UNUSED,
                              const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the parameter structure.
 *
 * This function is called by Edje to free the memory allocated by
 * `external_calendar_params_parse`.
 *
 * @param params The Elm_Params_Calendar structure to free.
 */
static void
external_calendar_params_free(void *params)
{
   Elm_Params_Calendar *mem = params;
   if (mem->select_mode)
     eina_stringshare_del(mem->select_mode);
   free(params);
}

/**
 * @brief Defines the external parameters for the calendar widget.
 *
 * This array provides information about the parameters that can be used
 * in an EDC file to configure a calendar widget. It includes common
 * parameters and calendar-specific ones like year range and selection mode.
 */
static Edje_External_Param_Info external_calendar_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_INT("year_min"),
   EDJE_EXTERNAL_PARAM_INFO_INT("year_max"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("select_mode"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(calendar, "calendar");
DEFINE_EXTERNAL_TYPE_SIMPLE(calendar, "Calendar");
