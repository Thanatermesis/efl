#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

/**
 * @internal
 * @addtogroup Widget
 * @{
 *
 * @section elm-photo-internal Internal functions for Elm_Photo widget
 *
 * These are internal functions, data structures, and macros used
 * for the Elm_Photo widget.
 */

#define EFL_ACCESS_OBJECT_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_photo.h"
#include "elm_photo_eo.h"
#include "elm_icon_eo.h"

#define MY_CLASS ELM_PHOTO_CLASS

#define MY_CLASS_NAME "Elm_Photo"
#define MY_CLASS_NAME_LEGACY "elm_photo"

static const char SIG_CLICKED[] = "clicked"; /**< Signal emitted when photo is clicked */
static const char SIG_DRAG_START[] = "drag,start"; /**< Signal emitted when dragging the inner image starts */
static const char SIG_DRAG_END[] = "drag,end"; /**< Signal emitted when the dragged image is dropped */

/**< Smart Callbacks descriptions for Elm_Photo */
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CLICKED, ""},
   {SIG_DRAG_START, ""},
   {SIG_DRAG_END, ""},
   {NULL, NULL}
};

/**
 * @internal
 * @brief Recalculates and applies sizing for the photo widget.
 *
 * This function is called when the photo's size, scale, or theme changes.
 * It sets the minimum and maximum size hints for the widget based on the
 * configured photo size and finger size adjustments.
 *
 * @param obj The Evas_Object (Elm_Photo) to evaluate sizing for.
 */
static void
_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = 0, minh = 0, maxw = -1, maxh = -1;
   double scale;

   ELM_PHOTO_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->size <= 0) return;

   scale = (sd->size * efl_gfx_entity_scale_get(obj) * elm_config_scale_get());

   evas_object_size_hint_min_set(sd->icon, scale, scale);
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc
     (wd->resize_obj, &minw, &minh, minw, minh);
   maxw = minw;
   maxh = minh;
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

/**
 * @internal
 * @brief Applies the theme to the Elm_Photo widget.
 *
 * This EOLIAN function is called when the widget's theme needs to be (re)applied.
 * It sets the theme for the base widget and the internal icon, handles mirroring,
 * and triggers a sizing evaluation.
 *
 * @param obj The Eo object (Elm_Photo).
 * @param sd The Elm_Photo_Data private data.
 * @return Eina_Error indicating success or failure.
 */
EOLIAN static Eina_Error
_elm_photo_efl_ui_widget_theme_apply(Eo *obj, Elm_Photo_Data *sd)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   edje_object_mirrored_set
     (wd->resize_obj, efl_ui_mirrored_get(obj));

   elm_widget_theme_object_set
     (obj, wd->resize_obj, "photo", "base",
     elm_widget_style_get(obj));

   elm_object_scale_set(sd->icon, efl_gfx_entity_scale_get(obj));

   edje_object_scale_set(wd->resize_obj,
                         efl_gfx_entity_scale_get(obj) * elm_config_scale_get());
   _sizing_eval(obj);

   return int_ret;
}

/**
 * @internal
 * @brief Sets whether the photo can be a drag target.
 * (Currently a stub, Elm_Photo is not a drop target by default via this Efl_Ui_Draggable interface).
 *
 * @param obj The Eo object (Elm_Photo).
 * @param pd The Elm_Photo_Data private data.
 * @param set EINA_TRUE to enable as drag target, EINA_FALSE otherwise.
 */
