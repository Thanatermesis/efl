#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_ACCESS_WIDGET_ACTION_PROTECTED

#define ELM_WIDGET_ITEM_PROTECTED
#include <Elementary.h>

#include "elm_priv.h"
#include "elm_slideshow_eo.h"
#include "elm_slideshow_item_eo.h"
#include "elm_widget_slideshow.h"

#define MY_CLASS ELM_SLIDESHOW_CLASS

#define MY_CLASS_NAME "Elm_Slideshow"
#define MY_CLASS_NAME_LEGACY "elm_slideshow"

static const char SIG_CHANGED[] = "changed";
static const char SIG_TRANSITION_END[] = "transition,end";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CHANGED, ""},
   {SIG_TRANSITION_END, ""},
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_LAYOUT_FOCUSED, ""}, /**< handled by elm_layout */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< handled by elm_layout */
   {NULL, NULL}
};

static Eina_Bool _key_action_move(Evas_Object *obj, const char *params);
static Eina_Bool _key_action_pause(Evas_Object *obj, const char *params);

static const Elm_Action key_actions[] = {
   {"move", _key_action_move},
   {"pause", _key_action_pause},
   {NULL, NULL}
};

/**
 * @internal
 * @brief Handles the "move" action, typically triggered by keyboard events.
 *
 * This function processes movement commands like "left" or "right"
 * to navigate to the previous or next slide, respectively.
 *
 * @param obj The slideshow widget object.
 * @param params A string indicating the direction of movement (e.g., "left", "right").
 * @return EINA_TRUE if the action was handled, EINA_FALSE otherwise.
 */
