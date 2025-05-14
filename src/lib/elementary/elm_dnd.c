/**
 * @file
 * @brief These routines are for Elementary X DND (Drag & Drop) support.
 *
 * Elm DND is the Elementary abstraction layer for DND support. It may use
 * the Ecore_X DND module underneath, or other DND protocols.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif


#include <Elementary.h>
#include "elm_priv.h"

int ELM_CNP_EVENT_SELECTION_CHANGED;

/**
 * @internal
 * @brief Structure to hold information for a drop target.
 */
typedef struct {
   void *enter_data, *leave_data, *pos_data, *drop_data; /**< User data for the respective callbacks */
   Elm_Drag_State enter_cb; /**< Callback function when a drag enters the target */
   Elm_Drag_State leave_cb; /**< Callback function when a drag leaves the target */
   Elm_Drag_Pos pos_cb;     /**< Callback function when a drag position changes over the target */
   Elm_Drop_Cb drop_cb;      /**< Callback function when a drop occurs on the target */
   Eina_Array *mime_types;   /**< Array of supported MIME types (char *). E.g., ["text/plain", "text/uri-list"] */
   Elm_Sel_Format format;   /**< The selection format bitmask associated with this target */
   Elm_Xdnd_Action action;   /**< The current drag action */
} Elm_Drop_Target;

/**
 * @internal
 * @brief Get the default seat ID for a given Evas object.
 * @param obj The Evas object.
 * @return The seat ID.
 */
static int
_default_seat(const Eo *obj)
{
   return evas_device_seat_id_get(evas_default_device_get(evas_object_evas_get(obj), EVAS_DEVICE_CLASS_SEAT));
}

/**
 * @internal
 * @brief Convert an Elm_Xdnd_Action enum to its string representation.
 * @param action The Xdnd action.
 * @return The string representation of the action, or "unknown".
 */
static const char*
_action_to_string(Elm_Xdnd_Action action)
{
   if (action == ELM_XDND_ACTION_COPY) return "copy";
   if (action == ELM_XDND_ACTION_MOVE) return "move";
   if (action == ELM_XDND_ACTION_PRIVATE) return "private";
   if (action == ELM_XDND_ACTION_ASK) return "ask";
   if (action == ELM_XDND_ACTION_LIST) return "list";
   if (action == ELM_XDND_ACTION_LINK) return "link";
   if (action == ELM_XDND_ACTION_DESCRIPTION) return "description";
   return "unknown";
}

/**
 * @internal
 * @brief Convert a string representation of an action to Elm_Xdnd_Action enum.
 * @param action The string representation of the action.
 * @return The Elm_Xdnd_Action enum, or ELM_XDND_ACTION_UNKNOWN.
 */
static Elm_Xdnd_Action
_string_to_action(const char* action)
{
   if (eina_streq(action, "copy")) return ELM_XDND_ACTION_COPY;
   else if (eina_streq(action, "move")) return ELM_XDND_ACTION_MOVE;
   else if (eina_streq(action, "private")) return ELM_XDND_ACTION_PRIVATE;
   else if (eina_streq(action, "ask")) return ELM_XDND_ACTION_ASK;
   else if (eina_streq(action, "list")) return ELM_XDND_ACTION_LIST;
   else if (eina_streq(action, "link")) return ELM_XDND_ACTION_LINK;
   else if (eina_streq(action, "description")) return ELM_XDND_ACTION_DESCRIPTION;
   return ELM_XDND_ACTION_UNKNOWN;
}

/**
 * @internal
 * @brief Callback for when a drag enters a drop target.
 * Invokes the user-provided enter callback.
 * @param data The Elm_Drop_Target structure.
 * @param ev The Efl_Event details.
 */
static void
_enter_cb(void *data, const Efl_Event *ev)
{
   Elm_Drop_Target *target = data;

   if (target->enter_cb)
     target->enter_cb(target->enter_data, ev->object);
}

/**
 * @internal
 * @brief Callback for when a drag leaves a drop target.
 * Invokes the user-provided leave callback.
 * @param data The Elm_Drop_Target structure.
 * @param ev The Efl_Event details.
 */
static void
_leave_cb(void *data, const Efl_Event *ev)
{
   Elm_Drop_Target *target = data;

   if (target->leave_cb)
     target->leave_cb(target->leave_data, ev->object);
}

/**
 * @internal
 * @brief Callback for when a drag position changes over a drop target.
 * Invokes the user-provided position callback.
 * @param data The Elm_Drop_Target structure.
 * @param ev The Efl_Event details, containing Efl_Ui_Drop_Event.
 */
static void
_pos_cb(void *data, const Efl_Event *ev)
{
   Elm_Drop_Target *target = data;
   Efl_Ui_Drop_Event *event = ev->info;

   if (target->pos_cb)
     target->pos_cb(target->pos_data, ev->object, event->position.x, event->position.y, target->action); //FIXME action
}

/**
 * @internal
 * @brief Delivers the dropped content to the user's drop callback.
 * This function is called when the future for efl_ui_dnd_drop_data_get resolves.
 * @param obj The Evas object that received the drop.
 * @param data The Elm_Drop_Target structure.
 * @param value The Eina_Value containing the dropped data (Eina_Content).
 * @return EINA_VALUE_EMPTY.
 */
static Eina_Value
_deliver_content(Eo *obj, void *data, const Eina_Value value)
{
   Elm_Drop_Target *target = data;
   Elm_Selection_Data sel_data;
   Eina_Content *content = eina_value_to_content(&value);

   sel_data.data = (void*)eina_content_data_get(content).mem;
   sel_data.len = eina_content_data_get(content).len;
   sel_data.action = target->action;
   sel_data.format = target->format;

   if (target->drop_cb)
     target->drop_cb(target->drop_data, obj, &sel_data);

   return EINA_VALUE_EMPTY;
}

/**
 * @internal
 * @brief Callback for when a drop occurs on a drop target.
 * Initiates fetching the drop data and then delivers it via _deliver_content.
 * @param data The Elm_Drop_Target structure.
 * @param ev The Efl_Event details, containing Efl_Ui_Drop_Dropped_Event.
 */
static void
_drop_cb(void *data, const Efl_Event *ev)
{
   Efl_Ui_Drop_Dropped_Event *event = ev->info;
   Elm_Drop_Target *target = data;
   target->action = _string_to_action(event->action);
   efl_future_then(ev->object, efl_ui_dnd_drop_data_get(elm_widget_is(ev->object) ? ev->object : efl_ui_win_get(ev->object), _default_seat(ev->object), eina_array_iterator_new(target->mime_types)),
    .success = _deliver_content,
    .data = target
   );
}

