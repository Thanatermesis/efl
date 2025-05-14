/**
 * @file
 * @brief These routines are bindings to an Elementary @c view_list object.
 *
 * Elm_View_List is a widget that aims to provide a list of items
 * that can be rendered as a genlist. It uses an Efl_Model to populate
 * the list items.
 */
#ifndef _ELM_VIEW_LIST_EO_H_
#define _ELM_VIEW_LIST_EO_H_

#ifndef _ELM_VIEW_LIST_EO_CLASS_TYPE
#define _ELM_VIEW_LIST_EO_CLASS_TYPE

/**
 * @brief Opaque handle to the Elm_View_List object.
 * @ingroup Elm_View_List
 */
typedef Eo Elm_View_List;

#endif

#ifndef _ELM_VIEW_LIST_EO_TYPES
#define _ELM_VIEW_LIST_EO_TYPES


#endif
/**
 * @brief Elementary view list class.
 *
 * @details Use this macro to get the Efl_Class for the Elm_View_List type.
 *
 * @ingroup Elm_View_List
 */
#define ELM_VIEW_LIST_CLASS elm_view_list_class_get()

EWAPI const Efl_Class *elm_view_list_class_get(void) EINA_CONST;

/**
 * @brief Constructor wrapper
 *
 * @param[in] obj The object.
 * @param[in] genlist Genlist object
 * @param[in] item_type Item type
 * @param[in] item_style The current item style name. @c null would be default.
 *
 * @since 1.11
 *
 * @ingroup Elm_View_List
 */
EOAPI void elm_view_list_genlist_set(Eo *obj, Efl_Canvas_Object *genlist, Elm_Genlist_Item_Type item_type, const char *item_style);

/**
 * @brief Return evas object
 *
 * @param[in] obj The object.
 * @param[out] widget Returned widget
 *
 * @since 1.11
 *
 * @ingroup Elm_View_List
 */
EOAPI void elm_view_list_evas_object_get(Eo *obj, Efl_Canvas_Object **widget);

/**
 * @brief Connect property
 *
 * @param[in] obj The object.
 * @param[in] property Property name
 * @param[in] part Edje's theme part
 *
 * @since 1.11
 *
 * @ingroup Elm_View_List
 */
EOAPI void elm_view_list_property_connect(Eo *obj, const char *property, const char *part);

/**
 * @brief Set model
 *
 * @param[in] obj The object.
 * @param[in] model Efl.Model to set
 *
 * @since 1.11
 *
 * @ingroup Elm_View_List
 */
EOAPI void elm_view_list_model_set(Eo *obj, Efl_Model *model);

/**
 * @brief Get model
 *
 * @param[in] obj The object.
 *
 * @return Efl.Model to set
 *
 * @since 1.11
 *
 * @ingroup Elm_View_List
 */
EOAPI Efl_Model *elm_view_list_model_get(const Eo *obj);

EWAPI extern const Efl_Event_Description _ELM_VIEW_LIST_EVENT_MODEL_SELECTED;

/**
 * @brief Called when a model item was selected in the view list.
 * @details The event_info payload will be an Efl_Object representing the selected model.
 * @return Efl_Object *
 *
 * @ingroup Elm_View_List
 */
#define ELM_VIEW_LIST_EVENT_MODEL_SELECTED (&(_ELM_VIEW_LIST_EVENT_MODEL_SELECTED))

#endif
