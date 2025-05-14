#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_WIDGET_PROTECTED
#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_notify.h"
#include "elm_widget_container.h"

#include "elm_notify_part.eo.h"
#include "elm_part_helper.h"

#define MY_CLASS ELM_NOTIFY_CLASS

#define MY_CLASS_NAME "Elm_Notify"
#define MY_CLASS_NAME_LEGACY "elm_notify"

/**
 * @internal
 * @brief Applies the current theme to the notify object.
 *
 * This function determines the appropriate theme group and style for the
 * notify based on its alignment (e.g., "top", "center", "bottom_right")
 * and then applies it to the underlying Edje object.
 *
 * @param obj The notify Evas_Object.
 * @return Eina_Error EFL_UI_THEME_APPLY_ERROR_NONE on success, or an error code on failure.
 */
static Eina_Error
_notify_theme_apply(Evas_Object *obj)
{
   const char *style = elm_widget_style_get(obj);
   const char *position;
   double ax, ay;

   ELM_NOTIFY_DATA_GET(obj, sd);

   ax = sd->horizontal_align;
   ay = sd->vertical_align;

   if (EINA_DBL_EQ(ay, 0.0))
     {
        if (EINA_DBL_EQ(ax, 0.0))
          position = "top_left";
        else if (EINA_DBL_EQ(ax, 1.0))
          position = "top_right";
        else
          position = "top";
     }
   else if (EINA_DBL_EQ(ay, 1.0))
     {
        if (EINA_DBL_EQ(ax, 0.0))
          position = "bottom_left";
        else if (EINA_DBL_EQ(ax, 1.0))
          position = "bottom_right";
        else
          position = "bottom";
     }
   else
     {
        if (EINA_DBL_EQ(ax, 0.0))
          position = "left";
        else if (EINA_DBL_EQ(ax, 1.0))
          position = "right";
        else
          position = "center";
     }

   return elm_widget_theme_object_set(obj, sd->notify, "notify", position, style);
}

/**
 * @internal
 * @brief Moves and resizes the notification based on its content size, alignment, and parent geometry.
 *
 * This function calculates the final position and size of the notification
 * within its parent. It considers the minimum size required by the content,
 * the alignment settings (horizontal_align, vertical_align), and whether
 * the UI is mirrored (RTL).
 *
 * This function moves notification to orientation
 * according to object RTL orientation.
 *
 * @param obj notification object.
 *
 * @param x, y, w, h geometry of object.
 *
 * @internal
 **/
static void
_notify_move_to_orientation(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   Evas_Coord minw = -1, minh = -1;
   double ax, ay;

   ELM_NOTIFY_DATA_GET(obj, sd);

   edje_object_size_min_get(sd->notify, &minw, &minh);
   edje_object_size_min_restricted_calc(sd->notify, &minw, &minh, minw, minh);

   ax = sd->horizontal_align;
   ay = sd->vertical_align;
   if ((efl_ui_mirrored_get(obj)) && (!EINA_DBL_EQ(ax, ELM_NOTIFY_ALIGN_FILL))) ax = 1.0 - ax;

   if (EINA_DBL_EQ(ax, ELM_NOTIFY_ALIGN_FILL)) minw = w;
   if (EINA_DBL_EQ(ay, ELM_NOTIFY_ALIGN_FILL)) minh = h;

   x = x + ((w - minw) * ax);
   y = y + ((h - minh) * ay);

   evas_object_geometry_set(sd->notify, x, y, minw, minh);
}

/**
 * @internal
 * @brief Applies the theme to the block_events object of the notify.
 *
 * The block_events object is a layout used to intercept pointer events
 * outside the notification when `allow_events` is false.
 *
 * @param obj The notify Evas_Object.
 */
static void
_block_events_theme_apply(Evas_Object *obj)
{
   ELM_NOTIFY_DATA_GET(obj, sd);

   const char *style = elm_widget_style_get(obj);

   if (!elm_layout_theme_set
       (sd->block_events, "notify", "block_events", style))
     CRI("Failed to set layout!");
}

