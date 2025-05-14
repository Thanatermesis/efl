#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED

#include <Elementary.h>
#include "Eio.h"
#include "elm_priv.h"
#include "elm_fileselector_button_eo.h"
#include "elm_fileselector_entry_eo.h"
#include "elm_interface_fileselector.h"
#include "elm_widget_fileselector_button.h"
#include "elm_fileselector_eo.h"

#define MY_CLASS ELM_FILESELECTOR_BUTTON_CLASS

#define MY_CLASS_NAME "Elm_Fileselector_Button"
#define MY_CLASS_NAME_LEGACY "elm_fileselector_button"

/* FIXME: need a way to find a gap between the size of item and thumbnail */
#define GENGRID_PADDING 16

#define DEFAULT_WINDOW_TITLE "Select a file"

#define ELM_PRIV_FILESELECTOR_BUTTON_SIGNALS(cmd) \
   cmd(SIG_FILE_CHOSEN, "file,chosen", "s") \

ELM_PRIV_FILESELECTOR_BUTTON_SIGNALS(ELM_PRIV_STATIC_VARIABLE_DECLARE);

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   ELM_PRIV_FILESELECTOR_BUTTON_SIGNALS(ELM_PRIV_SMART_CALLBACKS_DESC)
   {SIG_WIDGET_LANG_CHANGED, ""}, /**<handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**<handled by elm_widget */
   {SIG_LAYOUT_FOCUSED, ""}, /**< handled by elm_layout */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< handled by elm_layout */
   {NULL, NULL}
};
#undef ELM_PRIV_FILESELECTOR_BUTTON_SIGNALS

/**
 * @internal
 * @brief Applies the theme to the fileselector button.
 *
 * This function temporarily modifies the widget style to "fileselector_button/STYLE"
 * before calling the parent's theme_apply. This allows the button to use
 * specific theme elements intended for fileselector buttons, rather than
 * generic button styles. The original style is restored afterward.
 */
EOLIAN static Eina_Error
_elm_fileselector_button_efl_ui_widget_theme_apply(Eo *obj, Elm_Fileselector_Button_Data *sd EINA_UNUSED)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;

   char buf[4096];
   const char *style;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EFL_UI_THEME_APPLY_ERROR_GENERIC);

   style = eina_stringshare_add(elm_widget_style_get(obj));

   snprintf(buf, sizeof(buf), "fileselector_button/%s", style);

   /* file selector button's style has an extra bit */
   eina_stringshare_replace(&(wd->style), buf);

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   eina_stringshare_replace(&(wd->style), style);

   eina_stringshare_del(style);

   return int_ret;
}

/**
 * @internal
 * @brief Callback function invoked when file selection is complete or cancelled.
 *
 * This function is called when the internal fileselector widget emits a "done"
 * event (file chosen or dialog closed) or when the fileselector window
 * receives a delete request.
 *
 * It retrieves the selected file model from the fileselector, updates the
 * button's internal path and model, emits the "file,chosen" signal (both
 * Efl event and legacy smart callback), and then cleans up by deleting
 * the fileselector widget and its window.
 * If no file was selected, it emits "file,chosen" with a NULL model/path.
 *
 * @param data The Elm_Fileselector_Button_Data structure.
 * @param event The Efl_Event data (unused here).
 */
static void
_selection_done(void *data, const Efl_Event *event EINA_UNUSED)
{
   Elm_Fileselector_Button_Data *sd = data;
   Efl_Model *model;;
   Evas_Object *del;

   model = elm_interface_fileselector_selected_model_get(sd->fs);
   if (model)
     {
        Eina_Value *path;
        char *file;

        efl_replace(&sd->fsd.model, model);

        path = efl_model_property_get(model, "path");
        file = eina_value_to_string(path);
        eina_stringshare_replace(&sd->fsd.path, file);

        efl_event_callback_call
          (sd->obj, ELM_FILESELECTOR_BUTTON_EVENT_FILE_CHOSEN, model);
        _event_to_legacy_call
          (sd->obj, ELM_FILESELECTOR_BUTTON_EVENT_FILE_CHOSEN->name, file);

        eina_value_free(path);
        free(file);
     }
   else
     {
        _model_event_call
          (sd->obj, ELM_FILESELECTOR_BUTTON_EVENT_FILE_CHOSEN, ELM_FILESELECTOR_BUTTON_EVENT_FILE_CHOSEN->name, NULL, NULL);
     }
   eina_stringshare_replace(&sd->fsd.current_name, elm_interface_fileselector_current_name_get(sd->fs));
   del = sd->fsw;
   sd->fs = NULL;
   sd->fsw = NULL;
   evas_object_del(del);
}

/**
 * @internal
 * @brief Creates and configures a new window to host the fileselector.
 *
 * This window is a basic dialog window, titled with the button's
 * configured window title (e.g., "Select a file"). It's set to auto-delete
 * upon a delete request and has a background. The `_selection_done` callback
 * is attached to its EFL_UI_WIN_EVENT_DELETE_REQUEST event to handle closure.
 *
 * @param sd The Elm_Fileselector_Button_Data structure containing window settings.
 * @return The newly created Evas_Object (the window).
 */
static Evas_Object *
_new_window_add(Elm_Fileselector_Button_Data *sd)
{
   Evas_Object *win, *bg;

   win = elm_win_add(NULL, "fileselector_button", ELM_WIN_DIALOG_BASIC);
   elm_win_title_set(win, sd->window_title);
   elm_win_autodel_set(win, EINA_TRUE);
   efl_event_callback_add
         (win, EFL_UI_WIN_EVENT_DELETE_REQUEST, _selection_done, sd);

   bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   evas_object_resize(win, sd->w, sd->h);
   return win;
}

