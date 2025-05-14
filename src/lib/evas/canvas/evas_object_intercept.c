#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdarg.h>

#include "evas_common_private.h"
#include "evas_private.h"

/* local calls */

static void evas_object_intercept_init(Evas_Object_Protected_Data *obj);
static void evas_object_intercept_deinit(Evas_Object_Protected_Data *obj);

/**
 * @internal
 * @brief Initializes the interceptor structure for an Evas object.
 *
 * If the interceptor structure doesn't exist, this function allocates and
 * initializes it.
 *
 * @param obj The protected data of the Evas object.
 */
static void
evas_object_intercept_init(Evas_Object_Protected_Data *obj)
{
   if (!obj) return;
   if (!obj->interceptors)
     obj->interceptors = calloc(1, sizeof(Evas_Intercept_Func));
}

/**
 * @internal
 * @brief Deinitializes the interceptor structure for an Evas object.
 *
 * If the interceptor structure exists and no interceptor functions are set,
 * this function frees the interceptor structure.
 *
 * @param obj The protected data of the Evas object.
 */
static void
evas_object_intercept_deinit(Evas_Object_Protected_Data *obj)
{
   if (!obj || !obj->interceptors) return;
   if ((obj->interceptors->show.func) ||
       (obj->interceptors->hide.func) ||
       (obj->interceptors->move.func) ||
       (obj->interceptors->resize.func) ||
       (obj->interceptors->raise.func) ||
       (obj->interceptors->lower.func) ||
       (obj->interceptors->stack_above.func) ||
       (obj->interceptors->stack_below.func) ||
       (obj->interceptors->layer_set.func) ||
       (obj->interceptors->color_set.func) ||
       (obj->interceptors->clip_set.func) ||
       (obj->interceptors->clip_unset.func))
     return;
   free(obj->interceptors);
   obj->interceptors = NULL;
}

/* private calls */

/**
 * @internal
 * @brief Cleans up interceptors for an Evas object.
 *
 * This function is called when an Evas object is being deleted. It frees
 * the interceptor structure if it exists.
 *
 * @param eo_obj The Evas object.
 */
void
evas_object_intercept_cleanup(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   if (!obj) return;
   if (obj->interceptors) free(obj->interceptors);
}

#define COMMON_ARGS Evas_Object *eo_obj, Evas_Object_Protected_Data *obj
#define UNPACK_ARG1(a) , a
#define UNPACK_ARG2(a, b) , a, b
#define UNPACK_ARG4(a, b, c, d) , a, b, c, d

/**
 * @internal
 * @brief Macro to define an interceptor call function.
 *
 * This macro generates a static inline function for a specific interceptor type.
 * The generated function checks if an interceptor is set and, if so, calls it.
 * It also handles a flag to prevent re-entrant calls to the same interceptor.
 *
 * @param Type The type of the interceptor (e.g., show, hide, move).
 * @param Args The arguments for the interceptor function, including COMMON_ARGS.
 * @param ... Additional arguments to be unpacked and passed to the interceptor function.
 * @return 1 if the interceptor was called, 0 otherwise.
 */
#define EVAS_OBJECT_INTERCEPT_CALL(Type, Args, ...) \
  static inline int evas_object_intercept_call_##Type Args \
  { \
     if (!obj->interceptors) return 0; \
     if (obj->interceptors->Type.intercepted) return 0; \
     if (!obj->interceptors->Type.func) return 0; \
     obj->interceptors->Type.intercepted = EINA_TRUE; \
     obj->interceptors->Type.func(obj->interceptors->Type.data, eo_obj __VA_ARGS__); \
     if (obj->interceptors) obj->interceptors->Type.intercepted = EINA_FALSE; \
     return 1; \
  }

