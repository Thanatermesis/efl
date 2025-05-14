/**
 * @file
 * @brief Implementation of the Elementary View List Eo bindings.
 */

/**
 * @internal
 * @brief Event description for the "model,selected" event.
 * @details This event is triggered when an item in the view list, corresponding
 * to a model, is selected. The event info is the selected Efl_Object (model).
 */
EWAPI const Efl_Event_Description _ELM_VIEW_LIST_EVENT_MODEL_SELECTED =
   EFL_EVENT_DESCRIPTION("model,selected");

/**
 * @internal
 * @brief Sets the genlist object and item properties for the view list.
 * @param obj The Eo object.
 * @param pd Private data for the Elm_View_List instance.
 * @param genlist The genlist Efl_Canvas_Object to be used.
 * @param item_type The type of genlist item.
 * @param item_style The style for the genlist items.
 */
void _elm_view_list_genlist_set(Eo *obj, Elm_View_List_Data *pd, Efl_Canvas_Object *genlist, Elm_Genlist_Item_Type item_type, const char *item_style);

EOAPI EFL_VOID_FUNC_BODYV(elm_view_list_genlist_set, EFL_FUNC_CALL(genlist, item_type, item_style), Efl_Canvas_Object *genlist, Elm_Genlist_Item_Type item_type, const char *item_style);

/**
 * @internal
 * @brief Retrieves the Evas object (widget) associated with the view list.
 * @param obj The Eo object.
 * @param pd Private data for the Elm_View_List instance.
 * @param widget Pointer to store the retrieved Efl_Canvas_Object.
 */
void _elm_view_list_evas_object_get(Eo *obj, Elm_View_List_Data *pd, Efl_Canvas_Object **widget);

EOAPI EFL_VOID_FUNC_BODYV(elm_view_list_evas_object_get, EFL_FUNC_CALL(widget), Efl_Canvas_Object **widget);

/**
 * @internal
 * @brief Connects a model property to an Edje part of the list item theme.
 * @param obj The Eo object.
 * @param pd Private data for the Elm_View_List instance.
 * @param property The name of the model property.
 * @param part The name of the Edje part in the theme.
 */
void _elm_view_list_property_connect(Eo *obj, Elm_View_List_Data *pd, const char *property, const char *part);

EOAPI EFL_VOID_FUNC_BODYV(elm_view_list_property_connect, EFL_FUNC_CALL(property, part), const char *property, const char *part);

/**
 * @internal
 * @brief Sets the Efl_Model for the view list.
 * @param obj The Eo object.
 * @param pd Private data for the Elm_View_List instance.
 * @param model The Efl_Model to be used by the view list.
 */
void _elm_view_list_model_set(Eo *obj, Elm_View_List_Data *pd, Efl_Model *model);

EOAPI EFL_VOID_FUNC_BODYV(elm_view_list_model_set, EFL_FUNC_CALL(model), Efl_Model *model);

/**
 * @internal
 * @brief Gets the Efl_Model currently used by the view list.
 * @param obj The Eo object.
 * @param pd Private data for the Elm_View_List instance.
 * @return The current Efl_Model, or @c NULL if none is set.
 */
Efl_Model *_elm_view_list_model_get(const Eo *obj, Elm_View_List_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_view_list_model_get, Efl_Model *, NULL);

/**
 * @internal
 * @brief Destructor for the Elm_View_List object.
 * @param obj The Eo object being destroyed.
 * @param pd Private data for the Elm_View_List instance.
 */
void _elm_view_list_efl_object_destructor(Eo *obj, Elm_View_List_Data *pd);


/**
 * @internal
 * @brief Initializes the Elm_View_List Efl_Class.
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @details This function sets up the operations (methods) for the Elm_View_List class.
 */
static Eina_Bool
_elm_view_list_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_VIEW_LIST_EXTRA_OPS
#define ELM_VIEW_LIST_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_view_list_genlist_set, _elm_view_list_genlist_set),
      EFL_OBJECT_OP_FUNC(elm_view_list_evas_object_get, _elm_view_list_evas_object_get),
      EFL_OBJECT_OP_FUNC(elm_view_list_property_connect, _elm_view_list_property_connect),
      EFL_OBJECT_OP_FUNC(elm_view_list_model_set, _elm_view_list_model_set),
      EFL_OBJECT_OP_FUNC(elm_view_list_model_get, _elm_view_list_model_get),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_view_list_efl_object_destructor),
      ELM_VIEW_LIST_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_View_List Efl_Class.
 * @details This structure provides metadata for the Elm_View_List class,
 * including its version, name, type, size of private data, and initializer functions.
 */
static const Efl_Class_Description _elm_view_list_class_desc = {
   EO_VERSION,
   "Elm.View.List",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_View_List_Data),
   _elm_view_list_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_view_list_class_get, &_elm_view_list_class_desc, EFL_OBJECT_CLASS, NULL);