/**
 * @internal
 * @brief Retrieves the parent window of a given Evas_Object.
 *
 * Traverses up the widget hierarchy from @p obj using
 * `elm_object_parent_widget_get` until an object that is an
 * `EFL_UI_WIN_CLASS` is found. This is used for inwin mode.
 *
 * @param obj The Evas_Object from which to start searching for a parent window.
 * @return The parent window object (Efl_Ui_Win), or potentially the object
 *         itself if it's already a window. Returns NULL if traversal fails
 *         to find a window (which would be unusual in a typical UI).
 */
static Evas_Object *
_parent_win_get(Evas_Object *obj)
{
   while (!efl_isa(obj, EFL_UI_WIN_CLASS))
     obj = elm_object_parent_widget_get(obj);

   return obj;
}

/**
 * @internal
 * @brief Activates the fileselector UI, creating and showing the fileselector.
 *
 * This function is the core logic for displaying the fileselector dialog.
 * If a fileselector instance (`sd->fs`) associated with this button already
 * exists (i.e., the dialog is already open), it does nothing.
 *
 * It decides whether to use an "inwin" mode (embedding the fileselector
 * within the button's current window) or a new, separate dialog window.
 * This decision is based on `sd->inwin_mode` and the global Elementary
 * configuration (`_elm_config->inwin_dialogs_enable`).
 *
 * A new `elm_fileselector` widget is created and configured with the
 * button's current settings (e.g., path, folder_only, is_save, hidden_visible).
 * The `_selection_done` callback is attached to the fileselector's "done" event
 * to handle when the user makes a choice or closes the dialog.
 * Finally, the fileselector (and its containing window/inwin) is shown.
 *
 * @param sd The Elm_Fileselector_Button_Data structure.
 */
static void
_activate(Elm_Fileselector_Button_Data *sd)
{
   Eina_Bool is_inwin = EINA_FALSE;

   if (sd->fs) return;

   if (sd->inwin_mode)
     {
        sd->fsw = _parent_win_get(sd->obj);

        if (!sd->fsw)
          sd->fsw = _new_window_add(sd);
        else
          {
             sd->fsw = elm_win_inwin_add(sd->fsw);
             is_inwin = EINA_TRUE;
          }
     }
   else
     sd->fsw = _new_window_add(sd);

   sd->fs = elm_fileselector_add(sd->fsw);
   elm_fileselector_hidden_visible_set(sd->fs, sd->fsd.hidden_visible);
   efl_ui_mirrored_set
     (sd->fs, efl_ui_mirrored_get(sd->obj));
   efl_ui_mirrored_automatic_set(sd->fs, EINA_FALSE);
   elm_fileselector_expandable_set(sd->fs, sd->fsd.expandable);
   elm_fileselector_folder_only_set(sd->fs, sd->fsd.folder_only);
   elm_fileselector_is_save_set(sd->fs, sd->fsd.is_save);
   elm_interface_fileselector_selected_model_set(sd->fs, sd->fsd.model);
   evas_object_size_hint_weight_set
     (sd->fs, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(sd->fs, EVAS_HINT_FILL, EVAS_HINT_FILL);
   efl_event_callback_add
     (sd->fs, ELM_FILESELECTOR_EVENT_DONE, _selection_done, sd);
   evas_object_show(sd->fs);

   if (is_inwin)
     {
        elm_win_inwin_content_set(sd->fsw, sd->fs);
        elm_win_inwin_activate(sd->fsw);
     }
   else
     {
        elm_win_resize_object_add(sd->fsw, sd->fs);
        evas_object_show(sd->fsw);
     }
}

static void
_button_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _activate(data);
}

/**
 * @internal
 * @brief Callback for the "clicked" smart event on the fileselector button.
 *
 * This function is invoked when the user clicks the fileselector button.
 * Its sole purpose is to call `_activate` to display the fileselector UI.
 *
 * @param data The Elm_Fileselector_Button_Data structure, passed as callback data.
 * @param obj The Evas_Object that was clicked (the button itself), unused here.
 * @param event_info Event-specific information for the click, unused here.
 */
static void _noref_death(void *data EINA_UNUSED, const Efl_Event *event);
static void _invalidated(void *data EINA_UNUSED, const Efl_Event *event);

EFL_CALLBACKS_ARRAY_DEFINE(noref_death,
                           { EFL_EVENT_NOREF, _noref_death },
                           { EFL_EVENT_INVALIDATE, _invalidated });

/**
 * @internal
 * @brief Callback for EFL_EVENT_NOREF on the internal Efl_Io_Model (`sd->fsd.model`).
 *
 * This function is invoked when the reference count of the `sd->fsd.model`
 * (an Efl_Io_Model instance representing the current path/selection for the button)
 * drops to zero. This signifies that the model object is no longer referenced
 * by the button or any other part of the system and can be safely deleted.
 *
 * It first removes its own callback group (`noref_death`) from the event object
 * to prevent re-entry or issues during deletion, and then calls `efl_del()`
 * on the model object to free it. This is crucial for managing the lifecycle
 * of the model created in `_elm_fileselector_button_efl_canvas_group_group_add`
 * and potentially in `_elm_fileselector_button_path_set_internal`.
 *
 * @param data User data associated with the event (unused).
 * @param event The Efl_Event structure; `event->object` is the Efl_Io_Model.
 */
static void
_noref_death(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_event_callback_array_del(event->object, noref_death(), NULL);
   efl_del(event->object);
}

