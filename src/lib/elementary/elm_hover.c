/**
 * @internal
 * @addtogroup Widget
 * @{
 *
 * @section elm-hover-class The Elementary Hover Class
 *
 * Elementary, besides having the @ref Elm_Hover widget, exposes its
 * foundation -- the Elementary Hover Class -- in EFL. This class defines
 * the essential API for applications be able to deal with hover objects.
 *
 * @}
 */
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_WIDGET_PROTECTED
#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_ACCESS_WIDGET_ACTION_PROTECTED
#define ELM_LAYOUT_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_hover.h"

#include "elm_hover_part.eo.h"
#include "elm_part_helper.h"

#define MY_CLASS ELM_HOVER_CLASS /**< Efl_Ui_Hover class macro */
#define MY_CLASS_PFX elm_hover /**< Class prefix for hover */

#define MY_CLASS_NAME "Elm_Hover" /**< Full class name */
#define MY_CLASS_NAME_LEGACY "elm_hover" /**< Legacy class name */

/**
 * @brief Macro to iterate over all hover content parts.
 *
 * This macro simplifies iterating through the `sd->subs` array, which holds
 * information about the content objects in different hover slots.
 */
#define ELM_HOVER_PARTS_FOREACH                                         \
  for (unsigned int i = 0; i < sizeof(sd->subs) / sizeof(sd->subs[0]); i++)

#define _HOV_LEFT               (&(sd->subs[0])) /**< Convenience macro for left content part */
#define _HOV_TOP_LEFT           (&(sd->subs[1])) /**< Convenience macro for top-left content part */
#define _HOV_TOP                (&(sd->subs[2])) /**< Convenience macro for top content part */
#define _HOV_TOP_RIGHT          (&(sd->subs[3])) /**< Convenience macro for top-right content part (Note: Index was 2, corrected to 3 based on array structure) */
#define _HOV_RIGHT              (&(sd->subs[4])) /**< Convenience macro for right content part */
#define _HOV_BOTTOM_RIGHT       (&(sd->subs[5])) /**< Convenience macro for bottom-right content part */
#define _HOV_BOTTOM             (&(sd->subs[6])) /**< Convenience macro for bottom content part */
#define _HOV_BOTTOM_LEFT        (&(sd->subs[7])) /**< Convenience macro for bottom-left content part */
#define _HOV_MIDDLE             (&(sd->subs[8])) /**< Convenience macro for middle content part */

/**
 * @brief Defines aliases for hover content parts.
 *
 * This array maps user-friendly names (e.g., "left") to internal
 * Edje swallow part names (e.g., "elm.swallow.slot.left").
 * This allows users to set content using simple direction strings.
 *
 * Structure of elements:
 * @code
 * {
 *   "alias_name", // User-facing name for the content slot
 *   "part_name"   // Corresponding Edje swallow part name in the theme
 * }
 * @endcode
 */
const Elm_Layout_Part_Alias_Description _content_aliases[] =
{
   {"left", "elm.swallow.slot.left"},
   {"top-left", "elm.swallow.slot.top-left"},
   {"top", "elm.swallow.slot.top"},
   {"top-right", "elm.swallow.slot.top-right"},
   {"right", "elm.swallow.slot.right"},
   {"bottom-right", "elm.swallow.slot.bottom-right"},
   {"bottom", "elm.swallow.slot.bottom"},
   {"bottom-left", "elm.swallow.slot.bottom-left"},
   {"middle", "elm.swallow.slot.middle"},
   {NULL, NULL}
};

/**
 * @brief Defines content aliases specific to the "main_menu_submenu" style.
 *
 * This style of hover only supports a "bottom" content slot.
 *
 * Structure of elements:
 * @code
 * {
 *   "alias_name", // User-facing name for the content slot
 *   "part_name"   // Corresponding Edje swallow part name in the theme
 * }
 * @endcode
 */
const Elm_Layout_Part_Alias_Description _content_aliases_main_menu_submenu[] =
{
   {"bottom", "elm.swallow.slot.bottom"},
   {NULL, NULL}
};

#define ELM_PRIV_HOVER_SIGNALS(cmd) \
   cmd(SIG_CLICKED, "clicked", "") \
   cmd(SIG_DISMISSED, "dismissed", "") \
   cmd(SIG_SMART_LOCATION_CHANGED, "smart,changed", "")

ELM_PRIV_HOVER_SIGNALS(ELM_PRIV_STATIC_VARIABLE_DECLARE);

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   ELM_PRIV_HOVER_SIGNALS(ELM_PRIV_SMART_CALLBACKS_DESC)
   {SIG_LAYOUT_FOCUSED, ""}, /**< handled by elm_layout */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< handled by elm_layout */
   {NULL, NULL}
};
#undef ELM_PRIV_HOVER_SIGNALS

/**
 * @brief Callback invoked when the hover's parent object is moved.
 * @param data The hover object.
 * @param e Unused.
 * @param obj Unused.
 * @param event_info Unused.
 *
 * This function triggers a recalculation of the hover's sizing.
 */
static void
_parent_move_cb(void *data,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   elm_layout_sizing_eval(data);
}

/**
 * @brief Callback invoked when the hover's parent object is resized.
 * @param data The hover object.
 * @param e Unused.
 * @param obj Unused.
 * @param event_info Unused.
 *
 * This function triggers a recalculation of the hover's sizing.
 */
