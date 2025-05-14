#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

#define TAP_NAME "tap"
#define DOUBLE_TAP_NAME "double_tap"
#define TRIPLE_TAP_NAME "triple_tap"
#define LONG_PRESS_NAME "long_press"
#define FLICK_NAME "flick"
#define LINE_NAME "line"
#define MOMENTUM_NAME "momentum"
#define ROTATE_NAME "rotate"
#define ZOOM_NAME "zoom"

#define N_GESTURE_TYPE 9
#define MAX_DOUBLE_TAP 5
#define MAX_FLICK 5
#define MAX_LINE 5
#define MAX_LONG_PRESS 5
#define MAX_MOMENTUM 5
#define MAX_ROTATE 1
#define MAX_TAP 5
#define MAX_TRIPLE_TAP 5
#define MAX_ZOOM 1

#define TB_PADDING_X 4
#define TB_PADDING_Y 12

#define BX_PADDING_X 0
#define BX_PADDING_Y 2

/* Define initial RGBA values for icons */
#define INI_R 60
#define INI_G 66
#define INI_B 64
#define INI_A 128
#define COLOR_STEP 4

#define START_COLOR 220, 220, 200, 255
#define UPDATE_COLOR 255, 255, 0, 255
#define ABORT_COLOR 255, 0, 0, 255
#define END_COLOR 0, 255, 0, 255

/**
 * @brief Holds properties for a gesture icon.
 *
 * This struct maintains the state of an icon that visually represents a gesture,
 * including its Evas object, current color, and associated gesture name.
 */
struct _icon_properties
{
   Evas_Object *icon; /**< The Evas icon object. */
   int r; /**< Current red color component. */
   int g; /**< Current green color component. */
   int b; /**< Current blue color component. */
   int a; /**< Current alpha component. */

   const char *name; /**< The name of the gesture (e.g., "tap"). */
};
typedef struct _icon_properties icon_properties;

/**
 * @brief Application context data.
 *
 * This struct holds data passed between callbacks, serving as a replacement
 * for global variables. It contains all the necessary state for the
 * gesture test application.
 */
struct _infra_data
{  /* Some data that is passed aroung between callbacks (replacing globals) */
   /**
    * @brief Array of icon properties for each gesture type.
    * Example:
    * @code
    * icons[0] = { .name = "tap", .icon = tap_icon_obj, ... };
    * icons[1] = { .name = "double_tap", .icon = double_tap_icon_obj, ... };
    * @endcode
    */
   icon_properties *icons;
   Ecore_Timer *colortimer; /**< Timer for animating icon colors back to their initial state. */
   char buf[1024]; /**< General purpose buffer, primarily for building image file paths. */
   int long_press_count; /**< Counter for long press gestures (currently unused). */
};
typedef struct _infra_data infra_data;

/**
 * @brief Frees the application context data.
 * @param infra The application context data to free.
 */
static void
_infra_data_free(infra_data *infra)
{
   if (infra)
     {
        if (infra->colortimer)
          ecore_timer_del(infra->colortimer);

        if (infra->icons)
          free(infra->icons);

        free (infra);
     }
}

/**
 * @brief Allocates and initializes the application context data.
 * @return A new instance of infra_data, or NULL on failure.
 */
static infra_data *
_infra_data_alloc(void)
{
   infra_data *infra = malloc(sizeof(infra_data));
   if (!infra) return NULL;

   infra->icons = calloc(N_GESTURE_TYPE, sizeof(icon_properties ));
   infra->colortimer = NULL;

   return infra;
}

/**
 * @brief Callback function for the window's "delete,request" event.
 *
 * This function is called when the main window is requested to be deleted.
 * It ensures that the application data is freed.
 * @param data The application context data (infra_data).
 * @param obj The window object.
 * @param event_info Event-specific information (unused).
 */
