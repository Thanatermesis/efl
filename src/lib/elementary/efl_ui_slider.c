#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_ACCESS_WIDGET_ACTION_PROTECTED
#define ELM_LAYOUT_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "efl_ui_slider_private.h"
#include "elm_widget_layout.h"
#include "elm_part_helper.h"

#define MY_CLASS EFL_UI_SLIDER_CLASS
#define MY_CLASS_PFX efl_ui_slider
#define MY_CLASS_NAME "Efl.Ui.Slider"

#define SLIDER_DELAY_CHANGED_INTERVAL 0.2
#define SLIDER_STEP 0.05

static Eina_Bool _key_action_drag(Evas_Object *obj, const char *params);

/**
 * @brief Defines the mapping between key action names and their handler functions.
 *
 * This array is used by the accessibility framework to trigger actions on the widget.
 * Each Elm_Action struct consists of:
 * - char *name: The name of the action (e.g., "drag"). This name is referenced
 *               by Efl_Access_Action_Data.name.
 * - Evas_Object_Action_Cb func: The callback function to execute for this action.
 *                               The third element of Efl_Access_Action_Data (params)
 *                               is passed as the `params` argument to this callback.
 *
 * Example Efl_Access_Action_Data entry:
 *   { "drag,left", "drag", "left", _key_action_drag}
 *   Here, "drag" would match the name in this key_actions array, and "left"
 *   would be passed as `params` to `_key_action_drag`.
 */
static const Elm_Action key_actions[] = {
   {"drag", _key_action_drag},
   {NULL, NULL}
};

/**
 * @brief Callback function for a timer, used to signal that the slider value
 * has been stable for a certain period after a change.
 *
 * This function emits the EFL_UI_RANGE_EVENT_STEADY event. If AT-SPI
 * accessibility is enabled, it also emits an EFL_UI_RANGE_EVENT_CHANGED
 * accessibility event.
 *
 * @param data The slider widget (Evas_Object *).
 * @return ECORE_CALLBACK_CANCEL to automatically delete the timer.
 */
static Eina_Bool
_delay_change(void *data)
{
   EFL_UI_SLIDER_DATA_GET(data, sd);

   sd->delay = NULL;
   efl_event_callback_call(data, EFL_UI_RANGE_EVENT_STEADY, NULL);

   if (_elm_config->atspi_mode)
     efl_access_object_event_emit(data, EFL_UI_RANGE_EVENT_CHANGED, NULL);

   return ECORE_CALLBACK_CANCEL;
}

static inline Eina_Bool
_is_horizontal(Efl_Ui_Layout_Orientation dir)
{
   return efl_ui_layout_orientation_is_horizontal(dir, EINA_TRUE);
}

/**
 * @brief Determines if the slider's visual representation should be inverted.
 *
 * Inversion means that the minimum value might be on the right (for horizontal sliders)
 * or at the top (for vertical sliders). This depends on the slider's orientation
 * (sd->dir) and the UI's mirrored state (efl_ui_mirrored_get).
 *
 * @param obj The slider widget.
 * @param sd The private data of the slider widget.
 * @return EINA_TRUE if the slider is visually inverted, EINA_FALSE otherwise.
 */
static Eina_Bool
_is_inverted(Eo *obj, Efl_Ui_Slider_Data *sd)
{
   Eina_Bool mirrored, inverted;

   mirrored = efl_ui_mirrored_get(obj);
   inverted = efl_ui_layout_orientation_is_inverted(sd->dir);

   if ((_is_horizontal(sd->dir) && (mirrored ^ inverted)) ||
       (!_is_horizontal(sd->dir) && inverted))
     return EINA_TRUE;
   else
     return EINA_FALSE;
}

/**
 * @brief Emits standard range events based on the current slider value.
 *
 * This includes:
 * - EFL_UI_RANGE_EVENT_CHANGED: Always emitted.
 * - EFL_UI_RANGE_EVENT_MIN_REACHED: Emitted if the value is at its minimum.
 * - EFL_UI_RANGE_EVENT_MAX_REACHED: Emitted if the value is at its maximum.
 * - Accessibility EFL_UI_RANGE_EVENT_CHANGED: Emitted if AT-SPI is enabled.
 *
 * @param obj The slider widget.
 * @param sd The private data of the slider widget.
 */
