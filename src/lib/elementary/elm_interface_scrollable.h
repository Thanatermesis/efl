#ifndef ELM_INTEFARCE_SCROLLER_H
#define ELM_INTEFARCE_SCROLLER_H

/**
 * @addtogroup Widget
 * @{
 *
 * @section elm-scrollable-interface The Elementary Scrollable Interface
 *
 * This is a common interface for widgets having @b scrollable views.
 * Widgets using/implementing this must use the
 * @c EVAS_SMART_SUBCLASS_IFACE_NEW macro (instead of the
 * @c EVAS_SMART_SUBCLASS_NEW one) when declaring its smart class,
 * so an interface is also declared.
 *
 * The scrollable interface comes built with Elementary and is exposed
 * as #ELM_SCROLLABLE_IFACE.
 *
 * The interface API is explained in details at
 * #Elm_Scrollable_Smart_Interface.
 *
 * An Elementary scrollable interface will handle an internal @b
 * panning object. It has the function of clipping and moving the
 * actual scrollable content around, by the command of the scrollable
 * interface calls. Though it's not the common case, one might
 * want/have to change some aspects of the internal panning object
 * behavior.  For that, we have it also exposed here --
 * #Elm_Pan_Smart_Class. Use elm_pan_smart_class_get() to build your
 * custom panning object, when creating a scrollable widget (again,
 * only if you need a custom panning object) and set it with
 * Elm_Scrollable_Smart_Interface::extern_pan_set.
 */

#include <elm_pan_eo.legacy.h>
#include <elm_scroller.h>

/**
 * Elementary scroller panning base smart data.
 */
typedef struct _Elm_Pan_Smart_Data Elm_Pan_Smart_Data;
/**
 * @struct _Elm_Pan_Smart_Data
 * @brief Structure holding the smart data for an Elm_Pan object.
 *
 * This structure contains information about the pan object itself, its content,
 * dimensions, and current position.
 */
struct _Elm_Pan_Smart_Data
{
   Evas_Object                   *self; /**< The pan Evas_Object itself. */
   Evas_Object                   *content; /**< The content Evas_Object being panned. */
   Evas_Object                   *interface_object; /**< The Evas_Object implementing the scrollable interface that uses this pan. */
   Evas_Coord                     x, y, w, h; /**< Geometry of the pan object (position and size). */
   Evas_Coord                     content_w, content_h; /**< Size of the content object. */
   Evas_Coord                     px, py; /**< Current pan (scroll) position of the content. */
};

/**
 * Elementary scrollable interface base data.
 */

/**
 * @typedef Elm_Interface_Scrollable_Cb
 * @brief Generic callback function type for scrollable interface events.
 * @param obj The Evas_Object that triggered the callback.
 * @param data User-provided data for the callback.
 */
typedef void      (*Elm_Interface_Scrollable_Cb)(Evas_Object *obj, void *data);

/**
 * @typedef Elm_Interface_Scrollable_Min_Limit_Cb
 * @brief Callback function type for content minimum limit changes.
 * @param obj The Evas_Object that triggered the callback.
 * @param w EINA_TRUE if width is limited, EINA_FALSE otherwise.
 * @param h EINA_TRUE if height is limited, EINA_FALSE otherwise.
 */
typedef void      (*Elm_Interface_Scrollable_Min_Limit_Cb)(Evas_Object *obj, Eina_Bool w, Eina_Bool h);

/**
 * @typedef Elm_Interface_Scrollable_Resize_Cb
 * @brief Callback function type for content viewport resize events.
 * @param obj The Evas_Object that triggered the callback.
 * @param w The new width of the viewport.
 * @param h The new height of the viewport.
 */
typedef void      (*Elm_Interface_Scrollable_Resize_Cb)(Evas_Object *obj, Evas_Coord w, Evas_Coord h);

typedef struct _Elm_Scrollable_Smart_Interface_Data
  Elm_Scrollable_Smart_Interface_Data;

#include "elm_interface_scrollable.eo.h"

/**
 * @struct _Elm_Scrollable_Smart_Interface_Data
 * @brief Structure holding the smart data for the scrollable interface.
 *
 * This extensive structure manages all aspects of scrolling behavior,
 * including current position, content, panning object, event handling,
 * policies, animations, and callback functions.
 */
