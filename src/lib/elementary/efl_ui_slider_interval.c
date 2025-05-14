#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_ACCESS_WIDGET_ACTION_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"
#include "efl_ui_slider_private.h"
#include "efl_ui_slider_interval_private.h"

#define MY_CLASS EFL_UI_SLIDER_INTERVAL_CLASS
#define MY_CLASS_PFX efl_ui_slider_interval
#define MY_CLASS_NAME "Efl.Ui.Slider_Interval"

#define SLIDER_DELAY_CHANGED_INTERVAL 0.2
#define SLIDER_STEP 0.05

/**
 * @brief Handles key-based dragging actions for accessibility.
 * @param obj The slider object.
 * @param params A string indicating the direction of drag (e.g., "left", "right").
 * @return EINA_TRUE if the action was handled, EINA_FALSE otherwise.
 */
static Eina_Bool _key_action_drag(Evas_Object *obj, const char *params);

static const Elm_Action key_actions[] = {
   {"drag", _key_action_drag},
   {NULL, NULL}
};

/**
 * @brief Callback function to finalize a value change after a delay.
 *
 * This is used to emit EFL_UI_RANGE_EVENT_STEADY and accessibility events
 * after the user has stopped interacting with the slider for a short period.
 * @param data The slider object.
 * @return ECORE_CALLBACK_CANCEL to automatically remove the timer.
 */
static Eina_Bool
_delay_change(void *data)
{
   EFL_UI_SLIDER_INTERVAL_DATA_GET(data, pd);

   pd->delay = NULL;
   efl_event_callback_call(data, EFL_UI_RANGE_EVENT_STEADY, NULL);

   if (_elm_config->atspi_mode)
     efl_access_object_event_emit(data, EFL_UI_RANGE_EVENT_CHANGED, NULL);

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Helper function to check if the slider orientation is horizontal.
 * @param dir The orientation to check.
 * @return EINA_TRUE if horizontal, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_is_horizontal(Efl_Ui_Layout_Orientation dir)
{
   return efl_ui_layout_orientation_is_horizontal(dir, EINA_TRUE);
}

/**
 * @brief Emits standard range events (changed, min_reached, max_reached).
 * @param obj The slider object.
 * @param sd The private data of the slider.
 */
static void
_emit_events(Eo *obj, Efl_Ui_Slider_Interval_Data *sd)
{
   efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_CHANGED, NULL);
   if (EINA_DBL_EQ(sd->val, sd->val_min))
     efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_MIN_REACHED, NULL);
   if (EINA_DBL_EQ(sd->val, sd->val_max))
     efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_MAX_REACHED, NULL);
}

/**
 * @brief Sets the visual position of the slider's two draggable parts (knobs)
 * based on the current interval values (intvl_from, intvl_to) and range (val_min, val_max).
 * @param obj The slider object.
 */
static void
_val_set(Evas_Object *obj)
{
   double pos, pos2;

   EFL_UI_SLIDER_INTERVAL_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->val_max > sd->val_min)
     {
        pos = (sd->intvl_from - sd->val_min) / (sd->val_max - sd->val_min);
        pos2 = (sd->intvl_to - sd->val_min) / (sd->val_max - sd->val_min);
     }
   else
     {
        pos = 0.0;
        pos2 = 0.0;
     }

   if (pos < 0.0) pos = 0.0;
   else if (pos > 1.0)
     pos = 1.0;

   if (pos2 < 0.0) pos2 = 0.0;
   else if (pos2 > 1.0)
     pos2 = 1.0;

   efl_ui_drag_value_set(efl_part(wd->resize_obj, "efl.draggable.slider"),
                         pos, pos);
   efl_ui_drag_value_set(efl_part(wd->resize_obj, "efl.draggable2.slider"),
                         pos2, pos2);

   // emit accessibility event also if value was changed by API
   if (_elm_config->atspi_mode)
     efl_access_object_event_emit(obj, EFL_UI_RANGE_EVENT_CHANGED, NULL);

   evas_object_smart_changed(obj);
}

/**
 * @brief Fetches the current values from the draggable parts (knobs) of the slider
 * and updates the internal interval values (intvl_from, intvl_to).
 * It also handles emitting change events and managing a delay timer for steady events.
 * @param obj The slider object.
 * @param user_event EINA_TRUE if the change was initiated by user interaction, EINA_FALSE otherwise.
 */
