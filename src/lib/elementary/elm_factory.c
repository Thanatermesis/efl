#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"

// FIXME: handle if canvas resizes

/**
 * @internal
 * @brief Structure to hold the private data of the factory widget.
 */
typedef struct _Widget_Data Widget_Data;

/**
 * @internal
 * @brief Private data for the factory widget.
 */
struct _Widget_Data
{
   Evas_Object *obj; /**< The factory widget itself. */
   Evas_Object *content; /**< The current content object displayed by the factory. */
   int last_calc_count; /**< The smart object calculation count at the last realize/unrealize event. Used to detect if content should be unrealized. */
   Evas_Coord maxminw, maxminh; /**< Stored maximum width and height when maxmin mode is enabled. */
   Eina_Bool eval : 1; /**< Flag to indicate if the factory needs to re-evaluate its state. */
   Eina_Bool szeval : 1; /**< Flag to indicate if the factory needs to re-evaluate its size. */
   Eina_Bool maxmin : 1; /**< Flag indicating if max/min mode is enabled. @see elm_factory_maxmin_mode_set */
};

static const char *widtype = NULL; /**< Widget type string, used for type checking. */
static void _del_hook(Evas_Object *obj); /**< Hook called when the factory object is deleted. */
static Eina_Bool _focus_next_hook(const Evas_Object *obj, Elm_Focus_Direction dir, Evas_Object **next); /**< Hook for focus handling. */
static void _sizing_eval(Evas_Object *obj); /**< Evaluates and applies size hints for the factory. */
static void _eval(Evas_Object *obj); /**< Evaluates whether the content should be realized or unrealized based on viewport intersection. */
static void _changed(Evas_Object *obj); /**< Hook called when the factory object's smart data has changed. */
static void _move(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED); /**< Callback for EVAS_CALLBACK_MOVE on the factory object. */
static void _resize(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED); /**< Callback for EVAS_CALLBACK_RESIZE on the factory object. */
static void _child_change(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED); /**< Callback for EVAS_CALLBACK_CHANGED_SIZE_HINTS on the content object. */
static void _child_del(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED); /**< Callback for EVAS_CALLBACK_DEL on the content object. */
static void _content_set_hook(Evas_Object *obj, const char *part, Evas_Object *content); /**< Hook for setting content on the factory. */
static Evas_Object *_content_get_hook(const Evas_Object *obj, const char *part); /**< Hook for getting content from the factory. */
static Evas_Object *_content_unset_hook(Evas_Object *obj, const char *part); /**< Hook for unsetting content from the factory. */

static const char SIG_REALIZE[] = "realize"; /**< Signal emitted when the factory content becomes visible/realized. */
static const char SIG_UNREALIZE[] = "unrealize"; /**< Signal emitted when the factory content becomes hidden/unrealized. */

/**
 * @internal
 * @brief Array defining the smart callbacks supported by the factory widget.
 */
static const Evas_Smart_Cb_Description _signals[] = {
   {SIG_REALIZE, ""},
   {SIG_UNREALIZE, ""},
   {NULL, NULL}
};

static int fac = 0; /**< Counter, possibly for debugging or tracking active factories. Its exact purpose is unclear from the context. */

/**
 * @internal
 * @brief Cleans up the factory widget when it's deleted.
 *
 * This function is set as the delete hook for the factory widget. It frees
 * the widget data and unreferences/deletes its content if any.
 * @param obj The factory object being deleted.
 */
static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->content)
     {
        Evas_Object *o = wd->content;

        evas_object_event_callback_del_full(o,
                                            EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                            _child_change, obj);
        evas_object_event_callback_del_full(o,
                                            EVAS_CALLBACK_DEL,
                                            _child_del, obj);
        wd->content = NULL;
        evas_object_del(o);
        fac--;
//        DBG("FAC-- = %i", fac);
     }
   free(wd);
}

/**
 * @internal
 * @brief Handles focus movement for the factory widget.
 *
 * This function is set as the focus_next hook. It attempts to pass focus
 * to the factory's content object.
 * @param obj The factory object.
 * @param dir The direction of focus change.
 * @param next Pointer to store the next focusable object.
 * @return EINA_TRUE if focus was successfully passed, EINA_FALSE otherwise.
 */
static Eina_Bool
_focus_next_hook(const Evas_Object *obj, Elm_Focus_Direction dir, Evas_Object **next)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *cur;

   if ((!wd) || (!wd->content)) return EINA_FALSE;
   cur = wd->content;
   return efl_ui_widget_focus_next_get(cur, dir, next);
}

/**
 * @internal
 * @brief Calculates and sets the size hints for the factory widget.
 *
 * This function retrieves size hints from the content object and applies them
 * to the factory. If max/min mode is enabled, it updates and uses the
 * stored maximum minimum dimensions.
 * @param obj The factory object.
 */