struct _Elm_Scrollable_Smart_Interface_Data
{
   Evas_Coord                    x, y, w, h; /**< Current geometry of the scrollable widget itself. */
   Evas_Coord                    wx, wy, ww, wh; /**< Last "wanted" geometry for the content region (e.g., by elm_interface_scrollable_content_region_show). wx, wy are target content positions, ww, wh are target viewport dimensions. */

   Evas_Object                  *obj; /**< The Evas_Object implementing this scrollable interface. */
   Evas_Object                  *content; /**< The primary content object being scrolled (if not using extern_pan). */
   Evas_Object                  *pan_obj; /**< The pan object (Elm_Pan) that handles content movement and clipping. Can be internal or external. */
   Evas_Object                  *edje_obj; /**< The Edje object used for theming the scroller (e.g., scrollbars). */
   Evas_Object                  *event_rect; /**< The Evas_Object (typically a rectangle) used to capture input events for scrolling. */

   Evas_Object                  *parent_widget; /**< The parent widget of this scrollable object. */

   Elm_Scroller_Policy           hbar_flags, vbar_flags; /**< Policies for horizontal and vertical scrollbar visibility (ON, OFF, AUTO). */
   Elm_Scroller_Single_Direction one_direction_at_a_time; /**< Policy for restricting scrolling to one direction at a time (NONE, SOFT, HARD). */
   Efl_Ui_Layout_Orientation     block; /**< Flags to block scrolling in horizontal or vertical directions. */

   /**
    * @struct down
    * @brief State related to mouse/touch down events and subsequent dragging/animations.
    */
   struct
   {
      Evas_Coord x, y; /**< Initial canvas coordinates of the mouse/touch down event. */
      Evas_Coord sx, sy; /**< Initial scroll position (pan_obj position) at mouse/touch down. */
      Evas_Coord dx, dy; /**< Total displacement for momentum animation (horizontal and vertical). */
      Evas_Coord pdx, pdy; /**< Previous displacement, used for acceleration calculation. */
      Evas_Coord bx, by; /**< Current position during bounce animation. */
      Evas_Coord ax, ay; /**< Current displacement during momentum animation. */
      Evas_Coord bx0, by0; /**< Initial position before bounce starts (used with momentum). */
      Evas_Coord b0x, b0y; /**< Target position for bounce if coming from momentum. */
      Evas_Coord b2x, b2y; /**< Target stable position after bounce (e.g., edge of content). */

      /**
       * @struct history
       * @brief Stores recent mouse/touch move event coordinates and timestamps.
       * Used to calculate flick speed and direction for momentum scrolling.
       * Example: history[0] is the latest, history[59] is the oldest.
       * Each element: { .x = 100, .y = 150, .timestamp = 12345.678 }
       */
      struct
      {
         Evas_Coord x, y; /**< Canvas X coordinate of the event. */
         double     timestamp; /**< Timestamp of the event (in seconds). */
      } history[60];

      double est_timestamp_diff; /**< Estimated difference between ecore_loop_time and Evas event timestamps. */

      double          dragged_began_timestamp; /**< Timestamp when dragging officially started. */
      double          anim_start; /**< Start time for momentum animation. */
      double          anim_start2; /**< Start time for X-bounce animation. */
      double          anim_start3; /**< Start time for Y-bounce animation. */
      double          anim_dur; /**< Duration for momentum animation. */

      double          onhold_vx, onhold_vy; /**< Velocity for 'on hold' scrolling (when pointer is held near edge). */
      double          onhold_tlast; /**< Last timestamp for 'on hold' animation calculation. */
      double          onhold_vxe, onhold_vye; /**< Accumulated fractional 'on hold' scroll amount. This is used to sum up small scroll increments over time during 'on hold' scrolling, allowing for smoother animation when the calculated scroll per frame is less than a pixel. */

      double          last_time_x_wheel; /**< Timestamp of the last horizontal mouse wheel event. */
      double          last_time_y_wheel; /**< Timestamp of the last vertical mouse wheel event. */

