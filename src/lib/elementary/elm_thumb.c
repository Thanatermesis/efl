#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_thumb_eo.h"
#include "elm_widget_thumb.h"

#define MY_CLASS_NAME "Elm_Thumb"
#define MY_CLASS_NAME_LEGACY "elm_thumb"

#define MY_CLASS ELM_THUMB_CLASS

/** @internal @brief Signal emitted when the thumbnail is clicked. */
static const char SIG_CLICKED[] = "clicked";
/** @internal @brief Signal emitted when the thumbnail is double-clicked. */
static const char SIG_CLICKED_DOUBLE[] = "clicked,double";
/** @internal @brief Signal emitted when thumbnail generation fails. */
static const char SIG_GENERATE_ERROR[] = "generate,error";
/** @internal @brief Signal emitted when thumbnail generation starts. */
static const char SIG_GENERATE_START[] = "generate,start";
/** @internal @brief Signal emitted when thumbnail generation stops. */
static const char SIG_GENERATE_STOP[] = "generate,stop";
/** @internal @brief Signal emitted when loading the thumbnail image fails. */
static const char SIG_LOAD_ERROR[] = "load,error";
/** @internal @brief Signal emitted when the thumbnail is pressed. */
static const char SIG_PRESS[] = "press";

/**
 * @internal
 * @brief Descriptions for the smart callbacks supported by the Elm_Thumb widget.
 * These are used to register callbacks that users of the widget can subscribe to.
 */
static const Evas_Smart_Cb_Description _smart_callbacks[] =
{
   {SIG_CLICKED, ""},
   {SIG_CLICKED_DOUBLE, ""},
   {SIG_GENERATE_ERROR, ""},
   {SIG_GENERATE_START, ""},
   {SIG_GENERATE_STOP, ""},
   {SIG_LOAD_ERROR, ""},
   {SIG_PRESS, ""},
   {NULL, NULL}
};

#define EDJE_SIGNAL_GENERATE_START "elm,thumb,generate,start"
#define EDJE_SIGNAL_GENERATE_STOP  "elm,thumb,generate,stop"
#define EDJE_SIGNAL_GENERATE_ERROR "elm,thumb,generate,error"
#define EDJE_SIGNAL_LOAD_ERROR     "elm,thumb,load,error"
#define EDJE_SIGNAL_PULSE_START    "elm,state,pulse,start"
#define EDJE_SIGNAL_PULSE_STOP     "elm,state,pulse,stop"

/** @internal @brief Global Ethumb client instance used by all thumb widgets. */
static struct _Ethumb_Client *_elm_ethumb_client = NULL;
/** @internal @brief Flag indicating if the Ethumb client is currently connected to the server. */
static Eina_Bool _elm_ethumb_connected = EINA_FALSE;

/** @internal @brief List of Elm_Thumb_Data objects for which thumbnail generation/loading needs to be retried. */
static Eina_List *retry = NULL;
/** @internal @brief Counter for the number of currently pending thumbnail generation requests to Ethumb. */
static int pending_request = 0;

/** @internal @brief Ecore event type for Ethumb connection status changes. */
EAPI int ELM_ECORE_EVENT_ETHUMB_CONNECT = 0;

/**
 * @internal
 * @brief Callback function for mouse down events on the thumbnail object.
 *
 * Handles single clicks and initiates press event. It also checks for
 * double clicks, although the "clicked,double" signal is typically
 * emitted by the Evas framework based on a sequence of mouse events.
 *
 * @param data The widget data (Elm_Thumb_Data).
 * @param e Evas canvas.
 * @param obj The Evas object that received the event.
 * @param event_info Specific event information (Evas_Event_Mouse_Down).
 */
static void
_mouse_down_cb(void *data,
               Evas *e EINA_UNUSED,
               Evas_Object *obj,
               void *event_info)
{
   ELM_THUMB_DATA_GET(data, sd);
   Evas_Event_Mouse_Down *ev = event_info;

   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = EINA_TRUE;
   else sd->on_hold = EINA_FALSE;

   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     evas_object_smart_callback_call(obj, "clicked", NULL);
   else
     efl_event_callback_legacy_call(obj, ELM_THUMB_EVENT_PRESS, NULL);
}

/**
 * @internal
 * @brief Callback function for mouse up events on the thumbnail object.
 *
 * Completes a click action if the mouse button was released without
 * being held.
 *
 * @param data The widget data (Elm_Thumb_Data).
 * @param e Evas canvas.
 * @param obj The Evas object that received the event.
 * @param event_info Specific event information (Evas_Event_Mouse_Up).
 */
static void
_mouse_up_cb(void *data,
             Evas *e EINA_UNUSED,
             Evas_Object *obj,
             void *event_info)
{
   ELM_THUMB_DATA_GET(data, sd);
   Evas_Event_Mouse_Up *ev = event_info;

   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = EINA_TRUE;
   else sd->on_hold = EINA_FALSE;

   if (!sd->on_hold)
     evas_object_smart_callback_call(obj, "clicked", NULL);

   sd->on_hold = EINA_FALSE;
}

/**
 * @internal
 * @brief Informs the widget that the thumbnail is ready and updates the UI.
 *
 * This function is called once a thumbnail (either image or Edje file for video)
 * is successfully loaded or generated. It sets the thumbnail to the view object,
 * adjusts its size and aspect ratio, and emits signals to stop animations
 * and indicate completion.
 *
 * @param sd The widget's private data.
 * @param thumb_path The file path of the generated or cached thumbnail.
 * @param thumb_key The key for the thumbnail within an Edje file (if applicable).
 */
