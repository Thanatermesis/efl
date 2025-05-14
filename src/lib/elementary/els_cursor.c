#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include <Elementary_Cursor.h>

#include "elm_priv.h"

#ifdef HAVE_ELEMENTARY_X
#include <Ecore_X.h>
#include <Ecore_X_Cursor.h>
#endif

#ifdef HAVE_ELEMENTARY_WIN32
#include <Ecore_Win32.h>
#endif

#define _cursor_key "_elm_cursor"

/**
 * @struct _Cursor_Id
 * @brief Maps a cursor name to its platform-specific identifiers.
 *
 * This structure is used to associate a string-based cursor name
 * (e.g., "arrow") with its corresponding integer IDs for different
 * windowing systems like X11, Cocoa, and Win32.
 */
struct _Cursor_Id
{
   const char *name; /**< The name of the cursor, e.g., ELM_CURSOR_ARROW. */

#if defined(HAVE_ELEMENTARY_X) || defined(HAVE_ELEMENTARY_COCOA) || defined(HAVE_ELEMENTARY_WIN32)
   int id; /**< The cursor ID for X11 (from Ecore_X_Cursor.h) or Win32. */
   int cid; /**< The cursor ID for Cocoa (from Ecore_Cocoa.h). */
#endif
};

#if defined(HAVE_ELEMENTARY_X)
# if defined(HAVE_ELEMENTARY_COCOA)
#  define CURSOR(_name, _id, _cid) {_name, ECORE_X_CURSOR_##_id, _cid}
# else
#  define CURSOR(_name, _id, _cid) { _name, ECORE_X_CURSOR_##_id, -1 }
# endif
#elif defined(HAVE_ELEMENTARY_COCOA)
#  define CURSOR(_name, _id, _cid) {_name, -1, _cid}
#elif defined(HAVE_ELEMENTARY_WIN32)
#  define CURSOR(_name, _id, _cid) {_name, ECORE_WIN32_CURSOR_X11_SHAPE_##_id, -1 }
#else
#  define CURSOR(_name, _id, _cid) { _name }
#endif

#if defined(HAVE_ELEMENTARY_X) || defined(HAVE_ELEMENTARY_COCOA) || defined(HAVE_ELEMENTARY_WIN32)
/**
 * @var _cursors
 * @brief A static array of predefined cursors.
 *
 * This array holds the mappings between Elementary cursor names and their
 * native counterparts in different windowing systems (X11, Cocoa, Win32).
 * It is sorted alphabetically by cursor name to allow for efficient
 * searching using `bsearch`.
 *
 * Each element is a `_Cursor_Id` struct. For example:
 * @code
 * CURSOR(ELM_CURSOR_ARROW, ARROW, ECORE_COCOA_CURSOR_ARROW)
 * @endcode
 * This expands to:
 * @code
 * { "arrow", ECORE_X_CURSOR_ARROW, ECORE_COCOA_CURSOR_ARROW }
 * @endcode
 * under the assumption that both X and Cocoa are available. If a platform is
 * not available, its corresponding ID is set to a default value (e.g., -1).
 */