static void
my_win_del(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{  /* called when my_win_main is requested to be deleted */
   _infra_data_free(data);
}

/**
 * @brief Finds an icon_properties struct by its gesture name.
 * @param icons An array of icon_properties.
 * @param name The gesture name to search for (e.g., "tap").
 * @return A pointer to the matching icon_properties struct, or NULL if not found.
 */
static icon_properties *
_icon_properties_find(icon_properties *icons, char *name)
{
   int n;

   for (n = 0; n < N_GESTURE_TYPE; n++)
     if (!strcmp(icons[n].name, name))
       return &icons[n];

   return NULL;
}

/**
 * @brief Sets the color of a gesture icon.
 * @param i The icon properties struct to modify.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 */
static void
_icon_color_set(icon_properties *i, int r, int g, int b, int a)
{
   i->r =  r;
   i->g =  g;
   i->b =  b;
   i->a =  a;
   evas_object_color_set(i->icon, i->r,  i->g,  i->b,  i->a);
}

/**
 * @brief Timer callback to animate icon colors back to their initial state.
 *
 * This function is called repeatedly by an Ecore_Timer. It gradually
 * changes the color of each icon back to its initial RGBA values (INI_R,
 * INI_G, INI_B, INI_A). This creates a fade-out effect for the gesture
 * state colors (START, UPDATE, END, ABORT).
 *
 * @param data A pointer to the array of icon_properties.
 * @return ECORE_CALLBACK_RENEW to continue the timer, or ECORE_CALLBACK_CANCEL to stop.
 */
static Eina_Bool
_icon_color_set_cb(void *data)
{
#define INC_COLOR(C, NC) \
   do { \
        if (abs(i->C - NC) < COLOR_STEP) \
        i->C = NC; \
        else \
        i->C += ((i->C < NC) ? COLOR_STEP : (-COLOR_STEP)); \
   } while (0)

   int n;
   icon_properties *icons = data;
   icon_properties *i;

   for (n = 0; n < N_GESTURE_TYPE; n++)
     {
        i = &icons[n];

        INC_COLOR(r,INI_R);
        INC_COLOR(g,INI_G);
        INC_COLOR(b,INI_B);
        INC_COLOR(a,INI_A);

        /* Change Icon color */
        evas_object_color_set(i->icon, i->r,  i->g,  i->b,  i->a);
    }

   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Sets the image and color for a specific gesture icon.
 *
 * This function finds a gesture icon by its name and updates its visual state.
 * It sets the icon's image based on a sequence number and applies a specific color
 * to indicate the gesture state (e.g., start, end).
 *
 * @param infra The application context data.
 * @param name The name of the gesture to update (e.g., "tap").
 * @param n The sequence number for the icon image (e.g., 1 for tap_1.png).
 * @param max The maximum sequence number for the icon image.
 * @param r Red color component.
 * @param g Green color component.
 * @param b Blue color component.
 * @param a Alpha color component.
 */
static void
_color_and_icon_set(infra_data *infra, char *name, int n, int max,
      int r, int g, int b, int a)
{
   icon_properties *i;
   int nn = n;
   i = _icon_properties_find(infra->icons, name);
   if (i)
     {
        if (n < 1)
          nn = 1;

        if (n > max)
          nn = max;

        snprintf(infra->buf, sizeof(infra->buf),
              "%s/images/g_layer/%s_%d.png", elm_app_data_dir_get(), i->name, nn);
        elm_image_file_set(i->icon, infra->buf, NULL);
        _icon_color_set(i, r, g, b, a);
     }
}

/* START - Callbacks for gestures */
/**
 * @brief Handles the start of a tap gesture.
 *
 * Called when a tap gesture is recognized. It updates the tap icon to show
 * the "start" state and prints gesture details to stdout.
 *
 * @param data The application context data (infra_data).
 * @param tap The gesture object containing event details.
 */
static void
finger_tap_start(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);

   _color_and_icon_set(data, TAP_NAME, 1, MAX_TAP, START_COLOR);
   printf("Tap Gesture started x,y=<%d,%d> \n", pos.x, pos.y);
}

/**
 * @brief Handles the end of a tap gesture.
 *
 * Called when a tap gesture successfully completes. It updates the tap icon
 * to show the "end" state and prints gesture details to stdout.
 *
 * @param data The application context data (infra_data).
 * @param tap The gesture object containing event details.
 */
static void
finger_tap_end(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);

   _color_and_icon_set(data, TAP_NAME, 1, MAX_TAP, END_COLOR);
   printf("Tap Gesture ended x,y=<%d,%d> \n", pos.x, pos.y);
}