static void
_val_fetch(Evas_Object *obj, Eina_Bool user_event)
{
   double posx = 0.0, posy = 0.0, pos = 0.0, val;
   double posx2 = 0.0, posy2 = 0.0, pos2 = 0.0, val2;

   EFL_UI_SLIDER_INTERVAL_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   efl_ui_drag_value_get(efl_part(wd->resize_obj, "efl.draggable.slider"),
                           &posx, &posy);
   if (efl_ui_layout_orientation_is_horizontal(sd->dir, EINA_TRUE)) pos = posx;
   else pos = posy;

   efl_ui_drag_value_get(efl_part(wd->resize_obj, "efl.draggable2.slider"),
                           &posx2, &posy2);
   if (efl_ui_layout_orientation_is_horizontal(sd->dir, EINA_TRUE)) pos2 = posx2;
   else pos2 = posy2;

   val = (pos * (sd->val_max - sd->val_min)) + sd->val_min;
   val2 = (pos2 * (sd->val_max - sd->val_min)) + sd->val_min;

   if (val > sd->intvl_to)
     {
        val = sd->intvl_to;
        _val_set(obj);
     }
   else if (val2 < sd->intvl_from)
     {
        val2 = sd->intvl_from;
        _val_set(obj);
     }

   if (fabs(val - sd->intvl_from) > DBL_EPSILON)
     {
        sd->val = val;
        sd->intvl_from = val;
        if (user_event)
          {
             efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_CHANGED, NULL);
             efl_event_callback_legacy_call(obj, EFL_UI_RANGE_EVENT_CHANGED, NULL);
             ecore_timer_del(sd->delay);
             sd->delay = ecore_timer_add(SLIDER_DELAY_CHANGED_INTERVAL, _delay_change, obj);
          }
     }

   if (fabs(val2 - sd->intvl_to) > DBL_EPSILON)
     {
        sd->intvl_to = val2;
        if (user_event)
          {
             efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_CHANGED, NULL);
             efl_event_callback_legacy_call(obj, EFL_UI_RANGE_EVENT_CHANGED, NULL);
             ecore_timer_del(sd->delay);
             sd->delay = ecore_timer_add(SLIDER_DELAY_CHANGED_INTERVAL, _delay_change, obj);
          }
     }
}

/**
 * @brief Handles a mouse down event on the slider, determining which knob
 * is closer to the click and should be activated for dragging.
 * The positions button_x and button_y are relative (0.0 to 1.0).
 * @param obj The slider object.
 * @param button_x The relative horizontal position of the mouse click.
 * @param button_y The relative vertical position of the mouse click.
 */
static void
_down_knob(Evas_Object *obj, double button_x, double button_y)
{
   EFL_UI_SLIDER_INTERVAL_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   double posx = 0.0, posy = 0.0, posx2 = 0.0, posy2 = 0.0, diff1, diff2, diff3;

   sd->intvl_flag = 0;

   efl_ui_drag_value_get(efl_part(wd->resize_obj, "efl.draggable.slider"),
                         &posx, &posy);
   efl_ui_drag_value_get(efl_part(wd->resize_obj, "efl.draggable2.slider"),
                         &posx2, &posy2);

   if (efl_ui_layout_orientation_is_horizontal(sd->dir, EINA_TRUE))
     {
        diff1 = fabs(button_x - posx);
        diff2 = fabs(button_x - posx2);
        diff3 = button_x - posx;
     }
   else
     {
        diff1 = fabs(button_y - posy);
        diff2 = fabs(button_y - posy2);
        diff3 = button_y - posy;
     }

   if (diff1 < diff2)
     {
        efl_ui_drag_value_set(efl_part(wd->resize_obj, "efl.draggable.slider"),
                                button_x, button_y);
        sd->intvl_flag = 1;
     }
   else if (diff1 > diff2)
     {
        efl_ui_drag_value_set(efl_part(wd->resize_obj, "efl.draggable2.slider"),
                                button_x, button_y);
        sd->intvl_flag = 2;
     }
   else
     {
        if (diff3 < 0)
          {
             efl_ui_drag_value_set(efl_part(wd->resize_obj, "efl.draggable.slider"),
                                     button_x, button_y);
             sd->intvl_flag = 1;
          }
        else
          {
             efl_ui_drag_value_set(efl_part(wd->resize_obj, "efl.draggable2.slider"),
                                     button_x, button_y);
             sd->intvl_flag = 2;
          }
     }
}

/**
 * @brief Moves the currently active knob based on mouse movement.
 * The positions button_x and button_y are relative (0.0 to 1.0).
 * @param obj The slider object.
 * @param button_x The new relative horizontal position.
 * @param button_y The new relative vertical position.
 */
static void
_move_knob(Evas_Object *obj, double button_x, double button_y)
{
   EFL_UI_SLIDER_INTERVAL_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->intvl_flag == 1)
     {
        efl_ui_drag_value_set(efl_part(wd->resize_obj, "efl.draggable.slider"),
                                button_x, button_y);
     }
   else if (sd->intvl_flag == 2)
     {
        efl_ui_drag_value_set(efl_part(wd->resize_obj, "efl.draggable2.slider"),
                                button_x, button_y);
     }
}

/**
 * @brief EOLIAN function to set the 'from' and 'to' values of the interval.
 * The 'from' value also sets the primary range value of the slider.
 * Values are clamped to the min/max range.
 * @param obj The slider object.
 * @param pd Private data for the slider.
 * @param from The desired 'from' value of the interval.
 * @param to The desired 'to' value of the interval.
 */