/**
 * @internal
 * @brief Sets the mirrored (RTL/LTR) mode for the notify widget.
 *
 * This updates the underlying Edje object's mirrored state and then
 * re-calculates the notification's position to reflect the change.
 *
 * @param obj The notify Evas_Object.
 * @param rtl EINA_TRUE for RTL, EINA_FALSE for LTR.
 */
static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   Evas_Coord x, y, w, h;

   ELM_NOTIFY_DATA_GET(obj, sd);
   edje_object_mirrored_set(sd->notify, rtl);
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   _notify_move_to_orientation(obj, x, y, w, h);
}

/**
 * @internal
 * @brief Evaluates and applies the size of the notify widget based on its parent.
 *
 * If the notify has a parent, this function sets the notify's geometry
 * to match the parent's geometry. If the parent is a window,
 * the position is adjusted to (0,0) relative to the window.
 * This ensures the notify covers the intended area for positioning its content.
 *
 * @param obj The notify Evas_Object.
 */
static void
_sizing_eval(Evas_Object *obj)
{
   Evas_Coord x, y, w, h;

   ELM_NOTIFY_DATA_GET(obj, sd);

   if (!sd->parent) return;
   evas_object_geometry_get(sd->parent, &x, &y, &w, &h);
   if (efl_isa(sd->parent, EFL_UI_WIN_CLASS))
     {
        x = 0;
        y = 0;
     }
   evas_object_geometry_set(obj, x, y, w, h);
}

EOLIAN static Eina_Error
_elm_notify_efl_ui_widget_theme_apply(Eo *obj, Elm_Notify_Data *sd)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   Eina_Error notify_theme_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   _mirrored_set(obj, efl_ui_mirrored_get(obj));

   notify_theme_ret = _notify_theme_apply(obj);
   if (notify_theme_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     return notify_theme_ret;

   if (sd->block_events) _block_events_theme_apply(obj);

   edje_object_scale_set
     (sd->notify, efl_gfx_entity_scale_get(obj) * elm_config_scale_get());

   _sizing_eval(obj);

   if ((int_ret == EFL_UI_THEME_APPLY_ERROR_DEFAULT) ||
       (notify_theme_ret == EFL_UI_THEME_APPLY_ERROR_DEFAULT))
     return EFL_UI_THEME_APPLY_ERROR_DEFAULT;

   return EFL_UI_THEME_APPLY_ERROR_NONE;
}

/* Legacy compat. Note that notify has no text parts in the default theme... */
/**
 * @internal
 * @brief Legacy compatibility function to set text on a part of the notify.
 * @warning The default notify theme does not have text parts.
 * @param obj The notify Eo object (unused).
 * @param sd The notify widget data.
 * @param part The name of the Edje part to set text on.
 * @param label The text to set.
 */
static void
_elm_notify_text_set(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd, const char *part, const char *label)
{
   edje_object_part_text_set(sd->notify, part, label);
}

/* Legacy compat. Note that notify has no text parts in the default theme... */
/**
 * @internal
 * @brief Legacy compatibility function to get text from a part of the notify.
 * @warning The default notify theme does not have text parts.
 * @param obj The notify Eo object (unused).
 * @param sd The notify widget data.
 * @param part The name of the Edje part to get text from.
 * @return The text from the part, or NULL if not found.
 */
static const char*
_elm_notify_text_get(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd, const char *part)
{
   return edje_object_part_text_get(sd->notify, part);
}

/**
 * @internal
 * @brief Recalculates the size and position of the notify.
 *
 * This function first calls _sizing_eval() to update the base geometry
 * based on the parent, and then, if content exists, it calls
 * _notify_move_to_orientation() to position the actual notification
 * element (sd->notify) within that geometry.
 *
 * @param obj The notify Evas_Object.
 */
static void
_calc(Evas_Object *obj)
{
   Evas_Coord x, y, w, h;

   ELM_NOTIFY_DATA_GET(obj, sd);

   _sizing_eval(obj);

   evas_object_geometry_get(obj, &x, &y, &w, &h);

   if (sd->content)
     {
        _notify_move_to_orientation(obj, x, y, w, h);
     }
}