/**
 * @internal
 * @brief Callback for EFL_EVENT_INVALIDATE on the internal Efl_Io_Model (`sd->fsd.model`).
 *
 * This function is invoked when the `sd->fsd.model` is invalidated. This can
 * happen if, for example, its underlying provider (like Efl.Io.File) determines
 * the path is no longer valid or the object is being forcibly destroyed as part
 * of a larger cleanup.
 *
 * It removes the `noref_death` callback group (which includes both `_noref_death`
 * and `_invalidated` itself) from the model. This is a safety measure because
 * an `EFL_EVENT_NOREF` might still be triggered after `EFL_EVENT_INVALIDATE`
 * on an object that is already in an invalid state. Removing the callbacks
 * prevents `_noref_death` from potentially operating on an invalid object.
 * The object itself is not deleted here; `_noref_death` is responsible for
 * deletion if it's triggered by the reference count reaching zero.
 *
 * @param data User data associated with the event (unused).
 * @param event The Efl_Event structure; `event->object` is the Efl_Io_Model.
 */
static void
_invalidated(void *data EINA_UNUSED, const Efl_Event *event)
{
   // This means our parent is dying, EFL_EVENT_NOREF can be called after invalidated
   efl_event_callback_array_del(event->object, noref_death(), NULL);
}

/**
 * @internal
 * @brief Initializes the fileselector button when it's added to the canvas.
 *
 * This function is called as part of the Efl object construction lifecycle,
 * specifically when the widget is added to a canvas group (typically during `efl_add`).
 * It performs essential initial setup for the fileselector button:
 * - Calls the superclass's `efl_canvas_group_add` function.
 * - Sets a default window title (e.g., "Select a file").
 * - Initializes the default path (`priv->fsd.path`) to the user's home
 *   directory, or to "/" if the home directory cannot be determined.
 * - Creates an Efl_Io_Model (`priv->fsd.model`) representing this initial path.
 *   This model is `efl_add_ref`ed, meaning the button itself holds a reference.
 *   Lifecycle callbacks (`noref_death` and `_invalidated`) are attached to this
 *   model to manage its deletion when it's no longer referenced (e.g., when
 *   the button is destroyed or the model is replaced).
 * - Sets default configuration values based on `_elm_config` (e.g.,
 *   `fileselector_expand_enable` for expandable directories,
 *   `inwin_dialogs_enable` for the inwin mode preference).
 * - Sets default window dimensions (width, height) for the fileselector dialog.
 * - Disables automatic UI mirroring for the button widget itself (mirroring for
 *   the internal fileselector is handled separately when `_activate`d).
 * - Registers the `_button_clicked` function as a smart callback for the
 *   "clicked" event of the button.
 * - Applies the widget's theme using `efl_ui_widget_theme_apply`.
 * - Makes the widget focusable via `elm_widget_can_focus_set`.
 */
EOLIAN static void
_elm_fileselector_button_efl_canvas_group_group_add(Eo *obj, Elm_Fileselector_Button_Data *priv)
{
   const char *path;

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   priv->window_title = eina_stringshare_add(DEFAULT_WINDOW_TITLE);
   path = eina_environment_home_get();
   if (path) priv->fsd.path = eina_stringshare_add(path);
   else priv->fsd.path = eina_stringshare_add("/");

   priv->fsd.model = efl_add_ref(EFL_IO_MODEL_CLASS, obj,
                                 efl_io_model_path_set(efl_added, priv->fsd.path),
                                 efl_event_callback_array_add(efl_added, noref_death(), NULL));

   priv->fsd.expandable = _elm_config->fileselector_expand_enable;
   priv->inwin_mode = _elm_config->inwin_dialogs_enable;
   priv->w = 400;
   priv->h = 400;

   efl_ui_mirrored_automatic_set(obj, EINA_FALSE);

   evas_object_smart_callback_add(obj, "clicked", _button_clicked, priv);


   efl_ui_widget_theme_apply(obj);
   elm_widget_can_focus_set(obj, EINA_TRUE);
}

/**
 * @internal
 * @brief Cleans up the fileselector button when it's removed from the canvas or destroyed.
 *
 * This function is called as part of the Efl object destruction lifecycle,
 * specifically when the widget is deleted from a canvas group (e.g., via `efl_del`).
 * It is responsible for releasing all resources acquired by the widget:
 * - Calls `efl_replace` to set `sd->fsd.model` to NULL. This decrements the
 *   reference count of the current Efl_Io_Model. If this was the last reference,
 *   it will trigger the `_noref_death` callback, leading to the model's deletion.
 * - Deletes various stringshares used for storing the window title, current path
 *   (`sd->fsd.path`), and the path of the last selected item (`sd->fsd.selection_path`).
 * - Unreferences the Efl_Model for the last selected item (`sd->fsd.selection`)
 *   using `efl_unref`.
 * - Deletes the fileselector window (`sd->fsw`) if it exists. If the internal
 *   fileselector widget (`sd->fs`) was part of this window (i.e., not an inwin
 *   content of a user-provided window), it will also be deleted.
 * - Calls the superclass's `efl_canvas_group_del` function for further cleanup.
 */
