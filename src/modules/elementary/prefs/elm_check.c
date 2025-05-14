#include "private.h"

/**
 * @internal
 * @brief Lists the data types supported by a check widget in a prefs setting.
 *
 * This array defines that the check widget can handle boolean values, which are
 * represented by the `ELM_PREFS_TYPE_BOOL` type. The prefs system uses this
 * array to validate that an item that uses the "check" widget is of a
 * supported data type. The array must be terminated by `ELM_PREFS_TYPE_UNKNOWN`
 * to signify the end of the list.
 *
 * The structure of elements is a simple array of `Elm_Prefs_Item_Type` enums:
 * @code
 * {
 *    ELM_PREFS_TYPE_BOOL,      // This widget handles boolean types.
 *    ELM_PREFS_TYPE_UNKNOWN    // Terminator for the list of supported types.
 * }
 * @endcode
 */
static Elm_Prefs_Item_Type supported_types[] =
{
   ELM_PREFS_TYPE_BOOL,
   ELM_PREFS_TYPE_UNKNOWN
};

/**
 * @internal
 * @brief Callback for when the check item's state has changed.
 *
 * This function is invoked by the "changed" smart callback of the check
 * widget. It serves as a bridge to the user-provided callback, which is
 * passed as the @p data argument.
 *
 * @param data A pointer to the user-provided callback function
 *             (`Elm_Prefs_Item_Changed_Cb`) that should be executed when the
 *             check's state changes.
 * @param obj The check widget object that triggered the event.
 * @param event_info Unused event information, provided by the Evas smart
 *                   callback mechanism.
 */
static void
_item_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
    Elm_Prefs_Item_Changed_Cb prefs_it_changed_cb = data;

    prefs_it_changed_cb(obj);
}

/**
 * @internal
 * @brief Adds a new check widget to a prefs object.
 *
 * This function implements the `add` operation for the check item interface.
 * It creates a standard `Elm_Check` widget, sets its default state from the
 * provided spec, and registers a callback for state changes.
 *
 * The `spec` parameter's `b.def` field is used to set the initial checked state.
 *
 * @param iface The item interface, marked as unused as this function is
 *              specific to the check widget type.
 * @param prefs The parent prefs widget to which this check item will be added.
 * @param type The item type, unused as the function is specific to the 'check'
 *             widget.
 * @param spec The specification for the item, containing the default boolean
 *             value (`spec.b.def`).
 * @param cb The callback function to be invoked on value change, which is
 *           wrapped by `_item_changed_cb`.
 * @return A new `Evas_Object` representing the check widget.
 */
static Evas_Object *
elm_prefs_check_add(const Elm_Prefs_Item_Iface *iface EINA_UNUSED,
                    Evas_Object *prefs,
                    const Elm_Prefs_Item_Type type EINA_UNUSED,
                    const Elm_Prefs_Item_Spec spec,
                    Elm_Prefs_Item_Changed_Cb cb)
{
   Evas_Object *obj = elm_check_add(prefs);

   evas_object_smart_callback_add(obj, "changed", _item_changed_cb, cb);
   elm_check_state_set(obj, spec.b.def);

   return obj;
}

/**
 * @internal
 * @brief Sets the state of the check widget from an Eina_Value.
 *
 * This function implements the `value_set` operation. It expects the
 * `Eina_Value` to hold a boolean, which in Eina is stored as a `UCHAR`.
 * It then updates the visual state (checked/unchecked) of the widget.
 *
 * @param obj The check widget object whose state is to be set.
 * @param value A pointer to an `Eina_Value` of type `EINA_VALUE_TYPE_UCHAR`.
 *              A value of `1` means checked, `0` unchecked.
 *              Example: An `Eina_Value` containing the `unsigned char` `1`.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., if @p value
 *         has an incompatible type).
 */