      Evas_Coord      hold_x, hold_y; /**< Target coordinates for hold animator. */
      Evas_Coord      locked_x, locked_y; /**< Scroll position when one direction was locked. */
      int             hdir, vdir; /**< Horizontal/Vertical direction of initial drag (LEFT, RIGHT, UP, DOWN, or -1). */

      Ecore_Idle_Enterer *hold_enterer; /**< Idle enterer for smooth scrolling during drag. */

      Eina_Bool       hold_animator : 1; /**< True if the hold animator (smooth drag) is active. */
      Eina_Bool       onhold_animator : 1; /**< True if the 'on hold' edge scrolling animator is active. */
      Eina_Bool       momentum_animator : 1; /**< True if a momentum (flick) animation is active. */
      Eina_Bool       bounce_x_animator : 1; /**< True if an X-axis bounce animation is active. */
      Eina_Bool       bounce_y_animator : 1; /**< True if a Y-axis bounce animation is active. */

      Eina_Bool       last_hold_x_wheel : 1; /**< True if the last X wheel event was part of a hold sequence. */
      Eina_Bool       last_hold_y_wheel : 1; /**< True if the last Y wheel event was part of a hold sequence. */
      Eina_Bool       bounce_x_hold : 1; /**< True if X bounce is on hold (e.g., waiting for Y bounce to finish). */
      Eina_Bool       bounce_y_hold : 1; /**< True if Y bounce is on hold. */
      Eina_Bool       dragged_began : 1; /**< True if dragging has officially started (passed threshold). */
      Eina_Bool       want_dragged : 1; /**< True if a drag is intended but not yet started (waiting for threshold). */
      Eina_Bool       hold_parent : 1; /**< True if a parent widget is handling the scroll event (on hold). */
      Eina_Bool       want_reset : 1; /**< True if mouse down coordinates need reset due to hold/freeze. */
      Eina_Bool       cancelled : 1; /**< True if the current drag/scroll action was cancelled. */
      Eina_Bool       dragged : 1; /**< True if currently dragging. */
      Eina_Bool       locked : 1; /**< True if scroll direction is locked (e.g., for single direction scroll). */
      Eina_Bool       scroll : 1; /**< True if a scroll event occurred (used for event flags). */
      Eina_Bool       dir_x : 1; /**< True if horizontal scrolling is allowed for the current drag. */
      Eina_Bool       dir_y : 1; /**< True if vertical scrolling is allowed for the current drag. */
      Eina_Bool       hold : 1; /**< True if a hold event occurred (used for event flags). */
      Eina_Bool       now : 1; /**< True if mouse/touch is currently pressed down. */
   } down;

   /**
    * @struct content_info
    * @brief Information about the scrollable content's size.
    */
   struct
   {
      Evas_Coord w, h; /**< Current width and height of the content. */
      Eina_Bool  resized : 1; /**< Flag indicating if content was resized and needs wanted region update. */
   } content_info;

   /**
    * @struct step
    * @brief Step size for scrolling (e.g., via arrow keys or discrete wheel).
    */
   struct
   {
      Evas_Coord x, y; /**< Horizontal and vertical step size in pixels. */
   } step;

   /**
    * @struct page
    * @brief Page size for scrolling (e.g., via PageUp/PageDown or scrollbar clicks).
    * Can be absolute pixels or relative to viewport size (if negative).
    * Example: .x = 100 (100 pixels), .x = -50 (50% of viewport width).
    */
   struct
   {
      Evas_Coord x, y; /**< Horizontal and vertical page size. */
   } page;

   /**
    * @struct current_page
    * @brief Stores the top-left coordinates of the currently displayed page when paging is active.
    * This is used to detect page changes.
    */
   struct
   {
      Evas_Coord x, y; /**< X and Y coordinates of the current page's top-left corner. */
   } current_page;

