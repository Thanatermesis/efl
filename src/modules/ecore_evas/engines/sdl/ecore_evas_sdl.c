#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include <Ecore.h>
#include "ecore_private.h"
#include <Ecore_Input.h>
#include <Ecore_Input_Evas.h>
#include <Ecore_Sdl.h>
#include <Evas_Engine_Buffer.h>
#ifdef BUILD_ECORE_EVAS_OPENGL_SDL
# include <Evas_Engine_GL_SDL.h>
#endif

#include <Ecore_Evas.h>
#include "ecore_evas_private.h"

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

/*
 * SDL only handle one window at a time. That's by definition, there is nothing wrong here.
 *
 */

/* static char *ecore_evas_default_display = "0"; */
/* static Ecore_List *ecore_evas_input_devices = NULL; */

typedef struct _Ecore_Evas_SDL_Switch_Data Ecore_Evas_SDL_Switch_Data;
struct _Ecore_Evas_SDL_Switch_Data
{
   SDL_Texture *pages[2];
   SDL_Renderer *r;
   SDL_Window *w;

   unsigned char current;
};

static int                      _ecore_evas_init_count = 0;

static Ecore_Event_Handler      *ecore_evas_event_handlers[4] = {
   NULL, NULL, NULL, NULL
};

static const char               *ecore_evas_sdl_default = "EFL SDL";
static Ecore_Poller             *ecore_evas_event;
static int                      _ecore_evas_fps_debug = 0;
static int                       ecore_evas_sdl_count = 0;

/**
 * @brief Finds the Ecore_Evas instance associated with a given SDL window ID.
 *
 * @param windowID The ID of the SDL window.
 * @return A pointer to the Ecore_Evas instance if found, otherwise NULL.
 */
static Ecore_Evas *
_ecore_evas_sdl_match(unsigned int windowID)
{
   return SDL_GetWindowData(SDL_GetWindowFromID(windowID), "_Ecore_Evas");
}

/**
 * @brief Switches the rendering buffer for double-buffered setups.
 *
 * This function is called by Evas when it needs to switch to the next
 * buffer for rendering. It unlocks the current texture, copies it to the
 * renderer, presents it, then locks the next texture and returns its pixel data.
 *
 * @param data Pointer to Ecore_Evas_SDL_Switch_Data containing buffer information.
 * @param dest Unused parameter.
 * @return Pointer to the pixel data of the newly locked texture, or NULL on failure.
 */
static void *
_ecore_evas_sdl_switch_buffer(void *data, void *dest EINA_UNUSED)
{
   Ecore_Evas_SDL_Switch_Data *swd = data;
   void *pixels;
   int pitch;

   /* Push current buffer to screen */
   SDL_UnlockTexture(swd->pages[swd->current]);
   SDL_RenderCopy(swd->r, swd->pages[swd->current], NULL, NULL);
   SDL_RenderPresent(swd->r);

   /* Switch to next buffer for rendering */
   swd->current = (swd->current + 1) % 2;
   if (SDL_LockTexture(swd->pages[swd->current], NULL, &pixels, &pitch) < 0)
     return NULL;

   return pixels;
}

/**
 * @brief Handles the ECORE_SDL_EVENT_GOT_FOCUS event.
 *
 * Sets focus to the Ecore_Evas instance associated with the window that gained focus.
 *
 * @param data Unused.
 * @param type Unused.
 * @param event Pointer to Ecore_Sdl_Event_Window containing event details.
 * @return ECORE_CALLBACK_PASS_ON to allow other handlers to process the event.
 */
