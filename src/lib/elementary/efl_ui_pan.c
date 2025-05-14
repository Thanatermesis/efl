#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"
#include "efl_ui_widget_pan.h"

#define MY_CLASS EFL_UI_PAN_CLASS
#define MY_CLASS_NAME "Efl_Ui_Pan"

#define EFL_UI_PAN_DATA_GET(o, sd) \
  Efl_Ui_Pan_Data *sd = efl_data_scope_safe_get(o, MY_CLASS)

#define EFL_UI_PAN_DATA_GET_OR_RETURN(o, ptr, ...)                      \
  EFL_UI_PAN_DATA_GET(o, ptr);                                     \
  if (EINA_UNLIKELY(!ptr))                                         \
    {                                                              \
      ERR("No widget data for object %p (%s)",                     \
          o, evas_object_type_get(o));                             \
      return __VA_ARGS__;                                                      \
    }

/**
 * @brief Sets the position of the pan widget.
 *
 * This function is an Efl_Gfx_Entity interface implementation. It updates the
 * internal position state (psd->x, psd->y) and triggers a smart changed event.
 *
 * @param[in] obj The Efl_Ui_Pan object.
 * @param[in,out] psd The private data of the Efl_Ui_Pan object.
 * @param[in] pos The new position (x, y) to set.
 */
EOLIAN static void
_efl_ui_pan_efl_gfx_entity_position_set(Eo *obj, Efl_Ui_Pan_Data *psd, Eina_Position2D pos)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_MOVE, 0, pos.x, pos.y))
     return;

   efl_gfx_entity_position_set(efl_super(obj, MY_CLASS), pos);

   psd->x = pos.x;
   psd->y = pos.y;

   evas_object_smart_changed(obj);
}

/**
 * @brief Sets the size of the pan widget.
 *
 * This function is an Efl_Gfx_Entity interface implementation. It updates the
 * internal size state (psd->w, psd->h) and triggers a smart changed event.
 *
 * @param[in] obj The Efl_Ui_Pan object.
 * @param[in,out] psd The private data of the Efl_Ui_Pan object.
 * @param[in] sz The new size (width, height) to set.
 */
EOLIAN static void
_efl_ui_pan_efl_gfx_entity_size_set(Eo *obj, Efl_Ui_Pan_Data *psd, Eina_Size2D sz)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_RESIZE, 0, sz.w, sz.h))
     return;

   efl_gfx_entity_size_set(efl_super(obj, MY_CLASS), sz);

   psd->w = sz.w;
   psd->h = sz.h;

   evas_object_smart_changed(obj);
}

/**
 * @brief Sets the visibility of the pan widget and its content.
 *
 * This function is an Efl_Gfx_Entity interface implementation. It also propagates
 * the visibility state to the content object, if one exists.
 *
 * @param[in] obj The Efl_Ui_Pan object.
 * @param[in,out] psd The private data of the Efl_Ui_Pan object.
 * @param[in] vis EINA_TRUE if the object should be visible, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_ui_pan_efl_gfx_entity_visible_set(Eo *obj, Efl_Ui_Pan_Data *psd, Eina_Bool vis)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_VISIBLE, 0, vis))
     return;

   efl_gfx_entity_visible_set(efl_super(obj, MY_CLASS), vis);
   if (psd->content) efl_gfx_entity_visible_set(psd->content, vis);
}

/**
 * @brief Sets the scrolled position of the content within the pan widget.
 *
 * This function updates the internal pan position (psd->px, psd->py), which
 * represents the top-left point of the visible part of the content.
 * It triggers a smart changed event and emits the EFL_UI_PAN_EVENT_PAN_CONTENT_POSITION_CHANGED event.
 *
 * @param[in] obj The Efl_Ui_Pan object.
 * @param[in,out] psd The private data of the Efl_Ui_Pan object.
 * @param[in] pos The new pan position (x, y).
 */
