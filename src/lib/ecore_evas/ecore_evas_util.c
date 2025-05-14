#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <Ecore.h>
#include <Ecore_Getopt.h>
#include "ecore_private.h"

#include "Ecore_Evas.h"
#include "ecore_evas_private.h"
#define EFL_INTERNAL_UNSTABLE
#include "Evas_Internal.h"

/**
 * @internal
 * @brief Key used to store/retrieve an associated Evas_Object from an Ecore_Evas instance
 *        or an Ecore_Evas from an Evas_Object instance using evas_object_data_set/get
 *        and ecore_evas_data_set/get.
 */
static const char ASSOCIATE_KEY[] = "__Ecore_Evas_Associate";

static void _ecore_evas_object_associate(Ecore_Evas *ee, Evas_Object *obj, Ecore_Evas_Object_Associate_Flags flags);
static void _ecore_evas_object_dissociate(Ecore_Evas *ee, Evas_Object *obj);

/**
 * @internal
 * @brief Retrieves the Evas_Object associated with the given Ecore_Evas.
 * @param ee The Ecore_Evas instance.
 * @return The associated Evas_Object, or NULL if not associated.
 */
static Evas_Object *
_ecore_evas_associate_get(const Ecore_Evas *ee)
{
   return ecore_evas_data_get(ee, ASSOCIATE_KEY);
}

/**
 * @internal
 * @brief Associates an Evas_Object with the given Ecore_Evas.
 * @param ee The Ecore_Evas instance.
 * @param obj The Evas_Object to associate.
 */
static void
_ecore_evas_associate_set(Ecore_Evas *ee, Evas_Object *obj)
{
   ecore_evas_data_set(ee, ASSOCIATE_KEY, obj);
}

/**
 * @internal
 * @brief Removes the Evas_Object association from the given Ecore_Evas.
 * @param ee The Ecore_Evas instance.
 */
static void
_ecore_evas_associate_del(Ecore_Evas *ee)
{
   ecore_evas_data_set(ee, ASSOCIATE_KEY, NULL);
}

/**
 * @internal
 * @brief Retrieves the Ecore_Evas associated with the given Evas_Object.
 * @param obj The Evas_Object instance.
 * @return The associated Ecore_Evas, or NULL if not associated.
 */
static Ecore_Evas *
_evas_object_associate_get(const Evas_Object *obj)
{
   return evas_object_data_get(obj, ASSOCIATE_KEY);
}

/**
 * @internal
 * @brief Associates an Ecore_Evas with the given Evas_Object.
 * @param obj The Evas_Object instance.
 * @param ee The Ecore_Evas to associate.
 */
static void
_evas_object_associate_set(Evas_Object *obj, Ecore_Evas *ee)
{
   evas_object_data_set(obj, ASSOCIATE_KEY, ee);
}

/**
 * @internal
 * @brief Removes the Ecore_Evas association from the given Evas_Object.
 * @param obj The Evas_Object instance.
 */
static void
_evas_object_associate_del(Evas_Object *obj)
{
   evas_object_data_del(obj, ASSOCIATE_KEY);
}

/** Associated Events: ******************************************************/

/* Interceptors Callbacks */

/**
 * @internal
 * @brief Intercepts move events on the associated Evas_Object and propagates them to the Ecore_Evas.
 * If the Ecore_Evas is in override mode, it also moves the Evas_Object.
 */
static void
_ecore_evas_object_intercept_move(void *data, Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   Ecore_Evas *ee = data;
   // FIXME: account for frame
   ecore_evas_move(ee, x, y);
   if (ecore_evas_override_get(ee)) evas_object_move(obj, x, y);
}

/**
 * @internal
 * @brief Intercepts raise events on the associated Evas_Object and propagates them to the Ecore_Evas.
 */
static void
_ecore_evas_object_intercept_raise(void *data, Evas_Object *obj EINA_UNUSED)
{
   Ecore_Evas *ee = data;
   ecore_evas_raise(ee);
}

/**
 * @internal
 * @brief Intercepts lower events on the associated Evas_Object and propagates them to the Ecore_Evas.
 */
