#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "Elementary.h"
#include "private.h"

/**
 * @internal
 * @brief Log domain for Elementary externals.
 *
 * This variable stores the Eina log domain identifier used by the
 * Elementary external types system. It is initialized by elm_mod_init()
 * and unregistered by elm_mod_shutdown(). A value of -1 indicates
 * that the log domain has not been registered or has been unregistered.
 */
int _elm_ext_log_dom = -1;

/**
 * @internal
 * @brief Initialization counter for Elementary.
 *
 * This static variable keeps track of how many times Elementary has been
 * initialized via external_elm_init(). It ensures that the actual
 * elm_init() and elm_shutdown() functions are called only once, even if
 * external_elm_init() and external_elm_shutdown() are called multiple times
 * (e.g., by different external modules).
 */
static int init_count = 0;

/**
 * @internal
 * @brief Initializes Elementary if not already initialized.
 *
 * This function increments a counter and calls elm_init() only if
 * the counter was zero. This allows multiple external users to request
 * Elementary initialization without causing multiple actual initializations.
 * It retrieves application arguments using ecore_app_args_get() to pass
 * them to elm_init().
 */
void
external_elm_init(void)
{
   int argc = 0;
   char **argv = NULL;

   init_count++;
   DBG("elm_real_init\n");
   if (init_count > 1) return;
   ecore_app_args_get(&argc, &argv);
   elm_init(argc, argv);
}

/**
 * @internal
 * @brief Shuts down Elementary if this is the last user.
 *
 * This function decrements a counter and calls elm_shutdown() only if
 * the counter reaches zero. This allows multiple external users to request
 * Elementary shutdown without causing premature shutdown.
 */
static void
external_elm_shutdown(void)
{
   init_count--;
   DBG("elm_real_shutdown\n");
   if (init_count > 0) return;
   elm_shutdown();
}

/**
 * @internal
 * @brief Handles an external signal and relays it to an Edje object.
 *
 * This function is called when an external system (like an Edje external
 * object) emits a signal that needs to be processed by an Elementary widget
 * or its content. It parses the signal string, which is expected to be in
 * the format "part_name]:signal_name", retrieves the content object
 * associated with the `_signal` (part_name) from the `obj`'s external type,
 * and then emits the `signal_name` to that content object.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object that is the source of the external signal,
 *            typically an Edje external object.
 * @param sig The signal string, potentially in the format "part_name]:signal_name".
 *            For example, "elm.swallow.content]:my_signal".
 * @param source The source string of the signal.
 */
void
external_signal(void *data EINA_UNUSED, Evas_Object *obj, const char *sig,
                const char *source)
{
   char *_signal = strdup(sig);
   char *p = _signal;
   Evas_Object *content;
   Edje_External_Type *type;

   if (!p) goto on_error;

   while ((*p!='\0') && (*p!=']'))
     p++;


   if ((*p=='\0') || (*(p+1)!=':'))
     {
        ERR("Invalid External Signal received: '%s' '%s'", sig, source);
        goto on_error;
     }

   *p = '\0';
   p+=2; //jump ']' and ':'

   type = evas_object_data_get(obj, "Edje_External_Type");
   if (!type)
     {
        ERR("no external type for object %p", obj);
        goto on_error;
     }
   if (!type->content_get)
     {
        ERR("external type '%s' from module '%s' does not provide content_get()",
            type->module_name, type->module);
        goto on_error;
     }

   content = type->content_get(type->data, obj, _signal);
   if (content)
     edje_object_signal_emit(content, sig + (p - _signal), source);

on_error:
   free(_signal);
   return;
}

/**
 * @internal
 * @brief Translates a string using Elementary's text domain.
 *
 * Currently, this function is a placeholder and returns the original string.
 * In the future, it's intended to mark all parameters as translatable and
 * use dgettext() with the "elementary" text domain for localization.
 *
 * @param data Unused user data.
 * @param orig The original string to be translated.
 * @return The translated string, or the original string if no translation
 *         is available or if translation is not yet implemented.
 */
const char *
external_translate(void *data EINA_UNUSED, const char *orig)
{
   // in future, mark all params as translatable and use dgettext()
   // with "elementary" text domain here.
   return orig;
}

/**
 * @internal
 * @brief Context structure for proxying signals from an Evas_Object to an Edje object.
 *
 * This structure holds the necessary information to forward a smart callback
 * (signal) from a source Evas_Object to a target Edje object.
 * It includes the emission signal name, the source string, and a pointer
 * to the target Edje object.
 */
