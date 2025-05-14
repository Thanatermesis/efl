#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif


#include <Efl_Ui.h>
#include "elm_priv.h"

#include "efl_ui_tab_pager_private.h"
#include "efl_ui_tab_page_private.h"

#define MY_CLASS EFL_UI_TAB_PAGER_CLASS

/**
 * @internal
 * @brief Sets the spotlight manager for the tab pager.
 *
 * This function is a pass-through to the internal spotlight container's
 * spotlight manager.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param pd The private data of the Efl_Ui_Tab_Pager object.
 * @param manager The spotlight manager to set.
 */
static void
_efl_ui_tab_pager_spotlight_manager_set(Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *pd, Efl_Ui_Spotlight_Manager *manager)
{
   efl_ui_spotlight_manager_set(pd->spotlight, manager);
}

/**
 * @internal
 * @brief Callback invoked when a tab is selected in the tab bar.
 *
 * This function retrieves the page associated with the selected tab
 * and sets it as the active element in the spotlight container.
 *
 * @param data The Efl_Ui_Tab_Pager object (passed as user data).
 * @param event The Efl_Event details.
 */
static void
_tab_select_cb(void *data, const Efl_Event *event)
{
   Efl_Ui_Tab_Bar_Default_Item *selected;
   Efl_Ui_Tab_Page *page;
   Efl_Ui_Tab_Pager_Data *pd;

   pd = efl_data_scope_get(data, MY_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(pd);
   selected = efl_ui_selectable_last_selected_get(event->object);
   page = efl_parent_get(selected);
   if (efl_ui_spotlight_active_element_get(pd->spotlight))
     efl_ui_spotlight_active_element_set(pd->spotlight, page);
}

/**
 * @internal
 * @brief Gets the internal tab bar widget.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param sd The private data of the Efl_Ui_Tab_Pager object.
 * @return The internal Efl_Ui_Tab_Bar object.
 */
EOLIAN static Efl_Canvas_Object *
_efl_ui_tab_pager_tab_bar_get(const Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *sd)
{
   return sd->tab_bar;
}

/**
 * @internal
 * @brief Destructor for the Efl_Ui_Tab_Pager object.
 *
 * Cleans up resources, specifically removing the event callback
 * from the tab bar.
 *
 * @param obj The Efl_Ui_Tab_Pager object being destroyed.
 * @param sd The private data of the Efl_Ui_Tab_Pager object.
 */
EOLIAN static void
_efl_ui_tab_pager_efl_object_destructor(Eo *obj, Efl_Ui_Tab_Pager_Data *sd)
{
   if (sd->tab_bar != NULL)
     efl_event_callback_del(sd->tab_bar, EFL_UI_EVENT_ITEM_SELECTED, _tab_select_cb, obj);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Constructor for the Efl_Ui_Tab_Pager object.
 *
 * Initializes the tab pager, sets its theme class, creates the
 * internal tab bar and spotlight container, and sets up event handling.
 *
 * @param obj The Efl_Ui_Tab_Pager object being constructed.
 * @param sd The private data of the Efl_Ui_Tab_Pager object.
 * @return The constructed Efl_Ui_Tab_Pager object.
 */
EOLIAN static Efl_Object *
_efl_ui_tab_pager_efl_object_constructor(Eo *obj, Efl_Ui_Tab_Pager_Data *sd)
{
   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "tab_pager");

   obj = efl_constructor(efl_super(obj, MY_CLASS));

   efl_ui_widget_focus_allow_set(obj, EINA_TRUE);

   sd->tab_bar = efl_add(EFL_UI_TAB_BAR_CLASS, obj);
   efl_event_callback_add(sd->tab_bar, EFL_UI_EVENT_ITEM_SELECTED, _tab_select_cb, obj);
   efl_event_callback_forwarder_del(sd->tab_bar, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, obj);

   sd->spotlight = efl_add(EFL_UI_SPOTLIGHT_CONTAINER_CLASS, obj);

   return obj;
}

/**
 * @internal
 * @brief Applies the theme to the Efl_Ui_Tab_Pager widget.
 *
 * This function applies the theme to the base widget and then sets
 * the content of the "efl.tab_root" and "efl.page_root" parts
 * to the internal tab bar and spotlight container respectively.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param pd The private data of the Efl_Ui_Tab_Pager object.
 * @return EINA_ERROR_NONE on success, or an error code on failure.
 */
EOLIAN static Eina_Error
_efl_ui_tab_pager_efl_ui_widget_theme_apply(Eo *obj, Efl_Ui_Tab_Pager_Data *pd)
{
   Eina_Error err;

   err = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));

   efl_content_set(efl_part(obj, "efl.tab_root"), pd->tab_bar);
   efl_content_set(efl_part(obj, "efl.page_root"), pd->spotlight);

   return err;
}

