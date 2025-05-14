#ifndef _ELM_POPUP_ITEM_EO_H_
#define _ELM_POPUP_ITEM_EO_H_

/**
 * @file
 * @brief Define the Efl an Elementary Popup Item.
 *
 * This is the Efl Eo wrapper for Elm_Popup_Item object.
 */

#ifndef _ELM_POPUP_ITEM_EO_CLASS_TYPE
#define _ELM_POPUP_ITEM_EO_CLASS_TYPE

/**
 * @brief Represents an item within an Elementary Popup widget.
 * @ingroup Elm_Popup_Item
 */
typedef Eo Elm_Popup_Item;

#endif

#ifndef _ELM_POPUP_ITEM_EO_TYPES
#define _ELM_POPUP_ITEM_EO_TYPES


#endif
/** Elementary popup item class
 *
 * @ingroup Elm_Popup_Item
 */
#define ELM_POPUP_ITEM_CLASS elm_popup_item_class_get()

/**
 * @brief Get the Efl class description for the Elm_Popup_Item class.
 *
 * @return The Efl class description.
 * @ingroup Elm_Popup_Item
 */
EWAPI const Efl_Class *elm_popup_item_class_get(void) EINA_CONST;

#endif