static Eina_Bool
elm_prefs_check_value_set(Evas_Object *obj,
                          Eina_Value *value)
{
   Eina_Bool val;

   if (eina_value_type_get(value) != EINA_VALUE_TYPE_UCHAR) return EINA_FALSE;

   eina_value_get(value, &val);
   elm_check_state_set(obj, val);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the state of the check widget and stores it in an Eina_Value.
 *
 * This implements the `value_get` operation. The state is retrieved as a
 * boolean (`Eina_Bool`) and stored in the provided `Eina_Value`, which is set
 * up as `EINA_VALUE_TYPE_UCHAR` to represent the boolean state.
 *
 * @param obj The check widget object from which to get the state.
 * @param value A pointer to an `Eina_Value` where the state will be stored.
 *              The function will set this up as `EINA_VALUE_TYPE_UCHAR`. On
 *              success, it will contain `1` (for `EINA_TRUE`) if the widget is
 *              checked, or `0` (for `EINA_FALSE`) if unchecked.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., if the
 *         `Eina_Value` cannot be set up or the value cannot be set).
 */
static Eina_Bool
elm_prefs_check_value_get(Evas_Object *obj,
                          Eina_Value *value)
{
   Eina_Bool val;

   val = elm_check_state_get(obj);

   if (!eina_value_setup(value, EINA_VALUE_TYPE_UCHAR)) return EINA_FALSE;
   if (!eina_value_set(value, val)) return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Sets the label of the check widget.
 *
 * This implements the `label_set` operation. The label is set on the default
 * text part (`NULL` part name) of the widget's layout.
 *
 * @param obj The check widget object.
 * @param label The text to be used as the label. For example, "Enable Feature".
 * @return @c EINA_TRUE on success, @c EINA_FALSE if the text cannot be set.
 */
static Eina_Bool
elm_prefs_check_label_set(Evas_Object *obj,
                          const char *label)
{
   return elm_layout_text_set(obj, NULL, label);
}

/**
 * @internal
 * @brief Sets the icon of the check widget.
 *
 * This implements the `icon_set` operation. It creates a new standard icon
 * object from the given icon name (looked up in the theme) and sets it as the
 * "icon" content part of the layout. The icon is made non-resizable to
 * maintain a consistent look.
 *
 * @param obj The check widget object.
 * @param icon The name of a standard icon to be set, e.g., "home".
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., if the
 *         icon cannot be created or set as content).
 */
static Eina_Bool
elm_prefs_check_icon_set(Evas_Object *obj,
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
 * @brief Registers the check widget implementation with the prefs system.
 *
 * This macro call populates an `Elm_Prefs_Item_Iface` structure with the
 * functions defined in this file and registers this interface under the name
 * "check". This makes the "check" widget type available for use within an
 * `Elm_Prefs_Data` structure.
 *
 * The arguments to the macro correspond to the fields of the interface:
 * - The name to register ("check").
 * - A list of supported data types (`supported_types`).
 * - A function to set the widget's value (`elm_prefs_check_value_set`).
 * - A function to get the widget's value (`elm_prefs_check_value_get`).
 * - A function to set the widget's label (`elm_prefs_check_label_set`).
 * - A function to set the widget's icon (`elm_prefs_check_icon_set`).
 * - Other function pointers for operations like `value_free`, `editable_set`,
 *   `visible_set`, and the `add` function, some of which are `NULL` if not
 *   implemented or not applicable.
 *
 * @par Example
 *
 * To use this widget implementation in a preferences screen, you would
 * define an item in your `Elm_Prefs_Data` with the `widget` member set to
 * "check".
 *
 * @code
 * // Example of an Elm_Prefs_Data_Info struct for a "check" item.
 * const Elm_Prefs_Data_Info my_check_preference = {
 *   .key = "autosave_enabled",
 *   .type = ELM_PREFS_TYPE_BOOL,
 *   .widget = "check",
 *   .value.b.def = EINA_TRUE, // Default value is 'checked'
 *   .label = "Enable Autosave"
 * };
 * @endcode
 */
PREFS_ITEM_WIDGET_ADD(check,
                      supported_types,
                      elm_prefs_check_value_set,
                      elm_prefs_check_value_get,
                      NULL,
                      elm_prefs_check_label_set,
                      elm_prefs_check_icon_set,
                      NULL,
                      NULL,
                      NULL);
