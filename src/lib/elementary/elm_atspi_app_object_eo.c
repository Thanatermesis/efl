/**
 * @file
 * @brief These routines are EFL Atspi App Object interface plain C implementations.
 */

/**
 * @internal
 * @brief Destructor for Elm_Atspi_App_Object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 */
void _elm_atspi_app_object_efl_object_destructor(Eo *obj, Elm_Atspi_App_Object_Data *pd);

/**
 * @internal
 * @brief Gets the internationalized name for the object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @return The internationalized name.
 */
const char *_elm_atspi_app_object_efl_access_object_i18n_name_get(const Eo *obj, Elm_Atspi_App_Object_Data *pd);

/**
 * @internal
 * @brief Sets the description of the object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @param[in] description The description to set.
 */
void _elm_atspi_app_object_efl_access_object_description_set(Eo *obj, Elm_Atspi_App_Object_Data *pd, const char *description);

/**
 * @internal
 * @brief Gets the description of the object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @return The description of the object.
 */
const char *_elm_atspi_app_object_efl_access_object_description_get(const Eo *obj, Elm_Atspi_App_Object_Data *pd);

/**
 * @internal
 * @brief Gets the role of the object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @return The role of the object.
 */
Efl_Access_Role _elm_atspi_app_object_efl_access_object_role_get(const Eo *obj, Elm_Atspi_App_Object_Data *pd);

/**
 * @internal
 * @brief Gets the list of accessible children of the object.
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @return A list of accessible children.
 *         Each element in the list is an Efl_Access_Object.
 *         Example:
 *         [child1_accessible_object, child2_accessible_object, ...]
 */
Eina_List *_elm_atspi_app_object_efl_access_object_access_children_get(const Eo *obj, Elm_Atspi_App_Object_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Atspi_App_Object class.
 * @param[in] klass The class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_atspi_app_object_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_ATSPI_APP_OBJECT_EXTRA_OPS
#define ELM_ATSPI_APP_OBJECT_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_atspi_app_object_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(efl_access_object_i18n_name_get, _elm_atspi_app_object_efl_access_object_i18n_name_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_description_set, _elm_atspi_app_object_efl_access_object_description_set),
      EFL_OBJECT_OP_FUNC(efl_access_object_description_get, _elm_atspi_app_object_efl_access_object_description_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_role_get, _elm_atspi_app_object_efl_access_object_role_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_access_children_get, _elm_atspi_app_object_efl_access_object_access_children_get),
      ELM_ATSPI_APP_OBJECT_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief The Efl_Class_Description for the Elm_Atspi_App_Object class.
 */
static const Efl_Class_Description _elm_atspi_app_object_class_desc = {
   EO_VERSION, /**< Class version */
   "Elm.Atspi.App.Object", /**< Class name */
   EFL_CLASS_TYPE_REGULAR, /**< Class type */
   sizeof(Elm_Atspi_App_Object_Data),
   _elm_atspi_app_object_class_initializer, /**< Class initializer */
   NULL, /**< Class constructor */
   NULL /**< Class destructor */
};

/**
 * @brief Defines the Elm_Atspi_App_Object class.
 *
 * This macro defines the class getter function `elm_atspi_app_object_class_get`
 * and registers the class with the EFL object system.
 *
 * @param elm_atspi_app_object_class_get The name of the class get function to be defined.
 * @param &_elm_atspi_app_object_class_desc A pointer to the class description.
 * @param EFL_OBJECT_CLASS The parent class.
 * @param EFL_ACCESS_OBJECT_MIXIN A mixin class.
 * @param NULL Sentinel.
 */
EFL_DEFINE_CLASS(elm_atspi_app_object_class_get, &_elm_atspi_app_object_class_desc, EFL_OBJECT_CLASS, EFL_ACCESS_OBJECT_MIXIN, NULL);
