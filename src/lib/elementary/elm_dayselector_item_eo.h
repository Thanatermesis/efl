/**
 * @file
 * @brief EFL Dayselector Item class
 */
#ifndef _ELM_DAYSELECTOR_ITEM_EO_H_
#define _ELM_DAYSELECTOR_ITEM_EO_H_

#ifndef _ELM_DAYSELECTOR_ITEM_EO_CLASS_TYPE
#define _ELM_DAYSELECTOR_ITEM_EO_CLASS_TYPE

/**
 * @brief Represents an item in the Elm_Dayselector widget.
 * @ingroup Elm_Dayselector_Item
 */
typedef Eo Elm_Dayselector_Item;

#endif

#ifndef _ELM_DAYSELECTOR_ITEM_EO_TYPES
#define _ELM_DAYSELECTOR_ITEM_EO_TYPES


#endif
/** Elementary dayselector item class
 *
 * @ingroup Elm_Dayselector_Item
 */
#define ELM_DAYSELECTOR_ITEM_CLASS elm_dayselector_item_class_get()

/**
 * @brief Get the Efl_Class for the Elm_Dayselector_Item.
 *
 * @return The Efl_Class for Elm_Dayselector_Item.
 * @ingroup Elm_Dayselector_Item
 */
EWAPI const Efl_Class *elm_dayselector_item_class_get(void) EINA_CONST;

#endif