static void
_thumb_ready_inform(Elm_Thumb_Data *sd,
                    const char *thumb_path,
                    const char *thumb_key)
{
   Evas_Coord mw, mh;
   Evas_Coord aw, ah;
   ELM_WIDGET_DATA_GET_OR_RETURN(sd->obj, wd);

   if ((sd->is_video) && (sd->thumb.format == ETHUMB_THUMB_EET))
     {
        edje_object_size_min_get(sd->view, &mw, &mh);
        edje_object_size_min_restricted_calc(sd->view, &mw, &mh, mw, mh);
        evas_object_size_hint_min_set(sd->view, mw, mh);
     }
   else
     {
        evas_object_image_size_get(sd->view, &aw, &ah);
        evas_object_size_hint_aspect_set(sd->view, EVAS_ASPECT_CONTROL_BOTH,
                                         aw, ah);
     }

   edje_object_part_swallow(wd->resize_obj, "elm.swallow.content", sd->view);
   eina_stringshare_replace(&(sd->thumb.file), thumb_path);
   eina_stringshare_replace(&(sd->thumb.key), thumb_key);
   edje_object_signal_emit(wd->resize_obj, EDJE_SIGNAL_PULSE_STOP, "elm");
   edje_object_signal_emit(wd->resize_obj, EDJE_SIGNAL_GENERATE_STOP, "elm");
   efl_event_callback_legacy_call(sd->obj, ELM_THUMB_EVENT_GENERATE_STOP, NULL);
}

/**
 * @internal
 * @brief Callback invoked when an Evas image object has preloaded its data.
 *
 * This is used for regular image thumbnails (not Edje-based video thumbs)
 * to know when the image data is ready in memory, after which
 * _thumb_ready_inform() is called.
 *
 * @param data The widget data (Elm_Thumb_Data).
 * @param e Evas canvas.
 * @param obj The Evas image object that was preloaded.
 * @param event_info Not used.
 */
static void
_on_thumb_preloaded(void *data,
                    Evas *e EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   ELM_THUMB_DATA_GET(data, sd);
   const char *thumb_path;
   const char *thumb_key;

   evas_object_image_file_get(sd->view, &thumb_path, &thumb_key);

   _thumb_ready_inform(sd, thumb_path, thumb_key);
}

/* As we do use stat to check if a thumbnail is available, it's
 * possible that we end up accessing it before the file is completely
 * written on disk. By retrying each time a thumbnail is finished we
 * should be fine or not.
 *
 * @param sd The widget's private data for which to retry loading.
 * @return @c EINA_TRUE if the retry was successful or is now pending preload,
 *         @c EINA_FALSE if the retry failed immediately.
 */