EOLIAN static void
_elm_fileselector_button_efl_canvas_group_group_del(Eo *obj, Elm_Fileselector_Button_Data *sd)
{
   efl_replace(&sd->fsd.model, NULL);
   eina_stringshare_del(sd->window_title);
   eina_stringshare_del(sd->fsd.path);
   if (sd->fsd.selection)
     efl_unref(sd->fsd.selection);
   eina_stringshare_del(sd->fsd.selection_path);
   evas_object_del(sd->fsw);

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Overrides the Efl_Ui_Autorepeat.autorepeat_enabled_set behavior.
 *
 * Fileselector buttons do not support autorepeat functionality (where an
 * action is repeatedly triggered if the button is held down). This function
 * ensures that autorepeat is always disabled for this widget type.
 * If an attempt is made to enable it (`enabled` is EINA_TRUE), an error
 * message is printed to the console, and the call to the superclass's
 * `efl_ui_autorepeat_enabled_set` explicitly passes EINA_FALSE to disable it.
 *
 * @param obj The fileselector button object (unused).
 * @param sd The private data of the fileselector button (unused).
 * @param enabled The desired autorepeat state. This is effectively ignored if true,
 *                as autorepeat will be forced off.
 */
EOLIAN static void
_elm_fileselector_button_efl_ui_autorepeat_autorepeat_enabled_set(const Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd EINA_UNUSED, Eina_Bool enabled)
{
   if (enabled)
     ERR("You cannot enable autorepeat on this object");
   efl_ui_autorepeat_enabled_set(efl_super(obj, MY_CLASS), EINA_FALSE);
}

EAPI Evas_Object *
elm_fileselector_button_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Constructor for the Elm_Fileselector_Button Efl object.
 *
 * This function is called when a new fileselector button object is being
 * constructed (typically as the first step in `efl_add`).
 * It performs essential Efl object initialization:
 * - Calls the superclass's constructor (`efl_constructor`).
 * - Stores a pointer to the Eo object (`obj`) in the widget's private data
 *   structure (`sd->obj`) for easy access within other widget functions.
 * - Explicitly disables autorepeat functionality by calling
 *   `efl_ui_autorepeat_enabled_set(obj, EINA_FALSE)`.
 * - Sets the legacy Evas object type name using `efl_canvas_object_type_set`.
 * - Registers the smart callback descriptions (`_smart_callbacks`) for the widget.
 * - Sets the accessibility role to `EFL_ACCESS_ROLE_PUSH_BUTTON`.
 * - Initializes legacy focus handling mechanisms.
 *
 * @param obj The Eo object being constructed.
 * @param sd The private data structure (Elm_Fileselector_Button_Data) for the widget.
 * @return The constructed Eo object (usually the same as the input `obj`).
 */
EOLIAN static Eo *
_elm_fileselector_button_efl_object_constructor(Eo *obj, Elm_Fileselector_Button_Data *sd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   sd->obj = obj;

   efl_ui_autorepeat_enabled_set(obj, EINA_FALSE);
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_PUSH_BUTTON);
   legacy_object_focus_handle(obj);

   return obj;
}

EAPI void
elm_fileselector_button_window_title_set(Eo *obj, const char *title)
{
   ELM_FILESELECTOR_BUTTON_CHECK(obj);
   ELM_FILESELECTOR_BUTTON_DATA_GET_OR_RETURN(obj, sd);
   eina_stringshare_replace(&sd->window_title, title);
   if (sd->fsw) elm_win_title_set(sd->fsw, sd->window_title);
}

EAPI const char *
elm_fileselector_button_window_title_get(const Eo *obj)
{
   ELM_FILESELECTOR_BUTTON_CHECK(obj) NULL;
   ELM_FILESELECTOR_BUTTON_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);
   return sd->window_title;
}

EAPI void
elm_fileselector_button_window_size_set(Eo *obj, Evas_Coord width, Evas_Coord height)
{
   ELM_FILESELECTOR_BUTTON_CHECK(obj);
   ELM_FILESELECTOR_BUTTON_DATA_GET_OR_RETURN(obj, sd);
   sd->w = width;
   sd->h = height;
   if (sd->fsw) evas_object_resize(sd->fsw, sd->w, sd->h);
}

EAPI void
elm_fileselector_button_window_size_get(const Eo *obj, Evas_Coord *width, Evas_Coord *height)
{
   if (width) *width = 0;
   if (height) *height = 0;
   ELM_FILESELECTOR_BUTTON_CHECK(obj);
   ELM_FILESELECTOR_BUTTON_DATA_GET_OR_RETURN(obj, sd);
   if (width) *width = sd->w;
   if (height) *height = sd->h;
}

/**
 * @internal
 * @brief Internal function to set the current path for the fileselector button.
 *
 * This function updates the primary path associated with the fileselector button.
 * It creates a new Efl_Io_Model instance for the given @p path. This new model
 * then replaces the button's existing current model (`sd->fsd.model`).
 * The stringshare `sd->fsd.path` is also updated to reflect the new path.
 *
 * If an active fileselector widget (`sd->fs`, i.e., the dialog is open) exists,
 * its selected model is also updated to this new path, effectively changing
 * the directory or pre-selecting a file in the active dialog.
 *
 * @note The existing comment `// XXX: the efl_ref here smells wrong...` within
 * the function points to a potential issue with the reference counting of the
 * model created here. Specifically, `efl_add(EFL_IO_MODEL_CLASS, ...)` creates
 * a model with refcount 1. `efl_replace` correctly handles refs for `sd->fsd.model`.
 * The concern might be related to how this model's lifecycle is managed if
 * `_elm_fileselector_button_efl_canvas_group_group_add` also creates a model.
 * However, `sd->fsd.model` is consistently managed with `efl_replace` and
 * `noref_death` callbacks.
 *
 * @param obj The fileselector button Evas_Object.
 * @param path The new path to set. Example: "/home/user/Pictures".
 */