/**
 * @internal
 * @brief Callback invoked when the size hints of the notify's content change.
 *
 * This triggers a recalculation of the notify's layout via _calc().
 *
 * @param data The notify Evas_Object (passed as user data).
 * @param e The Evas canvas (unused).
 * @param obj The content object whose size hints changed (unused).
 * @param event_info Event-specific information (unused).
 */
static void
_changed_size_hints_cb(void *data,
                       Evas *e EINA_UNUSED,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   _calc(data);
}

EOLIAN static Eina_Bool
_elm_notify_efl_ui_widget_widget_sub_object_del(Eo *obj, Elm_Notify_Data *sd, Evas_Object *sobj)
{
   Eina_Bool int_ret = EINA_FALSE;
   int_ret = elm_widget_sub_object_del(efl_super(obj, MY_CLASS), sobj);
   if (!int_ret) return EINA_FALSE;

   if (sobj == sd->content)
     {
        evas_object_event_callback_del_full
          (sobj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
        sd->content = NULL;
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Callback invoked when the area outside the notification is clicked.
 *
 * This callback is active when `allow_events` is EINA_FALSE. It emits
 * the "block,clicked" signal on the notify object.
 *
 * @param data The notify Evas_Object (passed as user data).
 * @param obj The block_events Edje object that received the click (unused).
 * @param emission The Edje signal emitted (unused).
 * @param source The source of the Edje signal (unused).
 */
static void
_block_area_clicked_cb(void *data,
                       Evas_Object *obj EINA_UNUSED,
                       const char *emission EINA_UNUSED,
                       const char *source EINA_UNUSED)
{
   efl_event_callback_legacy_call(data, ELM_NOTIFY_EVENT_BLOCK_CLICKED, NULL);
}

EOLIAN static void
_elm_notify_efl_gfx_entity_size_set(Eo *obj, Elm_Notify_Data *sd, Eina_Size2D sz)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_RESIZE, 0, sz.w, sz.h))
     return;

   efl_gfx_entity_size_set(efl_super(obj, MY_CLASS), sz);

   if (!sd->parent && sd->content)
     {
        Eina_Position2D pos;

        pos = efl_gfx_entity_position_get(obj);
        _notify_move_to_orientation(obj, pos.x, pos.y, sz.w, sz.h);
     }
}

EOLIAN static void
_elm_notify_efl_gfx_entity_position_set(Eo *obj, Elm_Notify_Data *sd, Eina_Position2D pos)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_MOVE, 0, pos.x, pos.y))
     return;

   efl_gfx_entity_position_set(efl_super(obj, MY_CLASS), pos);

   if (!sd->parent && sd->content)
     {
        Evas_Coord w, h;

        evas_object_geometry_get(obj, NULL, NULL, &w, &h);
        _notify_move_to_orientation(obj, pos.x, pos.y, w, h);
     }
}

/**
 * @internal
 * @brief Callback function for the notify timeout timer.
 *
 * When the timer expires, this function hides the notify widget and
 * emits the "timeout" signal. It ensures the notify is visible before
 * attempting to hide and emit the signal.
 *
 * @param data The notify Evas_Object (passed as user data).
 * @return ECORE_CALLBACK_CANCEL to automatically delete the timer.
 */
static Eina_Bool
_timer_cb(void *data)
{
   Evas_Object *obj = data;

   ELM_NOTIFY_DATA_GET(obj, sd);

   sd->timer = NULL;
   if (!evas_object_visible_get(obj)) goto end;

   evas_object_hide(obj);
   sd->in_timeout = EINA_TRUE;
   efl_event_callback_legacy_call(obj, ELM_NOTIFY_EVENT_TIMEOUT, NULL);

end:
   return ECORE_CALLBACK_CANCEL;
}

/**
 * @internal
 * @brief Initializes or re-initializes the timeout timer for the notify.
 *
 * If a timer already exists, it's deleted. If the timeout value
 * (sd->timeout) is greater than 0, a new timer is added.
 *
 * @param obj The notify Evas_Object.
 * @param sd The notify widget data.
 */
