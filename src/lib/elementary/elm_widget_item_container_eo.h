#ifndef _ELM_WIDGET_ITEM_CONTAINER_EO_H_
#define _ELM_WIDGET_ITEM_CONTAINER_EO_H_

#ifndef _ELM_WIDGET_ITEM_CONTAINER_EO_CLASS_TYPE
#define _ELM_WIDGET_ITEM_CONTAINER_EO_CLASS_TYPE

/**
 * @brief Represents an Efl object that acts as a container for Elm_Widget_Item instances.
 * @ingroup Elm_Widget_Item_Container
 */
typedef Eo Elm_Widget_Item_Container;

#endif

#ifndef _ELM_WIDGET_ITEM_CONTAINER_EO_TYPES
#define _ELM_WIDGET_ITEM_CONTAINER_EO_TYPES


#endif

/**
 * @brief A macro convenience for #elm_widget_item_container_interface_get().
 * @details This macro provides a simpler way to get the Efl_Class object for
 * the Elm_Widget_Item_Container interface.
 * @ingroup Elm_Widget_Item_Container
 */
#define ELM_WIDGET_ITEM_CONTAINER_INTERFACE elm_widget_item_container_interface_get()

/**
 * @brief Get the Eo class description for the Elm_Widget_Item_Container interface.
 *
 * @return The Efl_Class pointer for the Elm_Widget_Item_Container interface.
 * @ingroup Elm_Widget_Item_Container
 */
EWAPI const Efl_Class *elm_widget_item_container_interface_get(void) EINA_CONST;

/**
 * @brief Get the focused widget item from the container.
 *
 * @details This function retrieves the item that currently has focus within
 * the widget item container.
 *
 * @param[in] obj The object.
 *
 * @return The focused item, or @c NULL if no item has focus.
 *
 * @ingroup Elm_Widget_Item_Container
 */
EOAPI Elm_Widget_Item *elm_widget_item_container_focused_item_get(const Eo *obj);

#endif
