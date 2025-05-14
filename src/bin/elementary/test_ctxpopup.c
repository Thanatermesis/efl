#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Global variable to track the mouse down state on the list.
 * This is used to prevent ctxpopups from opening on mouse up events if a
 * drag-like motion occurred.
 */
static int list_mouse_down = 0;

/**
 * @brief Callback for the "dismissed" smart event of a ctxpopup.
 * Deletes the ctxpopup object when it is dismissed.
 * @param data User data, unused.
 * @param obj The ctxpopup object that was dismissed.
 * @param event_info Event-specific information, unused.
 */
static void
_dismissed(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   evas_object_del(obj);
}

/**
 * @brief Prints the current direction of the ctxpopup.
 * The direction indicates where the popup appears relative to its parent.
 * It also prints the auto-hide mode.
 * @param obj The ctxpopup object.
 */
static void
_print_current_dir(Evas_Object *obj)
{
   Elm_Ctxpopup_Direction dir;
   dir = elm_ctxpopup_direction_get(obj);

   switch(dir)
     {
      case ELM_CTXPOPUP_DIRECTION_LEFT:
         printf("ctxpopup direction: left!\n");
         break;
      case ELM_CTXPOPUP_DIRECTION_RIGHT:
         printf("ctxpopup direction: right!\n");
         break;
      case ELM_CTXPOPUP_DIRECTION_UP:
         printf("ctxpopup direction: up!\n");
         break;
      case ELM_CTXPOPUP_DIRECTION_DOWN:
         printf("ctxpopup direction: down!\n");
         break;
      case ELM_CTXPOPUP_DIRECTION_UNKNOWN:
         printf("ctxpopup direction: unknown!\n");
         break;
     }
     printf(" [%s : %d] auto_hide_mode=%d\n", __func__, __LINE__, elm_ctxpopup_auto_hide_disabled_get(obj));
}

/**
 * @brief Callback for a button click event.
 * Toggles the disabled state of the object passed in the @p data parameter.
 * In this test, it is used to disable/enable the ctxpopup itself from a
 * button inside its content.
 * @param data The object to disable/enable (the ctxpopup).
 * @param obj The button object that was clicked, unused.
 * @param event_info Event-specific information, unused.
 */
static void
_btn_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   printf("Button Clicked\n");
   elm_object_disabled_set(data, !elm_object_disabled_get(data));
}

/**
 * @brief Callback for a button click event to remove an object.
 * This function deletes the button object itself when clicked.
 * It's used in a test where a ctxpopup has a button as its content.
 * @param data User data, unused.
 * @param obj The button object to be deleted.
 * @param event_info Event-specific information, unused.
 */
static void
_btn_remove_cb(void *data EINA_UNUSED, Evas_Object *obj,
                void *event_info EINA_UNUSED)
{
   printf("Button Remove\n");
   evas_object_del(obj);
}

/**
 * @brief Generic callback for ctxpopup item selection.
 * Prints information about the selected item and then dismisses the ctxpopup.
 * @param data User data, unused.
 * @param obj The ctxpopup object.
 * @param event_info The selected Elm_Object_Item.
 */
static void
_ctxpopup_item_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   printf("Item selected status: %d\n", elm_ctxpopup_item_selected_get(event_info));

   printf("ctxpopup item selected: %s\n",
         elm_object_item_text_get(elm_ctxpopup_selected_item_get(obj)));
   elm_ctxpopup_dismiss(obj);
}

/**
 * @brief Helper function to create and append a new item to a ctxpopup.
 * @param obj The ctxpopup object.
 * @param label The text label for the item.
 * @param icon The name of a standard icon for the item.
 * @return The newly created Elm_Object_Item, or NULL on failure.
 */
static Elm_Object_Item *
_ctxpopup_item_new(Evas_Object *obj, const char *label, const char *icon)
{
   Evas_Object *ic = NULL;
   Elm_Object_Item *it = NULL;

   if (!obj) return NULL;

   if (icon)
     {
        ic = elm_icon_add(obj);
        elm_icon_standard_set(ic, icon);
        elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
     }

   it = elm_ctxpopup_item_append(obj, label, ic, _ctxpopup_item_cb, NULL);
   return it;
}

