#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>
#include <Efl_Ui.h>

#ifndef EFL_BETA_API_SUPPORT
#define EFL_BETA_API_SUPPORT
#endif

#ifndef EFL_EO_API_SUPPORT
#define EFL_EO_API_SUPPORT
#endif

#ifdef BUILD_VG_LOADER_JSON

/**
 * @brief Structure to hold application specific data.
 *
 * This structure is used to pass around pointers to UI elements
 * that need to be accessed by different callback functions.
 */
typedef struct _App_Data
{
   Eo *label;   /**< Pointer to the label widget displaying animation state. */
   Eo *slider;  /**< Pointer to the slider widget controlling animation progress. */
} App_Data;

/**
 * @brief Callback function for button click events.
 *
 * Handles play, pause, resume, play backwards, and stop actions for the animation.
 *
 * @param data Pointer to the Evas_Object (animation view) to be controlled.
 * @param ev The Efl_Event structure containing event information.
 */
static void
btn_clicked_cb(void *data , const Efl_Event *ev )
{
   Evas_Object *anim_view = data;
   const char *text = efl_text_get(ev->object);

   if (!text) return;

   if (!strcmp("Play", text))
     {
        double speed = efl_player_playback_speed_get(anim_view);
        efl_player_playback_speed_set(anim_view, speed < 0 ? speed * -1 : speed);
        efl_player_playing_set(anim_view, EINA_TRUE);
     }
   else if (!strcmp("Pause", text))
     efl_player_paused_set(anim_view, EINA_TRUE);
   else if (!strcmp("Resume", text))
     efl_player_paused_set(anim_view, EINA_FALSE);
   else if (!strcmp("Play Backwards", text))
     {
        double speed = efl_player_playback_speed_get(anim_view);
        efl_player_playback_speed_set(anim_view, speed > 0 ? speed * -1 : speed);
        efl_player_playing_set(anim_view, EINA_TRUE);
     }
   else if (!strcmp("Stop", text))
     efl_player_playing_set(anim_view, EINA_FALSE);
}

/**
 * @brief Callback function for check (checkbox) state changes.
 *
 * Toggles the loop mode of the animation.
 *
 * @param data Pointer to the Evas_Object (animation view) whose loop mode is to be set.
 * @param event The Efl_Event structure containing event information, specifically the checkbox state.
 */
static void
check_changed_cb(void *data, const Efl_Event *event)
{
   Evas_Object *anim_view = data;
   efl_player_playback_loop_set(anim_view, efl_ui_selectable_selected_get(event->object));
}

/**
 * @brief Callback function for speed control check (checkbox) state changes.
 *
 * Sets the playback speed of the animation (e.g., normal speed or 0.25x).
 *
 * @param data Pointer to the Evas_Object (animation view) whose speed is to be set.
 * @param event The Efl_Event structure containing event information, specifically the checkbox state.
 */
static void
speed_changed_cb(void *data, const Efl_Event *event)
{
   Evas_Object *anim_view = data;
   double speed = 1;
   if (efl_ui_selectable_selected_get(event->object)) speed = 0.25;
   efl_player_playback_speed_set(anim_view, speed);
}

/**
 * @brief Callback function for limit frame check (checkbox) state changes.
 *
 * Sets the minimum and maximum frames for the animation playback, effectively
 * limiting the animation to a specific segment.
 *
 * @param data Pointer to the Evas_Object (animation view) whose frame limits are to be set.
 * @param event The Efl_Event structure containing event information, specifically the checkbox state.
 */
static void
limit_frame_cb(void *data, const Efl_Event *event)
{
   Evas_Object *anim_view = data;
   int frame_count = efl_ui_vg_animation_frame_count_get(anim_view);
   printf("Total Frame Count : %d\n", frame_count);
   if (efl_ui_selectable_selected_get(event->object))
     {
        efl_ui_vg_animation_min_frame_set(anim_view, 5);
        efl_ui_vg_animation_max_frame_set(anim_view, 10);
        printf("Frames to show 5-10 only\n");
     }
   else
     {
        efl_ui_vg_animation_min_frame_set(anim_view, 0);
        efl_ui_vg_animation_max_frame_set(anim_view, frame_count);
        printf("Showing all frames now\n");
     }
}

