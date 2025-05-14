#ifdef HAVE_CONFIG_H
#include "elementary_config.h"
#endif
#define EFL_UI_SCROLL_MANAGER_PROTECTED
#define EFL_UI_SCROLLBAR_PROTECTED

#include <Elementary.h>
#include <Efl_Ui.h>
#include "elm_priv.h"

/**
 * @internal
 * @brief Structure to hold the context for a scroll connector.
 *
 * This structure maintains the state and associations between a scrollable
 * object (typically a widget with an Edje theme) and its scroll manager.
 * It tracks visibility of scroll direction indicators, freeze states,
 * and other scroll-related properties.
 */
typedef struct {
   Eo *obj; /**< The Evas object (widget) this connector is associated with. */
   Eo *smanager; /**< The scroll manager (e.g., Efl.Ui.Scroll_Manager) controlling the scrollable content. */
   int freeze_want; /**< Stores the desired scroll freeze state before a drag operation. -1 if not set. */
   Eina_Bool scroll_count : 1; /**< Flag to manage scroll signal emission, preventing redundant signals. */
   Eina_Bool need_scroll : 1; /**< Flag indicating if a scroll signal needs to be emitted. */
   Eina_Bool show_up : 1; /**< Current visibility state of the 'up' direction indicator. */
   Eina_Bool show_down : 1; /**< Current visibility state of the 'down' direction indicator. */
   Eina_Bool show_left: 1; /**< Current visibility state of the 'left' direction indicator. */
   Eina_Bool show_right : 1; /**< Current visibility state of the 'right' direction indicator. */
} Scroll_Connector_Context;

/**
 * @internal
 * @brief Updates the visibility of scroll direction indicators (arrows/buttons)
 *        based on the current scrollbar positions and visibility.
 *
 * This function checks if the content can be scrolled further in each direction
 * and emits Edje signals to show or hide the corresponding direction indicators
 * in the theme.
 *
 * @param ctx The scroll connector context.
 */
static void
_scroll_connector_bar_direction_show_update(Scroll_Connector_Context *ctx)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(ctx->obj, wd);
   Eina_Bool hbar_visible = EINA_FALSE, vbar_visible = EINA_FALSE;
   Eina_Bool show_up = EINA_FALSE, show_down = EINA_FALSE, show_left = EINA_FALSE, show_right = EINA_FALSE;
   double vx = 0.0, vy = 0.0;

   edje_object_part_drag_value_get
     (wd->resize_obj, "efl.draggable.vertical_bar", NULL, &vy);

   edje_object_part_drag_value_get
     (wd->resize_obj, "efl.draggable.horizontal_bar", &vx, NULL);

   efl_ui_scrollbar_bar_visibility_get(ctx->smanager, &hbar_visible, &vbar_visible);
   if (hbar_visible)
     {
        if (vx < 1.0) show_right = EINA_TRUE;
        if (vx > 0.0) show_left = EINA_TRUE;
     }
   if (vbar_visible)
     {
        if (vy < 1.0) show_down = EINA_TRUE;
        if (vy > 0.0) show_up = EINA_TRUE;
     }
   if (show_right != ctx->show_right)
     {
        if (show_right)
          efl_layout_signal_emit(wd->resize_obj, "efl,action,show,right", "efl");
        else
          efl_layout_signal_emit(wd->resize_obj, "efl,action,hide,right", "efl");
        ctx->show_right = show_right;
     }
   if (show_left != ctx->show_left)
     {
        if (show_left)
          efl_layout_signal_emit(wd->resize_obj, "efl,action,show,left", "efl");
        else
          efl_layout_signal_emit(wd->resize_obj, "efl,action,hide,left", "efl");
        ctx->show_left = show_left;
     }
   if (show_up != ctx->show_up)
     {
        if (show_up)
          efl_layout_signal_emit(wd->resize_obj, "efl,action,show,up", "efl");
        else
          efl_layout_signal_emit(wd->resize_obj, "efl,action,hide,up", "efl");
        ctx->show_up = show_up;
     }
   if (show_down != ctx->show_down)
     {
        if (show_down)
          efl_layout_signal_emit(wd->resize_obj, "efl,action,show,down", "efl");
        else
          efl_layout_signal_emit(wd->resize_obj, "efl,action,hide,down", "efl");
        ctx->show_down = show_down;
     }
}

