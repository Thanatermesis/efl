#include "evas_common_private.h"
#include "evas_image_private.h"
#include "evas_private.h"

/* BEGIN: events to maintain compatibility with legacy */
EVAS_API EVAS_API_WEAK const Efl_Event_Description _EFL_GFX_ENTITY_EVENT_SHOW =
   EFL_EVENT_DESCRIPTION("show");
EVAS_API EVAS_API_WEAK const Efl_Event_Description _EFL_GFX_ENTITY_EVENT_HIDE =
   EFL_EVENT_DESCRIPTION("hide");
EVAS_API EVAS_API_WEAK const Efl_Event_Description _EFL_GFX_ENTITY_EVENT_IMAGE_PRELOAD =
   EFL_EVENT_DESCRIPTION("preload");
EVAS_API EVAS_API_WEAK const Efl_Event_Description _EFL_GFX_ENTITY_EVENT_IMAGE_UNLOAD =
   EFL_EVENT_DESCRIPTION("unload");
/* END: events to maintain compatibility with legacy */

/* local calls */

/**
 * @internal
 * @brief Informs that an Evas object is now visible.
 *
 * This function is called when an Evas object becomes visible. It triggers
 * the EVAS_CALLBACK_SHOW event and the EFL_GFX_ENTITY_EVENT_SHOW Efl event.
 *
 * @param eo_obj The Evas object that became visible.
 * @param obj The protected data of the Evas object.
 */
void
evas_object_inform_call_show(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj)
{
   int event_id = _evas_object_event_new();
   Eina_Bool vis = EINA_TRUE;

   evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_SHOW, &vis, event_id, EFL_GFX_ENTITY_EVENT_SHOW);
   _evas_post_event_callback_call(obj->layer->evas->evas, obj->layer->evas, event_id);
}

/**
 * @internal
 * @brief Informs that an Evas object is now hidden.
 *
 * This function is called when an Evas object becomes hidden. It triggers
 * the EVAS_CALLBACK_HIDE event and the EFL_GFX_ENTITY_EVENT_HIDE Efl event.
 *
 * @param eo_obj The Evas object that became hidden.
 * @param obj The protected data of the Evas object.
 */
void
evas_object_inform_call_hide(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj)
{
   int event_id = _evas_object_event_new();
   Eina_Bool vis = EINA_FALSE;

   evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_HIDE, &vis, event_id, EFL_GFX_ENTITY_EVENT_HIDE);
   _evas_post_event_callback_call(obj->layer->evas->evas, obj->layer->evas, event_id);
}

/**
 * @internal
 * @brief Informs that an Evas object has moved.
 *
 * This function is called when an Evas object's position changes. It triggers
 * the EVAS_CALLBACK_MOVE event and the EFL_GFX_ENTITY_EVENT_POSITION_CHANGED Efl event.
 * The event data contains the new position of the object.
 *
 * @param eo_obj The Evas object that moved.
 * @param obj The protected data of the Evas object.
 */
void
evas_object_inform_call_move(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj)
{
   Eina_Position2D pos;
   int event_id = _evas_object_event_new();

   pos = ((Eina_Rect) obj->cur->geometry).pos;

   evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOVE, &pos, event_id, EFL_GFX_ENTITY_EVENT_POSITION_CHANGED);
   _evas_post_event_callback_call(obj->layer->evas->evas, obj->layer->evas, event_id);
}

/**
 * @internal
 * @brief Informs that an Evas object has been resized.
 *
 * This function is called when an Evas object's size changes. It triggers
 * the EVAS_CALLBACK_RESIZE event and the EFL_GFX_ENTITY_EVENT_SIZE_CHANGED Efl event.
 * The event data contains the new size of the object.
 *
 * @param eo_obj The Evas object that was resized.
 * @param obj The protected data of the Evas object.
 */
void
evas_object_inform_call_resize(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj)
{
   Eina_Size2D size;
   int event_id = _evas_object_event_new();

   size = ((Eina_Rect) obj->cur->geometry).size;

   evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_RESIZE, &size, event_id, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED);
   _evas_post_event_callback_call(obj->layer->evas->evas, obj->layer->evas, event_id);
}

/**
 * @internal
 * @brief Informs that an Evas object's stacking order has changed.
 *
 * This function is called when an Evas object's position in the stacking order
 * (layer or Z-order within a layer) changes. It triggers the EVAS_CALLBACK_RESTACK
 * event and the EFL_GFX_ENTITY_EVENT_STACKING_CHANGED Efl event.
 *
 * @param eo_obj The Evas object whose stacking changed.
 * @param obj The protected data of the Evas object.
 */