static void
_parent_resize_cb(void *data,
                  Evas *e EINA_UNUSED,
                  Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   elm_layout_sizing_eval(data);
}

/**
 * @brief Callback invoked when the hover's parent object is shown.
 * @param data Unused.
 * @param e Unused.
 * @param obj Unused.
 * @param event_info Unused.
 *
 * Currently, this function does nothing.
 */
static void
_parent_show_cb(void *data EINA_UNUSED,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
}

/**
 * @brief Callback invoked when the hover's parent object is hidden.
 * @param data The hover object.
 * @param e Unused.
 * @param obj Unused.
 * @param event_info Unused.
 *
 * This function hides the hover object.
 */
static void
_parent_hide_cb(void *data,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   evas_object_hide(data);
}

/**
 * @brief Callback invoked when the hover's parent object is deleted.
 * @param data The hover object.
 * @param e Unused.
 * @param obj Unused.
 * @param event_info Unused.
 *
 * This function clears the hover's parent and triggers a sizing recalculation.
 */
static void
_parent_del_cb(void *data,
               Evas *e EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   elm_hover_parent_set(data, NULL);
   elm_layout_sizing_eval(data);
}

/**
 * @brief Detaches the hover object from its parent.
 * @param obj The hover object.
 *
 * This function removes all event callbacks that were set on the parent
 * object to monitor its state (move, resize, show, hide, delete).
 */
static void
_elm_hover_parent_detach(Evas_Object *obj)
{
   ELM_HOVER_DATA_GET(obj, sd);

   if (sd->parent)
     {
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_MOVE, _parent_move_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_RESIZE, _parent_resize_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_SHOW, _parent_show_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_HIDE, _parent_hide_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_DEL, _parent_del_cb, obj);
     }
}

/**
 * @brief Calculates the available space around the target object within its parent.
 * @param sd The hover's private data.
 * @param[out] spc_l Pointer to store the calculated space to the left of the target.
 * @param[out] spc_t Pointer to store the calculated space to the top of the target.
 * @param[out] spc_r Pointer to store the calculated space to the right of the target.
 * @param[out] spc_b Pointer to store the calculated space to the bottom of the target.
 *
 * This function determines how much space is available on each side of the
 * hover's target, relative to the hover's parent. This information is used
 * for smart content placement.
 */
static void
_elm_hover_left_space_calc(Elm_Hover_Data *sd,
                           Evas_Coord *spc_l,
                           Evas_Coord *spc_t,
                           Evas_Coord *spc_r,
                           Evas_Coord *spc_b)
{
   Evas_Coord x = 0, y = 0, w = 0, h = 0, x2 = 0, y2 = 0, w2 = 0, h2 = 0;

   if (sd->parent)
     {
        evas_object_geometry_get(sd->parent, &x, &y, &w, &h);
        if (efl_isa(sd->parent, EFL_UI_WIN_CLASS))
          {
             x = 0;
             y = 0;
          }
     }
   if (sd->target) evas_object_geometry_get(sd->target, &x2, &y2, &w2, &h2);

   *spc_l = x2 - x;
   *spc_r = (x + w) - (x2 + w2);
   if (*spc_l < 0) *spc_l = 0;
   if (*spc_r < 0) *spc_r = 0;

   *spc_t = y2 - y;
   *spc_b = (y + h) - (y2 + h2);
   if (*spc_t < 0) *spc_t = 0;
   if (*spc_b < 0) *spc_b = 0;
}

/**
 * @brief Determines the best location for "smart" content.
 * @param sd The hover's private data.
 * @param spc_l Space available to the left of the target.
 * @param spc_t Space available to the top of the target.
 * @param spc_r Space available to the right of the target.
 * @param spc_b Space available to the bottom of the target.
 * @return A pointer to the Content_Info struct representing the best slot.
 *
 * This function implements the logic to find the optimal slot (e.g., top,
 * bottom, left, right, or corners) to place content when the "smart"
 * placement policy is used. It prioritizes directions with more available
 * space and considers the content's minimum size.
 */
static Content_Info *
_elm_hover_smart_content_location_get(Elm_Hover_Data *sd,
                                      Evas_Coord spc_l,
                                      Evas_Coord spc_t,
                                      Evas_Coord spc_r,
                                      Evas_Coord spc_b)
{
   Evas_Coord c_w = 0, c_h = 0, mid_w, mid_h;
   int max;

   evas_object_size_hint_combined_min_get(sd->smt_sub->obj, &c_w, &c_h);
   mid_w = c_w / 2;
   mid_h = c_h / 2;

   if (spc_l > spc_r) goto left;

   max = MAX(spc_t, spc_r);
   max = MAX(max, spc_b);

   if (max == spc_t)
     {
        if (mid_w > spc_l) return _HOV_TOP_RIGHT;

        return _HOV_TOP;
     }

   if (max == spc_r)
     {
        if (mid_h > spc_t) return _HOV_BOTTOM_RIGHT;
        else if (mid_h > spc_b)
          return _HOV_TOP_RIGHT;

        return _HOV_RIGHT;
     }

   if (mid_h > spc_l)
     return _HOV_BOTTOM_RIGHT;

   return _HOV_BOTTOM;

left:
   max = MAX(spc_t, spc_l);
   max = MAX(max, spc_b);

   if (max == spc_t)
     {
        if (mid_w > spc_r) return _HOV_TOP_LEFT;

        return _HOV_TOP;
     }

   if (max == spc_l)
     {
        if (mid_h > spc_t) return _HOV_BOTTOM_LEFT;
        else if (mid_h > spc_b)
          return _HOV_TOP_LEFT;

        return _HOV_LEFT;
     }

   if (mid_h > spc_r) return _HOV_BOTTOM_LEFT;

   return _HOV_BOTTOM;
}

