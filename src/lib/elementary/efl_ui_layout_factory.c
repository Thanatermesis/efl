#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_FACTORY_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"

#define MY_CLASS EFL_UI_LAYOUT_FACTORY_CLASS
#define MY_CLASS_NAME "Efl.Ui.Layout_Factory"

/**
 * @brief Private data structure for the Efl.Ui.Layout_Factory class.
 *
 * This structure holds the data necessary for the layout factory to operate,
 * including bindings for properties and sub-factories, as well as theme
 * information (class, group, style) for the layouts it creates.
 */
typedef struct _Efl_Ui_Layout_Factory_Data
{
    struct {
       Eina_Hash *properties; /**< Hash table storing property bindings. Keys are target property names on the layout, values are source property names. */
       Eina_Hash *factories;  /**< Hash table storing sub-factory bindings. Keys are target part names on the layout, values are Efl_Ui_Factory instances. */
    } bind; /**< Structure containing bindings for properties and factories. */
    Eina_Stringshare *klass; /**< The class name for the theme of the layout. */
    Eina_Stringshare *group; /**< The group name for the theme of the layout. */
    Eina_Stringshare *style; /**< The style name for the theme of the layout. */
} Efl_Ui_Layout_Factory_Data;

/**
 * @brief Callback function to bind a property to a layout object.
 *
 * This function is used with eina_hash_foreach to iterate over property bindings
 * and apply them to the given layout object.
 *
 * @param hash The hash table being iterated (unused).
 * @param key The key from the hash table, representing the target property name on the layout.
 * @param data The data from the hash table, representing the source property name.
 * @param fdata The layout object (Eo *) to which the property should be bound.
 * @return EINA_TRUE to continue iteration, EINA_FALSE to stop.
 */
Eina_Bool
_property_bind(const Eina_Hash *hash EINA_UNUSED, const void *key, void *data, void *fdata)
{
   Eo *layout = fdata;
   Eina_Stringshare *ss_key = key;
   Eina_Stringshare *property = data;

   efl_ui_property_bind(layout, ss_key, property);

   return EINA_TRUE;
}

/**
 * @brief Callback function to bind a sub-factory to a layout object.
 *
 * This function is used with eina_hash_foreach to iterate over factory bindings
 * and apply them to the given layout object.
 *
 * @param hash The hash table being iterated (unused).
 * @param key The key from the hash table, representing the target part name on the layout.
 * @param data The data from the hash table, representing the Efl_Ui_Factory instance.
 * @param fdata The layout object (Eo *) to which the factory should be bound.
 * @return EINA_TRUE to continue iteration, EINA_FALSE to stop.
 */
Eina_Bool
_factory_bind(const Eina_Hash *hash EINA_UNUSED, const void *key, void *data, void *fdata)
{
   Eo *layout = fdata;
   Eina_Stringshare *ss_key = key;
   Efl_Ui_Factory *factory = data;

   efl_ui_factory_bind(layout, ss_key, factory);
   return EINA_TRUE;
}

/**
 * @brief Event callback triggered when an item (layout) is being built by the factory.
 *
 * This function applies the configured theme and bindings (properties and factories)
 * to the newly created layout object. It also sets default graphic hints for
 * weight and fill.
 *
 * @param data The private data of the Efl_Ui_Layout_Factory instance.
 * @param event The event information, where event->info is the Efl_Gfx_Entity (layout) being built.
 */
static void
_efl_ui_layout_factory_building(void *data, const Efl_Event *event)
{
   Efl_Ui_Layout_Factory_Data *pd = data;
   Efl_Gfx_Entity *ui_view = event->info;

   if (pd->klass || pd->group || pd->style)
     efl_ui_layout_theme_set(ui_view, pd->klass, pd->group, pd->style);

   eina_hash_foreach(pd->bind.properties, _property_bind, ui_view);
   eina_hash_foreach(pd->bind.factories, _factory_bind, ui_view);

   efl_gfx_hint_weight_set(ui_view, EFL_GFX_HINT_EXPAND, 0);
   efl_gfx_hint_fill_set(ui_view, EINA_TRUE, EINA_TRUE);
}

/**
 * @brief Constructor for the Efl_Ui_Layout_Factory.
 *
 * Initializes the factory, sets the default item class to EFL_UI_LAYOUT_CLASS,
 * creates hash tables for property and factory bindings, and registers the
 * _efl_ui_layout_factory_building callback for the ITEM_BUILDING event.
 *
 * @param obj The Efl_Object instance being constructed.
 * @param pd The private data for the instance.
 * @return The constructed Efl_Object instance.
 */