static void
_emit_events(Eo *obj, Efl_Ui_Slider_Data *sd)
{
   efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_CHANGED, NULL);

   if (EINA_DBL_EQ(sd->val, sd->val_min))
     efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_MIN_REACHED, NULL);

   if (EINA_DBL_EQ(sd->val, sd->val_max))
     efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_MAX_REACHED, NULL);

   // emit accessibility event also if value was changed by API
   if (_elm_config->atspi_mode)
     efl_access_object_event_emit(obj, EFL_UI_RANGE_EVENT_CHANGED, NULL);
}

/**
 * @brief Updates the slider's value as a result of user interaction and
 * manages the "delay changed" timer.
 *
 * This function sets the new value using efl_ui_range_value_set() and then
 * (re)starts a timer. When this timer expires (_delay_change is called),
 * the EFL_UI_RANGE_EVENT_STEADY event is emitted.
 *
 * @param obj The slider widget.
 * @param value The new value to set for the slider.
 */
static void
_user_value_update(Evas_Object *obj, double value)
{
   EFL_UI_SLIDER_DATA_GET(obj, sd);

   efl_ui_range_value_set(obj, value);

   ecore_timer_del(sd->delay);
   sd->delay = ecore_timer_add(SLIDER_DELAY_CHANGED_INTERVAL, _delay_change, obj);

   evas_object_smart_changed(obj);
}

/**
 * @brief Updates the slider's value by a specified step.
 *
 * The step is added to the current value, respecting the slider's min/max limits.
 * If the slider is inverted, the step direction is reversed.
 *
 * @param obj The slider widget.
 * @param step The amount to change the value by. Can be positive or negative.
 *             The sign is interpreted relative to a non-inverted slider
 *             (positive means increase / move right or up).
 */
static void
_step_value_update(Evas_Object *obj, double step)
{
   double value;

   EFL_UI_SLIDER_DATA_GET(obj, sd);

   if (_is_inverted(obj, sd))
     step *= -1.0;

   value = CLAMP(sd->val + step, sd->val_min, sd->val_max);
   _user_value_update(obj, value);

}

/**
 * @brief Calculates and applies the slider's value based on the current
 * position of its draggable thumb.
 *
 * This function reads the relative position (0.0 to 1.0) of the draggable part,
 * considers slider inversion, and then translates this position into a value
 * within the slider's min/max range. If the calculated value differs from the
 * current value, _user_value_update() is called.
 *
 * @param obj The slider widget.
 */
static void
_drag_value_fetch(Evas_Object *obj)
{
   EFL_UI_SLIDER_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   double posx = 0.0, posy = 0.0, pos = 0.0, val;

   efl_ui_drag_value_get(efl_part(wd->resize_obj, "efl.draggable.slider"),
                           &posx, &posy);
   if (_is_horizontal(sd->dir)) pos = posx;
   else pos = posy;

   if (_is_inverted(obj, sd))
     pos = 1.0 - pos;

   val = (pos * (sd->val_max - sd->val_min)) + sd->val_min;

   if (fabs(val - sd->val) > DBL_EPSILON)
     {
        _user_value_update(obj, val);
     }
}

/**
 * @brief Adjusts the slider's current value to the nearest valid step increment.
 *
 * If a step value is defined (pd->step is non-zero), this function calculates
 * the closest value to pd->val that is a multiple of the relative step size
 * and updates the slider to this new value using _user_value_update().
 * This is typically used to "snap" the slider to discrete values after a
 * drag or click operation.
 *
 * @param obj The slider widget.
 * @param pd The private data of the slider widget.
 */
static void
_adjust_to_step(Efl_Ui_Slider *obj,  Efl_Ui_Slider_Data *pd)
{
   if (EINA_DBL_NONZERO(pd->step))
     {
        double relative_step = pd->step/(pd->val_max - pd->val_min);
        double new_value = (round(pd->val/relative_step))*relative_step;
        _user_value_update(obj, new_value);
     }
}

/**
 * @brief Updates the visual position of the draggable thumb based on the
 * current slider value.
 *
 * This function calculates the relative position (0.0 to 1.0) of the thumb
 * based on the current value (sd->val) within the min/max range. It considers
 * slider inversion and then sets the position of the draggable part.
 *
 * @param obj The slider widget.
 */
static void
_drag_value_update(Evas_Object *obj)
{
   EFL_UI_SLIDER_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   double pos;

   pos = (sd->val - sd->val_min) / (sd->val_max - sd->val_min);

   if (_is_inverted(obj, sd))
     pos = 1.0 - pos;

   efl_ui_drag_value_set(efl_part(wd->resize_obj, "efl.draggable.slider"),
                           pos, pos);

   evas_object_smart_changed(obj);
}