/**
 * @internal
 * @brief Reads the current drag values from the Edje theme's draggable parts
 *        (scrollbars) and updates the scroll manager's bar position.
 *
 * After updating the scroll manager, it also calls
 * _scroll_connector_bar_direction_show_update() to refresh the
 * direction indicators.
 *
 * @param ctx The scroll connector context.
 */
static void
_scroll_connector_bar_read_and_update(Scroll_Connector_Context *ctx)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(ctx->obj, wd);
   double vx = 0.0, vy = 0.0;

   edje_object_part_drag_value_get
     (wd->resize_obj, "efl.draggable.vertical_bar", NULL, &vy);

   edje_object_part_drag_value_get
     (wd->resize_obj, "efl.draggable.horizontal_bar", &vx, NULL);

   efl_ui_scrollbar_bar_position_set(ctx->smanager, vx, vy);
   _scroll_connector_bar_direction_show_update(ctx);
}

/**
 * @internal
 * @brief Callback triggered when the Edje theme emits a "reload" signal.
 *
 * This function refreshes the visibility state of the horizontal and vertical
 * scrollbars in the theme based on the scroll manager's state. It also
 * updates the scrollbar visibility in the manager and refreshes direction
 * indicators.
 *
 * @param data The scroll connector context.
 * @param obj The Evas object that emitted the signal (unused).
 * @param emission The Edje signal emitted (unused).
 * @param source The source of the Edje signal (unused).
 */
static void
_scroll_connector_reload_cb(void *data,
                       Evas_Object *obj EINA_UNUSED,
                       const char *emission EINA_UNUSED,
                       const char *source EINA_UNUSED)
{
   Scroll_Connector_Context *ctx = data;
   ELM_WIDGET_DATA_GET_OR_RETURN(ctx->obj, wd);
   Eina_Bool hbar_visible = EINA_FALSE, vbar_visible = EINA_FALSE;

   efl_ui_scrollbar_bar_visibility_get(ctx->smanager, &hbar_visible, &vbar_visible);

   if (hbar_visible)
     efl_layout_signal_emit(wd->resize_obj, "efl,horizontal_bar,visible,on", "efl");
   else
     efl_layout_signal_emit(wd->resize_obj, "efl,horizontal_bar,visible,off", "efl");

   if (vbar_visible)
     efl_layout_signal_emit(wd->resize_obj, "efl,vertical_bar,visible,on", "efl");
   else
     efl_layout_signal_emit(wd->resize_obj, "efl,vertical_bar,visible,off", "efl");

   efl_ui_scrollbar_bar_visibility_update(ctx->smanager);
   _scroll_connector_bar_direction_show_update(ctx);
}

/**
 * @internal
 * @brief Callback for Edje drag events (drag,set; drag,step; drag,page) on scrollbars.
 *
 * This function is called when the user interacts with the scrollbar parts in
 * the Edje theme (e.g., dragging the thumb, clicking step arrows). It reads
 * the new scrollbar position and updates the scroll manager.
 *
 * @param data The scroll connector context.
 * @param obj The Evas object that emitted the signal (unused).
 * @param emission The Edje signal emitted (unused).
 * @param source The source of the Edje signal (unused).
 */
static void
_scroll_connector_edje_drag_cb(void *data,
                          Evas_Object *obj EINA_UNUSED,
                          const char *emission EINA_UNUSED,
                          const char *source EINA_UNUSED)
{
   Scroll_Connector_Context *ctx = data;

   _scroll_connector_bar_read_and_update(ctx);
}

