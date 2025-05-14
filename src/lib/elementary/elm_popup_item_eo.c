/**
 * @internal
 * @brief Constructor for Elm_Popup_Item objects.
 *
 * This function is called when a new Elm_Popup_Item object is created.
 * It initializes the object's private data.
 *
 * @param obj The Efl object to construct.
 * @param pd The private data for the Elm_Popup_Item.
 * @return The constructed Efl object.
 */
Efl_Object *_elm_popup_item_efl_object_constructor(Eo *obj, Elm_Popup_Item_Data *pd);

/**
 * @internal
 * @brief Destructor for Elm_Popup_Item objects.
 *
 * This function is called when an Elm_Popup_Item object is destroyed.
 * It cleans up any resources allocated by the object.
 *
 * @param obj The Efl object to destruct.
 * @param pd The private data for the Elm_Popup_Item.
 */
void _elm_popup_item_efl_object_destructor(Eo *obj, Elm_Popup_Item_Data *pd);

/**
 * @internal
 * @brief Disables the popup item.
 *
 * This function implements the elm_wdg_item_disable functionality for popup items.
 *
 * @param obj The Elm_Popup_Item object.
 * @param pd The private data for the Elm_Popup_Item.
 */
void _elm_popup_item_elm_widget_item_disable(Eo *obj, Elm_Popup_Item_Data *pd);

/**
 * @internal
 * @brief Emits a signal from the popup item.
 *
 * This function implements the elm_wdg_item_signal_emit functionality for popup items.
 *
 * @param obj The Elm_Popup_Item object.
 * @param pd The private data for the Elm_Popup_Item.
 * @param emission The signal name to emit. For example, "clicked".
 * @param source The source of the signal. For example, "mouse".
 */
void _elm_popup_item_elm_widget_item_signal_emit(Eo *obj, Elm_Popup_Item_Data *pd, const char *emission, const char *source);

/**
 * @internal
 * @brief Sets the text for a part of the popup item.
 *
 * This function implements the elm_wdg_item_part_text_set functionality for popup items.
 *
 * @param obj The Elm_Popup_Item object.
 * @param pd The private data for the Elm_Popup_Item.
 * @param part The name of the part to set the text for. E.g., "default", "title".
 * @param label The text to set.
 */
void _elm_popup_item_elm_widget_item_part_text_set(Eo *obj, Elm_Popup_Item_Data *pd, const char *part, const char *label);

/**
 * @internal
 * @brief Gets the text from a part of the popup item.
 *
 * This function implements the elm_wdg_item_part_text_get functionality for popup items.
 *
 * @param obj The Elm_Popup_Item object.
 * @param pd The private data for the Elm_Popup_Item.
 * @param part The name of the part to get the text from. E.g., "default", "title".
 * @return The text of the part, or NULL if not set or on error.
 */
const char *_elm_popup_item_elm_widget_item_part_text_get(const Eo *obj, Elm_Popup_Item_Data *pd, const char *part);

/**
 * @internal
 * @brief Sets the content for a part of the popup item.
 *
 * This function implements the elm_wdg_item_part_content_set functionality for popup items.
 *
 * @param obj The Elm_Popup_Item object.
 * @param pd The private data for the Elm_Popup_Item.
 * @param part The name of the part to set the content for. E.g., "icon", "end".
 * @param content The Efl_Canvas_Object to set as content.
 */
void _elm_popup_item_elm_widget_item_part_content_set(Eo *obj, Elm_Popup_Item_Data *pd, const char *part, Efl_Canvas_Object *content);

/**
 * @internal
 * @brief Gets the content from a part of the popup item.
 *
 * This function implements the elm_wdg_item_part_content_get functionality for popup items.
 *
 * @param obj The Elm_Popup_Item object.
 * @param pd The private data for the Elm_Popup_Item.
 * @param part The name of the part to get the content from. E.g., "icon", "end".
 * @return The Efl_Canvas_Object content of the part, or NULL if not set or on error.
 */
Efl_Canvas_Object *_elm_popup_item_elm_widget_item_part_content_get(const Eo *obj, Elm_Popup_Item_Data *pd, const char *part);

/**
 * @internal
 * @brief Unsets (removes) the content from a part of the popup item.
 *
 * This function implements the elm_wdg_item_part_content_unset functionality for popup items.
 *
 * @param obj The Elm_Popup_Item object.
 * @param pd The private data for the Elm_Popup_Item.
 * @param part The name of the part to unset the content from. E.g., "icon", "end".
 * @return The previously set Efl_Canvas_Object content, or NULL if not set or on error.
 *         The caller is responsible for deleting the returned object if it's no longer needed.
 */
Efl_Canvas_Object *_elm_popup_item_elm_widget_item_part_content_unset(Eo *obj, Elm_Popup_Item_Data *pd, const char *part);

/**
 * @internal
 * @brief Initializes the Elm_Popup_Item class.
 *
 * This function is called once when the Efl class system initializes the Elm_Popup_Item class.
 * It sets up the Efl_Object operations (ops) for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_popup_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_POPUP_ITEM_EXTRA_OPS
#define ELM_POPUP_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_popup_item_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_popup_item_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_disable, _elm_popup_item_elm_widget_item_disable),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_signal_emit, _elm_popup_item_elm_widget_item_signal_emit),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_set, _elm_popup_item_elm_widget_item_part_text_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_get, _elm_popup_item_elm_widget_item_part_text_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_set, _elm_popup_item_elm_widget_item_part_content_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_get, _elm_popup_item_elm_widget_item_part_content_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_unset, _elm_popup_item_elm_widget_item_part_content_unset),
      ELM_POPUP_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief The Efl_Class_Description for the Elm_Popup_Item class.
 *
 * This structure provides metadata about the Elm_Popup_Item class,
 * such as its name, version, size of instance data, and initializer functions.
 */
static const Efl_Class_Description _elm_popup_item_class_desc = {
   EO_VERSION, /**< Efl Object version. */
   "Elm.Popup.Item", /**< Class name. */
   EFL_CLASS_TYPE_REGULAR, /**< Class type. */
   sizeof(Elm_Popup_Item_Data), /**< Size of instance data (private data). */
   _elm_popup_item_class_initializer, /**< Class initializer function. */
   NULL, /**< Class constructor (usually NULL for EFL_CLASS_TYPE_REGULAR). */
   NULL /**< Class destructor (usually NULL for EFL_CLASS_TYPE_REGULAR). */
};

/**
 * @internal
 * @brief Macro to define the Elm_Popup_Item class.
 *
 * This macro uses the Efl class system to define the Elm_Popup_Item class,
 * specifying its class description, parent class (ELM_WIDGET_ITEM_CLASS),
 * and any implemented interfaces (EFL_UI_LEGACY_INTERFACE).
 */
EFL_DEFINE_CLASS(elm_popup_item_class_get, &_elm_popup_item_class_desc, ELM_WIDGET_ITEM_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);
