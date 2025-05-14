#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

typedef struct _Testitem
{
   Elm_Object_Item  *item;
   const char       *path;
   int               mode;
   int               onoff;
} Testitem;

static Elm_Gengrid_Item_Class gic;

/**
 * @brief Get the label for a gengrid item.
 *
 * This function provides the label for a gengrid item, which is constructed
 * using the image path stored in the item's data.
 *
 * @param data The item data (a Testitem struct).
 * @param obj The gengrid object.
 * @param part The part name for which to get the text.
 * @return A newly allocated string with the item's label. The caller must
 *         free this string.
 */
char *
grdt_lbl_get(void            *data,
             Evas_Object *obj EINA_UNUSED,
             const char *part EINA_UNUSED)
{
   const Testitem *ti = data;
   char buf[256];
   snprintf(buf, sizeof(buf), "Photo %s", ti->path);
   return strdup(buf);
}

/**
 * @brief Get the content for a gengrid item.
 *
 * This function provides the icon content for a gengrid item. It handles
 * the "elm.swallow.icon" part by creating a background object and setting
 * its file to the path from the item's data.
 *
 * @param data The item data (a Testitem struct).
 * @param obj The gengrid object.
 * @param part The part name to get content for.
 * @return An Evas_Object to be used as content, or NULL if the part is not handled.
 */
Evas_Object *
grdt_content_get(void        *data,
              Evas_Object *obj,
              const char  *part)
{
   const Testitem *ti = data;
   if (!strcmp(part, "elm.swallow.icon"))
     {
        Evas_Object *icon = elm_bg_add(obj);
        elm_bg_file_set(icon, ti->path, NULL);
        evas_object_size_hint_aspect_set(icon,
                                         EVAS_ASPECT_CONTROL_VERTICAL,
                                         1, 1);
        evas_object_show(icon);
        return icon;
     }
   return NULL;
}

static Elm_Genlist_Item_Class itct;

/**
 * @brief Callback for the "expanded" smart event of a genlist.
 *
 * This function is called when a genlist item is expanded. It dynamically
 * populates the item with three new sub-items, each with its own tooltip.
 * The data for the new items is derived from the parent item's data.
 *
 * @param data User data passed to the smart callback.
 * @param obj The genlist object.
 * @param event_info The genlist item that was expanded.
 */
static void
gltt_exp(void *data       EINA_UNUSED,
         Evas_Object *obj EINA_UNUSED,
         void            *event_info)
{
   Elm_Object_Item *glit = event_info;
   Evas_Object *gl = elm_object_item_widget_get(glit);
   int val = (int)(uintptr_t) elm_object_item_data_get(glit);
   Elm_Object_Item *glit1, *glit2, *glit3;

   val *= 10;
   glit1 = elm_genlist_item_append(gl, &itct, (void *)(uintptr_t) (val + 1), glit,
                                   ELM_GENLIST_ITEM_NONE, NULL, NULL);
   glit2 = elm_genlist_item_append(gl, &itct, (void *)(uintptr_t) (val + 2), glit,
                                   ELM_GENLIST_ITEM_NONE, NULL, NULL);
   glit3 = elm_genlist_item_append(gl, &itct, (void *)(uintptr_t) (val + 3), glit,
                                   ELM_GENLIST_ITEM_TREE, NULL, NULL);

   elm_genlist_item_tooltip_text_set(glit1, "Testing A");
   elm_genlist_item_tooltip_text_set(glit2, "Testing B");
   elm_genlist_item_tooltip_text_set(glit3, "Testing C");
}

/**
 * @brief Callback for the "contracted" smart event of a genlist.
 *
 * This function is called when a genlist item is contracted. It clears all
 * sub-items from the contracted item to free up resources.
 *
 * @param data User data passed to the smart callback.
 * @param obj The genlist object.
 * @param event_info The genlist item that was contracted.
 */
static void
gltt_con(void *data       EINA_UNUSED,
         Evas_Object *obj EINA_UNUSED,
         void            *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_subitems_clear(glit);
}

/**
 * @brief Callback for the "expand,request" smart event of a genlist.
 *
 * This function is called when expansion of a genlist item is requested by
 * the user (e.g., by clicking the expand button). It marks the item as
 * expanded, which will trigger the "expanded" event and the gltt_exp()
 * callback to populate sub-items.
 *
 * @param data User data passed to the smart callback.
 * @param obj The genlist object.
 * @param event_info The genlist item for which expansion is requested.
 */
static void
gltt_exp_req(void *data       EINA_UNUSED,
             Evas_Object *obj EINA_UNUSED,
             void            *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_TRUE);
}

/**
 * @brief Callback for the "contract,request" smart event of a genlist.
 *
 * This function is called when contraction of a genlist item is requested
 * by the user. It marks the item as not expanded, which will trigger the
 * "contracted" event and the gltt_con() callback.
 *
 * @param data User data passed to the smart callback.
 * @param obj The genlist object.
 * @param event_info The genlist item for which contraction is requested.
 */