/**
 * @internal
 * @brief Callback for when the drop target object is invalidated (deleted).
 * Cleans up the drop target registration.
 * @param data The Elm_Drop_Target structure.
 * @param ev The Efl_Event details.
 */
static void
_inv_cb(void *data, const Efl_Event *ev)
{
   Elm_Drop_Target *target = data;
   elm_drop_target_del(ev->object, target->format, target->enter_cb, target->enter_data, target->leave_cb,
                       target->leave_data, target->pos_cb, target->pos_data, target->drop_cb, target->drop_data);
}

/**
 * @internal
 * @brief Array of EFL event callbacks for drop target functionality.
 */
EFL_CALLBACKS_ARRAY_DEFINE(drop_target_cb,
  {EFL_UI_DND_EVENT_DROP_ENTERED, _enter_cb},
  {EFL_UI_DND_EVENT_DROP_LEFT, _leave_cb},
  {EFL_UI_DND_EVENT_DROP_POSITION_CHANGED, _pos_cb},
  {EFL_UI_DND_EVENT_DROP_DROPPED, _drop_cb},
  {EFL_EVENT_INVALIDATE, _inv_cb}
)

static Eina_Hash *target_register = NULL; /**< @internal Hash table to register drop targets per object. Key: Evas_Object*, Value: Eina_List of Elm_Drop_Target* */

/**
 * @internal
 * @brief Convert an Elm_Sel_Format bitmask to an Eina_Array of MIME type strings.
 * @param format The Elm_Sel_Format bitmask.
 *        Example: ELM_SEL_FORMAT_TEXT | ELM_SEL_FORMAT_URILIST
 * @return A new Eina_Array containing char* MIME types.
 *         The caller is responsible for freeing this array using eina_array_free().
 *         The strings inside are static and should not be freed.
 *         Example structure for (ELM_SEL_FORMAT_TEXT | ELM_SEL_FORMAT_URILIST):
 *         Eina_Array* -> ["text/uri-list", "text/plain", "text/plain;charset=utf-8"]
 *         (Note: "text/uri-list" is added again if ELM_SEL_FORMAT_TEXT is present without ELM_SEL_FORMAT_URILIST,
 *         but the example above assumes both are present, so it's added once by ELM_SEL_FORMAT_URILIST).
 */
static Eina_Array*
_format_to_mime_array(Elm_Sel_Format format)
{
   Eina_Array *ret = eina_array_new(10);

   if (format & ELM_SEL_FORMAT_URILIST)
     eina_array_push(ret, "text/uri-list");
   if (format & ELM_SEL_FORMAT_TEXT)
     {
        eina_array_push(ret, "text/plain");
        eina_array_push(ret, "text/plain;charset=utf-8");
        if (!(format & ELM_SEL_FORMAT_URILIST))
          eina_array_push(ret, "text/uri-list");
     }
   if (format & ELM_SEL_FORMAT_MARKUP)
     eina_array_push(ret, "application/x-elementary-markup");
   if (format & ELM_SEL_FORMAT_IMAGE)
     {
        eina_array_push(ret, "image/png");
        eina_array_push(ret, "image/jpeg");
        eina_array_push(ret, "image/x-ms-bmp");
        eina_array_push(ret, "image/gif");
        eina_array_push(ret, "image/tiff");
        eina_array_push(ret, "image/svg+xml");
        eina_array_push(ret, "image/x-xpixmap");
        eina_array_push(ret, "image/x-tga");
        eina_array_push(ret, "image/x-portable-pixmap");
     }
   if (format & ELM_SEL_FORMAT_VCARD)
     eina_array_push(ret, "text/vcard");
   if (format & ELM_SEL_FORMAT_HTML)
     eina_array_push(ret, "text/html");

   return ret;
}

/**
 * @brief Add drop target capability to an Evas object.
 *
 * This function allows an Evas object to become a target for drag and drop
 * operations. Callbacks can be provided for various drag events like enter,
 * leave, position change, and the actual drop.
 *
 * @param obj The Evas object to make a drop target.
 * @param format A bitmask of Elm_Sel_Format types that this target accepts.
 *        Example: ELM_SEL_FORMAT_TEXT | ELM_SEL_FORMAT_IMAGE
 * @param enter_cb Function called when a drag enters the object. Can be NULL.
 * @param enter_data User data for the enter_cb.
 * @param leave_cb Function called when a drag leaves the object. Can be NULL.
 * @param leave_data User data for the leave_cb.
 * @param pos_cb Function called when a drag moves over the object. Can be NULL.
 * @param pos_data User data for the pos_cb.
 * @param drop_cb Function called when a drop occurs on the object. Can be NULL.
 * @param drop_data User data for the drop_cb.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool
elm_drop_target_add(Evas_Object *obj, Elm_Sel_Format format,
                    Elm_Drag_State enter_cb, void *enter_data,
                    Elm_Drag_State leave_cb, void *leave_data,
                    Elm_Drag_Pos pos_cb, void *pos_data,
                    Elm_Drop_Cb drop_cb, void *drop_data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, EINA_FALSE);

   Elm_Drop_Target *target = calloc(1, sizeof(Elm_Drop_Target));
   target->enter_cb = enter_cb;
   target->enter_data = enter_data;
   target->leave_cb = leave_cb;
   target->leave_data = leave_data;
   target->pos_cb = pos_cb;
   target->pos_data = pos_data;
   target->drop_cb = drop_cb;
   target->drop_data = drop_data;
   target->mime_types = _format_to_mime_array(format);
   target->format = format;

   efl_event_callback_array_add(obj, drop_target_cb(), target);
   if (!efl_isa(obj, EFL_UI_WIDGET_CLASS))
     _drop_event_register(obj); //this is ensuring that we are also supporting none widgets
   if (!target_register)
     target_register = eina_hash_pointer_new(NULL);
   eina_hash_list_append(target_register, &obj, target);

   return EINA_TRUE;
}

/**
 * @brief Remove drop target capability from an Evas object.
 *
 * This function removes a previously added drop target configuration.
 * All parameters must match those used in elm_drop_target_add() to
 * identify the specific target configuration to remove.
 *
 * @param obj The Evas object.
 * @param format The Elm_Sel_Format bitmask used when adding.
 * @param enter_cb The enter_cb function used when adding.
 * @param enter_data The enter_data used when adding.
 * @param leave_cb The leave_cb function used when adding.
 * @param leave_data The leave_data used when adding.
 * @param pos_cb The pos_cb function used when adding.
 * @param pos_data The pos_data used when adding.
 * @param drop_cb The drop_cb function used when adding.
 * @param drop_data The drop_data used when adding.
 * @return EINA_TRUE on success or if not found, EINA_FALSE on error.
 */