   /**
    * @struct cb_func
    * @brief Collection of callback functions for various scroll events.
    */
   struct
   {
      Elm_Interface_Scrollable_Cb drag_start; /**< Called when dragging starts. */
      Elm_Interface_Scrollable_Cb drag_stop; /**< Called when dragging stops. */
      Elm_Interface_Scrollable_Cb animate_start; /**< Called when any animation (momentum, bounce, scroll_to) starts. */
      Elm_Interface_Scrollable_Cb animate_stop; /**< Called when any animation stops. */
      Elm_Interface_Scrollable_Cb scroll; /**< Called on any scroll movement. */
      Elm_Interface_Scrollable_Cb scroll_left; /**< Called when scrolled to the left. */
      Elm_Interface_Scrollable_Cb scroll_right; /**< Called when scrolled to the right. */
      Elm_Interface_Scrollable_Cb scroll_up; /**< Called when scrolled up. */
      Elm_Interface_Scrollable_Cb scroll_down; /**< Called when scrolled down. */
      Elm_Interface_Scrollable_Cb edge_left; /**< Called when the left edge is reached. */
      Elm_Interface_Scrollable_Cb edge_right; /**< Called when the right edge is reached. */
      Elm_Interface_Scrollable_Cb edge_top; /**< Called when the top edge is reached. */
      Elm_Interface_Scrollable_Cb edge_bottom; /**< Called when the bottom edge is reached. */
      Elm_Interface_Scrollable_Cb vbar_drag; /**< Called when the vertical scrollbar is dragged. */
      Elm_Interface_Scrollable_Cb vbar_press; /**< Called when the vertical scrollbar is pressed. */
      Elm_Interface_Scrollable_Cb vbar_unpress; /**< Called when the vertical scrollbar is unpressed. */
      Elm_Interface_Scrollable_Cb hbar_drag; /**< Called when the horizontal scrollbar is dragged. */
      Elm_Interface_Scrollable_Cb hbar_press; /**< Called when the horizontal scrollbar is pressed. */
      Elm_Interface_Scrollable_Cb hbar_unpress; /**< Called when the horizontal scrollbar is unpressed. */
      Elm_Interface_Scrollable_Cb page_change; /**< Called when the current page changes (due to paging). */

      Elm_Interface_Scrollable_Min_Limit_Cb content_min_limit; /**< Called to inform the widget about content minimum size limiting. */
      Elm_Interface_Scrollable_Resize_Cb content_viewport_resize; /**< Called when the content viewport resizes. */
   } cb_func;

   /**
    * @struct scrollto
    * @brief State for programmatic "scroll to" animations.
    */
   struct
   {
      /**
       * @struct x
       * @brief State for horizontal "scroll to" animation.
       */
      struct
      {
         Evas_Coord      start, end; /**< Start and end X coordinates for the animation. */
         double          t_start, t_end; /**< Start and end timestamps for the animation. */
         Eina_Bool       animator; /**< True if the X "scroll to" animator is active. */
      } x;
      /**
       * @struct y
       * @brief State for vertical "scroll to" animation.
       */
      struct
      {
         Evas_Coord      start, end; /**< Start and end Y coordinates for the animation. */
         double          t_start, t_end; /**< Start and end timestamps for the animation. */
         Eina_Bool       animator; /**< True if the Y "scroll to" animator is active. */
      } y;
   } scrollto;

   double     pagerel_h, pagerel_v; /**< Relative page sizes (horizontal and vertical) as a fraction of viewport size (0.0 to 1.0). */
   Evas_Coord pagesize_h, pagesize_v; /**< Absolute page sizes (horizontal and vertical) in pixels. Used if pagerel is 0. */
   int        page_limit_h, page_limit_v; /**< Limits for page scrolling (number of pages). */
   int        current_calc; /**< Stores the Evas smart objects calculation count during the last resize, to detect resize loops. */

   double       last_wheel_mul; /**< Multiplier for the last mouse wheel event, for acceleration. */
   unsigned int last_wheel; /**< Timestamp of the last mouse wheel event. */

   unsigned char size_adjust_recurse; /**< Recursion counter for _elm_scroll_scroll_bar_size_adjust to prevent infinite loops. */
   unsigned char size_count; /**< Counter for resize events within the same Evas calculation cycle. */
   void         *event_info; /**< Temporary storage for event_info, e.g., for posted wheel events. */

