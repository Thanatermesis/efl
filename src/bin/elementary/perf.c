#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>
#include "perf.h"
///////////////////////////////////////////////////////////////////////////////

/** @file perf.c
 * @brief Main application logic for the Elementary performance test suite.
 * This file contains the core framework for running and managing individual
 * performance tests.
 */

///////////////////////////////////////////////////////////////////////////////
static Evas *evas; /**< The main Evas canvas. */
static Evas_Object *win, *bg; /**< The main window and background Evas objects. */
static double total_time = 0.0; /**< Accumulated time spent in rendering frames for the current test. */
static int total_frames = 0; /**< Total number of frames rendered for the current test. */
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Structure defining a single performance test.
 */
typedef struct
{
   void        (*init) (Evas *e); /**< Function pointer to initialize the test.
                                   *   @param e The Evas canvas. */
   void        (*tick) (Evas *e, double pos, Evas_Coord win_w, Evas_Coord win_h); /**< Function pointer for each animation tick of the test.
                                                                                  *   @param e The Evas canvas.
                                                                                  *   @param pos The normalized position in the animation (0.0 to 1.0).
                                                                                  *   @param win_w The current width of the window.
                                                                                  *   @param win_h The current height of the window. */
   const char   *desc; /**< A human-readable description of the test. */
   double        weight; /**< The weight of this test in the overall average FPS calculation. */
} Test;

static Eina_List *cleanup_list = NULL; /**< List of Evas_Objects to be cleaned up after each test. */

/**
 * @brief Adds an Evas_Object to the cleanup list.
 * Objects in this list are deleted after the current test completes.
 * @param o The Evas_Object to add.
 */
void
cleanup_add(Evas_Object *o)
{
   cleanup_list = eina_list_append(cleanup_list, o);
}

#define T2
#include "perf_list.c"
#undef T2

#define T1
/**
 * @brief Array of all available performance tests.
 * This array is populated by including `perf_list.c` which uses the `TFUN` macro.
 * Each element is a `Test` struct.
 * Example structure:
 * @code
 * static Test tests[] = {
 *    { test_foo_init, test_foo_tick, "Description of foo test", 1.0 },
 *    { test_bar_init, test_bar_tick, "Description of bar test", 0.5 },
 *    { NULL, NULL, NULL, 0.0 } // Terminator
 * };
 * @endcode
 */
static Test tests[] = {
#define TFUN(x) test_ ## x ## _init, test_ ## x ## _tick
#include "perf_list.c"
   { NULL, NULL, NULL, 0.0 } /* Terminator for the tests array. */
};
#undef T1

static unsigned int test_pos = 0; /**< Index of the current test being run in the `tests_to_do` array. */
static double time_start = 0.0; /**< Timestamp when the current test's animation phase started. */
static double anim_tick_delta_total = 0.0; /**< Sum of time deltas between animation ticks for the current test. */
static int anim_tick_total = 0; /**< Total number of animation ticks for the current test. */
static Eina_Array *tests_to_do = NULL; /**< Array of test indices (1-based) to be executed. If NULL, all tests are run. */
static double tests_fps = 0.0; /**< Accumulated weighted FPS across all completed tests. */
static double tests_weights = 0.0; /**< Sum of weights of all completed tests. */
static double run_time = 5.0; /**< Duration in seconds for each test to run. Default is 5.0s. */
static double spin_up_delay = 2.0; /**< Initial delay in seconds before starting the first test, to allow system to settle. Default is 2.0s. */

/**
 * @brief Runs the next test in the sequence or finishes if all tests are done.
 * @param e The Evas canvas.
 */
static void all_tests(Evas *e);

/**
 * @brief Timer callback to introduce a delay before starting the next test.
 * This calls all_tests() to proceed.
 * @param data The Evas canvas (passed as user data).
 * @return EINA_FALSE to stop the timer.
 */
static Eina_Bool
next_test_delay(void *data EINA_UNUSED)
{
   all_tests(data);
   return EINA_FALSE;
}

#define ANIMATOR 1

#ifdef ANIMATOR
static Ecore_Animator *animator = NULL; /**< Ecore_Animator used for driving test animations. */

/**
 * @brief Animation tick callback (Ecore_Animator version).
 * This function is called repeatedly by the ecore animator to drive the
 * current test's animation and measure performance.
 * @param data The Evas canvas (passed as user data).
 * @return EINA_TRUE to continue the animator, EINA_FALSE to stop.
 */
static Eina_Bool
anim_tick(void *data)
#else
/**
 * @brief Animation tick callback (EFL event version).
 * This function is called repeatedly via EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK
 * to drive the current test's animation and measure performance.
 * @param data The Evas canvas (passed as user data).
 * @param event The EFL event details (unused).
 */