static void
gltt_con_req(void *data       EINA_UNUSED,
             Evas_Object *obj EINA_UNUSED,
             void            *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_FALSE);
}

/**
 * @brief Get the text for a genlist item.
 *
 * This function provides the label for a genlist item. The label is
 * constructed based on the integer value passed in the data parameter.
 *
 * @param data The item data, an integer cast to a void pointer.
 * @param obj The genlist object.
 * @param part The part name for which to get the text.
 * @return A newly allocated string with the item's label. The caller must
 *         free this string.
 */
char *
gltt_text_get(void            *data,
               Evas_Object *obj EINA_UNUSED,
               const char *part EINA_UNUSED)
{
   char buf[256];
   snprintf(buf, sizeof(buf), "Item mode %i", (int)(uintptr_t)data);
   return strdup(buf);
}

/**
 * @brief Tooltip content callback to create a small icon for an item.
 *
 * This callback is for elm_object_item_tooltip_content_cb_set(). It creates
 * a small icon to be used as the tooltip content for a list or grid item.
 *
 * @param data User data associated with the content callback.
 * @param obj The parent widget of the item.
 * @param tt The tooltip object itself.
 * @param item The item this tooltip is for.
 * @return An icon object to be used as tooltip content.
 */
static Evas_Object *
_tt_item_icon(void *data   EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED,
              Evas_Object *tt,
              void *item   EINA_UNUSED)
{
   Evas_Object *ic = elm_icon_add(tt);
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png",
            elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   evas_object_resize(ic, 64, 64);
   return ic;
}

/**
 * @brief Tooltip content callback to create a different icon for an item.
 *
 * Similar to _tt_item_icon(), but uses a different image file. This is
 * used to show tooltips with different content on different items.
 *
 * @param data User data associated with the content callback.
 * @param obj The parent widget of the item.
 * @param tt The tooltip object itself.
 * @param item The item this tooltip is for.
 * @return An icon object to be used as tooltip content.
 */
static Evas_Object *
_tt_item_icon2(void *data   EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED,
              Evas_Object *tt,
              void *item   EINA_UNUSED)
{
   Evas_Object *ic = elm_icon_add(tt);
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/images/logo.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   return ic;
}

/**
 * @brief Tooltip content callback to create a large icon that may be scaled.
 *
 * This function creates an icon from a very large image. It checks if the
 * image dimensions exceed a threshold (80% of screen size) and scales it
 * down proportionally if necessary to ensure it fits reasonably on screen.
 * This demonstrates handling of large tooltip content.
 *
 * @param data User data associated with the content callback.
 * @param obj The parent widget of the item.
 * @param tt The tooltip object itself.
 * @param item The item this tooltip is for.
 * @return An icon object to be used as tooltip content.
 */
static Evas_Object *
_tt_item_icon3(void *data   EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED,
              Evas_Object *tt,
              void *item   EINA_UNUSED)
{
   int w, h, sw, sh;
   Evas_Object *ic = elm_icon_add(tt);

   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/images/insanely_huge_test_image.jpg", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_object_size_get(ic, &w, &h);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   elm_win_screen_size_get(tt, NULL, NULL, &sw, &sh);
   if ((w > sw) || (h > sh))
     {
        float sc = 0;
        if ((float)w / (float)sw >= 0.8)
          sc = ((float)sw * 0.8) / (float)w;
        else if ((float)h / (float)sh >= 0.8)
          sc = ((float)sh * 0.8) / (float)h;
        if (EINA_FLT_NONZERO(sc)) elm_object_scale_set(ic, sc);
     }
   return ic;
}

/**
 * @brief Tooltip content callback to create a long, wrapped label for an item.
 *
 * This demonstrates using a text label with line wrapping as tooltip content,
 * suitable for displaying longer descriptive texts.
 *
 * @param data User data associated with the content callback.
 * @param obj The parent widget of the item.
 * @param tt The tooltip object itself.
 * @param item The item this tooltip is for.
 * @return A label object to be used as tooltip content.
 */
static Evas_Object *
_tt_item_label(void *data   EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED,
              Evas_Object *tt,
              void *item   EINA_UNUSED)
{
   Evas_Object *l = elm_label_add(tt);
   elm_object_text_set(l, "Something useful here?<ps/>"
                          "No probably not, but this is a super long label<ps/>"
                          "which probably breaks on your system, now doesn't it?<ps/>"
                          "Yeah, I thought so.");
   elm_label_line_wrap_set(l, ELM_WRAP_MIXED);
   return l;
}

/**
 * @brief Deletion callback for an item's tooltip content.
 *
 * This function is registered as a deletion callback for tooltip content
 * (e.g., created by _tt_item_icon()). It's used for testing to verify that the
 * callback is invoked correctly by printing the data it receives.
 *
 * @param data User data passed when setting the content callback (e.g., 456L).
 * @param obj The object the tooltip was attached to.
 * @param event_info The tooltip content object being deleted.
 */
