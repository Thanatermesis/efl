#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define EFL_UI_SCROLLBAR_PROTECTED
#define EFL_PART_PROTECTED
#define EFL_UI_FACTORY_PROTECTED

#include "eo_internal.h"

#include <Efl.h>

#include "interfaces/efl_config.eo.c"
#include "interfaces/efl_control.eo.c"
#include "interfaces/efl_duplicate.eo.c"
#include "interfaces/efl_gfx_image.eo.c"
#include "interfaces/efl_gfx_frame_controller.eo.c"
#include "interfaces/efl_gfx_image_load_controller.eo.c"
#include "interfaces/efl_part.eo.c"
#include "interfaces/efl_playable.eo.c"
#include "interfaces/efl_player.eo.c"
#include "interfaces/efl_audio_control.eo.c"
#include "interfaces/efl_text.eo.c"
#include "interfaces/efl_text_font_properties.eo.c"
#include "interfaces/efl_text_style.eo.c"
#include "interfaces/efl_text_format.eo.c"
#include "interfaces/efl_text_markup.eo.c"
#include "interfaces/efl_input_text_entity.eo.c"

#include "interfaces/efl_gfx_entity.eo.c"
#include "interfaces/efl_gfx_buffer.eo.c"
#include "interfaces/efl_gfx_stack.eo.c"
#include "interfaces/efl_gfx_fill.eo.c"
#include "interfaces/efl_gfx_view.eo.c"
#include "interfaces/efl_gfx_image_orientable.eo.c"

#include "interfaces/efl_gfx_text_class.eo.c"
#include "interfaces/efl_gfx_size_class.eo.c"

#include "interfaces/efl_gfx_gradient.eo.c"
#include "interfaces/efl_gfx_gradient_linear.eo.c"
#include "interfaces/efl_gfx_gradient_radial.eo.c"

#include "interfaces/efl_gfx_filter.eo.c"
#include "interfaces/efl_gfx_blur.eo.c"

#include "interfaces/efl_gfx_hint.eo.c"

#include "interfaces/efl_screen.eo.c"

/* Packing & containers */
#include "interfaces/efl_container.eo.c"
#include "interfaces/efl_content.eo.c"
#include "interfaces/efl_pack.eo.c"
#include "interfaces/efl_pack_layout.eo.c"
#include "interfaces/efl_pack_linear.eo.c"
#include "interfaces/efl_pack_table.eo.c"
#include "interfaces/efl_gfx_arrangement.eo.c"
#include "interfaces/efl_model.eo.c"
#include "interfaces/efl_interpolator.eo.c"
#include "interfaces/efl_ui_i18n.eo.c"
#include "interfaces/efl_ui_layout_orientable.eo.c"
#include "interfaces/efl_ui_drag.eo.c"
#include "interfaces/efl_ui_range_display.eo.c"
#include "interfaces/efl_ui_range_interactive.eo.c"
#include "interfaces/efl_ui_autorepeat.eo.c"
#include "interfaces/efl_ui_view.eo.c"
#include "interfaces/efl_ui_factory.eo.c"
#include "interfaces/efl_ui_property_bind.eo.c"
#include "interfaces/efl_ui_factory_bind.eo.c"

#include "interfaces/efl_ui_draggable.eo.c"
#include "interfaces/efl_ui_scrollable.eo.c"
#include "interfaces/efl_ui_scrollbar.eo.c"
#include "interfaces/efl_ui_container_selectable.eo.c"

#include "interfaces/efl_ui_zoom.eo.c"

#include "interfaces/efl_cached_item.eo.c"

/**
 * @internal
 * @brief Event callback triggered when an object's reference count drops to zero.
 *
 * This function is responsible for cleaning up the object by first removing
 * itself as an event callback for the EFL_EVENT_NOREF event, and then
 * deleting the object. This prevents potential issues if the object were
 * to be resurrected and then later no-ref'd again without this callback
 * being re-added.
 *
 * @param data User data passed to the event callback (unused in this function).
 * @param event Information about the event that occurred. The event->object
 *              is the object whose reference count reached zero.
 */
static void
_noref_death(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_event_callback_del(event->object, EFL_EVENT_NOREF, _noref_death, NULL);
   efl_del(event->object);
}

