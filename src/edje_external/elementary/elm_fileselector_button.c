#include "private.h"

/**
 * @brief Structure to hold the parameters for an Elm_Fileselector_Button widget.
 *
 * This structure is used to pass parameters when creating or configuring
 * an Elm_Fileselector_Button widget through the Edje external interface.
 */
typedef struct _Elm_Params_fileselector_button
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   const char *label; /**< The text label to display on the button. */
   Evas_Object *icon; /**< The icon object to display on the button. */

   /**
    * @brief Fileselector specific parameters.
    */
   struct {
      const char *path; /**< The initial path for the fileselector. */
      Eina_Bool is_save:1; /**< If true, the fileselector is in "save" mode. */
      Eina_Bool is_save_set:1; /**< Flag indicating if is_save has been set. */
      Eina_Bool folder_only:1; /**< If true, only folders can be selected. */
      Eina_Bool folder_only_set:1; /**< Flag indicating if folder_only has been set. */
      Eina_Bool expandable:1; /**< If true, the fileselector path is expandable. */
      Eina_Bool expandable_set:1; /**< Flag indicating if expandable has been set. */
      Eina_Bool inwin_mode:1; /**< If true, the fileselector opens in an in-win (internal window). */
      Eina_Bool inwin_mode_set:1; /**< Flag indicating if inwin_mode has been set. */
   } fs; /**< Fileselector specific settings. */
} Elm_Params_fileselector_button;

/**
 * @brief Sets the state of the fileselector button widget.
 *
 * This function is called to apply parameters to the fileselector button,
 * typically during widget creation or state transitions.
 *
 * @param data Unused.
 * @param obj The fileselector button Evas_Object to modify.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The new state parameters to apply.
 * @param pos Unused.
 */
static void
external_fileselector_button_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                                       const void *from_params,
                                       const void *to_params,
                                       float pos EINA_UNUSED)
{
   const Elm_Params_fileselector_button *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label)
     elm_object_text_set(obj, p->label);
   if (p->icon) elm_object_part_content_set(obj, "icon", p->icon);
   if (p->fs.path) elm_fileselector_path_set(obj, p->fs.path);
   if (p->fs.is_save_set)
     elm_fileselector_is_save_set(obj, p->fs.is_save);
   if (p->fs.folder_only_set)
     elm_fileselector_folder_only_set(obj, p->fs.folder_only);
   if (p->fs.expandable_set)
     elm_fileselector_expandable_set(obj, p->fs.expandable);
   if (p->fs.inwin_mode_set)
     elm_fileselector_button_inwin_mode_set(obj, p->fs.inwin_mode);
}

/**
 * @brief Sets a specific external parameter for the fileselector button widget.
 *
 * This function is called by Edje to set individual parameters on the
 * fileselector button widget based on EDC (Edje Data Collection) definitions.
 *
 * @param data Unused.
 * @param obj The fileselector button Evas_Object to modify.
 * @param param The Edje_External_Param to set.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_fileselector_button_param_set(void *data EINA_UNUSED, Evas_Object *obj,
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
   else if (!strcmp(param->name, "path"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_fileselector_path_set(obj, param->s);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "save"))
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
   else if (!strcmp(param->name, "expandable"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_fileselector_expandable_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "inwin mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_fileselector_button_inwin_mode_set(obj, param->i);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific external parameter from the fileselector button widget.
 *
 * This function is called by Edje to retrieve individual parameters from the
 * fileselector button widget.
 *
 * @param data Unused.
 * @param obj The fileselector button Evas_Object to query.
 * @param param The Edje_External_Param to fill with the retrieved value.
 *              The `name` field of this struct indicates which parameter to get.
 *              The `type` field indicates the expected type.
 *              The value will be stored in the appropriate union member (e.g., `s` for string, `i` for int/bool).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_fileselector_button_param_get(void *data EINA_UNUSED,
                                       const Evas_Object *obj,
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
   else if (!strcmp(param->name, "path"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_fileselector_path_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "save"))
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
   else if (!strcmp(param->name, "expandable"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_fileselector_expandable_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "inwin mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_fileselector_button_inwin_mode_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters and creates an
 *        Elm_Params_fileselector_button structure.
 *
 * This function is used to convert a list of parameters from Edje's
 * external interface into a structured format that can be used to
 * initialize or configure a fileselector button.
 *
 * @param data Unused.
 * @param obj The Evas_Object this parsing is for (used for icon parsing).
 * @param params A list of Edje_External_Param structures to parse.
 *               Example of `params` list elements:
 *               - Edje_External_Param { name="label", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="Open File" }
 *               - Edje_External_Param { name="path", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="/home/user/" }
 *               - Edje_External_Param { name="save", type=EDJE_EXTERNAL_PARAM_TYPE_BOOL, i=1 }
 * @return A pointer to a newly allocated Elm_Params_fileselector_button structure
 *         filled with the parsed parameters, or NULL on allocation failure.
 *         The caller is responsible for freeing this structure using
 *         external_fileselector_button_params_free().
 */
static void *
external_fileselector_button_params_parse(void *data EINA_UNUSED,
                                          Evas_Object *obj,
                                          const Eina_List *params)
{
   Elm_Params_fileselector_button *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_fileselector_button));
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "path"))
          mem->fs.path = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "save"))
          {
             mem->fs.is_save = !!param->i;
             mem->fs.is_save_set = EINA_TRUE;
          }
        else if (!strcmp(param->name, "folder only"))
          {
             mem->fs.folder_only = !!param->i;
             mem->fs.folder_only_set = EINA_TRUE;
          }
        else if (!strcmp(param->name, "expandable"))
          {
             mem->fs.expandable = !!param->i;
             mem->fs.expandable_set = EINA_TRUE;
          }
        else if (!strcmp(param->name, "inwin mode"))
          {
             mem->fs.inwin_mode = !!param->i;
             mem->fs.inwin_mode_set = EINA_TRUE;
          }
        else if (!strcmp(param->name, "label"))
          mem->label = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Retrieves content from the fileselector button.
 *
 * This function is part of the Edje external interface. For fileselector buttons,
 * it currently does not support named content parts and will always return NULL.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param content Unused.
 * @return Always NULL for fileselector_button.
 */
static Evas_Object *external_fileselector_button_content_get(void *data EINA_UNUSED,
                                                             const Evas_Object *obj EINA_UNUSED,
                                                             const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_fileselector_button.
 *
 * This function should be called to release the resources associated with
 * an Elm_Params_fileselector_button structure that was created by
 * external_fileselector_button_params_parse().
 *
 * @param params A pointer to the Elm_Params_fileselector_button structure to free.
 */
 static void
external_fileselector_button_params_free(void *params)
{
   Elm_Params_fileselector_button *mem = params;

   if (mem->fs.path)
     eina_stringshare_del(mem->fs.path);
   if (mem->label)
      eina_stringshare_del(mem->label);
   free(params);
}

/**
 * @brief Defines the external parameters available for the fileselector button widget.
 *
 * This array provides metadata about the parameters that can be set or retrieved
 * for a fileselector button via the Edje external interface. It includes the
 * parameter name and its type.
 */
static Edje_External_Param_Info external_fileselector_button_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "disabled", "visible". */
   EDJE_EXTERNAL_PARAM_INFO_STRING("label"), /**< The text label of the button. */
   EDJE_EXTERNAL_PARAM_INFO_STRING("icon"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("path"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("save"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("folder only"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("expandable"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("inwin mode"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(fileselector_button, "fileselector_button");
DEFINE_EXTERNAL_TYPE_SIMPLE(fileselector_button, "File Selector Button");