EVAS_OBJECT_INTERCEPT_CALL(show,        (COMMON_ARGS))
EVAS_OBJECT_INTERCEPT_CALL(hide,        (COMMON_ARGS))
EVAS_OBJECT_INTERCEPT_CALL(raise,       (COMMON_ARGS))
EVAS_OBJECT_INTERCEPT_CALL(lower,       (COMMON_ARGS))
EVAS_OBJECT_INTERCEPT_CALL(clip_unset,  (COMMON_ARGS))
EVAS_OBJECT_INTERCEPT_CALL(move,        (COMMON_ARGS, int a, int b), UNPACK_ARG2(a, b))
EVAS_OBJECT_INTERCEPT_CALL(resize,      (COMMON_ARGS, int a, int b), UNPACK_ARG2(a, b))
EVAS_OBJECT_INTERCEPT_CALL(stack_above, (COMMON_ARGS, Evas_Object *rel_to), UNPACK_ARG1(rel_to))
EVAS_OBJECT_INTERCEPT_CALL(stack_below, (COMMON_ARGS, Evas_Object *rel_to), UNPACK_ARG1(rel_to))
EVAS_OBJECT_INTERCEPT_CALL(layer_set,   (COMMON_ARGS, int l), UNPACK_ARG1(l))
EVAS_OBJECT_INTERCEPT_CALL(focus_set,   (COMMON_ARGS, int focus), UNPACK_ARG1(focus))
EVAS_OBJECT_INTERCEPT_CALL(device_focus_set,   (COMMON_ARGS, int focus, Eo *seat), UNPACK_ARG2(focus, seat))
EVAS_OBJECT_INTERCEPT_CALL(color_set,   (COMMON_ARGS, int r, int g, int b, int a), UNPACK_ARG4(r, g, b, a))
EVAS_OBJECT_INTERCEPT_CALL(clip_set,    (COMMON_ARGS, Evas_Object *clip), UNPACK_ARG1(clip))

/**
 * @internal
 * @brief Internal function to handle interceptor calls.
 *
 * This function dispatches the call to the appropriate interceptor based on
 * the callback type. It unpacks arguments from the va_list and calls the
 * generated interceptor functions (e.g., evas_object_intercept_call_show).
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param cb_type The type of the interceptor callback.
 * @param internal An internal flag, possibly to differentiate calls originating
 *                 from within Evas itself versus external API calls.
 * @param args A va_list containing the arguments for the interceptor.
 * @return EINA_TRUE if the call was blocked/handled by an interceptor, EINA_FALSE otherwise.
 */
