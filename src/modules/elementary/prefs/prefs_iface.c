#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "Elementary.h"

#ifndef EFL_BUILD
# define EFL_BUILD
#endif
#undef ELM_MODULE_HELPER_H
#include "private.h"

/* including declaration of each prefs item implementation iface struct */
#define PREFS_ADD(w_name) \
  extern const Elm_Prefs_Item_Iface prefs_##w_name##_impl;

#include "item_widgets.inc"

#undef PREFS_ADD

int _elm_prefs_log_dom = -1;

/**
 * @internal
 * @brief Array of registered prefs item widget implementations.
 *
 * This array holds information about all available prefs item widgets.
 * Each element is an #Elm_Prefs_Item_Iface_Info struct containing a
 * widget name and a pointer to its implementation interface. The array
 * is terminated by a {NULL, NULL} entry.
 *
 * It is used to register all item interfaces at module initialization.
 *
 * Example structure:
 * @code
 * static Elm_Prefs_Item_Iface_Info _elm_prefs_item_widgets[] =
 * {
 *   {"elm/separator", &prefs_separator_impl},
 *   {"elm/spinner", &prefs_spinner_impl},
 *   ...
 *   {NULL, NULL}
 * };
 * @endcode
 */
/* now building on array of those, to be put on a hash for lookup */
static Elm_Prefs_Item_Iface_Info _elm_prefs_item_widgets[] =
{
#define PREFS_ADD(w_name) \
  {"elm/" #w_name, &prefs_##w_name##_impl},

#include "item_widgets.inc"

#undef PREFS_ADD
   {NULL, NULL}
};

/* including declaration of each prefs page implementation iface struct */
#define PREFS_ADD(w_name) \
  extern const Elm_Prefs_Page_Iface prefs_##w_name##_impl;

#include "page_widgets.inc"

#undef PREFS_ADD

/**
 * @internal
 * @brief Array of registered prefs page widget implementations.
 *
 * This array holds information about all available prefs page widgets.
 * Each element is an #Elm_Prefs_Page_Iface_Info struct containing a
 * page widget name and a pointer to its implementation interface. The array
 * is terminated by a {NULL, NULL} entry.
 *
 * It is used to register all page interfaces at module initialization.
 *
 * Example structure:
 * @code
 * static Elm_Prefs_Page_Iface_Info _elm_prefs_page_widgets[] =
 * {
 *   {"elm/vbox", &prefs_vbox_impl},
 *   {"elm/hbox", &prefs_hbox_impl},
 *   ...
 *   {NULL, NULL}
 * };
 * @endcode
 */
/* now building on array of those, to be put on a hash for lookup */
static Elm_Prefs_Page_Iface_Info _elm_prefs_page_widgets[] =
{
#define PREFS_ADD(w_name) \
  {"elm/" #w_name, &prefs_##w_name##_impl},

#include "page_widgets.inc"

#undef PREFS_ADD
   {NULL, NULL}
};

/**
 * @internal
 * @brief Sets a boolean value on a prefs item widget.
 *
 * This is a helper function to facilitate setting a boolean value on a
 * prefs item. It creates an #Eina_Value of type UCHAR from the given
 * boolean and calls the item's `value_set` implementation function.
 *
 * @param it The prefs item widget.
 * @param iface The interface of the prefs item.
 * @param val The boolean value to set.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
Eina_Bool
elm_prefs_page_item_value_set(Evas_Object *it,
                              const Elm_Prefs_Item_Iface *iface,
                              Eina_Bool val)
{
   Eina_Value value;

   if (!iface->value_set) return EINA_FALSE;

   if ((!eina_value_setup(&value, EINA_VALUE_TYPE_UCHAR)) ||
       (!eina_value_set(&value, val)))
     return EINA_FALSE;

   return iface->value_set(it, &value);
}

/**
 * @internal
 * @brief Creates and adds a sub-box for laying out a prefs item with an icon.
 *
 * This function creates a horizontal box used as a container for a prefs
 * item and its associated icon. This allows the item and icon to be treated
 * as a single unit within the larger prefs page layout. The created box is
 * stored in the parent object's data under the key "sub_box".
 *
 * @param obj The parent container widget.
 * @param it The prefs item widget.
 * @return The newly created box widget.
 */
static Evas_Object *
_elm_prefs_page_box_add(Evas_Object *obj,
                        Evas_Object *it)
{
   Evas_Object *sbx;
   double align_x, align_y;

   evas_object_size_hint_align_get(it, &align_x, &align_y);

   sbx = elm_box_add(obj);
   elm_box_horizontal_set(sbx, EINA_TRUE);
   evas_object_size_hint_weight_set(sbx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(sbx, align_x, align_y);
   evas_object_data_set(obj, "sub_box", sbx);
   evas_object_show(sbx);

   return sbx;
}

/**
 * @internal
 * @brief Sets the size hints for a prefs item widget.
 *
 * This function configures the sizing behavior of a prefs item within its
 * container. It sets the weight to expand, ensuring the item fills
 * available space. If the item's interface indicates it wants to expand
 * (`expand_want`), it also sets the alignment to fill.
 *
 * @param it The prefs item widget.
 * @param iface The interface of the prefs item, which may specify sizing preferences.
 */
static void
_elm_prefs_page_item_hints_set(Evas_Object *it,
                          const Elm_Prefs_Item_Iface *iface)
{
   if (iface && iface->expand_want && iface->expand_want(it))
     evas_object_size_hint_align_set(it, EVAS_HINT_FILL, EVAS_HINT_FILL);

   evas_object_size_hint_weight_set(it, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
}

/**
 * @internal
 * @brief Packs a prefs item into a container.
 *
 * This function handles the layout of a prefs item, its optional label, and
 * its optional icon within a container object (`obj`). If an icon is present,
 * a sub-box is created to group the icon and the item widget together.
 * The label, if present, is packed before the item or the item-icon group.
 *
 * @param it The prefs item widget to pack.
 * @param obj The container (a box) to pack the item into.
 * @param iface The interface of the prefs item.
 */
static void
_elm_prefs_page_pack_setup(Evas_Object *it,
                               Evas_Object *obj,
                               const Elm_Prefs_Item_Iface *iface)
{
   Evas_Object *l, *i, *sbx;

   _elm_prefs_page_item_hints_set(it, iface);

   l = evas_object_data_get(it, "label_widget");
   i = evas_object_data_get(it, "icon_widget");
   if (i)
     {
        sbx = _elm_prefs_page_box_add(obj, it);
        elm_box_pack_end(obj, sbx);

        evas_object_size_hint_align_set(it, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_align_set(i, 0.0, EVAS_HINT_FILL);
        elm_box_pack_end(sbx, i);
        elm_box_pack_end(sbx, it);

        if (l) elm_box_pack_before(obj, l, sbx);
     }
   else
     {
        elm_box_pack_end(obj, it);
        if (l) elm_box_pack_before(obj, l, it);
     }
}

/**
 * @internal
 * @brief Packs a prefs item into a container before a reference item.
 *
 * This function is similar to _elm_prefs_page_pack_setup(), but it packs
 * the new item (`it`) and its associated widgets (label, icon) into the
 * container (`obj`) at a position just before another existing item
 * (`it_before`).
 *
 * @param it The prefs item widget to pack.
 * @param it_before The existing item before which to pack the new item.
 * @param obj The container (a box) to pack the item into.
 * @param iface The interface of the prefs item.
 */
static void
_elm_prefs_page_pack_before_setup(Evas_Object *it,
                                      Evas_Object *it_before,
                                      Evas_Object *obj,
                                      const Elm_Prefs_Item_Iface *iface)
{
   Evas_Object *l, *i, *sbx;

   _elm_prefs_page_item_hints_set(it, iface);

   l = evas_object_data_get(it, "label_widget");
   i = evas_object_data_get(it, "icon_widget");
   if (i)
     {
        sbx = _elm_prefs_page_box_add(obj, it);
        elm_box_pack_before(obj, sbx, it_before);

        evas_object_size_hint_align_set(i, EVAS_HINT_FILL, 0.5);
        elm_box_pack_end(sbx, i);
        elm_box_pack_end(sbx, it);

        if (l) elm_box_pack_before(obj, l, sbx);
     }
   else
     {
        elm_box_pack_before(obj, it, it_before);
        if (l) elm_box_pack_before(obj, l, it);
     }
}

/**
 * @internal
 * @brief Packs a prefs item into a container after a reference item.
 *
 * This function is similar to _elm_prefs_page_pack_setup(), but it packs
 * the new item (`it`) and its associated widgets (label, icon) into the
 * container (`obj`) at a position just after another existing item
 * (`it_after`).
 *
 * @param it The prefs item widget to pack.
 * @param it_after The existing item after which to pack the new item.
 * @param obj The container (a box) to pack the item into.
 * @param iface The interface of the prefs item.
 */
static void
_elm_prefs_page_pack_after_setup(Evas_Object *it,
                                     Evas_Object *it_after,
                                     Evas_Object *obj,
                                     const Elm_Prefs_Item_Iface *iface)
{
   Evas_Object *l, *i, *sbx;

   _elm_prefs_page_item_hints_set(it, iface);

   l = evas_object_data_get(it, "label_widget");
   i = evas_object_data_get(it, "icon_widget");
   if (i)
     {
        sbx = _elm_prefs_page_box_add(obj, it);
        elm_box_pack_after(obj, sbx, it_after);

        evas_object_size_hint_align_set(i, EVAS_HINT_FILL, 0.5);
        elm_box_pack_end(sbx, i);
        elm_box_pack_end(sbx, it);

        if (l) elm_box_pack_before(obj, l, sbx);
     }
   else
     {
        elm_box_pack_after(obj, it, it_after);
        if (l) elm_box_pack_before(obj, l, it);
     }
}

/**
 * @internal
 * @brief Common packing logic for items on a horizontal page.
 *
 * This function is a wrapper for packing an item into a page that lays out
 * its children horizontally. It sets appropriate alignment hints for this
 * orientation and then calls the generic packing setup function.
 *
 * @param it The prefs item widget to pack.
 * @param obj The container widget.
 * @param iface The interface of the prefs item.
 */
void
elm_prefs_horizontal_page_common_pack(Evas_Object *it,
                                      Evas_Object *obj,
                                      const Elm_Prefs_Item_Iface *iface)
{
   evas_object_size_hint_align_set(it, 0.5, EVAS_HINT_FILL);
   _elm_prefs_page_pack_setup(it, obj, iface);
}

/**
 * @internal
 * @brief Common packing logic for inserting items on a horizontal page.
 *
 * This function packs an item into a horizontal page layout before a
 * specified existing item.
 *
 * @param it The prefs item widget to pack.
 * @param it_before The existing item before which to pack.
 * @param obj The container widget.
 * @param iface The interface of the prefs item.
 */
void
elm_prefs_horizontal_page_common_pack_before(Evas_Object *it,
                                             Evas_Object *it_before,
                                             Evas_Object *obj,
                                             const Elm_Prefs_Item_Iface *iface)
{
   evas_object_size_hint_align_set(it, 0.5, EVAS_HINT_FILL);
   _elm_prefs_page_pack_before_setup(it, it_before, obj, iface);
}

/**
 * @internal
 * @brief Common packing logic for inserting items on a horizontal page.
 *
 * This function packs an item into a horizontal page layout after a
 * specified existing item.
 *
 * @param it The prefs item widget to pack.
 * @param it_after The existing item after which to pack.
 * @param obj The container widget.
 * @param iface The interface of the prefs item.
 */
void
elm_prefs_horizontal_page_common_pack_after(Evas_Object *it,
                                            Evas_Object *it_after,
                                            Evas_Object *obj,
                                            const Elm_Prefs_Item_Iface *iface)
{
   evas_object_size_hint_align_set(it, 0.5, EVAS_HINT_FILL);
   _elm_prefs_page_pack_after_setup(it, it_after, obj, iface);
}

/**
 * @internal
 * @brief Common packing logic for items on a vertical page.
 *
 * This function is a wrapper for packing an item into a page that lays out
 * its children vertically. It sets appropriate alignment hints for this
 * orientation and then calls the generic packing setup function.
 *
 * @param it The prefs item widget to pack.
 * @param obj The container widget.
 * @param iface The interface of the prefs item.
 */
void
elm_prefs_vertical_page_common_pack(Evas_Object *it,
                                    Evas_Object *obj,
                                    const Elm_Prefs_Item_Iface *iface)
{
   evas_object_size_hint_align_set(it, EVAS_HINT_FILL, 0.5);
   _elm_prefs_page_pack_setup(it, obj, iface);
}

/**
 * @internal
 * @brief Common packing logic for inserting items on a vertical page.
 *
 * This function packs an item into a vertical page layout before a
 * specified existing item.
 *
 * @param it The prefs item widget to pack.
 * @param it_before The existing item before which to pack.
 * @param obj The container widget.
 * @param iface The interface of the prefs item.
 */
void
elm_prefs_vertical_page_common_pack_before(Evas_Object *it,
                                           Evas_Object *it_before,
                                           Evas_Object *obj,
                                           const Elm_Prefs_Item_Iface *iface)
{
   evas_object_size_hint_align_set(it, EVAS_HINT_FILL, 0.5);
   _elm_prefs_page_pack_before_setup(it, it_before, obj, iface);
}

/**
 * @internal
 * @brief Common packing logic for inserting items on a vertical page.
 *
 * This function packs an item into a vertical page layout after a
 * specified existing item.
 *
 * @param it The prefs item widget to pack.
 * @param it_after The existing item after which to pack.
 * @param obj The container widget.
 * @param iface The interface of the prefs item.
 */
void
elm_prefs_vertical_page_common_pack_after(Evas_Object *it,
                                          Evas_Object *it_after,
                                          Evas_Object *obj,
                                          const Elm_Prefs_Item_Iface *iface)
{
   evas_object_size_hint_align_set(it, EVAS_HINT_FILL, 0.5);
   _elm_prefs_page_pack_after_setup(it, it_after, obj, iface);
}

/**
 * @internal
 * @brief Unpacks a prefs item from its container.
 *
 * This function removes a prefs item and its associated widgets (label,
 * icon container) from its parent container. It correctly handles items
 * that were packed with an icon into a sub-box, ensuring all associated
 * layout objects are unpacked and cleaned up.
 *
 * @param it The prefs item widget to unpack.
 * @param obj The container from which to unpack the item.
 */
void
elm_prefs_page_common_unpack(Evas_Object *it,
                             Evas_Object *obj)
{
   Evas_Object *l, *i, *sbx;

   l = evas_object_data_get(it, "label_widget");
   if (l) elm_box_unpack(obj, l);

   sbx = evas_object_data_get(it, "sub_box");
   i = evas_object_data_get(it, "icon_widget");

   if (i && sbx)
     {
        elm_box_unpack_all(sbx);
        elm_box_unpack(obj, sbx);
        evas_object_del(sbx);
     }
   else
     elm_box_unpack(obj, it);
}

EMODAPI int
elm_modapi_init(void *m EINA_UNUSED)
{
   _elm_prefs_log_dom = eina_log_domain_register
       ("elm-prefs", EINA_COLOR_YELLOW);

   elm_prefs_item_iface_register(_elm_prefs_item_widgets);
   elm_prefs_page_iface_register(_elm_prefs_page_widgets);

   return 1; // succeed always
}

EMODAPI int
elm_modapi_shutdown(void *m EINA_UNUSED)
{
   elm_prefs_item_iface_unregister(_elm_prefs_item_widgets);
   elm_prefs_page_iface_unregister(_elm_prefs_page_widgets);

   if (_elm_prefs_log_dom >= 0) eina_log_domain_unregister(_elm_prefs_log_dom);
   _elm_prefs_log_dom = -1;

   return 1; // succeed always
}
