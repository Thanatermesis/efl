#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_ACCESS_WIDGET_ACTION_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"

#define MY_CLASS ELM_ACCESS_CLASS

#define MY_CLASS_NAME "Elm_Access"
#define MY_CLASS_NAME_LEGACY "elm_access"

/**
 * @internal
 * @struct _Func_Data
 * @brief Structure to hold callback function and user data for an access action.
 */
struct _Func_Data
{
   void                *user_data; /**< User data to be passed to the callback. */
   Elm_Access_Action_Cb cb;        /**< The callback function for the action. */
};

typedef struct _Func_Data Func_Data;

/**
 * @internal
 * @struct _Action_Info
 * @brief Structure to store action callbacks for an Evas_Object.
 */
struct _Action_Info
{
   Evas_Object      *obj;                               /**< The Evas_Object associated with these actions. */
   Func_Data         fn[ELM_ACCESS_ACTION_LAST + 1]; /**< Array of function data for each possible access action. Indexed by Elm_Access_Action_Type. */
};

typedef struct _Action_Info Action_Info;

static Eina_Bool mouse_event_enable = EINA_TRUE;
static Eina_Bool auto_highlight = EINA_FALSE;
static Elm_Access_Action_Type action_by = ELM_ACCESS_ACTION_FIRST;

static Evas_Object * _elm_access_add(Evas_Object *parent);

static void _access_object_unregister(Evas_Object *obj);

/** @internal Signal emitted when an access object is activated. */
static const char SIG_ACTIVATED[] = "access,activated";
static const Evas_Smart_Cb_Description _smart_callbacks[] =
{
   {SIG_ACTIVATED, ""},
   {NULL, NULL}
};

