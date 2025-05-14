#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

#ifdef HAVE_DLSYM
# include <dlfcn.h>
#endif

#include <sys/types.h>
#ifdef HAVE_SYS_SYSINFO_H
# include <sys/sysinfo.h>
#endif

#ifdef _WIN32
# include <evil_private.h> /* setenv */
# undef HAVE_DLSYM
#endif

#include <Eina.h>
#include <Eo.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_File.h>
#include <Ecore_Getopt.h>
#include <Ecore_Con.h>
#include <Elementary.h>

#include "common.h"

#define STABILIZE_KEY_STR "F1"
#define SHOT_KEY_STR "F2"
#define SAVE_KEY_STR "F3"

#define DBG(...) EINA_LOG_DOM_DBG(_log_domain, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_log_domain, __VA_ARGS__)

static int _log_domain = -1;

static const char *_out_filename = NULL;

static Eina_List *_evas_list = NULL;
static unsigned int _last_evas_id = 0;

static Exactness_Unit *_unit = NULL;

static char *_shot_key = NULL;
static unsigned int _last_timestamp = 0.0;

/**
 * @brief Converts an Efl_Pointer_Action to an Exactness_Action_Type.
 *
 * @param t The EFL pointer action type.
 * @return The corresponding exactness action type, or EXACTNESS_ACTION_UNKNOWN.
 */
static Exactness_Action_Type
_event_pointer_type_get(Efl_Pointer_Action t)
{
   switch(t)
     {
      case EFL_POINTER_ACTION_IN: return EXACTNESS_ACTION_MOUSE_IN;
      case EFL_POINTER_ACTION_OUT: return EXACTNESS_ACTION_MOUSE_OUT;
      case EFL_POINTER_ACTION_DOWN: return EXACTNESS_ACTION_MULTI_DOWN;
      case EFL_POINTER_ACTION_UP: return EXACTNESS_ACTION_MULTI_UP;
      case EFL_POINTER_ACTION_MOVE: return EXACTNESS_ACTION_MULTI_MOVE;
      case EFL_POINTER_ACTION_WHEEL: return EXACTNESS_ACTION_MOUSE_WHEEL;
      default: return EXACTNESS_ACTION_UNKNOWN;
     }
}

/**
 * @brief Writes the recorded test unit data to the output file.
 *
 * This function is typically called at the end of a test run to save the
 * captured events.
 */
static void
_output_write()
{
   if (_unit) exactness_unit_file_write(_unit, _out_filename);
}

/**
 * @brief Creates and adds a new action to the list of recorded actions.
 *
 * This function appends a new action to the Exactness_Unit's action list.
 * It calculates the delay since the last event and copies the event-specific
 * data. To avoid redundant events, it checks if the new event is identical
 * to the previous one (same type, timestamp, canvas, and data) and, if so,
 * does not add it.
 *
 * @param type The type of the action to add.
 * @param n_evas The ID of the Evas canvas where the event occurred.
 * @param timestamp The timestamp of the event in milliseconds.
 * @param data A pointer to the action-specific data (e.g., coordinates).
 * @param len The length of the action-specific data in bytes.
 */
static void
_add_to_list(Exactness_Action_Type type, unsigned int n_evas, unsigned int timestamp, void *data, int len)
{
   if (_unit)
     {
        const Exactness_Action *prev_v = eina_list_last_data_get(_unit->actions);
        if (prev_v)
          {
             if (prev_v->type == type &&
                   timestamp == _last_timestamp &&
                   prev_v->n_evas == n_evas &&
                   (!len || !memcmp(prev_v->data, data, len))) return;
          }
        INF("Recording %s\n", _exactness_action_type_to_string_get(type));
        Exactness_Action *act =  malloc(sizeof(*act));
        act->type = type;
        act->n_evas = n_evas;
        act->delay_ms = timestamp - _last_timestamp;
        _last_timestamp = timestamp;
        if (len)
          {
             act->data = malloc(len);
             memcpy(act->data, data, len);
          }
        _unit->actions = eina_list_append(_unit->actions, act);
     }
}

/**
 * @brief Retrieves the unique ID associated with an Evas canvas.
 *
 * This ID is assigned when the Evas canvas is first seen by the recorder.
 *
 * @param e The Evas canvas object.
 * @return The unique integer ID for the canvas.
 */
static int
_evas_id_get(Evas *e)
{
   return (intptr_t)efl_key_data_get(e, "__evas_id");
}

/**
 * @brief Callback for handling pointer-related events (mouse, touch).
 *
 * This function is triggered by various pointer events. It extracts relevant
 * information like timestamp, position, and button from the event and records
 * it as an action using _add_to_list().
 *
 * @param data The Evas canvas Eo object.
 * @param event The EFL event information.
 */