static void
_timer_init(Evas_Object *obj,
            Elm_Notify_Data *sd)
{
   ecore_timer_del(sd->timer);
   if (sd->timeout > 0.0)
     sd->timer = ecore_timer_add(sd->timeout, _timer_cb, obj);
   else
     sd->timer = NULL;
}

/**
 * @internal
 * @brief Internal logic to show the notify widget.
 *
 * This function makes the notify and its associated elements (like the
 * block_events object if `allow_events` is false) visible. It also
 * resets timeout-related flags and starts the timeout timer.
 *
 * @param obj The notify Eo object.
 * @param sd The notify widget data.
 */
static void
_elm_notify_show(Eo *obj, Elm_Notify_Data *sd)
{
   sd->had_hidden = EINA_FALSE;
   sd->in_timeout = EINA_FALSE;
   efl_gfx_entity_visible_set(efl_super(obj, MY_CLASS), EINA_TRUE);

   evas_object_show(sd->notify);
   if (!sd->allow_events) evas_object_show(sd->block_events);
   _timer_init(obj, sd);
   elm_object_focus_set(obj, EINA_TRUE);
}

/**
 * @internal
 * @brief Internal logic to hide the notify widget.
 *
 * This function handles the process of hiding the notify. It checks if a
 * specific "hide_finished_signal" is defined in the theme. If so, it emits
 * signals to trigger animations. Otherwise, it directly hides the Evas objects.
 * It also stops the timeout timer.
 *
 * @param obj The notify Eo object (unused, but kept for consistency with show).
 * @param sd The notify widget data.
 */
static void
_elm_notify_hide(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd)
{
   const char *hide_signal;

   if (sd->had_hidden && !sd->in_timeout)
     return;

   hide_signal = edje_object_data_get(sd->notify, "hide_finished_signal");
   if (eina_streq(hide_signal, "on"))
     {
        if (!sd->in_timeout)
          {
             elm_layout_signal_emit(sd->block_events, "elm,state,hide", "elm");
             edje_object_signal_emit(sd->notify, "elm,state,hide", "elm");
          }
     }
   else //for backport supporting: edc without emitting hide finished signal
     {
        efl_gfx_entity_visible_set(efl_super(obj, MY_CLASS), EINA_FALSE);
        evas_object_hide(sd->notify);
        if (sd->allow_events) evas_object_hide(sd->block_events);
     }
   ELM_SAFE_FREE(sd->timer, ecore_timer_del);
}

EOLIAN static void
_elm_notify_efl_gfx_entity_visible_set(Eo *obj, Elm_Notify_Data *sd, Eina_Bool vis)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_VISIBLE, 0, vis))
     return;

   if (vis) _elm_notify_show(obj, sd);
   else _elm_notify_hide(obj, sd);
}

/**
 * @internal
 * @brief Callback invoked when the notify's parent object is deleted.
 *
 * This function unsets the parent of the notify and hides the notify.
 *
 * @param data The notify Evas_Object (passed as user data).
 * @param e The Evas canvas (unused).
 * @param obj The parent object that was deleted (unused).
 * @param event_info Event-specific information (unused).
 */
static void
_parent_del_cb(void *data,
               Evas *e EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   elm_notify_parent_set(data, NULL);
   evas_object_hide(data);
}

/**
 * @internal
 * @brief Callback invoked when the notify's parent object is hidden.
 *
 * This function hides the notify widget.
 *
 * @param data The notify Evas_Object (passed as user data).
 * @param e The Evas canvas (unused).
 * @param obj The parent object that was hidden (unused).
 * @param event_info Event-specific information (unused).
 */
static void
_parent_hide_cb(void *data,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   evas_object_hide(data);
}

/**
 * @internal
 * @brief Sets the content of the notify widget for a given part.
 *
 * For notify, only the "default" part is typically supported.
 * If new content is provided, the old content is deleted. The new content
 * is added as a sub-object and swallowed into the "elm.swallow.content"
 * part of the notify's Edje theme. Callbacks for size hint changes are set up.
 *
 * @param obj The notify Eo object.
 * @param sd The notify widget data.
 * @param part The name of the part to set content to (should be "default" or NULL).
 * @param content The Evas_Object to set as content.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid part).
 */