EOLIAN static void
_efl_ui_slider_interval_interval_value_set(Eo *obj, Efl_Ui_Slider_Interval_Data *pd, double from, double to)
{
   EFL_UI_SLIDER_INTERVAL_DATA_GET(obj, sd);
   pd->intvl_from = from;
   sd->val = from;
   pd->intvl_to = to;

   if (pd->intvl_from < sd->val_min) {
        pd->intvl_from = sd->val_min;
        sd->val = sd->val_min;
   }
   if (pd->intvl_to > sd->val_max) pd->intvl_to = sd->val_max;

   _val_set(obj);
}

/**
 * @brief EOLIAN function to get the 'from' and 'to' values of the interval.
 * Ensures 'from' is always less than or equal to 'to' in the returned values.
 * @param obj The slider object (unused).
 * @param pd Private data for the slider.
 * @param from Pointer to store the 'from' value. Can be NULL.
 * @param to Pointer to store the 'to' value. Can be NULL.
 */
EOLIAN static void
_efl_ui_slider_interval_interval_value_get(const Eo *obj EINA_UNUSED, Efl_Ui_Slider_Interval_Data *pd, double *from, double *to)
{
   if (from) *from = fmin(pd->intvl_from, pd->intvl_to);
   if (to) *to = fmax(pd->intvl_from, pd->intvl_to);
}

/**
 * @brief Helper function to find the position of a search string (e.g., "horizontal" or "vertical")
 * at the end of the current theme group string.
 * @param cur_group The current theme group string.
 * @param search The string to search for at the end of cur_group.
 * @param len The length of cur_group.
 * @return A pointer to the start of 'search' within 'cur_group' if found at the end, otherwise NULL.
 */
static const char *
_theme_group_modify_pos_get(const char *cur_group, const char *search, size_t len)
{
   const char *pos = NULL;
   const char *temp_str = NULL;

   temp_str = cur_group + len - strlen(search);
   if (temp_str >= cur_group)
     {
        if (!strcmp(temp_str, search))
          pos = temp_str;
     }

   return pos;
}

/**
 * @brief Constructs the appropriate theme group string for the slider based on its
 * current orientation (horizontal/vertical).
 * It modifies the existing theme group by replacing or appending the orientation part.
 * @param obj The slider object.
 * @param sd Private data for the slider, containing orientation information.
 * @return A newly allocated string with the theme group. The caller must free this string.
 */
static char *
_efl_ui_slider_interval_theme_group_get(Evas_Object *obj, Efl_Ui_Slider_Interval_Data *sd)
{
   const char *pos = NULL;
   const char *cur_group = elm_widget_theme_element_get(obj);
   Eina_Strbuf *new_group = eina_strbuf_new();
   size_t len = 0;

   if (cur_group)
     {
        len = strlen(cur_group);
        pos = _theme_group_modify_pos_get(cur_group, "horizontal", len);
        if (!pos)
          pos = _theme_group_modify_pos_get(cur_group, "vertical", len);

        // TODO: change separator when it is decided.
        //       can skip when prev_group == cur_group
        if (!pos)
          {
             eina_strbuf_append(new_group, cur_group);
             eina_strbuf_append(new_group, "/");
          }
        else
          {
             eina_strbuf_append_length(new_group, cur_group, pos - cur_group);
          }
     }

   if (_is_horizontal(sd->dir))
     eina_strbuf_append(new_group, "horizontal");
   else
     eina_strbuf_append(new_group, "vertical");

   return eina_strbuf_release(new_group);
}

/**
 * @brief Central update function for the slider.
 * Fetches values from UI, updates internal state, and marks the object as changed.
 * @param obj The slider object.
 * @param user_event EINA_TRUE if the update is due to direct user interaction.
 */
static void
_slider_update(Evas_Object *obj, Eina_Bool user_event)
{
   _val_fetch(obj, user_event);
   evas_object_smart_changed(obj);
}

/**
 * @brief Callback for "drag" signal from the layout. Updates slider value.
 * @param data The slider object.
 * @param obj Unused.
 * @param emission Unused.
 * @param source Unused.
 */
static void
_drag(void *data,
      Evas_Object *obj EINA_UNUSED,
      const char *emission EINA_UNUSED,
      const char *source EINA_UNUSED)
{
   _slider_update(data, EINA_TRUE);
}

/**
 * @brief Callback for "drag,start" signal. Sets focus, updates slider,
 * emits drag start event, and freezes scrolling on parent widgets.
 * @param data The slider object.
 * @param obj Unused.
 * @param emission Unused.
 * @param source Unused.
 */
static void
_drag_start(void *data,
            Evas_Object *obj EINA_UNUSED,
            const char *emission EINA_UNUSED,
            const char *source EINA_UNUSED)
{
   if (!efl_ui_focus_object_focus_get(data))
     elm_object_focus_set(data, EINA_TRUE);
   _slider_update(data, EINA_TRUE);
   efl_event_callback_call(data, EFL_UI_SLIDER_INTERVAL_EVENT_SLIDER_DRAG_START, NULL);
   efl_ui_widget_scroll_freeze_push(data);
}