/**
 * @internal
 * @brief Callback for the "efl,action,scroll" signal from the Edje theme.
 *
 * This function is typically triggered after a scrollbar position change.
 * It manages the emission of the "efl,action,scroll" signal to the theme,
 * ensuring it's not emitted excessively.
 *
 * @param data The scroll connector context.
 * @param obj The Evas object that emitted the signal.
 * @param emission The Edje signal emitted (unused).
 * @param source The source of the Edje signal (unused).
 */
static void
_scroll(void *data,
                          Evas_Object *obj,
                          const char *emission EINA_UNUSED,
                          const char *source EINA_UNUSED)
{
   Scroll_Connector_Context *ctx = data;

   ctx->scroll_count = EINA_FALSE;
   if (!ctx->need_scroll) return;
   ctx->need_scroll = EINA_FALSE;
   efl_layout_signal_emit(obj, "efl,action,scroll", "efl");
}

/**
 * @internal
 * @brief Callback for Edje "drag,start" signals on scrollbars.
 *
 * This function is called when the user starts dragging a scrollbar.
 * It reads and updates the scrollbar position, freezes scrolling on the
 * scroll manager to prevent conflicting updates, and emits the
 * EFL_UI_EVENT_SCROLL_DRAG_STARTED event.
 *
 * @param data The scroll connector context.
 * @param obj The Evas object that emitted the signal (unused).
 * @param emission The Edje signal emitted (unused).
 * @param source The source of the Edje signal (unused).
 */
static void
_scroll_connector_edje_drag_start_cb(void *data,
                                Evas_Object *obj EINA_UNUSED,
                                const char *emission EINA_UNUSED,
                                const char *source EINA_UNUSED)
{
   Scroll_Connector_Context *ctx = data;

   _scroll_connector_bar_read_and_update(ctx);

   ctx->freeze_want = efl_ui_scrollable_scroll_freeze_get(ctx->smanager);
   efl_ui_scrollable_scroll_freeze_set(ctx->smanager, EINA_TRUE);
   efl_event_callback_call(ctx->obj, EFL_UI_EVENT_SCROLL_DRAG_STARTED, NULL);
}

/**
 * @internal
 * @brief Callback for Edje "drag,stop" signals on scrollbars.
 *
 * This function is called when the user stops dragging a scrollbar.
 * It reads and updates the final scrollbar position, unfreezes scrolling
 * on the scroll manager (restoring its previous freeze state), and emits
 * the EFL_UI_EVENT_SCROLL_DRAG_FINISHED event.
 *
 * @param data The scroll connector context.
 * @param obj The Evas object that emitted the signal (unused).
 * @param emission The Edje signal emitted (unused).
 * @param source The source of the Edje signal (unused).
 */
static void
_scroll_connector_edje_drag_stop_cb(void *data,
                               Evas_Object *obj EINA_UNUSED,
                               const char *emission EINA_UNUSED,
                               const char *source EINA_UNUSED)
{
   Scroll_Connector_Context *ctx = data;

   _scroll_connector_bar_read_and_update(ctx);
   EINA_SAFETY_ON_TRUE_RETURN(ctx->freeze_want == -1);
   efl_ui_scrollable_scroll_freeze_set(ctx->smanager, ctx->freeze_want);
   ctx->freeze_want = -1;
   efl_event_callback_call(ctx->obj, EFL_UI_EVENT_SCROLL_DRAG_FINISHED, NULL);
}

/**
 * @internal
 * @brief Callback for Edje "drag" signals specifically from the vertical scrollbar.
 *
 * This function reads and updates the scrollbar position and then emits the
 * EFL_UI_SCROLLBAR_EVENT_BAR_DRAGGED event with orientation set to vertical.
 *
 * @param data The scroll connector context.
 * @param obj The Evas object that emitted the signal (unused).
 * @param emission The Edje signal emitted (unused).
 * @param source The source of the Edje signal (unused).
 */