static void
_event_pointer_cb(void *data, const Efl_Event *event)
{
   Eo *eo_e = data;
   Eo *evp = event->info;
   if (!evp) return;

   int timestamp = efl_input_timestamp_get(evp);
   int n_evas = _evas_id_get(eo_e);
   Efl_Pointer_Action action = efl_input_pointer_action_get(evp);
   Exactness_Action_Type evt = _event_pointer_type_get(action);

   if (!timestamp) return;

  DBG("Calling \"%s\" timestamp=<%u>\n", _exactness_action_type_to_string_get(evt), timestamp);

   switch (action)
     {
      case EFL_POINTER_ACTION_MOVE:
          {
             double rad = 0, radx = 0, rady = 0, pres = 0, ang = 0, fx = 0, fy = 0;
             int tool = efl_input_pointer_touch_id_get(evp);
             Eina_Position2D pos = efl_input_pointer_position_get(evp);
             Exactness_Action_Multi_Move t = { tool, pos.x, pos.y, rad, radx, rady, pres, ang, fx, fy };
             if (n_evas >= 0) _add_to_list(evt, n_evas, timestamp, &t, sizeof(t));
             break;
          }
      case EFL_POINTER_ACTION_DOWN:
      case EFL_POINTER_ACTION_UP:
          {
             double rad = 0, radx = 0, rady = 0, pres = 0, ang = 0, fx = 0, fy = 0;
             int b = efl_input_pointer_button_get(evp);
             int tool = efl_input_pointer_touch_id_get(evp);
             Eina_Position2D pos = efl_input_pointer_position_get(evp);
             Efl_Pointer_Flags flags = efl_input_pointer_button_flags_get(evp);
             Exactness_Action_Multi_Event t = { tool, b, pos.x, pos.y, rad, radx, rady, pres, ang,
                  fx, fy, (Evas_Button_Flags)flags };
             if (n_evas >= 0) _add_to_list(evt, n_evas, timestamp, &t, sizeof(t));
             break;
          }
      case EFL_POINTER_ACTION_IN:
      case EFL_POINTER_ACTION_OUT:
          {
             if (n_evas >= 0) _add_to_list(evt, n_evas, timestamp, NULL, 0);
             break;
          }
      case EFL_POINTER_ACTION_WHEEL:
          {
             Eina_Bool horiz = efl_input_pointer_wheel_horizontal_get(evp);
             int z = efl_input_pointer_wheel_delta_get(evp);
             Exactness_Action_Mouse_Wheel t = { horiz, z };
             if (n_evas >= 0) _add_to_list(evt, n_evas, timestamp, &t, sizeof(t));
             break;
          }
      default:
        break;
     }
}

/**
 * @brief Callback for handling keyboard events.
 *
 * This function processes key down and key up events. It handles special
 * control keys (like F1 for stabilize, F2 for screenshot, F3 for save)
 * separately. For regular keys, it records a key up/down action along with
 * key details like name, symbol, and string.
 *
 * @param data The Evas canvas Eo object.
 * @param event The EFL event information.
 */
static void
_event_key_cb(void *data, const Efl_Event *event)
{
   Efl_Input_Key *evk = event->info;
   Eo *eo_e = data;
   if (!evk) return;
   const char *key = efl_input_key_name_get(evk);
   int timestamp = efl_input_timestamp_get(evk);
   unsigned int n_evas = _evas_id_get(eo_e);
   Exactness_Action_Type evt = EXACTNESS_ACTION_KEY_UP;

   if (efl_input_key_pressed_get(evk))
     {
        if (!strcmp(key, _shot_key))
          {
             DBG("Take Screenshot: %s timestamp=<%u>\n", __func__, timestamp);
             _add_to_list(EXACTNESS_ACTION_TAKE_SHOT, n_evas, timestamp, NULL, 0);
             return;
          }
        if (!strcmp(key, STABILIZE_KEY_STR))
          {
             DBG("Stabilize: %s timestamp=<%u>\n", __func__, timestamp);
             _add_to_list(EXACTNESS_ACTION_STABILIZE, n_evas, timestamp, NULL, 0);
             return;
          }
        if (!strcmp(key, SAVE_KEY_STR))
          {
             _output_write();
             DBG("Save events: %s timestamp=<%u>\n", __func__, timestamp);
             return;
          }
        evt = EXACTNESS_ACTION_KEY_DOWN;
     }
   else
     {
        if (!strcmp(key, _shot_key) || !strcmp(key, SAVE_KEY_STR) || !strcmp(key, STABILIZE_KEY_STR)) return;
     }
   if (_unit)
     {  /* Construct duplicate strings, free them when list if freed */
        Exactness_Action_Key_Down_Up t;
        t.keyname = eina_stringshare_add(key);
        t.key = eina_stringshare_add(efl_input_key_sym_get(evk));
        t.string = eina_stringshare_add(efl_input_key_string_get(evk));
        t.compose = eina_stringshare_add(efl_input_key_compose_string_get(evk));
        t.keycode = efl_input_key_code_get(evk);
        _add_to_list(evt, n_evas, timestamp, &t, sizeof(t));
     }
}