static Eina_Bool
_evas_object_intercept_call_internal(Evas_Object *eo_obj,
                                     Evas_Object_Protected_Data *obj,
                                     Evas_Object_Intercept_Cb_Type cb_type,
                                     int internal, va_list args)
{
   Eina_Bool blocked = 0;
   Evas_Object *eo_other;
   int r, g, b, a, i, j;
   Eo *seat;

   evas_object_async_block(obj);

   switch (cb_type)
     {
      case EVAS_OBJECT_INTERCEPT_CB_VISIBLE:
        i = !!va_arg(args, int);
        if (!obj->legacy.visible_set) obj->legacy.visible_set = 1;
        if (i == obj->cur->visible)
          {
             /* If show is called during hide animation is running, then the
              * current hide animation is cancelled and show operation is
              * proceeded. */
             if ((!obj->anim_player) || (!efl_player_playing_get(obj->anim_player)))
               return 1;
          }
        if (!obj->interceptors) return 0;
        if (i) blocked = evas_object_intercept_call_show(eo_obj, obj);
        else blocked = evas_object_intercept_call_hide(eo_obj, obj);
        break;

      case EVAS_OBJECT_INTERCEPT_CB_MOVE:
        if (obj->doing.in_move > 0)
          {
             WRN("evas_object_move() called on object %p (%s) in the middle "
                 "of moving the same object", eo_obj, efl_class_name_get(eo_obj));
             return 1;
          }
        i = va_arg(args, int);
        j = va_arg(args, int);
        if (obj->interceptors)
          blocked = evas_object_intercept_call_move(eo_obj, obj, i, j);
        if (!blocked && (obj->cur->geometry.x == i) && (obj->cur->geometry.y == j))
          blocked = 1;
        break;

      case EVAS_OBJECT_INTERCEPT_CB_RESIZE:
        i = va_arg(args, int);
        j = va_arg(args, int);
        if (obj->interceptors)
          blocked = evas_object_intercept_call_resize(eo_obj, obj, i, j);
        if (!blocked && _efl_canvas_object_efl_gfx_entity_size_set_block(eo_obj, obj, i, j, internal))
          blocked = 1;
        break;

      case EVAS_OBJECT_INTERCEPT_CB_RAISE:
        if (!obj->interceptors) return 0;
        blocked = evas_object_intercept_call_raise(eo_obj, obj);
        break;

      case EVAS_OBJECT_INTERCEPT_CB_LOWER:
        if (!obj->interceptors) return 0;
        blocked = evas_object_intercept_call_lower(eo_obj, obj);
        break;

      case EVAS_OBJECT_INTERCEPT_CB_STACK_ABOVE:
        if (!obj->interceptors) return 0;
        eo_other = va_arg(args, Evas_Object *);
        blocked = evas_object_intercept_call_stack_above(eo_obj, obj, eo_other);
        break;

      case EVAS_OBJECT_INTERCEPT_CB_STACK_BELOW:
        if (!obj->interceptors) return 0;
        eo_other = va_arg(args, Evas_Object *);
        blocked = evas_object_intercept_call_stack_below(eo_obj, obj, eo_other);
        break;

      case EVAS_OBJECT_INTERCEPT_CB_LAYER_SET:
        if (!obj->interceptors) return 0;
        i = va_arg(args, int);
        blocked = evas_object_intercept_call_layer_set(eo_obj, obj, i);
        break;

      case EVAS_OBJECT_INTERCEPT_CB_FOCUS_SET:
        if (!obj->interceptors) return 0;
        i = va_arg(args, int);
        blocked = evas_object_intercept_call_focus_set(eo_obj, obj, !!i);
        break;

      case EVAS_OBJECT_INTERCEPT_CB_DEVICE_FOCUS_SET:
        if (!obj->interceptors) return 0;
        i = va_arg(args, int);
        seat = va_arg(args, Eo*);
        blocked = evas_object_intercept_call_device_focus_set(eo_obj, obj, !!i, seat);
        break;

      case EVAS_OBJECT_INTERCEPT_CB_COLOR_SET:
        if (!obj->interceptors) return 0;
        r = va_arg(args, int);
        g = va_arg(args, int);
        b = va_arg(args, int);
        a = va_arg(args, int);
        blocked = evas_object_intercept_call_color_set(eo_obj, obj, r, g, b, a);
        break;

      case EVAS_OBJECT_INTERCEPT_CB_CLIP_SET:
        eo_other = va_arg(args, Evas_Object *);
        if (eo_other)
          {
             if (!internal)
               {
                  if (_efl_canvas_object_clipper_set_block(eo_obj, obj, eo_other, NULL))
                    return 1;
               }
             if (!obj->interceptors) return 0;
             blocked = evas_object_intercept_call_clip_set(eo_obj, obj, eo_other);
          }
        else
          {
             if (!internal)
               {
                  if (_efl_canvas_object_clipper_unset_block(eo_obj, obj))
                    return 1;
               }
             if (!obj->interceptors) return 0;
             blocked = evas_object_intercept_call_clip_unset(eo_obj, obj);
          }
        break;
     }

   return blocked;
}

/* This is a legacy-only compatibility function.
 * Made public for other parts of EFL (elm, ecore_evas).
 */
/**
 * @brief Calls an interceptor function for an Evas object.
 * @deprecated This is a legacy-only compatibility function.
 *
 * This function is a wrapper around _evas_object_intercept_call_internal.
 * It is exposed for use by other parts of EFL like Elm and Ecore_Evas.
 *
 * @param eo_obj The Evas object.
 * @param cb_type The type of the interceptor callback.
 * @param internal An internal flag.
 * @param ... Variable arguments for the interceptor function.
 * @return EINA_TRUE if the call was blocked/handled by an interceptor, EINA_FALSE otherwise.
 *         Returns 1 if the object data is invalid.
 */
EVAS_API EVAS_API_WEAK Eina_Bool
_evas_object_intercept_call(Evas_Object *eo_obj, Evas_Object_Intercept_Cb_Type cb_type,
                            int internal, ...)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Eina_Bool ret;
   va_list args;

   EVAS_OBJECT_DATA_ALIVE_CHECK(obj, 1);

   va_start(args, internal);
   ret = _evas_object_intercept_call_internal(eo_obj, obj, cb_type, internal, args);
   va_end(args);

   return ret;
}

