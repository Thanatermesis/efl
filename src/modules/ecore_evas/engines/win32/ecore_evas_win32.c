
/**
 * @file
 * @brief Ecore_Evas module for Windows (Win32) platform.
 *
 * This module provides the Ecore_Evas integration for the Win32 API,
 * allowing Evas canvases to be displayed and managed as native Windows.
 * It supports various rendering engines like Software GDI, Software DDraw,
 * and OpenGL.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h> /* for NULL */
#include <inttypes.h> /* for UINT_MAX */

#include <Ecore.h>
#include "ecore_private.h"
#include <Ecore_Input.h>
#include <Ecore_Input_Evas.h>
#include <Ecore_Win32.h>
#include "ecore_win32_private.h"

#include "Ecore_Evas.h"
#include "ecore_evas_private.h"
#include "ecore_evas_win32.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#ifdef BUILD_ECORE_EVAS_SOFTWARE_GDI
# include <Evas_Engine_Software_Gdi.h>
#endif
#ifdef BUILD_ECORE_EVAS_SOFTWARE_DDRAW
# include <Evas_Engine_Software_DDraw.h>
#endif
#ifdef BUILD_ECORE_EVAS_OPENGL_WIN32
# include <Evas_Engine_GL_Win32.h>
#endif

#ifdef _WIN32
# ifndef EFL_MODULE_STATIC
#  define EMODAPI __declspec(dllexport)
# else
#  define EMODAPI
# endif
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EMODAPI __attribute__ ((visibility("default")))
#  endif
# endif
#endif /* ! _WIN32 */

#ifndef EMODAPI
# define EMODAPI
#endif

#ifdef BUILD_ECORE_EVAS_WIN32

#define ECORE_EVAS_EVENT_COUNT 11

#define EE_SZ(sz_) (ee->sz_ == 0) ? 1 : (ee->sz_)

static int _ecore_evas_init_count = 0;

static Ecore_Event_Handler *ecore_evas_event_handlers[ECORE_EVAS_EVENT_COUNT];
static const char *interface_win32_name = "win32";
static const int   interface_win32_version = 1;

typedef struct _Ecore_Evas_Engine_Data_Win32 Ecore_Evas_Engine_Data_Win32;

/**
 * @brief Structure holding engine-specific data for Win32 Ecore_Evas.
 */
struct _Ecore_Evas_Engine_Data_Win32
{
   Ecore_Win32_Window *parent; /**< Parent window, if any. */
   Ecore_Evas_Selection_Callbacks clipboard; /**< Callbacks for clipboard operations. */
   Eina_Future *delivery; /**< Future for asynchronous clipboard data delivery. */
   struct
   {
      unsigned char region     : 1; /**< Flag indicating if the window has a custom shape (region). */
      unsigned char fullscreen : 1; /**< Flag indicating if the window is in fullscreen mode. */
      unsigned char maximized  : 1; /**< Flag indicating if the window is maximized. */
   } state; /**< Current state flags of the window. */
};

static Ecore_Evas_Interface_Win32 *_ecore_evas_win32_interface_new(void);

/**
 * @brief Event handler for mouse enter events.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Event specific data (Ecore_Win32_Event_Mouse_In).
 * @return ECORE_CALLBACK_PASS_ON always.
 */
static Eina_Bool _ecore_evas_win32_event_mouse_in(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);

/**
 * @brief Event handler for mouse leave events.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Event specific data (Ecore_Win32_Event_Mouse_Out).
 * @return ECORE_CALLBACK_PASS_ON always.
 */
static Eina_Bool _ecore_evas_win32_event_mouse_out(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);

/**
 * @brief Event handler for window focus in events.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Event specific data (Ecore_Win32_Event_Window_Focus_In).
 * @return ECORE_CALLBACK_PASS_ON always.
 */
static Eina_Bool _ecore_evas_win32_event_window_focus_in(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);

/**
 * @brief Event handler for window focus out events.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Event specific data (Ecore_Win32_Event_Window_Focus_Out).
 * @return ECORE_CALLBACK_PASS_ON always.
 */
static Eina_Bool _ecore_evas_win32_event_window_focus_out(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);

/**
 * @brief Event handler for window damage events.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Event specific data (Ecore_Win32_Event_Window_Damage).
 * @return ECORE_CALLBACK_PASS_ON always.
 */
static Eina_Bool _ecore_evas_win32_event_window_damage(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);

/**
 * @brief Event handler for window destroy events.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Event specific data (Ecore_Win32_Event_Window_Destroy).
 * @return ECORE_CALLBACK_PASS_ON always.
 */
static Eina_Bool _ecore_evas_win32_event_window_destroy(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);

/**
 * @brief Event handler for window show events.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Event specific data (Ecore_Win32_Event_Window_Show).
 * @return ECORE_CALLBACK_PASS_ON always.
 */
static Eina_Bool _ecore_evas_win32_event_window_show(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);

/**
 * @brief Event handler for window hide events.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Event specific data (Ecore_Win32_Event_Window_Hide).
 * @return ECORE_CALLBACK_PASS_ON always.
 */
static Eina_Bool _ecore_evas_win32_event_window_hide(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);

/**
 * @brief Event handler for window configure (resize/move) events.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Event specific data (Ecore_Win32_Event_Window_Configure).
 * @return ECORE_CALLBACK_PASS_ON always.
 */
static Eina_Bool _ecore_evas_win32_event_window_configure(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);

/**
 * @brief Event handler for window delete request events (e.g., user clicks close button).
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Event specific data (Ecore_Win32_Event_Window_Delete_Request).
 * @return ECORE_CALLBACK_PASS_ON always.
 */
static Eina_Bool _ecore_evas_win32_event_window_delete_request(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);

/**
 * @brief Event handler for window property change events.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Event specific data (Ecore_Win32_Event_Window_Property).
 * @return ECORE_CALLBACK_PASS_ON always.
 */
static Eina_Bool _ecore_evas_win32_event_window_property_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);


/* Private functions */

/**
 * @brief Initializes the Ecore_Evas Win32 module.
 *
 * Sets up event handlers for various Win32 window events.
 * This function maintains an initialization counter to support multiple
 * init/shutdown calls.
 *
 * @return The current initialization count.
 */
static int
_ecore_evas_win32_init(void)
{
   _ecore_evas_init_count++;
   if (_ecore_evas_init_count > 1)
     return _ecore_evas_init_count;

   ecore_evas_event_handlers[0]  = ecore_event_handler_add(ECORE_WIN32_EVENT_MOUSE_IN, _ecore_evas_win32_event_mouse_in, NULL);
   ecore_evas_event_handlers[1]  = ecore_event_handler_add(ECORE_WIN32_EVENT_MOUSE_OUT, _ecore_evas_win32_event_mouse_out, NULL);
   ecore_evas_event_handlers[2]  = ecore_event_handler_add(ECORE_WIN32_EVENT_WINDOW_FOCUS_IN, _ecore_evas_win32_event_window_focus_in, NULL);
   ecore_evas_event_handlers[3]  = ecore_event_handler_add(ECORE_WIN32_EVENT_WINDOW_FOCUS_OUT, _ecore_evas_win32_event_window_focus_out, NULL);
   ecore_evas_event_handlers[4]  = ecore_event_handler_add(ECORE_WIN32_EVENT_WINDOW_DAMAGE, _ecore_evas_win32_event_window_damage, NULL);
   ecore_evas_event_handlers[5]  = ecore_event_handler_add(ECORE_WIN32_EVENT_WINDOW_DESTROY, _ecore_evas_win32_event_window_destroy, NULL);
   ecore_evas_event_handlers[6]  = ecore_event_handler_add(ECORE_WIN32_EVENT_WINDOW_SHOW, _ecore_evas_win32_event_window_show, NULL);
   ecore_evas_event_handlers[7]  = ecore_event_handler_add(ECORE_WIN32_EVENT_WINDOW_HIDE, _ecore_evas_win32_event_window_hide, NULL);
   ecore_evas_event_handlers[8]  = ecore_event_handler_add(ECORE_WIN32_EVENT_WINDOW_CONFIGURE, _ecore_evas_win32_event_window_configure, NULL);
   ecore_evas_event_handlers[9]  = ecore_event_handler_add(ECORE_WIN32_EVENT_WINDOW_DELETE_REQUEST, _ecore_evas_win32_event_window_delete_request, NULL);
   ecore_evas_event_handlers[10]  = ecore_event_handler_add(ECORE_WIN32_EVENT_WINDOW_PROPERTY, _ecore_evas_win32_event_window_property_change, NULL);

   ecore_event_evas_init();
   return _ecore_evas_init_count;
}

/**
 * @brief Shuts down the Ecore_Evas Win32 module.
 *
 * Removes event handlers and cleans up resources.
 * This function maintains an initialization counter.
 *
 * @return The current initialization count (0 if fully shut down).
 */
int
_ecore_evas_win32_shutdown(void)
{
   _ecore_evas_init_count--;
   if (_ecore_evas_init_count == 0)
     {
        int i;

        for (i = 0; i < ECORE_EVAS_EVENT_COUNT; i++)
          ecore_event_handler_del(ecore_evas_event_handlers[i]);
        ecore_event_evas_shutdown();
     }

   if (_ecore_evas_init_count < 0) _ecore_evas_init_count = 0;

   return _ecore_evas_init_count;
}