static void
_ecore_evas_object_intercept_lower(void *data, Evas_Object *obj EINA_UNUSED)
{
   Ecore_Evas *ee = data;
   ecore_evas_lower(ee);
}

/**
 * @internal
 * @brief Intercepts stack_above events on the associated Evas_Object. Currently a TODO.
 */
static void
_ecore_evas_object_intercept_stack_above(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, Evas_Object *above EINA_UNUSED)
{
   INF("TODO: %s", __func__);
}

/**
 * @internal
 * @brief Intercepts stack_below events on the associated Evas_Object. Currently a TODO.
 */
static void
_ecore_evas_object_intercept_stack_below(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, Evas_Object *below EINA_UNUSED)
{
   INF("TODO: %s", __func__);
}

/**
 * @internal
 * @brief Intercepts layer_set events on the associated Evas_Object and propagates them to the Ecore_Evas.
 */
static void
_ecore_evas_object_intercept_layer_set(void *data, Evas_Object *obj EINA_UNUSED, int l)
{
   Ecore_Evas *ee = data;
   ecore_evas_layer_set(ee, l);
}

/* Event Callbacks */

/**
 * @internal
 * @brief Handles EVAS_CALLBACK_SHOW on the associated Evas_Object, propagating to ecore_evas_show().
 */
static void
_ecore_evas_object_callback_show(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee = data;
   ecore_evas_show(ee);
}

/**
 * @internal
 * @brief Handles EVAS_CALLBACK_HIDE on the associated Evas_Object, propagating to ecore_evas_hide().
 */
static void
_ecore_evas_object_callback_hide(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee = data;
   ecore_evas_hide(ee);
}

/**
 * @internal
 * @brief Handles EVAS_CALLBACK_RESIZE on the associated Evas_Object, propagating to ecore_evas_resize().
 */
static void
_ecore_evas_object_callback_resize(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee = data;
   Evas_Coord ow, oh;

   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   ecore_evas_resize(ee, ow, oh);
}

/**
 * @internal
 * @brief Handles EVAS_CALLBACK_CHANGED_SIZE_HINTS on the associated Evas_Object.
 * Propagates min/max size hints from the Evas_Object to the Ecore_Evas.
 */
static void
_ecore_evas_object_callback_changed_size_hints(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee = data;
   Evas_Coord w, h;

   evas_object_size_hint_combined_min_get(obj, &w, &h);
   ecore_evas_size_min_set(ee, w, h);

   evas_object_size_hint_combined_max_get(obj, &w, &h);
   if (w < 1) w = -1; // evas uses 0 for no max, ecore_evas uses -1
   if (h < 1) h = -1; // evas uses 0 for no max, ecore_evas uses -1
   ecore_evas_size_max_set(ee, w, h);
}

/**
 * @internal
 * @brief Handles EVAS_CALLBACK_DEL on the associated Evas_Object.
 * This version dissociates the object and then frees the Ecore_Evas.
 * Used when ECORE_EVAS_OBJECT_ASSOCIATE_DEL flag is set.
 */
static void
_ecore_evas_object_callback_del(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee = data;
   _ecore_evas_object_dissociate(ee, obj);
   ecore_evas_free(ee);
}

/**
 * @internal
 * @brief Handles EVAS_CALLBACK_DEL on the associated Evas_Object.
 * This version only dissociates the object. The Ecore_Evas is not freed here.
 * Used when ECORE_EVAS_OBJECT_ASSOCIATE_DEL flag is NOT set.
 */
static void
_ecore_evas_object_callback_del_dissociate(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee = data;
   _ecore_evas_object_dissociate(ee, obj);
}

/**
 * @internal
 * @brief Callback for Ecore_Evas delete request.
 * Dissociates and deletes the associated Evas_Object, then frees the Ecore_Evas.
 * This is set when ECORE_EVAS_OBJECT_ASSOCIATE_DEL flag is used.
 */
static void
_ecore_evas_delete_request(Ecore_Evas *ee)
{
   Evas_Object *obj = _ecore_evas_associate_get(ee);
   _ecore_evas_object_dissociate(ee, obj); // obj might be NULL if already deleted/dissociated
   if (obj) evas_object_del(obj);
   ecore_evas_free(ee);
}