   double                         gravity_x, gravity_y; /**< Gravity for content positioning when viewport resizes (0.0 = top/left, 0.5 = center, 1.0 = bottom/right). */
   Evas_Coord                     prev_cw, prev_ch; /**< Previous content width and height, used with gravity. */

   Eina_Bool  size_adjust_recurse_abort : 1; /**< Flag to abort _elm_scroll_scroll_bar_size_adjust recursion if it exceeds limits. */

   Eina_Bool  momentum_animator_disabled : 1; /**< True if momentum animations are globally disabled. */
   Eina_Bool  bounce_animator_disabled : 1; /**< True if bounce animations are globally disabled. */
   Eina_Bool  page_snap_horiz : 1; /**< True to snap to page boundaries horizontally during drag/momentum. */
   Eina_Bool  page_snap_vert : 1; /**< True to snap to page boundaries vertically during drag/momentum. */
   Eina_Bool  wheel_disabled : 1; /**< True if mouse wheel scrolling is disabled. */
   Eina_Bool  hbar_visible : 1; /**< Current visibility state of the horizontal scrollbar. */
   Eina_Bool  vbar_visible : 1; /**< Current visibility state of the vertical scrollbar. */
   Eina_Bool  bounce_horiz : 1; /**< True if horizontal bouncing is allowed. */
   Eina_Bool  bounce_vert : 1; /**< True if vertical bouncing is allowed. */
   Eina_Bool  is_mirrored : 1; /**< True if the widget is in RTL (mirrored) mode. */
   Eina_Bool  extern_pan : 1; /**< True if an external pan object is being used. */
   Eina_Bool  bouncemey : 1; /**< True if a vertical bounce should occur (internal flag). */
   Eina_Bool  bouncemex : 1; /**< True if a horizontal bounce should occur (internal flag). */
   Eina_Bool  freeze : 1; /**< True to freeze scrolling. */
   Eina_Bool  freeze_want : 1; /**< Desired freeze state (used to restore after temporary unfreeze). */
   Eina_Bool  hold : 1; /**< True to hold scrolling (prevents user interaction). */
   Eina_Bool  min_w : 1; /**< True if content width is limited to viewport width (no horizontal scroll). */
   Eina_Bool  min_h : 1; /**< True if content height is limited to viewport height (no vertical scroll). */
   Eina_Bool  go_left : 1; /**< State of the "scroll left" indicator/arrow. */
   Eina_Bool  go_right : 1; /**< State of the "scroll right" indicator/arrow. */
   Eina_Bool  go_up : 1; /**< State of the "scroll up" indicator/arrow. */
   Eina_Bool  go_down : 1; /**< State of the "scroll down" indicator/arrow. */
   Eina_Bool  loop_h : 1; /**< True if horizontal content looping is enabled. */
   Eina_Bool  loop_v : 1; /**< True if vertical content looping is enabled. */

   void *manager; /**< Pointer to the Efl_Ui_Focus_Manager associated with this scrollable. */
};

/**
 * @def ELM_SCROLLABLE_CHECK(obj, ...)
 * @brief Macro to check if an object implements the scrollable interface.
 *
 * If the object @p obj does not implement #ELM_INTERFACE_SCROLLABLE_MIXIN,
 * an error is printed, and if the "ELM_ERROR_ABORT" environment variable
 * is set, the program aborts. Otherwise, it returns the provided
 * variadic arguments.
 *
 * @param obj The object to check.
 * @param ... Values to return if the check fails and not aborting.
 */
#define ELM_SCROLLABLE_CHECK(obj, ...)                                       \
                                                                             \
  if (!efl_isa(obj, ELM_INTERFACE_SCROLLABLE_MIXIN))                    \
    {                                                                        \
       ERR("The object (%p) doesn't implement the Elementary scrollable"     \
            " interface", obj);                                              \
       if (getenv("ELM_ERROR_ABORT")) abort();                               \
       return __VA_ARGS__;                                                   \
    }

#ifdef EFL_BETA_API_SUPPORT
EAPI void elm_pan_gravity_set(Elm_Pan *, double x, double) EINA_DEPRECATED;
EAPI void elm_pan_gravity_get(const Elm_Pan *, double *, double *) EINA_DEPRECATED;
#endif

/**
 * @}
 */

#endif
