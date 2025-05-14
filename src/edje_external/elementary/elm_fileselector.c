#include <assert.h>
#include "private.h"

/**
 * @brief Structure to hold the parameters for the fileselector widget.
 * This structure is used to store the state of the fileselector widget,
 * particularly when it's being created or modified through external parameters.
 */
typedef struct _Elm_Params_Fileselector
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   Eina_Bool is_save:1; /**< If true, the fileselector is in 'save' mode. */
   Eina_Bool is_save_set:1; /**< Tracks if the is_save parameter has been set. */
   Eina_Bool folder_only:1; /**< If true, only folders can be selected. */
   Eina_Bool folder_only_set:1; /**< Tracks if the folder_only parameter has been set. */
   Eina_Bool show_buttons:1; /**< If true, 'OK' and 'Cancel' buttons are shown. */
   Eina_Bool show_buttons_set:1; /**< Tracks if the show_buttons parameter has been set. */
   Eina_Bool expandable:1; /**< If true, the fileselector view is expandable. */
   Eina_Bool expandable_set:1; /**< Tracks if the expandable parameter has been set. */
} Elm_Params_Fileselector;

/**
 * @brief Sets the state of the fileselector widget based on parameters.
 *
 * This function is called to apply a new state to the fileselector widget,
 * typically during transitions or initial setup from external parameters.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (fileselector widget) to modify.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The new state parameters to apply (can be NULL).
 * @param pos Unused position value for transitions.
 */
static void
external_fileselector_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                                const void *from_params, const void *to_params,
                                float pos EINA_UNUSED)
{
   const Elm_Params_Fileselector *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if ((p->is_save_set) && (p->is_save))
     elm_fileselector_is_save_set(obj, p->is_save);
   if (p->folder_only_set)
     elm_fileselector_folder_only_set(obj, p->folder_only);
   if (p->show_buttons_set)
     elm_fileselector_buttons_ok_cancel_set(obj, p->show_buttons);
   if (p->expandable_set)
     elm_fileselector_expandable_set(obj, p->expandable);
}

/**
 * @brief Sets a specific external parameter on the fileselector widget.
 *
 * This function is called by the Edje external interface to set individual
 * properties of the fileselector widget.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (fileselector widget) to modify.
 * @param param The Edje_External_Param to apply.
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_fileselector_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                                const Edje_External_Param *param)
{
   if (!strcmp(param->name, "save"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_fileselector_is_save_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "folder only"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_fileselector_folder_only_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "show buttons"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_fileselector_buttons_ok_cancel_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "expandable"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_fileselector_expandable_set(obj, param->i);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific external parameter from the fileselector widget.
 *
 * This function is called by the Edje external interface to retrieve individual
 * properties of the fileselector widget.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (fileselector widget) to query.
 * @param param The Edje_External_Param to fill with the retrieved value.
 *              The `name` field indicates which parameter to get.
 *              The `type` field indicates the expected type.
 *              The value (e.g., `i` for int/bool) will be set if successful.
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_fileselector_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                                Edje_External_Param *param)
{
   if (!strcmp(param->name, "save"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_fileselector_is_save_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "folder only"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_fileselector_folder_only_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "show buttons"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_fileselector_buttons_ok_cancel_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "expandable"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_fileselector_expandable_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters and creates an Elm_Params_Fileselector structure.
 *
 * This function converts a list of Edje parameters into a structured
 * Elm_Params_Fileselector object, which can then be used to set the state
 * of a fileselector widget.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param params A list of Edje_External_Param objects to parse.
 *               Example of params list structure:
 *               - param1: name="save", type=EDJE_EXTERNAL_PARAM_TYPE_BOOL, i=1
 *               - param2: name="folder only", type=EDJE_EXTERNAL_PARAM_TYPE_BOOL, i=0
 * @return A newly allocated Elm_Params_Fileselector structure filled with parsed values,
 *         or NULL on failure. The caller is responsible for freeing this structure
 *         using external_fileselector_params_free().
 */
static void *
external_fileselector_params_parse(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const Eina_List *params)
{
   Elm_Params_Fileselector *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Fileselector));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "save"))
          {
             mem->is_save = !!param->i;
             mem->is_save_set = EINA_TRUE;
          }
        else if (!strcmp(param->name, "folder only"))
          {
             mem->folder_only = !!param->i;
             mem->folder_only_set = EINA_TRUE;
          }
        else if (!strcmp(param->name, "show buttons"))
          {
             mem->show_buttons = !!param->i;
             mem->show_buttons_set = EINA_TRUE;
          }
        else if (!strcmp(param->name, "expandable"))
          {
             mem->expandable = !!param->i;
             mem->expandable_set = EINA_TRUE;
          }
     }

   return mem;
}

/**
 * @brief Retrieves content from the fileselector widget.
 *
 * This function is part of the Edje external interface but is not
 * implemented for the fileselector widget as it does not provide named content parts.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param content Unused content name.
 * @return Always NULL for fileselector.
 */
static Evas_Object *external_fileselector_content_get(void *data EINA_UNUSED,
                                                      const Evas_Object *obj EINA_UNUSED,
                                                      const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Fileselector.
 *
 * @param params A pointer to the Elm_Params_Fileselector structure to be freed.
 */
static void
external_fileselector_params_free(void *params)
{
   Elm_Params_Fileselector *mem = params;
   free(mem);
}

/**
 * @brief Defines the external parameters available for the fileselector widget.
 *
 * This array provides metadata about the parameters that can be set or retrieved
 * for the fileselector widget via the Edje external interface.
 */
static Edje_External_Param_Info external_fileselector_params[] =
{
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_BOOL("save"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("folder only"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("show buttons"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("expandable"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(fileselector, "fileselector");
DEFINE_EXTERNAL_TYPE_SIMPLE(fileselector, "Fileselector");