/**
 * @internal
 * @brief Calls an interceptor function using protected data.
 *
 * Similar to _evas_object_intercept_call, but takes Evas_Object_Protected_Data
 * directly. This is likely used internally within Evas where the protected data
 * is already available.
 *
 * @param obj The protected data of the Evas object.
 * @param cb_type The type of the interceptor callback.
 * @param internal An internal flag.
 * @param ... Variable arguments for the interceptor function.
 * @return EINA_TRUE if the call was blocked/handled by an interceptor, EINA_FALSE otherwise.
 *         Returns 1 if the object data is invalid.
 */
Eina_Bool
_evas_object_intercept_call_evas(Evas_Object_Protected_Data *obj,
                                 Evas_Object_Intercept_Cb_Type cb_type,
                                 int internal, ...)
{
   Eina_Bool ret;
   va_list args;

   EVAS_OBJECT_DATA_ALIVE_CHECK(obj, 1);

   va_start(args, internal);
   ret = _evas_object_intercept_call_internal(obj->object, obj, cb_type, internal, args);
   va_end(args);

   return ret;
}

/* public calls */

/**
 * @internal
 * @brief Macro to define interceptor callback add and delete functions.
 *
 * This macro generates a pair of functions for adding and deleting interceptor
 * callbacks for a specific Evas object event (e.g., show, hide, move).
 * - `evas_object_intercept_Lower_Type_callback_add()`: Adds an interceptor callback.
 * - `evas_object_intercept_Lower_Type_callback_del()`: Deletes an interceptor callback.
 *
 * @param Up_Type The type of the interceptor in UpperCamelCase (e.g., Show, Move).
 *                This is used to form the Evas_Object_Intercept_##Up_Type##_Cb type.
 * @param Lower_Type The type of the interceptor in lowercase_snake_case (e.g., show, move).
 *                   This is used in function names and accessing interceptor struct members.
 */
#define EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Up_Type, Lower_Type)      \
  EVAS_API void                                                             \
  evas_object_intercept_##Lower_Type##_callback_add(Evas_Object *eo_obj,\
                                                    Evas_Object_Intercept_##Up_Type##_Cb func, const void *data) \
  {                                                                     \
     MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);                       \
     return;                                                            \
     MAGIC_CHECK_END();                                                 \
     Evas_Object_Protected_Data *obj = efl_data_scope_safe_get(eo_obj, EFL_CANVAS_OBJECT_CLASS); \
     if ((!obj) || (!func)) return;                                     \
     evas_object_intercept_init(obj);                                   \
     if (!obj->interceptors) return;                                    \
     obj->interceptors->Lower_Type.func = func;                         \
     obj->interceptors->Lower_Type.data = (void *)data;                 \
  }                                                                     \
                                                                        \
  EVAS_API void *                                                           \
  evas_object_intercept_##Lower_Type##_callback_del(Evas_Object *eo_obj,\
                                                    Evas_Object_Intercept_##Up_Type##_Cb func) \
  {                                                                     \
     void *data;                                                        \
                                                                        \
     MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);                       \
     return NULL;                                                       \
     MAGIC_CHECK_END();                                                 \
     Evas_Object_Protected_Data *obj = efl_data_scope_safe_get(eo_obj, EFL_CANVAS_OBJECT_CLASS); \
     if ((!obj) || (!func)) return NULL;                                \
     if (!obj->interceptors) return NULL;                               \
     obj->interceptors->Lower_Type.func = NULL;                         \
     data = obj->interceptors->Lower_Type.data;                         \
     obj->interceptors->Lower_Type.data = NULL;                         \
     evas_object_intercept_deinit(obj);                                 \
     return data;                                                       \
  }

EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Show, show);
EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Hide, hide);
EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Move, move);
EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Resize, resize);
EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Raise, raise);
EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Lower, lower);
EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Stack_Above, stack_above);
EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Stack_Below, stack_below);
EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Layer_Set, layer_set);
EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Focus_Set, focus_set);
EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Device_Focus_Set, device_focus_set);
EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Color_Set, color_set);
EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Clip_Set, clip_set);
EVAS_OBJECT_INTERCEPT_CALLBACK_DEFINE(Clip_Unset, clip_unset);
