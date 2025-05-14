#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_icon.h"
#include "efl_ui_widget_image.h"
#include "elm_icon_eo.h"

#define NON_EXISTING (void *)-1

#define MY_CLASS ELM_ICON_CLASS
#define MY_CLASS_NAME "Elm_Icon"
#define MY_CLASS_NAME_LEGACY "elm_icon"

/**
 * @internal
 * @brief List of icon data structures that require a retry for thumbnail generation.
 *
 * This list holds Elm_Icon_Data pointers for icons whose thumbnail
 * generation failed previously and should be retried.
 */
static Eina_List *_elm_icon_retry = NULL;

/**
 * @internal
 * @brief Counter for pending thumbnail generation requests.
 *
 * This counter tracks the number of active asynchronous thumbnail
 * requests to the ethumb client.
 */
static int _icon_pending_request = 0;

static const char SIG_THUMB_DONE[] = "thumb,done";
static const char SIG_THUMB_ERROR[] = "thumb,error";
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_THUMB_DONE, ""},
   {SIG_THUMB_ERROR, ""},
   {NULL, NULL}
};

/**
 * @internal
 * @brief Get the minimum dimension for the icon, with a lower bound.
 *
 * Calculates the minimum of the icon's width and height, but ensures
 * it's not less than 16. This is typically used to determine an
 * appropriate size for thumbnail requests.
 *
 * @param icon The icon Evas_Object.
 * @return The minimum dimension (at least 16).
 */
static inline int
_icon_size_min_get(Evas_Object *icon)
{
   int w, h;

   evas_object_geometry_get(icon, NULL, NULL, &w, &h);

   return MAX(16, MIN(w, h));
}

/**
 * @internal
 * @brief Stop an ongoing thumbnail generation request for an icon.
 *
 * If a thumbnail request is active for the given icon data, it cancels
 * the request using the ethumb client. It also removes the icon data
 * from the retry list if it was marked for retry.
 *
 * @param sd The icon's private data.
 * @param ethumbd The ethumb client instance.
 */
static void
_icon_thumb_stop(Elm_Icon_Data *sd,
                 void *ethumbd)
{
   if (sd->thumb.request)
     {
        ethumb_client_thumb_async_cancel(ethumbd, sd->thumb.request);
        sd->thumb.request = NULL;
        _icon_pending_request--;
     }

   if (sd->thumb.retry)
     {
        _elm_icon_retry = eina_list_remove(_elm_icon_retry, sd);
        sd->thumb.retry = EINA_FALSE;
     }
}

/**
 * @internal
 * @brief Attempts to display the generated thumbnail for the icon.
 *
 * Sets the icon's image file to the generated thumbnail path and key.
 * If the thumbnail format indicates an Edje file (EET) and the original
 * file was a video, it specifically handles setting the Edje file.
 * Emits "thumb,done" or "thumb,error" signals based on success.
 *
 * @param sd The icon's private data, containing thumbnail information.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
_icon_thumb_display(Elm_Icon_Data *sd)
{
   Eina_Bool ret = EINA_FALSE;

   if (sd->thumb.format == ETHUMB_THUMB_EET)
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

        prefix_size = eina_stringshare_strlen(sd->thumb.file.path) - 4;
        if (prefix_size >= 0)
          {
             ptr = sd->thumb.file.path + prefix_size;
             for (ext = extensions; *ext; ++ext)
               if (!strcasecmp(ptr, *ext))
                 {
                    sd->is_video = EINA_TRUE;
                    break;
                 }
          }

        ret = elm_image_file_set
            (sd->obj, sd->thumb.thumb.path,
            sd->thumb.thumb.key);

        sd->is_video = EINA_FALSE;
     }

   if (!ret)
     ret = elm_image_file_set
         (sd->obj, sd->thumb.thumb.path, sd->thumb.thumb.key);

   if (ret)
     efl_event_callback_legacy_call
       (sd->obj, ELM_ICON_EVENT_THUMB_DONE, NULL);
   else
     efl_event_callback_legacy_call
       (sd->obj, ELM_ICON_EVENT_THUMB_ERROR, NULL);

   return ret;
}

/**
 * @internal
 * @brief Retries displaying a thumbnail.
 *
 * This function is a simple wrapper around _icon_thumb_display,
 * intended to be called when retrying thumbnail display.
 *
 * @param sd The icon's private data.
 * @return @c EINA_TRUE if display was successful, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_icon_thumb_retry(Elm_Icon_Data *sd)
{
   return _icon_thumb_display(sd);
}

/**
 * @internal
 * @brief Cleans up thumbnail retry list and pending requests.
 *
 * Iterates through the list of icons marked for thumbnail retry (_elm_icon_retry).
 * For each icon, it attempts to display the thumbnail again. If successful,
 * the icon is removed from the retry list.
 * If there are no more pending requests (_icon_pending_request is 0),
 * it clears the entire retry list and stops any associated thumbnail operations.
 *
 * @param ethumbd The ethumb client instance.
 */
static void
_icon_thumb_cleanup(Ethumb_Client *ethumbd)
{
   Eina_List *l, *ll;
   Elm_Icon_Data *sd;

   EINA_LIST_FOREACH_SAFE(_elm_icon_retry, l, ll, sd)
     if (_icon_thumb_retry(sd))
       {
          _elm_icon_retry = eina_list_remove_list(_elm_icon_retry, l);
          sd->thumb.retry = EINA_FALSE;
       }

   if (_icon_pending_request == 0)
     EINA_LIST_FREE(_elm_icon_retry, sd)
       _icon_thumb_stop(sd, ethumbd);
}