void
_elm_fileselector_button_path_set_internal(Evas_Object *obj, const char *path)
{
   ELM_FILESELECTOR_BUTTON_DATA_GET_OR_RETURN(obj, sd);

   Efl_Model *model = efl_add(EFL_IO_MODEL_CLASS, obj, efl_io_model_path_set(efl_added, path));
   if (!model)
     {
        ERR("Efl.Model allocation error");
        return;
     }

   // XXX: the efl_ref here smells wrong. fsd.model is only unreffed ONCE so this obj leaks...
   efl_replace(&sd->fsd.model, model);

   eina_stringshare_replace(&sd->fsd.path, path);

   if (sd->fs) elm_interface_fileselector_selected_model_set(sd->fs, model);
}

EINA_DEPRECATED EAPI void
elm_fileselector_button_path_set(Evas_Object *obj, const char *path)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj);
   elm_fileselector_path_set(obj, path);
}

/**
 * @internal
 * @brief Implements Efl_Ui_View.model_set for the fileselector button.
 *
 * This function sets the underlying data model for the button. The model is
 * expected to be an `EFL_IO_MODEL_CLASS` instance, representing the file or
 * directory that the button should currently point to.
 *
 * It updates the internal `sd->fsd.model` by replacing it with the new @p model.
 * It also extracts the path string from the @p model and updates `sd->fsd.path`.
 * A legacy "file,chosen" event is emitted with the path from the new model.
 * If an active fileselector dialog (`sd->fs`) is open, its selection is also
 * updated to this new model.
 *
 * @param obj The fileselector button Eo object (unused).
 * @param sd The private data (Elm_Fileselector_Button_Data) of the button.
 * @param model The Efl_Model to set. This should be an Efl_Io_Model.
 *              Example: An Efl_Io_Model instance pointing to "/etc/hostname".
 */
EOLIAN static void
_elm_fileselector_button_efl_ui_view_model_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd, Efl_Model *model)
{
   char *file = NULL;

   if (!efl_isa(model, EFL_IO_MODEL_CLASS))
     return ;

   efl_replace(&sd->fsd.model, model);

   if (model)
     {
        Eina_Value *path;

        path = efl_model_property_get(model, "path");
        file = eina_value_to_string(path);
        eina_value_free(path);
     }

   eina_stringshare_replace(&sd->fsd.path, file);

   _event_to_legacy_call
     (sd->obj, ELM_FILESELECTOR_BUTTON_EVENT_FILE_CHOSEN->name, file);

   free(file);

   if (sd->fs) elm_interface_fileselector_selected_model_set(sd->fs, model);
}

/**
 * @internal
 * @brief Internal function to get the current path of the fileselector button.
 *
 * This retrieves the path currently associated with the button, which is
 * typically the directory the fileselector will open in, or the last
 * path set/selected.
 *
 * @param obj The fileselector button Evas_Object.
 * @return A stringshared const char* representing the current path.
 *         Example: "/var/log".
 *         Returns NULL if the object is invalid or its private data cannot be retrieved.
 */
const char *
_elm_fileselector_button_path_get_internal(const Evas_Object *obj)
{
   ELM_FILESELECTOR_BUTTON_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);
   return sd->fsd.path;
}

EINA_DEPRECATED EAPI const char *
elm_fileselector_button_path_get(const Evas_Object *obj)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj, NULL);
   return elm_fileselector_path_get(obj);
}

/**
 * @internal
 * @brief Implements Efl_Ui_View.model_get for the fileselector button.
 *
 * Retrieves the primary Efl_Model associated with the button. This model
 * typically represents the current path or the last selected item if no
 * specific selection model (`sd->fsd.selection`) is set differently.
 *
 * @param obj The fileselector button Eo object (unused).
 * @param sd The private data (Elm_Fileselector_Button_Data) of the button.
 * @return The current Efl_Io_Model (`sd->fsd.model`) associated with the button's path.
 *         Example: An Efl_Io_Model instance.
 */
EOLIAN static Efl_Model *
_elm_fileselector_button_efl_ui_view_model_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd)
{
   return sd->fsd.model;
}

EINA_DEPRECATED EAPI void
elm_fileselector_button_expandable_set(Evas_Object *obj,
                                       Eina_Bool value)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj);
   elm_interface_fileselector_expandable_set(obj, value);
}

/**
 * @internal
 * @brief Implements Elm_Interface_Fileselector.expandable_set.
 *
 * Sets whether the fileselector view should allow expanding/collapsing
 * of directories (e.g., in a tree view mode).
 * The @p value is stored in `sd->fsd.expandable`. If an internal fileselector
 * widget (`sd->fs`) is currently active, this setting is also applied to it directly.
 *
 * @param obj The fileselector button object (unused).
 * @param sd Private data of the fileselector button.
 * @param value EINA_TRUE to enable expandable folders, EINA_FALSE otherwise.
 */
EOLIAN static void
_elm_fileselector_button_elm_interface_fileselector_expandable_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd, Eina_Bool value)
{
   sd->fsd.expandable = value;

   if (sd->fs) elm_fileselector_expandable_set(sd->fs, sd->fsd.expandable);
}

EINA_DEPRECATED EAPI Eina_Bool
elm_fileselector_button_expandable_get(const Evas_Object *obj)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj, EINA_FALSE);
   Eina_Bool ret = EINA_FALSE;
   ret = elm_interface_fileselector_expandable_get((Eo *) obj);
   return ret;
}

EOLIAN static Eina_Bool
_elm_fileselector_button_elm_interface_fileselector_expandable_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd)
{
   return sd->fsd.expandable;
}