void
evas_object_inform_call_restack(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj)
{
   int event_id = _evas_object_event_new();

   evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_RESTACK, NULL, event_id, EFL_GFX_ENTITY_EVENT_STACKING_CHANGED);
   if (obj->layer)
     _evas_post_event_callback_call(obj->layer->evas->evas, obj->layer->evas, event_id);
}

/**
 * @internal
 * @brief Informs that an Evas object's size hints have changed.
 *
 * This function is called when an Evas object's size hints (e.g., min, max,
 * aspect, align, weight) are modified. It triggers the
 * EVAS_CALLBACK_CHANGED_SIZE_HINTS event and the
 * EFL_GFX_ENTITY_EVENT_HINTS_CHANGED Efl event.
 *
 * @param eo_obj The Evas object whose size hints changed.
 * @param obj The protected data of the Evas object.
 */
void
evas_object_inform_call_changed_size_hints(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj)
{
   int event_id = _evas_object_event_new();

   evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, NULL, event_id, EFL_GFX_ENTITY_EVENT_HINTS_CHANGED);
   _evas_post_event_callback_call(obj->layer->evas->evas, obj->layer->evas, event_id);
}

/**
 * @internal
 * @brief Informs that an Evas image object has finished preloading its data.
 *
 * This function is called when an Evas image object completes its preloading
 * operation, or if preloading was cancelled. It triggers the
 * EVAS_CALLBACK_IMAGE_PRELOADED event and the
 * EFL_GFX_IMAGE_EVENT_IMAGE_PRELOAD Efl event.
 *
 * @param eo_obj The Evas image object that finished preloading.
 */
void
evas_object_inform_call_image_preloaded(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   int event_id;

   EINA_SAFETY_ON_NULL_RETURN(obj);

   unsigned char preload = _evas_object_image_preloading_get(eo_obj);

   //Even cancelled, obj needs to draw image.
   _evas_image_load_post_update(eo_obj, obj);

   if ((preload & EVAS_IMAGE_PRELOADING) ||
     /* Boom! This cancellation call stack is in the intermediate render sequence. Need better idea.
          So far, this cancellation is triggered by other non-preload image instances,
          which doesn't require preloading. So by mechasnim we cancel preload other instances as well.
          and mimic as it finished preloading done. */
       (preload & EVAS_IMAGE_PRELOAD_CANCEL))
     {
        Eina_Bool val = EINA_TRUE;
        event_id = _evas_object_event_new();
        evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_IMAGE_PRELOADED, &val, event_id, EFL_GFX_IMAGE_EVENT_IMAGE_PRELOAD);
        _evas_post_event_callback_call(obj->layer->evas->evas, obj->layer->evas, event_id);
     }
}

/**
 * @internal
 * @brief Informs that an Evas image object has unloaded its data.
 *
 * This function is called when an Evas image object's data is unloaded from memory.
 * It triggers the EVAS_CALLBACK_IMAGE_UNLOADED event and the
 * EFL_GFX_IMAGE_EVENT_IMAGE_UNLOAD Efl event.
 *
 * @param eo_obj The Evas image object that was unloaded.
 */
void
evas_object_inform_call_image_unloaded(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   int event_id = _evas_object_event_new();
   Eina_Bool val = EINA_FALSE;

   evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_IMAGE_UNLOADED, &val, event_id, EFL_GFX_IMAGE_EVENT_IMAGE_UNLOAD);
   _evas_post_event_callback_call(obj->layer->evas->evas, obj->layer->evas, event_id);
}

/**
 * @internal
 * @brief Informs that an Evas image object's source image has been resized.
 *
 * This function is called when the source image data for an Evas image object
 * has been resized (e.g., due to scaling operations on the image data itself,
 * not the object's geometry). It triggers the EVAS_CALLBACK_IMAGE_RESIZE event
 * and the EFL_GFX_IMAGE_EVENT_IMAGE_RESIZED Efl event. The event data contains
 * the new size of the source image.
 *
 * @param eo_obj The Evas image object whose source image was resized.
 */
void
evas_object_inform_call_image_resize(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   int event_id = _evas_object_event_new();
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   Eina_Size2D sz = EINA_SIZE2D(o->file_size.w, o->file_size.h);

   evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_IMAGE_RESIZE, &sz, event_id, EFL_GFX_IMAGE_EVENT_IMAGE_RESIZED);
   _evas_post_event_callback_call(obj->layer->evas->evas, obj->layer->evas, event_id);
}