EOLIAN static void
_elm_access_efl_canvas_group_group_add(Eo *obj, void *_pd EINA_UNUSED)
{
   efl_canvas_group_add(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Calls the registered callback for a specific access action type.
 *
 * This function retrieves the action information associated with the object
 * and, if a callback is registered for the given action type, invokes it.
 * If @p action_info is NULL, a new Elm_Access_Action_Info struct is allocated
 * and populated.
 *
 * @param obj The Evas_Object on which the action is performed.
 * @param type The type of access action to perform.
 * @param action_info Detailed information about the action, or NULL.
 * @return @c EINA_TRUE if a callback was successfully called, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_access_action_callback_call(Evas_Object *obj,
                             Elm_Access_Action_Type type,
                             Elm_Access_Action_Info *action_info)
{
   Elm_Access_Action_Info *ai = NULL;
   Action_Info *a;
   Eina_Bool ret;

   ret = EINA_FALSE;
   a = evas_object_data_get(obj, "_elm_access_action_info");

   if (!action_info)
     {
        ai = calloc(1, sizeof(Elm_Access_Action_Info));
        action_info = ai;
     }

   action_info->action_type = type;

   if ((type == ELM_ACCESS_ACTION_HIGHLIGHT) &&
       (action_by != ELM_ACCESS_ACTION_FIRST))
     action_info->action_by = action_by;

   if (a && (a->fn[type].cb))
     ret = a->fn[type].cb(a->fn[type].user_data, obj, action_info);

   free(ai);

   return ret;
}

EOLIAN static Eina_Bool
_elm_access_efl_ui_widget_on_access_activate(Eo *obj, void *_pd EINA_UNUSED, Efl_Ui_Activate act)
{
   int type = ELM_ACCESS_ACTION_FIRST;

   Action_Info *a;
   a = evas_object_data_get(obj, "_elm_access_action_info");

   switch (act)
     {
      case EFL_UI_ACTIVATE_DEFAULT:
        type = ELM_ACCESS_ACTION_ACTIVATE;
        break;

      case EFL_UI_ACTIVATE_UP:
        type = ELM_ACCESS_ACTION_UP;
        break;

      case EFL_UI_ACTIVATE_DOWN:
        type = ELM_ACCESS_ACTION_DOWN;
        break;

      case EFL_UI_ACTIVATE_RIGHT:
        break;

      case EFL_UI_ACTIVATE_LEFT:
        break;

      case EFL_UI_ACTIVATE_BACK:
        type = ELM_ACCESS_ACTION_BACK;
        break;

      default:
        break;
     }

   if (type == ELM_ACCESS_ACTION_FIRST) return EINA_FALSE;

   /* if an access object has a callback, it would have the intention to do
      something. so, check here and return EINA_TRUE. */
   if ((a) && (type > ELM_ACCESS_ACTION_FIRST) &&
              (type < ELM_ACCESS_ACTION_LAST) &&
              (a->fn[type].cb))
     {
        _access_action_callback_call(obj, type, NULL);
        return EINA_TRUE;
     }

   /* TODO: deprecate below? */
   if (act != EFL_UI_ACTIVATE_DEFAULT) return EINA_FALSE;

   Elm_Access_Info *ac = evas_object_data_get(obj, "_elm_access");
   if (!ac) return EINA_FALSE;

   if (ac->activate)
     ac->activate(ac->activate_data, ac->part_object,
                  ac->widget_item->eo_obj);

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_elm_access_efl_ui_focus_object_on_focus_update(Eo *obj, void *_pd EINA_UNUSED)
{
   evas_object_focus_set(obj, efl_ui_focus_object_focus_get(obj));

   return EINA_TRUE;
}

/**
 * @internal
 * @struct _Mod_Api
 * @brief Structure to hold function pointers for an accessibility module's API.
 *
 * This allows Elementary to interact with different accessibility backends
 * (like AT-SPI) through a common interface.
 */
typedef struct _Mod_Api Mod_Api;

struct _Mod_Api
{
   void (*out_read) (const char *txt);
   void (*out_read_done) (void);
   void (*out_cancel) (void);
   void (*out_done_callback_set) (void (*func) (void *data), const void *data); /**< Sets a callback to be invoked when output is done. */
};

static int initted = 0; /**< Counter for access module initialization. */
static Mod_Api *mapi = NULL; /**< Pointer to the loaded accessibility module API. */

/**
 * @internal
 * @brief Initializes the accessibility module.
 *
 * Loads the "access/api" module and retrieves its API function pointers.
 * This function ensures the module is initialized only once.
 */
static void
_access_init(void)
{
   Elm_Module *m;

   if (initted > 0) return;
   if (!(m = _elm_module_find_as("access/api"))) return;
   if (m->init_func(m) < 0) return;
   initted++;

   m->api = malloc(sizeof(Mod_Api));
   if (!m->api) return;
   ((Mod_Api *)(m->api)      )->out_read = // called to read out some text
      _elm_module_symbol_get(m, "out_read");
   ((Mod_Api *)(m->api)      )->out_read_done = // called to set a done marker so when it is reached the done callback is called
      _elm_module_symbol_get(m, "out_read_done");
   ((Mod_Api *)(m->api)      )->out_cancel = // called to read out some text
      _elm_module_symbol_get(m, "out_cancel");
   ((Mod_Api *)(m->api)      )->out_done_callback_set = // called when last read done
      _elm_module_symbol_get(m, "out_done_callback_set");
   mapi = m->api;
}

/**
 * @internal
 * @brief Shuts down the accessibility module.
 *
 * If the accessibility module was initialized, this function calls its
 * shutdown routine and frees associated resources.
 */
static void
_access_shutdown(void)
{
   Elm_Module *m;
   if (initted == 0) return;
   if (!(m = _elm_module_find_as("access/api"))) return;

   m->shutdown_func(m);

   initted = 0;

   /* _elm_module_unload(); could access m->api and try to free(); */
   ELM_SAFE_FREE(m->api, free);
   mapi = NULL;
}

/**
 * @internal
 * @brief Adds or retrieves an Elm_Access_Item for a given type.
 *
 * If an item of the specified type already exists in the Elm_Access_Info,
 * it is returned after clearing its previous data. Otherwise, a new
 * Elm_Access_Item is created, prepended to the list, and returned.
 *
 * @param ac The Elm_Access_Info structure to modify.
 * @param type The type of access information (e.g., ELM_ACCESS_INFO, ELM_ACCESS_TYPE).
 * @return A pointer to the Elm_Access_Item, or NULL on failure.
 */
static Elm_Access_Item *
_access_add_set(Elm_Access_Info *ac, int type)
{
   Elm_Access_Item *ai;
   Eina_List *l;

   if (!ac) return NULL;
   EINA_LIST_FOREACH(ac->items, l, ai)
     {
        if (ai->type == type)
          {
             if (!ai->func)
               {
                  eina_stringshare_del(ai->data);
               }
             ai->func = NULL;
             ai->data = NULL;
             return ai;
          }
     }
   ai = calloc(1, sizeof(Elm_Access_Item));
   ai->type = type;
   ac->items = eina_list_prepend(ac->items, ai);
   return ai;
}

/**
 * @internal
 * @brief Gets the currently highlighted accessible object.
 *
 * It searches for a special display object named "_elm_access_disp" and
 * retrieves the target object associated with it.
 *
 * @param obj An Evas_Object (often the canvas or a container).
 * @return The Evas_Object that is currently highlighted for accessibility, or NULL.
 */
static Evas_Object *
_access_highlight_object_get(Evas_Object *obj)
{
   Evas_Object *o, *ho;

   o = evas_object_name_find(evas_object_evas_get(obj), "_elm_access_disp");
   if (!o) return NULL;

   ho = evas_object_data_get(o, "_elm_access_target");

   return ho;
}

/**
 * @internal
 * @brief Reads out the accessibility information for a highlighted object.
 *
 * This function gathers text from various info types (label, type, state, context)
 * associated with the object, concatenates them, and then uses the
 * accessibility module (e.g., AT-SPI via mapi) to speak the text.
 * It also triggers an on_highlight callback if set.
 *
 * @param ac The Elm_Access_Info for the object.
 * @param obj The Evas_Object whose information is to be read.
 */
static void
_access_highlight_read(Elm_Access_Info *ac, Evas_Object *obj)
{
   int type;
   char *txt = NULL;
   Eina_Strbuf *strbuf;

   strbuf = eina_strbuf_new();

   if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
     {
        if (ac->on_highlight) ac->on_highlight(ac->on_highlight_data);
        _elm_access_object_highlight(obj);

        for (type = ELM_ACCESS_INFO_FIRST + 1; type < ELM_ACCESS_INFO_LAST; type++)
          {
             txt = _elm_access_text_get(ac, type, obj);
             if (txt && (strlen(txt) > 0))
               {
                  if (eina_strbuf_length_get(strbuf) > 0)
                    eina_strbuf_append_printf(strbuf, ", %s", txt);
                  else
                    eina_strbuf_append(strbuf, txt);
               }
             free(txt);
          }
     }

   txt = eina_strbuf_string_steal(strbuf);
   eina_strbuf_free(strbuf);

   _elm_access_say(txt);
   free(txt);
}

/**
 * @internal
 * @brief Timeout callback for delayed reading of an object under the mouse.
 *
 * When the mouse hovers over an accessible object, this callback is triggered
 * after a short delay to read out its information. This prevents rapid-fire
 * reading when moving the mouse quickly.
 *
 * @param data The Evas_Object that was hovered over.
 * @return @c EINA_FALSE to automatically remove the timer.
 */
static Eina_Bool
_access_obj_over_timeout_cb(void *data)
{
   Elm_Access_Info *ac;
   Evas_Object *ho;

   if (!data) return EINA_FALSE;

   ac = evas_object_data_get(data, "_elm_access");
   if (!ac) return EINA_FALSE;

   ho = _access_highlight_object_get(data);
   if (ho != data) _access_highlight_read(ac, data);

   ac->delay_timer = NULL;
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_MOUSE_IN on an accessible object.
 *
 * When the mouse enters an accessible object, this function schedules a timer
 * to read out the object's information after a short delay, if mouse events
 * for accessibility are enabled.
 *
 * @param data The Evas_Object associated with the access info (the access object itself).
 * @param e The Evas canvas.
 * @param obj The Evas_Object that triggered the event (the hover object).
 * @param event_info Event-specific information (unused).
 */
static void
_access_hover_mouse_in_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info  EINA_UNUSED)
{
   Elm_Access_Info *ac;
   if (!mouse_event_enable) return;

    ac = evas_object_data_get(data, "_elm_access");
   if (!ac) return;

   ELM_SAFE_FREE(ac->delay_timer, ecore_timer_del);

   if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
      ac->delay_timer = ecore_timer_add(0.2, _access_obj_over_timeout_cb, data);
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_MOUSE_OUT on an accessible object.
 *
 * When the mouse leaves an accessible object, this function cancels any pending
 * read timer and unhighlights the object.
 *
 * @param data The Evas_Object associated with the access info (the access object itself).
 * @param e The Evas canvas.
 * @param obj The Evas_Object that triggered the event (the hover object).
 * @param event_info Event-specific information (unused).
 */
static void
_access_hover_mouse_out_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Elm_Access_Info *ac;
   if (!mouse_event_enable) return;

   ac = evas_object_data_get(data, "_elm_access");
   if (!ac) return;

   _elm_access_object_unhighlight(data);

   ELM_SAFE_FREE(ac->delay_timer, ecore_timer_del);
}

/**
 * @internal
 * @brief Callback invoked when the accessibility backend finishes reading text.
 * @param data User data (unused).
 * @todo Produce an event here.
 */
static void
_access_read_done(void *data EINA_UNUSED)
{
   DBG("read done");
   // FIXME: produce event here
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_DEL on an object with a pending 2nd click timer.
 *
 * This ensures that the 2nd click timer is cancelled if the object is deleted
 * before the timer expires.
 *
 * @param data User data (unused).
 * @param e The Evas canvas.
 * @param obj The Evas_Object being deleted.
 * @param event_info Event-specific information (unused).
 */
static void
_access_2nd_click_del_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Ecore_Timer *t;

   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_DEL,
                                       _access_2nd_click_del_cb, NULL);
   t = evas_object_data_get(obj, "_elm_2nd_timeout");
   if (t)
     {
        ecore_timer_del(t);
        evas_object_data_del(obj, "_elm_2nd_timeout");
     }
}

/**
 * @internal
 * @brief Timeout callback for detecting a second click (double-click like behavior).
 *
 * This is used to distinguish single clicks from potential double clicks for
 * accessibility actions. If the timer expires, it means a single click occurred.
 *
 * @param data The Evas_Object on which the click occurred.
 * @return @c EINA_FALSE to automatically remove the timer.
 */
static Eina_Bool
_access_2nd_click_timeout_cb(void *data)
{
   evas_object_event_callback_del_full(data, EVAS_CALLBACK_DEL,
                                       _access_2nd_click_del_cb, NULL);
   evas_object_data_del(data, "_elm_2nd_timeout");
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_DEL on a highlighted object.
 * Disables the highlight display when the object is deleted.
 * @param data User data (unused).
 * @param e The Evas canvas.
 * @param obj The Evas_Object being deleted (unused).
 * @param event_info Event-specific information (unused).
 */
static void
_access_obj_hilight_del_cb(void *data EINA_UNUSED, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _elm_access_object_highlight_disable(e);
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_HIDE on a highlighted object.
 * Disables the highlight display when the object is hidden.
 * @param data User data (unused).
 * @param e The Evas canvas.
 * @param obj The Evas_Object being hidden (unused).
 * @param event_info Event-specific information (unused).
 */
static void
_access_obj_hilight_hide_cb(void *data EINA_UNUSED, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _elm_access_object_highlight_disable(e);
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_MOVE on a highlighted object.
 * Moves the highlight display to match the object's new position.
 * @param data User data (unused).
 * @param e The Evas canvas (unused).
 * @param obj The Evas_Object that was moved.
 * @param event_info Event-specific information (unused).
 */
static void
_access_obj_hilight_move_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Coord x, y;
   Evas_Object *o;

   o = evas_object_name_find(evas_object_evas_get(obj), "_elm_access_disp");
   if (!o) return;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   evas_object_move(o, x, y);
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_RESIZE on a highlighted object.
 * Resizes the highlight display to match the object's new size.
 * @param data User data (unused).
 * @param e The Evas canvas (unused).
 * @param obj The Evas_Object that was resized.
 * @param event_info Event-specific information (unused).
 */
static void
_access_obj_hilight_resize_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Coord w, h;
   Evas_Object *o;

   o = evas_object_name_find(evas_object_evas_get(obj), "_elm_access_disp");
   if (!o) return;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   evas_object_resize(o, w, h);
}

/**
 * @internal
 * @brief Enables or disables mouse event processing for accessibility.
 *
 * This is used to temporarily ignore mouse events, for example, when
 * programmatically moving the mouse for accessibility purposes.
 *
 * @param enabled @c EINA_TRUE to enable, @c EINA_FALSE to disable.
 */
void
_elm_access_mouse_event_enabled_set(Eina_Bool enabled)
{
   enabled = !!enabled;
   if (mouse_event_enable == enabled) return;
   mouse_event_enable = enabled;
}

/**
 * @internal
 * @brief Sets the auto highlight mode for accessibility.
 *
 * Auto highlight is typically enabled during keyboard navigation or
 * programmatic highlight changes.
 *
 * @param enabled @c EINA_TRUE to enable auto highlight, @c EINA_FALSE otherwise.
 */
void
_elm_access_auto_highlight_set(Eina_Bool enabled)
{
   enabled = !!enabled;
   if (auto_highlight == enabled) return;
   auto_highlight = enabled;
}

/**
 * @internal
 * @brief Gets the current state of auto highlight mode.
 * @return @c EINA_TRUE if auto highlight is enabled, @c EINA_FALSE otherwise.
 */
Eina_Bool
_elm_access_auto_highlight_get(void)
{
   return auto_highlight;
}

/**
 * @internal
 * @brief Shuts down the Elementary accessibility system.
 * Calls the internal _access_shutdown function.
 */
void
_elm_access_shutdown()
{
   _access_shutdown();
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_DEL on an object in a widget item's access order.
 *
 * Removes the object from the widget item's custom access order list when
 * the object is deleted.
 *
 * @param data The Elm_Widget_Item_Data whose access order is being modified.
 * @param e The Evas canvas (unused).
 * @param obj The Evas_Object being deleted from the access order.
 * @param event_info Event-specific information (unused).
 */
static void
_access_order_del_cb(void *data,
                     Evas *e EINA_UNUSED,
                     Evas_Object *obj,
                     void *event_info EINA_UNUSED)
{
   Elm_Widget_Item_Data *item = data;

   item->access_order = eina_list_remove(item->access_order, obj);
}

/**
 * @internal
 * @brief Sets the custom accessibility order for a widget item.
 *
 * This allows defining a specific navigation order for accessible elements
 * within a widget item, overriding the default order.
 *
 * @param item The widget item.
 * @param objs An Eina_List of Evas_Object pointers representing the desired access order.
 *             The list itself is not copied, so it should not be modified or freed
 *             by the caller after this call unless it's first unset.
 *             Example:
 *             Eina_List *order = NULL;
 *             order = eina_list_append(order, child_obj1);
 *             order = eina_list_append(order, child_obj2);
 *             _elm_access_widget_item_access_order_set(item_data, order);
 */
void
_elm_access_widget_item_access_order_set(Elm_Widget_Item_Data *item,
                                         Eina_List *objs)
{
   Eina_List *l;
   Evas_Object *o;

   if (!item) return;

   _elm_access_widget_item_access_order_unset(item);

   EINA_LIST_FOREACH(objs, l, o)
     {
        evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
                                       _access_order_del_cb, item);
     }

   item->access_order = objs;
}

/**
 * @internal
 * @brief Gets the custom accessibility order for a widget item.
 * @param item The widget item.
 * @return A const Eina_List of Evas_Object pointers representing the access order,
 *         or NULL if no custom order is set or item is NULL.
 *         The returned list should not be modified.
 */
const Eina_List *
_elm_access_widget_item_access_order_get(const Elm_Widget_Item_Data *item)
{
   if (!item) return NULL;
   return item->access_order;
}

/**
 * @internal
 * @brief Unsets the custom accessibility order for a widget item.
 *
 * This removes any previously defined custom access order, reverting to the
 * default navigation behavior. It also cleans up associated callbacks.
 *
 * @param item The widget item.
 */
void
_elm_access_widget_item_access_order_unset(Elm_Widget_Item_Data *item)
{
   Eina_List *l, *l_next;
   Evas_Object *o;

   if (!item) return;

   EINA_LIST_FOREACH_SAFE(item->access_order, l, l_next, o)
     {
        evas_object_event_callback_del_full
          (o, EVAS_CALLBACK_DEL, _access_order_del_cb, item);
        item->access_order = eina_list_remove_list(item->access_order, l);
     }
}

/**
 * @internal
 * @brief Gets and highlights the next/previous accessible object.
 *
 * This function determines the next or previous object in the accessibility
 * chain based on the current highlight, custom highlight links (info->next/prev),
 * or the focus manager's relations. It then sets the highlight to that object.
 *
 * @param obj The current Evas_Object, often the highlight root or an object within it.
 * @param dir The direction to move the highlight (ELM_FOCUS_NEXT or ELM_FOCUS_PREVIOUS).
 * @return @c EINA_TRUE if a new object was highlighted, @c EINA_FALSE otherwise (e.g., at the end of the chain).
 */
static Eina_Bool
_access_highlight_next_get(Evas_Object *obj, Elm_Focus_Direction dir)
{
   int type;
   Evas_Object *ho, *parent, *target;
   Eina_Bool ret;

   target = NULL;
   ret = EINA_FALSE;

   if (!elm_widget_is(obj)) return ret;

   ho = _access_highlight_object_get(obj);
   if (!ho) ho = obj;

   parent = ho;

   /* find highlight root */
   do
     {
        ELM_WIDGET_DATA_GET_OR_RETURN(parent, wd, ret);
        if (wd->highlight_root)
          {
             /* change highlight root */
             obj = parent;
             break;
          }
        parent = elm_widget_parent_get(parent);
     }
   while (parent);

   _elm_access_auto_highlight_set(EINA_TRUE);

   if (dir == ELM_FOCUS_NEXT)
     type = ELM_ACCESS_ACTION_HIGHLIGHT_NEXT;
   else
     type = ELM_ACCESS_ACTION_HIGHLIGHT_PREV;

   /* this value is used in _elm_access_object_highlight();
      to inform the target object of how to get highlight */
   action_by = type;

   if (!_access_action_callback_call(ho, type, NULL))
     {
        if (ho)
          {
             Elm_Access_Info *info = _elm_access_info_get(ho);
             EINA_SAFETY_ON_NULL_RETURN_VAL(info, ret);
             if (type == ELM_ACCESS_ACTION_HIGHLIGHT_NEXT && info->next)
               target = info->next;
             else if (type == ELM_ACCESS_ACTION_HIGHLIGHT_PREV && info->prev)
               target = info->prev;
          }

        if (target)
          {
             _elm_access_highlight_set(target);
             elm_widget_focus_region_show(target);
             ret = EINA_TRUE;
          }
        else
          {
             Efl_Ui_Focus_Relations *rel;

             rel = efl_ui_focus_manager_fetch(efl_ui_focus_object_focus_manager_get(obj), obj);

             if (rel)
               {
                  if (dir == ELM_FOCUS_NEXT)
                    _elm_access_highlight_set(rel->next);
                  else
                    _elm_access_highlight_set(rel->prev);

                  free(rel);
               }
          }
     }

   action_by = ELM_ACCESS_ACTION_FIRST;

   _elm_access_auto_highlight_set(EINA_FALSE);

   return ret;
}

//-------------------------------------------------------------------------//
/**
 * @internal
 * @brief Sets the accessibility highlight to a specific object and reads its info.
 *
 * If the object is not already highlighted, this function updates the
 * highlight display and reads out the object's accessibility information.
 *
 * @param obj The Evas_Object to highlight.
 */
EAPI void
_elm_access_highlight_set(Evas_Object* obj)
{
   Elm_Access_Info *ac;
   Evas_Object *ho;

   if (!obj) return;

   ho = _access_highlight_object_get(obj);
   if (ho == obj) return;

   ac = evas_object_data_get(obj, "_elm_access");
   if (!ac) return;

   _access_highlight_read(ac, obj);
}

/**
 * @internal
 * @brief Clears all accessibility information items from an Elm_Access_Info structure.
 *
 * This function iterates through the list of Elm_Access_Item in @p ac,
 * frees their data (stringshared if not a callback), and then frees the items themselves.
 * It also cancels any pending delay timer.
 *
 * @param ac The Elm_Access_Info structure to clear.
 */
EAPI void
_elm_access_clear(Elm_Access_Info *ac)
{
   Elm_Access_Item *ai;

   if (!ac) return;
   ELM_SAFE_FREE(ac->delay_timer, ecore_timer_del);
   EINA_LIST_FREE(ac->items, ai)
     {
        if (!ai->func)
          {
             eina_stringshare_del(ai->data);
          }
        free(ai);
     }
}

/**
 * @internal
 * @brief Sets a static text string for a specific accessibility information type.
 *
 * This function associates a given text string with an access information type
 * (e.g., ELM_ACCESS_INFO, ELM_ACCESS_TYPE) for an accessible object.
 * The text is stringshared.
 *
 * @param ac The Elm_Access_Info structure for the object.
 * @param type The type of access information to set.
 * @param text The text string to set.
 */
EAPI void
_elm_access_text_set(Elm_Access_Info *ac, int type, const char *text)
{
   Elm_Access_Item *ai = _access_add_set(ac, type);
   if (!ai) return;
   ai->func = NULL;
   ai->data = eina_stringshare_add(text);
}

/**
 * @internal
 * @brief Sets a callback function to provide text for a specific accessibility information type.
 *
 * This allows dynamic generation of accessibility text. When the accessibility
 * system needs this information, the provided callback @p func will be called.
 *
 * @param ac The Elm_Access_Info structure for the object.
 * @param type The type of access information this callback provides.
 * @param func The callback function.
 * @param data User data to be passed to the callback function.
 */
EAPI void
_elm_access_callback_set(Elm_Access_Info *ac, int type, Elm_Access_Info_Cb func, const void *data)
{
   Elm_Access_Item *ai = _access_add_set(ac, type);
   if (!ai) return;
   ai->func = func;
   ai->data = data;
}

/**
 * @internal
 * @brief Sets a callback function to be invoked when an object is highlighted.
 *
 * @param ac The Elm_Access_Info structure for the object.
 * @param func The callback function to call on highlight.
 * @param data User data to be passed to the callback function.
 */
EAPI void
_elm_access_on_highlight_hook_set(Elm_Access_Info           *ac,
                                  Elm_Access_On_Highlight_Cb func,
                                  void                      *data)
{
    if (!ac) return;
    ac->on_highlight = func;
    ac->on_highlight_data = data;
}

/**
 * @internal
 * @brief Sets a callback function to be invoked when an accessible object is activated.
 *
 * @param ac The Elm_Access_Info structure for the object.
 * @param func The callback function to call on activation.
 * @param data User data to be passed to the callback function.
 */
EAPI void
_elm_access_activate_callback_set(Elm_Access_Info           *ac,
                                  Elm_Access_Activate_Cb     func,
                                  void                      *data)
{
   if (!ac) return;
   ac->activate = func;
   ac->activate_data = data;
}

/**
 * @internal
 * @brief Activates the currently highlighted accessible object.
 *
 * This function retrieves the object that currently has the accessibility
 * highlight and then attempts to activate it (e.g., simulate a click).
 * It also ensures the object has focus before activation.
 *
 * @param obj An Evas_Object (often the canvas or a container from which to find the highlight).
 * @param act The type of activation (e.g., default, up, down).
 */
EAPI void
_elm_access_highlight_object_activate(Evas_Object *obj, Efl_Ui_Activate act)
{
   Evas_Object *highlight;

   highlight = _access_highlight_object_get(obj);
   if (!highlight) return;

   _elm_access_auto_highlight_set(EINA_FALSE);

   if (!elm_object_focus_get(highlight))
     elm_object_focus_set(highlight, EINA_TRUE);

   elm_widget_activate(highlight, act);
   return;
}

/**
 * @internal
 * @brief Cycles the accessibility highlight to the next or previous object.
 *
 * This function is similar to _access_highlight_next_get but is typically
 * used for continuous cycling (e.g., Tab/Shift+Tab). It finds the highlight
 * root and then attempts to move the highlight. If custom next/prev links
 * exist, they are used; otherwise, it falls back to the focus manager.
 *
 * @param obj The current Evas_Object, often the highlight root or an object within it.
 * @param dir The direction to cycle the highlight (ELM_FOCUS_NEXT or ELM_FOCUS_PREVIOUS).
 */
EAPI void
_elm_access_highlight_cycle(Evas_Object *obj, Elm_Focus_Direction dir)
{
   int type;
   Evas_Object *ho, *parent;

   ho = _access_highlight_object_get(obj);
   if (!ho) return;

   parent = ho;

   /* find highlight root */
   do
     {
        ELM_WIDGET_DATA_GET_OR_RETURN(parent, wd);
        if (wd->highlight_root)
          {
             /* change highlight root */
             obj = parent;
             break;
          }
        parent = elm_widget_parent_get(parent);
     }
   while (parent);

   _elm_access_auto_highlight_set(EINA_TRUE);

   if (dir == ELM_FOCUS_NEXT)
     type = ELM_ACCESS_ACTION_HIGHLIGHT_NEXT;
   else
     type = ELM_ACCESS_ACTION_HIGHLIGHT_PREV;

   action_by = type;

   if (!_access_action_callback_call(ho, type, NULL))
     {
        Elm_Access_Info *info = _elm_access_info_get(ho);
        Evas_Object *comming = NULL;
        if (type == ELM_ACCESS_ACTION_HIGHLIGHT_NEXT)
          {
             if ((info) && (info->next)) comming = info->next;
          }
        else
          {
             if ((info) && (info->prev)) comming = info->prev;
          }
        if (comming)
          {
             _elm_access_highlight_set(comming);
             elm_widget_focus_region_show(comming);
          }
        else
          {
             efl_ui_focus_util_focus(obj);
             efl_ui_focus_manager_move(elm_widget_top_get(obj),
                                       (Efl_Ui_Focus_Direction)dir);
          }
     }

   action_by = ELM_ACCESS_ACTION_FIRST;

   _elm_access_auto_highlight_set(EINA_FALSE);
}

/**
 * @internal
 * @brief Retrieves the accessibility text for a specific type from an Elm_Access_Info structure.
 *
 * This function searches for an Elm_Access_Item of the given @p type.
 * If found, it either calls the associated callback function or returns a copy
 * of the static text.
 *
 * @param ac The Elm_Access_Info structure.
 * @param type The type of access information to retrieve.
 * @param obj The Evas_Object for which the information is requested (passed to callbacks).
 * @return A newly allocated string containing the accessibility text, or NULL if not found.
 *         The caller is responsible for freeing the returned string.
 */
EAPI char *
_elm_access_text_get(const Elm_Access_Info *ac, int type, const Evas_Object *obj)
{
   Elm_Access_Item *ai;
   Eina_List *l;

   if (!ac) return NULL;
   EINA_LIST_FOREACH(ac->items, l, ai)
     {
        if (ai->type == type)
          {
             if (ai->func) return ai->func((void *)(ai->data), (Evas_Object *)obj);
             else if (ai->data) return strdup(ai->data);
             return NULL;
          }
     }
   return NULL;
}

/**
 * @internal
 * @brief Reads out accessibility text of a specific type for an object.
 *
 * Retrieves the text for the given type and object using _elm_access_text_get,
 * then uses the accessibility module (mapi) to speak it. It handles special
 * types like ELM_ACCESS_DONE (signals end of reading) and ELM_ACCESS_CANCEL
 * (cancels current speech).
 *
 * @param ac The Elm_Access_Info structure for the object.
 * @param type The type of access information to read (e.g., ELM_ACCESS_INFO, ELM_ACCESS_DONE).
 * @param obj The Evas_Object whose information is to be read.
 */
EAPI void
_elm_access_read(Elm_Access_Info *ac, int type, const Evas_Object *obj)
{
   char *txt = _elm_access_text_get(ac, type, obj);

   _access_init();
   if (mapi)
     {
        if (mapi->out_done_callback_set)
           mapi->out_done_callback_set(_access_read_done, NULL);
        if (type == ELM_ACCESS_DONE)
          {
             if (mapi->out_read_done) mapi->out_read_done();
          }
        else if (type == ELM_ACCESS_CANCEL)
          {
             if (mapi->out_cancel) mapi->out_cancel();
          }
        else
          {
             if (txt)
               {
                  if (mapi->out_read) mapi->out_read(txt);
                  if (mapi->out_read) mapi->out_read(".\n");
               }
          }
     }
   free(txt);
}

/**
 * @internal
 * @brief Speaks the given text using the accessibility module.
 *
 * This is a direct way to make the accessibility system speak a string.
 * It initializes the access module if needed, cancels any ongoing speech,
 * then queues the new text to be read.
 *
 * @param txt The text string to be spoken.
 */
EAPI void
_elm_access_say(const char *txt)
{
   if (!_elm_config->access_mode) return;

   _access_init();
   if (mapi)
     {
        if (mapi->out_done_callback_set)
           mapi->out_done_callback_set(_access_read_done, NULL);
        if (mapi->out_cancel) mapi->out_cancel();
        if (txt)
          {
             if (mapi->out_read) mapi->out_read(txt);
             if (mapi->out_read) mapi->out_read(".\n");
          }
        if (mapi->out_read_done) mapi->out_read_done();
     }
}

/**
 * @internal
 * @brief Retrieves the Elm_Access_Info structure associated with an Evas_Object.
 * @param obj The Evas_Object.
 * @return The Elm_Access_Info pointer, or NULL if not set.
 */
EAPI Elm_Access_Info *
_elm_access_info_get(const Evas_Object *obj)
{
   return evas_object_data_get(obj, "_elm_access");
}

/**
 * @internal
 * @brief DEPRECATED: Alias for _elm_access_info_get.
 * @param obj The Evas_Object.
 * @return The Elm_Access_Info pointer, or NULL if not set.
 */
EAPI Elm_Access_Info *
_elm_access_object_get(const Evas_Object *obj)
{
   return _elm_access_info_get(obj);
}

/**
 * @internal
 * @brief Traverses up the Evas object hierarchy to find the owning Elementary widget.
 *
 * This is used to find the widget responsible for an Evas object, which might be
 * a part of a complex widget.
 *
 * @param obj The Evas_Object to start searching from.
 * @return The parent Elementary widget Evas_Object, or NULL if not found.
 */
static Evas_Object *
_elm_access_widget_target_get(Evas_Object *obj)
{
   Evas_Object *o = obj;

   do
     {
        if (elm_widget_is(o))
          break;
        else
          {
             o = elm_widget_parent_widget_get(o);
             if (!o)
               o = evas_object_smart_parent_get(o);
          }
     }
   while (o);

   return o;
}

/**
 * @internal
 * @brief Displays a visual highlight around an accessible object.
 *
 * This function creates or reuses a special Edje object ("_elm_access_disp")
 * to draw a highlight rectangle around the given @p obj. It handles theming
 * for the highlight and sets up callbacks to move/resize/hide the highlight
 * along with the target object.
 *
 * @param obj The Evas_Object to highlight.
 */
EAPI void
_elm_access_object_highlight(Evas_Object *obj)
{
   Evas_Object *o, *widget;
   Evas_Coord x, y, w, h;
   Eina_Bool in_theme = EINA_FALSE;

   o = evas_object_name_find(evas_object_evas_get(obj), "_elm_access_disp");
   if (!o)
     {
        o = edje_object_add(evas_object_evas_get(obj));
        evas_object_name_set(o, "_elm_access_disp");
        evas_object_layer_set(o, ELM_OBJECT_LAYER_TOOLTIP);
     }
   else
     {
        Evas_Object *ptarget = evas_object_data_get(o, "_elm_access_target");
        if (ptarget)
          {
             evas_object_data_del(o, "_elm_access_target");
             elm_widget_parent_highlight_set(ptarget, EINA_FALSE);

             evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_DEL,
                                                 _access_obj_hilight_del_cb, NULL);
             evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_HIDE,
                                                 _access_obj_hilight_hide_cb, NULL);
             evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_MOVE,
                                                 _access_obj_hilight_move_cb, NULL);
             evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_RESIZE,
                                                 _access_obj_hilight_resize_cb, NULL);

             widget = _elm_access_widget_target_get(ptarget);
             if (widget)
               {
                  if (elm_widget_access_highlight_in_theme_get(widget))
                    {
                       elm_widget_signal_emit(widget, "elm,action,access_highlight,hide", "elm");
                    }
               }
          }
     }
   evas_object_data_set(o, "_elm_access_target", obj);
   elm_widget_parent_highlight_set(obj, EINA_TRUE);

   elm_widget_theme_object_set(obj, o, "access", "base", "default");

   evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL,
                                  _access_obj_hilight_del_cb, NULL);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE,
                                  _access_obj_hilight_hide_cb, NULL);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE,
                                  _access_obj_hilight_move_cb, NULL);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE,
                                  _access_obj_hilight_resize_cb, NULL);
   evas_object_raise(o);
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_geometry_set(o, x, y, w, h);

   widget = _elm_access_widget_target_get(obj);
   if (widget)
     {
        if (elm_widget_access_highlight_in_theme_get(widget))
          {
             in_theme = EINA_TRUE;
             elm_widget_signal_emit(widget, "elm,action,access_highlight,show", "elm");
          }
     }
   /* use callback, should an access object do below every time when
    * a window gets a client message ECORE_X_ATOM_E_ILLMUE_ACTION_READ? */
   if (!in_theme &&
       !_access_action_callback_call(obj, ELM_ACCESS_ACTION_HIGHLIGHT, NULL))
     evas_object_show(o);
   else
     evas_object_hide(o);
}