EOLIAN static Eo *
_efl_ui_layout_factory_efl_object_constructor(Eo *obj, Efl_Ui_Layout_Factory_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   efl_ui_widget_factory_item_class_set(obj, EFL_UI_LAYOUT_CLASS);

   pd->bind.properties = eina_hash_stringshared_new(EINA_FREE_CB(eina_stringshare_del));
   pd->bind.factories = eina_hash_stringshared_new(EINA_FREE_CB(efl_unref));

   efl_event_callback_add(obj, EFL_UI_FACTORY_EVENT_ITEM_BUILDING, _efl_ui_layout_factory_building, pd);

   return obj;
}

/**
 * @brief Destructor for the Efl_Ui_Layout_Factory.
 *
 * Cleans up resources used by the factory, including freeing stringshares for
 * theme configuration and destroying the hash tables for property and factory bindings.
 *
 * @param obj The Efl_Object instance being destructed.
 * @param pd The private data for the instance.
 */
EOLIAN static void
_efl_ui_layout_factory_efl_object_destructor(Eo *obj, Efl_Ui_Layout_Factory_Data *pd)
{
   eina_stringshare_del(pd->klass);
   eina_stringshare_del(pd->group);
   eina_stringshare_del(pd->style);

   eina_hash_free(pd->bind.properties);
   eina_hash_free( pd->bind.factories);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Binds a sub-factory to a specific key (part name).
 *
 * When a layout is created by this factory, the sub-factory associated with 'key'
 * will be used to create content for the part named 'key' in the layout.
 * If 'factory' is NULL, any existing binding for 'key' is removed.
 *
 * @param obj The Efl_Ui_Layout_Factory object (unused).
 * @param pd The private data for the instance.
 * @param key The name of the part in the layout to bind the sub-factory to.
 * @param factory The Efl_Ui_Factory instance to bind, or NULL to unbind.
 * @return EINA_ERROR_NO_ERROR on success.
 */
EOLIAN static Eina_Error
_efl_ui_layout_factory_efl_ui_factory_bind_factory_bind(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Factory_Data *pd,
                                                        const char *key, Efl_Ui_Factory *factory)
{
   Eina_Stringshare *ss_key;
   Efl_Ui_Factory *f_old;
   ss_key = eina_stringshare_add(key);

   if (factory == NULL)
     {
        eina_hash_del(pd->bind.factories, ss_key, NULL);
        return EINA_ERROR_NO_ERROR;
     }

   f_old = eina_hash_set(pd->bind.factories, ss_key, efl_ref(factory));
   if (f_old)
     {
        efl_unref(f_old);
        eina_stringshare_del(ss_key);
     }

   return EINA_ERROR_NO_ERROR;
}

/**
 * @brief Binds a property of the created layout to a source property.
 *
 * This allows properties of the layout (e.g., "text") to be automatically
 * set from a data source when the layout is created.
 * If 'property' is NULL, any existing binding for 'key' is removed.
 *
 * @param obj The Efl_Ui_Layout_Factory object.
 * @param pd The private data for the instance.
 * @param key The name of the property on the layout to be bound (e.g., "elm.text").
 * @param property The name of the source property from which the value will be taken.
 * @return 0 on success (EINA_ERROR_NO_ERROR).
 */
EOLIAN static Eina_Error
_efl_ui_layout_factory_efl_ui_property_bind_property_bind(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Factory_Data *pd,
                                                          const char *key, const char *property)
{
   Eina_Stringshare *ss_key, *ss_prop;
   Eina_Stringshare *ss_old = NULL;
   ss_key = eina_stringshare_add(key);

   if (property == NULL)
     {
        eina_hash_del(pd->bind.properties, ss_key, NULL);
        goto end;
     }

   ss_prop = eina_stringshare_add(property);
   ss_old = eina_hash_set(pd->bind.properties, ss_key, ss_prop);
   if (ss_old) eina_stringshare_del(ss_old);

 end:
   efl_event_callback_call(obj, EFL_UI_PROPERTY_BIND_EVENT_PROPERTY_BOUND, (void*) ss_key);
   // Only delete our key ref it it was already present in the property hash
   if (ss_old) eina_stringshare_del(ss_key);
   return 0;
}

/**
 * @brief Configures the theme for layouts created by this factory.
 *
 * Sets the class, group, and style that will be applied to each layout
 * instance created by this factory.
 *
 * @param obj The Efl_Ui_Layout_Factory object (unused).
 * @param pd The private data for the instance.
 * @param klass The theme class name (e.g., "button").
 * @param group The theme group name (e.g., "base").
 * @param style The theme style name (e.g., "default").
 */
EOLIAN static void
_efl_ui_layout_factory_theme_config(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Factory_Data *pd,
                                    const char *klass, const char *group, const char *style)
{
   eina_stringshare_replace(&pd->klass, klass);
   eina_stringshare_replace(&pd->group, group);
   eina_stringshare_replace(&pd->style, style);
}

#include "efl_ui_layout_factory.eo.c"