static void
_tt_item_icon_del(void            *data,
                  Evas_Object *obj EINA_UNUSED,
                  void            *event_info)
{
   // test to check for del_cb behavior!
   printf("_tt_icon_del: data=%ld (== 456?), event_info=%p\n",
          (long)(uintptr_t)data, event_info);
}

/**
 * @brief Replaces the tooltip text with a counter on click.
 *
 * This function is a "clicked" callback for a button. Each time it's called,
 * it increments a static counter and updates the tooltip text of the button
 * to show the new count. This demonstrates dynamic tooltip text updates.
 *
 * @param data User data.
 * @param obj The button object whose tooltip text is to be replaced.
 * @param event_info Event-specific information.
 */
static void
_tt_text_replace(void *data       EINA_UNUSED,
                 Evas_Object     *obj,
                 void *event_info EINA_UNUSED)
{
   static int count = 0;
   char buf[64];
   snprintf(buf, sizeof(buf), "count=%d", count);
   count++;
   elm_object_tooltip_text_set(obj, buf);
}

/**
 * @brief Toggles the move freeze state of a tooltip on click.
 *
 * This is a "clicked" callback for a button. It pushes/pops the tooltip's
 * move freeze stack, effectively freezing or unfreezing the tooltip's
 * position updates. The tooltip text is updated to reflect the current state.
 *
 * @param data User data.
 * @param obj The object whose tooltip move freeze state is toggled.
 * @param event_info Event-specific information.
 */
static void
_tt_move_freeze(void *data       EINA_UNUSED,
              Evas_Object     *obj,
              void *event_info EINA_UNUSED)
{
   if (elm_object_tooltip_move_freeze_get(obj) == 0)
     {
        elm_object_tooltip_move_freeze_push(obj);
        elm_object_tooltip_text_set(obj, "Fronzen");
     }
   else
     {
        elm_object_tooltip_move_freeze_pop(obj);
        elm_object_tooltip_text_set(obj, "Free");
     }
}

/**
 * @brief Cycles through tooltip orientations on click and updates its text.
 *
 * This is a "clicked" callback for a button. Each click cycles to the next
 * tooltip orientation in the Elm_Tooltip_Orient enum. The tooltip text is
 * updated to display the name of the current orientation.
 *
 * @param data User data.
 * @param obj The object whose tooltip orientation is to be changed.
 * @param event_info Event-specific information.
 */
static void
_tt_orient_text_replace(void *data       EINA_UNUSED,
                        Evas_Object     *obj,
                        void *event_info EINA_UNUSED)
{
   static Elm_Tooltip_Orient orient;

   orient = elm_object_tooltip_orient_get(obj);
   orient++;
   if (orient >= ELM_TOOLTIP_ORIENT_LAST)
     orient = ELM_TOOLTIP_ORIENT_TOP_LEFT;
   switch(orient)
     {
      case ELM_TOOLTIP_ORIENT_TOP_LEFT:
        {
           elm_object_tooltip_orient_set(obj, ELM_TOOLTIP_ORIENT_TOP_LEFT);
           elm_object_tooltip_text_set(obj, "Top Left");
           printf("elm_object_tooltip_orient_get :: Orientation: ELM_TOOLTIP_ORIENT_TOP_LEFT\n");
           break;
        }
      case ELM_TOOLTIP_ORIENT_TOP:
        {
           elm_object_tooltip_text_set(obj, "Top");
           elm_object_tooltip_orient_set(obj, ELM_TOOLTIP_ORIENT_TOP);
           printf("elm_object_tooltip_orient_get :: Orientation: ELM_TOOLTIP_ORIENT_TOP\n");
           break;
        }
      case ELM_TOOLTIP_ORIENT_TOP_RIGHT:
        {
           elm_object_tooltip_text_set(obj, "Top Right");
           elm_object_tooltip_orient_set(obj, ELM_TOOLTIP_ORIENT_TOP_RIGHT);
           printf("elm_object_tooltip_orient_get :: Orientation: ELM_TOOLTIP_ORIENT_TOP_RIGHT\n");
           break;
        }
      case ELM_TOOLTIP_ORIENT_LEFT:
        {
           elm_object_tooltip_text_set(obj, "Left");
           elm_object_tooltip_orient_set(obj, ELM_TOOLTIP_ORIENT_LEFT);
           printf("elm_object_tooltip_orient_get :: Orientation: ELM_TOOLTIP_ORIENT_LEFT\n");
           break;
        }
      case ELM_TOOLTIP_ORIENT_CENTER:
        {
           elm_object_tooltip_text_set(obj, "Center");
           elm_object_tooltip_orient_set(obj, ELM_TOOLTIP_ORIENT_CENTER);
           printf("elm_object_tooltip_orient_get :: Orientation: ELM_TOOLTIP_ORIENT_CENTER\n");
           break;
        }
      case ELM_TOOLTIP_ORIENT_RIGHT:
        {
           elm_object_tooltip_text_set(obj, "Right");
           elm_object_tooltip_orient_set(obj, ELM_TOOLTIP_ORIENT_RIGHT);
           printf("elm_object_tooltip_orient_get :: Orientation: ELM_TOOLTIP_ORIENT_RIGHT\n");
           break;
        }
      case ELM_TOOLTIP_ORIENT_BOTTOM_LEFT:
        {
           elm_object_tooltip_text_set(obj, "Bottom Left");
           elm_object_tooltip_orient_set(obj, ELM_TOOLTIP_ORIENT_BOTTOM_LEFT);
           printf("elm_object_tooltip_orient_get :: Orientation: ELM_TOOLTIP_ORIENT_BOTTOM_LEFT\n");
           break;
        }
      case ELM_TOOLTIP_ORIENT_BOTTOM:
        {
           elm_object_tooltip_text_set(obj, "Bottom");
           elm_object_tooltip_orient_set(obj, ELM_TOOLTIP_ORIENT_BOTTOM);
           printf("elm_object_tooltip_orient_get :: Orientation: ELM_TOOLTIP_ORIENT_BOTTOM\n");
           break;
        }
      case ELM_TOOLTIP_ORIENT_BOTTOM_RIGHT:
        {
           elm_object_tooltip_text_set(obj, "Bottom Right");
           elm_object_tooltip_orient_set(obj, ELM_TOOLTIP_ORIENT_BOTTOM_RIGHT);
           printf("elm_object_tooltip_orient_get :: Orientation: ELM_TOOLTIP_ORIENT_BOTTOM_RIGHT\n");
           break;
        }
      default:
        {
           elm_object_tooltip_text_set(obj, "No Orientation");
           elm_object_tooltip_orient_set(obj, ELM_TOOLTIP_ORIENT_NONE);
           printf("elm_object_tooltip_orient_get :: Orientation: ELM_TOOLTIP_ORIENT_NONE\n");
        }
     };
}

