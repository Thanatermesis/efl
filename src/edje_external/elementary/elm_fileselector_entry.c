#include "private.h"

/**
 * @brief Structure to hold parameters for the fileselector entry widget.
 *
 * This structure is used to pass parameters when creating or updating
 * a fileselector entry widget through the Edje external interface.
 */
typedef struct _Elm_Params_fileselector_entry
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   const char *label; /**< The text label to display on the entry. */
   Evas_Object *icon; /**< An icon to display on the entry's button. */

   /**
    * @brief Fileselector specific parameters.
    */
   struct {
      const char *path; /**< The initial path for the fileselector. */
      Eina_Bool is_save:1; /**< EINA_TRUE if the fileselector is in save mode, EINA_FALSE otherwise. */
      Eina_Bool is_save_set:1; /**< EINA_TRUE if is_save has been set. */
      Eina_Bool folder_only:1; /**< EINA_TRUE to select folders only, EINA_FALSE otherwise. */
      Eina_Bool folder_only_set:1; /**< EINA_TRUE if folder_only has been set. */
      Eina_Bool expandable:1; /**< EINA_TRUE if the fileselector path is expandable, EINA_FALSE otherwise. */
      Eina_Bool expandable_set:1; /**< EINA_TRUE if expandable has been set. */
      Eina_Bool inwin_mode:1; /**< EINA_TRUE to use inwin mode, EINA_FALSE otherwise. */
      Eina_Bool inwin_mode_set:1; /**< EINA_TRUE if inwin_mode has been set. */
   } fs; /**< Fileselector specific settings. */
} Elm_Params_fileselector_entry;

/**
 * @brief Sets the state of the fileselector entry based on parameters.
 *
 * This function is called by Edje to apply state parameters to the
 * fileselector entry widget. It handles transitions between states.
 *
 * @param data Unused.
 * @param obj The fileselector entry widget.
 * @param from_params The parameters of the previous state (can be NULL).
 * @param to_params The parameters of the new state (can be NULL).
 * @param pos The position in the transition (unused).
 */
static void
external_fileselector_entry_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                                      const void *from_params,
                                      const void *to_params,
                                      float pos EINA_UNUSED)
{
   const Elm_Params_fileselector_entry *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label)
     elm_object_text_set(obj, p->label);
   if (p->icon) elm_object_part_content_set(obj, "button icon", p->icon);
   if (p->fs.path) elm_fileselector_selected_set(obj, p->fs.path);
   if (p->fs.is_save_set)
     elm_fileselector_is_save_set(obj, p->fs.is_save);
   if (p->fs.folder_only_set)
     elm_fileselector_folder_only_set(obj, p->fs.folder_only);
   if (p->fs.expandable_set)
     elm_fileselector_expandable_set(obj, p->fs.expandable);
   if (p->fs.inwin_mode_set)
     elm_fileselector_entry_inwin_mode_set(obj, p->fs.inwin_mode);
}

/**
 * @brief Sets a specific parameter for the fileselector entry.
 *
 * This function is called by Edje to set individual parameters on the
 * fileselector entry widget.
 *
 * @param data Unused.
 * @param obj The fileselector entry widget.
 * @param param The parameter to set.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_fileselector_entry_param_set(void *data EINA_UNUSED, Evas_Object *obj,
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
             elm_object_part_content_set(obj, "button icon", icon);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "path"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_fileselector_selected_set(obj, param->s);
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
             elm_fileselector_entry_inwin_mode_set(obj, param->i);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific parameter from the fileselector entry.
 *
 * This function is called by Edje to retrieve individual parameters from the
 * fileselector entry widget.
 *
 * @param data Unused.
 * @param obj The fileselector entry widget.
 * @param param The parameter to retrieve. Its value will be filled in.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_fileselector_entry_param_get(void *data EINA_UNUSED,
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
             param->s = elm_fileselector_selected_get(obj);
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
             param->i = elm_fileselector_entry_inwin_mode_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters and creates an
 *        Elm_Params_fileselector_entry structure.
 *
 * This function converts a list of Edje parameters into a structured
 * format that can be used to initialize or update a fileselector entry.
 *
 * @param data Unused.
 * @param obj The fileselector entry widget (used for icon parsing context).
 * @param params A list of Edje_External_Param to parse.
 * @return A newly allocated Elm_Params_fileselector_entry structure, or NULL on failure.
 *         The caller is responsible for freeing the returned structure using
 *         external_fileselector_entry_params_free().
 */
static void *
external_fileselector_entry_params_parse(void *data EINA_UNUSED,
                                         Evas_Object *obj,
                                         const Eina_List *params)
{
   Elm_Params_fileselector_entry *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_fileselector_entry));
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
 * @brief Retrieves content from the fileselector entry.
 *
 * This function is part of the Edje external interface but is not
 * implemented for fileselector entry as it does not support named content parts
 * in the same way other widgets might.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param content Unused.
 * @return Always NULL, as no content is supported.
 */
static Evas_Object *external_fileselector_entry_content_get(void *data EINA_UNUSED,
                                                            const Evas_Object *obj EINA_UNUSED,
                                                            const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_fileselector_entry.
 *
 * This function should be called to release the resources allocated by
 * external_fileselector_entry_params_parse().
 *
 * @param params The Elm_Params_fileselector_entry structure to free.
 */
static void
external_fileselector_entry_params_free(void *params)
{
   Elm_Params_fileselector_entry *mem = params;

   if (mem->fs.path)
     eina_stringshare_del(mem->fs.path);
   if (mem->label)
      eina_stringshare_del(mem->label);
   free(params);
}

/**
 * @brief Describes the Edje external parameters supported by the fileselector entry.
 *
 * This array provides metadata about the parameters that can be used
 * to configure a fileselector entry widget from an Edje theme.
 */
static Edje_External_Param_Info external_fileselector_entry_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "disabled", "visible". */
   EDJE_EXTERNAL_PARAM_INFO_STRING("label"), /**< Parameter for setting the entry's label text. Example: "label: 'Choose a file';" */
   EDJE_EXTERNAL_PARAM_INFO_STRING("icon"), /**< Parameter for setting the entry's icon. Example: "icon: 'document-open';" */
   EDJE_EXTERNAL_PARAM_INFO_STRING("path"), /**< Parameter for setting the initial path. Example: "path: '/home/user/Documents';" */
   EDJE_EXTERNAL_PARAM_INFO_BOOL("save"), /**< Parameter for setting save mode. Example: "save: true;" */
   EDJE_EXTERNAL_PARAM_INFO_BOOL("folder only"), /**< Parameter for setting folder-only selection mode. Example: "folder only: true;" */
   EDJE_EXTERNAL_PARAM_INFO_BOOL("expandable"), /**< Parameter for setting path expandability. Example: "expandable: false;" */
   EDJE_EXTERNAL_PARAM_INFO_BOOL("inwin mode"), /**< Parameter for setting in-window mode. Example: "inwin mode: true;" */
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL /**< Marks the end of the parameter list. */
};

DEFINE_EXTERNAL_ICON_ADD(fileselector_entry, "fileselector_entry");
DEFINE_EXTERNAL_TYPE_SIMPLE(fileselector_entry, "File Selector Entry");