EOLIAN static void
_efl_ui_pan_pan_position_set(Eo *obj EINA_UNUSED, Efl_Ui_Pan_Data *psd, Eina_Position2D pos)
{
   if ((pos.x == psd->px) && (pos.y == psd->py)) return;
   psd->px = pos.x;
   psd->py = pos.y;

   evas_object_smart_changed(obj);
   efl_event_callback_call(obj, EFL_UI_PAN_EVENT_PAN_CONTENT_POSITION_CHANGED, &pos);
}

/**
 * @brief Gets the current scrolled position of the content within the pan widget.
 *
 * @param[in] obj The Efl_Ui_Pan object (unused).
 * @param[in] psd The private data of the Efl_Ui_Pan object.
 * @return The current pan position (x, y) as Eina_Position2D.
 *         Example: { .x = 10, .y = 20 }
 */
EOLIAN static Eina_Position2D
_efl_ui_pan_pan_position_get(const Eo *obj EINA_UNUSED, Efl_Ui_Pan_Data *psd)
{
   return EINA_POSITION2D(psd->px, psd->py);
}

/**
 * @brief Gets the maximum scrollable position for the content.
 *
 * This is calculated based on the difference between the content size and the
 * pan widget's size. If the content is smaller than the pan area in a
 * dimension, the max scroll position for that dimension is 0.
 *
 * @param[in] obj The Efl_Ui_Pan object (unused).
 * @param[in] psd The private data of the Efl_Ui_Pan object.
 * @return The maximum pan position (x, y) as Eina_Position2D.
 *         Example: If content is 500x400 and pan is 100x100, returns { .x = 400, .y = 300 }
 */
EOLIAN static Eina_Position2D
_efl_ui_pan_pan_position_max_get(const Eo *obj EINA_UNUSED, Efl_Ui_Pan_Data *psd)
{
   Eina_Position2D pos = { 0, 0};
   if (psd->w < psd->content_w) pos.x = psd->content_w - psd->w;
   if (psd->h < psd->content_h) pos.y = psd->content_h - psd->h;

   return pos;
}

/**
 * @brief Gets the minimum scrollable position for the content.
 *
 * This typically returns (0,0) as the content cannot be scrolled beyond
 * its top-left origin.
 *
 * @param[in] obj The Efl_Ui_Pan object (unused).
 * @param[in] _pd The private data of the Efl_Ui_Pan object (unused).
 * @return The minimum pan position (x, y), which is always {0, 0}.
 */
EOLIAN static Eina_Position2D
_efl_ui_pan_pan_position_min_get(const Eo *obj EINA_UNUSED, Efl_Ui_Pan_Data *_pd EINA_UNUSED)
{
   return EINA_POSITION2D(0 ,0);
}

/**
 * @brief Gets the size of the content object within the pan widget.
 *
 * @param[in] obj The Efl_Ui_Pan object (unused).
 * @param[in] psd The private data of the Efl_Ui_Pan object.
 * @return The size (width, height) of the content as Eina_Size2D.
 *         Example: { .w = 500, .h = 300 }
 */
EOLIAN static Eina_Size2D
_efl_ui_pan_content_size_get(const Eo *obj EINA_UNUSED, Efl_Ui_Pan_Data *psd)
{
   return EINA_SIZE2D(psd->content_w, psd->content_h);
}

/**
 * @brief Constructor for the Efl_Ui_Pan object.
 *
 * Initializes the pan widget, notably setting it to be a clipped smart object,
 * meaning its content will be clipped to its boundaries.
 *
 * @param[in] obj The Efl_Ui_Pan object being constructed.
 * @param[in] _pd The private data of the Efl_Ui_Pan object (unused).
 * @return The constructed Efl_Ui_Pan object.
 */
EOLIAN static Eo *
_efl_ui_pan_efl_object_constructor(Eo *obj, Efl_Ui_Pan_Data *_pd EINA_UNUSED)
{
   efl_canvas_group_clipped_set(obj, EINA_TRUE);
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   return obj;
}

