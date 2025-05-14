#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_mapbuf.h"
#include "elm_widget_container.h"
#include "elm_mapbuf_eo.h"

#include "elm_mapbuf_part.eo.h"
#include "elm_part_helper.h"

#define MY_CLASS ELM_MAPBUF_CLASS

#define MY_CLASS_NAME "Elm_Mapbuf"
#define MY_CLASS_NAME_LEGACY "elm_mapbuf"

/**
 * @internal
 * @brief Recalculates and sets the size hints for the mapbuf object.
 *
 * This function retrieves the size hints from the content object, if any,
 * and applies them to the mapbuf object itself. It ensures that the mapbuf
 * correctly reports its minimum and maximum dimensions based on its content.
 *
 * @param obj The mapbuf Evas_Object.
 */
static void
_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = 0, minh = 0;
   Evas_Coord maxw = -1, maxh = -1;

   ELM_MAPBUF_DATA_GET(obj, sd);
   if (sd->content)
     {
        evas_object_size_hint_combined_min_get(sd->content, &minw, &minh);
        evas_object_size_hint_max_get(sd->content, &maxw, &maxh);
     }
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

/**
 * @internal
 * @brief Applies the theme to the mapbuf widget and re-evaluates its sizing.
 *
 * This function calls the parent class's theme apply function and then
 * triggers a re-evaluation of the mapbuf's size hints.
 *
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @return Eina_Error EFL_UI_THEME_APPLY_ERROR_GENERIC on failure, or the result of the parent's theme_apply.
 */
EOLIAN static Eina_Error
_elm_mapbuf_efl_ui_widget_theme_apply(Eo *obj, Elm_Mapbuf_Data *sd EINA_UNUSED)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   _sizing_eval(obj);

   return int_ret;
}

/**
 * @internal
 * @brief Callback function invoked when the size hints of the content object change.
 *
 * This function calls _sizing_eval to update the mapbuf's size hints accordingly.
 *
 * @param data The mapbuf Evas_Object (passed as user data).
 * @param e Evas canvas (unused).
 * @param obj The content Evas_Object that triggered the callback (unused).
 * @param event_info Event-specific information (unused).
 */
static void
_changed_size_hints_cb(void *data,
                       Evas *e EINA_UNUSED,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   _sizing_eval(data);
}

/**
 * @internal
 * @brief Internal logic for unsetting content from the mapbuf.
 *
 * This function performs the necessary cleanup when content is removed,
 * such as deleting data associated with the content, removing it as a smart member,
 * unsetting its clip, and cleaning up event callbacks.
 *
 * @param sd The private data of the mapbuf object.
 * @param obj The mapbuf Evas_Object.
 * @param content The Evas_Object to be unset as content.
 */
static void
_elm_mapbuf_content_unset_internal(Elm_Mapbuf_Data *sd, Evas_Object *obj,
                                   Evas_Object *content)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_data_del(content, "_elm_leaveme");
   evas_object_smart_member_del(content);
   evas_object_clip_unset(content);
   evas_object_color_set(wd->resize_obj, 0, 0, 0, 0);
   evas_object_event_callback_del_full
      (content, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints_cb,
       obj);
   sd->content = NULL;
   _sizing_eval(obj);
   ELM_SAFE_FREE(sd->idler, ecore_idler_del);
}

/**
 * @internal
 * @brief Handles the deletion of a sub-object from the mapbuf widget.
 *
 * If the sub-object being deleted is the current content of the mapbuf,
 * this function ensures that the content is properly unset.
 *
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @param sobj The sub-object being deleted.
 * @return EINA_TRUE if the sub-object was successfully handled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_mapbuf_efl_ui_widget_widget_sub_object_del(Eo *obj, Elm_Mapbuf_Data *sd, Evas_Object *sobj)
{
   Eina_Bool int_ret = EINA_FALSE;
   int_ret = elm_widget_sub_object_del(efl_super(obj, MY_CLASS), sobj);
   if (!int_ret) return EINA_FALSE;

   if (sobj == sd->content)
     _elm_mapbuf_content_unset_internal(sd, (Evas_Object *)obj, sobj);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Configures the mapbuf's content based on its current state (enabled, visibility, geometry).
 *
 * This function is responsible for setting up the Evas_Map on the content
 * object if mapbuf is enabled, or simply moving the content if disabled.
 * It populates the map with geometry and color data.
 *
 * @param obj The mapbuf Evas_Object.
 */