/**
 * @brief Callback for the "geometry,update" smart event of a ctxpopup.
 * Prints the new geometry of the ctxpopup.
 * @param data User data, unused.
 * @param obj The ctxpopup object, unused.
 * @param event_info A pointer to an Eina_Rectangle with the new geometry.
 */
static void
_geometry_update(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Eina_Rectangle *geom = event_info;
   printf("ctxpopup geometry(%d %d %d %d)\n", geom->x, geom->y, geom->w, geom->h);
}

/**
 * @brief Callback for selecting the first list item.
 * Creates a standard ctxpopup with both icons and labels for its items.
 * One item is pre-selected and another is disabled.
 * @param data User data, unused.
 * @param obj The list object.
 * @param event_info Event-specific information, unused.
 */
static void
_list_item_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ctxpopup;
   Elm_Object_Item *it = NULL;
   Evas_Coord x,y;

   if (list_mouse_down > 0) return;

   ctxpopup = elm_ctxpopup_add(obj);
   evas_object_smart_callback_add(ctxpopup, "dismissed", _dismissed, NULL);
   evas_object_smart_callback_add(ctxpopup, "geometry,update", _geometry_update, NULL);

   _ctxpopup_item_new(ctxpopup, "Go to home folder", "home");
   _ctxpopup_item_new(ctxpopup, "Save file", "file");
   it = _ctxpopup_item_new(ctxpopup, "Delete file", "delete");
   elm_ctxpopup_item_selected_set(it, EINA_TRUE);
   it = _ctxpopup_item_new(ctxpopup, "Navigate to folder", "folder");
   elm_object_item_disabled_set(it, EINA_TRUE);
   _ctxpopup_item_new(ctxpopup, "Edit entry", "edit");
   it = _ctxpopup_item_new(ctxpopup, "Set date and time", "clock");
   elm_object_item_disabled_set(it, EINA_TRUE);

   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &x, &y);
   evas_object_size_hint_max_set(ctxpopup, 240, 240);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);
   _print_current_dir(ctxpopup);
}

/**
 * @brief Callback for selecting the second list item.
 * Creates a ctxpopup with only icons for its items.
 * @param data User data, unused.
 * @param obj The list object.
 * @param event_info Event-specific information, unused.
 */
static void
_list_item_cb2(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ctxpopup;
   Elm_Object_Item *it = NULL;
   Evas_Coord x,y;

   if (list_mouse_down > 0) return;

   ctxpopup = elm_ctxpopup_add(obj);
   evas_object_smart_callback_add(ctxpopup, "dismissed", _dismissed, NULL);
   evas_object_smart_callback_add(ctxpopup, "geometry,update", _geometry_update, NULL);

   _ctxpopup_item_new(ctxpopup, NULL, "home");
   _ctxpopup_item_new(ctxpopup, NULL, "file");
   _ctxpopup_item_new(ctxpopup, NULL, "delete");
   _ctxpopup_item_new(ctxpopup, NULL, "folder");
   it = _ctxpopup_item_new(ctxpopup, NULL, "edit");
   elm_object_item_disabled_set(it, EINA_TRUE);
   _ctxpopup_item_new(ctxpopup, NULL, "clock");

   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &x, &y);
   evas_object_size_hint_max_set(ctxpopup, 240, 240);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);
   _print_current_dir(ctxpopup);
}

/**
 * @brief Callback for selecting the third list item.
 * Creates a ctxpopup with only labels for its items. Also demonstrates
 * deleting an item immediately after creation.
 * @param data User data, unused.
 * @param obj The list object.
 * @param event_info Event-specific information, unused.
 */