static void
_drag(void *data,
      Evas_Object *obj EINA_UNUSED,
      const char *emission EINA_UNUSED,
      const char *source EINA_UNUSED)
{
   // Called when the draggable part of the slider is moved.
   _drag_value_fetch(data);
}

/**
 * @brief Callback for the "drag,start" signal from the layout.
 *
 * Sets focus to the slider if not already focused, emits the
 * EFL_UI_SLIDER_EVENT_SLIDER_DRAG_START event, fetches the initial drag value,
 * and freezes widget scrolling to prevent interference.
 *
 * @param data The slider widget.
 * @param obj The Evas object emitting the signal (unused).
 * @param emission The emission string (unused).
 * @param source The source string (unused).
 */
static void
_drag_start(void *data,
            Evas_Object *obj EINA_UNUSED,
            const char *emission EINA_UNUSED,
            const char *source EINA_UNUSED)
{
   if (!efl_ui_focus_object_focus_get(data))
     elm_object_focus_set(data, EINA_TRUE);
   efl_event_callback_call(data, EFL_UI_SLIDER_EVENT_SLIDER_DRAG_START, NULL);
   _drag_value_fetch(data);
   elm_widget_scroll_freeze_push(data);
}

/**
 * @brief Callback for the "drag,stop" signal from the layout.
 *
 * Fetches the final drag value, emits the
 * EFL_UI_SLIDER_EVENT_SLIDER_DRAG_STOP event, and unfreezes widget scrolling.
 *
 * @param data The slider widget.
 * @param obj The Evas object emitting the signal (unused).
 * @param emission The emission string (unused).
 * @param source The source string (unused).
 */
static void
_drag_stop(void *data,
           Evas_Object *obj EINA_UNUSED,
           const char *emission EINA_UNUSED,
           const char *source EINA_UNUSED)
{
   _drag_value_fetch(data);
   efl_event_callback_call(data, EFL_UI_SLIDER_EVENT_SLIDER_DRAG_STOP, NULL);
   elm_widget_scroll_freeze_pop(data);
}

/**
 * @brief Callback for the "drag,step" signal from the layout.
 *
 * This is typically triggered by discrete drag actions (e.g., keyboard).
 * Fetches the value based on the new drag position.
 *
 * @param data The slider widget.
 * @param obj The Evas object emitting the signal (unused).
 * @param emission The emission string (unused).
 * @param source The source string (unused).
 */
static void
_drag_step(void *data,
           Evas_Object *obj EINA_UNUSED,
           const char *emission EINA_UNUSED,
           const char *source EINA_UNUSED)
{
   _drag_value_fetch(data);
}

/**
 * @brief Increases the slider value by one step.
 *
 * This function is used for discrete increments, typically from keyboard or
 * accessibility actions. It considers the slider's orientation and inversion
 * to determine the correct direction of change.
 * For a non-inverted horizontal slider, this moves the thumb to the right.
 * For a non-inverted vertical slider, this moves the thumb upwards.
 *
 * @param obj The slider widget.
 */
static void
_drag_up(Evas_Object *obj)
{
   double step;
   double relative_step;

   EFL_UI_SLIDER_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   step = sd->step;

   if (efl_ui_layout_orientation_is_inverted(sd->dir)) step *= -1.0;

   relative_step = step/(sd->val_max - sd->val_min);

   efl_ui_drag_step_move(efl_part(wd->resize_obj, "efl.draggable.slider"),
                           relative_step, relative_step);

   _step_value_update(obj, step);
}

/**
 * @brief Decreases the slider value by one step.
 *
 * This function is used for discrete decrements, typically from keyboard or
 * accessibility actions. It considers the slider's orientation and inversion
 * to determine the correct direction of change.
 * For a non-inverted horizontal slider, this moves the thumb to the left.
 * For a non-inverted vertical slider, this moves the thumb downwards.
 *
 * @param obj The slider widget.
 */
static void
_drag_down(Evas_Object *obj)
{
   double step;
   double relative_step;

   EFL_UI_SLIDER_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   step = -sd->step;

   if (efl_ui_layout_orientation_is_inverted(sd->dir)) step *= -1.0;

   relative_step = step/(sd->val_max - sd->val_min);

   efl_ui_drag_step_move(efl_part(wd->resize_obj, "efl.draggable.slider"),
                           relative_step, relative_step);

   _step_value_update(obj, step);
}

