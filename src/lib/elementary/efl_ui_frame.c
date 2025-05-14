#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define ELM_LAYOUT_PROTECTED
#define EFL_PART_PROTECTED
#define EFL_INPUT_CLICKABLE_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"
#include "efl_ui_widget_frame.h"
#include "elm_widget_layout.h"
#include "elm_part_helper.h"

#define MY_CLASS EFL_UI_FRAME_CLASS
#define MY_CLASS_PFX efl_ui_frame
#define MY_CLASS_NAME "Efl.Ui.Frame"

static const char SIG_CLICKED[] = "clicked";

/**
 * @brief Smart callback function descriptions.
 */
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CLICKED, ""},
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {NULL, NULL}
};

/**
 * @brief Evaluates the sizing of the frame widget.
 *
 * This function calculates the minimum size required by the frame's
 * internal Edje object and updates the frame's size hints accordingly.
 * It ensures that the frame's minimum size accommodates its content.
 *
 * @param obj The Evas object (frame widget).
 * @param sd The frame widget's private data (unused in this function).
 */
static void
_sizing_eval(Evas_Object *obj,
             Efl_Ui_Frame_Data *sd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord cminw = -1, cminh = -1;

   edje_object_size_min_calc(wd->resize_obj, &minw, &minh);
   evas_object_size_hint_min_get(obj, &cminw, &cminh);
   if ((minw == cminw) && (minh == cminh)) return;

   efl_gfx_hint_size_restricted_min_set(obj, EINA_SIZE2D(minw, minh));
   if (elm_widget_is_legacy(obj))
     evas_object_size_hint_max_set(obj, -1, -1);
   else
     efl_gfx_hint_size_restricted_min_set(obj, EINA_SIZE2D(minw, minh));
}

/**
 * @brief Callback function to trigger recalculation of the canvas group.
 *
 * This is typically used when the frame's layout might have changed,
 * for example, during collapse/expand animations.
 *
 * @param data The Evas object (frame widget).
 * @param event The Efl_Event data (unused).
 */
static void
_recalc(void *data, const Efl_Event *event EINA_UNUSED)
{
   efl_canvas_group_calculate(data);
}

/**
 * @brief Callback function invoked when a recalculation (e.g., animation) is done.
 *
 * This function is typically connected to the "elm,anim,done" or "efl,anim,done"
 * signals of the internal Edje object. It removes the recalculation callback
 * and resets the animation flag.
 *
 * @param data The Evas object (frame widget).
 * @param obj The Evas object that emitted the signal (unused).
 * @param sig The signal name (unused).
 * @param src The signal source (unused).
 */
static void
_on_recalc_done(void *data,
                Evas_Object *obj EINA_UNUSED,
                const char *sig EINA_UNUSED,
                const char *src EINA_UNUSED)
{
   EFL_UI_FRAME_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(data, wd);

   efl_event_callback_del
     (wd->resize_obj, EFL_LAYOUT_EVENT_RECALC, _recalc, data);
   sd->anim = EINA_FALSE;

   efl_canvas_group_calculate(data);
}

/**
 * @brief Callback function invoked when the frame is clicked (legacy Edje signal).
 *
 * This handles the click event for legacy frames. If the frame is collapsible
 * and not currently animating, it toggles the collapsed state and emits
 * the "clicked" smart callback.
 *
 * @param data The Evas object (frame widget).
 * @param obj The Evas object that emitted the signal (unused).
 * @param sig The signal name (unused).
 * @param src The signal source (unused).
 */
static void
_on_frame_clicked(void *data,
                  Evas_Object *obj EINA_UNUSED,
                  const char *sig EINA_UNUSED,
                  const char *src EINA_UNUSED)
{
   EFL_UI_FRAME_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(data, wd);

   if (sd->anim) return;

   if (sd->collapsible)
     {
        efl_event_callback_add(wd->resize_obj, EFL_LAYOUT_EVENT_RECALC, _recalc, data);

        if (elm_widget_is_legacy(data))
          elm_layout_signal_emit(data, "elm,action,toggle", "elm");
        else
          elm_layout_signal_emit(data, "efl,action,toggle", "efl");

        sd->collapsed++;
        sd->anim = EINA_TRUE;
        elm_widget_tree_unfocusable_set(data, sd->collapsed);
     }
   evas_object_smart_callback_call(data, "clicked", NULL);
}

/**
 * @brief Callback function invoked when the frame's close action is triggered (legacy Edje signal).
 *
 * Emits the "close" smart callback. This is typically connected to a
 * "close" button or similar element within the frame's theme.
 *
 * @param data The Evas object (frame widget).
 * @param obj The Evas object that emitted the signal (unused).
 * @param sig The signal name (unused).
 * @param src The signal source (unused).
 */