static void
_list_item_cb3(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ctxpopup;
   Elm_Object_Item *it = NULL;
   Evas_Coord x,y;

   if (list_mouse_down > 0) return;

   ctxpopup = elm_ctxpopup_add(obj);
   evas_object_smart_callback_add(ctxpopup, "dismissed", _dismissed, NULL);
   evas_object_smart_callback_add(ctxpopup, "geometry,update", _geometry_update, NULL);

   _ctxpopup_item_new(ctxpopup, "Eina", NULL);
   _ctxpopup_item_new(ctxpopup, "Eet", NULL);
   _ctxpopup_item_new(ctxpopup, "Evas", NULL);
   it = _ctxpopup_item_new(ctxpopup, "Ecore", NULL);
   elm_object_item_disabled_set(it, EINA_TRUE);
   _ctxpopup_item_new(ctxpopup, "Embryo", NULL);
   _ctxpopup_item_new(ctxpopup, "Edje", NULL);
   it = _ctxpopup_item_new(ctxpopup, "Never Show", NULL);
   elm_object_item_del(it);

   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &x, &y);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);
   _print_current_dir(ctxpopup);
}

/**
 * @brief Callback for selecting the fourth list item.
 * Creates a horizontal ctxpopup with only icons.
 * @param data User data, unused.
 * @param obj The list object.
 * @param event_info Event-specific information, unused.
 */
static void
_list_item_cb4(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ctxpopup;
   Evas_Coord x,y;

   if (list_mouse_down > 0) return;

   ctxpopup = elm_ctxpopup_add(obj);
   evas_object_smart_callback_add(ctxpopup, "dismissed", _dismissed, NULL);
   evas_object_smart_callback_add(ctxpopup, "geometry,update", _geometry_update, NULL);

   elm_ctxpopup_horizontal_set(ctxpopup, EINA_TRUE);

   _ctxpopup_item_new(ctxpopup, NULL, "home");
   _ctxpopup_item_new(ctxpopup, NULL, "file");
   _ctxpopup_item_new(ctxpopup, NULL, "delete");
   _ctxpopup_item_new(ctxpopup, NULL, "folder");
   _ctxpopup_item_new(ctxpopup, NULL, "edit");
   _ctxpopup_item_new(ctxpopup, NULL, "clock");

   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &x, &y);
   evas_object_size_hint_max_set(ctxpopup, 240, 240);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);
   _print_current_dir(ctxpopup);
}

/**
 * @brief Callback for selecting the fifth list item.
 * Creates a ctxpopup with custom content: a button inside a scroller.
 * The button inside can disable/enable the ctxpopup.
 * @param data User data, unused.
 * @param obj The list object.
 * @param event_info Event-specific information, unused.
 */
static void
_list_item_cb5(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ctxpopup, *btn, *sc, *bx;
   Evas_Coord x,y;

   if (list_mouse_down > 0) return;

   bx = elm_box_add(obj);
   evas_object_size_hint_min_set(bx, 150, 150);

   sc = elm_scroller_add(bx);
   elm_scroller_bounce_set(sc, EINA_FALSE, EINA_TRUE);
   evas_object_size_hint_fill_set(sc, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(sc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(sc);

   btn = elm_button_add(sc);
   elm_object_text_set(btn, "Enlightenment");
   evas_object_size_hint_min_set(btn, 150, 150);

   elm_object_content_set(sc, btn);

   elm_box_pack_end(bx, sc);

   ctxpopup = elm_ctxpopup_add(obj);
   evas_object_smart_callback_add(ctxpopup, "dismissed", _dismissed, NULL);
   evas_object_smart_callback_add(ctxpopup, "geometry,update", _geometry_update, NULL);

   elm_object_content_set(ctxpopup, bx);

   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &x, &y);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);
   _print_current_dir(ctxpopup);

   evas_object_smart_callback_add(btn, "clicked", _btn_clicked_cb, ctxpopup);
}

