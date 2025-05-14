/**
 * @internal
 * @addtogroup Elm_Plug_Group
 * @{
 */
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_plug.h"

#define MY_CLASS ELM_PLUG_CLASS

#define MY_CLASS_NAME "Elm_Plug"
#define MY_CLASS_NAME_LEGACY "elm_plug"

static const char PLUG_KEY[] = "__Plug_Ecore_Evas"; /**< Key to store the plug object in Ecore_Evas data */

static const char SIG_CLICKED[] = "clicked"; /**< Signal emitted when the plug is clicked */
static const char SIG_IMAGE_DELETED[] = "image,deleted"; /**< Signal emitted when the server-side image is deleted */
static const char SIG_IMAGE_RESIZED[] = "image,resized"; /**< Signal emitted when the server-side image is resized */

/**< Smart callback function descriptions */
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CLICKED, ""},
   {SIG_IMAGE_DELETED, ""},
   {SIG_IMAGE_RESIZED, "ii"}, /**< @c Evas_Coord_Size (two integers) */
   {NULL, NULL}
};

/**
 * @internal
 * @brief Evaluates the sizing of the plug object.
 * @param obj The plug object.
 *
 * This function is intended to recalculate and set the minimum and maximum
 * size hints for the plug widget based on its content (the socket object).
 * Currently, it's a placeholder.
 */
static void
_sizing_eval(Evas_Object *obj EINA_UNUSED)
{
   //Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

   //TODO: get socket object size
   //this reset plug's min/max size
   //evas_object_size_hint_min_set(obj, minw, minh);
   //evas_object_size_hint_max_set(obj, maxw, maxh);
}

/**
 * @internal
 * @brief Callback function for when the Ecore_Evas disconnects.
 * @param ee The Ecore_Evas that disconnected.
 *
 * This function is called when the connection to the service providing
 * the image is lost. It retrieves the associated plug widget and emits
 * the "image,deleted" signal.
 */
static void
_elm_plug_disconnected(Ecore_Evas *ee)
{
   Evas_Object *plug = ecore_evas_data_get(ee, PLUG_KEY);
   EINA_SAFETY_ON_NULL_RETURN(plug);
   efl_event_callback_legacy_call(plug, ELM_PLUG_EVENT_IMAGE_DELETED, NULL);
   /* TODO: was a typo. Deprecated, remove in future releases: */
   evas_object_smart_callback_call(plug, "image.deleted", NULL);
}

/**
 * @internal
 * @brief Callback function for when the Ecore_Evas is resized.
 * @param ee The Ecore_Evas that was resized.
 *
 * This function is called when the service providing the image resizes
 * its content. It retrieves the associated plug widget, gets the new
 * geometry, and emits the "image,resized" signal with the new dimensions.
 */
static void
_elm_plug_resized(Ecore_Evas *ee)
{
   Evas_Coord_Size size = {0, 0};
   Evas_Object *plug = ecore_evas_data_get(ee, PLUG_KEY);
   EINA_SAFETY_ON_NULL_RETURN(plug);

   ecore_evas_geometry_get(ee, NULL, NULL, &(size.w), &(size.h));
   efl_event_callback_legacy_call(plug, ELM_PLUG_EVENT_IMAGE_RESIZED, &size);
}

/**
 * @internal
 * @brief Handles focus updates for the plug object.
 * @param obj The plug Evas object.
 * @param sd Private data, unused.
 * @return EINA_TRUE if focus was successfully updated, EINA_FALSE otherwise.
 *
 * This function ensures that when the plug widget gains or loses focus,
 * the underlying image object (resize_obj) also receives or loses focus
 * accordingly. It calls the superclass's focus update function first.
 */
EOLIAN static Eina_Bool
_elm_plug_efl_ui_focus_object_on_focus_update(Eo *obj, void *sd EINA_UNUSED)
{
   Eina_Bool int_ret = EINA_FALSE;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);

   int_ret = efl_ui_focus_object_on_focus_update(efl_super(obj, MY_CLASS));
   if (!int_ret) return EINA_FALSE;

   if (efl_ui_focus_object_focus_get(obj))
     {
        evas_object_focus_set(wd->resize_obj, EINA_TRUE);
     }
   else
     {
        evas_object_focus_set(wd->resize_obj, EINA_FALSE);
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Applies the theme to the plug object.
 * @param obj The plug Evas object.
 * @param sd Private data, unused.
 * @return EFL_UI_THEME_APPLY_ERROR_GENERIC on failure, or the result of the superclass's theme_apply.
 *
 * This function applies the current theme to the plug widget. It first calls
 * the superclass's theme apply function and then calls _sizing_eval to
 * potentially adjust the widget's size based on the new theme.
 */
EOLIAN static Eina_Error
_elm_plug_efl_ui_widget_theme_apply(Eo *obj, void *sd EINA_UNUSED)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   _sizing_eval(obj);

   return int_ret;
}

