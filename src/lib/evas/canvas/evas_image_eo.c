/**
 * @file
 * @brief Implementation of the Evas Image legacy support.
 *
 * This file contains the C source code for the Evas Image object,
 * providing compatibility with older Evas APIs.
 */

/**
 * @brief Loads data into the Evas_Image object from a file.
 *
 * This function is an internal implementation detail for handling the
 * efl_file_load operation for Evas_Image objects.
 *
 * @param[in] obj The Evas_Image object.
 * @param[in] pd Private data for the Evas_Image object.
 * @return #EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
Eina_Error _evas_image_efl_file_load(Eo *obj, void *pd);


/**
 * @brief Initializes the Evas_Image class.
 *
 * This function sets up the operations and properties for the Evas_Image class.
 * It is called during the class construction process.
 *
 * @param[in,out] klass The Efl_Class to initialize.
 * @return #EINA_TRUE on success, #EINA_FALSE otherwise.
 */
static Eina_Bool
_evas_image_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EVAS_IMAGE_EXTRA_OPS
#define EVAS_IMAGE_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_file_loaded_get, _evas_image_efl_file_loaded_get),
      EFL_OBJECT_OP_FUNC(efl_file_mmap_get, _evas_image_efl_file_mmap_get),
      EFL_OBJECT_OP_FUNC(efl_file_load, _evas_image_efl_file_load),
      EFL_OBJECT_OP_FUNC(efl_file_unload, _evas_image_efl_file_unload),
      EVAS_IMAGE_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief Describes the Evas_Image class.
 *
 * This structure contains metadata about the Evas_Image class,
 * including its version, name, type, and initializer functions.
 */
static const Efl_Class_Description _evas_image_class_desc = {
   EO_VERSION, /**< The EO API version for this class. */
   "Evas.Image", /**< The name of the class. */
   EFL_CLASS_TYPE_REGULAR, /**< The type of the class (regular, interface, mixin). */
   0, /**< The size of the instance data. */
   _evas_image_class_initializer, /**< The class initializer function. */
   NULL, /**< The class constructor function. */
   NULL /**< The class destructor function. */
};

/**
 * @brief Defines the Evas_Image class.
 *
 * This macro effectively creates the Evas_Image class, associating it with its
 * description, parent class (EFL_CANVAS_IMAGE_INTERNAL_CLASS), and any mixins (EFL_FILE_MIXIN).
 * The `evas_image_class_get` function, which retrieves this class, is also defined here.
 */
EFL_DEFINE_CLASS(evas_image_class_get, &_evas_image_class_desc, EFL_CANVAS_IMAGE_INTERNAL_CLASS, EFL_FILE_MIXIN, NULL);
