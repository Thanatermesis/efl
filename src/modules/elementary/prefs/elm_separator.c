#include "private.h"

/**
 * @internal
 * @brief Lists the Elm_Prefs_Item_Type types that this factory supports.
 * @note This is a sentinel-terminated array.
 */
static Elm_Prefs_Item_Type supported_types[] =
{
   ELM_PREFS_TYPE_SEPARATOR,
   ELM_PREFS_TYPE_UNKNOWN
};

/**
 * @internal
 * @brief Creates and adds a separator widget to a prefs object.
 *
 * @param[in] iface The prefs item interface (unused).
 * @param[in] prefs The parent prefs object.
 * @param[in] type The item type (unused).
 * @param[in] spec The item specification (unused).
 * @param[in] cb The changed callback (unused).
 *
 * @return The newly created separator widget, or @c NULL on failure.
 */
static Evas_Object *
elm_prefs_separator_add(const Elm_Prefs_Item_Iface *iface EINA_UNUSED,
                        Evas_Object *prefs,
                        const Elm_Prefs_Item_Type type EINA_UNUSED,
                        const Elm_Prefs_Item_Spec spec EINA_UNUSED,
                        Elm_Prefs_Item_Changed_Cb cb EINA_UNUSED)
{
   Evas_Object *obj = elm_separator_add(prefs);

   return obj;
}

/**
 * @internal
 * @brief Sets the orientation of the separator widget.
 *
 * @param[in] obj The separator widget object.
 * @param[in] value The value to set. The Eina_Value must be of type
 *            EINA_VALUE_TYPE_UCHAR and contain a boolean value,
 *            where EINA_TRUE sets the separator to horizontal, and
 *            EINA_FALSE to vertical.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_separator_value_set(Evas_Object *obj,
                              Eina_Value *value)
{
   Eina_Bool val;

   if (eina_value_type_get(value) != EINA_VALUE_TYPE_UCHAR)
     return EINA_FALSE;

   eina_value_get(value, &val);
   elm_separator_horizontal_set(obj, val);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the orientation of the separator widget.
 *
 * @param[in] obj The separator widget object.
 * @param[out] value A pointer to an Eina_Value to store the orientation.
 *             The value will be of type EINA_VALUE_TYPE_UCHAR, containing
 *             EINA_TRUE if the separator is horizontal, EINA_FALSE otherwise.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_separator_value_get(Evas_Object *obj,
                              Eina_Value *value)
{
   Eina_Bool val = elm_separator_horizontal_get(obj);

   if (!eina_value_setup(value, EINA_VALUE_TYPE_UCHAR))
     return EINA_FALSE;

   if (!eina_value_set(value, val)) return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Defines and registers the separator prefs item widget.
 *
 * This macro call populates an Elm_Prefs_Item_Iface structure with the
 * functions for creating and manipulating a separator widget within the

 * prefs system.
 */
PREFS_ITEM_WIDGET_ADD(separator,
                      supported_types,
                      elm_prefs_separator_value_set,
                      elm_prefs_separator_value_get,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL);