/**
 * @brief Callback function for slider value changes.
 *
 * Sets the playback progress of the animation based on the slider's value.
 *
 * @param data Pointer to the Evas_Object (animation view) whose progress is to be set.
 * @param ev The Efl_Event structure containing event information, specifically the slider value.
 */
static void
_slider_changed_cb(void *data, const Efl_Event *ev)
{
   Evas_Object *anim_view = data;
   efl_player_playback_progress_set(anim_view, efl_ui_range_value_get(ev->object));
}

/**
 * @brief Updates the text of a label to reflect the current state of the animation.
 *
 * @param anim_view The animation view object whose state is to be queried.
 * @param label The label object whose text is to be updated.
 */
static void
update_anim_view_state(Evas_Object *anim_view, Evas_Object *label)
{
   Efl_Ui_Vg_Animation_State state = efl_ui_vg_animation_state_get(anim_view);

   switch (state)
     {
      case EFL_UI_VG_ANIMATION_STATE_NOT_READY:
         efl_text_set(label, "State = Not Ready");
         break;
      case EFL_UI_VG_ANIMATION_STATE_PLAYING:
         efl_text_set(label, "State = Playing");
         break;
      case EFL_UI_VG_ANIMATION_STATE_PLAYING_BACKWARDS:
         efl_text_set(label, "State = Playing Backwards");
         break;
      case EFL_UI_VG_ANIMATION_STATE_PAUSED:
         efl_text_set(label, "State = Paused");
         break;
      case EFL_UI_VG_ANIMATION_STATE_STOPPED:
         efl_text_set(label, "State = Stopped");
         break;
     }
}

/**
 * @brief Callback for EFL_PLAYER_EVENT_PLAYING_CHANGED event.
 *
 * Updates the animation state label and resets the slider if the animation stops.
 *
 * @param data Pointer to App_Data containing UI elements.
 * @param event The Efl_Event structure. The event->info is a pointer to Eina_Bool indicating playing state.
 */
static void
_animation_playing_changed_cb(void *data, const Efl_Event *event)
{
   Eina_Bool playing = *(Eina_Bool*)event->info;
   App_Data *ad = data;
   update_anim_view_state(event->object, ad->label);
   //Stopped
   if (!playing)
     efl_ui_range_value_set(ad->slider, 0);
}

/**
 * @brief Callback for EFL_PLAYER_EVENT_PAUSED_CHANGED event.
 *
 * Updates the animation state label.
 *
 * @param data Pointer to App_Data containing UI elements.
 * @param event The Efl_Event structure.
 */
static void
_animation_paused_changed_cb(void *data, const Efl_Event *event)
{
   App_Data *ad = data;
   update_anim_view_state(event->object, ad->label);
}

/**
 * @brief Callback for EFL_PLAYER_EVENT_PLAYBACK_PROGRESS_CHANGED event.
 *
 * Updates the slider position based on animation progress.
 *
 * @param data Pointer to App_Data containing UI elements.
 * @param event The Efl_Event structure. The event->info is a pointer to double indicating progress (0.0 to 1.0).
 */
static void
_animation_playback_progress_changed_cb(void *data, const Efl_Event *event)
{
   double progress = *(double*)event->info;
   App_Data *ad = data;
   efl_ui_range_value_set(ad->slider, progress);
}

/**
 * @brief Callback for EFL_PLAYER_EVENT_PLAYBACK_REPEATED event.
 *
 * Prints a message indicating the animation has looped.
 *
 * @param data Unused.
 * @param event The Efl_Event structure. The event->info is a pointer to int indicating repeat count.
 */
static void
_animation_playback_repeated_changed_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   int repeated_times = *(int*)event->info;
   printf("repeated! (times: %d)\n", repeated_times);
}

/**
 * @brief Callback for EFL_PLAYER_EVENT_PLAYBACK_FINISHED event.
 *
 * Prints a message indicating the animation has finished.
 *
 * @param data Unused.
 * @param event Unused.
 */
static void
_animation_playback_finished_changed_cb(void *data EINA_UNUSED, const Efl_Event *event EINA_UNUSED)
{
   printf("done!\n");
}

/**
 * @brief Array defining callbacks for various animation player events.
 *
 * This array maps player events (like playing changed, paused changed, etc.)
 * to their respective handler functions.
 *
 * Structure of elements:
 * @code
 * { EFL_PLAYER_EVENT_PLAYING_CHANGED, _animation_playing_changed_cb }
 * //  ^ Event to listen for          ^ Callback function to execute
 * @endcode
 */