/**
 * @brief Re-evaluates and updates the position of "smart" content.
 * @param obj The hover object.
 *
 * This function is called when the "smart" content's size hints change or
 * when the hover's theme is reapplied. It recalculates the best location
 * for the smart content and moves it to the new slot if necessary.
 * It also handles UI mirroring adjustments.
 */
static void
_elm_hover_smt_sub_re_eval(Evas_Object *obj)
{
   Evas_Coord spc_l, spc_r, spc_t, spc_b;
   Content_Info *prev;
   Evas_Object *sub;
   char buf[1024];

   ELM_HOVER_DATA_GET(obj, sd);

   if (!sd->smt_sub) return;
   prev = sd->smt_sub;

   _elm_hover_left_space_calc(sd, &spc_l, &spc_t, &spc_r, &spc_b);

   sub = sd->smt_sub->obj;

   sd->smt_sub =
     _elm_hover_smart_content_location_get(sd, spc_l, spc_t, spc_r, spc_b);

   sd->smt_sub->obj = sub;

   if (sd->smt_sub != prev)
     efl_event_callback_legacy_call
       (obj, ELM_HOVER_EVENT_SMART_CHANGED, (void *)sd->smt_sub->swallow);

   if (efl_ui_mirrored_get(obj))
     {
        if (sd->smt_sub == _HOV_BOTTOM_LEFT) sd->smt_sub = _HOV_BOTTOM_RIGHT;
        else if (sd->smt_sub == _HOV_BOTTOM_RIGHT)
          sd->smt_sub = _HOV_BOTTOM_LEFT;
        else if (sd->smt_sub == _HOV_RIGHT)
          sd->smt_sub = _HOV_LEFT;
        else if (sd->smt_sub == _HOV_LEFT)
          sd->smt_sub = _HOV_RIGHT;
        else if (sd->smt_sub == _HOV_TOP_RIGHT)
          sd->smt_sub = _HOV_TOP_LEFT;
        else if (sd->smt_sub == _HOV_TOP_LEFT)
          sd->smt_sub = _HOV_TOP_RIGHT;
     }

   snprintf(buf, sizeof(buf), "elm.swallow.slot.%s", sd->smt_sub->swallow);
   elm_layout_content_set(obj, buf, sd->smt_sub->obj);
}

/**
 * @brief Emits signals to show the hover and its content slots.
 * @param obj The hover object.
 *
 * This function sends Edje signals to the hover's theme to trigger
 * "show" animations for the main hover area and any visible content slots.
 */
static void
_hov_show_do(Evas_Object *obj)
{
   ELM_HOVER_DATA_GET(obj, sd);

   elm_layout_signal_emit(obj, "elm,action,show", "elm");

   ELM_HOVER_PARTS_FOREACH
   {
      char buf[1024];

      if (sd->subs[i].obj)
        {
           snprintf
             (buf, sizeof(buf), "elm,action,slot,%s,show",
             sd->subs[i].swallow);

           elm_layout_signal_emit(obj, buf, "elm");
        }
   }
}

EOLIAN static Eina_Error
_elm_hover_efl_ui_widget_theme_apply(Eo *obj, Elm_Hover_Data *sd)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   if (sd->smt_sub) _elm_hover_smt_sub_re_eval(obj);

   elm_layout_sizing_eval(obj);

   if (evas_object_visible_get(obj)) _hov_show_do(obj);

   return int_ret;
}

/**
 * @brief Calculates the hover's geometry based on its parent and target.
 * @param obj The hover object.
 * @param sd The hover's private data.
 *
 * This EOLIAN override is part of the Evas canvas group calculation phase.
 * It positions and sizes the hover to overlay its target correctly within
 * the parent's coordinate space. It sets the size hints for internal
 * offset and size rectangles used by the layout.
 */
EOLIAN static void
_elm_hover_efl_canvas_group_group_calculate(Eo *obj, Elm_Hover_Data *sd)
{
   Evas_Coord ofs_x, x = 0, y = 0, w = 0, h = 0, x2 = 0,
              y2 = 0, w2 = 0, h2 = 0;


   if (sd->on_del) return;

   if (sd->parent)
     {
        evas_object_geometry_get(sd->parent, &x, &y, &w, &h);
        if (efl_isa(sd->parent, EFL_UI_WIN_CLASS))
          {
             if (efl_canvas_object_is_frame_object_get(obj))
               evas_object_geometry_get(obj, &x, &y, NULL, NULL);
             else
               {
                  x = 0;
                  y = 0;
               }
          }
     }
   evas_object_geometry_get(obj, &x2, &y2, &w2, &h2);

   if (efl_ui_mirrored_get(obj)) ofs_x = w - (x2 - x) - w2;
   else ofs_x = x2 - x;

   if (y < 0)
     h += (-y);

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   evas_object_size_hint_min_set(sd->offset, ofs_x, y2 - y);
   evas_object_size_hint_min_set(sd->size, w2, h2);
   evas_object_geometry_set(wd->resize_obj, x, y, w, h);
}