static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

   if (!wd) return;
   if (!wd->content) return;
   evas_object_size_hint_combined_min_get(wd->content, &minw, &minh);
   evas_object_size_hint_max_get(wd->content, &maxw, &maxh);
   if (wd->maxmin)
     {
        if (minw > wd->maxminw) wd->maxminw = minw;
        if (minh > wd->maxminh) wd->maxminh = minh;
        evas_object_size_hint_min_set(obj, wd->maxminw, wd->maxminh);
     }
   else
     {
        evas_object_size_hint_min_set(obj, minw, minh);
     }
   evas_object_size_hint_max_set(obj, maxw, maxh);
//   DBG("FAC SZ: %i %i | %i %i", minw, minh, maxw, maxh);
}

/**
 * @internal
 * @brief Evaluates if the factory's content should be realized or unrealized.
 *
 * This function checks if the factory widget intersects with the Evas canvas
 * viewport. If it intersects and has no content, it emits the "realize" signal,
 * potentially triggering content creation. If it does not intersect and has
 * content, it emits the "unrealize" signal, potentially triggering content
 * deletion, but only if the Evas smart calculation count has changed since
 * the last realization (to avoid unrealizing during animations or fast moves
 * that don't involve actual recalculations).
 * @param obj The factory object.
 */
static void
_eval(Evas_Object *obj)
{
   Evas_Coord x, y, w, h, cvx, cvy, cvw, cvh;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   evas_event_freeze(evas_object_evas_get(obj));
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   if (w < 1) w = 1;
   if (h < 1) h = 1;
   evas_output_viewport_get(evas_object_evas_get(obj),
                            &cvx, &cvy, &cvw, &cvh);
   if ((cvw < 1) || (cvh < 1)) return;
   // need some fuzz value that's beyond the current viewport
   // for now just make it the viewport * 3 in size (so 1 vp in each direction)
   /*
   cvx -= cvw;
   cvy -= cvh;
   cvw *= 3;
   cvh *= 3;
    */
   if (ELM_RECTS_INTERSECT(x, y, w, h, cvx, cvy, cvw, cvh))
     {
        if (!wd->content)
          {
//             DBG("                 + %i %i %ix%i <> %i %i %ix%i", x, y, w, h, cvx, cvy, cvw, cvh);
             evas_object_smart_callback_call(obj, SIG_REALIZE, NULL);
             if (wd->content)
               {
                  if (evas_object_smart_data_get(wd->content))
                     evas_object_smart_calculate(wd->content);
               }
             wd->last_calc_count =
                evas_smart_objects_calculate_count_get(evas_object_evas_get(obj));
          }
     }
   else
     {
        if (wd->content)
          {
             if (wd->last_calc_count !=
                evas_smart_objects_calculate_count_get(evas_object_evas_get(obj)))
                evas_object_smart_callback_call(obj, SIG_UNREALIZE, NULL);
          }
     }
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));
}

/**
 * @internal
 * @brief Handles pending evaluations for the factory.
 *
 * This function is called when the factory's smart data changes. It checks
 * flags (wd->eval, wd->szeval) to see if a full state evaluation or sizing
 * evaluation is needed and calls the respective functions.
 * @param obj The factory object.
 */
static void
_changed(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->eval)
     {
        _eval(obj);
        wd->eval = EINA_FALSE;
     }
   if (wd->szeval)
     {
        _sizing_eval(obj);
        wd->szeval = EINA_FALSE;
     }
}

/**
 * @internal
 * @brief Callback for the EVAS_CALLBACK_MOVE event on the factory object.
 *
 * Schedules a re-evaluation of the factory's state.
 * @param data User data (unused).
 * @param e The Evas canvas (unused).
 * @param obj The factory object that moved.
 * @param event_info Event specific information (unused).
 */
static void
_move(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->eval = EINA_TRUE;
   evas_object_smart_changed(obj);
}

/**
 * @internal
 * @brief Callback for the EVAS_CALLBACK_RESIZE event on the factory object.
 *
 * Schedules a re-evaluation of the factory's state.
 * @param data User data (unused).
 * @param e The Evas canvas (unused).
 * @param obj The factory object that was resized.
 * @param event_info Event specific information (unused).
 */
static void
_resize(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->eval = EINA_TRUE;
   evas_object_smart_changed(obj);
}

/**
 * @internal
 * @brief Callback for the EVAS_CALLBACK_CHANGED_SIZE_HINTS event on the content object.
 *
 * Schedules a re-evaluation of the factory's state and size.
 * @param data The factory object (passed as user data).
 * @param e The Evas canvas (unused).
 * @param obj The content object whose size hints changed (unused).
 * @param event_info Event specific information (unused).
 */