static void
anim_tick(void *data, const Efl_Event *event EINA_UNUSED)
#endif
{
   Evas_Coord win_w, win_h;
   double f = ecore_time_get() - time_start;
   static double pf = 0.0;
   int p;

   if (total_frames == 1) time_start = ecore_time_get();
   if (anim_tick_total == 1)
     {
        anim_tick_delta_total = 0.0;
        pf  = f;
     }
   else
     {
        anim_tick_delta_total += (f - pf);
     }
   anim_tick_total++;
   pf = f;
   f = f / run_time; // time per test - 5sec.
   p = (int)(uintptr_t)eina_array_data_get(tests_to_do, test_pos) - 1;
   evas_output_viewport_get(data, NULL, NULL, &win_w, &win_h);
   tests[p].tick(data, f, win_w, win_h);
   if (f >= 1.0)
     {
        Evas_Object *o;
        double time_spent = ecore_time_get() - time_start;
        double load = total_time / time_spent;

        // only got 1 frame rendered? eek. just assume we got one
        if (total_frames < 2) total_frames = 2;
        if (anim_tick_total < 2) anim_tick_total = 2;
        if ((load <= 0.0) || (anim_tick_delta_total <= 0.0) ||
           (run_time <= 0))
          {
             printf("?? | %s\n", tests[p].desc);
          }
        else
          {
             printf("%1.2f (fr=%i load=%1.5f tick=%i@%1.2fHz) | %1.2f %s\n",
                    (double)(total_frames - 2) / (load * run_time),
                    total_frames - 2,
                    load,
                    anim_tick_total - 2,
                    (double)(anim_tick_total - 2) / anim_tick_delta_total,
                    tests[p].weight,
                    tests[p].desc);
             tests_fps += ((double)(total_frames - 2) / (load * run_time)) *
               tests[p].weight;
             tests_weights += tests[p].weight;
          }
        total_frames = 0.0;
        total_time = 0.0;
        EINA_LIST_FREE(cleanup_list, o) evas_object_del(o);
        test_pos++;
#ifdef ANIMATOR
        ecore_animator_del(animator);
        animator = NULL;
#else
        efl_event_callback_del(win, EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK, anim_tick, data);
#endif
        ecore_timer_add(0.5, next_test_delay, data);
     }
#ifdef ANIMATOR
   return EINA_TRUE;
#endif
}

/**
 * @brief Timer callback to delay exiting the application.
 * Allows final messages to be seen before the application closes.
 * @param data Unused.
 * @return EINA_FALSE to stop the timer.
 */
static Eina_Bool
exit_delay(void *data EINA_UNUSED)
{
   elm_exit();
   return EINA_FALSE;
}

/**
 * @brief Manages the execution of performance tests.
 * Initializes and starts the current test based on `test_pos`.
 * If all specified tests are completed, it prints the summary and schedules an exit.
 * @param e The Evas canvas.
 */
static void
all_tests(Evas *e)
{
   Evas_Coord win_w, win_h;
   int p;

   evas_output_viewport_get(e, NULL, NULL, &win_w, &win_h);
   if (test_pos >= eina_array_count_get(tests_to_do))
     {
        printf("--------------------------------------------------------------------------------\n");
        printf("Average weighted FPS: %1.2f\n", tests_fps / tests_weights);
        printf("--------------------------------------------------------------------------------\n");
        ecore_timer_add(1.0, exit_delay, NULL);
        return;
     }
   p = (int)(uintptr_t)eina_array_data_get(tests_to_do, test_pos) - 1;
   tests[p].init(e);
   tests[p].tick(e, 0.0, win_h, win_h);
   time_start = ecore_time_get();
   anim_tick_delta_total = 0.0;
   anim_tick_total = 0;
#ifdef ANIMATOR
   animator = ecore_animator_add(anim_tick, e);
#else
   efl_event_callback_add(win, EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK, anim_tick, e);
#endif
}

/**
 * @brief Timer callback to start the test sequence after an initial delay.
 * This calls all_tests() to begin the first test.
 * @param data The Evas canvas (passed as user data).
 * @return EINA_FALSE to stop the timer.
 */
static Eina_Bool
all_tests_delay(void *data)
{
   all_tests(data);
   return EINA_FALSE;
}

static double rtime = 0.0; /**< Timestamp captured just before rendering starts. */

/**
 * @brief Evas event callback executed before rendering a frame.
 * Captures the start time of the render operation.
 * @param data Unused.
 * @param e Unused.
 * @param info Unused.
 */
static void
render_pre(void *data EINA_UNUSED, Evas *e EINA_UNUSED, void *info EINA_UNUSED)
{
   rtime = ecore_time_get();
}