/**
 * @brief Handles the abortion of a tap gesture.
 *
 * Called when a tap gesture is canceled. It updates the tap icon to show
 * the "abort" state.
 *
 * @param data The application context data (infra_data).
 * @param tap The gesture object (unused).
 */
static void
finger_tap_abort(void *data , Efl_Canvas_Gesture *tap EINA_UNUSED)
{
   _color_and_icon_set(data, TAP_NAME, 1, MAX_TAP, ABORT_COLOR);
   printf("Tap Aborted\n");
}

static void
finger_flick_start(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);

   _color_and_icon_set(data, FLICK_NAME, 1, MAX_TAP, START_COLOR);
   printf("Flick Gesture started x,y=<%d,%d> \n", pos.x, pos.y);
}

static void
finger_flick_end(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);
   double angle = efl_gesture_flick_angle_get(tap);

   _color_and_icon_set(data, FLICK_NAME, 1, MAX_TAP, END_COLOR);
   printf("Flick Gesture ended x,y=<%d,%d> angle=<%f>\n", pos.x, pos.y, angle);
}

static void
finger_flick_abort(void *data , Efl_Canvas_Gesture *tap EINA_UNUSED)
{
   _color_and_icon_set(data, FLICK_NAME, 1, MAX_TAP, ABORT_COLOR);
   printf("Flick Aborted\n");
}

static void
finger_rotate_start(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);

   _color_and_icon_set(data, ROTATE_NAME, 1, MAX_TAP, START_COLOR);
   printf("Rotate Gesture started x,y=<%d,%d> \n", pos.x, pos.y);
}

static void
finger_rotate_end(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);
   double angle = efl_gesture_rotate_angle_get(tap);
   double radius = efl_gesture_rotate_radius_get(tap);

   _color_and_icon_set(data, ROTATE_NAME, 1, MAX_TAP, END_COLOR);
   printf("Rotate Gesture ended x,y=<%d,%d> angle=<%g> radius=<%f>\n", pos.x, pos.y, angle, radius);
}

static void
finger_rotate_abort(void *data , Efl_Canvas_Gesture *tap EINA_UNUSED)
{
   _color_and_icon_set(data, ROTATE_NAME, 1, MAX_TAP, ABORT_COLOR);
   printf("Rotate Aborted\n");
}

static void
finger_zoom_start(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);

   _color_and_icon_set(data, ZOOM_NAME, 1, MAX_TAP, START_COLOR);
   printf("Zoom Gesture started x,y=<%d,%d> \n", pos.x, pos.y);
}

static void
finger_zoom_end(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);
   double zoom = efl_gesture_zoom_get(tap);
   double radius = efl_gesture_zoom_radius_get(tap);

   _color_and_icon_set(data, ZOOM_NAME, 1, MAX_TAP, END_COLOR);
   printf("Zoom Gesture ended x,y=<%d,%d> zoom=<%g> radius=<%f>\n", pos.x, pos.y, zoom, radius);
}

static void
finger_zoom_abort(void *data , Efl_Canvas_Gesture *tap EINA_UNUSED)
{
   _color_and_icon_set(data, ZOOM_NAME, 1, MAX_TAP, ABORT_COLOR);
   printf("Zoom Aborted\n");
}

