/**
 * @file
 * @brief This file implements the static focus handling for Elm_Widget_Item.
 *
 * It provides an adapter for focus management, particularly for items
 * within Genlist and Gengrid widgets, ensuring that focus behaves
 * correctly even when items are realized or unrealized.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_FOCUS_COMPOSITION_ADAPTER_PROTECTED
#define ELM_WIDGET_ITEM_PROTECTED
#define MY_CLASS ELM_WIDGET_ITEM_STATIC_FOCUS_CLASS

#include <Elementary.h>
#include "elm_genlist_eo.h"
#include "elm_gengrid_eo.h"
#include "elm_priv.h"
#include "efl_ui_focus_composition_adapter.eo.h"

/**
 * @brief Private data structure for Elm_Widget_Item_Static_Focus.
 */
typedef struct {
   Eo *adapter; /**< The focus composition adapter object. This is created when the item needs to represent focus. */
   Eina_Bool realized; /**< Flag indicating if the item is currently realized (visible/active). */
   Eina_Bool in_unrealize; /**< Flag to prevent adapter deletion during the unrealize process if it's currently focused. */
} Elm_Widget_Item_Static_Focus_Data;

/**
 * @brief Sets the realized state of the widget item.
 * @param f The Elm_Widget_Item_Static_Focus object.
 */
static void
_realized_set(Elm_Widget_Item_Static_Focus *f)
{
   Elm_Widget_Item_Static_Focus_Data *pd = efl_data_scope_get(f, MY_CLASS);

   pd->realized = EINA_TRUE;
}

/**
 * @brief Callback invoked when a genlist item is realized.
 *
 * This function marks the item as realized and, if it's not disabled
 * and not a group item, sets up its focus order.
 * @param obj The Efl_Object representing the genlist item.
 */
static void
_list_realized_cb(Eo *obj)
{
   _realized_set(obj);

   if (!elm_object_item_disabled_get(obj) &&
       elm_genlist_item_type_get(obj) != ELM_GENLIST_ITEM_GROUP)
     efl_ui_focus_object_setup_order(obj);
}

/**
 * @brief Callback invoked when a gengrid item is realized.
 *
 * This function marks the item as realized. If the item is not disabled
 * and not a group item (identified by its item style "group_index"),
 * it sets up its focus order.
 * @param obj The Efl_Object representing the gengrid item.
 */
static void
_grid_realized_cb(Eo *obj)
{
   const Elm_Gen_Item_Class *itc;
   Eina_Bool is_group = EINA_FALSE;

   _realized_set(obj);

   itc = elm_gengrid_item_item_class_get(obj);

   is_group = (itc && itc->item_style && !strcmp(itc->item_style, "group_index"));

   if (!elm_object_item_disabled_get(obj) && !is_group)
     efl_ui_focus_object_setup_order(obj);
}

/**
 * @brief Callback invoked when an item is unrealized.
 *
 * This function marks the item as unrealized. It also handles the deletion
 * of the focus adapter, but only if the adapter exists and is not currently focused.
 * This prevents focus artifacts when items are scrolled out of view.
 * @param obj The Efl_Object representing the item.
 */
static void
_unrealized_cb(Eo *obj)
{
   Elm_Widget_Item_Static_Focus_Data *pd = efl_data_scope_get(obj, MY_CLASS);

   if (pd) /* if the obect is dead pd is NULL */
     {
        //only delete the adapter when not focused, this will lead to awfull artifacts
        if (pd->adapter && (!efl_ui_focus_object_focus_get(pd->adapter)))
          {
             pd->in_unrealize = EINA_TRUE;
             efl_del(pd->adapter);
             pd->in_unrealize = EINA_FALSE;
          }
        pd->realized = EINA_FALSE;
     }
}

/**
 * @brief Sets up the focus order for the widget item non-recursively.
 *
 * This function is crucial for managing focus when items are part of a larger
 * navigable structure (like Genlist or Gengrid). It ensures that a focus
 * adapter (EFL_UI_FOCUS_COMPOSITION_ADAPTER_CLASS) is created if needed,
 * or reconfigured if the logical focus child changes.
 *
 * The adapter acts as a proxy for the item in the focus chain of the parent widget.
 * This is necessary because widget items themselves are not direct children
 * in the focus manager's view.
 *
 * If the item is not realized, this function will log a warning and return,
 * as focus setup on an unrealized item can lead to issues.
 *
 * The logic handles cases where:
 * - No logical child exists for focus: an adapter is created.
 * - A logical child exists but is not the current adapter: the old adapter is deleted
 *   (if not in unrealize phase) and a new one might be implicitly created or the existing one reused.
 * - The logical child is the current adapter: it checks the next focusable widget to ensure
 *   the adapter is correctly positioned or if it should be skipped (e.g., if the next item is also
 *   a static focus item or another adapter for the same parent).
 *
 * @param obj The Elm_Widget_Item_Static_Focus object.
 * @param pd Private data for the object.
 */
