#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_part_helper.h"

#define MY_CLASS      EFL_UI_TAB_BAR_DEFAULT_ITEM_CLASS

/**
 * @brief Private data structure for the Efl_Ui_Tab_Bar_Default_Item class.
 */
typedef struct {
   Efl_Gfx_Image *icon; /**< The icon object displayed in the tab. */
   const char *icon_name; /**< The name of the icon (e.g., from a theme). */
} Efl_Ui_Tab_Bar_Default_Item_Data;

/**
 * @brief Callback function invoked when the icon setting animation is complete.
 *
 * This function is responsible for finalizing the icon switch after an animation.
 * It replaces the old icon with the new one and cleans up animation-related signals.
 *
 * @param data The Efl_Ui_Item associated with this tab.
 * @param obj The Edje object that emitted the signal.
 * @param emission The emission string of the signal.
 * @param source The source string of the signal.
 */
static void
_tab_icon_set_cb(void *data,
                 Eo *obj,
                 const char *emission,
                 const char *source)
{
   Efl_Ui_Item *ti = data;

   Efl_Ui_Image *new_img = efl_content_get(efl_part(obj, "efl.icon_new"));
   efl_content_set(efl_part(obj, "efl.icon"), new_img); //this must be efl.icon here as obj is the edje object

   efl_layout_signal_callback_del(obj, emission, source, ti, _tab_icon_set_cb, NULL);
   efl_layout_signal_emit(obj, "efl,state,icon,reset", "efl");
}

/**
 * @brief Sets the icon for the tab bar item.
 *
 * If an icon already exists, this function triggers an animation to transition
 * to the new icon. Otherwise, it creates and sets the new icon directly.
 *
 * @param obj The Efl_Ui_Tab_Bar_Default_Item object.
 * @param pd Private data for the object.
 * @param standard_name The name of the icon to set (e.g., "home", "settings").
 */
EOLIAN static void
_efl_ui_tab_bar_default_item_icon_set(Eo *obj, Efl_Ui_Tab_Bar_Default_Item_Data *pd, const char *standard_name)
{
   eina_stringshare_replace(&pd->icon_name, standard_name);

   //if there is a already a icon, create a animation
   if (pd->icon)
     {
        Efl_Ui_Image *new_icon = efl_add(EFL_UI_IMAGE_CLASS, obj);
        efl_content_set(efl_part(obj, "efl.icon_new"), new_icon);
        efl_ui_image_icon_set(new_icon, standard_name);
        efl_layout_signal_emit(obj, "efl,state,icon_new,set", "efl");
        efl_layout_signal_callback_add
          (obj, "efl,state,icon_set,done", "efl", obj, _tab_icon_set_cb, NULL);
     }
   else
     {
        pd->icon = efl_add(EFL_UI_IMAGE_CLASS, obj);
        efl_content_set(efl_part(obj,"icon"), pd->icon);
        efl_ui_image_icon_set(pd->icon, standard_name);
     }
}

/**
 * @brief Gets the name of the icon currently set for the tab bar item.
 *
 * @param obj The Efl_Ui_Tab_Bar_Default_Item object (unused).
 * @param pd Private data for the object.
 * @return The name of the icon, or NULL if no icon is set.
 */
EOLIAN static const char*
_efl_ui_tab_bar_default_item_icon_get(const Eo *obj EINA_UNUSED, Efl_Ui_Tab_Bar_Default_Item_Data *pd)
{
   return pd->icon_name;
}

/**
 * @brief Constructor for the Efl_Ui_Tab_Bar_Default_Item object.
 *
 * Initializes the tab bar item and sets its default theme class.
 *
 * @param obj The Efl_Ui_Tab_Bar_Default_Item object being constructed.
 * @param pd Private data for the object (unused in this function).
 * @return The constructed Eo object.
 */
EOLIAN static Efl_Object *
_efl_ui_tab_bar_default_item_efl_object_constructor(Eo *obj, Efl_Ui_Tab_Bar_Default_Item_Data *pd EINA_UNUSED)
{
   Eo *eo;

   eo = efl_constructor(efl_super(obj, MY_CLASS));

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "tab_bar/tab");

   return eo;
}

#include "efl_ui_tab_bar_default_item.eo.c"