static void
_configure(Evas_Object *obj)
{
   ELM_MAPBUF_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (!sd->content) return;
   if (sd->enabled && !evas_object_visible_get(obj)) return;

   Evas_Coord x, y, w, h;
   int i;
   evas_object_geometry_get(wd->resize_obj, &x, &y, &w, &h);

   if (sd->enabled)
     {
        if (!sd->map) sd->map = evas_map_new(4);
        evas_map_util_points_populate_from_geometry(sd->map, x, y, w, h, 0);
        for (i = 0; i < (int)(sizeof(sd->colors)/sizeof(sd->colors[0])); i++)
          {
             evas_map_point_color_set(sd->map, i, sd->colors[i].r,
                                      sd->colors[i].g, sd->colors[i].b,
                                      sd->colors[i].a);
          }

        evas_map_smooth_set(sd->map, sd->smooth);
        evas_map_alpha_set(sd->map, sd->alpha);
        evas_object_map_set(sd->content, sd->map);
        evas_object_map_enable_set(sd->content, EINA_TRUE);
     }
   else
     evas_object_move(sd->content, x, y);
}

/**
 * @internal
 * @brief Evaluates whether the mapbuf should be enabled or disabled when in 'auto' mode.
 *
 * In 'auto' mode, the mapbuf is enabled if it is visible and intersects with the
 * Evas output viewport. Otherwise, it is disabled.
 *
 * @param obj The mapbuf Evas_Object.
 * @param sd The private data of the mapbuf object.
 */
static void
_mapbuf_auto_eval(Evas_Object *obj, Elm_Mapbuf_Data *sd)
{
   Eina_Bool vis;
   Evas_Coord x, y, w, h;
   Evas_Coord vx, vy, vw, vh;
   Eina_Bool on = EINA_FALSE;

   if (!sd->automode) return ;
   vis = evas_object_visible_get(obj);
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_output_viewport_get(evas_object_evas_get(obj), &vx, &vy, &vw, &vh);
   if ((vis) && (ELM_RECTS_INTERSECT(x, y, w, h, vx, vy, vw, vh)))
     on = EINA_TRUE;
   elm_mapbuf_enabled_set(obj, on);
}

/**
 * @internal
 * @brief Idler callback function to restore smooth scaling after a move operation in auto mode.
 *
 * When auto mode is active and the mapbuf is moved, smooth scaling might be temporarily
 * disabled for performance. This idler restores the saved smooth setting.
 *
 * @param data The private data of the mapbuf object (Elm_Mapbuf_Data *).
 * @return EINA_FALSE to remove the idler after execution.
 */
static Eina_Bool
_mapbuf_move_end(void *data)
{
   Elm_Mapbuf_Data *sd = data;

   elm_mapbuf_smooth_set(sd->self, sd->smooth_saved);
   sd->idler = NULL;

   return EINA_FALSE;
}

/**
 * @internal
 * @brief Manages smooth scaling behavior during move operations in auto mode.
 *
 * If auto mode is enabled, this function schedules an idler (_mapbuf_move_end)
 * to restore smooth scaling after the move is complete. It temporarily disables
 * smooth scaling during the move for better performance.
 *
 * @param obj The mapbuf Evas_Object (unused).
 * @param sd The private data of the mapbuf object.
 */
static void
_mapbuf_auto_smooth(Evas_Object *obj EINA_UNUSED, Elm_Mapbuf_Data *sd)
{
   if (!sd->automode) return ;
   if (!sd->idler) sd->idler = ecore_idler_add(_mapbuf_move_end, sd);
   sd->smooth = EINA_FALSE;
}

/**
 * @internal
 * @brief Sets the position of the mapbuf object.
 *
 * This function calls the parent's position_set, then evaluates auto mode,
 * handles auto smoothing, and reconfigures the mapbuf.
 *
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @param pos The new 2D position (Eina_Position2D) for the object.
 *            Example: { .x = 10, .y = 20 }
 */
