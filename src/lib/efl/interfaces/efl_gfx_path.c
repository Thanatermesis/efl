#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <math.h>
#include <float.h>
#include <ctype.h>
#include <locale.h>

#include <Efl.h>

typedef struct _Efl_Gfx_Path_Data Efl_Gfx_Path_Data;
struct _Efl_Gfx_Path_Data
{
   struct {
      double x;
      double y;
   } current, current_ctrl;

   Efl_Gfx_Path_Command *commands;
   double *points;

   unsigned int commands_count;
   unsigned int points_count;

   unsigned int reserved_pts_cnt;   //Reserved Points Count
   unsigned int reserved_cmd_cnt;   //Reserved Commands Count

   char *path_data; ///< Stores the SVG path string if the path was created from one and contains arc commands, for interpolation purposes.
   Eina_Bool convex; ///< Flag indicating if the path is known to be convex.
};

static void _path_interpolation(Eo *obj, Efl_Gfx_Path_Data *pd, char *from, char *to, double pos);
static void _efl_gfx_path_reset(Eo *obj, Efl_Gfx_Path_Data *pd);

/**
 * @brief Get the number of coordinate points associated with a given path command.
 *
 * @param command The path command.
 * @return The number of points (doubles) this command requires. For example,
 *         EFL_GFX_PATH_COMMAND_TYPE_MOVE_TO requires 2 points (x, y).
 */
static inline unsigned int
_efl_gfx_path_command_length(Efl_Gfx_Path_Command command)
{
   switch (command)
     {
      case EFL_GFX_PATH_COMMAND_TYPE_END: return 0;
      case EFL_GFX_PATH_COMMAND_TYPE_MOVE_TO: return 2;
      case EFL_GFX_PATH_COMMAND_TYPE_LINE_TO: return 2;
      case EFL_GFX_PATH_COMMAND_TYPE_CUBIC_TO: return 6;
      case EFL_GFX_PATH_COMMAND_TYPE_CLOSE: return 0;
      case EFL_GFX_PATH_COMMAND_TYPE_LAST: return 0;
     }
   return 0;
}

static inline void
_efl_gfx_path_length(const Efl_Gfx_Path_Command *commands,
                     unsigned int *cmd_length,
                     unsigned int *pts_length)
{
   if (commands)
     {
        while (commands[*cmd_length] != EFL_GFX_PATH_COMMAND_TYPE_END)
          {
             *pts_length += _efl_gfx_path_command_length(commands[*cmd_length]);
             (*cmd_length)++;
          }
     }

   /* Accounting for END command and handle gracefully the NULL case
      at the same time */
   /* Accounting for END command and handle gracefully the NULL case
      at the same time */
   (*cmd_length)++;
}

/**
 * @brief Ensures that the path data arrays (commands and points) have enough
 *        space for a new command and its associated points. If not, it reallocates
 *        them, typically doubling the current reserved size.
 *
 * @param command The new command to be added.
 * @param pd The private data of the Efl_Gfx_Path object.
 * @param[out] offset_point A pointer that will be set to the location in the
 *                          pd->points array where the new points for the command
 *                          should be written.
 * @return EINA_TRUE on success, EINA_FALSE on memory allocation failure.
 */
static inline Eina_Bool
efl_gfx_path_grow(Efl_Gfx_Path_Command command,
                  Efl_Gfx_Path_Data *pd,
                  double **offset_point)
{
   unsigned int cmd_length = 0, pts_length = 0;

   cmd_length = pd->commands_count ? pd->commands_count : 1;
   pts_length = pd->points_count;

   if (_efl_gfx_path_command_length(command))
     {
        pts_length += _efl_gfx_path_command_length(command);

        //grow up twice
        if (pts_length > pd->reserved_pts_cnt)
          {
             double *pts_tmp = realloc(pd->points, sizeof(double) * (pts_length * 2));
             if (!pts_tmp) return EINA_FALSE;
             pd->reserved_pts_cnt = pts_length * 2;
             pd->points = pts_tmp;
          }

        *offset_point =
           pd->points + pts_length - _efl_gfx_path_command_length(command);
     }

   //grow up twice
   if ((cmd_length + 1) > pd->reserved_cmd_cnt)
     {
        Efl_Gfx_Path_Command *cmd_tmp =
           realloc(pd->commands, (cmd_length  * 2) * sizeof (Efl_Gfx_Path_Command));
        if (!cmd_tmp) return EINA_FALSE;
        pd->reserved_cmd_cnt = (cmd_length * 2);
        pd->commands = cmd_tmp;
     }

   pd->commands_count = cmd_length + 1;
   pd->points_count = pts_length;

   // Append the command
   pd->commands[cmd_length - 1] = command;

   // NULL terminate the stream
   pd->commands[cmd_length] = EFL_GFX_PATH_COMMAND_TYPE_END;
   pd->convex = EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @brief Iterates through the path commands and points to find the last
 *        "current point" and "current control point".
 *
 * This function is used to update the internal state (pd->current, pd->current_ctrl)
 * after setting or modifying the path.
 *
 * @param cmd Pointer to the array of path commands.
 * @param points Pointer to the array of path points.
 * @param[out] current_x Pointer to store the x-coordinate of the last current point.
 * @param[out] current_y Pointer to store the y-coordinate of the last current point.
 * @param[out] current_ctrl_x Pointer to store the x-coordinate of the last control point (relevant for cubic Bezier).
 * @param[out] current_ctrl_y Pointer to store the y-coordinate of the last control point (relevant for cubic Bezier).
 * @return EINA_TRUE on success, EINA_FALSE if cmd or points is NULL or an invalid command is encountered.
 */
static Eina_Bool
_efl_gfx_path_current_search(const Efl_Gfx_Path_Command *cmd,
                             const double *points,
                             double *current_x, double *current_y,
                             double *current_ctrl_x, double *current_ctrl_y)
{
   unsigned int i;

   if (current_x) *current_x = 0;
   if (current_y) *current_y = 0;
   if (current_ctrl_x) *current_ctrl_x = 0;
   if (current_ctrl_y) *current_ctrl_y = 0;

   if (!cmd || !points) return EINA_FALSE;

   for (i = 0; cmd[i] != EFL_GFX_PATH_COMMAND_TYPE_END; i++)
     {
        switch (cmd[i])
          {
           case EFL_GFX_PATH_COMMAND_TYPE_MOVE_TO:
           case EFL_GFX_PATH_COMMAND_TYPE_LINE_TO:
              if (current_x) *current_x = points[0];
              if (current_y) *current_y = points[1];
              points += 2;
              break;
           case EFL_GFX_PATH_COMMAND_TYPE_CUBIC_TO:
              if (current_x) *current_x = points[0];
              if (current_y) *current_y = points[1];
              if (current_ctrl_x) *current_ctrl_x = points[4];
              if (current_ctrl_y) *current_ctrl_y = points[5];
              points += 6;
              break;
           case EFL_GFX_PATH_COMMAND_TYPE_CLOSE:
              break;
           case EFL_GFX_PATH_COMMAND_TYPE_LAST:
           default:
              return EINA_FALSE;
          }
     }

   return EINA_TRUE;
}

EOLIAN static void
_efl_gfx_path_path_set(Eo *obj, Efl_Gfx_Path_Data *pd,
                       const Efl_Gfx_Path_Command *commands,
                       const double *points)
{
   // Documentation for this EOLIAN method should primarily be in the .eo file.
   // This C implementation sets the path data from the given command and point arrays.
   // - commands: An array of Efl_Gfx_Path_Command enum values, ending with EFL_GFX_PATH_COMMAND_TYPE_END.
   //   Example: {EFL_GFX_PATH_COMMAND_TYPE_MOVE_TO, EFL_GFX_PATH_COMMAND_TYPE_LINE_TO, EFL_GFX_PATH_COMMAND_TYPE_END}
   // - points: A flat array of doubles. The number of points for each command is determined by
   //   _efl_gfx_path_command_length().
   //   Example (for the commands above): {x0, y0, x1, y1}
   if (!commands)
     {
         _efl_gfx_path_reset(obj, pd);
         return;
     }

   Efl_Gfx_Path_Command *cmds;
   double *pts;
   unsigned int cmds_length = 0, pts_length = 0;

   _efl_gfx_path_length(commands, &cmds_length, &pts_length);

   cmds = realloc(pd->commands, sizeof (Efl_Gfx_Path_Command) * cmds_length);
   if (!cmds) return;

   pd->commands = cmds;

   pts = realloc(pd->points, sizeof (double) * pts_length);
   if (!pts) return;

   pd->points = pts;
   pd->commands_count = cmds_length;
   pd->points_count = pts_length;

   //full reserved memory
   pd->reserved_cmd_cnt = cmds_length;
   pd->reserved_pts_cnt = pts_length;

   memcpy(pd->commands, commands, sizeof(Efl_Gfx_Path_Command) * cmds_length);
   memcpy(pd->points, points, sizeof(double) * pts_length);

   _efl_gfx_path_current_search(pd->commands, pd->points,
                                &pd->current.x, &pd->current.y,
                                &pd->current_ctrl.x, &pd->current_ctrl.y);
}

EOLIAN static void
_efl_gfx_path_path_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Path_Data *pd,
                  const Efl_Gfx_Path_Command **commands,
                  const double **points)
{
   // Provides direct (read-only) access to the internal path command and point arrays.
   if (commands) *commands = pd->commands;
   if (points) *points = pd->points;
}