/**
 * @internal
 * @brief Removes the visual highlight from an accessible object.
 *
 * If the given @p obj is the currently highlighted object, this function
 * deletes the highlight display Edje object and cleans up associated callbacks.
 *
 * @param obj The Evas_Object to unhighlight.
 */
EAPI void
_elm_access_object_unhighlight(Evas_Object *obj)
{
   Evas_Object *o, *ptarget;

   o = evas_object_name_find(evas_object_evas_get(obj), "_elm_access_disp");
   if (!o) return;
   ptarget = evas_object_data_get(o, "_elm_access_target");
   if (ptarget == obj)
     {
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_DEL,
                                            _access_obj_hilight_del_cb, NULL);
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_HIDE,
                                            _access_obj_hilight_hide_cb, NULL);
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_MOVE,
                                            _access_obj_hilight_move_cb, NULL);
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_RESIZE,
                                            _access_obj_hilight_resize_cb, NULL);
        evas_object_del(o);
        elm_widget_parent_highlight_set(ptarget, EINA_FALSE);
     }
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_RESIZE on a content object managed by an access object.
 * Resizes the associated access object to match the content object's new size.
 * @param data The access Evas_Object.
 * @param e The Evas canvas (unused).
 * @param obj The content Evas_Object that was resized.
 * @param event_info Event-specific information (unused).
 */