static Eina_Bool
_thumb_retry(Elm_Thumb_Data *sd)
{
   int r;

   if ((sd->is_video) && (sd->thumb.format == ETHUMB_THUMB_EET))
     {
        edje_object_file_set(sd->view, NULL, NULL);
        if (!edje_object_file_set
              (sd->view, sd->thumb.thumb_path, "movie/thumb"))
          {
             if (pending_request == 0)
               ERR("could not set file=%s key=%s for %s",
                   sd->thumb.thumb_path,
                   sd->thumb.thumb_key,
                   sd->file);
             goto view_err;
          }
        /* FIXME: Do we want to reload a thumbnail when the file
           changes? I think not at the moment, as this may increase
           the presure on the system a lot when using it in a file
           browser */
     }
   else
     {
        evas_object_image_file_set(sd->view, NULL, NULL);
        evas_object_image_file_set
          (sd->view, sd->thumb.thumb_path, sd->thumb.thumb_key);
        r = evas_object_image_load_error_get(sd->view);
        if (r != EVAS_LOAD_ERROR_NONE)
          {
             if (pending_request == 0)
               ERR("%s: %s", sd->thumb.thumb_path, evas_load_error_str(r));
             goto view_err;
          }

        evas_object_event_callback_add
          (sd->view, EVAS_CALLBACK_IMAGE_PRELOADED, _on_thumb_preloaded, sd->obj);
        evas_object_image_preload(sd->view, EINA_TRUE);

        return EINA_TRUE;
     }

   _thumb_ready_inform(sd, sd->thumb.thumb_path, sd->thumb.thumb_key);

   ELM_SAFE_FREE(sd->thumb.thumb_path, eina_stringshare_del);
   ELM_SAFE_FREE(sd->thumb.thumb_key, eina_stringshare_del);

   return EINA_TRUE;

view_err:
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Finalizes the thumbnail display process.
 *
 * This function is called after a thumbnail has been successfully generated by
 * Ethumb or if it was found in cache and directly loaded. It sets up the
 * Evas object (image or Edje) to display the thumbnail. If loading fails,
 * it may add the thumbnail to a retry list. It also processes the retry list
 * for other thumbnails.
 *
 * @param sd The widget's private data.
 * @param thumb_path Path to the thumbnail file.
 * @param thumb_key Key for the thumbnail (e.g., in an Edje file).
 */
static void
_thumb_finish(Elm_Thumb_Data *sd,
              const char *thumb_path,
              const char *thumb_key)
{
   Eina_List *l, *ll;
   Evas *evas;
   int r;
   ELM_WIDGET_DATA_GET_OR_RETURN(sd->obj, wd);

   evas = evas_object_evas_get(sd->obj);
   if ((sd->view) && (sd->is_video ^ sd->was_video))
     {
        ELM_SAFE_FREE(sd->view, evas_object_del);
     }
   sd->was_video = sd->is_video;

   if ((sd->is_video) &&
       (ethumb_client_format_get(_elm_ethumb_client) == ETHUMB_THUMB_EET))
     {
        if (!sd->view)
          {
             sd->view = edje_object_add(evas);
             if (!sd->view)
               {
                  ERR("could not create edje object");
                  goto err;
               }
          }

        if (!edje_object_file_set(sd->view, thumb_path, thumb_key))
          {
             sd->thumb.thumb_path = eina_stringshare_ref(thumb_path);
             sd->thumb.thumb_key = eina_stringshare_ref(thumb_key);
             sd->thumb.format = ethumb_client_format_get(_elm_ethumb_client);
             sd->thumb.retry = EINA_TRUE;

             retry = eina_list_append(retry, sd);
             efl_data_ref(sd->obj, MY_CLASS);
             return;
          }
     }
   else
     {
        if (!sd->view)
          {
             sd->view = evas_object_image_filled_add(evas);
             if (!sd->view)
               {
                  ERR("could not create image object");
                  goto err;
               }
             evas_object_event_callback_add(sd->view, EVAS_CALLBACK_IMAGE_PRELOADED,
                                            _on_thumb_preloaded, sd->obj);
             evas_object_hide(sd->view);
          }

        evas_object_image_file_set(sd->view, thumb_path, thumb_key);
        r = evas_object_image_load_error_get(sd->view);
        if (r != EVAS_LOAD_ERROR_NONE)
          {
             WRN("%s: %s", thumb_path, evas_load_error_str(r));
             sd->thumb.thumb_path = eina_stringshare_ref(thumb_path);
             sd->thumb.thumb_key = eina_stringshare_ref(thumb_key);
             sd->thumb.format = ethumb_client_format_get(_elm_ethumb_client);
             sd->thumb.retry = EINA_TRUE;

             retry = eina_list_append(retry, sd);
             efl_data_ref(sd->obj, MY_CLASS);
             return;
          }

        evas_object_image_preload(sd->view, EINA_FALSE);
        return;
     }

   _thumb_ready_inform(sd, thumb_path, thumb_key);

   EINA_LIST_FOREACH_SAFE(retry, l, ll, sd)
     {
        if (_thumb_retry(sd))
          {
             retry = eina_list_remove_list(retry, l);
             efl_data_unref(sd->obj, sd);
          }

     }

   if (pending_request == 0)
     EINA_LIST_FREE(retry, sd)
       {
          efl_data_unref(sd->obj, sd);
          ELM_SAFE_FREE(sd->thumb.thumb_path, eina_stringshare_del);
          ELM_SAFE_FREE(sd->thumb.thumb_key, eina_stringshare_del);
          ELM_SAFE_FREE(sd->view, evas_object_del);

          wd = efl_data_scope_get(sd->obj, EFL_UI_WIDGET_CLASS);
          edje_object_signal_emit(wd->resize_obj, EDJE_SIGNAL_LOAD_ERROR, "elm");
          efl_event_callback_legacy_call(sd->obj, ELM_THUMB_EVENT_LOAD_ERROR, NULL);
       }

   return;

err:
   edje_object_signal_emit(wd->resize_obj, EDJE_SIGNAL_LOAD_ERROR, "elm");
   efl_event_callback_legacy_call(sd->obj, ELM_THUMB_EVENT_LOAD_ERROR, NULL);
}

/**
 * @internal
 * @brief Callback function invoked by Ethumb when thumbnail generation is successful.
 *
 * @param client The Ethumb client instance.
 * @param thumb_path The path to the generated thumbnail file.
 * @param thumb_key The key associated with the thumbnail (if any).
 * @param data The widget data (Elm_Thumb_Data) passed during the async request.
 */
static void
_on_ethumb_thumb_done(Ethumb_Client *client EINA_UNUSED,
                      const char *thumb_path,
                      const char *thumb_key,
                      void *data)
{
   ELM_THUMB_DATA_GET(data, sd);

   if (EINA_UNLIKELY(!sd->thumb.request))
     {
        ERR("Something odd happened with a thumbnail request");
        return;
     }

   pending_request--;
   sd->thumb.request = NULL;

   _thumb_finish(sd, thumb_path, thumb_key);
}

/**
 * @internal
 * @brief Callback function invoked by Ethumb when thumbnail generation fails.
 *
 * @param client The Ethumb client instance.
 * @param data The widget data (Elm_Thumb_Data) passed during the async request.
 */
static void
_on_ethumb_thumb_error(Ethumb_Client *client EINA_UNUSED,
                       void *data)
{
   ELM_THUMB_DATA_GET(data, sd);

   if (EINA_UNLIKELY(!sd->thumb.request))
     {
        ERR("Something odd happened with a thumbnail request");
        return;
     }
   evas_object_event_callback_del_full(sd->view, EVAS_CALLBACK_IMAGE_PRELOADED,
                                       _on_thumb_preloaded, sd);

   pending_request--;
   sd->thumb.request = NULL;

   ERR("could not generate thumbnail for %s (key: %s)",
       sd->thumb.file, sd->thumb.key ? sd->thumb.key : "");

   ELM_WIDGET_DATA_GET_OR_RETURN(data, wd);
   edje_object_signal_emit(wd->resize_obj, EDJE_SIGNAL_GENERATE_ERROR, "elm");
   edje_object_signal_emit(wd->resize_obj, EDJE_SIGNAL_PULSE_STOP, "elm");
   efl_event_callback_legacy_call(sd->obj, ELM_THUMB_EVENT_GENERATE_ERROR, NULL);
}

/**
 * @internal
 * @brief Initiates the thumbnail generation process for the given widget data.
 *
 * Configures the Ethumb client with the desired thumbnail parameters (aspect,
 * size, format, etc.) and then requests the thumbnail generation asynchronously.
 * Emits signals indicating the start of generation.
 *
 * @param sd The widget's private data.
 */
static void
_thumb_start(Elm_Thumb_Data *sd)
{
   if (sd->thumb.aspect)
     ethumb_client_aspect_set(_elm_ethumb_client, sd->thumb.aspect);
   if (sd->thumb.size)
     ethumb_client_fdo_set(_elm_ethumb_client, sd->thumb.size);
   if (sd->thumb.format)
     ethumb_client_format_set(_elm_ethumb_client, sd->thumb.format);
   if (sd->thumb.orient)
     ethumb_client_orientation_set(_elm_ethumb_client, sd->thumb.orient);
   if (sd->thumb.tw && sd->thumb.th)
     ethumb_client_size_set(_elm_ethumb_client, sd->thumb.tw, sd->thumb.th);
   if (!EINA_DBL_EQ(sd->thumb.cropx, 0) && !EINA_DBL_EQ(sd->thumb.cropy, 0))
     ethumb_client_crop_align_set(_elm_ethumb_client, sd->thumb.cropx, sd->thumb.cropy);
   if (sd->thumb.quality)
     ethumb_client_quality_set(_elm_ethumb_client, sd->thumb.quality);
   if (sd->thumb.compress)
     ethumb_client_compress_set(_elm_ethumb_client, sd->thumb.compress);
   if (sd->thumb.request)
     {
        ethumb_client_thumb_async_cancel(_elm_ethumb_client, sd->thumb.request);
        sd->thumb.request = NULL;
     }
   if (sd->thumb.retry)
     {
        retry = eina_list_remove(retry, sd);
        efl_data_unref(sd->obj, sd);
        sd->thumb.retry = EINA_FALSE;
     }

   if (!sd->file) return;

   ELM_WIDGET_DATA_GET_OR_RETURN(sd->obj, wd);
   edje_object_signal_emit(wd->resize_obj, EDJE_SIGNAL_PULSE_START, "elm");
   edje_object_signal_emit(wd->resize_obj, EDJE_SIGNAL_GENERATE_START, "elm");
   efl_event_callback_legacy_call(sd->obj, ELM_THUMB_EVENT_GENERATE_START, NULL);

   pending_request++;
   ethumb_client_file_set(_elm_ethumb_client, sd->file, sd->key);
   sd->thumb.request = ethumb_client_thumb_async_get(_elm_ethumb_client,
                                                     _on_ethumb_thumb_done,
                                                     _on_ethumb_thumb_error,
                                                     sd->obj);
}

/**
 * @internal
 * @brief Ecore event handler callback for ELM_ECORE_EVENT_ETHUMB_CONNECT.
 *
 * This function is called when the Ethumb client successfully connects to the
 * Ethumb server. It then calls _thumb_start() to begin processing the
 * thumbnail request for the associated widget.
 *
 * @param data The widget data (Elm_Thumb_Data).
 * @param type The type of the Ecore event (unused).
 * @param ev The event specific information (unused).
 * @return ECORE_CALLBACK_RENEW to keep the handler, or ECORE_CALLBACK_CANCEL to remove.
 *         Currently, it always renews.
 */
static Eina_Bool
_thumbnailing_available_cb(void *data,
                           int type EINA_UNUSED,
                           void *ev EINA_UNUSED)
{
   ELM_THUMB_DATA_GET(data, sd);
   _thumb_start(sd);

   return ECORE_CALLBACK_RENEW;
}

/** @internal @brief Flag indicating if Ethumb client is needed (initialized). */
static Eina_Bool _elm_need_ethumb = EINA_FALSE;
/** @internal @brief Forward declaration for _on_die_cb. */
static void _on_die_cb(void *, Ethumb_Client *);

/**
 * @internal
 * @brief Callback function invoked when an Ethumb client connection attempt finishes.
 *
 * If successful, it sets a callback for server death notifications and signals
 * that the Ethumb service is connected. If unsuccessful, it clears the global
 * client pointer.
 *
 * @param data User data passed to ethumb_client_connect() (unused here).
 * @param c The Ethumb_Client instance.
 * @param success EINA_TRUE if connection was successful, EINA_FALSE otherwise.
 */
static void
_connect_cb(void *data EINA_UNUSED,
            Ethumb_Client *c,
            Eina_Bool success)
{
   if (success)
     {
        ethumb_client_on_server_die_callback_set(c, _on_die_cb, NULL, NULL);
        _elm_ethumb_connected = EINA_TRUE;
        ecore_event_add(ELM_ECORE_EVENT_ETHUMB_CONNECT, NULL, NULL, NULL);
     }
   else
     _elm_ethumb_client = NULL;
}

/**
 * @internal
 * @brief Callback function invoked when the Ethumb server daemon dies or disconnects.
 *
 * It cleans up the existing client connection and attempts to reconnect if there
 * are pending thumbnail requests.
 *
 * @param data User data associated with the callback (unused here).
 * @param c The Ethumb_Client instance that detected the server death (unused here).
 */
static void
_on_die_cb(void *data EINA_UNUSED,
           Ethumb_Client *c EINA_UNUSED)
{
   if (_elm_ethumb_client)
     {
        ethumb_client_disconnect(_elm_ethumb_client);
        _elm_ethumb_client = NULL;
     }
   _elm_ethumb_connected = EINA_FALSE;
   if (pending_request > 0)
     _elm_ethumb_client = ethumb_client_connect(_connect_cb, NULL, NULL);
}

/**
 * @internal
 * @brief Makes the thumbnail visible and initiates the thumbnail generation/loading process.
 *
 * This function is typically called when the widget becomes visible or when a new
 * file is set. It ensures an Ethumb client is connected (or attempts to connect)
 * and then either starts the thumbnail generation process or registers an event
 * handler to do so once the connection is established.
 *
 * @param sd The widget's private data.
 */
static void
_thumb_show(Elm_Thumb_Data *sd)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(sd->obj, wd);
   evas_object_show(wd->resize_obj);

   if (!_elm_ethumb_client)
     _elm_ethumb_client = ethumb_client_connect(_connect_cb, NULL, NULL);
   else if (elm_thumb_ethumb_client_connected_get())
     {
        _thumb_start(sd);
        return;
     }

   if (!sd->eeh)
     sd->eeh = ecore_event_handler_add(ELM_ECORE_EVENT_ETHUMB_CONNECT,
                                       _thumbnailing_available_cb, sd->obj);
}