static Eina_Bool
_ecore_evas_sdl_event_got_focus(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Sdl_Event_Window *ev = event;
   Ecore_Evas *ee;

   ee = _ecore_evas_sdl_match(ev->windowID);
   /* pass on event */
   if (!ee) return ECORE_CALLBACK_PASS_ON;
   _ecore_evas_focus_device_set(ee, NULL, EINA_TRUE);
   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @brief Handles the ECORE_SDL_EVENT_LOST_FOCUS event.
 *
 * Removes focus from the Ecore_Evas instance associated with the window that lost focus.
 *
 * @param data Unused.
 * @param type Unused.
 * @param event Pointer to Ecore_Sdl_Event_Window containing event details.
 * @return ECORE_CALLBACK_PASS_ON to allow other handlers to process the event.
 */
static Eina_Bool
_ecore_evas_sdl_event_lost_focus(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Ecore_Sdl_Event_Window *ev = event;
   Ecore_Evas *ee;

   ee = _ecore_evas_sdl_match(ev->windowID);

   if (!ee) return ECORE_CALLBACK_PASS_ON;
   /* pass on event */
   _ecore_evas_focus_device_set(ee, NULL, EINA_FALSE);
   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @brief Handles the ECORE_SDL_EVENT_RESIZE event.
 *
 * Updates the Ecore_Evas and Evas dimensions when the SDL window is resized.
 * For buffer-based rendering, it recreates SDL textures to match the new size.
 *
 * @param data Unused.
 * @param type Unused.
 * @param event Pointer to Ecore_Sdl_Event_Video_Resize containing new dimensions.
 * @return ECORE_CALLBACK_PASS_ON to allow other handlers to process the event,
 *         or EINA_FALSE if setting new engine info fails.
 */
static Eina_Bool
_ecore_evas_sdl_event_video_resize(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Sdl_Event_Video_Resize *e;
   Ecore_Evas *ee;
   int rmethod;

   e = event;
   ee = _ecore_evas_sdl_match(e->windowID);

   if (!ee) return ECORE_CALLBACK_PASS_ON; /* pass on event */

   rmethod = evas_output_method_get(ee->evas);
   if (rmethod == evas_render_method_lookup("buffer"))
     {
        Evas_Engine_Info_Buffer *einfo;

        einfo = (Evas_Engine_Info_Buffer *) evas_engine_info_get(ee->evas);
        if (einfo)
          {
             Ecore_Evas_SDL_Switch_Data *swd = (Ecore_Evas_SDL_Switch_Data*)(ee + 1);
             void *pixels;
             int pitch;

             SDL_UnlockTexture(swd->pages[swd->current]);

             SDL_DestroyTexture(swd->pages[0]);
             SDL_DestroyTexture(swd->pages[1]);

             SDL_RenderClear(swd->r);

             swd->pages[0] = SDL_CreateTexture(swd->r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, e->w, e->h);
             swd->pages[1] = SDL_CreateTexture(swd->r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, e->w, e->h);

             SDL_LockTexture(swd->pages[swd->current], NULL, &pixels, &pitch);

             einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_RGB32;
             einfo->info.switch_data = swd;
             einfo->info.dest_buffer = pixels;
             einfo->info.dest_buffer_row_bytes = pitch;
             einfo->info.use_color_key = 0;
             einfo->info.alpha_threshold = 0;
             einfo->info.func.new_update_region = NULL;
             einfo->info.func.free_update_region = NULL;
             einfo->info.func.switch_buffer = _ecore_evas_sdl_switch_buffer;
             if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *) einfo))
               {
                  return EINA_FALSE;
               }
          }
     }

   ee->w = e->w;
   ee->h = e->h;
   ee->req.w = e->w;
   ee->req.h = e->h;

   evas_output_size_set(ee->evas, e->w, e->h);
   evas_output_viewport_set(ee->evas, 0, 0, e->w, e->h);

   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @brief Handles the ECORE_SDL_EVENT_EXPOSE event.
 *
 * Marks the entire Evas canvas as damaged to trigger a redraw when the
 * window is exposed.
 *
 * @param data Unused.
 * @param type Unused.
 * @param event Pointer to Ecore_Sdl_Event_Window containing event details.
 * @return ECORE_CALLBACK_PASS_ON to allow other handlers to process the event.
 */