EOLIAN static void
_elm_widget_item_static_focus_efl_ui_focus_object_setup_order_non_recursive(Eo *obj, Elm_Widget_Item_Static_Focus_Data *pd)
{
   Eo *logical_child;
   Elm_Widget_Item_Data *wpd = efl_data_scope_get(obj, ELM_WIDGET_ITEM_CLASS);

   efl_ui_focus_object_setup_order_non_recursive(efl_super(obj, MY_CLASS));

   if (!pd->realized)
     {
        WRN("This item is not realized, thus things will fall over, better return NOW");
        return;
     }

   logical_child = efl_ui_focus_manager_request_subchild(wpd->widget, obj);

   if (logical_child == pd->adapter)
     {
        Eo *next_widget;
        next_widget = efl_ui_focus_manager_request_move(wpd->widget, EFL_UI_FOCUS_DIRECTION_NEXT, logical_child, EINA_TRUE);

        if (efl_isa(next_widget, ELM_WIDGET_ITEM_STATIC_FOCUS_CLASS))
          {
             next_widget = NULL;
          }
        //check if this is the item block representation of genlist
        else if (efl_isa(next_widget, EFL_UI_FOCUS_COMPOSITION_ADAPTER_CLASS) && efl_ui_focus_object_focus_parent_get(next_widget) == wpd->widget)
          {
             next_widget = NULL;
          }
        logical_child = next_widget;
     }

   if (!logical_child)
     {
        if (!pd->adapter)
          {
             // parent has to stay the object, since this is used to get the item of a adapter
             pd->adapter = efl_add(EFL_UI_FOCUS_COMPOSITION_ADAPTER_CLASS, obj);
             efl_ui_focus_composition_adapter_focus_manager_parent_set(pd->adapter, wpd->widget);
             efl_ui_focus_composition_adapter_focus_manager_object_set(pd->adapter, wpd->widget);
             efl_wref_add(pd->adapter, &pd->adapter);
             efl_ui_focus_manager_calc_register(wpd->widget, pd->adapter, obj, NULL);
          }
     }
   else if (logical_child && logical_child != pd->adapter)
     {
        if (!pd->in_unrealize)
          {
             efl_del(pd->adapter);
             pd->adapter = NULL;
          }

     }

   //genlist sometimes changes views when doing quick scrolls so reset the view in every possible call
   if (pd->adapter)
     efl_ui_focus_composition_adapter_canvas_object_set(pd->adapter,  wpd->view);
}

/**
 * @brief Constructor for Elm_Widget_Item_Static_Focus.
 *
 * Initializes the widget item and sets up the appropriate realized/unrealized
 * callbacks based on whether the parent widget is a Genlist or Gengrid.
 *
 * @param obj The Efl_Object being constructed.
 * @param pd Private data for the object (unused in this function).
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object*
_elm_widget_item_static_focus_efl_object_constructor(Eo *obj, Elm_Widget_Item_Static_Focus_Data *pd EINA_UNUSED)
{
   Elm_Widget_Item_Data *wpd = efl_data_scope_get(obj, ELM_WIDGET_ITEM_CLASS);
   Eo *ret = efl_constructor(efl_super(obj, MY_CLASS));

   if (efl_isa(wpd->widget, ELM_GENLIST_CLASS))
     wpd->func.realized = _list_realized_cb;
   else
     wpd->func.realized = _grid_realized_cb;
   wpd->func.unrealized = _unrealized_cb;
   return ret;
}

/**
 * @brief Destructor for Elm_Widget_Item_Static_Focus.
 *
 * Cleans up resources used by the widget item, including nullifying
 * the realized/unrealized callbacks and deleting the focus adapter if it exists.
 *
 * @param obj The Efl_Object being destructed.
 * @param pd Private data for the object.
 */
EOLIAN static void
_elm_widget_item_static_focus_efl_object_destructor(Eo *obj, Elm_Widget_Item_Static_Focus_Data *pd EINA_UNUSED)
{
   Elm_Widget_Item_Data *wpd = efl_data_scope_get(obj, ELM_WIDGET_ITEM_CLASS);
   wpd->func.realized = NULL;
   wpd->func.unrealized = NULL;
   if (pd->adapter)
     efl_del(pd->adapter);

   return efl_destructor(efl_super(obj, MY_CLASS));
}


#include "elm_widget_item_static_focus_eo.c"