/**
 * @internal
 * @brief Finalizes the Efl_Ui_Tab_Pager object.
 *
 * This function finalizes the base object and ensures the tab bar
 * is set as content for the "efl.tab_root" part. This might seem
 * redundant with theme_apply, but finalize is called after all
 * parts are potentially created by the theme.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param pd The private data of the Efl_Ui_Tab_Pager object.
 * @return The finalized Efl_Ui_Tab_Pager object.
 */
EOLIAN static Efl_Object*
_efl_ui_tab_pager_efl_object_finalize(Eo *obj, Efl_Ui_Tab_Pager_Data *pd)
{
   obj = efl_finalize(efl_super(obj, MY_CLASS));

   efl_content_set(efl_part(obj, "efl.tab_root"), pd->tab_bar);

   return obj;
}

/**
 * @internal
 * @brief Gets the number of packed items (pages) in the tab pager.
 *
 * This delegates to the content count of the internal spotlight container.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param pd The private data of the Efl_Ui_Tab_Pager object.
 * @return The number of packed items.
 */
EOLIAN static int
_efl_ui_tab_pager_efl_container_content_count(Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *pd)
{
   return efl_content_count(pd->spotlight);
}

/**
 * @internal
 * @brief Gets an iterator for the packed items (pages) in the tab pager.
 *
 * This delegates to the content iterator of the internal spotlight container.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param pd The private data of the Efl_Ui_Tab_Pager object.
 * @return An iterator for the packed items.
 */
EOLIAN static Eina_Iterator*
_efl_ui_tab_pager_efl_container_content_iterate(Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *pd)
{
   return efl_content_iterate(pd->spotlight);
}

/**
 * @internal
 * @brief Packs a sub-object (page) into the tab pager.
 *
 * This delegates to the pack operation of the internal spotlight container.
 * The corresponding tab bar item is expected to be handled by the Efl_Ui_Tab_Page.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param pd The private data of the Efl_Ui_Tab_Pager object.
 * @param subobj The Efl_Gfx_Entity (page) to pack.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_tab_pager_efl_pack_pack(Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *pd, Efl_Gfx_Entity *subobj)
{
   return efl_pack(pd->spotlight, subobj);
}

/**
 * @internal
 * @brief Gets the packed content (page) at a specific index.
 *
 * This delegates to the internal spotlight container.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param pd The private data of the Efl_Ui_Tab_Pager object.
 * @param index The index of the content to retrieve.
 * @return The Efl_Gfx_Entity (page) at the given index, or NULL if out of bounds.
 */
EOLIAN static Efl_Gfx_Entity*
_efl_ui_tab_pager_efl_pack_linear_pack_content_get(Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *pd, int index)
{
   return efl_pack_content_get(pd->spotlight, index);
}

/**
 * @internal
 * @brief Gets the index of a packed sub-object (page).
 *
 * This delegates to the internal spotlight container.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param pd The private data of the Efl_Ui_Tab_Pager object.
 * @param subobj The Efl_Gfx_Entity (page) whose index is to be found.
 * @return The index of the sub-object, or -1 if not found.
 */
EOLIAN static int
_efl_ui_tab_pager_efl_pack_linear_pack_index_get(Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *pd, const Efl_Gfx_Entity *subobj)
{
   return efl_pack_index_get(pd->spotlight, subobj);
}

/**
 * @internal
 * @brief Clears all packed items from the tab pager.
 *
 * This clears both the internal tab bar and the spotlight container.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param sd The private data of the Efl_Ui_Tab_Pager object.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_tab_pager_efl_pack_pack_clear(Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *sd)
{
   if (!efl_pack_clear(sd->tab_bar))
     return EINA_FALSE;
   return efl_pack_clear(sd->spotlight);
}

/**
 * @internal
 * @brief Unpacks all items from the tab pager.
 *
 * This unpacks all items from both the internal tab bar and the spotlight container.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param sd The private data of the Efl_Ui_Tab_Pager object.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_tab_pager_efl_pack_unpack_all(Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *sd)
{
   if (!efl_pack_unpack_all(sd->tab_bar))
     return EINA_FALSE;
   return efl_pack_unpack_all(sd->spotlight);
}

/**
 * @internal
 * @brief Macro to get the tab bar item associated with a tab page.
 * @param s The Efl_Ui_Tab_Page object.
 * @return The Efl_Ui_Tab_Bar_Default_Item associated with the page.
 */