/**
 * @brief Destructor for the Efl_Ui_Pan object.
 *
 * Cleans up resources used by the pan widget. It explicitly unsets and deletes
 * the content object. The comment notes a potential issue with ownership if
 * other widgets (list, grid, scroller) assume ownership, but the current
 * implementation proceeds with deletion.
 *
 * @param[in] obj The Efl_Ui_Pan object being destructed.
 * @param[in] sd The private data of the Efl_Ui_Pan object (unused).
 */
EOLIAN static void
_efl_ui_pan_efl_object_destructor(Eo *obj, Efl_Ui_Pan_Data *sd EINA_UNUSED)
{
   /* our implementation is a little bit incomplete, efl_content_set(obj, NULL) does not delete the content, However, if we do that, list grid and scroller would fail, because the assume ownership of the content */
   Eo *content = efl_content_unset(obj);
   efl_del(content);
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Callback invoked when the content object is deleted.
 *
 * This function is registered with EVAS_CALLBACK_DEL on the content object.
 * Its purpose is to unset the content from the pan widget if the content
 * is deleted externally.
 *
 * @param[in] data The Efl_Ui_Pan object (passed as user data).
 * @param[in] e The Evas canvas (unused).
 * @param[in] obj The content object that was deleted (unused).
 * @param[in] event_info Additional event information (unused).
 */
static void
_efl_ui_pan_content_del_cb(void *data,
                        Evas *e EINA_UNUSED,
                        Evas_Object *obj EINA_UNUSED,
                        void *event_info EINA_UNUSED)
{
   efl_content_unset(data);
}

/**
 * @brief Callback invoked when the content object is resized.
 *
 * This function is registered with EVAS_CALLBACK_RESIZE on the content object.
 * It updates the pan widget's internal record of the content's size
 * (psd->content_w, psd->content_h), triggers a smart changed event on the pan
 * widget, and emits the EFL_UI_PAN_EVENT_PAN_CONTENT_SIZE_CHANGED event.
 *
 * @param[in] data The Efl_Ui_Pan object (passed as user data).
 * @param[in] e The Evas canvas (unused).
 * @param[in] obj The content object that was resized (unused).
 * @param[in] event_info Additional event information (unused).
 */
static void
_efl_ui_pan_content_resize_cb(void *data,
                           Evas *e EINA_UNUSED,
                           Evas_Object *obj EINA_UNUSED,
                           void *event_info EINA_UNUSED)
{
   Evas_Object *pobj = data;
   EFL_UI_PAN_DATA_GET_OR_RETURN(pobj, psd);

   Eina_Size2D sz = efl_gfx_entity_size_get(psd->content);
   if ((sz.w != psd->content_w) || (sz.h != psd->content_h))
     {
        psd->content_w = sz.w;
        psd->content_h = sz.h;
        evas_object_smart_changed(pobj);
     }
   efl_event_callback_call(pobj, EFL_UI_PAN_EVENT_PAN_CONTENT_SIZE_CHANGED, &sz);
}

/**
 * @brief Sets or replaces the content of the pan widget.
 *
 * This function is an Efl_Content interface implementation.
 * If there's existing content, it's first unset. The new content is then
 * added as a member of the pan widget's canvas group. Callbacks for deletion
 * (EVAS_CALLBACK_DEL) and resize (EVAS_CALLBACK_RESIZE) of the content are
 * registered. The content's visibility is synchronized with the pan widget's
 * visibility. Finally, it triggers a smart changed event and emits the
 * EFL_CONTENT_EVENT_CONTENT_CHANGED event.
 *
 * @param[in] obj The Efl_Ui_Pan object.
 * @param[in,out] psd The private data of the Efl_Ui_Pan object.
 * @param[in] content The new Evas_Object to set as content. Can be NULL to remove content.
 * @return EINA_TRUE on success, EINA_FALSE on failure (though currently always returns EINA_TRUE).
 */
EOLIAN static Eina_Bool
_efl_ui_pan_efl_content_content_set(Evas_Object *obj, Efl_Ui_Pan_Data *psd, Evas_Object *content)
{
   Eina_Size2D sz;

   if (content == psd->content) return EINA_TRUE;
   if (psd->content)
     {
        efl_content_unset(obj);
     }
   if (!content) return EINA_TRUE;

   psd->content = content;
   efl_canvas_group_member_add(obj, content);
   sz = efl_gfx_entity_size_get(psd->content);
   psd->content_w = sz.w;
   psd->content_h = sz.h;
   evas_object_event_callback_add
     (content, EVAS_CALLBACK_DEL, _efl_ui_pan_content_del_cb, obj);
   evas_object_event_callback_add
     (content, EVAS_CALLBACK_RESIZE, _efl_ui_pan_content_resize_cb, obj);

   if (evas_object_visible_get(obj))
     evas_object_show(psd->content);
   else
     evas_object_hide(psd->content);

   evas_object_smart_changed(obj);

   efl_event_callback_call(obj, EFL_CONTENT_EVENT_CONTENT_CHANGED, content);
   return EINA_TRUE;
}

/**
 * @brief Gets the current content object of the pan widget.
 *
 * This function is an Efl_Content interface implementation.
 *
 * @param[in] obj The Efl_Ui_Pan object (unused).
 * @param[in] pd The private data of the Efl_Ui_Pan object.
 * @return The current content Evas_Object, or NULL if no content is set.
 */
EOLIAN static Efl_Gfx_Entity*
_efl_ui_pan_efl_content_content_get(const Eo *obj EINA_UNUSED, Efl_Ui_Pan_Data *pd)
{
   return pd->content;
}

/**
 * @brief Unsets (removes) the content from the pan widget.
 *
 * This function is an Efl_Content interface implementation.
 * It removes the content object from the pan widget's canvas group, deregisters
 * the deletion and resize event callbacks, and resets internal state related
 * to content size and pan position. It emits the EFL_CONTENT_EVENT_CONTENT_CHANGED
 * event with NULL data.
 *
 * @param[in] obj The Efl_Ui_Pan object.
 * @param[in,out] pd The private data of the Efl_Ui_Pan object.
 * @return The Evas_Object that was previously set as content, or NULL if none was set.
 *         The caller may be responsible for deleting this object if it's no longer needed.
 */
EOLIAN static Efl_Gfx_Entity*
_efl_ui_pan_efl_content_content_unset(Eo *obj EINA_UNUSED, Efl_Ui_Pan_Data *pd)
{
   Efl_Gfx_Stack *old_content = pd->content;

   efl_canvas_group_member_remove(obj, pd->content);
   evas_object_event_callback_del_full
     (pd->content, EVAS_CALLBACK_DEL, _efl_ui_pan_content_del_cb, obj);
   evas_object_event_callback_del_full
     (pd->content, EVAS_CALLBACK_RESIZE, _efl_ui_pan_content_resize_cb,
     obj);
   pd->content = NULL;
   pd->content_w = pd->content_h = pd->px = pd->py = 0;
   efl_event_callback_call(obj, EFL_CONTENT_EVENT_CONTENT_CHANGED, NULL);

   return old_content;
}

/**
 * @brief Calculates the visual position of the content within the pan widget.
 *
 * This function is part of Evas' smart object rendering pipeline (group_calculate).
 * It positions the content object based on the pan widget's own position (psd->x, psd->y)
 * and the current pan (scroll) offset (psd->px, psd->py).
 * The content's top-left corner is effectively moved to (pan_x - scroll_x, pan_y - scroll_y).
 *
 * @param[in] obj The Efl_Ui_Pan object.
 * @param[in] psd The private data of the Efl_Ui_Pan object.
 */
EOLIAN static void
_efl_ui_pan_efl_canvas_group_group_calculate(Eo *obj EINA_UNUSED, Efl_Ui_Pan_Data *psd)
{
   efl_canvas_group_need_recalculate_set(obj, EINA_FALSE);
   efl_gfx_entity_position_set(psd->content, EINA_POSITION2D(psd->x - psd->px, psd->y - psd->py));
}
#include "efl_ui_pan.eo.c"