static void
_scroll_connector_vbar_drag_cb(void *data,
                          Evas_Object *obj EINA_UNUSED,
                          const char *emission EINA_UNUSED,
                          const char *source EINA_UNUSED)
{
   Scroll_Connector_Context *ctx = data;
   Efl_Ui_Layout_Orientation type;

   _scroll_connector_bar_read_and_update(ctx);

   type = EFL_UI_LAYOUT_ORIENTATION_VERTICAL;
   efl_event_callback_call(ctx->obj, EFL_UI_SCROLLBAR_EVENT_BAR_DRAGGED, &type);
}

/**
 * @internal
 * @brief Callback for Edje "efl,vertical_bar,press" signals.
 *
 * This function is called when the vertical scrollbar is pressed. It emits
 * the EFL_UI_SCROLLBAR_EVENT_BAR_PRESSED event with orientation set to vertical.
 *
 * @param data The scroll connector context.
 * @param obj The Evas object that emitted the signal (unused).
 * @param emission The Edje signal emitted (unused).
 * @param source The source of the Edje signal (unused).
 */
static void
_scroll_connector_vbar_press_cb(void *data,
                           Evas_Object *obj EINA_UNUSED,
                           const char *emission EINA_UNUSED,
                           const char *source EINA_UNUSED)
{
   Scroll_Connector_Context *ctx = data;
   Efl_Ui_Layout_Orientation type = EFL_UI_LAYOUT_ORIENTATION_VERTICAL;

   efl_event_callback_call(ctx->obj, EFL_UI_SCROLLBAR_EVENT_BAR_PRESSED, &type);
}

/**
 * @internal
 * @brief Callback for Edje "efl,vbar,unpress" signals.
 *
 * This function is called when the vertical scrollbar is unpressed (released).
 * It emits the EFL_UI_SCROLLBAR_EVENT_BAR_UNPRESSED event with orientation
 * set to vertical.
 *
 * @param data The scroll connector context.
 * @param obj The Evas object that emitted the signal (unused).
 * @param emission The Edje signal emitted (unused).
 * @param source The source of the Edje signal (unused).
 */
static void
_scroll_connector_vbar_unpress_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             const char *emission EINA_UNUSED,
                             const char *source EINA_UNUSED)
{
   Scroll_Connector_Context *ctx = data;
   Efl_Ui_Layout_Orientation type = EFL_UI_LAYOUT_ORIENTATION_VERTICAL;

   efl_event_callback_call(ctx->obj, EFL_UI_SCROLLBAR_EVENT_BAR_UNPRESSED, &type);
}

/**
 * @internal
 * @brief Callback for Edje "drag" signals specifically from the horizontal scrollbar.
 *
 * This function reads and updates the scrollbar position and then emits the
 * EFL_UI_SCROLLBAR_EVENT_BAR_DRAGGED event with orientation set to horizontal.
 *
 * @param data The scroll connector context.
 * @param obj The Evas object that emitted the signal (unused).
 * @param emission The Edje signal emitted (unused).
 * @param source The source of the Edje signal (unused).
 */
static void
_scroll_connector_hbar_drag_cb(void *data,
                          Evas_Object *obj EINA_UNUSED,
                          const char *emission EINA_UNUSED,
                          const char *source EINA_UNUSED)
{
   Scroll_Connector_Context *ctx = data;
   Efl_Ui_Layout_Orientation type = EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL;

   _scroll_connector_bar_read_and_update(ctx);
   efl_event_callback_call(ctx->obj, EFL_UI_SCROLLBAR_EVENT_BAR_DRAGGED, &type);
}

/**
 * @internal
 * @brief Callback for Edje "efl,horizontal_bar,press" signals.
 *
 * This function is called when the horizontal scrollbar is pressed. It emits
 * the EFL_UI_SCROLLBAR_EVENT_BAR_PRESSED event with orientation set to horizontal.
 *
 * @param data The scroll connector context.
 * @param obj The Evas object that emitted the signal (unused).
 * @param emission The Edje signal emitted (unused).
 * @param source The source of the Edje signal (unused).
 */