static void
_on_frame_close(void *data,
                Evas_Object *obj EINA_UNUSED,
                const char *sig EINA_UNUSED,
                const char *src EINA_UNUSED)
{
   evas_object_smart_callback_call(data, "close", NULL);
}

/**
 * @brief Implements the Efl.Canvas.Group group_calculate interface.
 * @eoapi
 *
 * This function is called when the canvas group needs to recalculate its layout.
 * It triggers the frame's own sizing evaluation logic.
 * This uses deferred sizing evaluation, similar to its parent class.
 *
 * @param obj The Efl object (frame widget).
 * @param sd The frame widget's private data.
 */
EOLIAN static void
_efl_ui_frame_efl_canvas_group_group_calculate(Eo *obj, Efl_Ui_Frame_Data *sd)
{
   /* calling OWN sizing evaluate code here */
   efl_canvas_group_need_recalculate_set(obj, EINA_FALSE);
   _sizing_eval(obj, sd);
}

/**
 * @brief Callback function for the EFL_INPUT_EVENT_CLICKED event.
 *
 * This handles click events for non-legacy frames. If the frame is collapsible
 * and not currently animating, it toggles the collapsed state.
 * The "clicked" smart callback is emitted by the clickable interface automatically.
 *
 * @param data The Evas object (frame widget).
 * @param ev The Efl_Event data for the click event (unused).
 */
static void
_clicked_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   EFL_UI_FRAME_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(data, wd);

   if (sd->anim) return;

   if (sd->collapsible)
     {
        efl_event_callback_add(wd->resize_obj, EFL_LAYOUT_EVENT_RECALC, _recalc, data);
        elm_layout_signal_emit(data, "efl,action,toggle", "efl");

        sd->collapsed++;
        sd->anim = EINA_TRUE;
        elm_widget_tree_unfocusable_set(data, sd->collapsed);
     }
}

/**
 * @brief Implements the Efl.Canvas.Group group_add interface.
 * @eoapi
 *
 * This function is called when the frame widget is added to a canvas group.
 * It sets up the theme, signals, and initial properties of the frame.
 *
 * @param obj The Efl object (frame widget).
 * @param _pd The frame widget's private data (unused).
 */
EOLIAN static void
_efl_ui_frame_efl_canvas_group_group_add(Eo *obj, Efl_Ui_Frame_Data *_pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "frame");
   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   if (elm_widget_is_legacy(obj))
     {
        edje_object_signal_callback_add
           (wd->resize_obj, "elm,anim,done", "elm",
            _on_recalc_done, obj);
        edje_object_signal_callback_add
           (wd->resize_obj, "elm,action,click", "elm",
            _on_frame_clicked, obj);
        edje_object_signal_callback_add
           (wd->resize_obj, "elm,action,close", "elm",
            _on_frame_close, obj);
     }
   else
     {
        edje_object_signal_callback_add
           (wd->resize_obj, "efl,anim,done", "efl",
            _on_recalc_done, obj);
        efl_ui_action_connector_bind_clickable_to_theme(wd->resize_obj, obj);
        efl_event_callback_add(obj, EFL_INPUT_EVENT_CLICKED, _clicked_cb, obj);
     }

   elm_widget_can_focus_set(obj, EINA_FALSE);

   if (elm_widget_theme_object_set(obj, wd->resize_obj,
                                       elm_widget_theme_klass_get(obj),
                                       elm_widget_theme_element_get(obj),
                                       elm_widget_theme_style_get(obj)) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     CRI("Failed to set layout!");
}

/**
 * @brief Implements the Efl.Object constructor.
 * @eoapi
 *
 * Initializes the frame widget after it has been constructed by the parent class.
 * Sets up smart callbacks and the accessibility role.
 *
 * @param obj The Efl object (frame widget).
 * @param _pd The frame widget's private data (unused).
 * @return The constructed Efl object.
 */
EOLIAN static Eo *
_efl_ui_frame_efl_object_constructor(Eo *obj, Efl_Ui_Frame_Data *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_FRAME);

   return obj;
}

/**
 * @brief Sets whether the frame is collapsible by user interaction.
 * @eoapi Efl.Ui.Frame.autocollapse_set
 *
 * @param obj The Efl object (frame widget) (unused).
 * @param sd The frame widget's private data.
 * @param autocollapse If @c EINA_TRUE, the frame can be collapsed by user interaction.
 */
EOLIAN static void
_efl_ui_frame_autocollapse_set(Eo *obj EINA_UNUSED, Efl_Ui_Frame_Data *sd, Eina_Bool autocollapse)
{

   sd->collapsible = !!autocollapse;
}