/* Please keep order in sync with Ecore_X_Cursor.h values! */
static struct _Cursor_Id _cursors[] =
{
   CURSOR(ELM_CURSOR_X                  , X                  , ECORE_COCOA_CURSOR_CROSSHAIR),
   CURSOR(ELM_CURSOR_ARROW              , ARROW              , ECORE_COCOA_CURSOR_ARROW),
   CURSOR(ELM_CURSOR_BASED_ARROW_DOWN   , BASED_ARROW_DOWN   , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_BASED_ARROW_UP     , UP                 , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_BOAT               , BOAT               , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_BOGOSITY           , BOGOSITY           , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_BOTTOM_LEFT_CORNER , BOTTOM_LEFT_CORNER , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_BOTTOM_RIGHT_CORNER, BOTTOM_RIGHT_CORNER, ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_BOTTOM_SIDE        , BOTTOM_SIDE        , ECORE_COCOA_CURSOR_RESIZE_DOWN),
   CURSOR(ELM_CURSOR_BOTTOM_TEE         , BOTTOM_TEE         , ECORE_COCOA_CURSOR_RESIZE_DOWN),
   CURSOR(ELM_CURSOR_BOX_SPIRAL         , BOX_SPIRAL         , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_CENTER_PTR         , CENTER_PTR         , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_CIRCLE             , CIRCLE             , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_CLOCK              , CLOCK              , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_COFFEE_MUG         , COFFEE_MUG         , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_CROSS              , CROSS              , ECORE_COCOA_CURSOR_CROSSHAIR),
   CURSOR(ELM_CURSOR_CROSS_REVERSE      , CROSS_REVERSE      , ECORE_COCOA_CURSOR_CROSSHAIR),
   CURSOR(ELM_CURSOR_CROSSHAIR          , CROSSHAIR          , ECORE_COCOA_CURSOR_CROSSHAIR),
   CURSOR(ELM_CURSOR_DIAMOND_CROSS      , DIAMOND_CROSS      , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_DOT                , DOT                , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_DOT_BOX_MASK       , DOT_BOX_MASK       , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_DOUBLE_ARROW       , DOUBLE_ARROW       , ECORE_COCOA_CURSOR_RESIZE_UP_DOWN),
   CURSOR(ELM_CURSOR_DRAFT_LARGE        , DRAFT_LARGE        , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_DRAFT_SMALL        , DRAFT_SMALL        , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_DRAPED_BOX         , DRAPED_BOX         , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_EXCHANGE           , EXCHANGE           , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_FLEUR              , FLEUR              , ECORE_COCOA_CURSOR_CLOSED_HAND),
   CURSOR(ELM_CURSOR_GOBBLER            , GOBBLER            , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_GUMBY              , GUMBY              , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_HAND1              , HAND1              , ECORE_COCOA_CURSOR_POINTING_HAND),
   CURSOR(ELM_CURSOR_HAND2              , HAND2              , ECORE_COCOA_CURSOR_POINTING_HAND),
   CURSOR(ELM_CURSOR_HEART              , HEART              , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_ICON               , ICON               , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_IRON_CROSS         , IRON_CROSS         , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_LEFT_PTR           , LEFT_PTR           , ECORE_COCOA_CURSOR_ARROW),
   CURSOR(ELM_CURSOR_LEFT_SIDE          , LEFT_SIDE          , ECORE_COCOA_CURSOR_RESIZE_LEFT),
   CURSOR(ELM_CURSOR_LEFT_TEE           , LEFT_TEE           , ECORE_COCOA_CURSOR_RESIZE_LEFT),
   CURSOR(ELM_CURSOR_LEFTBUTTON         , LEFTBUTTON         , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_LL_ANGLE           , LL_ANGLE           , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_LR_ANGLE           , LR_ANGLE           , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_MAN                , MAN                , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_MIDDLEBUTTON       , MIDDLEBUTTON       , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_MOUSE              , MOUSE              , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_PENCIL             , PENCIL             , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_PIRATE             , PIRATE             , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_PLUS               , PLUS               , ECORE_COCOA_CURSOR_CROSSHAIR),
   CURSOR(ELM_CURSOR_QUESTION_ARROW     , QUESTION_ARROW     , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_RIGHT_PTR          , RIGHT_PTR          , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_RIGHT_SIDE         , RIGHT_SIDE         , ECORE_COCOA_CURSOR_RESIZE_RIGHT),
   CURSOR(ELM_CURSOR_RIGHT_TEE          , RIGHT_TEE          , ECORE_COCOA_CURSOR_RESIZE_RIGHT),
   CURSOR(ELM_CURSOR_RIGHTBUTTON        , RIGHTBUTTON        , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_RTL_LOGO           , RTL_LOGO           , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_SAILBOAT           , SAILBOAT           , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_SB_DOWN_ARROW      , SB_DOWN_ARROW      , ECORE_COCOA_CURSOR_RESIZE_DOWN),
   CURSOR(ELM_CURSOR_SB_H_DOUBLE_ARROW  , SB_H_DOUBLE_ARROW  , ECORE_COCOA_CURSOR_RESIZE_LEFT_RIGHT),
   CURSOR(ELM_CURSOR_SB_LEFT_ARROW      , SB_LEFT_ARROW      , ECORE_COCOA_CURSOR_RESIZE_LEFT),
   CURSOR(ELM_CURSOR_SB_RIGHT_ARROW     , SB_RIGHT_ARROW     , ECORE_COCOA_CURSOR_RESIZE_RIGHT),
   CURSOR(ELM_CURSOR_SB_UP_ARROW        , SB_UP_ARROW        , ECORE_COCOA_CURSOR_RESIZE_UP),
   CURSOR(ELM_CURSOR_SB_V_DOUBLE_ARROW  , SB_V_DOUBLE_ARROW  , ECORE_COCOA_CURSOR_RESIZE_UP_DOWN),
   CURSOR(ELM_CURSOR_SHUTTLE            , SHUTTLE            , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_SIZING             , SIZING             , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_SPIDER             , SPIDER             , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_SPRAYCAN           , SPRAYCAN           , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_STAR               , STAR               , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_TARGET             , TARGET             , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_TCROSS             , TCROSS             , ECORE_COCOA_CURSOR_CROSSHAIR),
   CURSOR(ELM_CURSOR_TOP_LEFT_ARROW     , TOP_LEFT_ARROW     , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_TOP_LEFT_CORNER    , TOP_LEFT_CORNER    , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_TOP_RIGHT_CORNER   , TOP_RIGHT_CORNER   , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_TOP_SIDE           , TOP_SIDE           , ECORE_COCOA_CURSOR_RESIZE_UP),
   CURSOR(ELM_CURSOR_TOP_TEE            , TOP_TEE            , ECORE_COCOA_CURSOR_RESIZE_UP),
   CURSOR(ELM_CURSOR_TREK               , TREK               , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_UL_ANGLE           , UL_ANGLE           , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_UMBRELLA           , UMBRELLA           , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_UR_ANGLE           , UR_ANGLE           , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_WATCH              , WATCH              , ECORE_COCOA_CURSOR_DEFAULT),
   CURSOR(ELM_CURSOR_XTERM              , XTERM              , ECORE_COCOA_CURSOR_IBEAM)
};

static const int _cursors_count = sizeof(_cursors)/sizeof(struct _Cursor_Id);
#endif