static void
_scroll_connector_hbar_press_cb(void *data,
                           Evas_Object *obj EINA_UNUSED,
                           const char *emission EINA_UNUSED,
                           const char *source EINA_UNUSED)
{
   Scroll_Connector_Context *ctx = data;
   Efl_Ui_Layout_Orientation type = EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL;

   efl_event_callback_call(ctx->obj, EFL_UI_SCROLLBAR_EVENT_BAR_PRESSED, &type);
}

/**
 * @internal
 * @brief Callback for Edje "efl,hbar,unpress" signals.
 *
 * This function is called when the horizontal scrollbar is unpressed (released).
 * It emits the EFL_UI_SCROLLBAR_EVENT_BAR_UNPRESSED event with orientation
 * set to horizontal.
 *
 * @param data The scroll connector context.
 * @param obj The Evas object that emitted the signal (unused).
 * @param emission The Edje signal emitted (unused).
 * @param source The source of the Edje signal (unused).
 */
static void
_scroll_connector_hbar_unpress_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             const char *emission EINA_UNUSED,
                             const char *source EINA_UNUSED)
{
   Scroll_Connector_Context *ctx = data;
   Efl_Ui_Layout_Orientation type = EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL;

   efl_event_callback_call(ctx->obj, EFL_UI_SCROLLBAR_EVENT_BAR_UNPRESSED, &type);
}

/**
 * @internal
 * @brief Callback for the EFL_UI_SCROLLBAR_EVENT_BAR_SIZE_CHANGED event from the scroll manager.
 *
 * This function is triggered when the scroll manager indicates a change in
 * scrollbar thumb sizes. It retrieves the new sizes and updates the
 * corresponding Edje draggable parts (horizontal and vertical bars) in the theme.
 *
 * @param data The scroll connector context.
 * @param event The Efl_Event data (unused).
 */
static void
_scroll_connector_bar_size_changed_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   Scroll_Connector_Context *ctx = data;
   ELM_WIDGET_DATA_GET_OR_RETURN(ctx->obj, wd);

   double width = 0.0, height = 0.0;

   edje_object_calc_force(wd->resize_obj);
   efl_ui_scrollbar_bar_size_get(ctx->smanager, &width, &height);
   edje_object_part_drag_size_set(wd->resize_obj, "efl.draggable.horizontal_bar", width, 1.0);
   edje_object_part_drag_size_set(wd->resize_obj, "efl.draggable.vertical_bar", 1.0, height);
}

/**
 * @internal
 * @brief Callback for the EFL_UI_SCROLLBAR_EVENT_BAR_POS_CHANGED event from the scroll manager.
 *
 * This function is triggered when the scroll manager indicates a change in
 * scrollbar thumb positions. It updates the step size of the Edje draggable
 * parts based on content size and step size from the scroll manager. Then,
 * it sets the new position of the draggable parts in the theme and emits
 * an "efl,action,scroll" signal to the theme.
 *
 * @param data The scroll connector context.
 * @param event The Efl_Event data (unused).
 */
static void
_scroll_connector_bar_pos_changed_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   Scroll_Connector_Context *ctx = data;
   ELM_WIDGET_DATA_GET_OR_RETURN(ctx->obj, wd);

   double posx = 0.0, posy = 0.0;
   Eina_Size2D cs;
   Eina_Position2D step;

   step = efl_ui_scrollable_step_size_get(ctx->smanager);
   cs = efl_ui_scrollable_content_size_get(ctx->smanager);

   edje_object_part_drag_step_set(wd->resize_obj, "efl.draggable.horizontal_bar",
                                  (double)step.x / cs.w, 0.0);
   edje_object_part_drag_step_set(wd->resize_obj, "efl.draggable.vertical_bar",
                                  0.0, (double)step.y / cs.h);

   efl_ui_scrollbar_bar_position_get(ctx->smanager, &posx, &posy);
   edje_object_part_drag_value_set(wd->resize_obj, "efl.draggable.horizontal_bar", posx, 0.0);
   edje_object_part_drag_value_set(wd->resize_obj, "efl.draggable.vertical_bar", 0.0, posy);
   if (ctx->scroll_count)
     ctx->need_scroll = EINA_TRUE;
   else
     {
        efl_layout_signal_emit(wd->resize_obj, "efl,action,scroll", "efl");
        ctx->scroll_count = EINA_TRUE;
     }
}