static void
finger_momentum_start(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);
   unsigned int t = efl_gesture_timestamp_get(tap);

   _color_and_icon_set(data, MOMENTUM_NAME, 1, MAX_TAP, START_COLOR);
   printf("Momentum Gesture started x,y=<%d,%d> time=<%d>\n", pos.x, pos.y, t);
}

/**
 * @brief Handles the update of a momentum gesture.
 *
 * Called during a momentum gesture. It updates the momentum icon to show
 * the "update" state and prints gesture details to stdout.
 *
 * @param data The application context data (infra_data).
 * @param tap The gesture object containing event details.
 */
static void
finger_momentum_update(void *data , Efl_Canvas_Gesture *tap EINA_UNUSED)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);
   Eina_Vector2 m = efl_gesture_momentum_get(tap);
   unsigned int t = efl_gesture_timestamp_get(tap);

   _color_and_icon_set(data, MOMENTUM_NAME, 1, MAX_TAP, UPDATE_COLOR);
   printf("Momentum Gesture updated x,y=<%d,%d> momentum=<%f %f> time=<%d>\n",
          pos.x, pos.y, m.x, m.y, t);
}

static void
finger_momentum_end(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);
   Eina_Vector2 m = efl_gesture_momentum_get(tap);
   unsigned int t = efl_gesture_timestamp_get(tap);

   _color_and_icon_set(data, MOMENTUM_NAME, 1, MAX_TAP, END_COLOR);
   printf("Momentum Gesture ended x,y=<%d,%d> momentum=<%f %f> time=<%d>\n",
     pos.x, pos.y, m.x, m.y, t);
}

static void
finger_momentum_abort(void *data , Efl_Canvas_Gesture *tap EINA_UNUSED)
{
   _color_and_icon_set(data, MOMENTUM_NAME, 1, MAX_TAP, ABORT_COLOR);
   printf("Momentum Aborted\n");
}

static void
finger_triple_tap_start(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);

   _color_and_icon_set(data, TRIPLE_TAP_NAME, 1, MAX_TAP, START_COLOR);
   printf("Triple Tap Gesture started x,y=<%d,%d> \n", pos.x, pos.y);
}

static void
finger_triple_tap_update(void *data , Efl_Canvas_Gesture *tap EINA_UNUSED)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);

   _color_and_icon_set(data, TRIPLE_TAP_NAME, 1, MAX_TAP, UPDATE_COLOR);
   printf("Triple Tap Gesture updated x,y=<%d,%d> \n", pos.x, pos.y);
}

static void
finger_triple_tap_end(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);

   _color_and_icon_set(data, TRIPLE_TAP_NAME, 1, MAX_TAP, END_COLOR);
   printf("Triple Tap Gesture ended x,y=<%d,%d> \n", pos.x, pos.y);
}

static void
finger_triple_tap_abort(void *data , Efl_Canvas_Gesture *tap EINA_UNUSED)
{
   _color_and_icon_set(data, TRIPLE_TAP_NAME, 1, MAX_TAP, ABORT_COLOR);
   printf("Triple Tap Aborted\n");
}

static void
finger_double_tap_start(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);

   _color_and_icon_set(data, DOUBLE_TAP_NAME, 1, MAX_TAP, START_COLOR);
   printf("Double Tap Gesture started x,y=<%d,%d> \n", pos.x, pos.y);
}

static void
finger_double_tap_update(void *data , Efl_Canvas_Gesture *tap EINA_UNUSED)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);

   _color_and_icon_set(data, DOUBLE_TAP_NAME, 1, MAX_TAP, UPDATE_COLOR);
   printf("Double Tap Gesture updated x,y=<%d,%d> \n", pos.x, pos.y);
}

static void
finger_double_tap_end(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);

   _color_and_icon_set(data, DOUBLE_TAP_NAME, 1, MAX_TAP, END_COLOR);
   printf("Double Tap Gesture ended x,y=<%d,%d> \n", pos.x, pos.y);
}