/**
 * @brief Evas event callback executed after rendering a frame (post flush).
 * Calculates the time spent on rendering the frame and updates totals.
 * @param data Unused.
 * @param e Unused.
 * @param info Unused.
 */
static void
render_post(void *data EINA_UNUSED, Evas *e EINA_UNUSED, void *info EINA_UNUSED)
{
   double spent = ecore_time_get() - rtime;
   if (total_frames == 2) total_time = 0.0; // Reset total_time after the first couple of frames to ignore setup.
   total_time += spent;
   total_frames++;
}

/**
 * @brief Idler function to keep the CPU busy during the spin-up delay.
 * This helps in achieving a more consistent state before tests start,
 * potentially by encouraging the CPU to ramp up to its performance state.
 * @param data Unused.
 * @return EINA_TRUE to keep the idler running.
 */
static Eina_Bool
_spincpu_up_idler(void *data EINA_UNUSED)
{
   return EINA_TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/**
 * @brief Main entry point for the Elementary application.
 * Parses command-line arguments, sets up the Evas canvas and window,
 * and starts the performance test sequence.
 *
 * Command-line arguments:
 * - `-h` or `--help`: Display help message.
 * - `-l`: List all available tests with their numbers and descriptions.
 * - `-t N`: Run only test number N. Can be used multiple times to select multiple tests.
 *           Example: `-t 1 -t 5` runs test 1 and test 5.
 * - `-r N`: Set the duration for each test to N seconds.
 *           Example: `-r 10.0` runs each test for 10 seconds.
 * - `-d N`: Set the initial spin-up delay to N seconds before tests start.
 *           Example: `-d 3.0` waits 3 seconds before the first test.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return Exit status of the application.
 */
EAPI int
elm_main(int argc, char **argv)
{
   int i, j;

   for (i = 1; i < argc; i++)
     {
        if ((!strcmp(argv[i], "--help")) ||
            (!strcmp(argv[i], "-help")) ||
            (!strcmp(argv[i], "-h")))
          {
             printf("Usage:\n"
                    "  -h              : This help\n"
                    "  -l              : List all tests\n"
                    "  -t N            : Run test number N\n"
                    "  -r N            : Run each test for N seconds\n"
                    "  -d N            : Initial spin-up delay\n"
                    "\n");
             elm_exit();
             return 1;
          }
        else if (!strcmp(argv[i], "-l"))
          {
             for (j = 0; tests[j].init; j++)
               {
                  printf("  %3i | %s\n", j, tests[j].desc);
               }
             elm_exit();
             return 1;
          }
        else if ((!strcmp(argv[i], "-t")) && (i < (argc - 1)))
          {
             i++;
             if (!tests_to_do) tests_to_do = eina_array_new(32);
             eina_array_push(tests_to_do, (void *)(uintptr_t)atoi(argv[i]));
          }
        else if ((!strcmp(argv[i], "-r")) && (i < (argc - 1)))
          {
             i++;
             run_time = eina_convert_strtod_c(argv[i], NULL);
          }
        else if ((!strcmp(argv[i], "-d")) && (i < (argc - 1)))
          {
             i++;
             spin_up_delay = eina_convert_strtod_c(argv[i], NULL);
          }
     }
   if (!tests_to_do)
     {
        tests_to_do = eina_array_new(32);
        for (j = 0; tests[j].init; j++)
          {
             eina_array_push(tests_to_do, (void *)(uintptr_t)(j + 1));
          }
     }
   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_lib_dir_set(PACKAGE_LIB_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
   elm_app_info_set(elm_main, "elementary", "images/logo.png");

   win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
   if (!win)
     {
        elm_exit();
        return 1;
     }
   evas = evas_object_evas_get(win);
   elm_win_autodel_set(win, EINA_TRUE);
   elm_win_title_set(win, "Elementary Performance Test");
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   bg = evas_object_rectangle_add(evas);
   evas_object_color_set(bg, 128, 128, 128, 255);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   evas_event_callback_add(evas, EVAS_CALLBACK_RENDER_PRE, render_pre, NULL);
   evas_event_callback_add(evas, EVAS_CALLBACK_RENDER_FLUSH_POST, render_post, NULL);

   ecore_idler_add(_spincpu_up_idler, NULL);
   printf("--------------------------------------------------------------------------------\n");
   printf("Performance Test Engine: %s\n",
          ecore_evas_engine_name_get(ecore_evas_ecore_evas_get(evas)));
   printf("--------------------------------------------------------------------------------\n");
   ecore_timer_add(spin_up_delay, all_tests_delay, evas);

   evas_object_resize(win, 800, 800);
   evas_object_show(win);

   elm_run();

   return 0;
}
ELM_MAIN()
///////////////////////////////////////////////////////////////////////////////