EOLIAN static void
_efl_gfx_path_length_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Path_Data *pd,
                         unsigned int *commands, unsigned int *points)
{
   // Returns the number of commands (including the END command) and the total number of points.
   if (commands) *commands = pd->commands_count;
   if (points) *points = pd->points_count;
}

EOLIAN static void
_efl_gfx_path_bounds_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Path_Data *pd, Eina_Rect *r)
{
   double minx, miny, maxx, maxy;
   unsigned int i;

   EINA_RECTANGLE_SET(r, 0, 0, 0, 0);

   if (pd->points_count <= 0) return;

   minx = pd->points[0];
   miny = pd->points[1];
   maxx = pd->points[0];
   maxy = pd->points[1];

   for (i = 2; i < pd->points_count; i += 2)
     {
        minx = minx < pd->points[i] ? minx : pd->points[i];
        miny = miny < pd->points[i + 1] ? miny : pd->points[i + 1];
        maxx = maxx > pd->points[i] ? maxx : pd->points[i];
        maxy = maxy > pd->points[i + 1] ? maxy : pd->points[i + 1];
     }

   // Calculates the bounding box of the path by iterating through all points.
   EINA_RECTANGLE_SET(r, floor(minx), floor(miny), (ceil(maxx) - floor(minx)), (ceil(maxy) - floor(miny)));
}

EOLIAN static void
_efl_gfx_path_current_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Path_Data *pd,
                           double *x, double *y)
{
   // Gets the last explicitly set point (e.g., by MOVE_TO, LINE_TO, or the end point of a CUBIC_TO).
   if (x) *x = pd->current.x;
   if (y) *y = pd->current.y;
}

EOLIAN static void
_efl_gfx_path_current_ctrl_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Path_Data *pd,
                                double *x, double *y)
{
   // Gets the last control point, typically the second control point of a CUBIC_TO command.
   if (x) *x = pd->current_ctrl.x;
   if (y) *y = pd->current_ctrl.y;
}

/**
 * @brief Compares the command arrays of two path data structures.
 *
 * @param a The first path data.
 * @param b The second path data.
 * @return EINA_TRUE if the command sequences are identical, EINA_FALSE otherwise.
 *         Does not compare the point coordinates.
 */
EOLIAN static Eina_Bool
_efl_gfx_path_equal_commands_internal(Efl_Gfx_Path_Data *a,
                                       Efl_Gfx_Path_Data *b)
{
   unsigned int i;

   if (a->commands_count != b->commands_count) return EINA_FALSE;
   if (a->commands_count <= 0) return EINA_TRUE;

   for (i = 0; a->commands[i] == b->commands[i] &&
          a->commands[i] != EFL_GFX_PATH_COMMAND_TYPE_END; i++)
     {
        ;
     }

   return (a->commands[i] == b->commands[i]);
}

/**
 * @brief Linearly interpolates between two double values.
 *
 * @param from The starting value.
 * @param to The ending value.
 * @param pos_map The interpolation factor (0.0 to 1.0).
 * @return The interpolated value.
 */
static inline double
interpolate(double from, double to, double pos_map)
{
   return (from * (1.0 - pos_map)) + (to * pos_map);
}

