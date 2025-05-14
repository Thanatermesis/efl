#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_PART_PROTECTED

#include <Efl_Ui.h>
#include "elm_priv.h"
#include "efl_ui_tab_page_private.h"
#include "elm_part_helper.h"

#define MY_CLASS EFL_UI_TAB_PAGE_CLASS

/**
 * @brief Callback function invoked when the content of the tab page is invalidated.
 *
 * This function is registered with the EFL_EVENT_INVALIDATE event on the content object.
 * When the content object is invalidated (e.g., during deletion), this callback
 * unsets the content from its container (the tab page itself in this context,
 * passed as @p data).
 *
 * @param data The tab page object (Eo *) that holds the content.
 * @param ev The Efl_Event structure associated with the event (unused).
 */
static void
_invalidate_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   efl_content_unset(data);
}

/**
 * @brief Sets the content of the tab page.
 *
 * This function implements the Efl.Content.content_set Eolian interface.
 * It handles unsetting previous content, managing event callbacks for invalidation,
 * and setting the new content as a sub-object and as the main content part.
 *
 * @param obj The tab page object.
 * @param sd Private data for the tab page.
 * @param content The new content object to set. Can be NULL to unset content.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., if sub_object_add fails).
 */
EOLIAN static Eina_Bool
_efl_ui_tab_page_efl_content_content_set(Eo *obj, Efl_Ui_Tab_Page_Data *sd, Eo *content)
{
   if (sd->content)
     {
        efl_content_unset(efl_part(obj, "efl.content"));
        efl_event_callback_del(sd->content, EFL_EVENT_INVALIDATE, _invalidate_cb, obj);
        efl_del(sd->content);
        sd->content = NULL;
     }

   if (content && !efl_ui_widget_sub_object_add(obj, content))
     {
        efl_event_callback_call(obj, EFL_CONTENT_EVENT_CONTENT_CHANGED, NULL);
        return EINA_FALSE;
     }

   sd->content = content;
   efl_event_callback_add(sd->content, EFL_EVENT_INVALIDATE, _invalidate_cb, obj);
   efl_event_callback_call(obj, EFL_CONTENT_EVENT_CONTENT_CHANGED, content);

   efl_content_set(efl_part(obj, "efl.content"), sd->content);

   return EINA_TRUE;
}

/**
 * @brief Unsets the content of the tab page.
 *
 * This function implements the Efl.Content.content_unset Eolian interface.
 * It removes the invalidation callback, clears the internal content reference,
 * and unsets the content from the "efl.content" part.
 *
 * @param obj The tab page object.
 * @param pd Private data for the tab page.
 * @return The previously set content object, or NULL if none was set.
 */
EOLIAN static Efl_Gfx_Entity*
_efl_ui_tab_page_efl_content_content_unset(Eo *obj, Efl_Ui_Tab_Page_Data *pd)
{
   efl_event_callback_del(pd->content, EFL_EVENT_INVALIDATE, _invalidate_cb, obj);
   pd->content = NULL;
   efl_event_callback_call(obj, EFL_CONTENT_EVENT_CONTENT_CHANGED, NULL);
   return efl_content_unset(efl_part(obj, "efl.content"));
}


/**
 * @brief Gets the content of the tab page.
 *
 * This function implements the Efl.Content.content_get Eolian interface.
 *
 * @param obj The tab page object (unused).
 * @param sd Private data for the tab page.
 * @return The current content object, or NULL if none is set.
 */
EOLIAN static Eo *
_efl_ui_tab_page_efl_content_content_get(const Eo *obj EINA_UNUSED, Efl_Ui_Tab_Page_Data *sd)
{
   return sd->content;
}

/**
 * @brief Constructor for the Efl_Ui_Tab_Page object.
 *
 * Initializes the tab page, sets its theme, allows focus, and
 * initializes internal data members.
 *
 * @param obj The tab page object being constructed.
 * @param sd Private data for the tab page.
 * @return The constructed tab page object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_ui_tab_page_efl_object_constructor(Eo *obj, Efl_Ui_Tab_Page_Data *sd)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "tab_page");

   obj = efl_constructor(efl_super(obj, MY_CLASS));

   if (elm_widget_theme_object_set(obj, wd->resize_obj,
                                       elm_widget_theme_klass_get(obj),
                                       elm_widget_theme_element_get(obj),
                                       elm_widget_theme_style_get(obj)) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     CRI("Failed to set layout!");

   efl_ui_widget_focus_allow_set(obj, EINA_TRUE);

   sd->content = NULL;
   sd->tab_label = NULL;
   sd->tab_icon = NULL;

   return obj;
}

/**
 * @brief Destructor for the Efl_Ui_Tab_Page object.
 *
 * Performs cleanup by calling the superclass destructor.
 * Content and other resources are typically handled by their respective
 * unsetter methods or Efl.Object's ownership model.
 *
 * @param obj The tab page object being destructed.
 * @param sd Private data for the tab page (unused in this specific destructor,
 *           but present for consistency with Eolian function signature).
 */
EOLIAN static void
_efl_ui_tab_page_efl_object_destructor(Eo *obj, Efl_Ui_Tab_Page_Data *sd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));
}


/**
 * @brief Gets or creates the tab bar item associated with this tab page.
 *
 * This function is responsible for providing an Efl_Ui_Tab_Bar_Default_Item
 * that represents this tab page in a tab bar. If the item has not been
 * created yet, it will be instantiated, configured with the page's label
 * and icon, and then returned. Subsequent calls will return the same item instance.
 *
 * The `pd->tab_label` and `pd->tab_icon` are expected to be set on the tab page
 * (e.g., via Eolian properties) before this function is called for the first time
 * to ensure the tab bar item is correctly initialized.
 *
 * @param obj The tab page object.
 * @param pd Private data for the tab page, containing `tab_bar_icon`, `tab_label`, and `tab_icon`.
 * @return The Efl_Ui_Tab_Bar_Default_Item associated with this page.
 */
EOLIAN static Efl_Ui_Tab_Bar_Default_Item*
_efl_ui_tab_page_tab_bar_item_get(const Eo *obj, Efl_Ui_Tab_Page_Data *pd)
{
  if (!pd->tab_bar_icon)
    {
       pd->tab_bar_icon = efl_add(EFL_UI_TAB_BAR_DEFAULT_ITEM_CLASS, (Eo*)obj);
       efl_text_set(pd->tab_bar_icon, pd->tab_label);
       efl_ui_tab_bar_default_item_icon_set(pd->tab_bar_icon, pd->tab_icon);
    }

  return pd->tab_bar_icon;
}

/* Efl.Part end */

#include "efl_ui_tab_page.eo.c"
