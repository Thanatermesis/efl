#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"
#include "efl_ui_focus_parent_provider_gen_eo.h"
#include "efl_ui_focus_composition_adapter.eo.h"

/**
 * @brief Private data structure for the Efl_Ui_Focus_Parent_Provider_Gen class.
 */
typedef struct {
   Eina_Hash *map; /**< Hash map to store widget-item relationships. The key is a widget, and the value is its corresponding item. */
   Efl_Ui_Widget *container; /**< The container widget that this provider is associated with. */
   Efl_Ui_Focus_Parent_Provider *provider; /**< The parent provider, used to delegate requests if the current provider cannot handle them. */
} Efl_Ui_Focus_Parent_Provider_Gen_Data;

/**
 * @brief Sets the content item map.
 *
 * This map is used to find the logical parent of a widget.
 * The key of the map is an Efl_Ui_Widget, and the value is an Elm_Widget_Item.
 *
 * @param obj The Efl_Ui_Focus_Parent_Provider_Gen object.
 * @param pd The private data of the object.
 * @param map The hash map to set.
 */
EOLIAN static void
_efl_ui_focus_parent_provider_gen_content_item_map_set(Eo *obj, Efl_Ui_Focus_Parent_Provider_Gen_Data *pd, Eina_Hash *map)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(obj));

   pd->map = map;
}

/**
 * @brief Gets the content item map.
 *
 * @param obj The Efl_Ui_Focus_Parent_Provider_Gen object.
 * @param pd The private data of the object.
 * @return The hash map.
 */
EOLIAN static Eina_Hash*
_efl_ui_focus_parent_provider_gen_content_item_map_get(const Eo *obj EINA_UNUSED, Efl_Ui_Focus_Parent_Provider_Gen_Data *pd)
{
   return pd->map;
}

/**
 * @brief Sets the container widget.
 *
 * This also finds the parent provider from the container's parent and
 * updates shared window data if available.
 *
 * @param obj The Efl_Ui_Focus_Parent_Provider_Gen object.
 * @param pd The private data of the object.
 * @param container The container widget to set.
 */
EOLIAN static void
_efl_ui_focus_parent_provider_gen_container_set(Eo *obj, Efl_Ui_Focus_Parent_Provider_Gen_Data *pd, Efl_Ui_Widget *container)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(obj));

   pd->container = container;

   EINA_SAFETY_ON_NULL_RETURN(efl_parent_get(pd->container));

   pd->provider = efl_provider_find(efl_parent_get(pd->container), EFL_UI_FOCUS_PARENT_PROVIDER_INTERFACE);

   ELM_WIDGET_DATA_GET(pd->container, wid_pd);

   if (wid_pd->shared_win_data)
     ((Efl_Ui_Shared_Win_Data*)wid_pd->shared_win_data)->custom_parent_provider = EINA_TRUE;
}

/**
 * @brief Gets the container widget.
 *
 * @param obj The Efl_Ui_Focus_Parent_Provider_Gen object.
 * @param pd The private data of the object.
 * @return The container widget.
 */
EOLIAN static Efl_Ui_Widget*
_efl_ui_focus_parent_provider_gen_container_get(const Eo *obj EINA_UNUSED, Efl_Ui_Focus_Parent_Provider_Gen_Data *pd)
{
   return pd->container;
}

/**
 * @brief Finds the logical parent for focus management.
 *
 * This function attempts to find a logical parent for the given `widget`.
 * It first checks if the `widget` is an Efl_Ui_Focus_Composition_Adapter. If so,
 * its parent (which should be an Elm_Widget_Item) is returned.
 * Otherwise, if the `widget`'s parent is the provider's `container`, it looks
 * up the `widget` in the `pd->map`.
 * If no specific logical parent is found through these checks, it delegates
 * the search to the `pd->provider`.
 *
 * @param obj The Efl_Ui_Focus_Parent_Provider_Gen object.
 * @param pd The private data of the object.
 * @param widget The focus object for which to find the logical parent.
 * @return The logical parent focus object, or NULL if not found.
 */
EOLIAN static Efl_Ui_Focus_Object*
_efl_ui_focus_parent_provider_gen_efl_ui_focus_parent_provider_find_logical_parent(Eo *obj EINA_UNUSED, Efl_Ui_Focus_Parent_Provider_Gen_Data *pd EINA_UNUSED, Efl_Ui_Focus_Object *widget)
{
   //first check if this item is in the map
   Elm_Widget_Item *item, *above_gengrid = widget;
   Efl_Ui_Widget *parent;

   if (efl_isa(widget, EFL_UI_FOCUS_COMPOSITION_ADAPTER_CLASS))
     {
        item = efl_parent_get(widget);

        if (efl_isa(item, ELM_WIDGET_ITEM_CLASS))
          return item;
     }
   else
     {
        parent = elm_widget_parent_widget_get(widget);

        if (parent == pd->container)
          {
             item = eina_hash_find(pd->map, &above_gengrid);
             efl_ui_focus_object_setup_order(pd->container);

             if (item)
               return item;
          }
     }

   // We dont have a map entry
   return efl_ui_focus_parent_provider_find_logical_parent(pd->provider, widget);
}

/**
 * @brief Fetches the item associated with a widget, typically for focus management.
 *
 * This function tries to determine the Elm_Widget_Item that corresponds to the
 * given `widget` within the context of the `pd->container`.
 *
 * If `widget` is an Efl_Ui_Focus_Composition_Adapter and its parent is an
 * Elm_Widget_Item, that item is returned directly after setting up the focus order
 * for the container.
 *
 * Otherwise, it traverses up the widget hierarchy from `widget` until it reaches
 * a widget that is a direct child of `pd->container` or `pd->container` itself.
 * This "top-level" widget (relative to the container) is then used as a key
 * to look up an Elm_Widget_Item in `pd->map`.
 *
 * Finally, it ensures the focus order for `pd->container` is set up.
 *
 * @param obj The Efl_Ui_Focus_Parent_Provider_Gen object.
 * @param pd The private data of the object.
 * @param widget The widget for which to fetch the corresponding item.
 * @return The Elm_Widget_Item associated with the widget, or NULL if not found.
 */
EOLIAN static Efl_Ui_Widget*
_efl_ui_focus_parent_provider_gen_item_fetch(Eo *obj EINA_UNUSED, Efl_Ui_Focus_Parent_Provider_Gen_Data *pd, Efl_Ui_Widget *widget)
{
   //first check if this item is in the map
   Elm_Widget_Item *item, *above_gengrid = widget;
   Efl_Ui_Widget *parent;

   if (efl_isa(widget, EFL_UI_FOCUS_COMPOSITION_ADAPTER_CLASS))
     {
        item = efl_parent_get(widget);

        if (efl_isa(item, ELM_WIDGET_ITEM_CLASS))
          {
             efl_ui_focus_object_setup_order(pd->container);
             return item;
          }
        else
          {
             parent = item;
          }
     }
   else
     {
        parent = elm_widget_parent_widget_get(widget);
     }


   //move forward so we get the last widget above the gengrid level,
   //this may be the widget out of the map
   while (parent && parent != pd->container)
     {
        above_gengrid = parent;
        parent = elm_widget_parent_widget_get(above_gengrid);
     }
   item = eina_hash_find(pd->map, &above_gengrid);

   efl_ui_focus_object_setup_order(pd->container);

   return item;
}

#include "efl_ui_focus_parent_provider_gen_eo.c"