// note: "hold" event comes from above (elm), not below (ecore)
EFL_CALLBACKS_ARRAY_DEFINE(_event_pointer_callbacks,
      { EFL_EVENT_POINTER_MOVE, _event_pointer_cb },
      { EFL_EVENT_POINTER_DOWN, _event_pointer_cb },
      { EFL_EVENT_POINTER_UP, _event_pointer_cb },
      { EFL_EVENT_POINTER_IN, _event_pointer_cb },
      { EFL_EVENT_POINTER_OUT, _event_pointer_cb },
      { EFL_EVENT_POINTER_WHEEL, _event_pointer_cb },
      { EFL_EVENT_FINGER_MOVE, _event_pointer_cb },
      { EFL_EVENT_FINGER_DOWN, _event_pointer_cb },
      { EFL_EVENT_FINGER_UP, _event_pointer_cb },
      { EFL_EVENT_KEY_DOWN, _event_key_cb },
      { EFL_EVENT_KEY_UP, _event_key_cb }
      )

/**
 * @brief Factory function for creating new Evas canvases.
 *
 * This function is set as a callback via ecore_evas_callback_new_set() to
 * intercept all Evas canvas creations. It assigns a unique ID to each new
 * canvas and attaches the necessary event listeners for recording.
 *
 * @param w The width of the new canvas (unused).
 * @param h The height of the new canvas (unused).
 * @return A new Evas canvas object.
 */
static Evas *
_my_evas_new(int w EINA_UNUSED, int h EINA_UNUSED)
{
   Evas *e;
   e = evas_new();
   if (e)
     {
        INF("New Evas\n");
        _evas_list = eina_list_append(_evas_list, e);
        efl_key_data_set(e, "__evas_id", (void *)(intptr_t)_last_evas_id++);
        efl_event_callback_array_add(e, _event_pointer_callbacks(), e);
     }
   return e;
}

/**
 * @brief Initializes the main test unit data structure.
 *
 * This function allocates and initializes the global _unit structure, which
 * holds all the recorded actions and metadata for a test case. It ensures
 * this is only done once.
 */
static void
_setup_unit(void)
{
   if (_unit) return;

   _unit = calloc(1, sizeof(*_unit));
}

/**
 * @brief Sets up a consistent font environment for the test.
 *
 * To ensure reproducible rendering, this function configures the application
 * to use a specific set of fonts. It finds the most recently dated font
 * subdirectory within the provided `fonts_dir`, creates a temporary
 * fontconfig file pointing to it, and then sets the `FONTCONFIG_FILE`
 * environment variable to enforce its use.
 *
 * @param fonts_dir The path to the directory containing versioned font
 *                  subdirectories.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_setup_fonts_dir(const char *fonts_dir)
{
   if (fonts_dir)
     {
        Eina_Tmpstr *fonts_conf_name = NULL;
        if (!ecore_file_exists(fonts_dir))
          {
             fprintf(stderr, "Unable to find fonts directory %s\n", fonts_dir);
             return EINA_FALSE;
          }
        Eina_List *dated_fonts = ecore_file_ls(fonts_dir);
        char *date_dir;
        _unit->fonts_path = strdup(eina_list_last_data_get(dated_fonts));
        EINA_LIST_FREE(dated_fonts, date_dir) free(date_dir);
        if (_unit->fonts_path)
          {
             int tmp_fd = eina_file_mkstemp("/tmp/fonts_XXXXXX.conf", &fonts_conf_name);
             if (tmp_fd < 0) return EINA_FALSE;
             FILE *tmp_f = fdopen(tmp_fd, "wb");
             fprintf(tmp_f,
                   "<?xml version=\"1.0\"?>\n<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">\n<fontconfig>\n"
                   "<dir prefix=\"default\">%s/%s</dir>\n</fontconfig>\n",
                   fonts_dir, _unit->fonts_path);
             fclose(tmp_f);
             close(tmp_fd);

             setenv("FONTCONFIG_FILE", fonts_conf_name, 1);
          }
     }
   return EINA_TRUE;
}

/**
 * @brief Configures the key used to trigger screenshots.
 *
 * It reads the key name from the `SHOT_KEY` environment variable. If the
 * variable is not set, it falls back to a default value (F2).
 */
static void
_setup_shot_key(void)
{
   if (!_shot_key) _shot_key = getenv("SHOT_KEY");
   if (!_shot_key) _shot_key = SHOT_KEY_STR;
}