#define ELM_CURSOR_GET_OR_RETURN(cur, obj, ...)         \
  Elm_Cursor *cur;                                      \
  do                                                    \
    {                                                   \
       if (!(obj))                                      \
         {                                              \
            CRI("Null pointer: " #obj);            \
            return __VA_ARGS__;                         \
         }                                              \
       cur = evas_object_data_get((obj), _cursor_key);  \
       if (!cur)                                        \
         {                                              \
            ERR("Object does not have cursor: " #obj);  \
            return __VA_ARGS__;                         \
         }                                              \
    }                                                   \
  while (0)

/**
 * @struct _Elm_Cursor
 * @brief Represents a cursor instance attached to an Evas object.
 *
 * This structure holds all the state for a cursor, including its visual
 * representation, hotspot coordinates, and platform-specific handles for
 * engine-based cursors.
 */
struct _Elm_Cursor
{
   Evas_Object *obj, *hotobj; /**< Themed cursor object and its hotspot part. */
   Evas_Object *eventarea, *owner; /**< The object that triggers the cursor and its owner widget. */
   const char *style, *cursor_name; /**< The cursor style and name (e.g., "default", "arrow"). */
   int hot_x, hot_y; /**< The (x, y) coordinates of the cursor's hotspot. */
   Ecore_Evas *ee; /**< The Ecore_Evas instance. */
   Evas *evas; /**< The Evas canvas. */
   Ecore_Job *hotupdate_job; /**< Job for deferred hotspot updates. */
#ifdef HAVE_ELEMENTARY_X
   struct {
     Ecore_X_Cursor cursor; /**< The X11 cursor resource. */
     Ecore_X_Window win; /**< The X11 window ID. */
   } x;
#endif
#ifdef HAVE_ELEMENTARY_WL2
   struct {
     Ecore_Wl2_Window *win; /**< The Wayland window. */
   } wl;
#endif
#ifdef HAVE_ELEMENTARY_WIN32
   struct {
     Ecore_Win32_Cursor *cursor; /**< The Win32 cursor handle. */
     Ecore_Win32_Window *win; /**< The Win32 window handle. */
   } win32;
#endif

#ifdef HAVE_ELEMENTARY_COCOA
   struct {
      Ecore_Cocoa_Cursor  cursor; /**< The Cocoa cursor object. */
      Ecore_Cocoa_Window *win; /**< The Cocoa window. */
   } cocoa;
#endif
   /**
    * @struct prev
    * @brief Holds information about the previously active cursor.
    *
    * This is used to restore the cursor when the current one is unset or
    * the mouse moves out of the event area.
    */
   struct
   {
      Evas_Object *obj; /**< The previous cursor's Evas object. */
      int layer; /**< The layer of the previous cursor object. */
      int x, y; /**< The hotspot coordinates of the previous cursor. */
   } prev;

   Eina_Bool visible:1; /**< Flag indicating if the cursor is currently visible. */
   Eina_Bool use_engine:1; /**< Flag to use engine cursor instead of themed one. */
   Eina_Bool theme_search:1; /**< Flag to enable searching for the cursor in the theme. */
};

/**
 * @internal
 * @brief Callback for when the cursor object's size hints change.
 *
 * This function ensures the cursor object has a minimum size.
 * If the size hint is smaller than 8x8, it's enforced to be 8x8.
 * This prevents the cursor from becoming too small to be visible.
 *
 * @param data The Elm_Cursor data structure.
 * @param evas The Evas canvas.
 * @param obj The Evas object whose hints changed.
 * @param event_info Event-specific information (unused).
 */
static void
_elm_cursor_obj_hints(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Elm_Cursor *cur = data;
   int x, y;

   evas_object_size_hint_min_get(cur->obj, &x, &y);
   if ((x < 8) || (y < 8))
     {
        x = 8;
        y = 8;
     }
   evas_object_resize(cur->obj, x, y);
}

/**
 * @internal
 * @brief Callback for the deletion of the cursor's Evas object.
 *
 * When the underlying Evas object for a cursor is deleted, this function
 * cleans up associated resources. It removes event callbacks and nullifies
 * the object pointer within the Elm_Cursor structure to prevent use-after-free
 * errors.
 *
 * @param data The Elm_Cursor data structure.
 * @param evas The Evas canvas.
 * @param obj The Evas object being deleted.
 * @param event_info Event-specific information (unused).
 */
static void
_elm_cursor_obj_del(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Elm_Cursor *cur = data;

   if (cur)
     {
        evas_object_event_callback_del_full(cur->obj, EVAS_CALLBACK_DEL,
                                            _elm_cursor_obj_del, cur);
        evas_object_event_callback_del_full(cur->obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                            _elm_cursor_obj_hints, cur);
        cur->obj = NULL;
        ELM_SAFE_FREE(cur->hotobj, evas_object_del);
     }
}

/**
 * @internal
 * @brief Calculates and sets the cursor's hotspot.
 *
 * The hotspot is the specific pixel of the cursor image that aligns with the
 * mouse pointer's coordinates. This function determines the hotspot position,
 * either from the center of the "hotspot" swallow part of the Edje theme or
 * from explicit "hot_x" and "hot_y" data fields in the theme.
 * If the hotspot changes, it updates the cursor in ecore_evas.
 *
 * @param cur The cursor data structure.
 */
static void
_elm_cursor_set_hot_spots(Elm_Cursor *cur)
{
   const char *str;
   Evas_Coord cx, cy, cw, ch, x, y, w, h;
   int prev_hot_x, prev_hot_y;

   if (!cur->visible) return;

   prev_hot_x = cur->hot_x;
   prev_hot_y = cur->hot_y;

   evas_object_geometry_get(cur->obj, &cx, &cy, &cw, &ch);
   evas_object_geometry_get(cur->hotobj, &x, &y, &w, &h);
   cur->hot_x = (x + (w / 2)) - cx;
   cur->hot_y = (y + (h / 2)) - cy;

   str = edje_object_data_get(cur->obj, "hot_x");
   if (str) cur->hot_x = atoi(str);
   str = edje_object_data_get(cur->obj, "hot_y");
   if (str) cur->hot_y = atoi(str);

   if ((cur->visible) &&
       ((prev_hot_x != cur->hot_x) || (prev_hot_y != cur->hot_y)))
     {
        ecore_evas_object_cursor_set(cur->ee, cur->obj,
                                     ELM_OBJECT_LAYER_CURSOR,
                                     cur->hot_x, cur->hot_y);
     }
}

/**
 * @internal
 * @brief Ecore job to update the cursor hotspot.
 *
 * This function is scheduled as an Ecore job to asynchronously update the
 * cursor's hotspot. This is useful for batching updates that might occur
 * rapidly, such as during object resizing or moving.
 *
 * @param data The Elm_Cursor data structure.
 */
static void
_elm_cursor_set_hot_spots_job(void *data)
{
   Elm_Cursor *cur = data;

   cur->hotupdate_job = NULL;

   _elm_cursor_set_hot_spots(cur);
}

/**
 * @internal
 * @brief Callback for move or resize events on the cursor or hotspot object.
 *
 * When the cursor's visual representation (or its hotspot part) is moved or
 * resized, this function is called. It schedules a job to recalculate the
 * hotspot coordinates, avoiding redundant calculations on rapid successive
 * events.
 *
 * @param data The Elm_Cursor data structure.
 * @param evas The Evas canvas.
 * @param obj The Evas object that changed.
 * @param event_info Event-specific information (unused).
 */
static void
_elm_cursor_hot_change(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Elm_Cursor *cur = data;
   if (cur->hotupdate_job) ecore_job_del(cur->hotupdate_job);
   cur->hotupdate_job = ecore_job_add(_elm_cursor_set_hot_spots_job, data);
}

/**
 * @internal
 * @brief Creates and configures the Evas object for a themed cursor.
 *
 * This function attempts to create a cursor from the theme. It creates an
 * Edje object, applies the cursor theme, and sets up a hotspot object.
 * It's used for software-rendered cursors.
 *
 * @param obj The widget owning the cursor.
 * @param cur The cursor data structure to populate.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
_elm_cursor_obj_add(Evas_Object *obj, Elm_Cursor *cur)
{
#ifdef HAVE_ELEMENTARY_WL2
   const char *engine_name;

   engine_name = ecore_evas_engine_name_get(cur->ee);
   if ((engine_name) &&
       ((!strcmp(engine_name, ELM_WAYLAND_SHM)) ||
           (!strcmp(engine_name, ELM_WAYLAND_EGL))))
     return EINA_FALSE;
#endif

   cur->obj = edje_object_add(cur->evas);
   if (!cur->obj) return EINA_FALSE;
   edje_object_freeze(cur->obj);
   edje_object_update_hints_set(cur->obj, 1);

   if (elm_widget_theme_object_set(obj, cur->obj, "cursor", cur->cursor_name,
                             cur->style ? cur->style : "default") == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     {
        ELM_SAFE_FREE(cur->obj, evas_object_del);
        return EINA_FALSE;
     }
   evas_object_data_set(cur->obj, "elm-cursor", (void*)1);
   cur->hotobj = evas_object_rectangle_add(cur->evas);
   evas_object_color_set(cur->hotobj, 0, 0, 0, 0);
   evas_object_event_callback_add(cur->obj, EVAS_CALLBACK_MOVE,
                                  _elm_cursor_hot_change, cur);
   evas_object_event_callback_add(cur->obj, EVAS_CALLBACK_RESIZE,
                                  _elm_cursor_hot_change, cur);
   evas_object_event_callback_add(cur->hotobj, EVAS_CALLBACK_MOVE,
                                  _elm_cursor_hot_change, cur);
   evas_object_event_callback_add(cur->hotobj, EVAS_CALLBACK_RESIZE,
                                  _elm_cursor_hot_change, cur);

   if (elm_widget_is_legacy(obj))
     {
        if (edje_object_part_exists(cur->obj, "elm.swallow.hotspot"))
          edje_object_part_swallow(cur->obj, "elm.swallow.hotspot", cur->hotobj);
        else if (edje_object_part_exists(cur->obj, "elm.content.hotspot"))
          edje_object_part_swallow(cur->obj, "elm.content.hotspot", cur->hotobj);
        else
          {
             ELM_SAFE_FREE(cur->hotobj, evas_object_del);
             ELM_SAFE_FREE(cur->obj, evas_object_del);
             return EINA_FALSE;
          }
     }
   else
     {
        if (edje_object_part_exists(cur->obj, "efl.hotspot"))
          edje_object_part_swallow(cur->obj, "efl.hotspot", cur->hotobj);
        else if (edje_object_part_exists(cur->obj, "efl.content.hotspot"))
          edje_object_part_swallow(cur->obj, "efl.content.hotspot", cur->hotobj);
        else
          {
             ELM_SAFE_FREE(cur->hotobj, evas_object_del);
             ELM_SAFE_FREE(cur->obj, evas_object_del);
             return EINA_FALSE;
          }
     }

   evas_object_event_callback_add(cur->obj, EVAS_CALLBACK_DEL,
                                  _elm_cursor_obj_del, cur);
   evas_object_event_callback_add(cur->obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _elm_cursor_obj_hints, cur);
   edje_object_thaw(cur->obj);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Activates the specified cursor.
 *
 * This function is the core logic for displaying a cursor. It determines
 * whether to use a software-rendered (themed) cursor or a hardware
 * (engine-provided) cursor.
 *
 * For software cursors, it uses `ecore_evas_object_cursor_set()` with the
 * themed cursor object. It also saves the previous cursor to restore it later.
 * For hardware cursors, it uses platform-specific functions
 * (e.g., `ecore_x_window_cursor_set`) to set the native window cursor.
 *
 * @param cur The cursor to be set.
 */
static void
_elm_cursor_set(Elm_Cursor *cur)
{
   evas_event_freeze(cur->evas);
   if (!cur->use_engine)
     {
        if (cur->visible) goto end;

        if (!cur->obj)
          _elm_cursor_obj_add(cur->owner, cur);
        if (cur->obj)
          {
             ecore_evas_cursor_get(cur->ee, &cur->prev.obj, &cur->prev.layer, &cur->prev.x, &cur->prev.y);
             if (cur->prev.obj)
               {
                  if (evas_object_data_get(cur->prev.obj, "elm-cursor"))
                    memset(&cur->prev, 0, sizeof(cur->prev));
                  else
                    ecore_evas_cursor_unset(cur->ee);
               }
             ecore_evas_object_cursor_set(cur->ee, cur->obj,
                                       ELM_OBJECT_LAYER_CURSOR, cur->hot_x,
                                       cur->hot_y);
          }
        cur->visible = !!cur->obj;
     }
   else
     {
        cur->visible = EINA_TRUE;
        if (cur->obj)
          {
             evas_object_del(cur->obj);
             cur->obj = NULL;
             ELM_SAFE_FREE(cur->hotobj, evas_object_del);
          }
#ifdef HAVE_ELEMENTARY_X
        if (cur->x.win)
          ecore_x_window_cursor_set(cur->x.win, cur->x.cursor);
#endif
#ifdef HAVE_ELEMENTARY_WL2
        if (cur->wl.win)
          {
             Evas_Object *top;

             top = elm_widget_top_get(cur->owner);
             if ((top) && (efl_isa(top, EFL_UI_WIN_CLASS)))
               _elm_win_wl_cursor_set(top, cur->cursor_name);
          }
#endif

#ifdef HAVE_ELEMENTARY_COCOA
        if (cur->cocoa.win)
          ecore_cocoa_window_cursor_set(cur->cocoa.win, cur->cocoa.cursor);
#endif

#ifdef HAVE_ELEMENTARY_WIN32
        if (cur->win32.win)
          ecore_win32_window_cursor_set(cur->win32.win, cur->win32.cursor);
#endif
     }
end:
   evas_event_thaw(cur->evas);
}

/**
 * @internal
 * @brief Callback for mouse-in events on the cursor's event area.
 *
 * When the mouse enters the object associated with this cursor, this function
 * is called. It triggers the display of the cursor by calling `_elm_cursor_set`.
 * It ignores events that are on hold.
 *
 * @param data The Elm_Cursor data structure.
 * @param evas The Evas canvas.
 * @param obj The object the mouse entered.
 * @param event_info The `Evas_Event_Mouse_In` event details.
 */
static void
_elm_cursor_mouse_in(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Cursor *cur = data;

   Evas_Event_Mouse_In *ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;

   _elm_cursor_set(cur);
}

/**
 * @internal
 * @brief Callback for mouse-out events on the cursor's event area.
 *
 * When the mouse leaves the object's area, this function is responsible for
 * hiding the cursor. It either restores the previous cursor (if one was saved)
 * or sets the cursor to the default for the window. If the mouse moves into a
 * parent widget that also has a custom cursor, this function will activate
 * the parent's cursor. It ignores events that are on hold.
 *
 * @param data The Elm_Cursor data structure.
 * @param evas The Evas canvas.
 * @param obj The object the mouse left.
 * @param event_info The `Evas_Event_Mouse_Out` event details.
 */
static void
_elm_cursor_mouse_out(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Object *sobj_parent;
   Elm_Cursor *pcur = NULL;
   Elm_Cursor *cur = data;

   Evas_Event_Mouse_Out *ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;

   if (!cur->visible) return;
   evas_event_freeze(cur->evas);
   cur->visible = EINA_FALSE;

   sobj_parent = evas_object_data_get(cur->eventarea, "elm-parent");
   while (sobj_parent)
     {
        pcur = evas_object_data_get((sobj_parent), _cursor_key);
        if ((pcur) && (pcur->visible)) break;
        sobj_parent = evas_object_data_get(sobj_parent, "elm-parent");
     }

   if (pcur)
     {
        pcur->visible = EINA_FALSE;
        evas_event_thaw(cur->evas);
        _elm_cursor_set(pcur);
        return;
     }

   if (!cur->use_engine)
     {
        if (cur->prev.obj)
          ecore_evas_object_cursor_set(cur->ee, cur->prev.obj, cur->prev.layer,
                                       cur->prev.x, cur->prev.y);
        else
          ecore_evas_object_cursor_set(cur->ee, NULL, ELM_OBJECT_LAYER_CURSOR,
                                       cur->hot_x, cur->hot_y);
        memset(&cur->prev, 0, sizeof(cur->prev));
     }
   else
     {
#ifdef HAVE_ELEMENTARY_X
        if (cur->x.win)
          ecore_x_window_cursor_set(cur->x.win, ECORE_X_CURSOR_X);
#endif
#ifdef HAVE_ELEMENTARY_WL2
        if (cur->wl.win)
          {
             Evas_Object *top;

             top = elm_widget_top_get(cur->owner);
             if ((top) && (efl_isa(top, EFL_UI_WIN_CLASS)))
               _elm_win_wl_cursor_set(top, NULL);
          }
#endif

#ifdef HAVE_ELEMENTARY_COCOA
        if (cur->cocoa.win)
          ecore_cocoa_window_cursor_set(cur->cocoa.win, ECORE_COCOA_CURSOR_DEFAULT);
#endif

#ifdef HAVE_ELEMENTARY_WIN32
        if (cur->win32.win)
          ecore_win32_window_cursor_set(cur->win32.win, ecore_win32_cursor_shaped_new(ECORE_WIN32_CURSOR_SHAPE_ARROW));
#endif
     }
   evas_event_thaw(cur->evas);
}

/**
 * @internal
 * @brief Callback for the deletion of the object that has a cursor.
 *
 * This is attached to the `EVAS_CALLBACK_DEL` of the `eventarea` object.
 * When this object is deleted, we must unset the cursor to clean up all
 * associated resources.
 *
 * @param data Unused.
 * @param evas The Evas canvas.
 * @param obj The object being deleted.
 * @param event_info Unused.
 */
static void
_elm_cursor_del(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   elm_object_cursor_unset(obj);
}

#if defined(HAVE_ELEMENTARY_X) || defined(HAVE_ELEMENTARY_COCOA) || defined(HAVE_ELEMENTARY_WIN32)
/**
 * @internal
 * @brief Comparison function for `bsearch` on the `_cursors` array.
 *
 * This function is used to compare two `_Cursor_Id` structures based on their
 * string names. It is a requirement for using `bsearch` to find a cursor
 * in the sorted `_cursors` array.
 *
 * @param data1 A pointer to the first `_Cursor_Id` struct.
 * @param data2 A pointer to the second `_Cursor_Id` struct.
 * @return An integer less than, equal to, or greater than zero if the first
 *         name is found, respectively, to be less than, to match, or be
 *         greater than the second.
 */
static int
_elm_cursor_strcmp(const void *data1, const void *data2)
{
   const struct _Cursor_Id *c1 = data1;
   const struct _Cursor_Id *c2 = data2;
   return strcmp (c1->name, c2->name);
}
#endif

/**
 * @internal
 * @brief Initializes and configures the cursor settings.
 *
 * This function is called when a cursor is first set on an object. It decides
 * the cursor handling strategy: either engine-only (hardware) or theme-first.
 *
 * If theme search is enabled, it tries to load a themed cursor via
 * `_elm_cursor_obj_add`. If that fails, or if engine-only is configured, it
 * falls back to using an engine cursor.
 *
 * For engine cursors, it finds the platform-specific cursor ID by searching
 * in the `_cursors` array and then caches the native window handles.
 *
 * @param cur The cursor data structure to initialize.
 */
static void
_elm_cursor_cur_set(Elm_Cursor *cur)
{
   if (!cur->theme_search)
     {
        INF("Using only engine cursors");
        cur->use_engine = EINA_TRUE;
     }
   else if (_elm_cursor_obj_add(cur->owner, cur))
     {
        _elm_cursor_set_hot_spots(cur);
        cur->use_engine = EINA_FALSE;
        elm_widget_cursor_add(cur->owner, cur);
     }
   else
     {
        INF("Cursor couldn't be found on theme: %s", cur->cursor_name);
        cur->use_engine = EINA_TRUE;
     }

#ifdef HAVE_ELEMENTARY_DRM
   const char *engine_name;

   engine_name = ecore_evas_engine_name_get(cur->ee);
   if ((engine_name) && (!strcmp(engine_name, ELM_DRM)))
     cur->use_engine = EINA_FALSE;
#endif

   if (cur->use_engine)
     {
        Evas_Object *top;

        top = elm_widget_top_get(cur->owner);
        if ((top) && (efl_isa(top, EFL_UI_WIN_CLASS)))
          {
#ifdef HAVE_ELEMENTARY_X
             cur->x.win = elm_win_xwindow_get(top);
             if (cur->x.win)
               {
                  struct _Cursor_Id *cur_id;

                  cur_id = bsearch(&(cur->cursor_name), _cursors, _cursors_count,
                                   sizeof(struct _Cursor_Id), _elm_cursor_strcmp);

                  if (!cur_id)
                    {
                       INF("X cursor couldn't be found: %s. Using default.",
                           cur->cursor_name);
                       cur->x.cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_X);
                    }
                  else
                    cur->x.cursor = ecore_x_cursor_shape_get(cur_id->id);
               }
#endif

#ifdef HAVE_ELEMENTARY_COCOA
             cur->cocoa.win = elm_win_cocoa_window_get(top);
             if (cur->cocoa.win)
               {
                  struct _Cursor_Id *cur_id;

                  cur_id = bsearch(&(cur->cursor_name), _cursors, _cursors_count,
                                   sizeof(struct _Cursor_Id), _elm_cursor_strcmp);
                  if (!cur_id)
                    {
                       INF("Cocoa Cursor couldn't be found: %s. Using default...",
                           cur->cursor_name);
                       cur->cocoa.cursor = ECORE_COCOA_CURSOR_DEFAULT;
                    }
                  else
                    cur->cocoa.cursor = cur_id->cid;
               }
#endif
#ifdef HAVE_ELEMENTARY_WL2
             cur->wl.win = elm_win_wl_window_get(top);
#endif
#ifdef HAVE_ELEMENTARY_WIN32
             cur->win32.win = elm_win_win32_window_get(top);
             if (cur->win32.win)
               {
                  struct _Cursor_Id *cur_id;

                  cur_id = bsearch(&(cur->cursor_name), _cursors, _cursors_count,
                                   sizeof(struct _Cursor_Id), _elm_cursor_strcmp);

                  if (!cur_id)
                    {
                       INF("Win32 X cursor couldn't be found: %s. Using default.",
                           cur->cursor_name);
                       cur->win32.cursor = ecore_win32_cursor_shaped_new(ECORE_WIN32_CURSOR_SHAPE_ARROW);
                    }
                  else
                    cur->win32.cursor = (Ecore_Win32_Cursor *)ecore_win32_cursor_x11_shaped_get(cur_id->id);
               }
#endif
          }
     }

   if (efl_canvas_pointer_inside_get(cur->eventarea, NULL))
     _elm_cursor_set(cur);
}

/**
 * Set the cursor to be shown when mouse is over the object
 *
 * Set the cursor that will be displayed when mouse is over the
 * object. The object can have only one cursor set to it, so if
 * this function is called twice for an object, the previous set
 * will be unset.
 * If using X cursors, a definition of all the valid cursor names
 * is listed on Elementary_Cursors.h. If an invalid name is set
 * the default cursor will be used.
 *
 * This is an internal function that is used by objects with sub-items
 * that want to provide different cursors for each of them. The @a
 * owner object should be an elm_widget and will be used to track
 * theme changes and to feed @a func and @a del_cb. The @a eventarea
 * may be any object and is the one that should be used later on with
 * elm_object_cursor apis, such as elm_object_cursor_unset().
 *
 * @param eventarea the object being attached a cursor.
 * @param owner the elm_widget that owns this object, will be used to
 *        track theme changes and to be used in @a func or @a del_cb.
 * @param cursor the cursor name to be used.
 *
 * @internal
 * @ingroup Elm_Cursors
 */
void
elm_object_sub_cursor_set(Evas_Object *eventarea, Evas_Object *owner, const char *cursor)
{
   Elm_Cursor *cur = NULL;

   cur = evas_object_data_get(eventarea, _cursor_key);
   if (cur)
     elm_object_cursor_unset(eventarea);

   if (!cursor) return;

   cur = ELM_NEW(Elm_Cursor);
   if (!cur) return;

   cur->owner = owner;
   cur->eventarea = eventarea;
   cur->theme_search = !_elm_config->cursor_engine_only;
   cur->visible = EINA_FALSE;
   cur->style = eina_stringshare_add("default");

   cur->cursor_name = eina_stringshare_add(cursor);
   if (!cur->cursor_name)
     ERR("Could not store cursor name %s", cursor);

   cur->evas = evas_object_evas_get(eventarea);
   cur->ee = ecore_evas_ecore_evas_get(cur->evas);

   _elm_cursor_cur_set(cur);

   evas_object_data_set(eventarea, _cursor_key, cur);

   evas_object_event_callback_add(eventarea, EVAS_CALLBACK_MOUSE_IN,
                                  _elm_cursor_mouse_in, cur);
   evas_object_event_callback_add(eventarea, EVAS_CALLBACK_MOUSE_OUT,
                                  _elm_cursor_mouse_out, cur);
   evas_object_event_callback_add(eventarea, EVAS_CALLBACK_DEL,
                                  _elm_cursor_del, cur);
}

EOLIAN Eina_Bool
_efl_ui_widget_cursor_set(Evas_Object *obj, Elm_Widget_Smart_Data *pd EINA_UNUSED,
                                     const char *cursor)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, EINA_FALSE);
   elm_object_sub_cursor_set(obj, obj, cursor);
   return EINA_TRUE;
}

