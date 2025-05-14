#include "private.h"

/**
 * @brief Adds a new vertical box widget to a prefs page.
 *
 * This function creates a standard Elm_Box, configures it to be a vertical
 * container, and returns it. This box is intended to hold preference items.
 *
 * @param iface The prefs page interface (unused).
 * @param prefs The parent prefs widget.
 * @return A new Evas_Object* that is the vertical box widget, or @c NULL on
 *         failure.
 */
static Evas_Object *
elm_prefs_vertical_box_add(const Elm_Prefs_Page_Iface *iface EINA_UNUSED,
                           Evas_Object *prefs)
{
   Evas_Object *obj = elm_box_add(prefs);

   elm_box_horizontal_set(obj, EINA_FALSE);

   return obj;
}

/**
 * @brief Packs a preference item into the vertical box.
 *
 * This function is responsible for adding a preference item to the end of the
 * vertical box container. It performs special handling for separator items
 * and adjusts the alignment of the item's label. The actual packing logic is
 * delegated to a common helper function.
 *
 * @param obj The vertical box container widget.
 * @param it The preference item to pack.
 * @param type The type of the preference item.
 * @param iface The interface for the preference item.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_vertical_box_item_pack(Evas_Object *obj,
                                 Evas_Object *it,
                                 const Elm_Prefs_Item_Type type,
                                 const Elm_Prefs_Item_Iface *iface)
{
   Evas_Object *l;

   if ((type == ELM_PREFS_TYPE_SEPARATOR) &&
       (!elm_prefs_page_item_value_set(it, iface, EINA_TRUE)))
     return EINA_FALSE;

   l = evas_object_data_get(it, "label_widget");
   if (l)
     evas_object_size_hint_align_set(l, 0.0, 1.0);

   elm_prefs_vertical_page_common_pack(it, obj, iface);

   return EINA_TRUE;
}

/**
 * @brief Unpacks a preference item from the vertical box.
 *
 * This function removes a preference item from the box container. Before
 * removal, it resets the item's size hint alignment and weight to their
 * default values. The actual unpacking logic is handled by a common
 * helper function.
 *
 * @param obj The vertical box container widget.
 * @param it The preference item to unpack.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_vertical_box_item_unpack(Evas_Object *obj,
                                   Evas_Object *it)
{
   /* back to defaults */
   evas_object_size_hint_align_set(it, 0.5, 0.5);
   evas_object_size_hint_weight_set(it, 0.0, 0.0);

   elm_prefs_page_common_unpack(it, obj);

   return EINA_TRUE;
}

/**
 * @brief Packs a preference item into the vertical box before a reference item.
 *
 * This function adds a preference item into the container at a position
 * just before another specified item. It performs special handling for
 * separator items and adjusts the alignment of the new item's label.
 *
 * @param obj The vertical box container widget.
 * @param it The new preference item to pack.
 * @param it_before The existing item before which to pack the new item.
 * @param type The type of the new preference item.
 * @param iface The interface for the new preference item.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_vertical_box_item_pack_before(Evas_Object *obj,
                                        Evas_Object *it,
                                        Evas_Object *it_before,
                                        const Elm_Prefs_Item_Type type,
                                        const Elm_Prefs_Item_Iface *iface)
{
   Evas_Object *l;

   if ((type == ELM_PREFS_TYPE_SEPARATOR) &&
       (!elm_prefs_page_item_value_set(it, iface, EINA_TRUE)))
     return EINA_FALSE;

   l = evas_object_data_get(it, "label_widget");
   if (l)
     evas_object_size_hint_align_set(l, 0.0, 1.0);

   elm_prefs_vertical_page_common_pack_before(it, it_before, obj, iface);

   return EINA_TRUE;
}

/**
 * @brief Packs a preference item into the vertical box after a reference item.
 *
 * This function adds a preference item into the container at a position
 * just after another specified item. It performs special handling for
 * separator items and adjusts the alignment of the new item's label.
 *
 * @param obj The vertical box container widget.
 * @param it The new preference item to pack.
 * @param it_after The existing item after which to pack the new item.
 * @param type The type of the new preference item.
 * @param iface The interface for the new preference item.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_vertical_box_item_pack_after(Evas_Object *obj,
                                       Evas_Object *it,
                                       Evas_Object *it_after,
                                       const Elm_Prefs_Item_Type type,
                                       const Elm_Prefs_Item_Iface *iface)
{
   Evas_Object *l;

   if ((type == ELM_PREFS_TYPE_SEPARATOR) &&
       (!elm_prefs_page_item_value_set(it, iface, EINA_TRUE)))
     return EINA_FALSE;

   l = evas_object_data_get(it, "label_widget");
   if (l)
     evas_object_size_hint_align_set(l, 0.0, 1.0);

   elm_prefs_vertical_page_common_pack_after(it, it_after, obj, iface);

   return EINA_TRUE;
}

PREFS_PAGE_WIDGET_ADD(vertical_box,
                      NULL,
                      NULL,
                      NULL,
                      elm_prefs_vertical_box_item_pack,
                      elm_prefs_vertical_box_item_unpack,
                      elm_prefs_vertical_box_item_pack_before,
                      elm_prefs_vertical_box_item_pack_after);