#define ITEM(s) efl_ui_tab_page_tab_bar_item_get(s)

/**
 * @internal
 * @brief Unpacks a specific sub-object (page) from the tab pager.
 *
 * This unpacks the corresponding tab bar item and the page from the spotlight container.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param sd The private data of the Efl_Ui_Tab_Pager object.
 * @param subobj The Efl_Gfx_Entity (page) to unpack.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_tab_pager_efl_pack_unpack(Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *sd, Efl_Gfx_Entity *subobj)
{
   if (!efl_pack_unpack(sd->tab_bar, ITEM(subobj)))
     return EINA_FALSE;
   return efl_pack_unpack(sd->spotlight, subobj);
}

/**
 * @internal
 * @brief Packs a sub-object (page) at the beginning of the tab pager.
 *
 * This packs the corresponding tab bar item and the page at the beginning
 * of their respective containers.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param sd The private data of the Efl_Ui_Tab_Pager object.
 * @param subobj The Efl_Gfx_Entity (page) to pack.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_tab_pager_efl_pack_linear_pack_begin(Eo *obj EINA_UNUSED,
                                             Efl_Ui_Tab_Pager_Data *sd,
                                             Efl_Gfx_Entity *subobj)
{
   if (!efl_pack_begin(sd->tab_bar, ITEM(subobj)))
     return EINA_FALSE;
   return efl_pack_begin(sd->spotlight, subobj);
}

/**
 * @internal
 * @brief Packs a sub-object (page) at the end of the tab pager.
 *
 * This packs the corresponding tab bar item and the page at the end
 * of their respective containers.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param sd The private data of the Efl_Ui_Tab_Pager object.
 * @param subobj The Efl_Gfx_Entity (page) to pack.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_tab_pager_efl_pack_linear_pack_end(Eo *obj EINA_UNUSED,
                                           Efl_Ui_Tab_Pager_Data *sd,
                                           Efl_Gfx_Entity *subobj)
{
   if (!efl_pack_end(sd->tab_bar, ITEM(subobj)))
     return EINA_FALSE;
   return efl_pack_end(sd->spotlight, subobj);
}

/**
 * @internal
 * @brief Packs a sub-object (page) before an existing item in the tab pager.
 *
 * This packs the corresponding tab bar item and the page before the
 * specified existing items in their respective containers.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param sd The private data of the Efl_Ui_Tab_Pager object.
 * @param subobj The Efl_Gfx_Entity (page) to pack.
 * @param existing The existing Efl_Gfx_Entity (page) to pack before.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_tab_pager_efl_pack_linear_pack_before(Eo *obj EINA_UNUSED,
                                              Efl_Ui_Tab_Pager_Data *sd,
                                              Efl_Gfx_Entity *subobj,
                                              const Efl_Gfx_Entity *existing)
{
   if (!efl_pack_before(sd->tab_bar, ITEM(subobj), ITEM(existing)))
     return EINA_FALSE;
   return efl_pack_before(sd->spotlight, subobj, existing);
}

/**
 * @internal
 * @brief Packs a sub-object (page) after an existing item in the tab pager.
 *
 * This packs the corresponding tab bar item and the page after the
 * specified existing items in their respective containers.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param sd The private data of the Efl_Ui_Tab_Pager object.
 * @param subobj The Efl_Gfx_Entity (page) to pack.
 * @param existing The existing Efl_Gfx_Entity (page) to pack after.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_tab_pager_efl_pack_linear_pack_after(Eo *obj EINA_UNUSED,
                                             Efl_Ui_Tab_Pager_Data *sd,
                                             Efl_Gfx_Entity *subobj,
                                             const Efl_Gfx_Entity *existing)
{
   if (!efl_pack_after(sd->tab_bar, ITEM(subobj), ITEM(existing)))
     return EINA_FALSE;
   return efl_pack_after(sd->spotlight, subobj, existing);
}

/**
 * @internal
 * @brief Packs a sub-object (page) at a specific index in the tab pager.
 *
 * This packs the corresponding tab bar item and the page at the specified
 * index in their respective containers.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param sd The private data of the Efl_Ui_Tab_Pager object.
 * @param subobj The Efl_Gfx_Entity (page) to pack.
 * @param index The index at which to pack the item.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_tab_pager_efl_pack_linear_pack_at(Eo *obj EINA_UNUSED,
                                          Efl_Ui_Tab_Pager_Data *sd,
                                          Efl_Gfx_Entity *subobj,
                                          int index)
{
   if (!efl_pack_at(sd->tab_bar, ITEM(subobj), index))
     return EINA_FALSE;
   return efl_pack_at(sd->spotlight, subobj, index);
}

/**
 * @internal
 * @brief Unpacks the item (page) at a specific index from the tab pager.
 *
 * This unpacks the tab bar item and the page from the spotlight container
 * at the given index.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param sd The private data of the Efl_Ui_Tab_Pager object.
 * @param index The index of the item to unpack.
 * @return The unpacked Efl_Gfx_Entity (page), or NULL on failure.
 */