/**
 * @brief Callback for the EVAS_CALLBACK_DEL event on a ctxpopup.
 * Deletes an associated object when the ctxpopup is deleted.
 * Used to clean up the image created in the restacking test.
 * @param data The object to delete (an Evas_Object image).
 * @param e The Evas canvas, unused.
 * @param obj The ctxpopup object being deleted, unused.
 * @param event_info Event-specific information, unused.
 */
static void
_ctxpopup_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   evas_object_del(data);
}

/**
 * @brief Callback for a button click to test object stacking.
 * Creates an image view and raises the ctxpopup above it, to ensure
 * the ctxpopup stays on top. Also sets up a callback to delete the
 * image when the ctxpopup is deleted.
 * @param data The ctxpopup object.
 * @param obj The button object that was clicked.
 * @param event_info Event-specific information, unused.
 */
static void
_restack_btn_clicked_cb(void *data, Evas_Object *obj,
                        void *event_info EINA_UNUSED)
{
   Evas_Object *im, *ctxpopup = data;
   char buf[PATH_MAX];

   printf("Restack button clicked\n");

   im = evas_object_image_filled_add(evas_object_evas_get(obj));
   snprintf(buf, sizeof(buf), "%s/images/%s",
            elm_app_data_dir_get(), "twofish.jpg");
   evas_object_image_file_set(im, buf, NULL);
   evas_object_move(im, 40, 40);
   evas_object_resize(im, 320, 320);
   evas_object_show(im);

   evas_object_raise(ctxpopup);
   evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL,
                                  _ctxpopup_del_cb, im);
}

/**
 * @brief Callback for selecting the sixth list item.
 * Creates a ctxpopup to test object restacking. The content is a button
 * that, when clicked, creates an image and ensures the ctxpopup is raised
 * above it.
 * @param data User data, unused.
 * @param obj The list object.
 * @param event_info Event-specific information, unused.
 */
static void
_list_item_cb6(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ctxpopup, *btn, *sc, *bx;
   Evas_Coord x,y;

   if (list_mouse_down > 0) return;

   bx = elm_box_add(obj);
   evas_object_size_hint_min_set(bx, 200, 150);

   sc = elm_scroller_add(bx);
   elm_scroller_bounce_set(sc, EINA_FALSE, EINA_TRUE);
   evas_object_size_hint_fill_set(sc, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(sc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(sc);

   btn = elm_button_add(sc);
   elm_object_text_set(btn, "Ctxpop will be on the top of layer");
   evas_object_size_hint_min_set(btn, 190, 140);

   elm_object_content_set(sc, btn);

   elm_box_pack_end(bx, sc);

   ctxpopup = elm_ctxpopup_add(obj);
   evas_object_smart_callback_add(ctxpopup, "dismissed", _dismissed, NULL);
   evas_object_smart_callback_add(ctxpopup, "geometry,update", _geometry_update, NULL);

   elm_object_content_set(ctxpopup, bx);

   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &x, &y);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);
   _print_current_dir(ctxpopup);

   evas_object_smart_callback_add(btn, "clicked",
                                  _restack_btn_clicked_cb, ctxpopup);
}

/**
 * @brief Callback for a ctxpopup item that disables itself when selected.
 * @param data User data, unused.
 * @param obj The ctxpopup object, unused.
 * @param event_info The selected Elm_Object_Item, which will be disabled.
 */
static void
_ctxpopup_item_disable_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   printf("ctxpopup item selected: %s\n",
          elm_object_item_text_get(event_info));

   elm_object_item_disabled_set(event_info, EINA_TRUE);
}

/**
 * @brief Callback for a ctxpopup item that deletes the entire ctxpopup when selected.
 * @param data User data, unused.
 * @param obj The ctxpopup object to be deleted.
 * @param event_info The selected Elm_Object_Item.
 */
static void
_ctxpopup_item_delete_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   printf("ctxpopup item selected: %s\n",
          elm_object_item_text_get(event_info));

   evas_object_del(obj);
}

