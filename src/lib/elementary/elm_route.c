#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_route.h"

#define MY_CLASS ELM_ROUTE_CLASS

#define MY_CLASS_NAME "Elm_Route"
#define MY_CLASS_NAME_LEGACY "elm_route"

/**
 * @internal
 * @brief Clears all visual segments of the route.
 *
 * This function iterates over all segments stored in the route's private data,
 * deletes their Evas objects, and frees the segment structures. It also resets
 * the min/max longitude and latitude boundaries if ELM_EMAP is defined.
 *
 * @param obj The route Evas_Object.
 */
static void
_clear_route(Evas_Object *obj)
{
   Segment *segment;

   ELM_ROUTE_DATA_GET(obj, sd);

#ifdef ELM_EMAP
   sd->lon_min = EMAP_LON_MAX;
   sd->lon_max = EMAP_LON_MIN;
   sd->lat_min = EMAP_LAT_MAX;
   sd->lat_max = EMAP_LAT_MIN;
#endif

   EINA_LIST_FREE(sd->segments, segment)
     {
        evas_object_del(segment->obj);
        free(segment);
     }
}

/**
 * @internal
 * @brief Recalculates and redraws the route segments.
 *
 * This function is called when the route widget is resized, moved, or when
 * the underlying map data changes in a way that requires recalculating segment
 * positions (e.g., zoom level or bounds change). It iterates through all
 * segments, recalculates their start and end coordinates relative to the
 * widget's current geometry and the route's geographic bounds (if ELM_EMAP
 * is defined), and then updates the Evas line objects.
 *
 * @param obj The route Evas_Object.
 */
static void
_sizing_eval(Evas_Object *obj)
{
   Eina_List *l;
   Segment *segment;
   Evas_Coord x, y, w, h;
   Evas_Coord start_x, start_y, end_x, end_y;

   ELM_ROUTE_DATA_GET(obj, sd);

   evas_object_geometry_get(obj, &x, &y, &w, &h);

   EINA_LIST_FOREACH(sd->segments, l, segment)
     {
        if (sd->must_calc_segments || segment->must_calc)
          {
#ifdef ELM_EMAP
             segment->start_x =
               (emap_route_node_lon_get(segment->node_start) - sd->lon_min)
               / (float)(sd->lon_max - sd->lon_min);
             segment->start_y =
               1 - (emap_route_node_lat_get(segment->node_start)
                    - sd->lat_min) / (float)(sd->lat_max - sd->lat_min);
             segment->end_x =
               (emap_route_node_lon_get(segment->node_end) - sd->lon_min)
               / (float)(sd->lon_max - sd->lon_min);
             segment->end_y =
               1 - (emap_route_node_lat_get(segment->node_end)
                    - sd->lat_min) / (float)(sd->lat_max - sd->lat_min);
#endif
             segment->must_calc = EINA_FALSE;
          }

        start_x = x + (int)(segment->start_x * w);
        start_y = y + (int)(segment->start_y * h);
        end_x = x + (int)(segment->end_x * w);
        end_y = y + (int)(segment->end_y * h);

        evas_object_line_xy_set(segment->obj, start_x, start_y, end_x, end_y);
     }

   sd->must_calc_segments = EINA_FALSE;
}

/**
 * @internal
 * @brief Callback function for EVAS_CALLBACK_MOVE and EVAS_CALLBACK_RESIZE events.
 *
 * Triggers a recalculation and redraw of the route segments.
 *
 * @param data User data (unused).
 * @param e The Evas canvas (unused).
 * @param obj The Evas_Object that received the event.
 * @param event_info Event-specific information (unused).
 */
static void
_move_resize_cb(void *data EINA_UNUSED,
                Evas *e EINA_UNUSED,
                Evas_Object *obj,
                void *event_info EINA_UNUSED)
{
   _sizing_eval(obj);
}

/**
 * @internal
 * @brief Applies the theme to the route widget.
 *
 * This function is called when the widget's theme needs to be updated.
 * It calls the parent's theme apply function and then re-evaluates sizing.
 *
 * @param obj The Eo object.
 * @param sd Private data of the route widget (unused in this specific override, but part of the Eolian signature).
 * @return Eina_Error indicating success or failure.
 */
