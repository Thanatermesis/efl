
/**
 * @internal
 * @brief Implements the Efl.Object.constructor for Elm.Grid.
 *
 * This function is called when a new Elm.Grid object is constructed.
 * It is responsible for initializing the object's internal state.
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the object.
 * @return The constructed Eo object, or NULL on failure.
 */
void _elm_grid_grid_size_set(Eo *obj, void *pd, int w, int h);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_grid_size_set, EFL_FUNC_CALL(w, h), int w, int h);

/**
 * @internal
 * @brief Implements elm_obj_grid_size_get for Elm.Grid.
 *
 * Retrieves the virtual width and height of the grid.
 *
 * @param obj The Eo object.
 * @param pd The private data for the object.
 * @param w Pointer to store the virtual width.
 * @param h Pointer to store the virtual height.
 */
void _elm_grid_grid_size_get(const Eo *obj, void *pd, int *w, int *h);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_grid_size_get, EFL_FUNC_CALL(w, h), int *w, int *h);

/**
 * @internal
 * @brief Implements elm_obj_grid_children_get for Elm.Grid.
 *
 * Retrieves a list of all child objects packed into the grid.
 *
 * @param obj The Eo object.
 * @param pd The private data for the object.
 * @return A new Eina_List containing the child objects. The caller is responsible for freeing this list.
 */
Eina_List *_elm_grid_children_get(const Eo *obj, void *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_grid_children_get, Eina_List *, NULL);

/**
 * @internal
 * @brief Implements elm_obj_grid_clear for Elm.Grid.
 *
 * Removes all children from the grid.
 *
 * @param obj The Eo object.
 * @param pd The private data for the object.
 * @param clear If EINA_TRUE, also delete the child objects.
 */
void _elm_grid_clear(Eo *obj, void *pd, Eina_Bool clear);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_grid_clear, EFL_FUNC_CALL(clear), Eina_Bool clear);

/**
 * @internal
 * @brief Implements elm_obj_grid_unpack for Elm.Grid.
 *
 * Removes a specific child object from the grid.
 *
 * @param obj The Eo object.
 * @param pd The private data for the object.
 * @param subobj The child object to remove.
 */
void _elm_grid_unpack(Eo *obj, void *pd, Efl_Canvas_Object *subobj);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_grid_unpack, EFL_FUNC_CALL(subobj), Efl_Canvas_Object *subobj);

/**
 * @internal
 * @brief Implements elm_obj_grid_pack for Elm.Grid.
 *
 * Adds a child object to the grid at a specified position and size.
 *
 * @param obj The Eo object.
 * @param pd The private data for the object.
 * @param subobj The child object to add.
 * @param x The virtual x-coordinate in the grid.
 * @param y The virtual y-coordinate in the grid.
 * @param w The virtual width of the child in the grid.
 * @param h The virtual height of the child in the grid.
 */
void _elm_grid_pack(Eo *obj, void *pd, Efl_Canvas_Object *subobj, int x, int y, int w, int h);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_grid_pack, EFL_FUNC_CALL(subobj, x, y, w, h), Efl_Canvas_Object *subobj, int x, int y, int w, int h);

/**
 * @internal
 * @brief Implements the Efl.Object.constructor for Elm.Grid.
 *
 * This function is called when a new Elm.Grid object is constructed.
 * It is responsible for initializing the object's internal state.
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the object.
 * @return The constructed Eo object, or NULL on failure.
 */
Efl_Object *_elm_grid_efl_object_constructor(Eo *obj, void *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.theme_apply for Elm.Grid.
 *
 * This function is called when the theme is to be applied to the widget.
 * It should update the widget's appearance based on the current theme.
 *
 * @param obj The Eo object.
 * @param pd The private data for the object.
 * @return EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Error _elm_grid_efl_ui_widget_theme_apply(Eo *obj, void *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Focus.Composition.prepare for Elm.Grid.
 *
 * This function is called to prepare the widget for focus composition.
 * It may involve setting up internal elements or states related to focus handling.
 *
 * @param obj The Eo object.
 * @param pd The private data for the object.
 */
void _elm_grid_efl_ui_focus_composition_prepare(Eo *obj, void *pd);

/**
 * @internal
 * @brief Initializes the Elm.Grid Efl_Class.
 *
 * This function is called once when the Elm.Grid class is being set up.
 * It defines the operations (methods) provided by the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_grid_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_GRID_EXTRA_OPS
#define ELM_GRID_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_grid_size_set, _elm_grid_grid_size_set),
      EFL_OBJECT_OP_FUNC(elm_obj_grid_size_get, _elm_grid_grid_size_get),
      EFL_OBJECT_OP_FUNC(elm_obj_grid_children_get, _elm_grid_children_get),
      EFL_OBJECT_OP_FUNC(elm_obj_grid_clear, _elm_grid_clear),
      EFL_OBJECT_OP_FUNC(elm_obj_grid_unpack, _elm_grid_unpack),
      EFL_OBJECT_OP_FUNC(elm_obj_grid_pack, _elm_grid_pack),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_grid_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_grid_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_composition_prepare, _elm_grid_efl_ui_focus_composition_prepare),
      ELM_GRID_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm.Grid Efl_Class.
 *
 * This structure provides metadata for the Elm.Grid class,
 * including its version, name, type, and initializer functions.
 */
static const Efl_Class_Description _elm_grid_class_desc = {
   EO_VERSION, /**< EO_VERSION */
   "Elm.Grid", /**< Class name */
   EFL_CLASS_TYPE_REGULAR, /**< Class type */
   0,
   _elm_grid_class_initializer,
   _elm_grid_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_grid_class_get, &_elm_grid_class_desc, EFL_UI_WIDGET_CLASS, EFL_UI_FOCUS_COMPOSITION_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_grid_eo.legacy.c"