/**
 * @brief Callback for selecting the seventh list item.
 * Creates a ctxpopup and tests dynamic item manipulation functions like
 * elm_ctxpopup_item_prepend(), elm_ctxpopup_item_insert_before(), and
 * elm_ctxpopup_item_insert_after(). It also uses special callbacks for
 * some items.
 * @param data User data, unused.
 * @param obj The list object.
 * @param event_info Event-specific information, unused.
 */
static void
_list_item_cb7(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ctxpopup;
   Evas_Coord x,y;
   Elm_Object_Item *item;

   if (list_mouse_down > 0) return;
   ctxpopup = elm_ctxpopup_add(obj);
   evas_object_smart_callback_add(ctxpopup, "dismissed", _dismissed, NULL);
   evas_object_smart_callback_add(ctxpopup, "geometry,update", _geometry_update, NULL);

   item = elm_ctxpopup_item_prepend(ctxpopup, "Disable this item", NULL, _ctxpopup_item_disable_cb, ctxpopup);
   elm_ctxpopup_item_insert_before(ctxpopup, item, "Delete this ctxpopup", NULL, _ctxpopup_item_delete_cb, ctxpopup);
   elm_ctxpopup_item_insert_after(ctxpopup, item, "Another item", NULL, _ctxpopup_item_cb, NULL);

   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &x, &y);
   evas_object_size_hint_max_set(ctxpopup, 240, 240);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);
   _print_current_dir(ctxpopup);
}

/**
 * @brief Callback for selecting the eighth list item.
 * Creates a ctxpopup with auto-hide disabled. This means the popup will
 * not be dismissed when clicking outside of it.
 * @param data User data, unused.
 * @param obj The list object.
 * @param event_info Event-specific information, unused.
 */
static void
_list_item_cb8(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ctxpopup;
   Elm_Object_Item *it = NULL;
   Evas_Coord x,y;

   if (list_mouse_down > 0) return;

   ctxpopup = elm_ctxpopup_add(obj);
   evas_object_smart_callback_add(ctxpopup, "dismissed", _dismissed, NULL);
   evas_object_smart_callback_add(ctxpopup, "geometry,update", _geometry_update, NULL);

   elm_ctxpopup_auto_hide_disabled_set(ctxpopup, EINA_TRUE);

   _ctxpopup_item_new(ctxpopup, "Go to home folder", "home");
   _ctxpopup_item_new(ctxpopup, "Save file", "file");
   _ctxpopup_item_new(ctxpopup, "Delete file", "delete");
   it = _ctxpopup_item_new(ctxpopup, "Navigate to folder", "folder");
   elm_object_item_disabled_set(it, EINA_TRUE);
   _ctxpopup_item_new(ctxpopup, "Edit entry", "edit");

   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &x, &y);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);
   _print_current_dir(ctxpopup);
}

/**
 * @brief Callback for ctxpopup items that modify another item.
 * Depending on the selected item's text, this callback will set or unset
 * the text or icon of a target item.
 * @param data The target Elm_Object_Item to be modified.
 * @param obj The ctxpopup object.
 * @param event_info The selected Elm_Object_Item that triggered the callback.
 */
static void
_ctxpopup_item_select_cb(void *data, Evas_Object *obj, void *event_info)
{
   const char *text = NULL;
   Evas_Object *icon = NULL;
   Elm_Object_Item *it = (Elm_Object_Item *)data;

   text = elm_object_item_part_text_get((Elm_Object_Item *)event_info, "default");

   if (!strcmp(text, "Text NULL"))
     {
        elm_object_item_part_text_set(it, "default", NULL);
     }
   else if (!strcmp(text, "Text Set"))
     {
        elm_object_item_part_text_set(it, "default", "Save file");
     }
   else if (!strcmp(text, "Icon NULL"))
     {
        elm_object_item_part_content_set(it, "icon", NULL);
     }
   else if (!strcmp(text, "Icon Set"))
     {
        icon = elm_icon_add(obj);
        elm_icon_standard_set(icon, "file");
        elm_image_resizable_set(icon, EINA_FALSE, EINA_FALSE);

        elm_object_item_part_content_set(it, "icon", icon);
     }
   else
     {
        icon = elm_object_item_part_content_unset(it, "icon");
        if (icon) evas_object_del(icon);
     }
}