/**
 * @internal
 * @brief Callback for Ecore_Evas destruction.
 * Dissociates and deletes the associated Evas_Object if it exists.
 * This is set when ECORE_EVAS_OBJECT_ASSOCIATE_DEL flag is used.
 */
static void
_ecore_evas_destroy(Ecore_Evas *ee)
{
   Evas_Object *obj = _ecore_evas_associate_get(ee);
   if (!obj)
     return;
   _ecore_evas_object_dissociate(ee, obj);
   evas_object_del(obj);
}

/**
 * @internal
 * @brief Callback for Ecore_Evas resize.
 * Resizes the associated Evas_Object to match the Ecore_Evas geometry.
 */
static void
_ecore_evas_resize(Ecore_Evas *ee)
{
   Evas_Object *obj = _ecore_evas_associate_get(ee);
   Evas_Coord w, h;
   ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);
   if (obj) evas_object_resize(obj, w, h); // Check obj as it might be gone
}

/**
 * @internal
 * @brief Callback for Ecore_Evas pre-free.
 * Dissociates and deletes the associated Evas_Object if it exists.
 * This is always set when an object is associated.
 */
static void
_ecore_evas_pre_free(Ecore_Evas *ee)
{
   Evas_Object *obj = _ecore_evas_associate_get(ee);
   if (!obj)
     return;
   _ecore_evas_object_dissociate(ee, obj);
   evas_object_del(obj);
}

/**
 * @internal
 * @brief Checks if the Evas canvas of the Evas_Object matches the Evas canvas of the Ecore_Evas.
 * @param function Name of the calling function (for error reporting).
 * @param ee The Ecore_Evas instance.
 * @param obj The Evas_Object instance.
 * @return 1 (EINA_TRUE) if evas instances match, 0 (EINA_FALSE) otherwise.
 * Logs an error and may abort if ECORE_ERROR_ABORT is set and canvases do not match.
 */
static int
_ecore_evas_object_evas_check(const char *function EINA_UNUSED, const Ecore_Evas *ee, const Evas_Object *obj)
{
   const char *name, *type;
   Evas *e;

   e = evas_object_evas_get(obj);
   if (e == ee->evas)
     return 1;

   name = evas_object_name_get(obj);
   type = evas_object_type_get(obj);

   ERR("ERROR: %s(): object %p (name=\"%s\", type=\"%s\") evas "
       "is not the same as this Ecore_Evas evas: %p != %p",
       function, obj,
       name ? name : "", type ? type : "", e, ee->evas);
   fflush(stderr);
   if (getenv("ECORE_ERROR_ABORT")) abort();

   return 0;
}

EAPI Eina_Bool
ecore_evas_object_associate(Ecore_Evas *ee, Evas_Object *obj, Ecore_Evas_Object_Associate_Flags flags)
{
   Ecore_Evas *old_ee;
   Evas_Object *old_obj;

   if (!ECORE_MAGIC_CHECK(ee, ECORE_MAGIC_EVAS))
   {
      ECORE_MAGIC_FAIL(ee, ECORE_MAGIC_EVAS, __func__);
      return EINA_FALSE;
   }

   CHECK_PARAM_POINTER_RETURN("obj", obj, EINA_FALSE);
   if (!_ecore_evas_object_evas_check(__func__, ee, obj))
     return EINA_FALSE;

   old_ee = _evas_object_associate_get(obj);
   if (old_ee)
     ecore_evas_object_dissociate(old_ee, obj);

   old_obj = _ecore_evas_associate_get(ee);
   if (old_obj)
     ecore_evas_object_dissociate(ee, old_obj);

   _ecore_evas_object_associate(ee, obj, flags);
   return EINA_TRUE;
}