EOLIAN static Eina_Error
_elm_route_efl_ui_widget_theme_apply(Eo *obj, Elm_Route_Data *sd EINA_UNUSED)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   //TODO

   _sizing_eval(obj);

   return int_ret;
}

#ifdef ELM_EMAP
/**
 * @internal
 * @brief Updates the minimum and maximum longitude and latitude values for the route.
 *
 * This function is called when adding nodes to the route to keep track of
 * the overall geographic extent of the route. If the new longitude or latitude
 * extends the current bounds, the bounds are updated, and a flag
 * `must_calc_segments` is set to true to trigger a recalculation of segment
 * positions.
 *
 * @param obj The route Evas_Object.
 * @param lon The longitude of a point in the route.
 * @param lat The latitude of a point in the route.
 */
static void
_update_lon_lat_min_max(Evas_Object *obj,
                        double lon,
                        double lat)
{
   ELM_ROUTE_DATA_GET(obj, sd);

   if (sd->lon_min > lon)
     {
        sd->lon_min = lon;
        sd->must_calc_segments = EINA_TRUE;
     }
   if (sd->lat_min > lat)
     {
        sd->lat_min = lat;
        sd->must_calc_segments = EINA_TRUE;
     }

   if (sd->lon_max < lon)
     {
        sd->lon_max = lon;
        sd->must_calc_segments = EINA_TRUE;
     }
   if (sd->lat_max < lat)
     {
        sd->lat_max = lat;
        sd->must_calc_segments = EINA_TRUE;
     }
}

#endif

/**
 * @internal
 * @brief Efl_Canvas_Group group_add override. Called when the route object is added to a canvas.
 *
 * Initializes the route object. This includes:
 * - Calling the parent's group_add method.
 * - Setting the widget to be non-focusable.
 * - Adding callbacks for move and resize events to trigger `_sizing_eval`.
 * - Initializing longitude and latitude boundaries (if ELM_EMAP is defined).
 * - Performing an initial sizing evaluation.
 *
 * @param obj The Eo object being added.
 * @param priv The private data for the Elm_Route object.
 */
EOLIAN static void
_elm_route_efl_canvas_group_group_add(Eo *obj, Elm_Route_Data *priv)
{

   efl_canvas_group_add(efl_super(obj, MY_CLASS));
   elm_widget_can_focus_set(obj, EINA_FALSE);

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_MOVE, _move_resize_cb, obj);
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_RESIZE, _move_resize_cb, obj);

#ifdef ELM_EMAP
   priv->lon_min = EMAP_LON_MAX;
   priv->lon_max = EMAP_LON_MIN;
   priv->lat_min = EMAP_LAT_MAX;
   priv->lat_max = EMAP_LAT_MIN;
#else
   (void)priv;
#endif

   _sizing_eval(obj);
}

/**
 * @internal
 * @brief Efl_Canvas_Group group_del override. Called when the route object is being deleted.
 *
 * Cleans up resources used by the route object. This includes:
 * - Clearing all route segments via `_clear_route`.
 * - Calling the parent's group_del method.
 *
 * @param obj The Eo object being deleted.
 * @param _pd The private data for the Elm_Route object (unused).
 */
EOLIAN static void
_elm_route_efl_canvas_group_group_del(Eo *obj, Elm_Route_Data *_pd EINA_UNUSED)
{
   _clear_route(obj);

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @brief Add a new route widget to a parent Evas object.
 *
 * This function creates a new Elm_Route object as a child of the given parent.
 * The route widget is used to display a path, typically on a map.
 *
 * @param parent The Evas_Object to which the new route widget will be added.
 *               Must not be NULL.
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Elm_Route
 */
EAPI Evas_Object *
elm_route_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Efl_Object constructor override for Elm_Route.
 *
 * Calls the parent class constructor and sets the Evas object type
 * for legacy compatibility.
 *
 * @param obj The Eo object being constructed.
 * @param _pd The private data for the Elm_Route object (unused).
 * @return The constructed Eo object.
 */
EOLIAN static Eo *
_elm_route_efl_object_constructor(Eo *obj, Elm_Route_Data *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);

   return obj;
}