/**
 * @brief Callback for selecting the ninth list item.
 * Creates a ctxpopup to test modifying an item's parts (text and content)
 * dynamically from other items' callbacks.
 * @param data User data, unused.
 * @param obj The list object.
 * @param event_info Event-specific information, unused.
 */
static void
_list_item_cb9(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ctxpopup;
   Elm_Object_Item *it = NULL;
   Evas_Coord x,y;

   if (list_mouse_down > 0) return;

   ctxpopup = elm_ctxpopup_add(obj);
   evas_object_smart_callback_add(ctxpopup, "dismissed", _dismissed, NULL);
   evas_object_smart_callback_add(ctxpopup, "geometry,update", _geometry_update, NULL);

   it = elm_ctxpopup_item_append(ctxpopup, NULL, NULL, _ctxpopup_item_cb, NULL);
   elm_ctxpopup_item_append(ctxpopup, "Text NULL", NULL, _ctxpopup_item_select_cb, it);
   elm_ctxpopup_item_append(ctxpopup, "Text Set", NULL, _ctxpopup_item_select_cb, it);
   elm_ctxpopup_item_append(ctxpopup, "Icon NULL", NULL, _ctxpopup_item_select_cb, it);
   elm_ctxpopup_item_append(ctxpopup, "Icon Set", NULL, _ctxpopup_item_select_cb, it);
   elm_ctxpopup_item_append(ctxpopup, "Icon Unset", NULL, _ctxpopup_item_select_cb, it);

   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &x, &y);
   evas_object_size_hint_max_set(ctxpopup, 240, 240);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);
   _print_current_dir(ctxpopup);
}

/**
 * @brief Callback for selecting the tenth list item.
 * Creates a ctxpopup with a single button as content. Clicking the button
 * removes the button itself.
 * @param data User data, unused.
 * @param obj The list object.
 * @param event_info Event-specific information, unused.
 */
static void
_list_item_cb10(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ctxpopup, *btn;
   Evas_Coord x,y;

   if (list_mouse_down > 0) return;

   ctxpopup = elm_ctxpopup_add(obj);
   evas_object_smart_callback_add(ctxpopup, "dismissed", _dismissed, NULL);
   evas_object_smart_callback_add(ctxpopup, "geometry,update", _geometry_update, NULL);

   btn = elm_button_add(ctxpopup);
   elm_object_text_set(btn, "Click to remove");
   evas_object_size_hint_min_set(btn, 150, 150);
   evas_object_smart_callback_add(btn, "clicked", _btn_remove_cb, ctxpopup);

   elm_object_content_set(ctxpopup, btn);

   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &x, &y);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);
   _print_current_dir(ctxpopup);
}

/**
 * @brief Callback for selecting the eleventh list item.
 * Creates a ctxpopup with a large number of items (100) to test
 * performance and scrolling behavior.
 * @param data User data, unused.
 * @param obj The list object.
 * @param event_info Event-specific information, unused.
 */
static void
_list_item_cb11(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ctxpopup;
   Evas_Coord x,y;
   char label[18];
   int i = 0;

   if (list_mouse_down > 0) return;

   ctxpopup = elm_ctxpopup_add(obj);
   evas_object_smart_callback_add(ctxpopup, "dismissed", _dismissed, NULL);
   evas_object_smart_callback_add(ctxpopup, "geometry,update", _geometry_update, NULL);

   while (i++ < 100)
     {
        snprintf(label, sizeof(label), "Item %d", i);
        _ctxpopup_item_new(ctxpopup, label, "clock");
     }

   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &x, &y);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);
}