/**
 * @brief Handles keyboard-initiated drag actions.
 *
 * This function is called by the accessibility framework or key event handlers
 * to change the slider value based on directional input.
 *
 * @param obj The slider widget.
 * @param params A string indicating the direction of the drag.
 *               Expected values: "left", "right", "up", "down".
 * @return EINA_TRUE if the slider value changed as a result of the action,
 *         EINA_FALSE otherwise (e.g., action not applicable to current orientation,
 *         or value did not change).
 */
static Eina_Bool
_key_action_drag(Evas_Object *obj, const char *params)
{
   EFL_UI_SLIDER_DATA_GET(obj, sd);
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

// _slider_efl_ui_widget_widget_input_event_handler
ELM_WIDGET_KEY_DOWN_DEFAULT_IMPLEMENT(slider, Efl_Ui_Slider_Data)

/**
 * @brief Handles input events (key presses, mouse wheel) for the slider widget.
 *
 * For key down events, it uses the default Elm widget key handling and then
 * fetches the drag value.
 * For mouse wheel events, it adjusts the slider value up or down based on the
 * wheel direction and slider orientation.
 *
 * @param obj The slider widget.
 * @param sd The private data of the slider.
 * @param eo_event The Efl_Event for the input.
 * @param src The source Evas_Object of the event.
 * @return EINA_TRUE if the event was handled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_slider_efl_ui_widget_widget_input_event_handler(Eo *obj, Efl_Ui_Slider_Data *sd, const Efl_Event *eo_event, Evas_Object *src)
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

   _drag_value_fetch(obj);

   return EINA_TRUE;
}

/**
 * @brief Handles accessibility activation events for the slider.
 *
 * This function allows assistive technologies to control the slider.
 * It maps activation types (e.g., EFL_UI_ACTIVATE_UP, EFL_UI_ACTIVATE_LEFT)
 * to corresponding drag actions (_drag_up, _drag_down), considering the
 * slider's orientation and inversion.
 *
 * @param obj The slider widget.
 * @param sd The private data of the slider.
 * @param act The type of activation action (Efl_Ui_Activate).
 * @return EINA_TRUE if the activation was handled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_slider_efl_ui_widget_on_access_activate(Eo *obj, Efl_Ui_Slider_Data *sd, Efl_Ui_Activate act)
{
   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
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

   _drag_value_fetch(obj);

   return EINA_TRUE;
}

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
 * @brief Constructs the appropriate Edje theme group string for the slider
 * based on its orientation.
 *
 * It takes the base theme group (e.g., "slider") and appends "/horizontal"
 * or "/vertical" depending on the slider's orientation (sd->dir).
 * If the current theme group already ends with "horizontal" or "vertical",
 * it replaces that part.
 *
 * @param obj The slider widget.
 * @param sd The private data of the slider widget.
 * @return A newly allocated string containing the full theme group (e.g.,
 *         "elm/slider/horizontal/default"). The caller is responsible for
 *         freeing this string.
 */
static char *
_efl_ui_slider_theme_group_get(Evas_Object *obj, Efl_Ui_Slider_Data *sd)
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

EOLIAN static Eina_Error
_efl_ui_slider_efl_ui_widget_theme_apply(Eo *obj, Efl_Ui_Slider_Data *sd)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EFL_UI_THEME_APPLY_ERROR_GENERIC);
   char *group;

   group = _efl_ui_slider_theme_group_get(obj, sd);
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

   _drag_value_update(obj);

   efl_layout_signal_process(wd->resize_obj, EINA_FALSE);
   evas_object_smart_changed(obj);

   return int_ret;
}

static void
_spacer_down_cb(void *data,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info)
{
   // This callback handles mouse down events on the slider's track (spacer).
   // It initiates a drag operation or directly sets the slider value if clicked.
   EFL_UI_SLIDER_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(data, wd);

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

   efl_ui_drag_value_set(efl_part(wd->resize_obj, "efl.draggable.slider"),
                           button_x, button_y);

   if (!efl_ui_focus_object_focus_get(data))
     elm_object_focus_set(data, EINA_TRUE);
   efl_event_callback_call(data, EFL_UI_SLIDER_EVENT_SLIDER_DRAG_START, NULL);
   _drag_value_fetch(data);
}