static Eina_Bool
_ecore_evas_win32_event_mouse_in(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Evas *ee;
   Ecore_Win32_Event_Mouse_In *e;

   INF("mouse in");

   e = event;
   // Retrieve the Ecore_Evas instance associated with the window.
   ee = ecore_event_window_match((Ecore_Window)e->window);
   if ((!ee) || (ee->ignore_events)) return ECORE_CALLBACK_PASS_ON;
   if ((Ecore_Window)e->window != ee->prop.window) return ECORE_CALLBACK_PASS_ON;

   _ecore_evas_mouse_inout_set(ee, NULL, EINA_TRUE, EINA_FALSE);
   /* FIXME to do */
/*    _ecore_evas_x_modifier_locks_update(ee, e->modifiers); */
   evas_event_feed_mouse_in(ee->evas, e->timestamp, NULL);
   evas_focus_in(ee->evas);
   _ecore_evas_mouse_move_process(ee, e->x, e->y, e->timestamp);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_evas_win32_event_mouse_out(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Evas *ee;
   Ecore_Win32_Event_Mouse_Out *e;

   e = event;
   // Retrieve the Ecore_Evas instance associated with the window.
   ee = ecore_event_window_match((Ecore_Window)e->window);
   if ((!ee) || (ee->ignore_events)) return ECORE_CALLBACK_PASS_ON;
   if ((Ecore_Window)e->window != ee->prop.window) return ECORE_CALLBACK_PASS_ON;

   /* FIXME to do */
/*    _ecore_evas_x_modifier_locks_update(ee, e->modifiers); */
   _ecore_evas_mouse_move_process(ee, e->x, e->y, e->timestamp);

   if (_ecore_evas_mouse_in_check(ee, NULL))
     {
        if (evas_event_down_count_get(ee->evas) > 0) return ECORE_CALLBACK_PASS_ON;
        evas_event_feed_mouse_out(ee->evas, e->timestamp, NULL);
        _ecore_evas_mouse_inout_set(ee, NULL, EINA_FALSE, EINA_FALSE);
        _ecore_evas_default_cursor_hide(ee);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_evas_win32_event_window_focus_in(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Evas *ee;
   Ecore_Win32_Event_Window_Focus_In *e;

   e = event;
   // Retrieve the Ecore_Evas instance associated with the window.
   ee = ecore_event_window_match((Ecore_Window)e->window);
   if ((!ee) || (ee->ignore_events)) return ECORE_CALLBACK_PASS_ON;
   if ((Ecore_Window)e->window != ee->prop.window) return ECORE_CALLBACK_PASS_ON;

   _ecore_evas_focus_device_set(ee, NULL, EINA_TRUE);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_evas_win32_event_window_focus_out(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Evas *ee;
   Ecore_Win32_Event_Window_Focus_Out *e;

   e = event;
   // Retrieve the Ecore_Evas instance associated with the window.
   ee = ecore_event_window_match((Ecore_Window)e->window);
   if ((!ee) || (ee->ignore_events)) return ECORE_CALLBACK_PASS_ON;
   if ((Ecore_Window)e->window != ee->prop.window) return ECORE_CALLBACK_PASS_ON;

   _ecore_evas_focus_device_set(ee, NULL, EINA_FALSE);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_evas_win32_event_window_damage(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Evas *ee;
   Ecore_Win32_Event_Window_Damage *e;

   INF("window damage");

   e = event;
   // Retrieve the Ecore_Evas instance associated with the window.
   ee = ecore_event_window_match((Ecore_Window)e->window);
   if (!ee) return ECORE_CALLBACK_PASS_ON;
   if ((Ecore_Window)e->window != ee->prop.window) return ECORE_CALLBACK_PASS_ON;
   // Add the damaged rectangle to Evas, considering window rotation.

   if (ee->prop.avoid_damage)
     {
#warning [ECORE] [WIN32] No Region code
     }
   else
     {
        if (ee->rotation == 0)
          evas_damage_rectangle_add(ee->evas,
                                    e->x,
                                    e->y,
                                    e->width,
                                    e->height);
        else if (ee->rotation == 90)
          evas_damage_rectangle_add(ee->evas,
                                    ee->h - e->y - e->height,
                                    e->x,
                                    e->height,
                                    e->width);
        else if (ee->rotation == 180)
          evas_damage_rectangle_add(ee->evas,
                                    ee->w - e->x - e->width,
                                    ee->h - e->y - e->height,
                                    e->width,
                                    e->height);
        else if (ee->rotation == 270)
          evas_damage_rectangle_add(ee->evas,
                                    e->y,
                                    ee->w - e->x - e->width,
                                    e->height,
                                    e->width);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_evas_win32_event_window_destroy(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Evas *ee;
   Ecore_Win32_Event_Window_Destroy *e;

   INF("window destroy");

   e = event;
   // Retrieve the Ecore_Evas instance associated with the window.
   ee = ecore_event_window_match((Ecore_Window)e->window);
   if (!ee) return ECORE_CALLBACK_PASS_ON;
   if ((Ecore_Window)e->window != ee->prop.window) return ECORE_CALLBACK_PASS_ON;
   // Call the destroy callback and free the Ecore_Evas.
   if (ee->func.fn_destroy) ee->func.fn_destroy(ee);
   ecore_evas_free(ee);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_evas_win32_event_window_show(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Evas *ee;
   Ecore_Win32_Event_Window_Show *e;

   INF("window show");

   e = event;
   // Retrieve the Ecore_Evas instance associated with the window.
   ee = ecore_event_window_match((Ecore_Window)e->window);
   if (!ee) return ECORE_CALLBACK_PASS_ON; /* pass on event */
   if ((Ecore_Window)e->window != ee->prop.window) return ECORE_CALLBACK_PASS_ON;
   // Update visibility state and call the show callback.
   ee->prop.withdrawn = EINA_FALSE;
   if (ee->func.fn_state_change) ee->func.fn_state_change(ee);
   if (ee->visible) return ECORE_CALLBACK_PASS_ON;
   /* if (ee->visible) return ECORE_CALLBACK_PASS_DONE; /\* dont pass it on *\/ */
   ee->visible = 1;
   if (ee->func.fn_show) ee->func.fn_show(ee);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_evas_win32_event_window_hide(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Evas *ee;
   Ecore_Win32_Event_Window_Hide *e;

   INF("window hide");

   e = event;
   // Retrieve the Ecore_Evas instance associated with the window.
   ee = ecore_event_window_match((Ecore_Window)e->window);
   if (!ee) return ECORE_CALLBACK_PASS_ON; /* pass on event */
   if ((Ecore_Window)e->window != ee->prop.window) return ECORE_CALLBACK_PASS_ON;
   // Update visibility state and call the hide callback.
   ee->prop.withdrawn = EINA_TRUE;
   if (ee->func.fn_state_change) ee->func.fn_state_change(ee);
   if (ee->visible) return ECORE_CALLBACK_PASS_ON;
   /* if (ee->visible) return ECORE_CALLBACK_PASS_DONE; /\* dont pass it on *\/ */
   ee->visible = 0;
   if (ee->func.fn_hide) ee->func.fn_hide(ee);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_evas_win32_event_window_configure(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   const Evas_Device *pointer;
   Ecore_Evas_Cursor *cursor;
   Ecore_Evas *ee;
   Ecore_Win32_Event_Window_Configure *e;


   INF("window configure");

   e = event;
   // Retrieve the Ecore_Evas instance associated with the window.
   ee = ecore_event_window_match((Ecore_Window)e->window);
   if (!ee) return ECORE_CALLBACK_PASS_ON;
   if ((Ecore_Window)e->window != ee->prop.window) return ECORE_CALLBACK_PASS_ON;

   // Unblock drawing as configuration changes might require redraw.
   ee->draw_block = EINA_FALSE;
   pointer = evas_default_device_get(ee->evas, EFL_INPUT_DEVICE_TYPE_MOUSE);
   pointer = evas_device_parent_get(pointer);
   cursor = eina_hash_find(ee->prop.cursors, &pointer);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cursor, 1);

   if (ee->prop.override)
     {
        if ((ee->x != e->x) || (ee->y != e->y))
          {
             ee->x = e->x;
             ee->y = e->y;
             ee->req.x = ee->x;
             ee->req.y = ee->y;

             if (ee->func.fn_move) ee->func.fn_move(ee);
          }
     }

   if ((ee->w != e->width) || (ee->h != e->height))
     {
        ee->w = e->width;
        ee->h = e->height;
        ee->req.w = ee->w;
        ee->req.h = ee->h;

        if (ECORE_EVAS_PORTRAIT(ee))
          {
            evas_output_size_set(ee->evas, EE_SZ(w), EE_SZ(h));
            evas_output_viewport_set(ee->evas, 0, 0, EE_SZ(w), EE_SZ(h));
          }
        else
          {
             evas_output_size_set(ee->evas, EE_SZ(h), EE_SZ(w));
             evas_output_viewport_set(ee->evas, 0, 0, EE_SZ(h), EE_SZ(w));
          }
        if (ee->prop.avoid_damage)
          {
             int pdam;

             pdam = ecore_evas_avoid_damage_get(ee);
             ecore_evas_avoid_damage_set(ee, 0);
             ecore_evas_avoid_damage_set(ee, pdam);
          }
/*         if (ee->shaped) */
/*           _ecore_evas_win32_region_border_resize(ee); */
        if ((ee->expecting_resize.w > 0) &&
            (ee->expecting_resize.h > 0))
          {
             if ((ee->expecting_resize.w == ee->w) &&
                 (ee->expecting_resize.h == ee->h))
               _ecore_evas_mouse_move_process(ee, cursor->pos_x, cursor->pos_y,
                                              ecore_win32_current_time_get());
             ee->expecting_resize.w = 0;
             ee->expecting_resize.h = 0;
          }
        if (ee->func.fn_resize) ee->func.fn_resize(ee);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_evas_win32_event_window_delete_request(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Evas *ee;
   Ecore_Win32_Event_Window_Delete_Request *e;

   INF("window delete request");

   e = event;
   // Retrieve the Ecore_Evas instance associated with the window.
   ee = ecore_event_window_match((Ecore_Window)e->window);
   if (!ee) return ECORE_CALLBACK_PASS_ON;
   if ((Ecore_Window)e->window != ee->prop.window) return ECORE_CALLBACK_PASS_ON;
   // Call the delete request callback.
   if (ee->func.fn_delete_request) ee->func.fn_delete_request(ee);

   INF(" * ee event delete\n");
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_evas_win32_event_window_property_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   struct {
      struct {
         unsigned char maximized : 1;
         unsigned char fullscreen : 1;
      } win32;
      struct {
         Eina_Bool maximized : 1;
         Eina_Bool fullscreen : 1;
      } prop;
   } prev;
   Ecore_Evas *ee;
   Ecore_Win32_Event_Window_Property *e;
   Ecore_Evas_Engine_Data_Win32 *wdata;
   Ecore_Win32_Window_State *state;
   unsigned int num;
   unsigned int i;

   INF("window property");

   e = event;
   // Retrieve the Ecore_Evas instance associated with the window.
   ee = ecore_event_window_match((Ecore_Window)e->window);
   if (!ee) return ECORE_CALLBACK_PASS_ON; /* pass on event */
   if ((Ecore_Window)e->window != ee->prop.window) return ECORE_CALLBACK_PASS_ON;
   wdata = ee->engine.data;

   // Store previous states to detect changes.
   prev.win32.fullscreen = wdata->state.fullscreen;
   prev.win32.maximized = wdata->state.maximized;

   prev.prop.fullscreen = ee->prop.fullscreen;
   prev.prop.maximized = ee->prop.maximized;

   wdata->state.fullscreen = 0;
   wdata->state.maximized = 0;

   ee->prop.fullscreen = EINA_FALSE;
   ee->prop.maximized = EINA_FALSE;

   /* we get the states status */
   ecore_win32_window_state_get(e->window, &state, &num);
   if (state)
     {
        for (i = 0; i < num; i++)
          {
             switch (state[i])
               {
                case ECORE_WIN32_WINDOW_STATE_FULLSCREEN:
                   ee->prop.fullscreen = 1;
                   wdata->state.fullscreen = 1;
                   break;
                case ECORE_WIN32_WINDOW_STATE_MAXIMIZED:
                   ee->prop.maximized = 1;
                   wdata->state.maximized = 1;
                   break;
                default:
                   break;
               }
          }
        free(state);
     }

   if ((prev.win32.fullscreen != wdata->state.fullscreen) ||
       (prev.prop.fullscreen != ee->prop.fullscreen) ||
       (prev.win32.maximized != wdata->state.maximized) ||
       (prev.prop.maximized != ee->prop.maximized))
     {
        if (ee->func.fn_state_change)
          ee->func.fn_state_change(ee);
     }

   return ECORE_CALLBACK_PASS_ON;
}

/* FIXME, should be in idler */
/**
 * @brief Updates the Win32 window states based on Ecore_Evas properties.
 *
 * This function translates Ecore_Evas properties (modal, sticky, maximized, etc.)
 * into corresponding Win32 window states and applies them.
 *
 * @param ee The Ecore_Evas instance.
 */
static void
_ecore_evas_win32_state_update(Ecore_Evas *ee)
{
   Ecore_Win32_Window_State state[10]; // Array to hold Win32 window states.
   Ecore_Evas_Engine_Data_Win32 *edata = ee->engine.data;
   int num = 0;

   if (ee->prop.modal)
     state[num++] = ECORE_WIN32_WINDOW_STATE_MODAL;
   if (ee->prop.sticky)
     state[num++] = ECORE_WIN32_WINDOW_STATE_STICKY;
   if (ee->prop.maximized)
     state[num++] = ECORE_WIN32_WINDOW_STATE_MAXIMIZED_VERT;
   if (ee->prop.maximized)
     state[num++] = ECORE_WIN32_WINDOW_STATE_MAXIMIZED_HORZ;
   if (ee->prop.maximized)
     state[num++] = ECORE_WIN32_WINDOW_STATE_MAXIMIZED;
//   if (bd->client.netwm.state.shaded)
//     state[num++] = ECORE_WIN32_WINDOW_STATE_SHADED;
   /* if (ee->prop.focus_skip) */
   /*   state[num++] = ECORE_WIN32_WINDOW_STATE_SKIP_TASKBAR; */
   /* if (ee->prop.focus_skip) */
   /*   state[num++] = ECORE_WIN32_WINDOW_STATE_SKIP_PAGER; */
//   if (bd->client.netwm.state.hidden)
//     state[num++] = ECORE_WIN32_WINDOW_STATE_HIDDEN;
   if (edata->state.fullscreen)
     state[num++] = ECORE_WIN32_WINDOW_STATE_FULLSCREEN;
   /* if (edata->state.above) */
   /*   state[num++] = ECORE_WIN32_WINDOW_STATE_ABOVE; */
   /* if (edata->state.below) */
   /*   state[num++] = ECORE_WIN32_WINDOW_STATE_BELOW; */
   /* if (ee->prop.demand_attention) */
   /*   state[num++] = ECORE_WIN32_WINDOW_STATE_DEMANDS_ATTENTION; */

   ecore_win32_window_state_set((Ecore_Win32_Window *)ee->prop.window, state, num);
}


/* Ecore_Evas interface */

/**
 * @brief Frees resources associated with a Win32 Ecore_Evas.
 * @param ee The Ecore_Evas instance to free.
 */
static void
_ecore_evas_win32_free(Ecore_Evas *ee)
{
   INF("ecore evas free");

   ecore_win32_window_free((Ecore_Win32_Window *)ee->prop.window);
   ecore_event_window_unregister(ee->prop.window);
   free(ee->engine.data);
   _ecore_evas_win32_shutdown();
   ecore_win32_shutdown();
}

/**
 * @brief Sets the callback function for delete requests.
 * @param ee The Ecore_Evas instance.
 * @param func The callback function.
 */
static void
_ecore_evas_win32_callback_delete_request_set(Ecore_Evas *ee,
                                              Ecore_Evas_Event_Cb func)
{
   ee->func.fn_delete_request = func;
}

/**
 * @brief Moves the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 * @param x The new X coordinate.
 * @param y The new Y coordinate.
 */
static void
_ecore_evas_win32_move(Ecore_Evas *ee, int x, int y)
{
  INF("ecore evas move (%dx%d)", x, y);
   ee->req.x = x;
   ee->req.y = y;

   if ((x != ee->x) || (y != ee->y))
     {
        ee->x = x;
        ee->y = y;
        ecore_win32_window_move((Ecore_Win32_Window *)ee->prop.window,
                                x, y);
        if (ee->func.fn_move) ee->func.fn_move(ee);
     }
}

/**
 * @brief Resizes the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 * @param width The new width.
 * @param height The new height.
 */
static void
_ecore_evas_win32_resize(Ecore_Evas *ee, int width, int height)
{
   INF("ecore evas resize (%dx%d)", width, height);

   if ((ee->req.w != width) || (ee->req.h != height))
     {
        // Update requested size and resize the native window.
        ee->req.w = width;
        ee->req.h = height;
        ecore_win32_window_resize((Ecore_Win32_Window *)ee->prop.window,
                                  width, height);
     }
}

/**
 * @brief Moves and resizes the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 * @param x The new X coordinate.
 * @param y The new Y coordinate.
 * @param width The new width.
 * @param height The new height.
 */
static void
_ecore_evas_win32_move_resize(Ecore_Evas *ee, int x, int y, int width, int height)
{
   INF("ecore evas resize (%dx%d %dx%d)", x, y, width, height);
   // Update requested position and size.
   ee->req.x = x;
   ee->req.y = y;
   ee->req.w = width;
   ee->req.h = height;

   if ((ee->w != width) || (ee->h != height) || (x != ee->x) || (y != ee->y))
     {
        int change_size = 0;
        int change_pos = 0;

        if ((ee->w != width) || (ee->h != height)) change_size = 1;
        if ((x != ee->x) || (y != ee->y)) change_pos = 1;

        ee->x = x;
        ee->y = y;
        ee->w = width;
        ee->h = height;
        ecore_win32_window_move_resize((Ecore_Win32_Window *)ee->prop.window,
                                       x, y, width, height);
        if (ECORE_EVAS_PORTRAIT(ee))
          {
             evas_output_size_set(ee->evas, EE_SZ(w), EE_SZ(h));
             evas_output_viewport_set(ee->evas, 0, 0, EE_SZ(w), EE_SZ(h));
          }
        else
          {
             evas_output_size_set(ee->evas, EE_SZ(h), EE_SZ(w));
             evas_output_viewport_set(ee->evas, 0, 0, EE_SZ(h), EE_SZ(w));
          }
        if (ee->prop.avoid_damage)
          {
             int pdam;

             pdam = ecore_evas_avoid_damage_get(ee);
             ecore_evas_avoid_damage_set(ee, 0);
             ecore_evas_avoid_damage_set(ee, pdam);
          }
/*         if ((ee->shaped) || (ee->alpha)) */
/*           _ecore_evas_win32_region_border_resize(ee); */
        if (change_pos)
          {
             if (ee->func.fn_move) ee->func.fn_move(ee);
          }
        if (change_size)
          {
             if (ee->func.fn_resize) ee->func.fn_resize(ee);
          }
     }
}

/**
 * @brief Internal function to handle window rotation logic.
 *
 * This function adjusts window dimensions, Evas output size, and viewport
 * according to the new rotation. It also updates size hints (min, max, base, step)
 * and processes mouse movement to reflect the rotation.
 *
 * @param ee The Ecore_Evas instance.
 * @param rotation The new rotation angle (0, 90, 180, 270).
 */
static void
_ecore_evas_win32_rotation_set_internal(Ecore_Evas *ee, int rotation)
{
   const Evas_Device *pointer;
   Ecore_Evas_Cursor *cursor;
   int rot_dif;

   rot_dif = ee->rotation - rotation;
   if (rot_dif < 0) rot_dif = -rot_dif;

   pointer = evas_default_device_get(ee->evas, EFL_INPUT_DEVICE_TYPE_MOUSE);
   pointer = evas_device_parent_get(pointer);
   cursor = eina_hash_find(ee->prop.cursors, &pointer);
   EINA_SAFETY_ON_NULL_RETURN(cursor);

   if (rot_dif != 180)
     {
        int minw, minh, maxw, maxh, basew, baseh, stepw, steph;

        if (!ee->prop.fullscreen)
          {
             ecore_win32_window_resize((Ecore_Win32_Window *)ee->prop.window,
                                       ee->h, ee->w);
             ee->expecting_resize.w = ee->h;
             ee->expecting_resize.h = ee->w;
          }
        else
          {
             int w, h;

             ecore_win32_window_size_get((Ecore_Win32_Window *)ee->prop.window,
                                         &w, &h);
             ecore_win32_window_resize((Ecore_Win32_Window *)ee->prop.window,
                                       h, w);
             if (ECORE_EVAS_PORTRAIT(ee))
               {
                  evas_output_size_set(ee->evas, EE_SZ(w), EE_SZ(h));
                  evas_output_viewport_set(ee->evas, 0, 0, EE_SZ(w), EE_SZ(h));
               }
             else
               {
                  evas_output_size_set(ee->evas, EE_SZ(h), EE_SZ(w));
                  evas_output_viewport_set(ee->evas, 0, 0, EE_SZ(h), EE_SZ(w));
               }
             if (ee->func.fn_resize) ee->func.fn_resize(ee);
          }
        ecore_evas_size_min_get(ee, &minw, &minh);
        ecore_evas_size_max_get(ee, &maxw, &maxh);
        ecore_evas_size_base_get(ee, &basew, &baseh);
        ecore_evas_size_step_get(ee, &stepw, &steph);
        ee->rotation = rotation;
        ecore_evas_size_min_set(ee, minh, minw);
        ecore_evas_size_max_set(ee, maxh, maxw);
        ecore_evas_size_base_set(ee, baseh, basew);
        ecore_evas_size_step_set(ee, steph, stepw);
        _ecore_evas_mouse_move_process(ee, cursor->pos_x, cursor->pos_y,
                                       ecore_win32_current_time_get());
     }
   else
     {
        ee->rotation = rotation;
        _ecore_evas_mouse_move_process(ee, cursor->pos_x, cursor->pos_y,
                                       ecore_win32_current_time_get());
        if (ee->func.fn_resize) ee->func.fn_resize(ee);
     }

   if (ECORE_EVAS_PORTRAIT(ee))
     evas_damage_rectangle_add(ee->evas, 0, 0, ee->w, ee->h);
   else
     evas_damage_rectangle_add(ee->evas, 0, 0, ee->h, ee->w);
}

/**
 * @brief Sets the rotation of the Win32 Ecore_Evas window.
 *
 * This function updates the Evas engine info with the new rotation
 * and calls the internal rotation handling logic.
 *
 * @param ee The Ecore_Evas instance.
 * @param rotation The new rotation angle (0, 90, 180, 270).
 * @param resize Unused parameter.
 */
static void
_ecore_evas_win32_rotation_set(Ecore_Evas *ee, int rotation, int resize EINA_UNUSED)
{
   INF("ecore evas rotation: %s", rotation ? "yes" : "no");

   if (ee->rotation == rotation) return;

   // Engine-specific rotation handling.
#ifdef BUILD_ECORE_EVAS_SOFTWARE_GDI
   if (!strcmp(ee->driver, "software_gdi"))
     {
        Evas_Engine_Info_Software_Gdi *einfo;

        einfo = (Evas_Engine_Info_Software_Gdi *)evas_engine_info_get(ee->evas);
        if (!einfo) return;
        einfo->info.rotation = rotation;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
          }
        _ecore_evas_win32_rotation_set_internal(ee, rotation);
     }
#endif /* BUILD_ECORE_EVAS_SOFTWARE_GDI */

#ifdef BUILD_ECORE_EVAS_SOFTWARE_DDRAW
   if (!strcmp(ee->driver, "software_ddraw"))
     {
        Evas_Engine_Info_Software_DDraw *einfo;

        einfo = (Evas_Engine_Info_Software_DDraw *)evas_engine_info_get(ee->evas);
        if (!einfo) return;
        einfo->info.rotation = rotation;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
          }
        _ecore_evas_win32_rotation_set_internal(ee, rotation);
     }
#endif /* BUILD_ECORE_EVAS_SOFTWARE_DDRAW */

#ifdef BUILD_ECORE_EVAS_OPENGL_WIN32
   if (!strcmp(ee->driver, "opengl_win32"))
     {
        Evas_Engine_Info_GL_Win32 *einfo;

        einfo = (Evas_Engine_Info_GL_Win32 *)evas_engine_info_get(ee->evas);
        if (!einfo) return;
        einfo->info.rotation = rotation;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
          }
        _ecore_evas_win32_rotation_set_internal(ee, rotation);
     }
#endif /* BUILD_ECORE_EVAS_SOFTWARE_GDI */
}

/**
 * @brief Enables or disables shaping for the Win32 Ecore_Evas window.
 *
 * Shaping allows the window to have a non-rectangular form.
 * This is primarily supported by the GDI engine.
 *
 * @param ee The Ecore_Evas instance.
 * @param shaped 1 to enable shaping, 0 to disable.
 */
static void
_ecore_evas_win32_shaped_set(Ecore_Evas *ee, int shaped)
{
   Ecore_Evas_Engine_Data_Win32 *wdata;
   if (((ee->shaped) && (shaped)) || ((!ee->shaped) && (!shaped)))
     return; // No change in shaped state.

   wdata = ee->engine.data;
   if (!strcmp(ee->driver, "software_ddraw")) return;

#ifdef BUILD_ECORE_EVAS_SOFTWARE_GDI
   if (!strcmp(ee->driver, "software_gdi"))
     {
        Evas_Engine_Info_Software_Gdi *einfo;

        einfo = (Evas_Engine_Info_Software_Gdi *)evas_engine_info_get(ee->evas);
        ee->shaped = shaped;
        if (einfo)
          {
             wdata->state.region = ee->shaped;
             einfo->info.region = wdata->state.region;
             if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
               {
                  ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
               }
             if (ee->shaped)
               evas_damage_rectangle_add(ee->evas, 0, 0, ee->w, ee->h);
          }
     }
#endif /* BUILD_ECORE_EVAS_SOFTWARE_GDI */
}

/**
 * @brief Shows the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 */
static void
_ecore_evas_win32_show(Ecore_Evas *ee)
{
   INF("ecore evas show");

   ee->should_be_visible = 1; // Mark that the window should be visible.
   if (ee->prop.avoid_damage)
     {
        // If avoid_damage is set, render synchronously before showing.
        ecore_evas_render(ee);
        ecore_evas_render_wait(ee);
     }
   ecore_win32_window_show((Ecore_Win32_Window *)ee->prop.window);
/*    if (ee->prop.fullscreen) */
/*      ecore_win32_window_focus(ee->prop.window); */
}

/**
 * @brief Hides the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 */
static void
_ecore_evas_win32_hide(Ecore_Evas *ee)
{
   INF("ecore evas hide");

   ecore_win32_window_hide((Ecore_Win32_Window *)ee->prop.window);
   ee->should_be_visible = 0; // Mark that the window should not be visible.
}

/**
 * @brief Raises the Win32 Ecore_Evas window to the top of the stacking order.
 * @param ee The Ecore_Evas instance.
 */
static void
_ecore_evas_win32_raise(Ecore_Evas *ee)
{
   INF("ecore evas raise");

   if (!ee->prop.fullscreen)
     ecore_win32_window_raise((Ecore_Win32_Window *)ee->prop.window);
   else
     ecore_win32_window_raise((Ecore_Win32_Window *)ee->prop.window);
}

/**
 * @brief Lowers the Win32 Ecore_Evas window to the bottom of the stacking order.
 * @param ee The Ecore_Evas instance.
 */
static void
_ecore_evas_win32_lower(Ecore_Evas *ee)
{
   INF("ecore evas lower");

   if (!ee->prop.fullscreen)
     ecore_win32_window_lower((Ecore_Win32_Window *)ee->prop.window);
   else
     ecore_win32_window_lower((Ecore_Win32_Window *)ee->prop.window);
}

/**
 * @brief Activates (gives focus to) the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 */
static void
_ecore_evas_win32_activate(Ecore_Evas *ee)
{
   INF("ecore evas activate");

   ecore_evas_show(ee); // Ensure window is visible before activating.
   ecore_win32_window_activate((Ecore_Win32_Window *)ee->prop.window);
}

/**
 * @brief Sets the title of the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 * @param title The new title string.
 */
static void
_ecore_evas_win32_title_set(Ecore_Evas *ee, const char *title)
{
   INF("ecore evas title set");
   if (eina_streq(ee->prop.title, title)) return; // No change if title is the same.
   if (ee->prop.title) free(ee->prop.title);
   ee->prop.title = NULL;
   if (title) ee->prop.title = strdup(title);
   ecore_win32_window_title_set((Ecore_Win32_Window *)ee->prop.window,
                                ee->prop.title);
}

/**
 * @brief Sets the minimum size of the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 * @param width The minimum width.
 * @param height The minimum height.
 */
static void
_ecore_evas_win32_size_min_set(Ecore_Evas *ee, int width, int height)
{
   if (width < 0) width = 0;
   if (height < 0) height = 0;
   if ((ee->prop.min.w == width) && (ee->prop.min.h == height)) return; // No change.
   ee->prop.min.w = width;
   ee->prop.min.h = height;
   ecore_win32_window_size_min_set((Ecore_Win32_Window *)ee->prop.window,
                                   width, height);
}

/**
 * @brief Sets the maximum size of the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 * @param width The maximum width.
 * @param height The maximum height.
 */
static void
_ecore_evas_win32_size_max_set(Ecore_Evas *ee, int width, int height)
{
   if (width < 0) width = 0;
   if (height < 0) height = 0;
   if ((ee->prop.max.w == width) && (ee->prop.max.h == height)) return; // No change.
   ee->prop.max.w = width;
   ee->prop.max.h = height;
   ecore_win32_window_size_max_set((Ecore_Win32_Window *)ee->prop.window,
                                   width, height);
}

/**
 * @brief Sets the base size for window resizing steps.
 * @param ee The Ecore_Evas instance.
 * @param width The base width.
 * @param height The base height.
 */
static void
_ecore_evas_win32_size_base_set(Ecore_Evas *ee, int width, int height)
{
   if (width < 0) width = 0;
   if (height < 0) height = 0;
   if ((ee->prop.base.w == width) && (ee->prop.base.h == height)) return; // No change.
   ee->prop.base.w = width;
   ee->prop.base.h = height;
   ecore_win32_window_size_base_set((Ecore_Win32_Window *)ee->prop.window,
                                    width, height);
}

/**
 * @brief Sets the step size for window resizing.
 * @param ee The Ecore_Evas instance.
 * @param width The width increment for resizing.
 * @param height The height increment for resizing.
 */
static void
_ecore_evas_win32_size_step_set(Ecore_Evas *ee, int width, int height)
{
   if (width < 1) width = 1; // Ensure step is at least 1.
   if (height < 1) height = 1; // Ensure step is at least 1.
   if ((ee->prop.step.w == width) && (ee->prop.step.h == height)) return; // No change.
   ee->prop.step.w = width;
   ee->prop.step.h = height;
   ecore_win32_window_size_step_set((Ecore_Win32_Window *)ee->prop.window,
                                    width, height);
}

/**
 * @brief Sets a custom cursor object for the Ecore_Evas window.
 *
 * If a custom Evas object is set as the cursor, the native Win32 cursor
 * is hidden.
 *
 * @param ee The Ecore_Evas instance.
 * @param obj The Evas object to use as a cursor.
 * @param layer Unused.
 * @param hot_x Unused.
 * @param hot_y Unused.
 */
static void
_ecore_evas_win32_object_cursor_set(Ecore_Evas *ee, Evas_Object *obj,
                                    int layer EINA_UNUSED,
                                    int hot_x EINA_UNUSED,
                                    int hot_y EINA_UNUSED)
{
   // If the object is not the default cursor image, hide the system cursor.
   if (obj != _ecore_evas_default_cursor_image_get(ee))
     ecore_win32_cursor_show(EINA_FALSE);
}

/**
 * @brief Unsets a custom cursor object, reverting to the native cursor.
 * @param ee The Ecore_Evas instance (unused).
 */
static void
_ecore_evas_win32_object_cursor_unset(Ecore_Evas *ee EINA_UNUSED)
{
   // Show the system cursor.
   ecore_win32_cursor_show(EINA_TRUE);
}

/**
 * @brief Sets focus to the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 * @param on Unused (focus is always set).
 */
static void
_ecore_evas_win32_focus_set(Ecore_Evas *ee, Eina_Bool on EINA_UNUSED)
{
   ecore_win32_window_focus((Ecore_Win32_Window *)ee->prop.window);
}

/**
 * @brief Sets the iconified (minimized) state of the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 * @param on EINA_TRUE to iconify, EINA_FALSE to deiconify.
 */
static void
_ecore_evas_win32_iconified_set(Ecore_Evas *ee, Eina_Bool on)
{
/*    if (((ee->prop.borderless) && (on)) || */
/*        ((!ee->prop.borderless) && (!on))) return; */
   ee->prop.iconified = on;
   ecore_win32_window_iconified_set((Ecore_Win32_Window *)ee->prop.window,
                                    ee->prop.iconified);
}

/**
 * @brief Sets the borderless state of the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 * @param on EINA_TRUE for borderless, EINA_FALSE for bordered.
 */
static void
_ecore_evas_win32_borderless_set(Ecore_Evas *ee, Eina_Bool on)
{
   if (((ee->prop.borderless) && (on)) ||
       ((!ee->prop.borderless) && (!on))) return; // No change.
   ee->prop.borderless = on;
   ecore_win32_window_borderless_set((Ecore_Win32_Window *)ee->prop.window,
                                     ee->prop.borderless);

#ifdef BUILD_ECORE_EVAS_SOFTWARE_GDI
   if (!strcmp(ee->driver, "software_gdi"))
     {
        Evas_Engine_Info_Software_Gdi *einfo;

        einfo = (Evas_Engine_Info_Software_Gdi *)evas_engine_info_get(ee->evas);
        if (einfo)
          {
            einfo->info.borderless = ee->prop.borderless;
             if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
               {
                  ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
               }
             if (ee->prop.borderless)
               evas_damage_rectangle_add(ee->evas, 0, 0, ee->w, ee->h);
          }
     }
#endif /* BUILD_ECORE_EVAS_SOFTWARE_GDI */
}

/**
 * @brief Sets the override redirect state of the Win32 Ecore_Evas window.
 *
 * Override redirect windows are not managed by the window manager.
 * Currently, this is implemented by setting the borderless state.
 *
 * @param ee The Ecore_Evas instance.
 * @param on EINA_TRUE to enable override redirect, EINA_FALSE to disable.
 */
static void
_ecore_evas_win32_override_set(Ecore_Evas *ee, Eina_Bool on)
{
   Ecore_Win32_Window *window;

   INF("ecore evas override set");

   window = (Ecore_Win32_Window *)ee->prop.window;

   if (ee->prop.override == on) return; // No change.
   // Temporarily hide the window to apply changes.
   if (ee->should_be_visible) ecore_win32_window_hide(window);
   /* FIXME: use borderless_set for now */
   ecore_win32_window_borderless_set(window, on);
   if (ee->should_be_visible) ecore_win32_window_show(window);
   if (ecore_evas_focus_device_get(ee, NULL)) ecore_win32_window_focus(window);
   ee->prop.override = on;
}

/**
 * @brief Sets the maximized state of the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 * @param on EINA_TRUE to maximize, EINA_FALSE to unmaximize.
 */
static void
_ecore_evas_win32_maximized_set(Ecore_Evas *ee, Eina_Bool on)
{
   Ecore_Evas_Engine_Data_Win32 *wdata = ee->engine.data;

   INF("ecore evas maximized set");

   wdata->state.maximized = !!on; // Update internal state.
   if (ee->should_be_visible)
     {
        // If window is visible, apply maximization directly.
        struct _Ecore_Win32_Window *window;

        window = (Ecore_Win32_Window *)ee->prop.window;
        ecore_win32_window_maximized_set(window, on);
     }
   else
     {
        if (ee->prop.maximized == on) return;
        ee->prop.maximized = on;
        wdata->state.maximized = on;
        _ecore_evas_win32_state_update(ee);
     }
}

/**
 * @brief Sets the fullscreen state of the Win32 Ecore_Evas window.
 * @param ee The Ecore_Evas instance.
 * @param on EINA_TRUE for fullscreen, EINA_FALSE for windowed.
 */
static void
_ecore_evas_win32_fullscreen_set(Ecore_Evas *ee, Eina_Bool on)
{
   Ecore_Evas_Engine_Data_Win32 *wdata = ee->engine.data;

   INF("ecore evas fullscreen set");

   if (ee->prop.fullscreen == !!on) return; // No change.

   wdata->state.fullscreen = !!on; // Update internal state.
   if (ee->should_be_visible)
     {
        // If window is visible, apply fullscreen directly.
        struct _Ecore_Win32_Window *window;

        window = (Ecore_Win32_Window *)ee->prop.window;
        ecore_win32_window_fullscreen_set(window, on);
     }
   else
     _ecore_evas_win32_state_update(ee);

   /* Nothing to be done for the GDI backend at the evas level */
   /* Nothing to be done for the OpenGL backend at the evas level */

#ifdef BUILD_ECORE_EVAS_SOFTWRE_DDRAW
   if (strcmp(ee->driver, "software_ddraw") == 0)
     {
        Evas_Engine_Info_Software_DDraw *einfo;

        einfo = (Evas_Engine_Info_Software_DDraw *)evas_engine_info_get(ecore_evas_get(ee));
        if (einfo)
          {
             einfo->info.fullscreen = !!on;
/*           einfo->info.layered = window->shape.layered; */
             if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
               {
                  ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
               }
          }
     }
#endif /* BUILD_ECORE_EVAS_SOFTWARE_DDRAW */
}

/**
 * @brief Enables or disables alpha compositing for the Win32 Ecore_Evas window.
 *
 * This allows for per-pixel alpha blending with the desktop.
 * Primarily affects the GDI engine.
 *
 * @param ee The Ecore_Evas instance.
 * @param alpha 1 to enable alpha, 0 to disable.
 */
static void
_ecore_evas_win32_alpha_set(Ecore_Evas *ee, int alpha)
{
#warning "We need to handle window with alpha channel."
   /* Ecore_Evas_Engine_Data_Win32 *wdata = ee->engine.data; */
   alpha = !!alpha; // Normalize to 0 or 1.
   if (ee->alpha == alpha) return; // No change.

   if (!strcmp(ee->driver, "software_gdi"))
     {
        // Alpha handling for GDI engine.
#ifdef BUILD_ECORE_EVAS_SOFTWARE_GDI
        Evas_Engine_Info_Software_Gdi *einfo;

        einfo = (Evas_Engine_Info_Software_Gdi *)evas_engine_info_get(ee->evas);
        if (!einfo) return;

        ee->shaped = 0;
        ee->alpha = alpha;
        /* ecore_win32_window_free(ee->prop.window); */
        /* ecore_event_window_unregister(ee->prop.window); */
        /* if (ee->alpha) */
        /*   { */
        /*      if (ee->prop.override) */
        /*        ee->prop.window = ecore_x_window_override_argb_new(ee->engine.x.win_root, ee->req.x, ee->req.y, ee->req.w, ee->req.h); */
        /*      else */
        /*        ee->prop.window = ecore_x_window_argb_new(ee->engine.x.win_root, ee->req.x, ee->req.y, ee->req.w, ee->req.h); */
        /*      if (!ee->engine.x.mask) */
        /*        ee->engine.x.mask = ecore_x_pixmap_new(ee->prop.window, ee->req.w, ee->req.h, 1); */
        /*   } */
        /* else */
        /*   { */
        /*      if (ee->prop.override) */
        /*        ee->prop.window = ecore_win32_window_override_new(wdata->win_root, */
        /*                                                          ee->req.x, */
        /*                                                          ee->req.y, */
        /*                                                          ee->req.w, */
        /*                                                          ee->req.h); */
        /*      else */
        /*        ee->prop.window = ecore_win32_window_new(wdata->win_root, */
        /*                                                 ee->req.x, */
        /*                                                 ee->req.y, */
        /*                                                 ee->req.w, */
        /*                                                 ee->req.h); */
        /*      if (wdata->mask) ecore_x_pixmap_free(ee->engine.x.mask); */
        /*      wdata->mask = 0; */
        /*      ecore_win32_window_shape_input_mask_set(ee->prop.window, 0); */
        /*   } */

        /* einfo->info.destination_alpha = alpha; */
        einfo->info.region = alpha;

//        if (ee->engine.x.mask) ecore_x_pixmap_free(ee->engine.x.mask);
//        ee->engine.x.mask = 0;
        /* einfo->info.mask = wdata->mask; */
        /* einfo->info.drawable = ee->prop.window; */
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
          }
        evas_damage_rectangle_add(ee->evas, 0, 0, ee->req.w, ee->req.h);
        /* ecore_win32_window_shape_mask_set(ee->prop.window, 0); */
        /* ecore_event_window_register(ee->prop.window, ee, ee->evas, */
        /*                             (Ecore_Event_Mouse_Move_Cb)_ecore_evas_mouse_move_process, */
        /*                             (Ecore_Event_Multi_Move_Cb)_ecore_evas_mouse_multi_move_process, */
        /*                             (Ecore_Event_Multi_Down_Cb)_ecore_evas_mouse_multi_down_process, */
        /*                             (Ecore_Event_Multi_Up_Cb)_ecore_evas_mouse_multi_up_process); */
        if (ee->prop.borderless)
          ecore_win32_window_borderless_set((Ecore_Win32_Window *)ee->prop.window, ee->prop.borderless);
        if (ee->visible) ecore_win32_window_show((Ecore_Win32_Window *)ee->prop.window);
        if (ecore_evas_focus_device_get(ee, NULL)) ecore_win32_window_focus((Ecore_Win32_Window *)ee->prop.window);
        if (ee->prop.title)
          {
             ecore_win32_window_title_set((Ecore_Win32_Window *)ee->prop.window, ee->prop.title);
             /* ecore_win32_name_set(ee->prop.window, ee->prop.title); */
          }
        ecore_win32_window_type_set((Ecore_Win32_Window *)ee->prop.window, ECORE_WIN32_WINDOW_TYPE_NORMAL);
#endif /* BUILD_ECORE_EVAS_SOFTWARE_GDI */
     }
}

/**
 * @brief Gets the geometry of the screen containing the Ecore_Evas window.
 *
 * This function determines which monitor the window is primarily on and
 * returns its desktop coordinates and dimensions.
 *
 * @param ee The Ecore_Evas instance.
 * @param x Pointer to store the screen X coordinate.
 * @param y Pointer to store the screen Y coordinate.
 * @param w Pointer to store the screen width.
 * @param h Pointer to store the screen height.
 */
static void
_ecore_evas_win32_screen_geometry_get(const Ecore_Evas *ee, int *x, int *y, int *w, int *h)
{
   Eina_Iterator *iter;
   Ecore_Win32_Monitor *ewm;
   Ecore_Win32_Monitor *m = NULL;
   unsigned int dist;
   int lx;
   int ly;
   int wx;
   int wy;
   int ww;
   int wh;

   ecore_win32_window_geometry_get((Ecore_Win32_Window *)ee->prop.window,
                                   &wx, &wy, &ww, &wh);
   iter = ecore_win32_monitors_get();
   dist = UINT32_MAX;

   EINA_ITERATOR_FOREACH(iter, ewm)
     {
        unsigned int d;

        lx = ewm->desktop.x - wx + (ewm->desktop.w - ww) / 2;
        ly = ewm->desktop.y - wy + (ewm->desktop.h - wh) / 2;
        d = lx * lx + ly * ly;
        if (d < dist)
          {
             dist = d;
             m = ewm;
          }
     }
   eina_iterator_free(iter);

   if (!m)
     {
        HDC dc;

        if (x) *x = 0;
        if (y) *y = 0;
        dc = GetDC(NULL);
        if (w) *w = GetDeviceCaps(dc, HORZRES);
        if (h) *h = GetDeviceCaps(dc, VERTRES);
        ReleaseDC(NULL, dc);

        return;
     }

   if (x)
     *x = m->desktop.x;
   if (y)
     *y = m->desktop.y;
   if (w)
     *w = m->desktop.w;
   if (h)
     *h = m->desktop.h;
}

/**
 * @brief Gets the DPI of the screen containing the Ecore_Evas window.
 *
 * This function determines which monitor the window is primarily on and
 * returns its DPI values.
 *
 * @param ee The Ecore_Evas instance.
 * @param xdpi Pointer to store the horizontal DPI.
 * @param ydpi Pointer to store the vertical DPI.
 */
static void
_ecore_evas_win32_screen_dpi_get(const Ecore_Evas *ee, int *xdpi, int *ydpi)
{
   Eina_Iterator *iter;
   Ecore_Win32_Monitor *ewm;
   unsigned int dist;
   int x_dpi = -1;
   int y_dpi;
   int lx;
   int ly;
   int x;
   int y;
   int w;
   int h;

   ecore_win32_window_geometry_get((Ecore_Win32_Window *)ee->prop.window,
                                   &x, &y, &w, &h);
   iter = ecore_win32_monitors_get();
   dist = UINT32_MAX;

   EINA_ITERATOR_FOREACH(iter, ewm)
     {
        unsigned int d;

        lx = ewm->desktop.x - x + (ewm->desktop.w - w) / 2;
        ly = ewm->desktop.y - y + (ewm->desktop.h - h) / 2;
        d = lx * lx + ly * ly;
        if (d < dist)
          {
             dist = d;
             x_dpi = ewm->dpi.x;
             y_dpi = ewm->dpi.y;
          }
     }
   eina_iterator_free(iter);

   if (x_dpi == -1)
     {
        HDC dc;

        dc = GetDC(NULL);
        x_dpi = GetDeviceCaps(dc, LOGPIXELSX);
        y_dpi = GetDeviceCaps(dc, LOGPIXELSY);
        ReleaseDC(NULL, dc);
     }

   if (xdpi)
     *xdpi = x_dpi;
   if (ydpi)
     *ydpi = y_dpi;
}

/**
 * @brief Callback function for asynchronous clipboard data delivery.
 *
 * This function is called when the clipboard data is ready to be sent.
 * It retrieves the data using the provided delivery callback and sets it
 * on the Win32 clipboard.
 *
 * @param data The Ecore_Evas instance.
 * @param value Unused.
 * @param dead_future Unused.
 * @return EINA_VALUE_EMPTY.
 */
static Eina_Value
_delivery(void *data, const Eina_Value value EINA_UNUSED, const Eina_Future *dead_future EINA_UNUSED)
{
   Ecore_Evas *ee = data;
   Ecore_Evas_Engine_Data_Win32 *edata = ee->engine.data;
   Eina_Rw_Slice slice;
   const char *mime_type = NULL;

   EINA_SAFETY_ON_NULL_GOTO(edata->delivery, end);

   for (unsigned int i = 0; i < eina_array_count(edata->clipboard.available_types); ++i)
     {
        mime_type = eina_array_data_get(edata->clipboard.available_types, i);
        if (eina_str_has_prefix(mime_type, "text/"))
          break;
     }
   if (mime_type)
     {
        edata->clipboard.delivery(ee, 1, ECORE_EVAS_SELECTION_BUFFER_COPY_AND_PASTE_BUFFER, mime_type, &slice);
        EINA_SAFETY_ON_FALSE_GOTO(ecore_win32_clipboard_set((Ecore_Win32_Window *)ee->prop.window, slice.mem, slice.len, mime_type), end);
     }
   else
     {
        ERR("No compatible mime type found");
     }

end:
   return EINA_VALUE_EMPTY;
}

/**
 * @brief Claims ownership of a selection (clipboard).
 *
 * This function handles requests to become the owner of the clipboard.
 * It stores the provided callbacks for data delivery and cancellation.
 *
 * @param ee The Ecore_Evas instance.
 * @param seat The seat identifier (unused on Win32).
 * @param selection The selection buffer (only COPY_AND_PASTE_BUFFER is supported).
 * @param available_types An array of MIME types the application can provide.
 *                        Example: `eina_array_new(eina_array_string_alloc_free_get())`
 *                                 `eina_array_push(types, eina_stringshare_add("text/plain;charset=utf-8"));`
 * @param delivery Callback function to provide the selection data.
 * @param cancel Callback function if selection ownership is lost.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_ecore_evas_win32_selection_claim(Ecore_Evas *ee, unsigned int seat, Ecore_Evas_Selection_Buffer selection, Eina_Array *available_types, Ecore_Evas_Selection_Internal_Delivery delivery, Ecore_Evas_Selection_Internal_Cancel cancel)
{
   Ecore_Evas_Engine_Data_Win32 *edata = ee->engine.data;

   if (selection != ECORE_EVAS_SELECTION_BUFFER_COPY_AND_PASTE_BUFFER)
     return EINA_FALSE; // Only copy/paste buffer supported.

   if (!delivery && !cancel)
     {
        // If no delivery/cancel, clear the clipboard.
        edata->clipboard.delivery = NULL;
        edata->clipboard.cancel = NULL;
        eina_array_clean(edata->clipboard.available_types);
        ecore_win32_clipboard_clear((Ecore_Win32_Window *)ee->prop.window);
        return EINA_TRUE;
     }
   else
     {
        if (edata->clipboard.cancel)
          {
             edata->clipboard.cancel(ee, seat, selection);
             eina_array_free(edata->clipboard.available_types);
          }

        edata->delivery = efl_loop_job(efl_main_loop_get());
        eina_future_then(edata->delivery, _delivery, ee);
        edata->clipboard.delivery = delivery;
        edata->clipboard.cancel = cancel;
        edata->clipboard.available_types = available_types;
        return EINA_TRUE;
     }
}

/**
 * @brief Requests data from a selection (clipboard).
 *
 * This function retrieves data from the Win32 clipboard, matching one of the
 * acceptable MIME types.
 *
 * @param ee The Ecore_Evas instance (unused).
 * @param seat The seat identifier (unused on Win32).
 * @param selection The selection buffer (only COPY_AND_PASTE_BUFFER is supported).
 * @param acceptable_type An array of acceptable MIME types.
 *                        Example: `eina_array_new(eina_array_string_get_get())`
 *                                 `eina_array_push(types, "text/plain");`
 * @return A future that will resolve with an Eina_Content containing the data,
 *         or reject if no suitable data is found.
 *         Example of resolved value: `Eina_Value` of type `EINA_VALUE_TYPE_CONTENT`
 *                                    `eina_value_pget(&value, &content);`
 *                                    `eina_content_data_get(content, &mime, &slice);`
 */
Eina_Future*
_ecore_evas_win32_selection_request(Ecore_Evas *ee EINA_UNUSED, unsigned int seat EINA_UNUSED, Ecore_Evas_Selection_Buffer selection, Eina_Array *acceptable_type)
{
   Eina_Future *future;
   Eina_Promise *promise;
   const char *mime_type = NULL;

   if (selection != ECORE_EVAS_SELECTION_BUFFER_COPY_AND_PASTE_BUFFER)
     return eina_future_rejected(efl_loop_future_scheduler_get(efl_main_loop_get()), ecore_evas_no_selection);

   promise = efl_loop_promise_new(efl_main_loop_get());
   future = eina_future_new(promise);

   for (unsigned int i = 0; i < eina_array_count(acceptable_type); ++i)
     {
        mime_type = eina_array_data_get(acceptable_type, i);
        if (eina_str_has_prefix(mime_type, "text/"))
          break;
     }
   if (!mime_type)
     {
        eina_promise_reject(promise, ecore_evas_no_matching_type);
     }
   else
     {
        size_t size;
        void *data;
        Eina_Content *content;
        Eina_Rw_Slice slice;

        data = ecore_win32_clipboard_get((Ecore_Win32_Window *)ee->prop.window, &size, mime_type);
        if (size != 0)
          {
             if (eina_str_has_prefix(mime_type, "text/"))
               {
                  //ensure that we always have a \0 at the end, there is no assertion that \0 is included here.
                 slice.len = size + 1;
                 slice.mem = eina_memdup(data, size, EINA_TRUE);
                 free(data);
               }
             else
               {
                  slice.len = size;
                  slice.mem = data;
               }
             content = eina_content_new(eina_rw_slice_slice_get(slice), mime_type);
             free(slice.mem); //memory got duplicated in eina_content_new
             if (!content) // construction can fail because of some validation reasons
               eina_promise_reject(promise, ecore_evas_no_matching_type);
             else
               eina_promise_resolve(promise, eina_value_content_init(content));
          }
        else
          eina_promise_reject(promise, ecore_evas_no_matching_type);
     }
   return future;
}

/**
 * @brief Checks if a selection (clipboard) has an owner.
 *
 * On Win32, the copy/paste buffer is considered to always have an owner
 * (the system or another application).
 *
 * @param ee The Ecore_Evas instance (unused).
 * @param seat The seat identifier (unused on Win32).
 * @param selection The selection buffer.
 * @return EINA_TRUE if the selection is COPY_AND_PASTE_BUFFER, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_evas_win32_selection_has_owner(Ecore_Evas *ee EINA_UNUSED, unsigned int seat EINA_UNUSED, Ecore_Evas_Selection_Buffer selection)
{
   return (selection == ECORE_EVAS_SELECTION_BUFFER_COPY_AND_PASTE_BUFFER);
}

/**
 * @brief Structure defining the Win32 Ecore_Evas engine functions.
 *
 * This structure maps generic Ecore_Evas operations to their
 * Win32-specific implementations.
 */
static Ecore_Evas_Engine_Func _ecore_win32_engine_func =
{
   _ecore_evas_win32_free,
   NULL,
   NULL,
   NULL,
   NULL,
   _ecore_evas_win32_callback_delete_request_set,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   _ecore_evas_win32_move,
   NULL,
   _ecore_evas_win32_resize,
   _ecore_evas_win32_move_resize,
   _ecore_evas_win32_rotation_set,
   _ecore_evas_win32_shaped_set,
   _ecore_evas_win32_show,
   _ecore_evas_win32_hide,
   _ecore_evas_win32_raise,
   _ecore_evas_win32_lower,
   _ecore_evas_win32_activate,
   _ecore_evas_win32_title_set,
   NULL, /* _ecore_evas_x_name_class_set */
   _ecore_evas_win32_size_min_set,
   _ecore_evas_win32_size_max_set,
   _ecore_evas_win32_size_base_set,
   _ecore_evas_win32_size_step_set,
   _ecore_evas_win32_object_cursor_set,
   _ecore_evas_win32_object_cursor_unset,
   NULL, /* _ecore_evas_x_layer_set */
   _ecore_evas_win32_focus_set,
   _ecore_evas_win32_iconified_set,
   _ecore_evas_win32_borderless_set,
   _ecore_evas_win32_override_set,
   _ecore_evas_win32_maximized_set,
   _ecore_evas_win32_fullscreen_set,
   NULL, /* _ecore_evas_x_avoid_damage_set */
   NULL, /* _ecore_evas_x_withdrawn_set */
   NULL, /* _ecore_evas_x_sticky_set */
   NULL, /* _ecore_evas_x_ignore_events_set */
   _ecore_evas_win32_alpha_set,
   NULL, //transparent
   NULL, // profiles_set
   NULL, // profile_set

   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,

   NULL, // render
   _ecore_evas_win32_screen_geometry_get,
   _ecore_evas_win32_screen_dpi_get,
   NULL,
   NULL,  // msg_send

   NULL, // pointer_xy_get
   NULL, // pointer_warp

   NULL, // wm_rot_preferred_rotation_set
   NULL, // wm_rot_available_rotations_set
   NULL, // wm_rot_manual_rotation_done_set
   NULL, // wm_rot_manual_rotation_done

   NULL, // aux_hints_set

   NULL, // fn_animator_register
   NULL, // fn_animator_unregister

   NULL, // fn_evas_changed
   NULL, //fn_focus_device_set
   NULL, //fn_callback_focus_device_in_set
   NULL, //fn_callback_focus_device_out_set
   NULL, //fn_callback_device_mouse_in_set
   NULL, //fn_callback_device_mouse_out_set
   NULL, //fn_pointer_device_xy_get
   NULL, //fn_prepare
   NULL, //fn_last_tick_get
   _ecore_evas_win32_selection_claim, //fn_selection_claim
   _ecore_evas_win32_selection_has_owner, //fn_selection_has_owner
   _ecore_evas_win32_selection_request, //fn_selection_request
   NULL, //fn_dnd_start
   NULL, //fn_dnd_stop
};

#endif /* BUILD_ECORE_EVAS_WIN32 */

/* API */

#ifdef BUILD_ECORE_EVAS_SOFTWARE_GDI
/**
 * @brief Initializes the Evas Software GDI engine for a Win32 Ecore_Evas.
 * @param ee The Ecore_Evas instance.
 * @return 1 on success, 0 on failure.
 */
static int
_ecore_evas_engine_software_gdi_init(Ecore_Evas *ee)
{
   Evas_Engine_Info_Software_Gdi *einfo;
   const char                    *driver;
   int                            rmethod;

   driver = "software_gdi";

   rmethod = evas_render_method_lookup(driver);
   if (!rmethod)
     return 0;

   ee->driver = driver;
   evas_output_method_set(ee->evas, rmethod);

   einfo = (Evas_Engine_Info_Software_Gdi *)evas_engine_info_get(ee->evas);
   if (einfo)
     {
        /* FIXME: REDRAW_DEBUG missing for now */
        einfo->info.window = ((Ecore_Win32_Window *)ee->prop.window)->window;
        einfo->info.rotation = 0;
        einfo->info.borderless = 0;
        einfo->info.fullscreen = 0;
        einfo->info.region = 0;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
             return 0;
          }
     }
   else
     {
        ERR("evas_engine_info_set() init engine '%s' failed.", ee->driver);
        return 0;
     }

   return 1;
}
#endif /* BUILD_ECORE_EVAS_SOFTWARE_GDI */

#ifdef BUILD_ECORE_EVAS_SOFTWARE_DDRAW
/**
 * @brief Initializes the Evas Software DDraw engine for a Win32 Ecore_Evas.
 * @param ee The Ecore_Evas instance.
 * @return 1 on success, 0 on failure.
 */
static int
_ecore_evas_engine_software_ddraw_init(Ecore_Evas *ee)
{
   Evas_Engine_Info_Software_DDraw *einfo;
   const char                      *driver;
   int                              rmethod;

   driver = "software_ddraw";

   rmethod = evas_render_method_lookup(driver);
   if (!rmethod)
     return 0;

   ee->driver = driver;
   evas_output_method_set(ee->evas, rmethod);

   einfo = (Evas_Engine_Info_Software_DDraw *)evas_engine_info_get(ee->evas);
   if (einfo)
     {
        /* FIXME: REDRAW_DEBUG missing for now */
        einfo->info.window = ((Ecore_Win32_Window *)ee->prop.window)->window;
        einfo->info.rotation = 0;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
             return 0;
          }
     }
   else
     {
        ERR("evas_engine_info_set() init engine '%s' failed.", ee->driver);
        return 0;
     }

   return 1;
}
#endif /* BUILD_ECORE_EVAS_SOFTWARE_DDRAW */

#ifdef BUILD_ECORE_EVAS_OPENGL_WIN32
/**
 * @brief Initializes the Evas OpenGL Win32 engine for a Win32 Ecore_Evas.
 * @param ee The Ecore_Evas instance.
 * @return 1 on success, 0 on failure.
 */
static int
_ecore_evas_engine_opengl_win32_init(Ecore_Evas *ee)
{
   Evas_Engine_Info_GL_Win32 *einfo;
   const char                *driver;
   int                        rmethod;

   driver = "gl_win32";

   rmethod = evas_render_method_lookup(driver);
   if (!rmethod)
     return 0;

   ee->driver = driver;
   evas_output_method_set(ee->evas, rmethod);

   einfo = (Evas_Engine_Info_GL_Win32 *)evas_engine_info_get(ee->evas);
   if (einfo)
     {
        /* FIXME: REDRAW_DEBUG missing for now */
        einfo->info.window = ((Ecore_Win32_Window *)ee->prop.window)->window;
        einfo->info.rotation = 0;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
             return 0;
          }
     }
   else
     {
        ERR("evas_engine_info_set() init engine '%s' failed.", ee->driver);
        return 0;
     }

   return 1;
}
#endif /* BUILD_ECORE_EVAS_OPENGL_WIN32 */

/**
 * @brief Internal function to create a new Win32 Ecore_Evas instance.
 *
 * This function performs common initialization for all Win32 Ecore_Evas
 * backends (GDI, DDraw, OpenGL).
 *
 * @param _ecore_evas_engine_backend_init Function pointer to the specific
 *                                        engine initialization function.
 * @param parent Optional parent Ecore_Win32_Window.
 * @param x The initial X coordinate of the window.
 * @param y The initial Y coordinate of the window.
 * @param width The initial width of the window.
 * @param height The initial height of the window.
 * @return A new Ecore_Evas instance, or NULL on failure.
 */
static Ecore_Evas *
_ecore_evas_win32_new_internal(int (*_ecore_evas_engine_backend_init)(Ecore_Evas *ee),
                               Ecore_Win32_Window *parent,
                               int                 x,
                               int                 y,
                               int                 width,
                               int                 height)
{
   Ecore_Evas *ee;
   Ecore_Evas_Engine_Data_Win32 *wdata;
   Ecore_Evas_Interface_Win32 *iface;

   if (!ecore_win32_init())
     return NULL;

   ee = calloc(1, sizeof(Ecore_Evas));
   if (!ee)
     return NULL;
   wdata = calloc(1, sizeof(Ecore_Evas_Engine_Data_Win32));
   if (!wdata)
     {
	free(ee);
	return NULL;
     }

   ECORE_MAGIC_SET(ee, ECORE_MAGIC_EVAS);

   _ecore_evas_win32_init();

   ee->engine.func = (Ecore_Evas_Engine_Func *)&_ecore_win32_engine_func;
   ee->engine.data = wdata;

   iface = _ecore_evas_win32_interface_new();
   ee->engine.ifaces = eina_list_append(ee->engine.ifaces, iface);

   if (width < 1) width = 1;
   if (height < 1) height = 1;
   ee->x = x;
   ee->y = y;
   ee->w = width;
   ee->h = height;
   ee->req.x = ee->x;
   ee->req.y = ee->y;
   ee->req.w = ee->w;
   ee->req.h = ee->h;
   ee->can_async_render = EINA_FALSE;
   ee->draw_block = EINA_TRUE;

   ee->prop.max.w = 32767;
   ee->prop.max.h = 32767;
   ee->prop.layer = 4;
   ee->prop.request_pos = EINA_FALSE;
   ee->prop.sticky = EINA_FALSE;
   /* FIXME: sticky to add */
   ee->prop.window = 0;
   ee->prop.withdrawn = EINA_TRUE;

   /* init evas here */
   if (!ecore_evas_evas_new(ee, width, height))
     {
        ERR("Can not create Canvas.");
        free(ee);
        return NULL;
     }

   wdata->parent = parent;
   ee->prop.window = (Ecore_Window)ecore_win32_window_new(parent, x, y, width, height);
   if (!ee->prop.window)
     {
        _ecore_evas_win32_shutdown();
        free(ee);
        return NULL;
     }

   if (!_ecore_evas_engine_backend_init(ee))
     {
        _ecore_evas_win32_shutdown();
        free(ee);
        return NULL;
     }

   ecore_evas_done(ee, EINA_FALSE);

   return ee;
}

/**
 * @brief Creates a new Ecore_Evas instance using the Software GDI engine on Win32.
 * @param parent Optional parent Ecore_Win32_Window.
 * @param x The initial X coordinate of the window.
 * @param y The initial Y coordinate of the window.
 * @param width The initial width of the window.
 * @param height The initial height of the window.
 * @return A new Ecore_Evas instance, or NULL on failure or if GDI support is not built.
 * @ingroup Ecore_Evas_Win32_Group
 */
EMODAPI Ecore_Evas *
ecore_evas_software_gdi_new_internal(Ecore_Win32_Window *parent,
				     int                 x,
				     int                 y,
				     int                 width,
				     int                 height)
{
#ifdef BUILD_ECORE_EVAS_SOFTWARE_GDI
   return _ecore_evas_win32_new_internal(_ecore_evas_engine_software_gdi_init,
                                         parent,
                                         x,
                                         y,
                                         width,
                                         height);
#else
   (void) parent;
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   return NULL;
#endif
}

/**
 * @brief Creates a new Ecore_Evas instance using the Software DDraw engine on Win32.
 * @param parent Optional parent Ecore_Win32_Window.
 * @param x The initial X coordinate of the window.
 * @param y The initial Y coordinate of the window.
 * @param width The initial width of the window.
 * @param height The initial height of the window.
 * @return A new Ecore_Evas instance, or NULL on failure or if DDraw support is not built.
 * @ingroup Ecore_Evas_Win32_Group
 */
EMODAPI Ecore_Evas *
ecore_evas_software_ddraw_new_internal(Ecore_Win32_Window *parent,
				       int                 x,
				       int                 y,
				       int                 width,
				       int                 height)
{
#ifdef BUILD_ECORE_EVAS_SOFTWARE_DDRAW
   return _ecore_evas_win32_new_internal(_ecore_evas_engine_software_ddraw_init,
                                         parent,
                                         x,
                                         y,
                                         width,
                                         height);
#else
   (void) parent;
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   return NULL;
#endif /* ! BUILD_ECORE_EVAS_SOFTWARE_DDRAW */
}

/**
 * @brief Creates a new Ecore_Evas instance using the OpenGL engine on Win32.
 * @param parent Optional parent Ecore_Win32_Window.
 * @param x The initial X coordinate of the window.
 * @param y The initial Y coordinate of the window.
 * @param width The initial width of the window.
 * @param height The initial height of the window.
 * @return A new Ecore_Evas instance, or NULL on failure or if OpenGL support is not built.
 * @ingroup Ecore_Evas_Win32_Group
 */
EMODAPI Ecore_Evas *
ecore_evas_gl_win32_new_internal(Ecore_Win32_Window *parent,
                                 int                 x,
                                 int                 y,
                                 int                 width,
                                 int                 height)
{
#ifdef BUILD_ECORE_EVAS_OPENGL_WIN32
   return _ecore_evas_win32_new_internal(_ecore_evas_engine_opengl_win32_init,
                                         parent,
                                         x,
                                         y,
                                         width,
                                         height);
#else
   (void) parent;
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   return NULL;
#endif
}

/**
 * @brief Retrieves the native Ecore_Win32_Window from an Ecore_Evas instance.
 * @param ee The Ecore_Evas instance.
 * @return The Ecore_Win32_Window associated with the Ecore_Evas.
 * @ingroup Ecore_Evas_Win32_Group
 */
static Ecore_Win32_Window *
_ecore_evas_win32_window_get(const Ecore_Evas *ee)
{
   return (Ecore_Win32_Window *) ecore_evas_window_get(ee);
}

/**
 * @brief Creates a new Win32 Ecore_Evas interface structure.
 *
 * This interface provides Win32-specific functions, such as retrieving
 * the native window handle.
 *
 * @return A pointer to the newly allocated Ecore_Evas_Interface_Win32,
 *         or NULL on allocation failure.
 */
static Ecore_Evas_Interface_Win32 *
_ecore_evas_win32_interface_new(void)
{
   Ecore_Evas_Interface_Win32 *iface;

   iface = calloc(1, sizeof(Ecore_Evas_Interface_Win32));
   if (!iface) return NULL;

   iface->base.name = interface_win32_name;
   iface->base.version = interface_win32_version;

   iface->window_get = _ecore_evas_win32_window_get;

   return iface;
}
