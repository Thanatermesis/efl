/**
 * @brief Internal function to set the line coordinates.
 *
 * @param[in] obj The Evas_Line object.
 * @param[in] pd The Evas_Line_Data private data.
 * @param[in] x1 The X coordinate of the first point.
 * @param[in] y1 The Y coordinate of the first point.
 * @param[in] x2 The X coordinate of the second point.
 * @param[in] y2 The Y coordinate of the second point.
 */
void _evas_line_xy_set(Eo *obj, Evas_Line_Data *pd, int x1, int y1, int x2, int y2);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_line_xy_set, EFL_FUNC_CALL(x1, y1, x2, y2), int x1, int y1, int x2, int y2);

/**
 * @brief Internal function to get the line coordinates.
 *
 * @param[in] obj The Evas_Line object.
 * @param[in] pd The Evas_Line_Data private data.
 * @param[out] x1 Pointer to store the X coordinate of the first point.
 * @param[out] y1 Pointer to store the Y coordinate of the first point.
 * @param[out] x2 Pointer to store the X coordinate of the second point.
 * @param[out] y2 Pointer to store the Y coordinate of the second point.
 */
void _evas_line_xy_get(const Eo *obj, Evas_Line_Data *pd, int *x1, int *y1, int *x2, int *y2);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_line_xy_get, EFL_FUNC_CALL(x1, y1, x2, y2), int *x1, int *y1, int *x2, int *y2);

/**
 * @brief Constructor for Evas_Line objects.
 *
 * This function is called when a new Evas_Line object is created.
 * It initializes the private data and sets up the object.
 *
 * @param[in] obj The Evas_Line object being constructed.
 * @param[in] pd The Evas_Line_Data private data.
 * @return The constructed Efl_Object.
 */
Efl_Object *_evas_line_efl_object_constructor(Eo *obj, Evas_Line_Data *pd);

/**
 * @brief Class initializer for Evas_Line.
 *
 * This function is called once when the Evas_Line class is initialized.
 * It sets up the operations (functions) for the class.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_evas_line_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EVAS_LINE_EXTRA_OPS
#define EVAS_LINE_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(evas_obj_line_xy_set, _evas_line_xy_set),
      EFL_OBJECT_OP_FUNC(evas_obj_line_xy_get, _evas_line_xy_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _evas_line_efl_object_constructor),
      EVAS_LINE_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief Evas_Line class description.
 *
 * This structure provides metadata for the Evas_Line class,
 * including its version, name, type, size of private data,
 * and initializer functions.
 */
static const Efl_Class_Description _evas_line_class_desc = {
   EO_VERSION,
   "Evas.Line",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Evas_Line_Data),
   _evas_line_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(evas_line_class_get, &_evas_line_class_desc, EFL_CANVAS_OBJECT_CLASS, NULL);

#include "evas_line_eo.legacy.c"
