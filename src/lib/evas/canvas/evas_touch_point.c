#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @internal
 * @brief Appends a new touch point to the Evas canvas.
 *
 * This function is called when a new touch point (e.g., a finger press)
 * is detected. It allocates memory for the new touch point, initializes its
 * properties (ID, coordinates, state), and adds it to the list of active
 * touch points associated with the Evas canvas.
 *
 * @param eo_e The Evas canvas object.
 * @param id The unique identifier for the new touch point.
 * @param x The x-coordinate of the touch point.
 * @param y The y-coordinate of the touch point.
 */
void
_evas_touch_point_append(Evas *eo_e, int id, Evas_Coord x, Evas_Coord y)
{
   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);
   Evas_Coord_Touch_Point *point;

   /* create new Evas_Coord_Touch_Point */
   point = (Evas_Coord_Touch_Point *)calloc(1, sizeof(Evas_Coord_Touch_Point));
   point->x = x;
   point->y = y;
   point->id = id;
   point->state = EVAS_TOUCH_POINT_DOWN;
   e->touch_points = eina_list_append(e->touch_points, point);
}

/**
 * @internal
 * @brief Updates the state and coordinates of an existing touch point.
 *
 * This function is called when an existing touch point moves or its state
 * changes (e.g., from down to move). It searches for the touch point
 * by its ID in the list of active touch points and updates its
 * coordinates and state.
 *
 * @param eo_e The Evas canvas object.
 * @param id The unique identifier of the touch point to update.
 * @param x The new x-coordinate of the touch point.
 * @param y The new y-coordinate of the touch point.
 * @param state The new state of the touch point (e.g., EVAS_TOUCH_POINT_MOVE, EVAS_TOUCH_POINT_UP).
 */
void
_evas_touch_point_update(Evas *eo_e, int id, Evas_Coord x, Evas_Coord y, Evas_Touch_Point_State state)
{
   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);
   Eina_List *l;
   Evas_Coord_Touch_Point *point = NULL;

   EINA_LIST_FOREACH(e->touch_points, l, point)
     {
        if (point->id == id)
          {
             point->x = x;
             point->y = y;
             point->state = state;
             break;
          }
     }
}

/**
 * @internal
 * @brief Removes a touch point from the Evas canvas.
 *
 * This function is called when a touch point is released (e.g., a finger lift)
 * or cancelled. It searches for the touch point by its ID, removes it
 * from the list of active touch points, and frees the associated memory.
 *
 * @param eo_e The Evas canvas object.
 * @param id The unique identifier of the touch point to remove.
 */
void
_evas_touch_point_remove(Evas *eo_e, int id)
{
   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);
   Eina_List *l;
   Evas_Coord_Touch_Point *point = NULL;

   EINA_LIST_FOREACH(e->touch_points, l, point)
     {
        if (point->id == id)
          {
             e->touch_points = eina_list_remove(e->touch_points, point);
             free(point);
             break;
          }
     }
}

/**
 * @brief Get the number of currently active touch points on the Evas canvas.
 *
 * This function returns the total count of touch points currently registered
 * (e.g., fingers currently touching the screen).
 *
 * @param eo_e The Evas canvas object.
 * @return The number of active touch points. Returns 0 if an error occurs or
 *         if `eo_e` is NULL.
 * @ingroup Evas_Touch
 */
EVAS_API unsigned int
evas_touch_point_list_count(Eo *eo_e)
{
   EVAS_LEGACY_API(eo_e, e, 0);
   return eina_list_count(e->touch_points);
}

/* For Efl.Ui.Win only */
/**
 * @internal
 * @brief Retrieves the X and Y coordinates of the Nth touch point.
 *
 * This function is intended for internal use by Efl.Ui.Win. It fetches
 * the coordinates of the touch point at the given index `n` in the list
 * of active touch points.
 *
 * @param eo_e The Evas canvas object (unused in this specific implementation).
 * @param e Pointer to the Evas public data.
 * @param n The zero-based index of the touch point in the list.
 * @param[out] x Pointer to store the x-coordinate. If the point is not found,
 *               this is set to 0.
 * @param[out] y Pointer to store the y-coordinate. If the point is not found,
 *               this is set to 0.
 */