static Eina_Bool
_ecore_evas_sdl_event_video_expose(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Sdl_Event_Window *ev = event;
   Ecore_Evas *ee;
   int w;
   int h;

   ee = _ecore_evas_sdl_match(ev->windowID);

   if (!ee) return ECORE_CALLBACK_PASS_ON;
   evas_output_size_get(ee->evas, &w, &h);
   evas_damage_rectangle_add(ee->evas, 0, 0, w, h);

   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @brief Ecore poller callback to feed SDL events into the Ecore event loop.
 *
 * @param data Unused.
 * @return ECORE_CALLBACK_RENEW to keep the poller active.
 */
static Eina_Bool
_ecore_evas_sdl_event(void *data EINA_UNUSED)
{
   ecore_sdl_feed_events();
   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Initializes the Ecore_Evas SDL integration.
 *
 * Sets up event handlers and a poller for SDL events.
 * Manages an initialization counter to ensure it's only fully initialized once.
 *
 * @param w Unused width parameter.
 * @param h Unused height parameter.
 * @return The current initialization count.
 */
static int
_ecore_evas_sdl_init(int w EINA_UNUSED, int h EINA_UNUSED)
{
   _ecore_evas_init_count++;
   if (_ecore_evas_init_count > 1) return _ecore_evas_init_count;

#ifndef _WIN32
   if (getenv("ECORE_EVAS_FPS_DEBUG")) _ecore_evas_fps_debug = 1;
#endif /* _WIN32 */
   // this is pretty bad: poller? and set poll time? pol time is meant to be
   // adjustable for things like polling battery state, or amoutn of spare
   // memory etc.
   //
   ecore_evas_event = ecore_poller_add(ECORE_POLLER_CORE, 1, _ecore_evas_sdl_event, NULL);
   ecore_poller_poll_interval_set(ECORE_POLLER_CORE, 0.006);
#ifndef _WIN32
   if (_ecore_evas_fps_debug) _ecore_evas_fps_debug_init();
#endif /* _WIN32 */

   ecore_event_evas_init();

   ecore_evas_event_handlers[0] = ecore_event_handler_add(ECORE_SDL_EVENT_GOT_FOCUS, _ecore_evas_sdl_event_got_focus, NULL);
   ecore_evas_event_handlers[1] = ecore_event_handler_add(ECORE_SDL_EVENT_LOST_FOCUS, _ecore_evas_sdl_event_lost_focus, NULL);
   ecore_evas_event_handlers[2] = ecore_event_handler_add(ECORE_SDL_EVENT_RESIZE, _ecore_evas_sdl_event_video_resize, NULL);
   ecore_evas_event_handlers[3] = ecore_event_handler_add(ECORE_SDL_EVENT_EXPOSE, _ecore_evas_sdl_event_video_expose, NULL);

   return _ecore_evas_init_count;
}

/**
 * @brief Shuts down the Ecore_Evas SDL integration.
 *
 * Removes event handlers and the SDL event poller.
 * Manages a shutdown counter.
 *
 * @return The current initialization count after decrementing.
 */
static int
_ecore_evas_sdl_shutdown(void)
{
   _ecore_evas_init_count--;
   if (_ecore_evas_init_count == 0)
     {
        unsigned int i;

        for (i = 0; i < sizeof (ecore_evas_event_handlers) / sizeof (Ecore_Event_Handler*); i++)
          ecore_event_handler_del(ecore_evas_event_handlers[i]);
        ecore_event_evas_shutdown();
        ecore_poller_del(ecore_evas_event);
        ecore_evas_event = NULL;
#ifndef _WIN32
        if (_ecore_evas_fps_debug) _ecore_evas_fps_debug_shutdown();
#endif /* _WIN32 */
     }
   if (_ecore_evas_init_count < 0) _ecore_evas_init_count = 0;
   return _ecore_evas_init_count;
}

/**
 * @brief Frees resources associated with an Ecore_Evas SDL instance.
 *
 * This function is part of the Ecore_Evas_Engine_Func structure and is
 * called when an Ecore_Evas instance is destroyed. It cleans up SDL
 * textures, renderer, and window, and shuts down SDL subsystems if this
 * is the last Ecore_Evas SDL instance.
 *
 * @param ee The Ecore_Evas instance to free.
 */
static void
_ecore_evas_sdl_free(Ecore_Evas *ee)
{
   Ecore_Evas_SDL_Switch_Data *swd = (Ecore_Evas_SDL_Switch_Data*) (ee + 1);

   ecore_event_window_unregister(SDL_GetWindowID(swd->w));

   if (swd->pages[swd->current])
     SDL_UnlockTexture(swd->pages[swd->current]);

   if (swd->pages[0])
     SDL_DestroyTexture(swd->pages[0]);
   if (swd->pages[1])
     SDL_DestroyTexture(swd->pages[1]);
   if (swd->r)
     SDL_DestroyRenderer(swd->r);
   if (swd->w)
     SDL_DestroyWindow(swd->w);

   _ecore_evas_sdl_shutdown();
   ecore_sdl_shutdown();
   ecore_evas_sdl_count--;

   SDL_VideoQuit();
}

/**
 * @brief Resizes the Ecore_Evas canvas and associated SDL resources.
 *
 * This function is called when the Ecore_Evas needs to be resized.
 * It updates internal dimensions, and if using the buffer engine,
 * recreates SDL textures to match the new size. It then updates
 * Evas output and viewport sizes and damages the entire canvas.
 *
 * @param ee The Ecore_Evas instance to resize.
 * @param w The new width.
 * @param h The new height.
 */
static void
_ecore_evas_resize(Ecore_Evas *ee, int w, int h)
{
   int rmethod;

   if ((w == ee->w) && (h == ee->h)) return;
   ee->req.w = w;
   ee->req.h = h;
   ee->w = w;
   ee->h = h;

   rmethod = evas_output_method_get(ee->evas);
   if (rmethod == evas_render_method_lookup("buffer"))
     {
        Evas_Engine_Info_Buffer *einfo;

        einfo = (Evas_Engine_Info_Buffer *) evas_engine_info_get(ee->evas);
        if (einfo)
          {
             Ecore_Evas_SDL_Switch_Data *swd = (Ecore_Evas_SDL_Switch_Data*)(ee + 1);
             void *pixels;
             int pitch;

             SDL_UnlockTexture(swd->pages[swd->current]);

             SDL_DestroyTexture(swd->pages[0]);
             SDL_DestroyTexture(swd->pages[1]);

             SDL_RenderClear(swd->r);

             swd->pages[0] = SDL_CreateTexture(swd->r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
             swd->pages[1] = SDL_CreateTexture(swd->r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);

             SDL_LockTexture(swd->pages[swd->current], NULL, &pixels, &pitch);

             einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_RGB32;
             einfo->info.switch_data = swd;
             einfo->info.dest_buffer = pixels;
             einfo->info.dest_buffer_row_bytes = pitch;
             einfo->info.use_color_key = 0;
             einfo->info.alpha_threshold = 0;
             einfo->info.func.new_update_region = NULL;
             einfo->info.func.free_update_region = NULL;
             einfo->info.func.switch_buffer = _ecore_evas_sdl_switch_buffer;
             if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *) einfo))
               {
                  return;
               }
          }
     }

   evas_output_size_set(ee->evas, ee->w, ee->h);
   evas_output_viewport_set(ee->evas, 0, 0, ee->w, ee->h);
   evas_damage_rectangle_add(ee->evas, 0, 0, ee->w, ee->h);

   if (ee->func.fn_resize) ee->func.fn_resize(ee);
}