/**
 * @brief Callback for when the "smart" sub-object's size hints change.
 * @param data The hover object.
 * @param e Unused.
 * @param obj Unused.
 * @param event_info Unused.
 *
 * Triggers a re-evaluation of the smart content's position.
 */
static void
_on_smt_sub_changed(void *data,
                    Evas *e EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   _elm_hover_smt_sub_re_eval(data);
}

/**
 * @brief Handles adding a sub-object to the hover widget.
 * @param obj The hover object.
 * @param sd The hover's private data.
 * @param sobj The sub-object being added.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 *
 * This EOLIAN override calls the superclass method and then, if the added
 * sub-object is the "smart" content, sets up a callback to monitor its
 * size hint changes.
 */
EOLIAN static Eina_Bool
_elm_hover_efl_ui_widget_widget_sub_object_add(Eo *obj, Elm_Hover_Data *sd, Evas_Object *sobj)
{
   Eina_Bool int_ret = EINA_FALSE;

   if (evas_object_data_get(sobj, "elm-parent") == obj) return EINA_TRUE;

   int_ret = elm_widget_sub_object_add(efl_super(obj, MY_CLASS), sobj);
   if (!int_ret) return EINA_FALSE;

   if (sd->smt_sub && sd->smt_sub->obj == sobj)
     evas_object_event_callback_add
       (sobj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _on_smt_sub_changed, obj);

   return EINA_TRUE;
}

/**
 * @brief Handles deleting a sub-object from the hover widget.
 * @param obj The hover object.
 * @param sd The hover's private data.
 * @param sobj The sub-object being deleted.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 *
 * This EOLIAN override calls the superclass method. If the deleted
 * sub-object was the "smart" content, its change callback is removed.
 * Otherwise, it iterates through the standard content slots to clear
 * the reference to the deleted object.
 */
EOLIAN static Eina_Bool
_elm_hover_efl_ui_widget_widget_sub_object_del(Eo *obj, Elm_Hover_Data *sd, Evas_Object *sobj)
{
   Eina_Bool int_ret = EINA_FALSE;

   int_ret = elm_widget_sub_object_del(efl_super(obj, MY_CLASS), sobj);
   if (!int_ret) return EINA_FALSE;

   if (sd->smt_sub && sd->smt_sub->obj == sobj)
     {
        evas_object_event_callback_del_full
          (sd->smt_sub->obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _on_smt_sub_changed, obj);
     }
   else
     {
        ELM_HOVER_PARTS_FOREACH
        {
           if (sd->subs[i].obj == sobj)
             {
                sd->subs[i].obj = NULL;
                break;
             }
        }
     }

   return EINA_TRUE;
}

/**
 * @brief Deletes all content sub-objects from the hover.
 * @param sd The hover's private data.
 *
 * This function iterates through all standard content slots and deletes
 * their associated Evas objects. It also clears the "smart" content reference.
 */
static void
_elm_hover_subs_del(Elm_Hover_Data *sd)
{
   ELM_HOVER_PARTS_FOREACH
     ELM_SAFE_FREE(sd->subs[i].obj, evas_object_del);
   sd->smt_sub = NULL;
}

/**
 * @brief Sets content into a specified swallow part of the hover.
 * @param obj The hover object.
 * @param sd The hover's private data.
 * @param swallow The name of the swallow part (e.g., "left", "top", "smart").
 * @param content The Evas_Object to set as content.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * This function handles setting content for both standard directional slots
 * and the special "smart" slot. If "smart" is specified, it manages the
 * transition to or from smart mode and re-evaluates content placement.
 */
static Eina_Bool
_elm_hover_content_set(Eo *obj, Elm_Hover_Data *sd, const char *swallow, Evas_Object *content)
{
   Eina_Bool int_ret;

   if (!swallow) return EINA_FALSE;

   if (!strcmp(swallow, "smart"))
     {
        if (sd->smt_sub)     /* already under 'smart' mode */
          {
             if (sd->smt_sub->obj != content)
               {
                  evas_object_del(sd->smt_sub->obj);
                  sd->smt_sub = _HOV_LEFT;
                  sd->smt_sub->obj = content;
               }

             if (!content)
               {
                  sd->smt_sub->obj = NULL;
                  sd->smt_sub = NULL;
               }
             else _elm_hover_smt_sub_re_eval(obj);

             goto end;
          }
        else     /* switch from pristine spots to 'smart' */
          {
             _elm_hover_subs_del(sd);
             sd->smt_sub = _HOV_LEFT;
             sd->smt_sub->obj = content;

             _elm_hover_smt_sub_re_eval(obj);

             goto end;
          }
     }

   int_ret = efl_content_set(efl_part(efl_super(obj, MY_CLASS), swallow), content);
   if (!int_ret) return EINA_FALSE;

   if (!strncmp(swallow, "elm.swallow.slot.", sizeof("elm.swallow.slot")))
     swallow += sizeof("elm.swallow.slot");

   ELM_HOVER_PARTS_FOREACH
   {
      if (!strcmp(swallow, sd->subs[i].swallow))
        {
           sd->subs[i].obj = content;
           break;
        }
   }

end:
   elm_layout_sizing_eval(obj);
   return EINA_TRUE;
}