const char *
elm_object_sub_cursor_get(const Evas_Object *obj)
{
   ELM_CURSOR_GET_OR_RETURN(cur, obj, NULL);
   return cur->cursor_name;
}

EOLIAN const char *
_efl_ui_widget_cursor_get(const Evas_Object *obj, Elm_Widget_Smart_Data *pd EINA_UNUSED)
{
   return elm_object_sub_cursor_get(obj);
}

EAPI void
elm_object_cursor_unset(Evas_Object *obj)
{
   ELM_CURSOR_GET_OR_RETURN(cur, obj);

   eina_stringshare_del(cur->cursor_name);
   cur->cursor_name = NULL;
   eina_stringshare_del(cur->style);
   cur->style = NULL;

   if (cur->owner)
     elm_widget_cursor_del(cur->owner, cur);

   if (cur->obj)
     {
        evas_object_event_callback_del_full(cur->obj, EVAS_CALLBACK_DEL,
                                            _elm_cursor_obj_del, cur);
        evas_object_event_callback_del_full(cur->obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                            _elm_cursor_obj_hints, cur);
        ELM_SAFE_FREE(cur->obj, evas_object_del);
     }

   if (cur->visible)
     {
        if (!cur->use_engine)
          ecore_evas_object_cursor_set(cur->ee, NULL, ELM_OBJECT_LAYER_CURSOR,
                                       cur->hot_x, cur->hot_y);
#ifdef HAVE_ELEMENTARY_X
        else if (cur->x.win)
          ecore_x_window_cursor_set(cur->x.win, ECORE_X_CURSOR_X);
#endif
#ifdef HAVE_ELEMENTARY_COCOA
        else if (cur->cocoa.win)
          ecore_cocoa_window_cursor_set(cur->cocoa.win, ECORE_COCOA_CURSOR_DEFAULT);
#endif
#ifdef HAVE_ELEMENTARY_WL2
        else if (cur->wl.win)
          {
             Evas_Object *top;

             top = elm_widget_top_get(cur->owner);
             if ((top) && (efl_isa(top, EFL_UI_WIN_CLASS)))
               _elm_win_wl_cursor_set(top, NULL);
          }
#endif
#ifdef HAVE_ELEMENTARY_WIN32
        else
          ecore_win32_window_cursor_set(cur->win32.win, ecore_win32_cursor_shaped_new(ECORE_WIN32_CURSOR_SHAPE_ARROW));
#endif
     }

   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_MOUSE_IN,
                                  _elm_cursor_mouse_in, cur);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_MOUSE_OUT,
                                  _elm_cursor_mouse_out, cur);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_DEL,
                                       _elm_cursor_del, cur);
   evas_object_data_del(obj, _cursor_key);

   if (cur->hotupdate_job) ecore_job_del(cur->hotupdate_job);
   cur->hotupdate_job = NULL;

   free(cur);
}

