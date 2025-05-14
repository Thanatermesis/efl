#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_FACTORY_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"

#define MY_CLASS EFL_UI_IMAGE_FACTORY_CLASS
#define MY_CLASS_NAME "Efl.Ui.Image_Factory"

/**
 * @brief Private data for the Efl.Ui.Image_Factory class.
 */
typedef struct _Efl_Ui_Image_Factory_Data
{
    Eina_Stringshare *property; /**< The property name to bind to the image's 'filename' property. */
} Efl_Ui_Image_Factory_Data;

/**
 * @brief Callback function invoked when an item is being built by the factory.
 *
 * This function binds the factory's configured property (which should hold the image path)
 * to the "filename" property of the newly created Efl_Ui_Image widget.
 *
 * @param data The private data of the Efl_Ui_Image_Factory.
 * @param ev The event information, containing the UI view (Efl_Ui_Image) being built.
 */
static void
_efl_ui_image_factory_building(void *data, const Efl_Event *ev)
{
   Efl_Ui_Image_Factory_Data *pd = data;
   Efl_Gfx_Entity *ui_view = ev->info;

   efl_ui_property_bind(ui_view, "filename", pd->property);
}

/**
 * @brief Constructor for the Efl.Ui.Image_Factory.
 *
 * Initializes the factory, sets the item class to Efl_Ui_Image, and
 * registers the item building callback.
 *
 * @param obj The Efl_Object instance.
 * @param pd The private data for the Efl_Ui_Image_Factory.
 * @return The constructed Efl_Object.
 */
EOLIAN static Eo *
_efl_ui_image_factory_efl_object_constructor(Eo *obj, Efl_Ui_Image_Factory_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_ui_widget_factory_item_class_set(obj, EFL_UI_IMAGE_CLASS);

   efl_event_callback_add(obj, EFL_UI_FACTORY_EVENT_ITEM_BUILDING, _efl_ui_image_factory_building, pd);

   pd->property = NULL;

   return obj;
}

/**
 * @brief Destructor for the Efl.Ui.Image_Factory.
 *
 * Cleans up resources, specifically the stored property string.
 *
 * @param obj The Efl_Object instance (unused).
 * @param pd The private data for the Efl_Ui_Image_Factory.
 */
EOLIAN static void
_efl_ui_image_factory_efl_object_destructor(Eo *obj EINA_UNUSED, Efl_Ui_Image_Factory_Data *pd)
{
   eina_stringshare_del(pd->property);
   pd->property = NULL;

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Creates UI elements based on the provided models.
 *
 * This function is called to generate Efl_Ui_Image widgets. It requires
 * that a property has been bound via `efl_ui_property_bind` first, which
 * will be used to set the image file for each created item.
 *
 * @param obj The Efl_Object instance.
 * @param pd The private data for the Efl_Ui_Image_Factory.
 * @param models An iterator of models to create items from.
 * @return A future that resolves when the items are created or rejected on error.
 *         Returns EFL_FACTORY_ERROR_NOT_SUPPORTED if no property is bound.
 */
EOLIAN static Eina_Future *
_efl_ui_image_factory_efl_ui_factory_create(Eo *obj, Efl_Ui_Image_Factory_Data *pd, Eina_Iterator *models)
{
   if (!pd->property) return efl_loop_future_rejected(obj, EFL_FACTORY_ERROR_NOT_SUPPORTED);

   return efl_ui_factory_create(efl_super(obj, EFL_UI_IMAGE_FACTORY_CLASS), models);
}

/**
 * @brief Binds a data model property to be used by the factory.
 *
 * This function stores the `property` name. When items (Efl_Ui_Image widgets)
 * are created by this factory, the value of this `property` from the data model
 * will be used to set the "filename" of the image.
 *
 * @param obj The Efl_Object instance (unused).
 * @param pd The private data for the Efl_Ui_Image_Factory.
 * @param key The key to bind to (unused in this implementation, typically "filename" for an image).
 * @param property The name of the property in the data model that contains the image path.
 *                 Example: "image_path_from_model".
 * @return 0 on success, or an Eina_Error on failure.
 */
EOLIAN static Eina_Error
_efl_ui_image_factory_efl_ui_property_bind_property_bind(Eo *obj EINA_UNUSED, Efl_Ui_Image_Factory_Data *pd, const char *key EINA_UNUSED, const char *property)
{
   eina_stringshare_replace(&pd->property, property);
   return 0;
}

#include "efl_ui_image_factory.eo.c"