/**
 * @brief Gets whether the frame is collapsible by user interaction.
 * @eoapi Efl.Ui.Frame.autocollapse_get
 *
 * @param obj The Efl object (frame widget) (unused).
 * @param sd The frame widget's private data.
 * @return @c EINA_TRUE if the frame can be collapsed by user interaction, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_frame_autocollapse_get(const Eo *obj EINA_UNUSED, Efl_Ui_Frame_Data *sd)
{
   return sd->collapsible;
}

/**
 * @brief Sets the collapsed state of the frame immediately, without animation.
 * @eoapi Efl.Ui.Frame.collapse_set
 *
 * @param obj The Efl object (frame widget).
 * @param sd The frame widget's private data.
 * @param collapse The desired collapsed state. @c EINA_TRUE to collapse, @c EINA_FALSE to expand.
 */
EOLIAN static void
_efl_ui_frame_collapse_set(Eo *obj, Efl_Ui_Frame_Data *sd, Eina_Bool collapse)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   collapse = !!collapse;
   if (sd->collapsed == collapse) return;

   if (elm_widget_is_legacy(obj))
     elm_layout_signal_emit(obj, "elm,action,switch", "elm");
   else
     elm_layout_signal_emit(obj, "efl,action,switch", "efl");

   edje_object_message_signal_process(wd->resize_obj);
   sd->collapsed = !!collapse;
   sd->anim = EINA_FALSE;

   elm_widget_tree_unfocusable_set(obj, sd->collapsed);
   _sizing_eval(obj, sd);
}

/**
 * @brief Sets the collapsed state of the frame, playing an animation.
 * @eoapi Efl.Ui.Frame.collapse_go
 *
 * @param obj The Efl object (frame widget).
 * @param sd The frame widget's private data.
 * @param collapse The desired collapsed state. @c EINA_TRUE to collapse, @c EINA_FALSE to expand.
 */
EOLIAN static void
_efl_ui_frame_collapse_go(Eo *obj, Efl_Ui_Frame_Data *sd, Eina_Bool collapse)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   collapse = !!collapse;
   if (sd->collapsed == collapse) return;

   if (elm_widget_is_legacy(obj))
     elm_layout_signal_emit(obj, "elm,action,toggle", "elm");
   else
     elm_layout_signal_emit(obj, "efl,action,toggle", "efl");

   efl_event_callback_legacy_call
     (wd->resize_obj, EFL_LAYOUT_EVENT_RECALC, obj);
   sd->collapsed = collapse;
   elm_widget_tree_unfocusable_set(obj, sd->collapsed);
   sd->anim = EINA_TRUE;
}

/**
 * @brief Gets the current collapsed state of the frame.
 * @eoapi Efl.Ui.Frame.collapse_get
 *
 * @param obj The Efl object (frame widget) (unused).
 * @param sd The frame widget's private data.
 * @return @c EINA_TRUE if the frame is collapsed, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_frame_collapse_get(const Eo *obj EINA_UNUSED, Efl_Ui_Frame_Data *sd)
{
   return sd->collapsed;
}

/* Internal EO APIs and hidden overrides */

ELM_PART_TEXT_DEFAULT_IMPLEMENT(efl_ui_frame, Efl_Ui_Frame_Data)
ELM_PART_MARKUP_DEFAULT_IMPLEMENT(efl_ui_frame, Efl_Ui_Frame_Data)
ELM_PART_CONTENT_DEFAULT_IMPLEMENT(efl_ui_frame, Efl_Ui_Frame_Data)

#define EFL_UI_FRAME_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_OPS(efl_ui_frame)

#include "efl_ui_frame.eo.c"
#include "efl_ui_frame_eo.legacy.c"

#include "efl_ui_frame_legacy_eo.h"

#define MY_CLASS_NAME_LEGACY "elm_frame"

/**
 * @brief Legacy class constructor for elm_frame.
 *
 * Registers the legacy "elm_frame" type with the Evas smart system.
 *
 * @param klass The Efl_Class to construct.
 */
static void
_efl_ui_frame_legacy_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @brief Legacy Efl.Object constructor for elm_frame.
 * @eoapi
 *
 * Calls the superclass constructor and sets the Evas object type
 * to the legacy "elm_frame" name.
 *
 * @param obj The Efl object (legacy frame widget).
 * @param _pd Private data (unused).
 * @return The constructed Efl object.
 */
EOLIAN static Eo *
_efl_ui_frame_legacy_efl_object_constructor(Eo *obj, void *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, EFL_UI_FRAME_LEGACY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   return obj;
}

/**
 * @brief Adds a new legacy frame widget.
 * @deprecated Use efl_add(EFL_UI_FRAME_CLASS, parent) instead.
 *
 * This function creates and returns a new legacy frame widget as an
 * Evas_Object.
 *
 * @param parent The parent Evas_Object.
 * @return The new Evas_Object (frame widget), or @c NULL on failure.
 */
EAPI Evas_Object *
elm_frame_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(EFL_UI_FRAME_LEGACY_CLASS, parent);
}

#include "efl_ui_frame_legacy_eo.c"