/**
 * @internal
 * @brief Callback for mouse up events on the plug's image object.
 * @param data The plug Evas object (passed as user data).
 * @param e The Evas canvas, unused.
 * @param obj The Evas object that received the event (the image object), unused.
 * @param event_info Pointer to the Evas_Event_Mouse_Up structure.
 *
 * This function is triggered when a mouse button is released over the
 * plug's image. If it's the primary button (button 1) and not part of
 * an "on_hold" event, it emits the "clicked" signal for the plug widget.
 */
static void
_on_mouse_up(void *data,
             Evas *e EINA_UNUSED,
             Evas_Object *obj EINA_UNUSED,
             void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;

   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;

   evas_object_smart_callback_call(data, "clicked", NULL);
}

/**
 * @internal
 * @brief Adds the plug object to the canvas group.
 * @param obj The plug Evas object.
 * @param sd Private data, unused.
 *
 * This function is part of the Evas object's lifecycle. It initializes
 * the plug by creating an Ecore_Evas_Extn plug object, sets this as the
 * resize object for the widget, and attaches a mouse up event callback
 * to handle clicks. It also sets the widget to be non-focusable by default
 * and calls _sizing_eval.
 */
EOLIAN static void
_elm_plug_efl_canvas_group_group_add(Eo *obj, void *sd EINA_UNUSED)
{
   Evas_Object *p_obj;
   Ecore_Evas *ee;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));
   if (!ee) return;

   p_obj = ecore_evas_extn_plug_new(ee);
   if (!p_obj) return;

   elm_widget_resize_object_set(obj, p_obj);

   evas_object_event_callback_add
     (wd->resize_obj, EVAS_CALLBACK_MOUSE_UP, _on_mouse_up,
     obj);

   elm_widget_can_focus_set(obj, EINA_FALSE);
   _sizing_eval(obj);
}

/**
 * @brief Adds a new plug widget to the given parent Elementary object.
 *
 * @param parent The parent object.
 * @return The new object or @c NULL if it cannot be created.
 *
 * @ingroup Elm_Plug_Group
 */
EAPI Evas_Object *
elm_plug_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Constructor for the Elm_Plug Eolian object.
 * @param obj The Eolian object to construct.
 * @param sd Private data, unused.
 * @return The constructed Eolian object.
 *
 * This function initializes the Elm_Plug object. It calls the superclass
 * constructor, sets the legacy type name, registers smart callbacks,
 * and sets the accessibility role to image.
 */
EOLIAN static Eo *
_elm_plug_efl_object_constructor(Eo *obj, void *sd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_IMAGE);

   return obj;
}

/**
 * @internal
 * @brief Gets the underlying Evas image object of the plug.
 * @param obj The plug Evas object.
 * @param sd Private data, unused.
 * @return The Evas image object (resize_obj) or @c NULL on failure.
 *
 * This function returns the actual Evas object that displays the image
 * content from the external service.
 */
EOLIAN static Evas_Object*
_elm_plug_image_object_get(const Eo *obj, void *sd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);
   return wd->resize_obj;
}

/**
 * @internal
 * @brief Connects the plug widget to a service.
 * @param obj The plug Evas object.
 * @param sd Private data, unused.
 * @param svcname The service name to connect to.
 * @param svcnum The service number.
 * @param svcsys If true, connect to a system-wide service; otherwise, a user-session service.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 *
 * This function establishes the connection to an external service that
 * provides an image. It uses the underlying Ecore_Evas_Extn plug object
 * to make the connection. If successful, it sets up callbacks for
 * disconnection and resize events from the service.
 */
EOLIAN static Eina_Bool
_elm_plug_connect(Eo *obj, void *sd EINA_UNUSED, const char *svcname, int svcnum, Eina_Bool svcsys)
{
   Evas_Object *plug_img = NULL;

   ELM_PLUG_CHECK(obj) EINA_FALSE;

   plug_img = elm_plug_image_object_get(obj);
   if (!plug_img) return EINA_FALSE;

   if (ecore_evas_extn_plug_connect(plug_img, svcname, svcnum, svcsys))
     {
        Ecore_Evas *ee = NULL;
        ee = ecore_evas_object_ecore_evas_get(plug_img);
        if (!ee) return EINA_FALSE;

        ecore_evas_data_set(ee, PLUG_KEY, obj);
        ecore_evas_callback_delete_request_set(ee, _elm_plug_disconnected);
        ecore_evas_callback_resize_set(ee, _elm_plug_resized);
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @internal
 * @brief Class constructor for Elm_Plug.
 * @param klass The Efl_Class to construct.
 *
 * This function is called once when the Elm_Plug class is being set up.
 * It registers the legacy type name for the class, allowing it to be
 * used with older Elementary APIs.
 */
EOLIAN static void
_elm_plug_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Internal EO APIs and hidden overrides */

#define ELM_PLUG_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_OPS(elm_plug)

#include "elm_plug_eo.c"

/**
 * @}
 */