typedef struct {
   const char *emission; /**< The signal name to be emitted on the Edje object. */
   const char *source;   /**< The source string for the emitted signal. */
   Evas_Object *edje;    /**< The target Edje object to which the signal will be emitted. */
} Elm_External_Signals_Proxy_Context;

/**
 * @internal
 * @brief Frees the context for signal proxying and shuts down Elementary.
 *
 * This callback is invoked when the Evas_Object, to which the signal proxy
 * was attached, is freed. It releases the allocated
 * Elm_External_Signals_Proxy_Context and calls external_elm_shutdown()
 * to decrement the Elementary initialization counter.
 *
 * @param data The Elm_External_Signals_Proxy_Context to be freed.
 * @param e Unused Evas canvas.
 * @param obj Unused Evas_Object that was freed.
 * @param event_info Unused event information.
 */
static void
_external_signal_proxy_free_cb(void *data, Evas *e EINA_UNUSED,
                               Evas_Object *obj EINA_UNUSED,
                               void *event_info EINA_UNUSED)
{
   Elm_External_Signals_Proxy_Context *ctxt = data;
   external_elm_shutdown();
   free(ctxt);
}

/**
 * @internal
 * @brief Callback to proxy a signal from an Evas_Object to an Edje object.
 *
 * This function is called when a smart callback (signal) is emitted by the
 * source Evas_Object. It retrieves the context containing the target Edje
 * object, emission signal, and source, and then emits the signal on the
 * target Edje object.
 *
 * @param data The Elm_External_Signals_Proxy_Context containing proxy information.
 * @param obj Unused Evas_Object that emitted the signal.
 * @param event_info Unused event information associated with the signal.
 */
static void
_external_signal_proxy_cb(void *data, Evas_Object *obj EINA_UNUSED,
                          void *event_info EINA_UNUSED)
{
   Elm_External_Signals_Proxy_Context *ctxt = data;
   // TODO: Is it worth to check Evas_Smart_Cb_Description and do something
   // TODO: with event_info given its description?
   edje_object_signal_emit(ctxt->edje, ctxt->emission, ctxt->source);
}

/**
 * @internal
 * @brief Gets common Elementary object parameters.
 *
 * This function retrieves common properties of an Elementary object, such as
 * "style" or "disabled state", and populates the Edje_External_Param structure.
 * It is typically used by Edje external type implementations to query
 * properties of the underlying Elementary widget.
 *
 * @param data Unused user data.
 * @param obj The Elementary Evas_Object whose parameter is being queried.
 * @param param A pointer to an Edje_External_Param structure. The `name` field
 *              indicates which parameter to get (e.g., "style", "disabled").
 *              The `type` field should match the expected type of the parameter.
 *              The function will fill the appropriate value field (e.g., `s` for
 *              string, `i` for integer/boolean) if the parameter is found and
 *              the type matches.
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 *
 * @par Example
 * Edje_External_Param param;
 * param.name = "style";
 * param.type = EDJE_EXTERNAL_PARAM_TYPE_STRING;
 * if (external_common_param_get(NULL, my_elm_object, &param)) {
 *   printf("Style: %s\n", param.s); // param.s will be a stringshared string
 * }
 */