static void
_content_resize(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                void *event_info EINA_UNUSED)
{
   Evas_Object *accessobj;
   Evas_Coord w, h;

   accessobj = data;
   if (!accessobj) return;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   evas_object_resize(accessobj, w, h);
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_MOVE on a content object managed by an access object.
 * Moves the associated access object to match the content object's new position.
 * @param data The access Evas_Object.
 * @param e The Evas canvas (unused).
 * @param obj The content Evas_Object that was moved.
 * @param event_info Event-specific information (unused).
 */
static void
_content_move(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
              void *event_info EINA_UNUSED)
{
   Evas_Object *accessobj;
   Evas_Coord x, y;

   accessobj = data;
   if (!accessobj) return;

   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   evas_object_move(accessobj, x, y);
}

/**
 * @internal
 * @brief Registers an Evas_Object as an accessible object, creating an access counterpart.
 *
 * This function creates a new Elm_Access object (@c ao), associates it with the
 * given @p obj (often a part of a widget or a simple Evas object), and sets up
 * callbacks to keep their geometries synchronized. The @p obj becomes the
 * "content" or "part" object, and @c ao is its accessibility representation.
 *
 * @param obj The Evas_Object to make accessible (e.g., an Edje part, an image).
 * @param parent The Elementary widget that will be the parent of the new access object.
 * @return The newly created Elm_Access Evas_Object, or NULL on failure.
 */
static Evas_Object *
_access_object_register(Evas_Object *obj, Evas_Object *parent)
{
   Evas_Object *ao;
   Elm_Access_Info *ac;
   Evas_Coord x, y, w, h;

   if (!obj) return NULL;

   /* check previous access object */
   ao = evas_object_data_get(obj, "_part_access_obj");
   if (ao)
     _access_object_unregister(obj);

   /* create access object */
   ao = _elm_access_add(parent);
   if (!ao) return NULL;

   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE,
                                  _content_resize, ao);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE,
                                  _content_move, ao);

   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_geometry_set(ao, x, y, w, h);
   evas_object_show(ao);

   /* register access object */
   _elm_access_object_register(ao, obj);

   /* set access object */
   evas_object_data_set(obj, "_part_access_obj", ao);

   /* set owner part object */
   ac = evas_object_data_get(ao, "_elm_access");
   ac->part_object = obj;

   return ao;
}