EOLIAN static void
_elm_mapbuf_efl_gfx_entity_position_set(Eo *obj, Elm_Mapbuf_Data *sd, Eina_Position2D pos)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_MOVE, 0, pos.x, pos.y))
     return;

   efl_gfx_entity_position_set(efl_super(obj, MY_CLASS), pos);

   _mapbuf_auto_eval(obj, sd);
   _mapbuf_auto_smooth(obj, sd);
   _configure(obj);
}

/**
 * @internal
 * @brief Sets the size of the mapbuf object.
 *
 * This function calls the parent's size_set, resizes the content object if present,
 * then evaluates auto mode and reconfigures the mapbuf.
 *
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @param sz The new 2D size (Eina_Size2D) for the object.
 *           Example: { .w = 100, .h = 50 }
 */
EOLIAN static void
_elm_mapbuf_efl_gfx_entity_size_set(Eo *obj, Elm_Mapbuf_Data *sd, Eina_Size2D sz)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_RESIZE, 0, sz.w, sz.h))
     return;

   efl_gfx_entity_size_set(efl_super(obj, MY_CLASS), sz);
   if (sd->content)
     efl_gfx_entity_size_set(sd->content, sz);

   _mapbuf_auto_eval(obj, sd);
   _configure(obj);
}

/**
 * @internal
 * @brief Sets the visibility of the mapbuf object.
 *
 * This function calls the parent's visible_set, then evaluates auto mode
 * and reconfigures the mapbuf.
 *
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @param vis EINA_TRUE if the object should be visible, EINA_FALSE otherwise.
 */
EOLIAN static void
_elm_mapbuf_efl_gfx_entity_visible_set(Eo *obj, Elm_Mapbuf_Data *sd, Eina_Bool vis)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_VISIBLE, 0, vis))
     return;

   efl_gfx_entity_visible_set(efl_super(obj, MY_CLASS), vis);

   _mapbuf_auto_eval(obj, sd);
   _configure(obj);
}

/**
 * @internal
 * @brief Sets the content of the mapbuf for a given part.
 *
 * This function handles setting a new content object. It removes any existing
 * content, adds the new content as a sub-object, sets up necessary callbacks
 * and properties, and then re-evaluates sizing and reconfigures the mapbuf.
 * Currently, only the "default" part is supported.
 *
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @param part The name of the part to set content for (e.g., "default").
 * @param content The Evas_Object to set as content.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid part name).
 */