/**
 * @internal
 * @brief Callback for the EFL_UI_SCROLLBAR_EVENT_BAR_SHOW event from the scroll manager.
 *
 * This function is triggered when the scroll manager indicates that a scrollbar
 * (horizontal or vertical, specified in event->info) should be shown.
 * It emits the appropriate Edje signal to make the scrollbar visible in the theme
 * and updates the direction indicators.
 *
 * @param data The scroll connector context.
 * @param event The Efl_Event data, where event->info is an Efl_Ui_Layout_Orientation*
 *              indicating which bar to show.
 */
static void
_scroll_connector_bar_show_cb(void *data, const Efl_Event *event)
{
   Scroll_Connector_Context *ctx = data;
   ELM_WIDGET_DATA_GET_OR_RETURN(ctx->obj, wd);
   Efl_Ui_Layout_Orientation type = *(Efl_Ui_Layout_Orientation *)(event->info);

   if (type == EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL)
     efl_layout_signal_emit(wd->resize_obj, "efl,horizontal_bar,visible,on", "efl");
   else if (type == EFL_UI_LAYOUT_ORIENTATION_VERTICAL)
     efl_layout_signal_emit(wd->resize_obj, "efl,vertical_bar,visible,on", "efl");
   _scroll_connector_bar_direction_show_update(ctx);
}

/**
 * @internal
 * @brief Callback for the EFL_UI_SCROLLBAR_EVENT_BAR_HIDE event from the scroll manager.
 *
 * This function is triggered when the scroll manager indicates that a scrollbar
 * (horizontal or vertical, specified in event->info) should be hidden.
 * It emits the appropriate Edje signal to make the scrollbar invisible in the theme
 * and updates the direction indicators.
 *
 * @param data The scroll connector context.
 * @param event The Efl_Event data, where event->info is an Efl_Ui_Layout_Orientation*
 *              indicating which bar to hide.
 */
static void
_scroll_connector_bar_hide_cb(void *data, const Efl_Event *event)
{
   Scroll_Connector_Context *ctx = data;
   ELM_WIDGET_DATA_GET_OR_RETURN(ctx->obj, wd);
   Efl_Ui_Layout_Orientation type = *(Efl_Ui_Layout_Orientation *)(event->info);

   if (type == EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL)
     efl_layout_signal_emit(wd->resize_obj, "efl,horizontal_bar,visible,off", "efl");
   else if (type == EFL_UI_LAYOUT_ORIENTATION_VERTICAL)
     efl_layout_signal_emit(wd->resize_obj, "efl,vertical_bar,visible,off", "efl");
   _scroll_connector_bar_direction_show_update(ctx);
}

/**
 * @brief Binds a scrollable Evas object (widget) to a scroll manager.
 *
 * This function sets up the necessary callbacks and connections to synchronize
 * the scroll state between an Evas object (typically one that uses an Edje
 * theme for its scrollbars) and an Efl_Ui_Scroll_Manager. It creates and
 * stores a Scroll_Connector_Context to manage this binding.
 *
 * Callbacks are established for:
 * - Edje signals from the theme (e.g., drag, press on scrollbar parts) to update the scroll manager.
 * - Efl events from the scroll manager (e.g., bar size/position changes, show/hide requests)
 *   to update the Edje theme.
 *
 * @param obj The Evas object (widget) to bind. This object is expected to
 *            have an Edje theme with parts like "efl.draggable.vertical_bar"
 *            and "efl.draggable.horizontal_bar".
 * @param manager The Efl_Ui_Scroll_Manager (or compatible) object that
 *                controls the scrolling logic and state.
 */