/**
 * @internal
 * @brief Finalizes the thumbnail process after generation (success or error).
 *
 * This function is called after a thumbnail has been generated or an error occurred.
 * It attempts to display the new thumbnail. If display fails and a previous
 * image was set, it reverts to the previous image and adds the icon to a
 * retry list. Finally, it calls _icon_thumb_cleanup.
 *
 * @param sd The icon's private data.
 * @param ethumbd The ethumb client instance.
 */
static void
_icon_thumb_finish(Elm_Icon_Data *sd,
                   Ethumb_Client *ethumbd)
{
   const char *file = NULL, *group = NULL;
   Eina_Bool ret = EINA_FALSE;

   elm_image_file_get(sd->obj, &file, &group);
   file = eina_stringshare_ref(file);
   group = eina_stringshare_ref(group);

   ret = _icon_thumb_display(sd);

   if (!ret && file)
     {
        if (!sd->thumb.retry)
          {
             _elm_icon_retry = eina_list_append(_elm_icon_retry, sd);
             sd->thumb.retry = EINA_TRUE;
          }

        /* Back to previous image */
        elm_image_file_set(sd->obj, file, group);
     }

   _icon_thumb_cleanup(ethumbd);

   eina_stringshare_del(file);
   eina_stringshare_del(group);
}

/**
 * @internal
 * @brief Callback executed when thumbnail generation succeeds.
 *
 * This function is called by the ethumb client when a thumbnail
 * has been successfully generated. It updates the icon's data with the
 * new thumbnail path and key, decrements the pending request counter,
 * and then calls _icon_thumb_finish to display the thumbnail.
 *
 * @param client The ethumb client instance.
 * @param thumb_path The path to the generated thumbnail file.
 * @param thumb_key The key/group within the thumbnail file (if any).
 * @param data User data, expected to be Elm_Icon_Data *sd.
 */
static void
_icon_thumb_done(Ethumb_Client *client,
                 const char *thumb_path,
                 const char *thumb_key,
                 void *data)
{
   Elm_Icon_Data *sd = data;

   if (EINA_UNLIKELY(!sd->thumb.request))
     {
        ERR("Something odd happened with a thumbnail request");
        return;
     }

   _icon_pending_request--;
   sd->thumb.request = NULL;

   eina_stringshare_replace(&sd->thumb.thumb.path, thumb_path);
   eina_stringshare_replace(&sd->thumb.thumb.key, thumb_key);
   sd->thumb.format = ethumb_client_format_get(client);

   _icon_thumb_finish(sd, client);
}

/**
 * @internal
 * @brief Callback executed when thumbnail generation fails.
 *
 * This function is called by the ethumb client when thumbnail generation
 * encounters an error. It decrements the pending request counter, logs an error,
 * emits the ELM_ICON_EVENT_THUMB_ERROR signal, and then calls _icon_thumb_cleanup.
 *
 * @param client The ethumb client instance.
 * @param data User data, expected to be Elm_Icon_Data *sd.
 */
static void
_icon_thumb_error(Ethumb_Client *client,
                  void *data)
{
   Elm_Icon_Data *sd = data;

   if (EINA_UNLIKELY(!sd->thumb.request))
     {
        ERR("Something odd happened with a thumbnail request");
        return;
     }

   _icon_pending_request--;
   sd->thumb.request = NULL;

   ERR("could not generate thumbnail for %s (key: %s)",
       sd->thumb.file.path, sd->thumb.file.key);

   efl_event_callback_legacy_call(sd->obj, ELM_ICON_EVENT_THUMB_ERROR, NULL);

   _icon_thumb_cleanup(client);
}

/**
 * @internal
 * @brief Initiates or re-initiates a thumbnail generation request.
 *
 * Stops any existing thumbnail request for the icon. If a file path is set
 * in sd->thumb.file.path, it configures the ethumb client with the file,
 * desired size (based on _icon_size_min_get), and then asynchronously
 * requests the thumbnail generation. Callbacks _icon_thumb_done and
 * _icon_thumb_error are set to handle the result.
 *
 * @param sd The icon's private data.
 */
static void
_icon_thumb_apply(Elm_Icon_Data *sd)
{
   Ethumb_Client *ethumbd;
   int min_size;

   ethumbd = elm_thumb_ethumb_client_get();

   _icon_thumb_stop(sd, ethumbd);

   if (!sd->thumb.file.path) return;

   _icon_pending_request++;
   if (!ethumb_client_file_set
         (ethumbd, sd->thumb.file.path, sd->thumb.file.key)) return;

   min_size = _icon_size_min_get(sd->obj);
   ethumb_client_size_set(ethumbd, min_size, min_size);

   sd->thumb.request = ethumb_client_thumb_async_get
       (ethumbd, _icon_thumb_done, _icon_thumb_error, sd);
}

/**
 * @internal
 * @brief Ecore event callback to apply thumbnail generation.
 *
 * This callback is typically triggered when the Ethumb connection is established.
 * It retrieves the icon data and calls _icon_thumb_apply to start
 * the thumbnail generation process.
 *
 * @param data User data, expected to be the Evas_Object of the icon.
 * @param type The type of the event (unused).
 * @param ev The event information (unused).
 * @return ECORE_CALLBACK_RENEW to keep the handler.
 */