/**
 * @brief Moves and resizes the Ecore_Evas.
 *
 * Updates the position and dimensions of the Ecore_Evas.
 * If the position changes, it calls the `fn_move` callback.
 * Then, it calls _ecore_evas_resize to handle the size change.
 *
 * @param ee The Ecore_Evas instance.
 * @param x The new x-coordinate.
 * @param y The new y-coordinate.
 * @param w The new width.
 * @param h The new height.
 */
static void
_ecore_evas_move_resize(Ecore_Evas *ee, int x, int y, int w, int h)
{
   if ((ee->x != x) || (ee->y != y))
     {
        ee->req.x = x;
        ee->req.y = y;
        ee->x = x;
        ee->y = y;
        if (ee->func.fn_move) ee->func.fn_move(ee);
     }
   _ecore_evas_resize(ee, w, h);
}

/**
 * @brief Shows the Ecore_Evas window.
 *
 * Marks the Ecore_Evas as not withdrawn, calls the state change callback,
 * sets focus to the Evas, and feeds a mouse_in event.
 *
 * @param ee The Ecore_Evas instance to show.
 */
static void
_ecore_evas_show(Ecore_Evas *ee)
{
   ee->prop.withdrawn = EINA_FALSE;
   if (ee->func.fn_state_change) ee->func.fn_state_change(ee);
   if (ecore_evas_focus_device_get(ee, NULL)) return;
   _ecore_evas_focus_device_set(ee, NULL, EINA_TRUE);
   evas_event_feed_mouse_in(ee->evas, (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) & 0xffffffff), NULL);
}

