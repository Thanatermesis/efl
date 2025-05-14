#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_separator_eo.h"
#include "elm_widget_separator.h"
#include "elm_widget_layout.h"

#define MY_CLASS ELM_SEPARATOR_CLASS

#define MY_CLASS_NAME "Elm_Separator"
#define MY_CLASS_NAME_LEGACY "elm_separator"

/**
 * @internal
 * @brief Applies the theme to the separator widget.
 *
 * This function sets the theme element based on the separator's orientation
 * (horizontal or vertical) and then calls the parent class's theme apply function.
 *
 * @param[in] obj The Evas object.
 * @param[in] sd The widget data.
 * @return Eina_Error EFL_UI_THEME_APPLY_ERROR_GENERIC on failure, or the result of the parent's theme_apply.
 */
EOLIAN static Eina_Error
_elm_separator_efl_ui_widget_theme_apply(Eo *obj, Elm_Separator_Data *sd EINA_UNUSED)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;

   if (sd->horizontal)
     elm_widget_theme_element_set(obj, "horizontal");
   else
     elm_widget_theme_element_set(obj, "vertical");

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   return int_ret;
}

/**
 * @internal
 * @brief Adds the separator to the canvas group.
 *
 * This function performs initialization steps when the separator is added to a canvas group.
 * It sets the widget to be non-focusable, applies the default theme ("separator/vertical"),
 * and triggers a sizing evaluation.
 *
 * @param[in] obj The Evas object.
 * @param[in] sd The widget data.
 */
EOLIAN static void
_elm_separator_efl_canvas_group_group_add(Eo *obj, Elm_Separator_Data *sd EINA_UNUSED)
{
   efl_canvas_group_add(efl_super(obj, MY_CLASS));
   elm_widget_can_focus_set(obj, EINA_FALSE);

   if (!elm_layout_theme_set
       (obj, "separator", "vertical", elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   elm_layout_sizing_eval(obj);
}

/**
 * @brief Adds a new separator widget to the given parent Evas object.
 *
 * @param parent The parent object.
 * @return The new object or NULL if it cannot be created.
 */
EAPI Evas_Object *
elm_separator_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Constructor for the Elm_Separator object.
 *
 * Initializes the separator object, sets its type, accessibility role,
 * and default size hints.
 *
 * @param[in] obj The Evas object to construct.
 * @param[in] sd The widget data.
 * @return The constructed Evas object.
 */
EOLIAN static Eo *
_elm_separator_efl_object_constructor(Eo *obj, Elm_Separator_Data *sd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_SEPARATOR);
   efl_ui_layout_finger_size_multiplier_set(obj, 0, 0);
   evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);

   return obj;
}

/**
 * @internal
 * @brief Sets the orientation of the separator.
 *
 * @param[in] obj The Evas object.
 * @param[in] sd The widget data.
 * @param[in] horizontal If EINA_TRUE, the separator is horizontal, otherwise vertical.
 */
EOLIAN static void
_elm_separator_horizontal_set(Eo *obj, Elm_Separator_Data *sd, Eina_Bool horizontal)
{
   horizontal = !!horizontal; // Ensure it's a strict EINA_TRUE or EINA_FALSE
   if (sd->horizontal == horizontal) return;

   sd->horizontal = horizontal;

   efl_ui_widget_theme_apply(obj);
}

/**
 * @internal
 * @brief Gets the orientation of the separator.
 *
 * @param[in] obj The Evas object (unused).
 * @param[in] sd The widget data.
 * @return EINA_TRUE if the separator is horizontal, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_separator_horizontal_get(const Eo *obj EINA_UNUSED, Elm_Separator_Data *sd)
{
   return sd->horizontal;
}

/**
 * @internal
 * @brief Class constructor for Elm_Separator.
 *
 * Registers the legacy type name for the class.
 *
 * @param[in] klass The Efl_Class.
 */
EOLIAN static void
_elm_separator_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Internal EO APIs and hidden overrides */

#define ELM_SEPARATOR_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_OPS(elm_separator)

#include "elm_separator_eo.c"