static Eina_Bool
_elm_mapbuf_content_set(Eo *obj, Elm_Mapbuf_Data *sd, const char *part, Evas_Object *content)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);

   if (part && strcmp(part, "default")) return EINA_FALSE;
   if (sd->content == content) return EINA_TRUE;

   evas_object_del(sd->content);
   sd->content = content;

   if (content)
     {
        evas_object_data_set(content, "_elm_leaveme", (void *)1);
        elm_widget_sub_object_add(obj, content);
        evas_object_smart_member_add(content, obj);
        evas_object_clip_set(content, wd->resize_obj);
        evas_object_color_set
          (wd->resize_obj, 255, 255, 255, 255);
        evas_object_event_callback_add
          (content, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
     }
   else
     evas_object_color_set(wd->resize_obj, 0, 0, 0, 0);
   efl_event_callback_call(obj, EFL_CONTENT_EVENT_CONTENT_CHANGED, content);
   _sizing_eval(obj);
   _configure(obj);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the content of the mapbuf for a given part.
 *
 * Currently, only the "default" part is supported.
 *
 * @param obj The mapbuf Eo object (unused).
 * @param sd The private data of the mapbuf object.
 * @param part The name of the part to get content from (e.g., "default").
 * @return The Evas_Object set as content for the part, or NULL if no content or invalid part.
 */
static Evas_Object*
_elm_mapbuf_content_get(const Eo *obj EINA_UNUSED, Elm_Mapbuf_Data *sd, const char *part)
{
   if (part && strcmp(part, "default")) return NULL;
   return sd->content;
}

/**
 * @internal
 * @brief Unsets (removes) the content of the mapbuf for a given part.
 *
 * This function removes the content object, performs necessary cleanup,
 * and returns the unset content object. Currently, only the "default" part is supported.
 *
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @param part The name of the part to unset content from (e.g., "default").
 * @return The Evas_Object that was unset, or NULL if no content or invalid part.
 */
static Evas_Object*
_elm_mapbuf_content_unset(Eo *obj, Elm_Mapbuf_Data *sd, const char *part)
{
   Evas_Object *content;
   if (part && strcmp(part, "default")) return NULL;
   if (!sd->content) return NULL;

   content = sd->content;
   _elm_widget_sub_object_redirect_to_top(obj, content);
   _elm_mapbuf_content_unset_internal(sd, obj, content);
   efl_event_callback_call(obj, EFL_CONTENT_EVENT_CONTENT_CHANGED, NULL);
   return content;
}

/**
 * @internal
 * @brief Implements Efl.Content.content_set for the default content part.
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @param content The Evas_Object to set as content.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_elm_mapbuf_efl_content_content_set(Eo *obj, Elm_Mapbuf_Data *sd, Evas_Object *content)
{
   return _elm_mapbuf_content_set(obj, sd, NULL, content);
}

/**
 * @internal
 * @brief Implements Efl.Content.content_get for the default content part.
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @return The Evas_Object set as content, or NULL.
 */
EOLIAN static Evas_Object*
_elm_mapbuf_efl_content_content_get(const Eo *obj, Elm_Mapbuf_Data *sd)
{
   return _elm_mapbuf_content_get(obj, sd, NULL);
}

/**
 * @internal
 * @brief Implements Efl.Content.content_unset for the default content part.
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @return The Evas_Object that was unset, or NULL.
 */
EOLIAN static Evas_Object*
_elm_mapbuf_efl_content_content_unset(Eo *obj, Elm_Mapbuf_Data *sd)
{
   return _elm_mapbuf_content_unset(obj, sd, NULL);
}

/**
 * @internal
 * @brief Handles the deletion of the mapbuf object as a canvas group.
 *
 * This function cleans up resources specific to the mapbuf, such as the
 * ecore idler and the Evas_Map, before calling the parent's group_del.
 *
 * @param obj The mapbuf Eo object.
 * @param priv The private data of the mapbuf object.
 */
EOLIAN static void
_elm_mapbuf_efl_canvas_group_group_del(Eo *obj, Elm_Mapbuf_Data *priv)
{
   ELM_SAFE_FREE(priv->idler, ecore_idler_del);
   ELM_SAFE_FREE(priv->map, evas_map_free);

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Handles the addition of the mapbuf object as a canvas group.
 *
 * This function initializes the mapbuf, sets up its resize object (a transparent
 * rectangle), initializes default color and state values (alpha, smooth),
 * and sets initial sizing.
 *
 * @param obj The mapbuf Eo object.
 * @param priv The private data of the mapbuf object.
 */
EOLIAN static void
_elm_mapbuf_efl_canvas_group_group_add(Eo *obj, Elm_Mapbuf_Data *priv)
{
   Evas_Object *rect = evas_object_rectangle_add(evas_object_evas_get(obj));
   int i;

   elm_widget_resize_object_set(obj, rect);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   evas_object_static_clip_set(rect, EINA_TRUE);
   evas_object_pass_events_set(rect, EINA_TRUE);
   evas_object_color_set(rect, 0, 0, 0, 0);

   for (i = 0; i < (int)(sizeof(priv->colors)/sizeof(priv->colors[0])); i++)
     {
        priv->colors[i].r = 255;
        priv->colors[i].g = 255;
        priv->colors[i].b = 255;
        priv->colors[i].a = 255;
     }

   priv->self = obj;
   priv->alpha = EINA_TRUE;
   priv->smooth = EINA_TRUE;

   elm_widget_can_focus_set(obj, EINA_FALSE);

   _sizing_eval(obj);
}

EAPI Evas_Object *
elm_mapbuf_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Constructor for the mapbuf Eo object.
 *
 * Calls the parent constructor and sets the legacy type name and accessibility role.
 *
 * @param obj The mapbuf Eo object being constructed.
 * @param sd The private data of the mapbuf object (unused in this function).
 * @return The constructed Eo object.
 */
EOLIAN static Eo *
_elm_mapbuf_efl_object_constructor(Eo *obj, Elm_Mapbuf_Data *sd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_IMAGE_MAP);

   return obj;
}

/**
 * @internal
 * @brief Internal function to set the enabled state of the mapbuf.
 *
 * This function changes the enabled state and, if disabling, ensures
 * that any Evas_Map is removed from the content. It then reconfigures the mapbuf.
 *
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @param enabled EINA_TRUE to enable map mode, EINA_FALSE to disable.
 */
static void
_internal_enable_set(Eo *obj, Elm_Mapbuf_Data *sd, Eina_Bool enabled)
{
   if (sd->enabled == enabled) return;
   sd->enabled = enabled;

   if (!sd->enabled && sd->content)
     {
        evas_object_map_set(sd->content, NULL);
        evas_object_map_enable_set(sd->content, EINA_FALSE);
     }
   _configure(obj);
}

/**
 * @internal
 * @brief Sets whether the map rendering is enabled or not.
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @param enabled EINA_TRUE to enable map mode, EINA_FALSE to disable.
 */
EOLIAN static void
_elm_mapbuf_enabled_set(Eo *obj, Elm_Mapbuf_Data *sd, Eina_Bool enabled)
{
   _internal_enable_set(obj, sd, enabled);
}

/**
 * @internal
 * @brief Gets whether the map rendering is enabled or not.
 * @param obj The mapbuf Eo object (unused).
 * @param sd The private data of the mapbuf object.
 * @return EINA_TRUE if map mode is enabled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_mapbuf_enabled_get(const Eo *obj EINA_UNUSED, Elm_Mapbuf_Data *sd)
{
   return sd->enabled;
}

/**
 * @internal
 * @brief Sets whether smooth map rendering is enabled.
 *
 * Smooth rendering provides better quality but may be slower.
 *
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @param smooth EINA_TRUE for smooth rendering, EINA_FALSE for non-smooth.
 */
EOLIAN static void
_elm_mapbuf_smooth_set(Eo *obj, Elm_Mapbuf_Data *sd, Eina_Bool smooth)
{
   if (sd->smooth == smooth) return;
   sd->smooth = smooth;
   sd->smooth_saved = smooth;
   _configure(obj);
}

/**
 * @internal
 * @brief Gets whether smooth map rendering is enabled.
 * @param obj The mapbuf Eo object (unused).
 * @param sd The private data of the mapbuf object.
 * @return EINA_TRUE if smooth rendering is enabled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_mapbuf_smooth_get(const Eo *obj EINA_UNUSED, Elm_Mapbuf_Data *sd)
{
   return sd->smooth;
}

/**
 * @internal
 * @brief Sets whether alpha channel is used for map rendering.
 *
 * If alpha is disabled, the content will be opaque.
 *
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @param alpha EINA_TRUE to enable alpha, EINA_FALSE to disable.
 */
EOLIAN static void
_elm_mapbuf_alpha_set(Eo *obj, Elm_Mapbuf_Data *sd, Eina_Bool alpha)
{
   if (sd->alpha == alpha) return;
   sd->alpha = alpha;
   _configure(obj);
}

/**
 * @internal
 * @brief Gets whether alpha channel is used for map rendering.
 * @param obj The mapbuf Eo object (unused).
 * @param sd The private data of the mapbuf object.
 * @return EINA_TRUE if alpha is enabled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_mapbuf_alpha_get(const Eo *obj EINA_UNUSED, Elm_Mapbuf_Data *sd)
{
   return sd->alpha;
}

/**
 * @internal
 * @brief Sets the auto mode for enabling/disabling the map.
 *
 * In auto mode, the map is enabled only when the mapbuf is visible and
 * intersects the viewport. When auto mode is turned off, the map is explicitly
 * disabled.
 *
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @param on EINA_TRUE to enable auto mode, EINA_FALSE to disable.
 */
EOLIAN static void
_elm_mapbuf_auto_set(Eo *obj, Elm_Mapbuf_Data *sd, Eina_Bool on)
{
   if (sd->automode == on) return;
   sd->automode = on;
   if (on)
     {
        _mapbuf_auto_eval(obj, sd);
     }
   else
     {
        ELM_SAFE_FREE(sd->idler, ecore_idler_del);

        _internal_enable_set(obj, sd, EINA_FALSE);
     }
   _configure(obj);
}

/**
 * @internal
 * @brief Gets the current state of auto mode.
 * @param obj The mapbuf Eo object (unused).
 * @param sd The private data of the mapbuf object.
 * @return EINA_TRUE if auto mode is enabled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_mapbuf_auto_get(const Eo *obj EINA_UNUSED, Elm_Mapbuf_Data *sd)
{
   return sd->automode;
}

/**
 * @internal
 * @brief Gets the color of a specific point in the map.
 *
 * The map has 4 points, indexed 0 to 3.
 *
 * @param obj The mapbuf Eo object (unused).
 * @param sd The private data of the mapbuf object.
 * @param idx The index of the point (0-3).
 * @param r Pointer to store the red component (0-255).
 * @param g Pointer to store the green component (0-255).
 * @param b Pointer to store the blue component (0-255).
 * @param a Pointer to store the alpha component (0-255).
 */
EOLIAN static void
_elm_mapbuf_point_color_get(const Eo *obj EINA_UNUSED, Elm_Mapbuf_Data *sd, int idx, int *r, int *g, int *b, int *a)
{
   if ((idx < 0) || (idx >= 4))
     {
        ERR("idx value should be 0 ~ 4");
        return;
     }
   *r = sd->colors[idx].r;
   *g = sd->colors[idx].g;
   *b = sd->colors[idx].b;
   *a =sd->colors[idx].a;
}

/**
 * @internal
 * @brief Sets the color of a specific point in the map.
 *
 * The map has 4 points, indexed 0 to 3. Setting a point's color
 * will reconfigure the map.
 * Example for `sd->colors` array structure:
 * sd->colors[0] = { .r = 255, .g = 0, .b = 0, .a = 255 }; // Top-left point red
 * sd->colors[1] = { .r = 0, .g = 255, .b = 0, .a = 255 }; // Top-right point green
 * sd->colors[2] = { .r = 0, .g = 0, .b = 255, .a = 255 }; // Bottom-right point blue
 * sd->colors[3] = { .r = 255, .g = 255, .b = 0, .a = 128 }; // Bottom-left point yellow, semi-transparent
 *
 * @param obj The mapbuf Eo object.
 * @param sd The private data of the mapbuf object.
 * @param idx The index of the point (0-3).
 * @param r The red component (0-255).
 * @param g The green component (0-255).
 * @param b The blue component (0-255).
 * @param a The alpha component (0-255).
 */
EOLIAN static void
_elm_mapbuf_point_color_set(Eo *obj EINA_UNUSED, Elm_Mapbuf_Data *sd, int idx, int r, int g, int b, int a)
{
   if ((idx < 0) || (idx >= 4))
     {
        ERR("idx value should be 0 ~ 4");
        return;
     }
   sd->colors[idx].r = r;
   sd->colors[idx].g = g;
   sd->colors[idx].b = b;
   sd->colors[idx].a = a;

   _configure(obj);
}

/**
 * @internal
 * @brief Class constructor for Elm_Mapbuf.
 *
 * Registers the legacy Evas smart type for this class.
 *
 * @param klass The Efl_Class for Elm_Mapbuf.
 */
static void
_elm_mapbuf_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Efl.Part begin */

ELM_PART_OVERRIDE(elm_mapbuf, ELM_MAPBUF, Elm_Mapbuf_Data)
ELM_PART_OVERRIDE_CONTENT_SET(elm_mapbuf, ELM_MAPBUF, Elm_Mapbuf_Data)
ELM_PART_OVERRIDE_CONTENT_GET(elm_mapbuf, ELM_MAPBUF, Elm_Mapbuf_Data)
ELM_PART_OVERRIDE_CONTENT_UNSET(elm_mapbuf, ELM_MAPBUF, Elm_Mapbuf_Data)
ELM_PART_CONTENT_DEFAULT_GET(elm_mapbuf, "default")
#include "elm_mapbuf_part.eo.c"

/* Efl.Part end */

/* Internal EO APIs and hidden overrides */

#define ELM_MAPBUF_EXTRA_OPS \
   ELM_PART_CONTENT_DEFAULT_OPS(elm_mapbuf), \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_mapbuf)

#include "elm_mapbuf_eo.c"