/**
 * @brief Gets content from a specified swallow part of the hover.
 * @param obj The hover object.
 * @param sd The hover's private data.
 * @param swallow The name of the swallow part.
 * @return The content Evas_Object, or NULL if none or on error.
 *
 * If "smart" is specified, it retrieves content from the currently active
 * smart slot. Otherwise, it retrieves from the named standard slot.
 */
static Evas_Object*
_elm_hover_content_get(Eo *obj, Elm_Hover_Data *sd, const char *swallow)
{
   if (!swallow) return NULL;

   if (!strcmp(swallow, "smart"))
     return efl_content_get(efl_part(efl_super(obj, MY_CLASS), sd->smt_sub->swallow));
   else
     return efl_content_get(efl_part(efl_super(obj, MY_CLASS), swallow));
}

/**
 * @brief Unsets (removes) content from a specified swallow part of the hover.
 * @param obj The hover object.
 * @param sd The hover's private data.
 * @param swallow The name of the swallow part.
 * @return The previously set content Evas_Object, or NULL if none or on error.
 *
 * If "smart" is specified, it unsets content from the currently active
 * smart slot. Otherwise, it unsets from the named standard slot.
 */
static Evas_Object*
_elm_hover_content_unset(Eo *obj, Elm_Hover_Data *sd, const char *swallow)
{
   if (!swallow) return NULL;

   if (!strcmp(swallow, "smart"))
     return efl_content_unset(efl_part(efl_super(obj, MY_CLASS), sd->smt_sub->swallow));
   else
     return efl_content_unset(efl_part(efl_super(obj, MY_CLASS), swallow));
}

/**
 * @brief Callback invoked when the hover's target object is deleted.
 * @param data The hover object.
 * @param e Unused.
 * @param obj Unused.
 * @param event_info Unused.
 *
 * Clears the hover's reference to the target.
 */
static void
_target_del_cb(void *data,
               Evas *e EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   ELM_HOVER_DATA_GET(data, sd);

   sd->target = NULL;
}

/**
 * @brief Callback invoked when the hover's target object is moved or resized.
 * @param data The hover object.
 * @param e Unused.
 * @param obj Unused.
 * @param event_info Unused.
 *
 * Triggers a sizing recalculation for the hover and re-evaluates the
 * position of "smart" content.
 */
static void
_target_move_cb(void *data,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   elm_layout_sizing_eval(data);
   _elm_hover_smt_sub_re_eval(data);
}

/**
 * @brief Emits signals to hide the hover and its content slots.
 * @param obj The hover object.
 *
 * This function sends Edje signals to the hover's theme to trigger
 * "hide" animations for the main hover area and any visible content slots.
 */
static void
_hide_signals_emit(Evas_Object *obj)
{
   ELM_HOVER_DATA_GET(obj, sd);

   elm_layout_signal_emit(obj, "elm,action,hide", "elm");

   ELM_HOVER_PARTS_FOREACH
     {
        char buf[1024];

        if (sd->subs[i].obj)
          {
             snprintf(buf, sizeof(buf), "elm,action,slot,%s,hide",
                      sd->subs[i].swallow);
             elm_layout_signal_emit(obj, buf, "elm");
          }
     }
}

/**
 * @brief Callback for the "elm,action,hide,finished" signal from the layout.
 * @param data The hover object.
 * @param obj Unused.
 * @param emission Unused.
 * @param source Unused.
 *
 * This function is called when the hover's hide animation (if any) finishes.
 * If the "dismiss" property is set to "on" in the theme, it hides the
 * hover object and emits the "dismissed" event.
 */
static void
_hov_hide_cb(void *data,
                Evas_Object *obj EINA_UNUSED,
                const char *emission EINA_UNUSED,
                const char *source EINA_UNUSED)
{
   const char *dismissstr;

   dismissstr = elm_layout_data_get(data, "dismiss");

   if (dismissstr && !strcmp(dismissstr, "on"))
     {
        evas_object_hide(data);
        efl_event_callback_legacy_call(data, ELM_HOVER_EVENT_DISMISSED, NULL);
     }
}

/**
 * @brief Callback for the "elm,action,dismiss" signal from the layout.
 * @param data The hover object.
 * @param obj Unused.
 * @param emission Unused.
 * @param source Unused.
 *
 * This function is typically triggered when the user clicks on the
 * background area of the hover.
 * If the "dismiss" property is "on", it emits hide signals and the "clicked"
 * smart callback.
 * Otherwise (for backward compatibility or different theme behavior), it
 * directly hides the hover and emits both "clicked" and "dismissed" callbacks.
 */
static void
_hov_dismiss_cb(void *data,
                Evas_Object *obj EINA_UNUSED,
                const char *emission EINA_UNUSED,
                const char *source EINA_UNUSED)
{
   const char *dismissstr;

   dismissstr = elm_layout_data_get(data, "dismiss");

   if (dismissstr && !strcmp(dismissstr, "on"))
     {
        _hide_signals_emit(data);
        evas_object_smart_callback_call
          ( data, "clicked", NULL);
     }
   else
     {
        evas_object_hide(data);
        evas_object_smart_callback_call
          ( data, "clicked", NULL);
        efl_event_callback_legacy_call(data, ELM_HOVER_EVENT_DISMISSED, NULL);
     } // for backward compatibility
}