void
efl_ui_scroll_connector_bind(Eo *obj, Eo *manager)
{
   Scroll_Connector_Context *ctx = calloc(1, sizeof(Scroll_Connector_Context));
   if (!ctx) return;
   ctx->obj = obj;
   ctx->smanager = manager;
   efl_key_data_set(obj, "__context", ctx);

   //from the theme to the object
   efl_layout_signal_callback_add(obj, "reload", "efl",
                                  ctx, _scroll_connector_reload_cb, NULL);
   efl_layout_signal_callback_add(obj, "drag", "efl.draggable.vertical_bar",
                                  ctx, _scroll_connector_vbar_drag_cb, NULL);
   efl_layout_signal_callback_add(obj, "drag,set", "efl.draggable.vertical_bar",
                                  ctx, _scroll_connector_edje_drag_cb, NULL);
   efl_layout_signal_callback_add(obj, "drag,start", "efl.draggable.vertical_bar",
                                  ctx, _scroll_connector_edje_drag_start_cb, NULL);
   efl_layout_signal_callback_add(obj, "drag,stop", "efl.draggable.vertical_bar",
                                  ctx, _scroll_connector_edje_drag_stop_cb, NULL);
   efl_layout_signal_callback_add(obj, "drag,step", "efl.draggable.vertical_bar",
                                  ctx, _scroll_connector_edje_drag_cb, NULL);
   efl_layout_signal_callback_add(obj, "drag,page", "efl.draggable.vertical_bar",
                                  ctx, _scroll_connector_edje_drag_cb, NULL);
   efl_layout_signal_callback_add(obj, "efl,vertical_bar,press", "efl",
                                  ctx, _scroll_connector_vbar_press_cb, NULL);
   efl_layout_signal_callback_add(obj, "efl,vbar,unpress", "efl",
                                  ctx, _scroll_connector_vbar_unpress_cb, NULL);
   efl_layout_signal_callback_add(obj, "drag", "efl.draggable.horizontal_bar",
                                  ctx, _scroll_connector_hbar_drag_cb, NULL);
   efl_layout_signal_callback_add(obj, "drag,set", "efl.draggable.horizontal_bar",
                                  ctx, _scroll_connector_edje_drag_cb, NULL);
   efl_layout_signal_callback_add(obj, "drag,start", "efl.draggable.horizontal_bar",
                                  ctx, _scroll_connector_edje_drag_start_cb, NULL);
   efl_layout_signal_callback_add(obj, "drag,stop", "efl.draggable.horizontal_bar",
                                  ctx, _scroll_connector_edje_drag_stop_cb, NULL);
   efl_layout_signal_callback_add(obj, "drag,step", "efl.draggable.horizontal_bar",
                                  ctx, _scroll_connector_edje_drag_cb, NULL);
   efl_layout_signal_callback_add(obj, "drag,page", "efl.draggable.horizontal_bar",
                                  ctx, _scroll_connector_edje_drag_cb, NULL);
   efl_layout_signal_callback_add(obj, "efl,horizontal_bar,press", "efl",
                                  ctx, _scroll_connector_hbar_press_cb, NULL);
   efl_layout_signal_callback_add(obj, "efl,hbar,unpress", "efl",
                                  ctx, _scroll_connector_hbar_unpress_cb, NULL);
   efl_layout_signal_callback_add(obj, "efl,action,scroll", "efl",
                                  ctx, _scroll, NULL);

   //from the object to the theme
   efl_event_callback_add(obj, EFL_UI_SCROLLBAR_EVENT_BAR_SIZE_CHANGED,
                          _scroll_connector_bar_size_changed_cb, ctx);
   efl_event_callback_add(obj, EFL_UI_SCROLLBAR_EVENT_BAR_POS_CHANGED,
                          _scroll_connector_bar_pos_changed_cb, ctx);
   efl_event_callback_add(obj, EFL_UI_SCROLLBAR_EVENT_BAR_SHOW,
                          _scroll_connector_bar_show_cb, ctx);
   efl_event_callback_add(obj, EFL_UI_SCROLLBAR_EVENT_BAR_HIDE,
                          _scroll_connector_bar_hide_cb, ctx);
}

