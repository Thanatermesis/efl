#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "Efl_Canvas_Wl.h"
#include "Elementary.h"

static Evas_Object *win; ///< The main window object.
static Eina_Strbuf *buf; ///< Buffer to store command line arguments.
static Eo *exe; ///< The executable object run by efl_canvas_wl_run.

/**
 * @brief Handles the EFL_TASK_EVENT_EXIT event.
 *
 * This function is called when the executed task (exe) exits.
 * It quits the main loop if the event object is the executed task.
 *
 * @param d User data, unused.
 * @param ev The Efl_Event structure containing event information.
 */
static void
del_handler(void *d EINA_UNUSED, const Efl_Event *ev)
{
   if (ev->object == exe) ecore_main_loop_quit();
}

/**
 * @brief Handles the EVAS_CALLBACK_FOCUS_IN event for the main window.
 *
 * Sets focus to the Efl_Canvas_Wl object when the main window gains focus.
 *
 * @param data The Efl_Canvas_Wl object to focus.
 * @param e The Evas canvas, unused.
 * @param obj The Evas object that triggered the event (the window), unused.
 * @param event_info Event specific information, unused.
 */
static void
focus_in(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   evas_object_focus_set(data, 1);
}

/**
 * @brief Executes a command using efl_canvas_wl_run.
 *
 * This function is called by an ecore_timer. It runs the command
 * specified in the global buffer `buf` using the provided `data`
 * (Efl_Canvas_Wl object) as the Wayland surface.
 *
 * @param data The Efl_Canvas_Wl object to be used as the Wayland surface.
 * @return EINA_FALSE to stop the timer from recurring.
 */
static Eina_Bool
dostuff(void *data)
{
   exe = efl_canvas_wl_run(data, eina_strbuf_string_get(buf));
   efl_event_callback_add(exe, EFL_TASK_EVENT_EXIT, del_handler, NULL);
   evas_object_focus_set(data, 1);
   return EINA_FALSE;
}

/**
 * @brief Handles the EVAS_CALLBACK_CHANGED_SIZE_HINTS event.
 *
 * Propagates size hints (aspect, min, max) from the Efl_Canvas_Wl object
 * to the main window. This is used when efl_canvas_wl_aspect_propagate_set
 * and efl_canvas_wl_minmax_propagate_set are enabled.
 *
 * @param data The main window object (win).
 * @param e The Evas canvas, unused.
 * @param obj The Efl_Canvas_Wl object whose hints changed.
 * @param event_info Event specific information, unused.
 */
static void
hints_changed(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   int w, h;
   Evas_Aspect_Control aspect;

   evas_object_size_hint_aspect_get(obj, &aspect, &w, &h);
   evas_object_size_hint_aspect_set(data, aspect, w, h);
   evas_object_size_hint_min_get(obj, &w, &h);
   evas_object_size_hint_min_set(data, w, h);
   evas_object_size_hint_max_get(obj, &w, &h);
   evas_object_size_hint_max_set(data, w, h);
}

/**
 * @brief Main function for the efl_canvas_wl_test application.
 *
 * Initializes Elementary, creates a window, and sets up an Efl_Canvas_Wl
 * object to run a client application specified by command line arguments.
 * The client application's output will be displayed within this Efl_Canvas_Wl object.
 *
 * @param argc Number of command line arguments.
 * @param argv Array of command line argument strings.
 *             The first argument (argv[0]) is the program name.
 *             Subsequent arguments (argv[1] onwards) specify the
 *             command and its arguments to be executed by efl_canvas_wl_run.
 *             Example: ./efl_canvas_wl_test evas_test evas_object_textblock
 *
 * @return 0 on successful execution, non-zero otherwise.
 */
int
main(int argc, char *argv[])
{
   Evas_Object *o;
   int i;

   if (argc < 2) return 0;
   elm_init(argc, (char**)argv);
   buf = eina_strbuf_new();
   for (i = 1; i < argc; i++)
     {
        eina_strbuf_append_escaped(buf, argv[i]);
        if (i + 1 < argc) eina_strbuf_append_char(buf, ' ');
     }

   win = elm_win_util_standard_add("comp", "comp");
   elm_win_autodel_set(win, 1);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   o = efl_add(EFL_CANVAS_WL_CLASS, win);
   efl_canvas_wl_aspect_propagate_set(o, 1);
   efl_canvas_wl_minmax_propagate_set(o, 1);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_event_callback_add(o, EVAS_CALLBACK_CHANGED_SIZE_HINTS, hints_changed, win);
   elm_win_resize_object_add(win, o);
   evas_object_show(o);
   evas_object_show(win);
   evas_object_resize(win, 640, 480);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FOCUS_IN, focus_in, o);
   ecore_timer_add(1, dostuff, o);
   
   elm_run();
   elm_shutdown();
   return 0;
}