EINA_DEPRECATED EAPI void
elm_fileselector_button_folder_only_set(Evas_Object *obj,
                                        Eina_Bool value)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj);
   elm_interface_fileselector_folder_only_set(obj, value);
}

/**
 * @internal
 * @brief Implements Elm_Interface_Fileselector.folder_only_set.
 *
 * Sets whether the fileselector should only allow selection of folders.
 * The @p value is stored in `sd->fsd.folder_only`. If an internal fileselector
 * widget (`sd->fs`) is currently active, this setting is also applied to it.
 *
 * @param obj The fileselector button object (unused).
 * @param sd Private data of the fileselector button.
 * @param value EINA_TRUE to allow only folder selection, EINA_FALSE otherwise.
 */
EOLIAN static void
_elm_fileselector_button_elm_interface_fileselector_folder_only_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd, Eina_Bool value)
{
   sd->fsd.folder_only = value;

   if (sd->fs) elm_fileselector_folder_only_set(sd->fs, sd->fsd.folder_only);
}

EINA_DEPRECATED EAPI Eina_Bool
elm_fileselector_button_folder_only_get(const Evas_Object *obj)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj, EINA_FALSE);
   Eina_Bool ret = EINA_FALSE;
   ret = elm_interface_fileselector_folder_only_get((Eo *) obj);
   return ret;
}

EOLIAN static Eina_Bool
_elm_fileselector_button_elm_interface_fileselector_folder_only_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd)
{
   return sd->fsd.folder_only;
}

EINA_DEPRECATED EAPI void
elm_fileselector_button_is_save_set(Evas_Object *obj,
                                    Eina_Bool value)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj);
   elm_interface_fileselector_is_save_set(obj, value);
}

EOLIAN static void
_elm_fileselector_button_elm_interface_fileselector_is_save_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd, Eina_Bool value)
{
   sd->fsd.is_save = value;

   if (sd->fs) elm_fileselector_is_save_set(sd->fs, sd->fsd.is_save);
}

EINA_DEPRECATED EAPI Eina_Bool
elm_fileselector_button_is_save_get(const Evas_Object *obj)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj, EINA_FALSE);
   Eina_Bool ret = EINA_FALSE;
   ret = elm_interface_fileselector_is_save_get((Eo *) obj);
   return ret;
}

EOLIAN static Eina_Bool
_elm_fileselector_button_elm_interface_fileselector_is_save_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd)
{
   return sd->fsd.is_save;
}

EOLIAN static void
_elm_fileselector_button_elm_interface_fileselector_mode_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd, Elm_Fileselector_Mode mode)
{
   sd->fsd.mode = mode;

   if (sd->fs) elm_fileselector_mode_set(sd->fs, mode);
}

EOLIAN static Elm_Fileselector_Mode
_elm_fileselector_button_elm_interface_fileselector_mode_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd)
{
   return sd->fsd.mode;
}

EOLIAN static void
_elm_fileselector_button_elm_interface_fileselector_sort_method_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd, Elm_Fileselector_Sort sort)
{
   sd->fsd.sort_type = sort;

   if (sd->fs) elm_fileselector_sort_method_set(sd->fs, sort);
}

EOLIAN static Elm_Fileselector_Sort
_elm_fileselector_button_elm_interface_fileselector_sort_method_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd)
{
   return sd->fsd.sort_type;
}

EOLIAN static void
_elm_fileselector_button_elm_interface_fileselector_multi_select_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd, Eina_Bool value)
{
   sd->fsd.multi = value;

   if (sd->fs) elm_fileselector_multi_select_set(sd->fs, sd->fsd.multi);
}

EOLIAN static Eina_Bool
_elm_fileselector_button_elm_interface_fileselector_multi_select_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd)
{
   return sd->fsd.multi;
}

/**
 * @internal
 * @brief Internal function to get the list of selected paths.
 *
 * This function is primarily used when multi-selection is enabled.
 * If an internal fileselector widget (`sd->fs`) is currently active (i.e.,
 * the fileselector dialog is open), this function calls
 * `elm_fileselector_selected_paths_get()` on it to retrieve the list
 * of all currently selected paths.
 *
 * If no fileselector is active, it returns NULL, as the button itself
 * (when the dialog is closed) typically represents a single selection or path.
 *
 * @param obj The fileselector button Evas_Object.
 * @return A const Eina_List of (const char *) strings, where each string is a
 *         selected path. The list and its contents are owned by the internal
 *         fileselector and should not be freed or modified by the caller.
 *         The list is valid only as long as the selection in the active
 *         fileselector does not change.
 *         Example list structure if files "file1.txt" and "dir/file2.png" are selected:
 *         `("path/to/file1.txt", "path/to/dir/file2.png")`
 *         Returns NULL if no fileselector is active, if multi-selection is not
 *         relevant, or if the object is invalid.
 */
const Eina_List *
_elm_fileselector_button_selected_paths_get_internal(const Evas_Object *obj)
{
   ELM_FILESELECTOR_BUTTON_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   if (sd->fs) return elm_fileselector_selected_paths_get(sd->fs);

   return NULL;
}

EOLIAN static const Eina_List*
_elm_fileselector_button_elm_interface_fileselector_selected_models_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd)
{
   if (sd->fs) return elm_interface_fileselector_selected_models_get(sd->fs);

   return NULL;
}

