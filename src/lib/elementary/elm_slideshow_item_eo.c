
/**
 * @brief Internal implementation for elm_obj_slideshow_item_show.
 * @param obj The Eo object.
 * @param pd Private data for Elm_Slideshow_Item.
 */
void _elm_slideshow_item_show(Eo *obj, Elm_Slideshow_Item_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_slideshow_item_show);

/**
 * @brief Internal implementation for elm_obj_slideshow_item_object_get.
 * @param obj The Eo object.
 * @param pd Private data for Elm_Slideshow_Item.
 * @return Real Evas object or NULL.
 */
Efl_Canvas_Object *_elm_slideshow_item_object_get(const Eo *obj, Elm_Slideshow_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_slideshow_item_object_get, Efl_Canvas_Object *, NULL);

/**
 * @brief Constructor for Elm_Slideshow_Item objects.
 *
 * This function is called when a new Elm_Slideshow_Item object is created.
 * It handles the initial setup of the object.
 *
 * @param obj The Eo object to construct.
 * @param pd Private data for Elm_Slideshow_Item.
 * @return The constructed Eo object, or NULL on failure.
 */
Efl_Object *_elm_slideshow_item_efl_object_constructor(Eo *obj, Elm_Slideshow_Item_Data *pd);

/**
 * @brief Destructor for Elm_Slideshow_Item objects.
 *
 * This function is called when an Elm_Slideshow_Item object is being destroyed.
 * It handles the cleanup of resources used by the object.
 *
 * @param obj The Eo object to destruct.
 * @param pd Private data for Elm_Slideshow_Item.
 */
void _elm_slideshow_item_efl_object_destructor(Eo *obj, Elm_Slideshow_Item_Data *pd);

/**
 * @brief Initializes the Elm_Slideshow_Item Efl_Class.
 *
 * This function is called once when the class is first used.
 * It sets up the Efl_Object operations (methods) for the Elm_Slideshow_Item class,
 * mapping them to their C implementations.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_slideshow_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_SLIDESHOW_ITEM_EXTRA_OPS
#define ELM_SLIDESHOW_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_item_show, _elm_slideshow_item_show),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_item_object_get, _elm_slideshow_item_object_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_slideshow_item_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_slideshow_item_efl_object_destructor),
      ELM_SLIDESHOW_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_slideshow_item_class_desc = {
   EO_VERSION,
   "Elm.Slideshow.Item",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Slideshow_Item_Data),
   _elm_slideshow_item_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_slideshow_item_class_get, &_elm_slideshow_item_class_desc, ELM_WIDGET_ITEM_CLASS, NULL);

#include "elm_slideshow_item_eo.legacy.c"