static Eina_Bool
_key_action_move(Evas_Object *obj, const char *params)
{
   const char *dir = params;

   _elm_widget_focus_auto_show(obj);
   if (!strcmp(dir, "left"))
     {
        elm_slideshow_previous(obj);
     }
   else if (!strcmp(dir, "right"))
     {
        elm_slideshow_next(obj);
     }
   else return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Handles the "pause" action, typically triggered by keyboard events.
 *
 * This function toggles the slideshow's automatic progression. If a timeout
 * is set (meaning the slideshow is playing), it stops the timer. If the
 * timer is stopped, it restarts it with the configured timeout.
 *
 * @param obj The slideshow widget object.
 * @param params Action parameters (unused in this function).
 * @return EINA_TRUE indicating the action was handled.
 */
static Eina_Bool
_key_action_pause(Evas_Object *obj, const char *params EINA_UNUSED)
{
   ELM_SLIDESHOW_DATA_GET(obj, sd);

   if (EINA_DBL_NONZERO(sd->timeout))
     {
        if (sd->timer)
          ELM_SAFE_FREE(sd->timer, ecore_timer_del);
        else
          elm_slideshow_timeout_set(obj, sd->timeout);
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Retrieves the item preceding the given item in the slideshow.
 *
 * If looping is enabled and the current item is the first, this function
 * will return the last item in the slideshow.
 *
 * @param item The current slideshow item data.
 * @return The previous slideshow item data, or NULL if at the beginning and
 *         looping is disabled.
 */
static Elm_Slideshow_Item_Data *
_item_prev_get(Elm_Slideshow_Item_Data *item)
{
   ELM_SLIDESHOW_DATA_GET(WIDGET(item), sd);
   Elm_Object_Item *eo_prev = eina_list_data_get(eina_list_prev(item->l));

   if ((!eo_prev) && (sd->loop))
     eo_prev = eina_list_data_get(eina_list_last(item->l));

   ELM_SLIDESHOW_ITEM_DATA_GET(eo_prev, prev);
   return prev;
}

/**
 * @internal
 * @brief Retrieves the item following the given item in the slideshow.
 *
 * If looping is enabled and the current item is the last, this function
 * will return the first item in the slideshow.
 *
 * @param item The current slideshow item data.
 * @return The next slideshow item data, or NULL if at the end and
 *         looping is disabled.
 */
static Elm_Slideshow_Item_Data *
_item_next_get(Elm_Slideshow_Item_Data *item)
{
   ELM_SLIDESHOW_DATA_GET(WIDGET(item), sd);
   Elm_Object_Item *eo_next = eina_list_data_get(eina_list_next(item->l));

   if ((!eo_next) && (sd->loop))
     eo_next = eina_list_data_get(sd->items);

   ELM_SLIDESHOW_ITEM_DATA_GET(eo_next, next);
   return next;
}

/**
 * @internal
 * @brief Callback invoked when the size hints of the slideshow widget change.
 *
 * This function triggers a re-evaluation of the slideshow's layout sizing.
 *
 * @param data The slideshow widget object (passed as user data).
 * @param e The Evas canvas (unused).
 * @param obj The Evas object whose size hints changed (unused, should be same as data).
 * @param event_info Additional event information (unused).
 */
static void
_on_size_hints_changed(void *data,
                       Evas *e EINA_UNUSED,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   elm_layout_sizing_eval(data);
}

/**
 * @internal
 * @brief Realizes (loads and prepares) slideshow items, including the current
 *        item and those within the caching window (before and after).
 *
 * This function is crucial for performance. It ensures that the Evas object
 * for the current item is created (if not already) using the item's `get`
 * callback. It then pre-realizes a configurable number of items
 * before and after the current item to allow for smoother transitions.
 * Items outside this caching window are unrealized (their Evas objects
 * deleted via the `del` callback) to save resources.
 *
 * The `sd->items_built` list tracks items whose Evas objects are currently
 * realized. This list is managed to keep only the necessary items in memory.
 *
 * @param item The slideshow item that is currently, or about to be, displayed.
 *             This item and its neighbors (based on cache settings) will be
 *             processed.
 */
static void
_item_realize(Elm_Slideshow_Item_Data *item)
{
   Elm_Slideshow_Item_Data *_item_prev, *_item_next;
   Evas_Object *obj = WIDGET(item);
   int ac, bc, lc, ic = 0;

   ELM_SLIDESHOW_DATA_GET_OR_RETURN(obj, sd);

   if ((!VIEW(item)) && (item->itc->func.get))
     {
        VIEW_SET(item, item->itc->func.get(elm_object_item_data_get(EO_OBJ(item)), obj));
        item->l_built = eina_list_append(NULL, item);
        sd->items_built = eina_list_merge(sd->items_built, item->l_built);
        //FIXME: item could be shown by obj
        evas_object_hide(VIEW(item));
     }
   else if (item->l_built)
     sd->items_built = eina_list_demote_list(sd->items_built, item->l_built);

   //pre-create previous and next item
   ac = sd->count_item_pre_after;
   _item_next = item;
   bc = sd->count_item_pre_before;
   _item_prev = item;
   lc = eina_list_count(sd->items) - 1;

   while (lc > 0 && ((ac > 0) || (bc > 0)))
     {
        if (lc > 0 && ac > 0)
          {
             --ac;
             --lc;
             if (_item_next)
               {
                  _item_next = _item_next_get(_item_next);
                  if ((_item_next)
                      && (!VIEW(_item_next))
                      && (_item_next->itc->func.get))
                    {
                       ic++;
                       VIEW_SET(_item_next,
                         _item_next->itc->func.get(
                                                   elm_object_item_data_get(EO_OBJ(_item_next)), obj));
                       _item_next->l_built =
                         eina_list_append(NULL, _item_next);
                       sd->items_built = eina_list_merge
                           (sd->items_built, _item_next->l_built);
                       //FIXME: _item_next could be shown by obj later
                       evas_object_hide(VIEW(_item_next));
                    }
                  else if (_item_next && _item_next->l_built)
                    {
                       ic++;
                       sd->items_built =
                         eina_list_demote_list
                           (sd->items_built, _item_next->l_built);
                    }
               }
          }

        if (lc > 0 && bc > 0)
          {
             --bc;
             --lc;
             if (_item_prev)
               {
                  _item_prev = _item_prev_get(_item_prev);
                  if ((_item_prev)
                      && (!VIEW(_item_prev))
                      && (_item_prev->itc->func.get))
                    {
                       ic++;
                       VIEW_SET(_item_prev,
                         _item_prev->itc->func.get(
                                                   elm_object_item_data_get(EO_OBJ(_item_prev)), obj));
                       _item_prev->l_built =
                         eina_list_append(NULL, _item_prev);
                       sd->items_built = eina_list_merge
                           (sd->items_built, _item_prev->l_built);
                       //FIXME: _item_prev could be shown by obj later
                       evas_object_hide(VIEW(_item_prev));
                    }
                  else if (_item_prev && _item_prev->l_built)
                    {
                       ic++;
                       sd->items_built =
                         eina_list_demote_list
                           (sd->items_built, _item_prev->l_built);
                    }
               }
          }
     }

   //delete unused items
   lc = ic + 1;

   while ((int)eina_list_count(sd->items_built) > lc)
     {
        item = eina_list_data_get(sd->items_built);
        sd->items_built = eina_list_remove_list
            (sd->items_built, sd->items_built);
        if (item->itc->func.del)
          item->itc->func.del(elm_object_item_data_get(EO_OBJ(item)), VIEW(item));
        ELM_SAFE_FREE(VIEW(item), evas_object_del);
        item->l_built = NULL;
     }
}

/**
 * @internal
 * @brief Callback invoked when a slide transition animation completes.
 *
 * This function is connected to Edje signals "elm,end" and "end" from
 * the slideshow's layout. It performs cleanup of the previously displayed
 * item (hiding its view and unsetting it from the layout) and finalizes
 * the display of the current item. It also re-realizes items based on the
 * new current item and emits the "transition,end" signal.
 *
 * @param data The slideshow widget object (passed as user data).
 * @param obj The Edje object that emitted the signal (unused).
 * @param emission The emitted signal string (e.g., "elm,end").
 * @param source The source of the signal (e.g., "elm").
 */
static void
_on_slideshow_end(void *data,
                  Evas_Object *obj EINA_UNUSED,
                  const char *emission,
                  const char *source EINA_UNUSED)
{
   Elm_Slideshow_Item_Data *item;
   ELM_SLIDESHOW_DATA_GET(data, sd);

   item = sd->previous;
   if (item)
     {
        elm_layout_content_unset(data, "elm.swallow.1");
        evas_object_hide(VIEW(item));
        sd->previous = NULL;
     }

   item = sd->current;
   if ((!item) || (!VIEW(item))) return;

   _item_realize(item);
   elm_layout_content_unset(data, "elm.swallow.2");

   elm_layout_content_set(data, "elm.swallow.1", VIEW(item));
   elm_layout_signal_emit(data, "elm,anim,end", "elm");
   // XXX: fort backwards compat
   elm_layout_signal_emit(data, "anim,end", "slideshow");

   if (emission != NULL)
     efl_event_callback_legacy_call
       (data, ELM_SLIDESHOW_EVENT_TRANSITION_END, EO_OBJ(sd->current));
}

/**
 * @internal
 * @brief Timer callback function for automatic slideshow progression.
 *
 * When the slideshow is playing (i.e., a timeout is set), this function
 * is called after the timeout duration. It advances the slideshow to the
 * next item.
 *
 * @param data The slideshow widget object (passed as user data).
 * @return ECORE_CALLBACK_CANCEL to automatically delete the timer after it fires.
 *         The timer will be re-added if continuous play is desired.
 */
static Eina_Bool
_timer_cb(void *data)
{
   Evas_Object *obj = data;
   ELM_SLIDESHOW_DATA_GET(obj, sd);

   sd->timer = NULL;
   elm_slideshow_next(obj);

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @internal
 * @brief Destructor for an Elm_Slideshow_Item.
 *
 * This function is called when an item is deleted (e.g., via efl_del or
 * when the slideshow is cleared). It handles removing the item from the
 * slideshow's internal lists (`sd->items` and `sd->items_built`).
 * If the item being deleted is the current one, it attempts to show an
 * adjacent item. It also calls the item's `del` callback if its view
 * was realized, allowing the application to free associated resources.
 *
 * @param eo_item The Efl_Object instance of the slideshow item.
 * @param item The private data structure for the slideshow item.
 */
EOLIAN static void
_elm_slideshow_item_efl_object_destructor(Eo *eo_item, Elm_Slideshow_Item_Data *item)
{
   ELM_SLIDESHOW_DATA_GET_OR_RETURN(WIDGET(item), sd);

   if (sd->previous == item) sd->previous = NULL;
   if (sd->current == item)
     {
        Eina_List *l = eina_list_data_find_list(sd->items, eo_item);
        Eina_List *l2 = eina_list_next(l);
        sd->current = NULL;
        if (!l2)
          {
             l2 = eina_list_prev(l);
             if (l2)
               elm_slideshow_item_show(eina_list_data_get(l2));
          }
        else
          elm_slideshow_item_show(eina_list_data_get(l2));
     }

   sd->items = eina_list_remove_list(sd->items, item->l);
   sd->items_built = eina_list_remove_list(sd->items_built, item->l_built);

   if ((VIEW(item)) && (item->itc->func.del))
     item->itc->func.del(elm_object_item_data_get(eo_item), VIEW(item));

   efl_destructor(efl_super(eo_item, ELM_SLIDESHOW_ITEM_CLASS));
}

/**
 * @internal
 * @brief Efl_Canvas_Group group_add implementation for Elm_Slideshow.
 *
 * This function is called when the slideshow widget is created and added to
 * a canvas. It performs essential initialization, including:
 * - Setting the default theme and style.
 * - Retrieving available transitions and layouts from the theme.
 * - Setting default cache counts for items before and after the current one.
 * - Registering internal callbacks for Edje signals (e.g., "elm,end" for
 *   transition completion) and Evas events (e.g., size hints changes).
 * - Enabling focus for the widget.
 *
 * @param obj The Efl_Object instance of the slideshow.
 * @param priv The private data structure for the slideshow.
 */
EOLIAN static void
_elm_slideshow_efl_canvas_group_group_add(Eo *obj, Elm_Slideshow_Data *priv)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));
   efl_ui_layout_finger_size_multiplier_set(obj, 0, 0);

   priv->count_item_pre_before = 2;
   priv->count_item_pre_after = 2;

   if (!elm_layout_theme_set
       (obj, "slideshow", "base", elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   priv->transitions = elm_widget_stringlist_get
       (edje_object_data_get(wd->resize_obj,
                             "transitions"));
   if (eina_list_count(priv->transitions) > 0)
     priv->transition =
       eina_stringshare_add(eina_list_data_get(priv->transitions));

   priv->layout.list = elm_widget_stringlist_get
       (edje_object_data_get(wd->resize_obj, "layouts"));

   if (eina_list_count(priv->layout.list) > 0)
     priv->layout.current = eina_list_data_get(priv->layout.list);

   edje_object_signal_callback_add
     (wd->resize_obj, "elm,end", "elm", _on_slideshow_end,
     obj);
   // XXX: for backwards compat :(
   edje_object_signal_callback_add
     (wd->resize_obj, "end", "slideshow", _on_slideshow_end,
     obj);

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _on_size_hints_changed, obj);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   elm_layout_sizing_eval(obj);
}

/**
 * @internal
 * @brief Efl_Canvas_Group group_del implementation for Elm_Slideshow.
 *
 * This function is called when the slideshow widget is being deleted.
 * It performs necessary cleanup, such as:
 * - Clearing all slideshow items (which involves deleting their views and
 *   calling their `del` callbacks).
 * - Freeing the list of transition names and layout names.
 * - Deleting any active timer for automatic slideshow progression.
 *
 * @param obj The Efl_Object instance of the slideshow.
 * @param sd The private data structure for the slideshow.
 */
EOLIAN static void
_elm_slideshow_efl_canvas_group_group_del(Eo *obj, Elm_Slideshow_Data *sd)
{
   const char *layout;

   elm_slideshow_clear(obj);
   elm_widget_stringlist_free(sd->transitions);
   ecore_timer_del(sd->timer);

   EINA_LIST_FREE(sd->layout.list, layout)
     eina_stringshare_del(layout);

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

// Documentation for elm_slideshow_add is in elm_slideshow.h
EAPI Evas_Object *
elm_slideshow_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Efl_Object constructor implementation for Elm_Slideshow.
 *
 * Standard Eo constructor. Sets the object type, smart callbacks description,
 * accessibility role, and handles legacy focus.
 *
 * @param obj The Efl_Object instance being constructed.
 * @param _pd The private data for the slideshow (unused in this phase).
 * @return The constructed Efl_Object instance.
 */
EOLIAN static Eo *
_elm_slideshow_efl_object_constructor(Eo *obj, Elm_Slideshow_Data *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_DOCUMENT_PRESENTATION);
   legacy_object_focus_handle(obj);

   return obj;
}

/**
 * @internal
 * @brief Efl_Object constructor implementation for Elm_Slideshow_Item.
 *
 * Standard Eo constructor for slideshow items. It also retrieves and stores
 * the base widget item data.
 *
 * @param obj The Efl_Object instance of the item being constructed.
 * @param it The private data for the slideshow item.
 * @return The constructed Efl_Object instance.
 */
EOLIAN static Eo *
_elm_slideshow_item_efl_object_constructor(Eo *obj, Elm_Slideshow_Item_Data *it)
{
   obj = efl_constructor(efl_super(obj, ELM_SLIDESHOW_ITEM_CLASS));
   it->base = efl_data_scope_get(obj, ELM_WIDGET_ITEM_CLASS);

   return obj;
}

/**
 * @internal
 * @brief Adds a new item to the slideshow.
 *
 * This function creates a new slideshow item, associates it with the provided
 * item class and data, and appends it to the end of the slideshow's item list.
 * If it's the first item added, it's automatically shown.
 *
 * @param obj The slideshow Efl_Object.
 * @param sd The private data of the slideshow.
 * @param itc Pointer to the #Elm_Slideshow_Item_Class defining callbacks for the item.
 * @param data User data to be associated with this item.
 * @return The newly created Elm_Object_Item, or NULL on failure.
 */
EOLIAN static Elm_Object_Item*
_elm_slideshow_item_add(Eo *obj, Elm_Slideshow_Data *sd, const Elm_Slideshow_Item_Class *itc, const void *data)
{
   Eo *eo_item;

   eo_item = efl_add(ELM_SLIDESHOW_ITEM_CLASS, obj);
   if (!eo_item) return NULL;

   ELM_SLIDESHOW_ITEM_DATA_GET(eo_item, item);

   item->itc = itc;
   item->l = eina_list_append(item->l, eo_item);
   WIDGET_ITEM_DATA_SET(eo_item, data);

   sd->items = eina_list_merge(sd->items, item->l);

   if (!sd->current) elm_slideshow_item_show(eo_item);

   return eo_item;
}

/**
 * @internal
 * @brief Adds a new item to the slideshow in a sorted manner.
 *
 * This function creates a new slideshow item and inserts it into the
 * slideshow's item list based on the provided comparison function `func`.
 * If it's the first item added (or becomes the first after sorting),
 * it's automatically shown.
 *
 * @param obj The slideshow Efl_Object.
 * @param sd The private data of the slideshow.
 * @param itc Pointer to the #Elm_Slideshow_Item_Class defining callbacks for the item.
 * @param data User data to be associated with this item.
 * @param func The comparison function used to sort items.
 *             It should return < 0 if data1 < data2, 0 if data1 == data2,
 *             and > 0 if data1 > data2.
 * @return The newly created Elm_Object_Item, or NULL on failure.
 */
EOLIAN static Elm_Object_Item*
_elm_slideshow_item_sorted_insert(Eo *obj, Elm_Slideshow_Data *sd, const Elm_Slideshow_Item_Class *itc, const void *data, Eina_Compare_Cb func)
{
   Eo *eo_item;

   eo_item = efl_add(ELM_SLIDESHOW_ITEM_CLASS, obj);
   if (!eo_item) return NULL;

   ELM_SLIDESHOW_ITEM_DATA_GET(eo_item, item);

   item->itc = itc;
   item->l = eina_list_append(item->l, eo_item);
   WIDGET_ITEM_DATA_SET(eo_item, data);

   sd->items = eina_list_sorted_merge(sd->items, item->l, func);

   if (!sd->current) elm_slideshow_item_show(eo_item);

   return eo_item;
}

/**
 * @internal
 * @brief Shows the specified slideshow item.
 *
 * This function makes the given `item` the current visible item in the
 * slideshow. It involves:
 * - Calling `_on_slideshow_end` to clean up the state of any previously
 *   current item.
 * - Resetting and potentially restarting the slideshow timer if a timeout is set.
 * - Calling `_item_realize` to ensure the new item and its neighbors are loaded.
 * - Setting the new item's view into the "elm.swallow.2" part of the layout,
 *   which is typically used for the incoming slide in a transition.
 * - Emitting Edje signals to trigger the "next" transition animation
 *   (e.g., "elm,transition_name,next").
 * - Updating `sd->current` and `sd->previous` pointers.
 * - Emitting the "changed" signal to notify listeners of the new current item.
 *
 * @param eo_item The Efl_Object of the item to show (unused directly, item data is used).
 * @param item The private data of the slideshow item to be shown.
 */
EOLIAN static void
_elm_slideshow_item_show(Eo *eo_item EINA_UNUSED, Elm_Slideshow_Item_Data *item)
{
   char buf[1024];
   Elm_Slideshow_Item_Data *next = NULL;

   ELM_SLIDESHOW_DATA_GET(WIDGET(item), sd);

   if (item == sd->current) return;

   // Use 'item' as the 'next' item to be shown
   next = item;
   // Finalize previous slide display before switching
   _on_slideshow_end(WIDGET(item), WIDGET(item), NULL, NULL);

   // Restart timer if slideshow is playing
   ELM_SAFE_FREE(sd->timer, ecore_timer_del);
   if (sd->timeout > 0.0)
     sd->timer = ecore_timer_add(sd->timeout, _timer_cb, WIDGET(item));

   // Realize the new current item and its cache window
   _item_realize(next);
   // Set the view of the 'next' item into the layout part for incoming slides
   elm_layout_content_set(WIDGET(item), "elm.swallow.2", VIEW(next));

   // Emit signals to trigger the transition animation
   if (!sd->transition)
     sprintf(buf, "elm,none,next");
   else
     snprintf(buf, sizeof(buf), "elm,%s,next", sd->transition);
   elm_layout_signal_emit(WIDGET(item), buf, "elm");
   // XXX: for backwards compat
   if (!sd->transition)
     sprintf(buf,"none,next");
   else
     snprintf(buf, sizeof(buf), "%s,next", sd->transition);
   elm_layout_signal_emit(WIDGET(item), buf, "slideshow");

   // Update current and previous item pointers
   sd->previous = sd->current;
   sd->current = next;
   // Notify that the current item has changed
   efl_event_callback_legacy_call
     (WIDGET(item), ELM_SLIDESHOW_EVENT_CHANGED, EO_OBJ(sd->current));
}

/**
 * @internal
 * @brief Advances the slideshow to the next item.
 *
 * This function identifies the next item in the sequence (handling looping
 * if enabled) and makes it the current visible item. The process is similar
 * to `_elm_slideshow_item_show`:
 * - Calls `_on_slideshow_end` for cleanup.
 * - Manages the slideshow timer.
 * - Realizes the new item and its cache window.
 * - Sets the new item's view into "elm.swallow.2".
 * - Emits Edje signals for the "next" transition.
 * - Updates `sd->current` and `sd->previous`.
 * - Emits the "changed" signal.
 *
 * If there is no next item (and looping is off), or if the next item is the
 * same as the current (e.g., single item slideshow without loop),
 * this function does nothing.
 *
 * @param obj The slideshow Efl_Object.
 * @param sd The private data of the slideshow.
 */
EOLIAN static void
_elm_slideshow_next(Eo *obj, Elm_Slideshow_Data *sd)
{
   char buf[1024];
   Elm_Slideshow_Item_Data *next = NULL;

   if (sd->current) next = _item_next_get(sd->current);

   if ((!next) || (next == sd->current)) return;

   _on_slideshow_end(obj, obj, NULL, NULL);

   ELM_SAFE_FREE(sd->timer, ecore_timer_del);
   if (sd->timeout > 0.0)
     sd->timer = ecore_timer_add(sd->timeout, _timer_cb, obj);

   _item_realize(next);

   elm_layout_content_set(obj, "elm.swallow.2", VIEW(next));

   if (!sd->transition)
     sprintf(buf, "elm,none,next");
   else
     snprintf(buf, sizeof(buf), "elm,%s,next", sd->transition);
   elm_layout_signal_emit(obj, buf, "elm");
   // XXX: for backwards compat
   if (!sd->transition)
     sprintf(buf,"none,next");
   else
     snprintf(buf, sizeof(buf), "%s,next", sd->transition);
   elm_layout_signal_emit(obj, buf, "slideshow");

   sd->previous = sd->current;
   sd->current = next;
   efl_event_callback_legacy_call
     (obj, ELM_SLIDESHOW_EVENT_CHANGED, EO_OBJ(sd->current));
}

/**
 * @internal
 * @brief Moves the slideshow to the previous item.
 *
 * This function identifies the previous item in the sequence (handling looping
 * if enabled) and makes it the current visible item. The process mirrors
 * `_elm_slideshow_next` but for the "previous" direction:
 * - Calls `_on_slideshow_end` for cleanup.
 * - Manages the slideshow timer.
 * - Realizes the new item and its cache window.
 * - Sets the new item's view into "elm.swallow.2".
 * - Emits Edje signals for the "previous" transition (e.g., "elm,transition_name,previous").
 * - Updates `sd->current` and `sd->previous`.
 * - Emits the "changed" signal.
 *
 * If there is no previous item (and looping is off), or if the previous item
 * is the same as the current, this function does nothing.
 *
 * @param obj The slideshow Efl_Object.
 * @param sd The private data of the slideshow.
 */
EOLIAN static void
_elm_slideshow_previous(Eo *obj, Elm_Slideshow_Data *sd)
{
   char buf[1024];
   Elm_Slideshow_Item_Data *prev = NULL;

   if (sd->current) prev = _item_prev_get(sd->current);

   if ((!prev) || (prev == sd->current)) return;

   _on_slideshow_end(obj, obj, NULL, NULL);

   ELM_SAFE_FREE(sd->timer, ecore_timer_del);
   if (sd->timeout > 0.0)
     sd->timer = ecore_timer_add(sd->timeout, _timer_cb, obj);

   _item_realize(prev);

   elm_layout_content_set(obj, "elm.swallow.2", VIEW(prev));

   if (!sd->transition)
     sprintf(buf, "elm,none,previous");
   else
     snprintf(buf, 1024, "elm,%s,previous", sd->transition);
   elm_layout_signal_emit(obj, buf, "elm");
   // XXX: for backwards compat
   if (!sd->transition)
     sprintf(buf,"none,previous");
   else
     snprintf(buf, 1024, "%s,previous", sd->transition);
   elm_layout_signal_emit(obj, buf, "slideshow");

   sd->previous = sd->current;
   sd->current = prev;
   efl_event_callback_legacy_call
     (obj, ELM_SLIDESHOW_EVENT_CHANGED, EO_OBJ(sd->current));
}

/**
 * @internal
 * @brief Gets the list of available transition names for the slideshow.
 *
 * These names are typically defined in the slideshow's theme.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @return A const Eina_List of Eina_Stringshare instances, each representing
 *         a transition name. The list should not be modified by the caller.
 */
EOLIAN static const Eina_List*
_elm_slideshow_transitions_get(const Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd)
{
   return sd->transitions;
}

/**
 * @internal
 * @brief Gets the list of available layout names for the slideshow.
 *
 * These names are typically defined in the slideshow's theme and can
 * alter the visual arrangement or appearance of items.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @return A const Eina_List of Eina_Stringshare instances, each representing
 *         a layout name. The list should not be modified by the caller.
 */
EOLIAN static const Eina_List*
_elm_slideshow_layouts_get(const Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd)
{
   return sd->layout.list;
}

/**
 * @internal
 * @brief Sets the current transition animation for the slideshow.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @param transition The name of the transition to use (e.g., "fade", "slide").
 *                   This should be one of the names from `elm_slideshow_transitions_get()`.
 */
EOLIAN static void
_elm_slideshow_transition_set(Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd, const char *transition)
{
   eina_stringshare_replace(&sd->transition, transition);
}

/**
 * @internal
 * @brief Gets the name of the current transition animation.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @return The current transition name as a stringshared string, or NULL
 *         if no transition is set (implying a default or "none" transition).
 */
EOLIAN static const char*
_elm_slideshow_transition_get(const Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd)
{
   return sd->transition;
}

/**
 * @internal
 * @brief Sets the timeout for automatic slide advancement.
 *
 * If `timeout` is greater than 0, the slideshow will automatically advance
 * to the next slide after this duration (in seconds). If `timeout` is 0 or
 * negative, automatic advancement is disabled.
 *
 * @param obj The slideshow Efl_Object.
 * @param sd The private data of the slideshow.
 * @param timeout The timeout duration in seconds.
 */
EOLIAN static void
_elm_slideshow_timeout_set(Eo *obj, Elm_Slideshow_Data *sd, double timeout)
{
   sd->timeout = timeout;

   ELM_SAFE_FREE(sd->timer, ecore_timer_del);
   if (timeout > 0.0)
     sd->timer = ecore_timer_add(timeout, _timer_cb, obj);
}

/**
 * @internal
 * @brief Gets the current timeout for automatic slide advancement.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @return The timeout duration in seconds. A value of 0 or less indicates
 *         automatic advancement is disabled.
 */
EOLIAN static double
_elm_slideshow_timeout_get(const Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd)
{
   return sd->timeout;
}

/**
 * @internal
 * @brief Sets whether the slideshow should loop back to the beginning after
 *        reaching the end, or to the end after reaching the beginning.
 *
 * @param obj The slideshow Efl_Object.
 * @param sd The private data of the slideshow.
 * @param loop EINA_TRUE to enable looping, EINA_FALSE to disable.
 */
EOLIAN static void
_elm_slideshow_items_loop_set(Eo *obj, Elm_Slideshow_Data *sd, Eina_Bool loop)
{
   ELM_SLIDESHOW_CHECK(obj); // Macro for basic object validity check
   sd->loop = loop;
}

/**
 * @internal
 * @brief Gets the name of the current layout used by the slideshow.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @return The current layout name as a stringshared string, or NULL if
 *         no specific layout is set (implying a default layout).
 */
EOLIAN static const char*
_elm_slideshow_layout_get(const Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd)
{
   return sd->layout.current;
}

/**
 * @internal
 * @brief Sets the layout for the slideshow.
 *
 * This function updates the current layout of the slideshow and emits
 * Edje signals (e.g., "elm,layout,layout_name") to the underlying Edje
 * object to apply the new layout.
 *
 * @param obj The slideshow Efl_Object.
 * @param sd The private data of the slideshow.
 * @param layout The name of the layout to apply. This should be one of the
 *               names from `elm_slideshow_layouts_get()`.
 */
EOLIAN static void
_elm_slideshow_layout_set(Eo *obj, Elm_Slideshow_Data *sd, const char *layout)
{
   char buf[PATH_MAX];

   // Store the new layout name
   sd->layout.current = layout;
   // Emit signal to Edje to change the layout
   snprintf(buf, sizeof(buf), "elm,layout,%s", layout);
   elm_layout_signal_emit(obj, buf, "elm");
   // XXX: for bakcwards compat
   snprintf(buf, sizeof(buf), "layout,%s", layout);
   elm_layout_signal_emit(obj, buf, "slideshow");
}

/**
 * @internal
 * @brief Gets whether slideshow looping is enabled.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @return EINA_TRUE if looping is enabled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_slideshow_items_loop_get(const Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd)
{
   return sd->loop;
}

/**
 * @internal
 * @brief Removes all items from the slideshow.
 *
 * This function iterates through all items in the slideshow and deletes them
 * by calling `efl_del` on each. This, in turn, triggers the item's
 * destructor (`_elm_slideshow_item_efl_object_destructor`), which handles
 * view cleanup and calling the item's `del` callback.
 * It also resets the `sd->previous` and `sd->current` pointers.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 */
EOLIAN static void
_elm_slideshow_clear(Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd)
{
   Eo *eo_item;
   Eina_List *itr, *itr2;
   sd->previous = NULL;
   sd->current = NULL;

   EINA_LIST_FOREACH_SAFE(sd->items, itr, itr2, eo_item)
      efl_del(eo_item);
}

/**
 * @internal
 * @brief Gets the list of all items currently in the slideshow.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @return A const Eina_List of Elm_Object_Item instances. The list should
 *         not be modified by the caller. Each item in the list is an
 *         `Eo *` pointer to a slideshow item.
 */
EOLIAN static const Eina_List*
_elm_slideshow_items_get(const Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd)
{
   return sd->items;
}

/**
 * @internal
 * @brief Gets the currently displayed item in the slideshow.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @return The current Elm_Object_Item, or NULL if there are no items or
 *         none is currently set as active. This is an `Eo *` pointer.
 */
EOLIAN static Elm_Object_Item*
_elm_slideshow_item_current_get(const Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd)
{
   return EO_OBJ(sd->current);
}

/**
 * @internal
 * @brief Gets the Evas_Object (view) associated with a specific slideshow item.
 *
 * This is the actual Evas object created by the item's `get` callback.
 *
 * @param eo_item The Efl_Object of the slideshow item (unused).
 * @param it The private data of the slideshow item.
 * @return The Evas_Object representing the item's view, or NULL if the
 *         item has not been realized or has no view.
 */
EOLIAN static Evas_Object *
_elm_slideshow_item_object_get(const Eo *eo_item EINA_UNUSED, Elm_Slideshow_Item_Data *it)
{
   return VIEW(it);
}

/**
 * @internal
 * @brief Gets the number of items to be pre-loaded (cached) before the
 *        currently visible item.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @return The number of items to cache before the current one.
 */
EOLIAN static int
_elm_slideshow_cache_before_get(const Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd)
{
   return sd->count_item_pre_before;
}

/**
 * @internal
 * @brief Sets the number of items to be pre-loaded (cached) before the
 *        currently visible item.
 *
 * A non-negative count is enforced.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @param count The number of items to cache. If negative, it's set to 0.
 */
EOLIAN static void
_elm_slideshow_cache_before_set(Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd, int count)
{
   if (!sd) return;
   if (count < 0) count = 0;
   sd->count_item_pre_before = count;
}

/**
 * @internal
 * @brief Gets the number of items to be pre-loaded (cached) after the
 *        currently visible item.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @return The number of items to cache after the current one.
 */
EOLIAN static int
_elm_slideshow_cache_after_get(const Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd)
{
   return sd->count_item_pre_after;
}

/**
 * @internal
 * @brief Sets the number of items to be pre-loaded (cached) after the
 *        currently visible item.
 *
 * A non-negative count is enforced.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @param count The number of items to cache. If negative, it's set to 0.
 */
EOLIAN static void
_elm_slideshow_cache_after_set(Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd, int count)
{
   if (count < 0) count = 0;
   sd->count_item_pre_after = count;
}

/**
 * @internal
 * @brief Gets the Nth item in the slideshow's list of items.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @param nth The index (0-based) of the item to retrieve.
 * @return The Elm_Object_Item at the specified index, or NULL if `nth`
 *         is out of bounds. This is an `Eo *` pointer.
 */
EOLIAN static Elm_Object_Item*
_elm_slideshow_item_nth_get(const Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd, unsigned int nth)
{
   return eina_list_nth(sd->items, nth);
}

/**
 * @internal
 * @brief Gets the total number of items in the slideshow.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow.
 * @return The count of items in the slideshow.
 */
EOLIAN static unsigned int
_elm_slideshow_count_get(const Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd)
{
   return eina_list_count(sd->items);
}

/**
 * @internal
 * @brief Class constructor for Elm_Slideshow.
 *
 * This function is called once when the Elm_Slideshow class is being set up.
 * It registers the legacy type name for the widget, allowing it to be
 * created using older Elementary APIs.
 *
 * @param klass The Efl_Class for Elm_Slideshow.
 */
EOLIAN static void
_elm_slideshow_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @internal
 * @brief Provides accessibility actions for the slideshow widget.
 *
 * This function returns a list of actions that assistive technologies (like
 * screen readers) can invoke on the slideshow. These actions correspond to
 * key operations like moving left/right and pausing.
 *
 * @param obj The slideshow Efl_Object (unused).
 * @param sd The private data of the slideshow (unused).
 * @return A static array of #Efl_Access_Action_Data, terminated by an
 *         entry with NULL fields. This array defines actions such as "move"
 *         (with parameters "left" or "right") and "pause".
 *         Example structure of an element in atspi_actions:
 *         { "action_name", "action_keybinding", "action_params", callback_function }
 *         e.g., { "move,left", "move", "left", _key_action_move}
 */
EOLIAN static const Efl_Access_Action_Data*
_elm_slideshow_efl_access_widget_action_elm_actions_get(const Eo *obj EINA_UNUSED, Elm_Slideshow_Data *sd EINA_UNUSED)
{
   static Efl_Access_Action_Data atspi_actions[] = {
          { "move,left", "move", "left", _key_action_move},
          { "move,right", "move", "right", _key_action_move},
          { "pause", "pause", NULL, _key_action_pause},
          { NULL, NULL, NULL, NULL }
   };
   return &atspi_actions[0];
}

/* Standard widget overrides */

ELM_WIDGET_KEY_DOWN_DEFAULT_IMPLEMENT(elm_slideshow, Elm_Slideshow_Data)

/* Internal EO APIs and hidden overrides */

#define ELM_SLIDESHOW_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_slideshow)

#include "elm_slideshow_item_eo.c"
#include "elm_slideshow_eo.c"
