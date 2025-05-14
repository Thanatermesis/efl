#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_UI_FOCUS_COMPOSITION_PROTECTED

#include <Elementary.h>
#include <elm_grid_eo.h>
#include "elm_priv.h"
#include "elm_widget_grid.h"

#define MY_CLASS ELM_GRID_CLASS
#define MY_CLASS_NAME "Elm_Grid"
#define MY_CLASS_NAME_LEGACY "elm_grid"

/**
 * @internal
 * @brief Prepares the focus composition for the grid.
 *
 * This function retrieves the children of the grid, filters out non-widget
 * elements, and sets the remaining widgets as the focus composition elements.
 * This is necessary for managing focus order within the grid.
 *
 * @param obj The Evas object (grid).
 * @param pd Private data (unused).
 */
static void
_elm_grid_efl_ui_focus_composition_prepare(Eo *obj, void *pd EINA_UNUSED)
{
   Eina_List *l, *ll;
   Efl_Ui_Widget *elem;

   Elm_Widget_Smart_Data *wpd = efl_data_scope_get(obj, EFL_UI_WIDGET_CLASS);
   Eina_List *order = evas_object_grid_children_get(wpd->resize_obj);

   EINA_LIST_FOREACH_SAFE(order, l, ll, elem)
     {
        if (!efl_isa(elem, EFL_UI_WIDGET_CLASS))
          order = eina_list_remove(order, elem);
     }

   efl_ui_focus_composition_elements_set(obj, order);
}

/**
 * @internal
 * @brief Sets the mirrored mode of the grid.
 *
 * This function is a helper to apply the mirrored (right-to-left)
 * setting to the underlying Evas grid object.
 *
 * @param obj The Evas object (grid).
 * @param rtl EINA_TRUE if right-to-left mode is enabled, EINA_FALSE otherwise.
 */
static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_grid_mirrored_set(wd->resize_obj, rtl);
}

/**
 * @internal
 * @brief Applies the theme to the grid widget.
 *
 * This function calls the superclass's theme apply function and then
 * applies mirroring settings based on the current theme and widget state.
 *
 * @param obj The Evas object (grid).
 * @param sd Private data (unused).
 * @return Eina_Error Standard Efl_Ui_Theme_Apply error codes.
 */
EOLIAN static Eina_Error
_elm_grid_efl_ui_widget_theme_apply(Eo *obj, void *sd EINA_UNUSED)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   _mirrored_set(obj, efl_ui_mirrored_get(obj));

   return int_ret;
}

/**
 * @internal
 * @brief Handles the addition of the grid to a canvas.
 *
 * This function initializes the internal Evas grid object, sets its default
 * virtual size (100x100), and configures initial widget properties like focus.
 *
 * @param obj The Evas object (grid).
 * @param _pd Private data (unused).
 */
EOLIAN static void
_elm_grid_efl_canvas_group_group_add(Eo *obj, void *_pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   Evas_Object *grid;

   grid = evas_object_grid_add(evas_object_evas_get(obj));
   elm_widget_resize_object_set(obj, grid);
   evas_object_grid_size_set(wd->resize_obj, 100, 100);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   elm_widget_can_focus_set(obj, EINA_FALSE);

   efl_ui_widget_theme_apply(obj);
}

/**
 * @internal
 * @brief Handles the deletion of the grid from a canvas.
 *
 * This function ensures that the internal Evas grid object (resize_obj)
 * is processed last during deletion. This is important because the grid
 * might be the smart parent of other sub-objects, and destroying it
 * prematurely could lead to issues. It achieves this by moving the
 * resize_obj to the end of the children list before calling the superclass's
 * group_del.
 *
 * @param obj The Evas object (grid).
 * @param _pd Private data (unused).
 */
EOLIAN static void
_elm_grid_efl_canvas_group_group_del(Eo *obj, void *_pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   /* let's make our grid object the *last* to be processed, since it
    * may (smart) parent other sub objects here */
   {
      unsigned int resize_id = 0;
      if (eina_array_find(wd->children, wd->resize_obj, &resize_id))
        {
           //exchange with last
           eina_array_data_set(wd->children, resize_id, eina_array_data_get(wd->children, eina_array_count(wd->children) - 1));
           eina_array_data_set(wd->children, eina_array_count(wd->children) - 1, wd->resize_obj);
        }
   }

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

EAPI Evas_Object *
elm_grid_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Constructor for the Elm_Grid object.
 *
 * Initializes the grid object, sets its legacy type name, and
 * assigns its accessibility role.
 *
 * @param obj The Evas object (grid).
 * @param _pd Private data (unused).
 * @return The constructed Evas object.
 */
EOLIAN static Eo *
_elm_grid_efl_object_constructor(Eo *obj, void *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_FILLER);

   return obj;
}

/**
 * @internal
 * @brief Sets the virtual size of the grid.
 *
 * The children of the grid are placed and sized relative to this
 * virtual resolution. For example, if the virtual size is 100x100,
 * a child packed at x=50, y=50 will be centered.
 *
 * @param obj The Evas object (grid).
 * @param _pd Private data (unused).
 * @param w The virtual width.
 * @param h The virtual height.
 */
EOLIAN static void
_elm_grid_grid_size_set(Eo *obj, void *_pd EINA_UNUSED, Evas_Coord w, Evas_Coord h)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_grid_size_set(wd->resize_obj, w, h);
}