/**
 * @brief Callback for "drag,stop" signal. Updates slider, emits drag stop event,
 * and unfreezes scrolling on parent widgets.
 * @param data The slider object.
 * @param obj Unused.
 * @param emission Unused.
 * @param source Unused.
 */
static void
_drag_stop(void *data,
           Evas_Object *obj EINA_UNUSED,
           const char *emission EINA_UNUSED,
           const char *source EINA_UNUSED)
{
   _slider_update(data, EINA_TRUE);
   efl_event_callback_call(data, EFL_UI_SLIDER_INTERVAL_EVENT_SLIDER_DRAG_STOP, NULL);
   efl_ui_widget_scroll_freeze_pop(data);
}

/**
 * @brief Callback for "drag,step" signal. Updates slider value.
 * @param data The slider object.
 * @param obj Unused.
 * @param emission Unused.
 * @param source Unused.
 */
static void
_drag_step(void *data,
           Evas_Object *obj EINA_UNUSED,
           const char *emission EINA_UNUSED,
           const char *source EINA_UNUSED)
{
   _slider_update(data, EINA_TRUE);
}

/**
 * @brief Callback for mouse down events on the spacer object (the track of the slider).
 * This initiates a drag operation by moving the closest knob to the click position.
 * @param data The slider object.
 * @param e Unused.
 * @param obj Unused.
 * @param event_info Mouse down event details.
 */
static void
_spacer_down_cb(void *data,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info)
{
   EFL_UI_SLIDER_INTERVAL_DATA_GET(data, sd);

   Evas_Event_Mouse_Down *ev = event_info;
   Eina_Rect sr;
   double button_x = 0.0, button_y = 0.0;

   sd->spacer_down = EINA_TRUE;
   sr = efl_gfx_entity_geometry_get(sd->spacer);
   sd->downx = ev->canvas.x - sr.x;
   sd->downy = ev->canvas.y - sr.y;
   if (_is_horizontal(sd->dir))
     {
        button_x = ((double)ev->canvas.x - (double)sr.x) / (double)sr.w;
        if (button_x > 1) button_x = 1;
        if (button_x < 0) button_x = 0;
     }
   else
     {
        button_y = ((double)ev->canvas.y - (double)sr.y) / (double)sr.h;
        if (button_y > 1) button_y = 1;
        if (button_y < 0) button_y = 0;
     }

   _down_knob(data, button_x, button_y);

   if (!efl_ui_focus_object_focus_get(data))
     elm_object_focus_set(data, EINA_TRUE);
   _slider_update(data, EINA_TRUE);
   efl_event_callback_call(data, EFL_UI_SLIDER_INTERVAL_EVENT_SLIDER_DRAG_START, NULL);
}

/**
 * @brief Callback for mouse move events on the spacer object.
 * Handles dragging of a knob if a drag operation was initiated by _spacer_down_cb.
 * It also manages scroll freezing and on-hold event flags.
 * @param data The slider object.
 * @param e Unused.
 * @param obj Unused.
 * @param event_info Mouse move event details.
 */
static void
_spacer_move_cb(void *data,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info)
{
   EFL_UI_SLIDER_INTERVAL_DATA_GET(data, sd);

   Eina_Rect sr;
   double button_x = 0.0, button_y = 0.0;
   Evas_Event_Mouse_Move *ev = event_info;

   if (sd->spacer_down)
     {
        Evas_Coord d = 0;

        sr = efl_gfx_entity_geometry_get(sd->spacer);
        if (_is_horizontal(sd->dir))
          d = abs(ev->cur.canvas.x - sr.x - sd->downx);
        else d = abs(ev->cur.canvas.y - sr.y - sd->downy);
        if (d > (_elm_config->thumbscroll_threshold - 1))
          {
             if (!sd->frozen)
               {
                  efl_ui_widget_scroll_freeze_push(data);
                  sd->frozen = EINA_TRUE;
               }
             ev->event_flags &= ~EVAS_EVENT_FLAG_ON_HOLD;
          }

        if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
          {
             if (sd->spacer_down) sd->spacer_down = EINA_FALSE;
             _slider_update(data, EINA_TRUE);
             efl_event_callback_call
               (data, EFL_UI_SLIDER_INTERVAL_EVENT_SLIDER_DRAG_STOP, NULL);
             if (sd->frozen)
               {
                  efl_ui_widget_scroll_freeze_pop(data);
                  sd->frozen = EINA_FALSE;
               }
             return;
          }
        if (_is_horizontal(sd->dir))
          {
             button_x = ((double)ev->cur.canvas.x - (double)sr.x) / (double)sr.w;
             if (button_x > 1) button_x = 1;
             if (button_x < 0) button_x = 0;
          }
        else
          {
             button_y = ((double)ev->cur.canvas.y - (double)sr.y) / (double)sr.h;
             if (button_y > 1) button_y = 1;
             if (button_y < 0) button_y = 0;
          }

        _move_knob(data, button_x, button_y);
        _slider_update(data, EINA_TRUE);
     }
}