static void
finger_double_tap_abort(void *data , Efl_Canvas_Gesture *tap EINA_UNUSED)
{
   _color_and_icon_set(data, DOUBLE_TAP_NAME, 1, MAX_TAP, ABORT_COLOR);
   printf("Double Tap Aborted\n");
}

static void
finger_long_press_start(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);

   _color_and_icon_set(data, LONG_PRESS_NAME, 1, MAX_TAP, START_COLOR);
   printf("Long Tap Gesture started x,y=<%d,%d> \n", pos.x, pos.y);
}

static void
finger_long_press_update(void *data , Efl_Canvas_Gesture *tap EINA_UNUSED)
{
   _color_and_icon_set(data, LONG_PRESS_NAME, 1, MAX_TAP, UPDATE_COLOR);
   printf("Long Tap Gesture updated\n");
}

static void
finger_long_press_end(void *data , Efl_Canvas_Gesture *tap)
{
   Eina_Position2D pos = efl_gesture_hotspot_get(tap);

   _color_and_icon_set(data, LONG_PRESS_NAME, 1, MAX_TAP, END_COLOR);
   printf("Long Tap Gesture ended x,y=<%d,%d> \n",pos.x, pos.y);
}

static void
finger_long_press_abort(void *data , Efl_Canvas_Gesture *tap EINA_UNUSED)
{
   _color_and_icon_set(data, LONG_PRESS_NAME, 1, MAX_TAP, ABORT_COLOR);
   printf("Long Tap Aborted\n");
}

/**
 * @brief Dispatches tap gesture events to the appropriate handler.
 *
 * This callback is triggered for all state changes of a tap gesture.
 * It uses a switch statement on the gesture state to call the corresponding
 * function (e.g., finger_tap_start, finger_tap_end).
 *
 * @param data The application context data (infra_data).
 * @param ev The EFL event object, where ev->info is the Efl_Canvas_Gesture.
 */
static void
tap_gesture_cb(void *data , const Efl_Event *ev)
{
   Efl_Canvas_Gesture *g = ev->info;
   switch(efl_gesture_state_get(g))
   {
      case EFL_GESTURE_STATE_STARTED:
         finger_tap_start(data, g);
         break;
      case EFL_GESTURE_STATE_CANCELED:
         finger_tap_abort(data, g);
         break;
      case EFL_GESTURE_STATE_FINISHED:
         finger_tap_end(data, g);
         break;
      default:
         break;
   }
}

static void
flick_gesture_cb(void *data , const Efl_Event *ev)
{
   Efl_Canvas_Gesture *g = ev->info;
   switch(efl_gesture_state_get(g))
   {
      case EFL_GESTURE_STATE_STARTED:
         finger_flick_start(data, g);
         break;
      case EFL_GESTURE_STATE_CANCELED:
         finger_flick_abort(data, g);
         break;
      case EFL_GESTURE_STATE_FINISHED:
         finger_flick_end(data, g);
         break;
      default:
         break;
   }
}

static void
rotate_gesture_cb(void *data , const Efl_Event *ev)
{
   Efl_Canvas_Gesture *g = ev->info;
   switch(efl_gesture_state_get(g))
   {
      case EFL_GESTURE_STATE_STARTED:
         finger_rotate_start(data, g);
         break;
      case EFL_GESTURE_STATE_CANCELED:
         finger_rotate_abort(data, g);
         break;
      case EFL_GESTURE_STATE_FINISHED:
         finger_rotate_end(data, g);
         break;
      default:
         break;
   }
}

static void
zoom_gesture_cb(void *data , const Efl_Event *ev)
{
   Efl_Canvas_Gesture *g = ev->info;
   switch(efl_gesture_state_get(g))
   {
      case EFL_GESTURE_STATE_STARTED:
         finger_zoom_start(data, g);
         break;
      case EFL_GESTURE_STATE_CANCELED:
         finger_zoom_abort(data, g);
         break;
      case EFL_GESTURE_STATE_FINISHED:
         finger_zoom_end(data, g);
         break;
      default:
         break;
   }
}