/**
 * @internal
 * @brief Internal function to get the currently selected single file path.
 *
 * If an internal fileselector widget (`sd->fs`) is active (dialog is open),
 * this function queries that fileselector for its currently selected path
 * using `elm_fileselector_selected_get()`.
 *
 * If no fileselector is active (dialog is closed), it returns the path
 * stored from the last confirmed selection (`sd->fsd.selection_path`).
 * This allows retrieving the path of the file that was chosen even after
 * the fileselector dialog has been dismissed.
 *
 * @param obj The fileselector button Evas_Object.
 * @return A stringshared const char* representing the selected file path.
 *         Example: "/home/user/documents/report.pdf".
 *         Returns NULL if no path is selected, or if the object is invalid.
 */
const char *
_elm_fileselector_button_selected_get_internal(const Evas_Object *obj)
{
   ELM_FILESELECTOR_BUTTON_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   if (sd->fs) return elm_fileselector_selected_get(sd->fs);

   return sd->fsd.selection_path;
}

/**
 * @internal
 * @brief Implements Elm_Interface_Fileselector.selected_model_get.
 *
 * Retrieves the Efl_Model for the currently selected item.
 * If an internal fileselector widget (`sd->fs`) is active (dialog is open),
 * this function queries that fileselector for its currently selected model using
 * `elm_interface_fileselector_selected_model_get()`.
 *
 * If no fileselector is active (dialog is closed), it returns the model stored
 * from the last confirmed selection (`sd->fsd.selection`). This allows retrieving
 * the model of the file that was chosen even after the dialog is dismissed.
 *
 * @param obj The fileselector button Eo object (unused).
 * @param sd The private data (Elm_Fileselector_Button_Data) of the button.
 * @return The Efl_Model for the selected item. This is typically an Efl_Io_Model.
 *         Example: An Efl_Io_Model instance for "/path/to/chosen_file.dat".
 *         Returns NULL if no model is selected or available.
 */
EOLIAN static Efl_Model *
_elm_fileselector_button_elm_interface_fileselector_selected_model_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd)
{
   if (sd->fs) return elm_interface_fileselector_selected_model_get(sd->fs);

   return sd->fsd.selection;
}

/**
 * @internal
 * @brief Internal function to set the currently selected file path.
 *
 * This function attempts to set the selection of the fileselector button
 * to the given @_path.
 *
 * If an internal fileselector widget (`sd->fs`) is active (dialog is open),
 * it calls `elm_fileselector_selected_set()` on it to update the selection
 * within the live dialog.
 *
 * Regardless of the active fileselector's state, it performs a basic validation:
 * it checks if the @_path (after resolving symbolic links) points to an existing
 * file or directory. If this check fails, the function returns EINA_FALSE.
 *
 * The `sd->fsd.selection_path` (stringshared) is updated to store this new
 * selection path if validation passes.
 *
 * @param obj The fileselector button Evas_Object.
 * @param _path The path to set as selected. Example: "/usr/share/pixmaps/image.png".
 * @return EINA_TRUE on success (path is valid and, if applicable, set in the
 *         active fileselector), EINA_FALSE on failure (e.g., path does not exist,
 *         or the internal fileselector failed to set it).
 */
Eina_Bool
_elm_fileselector_button_selected_set_internal(Evas_Object *obj, const char *_path)
{
   ELM_FILESELECTOR_BUTTON_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);
   Eina_Bool ret = EINA_TRUE;

   if (sd->fs) ret = elm_fileselector_selected_set(sd->fs, _path);
   else
     {
        char *path = ecore_file_realpath(_path);
        if (!ecore_file_is_dir(path) && !ecore_file_exists(path))
          {
             free(path);
             return EINA_FALSE;
          }
        free(path);
     }

   eina_stringshare_replace(&sd->fsd.selection_path, _path);

   return ret;
}

/**
 * @internal
 * @brief Implements Elm_Interface_Fileselector.selected_model_set.
 *
 * Sets the selected item for the fileselector button using an Efl_Model.
 * If an internal fileselector widget (`sd->fs`) is currently active (dialog open),
 * this function calls `elm_interface_fileselector_selected_model_set()` on it
 * to update the selection within the live dialog.
 *
 * The provided @p model is also stored internally in `sd->fsd.selection`
 * (replacing any previous selection model) to represent the button's current
 * selected item, even if the dialog is not open.
 *
 * @param obj The fileselector button Eo object (unused).
 * @param sd The private data (Elm_Fileselector_Button_Data) of the button.
 * @param model The Efl_Model to set as selected. This should typically be an
 *              Efl_Io_Model instance. Example: An Efl_Io_Model for "/opt/app/data.bin".
 * @return Always returns EINA_TRUE. Note: The success of setting the model
 *         on an active `sd->fs` is determined by the underlying fileselector's
 *         implementation; this function itself doesn't propagate that specific status,
 *         but it will update `sd->fsd.selection` regardless.
 */