static void
_spacer_move_cb(void *data,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info)
{
   // This callback handles mouse move events over the slider's track (spacer)
   // while the mouse button is pressed. It updates the slider value during the drag.
   EFL_UI_SLIDER_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(data, wd);

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
                  elm_widget_scroll_freeze_push(data);
                  sd->frozen = EINA_TRUE;
               }
             ev->event_flags &= ~EVAS_EVENT_FLAG_ON_HOLD;
          }

        if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
          {
             if (sd->spacer_down) sd->spacer_down = EINA_FALSE;
             _drag_value_fetch(data);
             efl_event_callback_call
               (data, EFL_UI_SLIDER_EVENT_SLIDER_DRAG_STOP, NULL);
             if (sd->frozen)
               {
                  elm_widget_scroll_freeze_pop(data);
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

        efl_ui_drag_value_set(efl_part(wd->resize_obj, "efl.draggable.slider"),
                              button_x, button_y);
        _drag_value_fetch(data);
     }
}

static void
_spacer_up_cb(void *data,
              Evas *e EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED,
              void *event_info EINA_UNUSED)
{
   // This callback handles mouse up events over the slider's track (spacer).
   // It finalizes the drag operation, adjusts the value to the nearest step,
   // and emits drag stop events.
   EFL_UI_SLIDER_DATA_GET(data, sd);

   if (!sd->spacer_down) return;
   if (sd->spacer_down) sd->spacer_down = EINA_FALSE;

   _drag_value_fetch(data);
   _adjust_to_step(data, sd);
   efl_event_callback_call(data, EFL_UI_SLIDER_EVENT_SLIDER_DRAG_STOP, NULL);

   if (sd->frozen)
     {
        elm_widget_scroll_freeze_pop(data);
        sd->frozen = EINA_FALSE;
     }
}

static void
_mouse_in_cb(void *data EINA_UNUSED,
              Evas *e EINA_UNUSED,
              Evas_Object *obj,
              void *event_info EINA_UNUSED)
{
   // Called when the mouse pointer enters the slider widget area.
   // Pushes a scroll hold to prevent parent scrolling during interaction.
   efl_ui_widget_scroll_hold_push(obj);
}

/**
 * @brief Callback for mouse out events on the slider widget.
 *
 * @param data Unused.
 * @param e Unused.
 * @param obj The slider widget.
 * @param event_info Unused.
 */
static void
_mouse_out_cb(void *data EINA_UNUSED,
              Evas *e EINA_UNUSED,
              Evas_Object *obj,
              void *event_info EINA_UNUSED)
{
   // Called when the mouse pointer leaves the slider widget area.
   // Pops the scroll hold.
   efl_ui_widget_scroll_hold_pop(obj);
}

/**
 * @brief Provides a textual description of the slider for accessibility.
 *
 * @param data Unused.
 * @param obj The slider widget.
 * @return A newly allocated string with the access info, or NULL.
 *         The caller is responsible for freeing the returned string.
 */
static char *
_access_info_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   const char *txt = elm_widget_access_info_get(obj);

   if (!txt) txt = elm_layout_text_get(obj, NULL);
   if (txt) return strdup(txt);

   return NULL;
}

/**
 * @brief Provides a textual description of the slider's current state for accessibility.
 *
 * @param data Unused.
 * @param obj The slider widget.
 * @return A newly allocated string with the state information (e.g., " state: disabled"),
 *         or NULL if no specific state is noteworthy. The caller is responsible for
 *         freeing the returned string.
 */
static char *
_access_state_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   char *ret;
   Eina_Strbuf *buf = eina_strbuf_new();

   if (elm_widget_disabled_get(obj))
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

EOLIAN static Eo *
_efl_ui_slider_efl_object_constructor(Eo *obj, Efl_Ui_Slider_Data *priv)
{
   // This is the Eolian constructor for the Efl.Ui.Slider object.
   // It initializes the slider, sets up its theme, default values,
   // registers event callbacks for drag operations and mouse interactions
   // on the spacer (track), and configures accessibility features.
   char *group;
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_SLIDER);

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "slider");

   group = _efl_ui_slider_theme_group_get(obj, priv);
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

EOLIAN static void
_efl_ui_slider_efl_object_destructor(Eo *obj,
                                      Efl_Ui_Slider_Data *sd)
{
   // Eolian destructor. Cleans up resources, primarily the ecore_timer for delay_change.
   ecore_timer_del(sd->delay);

   efl_destructor(efl_super(obj, MY_CLASS));
}