Eina_Bool
elm_object_sub_cursor_style_set(Evas_Object *obj, const char *style)
{
   ELM_CURSOR_GET_OR_RETURN(cur, obj, EINA_FALSE);

   if (!eina_stringshare_replace(&cur->style, style))
     ERR("Could not set current style=%s", style);

   if (cur->use_engine) return EINA_FALSE;

   if (!cur->obj)
     {
        if (!_elm_cursor_obj_add(cur->owner, cur))
          {
             ERR("Could not create cursor object");
             return EINA_FALSE;
          }
        else
          _elm_cursor_set_hot_spots(cur);
     }
   else
     {
        if (elm_widget_theme_object_set(obj, cur->obj, "cursor", cur->cursor_name,
                                   style) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
          {
             ERR("Could not apply the theme to the cursor style=%s", style);
             return EINA_FALSE;
          }
        else
          _elm_cursor_set_hot_spots(cur);
     }

   return EINA_TRUE;
}

EOLIAN Eina_Bool
_efl_ui_widget_cursor_style_set(Evas_Object *obj, Elm_Widget_Smart_Data *pd EINA_UNUSED,
                                           const char *style)
{
   return elm_object_sub_cursor_style_set(obj, style);
}

const char *
elm_object_sub_cursor_style_get(const Evas_Object *obj)
{
   ELM_CURSOR_GET_OR_RETURN(cur, obj, NULL);
   return cur->style ? cur->style : "default";
}

EOLIAN const char *
_efl_ui_widget_cursor_style_get(const Evas_Object *obj, Elm_Widget_Smart_Data *pd EINA_UNUSED)
{
   return elm_object_sub_cursor_style_get(obj);
}

/**
 * Notify cursor should recalculate its theme.
 * @internal
 */
void
elm_cursor_theme(Elm_Cursor *cur)
{
   if ((!cur) || (!cur->obj)) return;
   if (elm_widget_theme_object_set(cur->owner, cur->obj, "cursor",
                              cur->cursor_name, cur->style) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     ERR("Could not apply the theme to the cursor style=%s", cur->style);
   else
     _elm_cursor_set_hot_spots(cur);
}

Eina_Bool
elm_object_sub_cursor_theme_search_enabled_set(Evas_Object *obj, Eina_Bool theme_search)
{
   ELM_CURSOR_GET_OR_RETURN(cur, obj, EINA_FALSE);
   cur->theme_search = theme_search;
   ELM_SAFE_FREE(cur->obj, evas_object_del);
   _elm_cursor_cur_set(cur);
   return EINA_TRUE;
}

EOLIAN Eina_Bool
_efl_ui_widget_cursor_theme_search_enabled_set(Evas_Object *obj, Elm_Widget_Smart_Data *pd EINA_UNUSED,
                                                             Eina_Bool theme_search)
{
   return elm_object_sub_cursor_theme_search_enabled_set(obj, theme_search);
}

Eina_Bool
elm_object_sub_cursor_theme_search_enabled_get(const Evas_Object *obj)
{
   ELM_CURSOR_GET_OR_RETURN(cur, obj, EINA_FALSE);
   return cur->theme_search;
}

EOLIAN Eina_Bool
_efl_ui_widget_cursor_theme_search_enabled_get(const Evas_Object *obj, Elm_Widget_Smart_Data *pd EINA_UNUSED)
{
   return elm_object_sub_cursor_theme_search_enabled_get(obj);
}