/**
 * @internal
 * @brief Unregisters an accessible object.
 *
 * This function handles the cleanup when an object (or its access counterpart)
 * is no longer needed. If @p obj has an associated access object (stored in
 * "_part_access_obj" data), that access object is deleted. Otherwise, if @p obj
 * is itself an access object, its hover object registration is cleaned up.
 *
 * @param obj The Evas_Object whose accessibility registration is to be removed.
 *            This can be the original content object or the access object itself.
 */
static void
_access_object_unregister(Evas_Object *obj)
{
   Elm_Access_Info *ac;
   Evas_Object *ao;

   if (!obj) return;

   ao = evas_object_data_get(obj, "_part_access_obj");

   if (ao)
     {
        /* delete callbacks and unregister access object in _access_obj_del_cb*/
        evas_object_del(ao);
     }
   else
     {
        /* button, check, label etc. */
        ac = evas_object_data_get(obj, "_elm_access");
        if (ac && ac->hoverobj)
          _elm_access_object_unregister(obj, ac->hoverobj);
     }
}

/**
 * @internal
 * @brief Registers a part of an Edje object as an accessible object.
 *
 * Retrieves the Evas_Object for the specified @p part from the Edje object @p eobj,
 * and then registers it using _access_object_register. The @p obj is used as the
 * parent for the new access object.
 *
 * @param obj The Elementary widget that will parent the access object.
 * @param eobj The Edje Evas_Object.
 * @param part The name of the part within the Edje object to make accessible.
 * @return The newly created Elm_Access Evas_Object, or NULL on failure.
 */