/**
 * @brief Dispatches momentum gesture events to the appropriate handler.
 *
 * This callback is triggered for all state changes of a momentum gesture.
 * It uses a switch statement on the gesture state to call the corresponding
 * function (e.g., finger_momentum_start, finger_momentum_update).
 *
 * @param data The application context data (infra_data).
 * @param ev The EFL event object, where ev->info is the Efl_Canvas_Gesture.
 */
static void
momentum_gesture_cb(void *data , const Efl_Event *ev)
{
   Efl_Canvas_Gesture *g = ev->info;
   switch(efl_gesture_state_get(g))
   {
      case EFL_GESTURE_STATE_STARTED:
         finger_momentum_start(data, g);
         break;
      case EFL_GESTURE_STATE_UPDATED:
         finger_momentum_update(data, g);
         break;
      case EFL_GESTURE_STATE_CANCELED:
         finger_momentum_abort(data, g);
         break;
      case EFL_GESTURE_STATE_FINISHED:
         finger_momentum_end(data, g);
         break;
      default:
         break;
   }
}

static void
triple_tap_gesture_cb(void *data , const Efl_Event *ev)
{
   Efl_Canvas_Gesture *g = ev->info;
   switch(efl_gesture_state_get(g))
   {
      case EFL_GESTURE_STATE_STARTED:
         finger_triple_tap_start(data, g);
         break;
      case EFL_GESTURE_STATE_UPDATED:
         finger_triple_tap_update(data, g);
         break;
      case EFL_GESTURE_STATE_CANCELED:
         finger_triple_tap_abort(data, g);
         break;
      case EFL_GESTURE_STATE_FINISHED:
         finger_triple_tap_end(data, g);
         break;
      default:
         break;
   }
}

static void
double_tap_gesture_cb(void *data , const Efl_Event *ev)
{
   Efl_Canvas_Gesture *g = ev->info;
   switch(efl_gesture_state_get(g))
   {
      case EFL_GESTURE_STATE_STARTED:
         finger_double_tap_start(data, g);
         break;
      case EFL_GESTURE_STATE_UPDATED:
         finger_double_tap_update(data, g);
         break;
      case EFL_GESTURE_STATE_CANCELED:
         finger_double_tap_abort(data, g);
         break;
      case EFL_GESTURE_STATE_FINISHED:
         finger_double_tap_end(data, g);
         break;
      default:
         break;
   }
}

static void
long_press_gesture_cb(void *data , const Efl_Event *ev)
{
   Efl_Canvas_Gesture *g = ev->info;
   switch(efl_gesture_state_get(g))
   {
      case EFL_GESTURE_STATE_STARTED:
         finger_long_press_start(data, g);
         break;
      case EFL_GESTURE_STATE_UPDATED:
         finger_long_press_update(data, g);
         break;
      case EFL_GESTURE_STATE_CANCELED:
         finger_long_press_abort(data, g);
         break;
      case EFL_GESTURE_STATE_FINISHED:
         finger_long_press_end(data, g);
         break;
      default:
         break;
   }
}

/* END   - Callbacks for gestures */

/**
 * @brief Creates a UI box containing a gesture icon and a label.
 *
 * This helper function constructs a vertical box with an icon representing a
 * gesture and a label with its name. It initializes the icon's properties
 * within the provided `icons` array.
 *
 * @param win The parent window.
 * @param icons The array of icon_properties where the new icon's data will be stored.
 * @param idx The index in the `icons` array to use.
 * @param name The name of the gesture (e.g., "tap"), used for finding image files.
 * @param lb_txt The text to display in the label below the icon.
 * @return The newly created Evas_Object (an elm_box).
 */