/**
 * @brief Unbinds a scrollable Evas object from its scroll manager.
 *
 * This function removes all callbacks and connections previously established
 * by efl_ui_scroll_connector_bind() and frees the associated
 * Scroll_Connector_Context.
 *
 * @param obj The Evas object (widget) to unbind.
 */
void
efl_ui_scroll_connector_unbind(Eo *obj)
{
   Scroll_Connector_Context *ctx;

   ctx = efl_key_data_get(obj, "__context");
   EINA_SAFETY_ON_NULL_RETURN(ctx);

   efl_layout_signal_callback_del(obj, "reload", "efl",
                                  ctx, _scroll_connector_reload_cb, NULL);
   efl_layout_signal_callback_del(obj, "drag", "efl.draggable.vertical_bar",
                                  ctx, _scroll_connector_vbar_drag_cb, NULL);
   efl_layout_signal_callback_del(obj, "drag,set", "efl.draggable.vertical_bar",
                                  ctx, _scroll_connector_edje_drag_cb, NULL);
   efl_layout_signal_callback_del(obj, "drag,start", "efl.draggable.vertical_bar",
                                  ctx, _scroll_connector_edje_drag_start_cb, NULL);
   efl_layout_signal_callback_del(obj, "drag,stop", "efl.draggable.vertical_bar",
                                  ctx, _scroll_connector_edje_drag_stop_cb, NULL);
   efl_layout_signal_callback_del(obj, "drag,step", "efl.draggable.vertical_bar",
                                  ctx, _scroll_connector_edje_drag_cb, NULL);
   efl_layout_signal_callback_del(obj, "drag,page", "efl.draggable.vertical_bar",
                                  ctx, _scroll_connector_edje_drag_cb, NULL);
   efl_layout_signal_callback_del(obj, "efl,vertical_bar,press", "efl",
                                  ctx, _scroll_connector_vbar_press_cb, NULL);
   efl_layout_signal_callback_del(obj, "efl,vbar,unpress", "efl",
                                  ctx, _scroll_connector_vbar_unpress_cb, NULL);

   efl_layout_signal_callback_del(obj, "drag", "efl.draggable.horizontal_bar",
                                  ctx, _scroll_connector_hbar_drag_cb, NULL);
   efl_layout_signal_callback_del(obj, "drag,set", "efl.draggable.horizontal_bar",
                                  ctx, _scroll_connector_edje_drag_cb, NULL);
   efl_layout_signal_callback_del(obj, "drag,start", "efl.draggable.horizontal_bar",
                                  ctx, _scroll_connector_edje_drag_start_cb, NULL);
   efl_layout_signal_callback_del(obj, "drag,stop", "efl.draggable.horizontal_bar",
                                  ctx, _scroll_connector_edje_drag_stop_cb, NULL);
   efl_layout_signal_callback_del(obj, "drag,step", "efl.draggable.horizontal_bar",
                                  ctx, _scroll_connector_edje_drag_cb, NULL);
   efl_layout_signal_callback_del(obj, "drag,page", "efl.draggable.horizontal_bar",
                                  ctx, _scroll_connector_edje_drag_cb, NULL);
   efl_layout_signal_callback_del(obj, "efl,horizontal_bar,press", "efl",
                                  ctx, _scroll_connector_hbar_press_cb, NULL);
   efl_layout_signal_callback_del(obj, "efl,hbar,unpress", "efl",
                                  ctx, _scroll_connector_hbar_unpress_cb, NULL);
   free(ctx);
}