EOLIAN static void
_elm_hover_efl_canvas_group_group_add(Eo *obj, Elm_Hover_Data *sd)
{
   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   ELM_HOVER_PARTS_FOREACH
     sd->subs[i].swallow = _content_aliases[i].alias;

   if (!elm_layout_theme_set(obj, "hover", "base", elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   elm_layout_signal_callback_add
     (obj, "elm,action,dismiss", "*", _hov_dismiss_cb, obj);
   elm_layout_signal_callback_add
     (obj, "elm,action,hide,finished", "elm", _hov_hide_cb, obj);

   sd->offset = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_pass_events_set(sd->offset, EINA_TRUE);
   evas_object_color_set(sd->offset, 0, 0, 0, 0);

   sd->size = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_pass_events_set(sd->size, EINA_TRUE);
   evas_object_color_set(sd->size, 0, 0, 0, 0);

   elm_layout_content_set(obj, "elm.swallow.offset", sd->offset);
   elm_layout_content_set(obj, "elm.swallow.size", sd->size);

   elm_widget_can_focus_set(obj, EINA_FALSE);
}

/**
 * @brief Cleans up the hover object when it's being deleted.
 * @param obj The hover object.
 * @param sd The hover's private data.
 *
 * This EOLIAN override is called during object destruction. It sets an
 * `on_del` flag, emits "clicked" and "dismissed" signals if the hover was
 * visible (for cleanup/notification), detaches from its target and parent,
 * and then calls the superclass's group_del method.
 */
EOLIAN static void
_elm_hover_efl_canvas_group_group_del(Eo *obj, Elm_Hover_Data *sd)
{

   sd->on_del = EINA_TRUE;

   if (evas_object_visible_get(obj))
     {
        evas_object_smart_callback_call
          ( obj, "clicked", NULL);
        efl_event_callback_legacy_call(obj, ELM_HOVER_EVENT_DISMISSED, NULL);
     }

   elm_hover_target_set(obj, NULL);

   _elm_hover_parent_detach(obj);
   sd->parent = NULL;

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @brief Sets the position of the hover object.
 * @param obj The hover object.
 * @param _pd Unused.
 * @param pos The new position (x, y).
 *
 * This EOLIAN override handles setting the hover's position. It first checks
 * for intercepting callbacks, then calls the superclass's position_set,
 * and finally triggers a sizing recalculation for the hover.
 */
EOLIAN static void
_elm_hover_efl_gfx_entity_position_set(Eo *obj, Elm_Hover_Data *_pd EINA_UNUSED, Eina_Position2D pos)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_MOVE, 0, pos.x, pos.y))
     return;

   efl_gfx_entity_position_set(efl_super(obj, MY_CLASS), pos);

   elm_layout_sizing_eval(obj);
}

/**
 * @brief Sets the size of the hover object.
 * @param obj The hover object.
 * @param _pd Unused.
 * @param sz The new size (width, height).
 *
 * This EOLIAN override handles setting the hover's size. It first checks
 * for intercepting callbacks, then calls the superclass's size_set,
 * and finally triggers a sizing recalculation for the hover.
 */
EOLIAN static void
_elm_hover_efl_gfx_entity_size_set(Eo *obj, Elm_Hover_Data *_pd EINA_UNUSED, Eina_Size2D sz)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_RESIZE, 0, sz.w, sz.h))
     return;

   efl_gfx_entity_size_set(efl_super(obj, MY_CLASS), sz);

   elm_layout_sizing_eval(obj);
}

/**
 * @brief Sets the visibility of the hover object.
 * @param obj The hover object.
 * @param pd Unused.
 * @param vis EINA_TRUE to show, EINA_FALSE to hide.
 *
 * This EOLIAN override handles setting the hover's visibility. It checks
 * for intercepting callbacks, calls the superclass's visible_set, and then
 * either emits "show" signals (if becoming visible) or "hide" signals
 * (if becoming hidden and not dismissed via theme "dismiss:on" property).
 */
EOLIAN static void
_elm_hover_efl_gfx_entity_visible_set(Eo *obj, Elm_Hover_Data *pd EINA_UNUSED, Eina_Bool vis)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_VISIBLE, 0, vis))
     return;

   efl_gfx_entity_visible_set(efl_super(obj, MY_CLASS), vis);

   if (vis) _hov_show_do(obj);
   else
     {
        // for backward compatibility
        const char *dismissstr = elm_layout_data_get(obj, "dismiss");

        if (!eina_streq(dismissstr, "on"))
          _hide_signals_emit(obj);
     }
}

/**
 * @brief Gets the appropriate content alias table based on the hover's style.
 * @param obj The hover object.
 * @param _pd Unused.
 * @return A pointer to an array of Elm_Layout_Part_Alias_Description.
 *
 * This function checks the current style of the hover. If the style is
 * "main_menu_submenu" (or contains it), it returns a specific alias table
 * that only defines the "bottom" slot. Otherwise, it returns the default
 * alias table with all standard directional slots.
 */