static Eina_Bool
_elm_notify_content_set(Eo *obj, Elm_Notify_Data *sd, const char *part, Evas_Object *content)
{
   if (part && strcmp(part, "default")) return EINA_FALSE;
   if (sd->content == content) return EINA_TRUE;

   evas_object_del(sd->content);
   sd->content = content;

   if (content)
     {
        elm_widget_sub_object_add(obj, content);
        evas_object_event_callback_add
          (content, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
        edje_object_part_swallow(sd->notify, "elm.swallow.content", content);
     }
   efl_event_callback_call(obj, EFL_CONTENT_EVENT_CONTENT_CHANGED, content);

   _calc(obj);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the content of the notify widget for a given part.
 *
 * For notify, only the "default" part is typically supported.
 *
 * @param obj The notify Eo object (unused).
 * @param sd The notify widget data.
 * @param part The name of the part to get content from (should be "default" or NULL).
 * @return The content Evas_Object, or NULL if not set or part is invalid.
 */
static Evas_Object*
_elm_notify_content_get(const Eo *obj EINA_UNUSED, Elm_Notify_Data *sd, const char *part)
{
   if (part && strcmp(part, "default")) return NULL;

   return sd->content;
}

/**
 * @internal
 * @brief Unsets (removes) the content from the notify widget for a given part.
 *
 * For notify, only the "default" part is typically supported.
 * The content object is unswallowed from the Edje theme and its sub-object
 * relationship is handled. The caller is responsible for deleting the returned object if needed.
 *
 * @param obj The notify Eo object.
 * @param sd The notify widget data.
 * @param part The name of the part to unset content from (should be "default" or NULL).
 * @return The previously set content Evas_Object, or NULL if no content or invalid part.
 */
static Evas_Object*
_elm_notify_content_unset(Eo *obj, Elm_Notify_Data *sd, const char *part)
{
   Evas_Object *content;

   if (part && strcmp(part, "default")) return NULL;
   if (!sd->content) return NULL;

   content = sd->content;
   _elm_widget_sub_object_redirect_to_top(obj, sd->content);
   edje_object_part_unswallow(sd->notify, content);
   efl_event_callback_call(obj, EFL_CONTENT_EVENT_CONTENT_CHANGED, NULL);

   return content;
}

EOLIAN static Eina_Bool
_elm_notify_efl_content_content_set(Eo *obj, Elm_Notify_Data *sd, Evas_Object *content)
{
   return _elm_notify_content_set(obj, sd, "default", content);
}

EOLIAN static Evas_Object*
_elm_notify_efl_content_content_get(const Eo *obj EINA_UNUSED, Elm_Notify_Data *sd)
{
   return _elm_notify_content_get(obj, sd, "default");
}

EOLIAN static Evas_Object*
_elm_notify_efl_content_content_unset(Eo *obj, Elm_Notify_Data *sd)
{
   return _elm_notify_content_unset(obj, sd, "default");
}

/**
 * @internal
 * @brief Callback invoked when the "elm,action,hide,finished" signal is emitted by the notify's Edje object.
 *
 * This typically signifies the end of a hide animation. It finalizes the hiding
 * process by hiding the Evas objects and emitting the ELM_NOTIFY_EVENT_DISMISSED legacy event.
 *
 * @param data The notify Evas_Object (passed as user data).
 * @param obj The Edje object that emitted the signal (unused).
 * @param emission The Edje signal name (unused).
 * @param source The Edje signal source (unused).
 */
static void
_hide_finished_cb(void *data,
                  Evas_Object *obj EINA_UNUSED,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
   ELM_NOTIFY_DATA_GET(data, sd);
   sd->had_hidden = EINA_TRUE;
   evas_object_hide(sd->notify);
   if (!sd->allow_events) evas_object_hide(sd->block_events);
   efl_gfx_entity_visible_set(efl_super(data, MY_CLASS), EINA_FALSE);
   efl_event_callback_legacy_call(data, ELM_NOTIFY_EVENT_DISMISSED, NULL);
}

EOLIAN static void
_elm_notify_efl_canvas_group_group_add(Eo *obj, Elm_Notify_Data *priv)
{
   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   priv->allow_events = EINA_TRUE;

   priv->notify = edje_object_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(priv->notify, obj);

   edje_object_signal_callback_add
      (priv->notify, "elm,action,hide,finished", "elm", _hide_finished_cb, obj);

   elm_widget_can_focus_set(obj, EINA_FALSE);
   elm_notify_align_set(obj, 0.5, 0.0);
}

EOLIAN static void
_elm_notify_efl_canvas_group_group_del(Eo *obj, Elm_Notify_Data *sd)
{
   edje_object_signal_callback_del_full
      (sd->notify, "elm,action,hide,finished", "elm", _hide_finished_cb, obj);
   elm_notify_parent_set(obj, NULL);
   ecore_timer_del(sd->timer);

   ELM_SAFE_FREE(sd->notify, evas_object_del);
   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

EAPI Evas_Object *
elm_notify_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Sets up or tears down event callbacks related to the notify's parent.
 *
 * When a parent is set, this function adds callbacks to the parent for events
 * like deletion, hiding, resizing, and moving. These callbacks allow the notify
 * to react appropriately (e.g., hide itself, recalculate its position).
 * When the parent is unset (NULL), existing callbacks are removed.
 *
 * @param obj The notify Eo object.
 * @param sd The notify widget data.
 * @param parent The new parent Evas_Object, or NULL to unset.
 */
static void
_parent_setup(Eo *obj, Elm_Notify_Data *sd, Evas_Object *parent)
{
   if (sd->parent)
     {
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_RESIZE, _changed_size_hints_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_MOVE, _changed_size_hints_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_DEL, _parent_del_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_HIDE, _parent_hide_cb, obj);
        sd->parent = NULL;
     }

   if (parent)
     {
        sd->parent = parent;
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_RESIZE, _changed_size_hints_cb, obj);
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_MOVE, _changed_size_hints_cb, obj);
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_DEL, _parent_del_cb, obj);
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_HIDE, _parent_hide_cb, obj);
     }

   _calc(obj);
}