static Evas_Object *
create_gesture_box(Evas_Object *win, icon_properties *icons,
                   int idx, const char *name, const char *lb_txt)
{  /* Creates a box with icon and label, later placed in a table */
   Evas_Object *lb, *bx = elm_box_add(win);
   char buf[1024];

   elm_box_padding_set(bx, BX_PADDING_X, BX_PADDING_Y);
   icons[idx].icon = elm_icon_add(win);
   icons[idx].name = name;
   snprintf(buf, sizeof(buf), "%s/images/g_layer/%s_1.png",
         elm_app_data_dir_get(), icons[idx].name);
   elm_image_file_set(icons[idx].icon, buf, NULL);
   elm_image_resizable_set(icons[idx].icon, EINA_FALSE, EINA_FALSE);
   evas_object_size_hint_align_set(icons[idx].icon, 0.5, 0.5);
   _icon_color_set(&icons[idx], INI_R, INI_G, INI_B, INI_A);
   elm_box_pack_end(bx, icons[idx].icon);
   evas_object_show(icons[idx].icon);

   lb = elm_label_add(win);
   elm_object_text_set(lb, lb_txt);
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(lb, 0.5, 0.5);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);
   evas_object_show(bx);

   return bx;
}

/**
 * @brief Callback for window resize events.
 *
 * This function is called when the main window is resized. It resizes the
 * transparent gesture target object to match the new window dimensions.
 *
 * @param data The target Evas_Object to resize.
 * @param e The Evas canvas (unused).
 * @param obj The object that triggered the event (the window).
 * @param event_info Event-specific information (unused).
 */
void
_tb_resize(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   int w,h;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   evas_object_resize(data, w, h);
   evas_object_color_set(data, 0, 0, 0, 0);
   evas_object_show(data);
}

