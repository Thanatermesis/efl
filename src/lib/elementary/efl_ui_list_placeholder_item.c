#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_LIST_PLACEHOLDER_ITEM_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_part_helper.h"

#define MY_CLASS      EFL_UI_LIST_PLACEHOLDER_ITEM_CLASS
#define MY_CLASS_PFX  efl_ui_list_placeholder_item

#define MY_CLASS_NAME "Efl.Ui.List_Placeholder_Item"

/**
 * @internal
 * @brief Finalizes the Efl.Ui.List_Placeholder_Item object.
 *
 * This function is called when the object is being finalized. It applies
 * the "placeholder" style from the "list_item" group of the theme.
 *
 * @param obj The Efl.Ui.List_Placeholder_Item object.
 * @param pd Private data for the Efl.Ui.List_Placeholder_Item class (unused).
 * @return The finalized Efl_Object, or @c NULL on error.
 */
EOLIAN static Efl_Object *
_efl_ui_list_placeholder_item_efl_object_finalize(Eo *obj, void *pd EINA_UNUSED)
{
   Eo *eo;
   eo = efl_finalize(efl_super(obj, MY_CLASS));
   ELM_WIDGET_DATA_GET_OR_RETURN(eo, wd, eo);
   Eina_Error theme_apply_ret = efl_ui_layout_theme_set(obj, "list_item", NULL, "placeholder");

   if (theme_apply_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     CRI("Empty Item(%p) failed to set theme [efl/list_item:placeholder]!", eo);
   return eo;
}

/**
 * @internal
 * @brief Destroys the Efl.Ui.List_Placeholder_Item object.
 *
 * This function is called when the object is being destroyed. It calls the
 * destructor of the parent class.
 *
 * @param obj The Efl.Ui.List_Placeholder_Item object.
 * @param pd Private data for the Efl.Ui.List_Placeholder_Item class (unused).
 */
EOLIAN static void
_efl_ui_list_placeholder_item_efl_object_destructor(Eo *obj, void *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));
}

/* Efl.Part */
/**
 * @internal
 * @brief Implements the default content get functionality for the "efl.content" part.
 * This macro generates a function that retrieves the content of the "efl.content" part.
 */
ELM_PART_CONTENT_DEFAULT_GET(efl_ui_list_placeholder_item, "efl.content")
/**
 * @internal
 * @brief Implements the default content set/unset functionality for parts.
 * This macro generates functions for setting and unsetting content for parts.
 * The 'void' parameter indicates that there's no specific data type associated
 * with the content part for this implementation.
 */
ELM_PART_CONTENT_DEFAULT_IMPLEMENT(efl_ui_list_placeholder_item, void)
/* Efl.Part end */

/**
 * @internal
 * @brief Gets a specific part of the Efl.Ui.List_Placeholder_Item object.
 *
 * This function is called to retrieve a part of the object, such as "efl.content".
 *
 * @param obj The Efl.Ui.List_Placeholder_Item object.
 * @param wd Private data for the Efl.Ui.List_Placeholder_Item class (unused).
 * @param part The name of the part to retrieve.
 *        Example: "efl.content"
 * @return The Efl_Object representing the part, or @c NULL if the part is not found or an error occurs.
 */
EOLIAN static Efl_Object *
_efl_ui_list_placeholder_item_efl_part_part_get(const Eo *obj, void *wd EINA_UNUSED, const char *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   if (eina_streq(part, "content"))
     return ELM_PART_IMPLEMENT(EFL_UI_LAYOUT_PART_CONTENT_CLASS, obj, "efl.content");

   return efl_part_get(efl_super(obj, MY_CLASS), part);
}

/* Internal EO APIs and hidden overrides */

/**
 * @internal
 * @brief Defines extra Eolian operations for Efl.Ui.List_Placeholder_Item.
 *
 * This macro includes default operations for content parts, likely related to
 * setting, getting, and unsetting content.
 */
#define EFL_UI_LIST_PLACEHOLDER_ITEM_EXTRA_OPS \
  ELM_PART_CONTENT_DEFAULT_OPS(efl_ui_list_placeholder_item)

#include "efl_ui_list_placeholder_item.eo.c"