/**
 * @brief Callback for the "selected" smart event on the list.
 * This is used to immediately unselect a list item after it has been clicked.
 * This prevents the item from remaining in a selected state visually.
 * @param data User data, unused.
 * @param obj The list object, unused.
 * @param event_info The selected Elm_Object_Item.
 */
static void
_list_clicked(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   elm_list_item_selected_set(event_info, EINA_FALSE);
}

/**
 * @brief Callback for the EVAS_CALLBACK_MOUSE_DOWN event on the list.
 * Increments a counter to track that the mouse button is pressed.
 * This helps prevent triggering actions on mouse up if a drag occurred.
 * @param data User data, unused.
 * @param e The Evas canvas, unused.
 * @param obj The list object, unused.
 * @param event_info Event-specific information, unused.
 */
static void
_list_mouse_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   list_mouse_down++;
}

/**
 * @brief Callback for the EVAS_CALLBACK_MOUSE_UP event on the list.
 * Decrements a counter to track that the mouse button is released.
 * @param data User data, unused.
 * @param e The Evas canvas, unused.
 * @param obj The list object, unused.
 * @param event_info Event-specific information, unused.
 */
static void
_list_mouse_up(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   list_mouse_down--;
}

/**
 * @brief Callback for the "delete,request" smart event on the window.
 * Resets the global list_mouse_down counter when the window is closed.
 * @param data User data, unused.
 * @param obj The window object, unused.
 * @param event_info Event-specific information, unused.
 */
static void
_win_del(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   list_mouse_down = 0;
}

/**
 * @brief The main function for the Ctxpopup test.
 * This function creates a window and a list. The list contains several items,
 * each demonstrating a different feature or use case of the Elm_Ctxpopup widget.
 * @param data User data, unused.
 * @param obj The parent object, unused.
 * @param event_info Event-specific information, unused.
 */
void
test_ctxpopup(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *list;

   win = elm_win_util_standard_add("contextual-popup", "Contextual Popup");
   evas_object_smart_callback_add(win, "delete,request", _win_del, NULL);
   elm_win_autodel_set(win, EINA_TRUE);

   list = elm_list_add(win);
   evas_object_event_callback_add(list, EVAS_CALLBACK_MOUSE_DOWN,
                                  _list_mouse_down, NULL);
   evas_object_event_callback_add(list, EVAS_CALLBACK_MOUSE_UP,
                                  _list_mouse_up, NULL);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, list);
   elm_list_mode_set(list, ELM_LIST_COMPRESS);
   evas_object_smart_callback_add(list, "selected", _list_clicked, NULL);

   elm_list_item_append(list, "Ctxpopup with icons and labels", NULL, NULL,
                        _list_item_cb, NULL);
   elm_list_item_append(list, "Ctxpopup with icons only", NULL, NULL,
                        _list_item_cb2, NULL);
   elm_list_item_append(list, "Ctxpopup with labels only", NULL, NULL,
                        _list_item_cb3, NULL);
   elm_list_item_append(list, "Ctxpopup at horizontal mode", NULL, NULL,
                        _list_item_cb4, NULL);
   elm_list_item_append(list, "Ctxpopup with user content", NULL, NULL,
                        _list_item_cb5, NULL);
   elm_list_item_append(list, "Ctxpopup with restacking", NULL, NULL,
                        _list_item_cb6, NULL);
   elm_list_item_append(list, "Ctxpopup with callback function", NULL, NULL,
                        _list_item_cb7, NULL);
   elm_list_item_append(list, "Ctxpopup with auto hide disabled mode", NULL, NULL,
                        _list_item_cb8, NULL);
   elm_list_item_append(list, "Ctxpopup with part text & content function", NULL, NULL,
                        _list_item_cb9, NULL);
   elm_list_item_append(list, "Ctxpopup with user content (enable to remove)", NULL, NULL,
                        _list_item_cb10, NULL);
   elm_list_item_append(list, "Ctxpopup with more items", NULL, NULL,
                        _list_item_cb11, NULL);
   evas_object_show(list);
   elm_list_go(list);

   evas_object_resize(win, 400 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}