/**
 * @internal
 * @brief Gets the virtual size of the grid.
 *
 * @param obj The Evas object (grid).
 * @param _pd Private data (unused).
 * @param w Pointer to store the virtual width.
 * @param h Pointer to store the virtual height.
 */
EOLIAN static void
_elm_grid_grid_size_get(const Eo *obj, void *_pd EINA_UNUSED, Evas_Coord *w, Evas_Coord *h)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_grid_size_get(wd->resize_obj, w, h);
}

/**
 * @internal
 * @brief Packs a sub-object into the grid.
 *
 * Adds the sub-object as a child of the grid and positions/sizes it
 * according to the provided virtual coordinates and dimensions.
 * Also marks the focus composition as dirty.
 *
 * @param obj The Evas object (grid).
 * @param _pd Private data (unused).
 * @param subobj The sub-object to pack.
 * @param x The virtual x-coordinate.
 * @param y The virtual y-coordinate.
 * @param w The virtual width.
 * @param h The virtual height.
 */
EOLIAN static void
_elm_grid_pack(Eo *obj, void *_pd EINA_UNUSED, Evas_Object *subobj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   elm_widget_sub_object_add(obj, subobj);
   evas_object_grid_pack(wd->resize_obj, subobj, x, y, w, h);
   efl_ui_focus_composition_dirty(obj);
}

/**
 * @internal
 * @brief Unpacks a sub-object from the grid.
 *
 * Removes the sub-object from the grid's layout management.
 * Before unpacking from the Evas grid, it redirects the sub-object's
 * parentage to ensure proper cleanup if it was an Elm widget.
 * Also marks the focus composition as dirty.
 *
 * @param obj The Evas object (grid).
 * @param _pd Private data (unused).
 * @param subobj The sub-object to unpack.
 */
EOLIAN static void
_elm_grid_unpack(Eo *obj, void *_pd EINA_UNUSED, Evas_Object *subobj)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   _elm_widget_sub_object_redirect_to_top(obj, subobj);
   evas_object_grid_unpack(wd->resize_obj, subobj);
   efl_ui_focus_composition_dirty(obj);
}

/**
 * @internal
 * @brief Clears the grid, removing all packed sub-objects.
 *
 * If `clear` is EINA_FALSE, it first redirects the parentage of Elm widget
 * children to ensure they are properly unparented before being removed
 * from the Evas grid. If `clear` is EINA_TRUE, the Evas grid directly
 * deletes the children. Marks focus composition as dirty.
 *
 * @param obj The Evas object (grid).
 * @param _pd Private data (unused).
 * @param clear If EINA_TRUE, sub-objects are deleted. If EINA_FALSE,
 *              they are just unpacked (and potentially reparented if Elm widgets).
 */
EOLIAN static void
_elm_grid_clear(Eo *obj, void *_pd EINA_UNUSED, Eina_Bool clear)
{
   Eina_List *chld;
   Evas_Object *o;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (!clear)
     {
        chld = evas_object_grid_children_get(wd->resize_obj);
        EINA_LIST_FREE(chld, o)
          _elm_widget_sub_object_redirect_to_top(obj, o);
     }

   evas_object_grid_clear(wd->resize_obj, clear);
   efl_ui_focus_composition_dirty(obj);
}

EAPI void
elm_grid_pack_set(Evas_Object *subobj,
                  Evas_Coord x,
                  Evas_Coord y,
                  Evas_Coord w,
                  Evas_Coord h)
{
   Evas_Object *obj = elm_widget_parent_widget_get(subobj);

   ELM_GRID_CHECK(obj);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_grid_pack(wd->resize_obj, subobj, x, y, w, h);
   efl_ui_focus_composition_dirty(obj);
}

EAPI void
elm_grid_pack_get(Evas_Object *subobj,
                  int *x,
                  int *y,
                  int *w,
                  int *h)
{
   Evas_Object *obj = elm_widget_parent_widget_get(subobj);

   ELM_GRID_CHECK(obj);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_grid_pack_get
     (wd->resize_obj, subobj, x, y, w, h);
}

/**
 * @internal
 * @brief Gets the list of children packed into the grid.
 *
 * @param obj The Evas object (grid).
 * @param _pd Private data (unused).
 * @return A list of Evas_Object children. The list itself should not be modified
 *         by the caller and is valid as long as the grid children are not changed.
 *         It may be an EINA_LIST_EMPTY if there are no children.
 *         Example of iterating:
 *         Eina_List *children, *l;
 *         Evas_Object *child_obj;
 *         children = _elm_grid_children_get(grid_obj, NULL);
 *         EINA_LIST_FOREACH(children, l, child_obj) {
 *            // process child_obj
 *         }
 *         // Do not eina_list_free(children) if it's from evas_object_grid_children_get
 */
EOLIAN static Eina_List*
_elm_grid_children_get(const Eo *obj, void *_pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);
   return evas_object_grid_children_get(wd->resize_obj);
}

/**
 * @internal
 * @brief Class constructor for Elm_Grid.
 *
 * Registers the legacy type name for the Elm_Grid class. This is
 * important for backward compatibility and for Evas smart object
 * handling.
 *
 * @param klass The Efl_Class for Elm_Grid.
 */
static void
_elm_grid_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Internal EO APIs and hidden overrides */

#define ELM_GRID_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_grid)

#include "elm_grid_eo.c"