/**
 * @brief Callback for mouse up events on the spacer object.
 * Finalizes a drag operation, emits drag stop event, and unfreezes scrolling.
 * @param data The slider object.
 * @param e Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
static void
_spacer_up_cb(void *data,
              Evas *e EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED,
              void *event_info EINA_UNUSED)
{
   EFL_UI_SLIDER_INTERVAL_DATA_GET(data, sd);

   if (!sd->spacer_down) return;
   if (sd->spacer_down) sd->spacer_down = EINA_FALSE;

   _slider_update(data, EINA_TRUE);
   efl_event_callback_call(data, EFL_UI_SLIDER_INTERVAL_EVENT_SLIDER_DRAG_STOP, NULL);

   if (sd->frozen)
     {
        efl_ui_widget_scroll_freeze_pop(data);
        sd->frozen = EINA_FALSE;
     }
}

/**
 * @brief Callback for mouse in events on the slider widget.
 * Pushes a scroll hold to prevent parent scrolling while interacting with the slider.
 * @param data Unused.
 * @param e Unused.
 * @param obj The slider object.
 * @param event_info Unused.
 */
static void
_mouse_in_cb(void *data EINA_UNUSED,
              Evas *e EINA_UNUSED,
              Evas_Object *obj,
              void *event_info EINA_UNUSED)
{
   efl_ui_widget_scroll_hold_push(obj);
}

/**
 * @brief Callback for mouse out events on the slider widget.
 * Pops the scroll hold.
 * @param data Unused.
 * @param e Unused.
 * @param obj The slider object.
 * @param event_info Unused.
 */
static void
_mouse_out_cb(void *data EINA_UNUSED,
              Evas *e EINA_UNUSED,
              Evas_Object *obj,
              void *event_info EINA_UNUSED)
{
   efl_ui_widget_scroll_hold_pop(obj);
}

/**
 * @brief Provides accessibility information for the slider.
 * @param data Unused.
 * @param obj The slider object.
 * @return A string with accessibility information, or NULL. Caller owns the string.
 */
static char *
_access_info_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   const char *txt = efl_ui_widget_access_info_get(obj);

   if (!txt) txt = elm_layout_text_get(obj, NULL);
   if (txt) return strdup(txt);

   return NULL;
}

/**
 * @brief Provides the accessibility state for the slider.
 * @param data Unused.
 * @param obj The slider object.
 * @return A string describing the state (e.g., " state: disabled"), or NULL. Caller owns the string.
 */
static char *
_access_state_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   char *ret;
   Eina_Strbuf *buf = eina_strbuf_new();

   if (efl_ui_widget_disabled_get(obj))
     eina_strbuf_append(buf, " state: disabled");

   if (eina_strbuf_length_get(buf))
     {
        ret = eina_strbuf_string_steal(buf);
        eina_strbuf_free(buf);
        return ret;
     }

   eina_strbuf_free(buf);
   return NULL;
}

/**
 * @brief EOLIAN constructor for the Efl.Ui.Slider_Interval object.
 * Initializes the slider, sets up its theme, default values, and event callbacks.
 * @param obj The Efl.Ui.Slider_Interval object being constructed.
 * @param priv The private data structure for this slider instance.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_ui_slider_interval_efl_object_constructor(Eo *obj, Efl_Ui_Slider_Interval_Data *priv)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);
   char *group;

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "slider_interval");

   obj = efl_constructor(efl_super(obj, MY_CLASS));

   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_SLIDER);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "slider");

   group = _efl_ui_slider_interval_theme_group_get(obj, priv);
   if (elm_widget_theme_object_set(obj, wd->resize_obj,
                                       elm_widget_theme_klass_get(obj),
                                       group,
                                       elm_widget_theme_style_get(obj)) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     CRI("Failed to set layout!");

   free(group);

   priv->dir = EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL;
   priv->val_max = 1.0;
   priv->step = SLIDER_STEP;

   efl_layout_signal_callback_add(obj, "drag", "*", obj, _drag, NULL);
   efl_layout_signal_callback_add(obj, "drag,start", "*", obj, _drag_start, NULL);
   efl_layout_signal_callback_add(obj, "drag,stop", "*", obj, _drag_stop, NULL);
   efl_layout_signal_callback_add(obj, "drag,step", "*", obj, _drag_step, NULL);
   efl_layout_signal_callback_add(obj, "drag,page", "*", obj, _drag_stop, NULL);

   priv->spacer = efl_add(EFL_CANVAS_RECTANGLE_CLASS, obj,
                          efl_gfx_color_set(efl_added, 0, 0, 0, 0));

   efl_content_set(efl_part(obj, "efl.bar"), priv->spacer);

   evas_object_event_callback_add
     (priv->spacer, EVAS_CALLBACK_MOUSE_DOWN, _spacer_down_cb, obj);
   evas_object_event_callback_add
     (priv->spacer, EVAS_CALLBACK_MOUSE_MOVE, _spacer_move_cb, obj);
   evas_object_event_callback_add
     (priv->spacer, EVAS_CALLBACK_MOUSE_UP, _spacer_up_cb, obj);
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_MOUSE_IN, _mouse_in_cb, obj);
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_MOUSE_OUT, _mouse_out_cb, obj);

   efl_ui_widget_focus_allow_set(obj, EINA_TRUE);

   _elm_access_object_register(obj, wd->resize_obj);
   _elm_access_text_set
     (_elm_access_info_get(obj), ELM_ACCESS_TYPE, E_("slider"));
   _elm_access_callback_set
     (_elm_access_info_get(obj), ELM_ACCESS_INFO, _access_info_cb, NULL);
   _elm_access_callback_set
     (_elm_access_info_get(obj), ELM_ACCESS_STATE, _access_state_cb, NULL);

   evas_object_smart_changed(obj);

   return obj;
}

/**
 * @brief EOLIAN destructor for the Efl.Ui.Slider_Interval object.
 * Cleans up resources, such as timers.
 * @param obj The Efl.Ui.Slider_Interval object being destructed.
 * @param pd The private data structure for this slider instance.
 */