EAPI Evas_Object *
_elm_access_edje_object_part_object_register(Evas_Object* obj,
                                             const Evas_Object *eobj,
                                             const char* part)
{
   Evas_Object *ao, *po;

   edje_object_freeze((Evas_Object *)eobj);
   po = (Evas_Object *)edje_object_part_object_get(eobj, part);
   edje_object_thaw((Evas_Object *)eobj);
   if (!obj || !po) return NULL;

   /* check previous access object */
   ao = evas_object_data_get(po, "_part_access_obj");
   if (ao)
     _elm_access_edje_object_part_object_unregister(obj, eobj, part);

   ao = _access_object_register(po, obj);

   return ao;
}

/**
 * @internal
 * @brief Unregisters an accessible Edje object part.
 *
 * Retrieves the Evas_Object for the specified @p part from the Edje object @p eobj,
 * and then unregisters its accessibility features using _access_object_unregister.
 *
 * @param obj The Elementary widget (currently unused, marked with FIXME).
 * @param eobj The Edje Evas_Object.
 * @param part The name of the part within the Edje object.
 */
//FIXME: unused obj should be removed from here and each widget.
EAPI void
_elm_access_edje_object_part_object_unregister(Evas_Object* obj EINA_UNUSED,
                                               const Evas_Object *eobj,
                                               const char* part)
{
   Evas_Object *po;

   edje_object_freeze((Evas_Object *)eobj);
   po = (Evas_Object *)edje_object_part_object_get(eobj, part);
   edje_object_thaw((Evas_Object *)eobj);
   if (!po) return;

   _access_object_unregister(po);
}

/**
 * @internal
 * @brief Disables and removes the global accessibility highlight display for a given Evas canvas.
 *
 * Finds the highlight display object ("_elm_access_disp") on the canvas,
 * cleans up its callbacks and associated data from its target, and deletes it.
 *
 * @param e The Evas canvas from which to remove the highlight.
 */
EAPI void
_elm_access_object_highlight_disable(Evas *e)
{
   Evas_Object *o, *ptarget;

   o = evas_object_name_find(e, "_elm_access_disp");
   if (!o) return;
   ptarget = evas_object_data_get(o, "_elm_access_target");
   if (ptarget)
     {
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_DEL,
                                            _access_obj_hilight_del_cb, NULL);
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_HIDE,
                                            _access_obj_hilight_hide_cb, NULL);
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_MOVE,
                                            _access_obj_hilight_move_cb, NULL);
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_RESIZE,
                                            _access_obj_hilight_resize_cb, NULL);
     }
   evas_object_del(o);
   elm_widget_parent_highlight_set(ptarget, EINA_FALSE);
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_DEL on an access object or its hover object.
 *
 * This function handles the cleanup when either an access object or the
 * object it's providing access for (hover object) is deleted.
 * It ensures that associated resources and callbacks are cleaned up.
 * If @p data is not NULL, it means @p obj is the access object and @p data is the hover object.
 * Otherwise, @p obj is the hover object and @p data is NULL (or was the access object, now gone).
 *
 * @param data The hover Evas_Object if @p obj is an access object, or NULL/access_obj.
 * @param e The Evas canvas (unused).
 * @param obj The Evas_Object being deleted.
 * @param event_info Event-specific information (unused).
 */
static void
_access_obj_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{

   Ecore_Job *ao_del_job = NULL;

   evas_object_event_callback_del(obj, EVAS_CALLBACK_DEL, _access_obj_del_cb);

   if (data) /* hover object */
     {
        evas_object_event_callback_del_full(data, EVAS_CALLBACK_RESIZE,
                                            _content_resize, obj);
        evas_object_event_callback_del_full(data, EVAS_CALLBACK_MOVE,
                                            _content_move, obj);

        _elm_access_object_unregister(obj, data);
     }

   ao_del_job = evas_object_data_get(obj, "_access_obj_del_job");

   if (ao_del_job)
     {
        ecore_job_del(ao_del_job);
        evas_object_data_del(obj, "_access_obj_del_job");
     }
}

/**
 * @internal
 * @brief Ecore_Job callback to defer the deletion of an access object.
 *
 * This is used to safely delete an access object, ensuring that it happens
 * outside of certain event callback flows.
 *
 * @param data The Evas_Object (access object) to be deleted.
 */
static void
_access_obj_del_job(void *data)
{
   if (!data) return;

   evas_object_data_del(data, "_access_obj_del_job");

   evas_object_event_callback_del(data, EVAS_CALLBACK_DEL, _access_obj_del_cb);
   evas_object_del(data);
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_DEL on a hover object.
 *
 * This function is called when the object for which accessibility is provided
 * (the "hover object") is deleted. It unregisters the associated access object
 * and schedules the access object itself for deletion via an Ecore_Job.
 *
 * @param data The access Evas_Object associated with the hover object.
 * @param e The Evas canvas (unused).
 * @param obj The hover Evas_Object being deleted.
 * @param event_info Event-specific information (unused).
 */
static void
_access_hover_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Ecore_Job *ao_del_job = NULL;

   /* data - access object - could be NULL */
   if (!data) return;

   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_RESIZE,
                                       _content_resize, data);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_MOVE,
                                       _content_move, data);

   _elm_access_object_unregister(data, obj);

   /* delete access object in job */
   ao_del_job = evas_object_data_get(data, "_access_obj_del_job");
   if (ao_del_job)
     {
        ecore_job_del(ao_del_job);
        evas_object_data_del(data, "_access_obj_del_job");
     }

   ao_del_job = ecore_job_add(_access_obj_del_job, data);
   evas_object_data_set(data, "_access_obj_del_job", ao_del_job);
}

/**
 * @internal
 * @brief Registers an Evas_Object to provide accessibility information for another (hover) object.
 *
 * This sets up the core relationship for accessibility. The @p obj becomes the
 * "access object," which holds accessibility data (Elm_Access_Info) and handles
 * accessibility events. The @p hoverobj is the actual UI element that users
 * interact with visually. Callbacks are set on @p hoverobj (MOUSE_IN, MOUSE_OUT, DEL)
 * to trigger accessibility behaviors on @p obj.
 *
 * @param obj The Evas_Object that will act as the access object.
 * @param hoverobj The Evas_Object that is being made accessible (e.g., a button, an icon).
 */