static const Elm_Layout_Part_Alias_Description*
_elm_hover_content_aliases_get(Eo *obj, void *_pd EINA_UNUSED)
{
   const char *style = elm_widget_style_get(obj);

   // main_menu_submenu only has a single slot "bottom"
   if (style && strstr(style, "main_menu_submenu"))
     return _content_aliases_main_menu_submenu;
   return _content_aliases;
}

EAPI Evas_Object *
elm_hover_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @brief Sets up the hover's relationship with its parent object.
 * @param obj The hover object.
 * @param sd The hover's private data.
 * @param parent The new parent object.
 *
 * This function first detaches the hover from any existing parent.
 * Then, if a new parent is provided, it sets up event callbacks on the
 * parent to monitor its move, resize, show, hide, and delete events.
 * Finally, it triggers a sizing recalculation for the hover.
 */
static void
_parent_setup(Eo *obj, Elm_Hover_Data *sd, Evas_Object *parent)
{
   _elm_hover_parent_detach(obj);

   sd->parent = parent;
   if (sd->parent)
     {
        evas_object_event_callback_add
          (sd->parent, EVAS_CALLBACK_MOVE, _parent_move_cb, obj);
        evas_object_event_callback_add
          (sd->parent, EVAS_CALLBACK_RESIZE, _parent_resize_cb, obj);
        evas_object_event_callback_add
          (sd->parent, EVAS_CALLBACK_SHOW, _parent_show_cb, obj);
        evas_object_event_callback_add
          (sd->parent, EVAS_CALLBACK_HIDE, _parent_hide_cb, obj);
        evas_object_event_callback_add
          (sd->parent, EVAS_CALLBACK_DEL, _parent_del_cb, obj);
     }

   elm_layout_sizing_eval(obj);
}

EOLIAN static Eo *
_elm_hover_efl_object_constructor(Eo *obj, Elm_Hover_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_POPUP_MENU);
   legacy_child_focus_handle(obj);
   _parent_setup(obj, pd, efl_parent_get(obj));

   return obj;
}

/**
 * @brief Sets the target object for the hover.
 * @param obj The hover object.
 * @param sd The hover's private data.
 * @param target The Evas_Object to be the hover's target.
 *
 * The hover will position itself relative to this target object.
 * This function detaches from any previous target, then sets up event
 * callbacks (DEL, MOVE, RESIZE) on the new target. It also informs the
 * target widget that it is being hovered by this hover object.
 */
EOLIAN static void
_elm_hover_target_set(Eo *obj, Elm_Hover_Data *sd, Evas_Object *target)
{

   if (sd->target)
     {
        evas_object_event_callback_del_full
          (sd->target, EVAS_CALLBACK_DEL, _target_del_cb, obj);
        evas_object_event_callback_del_full
          (sd->target, EVAS_CALLBACK_MOVE, _target_move_cb, obj);
        evas_object_event_callback_del_full
          (sd->target, EVAS_CALLBACK_RESIZE, _target_move_cb, obj);
        elm_widget_hover_object_set(sd->target, NULL);
     }

   sd->target = target;
   if (sd->target)
     {
        evas_object_event_callback_add
          (sd->target, EVAS_CALLBACK_DEL, _target_del_cb, obj);
        evas_object_event_callback_add
          (sd->target, EVAS_CALLBACK_MOVE, _target_move_cb, obj);
        evas_object_event_callback_add
          (sd->target, EVAS_CALLBACK_RESIZE, _target_move_cb, obj);
        elm_widget_hover_object_set(target, obj);
        elm_layout_sizing_eval(obj);
     }

}
EAPI void
elm_hover_parent_set(Evas_Object *obj,
                     Evas_Object *parent)
{
   ELM_HOVER_CHECK(obj);
   ELM_HOVER_DATA_GET(obj, sd);
   if (parent)
     efl_ui_widget_sub_object_add(parent, obj);
   _parent_setup(obj, sd, parent);
}

EOLIAN static Evas_Object*
_elm_hover_target_get(const Eo *obj EINA_UNUSED, Elm_Hover_Data *sd)
{
   return sd->target;
}

/**
 * @brief Gets the parent object of the hover.
 * @param obj The hover object.
 * @return The parent Evas_Object, or NULL if none.
 * @deprecated Use efl_ui_widget_parent_get() instead.
 */
EAPI Evas_Object *
elm_hover_parent_get(const Evas_Object *obj)
{
   ELM_HOVER_CHECK(obj) NULL;
   return efl_ui_widget_parent_get((Eo *) obj);
}

/**
 * @brief Determines the best content location based on available space and preferred axis.
 * @param obj Unused.
 * @param sd The hover's private data.
 * @param pref_axis The preferred axis (horizontal, vertical, or any) for placement.
 * @return The string name of the best swallow slot (e.g., "left", "top").
 *
 * This function calculates available space around the target and, based on
 * the `pref_axis`, suggests the most suitable content slot.
 * If `ELM_HOVER_AXIS_HORIZONTAL` is preferred, it chooses between "left" and "right".
 * If `ELM_HOVER_AXIS_VERTICAL` is preferred, it chooses between "top" and "bottom".
 * Otherwise, it picks the direction with the most space among the four cardinal directions.
 */