static Eina_Bool
_icon_thumb_apply_cb(void *data,
                     int type EINA_UNUSED,
                     void *ev EINA_UNUSED)
{
   ELM_ICON_DATA_GET(data, sd);

   _icon_thumb_apply(sd);

   return ECORE_CALLBACK_RENEW;
}

/**
 * @internal
 * @brief Sets the icon from a freedesktop.org theme.
 *
 * Attempts to find an icon by name within the specified FDO theme (or the
 * configured default if theme is NULL) at the given size. If found,
 * it sets the icon's image file to the discovered path and updates
 * internal state to indicate it's using an FDO icon.
 *
 * @param obj The icon Evas_Object.
 * @param theme The name of the FDO theme to search (e.g., "hicolor").
 *              If NULL, uses the system's configured icon theme.
 * @param name The name of the icon (e.g., "document-open").
 * @param size The desired size of the icon.
 * @return @c EINA_TRUE if the icon was found and set, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_icon_freedesktop_set(Evas_Object *obj,
                      const char *theme,
                      const char *name,
                      int size)
{
   const char *path;

   ELM_ICON_DATA_GET(obj, sd);

   elm_need_efreet();
   if (!theme)
     theme = elm_config_icon_theme_get();

   path = efreet_icon_path_find(theme, name, size);
   sd->freedesktop.use = !!path;
   if (sd->freedesktop.use)
     {
        sd->freedesktop.requested_size = size;
        elm_image_file_set(obj, path, NULL);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Internal callback for Edje signals.
 *
 * This function acts as a trampoline for Edje signal callbacks.
 * When an Edje signal is received on the icon's internal Edje object,
 * this callback is invoked. It then calls the user-provided callback
 * function stored in Edje_Signal_Data.
 *
 * @param data Pointer to Edje_Signal_Data containing the original callback and user data.
 * @param obj The Evas_Object emitting the signal (the Edje object, unused here).
 * @param emission The emission string of the signal.
 * @param source The source string of the signal.
 */
static void
_edje_signal_callback(void *data,
                      Evas_Object *obj EINA_UNUSED,
                      const char *emission,
                      const char *source)
{
   Edje_Signal_Data *esd = data;

   esd->func(esd->data, esd->obj, emission, source);
}

/**
 * @internal
 * @brief Frees all registered Edje signal callbacks for an icon.
 *
 * Iterates through the list of registered Edje signal callbacks
 * (sd->edje_signals), removes them from the internal Edje object,
 * and frees the associated Edje_Signal_Data structures.
 *
 * @param sd The icon's private data.
 */
static void
_edje_signals_free(Elm_Icon_Data *sd)
{
   Edje_Signal_Data *esd;
   Efl_Ui_Image_Data *id = efl_data_scope_get(sd->obj, EFL_UI_IMAGE_CLASS);

   EINA_LIST_FREE(sd->edje_signals, esd)
     {
        edje_object_signal_callback_del_full
           (id->img, esd->emission, esd->source,
            _edje_signal_callback, esd);
        eina_stringshare_del(esd->emission);
        eina_stringshare_del(esd->source);
        free(esd);
     }
}

/**
 * @internal
 * @brief Implements the efl_file.load interface for Elm_Icon.
 * @details This function handles loading the image file for the icon.
 * It first calls the parent's efl_file_load implementation.
 * If the icon is not using a freedesktop icon and is not a video,
 * it performs minimal cleanup.
 * Special handling is done for video files, which might be represented
 * as Edje files (.eet). In such cases, it ensures the internal image
 * object is an Edje object and loads the file into it.
 *
 * @param obj The icon object.
 * @param sd The icon's private data.
 * @return Eina_Error 0 on success, or an error code on failure.
 */
EOLIAN static Eina_Error
_elm_icon_efl_file_load(Eo *obj, Elm_Icon_Data *sd)
{
   Evas_Object *pclip;
   const char *key;
   Eina_Error err;

   if (efl_file_loaded_get(obj)) return 0;
   err = efl_file_load(efl_super(obj, MY_CLASS));
   if (err) return err;

   Efl_Ui_Image_Data *id = efl_data_scope_get(obj, EFL_UI_IMAGE_CLASS);

   _edje_signals_free(sd);

   if (!sd->freedesktop.use)
     ELM_SAFE_FREE(sd->stdicon, eina_stringshare_del);

   if (!sd->is_video) return 0;

   /* parent's edje file setting path replicated here (we got .eet
    * extension, so bypassing it) */
   ELM_SAFE_FREE(id->prev_img, evas_object_del);

   if (!id->edje)
     {
        pclip = evas_object_clip_get(id->img);
        evas_object_del(id->img);

        /* Edje object instead */
        id->img = edje_object_add(evas_object_evas_get(obj));
        evas_object_smart_member_add(id->img, obj);
        if (id->show)
          evas_object_show(id->img);
        evas_object_clip_set(id->img, pclip);
        id->edje = EINA_TRUE;
     }
   key = efl_file_key_get(obj);
   efl_file_key_set(id->img, key);
   err = efl_file_mmap_set(id->img, efl_file_mmap_get(obj));
   if (!err) err = efl_file_load(id->img);
   if (err)
     {
        ERR("failed to set edje file '%s', group '%s': %s", efl_file_get(id->img), key,
            edje_load_error_str
              (edje_object_load_error_get(id->img)));
        return err;
     }

   efl_gfx_entity_geometry_set(id->img, efl_gfx_entity_geometry_get(obj));

   return 0;
}