EOLIAN static Eina_Bool
_efl_gfx_path_interpolate(Eo *obj, Efl_Gfx_Path_Data *pd,
                          const Eo *from, const Eo *to, double pos_map)
{
   Efl_Gfx_Path_Data *from_pd, *to_pd;
   Efl_Gfx_Path_Command *cmds;
   double interv;    // interpolated value
   double *pts;

   // This function interpolates the path data of the current object (pd)
   // based on two other path objects, 'from' and 'to', and an interpolation
   // factor 'pos_map'.
   // If both 'from' and 'to' paths were created from SVG path strings
   // (and contain arc commands, hence path_data is set), it uses
   // _path_interpolation to interpolate the SVG strings themselves.
   // Otherwise, it requires 'from' and 'to' to have identical command sequences
   // and interpolates their corresponding points directly.

   if (!efl_isa(from, EFL_GFX_PATH_MIXIN) || !efl_isa(to, EFL_GFX_PATH_MIXIN))
     return EINA_FALSE;

   from_pd = efl_data_scope_get(from, EFL_GFX_PATH_MIXIN);
   to_pd = efl_data_scope_get(to, EFL_GFX_PATH_MIXIN);

   // Avoid interpolating an object with itself.
   if (pd == from_pd || pd == to_pd) return EINA_FALSE;

   if (from_pd->path_data && to_pd->path_data)
     {
        // If both source paths have SVG string data (likely due to arc commands),
        // interpolate using the SVG string representation.
        _efl_gfx_path_reset(obj, pd);
        _path_interpolation(obj, pd,
                            from_pd->path_data, to_pd->path_data, pos_map);
     }
   else
     {
        // Otherwise, interpolate point by point. This requires command lists to be identical.
        if (!_efl_gfx_path_equal_commands_internal(from_pd, to_pd))
          return EINA_FALSE; // Command structures must match for point-wise interpolation.

        cmds = realloc(pd->commands,
                       sizeof(Efl_Gfx_Path_Command) * from_pd->commands_count);
        if (!cmds && (from_pd->commands_count > 0)) return EINA_FALSE;

        pd->commands = cmds;

        pts = realloc(pd->points,
                      sizeof(double) * from_pd->points_count);
        if (!pts && (from_pd->points_count > 0)) return EINA_FALSE;

        pd->points = pts;

        if (cmds)
          {
             memcpy(cmds, from_pd->commands,
                    sizeof (Efl_Gfx_Path_Command) * from_pd->commands_count);

             if (pts)
               {
                  double *to_pts = to_pd->points;
                  double *from_pts = from_pd->points;
                  unsigned int i, j;

                  for (i = 0; cmds[i] != EFL_GFX_PATH_COMMAND_TYPE_END; i++)
                    for (j = 0; j < _efl_gfx_path_command_length(cmds[i]); j++)
                      {
                         *pts = interpolate(*from_pts, *to_pts, pos_map);
                         pts++;
                         from_pts++;
                         to_pts++;
                      }
               }
          }

        pd->points_count = from_pd->points_count;
        pd->commands_count = from_pd->commands_count;
        pd->reserved_cmd_cnt = from_pd->commands_count;
        pd->reserved_pts_cnt = from_pd->points_count;

        interv = interpolate(from_pd->current.x, to_pd->current.x, pos_map);
        pd->current.x = interv;

        interv = interpolate(from_pd->current.y, to_pd->current.y, pos_map);
        pd->current.y = interv;

        interv = interpolate(from_pd->current_ctrl.x, to_pd->current_ctrl.x,
                             pos_map);
        pd->current_ctrl.x = interv;

        interv = interpolate(from_pd->current_ctrl.y, to_pd->current_ctrl.y,
                             pos_map);
        pd->current_ctrl.y = interv;

   }

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_gfx_path_equal_commands(Eo *obj EINA_UNUSED,
                              Efl_Gfx_Path_Data *pd,
                              const Eo *with)
{
   Efl_Gfx_Path_Data *with_pd;

   with_pd = efl_data_scope_get(with, EFL_GFX_PATH_MIXIN);
   if (!with_pd) return EINA_FALSE;

   // Compares the command sequence of the current path (pd) with another path object (with).
   return _efl_gfx_path_equal_commands_internal(with_pd, pd);
}

EOLIAN static void
_efl_gfx_path_reserve(Eo *obj EINA_UNUSED, Efl_Gfx_Path_Data *pd,
                      unsigned int cmd_count, unsigned int pts_count)
{
   // Pre-allocates memory for the command and point arrays to avoid multiple
   // reallocations when appending a known number of commands/points.
   if (pd->reserved_cmd_cnt < cmd_count)
      {
         // +1 for a potential implicit EFL_GFX_PATH_COMMAND_TYPE_END command.
         pd->reserved_cmd_cnt = cmd_count + 1;
         pd->commands = realloc(pd->commands, sizeof(Efl_Gfx_Path_Command) * pd->reserved_cmd_cnt);
      }

   if (pd->reserved_pts_cnt < pts_count)
      {
         pd->reserved_pts_cnt = pts_count;
         pd->points = realloc(pd->points, sizeof(double) * pts_count);
      }
}

EOLIAN static void
_efl_gfx_path_reset(Eo *obj EINA_UNUSED, Efl_Gfx_Path_Data *pd)
{
   free(pd->commands);
   pd->reserved_cmd_cnt = 0;
   pd->commands = NULL;
   pd->commands_count = 0;

   free(pd->points);
   pd->reserved_pts_cnt = 0;
   pd->points = NULL;
   pd->points_count = 0;

   free(pd->path_data);
   pd->path_data = NULL;

   pd->current.x = 0;
   pd->current.y = 0;
   pd->current_ctrl.x = 0;
   pd->current_ctrl.y = 0;
   pd->convex = EINA_FALSE; // Reset convex hint as path is now empty or changed.
}

EOLIAN static void
_efl_gfx_path_append_move_to(Eo *obj EINA_UNUSED,
                             Efl_Gfx_Path_Data *pd,
                             double x, double y)
{
   double *offset_point;

   if (!efl_gfx_path_grow(EFL_GFX_PATH_COMMAND_TYPE_MOVE_TO, pd, &offset_point))
     return;

   offset_point[0] = x;
   offset_point[1] = y;

   pd->current.x = x;
   // Appends a "move to" command, starting a new sub-path.
   // Updates the current point to (x, y).
   pd->current.y = y;
}

EOLIAN static void
_efl_gfx_path_append_line_to(Eo *obj EINA_UNUSED,
                             Efl_Gfx_Path_Data *pd,
                             double x, double y)
{
   double *offset_point;

   if (!efl_gfx_path_grow(EFL_GFX_PATH_COMMAND_TYPE_LINE_TO, pd, &offset_point))
     return;

   offset_point[0] = x;
   offset_point[1] = y;

   pd->current.x = x;
   // Appends a "line to" command, drawing a line from the current point to (x, y).
   // Updates the current point to (x, y).
   pd->current.y = y;
}

EOLIAN static void
_efl_gfx_path_append_cubic_to(Eo *obj EINA_UNUSED,
                              Efl_Gfx_Path_Data *pd,
                              double ctrl_x0, double ctrl_y0, // First control point
                              double ctrl_x1, double ctrl_y1, // Second control point
                              double x, double y)              // End point
{
   double *offset_point;

   if (!efl_gfx_path_grow(EFL_GFX_PATH_COMMAND_TYPE_CUBIC_TO,
                          pd, &offset_point))
     return;

   offset_point[0] = ctrl_x0;
   offset_point[1] = ctrl_y0;
   offset_point[2] = ctrl_x1;
   offset_point[3] = ctrl_y1;
   offset_point[4] = x;
   offset_point[5] = y;

   pd->current.x = x;
   pd->current.y = y;
   pd->current_ctrl.x = ctrl_x1;
   // Appends a cubic Bezier curve.
   // Updates the current point to (x, y) and current control point to (ctrl_x1, ctrl_y1).
   pd->current_ctrl.y = ctrl_y1;
}

EOLIAN static void
_efl_gfx_path_append_scubic_to(Eo *obj, Efl_Gfx_Path_Data *pd,
                                double x, double y,              // End point
                                double ctrl_x, double ctrl_y)    // Second control point
{
   double ctrl_x0, ctrl_y0;
   double current_x = 0, current_y = 0;
   double current_ctrl_x = 0, current_ctrl_y = 0;

   current_x = pd->current.x;
   current_y = pd->current.y;
   current_ctrl_x = pd->current_ctrl.x;
   current_ctrl_y = pd->current_ctrl.y;

   /* if previous command is cubic then use reflection point of current control
      point as the first control point */
   if ((pd->commands_count > 1) && (pd->commands[pd->commands_count-2] ==
        EFL_GFX_PATH_COMMAND_TYPE_CUBIC_TO))
     {
        ctrl_x0 = 2 * current_x - current_ctrl_x;
        ctrl_y0 = 2 * current_y - current_ctrl_y;
     }
   else
     {
        // use currnt point as the 1st control point
        ctrl_x0 = current_x;
        ctrl_y0 = current_y;
     }

   _efl_gfx_path_append_cubic_to(obj, pd, ctrl_x0, ctrl_y0, ctrl_x, ctrl_y,
   // Appends a smooth cubic Bezier curve.
   // The first control point is a reflection of the previous curve's second control point
   // relative to the current point. If the previous command was not a cubic Bezier,
   // the current point is used as the first control point.
   // (ctrl_x, ctrl_y) is the second control point.
   // (x, y) is the end point of the curve.
                                 x, y);
}

EOLIAN static void
_efl_gfx_path_append_quadratic_to(Eo *obj, Efl_Gfx_Path_Data *pd,
                                   double x, double y,           // End point
                                   double ctrl_x, double ctrl_y) // Control point
{
   double current_x = 0, current_y = 0;
   double ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1;

   current_x = pd->current.x;
   current_y = pd->current.y;

   // Convert quadratic bezier to cubic
   ctrl_x0 = (current_x + 2 * ctrl_x) * (1.0 / 3.0);
   ctrl_y0 = (current_y + 2 * ctrl_y) * (1.0 / 3.0);
   ctrl_x1 = (x + 2 * ctrl_x) * (1.0 / 3.0);
   ctrl_y1 = (y + 2 * ctrl_y) * (1.0 / 3.0);

   _efl_gfx_path_append_cubic_to(obj, pd, ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1,
   // Appends a quadratic Bezier curve by converting it to an equivalent cubic Bezier curve.
   // (ctrl_x, ctrl_y) is the quadratic control point.
   // (x, y) is the end point of the curve.
                                 x, y);
}

EOLIAN static void
_efl_gfx_path_append_squadratic_to(Eo *obj, Efl_Gfx_Path_Data *pd,
                                    double x, double y) // End point
{
   double xc, yc; /* quadratic control point */
   double ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1;
   double current_x = 0, current_y = 0;
   double current_ctrl_x = 0, current_ctrl_y = 0;

   current_x = pd->current.x;
   current_y = pd->current.y;
   current_ctrl_x = pd->current_ctrl.x;
   current_ctrl_y = pd->current_ctrl.y;

   xc = 2 * current_x - current_ctrl_x;
   yc = 2 * current_y - current_ctrl_y;

   /* generate a quadratic bezier with control point = xc, yc */
   ctrl_x0 = (current_x + 2 * xc) * (1.0 / 3.0);
   ctrl_y0 = (current_y + 2 * yc) * (1.0 / 3.0);
   ctrl_x1 = (x + 2 * xc) * (1.0 / 3.0);
   ctrl_y1 = (y + 2 * yc) * (1.0 / 3.0);

   _efl_gfx_path_append_cubic_to(obj, pd, ctrl_x0, ctrl_y0,
                                  ctrl_x1, ctrl_y1,
   // Appends a smooth quadratic Bezier curve.
   // The single control point for the quadratic curve is calculated as a reflection
   // of the previous curve's control point (implicitly, as it's converted to cubic).
   // (x,y) is the end point of the curve.
   // This is then converted to an equivalent cubic Bezier curve.
                                   x, y);
}

/*
 * code adapted from enesim which was adapted from moonlight sources
 */
/**
 * @brief Appends an elliptical arc, approximating it with one or more cubic Bezier curves.
 * This function implements the arc command as defined in the SVG specification (A/a commands).
 *
 * @param obj The Efl_Gfx_Path object.
 * @param pd The private data of the Efl_Gfx_Path object.
 * @param x The x-coordinate of the end point of the arc.
 * @param y The y-coordinate of the end point of the arc.
 * @param rx The x-radius of the ellipse.
 * @param ry The y-radius of the ellipse.
 * @param angle The rotation angle of the ellipse's x-axis relative to the coordinate system's x-axis, in degrees.
 * @param large_arc If EINA_TRUE, the larger of the two possible arcs is chosen. If EINA_FALSE, the smaller arc is chosen.
 * @param sweep If EINA_TRUE, the arc is drawn in a "positive-angle" direction (clockwise). If EINA_FALSE, in a "negative-angle" direction (counter-clockwise).
 */
EOLIAN static void
_efl_gfx_path_append_arc_to(Eo *obj, Efl_Gfx_Path_Data *pd,
                             double x, double y,
                             double rx, double ry,
                             double angle,
                             Eina_Bool large_arc, Eina_Bool sweep)
{
   double cxp, cyp, cx, cy;
   double sx, sy;
   double cos_phi, sin_phi;
   double dx2, dy2;
   double x1p, y1p;
   double x1p2, y1p2;
   double rx2, ry2;
   double lambda;
   double c;
   double at;
   double theta1, delta_theta;
   double nat;
   double delta, bcp;
   double cos_phi_rx, cos_phi_ry;
   double sin_phi_rx, sin_phi_ry;
   double cos_theta1, sin_theta1;
   int segments, i;

   // some helpful stuff is available here:
   // http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes
   sx = pd->current.x;
   sy = pd->current.y;

   // if start and end points are identical, then no arc is drawn
   if ((fabs(x - sx) < (1 / 256.0)) && (fabs(y - sy) < (1 / 256.0)))
     return;

   // Correction of out-of-range radii, see F6.6.1 (step 2)
   rx = fabs(rx);
   ry = fabs(ry);

   angle = angle * M_PI / 180.0;
   cos_phi = cos(angle);
   sin_phi = sin(angle);
   dx2 = (sx - x) / 2.0;
   dy2 = (sy - y) / 2.0;
   x1p = cos_phi * dx2 + sin_phi * dy2;
   y1p = cos_phi * dy2 - sin_phi * dx2;
   x1p2 = x1p * x1p;
   y1p2 = y1p * y1p;
   rx2 = rx * rx;
   ry2 = ry * ry;
   lambda = (x1p2 / rx2) + (y1p2 / ry2);

   // Correction of out-of-range radii, see F6.6.2 (step 4)
   if (lambda > 1.0)
     {
        // see F6.6.3
        double lambda_root = sqrt(lambda);

        rx *= lambda_root;
        ry *= lambda_root;
        // update rx2 and ry2
        rx2 = rx * rx;
        ry2 = ry * ry;
     }

   c = (rx2 * ry2) - (rx2 * y1p2) - (ry2 * x1p2);

   // check if there is no possible solution
   // (i.e. we can't do a square root of a negative value)
   if (c < 0.0)
     {
        // scale uniformly until we have a single solution
        // (see F6.2) i.e. when c == 0.0
        double scale = sqrt(1.0 - c / (rx2 * ry2));
        rx *= scale;
        ry *= scale;
        // update rx2 and ry2
        rx2 = rx * rx;
        ry2 = ry * ry;

        // step 2 (F6.5.2) - simplified since c == 0.0
        cxp = 0.0;
        cyp = 0.0;
        // step 3 (F6.5.3 first part) - simplified since cxp and cyp == 0.0
        cx = 0.0;
        cy = 0.0;
     }
   else
     {
        // complete c calculation
        c = sqrt(c / ((rx2 * y1p2) + (ry2 * x1p2)));
        // inverse sign if Fa == Fs
        if (large_arc == sweep)
          c = -c;

        // step 2 (F6.5.2)
        cxp = c * ( rx * y1p / ry);
        cyp = c * (-ry * x1p / rx);

        // step 3 (F6.5.3 first part)
        cx = cos_phi * cxp - sin_phi * cyp;
        cy = sin_phi * cxp + cos_phi * cyp;
     }

   // step 3 (F6.5.3 second part) we now have the center point of the ellipse
   cx += (sx + x) / 2.0;
   cy += (sy + y) / 2.0;

   // step 4 (F6.5.4)
   // we don't use arccos (as per w3c doc), see
   // http://www.euclideanspace.com/maths/algebra/vectors/angleBetween/index.htm
   // note: atan2 (0.0, 1.0) == 0.0
   at = atan2(((y1p - cyp) / ry), ((x1p - cxp) / rx));
   theta1 = (at < 0.0) ? 2.0 * M_PI + at : at;

   nat = atan2(((-y1p - cyp) / ry), ((-x1p - cxp) / rx));
   delta_theta = (nat < at) ? 2.0 * M_PI - at + nat : nat - at;

   if (sweep)
     {
        // ensure delta theta < 0 or else add 360 degrees
        if (delta_theta < 0.0)
          delta_theta += 2.0 * M_PI;
     }
   else
     {
        // ensure delta theta > 0 or else substract 360 degrees
        if (delta_theta > 0.0)
          delta_theta -= 2.0 * M_PI;
     }

   // add several cubic bezier to approximate the arc
   // (smaller than 90 degrees)
   // we add one extra segment because we want something
   // smaller than 90deg (i.e. not 90 itself)
   segments = (int) (fabs(delta_theta / M_PI_2)) + 1;
   delta = delta_theta / segments;

   // http://www.stillhq.com/ctpfaq/2001/comp.text.pdf-faq-2001-04.txt (section 2.13)
   bcp = 4.0 / 3 * (1 - cos(delta / 2)) / sin(delta / 2);

   cos_phi_rx = cos_phi * rx;
   cos_phi_ry = cos_phi * ry;
   sin_phi_rx = sin_phi * rx;
   sin_phi_ry = sin_phi * ry;

   cos_theta1 = cos(theta1);
   sin_theta1 = sin(theta1);

   for (i = 0; i < segments; ++i)
     {
        // end angle (for this segment) = current + delta
        double c1x, c1y, ex, ey, c2x, c2y;
        double theta2 = theta1 + delta;
        double cos_theta2 = cos(theta2);
        double sin_theta2 = sin(theta2);

        // first control point (based on start point sx,sy)
        c1x = sx - bcp * (cos_phi_rx * sin_theta1 + sin_phi_ry * cos_theta1);
        c1y = sy + bcp * (cos_phi_ry * cos_theta1 - sin_phi_rx * sin_theta1);

        // end point (for this segment)
        ex = cx + (cos_phi_rx * cos_theta2 - sin_phi_ry * sin_theta2);
        ey = cy + (sin_phi_rx * cos_theta2 + cos_phi_ry * sin_theta2);

        // second control point (based on end point ex,ey)
        c2x = ex + bcp * (cos_phi_rx * sin_theta2 + sin_phi_ry * cos_theta2);
        c2y = ey + bcp * (sin_phi_rx * sin_theta2 - cos_phi_ry * cos_theta2);

        _efl_gfx_path_append_cubic_to(obj, pd, c1x, c1y, c2x, c2y, ex, ey);

        // next start point is the current end point (same for angle)
        sx = ex;
        sy = ey;
        theta1 = theta2;
        // avoid recomputations
        cos_theta1 = cos_theta2;
        sin_theta1 = sin_theta2;
     }
}

// append arc implementation
typedef struct _Point
{
   double x;
   double y;
} Point;

inline static void
_bezier_coefficients(double t, double *ap, double *bp, double *cp, double *dp)
{
   double a,b,c,d;
   double m_t = 1.0 - t; // (1-t)

   // Calculates coefficients for a cubic Bezier curve:
   // a = (1-t)^3
   // b = 3 * t * (1-t)^2
   // c = 3 * t^2 * (1-t)
   // d = t^3
   // These are used in B(t) = P0*a + P1*b + P2*c + P3*d

   b = m_t * m_t; // (1-t)^2
   c = t * t;     // t^2
   d = c * t;
   a = b * m_t;
   b *= 3.0 * t;
   c *= 3.0 * m_t;
   *ap = a;
   *bp = b;
   *cp = c;
   *dp = d;
}

#define PATH_KAPPA 0.5522847498 /**< Kappa value for approximating a circular arc with a cubic Bezier curve. (4/3)*tan(pi/8) */

/**
 * @brief Calculates the parameter 't' for a Bezier curve segment that
 *        approximates a circular arc of a given angle (up to 90 degrees).
 *
 * This function is used to determine how to segment a Bezier curve
 * when approximating an elliptical arc, particularly for splitting
 * arcs at quadrant boundaries.
 *
 * @param angle The angle of the arc segment in degrees (expected to be <= 90).
 * @return The Bezier parameter 't' (0 to 1) corresponding to the end of the arc segment.
 */
static double
_efl_gfx_t_for_arc_angle(double angle)
{
   double radians, cos_angle, sin_angle, tc, ts, t;

   if (angle < 0.00001) return 0; // Very small angle, t is effectively 0
   if (EINA_FLT_EQ(angle, 90.0)) return 1;

   radians = (angle/180) * M_PI;

   cos_angle = cos(radians);
   sin_angle = sin(radians);

   // initial guess
   tc = angle / 90;

   // do some iterations of newton's method to approximate cos_angle
   // finds the zero of the function b.pointAt(tc).x() - cos_angle
   tc -= ((((2-3*PATH_KAPPA) * tc + 3*(PATH_KAPPA-1)) * tc) * tc + 1 - cos_angle) // value
   / (((6-9*PATH_KAPPA) * tc + 6*(PATH_KAPPA-1)) * tc); // derivative
   tc -= ((((2-3*PATH_KAPPA) * tc + 3*(PATH_KAPPA-1)) * tc) * tc + 1 - cos_angle) // value
   / (((6-9*PATH_KAPPA) * tc + 6*(PATH_KAPPA-1)) * tc); // derivative

   // initial guess
   ts = tc;
   // do some iterations of newton's method to approximate sin_angle
   // finds the zero of the function b.pointAt(tc).y() - sin_angle
   ts -= ((((3*PATH_KAPPA-2) * ts -  6*PATH_KAPPA + 3) * ts + 3*PATH_KAPPA) * ts - sin_angle)
   / (((9*PATH_KAPPA-6) * ts + 12*PATH_KAPPA - 6) * ts + 3*PATH_KAPPA);
   ts -= ((((3*PATH_KAPPA-2) * ts -  6*PATH_KAPPA + 3) * ts + 3*PATH_KAPPA) * ts - sin_angle)
   / (((9*PATH_KAPPA-6) * ts + 12*PATH_KAPPA - 6) * ts + 3*PATH_KAPPA);

   // use the average of the t that best approximates cos_angle
   // and the t that best approximates sin_angle
   t = 0.5 * (tc + ts);
   return t;
}

/**
 * @brief Calculates the Cartesian coordinates of points on an ellipse defined by a
 *        bounding box (x, y, w, h) at specified angles.
 *
 * This is a helper for arc calculations, determining the start and end
 * points of an arc segment on the ellipse.
 *
 * @param x The x-coordinate of the ellipse's bounding box.
 * @param y The y-coordinate of the ellipse's bounding box.
 * @param w The width of the ellipse's bounding box.
 * @param h The height of the ellipse's bounding box.
 * @param angle The starting angle in degrees.
 * @param length The sweep length of the arc in degrees.
 * @param[out] start_point If not NULL, populated with the coordinates of the point at 'angle'.
 * @param[out] end_point If not NULL, populated with the coordinates of the point at 'angle + length'.
 */
static void
_find_ellipse_coords(double x, double y, double w, double h, double angle,
                     double length, Point* start_point, Point *end_point)
{
   int i, quadrant;
   double theta, t, a, b, c, d, px, py, cx, cy; // Intermediate calculation variables
   double w2 = w / 2;
   double h2 = h / 2;
   double angles[2] = { angle, angle + length };
   Point *points[2];

   if (EINA_FLT_EQ(w, 0.0) || EINA_FLT_EQ(h, 0.0))
     {
        if (start_point)
          {
            start_point->x = 0;
            start_point->y = 0;
          }
        if (end_point)
          {
             end_point->x = 0;
             end_point->y = 0;
          }
        return;
     }

   points[0] = start_point;
   points[1] = end_point;

   for (i = 0; i < 2; ++i)
     {
        if (!points[i])
          continue;

        theta = angles[i] - 360 * floor(angles[i] / 360);
        t = theta / 90;
        // truncate
        quadrant = (int)t;
        t -= quadrant;

        t = _efl_gfx_t_for_arc_angle(90 * t);

        // swap x and y?
        if (quadrant & 1)
          t = 1 - t;

        _bezier_coefficients(t, &a, &b, &c, &d);
        px = a + b + c*PATH_KAPPA;
        py = d + c + b*PATH_KAPPA;

        // left quadrants
        if (quadrant == 1 || quadrant == 2)
          px = -px;

        // top quadrants
        if (quadrant == 0 || quadrant == 1)
          py = -py;
        cx = x+w/2;
        cy = y+h/2;
        points[i]->x = cx + w2 * px;
        points[i]->y = cy + h2 * py;
     }
}

/**
 * @brief Generates a sequence of cubic Bezier curve control points to approximate
 *        an elliptical arc.
 *
 * This function breaks down the arc into segments (at most 90 degrees each)
 * and calculates the control points for cubic Bezier curves that approximate
 * these segments. The kappa value (PATH_KAPPA) is used for this approximation.
 *
 * @param x The x-coordinate of the ellipse's bounding box.
 * @param y The y-coordinate of the ellipse's bounding box.
 * @param w The width of the ellipse's bounding box.
 * @param h The height of the ellipse's bounding box.
 * @param start_angle The starting angle of the arc in degrees.
 * @param sweep_length The angular extent of the arc in degrees (can be negative).
 * @param[out] curves An array to store the generated Bezier control points.
 *                    It should be large enough (e.g., 15 Points for a full circle).
 *                    Each set of 3 points defines a Bezier segment:
 *                    (ctrl_start.x, ctrl_start.y), (ctrl_end.x, ctrl_end.y), (end.x, end.y).
 * @param[out] point_count The number of points written to the `curves` array.
 * @return The starting point (first point on the arc) of the generated curve sequence.
 */
static Point
_curves_for_arc(double x, double y, double w, double h,
                double start_angle, double sweep_length,
                Point *curves, int *point_count)
{
   int start_segment, end_segment, delta, i, j, end, quadrant; // Loop and segment calculation variables
   double start_t, end_t; // Bezier t-parameters for start/end of partial segments
   Eina_Bool split_at_start, split_at_end; // Flags if arc starts/ends mid-segment
   Eina_Bezier b, res; // Bezier structures for calculation
   Point start_point, end_point; // Calculated start/end points of the entire arc
   double w2 = w / 2;
   double w2k = w2 * PATH_KAPPA;
   double h2 = h / 2;
   double h2k = h2 * PATH_KAPPA;

   Point points[16] =
     {
        // start point
          { x + w, y + h2 },

          // 0 -> 270 degrees
          { x + w, y + h2 + h2k },
          { x + w2 + w2k, y + h },
          { x + w2, y + h },

          // 270 -> 180 degrees
          { x + w2 - w2k, y + h },
          { x, y + h2 + h2k },
          { x, y + h2 },

          // 180 -> 90 degrees
          { x, y + h2 - h2k },
          { x + w2 - w2k, y },
          { x + w2, y },

          // 90 -> 0 degrees
          { x + w2 + w2k, y },
          { x + w, y + h2 - h2k },
          { x + w, y + h2 }
     };
   // points array defines the control points for four 90-degree Bezier curves
   // that form a unit circle/ellipse, starting from (x+w, y+h2) (0 degrees on a standard cartesian plane if centered at origin).
   // Each segment: P_end, CP1_prev_segment_reflected, CP2_curr_segment, P_start_curr_segment
   // Example: points[0] is the end point of the 90->0 degree segment.
   // points[1], points[2], points[3] define the 0->270 degree segment (clockwise).
   // (points[3] is end point, points[2] is ctrl_end, points[1] is ctrl_start)

   *point_count = 0;

   // Clamp sweep_length to +/- 360 degrees.
   if (sweep_length > 360) sweep_length = 360;
   else if (sweep_length < -360) sweep_length = -360;

   // Special case fast paths for full circles.
   if (EINA_FLT_EQ(start_angle, 0))
     {
        if (EINA_FLT_EQ(sweep_length, 360))
          {
             for (i = 11; i >= 0; --i)
               curves[(*point_count)++] = points[i];
             return points[12];
          }
        else if (EINA_FLT_EQ(sweep_length, -360))
          {
             for (i = 1; i <= 12; ++i)
               curves[(*point_count)++] = points[i];
             return points[0];
          }
     }

   start_segment = (int)(floor(start_angle / 90));
   end_segment = (int)(floor((start_angle + sweep_length) / 90));

   start_t = (start_angle - start_segment * 90) / 90;
   end_t = (start_angle + sweep_length - end_segment * 90) / 90;

   delta = sweep_length > 0 ? 1 : -1;
   if (delta < 0)
     {
        start_t = 1 - start_t;
        end_t = 1 - end_t;
     }

   // avoid empty start segment
   if (EINA_FLT_EQ(start_t, 1.0))
     {
        start_t = 0;
        start_segment += delta;
     }

   // avoid empty end segment
   if (EINA_FLT_EQ(end_t, 0.0))
     {
        end_t = 1;
        end_segment -= delta;
     }

   start_t = _efl_gfx_t_for_arc_angle(start_t * 90);
   end_t = _efl_gfx_t_for_arc_angle(end_t * 90);

   split_at_start = !(fabs(start_t) <= 0.00001f);
   split_at_end = !(fabs(end_t - 1.0) <= 0.00001f);

   end = end_segment + delta;

   // empty arc?
   if (start_segment == end)
     {
        quadrant = 3 - ((start_segment % 4) + 4) % 4;
        j = 3 * quadrant;
        return delta > 0 ? points[j + 3] : points[j];
     }

   _find_ellipse_coords(x, y, w, h, start_angle, sweep_length,
                        &start_point, &end_point);

   for (i = start_segment; i != end; i += delta)
     {
        quadrant = 3 - ((i % 4) + 4) % 4;
        j = 3 * quadrant;

        if (delta > 0)
          eina_bezier_values_set(&b, points[j + 3].x, points[j + 3].y,
                                 points[j + 2].x, points[j + 2].y,
                                 points[j + 1].x, points[j + 1].y,
                                 points[j].x, points[j].y);
        else
          eina_bezier_values_set(&b, points[j].x, points[j].y,
                                 points[j + 1].x, points[j + 1].y,
                                 points[j + 2].x, points[j + 2].y,
                                 points[j + 3].x, points[j + 3].y);

        // empty arc?
        if (start_segment == end_segment && (EINA_FLT_EQ(start_t, end_t)))
            return start_point;

        res = b;
        if (i == start_segment)
          {
             if (i == end_segment && split_at_end)
               eina_bezier_on_interval(&b, start_t, end_t, &res);
             else if (split_at_start)
               eina_bezier_on_interval(&b, start_t, 1, &res);
          }
        else if (i == end_segment && split_at_end)
          {
             eina_bezier_on_interval(&b, 0, end_t, &res);
          }

        // push control points
        curves[(*point_count)].x = res.ctrl_start.x;
        curves[(*point_count)++].y = res.ctrl_start.y;
        curves[(*point_count)].x = res.ctrl_end.x;
        curves[(*point_count)++].y = res.ctrl_end.y;
        curves[(*point_count)].x = res.end.x;
        curves[(*point_count)++].y = res.end.y;
     }

   curves[*(point_count)-1] = end_point;

   return start_point;
}

/**
 * @brief Appends an elliptical arc defined by its bounding box, start angle, and sweep length.
 *
 * This function uses _curves_for_arc to generate Bezier segments and then appends them.
 * It will first add a LINE_TO or MOVE_TO to the start of the arc if necessary.
 *
 * @param obj The Efl_Gfx_Path object.
 * @param pd The private data of the Efl_Gfx_Path object.
 * @param x The x-coordinate of the arc's bounding ellipse.
 * @param y The y-coordinate of the arc's bounding ellipse.
 * @param w The width of the arc's bounding ellipse.
 * @param h The height of the arc's bounding ellipse.
 * @param start_angle The starting angle of the arc in degrees.
 * @param sweep_length The angular extent of the arc in degrees.
 */
EOLIAN static void
_efl_gfx_path_append_arc(Eo *obj, Efl_Gfx_Path_Data *pd,
                          double x, double y, double w, double h,
                          double start_angle, double sweep_length)
{
   int i, point_count;
   Point pts[15]; // Sufficient for up to 4 Bezier segments (3 points each) + 3 for safety/growth.
                  // A full circle (360 deg) is approximated by 4 cubic Bezier curves.
                  // Each curve needs 3 points (ctrl1, ctrl2, end_point). So 4*3=12 points.
   Point curve_start =
      _curves_for_arc(x, y, w, h, start_angle, sweep_length, pts, &point_count);

   // If there's an existing path and it's not closed, draw a line to the start of the arc.
   // Otherwise, move to the start of the arc.
   if (pd->commands_count &&
       (pd->commands_count > 1 && pd->commands[pd->commands_count-2] != EFL_GFX_PATH_COMMAND_TYPE_CLOSE))
     _efl_gfx_path_append_line_to(obj, pd, curve_start.x, curve_start.y);
   else
     _efl_gfx_path_append_move_to(obj, pd, curve_start.x, curve_start.y);

   // Append the cubic Bezier segments that approximate the arc.
   for (i = 0; i < point_count; i += 3)
     {
        _efl_gfx_path_append_cubic_to(obj, pd, pts[i].x, pts[i].y,
                                       pts[i+1].x, pts[i+1].y,
                                       pts[i+2].x, pts[i+2].y);
     }
}

EOLIAN static void
_efl_gfx_path_append_close(Eo *obj EINA_UNUSED, Efl_Gfx_Path_Data *pd)
{
   double *offset_point;
   // Appends a "close path" command. This typically draws a line from the
   // current point to the starting point of the current sub-path.
   efl_gfx_path_grow(EFL_GFX_PATH_COMMAND_TYPE_CLOSE, pd, &offset_point);
}

/**
 * @brief Appends a complete circle to the path.
 *
 * @param obj The Efl_Gfx_Path object.
 * @param pd The private data of the Efl_Gfx_Path object.
 * @param xc The x-coordinate of the circle's center.
 * @param yc The y-coordinate of the circle's center.
 * @param radius The radius of the circle.
 */
static void
_efl_gfx_path_append_circle(Eo *obj, Efl_Gfx_Path_Data *pd,
                             double xc, double yc, double radius)
{
   Eina_Bool first = (pd->commands_count <= 0); // Is this the first element in the path?

   // A circle is a special case of an arc.
   _efl_gfx_path_append_arc(obj, pd, (xc - radius), (yc - radius), // x, y of bounding box
                           (2 * radius), (2 * radius),             // width, height of bounding box
                           0, 360);                                // start angle 0, sweep 360 degrees
   _efl_gfx_path_append_close(obj, pd); // Close the circle path.

   // A single, closed circle is convex.
   pd->convex = first;
}

EOLIAN static void
_efl_gfx_path_append_rect(Eo *obj, Efl_Gfx_Path_Data *pd,
                           double x, double y, double w, double h, // Rectangle geometry
                           double rx, double ry)                   // Corner radii for rounded rectangle
{
   Eina_Bool first = (pd->commands_count <= 0);

   // check for invalid rectangle
   if (w <=0 || h<= 0) return;

   if (rx <=0 || ry<=0)
     {
         // add a normal rect.
         _efl_gfx_path_append_move_to(obj, pd, x, y);
         _efl_gfx_path_append_line_to(obj, pd, x, y + h);
         _efl_gfx_path_append_line_to(obj, pd, x + w, y + h);
         _efl_gfx_path_append_line_to(obj, pd, x + w, y);
         _efl_gfx_path_append_close(obj, pd);
         return;
     }

   // clamp the rx and ry radius value.
   rx = 2*rx;
   ry = 2*ry;
   if (rx > w) rx = w;
   if (ry > h) ry = h;

   _efl_gfx_path_append_move_to(obj, pd, x, y + h/2);
   _efl_gfx_path_append_arc(obj, pd, x, y + h - ry, rx, ry, 180, 90);
   _efl_gfx_path_append_arc(obj, pd, x + w - rx, y + h - ry, rx, ry, 270, 90);
   _efl_gfx_path_append_arc(obj, pd, x + w - rx, y, rx, ry, 0, 90);
   _efl_gfx_path_append_arc(obj, pd, x, y, rx, ry, 90, 90);
   _efl_gfx_path_append_close(obj, pd);

   //update convex flag
   // Appends a rectangle, possibly with rounded corners.
   // If rx or ry is zero or negative, a sharp-cornered rectangle is drawn.
   // If this is the first shape in the path, it's marked as convex.
   pd->convex = first;
}

EOLIAN static void
_efl_gfx_path_append_horizontal_to(Eo *obj, Efl_Gfx_Path_Data *pd, double d, // Target x-coordinate
                                   double current_x EINA_UNUSED, // Current x (unused, new x is 'd')
                                   double current_y)             // Current y (remains the same)
{
   // Appends a horizontal line from the current point to (d, current_y).
   _efl_gfx_path_append_line_to(obj, pd, d, current_y);
}

EOLIAN static void
_efl_gfx_path_append_vertical_to(Eo *obj, Efl_Gfx_Path_Data *pd, double d, // Target y-coordinate
                                 double current_x,           // Current x (remains the same)
                                 double current_y EINA_UNUSED) // Current y (unused, new y is 'd')
{
   // Appends a vertical line from the current point to (current_x, d).
   _efl_gfx_path_append_line_to(obj, pd, current_x, d);
}

/**
 * @brief Skips leading whitespace and an optional comma from a string.
 * Used in SVG path parsing.
 *
 * @param content The string to parse.
 * @return Pointer to the string after skipped characters.
 */
static char *
_skipcomma(const char *content)
{
   while (*content && isspace(*content)) content++;
   if (*content == ',') return (char*) content + 1;
   return (char*) content;
}

#if 0
static inline Eina_Bool
_next_isnumber(const char *content)
{
   char *tmp = NULL;

   (void) strtod(content, &tmp);
   return content != tmp;
}
#endif

/**
 * @brief Parses a floating-point number from the beginning of a string.
 * Advances the string pointer past the parsed number and any subsequent comma/whitespace.
 * Assumes "POSIX" locale for decimal point.
 *
 * @param[in,out] content Pointer to the string pointer to parse from. Updated on success.
 * @param[out] number Where the parsed double is stored.
 * @return EINA_TRUE if a number was successfully parsed, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_parse_number(char **content, double *number)
{
   char *end = NULL;
   *number = strtod(*content, &end);
   // if the start of string is not number
   if ((*content) == end) return EINA_FALSE;
   //skip comma if any
   *content = _skipcomma(end);
   return EINA_TRUE;
}

/**
 * @brief Parses an integer from the beginning of a string, treating it as a boolean flag (0 or 1).
 * Advances the string pointer past the parsed number and any subsequent comma/whitespace.
 * Used for parsing flags like "large-arc-flag" or "sweep-flag" in SVG paths.
 *
 * @param[in,out] content Pointer to the string pointer to parse from. Updated on success.
 * @param[out] number Where the parsed flag (0 or 1) is stored.
 * @return EINA_TRUE if a number was successfully parsed, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_parse_long(char **content, int *number)
{
   char *end = NULL;
   *number = strtol(*content, &end, 10) ? 1 : 0; // Converts parsed long to 0 or 1.
   // if the start of string is not number
   if ((*content) == end) return EINA_FALSE;
   *content = _skipcomma(end);
   return EINA_TRUE;
}

/**
 * @brief Determines the number of numeric arguments expected for a given SVG path command character.
 *
 * @param cmd The SVG path command character (e.g., 'M', 'l', 'C').
 * @return The count of expected numeric arguments.
 *         For 'A' (arc): 7 arguments (rx, ry, x-axis-rotation, large-arc-flag, sweep-flag, x, y)
 *         For 'C' (cubic): 6 arguments (x1, y1, x2, y2, x, y)
 *         For 'M', 'L' (moveto, lineto): 2 arguments (x, y)
 *         ...and so on.
 */
static int
_number_count(char cmd)
{
   int count = 0;
   switch (cmd)
     {
      case 'M':
      case 'm':
      case 'L':
      case 'l':
        {
           count = 2;
           break;
        }
      case 'C':
      case 'c':
      case 'E':
      case 'e':
        {
           count = 6;
           break;
        }
      case 'H':
      case 'h':
      case 'V':
      case 'v':
        {
           count = 1;
           break;
        }
      case 'S':
      case 's':
      case 'Q':
      case 'q':
      case 'T':
      case 't':
        {
           count = 4;
           break;
        }
      case 'A':
      case 'a':
        {
           count = 7;
           break;
        }
      default:
         break;
      }
   return count;
}

/**
 * @brief Processes a single SVG path command and appends it to the Efl_Gfx_Path object.
 *
 * Handles relative vs. absolute coordinates based on the command character (lowercase vs. uppercase).
 * Updates the current point (cur_x, cur_y) and the start point of the current subpath (start_x, start_y).
 *
 * @param obj The Efl_Gfx_Path object.
 * @param pd The private data of the Efl_Gfx_Path object.
 * @param cmd The SVG command character (e.g., 'M', 'm', 'L', 'l').
 * @param arr Array of numeric arguments for the command.
 *            Example for 'C x1 y1 x2 y2 x y': arr = {x1, y1, x2, y2, x, y}
 *            Example for 'A rx ry rot laf sf x y': arr = {rx, ry, rot, laf, sf, x, y}
 * @param count The number of arguments in `arr`.
 * @param[in,out] cur_x Pointer to the current x-coordinate. Updated by the function.
 * @param[in,out] cur_y Pointer to the current y-coordinate. Updated by the function.
 * @param[in,out] start_x Pointer to the x-coordinate of the start of the current subpath. Updated by 'M'/'m'.
 * @param[in,out] start_y Pointer to the y-coordinate of the start of the current subpath. Updated by 'M'/'m'.
 */
static void
process_command(Eo *obj, Efl_Gfx_Path_Data *pd, char cmd, double *arr, int count, double *cur_x, double *cur_y, double *start_x, double *start_y)
{
   int i;
   // Adjust coordinates for relative commands (lowercase)
   switch (cmd)
     {
      case 'm':
      case 'l':
      case 'c':
      case 's':
      case 'q':
      case 't':
        {
           for(i = 0; i < count - 1; i += 2)
             {
                arr[i] = arr[i] + *cur_x;
                arr[i+1] = arr[i+1] + *cur_y;
             }
           break;
        }
      case 'h':
        {
           arr[0] = arr[0] + *cur_x;
           break;
        }
      case 'v':
        {
           arr[0] = arr[0] + *cur_y;
           break;
        }
      case 'a':
        {
           arr[5] = arr[5] + *cur_x;
           arr[6] = arr[6] + *cur_y;
           break;
        }
      default:
         break;
      }

   switch (cmd)
     {
      case 'm':
      case 'M':
        {
           _efl_gfx_path_append_move_to(obj, pd, arr[0], arr[1]);
           *cur_x = arr[0];
           *cur_y = arr[1];
           *start_x = arr[0];
           *start_y = arr[1];
           break;
        }
      case 'l':
      case 'L':
        {
           _efl_gfx_path_append_line_to(obj, pd, arr[0], arr[1]);
           *cur_x = arr[0];
           *cur_y = arr[1];
           break;
        }
      case 'c':
      case 'C':
        {
           _efl_gfx_path_append_cubic_to(obj, pd, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
           *cur_x = arr[4];
           *cur_y = arr[5];
           break;
        }
      case 's':
      case 'S':
        {
           _efl_gfx_path_append_scubic_to(obj, pd, arr[2], arr[3], arr[0], arr[1]);
           *cur_x = arr[2];
           *cur_y = arr[3];
           break;
        }
      case 'q':
      case 'Q':
        {
           _efl_gfx_path_append_quadratic_to(obj, pd, arr[2], arr[3], arr[0], arr[1]);
           *cur_x = arr[2];
           *cur_y = arr[3];
           break;
        }
      case 't':
      case 'T':
        {
           _efl_gfx_path_append_move_to(obj, pd, arr[0], arr[1]);
           *cur_x = arr[0];
           *cur_y = arr[1];
           break;
        }
      case 'h':
      case 'H':
        {
           _efl_gfx_path_append_horizontal_to(obj, pd, arr[0], *cur_x, *cur_y);
           *cur_x = arr[0];
           break;
        }
      case 'v':
      case 'V':
        {
           _efl_gfx_path_append_vertical_to(obj, pd, arr[0], *cur_x, *cur_y);
           *cur_y = arr[0];
           break;
        }
      case 'z':
      case 'Z':
        {
           _efl_gfx_path_append_close(obj, pd);
           *cur_x = *start_x;
           *cur_y = *start_y;
           break;
        }
      case 'a':
      case 'A':
        {
           _efl_gfx_path_append_arc_to(obj, pd, arr[5], arr[6], arr[0], arr[1], arr[2], arr[3], arr[4]);
           *cur_x = arr[5];
           *cur_y = arr[6];
           break;
        }
      case 'E':
      case 'e':
        {
           _efl_gfx_path_append_arc(obj, pd, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
           break;
        }
      default:
         break;
      }
}

/**
 * @brief Parses the next command and its arguments from an SVG path string.
 *
 * Handles implicit commands (e.g., if 'M' is followed by multiple coordinate pairs,
 * subsequent pairs are treated as 'L' commands).
 *
 * @param path The current position in the SVG path string.
 * @param[in,out] cmd Pointer to store the parsed command character. If the next token is not
 *                    a letter, this reuses the previous command (e.g. 'M' becomes 'L').
 * @param[out] arr Array to store the parsed numeric arguments for the command.
 *                 Must be large enough for the command with the most arguments (e.g., 7 for arc).
 * @param[out] count Pointer to store the number of arguments parsed into `arr`.
 * @return Pointer to the string after the parsed command and arguments, or NULL on error.
 */
static char *
_next_command(char *path, char *cmd, double *arr, int *count)
{
   int i=0, large, sweep; // Variables for parsing arc flags

   path = _skipcomma(path); // Skip leading whitespace/commas
   if (isalpha(*path)) // Check if the next token is a command letter
     {
        *cmd = *path; // Store the new command
        path++;
        *count = _number_count(*cmd); // Get expected argument count for this command
     }
   else // Not a letter, so it's an implicit command (more arguments for the previous command type)
     {
        // SVG spec: If a moveto is followed by multiple pairs of coordinates,
        // the subsequent pairs are treated as implicit lineto commands.
        if (*cmd == 'm')
          *cmd = 'l'; // Implicit relative lineto
        else if (*cmd == 'M')
          *cmd = 'L'; // Implicit absolute lineto
        // For other commands, if more data follows, it's for the same command type.
        // *count remains as set by the previous explicit command.
     }

   if ( *count == 7) // Special parsing for arc command (A/a) due to flag arguments
     {
        // Arc command arguments: rx, ry, x-axis-rotation, large-arc-flag, sweep-flag, x, y
        if(_parse_number(&path, &arr[0])) // rx
          if(_parse_number(&path, &arr[1])) // ry
            if(_parse_number(&path, &arr[2])) // x-axis-rotation
               if(_parse_long(&path, &large))  // large-arc-flag
                  if(_parse_long(&path, &sweep)) // sweep-flag
                     if(_parse_number(&path, &arr[5])) // x
                        if(_parse_number(&path, &arr[6])) // y
                          {
                             arr[3] = large; // Store parsed large-arc-flag
                             arr[4] = sweep; // Store parsed sweep-flag
                             return path;    // Successfully parsed arc command
                          }
         *count = 0; // Parsing failed
         return NULL;
     }

   // Parse numeric arguments for other commands
   for (i = 0; i < *count; i++)
     {
        if (!_parse_number(&path, &arr[i]))
          {
             *count = 0; // Parsing failed
             return NULL;
          }
        path = _skipcomma(path); // Skip separators
     }
   return path; // Successfully parsed command and its arguments
}

/**
 * @brief Interpolates between two SVG path strings and applies the result to the Efl_Gfx_Path object.
 *
 * This function is called when interpolating paths that were originally defined by SVG strings
 * and contain arc commands (which are complex to interpolate numerically point-wise).
 * It parses both 'from' and 'to' SVG path strings command by command, interpolates the
 * corresponding numeric arguments, and then processes the interpolated command.
 *
 * @note This function temporarily sets the LC_NUMERIC locale to "POSIX" to ensure
 *       correct parsing of floating-point numbers (using '.' as decimal separator).
 *
 * @param obj The Efl_Gfx_Path object to apply the interpolated path to.
 * @param pd The private data of the Efl_Gfx_Path object.
 * @param from The starting SVG path string.
 * @param to The ending SVG path string.
 * @param pos The interpolation factor (0.0 for 'from', 1.0 for 'to').
 */
static void
_path_interpolation(Eo *obj, Efl_Gfx_Path_Data *pd,
                     char *from, char *to, double pos)
{
   int i;
   double from_arr[7], to_arr[7];
   int from_count=0, to_count=0;
   double cur_x=0, cur_y=0;
   double start_x=0, start_y=0;
   char from_cmd= 0, to_cmd = 0;
   char *cur_locale;

   if (!from || !to)
     return;

   cur_locale = setlocale(LC_NUMERIC, NULL);
   if (cur_locale)
     cur_locale = strdup(cur_locale);
   setlocale(LC_NUMERIC, "POSIX");

   while ((from[0] != '\0') && (to[0] != '\0'))
     {
        from = _next_command(from, &from_cmd, from_arr, &from_count);
        to = _next_command(to, &to_cmd, to_arr, &to_count);
        if (from_cmd == to_cmd)
          {
             if (from_count == 7)
               {
                  //special case for arc command
                  i=0;
                  from_arr[i] = interpolate(from_arr[i], to_arr[i], pos);
                  i=1;
                  from_arr[i] = interpolate(from_arr[i], to_arr[i], pos);
                  i=2;
                  from_arr[i] = interpolate(from_arr[i], to_arr[i], pos);
                  i=5;
                  from_arr[i] = interpolate(from_arr[i], to_arr[i], pos);
                  i=6;
                  from_arr[i] = interpolate(from_arr[i], to_arr[i], pos);
               }
             else
               {
                  for(i=0; i < from_count; i++)
                    {
                       from_arr[i] = interpolate(from_arr[i], to_arr[i], pos);
                    }
               }
            process_command(obj, pd, from_cmd, from_arr, from_count, &cur_x, &cur_y, &start_x, &start_y);
          }
        else
          {
             goto error;
          }
     }

error:
   setlocale(LC_NUMERIC, cur_locale);
   if (cur_locale)
     free(cur_locale);
}

/**
 * @brief Parses an SVG path data string and appends the described path to the Efl_Gfx_Path object.
 *
 * Supports standard SVG path commands (M, m, L, l, H, h, V, v, C, c, S, s, Q, q, T, t, A, a, Z, z).
 * Also supports a non-standard 'E'/'e' command for appending an arc defined by bounding box,
 * start angle, and sweep length (similar to _efl_gfx_path_append_arc).
 *
 * @note This function temporarily sets the LC_NUMERIC locale to "POSIX" to ensure
 *       correct parsing of floating-point numbers (using '.' as decimal separator).
 *       If the SVG path string contains arc commands ('a', 'A', 'e', 'E'), a copy of
 *       the string is stored in `pd->path_data` for potential future interpolation.
 *
 * @param obj The Efl_Gfx_Path object.
 * @param pd The private data of the Efl_Gfx_Path object.
 * @param svg_path_data The SVG path string to parse.
 *        Example: "M 10 10 L 20 20 C 20 30, 30 30, 40 20 Z"
 */
EOLIAN static void
_efl_gfx_path_append_svg_path(Eo *obj, Efl_Gfx_Path_Data *pd,
                              const char *svg_path_data)
{
   double number_array[7]; // Buffer for parsed numeric arguments, 7 is max for Arc command.
   int number_count = 0;   // Number of arguments parsed for the current command.
   double cur_x=0, cur_y=0; // Current point in the path.
   double start_x=0, start_y=0; // Start point of the current sub-path (for 'Z' command).
   char cmd= 0;             // Current SVG command character.
   char *path = (char *) svg_path_data; // Mutable pointer to traverse the path string.
   Eina_Bool has_arc_command = EINA_FALSE; // Flag if any arc command was encountered.
   char *cur_locale;       // To save and restore original locale.

   if (!path)
     return;

   // Ensure POSIX locale for number parsing ('.' as decimal separator).
   cur_locale = setlocale(LC_NUMERIC, NULL);
   if (cur_locale)
     cur_locale = strdup(cur_locale); // strdup because setlocale might return a static buffer.
   setlocale(LC_NUMERIC, "POSIX");

   while ((path[0] != '\0')) // Loop until end of path string.
     {
        path = _next_command(path, &cmd, number_array, &number_count);
        if (!path) // Error during command parsing.
          {
             // ERR("Error parsing SVG path command near: %s", svg_path_data + (path - svg_path_data) - 10 ); // Example error log
             goto error;
          }
        process_command(obj, pd, cmd, number_array, number_count, &cur_x, &cur_y, &start_x, &start_y);
        if ((!has_arc_command) && ((cmd == 'a') || (cmd == 'A') ||
            (cmd == 'e') || (cmd == 'E'))) // Non-standard 'e'/'E' also considered arc for interpolation.
          has_arc_command = EINA_TRUE;
     }

   if (has_arc_command)
     {
        // If the path string contained arc commands, store the original string.
        // This is needed for _path_interpolation, as arc interpolation is done
        // on the string representation rather than numerically on Bezier points.
        if (pd->path_data)
          free(pd->path_data);
        pd->path_data = strdup(svg_path_data); // Store a copy.
        if (!pd->path_data) { /* ERR("Failed to duplicate svg_path_data"); */ goto error; }
     }

error:
   // Restore original locale.
   setlocale(LC_NUMERIC, cur_locale);
   if (cur_locale)
     free(cur_locale);
}

EOLIAN static void
_efl_gfx_path_copy_from(Eo *obj, Efl_Gfx_Path_Data *pd, const Eo *dup_from)
{
   Efl_Gfx_Path_Data *from;

   if (obj == dup_from) return;
   from = efl_data_scope_get(dup_from, EFL_GFX_PATH_MIXIN);
   if (!from) return;

   // Copies all path data (commands, points, current point, control point, convex flag)
   // from another Efl_Gfx_Path object (dup_from) to this object (pd).
   // If the source path had an SVG string stored (from->path_data), that string is
   // also duplicated and stored if it contained arc commands. This is handled
   // within _efl_gfx_path_path_set if it calls _efl_gfx_path_append_svg_path,
   // or more directly, if path_data is also copied.
   // Current implementation of _efl_gfx_path_path_set does not preserve path_data,
   // so this copy might lose the SVG string if not handled carefully.
   // However, _efl_gfx_path_path_set will re-evaluate current point and control point.
   // If `from->path_data` exists and is important, it should be explicitly copied here too.
   // For now, it relies on `_efl_gfx_path_path_set` to reconstruct the path.

   pd->convex = from->convex;

   // If the source path has an SVG string representation (due to arcs),
   // and we want to preserve it for interpolation, we should copy it.
   // The _efl_gfx_path_path_set will set commands and points, but might clear path_data.
   // To ensure path_data is preserved if it exists and contains arcs:
   if (from->path_data)
     {
        // A simple way is to re-append if it's an SVG path with arcs.
        // However, _efl_gfx_path_path_set is more direct for command/point copy.
        // Let's ensure path_data is copied if it exists.
        // This assumes that if from->path_data is set, it's the canonical source.
        // A more robust copy would involve checking if from->path_data is set and using append_svg_path,
        // or copying commands/points and then path_data.
        // For now, let's ensure path_data is copied if it exists.
        if (pd->path_data) free(pd->path_data);
        pd->path_data = from->path_data ? strdup(from->path_data) : NULL;
     }


   _efl_gfx_path_path_set(obj, pd, from->commands, from->points);
   // After _efl_gfx_path_path_set, pd->current and pd->current_ctrl are updated.
   // pd->path_data might be overwritten by _efl_gfx_path_path_set if it internally calls reset.
   // The above explicit copy of path_data might be cleared by _efl_gfx_path_path_set.
   // The most robust way is to copy commands, points, and then explicitly copy path_data if it was present.
   // Let's refine:
   // 1. Reset current path
   // 2. Copy commands and points
   // 3. Copy path_data if present in source
   // 4. Update current points (done by _efl_gfx_path_path_set)

   // The current _efl_gfx_path_path_set reallocs and memcpys, then recalculates current points.
   // It does not touch pd->path_data. So, the earlier copy of pd->path_data is fine.
}

#include "interfaces/efl_gfx_path.eo.c"