EOLIAN static void
_elm_photo_efl_ui_draggable_drag_target_set(Eo *obj EINA_UNUSED,
                                            Elm_Photo_Data *pd EINA_UNUSED,
                                            Eina_Bool set EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Gets whether the photo can be a drag target.
 * (Currently a stub, Elm_Photo is not a drop target by default via this Efl_Ui_Draggable interface).
 *
 * @param obj The Eo object (Elm_Photo).
 * @param pd The Elm_Photo_Data private data.
 * @return EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_photo_efl_ui_draggable_drag_target_get(const Eo *obj EINA_UNUSED,
                                            Elm_Photo_Data *pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Callback for move or resize events on the internal icon.
 *
 * If `fill_inside` is enabled, this function sends a message to the Edje
 * theme object with the new dimensions of the image. It also re-applies
 * the thumbnail if one is set.
 *
 * @param data The Elm_Photo Evas_Object.
 * @param e The Evas canvas.
 * @param obj The Evas_Object that triggered the event (the internal icon's image).
 * @param event_info Event-specific information (unused).
 */
static void
_icon_move_resize_cb(void *data,
                     Evas *e EINA_UNUSED,
                     Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   Evas_Coord w, h;

   ELM_PHOTO_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->fill_inside)
     {
        Edje_Message_Int_Set *msg;
        Evas_Object *img = elm_image_object_get(sd->icon);

        evas_object_geometry_get(img, NULL, NULL, &w, &h);
        msg = alloca(sizeof(Edje_Message_Int_Set) + (sizeof(int)));
        msg->count = 2;
        msg->val[0] = (int)w;
        msg->val[1] = (int)h;

        edje_object_message_send
          (wd->resize_obj, EDJE_MESSAGE_INT_SET, 0, msg);
     }

   if (sd->thumb.file.path)
     elm_icon_thumb_set(sd->icon, sd->thumb.file.path, sd->thumb.file.key);
}

/**
 * @internal
 * @brief Callback invoked when a drag operation initiated by the photo is completed.
 *
 * This function unfreezes scrolling on the object and emits the "drag,end"
 * smart callback. It also resets the `drag_started` flag.
 *
 * @param unused Unused data pointer.
 * @param obj The Elm_Photo Evas_Object.
 */
static void
_drag_done_cb(void *unused EINA_UNUSED,
              Evas_Object *obj)
{
   ELM_PHOTO_DATA_GET(obj, sd);

   elm_object_scroll_freeze_pop(obj);
   evas_object_smart_callback_call(obj, "drag,end", NULL);
   sd->drag_started = EINA_FALSE;
}

/**
 * @internal
 * @brief Callback for mouse move events on the icon, used for long press detection.
 *
 * If a long press timer is active, this function checks if the mouse has
 * moved beyond a certain threshold or if the event is on hold. If so,
 * it cancels the long press timer.
 *
 * @param data The Elm_Photo Evas_Object.
 * @param e The Evas canvas.
 * @param icon The Evas_Object that triggered the event (the internal icon).
 * @param event The Evas_Event_Mouse_Move event information.
 */
static void
_mouse_move(void *data,
            Evas *e EINA_UNUSED,
            Evas_Object *icon,
            void *event)
{
   Evas_Event_Mouse_Move *move = event;

   ELM_PHOTO_DATA_GET(data, sd);

   /* Sanity */
   if (!sd->long_press_timer)
     {
        evas_object_event_callback_del
          (icon, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move);
        return;
     }

   /* if the event is held, stop waiting */
   if (move->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        /* Moved too far: No longpress for you! */
        ELM_SAFE_FREE(sd->long_press_timer, ecore_timer_del);
        evas_object_event_callback_del
          (icon, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move);
     }
}

/**
 * @internal
 * @brief Callback for the long press timer.
 *
 * This function is triggered when the long press timeout is reached.
 * It initiates a drag operation for the photo's image if a file is set.
 *
 * @param obj The Elm_Photo Evas_Object.
 * @return EINA_FALSE to ensure the timer does not run again.
 */
static Eina_Bool
_long_press_cb(void *obj)
{
   Evas_Object *img;
   const char *file;
   char *sfile;

   ELM_PHOTO_DATA_GET(obj, sd);

   DBG("Long press: start drag!");
   sd->long_press_timer = NULL; /* clear: must return NULL now */
   evas_object_event_callback_del
     (sd->icon, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move);

   img = elm_image_object_get(sd->icon);
   file = NULL;
   evas_object_image_file_get(img, &file, NULL);
   if (file)
     {
        char buf[4096 + 7];

        sfile = eina_file_path_sanitize(file);
        snprintf(buf, sizeof(buf), "file://%s", sfile);
        free(sfile);
        if (elm_drag_start
              (obj, ELM_SEL_FORMAT_IMAGE, buf, ELM_XDND_ACTION_MOVE,
                  NULL, NULL,
                  NULL, NULL,
                  NULL, NULL,
                  _drag_done_cb, NULL))
          {
             elm_object_scroll_freeze_push(obj);
             evas_object_smart_callback_call
               (obj, "drag,start", NULL);
             sd->drag_started = EINA_TRUE;
          }
     }

   return EINA_FALSE; /* Don't call again */
}

/**
 * @internal
 * @brief Callback for mouse down events on the icon.
 *
 * This function starts a long press timer if the primary mouse button (button 1)
 * is pressed. It also registers a mouse move callback to detect if the
 * pointer moves significantly during the long press period.
 *
 * @param data The Elm_Photo Evas_Object.
 * @param e The Evas canvas.
 * @param icon The Evas_Object that triggered the event (the internal icon).
 * @param event_info The Evas_Event_Mouse_Down event information.
 */
static void
_mouse_down(void *data,
            Evas *e EINA_UNUSED,
            Evas_Object *icon,
            void *event_info EINA_UNUSED)
{
   Evas_Event_Mouse_Down *ev = event_info;

   ELM_PHOTO_DATA_GET(data, sd);

   if (ev->button != 1) return;

   ecore_timer_del(sd->long_press_timer);
   sd->long_press_timer = ecore_timer_add(_elm_config->longpress_timeout,
                                          _long_press_cb, data);
   evas_object_event_callback_add
     (icon, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move, data);
}

/**
 * @internal
 * @brief Callback for mouse up events on the icon.
 *
 * This function cancels any active long press timer. If a drag operation
 * was not started, it emits the "clicked" smart callback.
 *
 * @param data The Elm_Photo Evas_Object.
 * @param e The Evas canvas.
 * @param obj The Evas_Object that triggered the event (the internal icon).
 * @param event_info The Evas_Event_Mouse_Up event information.
 */
static void
_mouse_up(void *data,
          Evas *e EINA_UNUSED,
          Evas_Object *obj EINA_UNUSED,
          void *event_info EINA_UNUSED)
{
   Evas_Event_Mouse_Up *ev = event_info;
   ELM_PHOTO_DATA_GET(data, sd);

   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;

   ELM_SAFE_FREE(sd->long_press_timer, ecore_timer_del);

   if (!sd->drag_started)
     evas_object_smart_callback_call(data, "clicked", NULL);
}

/**
 * @internal
 * @brief Sets up callbacks on the internal image object of the icon.
 *
 * This function retrieves the actual image object from the internal icon
 * and attaches move and resize event callbacks to it (_icon_move_resize_cb).
 * This allows the photo widget to react to changes in the displayed image's
 * geometry, for example, to update Edje messages when `fill_inside` is true.
 *
 * @param obj The Elm_Photo Evas_Object.
 */
static void
_elm_photo_internal_image_follow(Evas_Object *obj)
{
   Evas_Object *img;

   ELM_PHOTO_DATA_GET(obj, sd);

   img = elm_image_object_get(sd->icon);

   evas_object_event_callback_add
     (img, EVAS_CALLBACK_MOVE, _icon_move_resize_cb, obj);
   evas_object_event_callback_add
     (img, EVAS_CALLBACK_RESIZE, _icon_move_resize_cb, obj);
}

/**
 * @internal
 * @brief Callback for the ELM_ICON_EVENT_THUMB_DONE event.
 *
 * This function is called when the icon's thumbnail generation is complete.
 * It then calls _elm_photo_internal_image_follow to set up necessary
 * callbacks on the (newly available) internal image.
 *
 * @param data The Elm_Photo Evas_Object.
 * @param event The Efl_Event details (unused).
 */
static void
_on_thumb_done(void *data, const Efl_Event *event EINA_UNUSED)
{
   _elm_photo_internal_image_follow(data);
}

/**
 * @internal
 * @brief EOLIAN function called when the Elm_Photo object is added to a canvas group.
 *
 * This function performs initialization for the photo widget. It creates
 * the internal icon, sets up mouse event callbacks for click and drag detection,
 * initializes the theme, and sets the initial sizing.
 *
 * @param obj The Eo object (Elm_Photo).
 * @param priv The Elm_Photo_Data private data.
 */
EOLIAN static void
_elm_photo_efl_canvas_group_group_add(Eo *obj, Elm_Photo_Data *priv)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   elm_widget_can_focus_set(obj, EINA_FALSE);

   priv->icon = elm_icon_add(obj);
   evas_object_repeat_events_set(priv->icon, EINA_TRUE);

   elm_image_resizable_set(priv->icon, EINA_TRUE, EINA_TRUE);
   elm_image_smooth_set(priv->icon, EINA_TRUE);
   elm_image_fill_outside_set(priv->icon, !priv->fill_inside);
   elm_image_prescale_set(priv->icon, 0);

   elm_object_scale_set(priv->icon, efl_gfx_entity_scale_get(obj));

   evas_object_event_callback_add
     (priv->icon, EVAS_CALLBACK_MOUSE_UP, _mouse_up, obj);
   evas_object_event_callback_add
     (priv->icon, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, obj);

   efl_event_callback_add
     (priv->icon, ELM_ICON_EVENT_THUMB_DONE, _on_thumb_done, obj);

   _elm_photo_internal_image_follow(obj);

   _sizing_eval(obj);

   elm_widget_resize_object_set(obj, edje_object_add(evas_object_evas_get(obj)));

   elm_widget_theme_object_set
     (obj, wd->resize_obj, "photo", "base", "default");

   edje_object_part_swallow
     (wd->resize_obj, "elm.swallow.content", priv->icon);

   elm_photo_file_set(obj, NULL);
}

/**
 * @internal
 * @brief EOLIAN function called when the Elm_Photo object is being deleted from a canvas group.
 *
 * This function performs cleanup, primarily deleting any active long press timer.
 *
 * @param obj The Eo object (Elm_Photo).
 * @param sd The Elm_Photo_Data private data.
 */
EOLIAN static void
_elm_photo_efl_canvas_group_group_del(Eo *obj, Elm_Photo_Data *sd)
{
   ecore_timer_del(sd->long_press_timer);

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @brief Adds a new photo widget to the given parent Evas_Object.
 * @param parent The parent object.
 * @return The new photo object, or @c NULL on errors.
 * @ingroup Elm_Photo
 */
EAPI Evas_Object *
elm_photo_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief EOLIAN function called when the Elm_Photo object is finalized.
 *
 * This function finalizes the object creation. If a file or mmap was set on
 * the internal icon, it triggers a load operation.
 *
 * @param obj The Eo object (Elm_Photo).
 * @param sd The Elm_Photo_Data private data.
 * @return The finalized Eo object, or NULL on failure.
 */
EOLIAN static Eo *
_elm_photo_efl_object_finalize(Eo *obj, Elm_Photo_Data *sd)
{
   obj = efl_finalize(efl_super(obj, MY_CLASS));
   if (!obj) return NULL;
   if (efl_file_get(sd->icon) || efl_file_mmap_get(sd->icon))
     efl_file_load(sd->icon);

   return obj;
}

/**
 * @internal
 * @brief EOLIAN constructor for the Elm_Photo object.
 *
 * This function is called during object construction. It sets the legacy
 * class name, registers smart callback descriptions, and sets the
 * accessibility role.
 *
 * @param obj The Eo object (Elm_Photo).
 * @param _pd The Elm_Photo_Data private data (unused in this function).
 * @return The constructed Eo object.
 */
EOLIAN static Eo *
_elm_photo_efl_object_constructor(Eo *obj, Elm_Photo_Data *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_IMAGE);

   return obj;
}

/**
 * @internal
 * @brief EOLIAN implementation for Efl_File.unload.
 *
 * Unloads the current image from the photo, reverting to the "no_photo"
 * standard icon and re-evaluating sizing.
 *
 * @param obj The Eo object (Elm_Photo).
 * @param sd The Elm_Photo_Data private data.
 */
EOLIAN static void
_elm_photo_efl_file_unload(Eo *obj, Elm_Photo_Data *sd)
{
   elm_icon_standard_set(sd->icon, "no_photo");
   _sizing_eval(obj);
}

/**
 * @internal
 * @brief EOLIAN implementation for Efl_File.load.
 *
 * Loads the image file specified by efl_file_get() into the photo.
 * If no file is set, it attempts to set the "no_photo" standard icon.
 * Triggers a sizing evaluation after loading.
 *
 * @param obj The Eo object (Elm_Photo).
 * @param sd The Elm_Photo_Data private data.
 * @return Eina_Error indicating success or failure of the load operation.
 */
EOLIAN static Eina_Error
_elm_photo_efl_file_load(Eo *obj, Elm_Photo_Data *sd)
{
   const char *file = efl_file_get(sd->icon);
   Eina_Error err = 0;
   if (!file)
     {
        if (!elm_icon_standard_set(sd->icon, "no_photo")) return EINA_FALSE;
     }
   else
     {
        if (efl_file_loaded_get(obj)) return 0;
        err = efl_file_load(sd->icon);
        if (err) return err;
     }

   _sizing_eval(obj);

   return 0;
}

/**
 * @internal
 * @brief EOLIAN implementation for Efl_File.mmap_get.
 * Delegates to the internal icon's efl_file_mmap_get.
 */
EOLIAN static const Eina_File *
_elm_photo_efl_file_mmap_get(const Eo *obj EINA_UNUSED, Elm_Photo_Data *sd)
{
   return efl_file_mmap_get(sd->icon);
}

/**
 * @internal
 * @brief EOLIAN implementation for Efl_File.mmap_set.
 * Delegates to the internal icon's efl_file_mmap_set.
 */
EOLIAN static Eina_Error
_elm_photo_efl_file_mmap_set(Eo *obj EINA_UNUSED, Elm_Photo_Data *sd, const Eina_File *file)
{
   return efl_file_mmap_set(sd->icon, file);
}

/**
 * @internal
 * @brief EOLIAN implementation for Efl_File.file_set.
 * Delegates to the internal icon's efl_file_set.
 */
EOLIAN static Eina_Error
_elm_photo_efl_file_file_set(Eo *obj EINA_UNUSED, Elm_Photo_Data *sd, const char *file)
{
   return efl_file_set(sd->icon, file);
}

/**
 * @internal
 * @brief EOLIAN implementation for Efl_File.file_get.
 * Delegates to the internal icon's efl_file_get.
 */
EOLIAN static const char *
_elm_photo_efl_file_file_get(const Eo *obj EINA_UNUSED, Elm_Photo_Data *sd)
{
   return efl_file_get(sd->icon);
}

/**
 * @internal
 * @brief EOLIAN implementation for Efl_File.key_set.
 * Delegates to the internal icon's efl_file_key_set.
 */
EOLIAN static void
_elm_photo_efl_file_key_set(Eo *obj EINA_UNUSED, Elm_Photo_Data *sd, const char *key)
{
   return efl_file_key_set(sd->icon, key);
}

/**
 * @internal
 * @brief EOLIAN implementation for Efl_File.key_get.
 * Delegates to the internal icon's efl_file_key_get.
 */
EOLIAN static const char *
_elm_photo_efl_file_key_get(const Eo *obj EINA_UNUSED, Elm_Photo_Data *sd)
{
   return efl_file_key_get(sd->icon);
}

/**
 * @internal
 * @brief Class constructor for Elm_Photo.
 *
 * Registers the legacy type name for the Elm_Photo class.
 *
 * @param klass The Efl_Class to construct.
 */
static void
_elm_photo_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @brief Set the file to be shown in the photo widget.
 *
 * @param obj The photo object.
 * @param file The path to the image file.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 *
 * @deprecated Use efl_file_set() instead.
 * @ingroup Elm_Photo
 */
EAPI Eina_Bool
elm_photo_file_set(Eo *obj, const char *file)
{
   return efl_file_simple_load((Eo *) obj, file, NULL);
}

/* Legacy deprecated functions */

/**
 * @brief Set the edje group to be used for the photo frame when in editable mode.
 *
 * @param obj The photo object.
 * @param edit @c EINA_TRUE to set editable, @c EINA_FALSE otherwise.
 *
 * @deprecated This function is deprecated as edit mode is handled by elm_image.
 * @ingroup Elm_Photo
 */
EAPI void
elm_photo_editable_set(Evas_Object *obj, Eina_Bool edit)
{
   ELM_PHOTO_CHECK(obj);
   ELM_PHOTO_DATA_GET(obj, sd);
   elm_image_editable_set(sd->icon, edit);
}

/**
 * @brief Get the editable state of the photo.
 *
 * @param obj The photo object.
 * @return @c EINA_TRUE if editable, @c EINA_FALSE otherwise.
 *
 * @deprecated This function is deprecated.
 * @ingroup Elm_Photo
 */
EAPI Eina_Bool
elm_photo_editable_get(const Evas_Object *obj)
{
   ELM_PHOTO_CHECK(obj) EINA_FALSE;
   ELM_PHOTO_DATA_GET(obj, sd);
   return elm_image_editable_get(sd->icon);
}

/**
 * @brief Set the size of the photo.
 *
 * This is the size of the inner icon, not the entire widget.
 *
 * @param obj The photo object.
 * @param size The size (width and height) to set. Must be greater than 0.
 * @ingroup Elm_Photo
 */
EAPI void
elm_photo_size_set(Evas_Object *obj, int size)
{
   ELM_PHOTO_CHECK(obj);
   ELM_PHOTO_DATA_GET(obj, sd);
   sd->size = (size > 0) ? size : 0;

   elm_image_prescale_set(sd->icon, sd->size);

   _sizing_eval(obj);
}

/**
 * @brief Get the size of the photo.
 *
 * @param obj The photo object.
 * @return The size of the photo.
 * @ingroup Elm_Photo
 */
EAPI int
elm_photo_size_get(const Evas_Object *obj)
{
   ELM_PHOTO_CHECK(obj) 0;
   ELM_PHOTO_DATA_GET(obj, sd);
   return sd->size;
}

/**
 * @brief Set whether the original photo should be fit to photo widget's area.
 *
 * When @p fill is @c EINA_FALSE, the photo will be scaled to fit
 * within the object's bounds without cropping. When @p fill is
 * @c EINA_TRUE, the photo will be scaled to fill the object's bounds,
 * potentially cropping parts of the image.
 *
 * @param obj The photo object.
 * @param fill @c EINA_TRUE to fill the photo widget's area,
 *             @c EINA_FALSE to fit into it.
 * @ingroup Elm_Photo
 */
EAPI void
elm_photo_fill_inside_set(Evas_Object *obj, Eina_Bool fill)
{
   ELM_PHOTO_CHECK(obj);
   ELM_PHOTO_DATA_GET(obj, sd);
   elm_image_fill_outside_set(sd->icon, !fill);
   sd->fill_inside = !!fill;

   _sizing_eval(obj);
}

/**
 * @brief Get whether the original photo should be fit to photo widget's area.
 *
 * @param obj The photo object.
 * @return @c EINA_TRUE if the photo is set to fill the widget's area,
 *         @c EINA_FALSE otherwise.
 * @ingroup Elm_Photo
 */
EAPI Eina_Bool
elm_photo_fill_inside_get(const Evas_Object *obj)
{
   ELM_PHOTO_CHECK(obj) EINA_FALSE;
   ELM_PHOTO_DATA_GET(obj, sd);
   return sd->fill_inside;
}

/**
 * @brief Set whether the photo widget should keep its aspect ratio.
 *
 * @param obj The photo object.
 * @param fixed @c EINA_TRUE to keep aspect ratio, @c EINA_FALSE otherwise.
 * @ingroup Elm_Photo
 */
EAPI void
elm_photo_aspect_fixed_set(Evas_Object *obj, Eina_Bool fixed)
{
   ELM_PHOTO_CHECK(obj);
   ELM_PHOTO_DATA_GET(obj, sd);
   elm_image_aspect_fixed_set(sd->icon, fixed);
}

/**
 * @brief Get whether the photo widget should keep its aspect ratio.
 *
 * @param obj The photo object.
 * @return @c EINA_TRUE if aspect ratio is fixed, @c EINA_FALSE otherwise.
 * @ingroup Elm_Photo
 */
EAPI Eina_Bool
elm_photo_aspect_fixed_get(const Evas_Object *obj)
{
   ELM_PHOTO_CHECK(obj) EINA_FALSE;
   ELM_PHOTO_DATA_GET(obj, sd);
   return elm_image_aspect_fixed_get(sd->icon);
}

/**
 * @brief Set the file that will be used as a thumbnail for the photo.
 *
 * This function sets a thumbnail for the photo widget using the Evas
 * Eet_File (or Edje_File) and group key provided. This is typically
 * used for faster loading of a preview before the full image is decoded.
 *
 * @param obj The photo object.
 * @param file The path to the EET/EDJ file containing the thumbnail.
 * @param group The key or group within the file for the thumbnail image.
 * @ingroup Elm_Photo
 */
EAPI void
elm_photo_thumb_set(Evas_Object *obj, const char *file, const char *group)
{
   ELM_PHOTO_CHECK(obj);
   ELM_PHOTO_DATA_GET(obj, sd);
   eina_stringshare_replace(&sd->thumb.file.path, file);
   eina_stringshare_replace(&sd->thumb.file.key, group);

   elm_icon_thumb_set(sd->icon, file, group);
}

/* Internal EO APIs and hidden overrides */

#define ELM_PHOTO_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_photo) /**< Macro defining extra Eolian operations for Elm_Photo */

#include "elm_photo_eo.c"

/**
 * @}
 */
