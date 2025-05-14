#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Application data structure for the EFL animation start delay test.
 *
 * This structure holds all the necessary Evas objects and state
 * for managing the animation test, including animations, widgets,
 * and visibility status.
 */
typedef struct _App_Data
{
   Efl_Canvas_Animation        *show_anim; /**< The animation to show the button (fade in). */
   Efl_Canvas_Animation        *hide_anim; /**< The animation to hide the button (fade out). */
   Elm_Button                  *button; /**< The button that will be animated. */

   Evas_Object          *start_delay_spin; /**< Spinner to set the animation start delay. */

   Eina_Bool             is_btn_visible; /**< Flag to track the visibility state of the button. */
} App_Data;

/**
 * @brief Callback function for animation state changes (started/ended).
 *
 * This function is triggered when an animation begins or concludes. It prints a
 * status message and manages the interactivity of the start delay spinner,
 * disabling it during animation playback to prevent conflicts.
 *
 * @param data The application data (App_Data instance).
 * @param event The EFL event description. The event's info will be the
 *              animation object on start, and NULL on end.
 */
static void
_anim_changed_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *anim = event->info;
   App_Data *ad = data;

   if (anim)
     {
        printf("Animation has been started!\n");
        elm_object_disabled_set(ad->start_delay_spin, EINA_FALSE);
     }
   else
     {
        printf("Animation has been ended!\n");
        elm_object_disabled_set(ad->start_delay_spin, EINA_TRUE);
     }
}

/**
 * @brief Callback for animation progress updates.
 *
 * Called repeatedly while an animation is running. It prints the current
 * progress of the animation.
 *
 * @param data Application data (unused).
 * @param event The EFL event description. The event's info is a pointer to a
 *              double representing the animation progress (from 0.0 to 1.0).
 */
static void
_anim_running_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   double *progress = event->info;
   printf("Animation is running! Current progress(%lf)\n", *progress);
}

/**
 * @brief Array of callbacks for monitoring animation status.
 *
 * This array maps animation events to their respective handler functions.
 * It's used to attach multiple callbacks to the animated button at once.
 * @li EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_CHANGED: Handled by _anim_changed_cb.
 * @li EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_PROGRESS_UPDATED: Handled by _anim_running_cb.
 */
EFL_CALLBACKS_ARRAY_DEFINE(animation_stats_cb,
  {EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_CHANGED, _anim_changed_cb },
  {EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_PROGRESS_UPDATED, _anim_running_cb },
)

/**
 * @brief Callback for the "Start Animation" button click.
 *
 * This function is called when the user clicks the button to start an animation.
 * It toggles between the show and hide animations based on the button's current
 * visibility state. It retrieves the start delay value from the spinner, applies
 * it to the appropriate animation, and then starts the animation on the target button.
 *
 * @param data The application data (App_Data instance).
 * @param obj The button that was clicked to start the animation.
 * @param event_info Evas event info (unused).
 */
static void
_start_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   App_Data *ad = data;

   ad->is_btn_visible = !(ad->is_btn_visible);

   double start_delay = elm_spinner_value_get(ad->start_delay_spin);
   elm_object_disabled_set(ad->start_delay_spin, EINA_TRUE);

   if (ad->is_btn_visible)
     {
        //Set animation start delay
        efl_animation_start_delay_set(ad->show_anim, start_delay);

        //Create Animation Object from Animation
        efl_canvas_object_animation_start(ad->button, ad->show_anim, 1.0, 0.0);
        efl_text_set(obj, "Start Alpha Animation from 1.0 to 0.0");
     }
   else
     {
        //Set animation start delay
        efl_animation_start_delay_set(ad->hide_anim, start_delay);

        //Create Animation Object from Animation
        efl_canvas_object_animation_start(ad->button, ad->hide_anim, 1.0, 0.0);
        efl_text_set(obj, "Start Alpha Animation from 0.0 to 1.0");
     }
}

/**
 * @brief Callback for window deletion request.
 *
 * This function is called when the window is about to be closed.
 * It frees the application data structure to prevent memory leaks.
 *
 * @param data The application data (App_Data instance) to be freed.
 * @param obj The window object (unused).
 * @param event_info Evas event info (unused).
 */
static void
_win_del_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   App_Data *ad = data;
   free(ad);
}

/**
 * @brief Main function for the EFL animation start delay test.
 *
 * This function sets up the Elementary test window, creates all necessary UI
 * components (buttons, spinner), and initializes the alpha animations. It also
 * allocates and initializes the application data structure and connects all
 * callbacks to their respective events. This test demonstrates how to use
 * efl_animation_start_delay_set() to postpone the start of an animation.
 *
 * @param data Test data (unused).
 * @param obj Parent object (unused).
 * @param event_info Event info (unused).
 */
void
test_efl_anim_start_delay(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   App_Data *ad = calloc(1, sizeof(App_Data));
   if (!ad) return;

   Evas_Object *win = elm_win_add(NULL, "Efl Animation Start Delay", ELM_WIN_BASIC);
   elm_win_title_set(win, "Efl Animation Start Delay");
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


   //Button to start animation
   Evas_Object *start_btn = elm_button_add(win);
   elm_object_text_set(start_btn, "Start Alpha Animation from 1.0 to 0.0");
   evas_object_smart_callback_add(start_btn, "clicked", _start_btn_clicked_cb, ad);
   evas_object_size_hint_weight_set(start_btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_resize(start_btn, 200, 50);
   evas_object_move(start_btn, 100, 300);
   evas_object_show(start_btn);

   //Spinner to set animation start delay
   Evas_Object *start_delay_spin = elm_spinner_add(win);
   elm_spinner_label_format_set(start_delay_spin, "Start Delay: %.1f second");
   elm_spinner_editable_set(start_delay_spin, EINA_FALSE);
   elm_spinner_min_max_set(start_delay_spin, 0.0, 10.0);
   elm_spinner_step_set(start_delay_spin, 0.5);
   elm_spinner_value_set(start_delay_spin, 0.0);
   evas_object_resize(start_delay_spin, 200, 50);
   evas_object_move(start_delay_spin, 100, 350);
   evas_object_show(start_delay_spin);

   //Initialize App Data
   ad->show_anim = show_anim;
   ad->hide_anim = hide_anim;
   ad->start_delay_spin = start_delay_spin;
   ad->is_btn_visible = EINA_TRUE;
   ad->button = btn;

   evas_object_resize(win, 400, 450);
   evas_object_show(win);
}