Eina_Bool
external_common_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                          Edje_External_Param *param)
{
   if (!strcmp(param->name, "style"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_object_style_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "disabled"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_object_disabled_get(obj);
             return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Sets common Elementary object parameters.
 *
 * This function sets common properties of an Elementary object, such as
 * "style" or "disabled state", based on the provided Edje_External_Param.
 * It is typically used by Edje external type implementations to configure
 * the underlying Elementary widget.
 *
 * @param data Unused user data.
 * @param obj The Elementary Evas_Object whose parameter is being set.
 * @param param A pointer to a const Edje_External_Param structure containing
 *              the parameter name (e.g., "style", "disabled"), type, and
 *              value to set.
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 *
 * @par Example
 * Edje_External_Param param;
 * param.name = "style";
 * param.type = EDJE_EXTERNAL_PARAM_TYPE_STRING;
 * param.s = "my_custom_style";
 * external_common_param_set(NULL, my_elm_object, &param);
 *
 * param.name = "disabled";
 * param.type = EDJE_EXTERNAL_PARAM_TYPE_BOOL;
 * param.i = EINA_TRUE; // Disable the object
 * external_common_param_set(NULL, my_elm_object, &param);
 */
Eina_Bool
external_common_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                          const Edje_External_Param *param)
{
   if (!strcmp(param->name, "style"))
     {
         if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
           {
              elm_object_style_set(obj, param->s);
              return EINA_TRUE;
           }
     }
   else if (!strcmp(param->name, "disabled"))
     {
         if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
           {
              elm_object_disabled_set(obj, param->i);
              return EINA_TRUE;
           }
     }
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Sets up proxying for all smart callbacks from one Evas_Object to an Edje object.
 *
 * This function iterates over all class and instance smart callback descriptions
 * of the given `obj`. For each described callback, it adds a generic proxy
 * callback (_external_signal_proxy_cb) to `obj`. When `obj` emits one of these
 * smart callbacks, the proxy callback will be triggered, which in turn emits
 * a corresponding signal on the `edje` object.
 *
 * This is useful for automatically forwarding signals from an Elementary widget
 * (or any Evas_Object with smart callbacks) to an Edje object that might be
 * visually representing or controlling it.
 *
 * A context structure (Elm_External_Signals_Proxy_Context) is allocated to store
 * information for all proxies. This context is freed when `obj` is deleted,
 * using EVAS_CALLBACK_FREE.
 *
 * @param obj The source Evas_Object whose smart callbacks are to be proxied.
 * @param edje The target Edje Evas_Object to which signals will be emitted.
 * @param part_name The "source" string to be used when emitting signals on the
 *                  `edje` object. This often corresponds to an Edje part name.
 */
void
external_signals_proxy(Evas_Object *obj, Evas_Object *edje, const char *part_name)
{
   const Evas_Smart_Cb_Description **cls_descs, **inst_descs;
   unsigned int cls_count, inst_count, total;
   Elm_External_Signals_Proxy_Context *ctxt;

   evas_object_smart_callbacks_descriptions_get
     (obj, &cls_descs, &cls_count, &inst_descs, &inst_count);

   total = cls_count + inst_count;
   if (!total) return;
   ctxt = malloc(sizeof(Elm_External_Signals_Proxy_Context) * total);
   if (!ctxt) return;
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_FREE, _external_signal_proxy_free_cb, ctxt);

   for (; cls_count > 0; cls_count--,  cls_descs++, ctxt++)
     {
        const Evas_Smart_Cb_Description *d = *cls_descs;
        ctxt->emission = d->name;
        ctxt->source = part_name;
        ctxt->edje = edje;
        evas_object_smart_callback_add
           (obj, d->name, _external_signal_proxy_cb, ctxt);
     }

   for (; inst_count > 0; inst_count--,  inst_descs++, ctxt++)
     {
        const Evas_Smart_Cb_Description *d = *inst_descs;
        ctxt->emission = d->name;
        ctxt->source = part_name;
        ctxt->edje = edje;
        evas_object_smart_callback_add
           (obj, d->name, _external_signal_proxy_cb, ctxt);
     }
}

/**
 * @internal
 * @brief Parses a list of Edje_External_Param and populates an Elm_Params structure.
 *
 * This function iterates through a list of Edje external parameters (typically
 * provided during the creation or update of an Edje external object) and
 * extracts common Elementary parameters like "style" and "disabled".
 * The extracted values are stored in the `Elm_Params` structure pointed to by `mem`.
 *
 * @param mem Pointer to an Elm_Params structure to be filled. This memory
 *            is expected to be managed by the caller (e.g., part of a larger
 *            widget-specific parameter structure).
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param params A list (Eina_List) of Edje_External_Param structures to parse.
 *               Each element in the list is an `Edje_External_Param *`.
 *               Example `params` list structure:
 *               - Element 1: Edje_External_Param { name="style", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="custom" }
 *               - Element 2: Edje_External_Param { name="disabled", type=EDJE_EXTERNAL_PARAM_TYPE_BOOL, i=EINA_TRUE }
 */
void
external_common_params_parse(void *mem, void *data EINA_UNUSED,
                             Evas_Object *obj EINA_UNUSED,
                             const Eina_List *params)
{
   Elm_Params *p;
   const Eina_List *l;
   Edje_External_Param *param;

   p = mem;
   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "style"))
          p->style = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "disabled"))
          {
             p->disabled = param->i;
             p->disabled_exists = EINA_TRUE;
          }
     }
}

/**
 * @internal
 * @brief Applies common Elementary parameters from an Elm_Params structure to an object.
 *
 * This function is typically used in the state_set callback of an Edje external
 * type. It takes an `Elm_Params` structure (which might have been populated by
 * external_common_params_parse) and applies the "style" and "disabled"
 * properties to the given `obj`.
 *
 * If `to_params` is provided, it is used. Otherwise, if `from_params` is
 * provided, it is used. If neither is provided, the function does nothing.
 * The `pos` parameter (position for animation) is currently unused for these
 * common parameters.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (typically an Elementary widget) to which the
 *            parameters will be applied.
 * @param from_params Pointer to the source Elm_Params structure (e.g., for the
 *                    start state of an animation).
 * @param to_params Pointer to the target Elm_Params structure (e.g., for the
 *                  end state of an animation, or the current state if not animating).
 * @param pos Unused animation position (0.0 to 1.0).
 */
