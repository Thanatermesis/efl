#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Application data structure to hold widgets and state for the test.
 */
typedef struct _App_Data
{
   Efl_Canvas_Animation        *show_anim; /**< Animation to fade in the button. */
   Efl_Canvas_Animation        *hide_anim; /**< Animation to fade out the button. */
   Elm_Button                  *button;    /**< The button that will be animated. */

   Eina_Bool             is_btn_visible;   /**< Flag to track the visibility state of the button. */
} App_Data;

/**
 * @brief Callback function for the 'animation,changed' event.
 *
 * This function is called when an animation starts or ends.
 * It prints a message indicating the state change.
 * @param data User data. Not used in this function.
 * @param event The EFL event data. The event->info will be the animation
 * object when the animation starts, and NULL when it ends.
 */
static void
_anim_changed_cb(void *data EINA_UNUSED, const Efl_Event *event EINA_UNUSED)
{
   Eo *anim = event->info;

   if (anim)
     {
        printf("Animation has been started!\n");
     }
   else
     {
        printf("Animation has been ended!\n");
     }
}

/**
 * @brief Callback function for the 'animation,progress,updated' event.
 *
 * This function is called repeatedly as the animation runs, providing progress updates.
 * @param data User data. Not used in this function.
 * @param event The EFL event data. The event->info is a pointer to a double
 * representing the current progress of the animation (from 0.0 to 1.0).
 */
static void
_anim_running_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   double *progress = event->info;
   printf("Animation is running! Current progress(%lf)\n", *progress);
}

/**
 * @brief Defines an array of EFL event callbacks for monitoring animation status.
 *
 * This array maps animation events to their corresponding callback functions:
 * - EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_CHANGED: Triggered when an animation
 *   starts or stops. Handled by _anim_changed_cb().
 * - EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_PROGRESS_UPDATED: Triggered during
 *   animation playback. Handled by _anim_running_cb().
 */
EFL_CALLBACKS_ARRAY_DEFINE(animation_stats_cb,
  {EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_CHANGED, _anim_changed_cb },
  {EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_PROGRESS_UPDATED, _anim_running_cb },
)

/**
 * @brief Callback for the 'clicked' event on the control button.
 *
 * Toggles the visibility of the target button by starting the appropriate
 * fade-in or fade-out animation.
 * @param data A pointer to the App_Data structure.
 * @param obj The button that was clicked to trigger the animation.
 * @param event_info Extra event information. Not used here.
 */
static void
_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   App_Data *ad = data;

   ad->is_btn_visible = !(ad->is_btn_visible);

   if (ad->is_btn_visible)
     {
        //Create Animation Object from Animation
        efl_canvas_object_animation_start(ad->button, ad->show_anim, 1.0, 0.0);
        efl_text_set(obj, "Start Alpha Animation from 1.0 to 0.0");
     }
   else
     {
        //Create Animation Object from Animation
        efl_canvas_object_animation_start(ad->button, ad->hide_anim, 1.0, 0.0);
        efl_text_set(obj, "Start Alpha Animation from 0.0 to 1.0");
     }

   //Let Animation Object start animation
}

/**
 * @brief Callback for the window's 'delete,request' event.
 *
 * Cleans up resources by freeing the App_Data structure.
 * @param data A pointer to the App_Data structure to be freed.
 * @param obj The window object. Not used here.
 * @param event_info Extra event information. Not used here.
 */
static void
_win_del_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   App_Data *ad = data;
   free(ad);
}

/**
 * @brief Main function for the EFL alpha animation test.
 *
 * This test demonstrates how to use Efl_Canvas_Alpha_Animation to animate the
 * alpha property of an object. It sets up a window with two buttons. One button
 * is the target of the animation, and the other button controls the start of
 * the animation.
 *
 * The alpha animation transitions the object's color between two alpha values
 * over a specified duration.
 *
 * @param data User data. Not used in this test.
 * @param obj Parent object. Not used in this test.
 * @param event_info Extra event information. Not used in this test.
 */
void
test_efl_anim_alpha(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   App_Data *ad = calloc(1, sizeof(App_Data));
   if (!ad) return;

   Evas_Object *win = elm_win_add(NULL, "Efl Animation Alpha", ELM_WIN_BASIC);
   elm_win_title_set(win, "Efl Animation Alpha");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", _win_del_cb, ad);

   //Button to be animated
   Evas_Object *btn = elm_button_add(win);
   elm_object_text_set(btn, "Button");
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_resize(btn, 200, 200);
   evas_object_move(btn, 100, 50);
   evas_object_show(btn);
   efl_event_callback_array_add(btn, animation_stats_cb(), ad);

   //Show Animation
   Efl_Canvas_Animation *show_anim = efl_add(EFL_CANVAS_ALPHA_ANIMATION_CLASS, win);
   efl_animation_alpha_set(show_anim, 0.0, 1.0);
   efl_animation_duration_set(show_anim, 1.0);
   efl_animation_final_state_keep_set(show_anim, EINA_TRUE);

   //Hide Animation
   Efl_Canvas_Animation *hide_anim = efl_add(EFL_CANVAS_ALPHA_ANIMATION_CLASS, win);
   efl_animation_alpha_set(hide_anim, 1.0, 0.0);
   efl_animation_duration_set(hide_anim, 1.0);
   efl_animation_final_state_keep_set(hide_anim, EINA_TRUE);

   //Initialize App Data
   ad->show_anim = show_anim;
   ad->hide_anim = hide_anim;
   ad->is_btn_visible = EINA_TRUE;
   ad->button = btn;

   //Button to start animation
   Evas_Object *btn2 = elm_button_add(win);
   elm_object_text_set(btn2, "Start Alpha Animation from 1.0 to 0.0");
   evas_object_smart_callback_add(btn2, "clicked", _btn_clicked_cb, ad);
   evas_object_size_hint_weight_set(btn2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_resize(btn2, 200, 50);
   evas_object_move(btn2, 100, 300);
   evas_object_show(btn2);

   evas_object_resize(win, 400 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}