EOLIAN static Eina_Bool
_elm_fileselector_button_elm_interface_fileselector_selected_model_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd, Efl_Model *model)
{
   if (sd->fs)
     elm_interface_fileselector_selected_model_set(sd->fs, model);

   efl_replace(&sd->fsd.selection, model);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Implements Elm_Interface_Fileselector.thumbnail_size_set.
 *
 * Sets the preferred size (width and height) for thumbnails displayed in
 * the fileselector (e.g., in an icon view mode).
 *
 * If an active fileselector widget (`sd->fs`) exists, its thumbnail size is
 * updated directly using `elm_fileselector_thumbnail_size_set()`, and the
 * actual applied dimensions are read back using `elm_fileselector_thumbnail_size_get()`
 * to ensure `sd->fsd.thumbnail_size` reflects the true dimensions.
 *
 * If no fileselector is active, and if either the provided @p w or @p h is zero,
 * a default size is calculated based on `elm_config_finger_size_get()` and
 * `GENGRID_PADDING`. This default is intended to provide a reasonable touch-friendly size.
 *
 * The determined dimensions are stored in `sd->fsd.thumbnail_size.w` and
 * `sd->fsd.thumbnail_size.h`.
 *
 * @param obj The fileselector button Eo object (unused).
 * @param sd The private data (Elm_Fileselector_Button_Data) of the button.
 * @param w The desired width for thumbnails, in pixels. Example: 128.
 * @param h The desired height for thumbnails, in pixels. Example: 128.
 */
EOLIAN static void
_elm_fileselector_button_elm_interface_fileselector_thumbnail_size_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd, Evas_Coord w, Evas_Coord h)
{
   if (sd->fs)
     {
        elm_fileselector_thumbnail_size_set(sd->fs, w, h);
        elm_fileselector_thumbnail_size_get(sd->fs, &w, &h);
     }
   else if (!w || !h)
     w = h = elm_config_finger_size_get() * 2 - GENGRID_PADDING;

   sd->fsd.thumbnail_size.w = w;
   sd->fsd.thumbnail_size.h = h;
}

EOLIAN static void
_elm_fileselector_button_elm_interface_fileselector_thumbnail_size_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd, Evas_Coord *w, Evas_Coord *h)
{
   if (w) *w = sd->fsd.thumbnail_size.w;
   if (h) *h = sd->fsd.thumbnail_size.h;
}

EOLIAN static void
_elm_fileselector_button_elm_interface_fileselector_hidden_visible_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd, Eina_Bool visible)
{
   sd->fsd.hidden_visible = visible;

   if (sd->fs) elm_fileselector_hidden_visible_set(sd->fs, visible);
}

EOLIAN static void
_elm_fileselector_button_elm_interface_fileselector_current_name_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd, const char *name)
{
   eina_stringshare_replace(&sd->fsd.current_name, name);
   if (sd->fs) elm_fileselector_current_name_set(sd->fs, sd->fsd.current_name);
}

EOLIAN static const char*
_elm_fileselector_button_elm_interface_fileselector_current_name_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd)
{
  if (sd->fs)
    return elm_fileselector_current_name_get(sd->fs);

  return sd->fsd.current_name;
}

#define FS_USAGE_API(ret)\
   if (!pd->fs) \
     { \
        ERR("This function is only supported when there is a fileselector"); \
        return ret; \
     } \

/**
 * @internal
 * @brief Implements Elm_Interface_Fileselector.custom_filter_append.
 *
 * Appends a custom filter function to the list of available filters in the
 * fileselector dialog. This allows for programmatic filtering of displayed files.
 * This function only has an effect if the internal fileselector widget (`pd->fs`)
 * is currently active (i.e., the fileselector dialog is open). If not, an error
 * is printed, and EINA_FALSE is returned (due to `FS_USAGE_API` macro).
 *
 * @param obj The fileselector button Eo object (unused).
 * @param pd The private data (Elm_Fileselector_Button_Data) of the button.
 * @param func The custom filter function. This function takes an Efl_Io_Model
 *             and user data, and returns EINA_TRUE if the item should be shown.
 *             Example: `my_image_filter_func`.
 * @param data User-defined data to be passed to the filter function.
 * @param filter_name A human-readable name for this filter, which may be
 *                    displayed in the fileselector UI. Example: "Image Files".
 * @return EINA_TRUE if the filter was successfully appended to the active
 *         fileselector, EINA_FALSE otherwise (e.g., no active fileselector,
 *         or the underlying fileselector failed to append the filter).
 */
EOLIAN static Eina_Bool
_elm_fileselector_button_elm_interface_fileselector_custom_filter_append(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *pd, Elm_Fileselector_Filter_Func func, void *data, const char *filter_name)
{
   FS_USAGE_API(EINA_FALSE)

   return elm_interface_fileselector_custom_filter_append(pd->fs, func, data, filter_name);
}

EOLIAN static Eina_Bool
_elm_fileselector_button_elm_interface_fileselector_mime_types_filter_append(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *pd, const char *mime_types, const char *filter_name)
{
   FS_USAGE_API(EINA_FALSE)

   return elm_interface_fileselector_mime_types_filter_append(pd->fs, mime_types, filter_name);
}

EOLIAN static void
_elm_fileselector_button_elm_interface_fileselector_filters_clear(Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *pd)
{
   FS_USAGE_API()

   elm_interface_fileselector_filters_clear(pd->fs);
}


EOLIAN static Eina_Bool
_elm_fileselector_button_elm_interface_fileselector_hidden_visible_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Button_Data *sd)
{
   return sd->fsd.hidden_visible;
}

EAPI void
elm_fileselector_button_inwin_mode_set(Eo *obj, Eina_Bool value)
{
   ELM_FILESELECTOR_BUTTON_CHECK(obj);
   ELM_FILESELECTOR_BUTTON_DATA_GET_OR_RETURN(obj, sd);
   sd->inwin_mode = value;
}

EAPI Eina_Bool
elm_fileselector_button_inwin_mode_get(const Eo *obj)
{
   ELM_FILESELECTOR_BUTTON_CHECK(obj) EINA_FALSE;
   ELM_FILESELECTOR_BUTTON_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);
   return sd->inwin_mode;
}

/* Internal EO APIs and hidden overrides */

#define ELM_FILESELECTOR_BUTTON_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_fileselector_button)

#include "elm_fileselector_button_eo.c"