EAPI Eina_Bool
elm_drop_target_del(Evas_Object *obj, Elm_Sel_Format format,
                    Elm_Drag_State enter_cb, void *enter_data,
                    Elm_Drag_State leave_cb, void *leave_data,
                    Elm_Drag_Pos pos_cb, void *pos_data,
                    Elm_Drop_Cb drop_cb, void *drop_data)
{
   Elm_Drop_Target *target;
   Eina_List *n, *found = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, EINA_FALSE);

   if (!target_register)
     return EINA_TRUE;
   Eina_List *targets = eina_hash_find(target_register, &obj);

   if (!targets)
     return EINA_TRUE;

   EINA_LIST_FOREACH(targets, n, target)
     {
        if (target->enter_cb == enter_cb && target->enter_data == enter_data &&
            target->leave_cb == leave_cb && target->leave_data == leave_data &&
            target->pos_cb == pos_cb && target->pos_data == pos_data &&
            target->drop_cb == drop_cb && target->drop_data == drop_data &&
            target->format == format)
          {

             found = n;
             break;
          }
     }
   if (found)
     {
        efl_event_callback_array_del(obj, drop_target_cb(), eina_list_data_get(found));
        eina_hash_list_remove(target_register, &obj, target);
        eina_array_free(target->mime_types);
        _drop_event_unregister(obj); //this is ensuring that we are also supporting none widgets
        free(target);
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Structure to hold information for item container drag support.
 */
struct _Item_Container_Drag_Info
{  /* Info kept for containers to support drag */
   Evas_Object *obj; /**< The container object being dragged from */
   Ecore_Timer *tm;    /**< Timer that, when expired, starts the drag or animation */
   double anim_tm;  /**< Duration for the drag start animation */
   double tm_to_drag;  /**< Delay before starting drag (or animation if anim_tm > 0) */
   Elm_Xy_Item_Get_Cb itemgetcb; /**< Callback to get the item under specific coordinates */
   Elm_Item_Container_Data_Get_Cb data_get; /**< Callback to get drag data for an item */

   Evas_Coord x_down;  /**< X coordinate of mouse down event */
   Evas_Coord y_down;  /**< Y coordinate of mouse down event */

   /* Some extra information needed to impl default anim */
   Evas *e; /**< Evas canvas */
   Eina_List *icons;   /**< List of Anim_Icon structures for animation */
   int final_icon_w; /**< Width of the final drag icon (used in animation) */
   int final_icon_h; /**< Height of the final drag icon (used in animation) */
   Ecore_Animator *ea; /**< Ecore animator for the drag start animation */

   Elm_Drag_User_Info user_info; /**< User-provided information for the drag operation */
};
typedef struct _Item_Container_Drag_Info Item_Container_Drag_Info;

/**
 * @internal
 * @brief Structure to hold information for an icon being animated during drag initiation.
 */
struct _Anim_Icon
{
   int start_x; /**< Initial X coordinate of the icon */
   int start_y; /**< Initial Y coordinate of the icon */
   int start_w; /**< Initial width of the icon */
   int start_h; /**< Initial height of the icon */
   Evas_Object *o; /**< The Evas_Object representing the icon */
};
typedef struct _Anim_Icon Anim_Icon;
static Eina_List *cont_drag_tg = NULL; /**< @internal List of Item_Container_Drag_Info structures for active container drags */

static Eina_Bool elm_drag_item_container_del_internal(Evas_Object *obj, Eina_Bool full);
static void _cont_obj_mouse_move(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info);
static void _cont_obj_mouse_up(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info);
static void _cont_obj_mouse_down(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info);

/**
 * @internal
 * @brief Callback invoked when a container drag operation is finished or cancelled.
 * @param data The Item_Container_Drag_Info structure.
 * @param obj The object associated with the drag (unused here).
 */
static void
_cont_drag_done_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   Item_Container_Drag_Info *st = data;
   elm_widget_scroll_freeze_pop(st->obj);
   if (st->user_info.dragdone)
     st->user_info.dragdone(st->user_info.donecbdata, NULL, EINA_FALSE); /*FIXME: Second obj param should be the one drag was done on? Accepted status? */
}

/**
 * @internal
 * @brief Actually starts the drag operation for an item container.
 * This is called after any initial timer or animation completes.
 * @param data The Item_Container_Drag_Info structure.
 * @return ECORE_CALLBACK_CANCEL to stop further timer processing.
 */