static Ecore_Evas_Engine_Func _ecore_sdl_engine_func =
{
   _ecore_evas_sdl_free,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   _ecore_evas_resize,
   _ecore_evas_move_resize,
   NULL,
   NULL,
   _ecore_evas_show,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
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
   NULL, // screen_geometry_get
   NULL, // screen_dpi_get
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
   NULL, //fn_selection_claim
   NULL, //fn_selection_has_owner
   NULL, //fn_selection_request
};

/**
 * @brief Internal function to create a new Ecore_Evas SDL instance.
 *
 * This function handles the common logic for creating an Ecore_Evas backed by SDL,
 * supporting different rendering methods (buffer or GL).
 *
 * @param rmethod The Evas render method ID (e.g., from evas_render_method_lookup("buffer")).
 * @param name The window title.
 * @param w The initial width of the window.
 * @param h The initial height of the window.
 * @param fullscreen Boolean, true if the window should be fullscreen.
 * @param hwsurface Boolean, (often ignored or specific to older SDL versions/engines).
 * @param noframe Boolean, (used by GL engine) true if the window should be borderless.
 * @param alpha Boolean, true if the window should support alpha.
 * @return A pointer to the newly created Ecore_Evas instance, or NULL on failure.
 */
static Ecore_Evas*
_ecore_evas_internal_sdl_new(int rmethod, const char* name, int w, int h, int fullscreen, int hwsurface, int noframe EINA_UNUSED, int alpha)
{
   Ecore_Evas_SDL_Switch_Data *swd;
   Ecore_Evas *ee;
   Eina_Bool gl = EINA_FALSE;

   if (ecore_evas_sdl_count > 0) return NULL;
   if (!name)
     name = ecore_evas_sdl_default;

   if (!ecore_sdl_init(name)) return NULL;

   if (SDL_VideoInit(NULL) != 0)
     {
        ERR("SDL Video initialization failed !");
        return NULL;
     }

   ee = calloc(1, sizeof(Ecore_Evas) + sizeof (Ecore_Evas_SDL_Switch_Data));
   if (!ee) return NULL;

   swd = (Ecore_Evas_SDL_Switch_Data*)(ee + 1);

   ECORE_MAGIC_SET(ee, ECORE_MAGIC_EVAS);

   ee->engine.func = (Ecore_Evas_Engine_Func *)&_ecore_sdl_engine_func;

   ee->driver = "sdl";
   if (name) ee->name = strdup(name);

   if (w < 1) w = 1;
   if (h < 1) h = 1;
   ee->visible = 1;
   ee->req.w = w;
   ee->req.h = h;
   ee->w = w;
   ee->h = h;

   ee->prop.max.w = 0;
   ee->prop.max.h = 0;
   ee->prop.layer = 0;
   ee->prop.borderless = EINA_TRUE;
   ee->prop.override = EINA_TRUE;
   ee->prop.maximized = EINA_TRUE;
   ee->prop.fullscreen = fullscreen;
   ee->prop.withdrawn = EINA_TRUE;
   ee->prop.sticky = EINA_FALSE;
   ee->prop.window = 0;
   ee->alpha = alpha;
   ee->prop.hwsurface = hwsurface;

   /* init evas here */
   if (!ecore_evas_evas_new(ee, w, h))
     {
        ERR("Can not create Canvas.");
        goto on_error;
     }

   evas_output_method_set(ee->evas, rmethod);

   gl = !(rmethod == evas_render_method_lookup("buffer"));
   ee->can_async_render = gl ? EINA_FALSE : EINA_TRUE;

   swd->w = SDL_CreateWindow(name,
                             SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED,
                             w, h,
                             SDL_WINDOW_RESIZABLE | (gl ? SDL_WINDOW_OPENGL : 0));
   if (!swd->w)
     {
        ERR("SDL_CreateWindow failed.");
        goto on_error;
     }

   if (!gl)
     {
        Evas_Engine_Info_Buffer *einfo;

        einfo = (Evas_Engine_Info_Buffer *) evas_engine_info_get(ee->evas);
        if (einfo)
          {
             void *pixels;
             int pitch;

             swd->r = SDL_CreateRenderer(swd->w, -1, 0);
             if (!swd->r)
               {
                  ERR("SDL_CreateRenderer failed.");
                  goto on_error;
               }

             swd->pages[0] = SDL_CreateTexture(swd->r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
             swd->pages[1] = SDL_CreateTexture(swd->r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);

             einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_RGB32;
             einfo->info.switch_data = swd;

             SDL_RenderClear(swd->r);
             if (SDL_LockTexture(swd->pages[0], NULL, &pixels, &pitch) < 0)
               {
                  ERR("SDL_LockTexture failed.");
                  goto on_error;
               }

             einfo->info.dest_buffer = pixels;
             einfo->info.dest_buffer_row_bytes = pitch;
             einfo->info.use_color_key = 0;
             einfo->info.alpha_threshold = 0;
             einfo->info.func.new_update_region = NULL;
             einfo->info.func.free_update_region = NULL;
             einfo->info.func.switch_buffer = _ecore_evas_sdl_switch_buffer;
             if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *) einfo))
               {
                  ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
                  ecore_evas_free(ee);
                  return NULL;
               }
          }
        else
          {
             ERR("evas_engine_info_set() init engine '%s' failed.", ee->driver);
             ecore_evas_free(ee);
             return NULL;
          }
     }
   else
     {
        /* FIXME */
#ifdef BUILD_ECORE_EVAS_OPENGL_SDL
        Evas_Engine_Info_GL_SDL *einfo;

        einfo = (Evas_Engine_Info_GL_SDL *) evas_engine_info_get(ee->evas);
        if (einfo)
          {
             einfo->flags.fullscreen = fullscreen;
             einfo->flags.noframe = noframe;
             einfo->window = swd->w;
             if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
               {
                  ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
                  ecore_evas_free(ee);
                  return NULL;
               }
          }
        else
          {
             ERR("evas_engine_info_set() init engine '%s' failed.", ee->driver);
             ecore_evas_free(ee);
             return NULL;
          }
#endif
     }

   _ecore_evas_sdl_init(w, h);
   ee->prop.window = SDL_GetWindowID(swd->w);

   ecore_evas_done(ee, EINA_FALSE);

   SDL_SetWindowData(swd->w, "_Ecore_Evas", ee);
   SDL_ShowCursor(SDL_ENABLE);

   _ecore_evas_focus_device_set(ee, NULL, EINA_TRUE);
   ecore_evas_sdl_count++;
   return ee;

 on_error:
   ecore_evas_free(ee);
   return NULL;
}