EOLIAN static void
_efl_ui_slider_interval_efl_object_destructor(Eo *obj,
                                      Efl_Ui_Slider_Interval_Data *pd)
{
   ecore_timer_del(pd->delay);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief EOLIAN function to apply the theme to the slider.
 * This function updates the theme group based on orientation and inversion state,
 * then calls the parent's theme apply function and updates the slider's visual state.
 * @param obj The slider object.
 * @param sd The private data for the slider.
 * @return Eina_Error indicating success or failure of theme application.
 */
EOLIAN static Eina_Error
_efl_ui_slider_interval_efl_ui_widget_theme_apply(Eo *obj, Efl_Ui_Slider_Interval_Data *sd)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EFL_UI_THEME_APPLY_ERROR_GENERIC);
   char *group;

   group = _efl_ui_slider_interval_theme_group_get(obj, sd);
   if (group)
     {
        elm_widget_theme_element_set(obj, group);
        free(group);
     }

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   if (efl_ui_layout_orientation_is_inverted(sd->dir))
     efl_layout_signal_emit(obj, "efl,state,inverted,on", "efl");
   else
     efl_layout_signal_emit(obj, "efl,state,inverted,off", "efl");

   _val_set(obj);

   efl_layout_signal_process(wd->resize_obj, EINA_FALSE);
   evas_object_smart_changed(obj);

   return int_ret;
}

/**
 * @brief Moves the slider's primary knob up (or right, depending on orientation and inversion) by one step.
 * This is typically used for keyboard navigation or programmatic control.
 * @param obj The slider object.
 */
static void
_drag_up(Evas_Object *obj)
{
   double step;
   double relative_step;

   EFL_UI_SLIDER_INTERVAL_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   step = sd->step;

   if (efl_ui_layout_orientation_is_inverted(sd->dir)) step *= -1.0;

   relative_step = step/(sd->val_max - sd->val_min);

   efl_ui_drag_step_move(efl_part(wd->resize_obj, "efl.draggable.slider"),
                           relative_step, relative_step);
   _slider_update(obj, EINA_TRUE);
}

/**
 * @brief Moves the slider's primary knob down (or left, depending on orientation and inversion) by one step.
 * This is typically used for keyboard navigation or programmatic control.
 * @param obj The slider object.
 */
static void
_drag_down(Evas_Object *obj)
{
   double step;
   double relative_step;

   EFL_UI_SLIDER_INTERVAL_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   step = -sd->step;

   if (efl_ui_layout_orientation_is_inverted(sd->dir)) step *= -1.0;

   relative_step = step/(sd->val_max - sd->val_min);

   efl_ui_drag_step_move(efl_part(wd->resize_obj, "efl.draggable.slider"),
                           relative_step, relative_step);
   _slider_update(obj, EINA_TRUE);
}

// Documentation for _key_action_drag is at its declaration.
static Eina_Bool
_key_action_drag(Evas_Object *obj, const char *params)
{
   EFL_UI_SLIDER_INTERVAL_DATA_GET(obj, sd);
   const char *dir = params;
   double old_value, new_value;

   old_value = efl_ui_range_value_get(obj);

   if (!strcmp(dir, "left"))
     {
        if (!_is_horizontal(sd->dir))
          return EINA_FALSE;
        if (!efl_ui_layout_orientation_is_inverted(sd->dir))
          _drag_down(obj);
        else _drag_up(obj);
     }
   else if (!strcmp(dir, "right"))
     {
        if (!_is_horizontal(sd->dir))
          return EINA_FALSE;
        if (!efl_ui_layout_orientation_is_inverted(sd->dir))
          _drag_up(obj);
        else _drag_down(obj);
     }
   else if (!strcmp(dir, "up"))
     {
        if (_is_horizontal(sd->dir))
          return EINA_FALSE;
        if (efl_ui_layout_orientation_is_inverted(sd->dir))
          _drag_up(obj);
        else _drag_down(obj);
     }
   else if (!strcmp(dir, "down"))
     {
        if (_is_horizontal(sd->dir))
          return EINA_FALSE;
        if (efl_ui_layout_orientation_is_inverted(sd->dir))
          _drag_down(obj);
        else _drag_up(obj);
     }
   else return EINA_FALSE;

   new_value = efl_ui_range_value_get(obj);
   return !EINA_DBL_EQ(new_value, old_value);
}