static Eina_Bool
_cont_obj_drag_start(void *data)
{  /* Start a drag-action when timer expires */
   Item_Container_Drag_Info *st = data;
   st->tm = NULL;
   Elm_Drag_User_Info *info = &st->user_info;
   if (info->dragstart) info->dragstart(info->startcbdata, st->obj);
   elm_widget_scroll_freeze_push(st->obj);
   evas_object_event_callback_del_full
      (st->obj, EVAS_CALLBACK_MOUSE_MOVE, _cont_obj_mouse_move, st);
   elm_drag_start(  /* Commit the start only if data_get successful */
         st->obj, info->format,
         info->data, info->action,
         info->createicon, info->createdata,
         info->dragpos, info->dragdata,
         info->acceptcb, info->acceptdata,
         _cont_drag_done_cb, st);
   ELM_SAFE_FREE(info->data, free);

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @internal
 * @brief Creates a list of Anim_Icon structures from a list of Evas_Object icons.
 * This is used to prepare icons for the drag start animation.
 * @param icons An Eina_List of Evas_Object* representing the icons to animate.
 *        Example: Eina_List containing [Evas_Object_Icon1, Evas_Object_Icon2]
 * @return A new Eina_List of Anim_Icon* structures. Caller frees this list and its contents.
 *         Example structure: Eina_List containing [Anim_Icon_Struct1, Anim_Icon_Struct2]
 *         where Anim_Icon_Struct contains geometry and the Evas_Object.
 */
static inline Eina_List *
_anim_icons_make(Eina_List *icons)
{  /* Make local copies of all icons, add them to list */
   Eina_List *list = NULL, *itr;
   Evas_Object *o;

   EINA_LIST_FOREACH(icons, itr, o)
     {  /* Now add icons to animation window */
        Anim_Icon *st = calloc(1, sizeof(*st));

        if (!st)
          {
             ERR("Failed to allocate memory for icon!");
             continue;
          }

        evas_object_geometry_get(o, &st->start_x, &st->start_y, &st->start_w, &st->start_h);
        evas_object_show(o);
        st->o = o;
        list = eina_list_append(list, st);
     }

   return list;
}

/**
 * @internal
 * @brief Animator function for the drag start icon animation.
 * This function is called by ecore_animator at each frame.
 * It animates the icons from their start position/size towards the mouse
 * pointer and final icon size.
 * @param data The Item_Container_Drag_Info structure.
 * @param pos The current position in the animation timeline (0.0 to 1.0).
 * @return ECORE_CALLBACK_RENEW to continue animation, ECORE_CALLBACK_CANCEL to stop.
 */
static Eina_Bool
_drag_anim_play(void *data, double pos)
{  /* Impl of the animation of icons, called on frame time */
   Item_Container_Drag_Info *st = data;
   Eina_List *l;
   Anim_Icon *sti;

   if (st->ea)
     {
        if (pos > 0.99) // Animation finished
          {
             st->ea = NULL;  /* Avoid deleting on mouse up if animation finishes normally */
             EINA_LIST_FOREACH(st->icons, l, sti)
                evas_object_hide(sti->o); // Hide animated icons

             _cont_obj_drag_start(st);  /* Proceed to actual drag start */
             return ECORE_CALLBACK_CANCEL;
          }

        Evas_Coord xm, ym;
        evas_pointer_canvas_xy_get(st->e, &xm, &ym); // Current mouse position
        EINA_LIST_FOREACH(st->icons, l, sti)
          {
             // Interpolate position and size
             int x, y, h, w;
             w = sti->start_w + ((st->final_icon_w - sti->start_w) * pos);
             h = sti->start_h + ((st->final_icon_h - sti->start_h) * pos);
             // Move towards mouse pointer
             x = sti->start_x - (pos * ((sti->start_x + (sti->start_w/2) - xm) - (w/2 - sti->start_w/2))); // Center of icon moves towards mouse
             y = sti->start_y - (pos * ((sti->start_y + (sti->start_h/2) - ym) - (h/2 - sti->start_h/2)));
             evas_object_move(sti->o, x, y);
             evas_object_resize(sti->o, w, h);
          }

        return ECORE_CALLBACK_RENEW;
     }

   return ECORE_CALLBACK_CANCEL; // Animator was deleted elsewhere (e.g., mouse up)
}

/**
 * @internal
 * @brief Starts the default drag initiation animation.
 * It prepares the icons and starts an ecore animator.
 * @param data The Item_Container_Drag_Info structure.
 * @return EINA_FALSE, as this function doesn't act as an Ecore timer callback returning cancel/renew.
 */
static inline Eina_Bool
_drag_anim_start(void *data)
{  /* Start default animation */
   Item_Container_Drag_Info *st = data;

   st->tm = NULL; // Timer that triggered this is now handled
   /* Now we need to build an (Anim_Icon *) list */
   st->icons = _anim_icons_make(st->user_info.icons); // Prepare Anim_Icon list from user's Evas_Object list
   if (st->user_info.createicon) // If user provides a way to create the final drag icon
     {
        // Create a temporary window to realize the final drag icon and get its size
        Evas_Object *temp_win = elm_win_add(NULL, "Temp", ELM_WIN_DND);
        Evas_Object *final_icon = st->user_info.createicon(st->user_info.createdata, temp_win, NULL, NULL);
        evas_object_geometry_get(final_icon, NULL, NULL, &st->final_icon_w, &st->final_icon_h);
        evas_object_del(final_icon);
        evas_object_del(temp_win);
     }
   st->ea = ecore_animator_timeline_add(st->anim_tm, _drag_anim_play, st); // Start the animation

   return EINA_FALSE; // Not a timer callback, so return value doesn't signify renew/cancel
}

/**
 * @internal
 * @brief Called when the initial timer (tm_to_drag) expires after mouse down.
 * This function determines if an animation should start or if dragging should
 * commence immediately. It also gathers drag data using the data_get callback.
 * @param data The Item_Container_Drag_Info structure.
 * @return ECORE_CALLBACK_CANCEL to stop further timer processing.
 */
static Eina_Bool
_cont_obj_anim_start(void *data)
{  /* Start a drag-action when timer expires */
   Item_Container_Drag_Info *st = data;
   int xposret, yposret;  /* Relative coordinates within the item, unused by this function directly */
   Elm_Object_Item *it = (st->itemgetcb) ? // Get the item under the mouse down position
      (st->itemgetcb(st->obj, st->x_down, st->y_down, &xposret, &yposret))
      : NULL;

   st->tm = NULL; // Timer is consumed
   st->user_info.format = ELM_SEL_FORMAT_TARGETS; /* Default */
   st->icons = NULL;
   st->user_info.data = NULL;
   st->user_info.action = ELM_XDND_ACTION_COPY;  /* Default */

   if (!it)   /* Failed to get mouse-down item, abort drag */
     return ECORE_CALLBACK_CANCEL;

   if (st->data_get)
     {  /* collect info then start animation or start dragging */
        if (st->data_get(    /* Collect drag info */
                 st->obj,      /* The container object */
                 it,           /* Drag started on this item */
                 &st->user_info))
          {
             if (st->user_info.icons)
               _drag_anim_start(st);
             else
               {
                  if (EINA_DBL_NONZERO(st->anim_tm))
                    {
                       // even if we don't manage the icons animation, we have
                       // to wait until it is finished before beginning drag.
                       st->tm = ecore_timer_add(st->anim_tm, _cont_obj_drag_start, st);
                    }
                  else
                    _cont_obj_drag_start(st);  /* Start dragging, no anim */
               }
          }
     }

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @internal
 * @brief Comparison function for searching Item_Container_Drag_Info in a list by object pointer.
 * @param d1 Pointer to an Item_Container_Drag_Info structure.
 * @param d2 Pointer to an Evas_Object* (obj).
 * @return Difference between the object pointers.
 */
static int
_drag_item_container_cmp(const void *d1,
               const void *d2)
{
   const Item_Container_Drag_Info *st = d1;
   return (((uintptr_t) (st->obj)) - ((uintptr_t) d2));
}

/**
 * @internal
 * @brief Frees resources associated with a drag start animation.
 * Stops any ongoing animator and frees the list of animation icons.
 * @param st The Item_Container_Drag_Info structure whose animation state needs freeing.
 */
void
_anim_st_free(Item_Container_Drag_Info *st)
{  /* Stops and free mem of ongoing animation */
   if (st)
     {
        ELM_SAFE_FREE(st->ea, ecore_animator_del); // Stop animator
        Anim_Icon *sti;

        EINA_LIST_FREE(st->icons, sti) // Free animated icons list
          {
             evas_object_del(sti->o); // Delete icon object
             free(sti); // Free Anim_Icon struct
          }

        st->icons = NULL;
     }
}

/**
 * @internal
 * @brief Mouse up event callback for an item container.
 * Cancels any pending drag start timer or ongoing drag start animation.
 * @param data The Item_Container_Drag_Info structure.
 * @param e Evas canvas (unused).
 * @param obj The Evas object that received the event (unused).
 * @param event_info Evas_Event_Mouse_Up details.
 */
static void
_cont_obj_mouse_up(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{  /* Cancel any drag waiting to start on timeout */
   Item_Container_Drag_Info *st = data;

   if (((Evas_Event_Mouse_Up *)event_info)->button != 1) // Only care about left mouse button
     return;

   // Remove mouse move and up callbacks that were added on mouse down
   evas_object_event_callback_del_full
      (st->obj, EVAS_CALLBACK_MOUSE_MOVE, _cont_obj_mouse_move, st);
   evas_object_event_callback_del_full
      (st->obj, EVAS_CALLBACK_MOUSE_UP, _cont_obj_mouse_up, st);

   ELM_SAFE_FREE(st->tm, ecore_timer_del); // Delete pending drag start timer

   _anim_st_free(st); // Clean up any animation state
}

/**
 * @internal
 * @brief Mouse move event callback for an item container.
 * If the mouse moves significantly while pressed (before drag starts) and the
 * event is marked as "on_hold" (usually meaning a gesture like scroll is
 * starting), it cancels the pending drag operation.
 * @param data The Item_Container_Drag_Info structure.
 * @param e Evas canvas (unused).
 * @param obj The Evas object that received the event (unused).
 * @param event_info Evas_Event_Mouse_Move details.
 */
static void
_cont_obj_mouse_move(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{  /* Cancel any drag waiting to start on timeout */
   // If event is on hold (e.g. scroller took over), cancel drag initiation
   if (((Evas_Event_Mouse_Move *)event_info)->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        Item_Container_Drag_Info *st = data;

        // Remove mouse move and up callbacks
        evas_object_event_callback_del_full
           (st->obj, EVAS_CALLBACK_MOUSE_MOVE, _cont_obj_mouse_move, st);
        evas_object_event_callback_del_full
           (st->obj, EVAS_CALLBACK_MOUSE_UP, _cont_obj_mouse_up, st);
        // elm_drag_item_container_del_internal(st->obj, EINA_FALSE); // obj here is the one from params, should be st->obj

        ELM_SAFE_FREE(st->tm, ecore_timer_del); // Delete pending drag start timer

        _anim_st_free(st); // Clean up animation state
        // Note: elm_drag_item_container_del_internal is not called here anymore,
        // which seems correct as we are just cancelling the *initiation*.
        // The main mouse_down callback is still active for future drag attempts.
     }
}

/**
 * @internal
 * @brief Internal function to delete item container drag support.
 * @param obj The container object.
 * @param full If EINA_TRUE, fully remove and free resources. If EINA_FALSE,
 *        only clean up active state (timers, animators) but keep registration
 *        for potential update by elm_drag_item_container_add.
 * @return EINA_TRUE if found and processed, EINA_FALSE otherwise.
 */
static Eina_Bool
elm_drag_item_container_del_internal(Evas_Object *obj, Eina_Bool full)
{
   Item_Container_Drag_Info *st =
      eina_list_search_unsorted(cont_drag_tg, _drag_item_container_cmp, obj);

   if (st)
     {
        ELM_SAFE_FREE(st->tm, ecore_timer_del); /* Cancel drag-start timer */

        if (st->ea)  /* Cancel ongoing default animation */
          _anim_st_free(st);

        if (full) // Full deletion
          {
             st->itemgetcb = NULL;
             st->data_get = NULL;
             evas_object_event_callback_del_full // Remove mouse down listener
                (obj, EVAS_CALLBACK_MOUSE_DOWN, _cont_obj_mouse_down, st);

             cont_drag_tg = eina_list_remove(cont_drag_tg, st); // Remove from global list
             ELM_SAFE_FREE(st->user_info.data, free); // Free any user data string
             free(st); // Free the info struct
          }

        return EINA_TRUE;
     }
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Mouse down event callback for an item container to initiate drag.
 * Sets up a timer to start the drag process (either animation or direct drag).
 * @param data The Item_Container_Drag_Info structure.
 * @param e The Evas canvas.
 * @param obj The Evas object that received the event (unused).
 * @param event_info Evas_Event_Mouse_Down details.
 */
static void
_cont_obj_mouse_down(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{  /* Launch a timer to start dragging */
   Evas_Event_Mouse_Down *ev = event_info;
   if (ev->button != 1) // Only care about left mouse button
     return;

   Item_Container_Drag_Info *st = data;
   // Add mouse move and up callbacks to monitor drag cancellation or completion
   evas_object_event_callback_add(st->obj, EVAS_CALLBACK_MOUSE_MOVE,
         _cont_obj_mouse_move, st);

   evas_object_event_callback_add(st->obj, EVAS_CALLBACK_MOUSE_UP,
         _cont_obj_mouse_up, st);

   ELM_SAFE_FREE(st->tm, ecore_timer_del); // Delete any pre-existing timer for this object

   st->e = e; // Store canvas
   st->x_down = ev->canvas.x; // Store mouse down coordinates
   st->y_down = ev->canvas.y;
   // Start timer: after tm_to_drag, _cont_obj_anim_start will be called
   st->tm = ecore_timer_add(st->tm_to_drag, _cont_obj_anim_start, st);
}

/**
 * @brief Remove drag support for an item container.
 *
 * This function disables the drag capabilities previously added to the
 * container object with elm_drag_item_container_add().
 *
 * @param obj The item container object.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
elm_drag_item_container_del(Evas_Object *obj)
{
   return elm_drag_item_container_del_internal(obj, EINA_TRUE);
}

/**
 * @brief Add drag support for an item container (like a list or grid).
 *
 * This enables items within a container to be dragged. It requires callbacks
 * to identify items under the cursor and to fetch data for the drag operation.
 *
 * @param obj The container object (e.g., a genlist).
 * @param anim_tm Time in seconds for the drag start animation (e.g., icons merging).
 *        If 0, no animation, drag starts after tm_to_drag.
 * @param tm_to_drag Time in seconds to wait after mouse down before starting
 *        the drag (or drag animation if anim_tm > 0).
 * @param itemgetcb Callback to find which item is at given coordinates.
 *        Prototype: Elm_Object_Item *(*Elm_Xy_Item_Get_Cb)(Evas_Object *obj, Evas_Coord x, Evas_Coord y, int *xposret, int *yposret);
 *        It should return the item at (x, y) canvas coordinates.
 *        xposret, yposret can be filled with relative coordinates within the item.
 * @param data_get Callback to retrieve data and settings for the drag.
 *        Prototype: Eina_Bool (*Elm_Item_Container_Data_Get_Cb)(Evas_Object *obj, Elm_Object_Item *it, Elm_Drag_User_Info *info);
 *        This function should fill the Elm_Drag_User_Info struct.
 *        `info->data` should be a string for the data.
 *        `info->format` the format of the data.
 *        `info->icons` can be an Eina_List of Evas_Object* to be animated.
 *        `info->createicon` callback to create the final drag icon.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool
elm_drag_item_container_add(Evas_Object *obj,
                            double anim_tm,
                            double tm_to_drag,
                            Elm_Xy_Item_Get_Cb itemgetcb,
                            Elm_Item_Container_Data_Get_Cb data_get)
{
   Item_Container_Drag_Info *st;

   // Try to update if already exists, otherwise create new
   if (elm_drag_item_container_del_internal(obj, EINA_FALSE)) // EINA_FALSE: don't fully delete, just clean state
     {  /* Updating info of existing obj */
        st = eina_list_search_unsorted(cont_drag_tg, _drag_item_container_cmp, obj);
        if (!st) return EINA_FALSE; // Should not happen if del_internal returned TRUE
     }
   else // New registration
     {
        st = calloc(1, sizeof(*st));
        if (!st) return EINA_FALSE;

        st->obj = obj;
        cont_drag_tg = eina_list_append(cont_drag_tg, st);

        /* Register for mouse callback for container to start/abort drag */
        evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN,
                                       _cont_obj_mouse_down, st);
     }

   st->tm = NULL; // Ensure timer is initially null
   st->anim_tm = anim_tm;
   st->tm_to_drag = tm_to_drag;
   st->itemgetcb = itemgetcb;
   st->data_get = data_get;
   return EINA_TRUE;
}

static Eina_List *cont_drop_tg = NULL; /* List of Item_Container_Drop_Info */

struct _Item_Container_Drop_Info
{  /* Info kept for containers to support drop */
   Evas_Object *obj; /**< The container object that can receive drops */
   Elm_Xy_Item_Get_Cb itemgetcb; /**< Callback to get the item under specific coordinates */
   Elm_Drop_Item_Container_Cb dropcb; /**< Callback when a drop occurs on an item */
   Elm_Drag_Item_Container_Pos poscb; /**< Callback when drag position changes over an item */
};
typedef struct _Item_Container_Drop_Info Item_Container_Drop_Info;


/**
 * @internal
 * @brief Structure for managing drop capabilities on an object (seems partially used or legacy).
 * This structure appears to be related to an older or different approach to handling drops,
 * possibly before the full integration with EFL DND events. The `target_register` and
 * `Elm_Drop_Target` seem to be the primary mechanism now.
 */
typedef struct
{
   Evas_Object    *obj; /**< The object that can receive drops */
   /* FIXME: Cache window */
   Eina_Inlist    *cbs_list; /**< List of Dropable_Cbs (definition not visible here) */
   struct {
      Evas_Coord      x, y; /**< Last known coordinates of the drag */
      Eina_Bool       in : 1; /**< Flag: is the drag currently inside the object? */
      const char     *type; /**< Last known type/format of the drag */
      Elm_Sel_Format  format; /**< Last known Elm_Sel_Format of the drag */
   } last; /**< Information about the last drag event over this dropable */
} Dropable;

/**
 * @internal
 * @brief Comparison function for searching Item_Container_Drop_Info in a list by object pointer.
 * @param d1 Pointer to an Item_Container_Drop_Info structure.
 * @param d2 Pointer to an Evas_Object* (obj).
 * @return Difference between the object pointers.
 */
static int
_drop_item_container_cmp(const void *d1,
               const void *d2)
{
   const Item_Container_Drop_Info *st = d1;
   return (((uintptr_t) (st->obj)) - ((uintptr_t) d2));
}

/**
 * @internal
 * @brief Callback for drag position changes over an item container.
 * This is an intermediary callback registered with elm_drop_target_add.
 * It resolves the specific item under the cursor and then calls the
 * user-provided container-specific position callback (poscb).
 * @param data User data for the container's poscb.
 * @param obj The container Evas_Object.
 * @param x X coordinate relative to the container object.
 * @param y Y coordinate relative to the container object.
 * @param action The current drag action.
 */
static void
_elm_item_container_pos_cb(void *data, Evas_Object *obj, Evas_Coord x, Evas_Coord y, Elm_Xdnd_Action action)
{  /* obj is the container pointer */
   Elm_Object_Item *it = NULL;
   int xposret = 0; // Relative X within item
   int yposret = 0; // Relative Y within item

   Item_Container_Drop_Info *st =
      eina_list_search_unsorted(cont_drop_tg, _drop_item_container_cmp, obj);

   if (st && st->poscb)
     {  /* Call container drop func with specific item pointer */
        int xo = 0; // Container's X offset on canvas
        int yo = 0; // Container's Y offset on canvas

        evas_object_geometry_get(obj, &xo, &yo, NULL, NULL);
        if (st->itemgetcb) // If item getter is provided
          // Convert canvas relative coords (x+xo, y+yo) to find item
          it = st->itemgetcb(obj, x + xo, y + yo, &xposret, &yposret);

        // Call user's position callback
        st->poscb(data, obj, it, x, y, xposret, yposret, action);
     }
}

/**
 * @internal
 * @brief Callback for a drop occurring on an item container.
 * This is an intermediary callback registered with elm_drop_target_add.
 * It resolves the specific item under the cursor and then calls the
 * user-provided container-specific drop callback (dropcb).
 * @param data User data for the container's dropcb.
 * @param obj The container Evas_Object.
 * @param ev The Elm_Selection_Data containing drop information.
 * @return EINA_TRUE if the drop was handled, EINA_FALSE otherwise (from user's dropcb).
 */
static Eina_Bool
_elm_item_container_drop_cb(void *data, Evas_Object *obj , Elm_Selection_Data *ev)
{  /* obj is the container pointer */
   Elm_Object_Item *it = NULL;
   int xposret = 0; // Relative X within item
   int yposret = 0; // Relative Y within item

   Item_Container_Drop_Info *st =
      eina_list_search_unsorted(cont_drop_tg, _drop_item_container_cmp, obj);

   if (st && st->dropcb)
     {  /* Call container drop func with specific item pointer */
        int xo = 0; // Container's X offset on canvas
        int yo = 0; // Container's Y offset on canvas

        evas_object_geometry_get(obj, &xo, &yo, NULL, NULL);
        if (st->itemgetcb) // If item getter is provided
          // Convert canvas relative drop coords (ev->x+xo, ev->y+yo) to find item
          it = st->itemgetcb(obj, ev->x + xo, ev->y + yo, &xposret, &yposret);

        // Call user's drop callback
        return st->dropcb(data, obj, it, ev, xposret, yposret);
     }

   return EINA_FALSE; // Drop not handled
}

/**
 * @internal
 * @brief Internal function to delete item container drop support.
 * @param obj The container object.
 * @param full If EINA_TRUE, fully remove and free resources. If EINA_FALSE,
 *        only clear callbacks in the struct (for potential update).
 * @return EINA_TRUE if found and processed, EINA_FALSE otherwise.
 */
static Eina_Bool
elm_drop_item_container_del_internal(Evas_Object *obj, Eina_Bool full)
{
   Item_Container_Drop_Info *st =
      eina_list_search_unsorted(cont_drop_tg, _drop_item_container_cmp, obj);

   if (st)
     {
        // Clear callbacks. The actual elm_drop_target_del for the underlying
        // generic drop target is handled by _inv_cb or explicit elm_drop_target_del.
        st->itemgetcb= NULL;
        st->poscb = NULL;
        st->dropcb = NULL;

        if (full) // Full deletion
          {
             cont_drop_tg = eina_list_remove(cont_drop_tg, st); // Remove from global list
             free(st); // Free the info struct
          }
        // Note: This function doesn't call elm_drop_target_del itself.
        // The generic drop target added by elm_drop_item_container_add
        // needs to be deleted separately if this 'full' deletion is meant
        // to completely remove all DND capabilities. Typically, elm_drop_target_del
        // would be called with the original callbacks passed to elm_drop_item_container_add.

        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Add drop support for an item container (like a list or grid).
 *
 * This enables items within a container to receive drops. It uses the
 * generic elm_drop_target_add internally but provides a higher-level
 * interface specific to containers with items.
 *
 * @param obj The container object (e.g., a genlist).
 * @param format A bitmask of Elm_Sel_Format types that this target accepts.
 * @param itemgetcb Callback to find which item is at given coordinates.
 *        (Same as in elm_drag_item_container_add).
 * @param entercb Function called when a drag enters the container. Can be NULL.
 * @param enterdata User data for entercb.
 * @param leavecb Function called when a drag leaves the container. Can be NULL.
 * @param leavedata User data for leavecb.
 * @param poscb Function called when a drag moves over an item in the container. Can be NULL.
 *        Prototype: void (*Elm_Drag_Item_Container_Pos)(void *data, Evas_Object *obj, Elm_Object_Item *it, Evas_Coord x, Evas_Coord y, int xposret, int yposret, Elm_Xdnd_Action action);
 *        `it` is the item under cursor, (x,y) are relative to container, (xposret,yposret) relative to item.
 * @param posdata User data for poscb.
 * @param dropcb Function called when a drop occurs on an item in the container. Can be NULL.
 *        Prototype: Eina_Bool (*Elm_Drop_Item_Container_Cb)(void *data, Evas_Object *obj, Elm_Object_Item *it, Elm_Selection_Data *ev, int xposret, int yposret);
 *        `it` is the item dropped on, `ev` contains drop data. Return EINA_TRUE if handled.
 * @param dropdata User data for dropcb.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool
elm_drop_item_container_add(Evas_Object *obj,
                            Elm_Sel_Format format,
                            Elm_Xy_Item_Get_Cb itemgetcb,
                            Elm_Drag_State entercb, void *enterdata,
                            Elm_Drag_State leavecb, void *leavedata,
                            Elm_Drag_Item_Container_Pos poscb, void *posdata,
                            Elm_Drop_Item_Container_Cb dropcb, void *dropdata)
{
   Item_Container_Drop_Info *st;

   // Try to update if already exists, otherwise create new
   if (elm_drop_item_container_del_internal(obj, EINA_FALSE)) // EINA_FALSE: don't fully delete, just clean state
     {  /* Updating info of existing obj */
        st = eina_list_search_unsorted(cont_drop_tg, _drop_item_container_cmp, obj);
        if (!st) return EINA_FALSE; // Should not happen
     }
   else // New registration
     {
        st = calloc(1, sizeof(*st));
        if (!st) return EINA_FALSE;

        st->obj = obj;
        cont_drop_tg = eina_list_append(cont_drop_tg, st);
     }

   st->itemgetcb = itemgetcb;
   st->poscb = poscb;
   st->dropcb = dropcb;
   elm_drop_target_add(obj, format,
                       entercb, enterdata,
                       leavecb, leavedata,
                       _elm_item_container_pos_cb, posdata,
                       _elm_item_container_drop_cb, dropdata);
   return EINA_TRUE;
}

/**
 * @brief Remove drop support for an item container.
 *
 * This function disables the drop capabilities previously added to the
 * container object with elm_drop_item_container_add().
 * It also implies that the underlying generic drop target should be removed.
 *
 * @param obj The item container object.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @note This function should ideally also call elm_drop_target_del with the
 *       original parameters (format, callbacks) to fully clean up. The current
 *       implementation only cleans the Item_Container_Drop_Info.
 */
EAPI Eina_Bool
elm_drop_item_container_del(Evas_Object *obj)
{
   // TODO: Need to find the original callbacks and format to properly call
   // elm_drop_target_del for the target added by elm_drop_item_container_add.
   // This currently only removes the Item_Container_Drop_Info struct.
   return elm_drop_item_container_del_internal(obj, EINA_TRUE);
}

/**
 * @internal
 * @brief Structure to hold data associated with a drag operation started by elm_drag_start.
 */
typedef struct {
  void *dragdata;   /**< User data for the dragposcb callback */
  void *acceptdata; /**< User data for the acceptcb callback */
  void *donecbdata; /**< User data for the dragdonecb callback */
  Elm_Drag_Pos dragposcb; /**< Callback for drag position updates (legacy, EFL DND handles this) */
  Elm_Drag_Accept acceptcb; /**< Callback for when the drop is accepted or rejected by the target */
  Elm_Drag_State dragdonecb; /**< Callback for when the drag operation finishes (dropped or cancelled) */
} Elm_Drag_Data;

/**
 * @internal
 * @brief Callback invoked when an EFL_UI_DND_EVENT_DRAG_FINISHED event occurs.
 * This function calls the user-provided acceptcb and dragdonecb.
 * @param data The Elm_Drag_Data structure.
 * @param ev The Efl_Event details, where ev->info is an Eina_Bool* indicating if the drop was accepted.
 */
static void
_drag_finished_cb(void *data, const Efl_Event *ev)
{
   Elm_Drag_Data *dd = data;
   Eina_Bool *accepted = ev->info; // True if drop was accepted, False otherwise

   if (dd->acceptcb) // Notify user if drop was accepted/rejected
     dd->acceptcb(dd->acceptdata, ev->object, *accepted);

   if (dd->dragdonecb) // Notify user that drag operation has concluded
     dd->dragdonecb(dd->donecbdata, ev->object);

   // Clean up: remove this callback and free the associated data
   efl_event_callback_del(ev->object, EFL_UI_DND_EVENT_DRAG_FINISHED, _drag_finished_cb, dd);
   free(dd);
}

/**
 * @brief Start a drag operation.
 *
 * This function initiates a drag and drop operation from the given Evas object.
 *
 * @param obj The Evas object initiating the drag.
 * @param format The format of the data being dragged (e.g., ELM_SEL_FORMAT_TEXT).
 *        Note: Internally, this is converted to MIME types. Only the first
 *        MIME type corresponding to the format bits will likely be used effectively
 *        by efl_ui_dnd_drag_start if multiple formats are bitwise OR-ed.
 *        It's best to use a single, specific format.
 * @param data The actual data to be dragged, as a string.
 * @param action The suggested Xdnd action (e.g., ELM_XDND_ACTION_COPY).
 * @param createicon Callback to create the drag icon.
 *        Prototype: Evas_Object *(*Elm_Drag_Icon_Create_Cb)(void *data, Evas_Object *win, Evas_Coord *xoff, Evas_Coord *yoff);
 *        `win` is the drag window (Efl_Content*), `xoff`, `yoff` can be set for icon offset from cursor.
 * @param createdata User data for the createicon callback.
 * @param dragpos Callback for drag position updates (legacy, largely superseded by EFL DND). Can be NULL.
 * @param dragdata User data for the dragpos callback.
 * @param acceptcb Callback invoked when the target accepts or rejects the drop. Can be NULL.
 * @param acceptdata User data for the acceptcb.
 * @param dragdone Callback invoked when the drag operation is finished or cancelled. Can be NULL.
 * @param donecbdata User data for the dragdone callback.
 * @return EINA_TRUE on success or if data is NULL (for backward compatibility), EINA_FALSE on failure.
 */
EAPI Eina_Bool
elm_drag_start(Evas_Object *obj, Elm_Sel_Format format,
               const char *data, Elm_Xdnd_Action action,
               Elm_Drag_Icon_Create_Cb createicon,
               void *createdata,
               Elm_Drag_Pos dragpos, void *dragdata,
               Elm_Drag_Accept acceptcb, void *acceptdata,
               Elm_Drag_State dragdone, void *donecbdata)
{
   Eina_Array *mime_types;
   Eina_Content *content;
   Efl_Content *ui; // This is the drag object (Efl_Ui_Drag_Object)
   int x = 0, y = 0, w, h;
   Efl_Ui_Widget *widget;
   Elm_Drag_Data *dd;
   const char *str_action;
   Eina_Position2D pointer;

   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, EINA_FALSE);

   //it should return EINA_TRUE to keep backward compatibility
   if (!data)
     return EINA_TRUE;

   str_action = _action_to_string(action);
   dd = calloc(1, sizeof(Elm_Drag_Data));
   dd->dragposcb = dragpos;
   dd->dragdata = dragdata;
   dd->acceptcb = acceptcb;
   dd->acceptdata = acceptdata;
   dd->dragdonecb = dragdone;
   dd->donecbdata = donecbdata;
   mime_types = _format_to_mime_array(format);
   if (eina_array_count(mime_types) != 1)
     {
        WRN("You passed more than one format, this is not going to work well");
     }
   content = eina_content_new((Eina_Slice) EINA_SLICE_STR_FULL(data), eina_array_data_get(mime_types, 0));
   ui = efl_ui_dnd_drag_start(obj, content, str_action, _default_seat(obj));
   widget = createicon(createdata, ui, &x, &y);

   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &pointer.x, &pointer.y);
   efl_ui_dnd_drag_offset_set(obj, _default_seat(obj), EINA_SIZE2D(x - pointer.x, y - pointer.y));
   evas_object_geometry_get(widget, NULL, NULL, &w, &h);
   evas_object_show(widget);
   efl_content_set(ui, widget);
   efl_gfx_entity_size_set(ui, EINA_SIZE2D(w, h));
   eina_array_free(mime_types);

   efl_event_callback_add(obj, EFL_UI_DND_EVENT_DRAG_FINISHED, _drag_finished_cb, dd);

   return EINA_TRUE;
}

/**
 * @brief Cancel an ongoing drag operation.
 *
 * If a drag was started from `obj` and is still in progress, this function
 * will attempt to cancel it.
 *
 * @param obj The Evas object that initiated the drag.
 * @return EINA_TRUE always (actual cancellation depends on the DND system).
 */
EAPI Eina_Bool
elm_drag_cancel(Evas_Object *obj)
{
   efl_ui_dnd_drag_cancel(obj, _default_seat(obj));

   return EINA_TRUE;
}

/**
 * @brief Set the current drag action (Deprecated).
 *
 * This function is deprecated as drag actions are typically negotiated
 * between source and target, or set at the start of the drag.
 * Modifying it mid-drag is not standard.
 *
 * @param obj Unused.
 * @param action Unused.
 * @return EINA_FALSE always, as this operation is not supported.
 */
EAPI Eina_Bool
elm_drag_action_set(Evas_Object *obj EINA_UNUSED, Elm_Xdnd_Action action EINA_UNUSED)
{
   ERR("This operation is not supported anymore.");
   return EINA_FALSE;
}