/**
 * @brief Deletes the timer associated with a test object upon its deletion.
 *
 * This is an EVAS_CALLBACK_DEL event callback. It retrieves a timer from
 * the object's data ("test-timer") and deletes it. This ensures timers are
 * cleaned up properly when the object is destroyed, preventing dangling
 * pointers and memory leaks.
 *
 * @param data User data.
 * @param e The Evas canvas.
 * @param obj The object being deleted.
 * @param event_info Event-specific information.
 */
static void
_tt_timer_del(void *data       EINA_UNUSED,
              Evas *e          EINA_UNUSED,
              Evas_Object     *obj,
              void *event_info EINA_UNUSED)
{
   Ecore_Timer *timer = evas_object_data_del(obj, "test-timer");
   if (!timer) return;
   ecore_timer_del(timer);
}

/**
 * @brief Timer callback to replace tooltip text.
 *
 * This function is called periodically by a timer. It calls
 * _tt_text_replace() to update the tooltip text of the given object.
 *
 * @param data The object whose tooltip text should be replaced.
 * @return EINA_TRUE to keep the timer running, EINA_FALSE to stop.
 */
static Eina_Bool
_tt_text_replace_timer_cb(void *data)
{
   _tt_text_replace(NULL, data, NULL);
   return EINA_TRUE;
}

/**
 * @brief Starts or stops a timer that periodically replaces tooltip text.
 *
 * This is a "clicked" callback for a button. If no timer is running, it
 * starts one that calls _tt_text_replace_timer_cb() every 1.5 seconds.
 * If a timer is already running, it stops and deletes it. The button text
 * is updated to reflect the action (start/stop).
 *
 * @param data User data.
 * @param obj The button object that triggers the timer.
 * @param event_info Event-specific information.
 */
static void
_tt_text_replace_timed(void *data       EINA_UNUSED,
                       Evas_Object     *obj,
                       void *event_info EINA_UNUSED)
{
   Ecore_Timer *timer = evas_object_data_get(obj, "test-timer");
   if (timer)
     {
        ecore_timer_del(timer);
        evas_object_data_del(obj, "test-timer");
        elm_object_text_set(obj, "Simple text tooltip, click to start"
                                  " changed timed");
        return;
     }

   timer = ecore_timer_add(1.5, _tt_text_replace_timer_cb, obj);
   evas_object_data_set(obj, "test-timer", timer);
   elm_object_text_set(obj, "Simple text tooltip, click to stop changed"
                             " timed");
}

/**
 * @brief Tooltip content callback that creates content without a minimum size.
 *
 * This function creates a box as tooltip content, which expands to fill the
 * available space. It's used with a custom "nomin" style tooltip to demonstrate
 * tooltips that don't enforce a minimum size.
 *
 * @param data User data.
 * @param obj The object the tooltip is attached to.
 * @param tt The tooltip object.
 * @return An Evas_Object to be used as tooltip content.
 */
static Evas_Object *
_no_min_tt_icon(void *data   EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                Evas_Object *tt)
{
   Evas_Object *box = elm_box_add(tt);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(box);

   return box;
}

/**
 * @brief Tooltip content callback to create an icon.
 *
 * This callback is for elm_object_tooltip_content_cb_set(). It creates an
 * icon to be used as the tooltip content for a widget.
 *
 * @param data User data associated with the content callback.
 * @param obj The object the tooltip is attached to.
 * @param tt The tooltip object itself.
 * @return An icon object to be used as tooltip content.
 */