/**
 * @brief EOLIAN function to handle accessibility activation requests.
 * Translates activation types (UP, DOWN, LEFT, RIGHT) into slider drag actions.
 * @param obj The slider object.
 * @param sd Private data for the slider.
 * @param act The type of activation requested (e.g., EFL_UI_ACTIVATE_UP).
 * @return EINA_TRUE if the activation was handled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_slider_interval_efl_ui_widget_on_access_activate(Eo *obj, Efl_Ui_Slider_Interval_Data *sd, Efl_Ui_Activate act)
{
   if (efl_ui_widget_disabled_get(obj)) return EINA_FALSE;
   if (act == EFL_UI_ACTIVATE_DEFAULT) return EINA_FALSE;

   if ((act == EFL_UI_ACTIVATE_UP) ||
       (act == EFL_UI_ACTIVATE_RIGHT))
     {
        if (!efl_ui_layout_orientation_is_inverted(sd->dir))
          _drag_up(obj);
        else _drag_down(obj);
     }
   else if ((act == EFL_UI_ACTIVATE_DOWN) ||
            (act == EFL_UI_ACTIVATE_LEFT))
     {
        if (!efl_ui_layout_orientation_is_inverted(sd->dir))
          _drag_down(obj);
        else _drag_up(obj);
     }

   _slider_update(obj, EINA_TRUE);

   return EINA_TRUE;
}

// _slider_efl_ui_widget_widget_input_event_handler
ELM_WIDGET_KEY_DOWN_DEFAULT_IMPLEMENT(slider, Efl_Ui_Slider_Interval_Data)

/**
 * @brief EOLIAN function to handle input events for the slider widget.
 * This processes key down events (using the default Elm_Widget handler),
 * key up events (ignored), and pointer wheel events to adjust the slider value.
 * @param obj The slider object.
 * @param sd Private data for the slider.
 * @param eo_event The Efl_Event containing details about the input event.
 * @param src The source object of the event.
 * @return EINA_TRUE if the event was handled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_slider_interval_efl_ui_widget_widget_input_event_handler(Eo *obj, Efl_Ui_Slider_Interval_Data *sd, const Efl_Event *eo_event, Evas_Object *src)
{
   Eo *ev = eo_event->info;

   if (eo_event->desc == EFL_EVENT_KEY_DOWN)
     {
        if (!_slider_efl_ui_widget_widget_input_event_handler(obj, sd, eo_event, src))
          return EINA_FALSE;
     }
   else if (eo_event->desc == EFL_EVENT_KEY_UP)
     {
        return EINA_FALSE;
     }
   else if (eo_event->desc == EFL_EVENT_POINTER_WHEEL)
     {
        if (efl_input_processed_get(ev)) return EINA_FALSE;
        if (efl_input_pointer_wheel_delta_get(ev) < 0)
          {
             if (_is_horizontal(sd->dir))
               _drag_up(obj);
             else
               _drag_down(obj);
          }
        else
          {
             if (_is_horizontal(sd->dir))
               _drag_down(obj);
             else
               _drag_up(obj);
          }
        efl_input_processed_set(ev, EINA_TRUE);
     }
   else return EINA_FALSE;

   _slider_update(obj, EINA_TRUE);

   return EINA_TRUE;
}

/**
 * @brief EOLIAN function to set the minimum and maximum displayable range of the slider.
 * @param obj The slider object.
 * @param sd Private data for the slider.
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 */
EOLIAN static void
_efl_ui_slider_interval_efl_ui_range_display_range_limits_set(Eo *obj, Efl_Ui_Slider_Interval_Data *sd, double min, double max)
{
   if (max < min)
     {
        ERR("Wrong params. min(%lf) is greater than max(%lf).", min, max);
        return;
     }
   if (EINA_DBL_EQ(max, min))
     {
        ERR("min and max must have a different value");
        return;
     }
   if ((EINA_DBL_EQ(sd->val_min, min)) && (EINA_DBL_EQ(sd->val_max, max))) return;
   sd->val_min = min;
   sd->val_max = max;
   if (sd->val < sd->val_min) sd->val = sd->val_min;
   if (sd->val > sd->val_max) sd->val = sd->val_max;

   _val_set(obj);
}

/**
 * @brief EOLIAN function to get the minimum and maximum displayable range of the slider.
 * @param obj The slider object (unused).
 * @param sd Private data for the slider.
 * @param min Pointer to store the minimum value. Can be NULL.
 * @param max Pointer to store the maximum value. Can be NULL.
 */
