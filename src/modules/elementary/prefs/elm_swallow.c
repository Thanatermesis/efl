#include "private.h"

/**
 * @internal
 * @brief The item types supported by this widget implementation.
 *
 * This array lists the Elm_Prefs_Item_Type values that this swallow
 * item interface can handle. It is terminated by ELM_PREFS_TYPE_UNKNOWN.
 */
static Elm_Prefs_Item_Type supported_types[] =
{
   ELM_PREFS_TYPE_SWALLOW,
   ELM_PREFS_TYPE_UNKNOWN
};

/**
 * @internal
 *
 * @brief Adds a new swallow widget to the prefs object.
 *
 * @param iface The item interface.
 * @param prefs The prefs widget to add the swallow item to.
 * @param type The item type.
 * @param spec The item specification.
 * @param cb The changed callback.
 *
 * @return The new swallow widget.
 */
static Evas_Object *
elm_prefs_swallow_add(const Elm_Prefs_Item_Iface *iface EINA_UNUSED,
                      Evas_Object *prefs,
                      const Elm_Prefs_Item_Type type EINA_UNUSED,
                      const Elm_Prefs_Item_Spec spec EINA_UNUSED,
                      Elm_Prefs_Item_Changed_Cb cb EINA_UNUSED)
{
   Evas_Object *obj = elm_layout_add(prefs);
   char layout_edj[PATH_MAX];

   snprintf(layout_edj, sizeof(layout_edj), "%s/elementary/modules/prefs/%s/elm_prefs_swallow.edj", elm_app_lib_dir_get(), MODULE_ARCH);

   elm_layout_file_set(obj, layout_edj, "elm_prefs_swallow");

   return obj;
}

/**
 * @internal
 *
 * @brief Swallows a sub-object into the swallow widget.
 *
 * This function takes a value which is expected to contain an Evas_Object
 * and sets it as the content of the swallow layout.
 *
 * @param obj The swallow widget.
 * @param value A pointer to an Eina_Value of type EINA_VALUE_TYPE_UINT64,
 *              which holds the Evas_Object* to be swallowed.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_swallow_swallow(Evas_Object *obj,
                          Eina_Value *value)
{
   Evas_Object *subobj;

   if (eina_value_type_get(value) != EINA_VALUE_TYPE_UINT64 ||
       !eina_value_get(value, &subobj))
     return EINA_FALSE;

   elm_layout_content_set(obj, "content", subobj);

   return EINA_TRUE;
}

/**
 * @internal
 *
 * @brief Unswallows a sub-object from the swallow widget.
 *
 * This function unsets the content from the swallow layout and stores
 * the previously swallowed Evas_Object back into the provided Eina_Value.
 *
 * @param obj The swallow widget.
 * @param value A pointer to an Eina_Value that will be set up to hold the
 *              unswallowed Evas_Object*.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
elm_prefs_swallow_unswallow(Evas_Object *obj,
                            Eina_Value *value)
{
   Evas_Object *subobj = elm_layout_content_unset(obj, "content");

   if (!eina_value_setup(value, EINA_VALUE_TYPE_UINT64) ||
       !eina_value_set(value, subobj))
     return EINA_FALSE;

   return EINA_TRUE;
}

PREFS_ITEM_WIDGET_ADD(swallow,
                      supported_types,
                      elm_prefs_swallow_swallow,
                      elm_prefs_swallow_unswallow,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL);
