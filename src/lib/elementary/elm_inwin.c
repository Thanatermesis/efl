#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_WIDGET_PROTECTED
#define EFL_ACCESS_OBJECT_PROTECTED
#define ELM_LAYOUT_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_inwin_eo.h"
#include "elm_widget_inwin.h"
#include "elm_widget_layout.h"
#include "elm_part_helper.h"

#define MY_CLASS ELM_INWIN_CLASS
#define MY_CLASS_PFX elm_inwin

#define MY_CLASS_NAME "Elm_Inwin"
#define MY_CLASS_NAME_LEGACY "elm_inwin"

/**
 * @brief Private data for the Elm_Inwin widget.
 *
 * This structure holds the private data members for an Elm_Inwin instance.
 * Currently, it is empty but is defined for future extensions and to
 * maintain consistency with other Elementary widget patterns.
 */
typedef struct {

} Elm_Inwin_Data;

/**
 * @brief Calculates the size of the inwin object.
 * @internal
 *
 * This function is an Efl_Canvas_Group_Group_Calculate implementation for Elm_Inwin.
 * It triggers a size calculation of the superclass if the inwin has content.
 *
 * @param[in] obj The Evas object (Elm_Inwin instance).
 * @param[in] pd The private data of the Elm_Inwin instance.
 */
EOLIAN static void
_elm_inwin_efl_canvas_group_group_calculate(Eo *obj, Elm_Inwin_Data *pd EINA_UNUSED)
{
   if (elm_layout_content_get(obj, NULL))
     efl_canvas_group_calculate(efl_super(obj, MY_CLASS));
}

/**
 * @brief Adds the inwin object to the canvas.
 * @internal
 *
 * This function is an Efl_Canvas_Group_Group_Add implementation for Elm_Inwin.
 * It performs initialization common to Elm_Inwin objects when they are added
 * to the canvas, such as setting focus properties, size hints, and the theme.
 *
 * @param[in] obj The Evas object (Elm_Inwin instance).
 * @param[in] pd The private data of the Elm_Inwin instance.
 */
EOLIAN static void
_elm_inwin_efl_canvas_group_group_add(Eo *obj, Elm_Inwin_Data *pd EINA_UNUSED)
{
   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   elm_widget_can_focus_set(obj, EINA_FALSE);
   elm_widget_highlight_ignore_set(obj, EINA_TRUE);

   evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
   if (!elm_layout_theme_set(obj, "win", "inwin", elm_object_style_get(obj)))
     CRI("Failed to set layout!");
}

/**
 * @brief Adds a new inwin object to the given parent window.
 *
 * This function creates and returns a new inwin widget. The @p parent
 * must be an @ref Win "elm_win" object.
 *
 * @param[in] parent The parent Evas object (must be an Elm_Win).
 * @return The new Evas_Object (Elm_Inwin instance) or @c NULL on failure.
 *
 * @see elm_object_style_set() to set the style.
 * @see elm_win_inwin_content_set() to set the content.
 * @see elm_win_inwin_activate() to show and raise the inwin.
 *
 * @ingroup Elm_Inwin_Group
 */
EAPI Evas_Object *
elm_win_inwin_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @brief Constructor for the Elm_Inwin Eolian object.
 * @internal
 *
 * This function is called when a new Elm_Inwin object is constructed.
 * It validates the parent, calls the superclass constructor, adds the
 * inwin to the parent window's resize objects, and sets initial properties.
 *
 * @param[in] obj The Evas object (Elm_Inwin instance) being constructed.
 * @param[in] pd The private data of the Elm_Inwin instance.
 * @return The constructed Evas_Object (Elm_Inwin instance) or @c NULL on failure.
 */
EOLIAN static Eo *
_elm_inwin_efl_object_constructor(Eo *obj, Elm_Inwin_Data *pd EINA_UNUSED)
{
   Evas_Object *parent = NULL;

   parent = efl_parent_get(obj);

   if (parent && !efl_isa(parent, EFL_UI_WIN_CLASS))
     {
        ERR("Failed");
        return NULL;
     }

   obj = efl_constructor(efl_super(obj, MY_CLASS));
   elm_win_resize_object_add(efl_parent_get(obj), obj);
   elm_layout_sizing_eval(obj);
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_GLASS_PANE);

   return obj;
}

/**
 * @brief Activates the inwin object.
 * @internal
 *
 * This function makes the inwin visible, raises it to the top of its
 * parent window's stacking order, and attempts to set focus to it.
 * It also emits a signal "elm,action,show" on the inwin's resize object.
 * This is typically called to show the inwin after it has been hidden or
 * to bring it to the front.
 *
 * @param[in] obj The Evas object (Elm_Inwin instance).
 * @param[in] pd The private data of the Elm_Inwin instance.
 */
EOLIAN static void
_elm_inwin_activate(Eo *obj, Elm_Inwin_Data *pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (elm_widget_disabled_get(obj)) return;

   evas_object_raise(obj);
   evas_object_show(obj);
   edje_object_signal_emit
     (wd->resize_obj, "elm,action,show", "elm");
   elm_object_focus_set(obj, EINA_TRUE);
}

/**
 * @brief Sets the content of an inwin object.
 *
 * This function sets the main content object for the inwin. The content
 * will be displayed within the inwin.
 *
 * @param[in] obj The inwin object.
 * @param[in] content The content Evas_Object to set.
 *
 * @ingroup Elm_Inwin_Group
 */
EAPI void
elm_win_inwin_content_set(Evas_Object *obj, Evas_Object *content)
{
   ELM_INWIN_CHECK(obj);
   efl_content_set(obj, content);
}

/**
 * @brief Gets the content of an inwin object.
 *
 * This function returns the main content object that was previously set
 * using elm_win_inwin_content_set().
 *
 * @param[in] obj The inwin object.
 * @return The content Evas_Object, or @c NULL if no content is set or on error.
 *
 * @ingroup Elm_Inwin_Group
 */
EAPI Evas_Object *
elm_win_inwin_content_get(const Evas_Object *obj)
{
   ELM_INWIN_CHECK(obj) NULL;
   return efl_content_get(obj);
}

/**
 * @brief Unsets the content of an inwin object.
 *
 * This function removes and returns the main content object from the inwin.
 * The caller is responsible for managing the lifecycle of the returned object
 * (e.g., deleting it if it's no longer needed).
 *
 * @param[in] obj The inwin object.
 * @return The previously set content Evas_Object, or @c NULL if no content was set or on error.
 *
 * @ingroup Elm_Inwin_Group
 */
EAPI Evas_Object *
elm_win_inwin_content_unset(Evas_Object *obj)
{
   ELM_INWIN_CHECK(obj) NULL;
   return efl_content_unset(obj);
}

/**
 * @brief Class constructor for Elm_Inwin.
 * @internal
 *
 * This function is called once when the Elm_Inwin class is being set up.
 * It registers the legacy Evas smart type for Elm_Inwin.
 *
 * @param[in] klass The Efl_Class for Elm_Inwin.
 */
static void
_elm_inwin_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Internal EO APIs and hidden overrides */

ELM_PART_CONTENT_DEFAULT_IMPLEMENT(elm_inwin, Elm_Inwin_Data)

#define ELM_INWIN_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_OPS(elm_inwin), \
   EFL_CANVAS_GROUP_CALC_OPS(elm_inwin)

#include "elm_inwin_eo.c"