/**
 * @internal @brief Eolian implementation for efl_gfx_entity_visible_set.
 *
 * Handles visibility changes of the thumbnail widget. If the widget becomes
 * visible, it triggers the thumbnail loading/generation process via _thumb_show().
 * If it becomes hidden, it cancels any ongoing thumbnail requests and cleans up
 * related resources like retry entries or event handlers.
 *
 * @param obj The Evas object.
 * @param sd The widget's private data.
 * @param vis EINA_TRUE if the object is to be shown, EINA_FALSE if hidden.
 */
EOLIAN static void
_elm_thumb_efl_gfx_entity_visible_set(Eo *obj, Elm_Thumb_Data *sd, Eina_Bool vis)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_VISIBLE, 0, vis))
     return;

   efl_gfx_entity_visible_set(efl_super(obj, MY_CLASS), vis);

   if (vis)
     {
        _thumb_show(sd);
        return;
     }

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->thumb.request)
     {
        ethumb_client_thumb_async_cancel(_elm_ethumb_client, sd->thumb.request);
        sd->thumb.request = NULL;

        edje_object_signal_emit(wd->resize_obj, EDJE_SIGNAL_GENERATE_STOP, "elm");
        efl_event_callback_legacy_call(sd->obj, ELM_THUMB_EVENT_GENERATE_STOP, NULL);
     }

   if (sd->thumb.retry)
     {
        retry = eina_list_remove(retry, sd);
        efl_data_unref(sd->obj, sd);
        sd->thumb.retry = EINA_FALSE;
     }

   ELM_SAFE_FREE(sd->eeh, ecore_event_handler_del);
}

