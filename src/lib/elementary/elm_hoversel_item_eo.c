/**
 * @internal
 * @brief Internal implementation for setting the icon of a hoversel item.
 * @see elm_obj_hoversel_item_icon_set
 */
void _elm_hoversel_item_icon_set(Eo *obj, Elm_Hoversel_Item_Data *pd, const char *icon_file, const char *icon_group, Elm_Icon_Type icon_type);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_hoversel_item_icon_set, EFL_FUNC_CALL(icon_file, icon_group, icon_type), const char *icon_file, const char *icon_group, Elm_Icon_Type icon_type);

/**
 * @internal
 * @brief Internal implementation for getting the icon of a hoversel item.
 * @see elm_obj_hoversel_item_icon_get
 */
void _elm_hoversel_item_icon_get(const Eo *obj, Elm_Hoversel_Item_Data *pd, const char **icon_file, const char **icon_group, Elm_Icon_Type *icon_type);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_hoversel_item_icon_get, EFL_FUNC_CALL(icon_file, icon_group, icon_type), const char **icon_file, const char **icon_group, Elm_Icon_Type *icon_type);

/**
 * @internal
 * @brief Internal constructor for Elm_Hoversel_Item objects.
 *
 * This function is called when a new hoversel item object is created.
 * It initializes the private data structure @c Elm_Hoversel_Item_Data.
 */
Efl_Object *_elm_hoversel_item_efl_object_constructor(Eo *obj, Elm_Hoversel_Item_Data *pd);

/**
 * @internal
 * @brief Internal destructor for Elm_Hoversel_Item objects.
 *
 * This function is called when a hoversel item object is being destroyed.
 * It cleans up resources allocated by the item.
 */
void _elm_hoversel_item_efl_object_destructor(Eo *obj, Elm_Hoversel_Item_Data *pd);

/**
 * @internal
 * @brief Internal implementation for disabling a hoversel item.
 * @see elm_wdg_item_disable_set
 */
void _elm_hoversel_item_elm_widget_item_disable(Eo *obj, Elm_Hoversel_Item_Data *pd);

/**
 * @internal
 * @brief Internal implementation for emitting a signal from a hoversel item.
 * @see elm_wdg_item_signal_emit
 */
void _elm_hoversel_item_elm_widget_item_signal_emit(Eo *obj, Elm_Hoversel_Item_Data *pd, const char *emission, const char *source);

/**
 * @internal
 * @brief Internal implementation for setting the text of a part of a hoversel item.
 * @see elm_wdg_item_part_text_set
 */
void _elm_hoversel_item_elm_widget_item_part_text_set(Eo *obj, Elm_Hoversel_Item_Data *pd, const char *part, const char *label);

/**
 * @internal
 * @brief Internal implementation for getting the text of a part of a hoversel item.
 * @see elm_wdg_item_part_text_get
 */
const char *_elm_hoversel_item_elm_widget_item_part_text_get(const Eo *obj, Elm_Hoversel_Item_Data *pd, const char *part);

/**
 * @internal
 * @brief Internal implementation for setting the style of a hoversel item.
 * @see elm_wdg_item_style_set
 */
void _elm_hoversel_item_elm_widget_item_style_set(Eo *obj, Elm_Hoversel_Item_Data *pd, const char *style);

/**
 * @internal
 * @brief Internal implementation for getting the style of a hoversel item.
 * @see elm_wdg_item_style_get
 */
const char *_elm_hoversel_item_elm_widget_item_style_get(const Eo *obj, Elm_Hoversel_Item_Data *pd);

/**
 * @internal
 * @brief Internal implementation for setting the focus state of a hoversel item.
 * @see elm_wdg_item_focus_set
 */
void _elm_hoversel_item_elm_widget_item_item_focus_set(Eo *obj, Elm_Hoversel_Item_Data *pd, Eina_Bool focused);

/**
 * @internal
 * @brief Internal implementation for getting the focus state of a hoversel item.
 * @see elm_wdg_item_focus_get
 */
Eina_Bool _elm_hoversel_item_elm_widget_item_item_focus_get(const Eo *obj, Elm_Hoversel_Item_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Hoversel_Item class.
 *
 * This function is called once when the Efl class system initializes
 * the Elm_Hoversel_Item class. It sets up the Efl_Object operations
 * (methods) for this class.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_hoversel_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_HOVERSEL_ITEM_EXTRA_OPS
#define ELM_HOVERSEL_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_hoversel_item_icon_set, _elm_hoversel_item_icon_set),
      EFL_OBJECT_OP_FUNC(elm_obj_hoversel_item_icon_get, _elm_hoversel_item_icon_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_hoversel_item_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_hoversel_item_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_disable, _elm_hoversel_item_elm_widget_item_disable),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_signal_emit, _elm_hoversel_item_elm_widget_item_signal_emit),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_set, _elm_hoversel_item_elm_widget_item_part_text_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_get, _elm_hoversel_item_elm_widget_item_part_text_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_style_set, _elm_hoversel_item_elm_widget_item_style_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_style_get, _elm_hoversel_item_elm_widget_item_style_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_focus_set, _elm_hoversel_item_elm_widget_item_item_focus_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_focus_get, _elm_hoversel_item_elm_widget_item_item_focus_get),
      ELM_HOVERSEL_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Hoversel_Item Efl class.
 *
 * This structure provides metadata for the Elm_Hoversel_Item class,
 * including its version, name, type, instance size, and initializer functions.
 */
static const Efl_Class_Description _elm_hoversel_item_class_desc = {
   EO_VERSION, /**< EFL object version. */
   "Elm.Hoversel.Item", /**< The full name of the class. */
   EFL_CLASS_TYPE_REGULAR, /**< Specifies that this is a regular instantiable class. */
   sizeof(Elm_Hoversel_Item_Data), /**< The size of the instance data for this class. */
   _elm_hoversel_item_class_initializer, /**< Function to initialize the class. */
   NULL, /**< Class constructor (optional, typically handled by _elm_hoversel_item_class_initializer for ops). */
   NULL /**< Class destructor (optional). */
};

EFL_DEFINE_CLASS(elm_hoversel_item_class_get, &_elm_hoversel_item_class_desc, ELM_WIDGET_ITEM_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_hoversel_item_eo.legacy.c"