EAPI void
_elm_access_object_register(Evas_Object *obj, Evas_Object *hoverobj)
{
   Elm_Access_Info *ac;

   evas_object_event_callback_add(hoverobj, EVAS_CALLBACK_MOUSE_IN,
                                  _access_hover_mouse_in_cb, obj);
   evas_object_event_callback_add(hoverobj, EVAS_CALLBACK_MOUSE_OUT,
                                  _access_hover_mouse_out_cb, obj);
   evas_object_event_callback_add(hoverobj, EVAS_CALLBACK_DEL,
                                  _access_hover_del_cb, obj);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL,
                                  _access_obj_del_cb, hoverobj);

   ac = calloc(1, sizeof(Elm_Access_Info));
   evas_object_data_set(obj, "_elm_access", ac);

   ac->hoverobj = hoverobj;
}

/**
 * @internal
 * @brief Unregisters an access object from its hover object.
 *
 * This function cleans up the relationship established by
 * _elm_access_object_register. It removes event callbacks from the @p hoverobj,
 * deletes any stored "_part_access_obj" data from the @p hoverobj,
 * and frees the Elm_Access_Info and Action_Info associated with the @p obj (access object).
 *
 * @param obj The access Evas_Object.
 * @param hoverobj The hover Evas_Object from which to unregister.
 */
EAPI void
_elm_access_object_unregister(Evas_Object *obj, Evas_Object *hoverobj)
{
   Elm_Access_Info *ac;
   Evas_Object *ao;

   evas_object_event_callback_del_full(hoverobj, EVAS_CALLBACK_MOUSE_IN,
                                       _access_hover_mouse_in_cb, obj);
   evas_object_event_callback_del_full(hoverobj, EVAS_CALLBACK_MOUSE_OUT,
                                       _access_hover_mouse_out_cb, obj);
   evas_object_event_callback_del_full(hoverobj, EVAS_CALLBACK_DEL,
                                       _access_hover_del_cb, obj);

   /* _access_obj_del_cb and _access_hover_del_cb calls this function,
      both do not need _part_access_obj data, so delete the data here. */
   ao = evas_object_data_get(hoverobj, "_part_access_obj");
   if (ao) evas_object_data_del(hoverobj, "_part_access_obj");

   ac = evas_object_data_get(obj, "_elm_access");
   evas_object_data_del(obj, "_elm_access");
   if (ac)
     {
        _elm_access_clear(ac);
        free(ac);
     }

   Action_Info *a;
   a = evas_object_data_get(obj, "_elm_access_action_info");
   evas_object_data_del(obj,  "_elm_access_action_info");
   free(a);
}

/**
 * @internal
 * @brief Registers accessibility for an Elm_Widget_Item.
 *
 * This function creates an Elm_Access object for a given widget item.
 * The item's view object (@c item->view) becomes the "hover object," and a new
 * access object is created and associated with it. Geometry synchronization
 * callbacks are set up. The new access object is stored in @c item->access_obj.
 *
 * @param item The Elm_Widget_Item_Data for the item to make accessible.
 */
EAPI void
_elm_access_widget_item_register(Elm_Widget_Item_Data *item)
{
   Evas_Object *ao, *ho;
   Evas_Coord x, y, w, h;
   Elm_Access_Info *ac;

   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);

   /* check previous access object */
   if (item->access_obj)
     _elm_access_widget_item_unregister(item);

   // create access object
   ho = item->view;
   ao = _elm_access_add(item->widget);
   if (!ao) return;

   evas_object_event_callback_add(ho, EVAS_CALLBACK_RESIZE,
                                  _content_resize, ao);
   evas_object_event_callback_add(ho, EVAS_CALLBACK_MOVE,
                                  _content_move, ao);

   evas_object_geometry_get(ho, &x, &y, &w, &h);
   evas_object_geometry_set(ao, x, y, w, h);
   evas_object_show(ao);

   // register access object
   _elm_access_object_register(ao, ho);

   item->access_obj = ao;

   /* set owner widget item */
   ac = evas_object_data_get(ao, "_elm_access");
   ac->widget_item = item;
}

/**
 * @internal
 * @brief Unregisters accessibility for an Elm_Widget_Item.
 *
 * If the widget item has an associated access object (@c item->access_obj),
 * this function deletes that access object, which in turn triggers cleanup
 * of its resources and callbacks.
 *
 * @param item The Elm_Widget_Item_Data for the item whose accessibility is to be unregistered.
 */
EAPI void
_elm_access_widget_item_unregister(Elm_Widget_Item_Data *item)
{
   Evas_Object *ao;

   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);

   if (!item->access_obj) return;

   /* delete callbacks and unregister access object in _access_obj_del_cb*/
   ao = item->access_obj;
   item->access_obj = NULL;

   evas_object_del(ao);
}

/**
 * @internal
 * @brief Manages a timeout for detecting a "second click" (like a double-click).
 *
 * This function is used to differentiate between a single click and a rapid
 * succession of clicks that might be interpreted as a double-click for
 * accessibility purposes.
 * If a timer for a 2nd click is already active on @p obj, it's deleted, and
 * @c EINA_TRUE is returned (indicating a 2nd click was detected within the timeout).
 * Otherwise, a new timer is started, and @c EINA_FALSE is returned.
 *
 * @param obj The Evas_Object on which the click occurred.
 * @return @c EINA_TRUE if a 2nd click was detected within the timeout, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
_elm_access_2nd_click_timeout(Evas_Object *obj)
{
   Ecore_Timer *t;

   t = evas_object_data_get(obj, "_elm_2nd_timeout");
   if (t)
     {
        ecore_timer_del(t);
        evas_object_data_del(obj, "_elm_2nd_timeout");
        evas_object_event_callback_del_full(obj, EVAS_CALLBACK_DEL,
                                            _access_2nd_click_del_cb, NULL);
        return EINA_TRUE;
     }
   t = ecore_timer_add(0.3, _access_2nd_click_timeout_cb, obj);
   evas_object_data_set(obj, "_elm_2nd_timeout", t);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL,
                                  _access_2nd_click_del_cb, NULL);
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Creates a new Elm_Access object.
 * This is a wrapper around elm_legacy_add for the Elm_Access class.
 * @param parent The parent Evas_Object for the new access object.
 * @return The newly created Elm_Access Evas_Object, or NULL on failure.
 */
static Evas_Object *
_elm_access_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

EOLIAN static Eo *
_elm_access_efl_object_constructor(Eo *obj, void *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);

   return obj;
}

EAPI Evas_Object *
elm_access_object_register(Evas_Object *obj, Evas_Object *parent)
{
   return _access_object_register(obj, parent);
}

EAPI void
elm_access_object_unregister(Evas_Object *obj)
{
   _access_object_unregister(obj);
}

EAPI Evas_Object *
elm_access_object_get(const Evas_Object *obj)
{
   return evas_object_data_get(obj, "_part_access_obj");
}

EAPI void
elm_access_info_set(Evas_Object *obj, int type, const char *text)
{
   _elm_access_text_set(_elm_access_info_get(obj), type, text);
}

EAPI char *
elm_access_info_get(const Evas_Object *obj, int type)
{
   return _elm_access_text_get(_elm_access_info_get(obj), type, obj);
}

EAPI void
elm_access_info_cb_set(Evas_Object *obj, int type,
                          Elm_Access_Info_Cb func, const void *data)
{
   _elm_access_callback_set(_elm_access_info_get(obj), type, func, data);
}

EAPI void
elm_access_activate_cb_set(Evas_Object *obj,
                           Elm_Access_Activate_Cb  func, void *data)
{
   Elm_Access_Info *ac;

   ac = _elm_access_info_get(obj);
   if (!ac) return;

   ac->activate = func;
   ac->activate_data = data;
}

EAPI void
elm_access_say(const char *text)
{
   if (!text) return;

   _elm_access_say(text);
}

EAPI void
elm_access_highlight_set(Evas_Object* obj)
{
   _elm_access_highlight_set(obj);
}