EOLIAN static void
_efl_ui_slider_interval_efl_ui_range_display_range_limits_get(const Eo *obj EINA_UNUSED, Efl_Ui_Slider_Interval_Data *sd, double *min, double *max)
{
   if (min) *min = sd->val_min;
   if (max) *max = sd->val_max;
}

/**
 * @brief EOLIAN function to set the primary value of the slider.
 * This corresponds to the 'from' value of the interval.
 * @param obj The slider object.
 * @param sd Private data for the slider.
 * @param val The value to set.
 */
EOLIAN static void
_efl_ui_slider_interval_efl_ui_range_display_range_value_set(Eo *obj, Efl_Ui_Slider_Interval_Data *sd, double val)
{
   if (val < sd->val_min)
     {
        ERR("Error, value is less than minimum");
        return;
     }
   if (val > sd->val_max)
     {
        ERR("Error, value is greater than maximum");
        return;
     }

   if (EINA_DBL_EQ(val, sd->val)) return;
   sd->val = val;

   _emit_events(obj, sd);
   _val_set(obj);
}

/**
 * @brief EOLIAN function to get the primary value of the slider.
 * This corresponds to the 'from' value of the interval.
 * @param obj The slider object (unused).
 * @param sd Private data for the slider.
 * @return The primary value of the slider.
 */
EOLIAN static double
_efl_ui_slider_interval_efl_ui_range_display_range_value_get(const Eo *obj EINA_UNUSED, Efl_Ui_Slider_Interval_Data *sd)
{
   return sd->val;
}

/**
 * @brief EOLIAN function to get the step value for interactive changes.
 * @param obj The slider object (unused).
 * @param sd Private data for the slider.
 * @return The step value.
 */
EOLIAN static double
_efl_ui_slider_interval_efl_ui_range_interactive_range_step_get(const Eo *obj EINA_UNUSED, Efl_Ui_Slider_Interval_Data *sd)
{
   return sd->step;
}

/**
 * @brief EOLIAN function to set the step value for interactive changes.
 * The step value must be greater than 0.
 * @param obj The slider object (unused).
 * @param sd Private data for the slider.
 * @param step The step value to set.
 */
EOLIAN static void
_efl_ui_slider_interval_efl_ui_range_interactive_range_step_set(Eo *obj EINA_UNUSED, Efl_Ui_Slider_Interval_Data *sd, double step)
{
   if (step <= 0)
     {
        ERR("Wrong param. The step(%lf) should be greater than 0.0", step);
        return;
     }

   if (EINA_DBL_EQ(sd->step, step)) return;

   sd->step = step;
}

/**
 * @brief EOLIAN function to set the orientation of the slider.
 * @param obj The slider object.
 * @param sd Private data for the slider.
 * @param dir The desired orientation (horizontal or vertical).
 */
EOLIAN static void
_efl_ui_slider_interval_efl_ui_layout_orientable_orientation_set(Eo *obj, Efl_Ui_Slider_Interval_Data *sd, Efl_Ui_Layout_Orientation dir)
{
   sd->dir = dir;

   efl_ui_widget_theme_apply(obj);
}

/**
 * @brief EOLIAN function to get the orientation of the slider.
 * @param obj The slider object (unused).
 * @param sd Private data for the slider.
 * @return The current orientation of the slider.
 */
EOLIAN static Efl_Ui_Layout_Orientation
_efl_ui_slider_interval_efl_ui_layout_orientable_orientation_get(const Eo *obj EINA_UNUSED, Efl_Ui_Slider_Interval_Data *sd)
{
   return sd->dir;
}

// A11Y Accessibility

/**
 * @brief EOLIAN function to get the accessibility actions for the slider.
 * Provides actions for dragging the slider knobs via accessibility interfaces.
 * @param obj The slider object (unused).
 * @param pd Private data for the slider (unused).
 * @return A static array of Efl_Access_Action_Data describing the available actions.
 *         Example: { "drag,left", "drag", "left", _key_action_drag}
 *                  This defines an action named "drag,left", of type "drag",
 *                  with parameter "left", handled by _key_action_drag function.
 */
EOLIAN const Efl_Access_Action_Data *
_efl_ui_slider_interval_efl_access_widget_action_elm_actions_get(const Eo *obj EINA_UNUSED, Efl_Ui_Slider_Interval_Data *pd EINA_UNUSED)
{
   static Efl_Access_Action_Data atspi_actions[] = {
          { "drag,left", "drag", "left", _key_action_drag},
          { "drag,right", "drag", "right", _key_action_drag},
          { "drag,up", "drag", "up", _key_action_drag},
          { "drag,down", "drag", "down", _key_action_drag},
          { NULL, NULL, NULL, NULL}
   };
   return &atspi_actions[0];
}
// A11Y Accessibility - END

#include "efl_ui_slider_interval.eo.c"