/**
 * @internal
 * @brief Disconnects from Ethumb and shuts down the client library.
 *
 * This function is called when the Ethumb client is no longer needed,
 * typically during application shutdown or when Elementary itself is shut down.
 * It ensures that the connection to the Ethumb service is closed and
 * Ethumb client resources are released.
 */
void
_elm_unneed_ethumb(void)
{
   if (!_elm_need_ethumb) return;
   _elm_need_ethumb = EINA_FALSE;

   if (_elm_ethumb_client)
     {
        ethumb_client_disconnect(_elm_ethumb_client);
        _elm_ethumb_client = NULL;
     }
   ethumb_client_shutdown();

   ecore_event_type_flush(ELM_ECORE_EVENT_ETHUMB_CONNECT);
}

/**
 * @internal
 * @brief Callback for drag and drop operations onto the thumbnail.
 *
 * When a file (typically an image path) is dropped onto the thumbnail widget,
 * this function is called. It sets the dropped file path as the new source
 * for the thumbnail.
 *
 * @param data User data associated with the drop target (the thumb Evas_Object itself).
 * @param o The Evas_Object that is the drop target (the thumb widget).
 * @param drop Elm_Selection_Data containing information about the dropped item.
 *             `drop->data` is expected to be a string (file path).
 * @return EINA_TRUE if the drop was handled, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_thumb_dnd_cb(void *data EINA_UNUSED,
                  Evas_Object *o,
                  Elm_Selection_Data *drop)
{
   if ((!o) || (!drop) || (!drop->data)) return EINA_FALSE;
   elm_thumb_file_set(o, drop->data, NULL);
   return EINA_TRUE;
}

/**
 * @brief Initializes the Ethumb client connection if not already done.
 *
 * This function ensures that the Ethumb client library is initialized and
 * an Ecore event type for Ethumb connection events is created. It's typically
 * called by applications or other Elementary components that require
 * thumbnailing capabilities.
 *
 * @return EINA_TRUE if Ethumb is now initialized (or was already),
 *         EINA_FALSE on failure to initialize.
 * @ingroup Elm_Thumb
 */
EAPI Eina_Bool
elm_need_ethumb(void)
{
   if (_elm_need_ethumb) return EINA_TRUE;
   _elm_need_ethumb = EINA_TRUE;

   ELM_ECORE_EVENT_ETHUMB_CONNECT = ecore_event_type_new();
   ethumb_client_init();

   return EINA_TRUE;
}

/**
 * @internal @brief Eolian implementation for efl_canvas_group_group_add.
 *
 * Called when the thumbnail widget is added to a parent Evas object.
 * It sets the default theme, and registers mouse event callbacks.
 *
 * @param obj The Evas object.
 * @param _pd The widget's private data (unused in this function).
 */
