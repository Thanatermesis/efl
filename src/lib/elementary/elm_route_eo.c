/**
 * @file
 * @brief Implementation of the Elm_Route Eo class.
 *
 * This file contains the internal C implementation for the Elm_Route
 * EFL Evas Object. It includes the method implementations and class
 * setup functions.
 */

/**
 * @internal
 * @brief Internal implementation for elm_obj_route_emap_set().
 * @see elm_obj_route_emap_set()
 */
void _elm_route_emap_set(Eo *obj, Elm_Route_Data *pd, void *emap);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_route_emap_set, EFL_FUNC_CALL(emap), void *emap);

/**
 * @internal
 * @brief Internal implementation for elm_obj_route_longitude_min_max_get().
 * @see elm_obj_route_longitude_min_max_get()
 */
void _elm_route_longitude_min_max_get(const Eo *obj, Elm_Route_Data *pd, double *min, double *max);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_route_longitude_min_max_get, EFL_FUNC_CALL(min, max), double *min, double *max);

/**
 * @internal
 * @brief Internal implementation for elm_obj_route_latitude_min_max_get().
 * @see elm_obj_route_latitude_min_max_get()
 */
void _elm_route_latitude_min_max_get(const Eo *obj, Elm_Route_Data *pd, double *min, double *max);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_route_latitude_min_max_get, EFL_FUNC_CALL(min, max), double *min, double *max);

/**
 * @internal
 * @brief Internal implementation for efl_constructor for Elm_Route.
 *
 * This function is called when a new Elm_Route object is constructed.
 * It handles the initialization of the object.
 * @param[in] obj The Eo object to construct.
 * @param[in] pd The private data for the Elm_Route instance.
 * @return The constructed Efl_Object, or NULL on failure.
 */
Efl_Object *_elm_route_efl_object_constructor(Eo *obj, Elm_Route_Data *pd);

/**
 * @internal
 * @brief Internal implementation for efl_ui_widget_theme_apply for Elm_Route.
 *
 * This function is called when the theme is to be applied to the Elm_Route widget.
 * It handles applying the current theme to the widget's visual elements.
 * @param[in] obj The Eo object.
 * @param[in] pd The private data for the Elm_Route instance.
 * @return EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Error _elm_route_efl_ui_widget_theme_apply(Eo *obj, Elm_Route_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Route class.
 *
 * This function is called once when the Elm_Route class is being set up.
 * It defines the Efl_Object operations (methods) for the class.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_route_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_ROUTE_EXTRA_OPS
#define ELM_ROUTE_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_route_emap_set, _elm_route_emap_set),
      EFL_OBJECT_OP_FUNC(elm_obj_route_longitude_min_max_get, _elm_route_longitude_min_max_get),
      EFL_OBJECT_OP_FUNC(elm_obj_route_latitude_min_max_get, _elm_route_latitude_min_max_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_route_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_route_efl_ui_widget_theme_apply),
      ELM_ROUTE_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Route Eo class.
 *
 * This static structure provides metadata about the Elm_Route class,
 * such as its version, name, type, size of instance data,
 * and pointers to class initializer and constructor functions.
 */
static const Efl_Class_Description _elm_route_class_desc = {
   EO_VERSION, /**< EO_VERSION */
   "Elm.Route", /**< Class name */
   EFL_CLASS_TYPE_REGULAR, /**< Class type */
   sizeof(Elm_Route_Data), /**< Size of instance data (Elm_Route_Data) */
   _elm_route_class_initializer, /**< Function to initialize the class */
   _elm_route_class_constructor, /**< Function to construct an instance */
   NULL /**< No class destructor */
};

EFL_DEFINE_CLASS(elm_route_class_get, &_elm_route_class_desc, EFL_UI_WIDGET_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_route_eo.legacy.c"