/**
 * @brief Main function for the gesture framework test.
 *
 * This function sets up the entire UI for the gesture test application. It:
 * - Creates the main window.
 * - Allocates the application data structure.
 * - Builds a table of gesture icons and a legend for gesture states.
 * - Creates a transparent rectangle (`target`) overlaid on the UI to capture
 *   all gesture events.
 * - Registers callbacks for all supported gesture types on the `target` object.
 * - Starts a timer to animate icon colors.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_gesture_framework(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
      void *event_info EINA_UNUSED)
{
   Evas_Object *win, *tb, *lb, *bx;
   Evas_Object *r, *target; /* Gesture layer transparent object */

   infra_data *infra = _infra_data_alloc();

   win = elm_win_util_standard_add("gesture-layer2", "Gesture (EO)");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", my_win_del, infra);

   /* START - Building icons table */
   bx = elm_box_add(win);
   tb = elm_table_add(win);
   elm_box_pack_end(bx, tb);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tb, 0.5, 0.5);
   elm_win_resize_object_add(win, bx);
   evas_object_show(tb);
   evas_object_show(bx);

   target = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_event_callback_add(win, EVAS_CALLBACK_RESIZE, _tb_resize, target);

   /* Box of Tap icon and label */
   bx = create_gesture_box(win, infra->icons, 0, TAP_NAME, "Tap");
   elm_table_pack(tb, bx, 0, 0, 1, 1);

   /* Box of Double Tap icon and label */
   bx = create_gesture_box(win, infra->icons, 1, DOUBLE_TAP_NAME, "Double Tap");
   elm_table_pack(tb, bx, 1, 0, 1, 1);

   /* Box of Triple Tap icon and label */
   bx = create_gesture_box(win, infra->icons, 2, TRIPLE_TAP_NAME, "Triple Tap");
   elm_table_pack(tb, bx, 2, 0, 1, 1);

   /* Box of Long Tap icon and label */
   bx = create_gesture_box(win, infra->icons, 3, LONG_PRESS_NAME, "Long Tap");
   elm_table_pack(tb, bx, 3, 0, 1, 1);

   /* Box of Momentum icon and label */
   bx = create_gesture_box(win, infra->icons, 4, MOMENTUM_NAME, "Momentum");
   elm_table_pack(tb, bx, 0, 2, 1, 1);

   /* Box of Line icon and label */
   bx = create_gesture_box(win, infra->icons, 5, LINE_NAME, "Line");
   elm_table_pack(tb, bx, 1, 2, 1, 1);

   /* Box of Flick icon and label */
   bx = create_gesture_box(win, infra->icons, 6, FLICK_NAME, "Flick");
   elm_table_pack(tb, bx, 2, 2, 1, 1);

   /* Box of Zoom icon and label */
   bx = create_gesture_box(win, infra->icons, 7, ZOOM_NAME, "Zoom");
   elm_table_pack(tb, bx, 0, 3, 1, 1);

   /* Box of Rotate icon and label */
   bx = create_gesture_box(win, infra->icons, 8, ROTATE_NAME, "Rotate");
   elm_table_pack(tb, bx, 1, 3, 1, 1);

   /* Legend of gestures - states */
   lb = elm_label_add(win);
   elm_object_text_set(lb, "<b>Gesture States</b>");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, lb, 0, 6, 4, 2);
   evas_object_show(lb);

   r = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_resize(r, 64, 64);
   evas_object_color_set(r, INI_R, INI_G, INI_B, INI_A);
   evas_object_size_hint_weight_set(r, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(r, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, r, 0, 7, 1, 1);
   evas_object_show(r);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Not Started");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, lb, 1, 7, 3, 1);
   evas_object_show(lb);

   r = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_resize(r, 64, 64);
   evas_object_color_set(r, START_COLOR);
   evas_object_size_hint_weight_set(r, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(r, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, r, 0, 8, 1, 1);
   evas_object_show(r);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Gesture START");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, lb, 1, 8, 3, 1);
   evas_object_show(lb);

   r = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_resize(r, 64, 64);
   evas_object_color_set(r, UPDATE_COLOR);
   evas_object_size_hint_weight_set(r, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(r, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, r, 0, 9, 1, 1);
   evas_object_show(r);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Gesture UPDATE");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, lb, 1, 9, 3, 1);
   evas_object_show(lb);

   r = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_resize(r, 64, 64);
   evas_object_color_set(r, END_COLOR);
   evas_object_size_hint_weight_set(r, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(r, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, r, 0, 10, 1, 1);
   evas_object_show(r);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Gesture END");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, lb, 1, 10, 3, 1);
   evas_object_show(lb);

   r = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_resize(r, 64, 64);
   evas_object_color_set(r, ABORT_COLOR);
   evas_object_size_hint_weight_set(r, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(r, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, r, 0, 11, 1, 1);
   evas_object_show(r);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Gesture ABORT");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, lb, 1, 11, 3, 1);

   elm_table_padding_set(tb, TB_PADDING_X, TB_PADDING_Y);
   evas_object_show(lb);
   /* END   - Building icons table */

   // LISTEN FOR GESTURES
   efl_event_callback_add(target, EFL_EVENT_GESTURE_TAP, tap_gesture_cb, infra);
   efl_event_callback_add(target, EFL_EVENT_GESTURE_LONG_PRESS, long_press_gesture_cb, infra);
   efl_event_callback_add(target, EFL_EVENT_GESTURE_DOUBLE_TAP, double_tap_gesture_cb, infra);
   efl_event_callback_add(target, EFL_EVENT_GESTURE_TRIPLE_TAP, triple_tap_gesture_cb, infra);
   efl_event_callback_add(target, EFL_EVENT_GESTURE_MOMENTUM, momentum_gesture_cb, infra);
   efl_event_callback_add(target, EFL_EVENT_GESTURE_FLICK, flick_gesture_cb, infra);
   efl_event_callback_add(target, EFL_EVENT_GESTURE_ROTATE, rotate_gesture_cb, infra);
   efl_event_callback_add(target, EFL_EVENT_GESTURE_ZOOM, zoom_gesture_cb, infra);

   /* Update color state 20 times a second */
   infra->colortimer = ecore_timer_add(0.05, _icon_color_set_cb, infra->icons);

   evas_object_show(win);
}