EOLIAN static void
_elm_thumb_efl_canvas_group_group_add(Eo *obj, Elm_Thumb_Data *_pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   if (!elm_layout_theme_set(obj, "thumb", "base", elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN,
                                  _mouse_down_cb, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_UP,
                                  _mouse_up_cb, obj);

   elm_widget_can_focus_set(obj, EINA_FALSE);
}

/**
 * @internal @brief Eolian implementation for efl_canvas_group_group_del.
 *
 * Called when the thumbnail widget is being deleted. It performs cleanup
 * operations such as canceling any pending thumbnail requests, removing
 * items from the retry list, deleting internal Evas objects (like the view),
 * and freeing stringshared resources.
 *
 * @param obj The Evas object.
 * @param sd The widget's private data.
 */
EOLIAN static void
_elm_thumb_efl_canvas_group_group_del(Eo *obj, Elm_Thumb_Data *sd)
{
   if (sd->thumb.request)
     {
        ethumb_client_thumb_async_cancel(_elm_ethumb_client, sd->thumb.request);
        sd->thumb.request = NULL;
     }
   if (sd->thumb.retry)
     {
        retry = eina_list_remove(retry, sd);
        efl_data_unref(sd->obj, sd);
        sd->thumb.retry = EINA_FALSE;
     }
   evas_object_event_callback_del_full(sd->view, EVAS_CALLBACK_IMAGE_PRELOADED,
                                       _on_thumb_preloaded, sd);

   ELM_SAFE_FREE(sd->view, evas_object_del);
   eina_stringshare_del(sd->thumb.thumb_path);
   eina_stringshare_del(sd->thumb.thumb_key);

   eina_stringshare_del(sd->file);
   eina_stringshare_del(sd->key);

   ecore_event_handler_del(sd->eeh);

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

EAPI Evas_Object *
elm_thumb_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal @brief Eolian implementation for efl_object_finalize.
 *
 * Finalizes the object's construction. If a file was set during construction,
 * this triggers the initial load.
 *
 * @param obj The Evas object.
 * @param sd The widget's private data.
 * @return The finalized Evas object.
 */
EOLIAN static Eo *
_elm_thumb_efl_object_finalize(Eo *obj, Elm_Thumb_Data *sd)
{
   obj = efl_finalize(efl_super(obj, MY_CLASS));
   if (sd->file) efl_file_load(obj);
   return obj;
}

/**
 * @internal @brief Eolian implementation for efl_object_constructor.
 *
 * Basic setup for the thumb object during construction. Sets the object type,
 * smart callback descriptions, and accessibility role.
 *
 * @param obj The Evas object.
 * @param sd The widget's private data.
 * @return The constructed Evas object.
 */
EOLIAN static Eo *
_elm_thumb_efl_object_constructor(Eo *obj, Elm_Thumb_Data *sd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, "Elm_Thumb");
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_IMAGE);

   sd->obj = obj;

   return obj;
}

/**
 * @internal @brief Eolian implementation for efl_file_set (property: file).
 *
 * Sets the source file path for the thumbnail. If the file path changes,
 * the `loaded` flag is reset.
 *
 * @param obj The Evas object (unused).
 * @param sd The widget's private data.
 * @param file The path to the source file for which to generate a thumbnail.
 * @return Always 0 (Eina_Error indicating success).
 */
EOLIAN static Eina_Error
_elm_thumb_efl_file_file_set(Eo *obj EINA_UNUSED, Elm_Thumb_Data *sd, const char *file)
{
   if (eina_stringshare_replace(&(sd->file), file))
     sd->loaded = EINA_FALSE;
   return 0;
}

/**
 * @internal @brief Eolian implementation for efl_file_set (property: key).
 *
 * Sets the key for the source file, used for example with EET files.
 * If the key changes, the `loaded` flag is reset.
 *
 * @param obj The Evas object (unused).
 * @param sd The widget's private data.
 * @param key The key associated with the source file.
 */
EOLIAN static void
_elm_thumb_efl_file_key_set(Eo *obj EINA_UNUSED, Elm_Thumb_Data *sd, const char *key)
{
   if (eina_stringshare_replace(&(sd->key), key))
     sd->loaded = EINA_FALSE;
}

/**
 * @internal @brief Eolian implementation for efl_file_get (property: file).
 * @param obj The Evas object (unused).
 * @param sd The widget's private data.
 * @return The current source file path.
 */
EOLIAN static const char *
_elm_thumb_efl_file_file_get(const Eo *obj EINA_UNUSED, Elm_Thumb_Data *sd)
{
   return sd->file;
}

/**
 * @internal @brief Eolian implementation for efl_file_get (property: key).
 * @param obj The Evas object (unused).
 * @param sd The widget's private data.
 * @return The current key for the source file.
 */
EOLIAN static const char *
_elm_thumb_efl_file_key_get(const Eo *obj EINA_UNUSED, Elm_Thumb_Data *sd)
{
   return sd->key;
}

/**
 * @internal @brief Eolian implementation for efl_file_loaded_get.
 * @param obj The Evas object (unused).
 * @param sd The widget's private data.
 * @return EINA_TRUE if the file (and its thumbnail) is considered loaded, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_thumb_efl_file_loaded_get(const Eo *obj EINA_UNUSED, Elm_Thumb_Data *sd)
{
   return sd->loaded;
}

/**
 * @internal @brief Eolian implementation for efl_file_unload.
 *
 * Resets the thumbnail state, effectively unloading any currently displayed
 * thumbnail and canceling ongoing operations. It clears internal paths,
 * flags, and view objects.
 *
 * @param obj The Evas object (unused).
 * @param sd The widget's private data.
 */
EOLIAN static void
_elm_thumb_efl_file_unload(Eo *obj EINA_UNUSED, Elm_Thumb_Data *sd)
{
   sd->is_video = EINA_FALSE;
   eina_stringshare_replace(&(sd->thumb.file), NULL);
   eina_stringshare_replace(&(sd->thumb.key), NULL);
   sd->loaded = EINA_FALSE;

   if (sd->thumb.request)
     {
        ethumb_client_thumb_async_cancel(_elm_ethumb_client, sd->thumb.request);
        sd->thumb.request = NULL;
     }
   if (sd->thumb.retry)
     {
        retry = eina_list_remove(retry, sd);
        efl_data_unref(sd->obj, sd);
        sd->thumb.retry = EINA_FALSE;
     }
   evas_object_event_callback_del_full(sd->view, EVAS_CALLBACK_IMAGE_PRELOADED,
                                       _on_thumb_preloaded, sd);

   ELM_SAFE_FREE(sd->view, evas_object_del);
   eina_stringshare_replace(&sd->thumb.thumb_path, NULL);
   eina_stringshare_replace(&sd->thumb.thumb_key, NULL);

   ELM_SAFE_FREE(sd->eeh, ecore_event_handler_del);

}