EOLIAN static Eo *
_elm_notify_efl_object_constructor(Eo *obj, Elm_Notify_Data *sd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_NOTIFICATION);
   _parent_setup(obj, sd, efl_parent_get(obj));

   return obj;
}

EAPI void
elm_notify_parent_set(Evas_Object *obj,
                      Evas_Object *parent)
{
   ELM_NOTIFY_CHECK(obj);
   ELM_NOTIFY_DATA_GET(obj, sd);
   if (parent)
     efl_ui_widget_sub_object_add(parent, obj);
   _parent_setup(obj, sd, parent);
}

EAPI Evas_Object *
elm_notify_parent_get(const Evas_Object *obj)
{
   ELM_NOTIFY_CHECK(obj) NULL;
   Evas_Object *ret = NULL;
   ret = efl_ui_widget_parent_get((Eo *) obj);
   return ret;
}

EINA_DEPRECATED EAPI void
elm_notify_orient_set(Evas_Object *obj,
                      Elm_Notify_Orient orient)
{
   double horizontal = 0, vertical = 0;

   switch (orient)
     {
      case ELM_NOTIFY_ORIENT_TOP:
         horizontal = 0.5; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_CENTER:
         horizontal = 0.5; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM:
         horizontal = 0.5; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_LEFT:
         horizontal = 0.0; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_RIGHT:
         horizontal = 1.0; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_TOP_LEFT:
         horizontal = 0.0; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_TOP_RIGHT:
         horizontal = 1.0; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM_LEFT:
         horizontal = 0.0; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM_RIGHT:
         horizontal = 1.0; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_LAST:
        break;
     }
   elm_notify_align_set(obj, horizontal, vertical);
}