static void
_child_change(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   wd->eval = EINA_TRUE;
   wd->szeval = EINA_TRUE;
   evas_object_smart_changed(data);
}

/**
 * @internal
 * @brief Callback for the EVAS_CALLBACK_DEL event on the content object.
 *
 * This function is called when the content object is deleted externally.
 * It cleans up references to the content within the factory.
 * @param data The factory object (passed as user data).
 * @param e The Evas canvas (unused).
 * @param obj The content object being deleted.
 * @param event_info Event specific information (unused).
 */
static void
_child_del(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *fobj = data;
   Widget_Data *wd = elm_widget_data_get(fobj);
   if (!wd) return;
   if (wd->content != obj) return;
   evas_object_event_callback_del_full(wd->content,
                                       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _child_change, obj);
   evas_object_event_callback_del_full(wd->content,
                                       EVAS_CALLBACK_DEL,
                                       _child_del, obj);
   wd->content = NULL;
   fac--;
//   DBG("FAC-- = %i", fac);
}

/**
 * @internal
 * @brief Unsets (removes) content from the factory.
 *
 * This is the widget content unset hook. It removes the specified content part
 * (currently only "default" is supported). It detaches callbacks from the
 * content and clears the factory's reference to it.
 * @param obj The factory object.
 * @param part The name of the content part to unset (e.g., "default").
 * @return The Evas_Object that was unset, or NULL if no content was set for the part.
 */
static Evas_Object *
_content_unset_hook(Evas_Object *obj, const char *part)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;
   Evas_Object *content;

   if (part && strcmp(part, "default")) return NULL;
   wd = elm_widget_data_get(obj);
   if (!wd || !wd->content) return NULL;

   content = wd->content;
   evas_object_event_callback_del_full(content,
                                       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _child_change, obj);
   evas_object_event_callback_del_full(content,
                                       EVAS_CALLBACK_DEL,
                                       _child_del, obj);
   wd->content = NULL;
   fac--;
//         DBG("FAC-- = %i", fac);
   return content;
}

/**
 * @internal
 * @brief Sets content on the factory.
 *
 * This is the widget content set hook. It sets the given Evas_Object as the
 * content for the specified part (currently only "default" is supported).
 * It unsets any previous content, sets up callbacks on the new content,
 * and schedules a re-evaluation.
 * @param obj The factory object.
 * @param part The name of the content part to set (e.g., "default").
 * @param content The Evas_Object to set as content.
 */
static void
_content_set_hook(Evas_Object *obj, const char *part, Evas_Object *content)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd;
   Evas_Object *prev_content;

   if (part && strcmp(part, "default")) return;
   wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->content == content) return;

   prev_content = _content_unset_hook(obj, part);
   evas_object_del(prev_content);

   wd->content = content;
   if (!content) return;

   elm_widget_resize_object_set(obj, content);
   evas_object_event_callback_add(content, EVAS_CALLBACK_DEL, _child_del, obj);
   evas_object_event_callback_add(content, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _child_change, obj);
   wd->eval = EINA_TRUE;
   wd->szeval = EINA_TRUE;
   evas_object_smart_changed(obj);
   fac++;
}

/**
 * @internal
 * @brief Gets content from the factory.
 *
 * This is the widget content get hook. It retrieves the Evas_Object currently
 * set as content for the specified part (currently only "default" is supported).
 * @param obj The factory object.
 * @param part The name of the content part to get (e.g., "default").
 * @return The Evas_Object set as content, or NULL if none.
 */
static Evas_Object *
_content_get_hook(const Evas_Object *obj, const char *part)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd;
   if (part && strcmp(part, "default")) return NULL;
   wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->content;
}

EAPI Evas_Object *
elm_factory_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "factory");
   elm_widget_type_set(obj, "factory");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   efl_ui_widget_focus_next_hook_set(obj, _focus_next_hook);
   elm_widget_content_set_hook_set(obj, _content_set_hook);
   elm_widget_content_get_hook_set(obj, _content_get_hook);
   elm_widget_content_unset_hook_set(obj, _content_unset_hook);
   elm_widget_can_focus_set(obj, EINA_FALSE);
   elm_widget_changed_hook_set(obj, _changed);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _move, NULL);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize, NULL);

   evas_object_smart_callbacks_descriptions_set(obj, _signals);

   wd->obj = obj;
   wd->last_calc_count = -1;
   return obj;
}

EAPI void
elm_factory_maxmin_mode_set(Evas_Object *obj, Eina_Bool enabled)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->maxmin = !!enabled;
}

EAPI Eina_Bool
elm_factory_maxmin_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->maxmin;
}

EAPI void
elm_factory_maxmin_reset_set(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   wd->maxminw = 0;
   wd->maxminh = 0;
   wd->eval = EINA_TRUE;
   wd->szeval = EINA_TRUE;
   evas_object_smart_changed(obj);
}
