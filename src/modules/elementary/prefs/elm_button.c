#include "private.h"

/**
 * @internal
 * @brief Lists the Elm_Prefs_Item_Type values that this widget supports.
 * @details This array defines the types of buttons that can be created using this
 * prefs item implementation. The supported types are for action, reset, and save
 * buttons. The list is terminated by ELM_PREFS_TYPE_UNKNOWN.
 *
 * The array elements are:
 * - @c ELM_PREFS_TYPE_ACTION: A generic action button.
 * - @c ELM_PREFS_TYPE_RESET: A button to reset preferences to their default values.
 * - @c ELM_PREFS_TYPE_SAVE: A button to save the current preferences.
 * - @c ELM_PREFS_TYPE_UNKNOWN: Sentinel value to mark the end of the array.
 */
static Elm_Prefs_Item_Type supported_types[] =
{
   ELM_PREFS_TYPE_ACTION,
   ELM_PREFS_TYPE_RESET,
   ELM_PREFS_TYPE_SAVE,
   ELM_PREFS_TYPE_UNKNOWN
};

/**
 * @internal
 * @brief Callback for the "clicked" event on the button widget.
 * @details This function is a wrapper that forwards the "clicked" event from the
 * Elementary button to the user-provided `Elm_Prefs_Item_Changed_Cb` callback.
 *
 * @param data The user-provided callback function (`Elm_Prefs_Item_Changed_Cb`).
 * @param obj The Evas_Object that was clicked (the button).
 * @param event_info Unused event information.
 */
static void
_item_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Elm_Prefs_Item_Changed_Cb prefs_it_changed_cb = data;

   prefs_it_changed_cb(obj);
}

/**
 * @internal
 * @brief Creates a new button widget for a prefs item.
 * @details This function is part of the Elm_Prefs_Item_Iface implementation for
 * buttons. It creates an `elm_button` widget and sets up a callback for when it is
 * clicked.
 *
 * @param iface The prefs item interface (unused).
 * @param prefs The parent prefs widget.
 * @param type The type of button to create (unused, as this implementation
 *        handles multiple types).
 * @param spec The specification for the item (unused).
 * @param cb The callback function to be invoked when the button is clicked.
 * @return The newly created button widget as an Evas_Object, or @c NULL on
 *         failure.
 */
static Evas_Object *
elm_prefs_button_add(const Elm_Prefs_Item_Iface *iface EINA_UNUSED,
                     Evas_Object *prefs,
                     const Elm_Prefs_Item_Type type EINA_UNUSED,
                     const Elm_Prefs_Item_Spec spec EINA_UNUSED,
                     Elm_Prefs_Item_Changed_Cb cb)
{
   Evas_Object *obj = elm_button_add(prefs);

   evas_object_smart_callback_add(obj, "clicked", _item_changed_cb, cb);

   return obj;
}

/**
 * @internal
 * @brief Sets the label of the button widget.
 * @details This function is part of the Elm_Prefs_Item_Iface implementation for
 * buttons. It sets the text label of the button.
 *
 * @param obj The button widget.
 * @param label The text to set as the button's label. e.g., "Save".
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_button_label_set(Evas_Object *obj,
                           const char *label)
{
   return elm_layout_text_set(obj, NULL, label);
}

/**
 * @internal
 * @brief Sets the icon of the button widget.
 * @details This function is part of the Elm_Prefs_Item_Iface implementation for
 * buttons. It creates an icon object from a standard icon name and sets it
 * as content on the button. The icon is set to be non-resizable.
 * If setting the icon fails, the created icon object is deleted to prevent
 * memory leaks.
 *
 * @param obj The button widget.
 * @param icon The name of a standard icon to use. e.g., "document-save".
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_button_icon_set(Evas_Object *obj,
                          const char *icon)
{
   Evas_Object *ic = elm_icon_add(obj);
   Eina_Bool ret;

   if (!elm_icon_standard_set(ic, icon)) goto err;

   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);

   ret = elm_layout_content_set(obj, "icon", ic);
   if (!ret) goto err;

   return ret;

err:
   evas_object_del(ic);
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Registers the button widget implementation with the Elm_Prefs system.
 * @details This macro expands to a struct that holds the function pointers and
 * data needed to manage a button prefs item. It effectively registers this
 * implementation under the "button" name.
 */
PREFS_ITEM_WIDGET_ADD(button,
                      supported_types,
                      NULL,
                      NULL,
                      NULL,
                      elm_prefs_button_label_set,
                      elm_prefs_button_icon_set,
                      NULL,
                      NULL,
                      NULL);