/**
 * @internal
 * @brief Implements the efl_ui_widget.theme_apply interface for Elm_Icon.
 * @details Applies the current theme to the icon. If a standard icon name
 * (sd->stdicon) is set, it attempts to set the icon from the theme
 * using that name and the widget's current style.
 * After that, it calls the parent's theme_apply implementation.
 *
 * @param obj The icon object.
 * @param sd The icon's private data.
 * @return EFL_UI_THEME_APPLY_ERROR_NONE on success, or an error code.
 */
EOLIAN static Eina_Error
_elm_icon_efl_ui_widget_theme_apply(Eo *obj, Elm_Icon_Data *sd)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;

   if (sd->stdicon)
     _elm_theme_object_icon_set(obj, sd->stdicon, elm_widget_style_get(obj));

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   return int_ret;
}

/**
 * @internal
 * @brief Sets a standard icon from the Elementary theme.
 *
 * Attempts to set the icon's image using the provided name, looking it up
 * in the "default" Elementary theme group. If successful, it clears the
 * freedesktop usage flag.
 *
 * @param obj The icon Evas_Object.
 * @param name The standard name of the icon (e.g., "home", "delete").
 * @return @c EINA_TRUE if the icon was successfully set from the theme,
 *         @c EINA_FALSE otherwise.
 */
