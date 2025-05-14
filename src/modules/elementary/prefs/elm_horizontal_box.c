#include "private.h"

/**
 * @internal
 *
 * @brief Adds a new horizontal box widget to a prefs page.
 *
 * This function is the factory for creating horizontal box layout
 * containers within a preferences page.
 *
 * @param iface The prefs page widget interface (unused).
 * @param prefs The parent prefs widget.
 *
 * @return The new horizontal box object.
 */
static Evas_Object *
elm_prefs_horizontal_box_add(const Elm_Prefs_Page_Iface *iface EINA_UNUSED,
                             Evas_Object *prefs)
{
   Evas_Object *obj = elm_box_add(prefs);

   elm_box_horizontal_set(obj, EINA_TRUE);

   return obj;
}

/**
 * @internal
 *
 * @brief Packs a prefs item into a horizontal box container.
 *
 * This function handles the placement and configuration of a prefs item
 * within the horizontal box. It adjusts alignment for labels and handles
 * special item types like separators.
 *
 * @param obj The horizontal box container.
 * @param it The prefs item to be packed.
 * @param type The type of the prefs item.
 * @param iface The interface for the prefs item.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_horizontal_box_item_pack(Evas_Object *obj,
                                   Evas_Object *it,
                                   const Elm_Prefs_Item_Type type,
                                   const Elm_Prefs_Item_Iface *iface)
{
   Evas_Object *l;

   if ((type == ELM_PREFS_TYPE_SEPARATOR) &&
       (!elm_prefs_page_item_value_set(it, iface, EINA_FALSE)))
     return EINA_FALSE;

   l = evas_object_data_get(it, "label_widget");
   if (l)
     evas_object_size_hint_align_set(l, 1.0, 0.5);

   elm_prefs_horizontal_page_common_pack(it, obj, iface);

   return EINA_TRUE;
}

/**
 * @internal
 *
 * @brief Unpacks a prefs item from a horizontal box container.
 *
 * This function removes a prefs item from the horizontal box and resets
 * its visual properties (size hints) to their default state.
 *
 * @param obj The horizontal box container.
 * @param it The prefs item to be unpacked.
 *
 * @return @c EINA_TRUE on success.
 */
static Eina_Bool
elm_prefs_horizontal_box_item_unpack(Evas_Object *obj,
                                     Evas_Object *it)
{
   /* back to defaults */
   evas_object_size_hint_align_set(it, 0.5, 0.5);
   evas_object_size_hint_weight_set(it, 0.0, 0.0);

   elm_prefs_page_common_unpack(it, obj);

   return EINA_TRUE;
}

/**
 * @internal
 *
 * @brief Packs a prefs item into a horizontal box before a reference item.
 *
 * This function inserts a prefs item into the horizontal box at a specific
 * position, before another existing item. It also handles item-specific
 * setup, like label alignment.
 *
 * @param obj The horizontal box container.
 * @param it The new prefs item to be packed.
 * @param it_before The existing item before which to pack.
 * @param type The type of the new prefs item.
 * @param iface The interface for the new prefs item.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_horizontal_box_item_pack_before(Evas_Object *obj,
                                          Evas_Object *it,
                                          Evas_Object *it_before,
                                          const Elm_Prefs_Item_Type type,
                                          const Elm_Prefs_Item_Iface *iface)
{
   Evas_Object *l;

   if ((type == ELM_PREFS_TYPE_SEPARATOR) &&
       (!elm_prefs_page_item_value_set(it, iface, EINA_FALSE)))
     return EINA_FALSE;

   l = evas_object_data_get(it, "label_widget");
   if (l)
     evas_object_size_hint_align_set(l, 1.0, 0.5);

   elm_prefs_horizontal_page_common_pack_before(it, it_before, obj, iface);

   return EINA_TRUE;
}

/**
 * @internal
 *
 * @brief Packs a prefs item into a horizontal box after a reference item.
 *
 * This function inserts a prefs item into the horizontal box at a specific
 * position, after another existing item. It also handles item-specific
 * setup, like label alignment.
 *
 * @param obj The horizontal box container.
 * @param it The new prefs item to be packed.
 * @param it_after The existing item after which to pack.
 * @param type The type of the new prefs item.
 * @param iface The interface for the new prefs item.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_horizontal_box_item_pack_after(Evas_Object *obj,
                                         Evas_Object *it,
                                         Evas_Object *it_after,
                                         const Elm_Prefs_Item_Type type,
                                         const Elm_Prefs_Item_Iface *iface)
{
   Evas_Object *l;

   if ((type == ELM_PREFS_TYPE_SEPARATOR) &&
       (!elm_prefs_page_item_value_set(it, iface, EINA_FALSE)))
     return EINA_FALSE;

   l = evas_object_data_get(it, "label_widget");
   if (l)
     evas_object_size_hint_align_set(l, 1.0, 0.5);

   elm_prefs_horizontal_page_common_pack_after(it, it_after, obj, iface);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Registers the "horizontal_box" page widget implementation.
 *
 * This macro call defines and registers the interface for the horizontal_box
 * page widget. It provides the function pointers for item packing, unpacking,
 * and reordering, allowing the prefs framework to manage items within this
 * specific container type. The NULL arguments indicate that this widget does
 * not support item addition, sub-object retrieval, or icon setting at the
 * page widget level itself.
 */
PREFS_PAGE_WIDGET_ADD(horizontal_box,
                      NULL,
                      NULL,
                      NULL,
                      elm_prefs_horizontal_box_item_pack,
                      elm_prefs_horizontal_box_item_unpack,
                      elm_prefs_horizontal_box_item_pack_before,
                      elm_prefs_horizontal_box_item_pack_after);
