#include "private.h"

/**
 * @internal
 * @brief Adds a new horizontal frame widget to a preferences page.
 *
 * This function creates an elm_frame object and sets a horizontal elm_box
 * as its content. This container is used to lay out preference items
 * horizontally.
 *
 * @param iface The preferences page interface (unused).
 * @param prefs The parent preferences object.
 * @return The newly created frame object, or NULL on failure.
 */
static Evas_Object *
elm_prefs_horizontal_frame_add(const Elm_Prefs_Page_Iface *iface EINA_UNUSED,
                               Evas_Object *prefs)
{
   Evas_Object *bx, *obj = elm_frame_add(prefs);

   bx = elm_box_add(obj);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   elm_box_horizontal_set(bx, EINA_TRUE);

   elm_layout_content_set(obj, NULL, bx);

   evas_object_data_set(obj, "bx_container", bx);

   return obj;
}

/**
 * @internal
 * @brief Sets the title of the horizontal frame widget.
 *
 * @param obj The horizontal frame object.
 * @param title The title string to set.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
elm_prefs_horizontal_frame_title_set(Evas_Object *obj,
                                     const char *title)
{
   elm_layout_text_set(obj, NULL, title);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Packs a preferences item into the horizontal frame.
 *
 * This function packs a given item into the frame's horizontal box.
 * It handles special item types, such as separators, and adjusts the
 * alignment of item labels for a consistent horizontal layout.
 *
 * @param obj The horizontal frame object.
 * @param it The item to pack.
 * @param type The type of the item being packed.
 * @param iface The interface for the item.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
elm_prefs_horizontal_frame_item_pack(Evas_Object *obj,
                                     Evas_Object *it,
                                     const Elm_Prefs_Item_Type type,
                                     const Elm_Prefs_Item_Iface *iface)
{
   Evas_Object *l, *bx = evas_object_data_get(obj, "bx_container");

   if ((type == ELM_PREFS_TYPE_SEPARATOR) &&
       (!elm_prefs_page_item_value_set(it, iface, EINA_FALSE)))
     return EINA_FALSE;

   l = evas_object_data_get(it, "label_widget");
   if (l)
     evas_object_size_hint_align_set(l, 1.0, 0.5);

   elm_prefs_horizontal_page_common_pack(it, bx, iface);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Unpacks a preferences item from the horizontal frame.
 *
 * This function removes an item from the frame's box container and resets
 * its size hints to their default values.
 *
 * @param obj The horizontal frame object.
 * @param it The item to unpack.
 * @return @c EINA_TRUE on success.
 */
static Eina_Bool
elm_prefs_horizontal_frame_item_unpack(Evas_Object *obj,
                                       Evas_Object *it)
{
   Evas_Object *bx = evas_object_data_get(obj, "bx_container");

   /* back to defaults */
   evas_object_size_hint_align_set(it, 0.5, 0.5);
   evas_object_size_hint_weight_set(it, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   elm_prefs_page_common_unpack(it, bx);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Packs a preferences item into the horizontal frame before a reference item.
 *
 * Similar to elm_prefs_horizontal_frame_item_pack(), but inserts the item
 * before another specified item in the container.
 *
 * @param obj The horizontal frame object.
 * @param it The new item to pack.
 * @param it_before The existing item before which to pack the new item.
 * @param type The type of the new item.
 * @param iface The interface for the new item.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
elm_prefs_horizontal_frame_item_pack_before(Evas_Object *obj,
                                            Evas_Object *it,
                                            Evas_Object *it_before,
                                            const Elm_Prefs_Item_Type type,
                                            const Elm_Prefs_Item_Iface *iface)
{
   Evas_Object *l, *bx = evas_object_data_get(obj, "bx_container");

   if ((type == ELM_PREFS_TYPE_SEPARATOR) &&
       (!elm_prefs_page_item_value_set(it, iface, EINA_FALSE)))
     return EINA_FALSE;

   l = evas_object_data_get(it, "label_widget");
   if (l)
     evas_object_size_hint_align_set(l, 1.0, 0.5);

   elm_prefs_horizontal_page_common_pack_before(it, it_before, bx, iface);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Packs a preferences item into the horizontal frame after a reference item.
 *
 * Similar to elm_prefs_horizontal_frame_item_pack(), but inserts the item
 * after another specified item in the container.
 *
 * @param obj The horizontal frame object.
 * @param it The new item to pack.
 * @param it_after The existing item after which to pack the new item.
 * @param type The type of the new item.
 * @param iface The interface for the new item.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
elm_prefs_horizontal_frame_item_pack_after(Evas_Object *obj,
                                           Evas_Object *it,
                                           Evas_Object *it_after,
                                           const Elm_Prefs_Item_Type type,
                                           const Elm_Prefs_Item_Iface *iface)
{
   Evas_Object *l, *bx = evas_object_data_get(obj, "bx_container");

   if ((type == ELM_PREFS_TYPE_SEPARATOR) &&
       (!elm_prefs_page_item_value_set(it, iface, EINA_FALSE)))
     return EINA_FALSE;

   l = evas_object_data_get(it, "label_widget");
   if (l)
     evas_object_size_hint_align_set(l, 1.0, 0.5);

   elm_prefs_horizontal_page_common_pack_after(it, it_after, bx, iface);

   return EINA_TRUE;
}

PREFS_PAGE_WIDGET_ADD(horizontal_frame,
                      elm_prefs_horizontal_frame_title_set,
                      NULL,
                      NULL,
                      elm_prefs_horizontal_frame_item_pack,
                      elm_prefs_horizontal_frame_item_unpack,
                      elm_prefs_horizontal_frame_item_pack_before,
                      elm_prefs_horizontal_frame_item_pack_after);
