#include "private.h"

/**
 * @brief Structure for web widget parameters.
 *
 * This structure holds all the parameters that can be set for an
 * Elementary web widget through the Edje external interface.
 */
typedef struct _Elm_Params_Web
{
   Elm_Params base; /**< Base parameters */
   const char *uri; /**< The URI to load in the web widget. Example: "http://www.enlightenment.org" */
   double zoom; /**< The zoom level of the web widget. Example: 1.0 for normal, 2.0 for double size. */
   Elm_Web_Zoom_Mode zoom_mode; /**< The zoom mode of the web widget. @see Elm_Web_Zoom_Mode */
   Eina_Bool inwin_mode; /**< Whether the web widget is in inwin mode. */
   Eina_Bool zoom_set:1; /**< Flag indicating if the zoom level has been set. */
   Eina_Bool inwin_mode_set:1; /**< Flag indicating if the inwin mode has been set. */
} Elm_Params_Web;

static const char *zoom_choices[] = { "manual", "auto fit", "auto fill", NULL };

/**
 * @brief Get the zoom mode from a string.
 *
 * @param zoom The string representation of the zoom mode.
 *             Example: "manual", "auto fit", "auto fill".
 * @return The corresponding Elm_Web_Zoom_Mode enum value, or ELM_WEB_ZOOM_MODE_LAST if not found.
 */
static Elm_Web_Zoom_Mode
_zoom_mode_get(const char *zoom)
{
   unsigned int i;

   for (i = 0; i < ELM_WEB_ZOOM_MODE_LAST; i++)
     if (!strcmp(zoom, zoom_choices[i])) return i;

   return ELM_WEB_ZOOM_MODE_LAST;
}

/**
 * @brief Set the state of the web widget.
 *
 * This function is called by Edje to apply parameters to the web widget
 * during state transitions.
 *
 * @param data Unused.
 * @param obj The web widget object.
 * @param from_params The parameters of the previous state.
 * @param to_params The parameters of the target state.
 * @param pos Unused.
 */
static void
external_web_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                       const void *from_params, const void *to_params,
                       float pos EINA_UNUSED)
{
   const Elm_Params_Web *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->uri)
     elm_web_url_set(obj, p->uri);
   if (p->zoom_mode < ELM_WEB_ZOOM_MODE_LAST)
     elm_web_zoom_mode_set(obj, p->zoom_mode);
   if (p->zoom_set)
     elm_web_zoom_set(obj, p->zoom);
   if (p->inwin_mode_set)
     elm_web_inwin_mode_set(obj, p->inwin_mode);
}

/**
 * @brief Set a specific parameter for the web widget.
 *
 * This function is called by Edje to set individual parameters on the
 * web widget.
 *
 * @param data Unused.
 * @param obj The web widget object.
 * @param param The parameter to set.
 *              Example for 'uri': param->name = "uri", param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING, param->s = "http://example.com"
 *              Example for 'zoom level': param->name = "zoom level", param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, param->d = 1.5
 *              Example for 'zoom mode': param->name = "zoom mode", param->type = EDJE_EXTERNAL_PARAM_TYPE_CHOICE, param->s = "auto fit"
 *              Example for 'inwin mode': param->name = "inwin mode", param->type = EDJE_EXTERNAL_PARAM_TYPE_BOOL, param->i = 1
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_web_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                       const Edje_External_Param *param)
{
   if (!strcmp(param->name, "uri"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_web_url_set(obj, param->s);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "zoom level"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             elm_web_zoom_set(obj, param->d);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "zoom mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_Web_Zoom_Mode mode = _zoom_mode_get(param->s);
             if (mode == ELM_WEB_ZOOM_MODE_LAST)
               return EINA_FALSE;
             elm_web_zoom_mode_set(obj, mode);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "inwin mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_web_inwin_mode_set(obj, !!param->i);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Get a specific parameter from the web widget.
 *
 * This function is called by Edje to retrieve individual parameters from
 * the web widget.
 *
 * @param data Unused.
 * @param obj The web widget object.
 * @param param The parameter to get (name and type are set, value is filled).
 *              Example for 'uri': param->name = "uri", param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING. param->s will be set.
 *              Example for 'zoom level': param->name = "zoom level", param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE. param->d will be set.
 *              Example for 'zoom mode': param->name = "zoom mode", param->type = EDJE_EXTERNAL_PARAM_TYPE_CHOICE. param->s will be set.
 *              Example for 'inwin mode': param->name = "inwin mode", param->type = EDJE_EXTERNAL_PARAM_TYPE_BOOL. param->i will be set.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_web_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                       Edje_External_Param *param)
{
   if (!strcmp(param->name, "uri"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_web_url_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "zoom level"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             param->d = elm_web_zoom_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "zoom mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_Web_Zoom_Mode mode = elm_web_zoom_mode_get(obj);
             if (mode == ELM_WEB_ZOOM_MODE_LAST)
               return EINA_FALSE;
             param->s = zoom_choices[mode];
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "inwin mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_web_inwin_mode_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parse a list of parameters for the web widget.
 *
 * This function is called by Edje to parse a list of parameters and
 * create an Elm_Params_Web structure.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param params A list of Edje_External_Param structures.
 *               Example list structure:
 *               params = [
 *                 { name = "uri", type = EDJE_EXTERNAL_PARAM_TYPE_STRING, s = "http://example.com" },
 *                 { name = "zoom level", type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, d = 1.5 },
 *                 ...
 *               ]
 * @return A newly allocated Elm_Params_Web structure, or NULL on failure.
 */