EAPI Eina_Bool
ecore_evas_object_dissociate(Ecore_Evas *ee, Evas_Object *obj)
{
   Ecore_Evas *old_ee;
   Evas_Object *old_obj;

   if (!ECORE_MAGIC_CHECK(ee, ECORE_MAGIC_EVAS))
   {
      ECORE_MAGIC_FAIL(ee, ECORE_MAGIC_EVAS, __func__);
      return EINA_FALSE;
   }

   CHECK_PARAM_POINTER_RETURN("obj", obj, EINA_FALSE);
   old_ee = _evas_object_associate_get(obj);
   if (ee != old_ee) {
      ERR("ERROR: trying to dissociate object that is not using "
          "this Ecore_Evas: %p != %p", ee, old_ee);
      return EINA_FALSE;
   }

   old_obj = _ecore_evas_associate_get(ee);
   if (old_obj != obj) {
      ERR("ERROR: trying to dissociate object that is not being "
          "used by this Ecore_Evas: %p != %p", old_obj, obj);
      return EINA_FALSE;
   }

   _ecore_evas_object_dissociate(ee, obj);

   return EINA_TRUE;
}

EAPI Evas_Object *
ecore_evas_object_associate_get(const Ecore_Evas *ee)
{
   if (!ECORE_MAGIC_CHECK(ee, ECORE_MAGIC_EVAS))
   {
      ECORE_MAGIC_FAIL(ee, ECORE_MAGIC_EVAS, __func__);
      return NULL;
   }
   return _ecore_evas_associate_get(ee);
}

/**
 * @internal
 * @brief Core logic to associate an Evas_Object with an Ecore_Evas.
 * Sets up various event callbacks and interceptors on the Evas_Object to synchronize
 * its state with the Ecore_Evas, and vice-versa. Also sets up Ecore_Evas lifecycle
 * callbacks to manage the associated object.
 *
 * @param ee The Ecore_Evas to associate with.
 * @param obj The Evas_Object to associate.
 * @param flags Flags controlling the association behavior (e.g., deletion handling, event propagation).
 * @see Ecore_Evas_Object_Associate_Flags
 */
static void
_ecore_evas_object_associate(Ecore_Evas *ee, Evas_Object *obj, Ecore_Evas_Object_Associate_Flags flags)
{
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_SHOW,
      _ecore_evas_object_callback_show, ee);
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_HIDE,
      _ecore_evas_object_callback_hide, ee);
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_RESIZE,
      _ecore_evas_object_callback_resize, ee);
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
      _ecore_evas_object_callback_changed_size_hints, ee);
   if (flags & ECORE_EVAS_OBJECT_ASSOCIATE_DEL)
     evas_object_event_callback_add
       (obj, EVAS_CALLBACK_DEL, _ecore_evas_object_callback_del, ee);
   else
     evas_object_event_callback_add
       (obj, EVAS_CALLBACK_DEL, _ecore_evas_object_callback_del_dissociate, ee);

   evas_object_intercept_move_callback_add
     (obj, _ecore_evas_object_intercept_move, ee);

   if (flags & ECORE_EVAS_OBJECT_ASSOCIATE_STACK)
     {
        evas_object_intercept_raise_callback_add
          (obj, _ecore_evas_object_intercept_raise, ee);
        evas_object_intercept_lower_callback_add
          (obj, _ecore_evas_object_intercept_lower, ee);
        evas_object_intercept_stack_above_callback_add
          (obj, _ecore_evas_object_intercept_stack_above, ee);
        evas_object_intercept_stack_below_callback_add
          (obj, _ecore_evas_object_intercept_stack_below, ee);
     }

   if (flags & ECORE_EVAS_OBJECT_ASSOCIATE_LAYER)
     evas_object_intercept_layer_set_callback_add
       (obj, _ecore_evas_object_intercept_layer_set, ee);

   if (flags & ECORE_EVAS_OBJECT_ASSOCIATE_DEL)
     {
        ecore_evas_callback_delete_request_set(ee, _ecore_evas_delete_request);
        ecore_evas_callback_destroy_set(ee, _ecore_evas_destroy);
     }
   ecore_evas_callback_pre_free_set(ee, _ecore_evas_pre_free);
   ecore_evas_callback_resize_set(ee, _ecore_evas_resize);

   _evas_object_associate_set(obj, ee);
   _ecore_evas_associate_set(ee, obj);
}

