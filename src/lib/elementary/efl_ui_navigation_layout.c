#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#define EFL_PART_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "efl_ui_navigation_layout_private.h"
#include "elm_part_helper.h"

#define MY_CLASS EFL_UI_NAVIGATION_LAYOUT_CLASS
#define MY_CLASS_NAME "Efl.Ui.Navigation_Layout"

/**
 * @brief Sets the bar part of the navigation layout.
 *
 * This function sets the layout to be used as the bar (typically a header or toolbar)
 * for the navigation layout. The bar is usually displayed at the top.
 *
 * @param[in] obj The Efl.Ui.Navigation_Layout object.
 * @param[in] pd The private data of the Efl.Ui.Navigation_Layout object.
 * @param[in] bar The Efl.Ui.Layout object to set as the bar. Must be a valid Efl.Ui.Layout.
 */
EOLIAN static void
_efl_ui_navigation_layout_bar_set(Eo *obj, Efl_Ui_Navigation_Layout_Data *pd, Efl_Ui_Layout *bar)
{
   EINA_SAFETY_ON_FALSE_RETURN(efl_isa(bar, EFL_UI_LAYOUT_BASE_CLASS));

   efl_content_set(efl_part(obj, "efl.bar"), bar);
   pd->bar = bar;
}

/**
 * @brief Gets the bar part of the navigation layout.
 *
 * This function retrieves the layout currently set as the bar for the navigation layout.
 *
 * @param[in] obj The Efl.Ui.Navigation_Layout object. (Unused)
 * @param[in] pd The private data of the Efl.Ui.Navigation_Layout object.
 * @return The Efl.Ui.Layout object used as the bar, or NULL if none is set.
 */
EOLIAN static Efl_Ui_Layout *
_efl_ui_navigation_layout_bar_get(const Eo *obj EINA_UNUSED, Efl_Ui_Navigation_Layout_Data *pd)
{
   return pd->bar;
}

/**
 * @brief Constructor for the Efl.Ui.Navigation_Layout object.
 *
 * This function is called when a new Efl.Ui.Navigation_Layout object is created.
 * It initializes the widget, sets its theme, and prepares it for use.
 *
 * @param[in] obj The Efl.Ui.Navigation_Layout object being constructed.
 * @param[in] pd The private data of the Efl.Ui.Navigation_Layout object. (Unused for initial construction logic here)
 * @return The constructed Efl.Ui.Navigation_Layout object, or NULL on failure.
 */
EOLIAN static Eo *
_efl_ui_navigation_layout_efl_object_constructor(Eo *obj, Efl_Ui_Navigation_Layout_Data *pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "navigation_layout");
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME);

   elm_widget_can_focus_set(obj, EINA_TRUE);
   if (elm_widget_theme_object_set(obj, wd->resize_obj,
                                       elm_widget_theme_klass_get(obj),
                                       elm_widget_theme_element_get(obj),
                                       elm_widget_theme_style_get(obj)) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     CRI("Failed to set layout!");

   return obj;
}

/* Standard widget overrides */
ELM_PART_CONTENT_DEFAULT_IMPLEMENT(efl_ui_navigation_layout, Efl_Ui_Navigation_Layout_Data)

#include "efl_ui_navigation_layout.eo.c"