void
external_common_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                          const void *from_params, const void *to_params,
                          float pos EINA_UNUSED)
{
   const Elm_Params *p;
   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->style)
      elm_object_style_set(obj, p->style);
   if (p->disabled_exists)
     elm_object_disabled_set(obj, p->disabled);
}

/**
 * @internal
 * @brief Creates an icon object based on an Edje external parameter.
 *
 * This function attempts to create an Elementary icon (elm_icon) based on
 * the string value provided in an Edje_External_Param. It first tries to
 * load the icon from the Edje file associated with `obj`'s smart parent.
 * If that fails, it tries to set a standard icon.
 *
 * The parent for the new icon is determined by `elm_widget_parent_widget_get(obj)`.
 * If no widget parent exists, the Edje object itself is used as a fallback parent
 * (though this might not always be appropriate for elm_icon_add).
 *
 * @param obj The Evas_Object for which the icon is being created. This object's
 *            smart parent is assumed to be an Edje object, and its widget parent
 *            is used for the new icon.
 * @param p An Edje_External_Param of type EDJE_EXTERNAL_PARAM_TYPE_STRING.
 *          The `p->s` string contains the name of the icon to load.
 *          Example: p = { name="icon_name", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="my_icon_group" }
 *          or p = { name="icon_name", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="home" } (for standard icon)
 * @return A new Evas_Object (elm_icon) on success, or NULL on failure.
 *         The caller is responsible for managing the returned object's lifecycle
 *         (e.g., deleting it if it's no longer needed or if an error occurs later).
 */
Evas_Object *
external_common_param_icon_get(Evas_Object *obj, const Edje_External_Param *p)
{
   Evas_Object *edje, *parent_widget, *icon;
   const char *file;

   if ((!p) || (!p->s) || (p->type != EDJE_EXTERNAL_PARAM_TYPE_STRING))
       return NULL;

   edje = evas_object_smart_parent_get(obj);
   edje_object_file_get(edje, &file, NULL);

   parent_widget = elm_widget_parent_widget_get(obj);
   if (!parent_widget)
     parent_widget = edje;
   icon = elm_icon_add(parent_widget);

   if ((edje_file_group_exists(file, p->s)) &&
       (elm_image_file_set(icon, file, p->s)))
     return icon;
   if (elm_icon_standard_set(icon, p->s))
     return icon;

   ERR("Failed to set icon: '%s'", p->s);
   evas_object_del(icon);
   return NULL;
}

/**
 * @internal
 * @brief Parses an "icon" parameter from a list and creates an icon object.
 *
 * This function searches for an Edje_External_Param named "icon" within the
 * provided list `params`. If found, it calls external_common_param_icon_get()
 * to create the icon object and stores the result in the location pointed to by `icon`.
 *
 * @param icon Pointer to an Evas_Object* where the created icon object will be stored.
 *             The caller should initialize `*icon` (e.g., to NULL).
 * @param obj The Evas_Object context, passed to external_common_param_icon_get().
 * @param params A list (Eina_List) of Edje_External_Param structures to search.
 *               Example `params` list structure:
 *               - Element 1: Edje_External_Param { name="icon", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="my_icon_name" }
 *               - ... other parameters ...
 */
void
external_common_icon_param_parse(Evas_Object **icon, Evas_Object *obj,
                                 const Eina_List *params)
{
   Edje_External_Param *p = edje_external_param_find(params, "icon");
   *icon = external_common_param_icon_get(obj, p);
}

/**
 * @internal
 * @brief Creates an Elementary layout or Edje object based on an Edje external parameter.
 *
 * This function attempts to create an elm_layout or an edje_object based on the
 * string value provided in an Edje_External_Param. The string `p->s` is
 * interpreted as a group name within an Edje file. The Edje file path is
 * obtained from the smart parent of `obj`.
 *
 * If `obj` has an Elementary widget parent, an `elm_layout` is created and
 * `elm_layout_file_set()` is used. Otherwise, a raw `edje_object` is created
 * and `edje_object_file_set()` is used.
 *
 * @param obj The Evas_Object for which the layout/Edje object is being created.
 *            Its smart parent is assumed to be an Edje object (to get the file path),
 *            and its widget parent determines whether to create an elm_layout or edje_object.
 * @param p An Edje_External_Param of type EDJE_EXTERNAL_PARAM_TYPE_STRING.
 *          The `p->s` string contains the name of the Edje group to load.
 *          Example: p = { name="layout_group", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="my_custom_layout_group" }
 * @return A new Evas_Object (elm_layout or edje_object) on success, or NULL on failure.
 *         The caller is responsible for managing the returned object's lifecycle.
 */