EOLIAN static Efl_Gfx_Entity *
_efl_ui_tab_pager_efl_pack_linear_pack_unpack_at(Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *sd, int index)
{
   if (!efl_pack_unpack_at(sd->tab_bar, index))
     return NULL;
   return efl_pack_unpack_at(sd->spotlight, index);
}

/**
 * @internal
 * @brief Gets the last selected item (page) in the tab pager.
 *
 * This retrieves the last selected item from the internal tab bar
 * and returns its parent, which is the Efl_Ui_Tab_Page.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param pd The private data of the Efl_Ui_Tab_Pager object.
 * @return The last selected Efl_Ui_Selectable (page), or NULL if none.
 */
EOLIAN static Efl_Ui_Selectable*
_efl_ui_tab_pager_efl_ui_single_selectable_last_selected_get(const Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *pd)
{
   Efl_Ui_Tab_Bar_Default_Item *item = efl_ui_selectable_last_selected_get(pd->tab_bar);

   return efl_parent_get(item);
}

/**
 * @internal
 * @brief Sets the fallback selection for the tab pager.
 *
 * This sets the fallback selection on the internal tab bar, using the
 * tab bar item associated with the provided fallback page.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param pd The private data of the Efl_Ui_Tab_Pager object.
 * @param fallback The Efl_Ui_Selectable (page) to set as fallback.
 */
EOLIAN static void
_efl_ui_tab_pager_efl_ui_single_selectable_fallback_selection_set(Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *pd, Efl_Ui_Selectable *fallback)
{
   efl_ui_selectable_fallback_selection_set(pd->tab_bar, efl_ui_tab_page_tab_bar_item_get(fallback));
}

/**
 * @internal
 * @brief Gets the fallback selection for the tab pager.
 *
 * This retrieves the fallback selection from the internal tab bar
 * and returns its parent, which is the Efl_Ui_Tab_Page.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param pd The private data of the Efl_Ui_Tab_Pager object.
 * @return The fallback Efl_Ui_Selectable (page), or NULL if none.
 */
EOLIAN static Efl_Ui_Selectable*
_efl_ui_tab_pager_efl_ui_single_selectable_fallback_selection_get(const Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *pd)
{
   Efl_Ui_Tab_Bar_Default_Item *item = efl_ui_selectable_fallback_selection_get(pd->tab_bar);

   return efl_parent_get(item);
}

/**
 * @internal
 * @brief Sets whether manual deselection is allowed for the tab pager.
 *
 * This delegates the setting to the internal tab bar.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param pd The private data of the Efl_Ui_Tab_Pager object.
 * @param allow_manual_deselection EINA_TRUE to allow manual deselection, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_ui_tab_pager_efl_ui_single_selectable_allow_manual_deselection_set(Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *pd, Eina_Bool allow_manual_deselection)
{
   efl_ui_selectable_allow_manual_deselection_set(pd->tab_bar, allow_manual_deselection);
}

/**
 * @internal
 * @brief Gets whether manual deselection is allowed for the tab pager.
 *
 * This delegates the query to the internal tab bar.
 *
 * @param obj The Efl_Ui_Tab_Pager object.
 * @param pd The private data of the Efl_Ui_Tab_Pager object.
 * @return EINA_TRUE if manual deselection is allowed, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_tab_pager_efl_ui_single_selectable_allow_manual_deselection_get(const Eo *obj EINA_UNUSED, Efl_Ui_Tab_Pager_Data *pd)
{
   return efl_ui_selectable_allow_manual_deselection_get(pd->tab_bar);
}


#include "efl_ui_tab_pager.eo.c"