EOLIAN static const char*
_elm_hover_best_content_location_get(const Eo *obj EINA_UNUSED, Elm_Hover_Data *sd, Elm_Hover_Axis pref_axis)
{
   Evas_Coord spc_l, spc_r, spc_t, spc_b;

   _elm_hover_left_space_calc(sd, &spc_l, &spc_t, &spc_r, &spc_b);

   if (pref_axis == ELM_HOVER_AXIS_HORIZONTAL)
     {
        if (spc_l < spc_r) return (_HOV_RIGHT)->swallow;
        else return (_HOV_LEFT)->swallow;
     }
   else if (pref_axis == ELM_HOVER_AXIS_VERTICAL)
     {
        if (spc_t <= spc_b) return (_HOV_BOTTOM)->swallow;
        else return (_HOV_TOP)->swallow;
     }

   if (spc_l < spc_r)
     {
        if (spc_t > spc_r)
           return (_HOV_TOP)->swallow;
        else if (spc_b > spc_r)
           return (_HOV_BOTTOM)->swallow;
        else
           return (_HOV_RIGHT)->swallow;
     }

   if (spc_t > spc_r)
      return (_HOV_TOP)->swallow;
   else if (spc_b > spc_r)
      return (_HOV_BOTTOM)->swallow;
   else
      return (_HOV_LEFT)->swallow;

   return NULL;
}

/**
 * @brief Dismisses the hover.
 * @param obj The hover object.
 * @param _pd Unused.
 *
 * This function triggers the dismiss action for the hover. It checks a
 * theme-provided "dismiss" data item. If this item is not "on", it emits
 * an "elm,action,dismiss" signal with an empty source (for compatibility).
 * Regardless, it always emits "elm,action,dismiss" with "elm" as the source
 * to trigger the standard dismiss behavior defined in the theme.
 */
EOLIAN static void
_elm_hover_dismiss(Eo *obj, Elm_Hover_Data *_pd EINA_UNUSED)
{
   const char *dismissstr;

   dismissstr = elm_layout_data_get(obj, "dismiss");

   if (!dismissstr || strcmp(dismissstr, "on"))
     elm_layout_signal_emit(obj, "elm,action,dismiss", ""); // XXX: for compat
   elm_layout_signal_emit(obj, "elm,action,dismiss", "elm");
}

EOLIAN static void
_elm_hover_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @brief Accessibility action to dismiss the hover.
 * @param obj The hover object.
 * @param params Unused.
 * @return EINA_TRUE, indicating the action was handled.
 */
static Eina_Bool
_action_dismiss(Evas_Object *obj, const char *params EINA_UNUSED)
{
   elm_obj_hover_dismiss(obj);
   return EINA_TRUE;
}

/**
 * @brief Gets the accessibility actions for the hover widget.
 * @param obj Unused.
 * @param pd Unused.
 * @return A pointer to an array of Efl_Access_Action_Data.
 *
 * Provides the "dismiss" action for accessibility tools.
 */
EOLIAN const Efl_Access_Action_Data *
_elm_hover_efl_access_widget_action_elm_actions_get(const Eo *obj EINA_UNUSED, Elm_Hover_Data *pd EINA_UNUSED)
{
   static Efl_Access_Action_Data atspi_actions[] = {
          { "dismiss", NULL, NULL, _action_dismiss},
          { NULL, NULL, NULL, NULL}
   };
   return &atspi_actions[0];
}

/**
 * @brief Gets the accessibility state set for the hover object.
 * @param obj The hover object.
 * @param pd Unused.
 * @return The accessibility state set.
 *
 * This EOLIAN override calls the superclass method to get the base states
 * and then adds the `EFL_ACCESS_STATE_TYPE_MODAL` state, as hovers typically
 * behave like modal dialogs.
 */
EOLIAN static Efl_Access_State_Set
_elm_hover_efl_access_object_state_set_get(const Eo *obj, Elm_Hover_Data *pd EINA_UNUSED)
{
   Efl_Access_State_Set states;
   states = efl_access_object_state_set_get(efl_super(obj, MY_CLASS));

   STATE_TYPE_SET(states, EFL_ACCESS_STATE_TYPE_MODAL);
   return states;
}

/* Efl.Part begin */

ELM_PART_OVERRIDE(elm_hover, ELM_HOVER, Elm_Hover_Data)
ELM_PART_OVERRIDE_CONTENT_SET(elm_hover, ELM_HOVER, Elm_Hover_Data)
ELM_PART_OVERRIDE_CONTENT_GET(elm_hover, ELM_HOVER, Elm_Hover_Data)
ELM_PART_OVERRIDE_CONTENT_UNSET(elm_hover, ELM_HOVER, Elm_Hover_Data)
#include "elm_hover_part.eo.c"

/* Efl.Part end */

/* Internal EO APIs and hidden overrides */

// EFL_UI_LAYOUT_CONTENT_ALIASES_IMPLEMENT(MY_CLASS_PFX) is overridden with an if()
// EFL_UI_LAYOUT_CONTENT_ALIASES_OPS(MY_CLASS_PFX) somehow doesn't compile!?

#define ELM_HOVER_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_hover), \
   EFL_CANVAS_GROUP_CALC_OPS(elm_hover), \
   _EFL_UI_LAYOUT_ALIASES_OPS(elm_hover, content)

#include "elm_hover_eo.c"