static Eina_Bool
_icon_standard_set(Evas_Object *obj,
                   const char *name)
{
   ELM_ICON_DATA_GET(obj, sd);

   if (_elm_theme_object_icon_set(obj, name, "default"))
     {
        /* TODO: elm_unneed_efreet() */
        sd->freedesktop.use = EINA_FALSE;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @internal
 * @brief Sets the icon from a direct file path.
 *
 * Sets the icon's image using elm_image_file_set with the given path.
 * If successful, it clears the freedesktop usage flag.
 *
 * @param sd The icon's private data.
 * @param obj The icon Evas_Object.
 * @param path The file path to the image.
 * @return @c EINA_TRUE if the file was successfully set, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_icon_file_set(Elm_Icon_Data *sd,
               Evas_Object *obj,
               const char *path)
{
   if (elm_image_file_set(obj, path, NULL))
     {
        /* TODO: elm_unneed_efreet() */
        sd->freedesktop.use = EINA_FALSE;
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Core logic for setting a standard icon, with fallback mechanisms.
 *
 * This function attempts to set an icon based on a standard name.
 * The lookup order is:
 * 1. Elementary theme (using _icon_standard_set).
 * 2. If Elementary theme is the configured theme:
 *    a. Try Elementary theme again (redundant, but present).
 *    b. Try Freedesktop "hicolor" theme.
 * 3. If a different theme is configured:
 *    a. Try Freedesktop with the configured theme.
 * 4. If the name is an absolute path, try loading it as a direct file.
 * 5. If the name contains a '/', try stripping the prefix (e.g., "size/")
 *    and recursively call this function with the base name.
 *
 * @param obj The icon Evas_Object.
 * @param name The standard name or path of the icon.
 * @param[out] fdo Pointer to a boolean that will be set to @c EINA_TRUE if
 *                 a Freedesktop icon was successfully used, @c EINA_FALSE otherwise.
 *                 Can be NULL if this information is not needed.
 * @return @c EINA_TRUE if an icon was successfully set, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_internal_elm_icon_standard_set(Evas_Object *obj,
                       const char *name,
                       Eina_Bool *fdo)
{
   char *tmp;
   const char *stdtmp;
   Eina_Bool ret = EINA_FALSE;

   ELM_ICON_DATA_GET(obj, sd);

   /* try locating the icon using the specified theme */
   stdtmp = sd->stdicon;
   sd->stdicon = NULL;
   ret = _icon_standard_set(obj, name);
   sd->stdicon = stdtmp;
   if (ret && fdo) *fdo = EINA_FALSE;
   if (!ret)
     {
        /* try locating the icon using the specified theme */
        if (!strcmp(ELM_CONFIG_ICON_THEME_ELEMENTARY, elm_config_icon_theme_get()))
          {
             ret = _icon_standard_set(obj, name);
             if (ret && fdo) *fdo = EINA_FALSE;

             if (!ret)
               {
                  ret = _icon_freedesktop_set(obj, "hicolor", name, _icon_size_min_get(obj));
                  if (ret && fdo) *fdo = EINA_TRUE;
               }
          }
        else
          {
             ret = _icon_freedesktop_set(obj, NULL, name, _icon_size_min_get(obj));
             if (ret && fdo) *fdo = EINA_TRUE;
          }
     }

   if (ret)
     {
        eina_stringshare_replace(&sd->stdicon, name);
        efl_canvas_group_change(obj);
        return EINA_TRUE;
     }

   if (!eina_file_path_relative(name))
     {
        if (fdo)
          *fdo = EINA_FALSE;
        return _icon_file_set(sd, obj, name);
     }

   /* if that fails, see if icon name is in the format size/name. if so,
      try locating a fallback without the size specification */
   if (!(tmp = strchr(name, '/'))) return EINA_FALSE;
   ++tmp;
   if (*tmp) return _internal_elm_icon_standard_set(obj, tmp, fdo);
   /* give up */
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_RESIZE when a standard icon is set.
 *
 * This callback is registered when a standard icon (especially an FDO one)
 * is set. On resize, it re-evaluates and re-sets the standard icon. This is
 * important for FDO icons, as the best icon might change with size.
 * If the re-set operation fails or does not result in an FDO icon,
 * the resize callback is removed.
 *
 * @param data User data, expected to be the Evas_Object of the icon.
 * @param e The Evas canvas (unused).
 * @param obj The Evas_Object that was resized (the icon itself).
 * @param event_info Event-specific information (unused).
 */
static void
_elm_icon_standard_resize_cb(void *data,
                             Evas *e EINA_UNUSED,
                             Evas_Object *obj,
                             void *event_info EINA_UNUSED)
{
   ELM_ICON_DATA_GET(data, sd);
   const char *refup = eina_stringshare_ref(sd->stdicon);
   Eina_Bool fdo = EINA_FALSE;

   if (!_internal_elm_icon_standard_set(obj, sd->stdicon, &fdo) || (!fdo))
     evas_object_event_callback_del_full
       (obj, EVAS_CALLBACK_RESIZE, _elm_icon_standard_resize_cb, data);
   eina_stringshare_del(refup);
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_RESIZE when a thumbnail is set.
 *
 * This callback is registered when elm_icon_thumb_set() is called.
 * On resize, it re-triggers the thumbnail generation process for the
 * current file and key, potentially requesting a different thumbnail size.
 *
 * @param data User data, expected to be the Evas_Object of the icon.
 * @param e The Evas canvas (unused).
 * @param obj The Evas_Object that was resized (the icon itself).
 * @param event_info Event-specific information (unused).
 */
static void
_elm_icon_thumb_resize_cb(void *data,
                          Evas *e EINA_UNUSED,
                          Evas_Object *obj,
                          void *event_info EINA_UNUSED)
{
   ELM_ICON_DATA_GET(data, sd);

   if (sd->thumb.file.path)
     elm_icon_thumb_set(obj, sd->thumb.file.path, sd->thumb.file.key);
}

/**
 * @internal
 * @brief Implements efl_canvas_group.group_add for Elm_Icon.
 * @details Called when the icon is added to a smart group (parent).
 * Initializes the thumbnail request pointer.
 *
 * @param obj The icon object.
 * @param priv The icon's private data.
 */
EOLIAN static void
_elm_icon_efl_canvas_group_group_add(Eo *obj, Elm_Icon_Data *priv)
{
   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   priv->thumb.request = NULL;
}

/**
 * @internal
 * @brief Implements efl_canvas_group.group_del for Elm_Icon.
 * @details Called when the icon is being removed from its smart group (parent)
 * or being deleted. Cleans up resources associated with the icon,
 * including standard icon name, thumbnail request and data,
 * event handlers, and Edje signals.
 *
 * @param obj The icon object.
 * @param sd The icon's private data.
 */
EOLIAN static void
_elm_icon_efl_canvas_group_group_del(Eo *obj, Elm_Icon_Data *sd)
{
   eina_stringshare_del(sd->stdicon);

   if (sd->thumb.request)
     {
        Ethumb_Client *ethumbd = elm_thumb_ethumb_client_get();
        if (ethumbd) _icon_thumb_stop(sd, ethumbd);
     }

   eina_stringshare_del(sd->thumb.file.path);
   eina_stringshare_del(sd->thumb.file.key);
   eina_stringshare_del(sd->thumb.thumb.path);
   eina_stringshare_del(sd->thumb.thumb.key);
   ecore_event_handler_del(sd->thumb.eeh);

   _edje_signals_free(sd);

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Emits an Edje signal on the icon's internal Edje object.
 * @deprecated This function relies on internal Edje object structure and
 *             is planned for removal.
 *
 * If the icon's underlying image is an Edje object, this function
 * emits the specified signal.
 *
 * @param obj The icon Evas_Object.
 * @param emission The emission string of the signal.
 * @param source The source string of the signal.
 */
/* WARNING: to be deprecated */
void
_elm_icon_signal_emit(Evas_Object *obj,
                      const char *emission,
                      const char *source)
{

   Efl_Ui_Image_Data *id = efl_data_scope_get(obj, EFL_UI_IMAGE_CLASS);

   if (!id->edje) return;

   edje_object_signal_emit(id->img, emission, source);
}

/**
 * @internal
 * @brief Adds a callback for an Edje signal on the icon's internal Edje object.
 * @deprecated This function relies on internal Edje object structure and
 *             is planned for removal.
 *
 * If the icon's underlying image is an Edje object, this function
 * registers a callback for the specified signal. The actual callback
 * registered with Edje is _edje_signal_callback, which then calls
 * the user-provided func_cb.
 *
 * @param obj The icon Evas_Object.
 * @param emission The emission string to listen for.
 * @param source The source string to listen for.
 * @param func_cb The user's callback function.
 * @param data User data to pass to the callback.
 */
/* WARNING: to be deprecated */
void
_elm_icon_signal_callback_add(Evas_Object *obj,
                              const char *emission,
                              const char *source,
                              Edje_Signal_Cb func_cb,
                              void *data)
{
   Edje_Signal_Data *esd;

   ELM_ICON_DATA_GET(obj, sd);
   Efl_Ui_Image_Data *id = efl_data_scope_get(obj, EFL_UI_IMAGE_CLASS);

   if (!id->edje) return;

   esd = ELM_NEW(Edje_Signal_Data);
   if (!esd) return;

   esd->obj = obj;
   esd->func = func_cb;
   esd->emission = eina_stringshare_add(emission);
   esd->source = eina_stringshare_add(source);
   esd->data = data;
   sd->edje_signals =
     eina_list_append(sd->edje_signals, esd);

   edje_object_signal_callback_add
     (id->img, emission, source, _edje_signal_callback, esd);
}

/**
 * @internal
 * @brief Deletes a previously added Edje signal callback.
 * @deprecated This function relies on internal Edje object structure and
 *             is planned for removal.
 *
 * Removes a signal callback that matches the emission, source, and
 * function pointer.
 *
 * @param obj The icon Evas_Object.
 * @param emission The emission string of the callback to remove.
 * @param source The source string of the callback to remove.
 * @param func_cb The function pointer of the callback to remove.
 * @return The user data associated with the removed callback, or NULL if not found.
 */
/* WARNING: to be deprecated */
void *
_elm_icon_signal_callback_del(Evas_Object *obj,
                              const char *emission,
                              const char *source,
                              Edje_Signal_Cb func_cb)
{
   Edje_Signal_Data *esd = NULL;
   void *data = NULL;
   Eina_List *l;

   ELM_ICON_DATA_GET(obj, sd);
   Efl_Ui_Image_Data *id = efl_data_scope_get(obj, EFL_UI_IMAGE_CLASS);

   if (!id->edje) return NULL;

   EINA_LIST_FOREACH(sd->edje_signals, l, esd)
     {
        if ((esd->func == func_cb) && (!strcmp(esd->emission, emission)) &&
            (!strcmp(esd->source, source)))
          {
             sd->edje_signals = eina_list_remove_list(sd->edje_signals, l);
             eina_stringshare_del(esd->emission);
             eina_stringshare_del(esd->source);
             data = esd->data;

             edje_object_signal_callback_del_full
               (id->img, emission, source,
               _edje_signal_callback, esd);

             free(esd);

             return data; /* stop at 1st match */
          }
     }

   return data;
}

EAPI Evas_Object *
elm_icon_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Implements efl_object.constructor for Elm_Icon.
 * @details This is the constructor for Elm_Icon objects. It calls the parent
 * class constructor, sets the object type legacy name, registers smart
 * callbacks, and sets the accessibility role.
 *
 * @param obj The icon object being constructed.
 * @param sd The icon's private data.
 * @return The constructed Eo object.
 */
EOLIAN static Eo *
_elm_icon_efl_object_constructor(Eo *obj, Elm_Icon_Data *sd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   sd->obj = obj;

   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_ICON);

   return obj;
}

/**
 * @internal
 * @brief Class constructor for Elm_Icon.
 * @details This function is called once when the Elm_Icon class is being set up.
 * It registers the legacy "elm_icon" smart type name with the Efl_Class.
 *
 * @param klass The Efl_Class for Elm_Icon.
 */
static void
_elm_icon_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Legacy deprecated functions */

/**
 * @brief Set the icon by an image memory area.
 * @deprecated Use efl_file_set() and efl_file_key_set() for Edje files,
 *             or elm_image_memfile_set() on the result of elm_icon_object_get()
 *             for other image types.
 *
 * @param obj The icon object.
 * @param img The binary data of the image.
 * @param size The size of the binary data.
 * @param format The format of the image (e.g., "png", "jpg"). Can be NULL.
 * @param key The Edje key if the image is an Edje file. Can be NULL.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
elm_icon_memfile_set(Evas_Object *obj,
                     const void *img,
                     size_t size,
                     const char *format,
                     const char *key)
{
   ELM_ICON_CHECK(obj) EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(img, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!size, EINA_FALSE);

   ELM_ICON_DATA_GET(obj, sd);
   ELM_SAFE_FREE(sd->stdicon, eina_stringshare_del);

   _edje_signals_free(sd);

   return elm_image_memfile_set(efl_super(obj, MY_CLASS), img, size, format, key);
}

/**
 * @brief Set the icon by an image file path.
 * @deprecated Use efl_file_simple_load().
 *
 * @param obj The icon object.
 * @param file The path to the image file.
 * @param group The Edje group if the file is an Edje file. Can be NULL.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
elm_icon_file_set(Evas_Object *obj,
                  const char *file,
                  const char *group)
{
   ELM_ICON_CHECK(obj) EINA_FALSE;
   EINA_SAFETY_ON_NULL_RETURN_VAL(file, EINA_FALSE);

   return efl_file_simple_load(obj, file, group);
}

/**
 * @brief Get the file path and group of the currently set icon.
 * @deprecated Use efl_file_get() and efl_file_key_get().
 *
 * @param obj The icon object.
 * @param[out] file Pointer to store the file path.
 * @param[out] group Pointer to store the Edje group.
 */
EAPI void
elm_icon_file_get(const Evas_Object *obj,
                  const char **file,
                  const char **group)
{
   ELM_ICON_CHECK(obj);

   elm_image_file_get(obj, file, group);
}

/**
 * @brief Check if the icon's image can be animated.
 * @deprecated Use elm_image_animated_available_get() on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @return @c EINA_TRUE if animatable, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
elm_icon_animated_available_get(const Evas_Object *obj)
{
   ELM_ICON_CHECK(obj) EINA_FALSE;

   return elm_image_animated_available_get(obj);
}

/**
 * @brief Enable or disable animation for the icon.
 * @deprecated Use elm_image_animated_set() on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @param anim @c EINA_TRUE to enable animation, @c EINA_FALSE to disable.
 */
EAPI void
elm_icon_animated_set(Evas_Object *obj,
                      Eina_Bool anim)
{
   ELM_ICON_CHECK(obj);

   return elm_image_animated_set(obj, anim);
}

/**
 * @brief Get the animation state of the icon.
 * @deprecated Use elm_image_animated_get() on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @return @c EINA_TRUE if animation is enabled, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
elm_icon_animated_get(const Evas_Object *obj)
{
   ELM_ICON_CHECK(obj) EINA_FALSE;

   return elm_image_animated_get(obj);
}

/**
 * @brief Start or stop the animation of the icon.
 * @deprecated Use elm_image_animated_play_set() on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @param play @c EINA_TRUE to play, @c EINA_FALSE to stop.
 */
EAPI void
elm_icon_animated_play_set(Evas_Object *obj,
                           Eina_Bool play)
{
   ELM_ICON_CHECK(obj);

   elm_image_animated_play_set(obj, play);
}

/**
 * @brief Get the playing state of the icon's animation.
 * @deprecated Use elm_image_animated_play_get() on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @return @c EINA_TRUE if playing, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
elm_icon_animated_play_get(const Evas_Object *obj)
{
   ELM_ICON_CHECK(obj) EINA_FALSE;

   return elm_image_animated_play_get(obj);
}

/**
 * @brief Set the smooth scaling property for the icon's image.
 * @deprecated Use efl_gfx_image_smooth_scale_set() or elm_image_smooth_set()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @param smooth @c EINA_TRUE for smooth scaling, @c EINA_FALSE for non-smooth.
 */
EAPI void
elm_icon_smooth_set(Evas_Object *obj,
                    Eina_Bool smooth)
{
   ELM_ICON_CHECK(obj);

   elm_image_smooth_set(obj, smooth);
}

/**
 * @brief Get the smooth scaling property of the icon's image.
 * @deprecated Use efl_gfx_image_smooth_scale_get() or elm_image_smooth_get()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @return @c EINA_TRUE if smooth scaling is enabled, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
elm_icon_smooth_get(const Evas_Object *obj)
{
   ELM_ICON_CHECK(obj) EINA_FALSE;

   return elm_image_smooth_get(obj);
}

/**
 * @brief Set whether the icon's image should not be scaled.
 * @deprecated Use efl_gfx_image_scale_type_set() or elm_image_no_scale_set()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @param no_scale @c EINA_TRUE to disable scaling, @c EINA_FALSE to enable.
 */
EAPI void
elm_icon_no_scale_set(Evas_Object *obj,
                      Eina_Bool no_scale)
{
   ELM_ICON_CHECK(obj);

   elm_image_no_scale_set(obj, no_scale);
}

/**
 * @brief Get whether the icon's image is set to not scale.
 * @deprecated Use efl_gfx_image_scale_type_get() or elm_image_no_scale_get()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @return @c EINA_TRUE if scaling is disabled, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
elm_icon_no_scale_get(const Evas_Object *obj)
{
   ELM_ICON_CHECK(obj) EINA_FALSE;

   return elm_image_no_scale_get(obj);
}

/**
 * @brief Set the resizability of the icon's image.
 * @deprecated Use efl_gfx_image_resizable_set() or elm_image_resizable_set()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @param size_up @c EINA_TRUE if the image can be scaled up.
 * @param size_down @c EINA_TRUE if the image can be scaled down.
 */
EAPI void
elm_icon_resizable_set(Evas_Object *obj,
                       Eina_Bool size_up,
                       Eina_Bool size_down)
{
   ELM_ICON_CHECK(obj);

   elm_image_resizable_set(obj, size_up, size_down);
}

/**
 * @brief Get the resizability of the icon's image.
 * @deprecated Use efl_gfx_image_resizable_get() or elm_image_resizable_get()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @param[out] size_up Pointer to store if scaling up is allowed.
 * @param[out] size_down Pointer to store if scaling down is allowed.
 */
EAPI void
elm_icon_resizable_get(const Evas_Object *obj,
                       Eina_Bool *size_up,
                       Eina_Bool *size_down)
{
   ELM_ICON_CHECK(obj);

   elm_image_resizable_get(obj, size_up, size_down);
}

/**
 * @brief Set whether the icon's image should fill outside its boundaries.
 * @deprecated Use efl_gfx_image_fill_outside_set() or elm_image_fill_outside_set()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @param fill_outside @c EINA_TRUE to fill outside, @c EINA_FALSE otherwise.
 */
EAPI void
elm_icon_fill_outside_set(Evas_Object *obj,
                          Eina_Bool fill_outside)
{
   ELM_ICON_CHECK(obj);

   elm_image_fill_outside_set(obj, fill_outside);
}

/**
 * @brief Get whether the icon's image fills outside its boundaries.
 * @deprecated Use efl_gfx_image_fill_outside_get() or elm_image_fill_outside_get()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @return @c EINA_TRUE if filling outside, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
elm_icon_fill_outside_get(const Evas_Object *obj)
{
   ELM_ICON_CHECK(obj) EINA_FALSE;

   return elm_image_fill_outside_get(obj);
}

/**
 * @brief Get the original size of the icon's image.
 * @deprecated Use efl_gfx_entity_size_get() or elm_image_object_size_get()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @param[out] w Pointer to store the width.
 * @param[out] h Pointer to store the height.
 */
EAPI void
elm_icon_size_get(const Evas_Object *obj,
                  int *w,
                  int *h)
{
   ELM_ICON_CHECK(obj);

   elm_image_object_size_get(obj, w, h);
}

/**
 * @brief Set the prescale size for the icon's image.
 * @deprecated Use efl_gfx_image_load_size_set() or elm_image_prescale_set()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @param size The prescale size.
 */
EAPI void
elm_icon_prescale_set(Evas_Object *obj,
                      int size)
{
   ELM_ICON_CHECK(obj);

   elm_image_prescale_set(obj, size);
}

/**
 * @brief Get the prescale size of the icon's image.
 * @deprecated Use efl_gfx_image_load_size_get() or elm_image_prescale_get()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @return The prescale size.
 */
EAPI int
elm_icon_prescale_get(const Evas_Object *obj)
{
   ELM_ICON_CHECK(obj) 0;

   return elm_image_prescale_get(obj);
}

/**
 * @brief Get the internal Evas image object used by the icon.
 * @deprecated Use efl_ui_image_object_get().
 *
 * @param obj The icon object.
 * @return The internal Evas_Object (image or Edje).
 */
EAPI Evas_Object *
elm_icon_object_get(Evas_Object *obj)
{
   ELM_ICON_CHECK(obj) 0;

   return elm_image_object_get(obj);
}

/**
 * @brief Disable or enable preloading for the icon's image.
 * @deprecated Use efl_gfx_image_load_controller_set() or elm_image_preload_disabled_set()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @param disabled @c EINA_TRUE to disable preloading, @c EINA_FALSE to enable.
 */
EAPI void
elm_icon_preload_disabled_set(Evas_Object *obj,
                              Eina_Bool disabled)
{
   ELM_ICON_CHECK(obj);

   elm_image_preload_disabled_set(obj, disabled);
}

/**
 * @brief Set whether the icon's aspect ratio should be fixed.
 * @deprecated Use efl_gfx_view_keep_ratio_set() or elm_image_aspect_fixed_set()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @param fixed @c EINA_TRUE to fix aspect ratio, @c EINA_FALSE otherwise.
 */
EAPI void
elm_icon_aspect_fixed_set(Evas_Object *obj,
                          Eina_Bool fixed)
{
   ELM_ICON_CHECK(obj);

   elm_image_aspect_fixed_set(obj, fixed);
}

/**
 * @brief Get whether the icon's aspect ratio is fixed.
 * @deprecated Use efl_gfx_view_keep_ratio_get() or elm_image_aspect_fixed_get()
 *             on the result of elm_icon_object_get().
 *
 * @param obj The icon object.
 * @return @c EINA_TRUE if aspect ratio is fixed, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
elm_icon_aspect_fixed_get(const Evas_Object *obj)
{
   ELM_ICON_CHECK(obj) EINA_FALSE;

   return elm_image_aspect_fixed_get(obj);
}

EAPI void
elm_icon_thumb_set(Evas_Object *obj, const char *file, const char *group)
{
   ELM_ICON_CHECK(obj);
   ELM_ICON_DATA_GET(obj, sd);

   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_RESIZE, _elm_icon_standard_resize_cb, obj);
   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_RESIZE, _elm_icon_thumb_resize_cb, obj);

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_RESIZE, _elm_icon_thumb_resize_cb, obj);

   eina_stringshare_replace(&sd->thumb.file.path, file);
   eina_stringshare_replace(&sd->thumb.file.key, group);

   if (elm_thumb_ethumb_client_connected_get())
     {
        _icon_thumb_apply(sd);
        return;
     }

   if (!sd->thumb.eeh)
     {
        sd->thumb.eeh = ecore_event_handler_add
            (ELM_ECORE_EVENT_ETHUMB_CONNECT, _icon_thumb_apply_cb, obj);
     }
}

EAPI Eina_Bool
elm_icon_standard_set(Evas_Object *obj, const char *name)
{
   Eina_Bool fdo = EINA_FALSE;

   ELM_ICON_CHECK(obj) EINA_FALSE;

   if (!name) return EINA_FALSE;

   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_RESIZE, _elm_icon_standard_resize_cb, obj);

   Eina_Bool int_ret = _internal_elm_icon_standard_set(obj, name, &fdo);

   if (fdo)
     evas_object_event_callback_add
       (obj, EVAS_CALLBACK_RESIZE, _elm_icon_standard_resize_cb, obj);

   return int_ret;
}

EAPI const char*
elm_icon_standard_get(const Evas_Object *obj)
{
   ELM_ICON_CHECK(obj) NULL;
   ELM_ICON_DATA_GET(obj, sd);

   return sd->stdicon;
}

EAPI void
elm_icon_order_lookup_set(Evas_Object *obj EINA_UNUSED,
                           Elm_Icon_Lookup_Order order EINA_UNUSED)
{
   // this method's behaviour has been overridden by elm_config_icon_theme_set
}

EAPI Elm_Icon_Lookup_Order
elm_icon_order_lookup_get(const Evas_Object *obj EINA_UNUSED)
{
   return ELM_ICON_LOOKUP_FDO_THEME;
}

/* Internal EO APIs and hidden overrides */

#define ELM_ICON_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_icon)

#include "elm_icon_eo.c"