/**
 * @internal @brief Eolian implementation for efl_file_load.
 *
 * Initiates the loading process for the thumbnail. It first determines if the
 * source file is a video based on its extension. Then, it resets internal
 * thumbnail path/key information and sets the `loaded` flag to EINA_TRUE
 * (indicating a load attempt is in progress or completed). If the widget is
 * visible, it calls _thumb_show() to start the actual thumbnail generation
 * or retrieval from cache.
 *
 * @param obj The Evas object.
 * @param sd The widget's private data.
 * @return Always 0 (Eina_Error indicating success or that the process has started).
 */
EOLIAN static Eina_Error
_elm_thumb_efl_file_load(Eo *obj, Elm_Thumb_Data *sd)
{
   int prefix_size;
   const char **ext, *ptr;
   static const char *extensions[] =
     {
        ".asf", ".avi", ".bdm", ".bdmv", ".clpi", ".cpi", ".dv", ".fla",
        ".flv", ".m1v", ".m2t", ".m2v", ".m4v", ".mkv", ".mov", ".mp2",
        ".mp2ts", ".mp4", ".mpe", ".mpeg", ".mpg", ".mpl", ".mpls", ".mts",
        ".mxf", ".nut", ".nuv", ".ogg", ".ogm", ".ogv", ".qt", ".rm", ".rmj",
        ".rmm", ".rms", ".rmvb", ".rmx", ".rv", ".swf", ".ts", ".weba",
        ".webm", ".wmv", ".3g2", ".3gp", ".3gp2", ".3gpp", ".3gpp2", ".3p2",
        ".264",
        NULL
     };

   if (efl_file_loaded_get(obj)) return 0;
   prefix_size = eina_stringshare_strlen(sd->file) - 4;
   if (prefix_size >= 0)
     {
        ptr = sd->file + prefix_size;
        sd->is_video = EINA_FALSE;
        for (ext = extensions; *ext; ext++)
          if (!strcasecmp(ptr, *ext))
            {
               sd->is_video = EINA_TRUE;
               break;
            }
     }

   eina_stringshare_replace(&(sd->thumb.file), NULL);
   eina_stringshare_replace(&(sd->thumb.key), NULL);
   sd->loaded = EINA_TRUE;

   if (evas_object_visible_get(obj))
     _thumb_show(sd);

   return 0;
}

EAPI void *
elm_thumb_ethumb_client_get(void)
{
   return _elm_ethumb_client;
}

/**
 * @brief Gets whether the Ethumb client is currently connected.
 *
 * @return @c EINA_TRUE if connected, @c EINA_FALSE otherwise.
 * @ingroup Elm_Thumb
 */
EAPI Eina_Bool
elm_thumb_ethumb_client_connected_get(void)
{
   return _elm_ethumb_connected;
}

/**
 * @internal @brief Eolian implementation for efl_ui_draggable_drag_target_set.
 *
 * Enables or disables the thumbnail widget as a drop target for drag and drop
 * operations. If enabled, it allows files to be dropped onto it to change
 * the thumbnail's source file.
 *
 * @param obj The Evas object.
 * @param sd The widget's private data.
 * @param edit If EINA_TRUE, enables drag target capability; otherwise, disables it.
 */
EOLIAN static void
_elm_thumb_efl_ui_draggable_drag_target_set(Eo *obj, Elm_Thumb_Data *sd, Eina_Bool edit)
{
   edit = !!edit;
   if (sd->edit == edit) return;

   sd->edit = edit;
   if (sd->edit)
     elm_drop_target_add(obj, ELM_SEL_FORMAT_IMAGE,
                         NULL, NULL,
                         NULL, NULL,
                         NULL, NULL,
                         _elm_thumb_dnd_cb, obj);
   else
     elm_drop_target_del(obj, ELM_SEL_FORMAT_IMAGE,
                         NULL, NULL,
                         NULL, NULL,
                         NULL, NULL,
                         _elm_thumb_dnd_cb, obj);

   return;
}

/**
 * @internal @brief Eolian implementation for efl_ui_draggable_drag_target_get.
 *
 * @param obj The Evas object (unused).
 * @param sd The widget's private data.
 * @return EINA_TRUE if the widget is currently a drag target, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_thumb_efl_ui_draggable_drag_target_get(const Eo *obj EINA_UNUSED, Elm_Thumb_Data *sd)
{
   return sd->edit;
}

/**
 * @internal @brief Eolian class constructor for Elm_Thumb.
 *
 * Registers the legacy type name for this class.
 *
 * @param klass The Efl_Class being constructed.
 */
EOLIAN static void
_elm_thumb_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @brief Set the file that will be used as thumbnail.
 *
 * @param obj The thumbnail object.
 * @param file The path to the file.
 * @param key The key used to generate the thumbnail from file. (For Eet
 * files, the key will be passed to @c eet_data_image_read(). For other files
 * this parameter is ignored.)
 *
 * @ingroup Elm_Thumb
 */
EAPI void
elm_thumb_file_set(Eo *obj, const char *file, const char *key)
{
   efl_file_simple_load((Eo *) obj, file, key);
}

/**
 * @brief Get the file path and key that will be used as thumbnail.
 *
 * @param obj The thumbnail object.
 * @param file Pointer to store the file path.
 * @param key Pointer to store the key.
 *
 * @ingroup Elm_Thumb
 */
EAPI void
elm_thumb_file_get(const Eo *obj, const char **file, const char **key)
{
   efl_file_simple_get((Eo *) obj, file, key);
}

/* Legacy deprecated functions */