EINA_DEPRECATED EAPI Elm_Notify_Orient
elm_notify_orient_get(const Evas_Object *obj)
{
   Elm_Notify_Orient orient;
   double horizontal, vertical;

   elm_notify_align_get(obj, &horizontal, &vertical);

   if (EINA_DBL_EQ(horizontal, 0.5) && EINA_DBL_EQ(vertical, 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP;
   else if (EINA_DBL_EQ(horizontal, 0.5) && EINA_DBL_EQ(vertical, 0.5))
     orient = ELM_NOTIFY_ORIENT_CENTER;
   else if (EINA_DBL_EQ(horizontal, 0.5) && EINA_DBL_EQ(vertical, 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM;
   else if (EINA_DBL_EQ(horizontal, 0.0) && EINA_DBL_EQ(vertical, 0.5))
     orient = ELM_NOTIFY_ORIENT_LEFT;
   else if (EINA_DBL_EQ(horizontal, 1.0) && EINA_DBL_EQ(vertical, 0.5))
     orient = ELM_NOTIFY_ORIENT_RIGHT;
   else if (EINA_DBL_EQ(horizontal, 0.0) && EINA_DBL_EQ(vertical, 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP_LEFT;
   else if (EINA_DBL_EQ(horizontal, 1.0) && EINA_DBL_EQ(vertical, 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP_RIGHT;
   else if (EINA_DBL_EQ(horizontal, 0.0) && EINA_DBL_EQ(vertical, 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM_LEFT;
   else if (EINA_DBL_EQ(horizontal, 1.0) && EINA_DBL_EQ(vertical, 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM_RIGHT;
   else
     orient = ELM_NOTIFY_ORIENT_TOP;

   return orient;
}

/**
 * @internal
 * @brief Sets the timeout value for the notification.
 *
 * The notification will automatically hide after this duration (in seconds).
 * A value of 0.0 or less disables the timeout.
 *
 * @param obj The notify Eo object.
 * @param sd The notify widget data.
 * @param timeout The timeout duration in seconds.
 */
EOLIAN static void
_elm_notify_timeout_set(Eo *obj, Elm_Notify_Data *sd, double timeout)
{
   sd->timeout = timeout;
   _timer_init(obj, sd);
}

/**
 * @internal
 * @brief Gets the timeout value for the notification.
 *
 * @param obj The notify Eo object (unused).
 * @param sd The notify widget data.
 * @return The timeout duration in seconds.
 */
EOLIAN static double
_elm_notify_timeout_get(const Eo *obj EINA_UNUSED, Elm_Notify_Data *sd)
{
   return sd->timeout;
}

/**
 * @internal
 * @brief Sets whether events are allowed to pass through to objects below the notification.
 *
 * If `allow` is EINA_FALSE (default), a blocking area is created behind the
 * notification that intercepts mouse events. Clicking this area emits a
 * "block,clicked" signal. If `allow` is EINA_TRUE, events pass through.
 *
 * @param obj The notify Eo object.
 * @param sd The notify widget data.
 * @param allow EINA_TRUE to allow events, EINA_FALSE to block them.
 */
EOLIAN static void
_elm_notify_allow_events_set(Eo *obj, Elm_Notify_Data *sd, Eina_Bool allow)
{
   if (allow == sd->allow_events) return;
   sd->allow_events = allow;
   if (!allow)
     {
        sd->block_events = elm_layout_add(obj);
        _block_events_theme_apply(obj);
        elm_widget_resize_object_set(obj, sd->block_events);
        evas_object_stack_above(sd->notify, sd->block_events);
        elm_layout_signal_callback_add
          (sd->block_events, "elm,action,click", "elm",
          _block_area_clicked_cb, obj);
     }
   else
     {
        evas_object_del(sd->block_events);
        sd->block_events = NULL;
     }
}

/**
 * @internal
 * @brief Gets whether events are allowed to pass through to objects below the notification.
 *
 * @param obj The notify Eo object (unused).
 * @param sd The notify widget data.
 * @return EINA_TRUE if events are allowed, EINA_FALSE if they are blocked.
 */
EOLIAN static Eina_Bool
_elm_notify_allow_events_get(const Eo *obj EINA_UNUSED, Elm_Notify_Data *sd)
{
   return sd->allow_events;
}

/**
 * @internal
 * @brief Sets the alignment of the notification within its parent.
 *
 * Alignment values range from 0.0 to 1.0.
 * For horizontal alignment:
 *   - 0.0 means left aligned.
 *   - 0.5 means center aligned.
 *   - 1.0 means right aligned.
 * For vertical alignment:
 *   - 0.0 means top aligned.
 *   - 0.5 means center aligned.
 *   - 1.0 means bottom aligned.
 * Special value ELM_NOTIFY_ALIGN_FILL can be used to make the notification
 * fill the entire width/height of its parent.
 *
 * @param obj The notify Eo object.
 * @param sd The notify widget data.
 * @param horizontal The horizontal alignment value.
 * @param vertical The vertical alignment value.
 */
EOLIAN static void
_elm_notify_align_set(Eo *obj, Elm_Notify_Data *sd, double horizontal, double vertical)
{
   sd->horizontal_align = horizontal;
   sd->vertical_align = vertical;

   _notify_theme_apply(obj);
   _calc(obj);
}

/**
 * @internal
 * @brief Dismisses the notification.
 *
 * This function triggers the hide animation/process for the notification
 * by emitting "elm,state,hide" signals to its underlying Edje objects.
 *
 * @param obj The notify Eo object (unused).
 * @param sd The notify widget data.
 */
EOLIAN static void
_elm_notify_dismiss(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd)
{
   elm_layout_signal_emit(sd->block_events, "elm,state,hide", "elm");
   edje_object_signal_emit(sd->notify, "elm,state,hide", "elm");
}

/**
 * @internal
 * @brief Gets the alignment of the notification.
 *
 * @param obj The notify Eo object (unused).
 * @param sd The notify widget data.
 * @param horizontal Pointer to store the horizontal alignment value.
 * @param vertical Pointer to store the vertical alignment value.
 */
EOLIAN static void
_elm_notify_align_get(const Eo *obj EINA_UNUSED, Elm_Notify_Data *sd, double *horizontal, double *vertical)
{
   if (horizontal)
     *horizontal = sd->horizontal_align;
   if (vertical)
     *vertical = sd->vertical_align;
}

/**
 * @internal
 * @brief Class constructor for the Elm_Notify widget.
 *
 * This function is called once when the Elm_Notify class is being set up.
 * It registers the legacy type name for the widget.
 *
 * @param klass The Efl_Class for Elm_Notify.
 */
static void
_elm_notify_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Efl.Part begin */

ELM_PART_OVERRIDE(elm_notify, ELM_NOTIFY, Elm_Notify_Data)
ELM_PART_OVERRIDE_TEXT_SET(elm_notify, ELM_NOTIFY, Elm_Notify_Data)
ELM_PART_OVERRIDE_TEXT_GET(elm_notify, ELM_NOTIFY, Elm_Notify_Data)
ELM_PART_OVERRIDE_CONTENT_SET(elm_notify, ELM_NOTIFY, Elm_Notify_Data)
ELM_PART_OVERRIDE_CONTENT_GET(elm_notify, ELM_NOTIFY, Elm_Notify_Data)
ELM_PART_OVERRIDE_CONTENT_UNSET(elm_notify, ELM_NOTIFY, Elm_Notify_Data)
ELM_PART_CONTENT_DEFAULT_GET(elm_notify, "default")

EOLIAN static const char *
_elm_notify_part_efl_ui_l10n_l10n_text_get(const Eo *obj, void *_pd EINA_UNUSED, const char **domain)
{
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS);
   return elm_widget_part_translatable_text_get(pd->obj, pd->part, domain);
}

EOLIAN static void
_elm_notify_part_efl_ui_l10n_l10n_text_set(Eo *obj, void *_pd EINA_UNUSED, const char *label, const char *domain)
{
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS);
   elm_widget_part_translatable_text_set(pd->obj, pd->part, label, domain);
}

#include "elm_notify_part.eo.c"

/* Efl.Part end */

/* Internal EO APIs and hidden overrides */

#define ELM_NOTIFY_EXTRA_OPS \
   ELM_PART_CONTENT_DEFAULT_OPS(elm_notify), \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_notify)

#include "elm_notify_eo.c"