/**
 * @internal
 * @brief Sets the EMap route data for the widget.
 *
 * This function processes an EMap_Route object, creating visual segments
 * (lines) for each connection between nodes in the route. It updates the
 * route's geographic boundaries based on the nodes and triggers a redraw.
 * This function is only effective if ELM_EMAP is defined.
 *
 * @param obj The Eo object (Elm_Route instance).
 * @param sd The private data for the Elm_Route object.
 * @param _emap A pointer to an EMap_Route object. This is cast to EMap_Route*
 *              internally.
 */
EOLIAN static void
_elm_route_emap_set(Eo *obj, Elm_Route_Data *sd, void *_emap)
{
#ifdef ELM_EMAP
   EMap_Route *emap = _emap;

   EMap_Route_Node *node, *node_prev = NULL;
   Evas_Object *o;
   Eina_List *l;

   sd->emap = emap;

   _clear_route(obj);

   EINA_LIST_FOREACH(emap_route_nodes_get(sd->emap), l, node)
     {
        if (node_prev)
          {
             Segment *segment = calloc(1, sizeof(Segment));

             segment->node_start = node_prev;
             segment->node_end = node;

             o = evas_object_line_add(evas_object_evas_get(obj));
             segment->obj = o;
             evas_object_smart_member_add(o, obj);

             segment->must_calc = EINA_TRUE;

             _update_lon_lat_min_max
               (obj, emap_route_node_lon_get(node_prev),
               emap_route_node_lat_get(node_prev));
             _update_lon_lat_min_max
               (obj, emap_route_node_lon_get(node),
               emap_route_node_lat_get(node));

             sd->segments = eina_list_append(sd->segments, segment);
          }

        node_prev = node;
     }

   _sizing_eval(obj);
#else
   (void)obj;
   (void)sd;
   (void)_emap;
#endif
}

/**
 * @internal
 * @brief Gets the minimum and maximum longitude values of the route.
 *
 * These values represent the geographic extent of the route along the
 * longitudinal axis.
 *
 * @param obj The Eo object (Elm_Route instance, unused).
 * @param sd The private data for the Elm_Route object, containing lon_min and lon_max.
 * @param[out] min Pointer to a double where the minimum longitude will be stored.
 *                 Can be NULL if not needed.
 * @param[out] max Pointer to a double where the maximum longitude will be stored.
 *                 Can be NULL if not needed.
 */
EOLIAN static void
_elm_route_longitude_min_max_get(const Eo *obj EINA_UNUSED, Elm_Route_Data *sd, double *min, double *max)
{
   if (min) *min = sd->lon_min;
   if (max) *max = sd->lon_max;
}

/**
 * @internal
 * @brief Gets the minimum and maximum latitude values of the route.
 *
 * These values represent the geographic extent of the route along the
 * latitudinal axis.
 *
 * @param obj The Eo object (Elm_Route instance, unused).
 * @param sd The private data for the Elm_Route object, containing lat_min and lat_max.
 * @param[out] min Pointer to a double where the minimum latitude will be stored.
 *                 Can be NULL if not needed.
 * @param[out] max Pointer to a double where the maximum latitude will be stored.
 *                 Can be NULL if not needed.
 */
EOLIAN static void
_elm_route_latitude_min_max_get(const Eo *obj EINA_UNUSED, Elm_Route_Data *sd, double *min, double *max)
{
   if (min) *min = sd->lat_min;
   if (max) *max = sd->lat_max;
}

/**
 * @internal
 * @brief Class constructor for Elm_Route.
 *
 * This function is called once when the Elm_Route class is being set up.
 * It registers the legacy type name for the class, allowing it to be
 * used with older Evas smart object APIs.
 *
 * @param klass The Efl_Class being constructed.
 */
EOLIAN static void
_elm_route_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Internal EO APIs and hidden overrides */

#define ELM_ROUTE_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_route)

#include "elm_route_eo.c"