EOLIAN static void
_efl_ui_slider_efl_ui_layout_orientable_orientation_set(Eo *obj, Efl_Ui_Slider_Data *sd, Efl_Ui_Layout_Orientation dir)
{
   // Sets the orientation (horizontal/vertical) of the slider and reapplies the theme
   // to reflect the change.
   sd->dir = dir;

   efl_ui_widget_theme_apply(obj);
}

EOLIAN static Efl_Ui_Layout_Orientation
_efl_ui_slider_efl_ui_layout_orientable_orientation_get(const Eo *obj EINA_UNUSED, Efl_Ui_Slider_Data *sd)
{
   // Gets the current orientation of the slider.
   return sd->dir;
}

EOLIAN static void
_efl_ui_slider_efl_ui_range_display_range_limits_set(Eo *obj, Efl_Ui_Slider_Data *sd, double min, double max)
{
   // Sets the minimum and maximum values for the slider.
   // If the current value is outside the new limits, it's clamped to the new range.
   double val;

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

   val = sd->val;
   if (val < sd->val_min) val = sd->val_min;
   if (val > sd->val_max) val = sd->val_max;

   efl_ui_range_value_set(obj, val);
}

EOLIAN static void
_efl_ui_slider_efl_ui_range_display_range_limits_get(const Eo *obj EINA_UNUSED, Efl_Ui_Slider_Data *sd, double *min, double *max)
{
   // Gets the minimum and maximum values of the slider.
   if (min) *min = sd->val_min;
   if (max) *max = sd->val_max;
}

EOLIAN static void
_efl_ui_slider_efl_ui_range_display_range_value_set(Eo *obj, Efl_Ui_Slider_Data *sd, double val)
{
   // Sets the current value of the slider.
   // The value is clamped to the min/max range.
   // Updates the visual representation (_drag_value_update) and emits change events.
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

   _drag_value_update(obj);
   _emit_events(obj, sd);
}

EOLIAN static double
_efl_ui_slider_efl_ui_range_display_range_value_get(const Eo *obj EINA_UNUSED, Efl_Ui_Slider_Data *sd)
{
   // Gets the current value of the slider.
   return sd->val;
}

EOLIAN static double
_efl_ui_slider_efl_ui_range_interactive_range_step_get(const Eo *obj EINA_UNUSED, Efl_Ui_Slider_Data *sd)
{
   // Gets the step increment for the slider.
   return sd->step;
}

EOLIAN static void
_efl_ui_slider_efl_ui_range_interactive_range_step_set(Eo *obj EINA_UNUSED, Efl_Ui_Slider_Data *sd, double step)
{
   // Sets the step increment for the slider.
   // The step value must be greater than 0.
   if (step <= 0)
     {
        ERR("Wrong param. The step(%lf) should be greater than 0.0", step);
        return;
     }

   if (EINA_DBL_EQ(sd->step, step)) return;

   sd->step = step;
}

EOLIAN static Eina_Bool
_efl_ui_slider_efl_ui_focus_object_on_focus_update(Eo *obj, Efl_Ui_Slider_Data *sd EINA_UNUSED)
{
   Eina_Bool int_ret = EINA_FALSE;

   int_ret = efl_ui_focus_object_on_focus_update(efl_super(obj, MY_CLASS));

   return int_ret;
}

// A11Y Accessibility

/**
 * @brief Provides the list of Elementary accessibility actions supported by the slider.
 *
 * This function returns a static array of Efl_Access_Action_Data structures.
 * Each structure defines an accessibility action:
 * - const char *name: A unique name for the action (e.g., "drag,left").
 * - const char *action: The general action type (e.g., "drag"). This typically
 *                       matches an action name in the `key_actions` array.
 * - const char *param: A parameter string passed to the action callback (e.g., "left").
 * - Elm_Access_Action_Cb callback: The function to call when this action is activated.
 *                                  This is usually the corresponding key action handler.
 *
 * @param obj The slider widget (unused).
 * @param pd The private data of the slider (unused).
 * @return A pointer to the array of Efl_Access_Action_Data.
 */
EOLIAN const Efl_Access_Action_Data *
_efl_ui_slider_efl_access_widget_action_elm_actions_get(const Eo *obj EINA_UNUSED, Efl_Ui_Slider_Data *pd EINA_UNUSED)
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

#include "efl_ui_slider.eo.c"