static Evas_Object *
_tt_icon(void *data   EINA_UNUSED,
         Evas_Object *obj EINA_UNUSED,
         Evas_Object *tt)
{
   Evas_Object *ic = elm_icon_add(tt);
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png",
            elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   evas_object_resize(ic, 64, 64);
   return ic;
}

/**
 * @brief Tooltip content callback to create a different icon.
 *
 * This callback is used to demonstrate timed replacement of tooltip content.
 * It provides an alternative icon to the one created by _tt_icon().
 *
 * @param data User data associated with the content callback.
 * @param obj The object the tooltip is attached to.
 * @param tt The tooltip object itself.
 * @return An icon object to be used as tooltip content.
 */
static Evas_Object *
_tt_icon2(void *data   EINA_UNUSED,
          Evas_Object *obj EINA_UNUSED,
          Evas_Object *tt)
{
   Evas_Object *ic = elm_icon_add(tt);
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/images/icon_00.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   evas_object_resize(ic, 64, 64);
   return ic;
}

/**
 * @brief Deletion callback for a tooltip's content.
 *
 * This function is registered as a deletion callback for tooltip content
 * created by _tt_icon(). It's used for testing to verify that the
 * callback is invoked correctly by printing the data it receives.
 *
 * @param data User data passed when setting the content cb (e.g., 123L).
 * @param obj The object the tooltip was attached to.
 * @param event_info The tooltip content object being deleted.
 */
static void
_tt_icon_del(void            *data,
             Evas_Object *obj EINA_UNUSED,
             void            *event_info)
{
   // test to check for del_cb behavior!
   printf("_tt_icon_del: data=%ld (== 123?), event_info=%p\n",
          (long)(uintptr_t)data, event_info);
}

/**
 * @brief Timer callback to replace tooltip icon content.
 *
 * This function is called periodically by a timer. It toggles between two
 * different icon creation callbacks (_tt_icon and _tt_icon2) for the
 * tooltip content, demonstrating dynamic content updates.
 *
 * @param data The object whose tooltip content is to be replaced.
 * @return EINA_TRUE to keep the timer running.
 */
static Eina_Bool
_tt_icon_replace_timer_cb(void *data)
{
   static int current = 0;

   elm_object_tooltip_content_cb_set
     (data, current ? _tt_icon2 : _tt_icon, NULL, NULL);

   current = !current;
   return EINA_TRUE;
}

/**
 * @brief Starts or stops a timer that periodically replaces the tooltip icon.
 *
 * This is a "clicked" callback. If no timer is running, it starts one that
 * calls _tt_icon_replace_timer_cb() every 1.5 seconds. If a timer is running,
 * it's stopped. The button text is updated to reflect the state.
 *
 * @param data User data.
 * @param obj The button object that triggers the timer.
 * @param event_info Event-specific information.
 */
static void
_tt_icon_replace_timed(void *data       EINA_UNUSED,
                       Evas_Object     *obj,
                       void *event_info EINA_UNUSED)
{
   Ecore_Timer *timer = evas_object_data_get(obj, "test-timer");
   if (timer)
     {
        ecore_timer_del(timer);
        evas_object_data_del(obj, "test-timer");
        elm_object_text_set(obj, "Icon tooltip, click to start changed"
                                  " timed");
        return;
     }

   timer = ecore_timer_add(1.5, _tt_icon_replace_timer_cb, obj);
   evas_object_data_set(obj, "test-timer", timer);
   elm_object_text_set(obj, "Icon tooltip, click to stop changed timed");
}

/**
 * @brief Timer callback to replace tooltip style.
 *
 * This function is called periodically by a timer. It toggles the tooltip
 * style between the default (NULL) and "transparent", demonstrating dynamic
 * style changes.
 *
 * @param data The object whose tooltip style is to be replaced.
 * @return EINA_TRUE to keep the timer running.
 */
static Eina_Bool
_tt_style_replace_timer_cb(void *data)
{
   static int current = 0;
   elm_object_tooltip_style_set(data, current ? NULL : "transparent");
   current = !current;
   return EINA_TRUE;
}

/**
 * @brief Starts or stops a timer that periodically replaces the tooltip style.
 *
 * This is a "clicked" callback. If no timer is running, it starts one that
 * calls _tt_style_replace_timer_cb() every 1.5 seconds to alternate the
 * tooltip's style. If a timer is running, it's stopped.
 *
 * @param data User data.
 * @param obj The button object that triggers the timer.
 * @param event_info Event-specific information.
 */
static void
_tt_style_replace_timed(void *data       EINA_UNUSED,
                        Evas_Object     *obj,
                        void *event_info EINA_UNUSED)
{
   Ecore_Timer *timer = evas_object_data_get(obj, "test-timer");
   if (timer)
     {
        ecore_timer_del(timer);
        evas_object_data_del(obj, "test-timer");
        elm_object_text_set(obj, "Icon tooltip style, click to start"
                                  " changed timed");
        return;
     }

   timer = ecore_timer_add(1.5, _tt_style_replace_timer_cb, obj);
   evas_object_data_set(obj, "test-timer", timer);
   elm_object_text_set(obj, "Icon tooltip style, click to stop changed"
                             " timed");
}

