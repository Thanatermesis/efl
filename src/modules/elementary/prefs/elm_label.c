#include "private.h"

/**
 * @internal
 * @brief The types of Efl.Ui.Prefs_Item this implementation can handle.
 *
 * @see Elm_Prefs_Item_Type
 */
static Elm_Prefs_Item_Type supported_types[] =
{
   ELM_PREFS_TYPE_LABEL,
   ELM_PREFS_TYPE_UNKNOWN
};

/**
 * @internal
 * @brief Creates a label widget for a preferences item.
 *
 * This function serves as the factory for creating the Evas_Object (a label)
 * that is displayed in the preferences UI.
 *
 * @param[in] iface The item interface. Unused.
 * @param[in] prefs The parent preferences widget.
 * @param[in] type The type of the item to create. Unused.
 * @param[in] spec The specific configuration for this item. Unused.
 * @param[in] it_changed_cb A callback for value changes, unused for a static
 *                          label.
 *
 * @return The newly created label widget, or @c NULL on failure.
 */
static Evas_Object *
elm_prefs_label_add(const Elm_Prefs_Item_Iface *iface EINA_UNUSED,
                    Evas_Object *prefs,
                    const Elm_Prefs_Item_Type type EINA_UNUSED,
                    const Elm_Prefs_Item_Spec spec EINA_UNUSED,
                    Elm_Prefs_Item_Changed_Cb it_changed_cb EINA_UNUSED)
{
   Evas_Object *obj = elm_label_add(prefs);

   return obj;
}

/**
 * @internal
 * @brief Sets the text label for the preferences item widget.
 *
 * @param[in] obj The label widget object.
 * @param[in] label The text to set as the label.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
elm_prefs_label_label_set(Evas_Object *obj,
                          const char *label)
{
   return elm_layout_text_set(obj, NULL, label);
}

/**
 * @internal
 * @brief Registers the "label" preferences item widget implementation.
 *
 * This macro call registers a new preferences item type named "label".
 * It provides the necessary function pointers for creating the widget and
 * setting its properties, like the label text. Many callbacks are NULL
 * because a simple label is not interactive and has no value to be persisted.
 */
PREFS_ITEM_WIDGET_ADD(label,
                      supported_types,
                      NULL,
                      NULL,
                      NULL,
                      elm_prefs_label_label_set,
                      NULL,
                      NULL,
                      NULL,
                      NULL);
