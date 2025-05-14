/**
 * @internal
 * @brief Implements the evas_obj_grid_size_set logic.
 * @param obj The Evas_Grid object.
 * @param pd Private data for the Evas_Grid object.
 * @param w The virtual horizontal size.
 * @param h The virtual vertical size.
 */
void _evas_grid_grid_size_set(Eo *obj, Evas_Grid_Data *pd, int w, int h);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_grid_size_set, EFL_FUNC_CALL(w, h), int w, int h);

/**
 * @internal
 * @brief Implements the evas_obj_grid_size_get logic.
 * @param obj The Evas_Grid object.
 * @param pd Private data for the Evas_Grid object.
 * @param[out] w Pointer to store the virtual horizontal size.
 * @param[out] h Pointer to store the virtual vertical size.
 */
void _evas_grid_grid_size_get(const Eo *obj, Evas_Grid_Data *pd, int *w, int *h);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_grid_size_get, EFL_FUNC_CALL(w, h), int *w, int *h);

/**
 * @internal
 * @brief Implements the evas_obj_grid_children_get logic.
 * @param obj The Evas_Grid object.
 * @param pd Private data for the Evas_Grid object.
 * @return A list of child objects. The caller is responsible for freeing this list.
 */
Eina_List *_evas_grid_children_get(const Eo *obj, Evas_Grid_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_grid_children_get, Eina_List *, NULL);

/**
 * @internal
 * @brief Implements the evas_obj_grid_accessor_new logic.
 * @param obj The Evas_Grid object.
 * @param pd Private data for the Evas_Grid object.
 * @return An accessor for the grid's children. The caller is responsible for freeing this accessor.
 */
Eina_Accessor *_evas_grid_accessor_new(const Eo *obj, Evas_Grid_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_grid_accessor_new, Eina_Accessor *, NULL);

/**
 * @internal
 * @brief Implements the evas_obj_grid_clear logic.
 * @param obj The Evas_Grid object.
 * @param pd Private data for the Evas_Grid object.
 * @param clear If EINA_TRUE, also delete the children.
 */
void _evas_grid_clear(Eo *obj, Evas_Grid_Data *pd, Eina_Bool clear);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_grid_clear, EFL_FUNC_CALL(clear), Eina_Bool clear);

/**
 * @internal
 * @brief Implements the evas_obj_grid_iterator_new logic.
 * @param obj The Evas_Grid object.
 * @param pd Private data for the Evas_Grid object.
 * @return An iterator for the grid's children. The caller is responsible for freeing this iterator.
 */
Eina_Iterator *_evas_grid_iterator_new(const Eo *obj, Evas_Grid_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_grid_iterator_new, Eina_Iterator *, NULL);

/**
 * @internal
 * @brief Implements the evas_obj_grid_add_to logic.
 * @param obj The Evas_Grid object to which a new grid will be added as a child.
 * @param pd Private data for the Evas_Grid object.
 * @return The new Efl_Canvas_Object grid, or NULL on failure.
 */