static void *
external_web_params_parse(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                          const Eina_List *params)
{
   Elm_Params_Web *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Web));
   if (!mem) return NULL;

   mem->zoom_mode = ELM_WEB_ZOOM_MODE_LAST;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "zoom level"))
          {
             mem->zoom = param->d;
             mem->zoom_set = EINA_TRUE;
          }
        else if (!strcmp(param->name, "zoom mode"))
          mem->zoom_mode = _zoom_mode_get(param->s);
        else if (!strcmp(param->name, "uri"))
          mem->uri = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "inwin mode"))
          {
             mem->inwin_mode = !!param->i;
             mem->inwin_mode_set = EINA_TRUE;
          }
     }

   return mem;
}

/**
 * @brief Free the parsed web widget parameters.
 *
 * This function is called by Edje to free the Elm_Params_Web structure
 * created by external_web_params_parse().
 *
 * @param params The Elm_Params_Web structure to free.
 */
static void
external_web_params_free(void *params)
{
   Elm_Params_Web *mem = params;

   if (mem->uri)
     eina_stringshare_del(mem->uri);
   free(mem);
}

/**
 * @brief Get content from the web widget.
 *
 * This function is currently not implemented and always returns NULL.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param content Unused.
 * @return Always NULL.
 */
static Evas_Object *
external_web_content_get(void *data EINA_UNUSED,
                         const Evas_Object *obj EINA_UNUSED,
                         const char *content EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Information about the external parameters for the web widget.
 *
 * This array defines the parameters that can be used with the web widget
 * in an Edje theme.
 * Example of usage in an .edc file:
 *
 * @code
 * external "elm/web";
 * params {
 *   "uri" string: "http://www.enlightenment.org";
 *   "zoom level" float: 1.0;
 *   "zoom mode" choice: "manual", "auto fit", "auto fill"; // Default: "manual"
 *   "inwin mode" bool: false; // Default: false
 * }
 * @endcode
 */
static Edje_External_Param_Info external_web_params[] =
{
   EDJE_EXTERNAL_PARAM_INFO_STRING("uri"),
   EDJE_EXTERNAL_PARAM_INFO_DOUBLE_DEFAULT("zoom level", 1.0),
   EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("zoom mode", "manual", zoom_choices),
   EDJE_EXTERNAL_PARAM_INFO_BOOL_DEFAULT("inwin mode", EINA_FALSE),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

/**
 * @brief Create a new web widget as an external object.
 *
 * This function is called by Edje to create a new web widget instance
 * when it's used as an external part in a theme.
 *
 * @param data Unused.
 * @param evas Unused.
 * @param edje The Edje object that is requesting the web widget.
 * @param params Unused.
 * @param part_name The name of the part in the Edje theme.
 * @return The newly created web widget object.
 */
static Evas_Object *
external_web_add(void *data EINA_UNUSED, Evas *evas EINA_UNUSED,
                 Evas_Object *edje, const Eina_List *params EINA_UNUSED,
                 const char *part_name)
{
   Evas_Object *parent, *obj;
   external_elm_init();
   parent = elm_widget_parent_widget_get(edje);
   if (!parent) parent = edje;
   elm_need_web(); /* extra command needed */
   obj = elm_web_add(parent);
   external_signals_proxy(obj, edje, part_name);
   return obj;
}

DEFINE_EXTERNAL_ICON_ADD(web, "web");
DEFINE_EXTERNAL_TYPE(web, "Web");