/**
 * @brief Creates a new Ecore_Evas SDL instance using the software buffer engine.
 * @ingroup Ecore_Evas_SDL_Group
 *
 * @param name The window title. If NULL, a default title "EFL SDL" is used.
 * @param w The initial width of the window.
 * @param h The initial height of the window.
 * @param fullscreen EINA_TRUE for fullscreen, EINA_FALSE otherwise.
 * @param hwsurface EINA_TRUE to request hardware surface (behavior may vary).
 * @param noframe EINA_TRUE for a borderless window.
 * @param alpha EINA_TRUE to enable alpha channel support for the window.
 * @return A pointer to the newly created Ecore_Evas instance, or NULL on failure.
 *
 * This function creates an Ecore_Evas that uses SDL for windowing and
 * the Evas software buffer rendering engine.
 */
EMODAPI Ecore_Evas *
ecore_evas_sdl_new_internal(const char* name, int w, int h, int fullscreen,
                            int hwsurface, int noframe, int alpha)
{
   Ecore_Evas          *ee;
   int                  rmethod;

   rmethod = evas_render_method_lookup("buffer");
   if (!rmethod) return NULL;

   ee = _ecore_evas_internal_sdl_new(rmethod, name, w, h, fullscreen, hwsurface, noframe, alpha);
   return ee;
}