Efl_Canvas_Object *_evas_grid_add_to(Eo *obj, Evas_Grid_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY(evas_obj_grid_add_to, Efl_Canvas_Object *, NULL);

/**
 * @internal
 * @brief Implements the evas_obj_grid_unpack logic.
 * @param obj The Evas_Grid object.
 * @param pd Private data for the Evas_Grid object.
 * @param child The child object to remove from the grid.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool _evas_grid_unpack(Eo *obj, Evas_Grid_Data *pd, Efl_Canvas_Object *child);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_grid_unpack, Eina_Bool, 0, EFL_FUNC_CALL(child), Efl_Canvas_Object *child);

/**
 * @internal
 * @brief Implements the evas_obj_grid_pack_get logic.
 * @param obj The Evas_Grid object.
 * @param pd Private data for the Evas_Grid object.
 * @param child The child object to query.
 * @param[out] x Pointer to store the virtual x coordinate.
 * @param[out] y Pointer to store the virtual y coordinate.
 * @param[out] w Pointer to store the virtual width.
 * @param[out] h Pointer to store the virtual height.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool _evas_grid_pack_get(const Eo *obj, Evas_Grid_Data *pd, Efl_Canvas_Object *child, int *x, int *y, int *w, int *h);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV_CONST(evas_obj_grid_pack_get, Eina_Bool, 0, EFL_FUNC_CALL(child, x, y, w, h), Efl_Canvas_Object *child, int *x, int *y, int *w, int *h);

/**
 * @internal
 * @brief Implements the evas_obj_grid_pack logic.
 * @param obj The Evas_Grid object.
 * @param pd Private data for the Evas_Grid object.
 * @param child The child object to add to the grid.
 * @param x The virtual x coordinate.
 * @param y The virtual y coordinate.
 * @param w The virtual width.
 * @param h The virtual height.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool _evas_grid_pack(Eo *obj, Evas_Grid_Data *pd, Efl_Canvas_Object *child, int x, int y, int w, int h);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV(evas_obj_grid_pack, Eina_Bool, 0, EFL_FUNC_CALL(child, x, y, w, h), Efl_Canvas_Object *child, int x, int y, int w, int h);

/**
 * @internal
 * @brief Constructor for Evas_Grid objects.
 *
 * Called when a new Evas_Grid object is instantiated.
 * Performs initial setup for the object.
 *
 * @param obj The Evas_Grid object being constructed.
 * @param pd Private data for the Evas_Grid object.
 * @return The constructed Efl_Object, or NULL on failure.
 */
Efl_Object *_evas_grid_efl_object_constructor(Eo *obj, Evas_Grid_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.I18n.mirrored_set interface method.
 *
 * Sets the mirrored (right-to-left) mode for the grid.
 *
 * @param obj The Evas_Grid object.
 * @param pd Private data for the Evas_Grid object.
 * @param rtl EINA_TRUE if mirrored mode should be enabled, EINA_FALSE otherwise.
 */
void _evas_grid_efl_ui_i18n_mirrored_set(Eo *obj, Evas_Grid_Data *pd, Eina_Bool rtl);

/**
 * @internal
 * @brief Implements the Efl.Ui.I18n.mirrored_get interface method.
 *
 * Gets the mirrored (right-to-left) mode for the grid.
 *
 * @param obj The Evas_Grid object.
 * @param pd Private data for the Evas_Grid object.
 * @return EINA_TRUE if mirrored mode is enabled, EINA_FALSE otherwise.
 */
Eina_Bool _evas_grid_efl_ui_i18n_mirrored_get(const Eo *obj, Evas_Grid_Data *pd);

/**
 * @internal
 * @brief Initializes the Evas_Grid class.
 *
 * This function is called once when the Evas_Grid class is first used.
 * It sets up the operations (methods) for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_evas_grid_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EVAS_GRID_EXTRA_OPS
#define EVAS_GRID_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(evas_obj_grid_size_set, _evas_grid_grid_size_set),
      EFL_OBJECT_OP_FUNC(evas_obj_grid_size_get, _evas_grid_grid_size_get),
      EFL_OBJECT_OP_FUNC(evas_obj_grid_children_get, _evas_grid_children_get),
      EFL_OBJECT_OP_FUNC(evas_obj_grid_accessor_new, _evas_grid_accessor_new),
      EFL_OBJECT_OP_FUNC(evas_obj_grid_clear, _evas_grid_clear),
      EFL_OBJECT_OP_FUNC(evas_obj_grid_iterator_new, _evas_grid_iterator_new),
      EFL_OBJECT_OP_FUNC(evas_obj_grid_add_to, _evas_grid_add_to),
      EFL_OBJECT_OP_FUNC(evas_obj_grid_unpack, _evas_grid_unpack),
      EFL_OBJECT_OP_FUNC(evas_obj_grid_pack_get, _evas_grid_pack_get),
      EFL_OBJECT_OP_FUNC(evas_obj_grid_pack, _evas_grid_pack),
      EFL_OBJECT_OP_FUNC(efl_constructor, _evas_grid_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_mirrored_set, _evas_grid_efl_ui_i18n_mirrored_set),
      EFL_OBJECT_OP_FUNC(efl_ui_mirrored_get, _evas_grid_efl_ui_i18n_mirrored_get),
      EVAS_GRID_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Evas_Grid class description.
 *
 * Defines metadata for the Evas_Grid class, including its name, type,
 * size of instance data, and initializer functions.
 */
static const Efl_Class_Description _evas_grid_class_desc = {
   EO_VERSION, /*!< EFL object version */
   "Evas.Grid", /*!< Class name */
   EFL_CLASS_TYPE_REGULAR, /*!< Class type */
   sizeof(Evas_Grid_Data), /*!< Size of instance data */
   _evas_grid_class_initializer, /*!< Class initializer function */
   NULL, /*!< Class constructor for legacy objects */
   NULL /*!< Class destructor for legacy objects */
};

EFL_DEFINE_CLASS(evas_grid_class_get, &_evas_grid_class_desc, EFL_CANVAS_GROUP_CLASS, EFL_UI_I18N_INTERFACE, NULL);

#include "evas_grid_eo.legacy.c"
