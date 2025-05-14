/**
 * @internal
 * @brief Constructor for Edje_Edit objects.
 *
 * @param[in] obj The object to construct.
 * @param[in] pd The private data for the Edje_Edit object.
 * @return The constructed object, or NULL on failure.
 */
Efl_Object *_edje_edit_efl_object_constructor(Eo *obj, Edje_Edit *pd);

/**
 * @internal
 * @brief Destructor for Edje_Edit objects.
 *
 * @param[in] obj The object to destruct.
 * @param[in] pd The private data for the Edje_Edit object.
 */
void _edje_edit_efl_object_destructor(Eo *obj, Edje_Edit *pd);

/**
 * @internal
 * @brief Loads data from a file into the Edje_Edit object.
 *
 * This function is part of the Efl.File interface implementation.
 *
 * @param[in] obj The object to load data into.
 * @param[in] pd The private data for the Edje_Edit object.
 * @return EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Error _edje_edit_efl_file_load(Eo *obj, Edje_Edit *pd);

/**
 * @internal
 * @brief Initializes the Edje_Edit class.
 *
 * Sets up the operations (methods) for the Edje_Edit class.
 *
 * @param[in] klass The class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_edje_edit_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EDJE_EDIT_EXTRA_OPS
#define EDJE_EDIT_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _edje_edit_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _edje_edit_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(efl_file_load, _edje_edit_efl_file_load),
      EFL_OBJECT_OP_FUNC(efl_file_unload, _edje_edit_efl_file_unload),
      EDJE_EDIT_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Edje_Edit class.
 *
 * This structure provides metadata for the Edje_Edit class,
 * including its version, name, type, size of its private data,
 * and pointers to its initializer and constructor/destructor functions.
 */
static const Efl_Class_Description _edje_edit_class_desc = {
   EO_VERSION, /**< EFL object version. */
   "Edje.Edit", /**< Class name. */
   EFL_CLASS_TYPE_REGULAR, /**< Class type. */
   sizeof(Edje_Edit), /**< Size of private data. */
   _edje_edit_class_initializer, /**< Class initializer function. */
   NULL, /**< Class constructor (managed by EFL_DEFINE_CLASS). */
   NULL /**< Class destructor (managed by EFL_DEFINE_CLASS). */
};

/**
 * @internal
 * @brief Defines the Edje_Edit class.
 *
 * This macro generates the edje_edit_class_get() function and
 * registers the Edje_Edit class with the EFL object system.
 * It inherits from EFL_CANVAS_LAYOUT_CLASS.
 */
EFL_DEFINE_CLASS(edje_edit_class_get, &_edje_edit_class_desc, EFL_CANVAS_LAYOUT_CLASS, NULL);