/**
 * @brief Turn on/off the eaditable state for a given thumbnail object
 *
 * @param obj The thumbnail object
 * @param edit If @c EINA_TRUE, the object is marked as editable.
 * If @c EINA_FALSE, it's marked as non-editable
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 *
 * @deprecated Use efl_ui_draggable_drag_target_set() instead.
 *
 * @ingroup Elm_Thumb
 */
EAPI Eina_Bool
elm_thumb_editable_set(Evas_Object *obj, Eina_Bool edit)
{
   efl_ui_draggable_drag_target_set(obj, edit);
   return EINA_TRUE;
}

EAPI Eina_Bool
elm_thumb_editable_get(const Evas_Object *obj)
{
   return efl_ui_draggable_drag_target_get(obj);
}

EAPI void
elm_thumb_aspect_set(Evas_Object *obj, Ethumb_Thumb_Aspect aspect)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   sd->thumb.aspect = aspect;
}

EAPI Ethumb_Thumb_Aspect
elm_thumb_aspect_get(const Evas_Object *obj)
{
   ELM_THUMB_CHECK(obj) ETHUMB_THUMB_KEEP_ASPECT;
   ELM_THUMB_DATA_GET(obj, sd);
   return sd->thumb.aspect;
}

EAPI void
elm_thumb_fdo_size_set(Evas_Object *obj, Ethumb_Thumb_FDO_Size size)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   sd->thumb.size = size;
}

EAPI Ethumb_Thumb_FDO_Size
elm_thumb_fdo_size_get(const Evas_Object *obj)
{
   ELM_THUMB_CHECK(obj) ETHUMB_THUMB_NORMAL;
   ELM_THUMB_DATA_GET(obj, sd);
   return sd->thumb.size;
}

EAPI void
elm_thumb_format_set(Evas_Object *obj, Ethumb_Thumb_Format format)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   sd->thumb.format = format;
}

EAPI Ethumb_Thumb_Format
elm_thumb_format_get(const Evas_Object *obj)
{
   ELM_THUMB_CHECK(obj) ETHUMB_THUMB_FDO;
   ELM_THUMB_DATA_GET(obj, sd);
   return sd->thumb.format;
}

EAPI void
elm_thumb_orientation_set(Evas_Object *obj, Ethumb_Thumb_Orientation orient)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   sd->thumb.orient = orient;
}

EAPI Ethumb_Thumb_Orientation
elm_thumb_orientation_get(const Evas_Object *obj)
{
   ELM_THUMB_CHECK(obj) ETHUMB_THUMB_ORIENT_NONE;
   ELM_THUMB_DATA_GET(obj, sd);
   return sd->thumb.orient;
}

EAPI void
elm_thumb_size_set(Evas_Object *obj, int tw, int th)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   sd->thumb.tw = tw;
   sd->thumb.th = th;
}

EAPI void
elm_thumb_size_get(const Evas_Object *obj, int *tw, int *th)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   if (tw)
     *tw = sd->thumb.tw;
   if (th)
     *th = sd->thumb.th;
}

EAPI void
elm_thumb_crop_align_set(Evas_Object *obj, double cropx, double cropy)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   sd->thumb.cropx = cropx;
   sd->thumb.cropy = cropy;
}

EAPI void
elm_thumb_crop_align_get(const Evas_Object *obj, double *cropx, double *cropy)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   if (cropx)
     *cropx = sd->thumb.cropx;
   if (cropy)
     *cropy = sd->thumb.cropy;
}

EAPI void
elm_thumb_compress_set(Evas_Object *obj, int compress)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   sd->thumb.compress = compress;
}

EAPI void
elm_thumb_compress_get(const Evas_Object *obj, int *compress)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   if (compress)
      *compress = sd->thumb.compress;
}

EAPI void
elm_thumb_quality_set(Evas_Object *obj, int quality)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   sd->thumb.quality = quality;
}

EAPI void
elm_thumb_quality_get(const Evas_Object *obj, int *quality)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   if (quality)
      *quality = sd->thumb.quality;
}

EAPI void
elm_thumb_animate_set(Evas_Object *obj, Elm_Thumb_Animation_Setting setting)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   EINA_SAFETY_ON_TRUE_RETURN(setting >= ELM_THUMB_ANIMATION_LAST);

   sd->anim_setting = setting;

   if ((sd->is_video) && (sd->thumb.format == ETHUMB_THUMB_EET))
     {
        if (setting == ELM_THUMB_ANIMATION_LOOP)
          edje_object_signal_emit(sd->view, "elm,action,animate_loop", "elm");
        else if (setting == ELM_THUMB_ANIMATION_START)
          edje_object_signal_emit(sd->view, "elm,action,animate", "elm");
        else if (setting == ELM_THUMB_ANIMATION_STOP)
          edje_object_signal_emit(sd->view, "elm,action,animate_stop", "elm");
     }
}

EAPI Elm_Thumb_Animation_Setting
elm_thumb_animate_get(const Evas_Object *obj)
{
   ELM_THUMB_CHECK(obj) 0;
   ELM_THUMB_DATA_GET(obj, sd);
   return sd->anim_setting;
}

EAPI void
elm_thumb_path_get(const Evas_Object *obj, const char **file, const char **key)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   if (file)
     *file = sd->thumb.file;
   if (key)
     *key = sd->thumb.key;
}

EAPI void
elm_thumb_reload(Evas_Object *obj)
{
   ELM_THUMB_CHECK(obj);
   ELM_THUMB_DATA_GET(obj, sd);
   eina_stringshare_replace(&(sd->thumb.file), NULL);
   eina_stringshare_replace(&(sd->thumb.key), NULL);

   if (evas_object_visible_get(obj))
     _thumb_show(sd);
}

/* Internal EO APIs and hidden overrides */

#define ELM_THUMB_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_thumb)

#include "elm_thumb_eo.c"