/**
 * @brief Sets up the interception of Ecore_Evas creation.
 *
 * This function registers _my_evas_new() as the factory for new Evas canvases
 * and initializes the base timestamp for event recording.
 */
static void
_setup_ee_creation(void)
{
   ecore_evas_callback_new_set(_my_evas_new);
   _last_timestamp = ecore_time_get() * 1000;
}

#ifdef HAVE_DLSYM
# ifdef FUNC_DOT_DOT_DOT
#  define FUNCARGS ...
# else
#  define FUNCARGS
# endif
# define ORIGINAL_CALL_T(t, name, ...) \
   t (*_original_init_cb)(FUNCARGS); \
   _original_init_cb = dlsym(RTLD_NEXT, name); \
   original_return = _original_init_cb(__VA_ARGS__);
#else
# define ORIGINAL_CALL_T(t, name, ...) \
   printf("THIS IS NOT SUPPORTED ON WINDOWS\n"); \
   abort();
#endif

#define ORIGINAL_CALL(name, ...) \
   ORIGINAL_CALL_T(int, name, __VA_ARGS__)

/**
 * @brief Intercepts eina_init() to set up the recorder.
 *
 * This is one of the primary entry points for the recorder library. After
 * calling the original eina_init(), it performs initial setup if this is the
 * main application process. This includes registering a log domain, reading
 * environment variables for configuration, and setting up the font
 * environment.
 *
 * @return The return value from the original eina_init().
 */
EAPI int
eina_init(void)
{
   int original_return;

   ORIGINAL_CALL("eina_init");

   ex_set_original_envvar();

   if (ex_is_original_app() && original_return == 1)
     {
        _log_domain = eina_log_domain_register("exactness_recorder", NULL);

        _out_filename = getenv("EXACTNESS_DEST");
        _setup_unit();
        if (!_setup_fonts_dir(getenv("EXACTNESS_FONTS_DIR")))
          return -1;

        _setup_shot_key();
     }

   return original_return;
}

/**
 * @brief Intercepts ecore_evas_init() to set up canvas creation hooks.
 *
 * After calling the original function, it sets up the mechanism to
 * intercept Evas canvas creation, allowing the recorder to attach event
 * listeners.
 *
 * @return The return value from the original ecore_evas_init().
 */
EAPI int
ecore_evas_init(void)
{
   int original_return;

   ORIGINAL_CALL("ecore_evas_init")

   if (ex_is_original_app() && original_return == 1)
     {
        _setup_ee_creation();

     }

   return original_return;
}

/**
 * @brief Intercepts elm_init() to prepare an overlay theme.
 *
 * This hook is used to apply a specific theme or overlay, likely to ensure
 * consistent widget appearance during tests.
 *
 * @param argc Argument count for the original elm_init.
 * @param argv Argument vector for the original elm_init.
 * @return The return value from the original elm_init().
 */
//hook, to hook in our theme
EAPI int
elm_init(int argc, char **argv)
{
   int original_return;
   ORIGINAL_CALL("elm_init", argc, argv)

   if (ex_is_original_app() && original_return == 1)
     ex_prepare_elm_overlay();

   return original_return;
}

/**
 * @brief Intercepts ecore_main_loop_begin() to write output upon exit.
 *
 * This function is hooked to detect the end of the application's main loop.
 * It triggers writing the recorded session to a file. This hook is for
 * older EFL applications. For newer ones, see efl_loop_begin().
 */
EAPI void
ecore_main_loop_begin(void)
{
   int original_return;
   ORIGINAL_CALL("ecore_main_loop_begin")
   if (ex_is_original_app())
     _output_write();
   (void)original_return;
}

/**
 * @brief Intercepts efl_loop_begin() to write output upon exit.
 *
 * This function is hooked to detect the end of the application's main loop
 * for modern, Eo-based EFL applications. It triggers writing the recorded
 * session to a file.
 *
 * @param obj The loop object.
 * @return The return value from the original efl_loop_begin().
 */
EAPI Eina_Value*
efl_loop_begin(Eo *obj)
{
   Eina_Value *original_return;
   ORIGINAL_CALL_T(Eina_Value*, "efl_loop_begin", obj);
   if (ex_is_original_app())
     _output_write();
   return original_return;
}

/**
 * @brief Intercepts eina_shutdown() to ensure output is written.
 *
 * This hook acts as a final safeguard to write the recorded data before the
 * application fully terminates. It includes a check to prevent writing the
 * output multiple times if other exit hooks have already done so.
 *
 * @return The return value from the original eina_shutdown().
 */
EAPI int
eina_shutdown(void)
{
   int original_return;
   static Eina_Bool output_written = EINA_FALSE;
   ORIGINAL_CALL("eina_shutdown")
   if (ex_is_original_app() && original_return == 1 && !output_written)
     {
        output_written = EINA_TRUE;
        _output_write();
     }

   return original_return;
}