/**
 * @brief Placeholder for SDL 1.6 backend (deprecated/non-functional).
 * @ingroup Ecore_Evas_SDL_Group
 *
 * @param name Unused.
 * @param w Unused.
 * @param h Unused.
 * @param fullscreen Unused.
 * @param hwsurface Unused.
 * @param noframe Unused.
 * @param alpha Unused.
 * @return Always NULL.
 *
 * This function is a stub and will print an error message.
 * It's likely a remnant from older versions or for compatibility
 * that is no longer maintained for SDL 2.
 */
EMODAPI Ecore_Evas*
ecore_evas_sdl16_new_internal(const char* name EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED, int fullscreen EINA_UNUSED, int hwsurface EINA_UNUSED, int noframe EINA_UNUSED, int alpha EINA_UNUSED)
{
   ERR("OUCH !");
   return NULL;
}

#ifdef BUILD_ECORE_EVAS_OPENGL_SDL
/**
 * @brief Creates a new Ecore_Evas SDL instance using the OpenGL SDL engine.
 * @ingroup Ecore_Evas_GL_SDL_Group
 *
 * @param name The window title. If NULL, a default title "EFL SDL" is used.
 * @param w The initial width of the window.
 * @param h The initial height of the window.
 * @param fullscreen EINA_TRUE for fullscreen, EINA_FALSE otherwise.
 * @param noframe EINA_TRUE for a borderless window.
 * @return A pointer to the newly created Ecore_Evas instance, or NULL on failure.
 *
 * This function creates an Ecore_Evas that uses SDL for windowing and
 * the Evas OpenGL SDL rendering engine.
 */
EMODAPI Ecore_Evas *
ecore_evas_gl_sdl_new_internal(const char* name, int w, int h, int fullscreen, int noframe)
{
   Ecore_Evas          *ee;
   int                  rmethod;

   rmethod = evas_render_method_lookup("gl_sdl");
   if (!rmethod) return NULL;

   ee = _ecore_evas_internal_sdl_new(rmethod, name, w, h, fullscreen, 0, noframe, 0);
   if (ee) ee->driver = "gl_sdl";
   return ee;
}
#endif