EOLIAN void
_evas_canvas_touch_point_list_nth_xy_get(Evas_Canvas *eo_e EINA_UNUSED,
                                         Evas_Public_Data *e, unsigned int n,
                                         double *x, double *y)
{
   Evas_Coord_Touch_Point *point;

   point = eina_list_nth(e->touch_points, n);
   if (!point)
     {
        if (x) *x = 0;
        if (y) *y = 0;
        return;
     }
   if (x) *x = point->x;
   if (y) *y = point->y;
}

/**
 * @brief Get the coordinates of the Nth touch point in the list.
 *
 * This function retrieves the X and Y coordinates of the touch point
 * at the specified index `n` from the list of currently active touch points.
 * The order of touch points in the list is the order in which they were
 * appended.
 *
 * @param eo_e The Evas canvas object.
 * @param n The zero-based index of the touch point.
 * @param[out] x Pointer to store the x-coordinate. If the point is not found or
 *               `eo_e` is NULL, this value is not guaranteed to be meaningful
 *               (often set to 0 by the underlying call).
 * @param[out] y Pointer to store the y-coordinate. If the point is not found or
 *               `eo_e` is NULL, this value is not guaranteed to be meaningful
 *               (often set to 0 by the underlying call).
 * @ingroup Evas_Touch
 */
EVAS_API void
evas_touch_point_list_nth_xy_get(Evas *eo_e, unsigned int n,
                                 Evas_Coord *x, Evas_Coord *y)
{
   double X, Y;

   EVAS_LEGACY_API(eo_e, e);
   _evas_canvas_touch_point_list_nth_xy_get(eo_e, e, n, &X, &Y);
   if (x) *x = X;
   if (y) *y = Y;
}

/**
 * @brief Get the ID of the Nth touch point in the list.
 *
 * This function retrieves the unique identifier of the touch point
 * at the specified index `n` from the list of currently active touch points.
 *
 * @param eo_e The Evas canvas object.
 * @param n The zero-based index of the touch point.
 * @return The ID of the Nth touch point, or -1 if the touch point is not
 *         found, `eo_e` is NULL, or an error occurs.
 * @ingroup Evas_Touch
 */
EVAS_API int
evas_touch_point_list_nth_id_get(Evas *eo_e, unsigned int n)
{
   Evas_Coord_Touch_Point *point;

   EVAS_LEGACY_API(eo_e, e, -1);
   point = eina_list_nth(e->touch_points, n);
   if (!point) return -1;
   else return point->id;
}

/**
 * @brief Get the state of the Nth touch point in the list.
 *
 * This function retrieves the current state (e.g., down, up, move, cancel)
 * of the touch point at the specified index `n` from the list of
 * currently active touch points.
 *
 * @param eo_e The Evas canvas object.
 * @param n The zero-based index of the touch point.
 * @return The state of the Nth touch point (e.g., #EVAS_TOUCH_POINT_DOWN,
 *         #EVAS_TOUCH_POINT_UP, #EVAS_TOUCH_POINT_MOVE, #EVAS_TOUCH_POINT_STILL,
 *         #EVAS_TOUCH_POINT_CANCEL). Returns #EVAS_TOUCH_POINT_CANCEL if the
 *         touch point is not found, `eo_e` is NULL, or an error occurs.
 * @ingroup Evas_Touch
 */
EVAS_API Evas_Touch_Point_State
evas_touch_point_list_nth_state_get(Evas *eo_e, unsigned int n)
{
   Evas_Coord_Touch_Point *point;

   EVAS_LEGACY_API(eo_e, e, EVAS_TOUCH_POINT_CANCEL);
   point = eina_list_nth(e->touch_points, n);
   if (!point) return EVAS_TOUCH_POINT_CANCEL;
   else return point->state;
}