/**
 * @brief Retrieves a part object from a given Eo object and manages its lifecycle.
 *
 * This function fetches a part named @p name from the object @p obj.
 * If the part is found, it sets up an event callback (_noref_death)
 * to automatically delete the part when its reference count drops to zero.
 * It also performs safety checks to ensure the part has a parent and its
 * reference count is 1 before enabling auto-unreffing and returning a
 * new reference to the part.
 *
 * @param obj The parent object from which to get the part.
 * @param name The name of the part to retrieve.
 * @return A new reference to the retrieved part object, or @c NULL if the part
 *         is not found or if safety checks fail. The caller is responsible
 *         for unreferencing this object when it's no longer needed.
 */
EAPI Efl_Object *
efl_part(const Eo *obj, const char *name)
{
   Efl_Object *r;

   r = efl_part_get(obj, name);
   if (!r) return NULL;

   efl_event_callback_add(r, EFL_EVENT_NOREF, _noref_death, NULL);

   //ensure that the parts that we have here are never leaked
   //by checking theire references and ownership details
   EINA_SAFETY_ON_NULL_RETURN_VAL(efl_parent_get(r), r);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_ref_count(r) == 1, r);

   ___efl_auto_unref_set(r, EINA_TRUE);

   return efl_ref(r);
}

/**
 * @internal
 * @brief Initializes internal EFL components.
 *
 * This function is called to set up necessary internal systems within EFL.
 * Currently, it initializes the EFL model system.
 */
EAPI void
__efl_internal_init(void)
{
   efl_model_init();
}

/**
 * @internal
 * @brief Callback function invoked after items are created by an Efl_Ui_View_Factory.
 *
 * This function iterates through an Eina_Value array, which contains
 * Efl_Gfx_Entity items created by the factory. For each item, it triggers the
 * EFL_UI_FACTORY_EVENT_ITEM_CREATED event on the factory object.
 *
 * @param factory The factory object that created the items.
 * @param data User data passed to the callback (unused in this function).
 * @param v An Eina_Value of type EINA_VALUE_TYPE_ARRAY. Each element in the
 *          array is an Efl_Gfx_Entity* representing a newly created UI item.
 *          Example:
 *          v (Eina_Value array)
 *          |
 *          +-- [0] (Efl_Gfx_Entity *) item1
 *          +-- [1] (Efl_Gfx_Entity *) item2
 *          ...
 *          +-- [n-1] (Efl_Gfx_Entity *) itemN
 *
 * @return The original Eina_Value @p v, passed through.
 */
static Eina_Value
_efl_ui_view_factory_item_created(Eo *factory, void *data EINA_UNUSED, const Eina_Value v)
{
   Efl_Gfx_Entity *item;
   int len, i;

   EINA_VALUE_ARRAY_FOREACH(&v, len, i, item)
     efl_event_callback_call(factory, EFL_UI_FACTORY_EVENT_ITEM_CREATED, item);

   return v;
}

/**
 * @internal
 * @brief Creates UI items using a factory and emits an event upon completion.
 *
 * This Eolian-exposed static function initiates the creation of UI items
 * based on the provided models using the `efl_ui_factory_create` method.
 * Once the creation process (which is asynchronous and returns a future)
 * completes successfully, it then calls the `_efl_ui_view_factory_item_created`
 * callback. This callback is responsible for emitting the
 * `EFL_UI_FACTORY_EVENT_ITEM_CREATED` event for each created item.
 *
 * @param factory The Efl_Ui_Factory instance to use for creating items.
 * @param models An iterator providing the models for which UI items should be created.
 * @return An Eina_Future that resolves with an Eina_Value array containing the
 *         created Efl_Gfx_Entity items. Returns @c NULL if the factory is @c NULL.
 *         The structure of the resolved Eina_Value array is the same as described
 *         for the @p v parameter in `_efl_ui_view_factory_item_created`.
 */
EOLIAN static Eina_Future *
_efl_ui_view_factory_create_with_event(Efl_Ui_Factory *factory, Eina_Iterator *models)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(factory, NULL);
   return efl_future_then(factory, efl_ui_factory_create(factory, models),
                          .success_type = EINA_VALUE_TYPE_ARRAY,
                          .success = _efl_ui_view_factory_item_created);
}

#include "efl_ui_view_factory.eo.c"