Evas_Object *
external_common_param_elm_layout_get(Evas_Object *obj,
                                     const Edje_External_Param *p)
{
   Evas_Object *edje, *parent_widget, *ret;
   const char *file;

   if ((!p) || (!p->s) || (p->type != EDJE_EXTERNAL_PARAM_TYPE_STRING))
       return NULL;

   edje = evas_object_smart_parent_get(obj);
   edje_object_file_get(edje, &file, NULL);

   parent_widget = elm_widget_parent_widget_get(obj);
   if (parent_widget)
     {
        ret = elm_layout_add(parent_widget);
        if (elm_layout_file_set(ret, file, p->s))
          return ret;
     }
   else
     {
        ret = edje_object_add(evas_object_evas_get(edje));
        if (edje_object_file_set(ret, file, p->s))
          return ret;
     }
   evas_object_del(ret);
   return NULL;
}

/**
 * @internal
 * @brief Frees resources associated with common Elementary parameters.
 *
 * This function is responsible for releasing any resources allocated by
 * external_common_params_parse(), specifically the stringshared `style`
 * member in the Elm_Params structure.
 *
 * It should be called when the `Elm_Params` structure (or the larger
 * widget-specific parameter structure containing it) is being freed.
 *
 * @param params A pointer to the Elm_Params structure whose resources are to be freed.
 */
void
external_common_params_free(void *params)
{
   Elm_Params *p = params;
   if (p->style)
     eina_stringshare_del(p->style);
}

#define DEFINE_TYPE(type_name) \
  extern const Edje_External_Type external_##type_name##_type;
#include "modules.inc"
#undef DEFINE_TYPE

/**
 * @internal
 * @brief Array of Edje_External_Type_Info structures for Elementary external types.
 *
 * This array is populated using macros and `modules.inc`. Each entry maps
 * an external type name (e.g., "elm/button") to its corresponding
 * Edje_External_Type implementation (e.g., `external_button_type`).
 * This array is registered with Edje during module initialization.
 *
 * The structure of each element is:
 * @code
 * {
 *   "elm/type_name_from_modules_inc", // Name registered with Edje
 *   &external_type_name_from_modules_inc_type // Pointer to the Edje_External_Type struct
 * }
 * @endcode
 * For example, if `modules.inc` contains `DEFINE_TYPE(button)`, this will generate:
 * @code
 * { "elm/button", &external_button_type },
 * @endcode
 */
static Edje_External_Type_Info elm_external_types[] =
{
#define DEFINE_TYPE(type_name)              \
  { "elm/"#type_name, &external_##type_name##_type },
#include "modules.inc"
#undef DEFINE_TYPE
   { NULL, NULL } /* Sentinel to mark the end of the array. */
};

/**
 * @internal
 * @brief Initializes the Elementary external types module.
 *
 * This function is called when the EINA_MODULE is loaded. It performs two main tasks:
 * 1. Registers a log domain named "elm-externals" for logging messages
 *    related to Elementary's Edje external types.
 * 2. Registers all defined Elementary external types (from `elm_external_types` array)
 *    with the Edje library using `edje_external_type_array_register`.
 *
 * @return EINA_TRUE on successful initialization, EINA_FALSE otherwise.
 */
static Eina_Bool
elm_mod_init(void)
{
   _elm_ext_log_dom = eina_log_domain_register("elm-externals",
                                               EINA_COLOR_LIGHTBLUE);
   edje_external_type_array_register(elm_external_types);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Shuts down the Elementary external types module.
 *
 * This function is called when the EINA_MODULE is unloaded. It performs two main tasks:
 * 1. Unregisters all Elementary external types (from `elm_external_types` array)
 *    from the Edje library using `edje_external_type_array_unregister`.
 * 2. Unregisters the "elm-externals" log domain if it was previously registered.
 */
static void
elm_mod_shutdown(void)
{
   edje_external_type_array_unregister(elm_external_types);
   if (_elm_ext_log_dom >= 0) eina_log_domain_unregister(_elm_ext_log_dom);
   _elm_ext_log_dom = -1;
}

EINA_MODULE_INIT(elm_mod_init);
EINA_MODULE_SHUTDOWN(elm_mod_shutdown);