EFL_CALLBACKS_ARRAY_DEFINE(animation_stats_cb,
  {EFL_PLAYER_EVENT_PLAYING_CHANGED, _animation_playing_changed_cb },
  {EFL_PLAYER_EVENT_PAUSED_CHANGED, _animation_paused_changed_cb },
  {EFL_PLAYER_EVENT_PLAYBACK_PROGRESS_CHANGED, _animation_playback_progress_changed_cb },
  {EFL_PLAYER_EVENT_PLAYBACK_REPEATED, _animation_playback_repeated_changed_cb },
  {EFL_PLAYER_EVENT_PLAYBACK_FINISHED, _animation_playback_finished_changed_cb },
)

/**
 * @brief Callback function for window deletion (close) event.
 *
 * Frees the allocated App_Data structure.
 *
 * @param data Pointer to App_Data to be freed.
 * @param ev Unused.
 */
static void
_win_del_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   App_Data *ad = data;
   free(ad);
}

/**
 * @brief Main function to set up and run the Efl_Ui_Vg_Animation demo.
 *
 * This function creates the window, layout, animation view, and control widgets
 * for the Lottie animation player demo. It handles the case where JSON Vg
 * loader is not available by displaying a static SVG image instead.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_efl_ui_vg_animation(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *box, *box2, *box3, *box4, *label, *anim_view, *check, *slider;
   char buf[255];
   App_Data *ad = calloc(1, sizeof(App_Data));
   if (!ad) return;

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                 efl_text_set(efl_added, "Efl_Ui_Vg_Animation demo"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE),
                 efl_event_callback_add(efl_added, EFL_EVENT_DEL, _win_del_cb, ad));

   // Create a box in Canvas
   box = efl_add(EFL_UI_BOX_CLASS, win,
                 efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, EFL_GFX_HINT_EXPAND),
                 efl_content_set(win, efl_added));

   //Create a label to show state of animation
   label = efl_add(EFL_UI_TEXTBOX_CLASS, win,
                   efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0.1),
                   efl_gfx_hint_fill_set(efl_added, EINA_FALSE, EINA_FALSE),
                   efl_pack(box, efl_added));

   //Create Animation View to play animation directly from JSON file
   snprintf(buf, sizeof(buf), "%s/images/emoji_wink.json", elm_app_data_dir_get());
   anim_view = efl_add(EFL_UI_VG_ANIMATION_CLASS, win,
                       efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, EFL_GFX_HINT_EXPAND),
                       efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
                       efl_gfx_entity_size_set(efl_added, EINA_SIZE2D(600, 600)),
                       efl_file_set(efl_added, buf),
                       efl_pack(box, efl_added));

   // Controller Set : 0
   box2 = efl_add(EFL_UI_BOX_CLASS, box,
                  efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0.1),
                  efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
                  efl_pack(box, efl_added));

   //Loop
   check = efl_add(EFL_UI_CHECK_CLASS, box2,
                   efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0),
                   efl_gfx_hint_fill_set(efl_added, EINA_FALSE, EINA_FALSE),
                   efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
                   efl_pack(box2, efl_added));
   efl_text_set(check, "Loop");
   efl_event_callback_add(check, EFL_UI_EVENT_SELECTED_CHANGED,  check_changed_cb, anim_view);

   //Speed: 0.5x
   check = efl_add(EFL_UI_CHECK_CLASS, box2,
                   efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0),
                   efl_gfx_hint_fill_set(efl_added, EINA_FALSE, EINA_FALSE),
                   efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
                   efl_pack(box2, efl_added));
   efl_text_set(check, "Speed: 0.25x");
   efl_event_callback_add(check, EFL_UI_EVENT_SELECTED_CHANGED,  speed_changed_cb, anim_view);

   //Limit Frames
   check = efl_add(EFL_UI_CHECK_CLASS, box2,
                   efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0),
                   efl_gfx_hint_fill_set(efl_added, EINA_FALSE, EINA_FALSE),
                   efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
                   efl_pack(box2, efl_added));
   efl_text_set(check, "Limit Frames");
   efl_event_callback_add(check, EFL_UI_EVENT_SELECTED_CHANGED,  limit_frame_cb, anim_view);


   //Duration Text
   snprintf(buf, sizeof(buf), "Duration(Length): %1.2fs", efl_playable_length_get(anim_view));
   efl_add(EFL_UI_TEXTBOX_CLASS, box2,
           efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0),
           efl_gfx_hint_fill_set(efl_added, EINA_FALSE, EINA_FALSE),
           efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
           efl_text_set(efl_added, buf),
           efl_pack(box2, efl_added));

   //Slider
   slider = efl_add(EFL_UI_SLIDER_CLASS, box,
                    efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0.1),
                    efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
                    efl_ui_range_limits_set(efl_added, 0, 1),
                    efl_event_callback_add(efl_added, EFL_UI_RANGE_EVENT_CHANGED, _slider_changed_cb, anim_view),
                    efl_pack(box, efl_added));

   //Controller Set: 1
   box3 = efl_add(EFL_UI_BOX_CLASS, box,
                  efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0),
                  efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, 1),
                  efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
                  efl_pack(box, efl_added));

   //Play Button
   efl_add(EFL_UI_BUTTON_CLASS, box3,
           efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0),
           efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
           efl_text_set(efl_added, "Play"),
           efl_pack(box3, efl_added),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, btn_clicked_cb, anim_view));


   //Play Back Button
   efl_add(EFL_UI_BUTTON_CLASS, box3,
           efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0),
           efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
           efl_text_set(efl_added, "Play Backwards"),
           efl_pack(box3, efl_added),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, btn_clicked_cb, anim_view));

   //Stop Button
   efl_add(EFL_UI_BUTTON_CLASS, box3,
           efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0),
           efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
           efl_text_set(efl_added, "Stop"),
           efl_pack(box3, efl_added),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, btn_clicked_cb, anim_view));

   //Controller Set: 2
   box4 = efl_add(EFL_UI_BOX_CLASS, box,
                  efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0),
                  efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, 1),
                  efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
                  efl_pack(box, efl_added));

   //Pause Button
   efl_add(EFL_UI_BUTTON_CLASS, box3,
           efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0),
           efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
           efl_text_set(efl_added, "Pause"),
           efl_pack(box4, efl_added),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, btn_clicked_cb, anim_view));

   //Resume Button
   efl_add(EFL_UI_BUTTON_CLASS, box3,
           efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0),
           efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
           efl_text_set(efl_added, "Resume"),
           efl_pack(box4, efl_added),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, btn_clicked_cb, anim_view));

   efl_event_callback_array_add(anim_view, animation_stats_cb(), ad);

   update_anim_view_state(anim_view, label);

   ad->label = label;
   ad->slider = slider;

   efl_gfx_entity_size_set(win, EINA_SIZE2D(600, 730));
}

#else

/**
 * @brief Fallback function for the Efl_Ui_Vg_Animation demo when JSON Vg loader is not available.
 *
 * This function creates a window and displays a static SVG image with a message
 * indicating that the Lottie loader is not supported.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_efl_ui_vg_animation(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *box;
   char buf[255];

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl_Ui_Vg_Animation demo"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   // Create a box
   box = efl_add(EFL_UI_BOX_CLASS, win,
                 efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, EFL_GFX_HINT_EXPAND),
                 efl_content_set(win, efl_added));

   efl_add(EFL_UI_TEXTBOX_CLASS, win,
           efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, 0.1),
           efl_gfx_hint_fill_set(efl_added, EINA_FALSE, EINA_FALSE),
           efl_text_set(efl_added, "Evas Vg Json (Lottie) Loader is not supported, Only Static Vector Image is available!"),
           efl_pack(box, efl_added));

   //Create Vector object.
   snprintf(buf, sizeof(buf), "%s/images/tiger.svg", elm_app_data_dir_get());
   efl_add(EFL_CANVAS_VG_OBJECT_CLASS, win,
           efl_gfx_hint_weight_set(efl_added, EFL_GFX_HINT_EXPAND, EFL_GFX_HINT_EXPAND),
           efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
           efl_gfx_entity_size_set(efl_added, EINA_SIZE2D(600, 600)),
           efl_file_simple_load(efl_added, buf, NULL),
           efl_gfx_entity_visible_set(efl_added, EINA_TRUE),
           efl_pack(box, efl_added));

   efl_gfx_entity_size_set(win, EINA_SIZE2D(600, 730));
}

#endif