/**
 * @internal
 * @brief Core logic to dissociate an Evas_Object from an Ecore_Evas.
 * Removes all event callbacks, interceptors, and Ecore_Evas lifecycle callbacks
 * that were set up during association. Clears the association data from both
 * the Ecore_Evas and the Evas_Object.
 *
 * @param ee The Ecore_Evas to dissociate from.
 * @param obj The Evas_Object to dissociate.
 */
static void
_ecore_evas_object_dissociate(Ecore_Evas *ee, Evas_Object *obj)
{
   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_SHOW,
      _ecore_evas_object_callback_show, ee);
   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_HIDE,
      _ecore_evas_object_callback_hide, ee);
   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_RESIZE,
      _ecore_evas_object_callback_resize, ee);
   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
      _ecore_evas_object_callback_changed_size_hints, ee);
   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_DEL, _ecore_evas_object_callback_del, ee);
   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_DEL, _ecore_evas_object_callback_del_dissociate, ee);

   evas_object_intercept_move_callback_del
     (obj, _ecore_evas_object_intercept_move);

   evas_object_intercept_raise_callback_del
     (obj, _ecore_evas_object_intercept_raise);
   evas_object_intercept_lower_callback_del
     (obj, _ecore_evas_object_intercept_lower);
   evas_object_intercept_stack_above_callback_del
     (obj, _ecore_evas_object_intercept_stack_above);
   evas_object_intercept_stack_below_callback_del
     (obj, _ecore_evas_object_intercept_stack_below);

   evas_object_intercept_layer_set_callback_del
     (obj, _ecore_evas_object_intercept_layer_set);

   if (!ECORE_MAGIC_CHECK(ee, ECORE_MAGIC_EVAS))
   {
      ECORE_MAGIC_FAIL(ee, ECORE_MAGIC_EVAS, __func__);
   }
   else
   {
      if (ee->func.fn_delete_request == _ecore_evas_delete_request)
        ecore_evas_callback_delete_request_set(ee, NULL);
      if (ee->func.fn_destroy == _ecore_evas_destroy)
        ecore_evas_callback_destroy_set(ee, NULL);
      if (ee->func.fn_resize == _ecore_evas_resize)
        ecore_evas_callback_resize_set(ee, NULL);
      if (ee->func.fn_pre_free == _ecore_evas_pre_free)
        ecore_evas_callback_pre_free_set(ee, NULL);

      _ecore_evas_associate_del(ee);
   }

   _evas_object_associate_del(obj);
}

/**
 * Helper ecore_getopt callback to list available Ecore_Evas engines.
 *
 * This will list all available engines except buffer, this is useful
 * for applications to let user choose how they should create windows
 * with ecore_evas_new().
 *
 * @c callback_data value is used as @c FILE* and says where to output
 * messages, by default it is @c stdout. You can specify this value
 * with ECORE_GETOPT_CALLBACK_FULL() or ECORE_GETOPT_CALLBACK_ARGS().
 *
 * If there is a boolean storage provided, then it is marked with 1
 * when this option is executed.
 * @param parser This parameter isn't in use.
 * @param desc This parameter isn't in use.
 * @param str This parameter isn't in use.
 * @param data The data to be used.
 * @param storage The storage to be used.
 * @return The function returns 1, when storage is NULL it returns 0.
 */
unsigned char
ecore_getopt_callback_ecore_evas_list_engines(const Ecore_Getopt *parser EINA_UNUSED, const Ecore_Getopt_Desc *desc EINA_UNUSED, const char *str EINA_UNUSED, void *data, Ecore_Getopt_Value *storage)
{
   Eina_List  *lst, *n;
   const char *engine;

   if (!storage)
     {
        ERR("Storage is missing");
        return 0;
     }

   FILE *fp = data;
   if (!fp)
     fp = stdout;

   lst = ecore_evas_engines_get();

   fputs("supported engines:\n", fp);
   EINA_LIST_FOREACH(lst, n, engine)
     if (strcmp(engine, "buffer") != 0)
       fprintf(fp, "\t%s\n", engine);

   ecore_evas_engines_free(lst);

   if (storage->boolp)
     *storage->boolp = 1;

   return 1;
}