EAPI Eina_Bool
elm_access_action(Evas_Object *obj, const Elm_Access_Action_Type type, Elm_Access_Action_Info *action_info)
{
   Evas *evas;
   Evas_Object *ho;
   Elm_Access_Action_Info *a = action_info;

   switch (type)
     {
      case ELM_ACCESS_ACTION_READ:
      case ELM_ACCESS_ACTION_HIGHLIGHT:
        evas = evas_object_evas_get(obj);
        if (!evas) return EINA_FALSE;

        evas_event_feed_mouse_in(evas, 0, NULL);

        _elm_access_mouse_event_enabled_set(EINA_TRUE);
        evas_event_feed_mouse_move(evas, a->x, a->y, 0, NULL);
        _elm_access_mouse_event_enabled_set(EINA_FALSE);

        ho = _access_highlight_object_get(obj);
        if (ho)
          _access_action_callback_call(ho, ELM_ACCESS_ACTION_READ, a);
        break;

      case ELM_ACCESS_ACTION_UNHIGHLIGHT:
        evas = evas_object_evas_get(obj);
        if (!evas) return EINA_FALSE;
        _elm_access_object_highlight_disable(evas);
        break;

      case ELM_ACCESS_ACTION_HIGHLIGHT_NEXT:
        if (a->highlight_cycle)
          _elm_access_highlight_cycle(obj, ELM_FOCUS_NEXT);
        else
          return _access_highlight_next_get(obj, ELM_FOCUS_NEXT);
        break;

      case ELM_ACCESS_ACTION_HIGHLIGHT_PREV:
        if (a->highlight_cycle)
          _elm_access_highlight_cycle(obj, ELM_FOCUS_PREVIOUS);
        else
          return _access_highlight_next_get(obj, ELM_FOCUS_PREVIOUS);
        break;

      case ELM_ACCESS_ACTION_ACTIVATE:
        _elm_access_highlight_object_activate(obj, EFL_UI_ACTIVATE_DEFAULT);
        break;

      case ELM_ACCESS_ACTION_UP:
        _elm_access_highlight_object_activate(obj, EFL_UI_ACTIVATE_UP);
        break;

      case ELM_ACCESS_ACTION_DOWN:
        _elm_access_highlight_object_activate(obj, EFL_UI_ACTIVATE_DOWN);
        break;

      case ELM_ACCESS_ACTION_SCROLL:
        //TODO: SCROLL HIGHLIGHT OBJECT
        break;

      case ELM_ACCESS_ACTION_BACK:
        break;

      default:
        break;
     }

   return EINA_TRUE;
}

EAPI void
elm_access_action_cb_set(Evas_Object *obj, const Elm_Access_Action_Type type, const Elm_Access_Action_Cb cb, const void *data)
{
   Action_Info *a;
   a =  evas_object_data_get(obj, "_elm_access_action_info");

   if (!a)
     {
        a = calloc(1, sizeof(Action_Info));
        evas_object_data_set(obj, "_elm_access_action_info", a);
     }

   a->obj = obj;
   a->fn[type].cb = cb;
   a->fn[type].user_data = (void *)data;
}
/**
 * @brief Set contextual information text for an accessible object.
 * @since 1.8
 *
 * This is a convenience function that sets the text for the
 * ELM_ACCESS_CONTEXT_INFO type.
 *
 * @param obj The accessible Evas_Object.
 * @param text The contextual information string.
 *
 * @ingroup Access
 */
EAPI void
elm_access_external_info_set(Evas_Object *obj, const char *text)
{
   _elm_access_text_set
     (_elm_access_info_get(obj), ELM_ACCESS_CONTEXT_INFO, text);
}

/**
 * @brief Get contextual information text from an accessible object.
 * @since 1.8
 *
 * This is a convenience function that retrieves the text for the
 * ELM_ACCESS_CONTEXT_INFO type.
 *
 * @param obj The accessible Evas_Object.
 * @return A newly allocated string containing the contextual information,
 *         or @c NULL if not set. The caller must free this string.
 *
 * @ingroup Access
 */
EAPI char *
elm_access_external_info_get(const Evas_Object *obj)
{
   Elm_Access_Info *ac;

   ac = _elm_access_info_get(obj);
   return _elm_access_text_get(ac, ELM_ACCESS_CONTEXT_INFO, obj);
}

EAPI void
elm_access_highlight_next_set(Evas_Object *obj, Elm_Highlight_Direction dir, Evas_Object *next)
{
   EINA_SAFETY_ON_FALSE_RETURN(obj);
   EINA_SAFETY_ON_FALSE_RETURN(next);

   Elm_Access_Info *info = _elm_access_info_get(obj);
   Elm_Access_Info *info_next = _elm_access_info_get(next);

   if (!info || !info_next)
     {
        ERR("There is no access information");
        return;
     }

   if (dir == ELM_HIGHLIGHT_DIR_NEXT)
     {
        info_next->prev = obj;
        info->next = next;
     }
   else if (dir == ELM_HIGHLIGHT_DIR_PREVIOUS)
     {
        info_next->next = obj;
        info->prev = next;
     }
   else
      ERR("Not supported focus direction for access highlight [%d]", dir);
}

EOLIAN static void
_elm_access_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @internal
 * @brief Executes an accessibility action based on a string command.
 *
 * This function is typically used by accessibility backends (like AT-SPI)
 * to trigger actions on an Elm_Access object. It parses the @p params string
 * (e.g., "highlight", "activate") and calls the appropriate internal
 * action callback.
 *
 * @param obj The Elm_Access Evas_Object on which to perform the action.
 * @param params A string describing the action to perform.
 *               Examples: "highlight", "unhighlight", "activate", "value,up".
 * @return @c EINA_TRUE if the action was recognized and attempted, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_access_atspi_action_do(Evas_Object *obj, const char *params)
{
   Eina_Bool ret;

   ret = EINA_FALSE;
   if (!strcmp(params, "highlight"))
      ret = _access_action_callback_call(obj, ELM_ACCESS_ACTION_HIGHLIGHT, NULL);
   else if (!strcmp(params, "unhighlight"))
      ret = _access_action_callback_call(obj, ELM_ACCESS_ACTION_UNHIGHLIGHT, NULL);
   else if (!strcmp(params, "highlight,next"))
      ret = _access_action_callback_call(obj, ELM_ACCESS_ACTION_HIGHLIGHT_NEXT, NULL);
   else if (!strcmp(params, "highlight,prev"))
      ret = _access_action_callback_call(obj, ELM_ACCESS_ACTION_HIGHLIGHT_PREV, NULL);
   else if (!strcmp(params, "activate"))
     {
        evas_object_smart_callback_call(obj, SIG_ACTIVATED, NULL);
        ret = _access_action_callback_call(obj, ELM_ACCESS_ACTION_ACTIVATE, NULL);
     }
   else if (!strcmp(params, "value,up"))
      ret = _access_action_callback_call(obj, ELM_ACCESS_ACTION_UP, NULL);
   else if (!strcmp(params, "value,down"))
      ret = _access_action_callback_call(obj, ELM_ACCESS_ACTION_DOWN, NULL);
   else if (!strcmp(params, "read"))
      ret = _access_action_callback_call(obj, ELM_ACCESS_ACTION_READ, NULL);

   return ret;
}

EOLIAN const Efl_Access_Action_Data *
_elm_access_efl_access_widget_action_elm_actions_get(const Eo *obj EINA_UNUSED, void *pd EINA_UNUSED)
{
   static Efl_Access_Action_Data atspi_actions[] = {
          { "highlight", NULL, "highlight", _access_atspi_action_do},
          { "unhighlight", NULL, "unhighlight", _access_atspi_action_do},
          { "highlight,next", NULL, "highlight,next", _access_atspi_action_do},
          { "highlight,prev", NULL, "highlight,prev", _access_atspi_action_do},
          { "activate", NULL, "activate", _access_atspi_action_do},
          { "value,up", NULL, "value,up", _access_atspi_action_do},
          { "value,down", NULL, "value,down", _access_atspi_action_do},
          { "read", NULL, "read", _access_atspi_action_do},
          { NULL, NULL, NULL, NULL }
   };
   return &atspi_actions[0];
}

EOLIAN static Efl_Access_State_Set
_elm_access_efl_access_object_state_set_get(const Eo *obj, void *pd EINA_UNUSED)
{
   Efl_Access_State_Set ret;
   ret = efl_access_object_state_set_get(efl_super(obj, ELM_ACCESS_CLASS));

   Elm_Access_Info *info = _elm_access_info_get(obj);
   if (info && !evas_object_visible_get(info->part_object))
     {
        STATE_TYPE_UNSET(ret, EFL_ACCESS_STATE_TYPE_VISIBLE);
        STATE_TYPE_UNSET(ret, EFL_ACCESS_STATE_TYPE_SHOWING);
     }

   return ret;
}

/* Internal EO APIs and hidden overrides */

#define ELM_ACCESS_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_OPS(elm_access)

#include "elm_access_eo.c"