/**
 * @brief Toggles the visibility lock of a tooltip.
 *
 * This is a "clicked" callback for a button. It alternates between locking
 * the tooltip to be permanently visible (using elm_object_tooltip_show())
 * and unlocking it (using elm_object_tooltip_hide()). The button and tooltip
 * texts are updated to reflect the current state (locked/unlocked).
 *
 * @param data User data.
 * @param obj The button object.
 * @param event_info Event-specific information.
 */
static void
_tt_visible_lock_toggle(void *data       EINA_UNUSED,
                        Evas_Object     *obj,
                        void *event_info EINA_UNUSED)
{
   static int locked = 0;

   locked = !locked;
   if (locked)
     {
        elm_object_text_set(obj, "Locked tooltip visibility");
        elm_object_tooltip_text_set(obj, "This tooltip is locked"
                                         " visible,<br/> click the button"
                                         " to unlock!");
        elm_object_tooltip_show(obj);
     }
   else
     {
        elm_object_text_set(obj, "Unlocked tooltip visibility");
        elm_object_tooltip_text_set(obj, "This tooltip is unlocked"
                                         " visible,<br/> click the button"
                                         " to lock!");
        elm_object_tooltip_hide(obj);
     }
}

/**
 * @brief Main test function for various tooltip features.
 *
 * This function creates a window with several widgets to demonstrate:
 * - Simple text tooltips on buttons and toolbar items.
 * - Icon and label content in tooltips.
 * - Dynamically changing tooltip text, content, and style.
 * - Freezing tooltip movement.
 * - Changing tooltip orientation.
 * - Timed updates of tooltips.
 * - Custom tooltip styles ("transparent", "nomin").
 * - Locking tooltip visibility.
 * - Tooltips on entry widgets.
 * - Tooltips on list items with various content and window mode.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_tooltip(void *data       EINA_UNUSED,
             Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *tb, *bt, *se, *lst;
   Elm_Object_Item *tb_it;
   Elm_Object_Item *lit;

   win = elm_win_util_standard_add("tooltip", "Tooltip");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   tb = elm_toolbar_add(win);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);
   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   tb_it = elm_toolbar_item_append(tb, NULL, "Open", NULL, NULL);
   elm_object_item_tooltip_text_set(tb_it, "Opens a file");

   tb_it = elm_toolbar_item_append(tb, NULL, "Icon", NULL, NULL);
   elm_object_item_tooltip_content_cb_set
     (tb_it, _tt_item_icon, (void *)456L, _tt_item_icon_del);
   elm_object_item_tooltip_style_set(tb_it, "transparent");

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Simple text tooltip");
   elm_object_tooltip_text_set(bt, "Simple text tooltip");
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Movement Freeze Tooltip, click to change");
   elm_object_tooltip_text_set(bt, "Free");
   evas_object_smart_callback_add(bt, "clicked", _tt_move_freeze, NULL);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Orient Tooltip, click to change");
   elm_object_tooltip_orient_set(bt, ELM_TOOLTIP_ORIENT_TOP_LEFT);
   elm_object_tooltip_text_set(bt, "Top Left");
   evas_object_smart_callback_add(bt, "clicked", _tt_orient_text_replace, NULL);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Simple text tooltip, click to change");
   elm_object_tooltip_text_set(bt, "Initial");
   evas_object_smart_callback_add(bt, "clicked", _tt_text_replace, NULL);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Simple text tooltip, click to start"
                            " changed timed");
   elm_object_tooltip_text_set(bt, "Initial");
   evas_object_smart_callback_add(bt, "clicked", _tt_text_replace_timed,
                                  NULL);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   evas_object_event_callback_add(bt, EVAS_CALLBACK_DEL, _tt_timer_del,
                                  NULL);

   /* Add no min size tooltip style */
   elm_theme_extension_add(NULL, eina_slstr_printf("%s/objects/test_tooltip.edj", elm_app_data_dir_get()));

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Tooltip with no min size");
   elm_object_tooltip_content_cb_set(bt, _no_min_tt_icon, (void *)123L, _tt_icon_del);
   elm_object_tooltip_style_set(bt, "nomin");
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Icon tooltip, click to start changed timed");
   elm_object_tooltip_content_cb_set(bt, _tt_icon, NULL, NULL);
   evas_object_smart_callback_add(bt, "clicked", _tt_icon_replace_timed,
                                  NULL);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   evas_object_event_callback_add(bt, EVAS_CALLBACK_DEL, _tt_timer_del,
                                  NULL);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Transparent Icon tooltip");
   elm_object_tooltip_style_set(bt, "transparent");
   elm_object_tooltip_content_cb_set(bt, _tt_icon, NULL, NULL);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Icon tooltip style, click to start changed"
                            " timed");
   elm_object_tooltip_content_cb_set(bt, _tt_icon, NULL, NULL);
   evas_object_smart_callback_add(bt, "clicked", _tt_style_replace_timed,
                                  NULL);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   evas_object_event_callback_add(bt, EVAS_CALLBACK_DEL, _tt_timer_del,
                                  NULL);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Unlocked tooltip visibility");
   elm_object_tooltip_text_set(bt, "This tooltip is unlocked visible,<br/>"
                                   " click the button to lock!");
   evas_object_smart_callback_add(bt, "clicked", _tt_visible_lock_toggle,
                                  NULL);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   se = elm_entry_add(win);
   elm_entry_scrollable_set(se, EINA_TRUE);
   evas_object_size_hint_weight_set(se, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(se, EVAS_HINT_FILL, 0.5);
   elm_scroller_policy_set(se, ELM_SCROLLER_POLICY_OFF,
                                           ELM_SCROLLER_POLICY_OFF);
   elm_object_text_set(se, "Hello, some entry here!");
   elm_object_tooltip_text_set(se, "Type something here!");
   elm_entry_single_line_set(se, EINA_TRUE);
   elm_box_pack_end(bx, se);
   evas_object_show(se);

   lst = elm_list_add(win);
   lit = elm_list_item_append(lst, "Hello", NULL, NULL, NULL, NULL);
   elm_object_item_tooltip_window_mode_set(lit, EINA_TRUE);
   elm_object_item_tooltip_content_cb_set(lit, _tt_item_label, NULL, NULL);
   lit = elm_list_item_append(lst, "Icon Tooltip", NULL, NULL, NULL, NULL);
   elm_object_item_tooltip_content_cb_set(lit, _tt_item_icon, NULL, NULL);
   lit = elm_list_item_append(lst, "Big Icon Tooltip", NULL, NULL, NULL, NULL);
   elm_object_item_tooltip_content_cb_set(lit, _tt_item_icon2, NULL, NULL);
   elm_object_item_tooltip_style_set(lit, "transparent");
   elm_object_item_tooltip_window_mode_set(lit, EINA_TRUE);
   lit = elm_list_item_append(lst, "Insanely Big Icon Tooltip", NULL, NULL, NULL, NULL);
   elm_object_item_tooltip_content_cb_set(lit, _tt_item_icon3, NULL, NULL);
   elm_object_item_tooltip_style_set(lit, "transparent");
   elm_object_item_tooltip_window_mode_set(lit, EINA_TRUE);
   evas_object_size_hint_weight_set(lst, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(lst, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(lst, 100, 100);
   elm_list_go(lst);
   elm_box_pack_end(bx, lst);
   evas_object_show(lst);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           580 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test function for tooltips on complex collection widgets.
 *
 * This function creates a window with a gengrid and a genlist to demonstrate
 * tooltips on their items.
 * - The gengrid displays a grid of photos, each with a text tooltip. The item
 *   data is stored in an array of Testitem structs.
 *   `static Testitem ti[144];`
 *   Each element contains:
 *   - `item`: The Elm_Object_Item itself.
 *   - `path`: `const char*` to an image file.
 *   - `mode`, `onoff`: integers for state.
 * - The genlist has expandable items. Tooltips are set on parent items,
 *   and also on the sub-items that are created dynamically when a parent
 *   is expanded.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_tooltip2(void *data       EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED,
              void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *grid, *gl;
   Elm_Object_Item *glit1, *glit2, *glit3;
   static Testitem ti[144];
   int i, n;
   char buf[PATH_MAX];
   const char *img[9] =
   {
      "panel_01.jpg",
      "plant_01.jpg",
      "rock_01.jpg",
      "rock_02.jpg",
      "sky_01.jpg",
      "sky_02.jpg",
      "sky_03.jpg",
      "sky_04.jpg",
      "wood_01.jpg",
   };

   win = elm_win_util_standard_add("tooltip2", "Tooltip 2");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   grid = elm_gengrid_add(win);
   elm_gengrid_item_size_set(grid, 100, 100);
   elm_gengrid_horizontal_set(grid, EINA_FALSE);
   elm_gengrid_multi_select_set(grid, EINA_TRUE);
   evas_object_size_hint_align_set(grid, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(grid, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);

   gic.item_style = "default";
   gic.func.text_get = grdt_lbl_get;
   gic.func.content_get = grdt_content_get;

   n = 0;
   for (i = 0; i < 9; i++)
     {
        snprintf(buf, sizeof(buf), "%s/images/%s", elm_app_data_dir_get(),
                 img[n]);
        n++;
        ti[i].mode = i;
        ti[i].path = eina_stringshare_add(buf);
        ti[i].item = elm_gengrid_item_append(grid, &gic, &(ti[i]), NULL,
                                             NULL);
        if (n % 2)
          elm_gengrid_item_tooltip_text_set(ti[i].item, "Testing X");
        else
          elm_gengrid_item_tooltip_text_set(ti[i].item, "Testing Y");
        if (!(i % 5))
          elm_gengrid_item_selected_set(ti[i].item, EINA_TRUE);
     }

   elm_box_pack_end(bx, grid);
   evas_object_show(grid);

   gl = elm_genlist_add(win);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);

   itct.item_style = "default";
   itct.func.text_get = gltt_text_get;

   glit1 = elm_genlist_item_append(gl, &itct, (void *)1, NULL,
                                   ELM_GENLIST_ITEM_TREE, NULL, NULL);
   glit2 = elm_genlist_item_append(gl, &itct, (void *)2, NULL,
                                   ELM_GENLIST_ITEM_TREE, NULL, NULL);
   glit3 = elm_genlist_item_append(gl, &itct, (void *)3, NULL,
                                   ELM_GENLIST_ITEM_NONE, NULL, NULL);

   elm_genlist_item_tooltip_text_set(glit1, "Testing 1");
   elm_genlist_item_tooltip_text_set(glit2, "Testing 2");
   elm_genlist_item_tooltip_text_set(glit3, "Testing 3");

   evas_object_smart_callback_add(gl, "expand,request", gltt_exp_req, gl);
   evas_object_smart_callback_add(gl, "contract,request", gltt_con_req,
                                  gl);
   evas_object_smart_callback_add(gl, "expanded", gltt_exp, gl);
   evas_object_smart_callback_add(gl, "contracted", gltt_con, gl);

   elm_box_pack_end(bx, gl);
   evas_object_show(gl);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           480 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test function for tooltip behavior with object layers.
 *
 * This function creates a window with a button and a rectangle object. The
 * button is placed on a higher Evas layer than the rectangle. The test is to
 * ensure the button's tooltip is displayed correctly above other objects,
 * respecting the layering.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_tooltip3(void *data       EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED,
              void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bt, *rect;

   win = elm_win_util_standard_add("tooltip3", "Tooltip 3");
   elm_win_autodel_set(win, EINA_TRUE);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "I have layer 200, below rect has layer 100.");
   elm_object_tooltip_text_set(bt, "Can you see me?");
   evas_object_resize(bt, 250, 30);
   evas_object_move(bt, 25, 135);
   evas_object_layer_set(bt, 200);
   evas_object_show(bt);

   rect = evas_object_rectangle_add(evas_object_evas_get(bt));
   evas_object_resize(rect, 150, 300);
   evas_object_move(rect, 0, 0);
   evas_object_show(rect);
   evas_object_layer_set(rect, 100);

   evas_object_resize(win, 300 * elm_config_scale_get(),
                           300 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test function for tooltip orientation at window edges.
 *
 * This function creates a window and places several buttons near the edges
 * and corners. Each button has a tooltip with a specific orientation set.
 * This test verifies that tooltips are correctly positioned and constrained
 * within the window boundaries, even when their preferred orientation would
 * place them outside.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_tooltip4(void *data       EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED,
              void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bt;

   win = elm_win_util_standard_add("tooltip4", "Tooltip 4");
   elm_win_autodel_set(win, EINA_TRUE);

   bt = elm_button_add(win);
   elm_object_tooltip_text_set(bt, "Tooltip!!!!!!!!!!!!!!!!!!");
   elm_object_tooltip_orient_set(bt, ELM_TOOLTIP_ORIENT_BOTTOM);
   evas_object_resize(bt, 30, 30);
   evas_object_move(bt, 5, 5);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_tooltip_text_set(bt, "Tooltip!!!!!!!!!!!!!!!!!!");
   elm_object_tooltip_orient_set(bt, ELM_TOOLTIP_ORIENT_BOTTOM);
   evas_object_resize(bt, 30, 30);
   evas_object_move(bt, 250, 5);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_tooltip_text_set(bt, "Tooltip!!!!!!!!!!!!!!!!!!");
   elm_object_tooltip_orient_set(bt, ELM_TOOLTIP_ORIENT_TOP);
   evas_object_resize(bt, 30, 30);
   evas_object_move(bt, 25, 265);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_tooltip_text_set(bt, "Tooltip!!!!!!!!!!!!!!!!!!");
   elm_object_tooltip_orient_set(bt, ELM_TOOLTIP_ORIENT_TOP);
   evas_object_resize(bt, 30, 30);
   evas_object_move(bt, 235, 265);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_tooltip_content_cb_set(bt, _tt_icon, (void *)123L,
                                     _tt_icon_del);
   elm_object_tooltip_orient_set(bt, ELM_TOOLTIP_ORIENT_RIGHT);
   evas_object_resize(bt, 30, 30);
   evas_object_move(bt, 135, 5);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_tooltip_content_cb_set(bt, _tt_icon, (void *)123L,
                                     _tt_icon_del);
   elm_object_tooltip_orient_set(bt, ELM_TOOLTIP_ORIENT_LEFT);
   evas_object_resize(bt, 30, 30);
   evas_object_move(bt, 135, 255);
   evas_object_show(bt);

   evas_object_resize(win, 300 * elm_config_scale_get(),
                           300 * elm_config_scale_get());
   evas_object_show(win);
}
