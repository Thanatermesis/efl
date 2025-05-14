#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_LAYOUT_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"

#include "elm_widget_layout.h"
#include "efl_ui_textpath_part.eo.h"
#include "elm_part_helper.h"


#define MY_CLASS EFL_UI_TEXTPATH_CLASS
#define MY_CLASS_NAME "Efl.Ui.Textpath"
#define LEGACY_TEXT_PART_NAME "elm.text"
#define EFL_UI_TEXT_PART_NAME "efl.text"

#define SLICE_DEFAULT_NO 99

/**
 * @brief Represents a 2D point.
 */
typedef struct _Efl_Ui_Textpath_Point Efl_Ui_Textpath_Point;
/**
 * @brief Represents a line segment defined by two points.
 */
typedef struct _Efl_Ui_Textpath_Line Efl_Ui_Textpath_Line;
/**
 * @brief Represents a segment of the path, which can be a line or a Bezier curve.
 *        These segments form an Eina_Inlist.
 */
typedef struct _Efl_Ui_Textpath_Segment Efl_Ui_Textpath_Segment;
/**
 * @brief Private data structure for the Efl_Ui_Textpath widget.
 */
typedef struct _Efl_Ui_Textpath_Data Efl_Ui_Textpath_Data;

struct _Efl_Ui_Textpath_Point
{
   double x; /**< The x-coordinate of the point. */
   double y; /**< The y-coordinate of the point. */
};

struct _Efl_Ui_Textpath_Line
{
   Efl_Ui_Textpath_Point start; /**< The starting point of the line. */
   Efl_Ui_Textpath_Point end;   /**< The ending point of the line. */
};

struct _Efl_Ui_Textpath_Segment
{
   EINA_INLIST; /**< Macro to make this struct usable in an Eina_Inlist. */
   int length;  /**< The calculated length of this segment. */
   Efl_Gfx_Path_Command_Type type; /**< The type of the path segment (line or cubic Bezier). */
   union
     {
        Eina_Bezier bezier; /**< Bezier curve data if type is EFL_GFX_PATH_COMMAND_TYPE_CUBIC_TO. */
        Efl_Ui_Textpath_Line line; /**< Line data if type is EFL_GFX_PATH_COMMAND_TYPE_LINE_TO. */
     };
};

/* If you need to draw slices using Evas Line,
 * define the following debug flag manually. */
//#define EFL_UI_TEXTPATH_LINE_DEBUG

struct _Efl_Ui_Textpath_Data
{
   Evas_Object *text_obj; /**< The internal Edje object used to render the text. */
   char *text; /**< The text string to be displayed along the path. (Legacy, consider if still needed directly) */
   Eina_Strbuf *user_style; /**< User-defined text style string. */
   Efl_Gfx_Path *path;      /**< The graphics path object defining the trajectory for the text. (Legacy, path is now part of the object itself) */
   /**
    * @brief Parameters for circular text path.
    */
   struct {
        double x;           /**< X-coordinate of the circle's center. */
        double y;           /**< Y-coordinate of the circle's center. */
        double radius;      /**< Radius of the circle. */
        double start_angle; /**< Starting angle for the text on the circle (in degrees). */
   } circle;
   Efl_Ui_Textpath_Direction direction; /**< Direction of text on a circular path (clockwise/counter-clockwise). */
   int slice_no; /**< Number of slices used to approximate curves, affecting smoothness. */
   Eina_Bool ellipsis; /**< Whether to apply ellipsis if the text overflows the path. */

   Eina_Inlist *segments; /**< List of path segments (lines or Bezier curves). @see _Efl_Ui_Textpath_Segment */
   int total_length;      /**< Total calculated length of all path segments. */
#ifdef EFL_UI_TEXTPATH_LINE_DEBUG
   Eina_List *lines; /**< List of Evas_Object lines for debugging segment drawing. */
#endif
   Eina_Bool need_redraw : 1; /**< Flag indicating if the text path needs to be redrawn. */
   Eina_Bool circular : 1;   /**< Flag indicating if the path is currently circular. TODO: Remove this flag when elm_textpath_circle_set() is removed. */
};

#define EFL_UI_TEXTPATH_DATA_GET(o, sd) \
   Efl_Ui_Textpath_Data *sd = efl_data_scope_get(o, EFL_UI_TEXTPATH_CLASS)

/**
 * @brief Converts radians to degrees.
 * @param rad Angle in radians.
 * @return Angle in degrees.
 */
static inline double
_rad_to_deg(double rad)
{
   return 180 * rad / M_PI;
}

/**
 * @brief Draws a segment of the text along a Bezier curve using Evas_Map.
 *
 * This function divides the Bezier curve segment into smaller slices and maps
 * corresponding parts of the text object onto these slices.
 *
 * @param pd Private data of the textpath widget.
 * @param slice_no Number of slices to divide this Bezier segment into.
 * @param dt Parameter step for traversing the Bezier curve for each slice.
 * @param dist Horizontal distance in the source text image corresponding to each slice.
 * @param w1 Starting horizontal position (u-coordinate) in the source text image for this segment.
 * @param cmp Current map point index to start adding points from.
 * @param map The Evas_Map to populate with transformation points.
 * @param bezier The Bezier curve definition for this segment.
 * @param last_x1 Pointer to store the x-coordinate of the last top-left point.
 * @param last_y1 Pointer to store the y-coordinate of the last top-left point.
 * @param last_x2 Pointer to store the x-coordinate of the last bottom-left point.
 * @param last_y2 Pointer to store the y-coordinate of the last bottom-left point.
 */
static void
_segment_draw(Efl_Ui_Textpath_Data *pd, int slice_no, double dt, double dist,
              int w1, int cmp, Evas_Map *map, Eina_Bezier bezier,
              int *last_x1, int *last_y1, int *last_x2, int *last_y2)
{
   int i;
   double u0, u1, v0, v1;
   double t;
   double px, py, px2, py2;
   Eina_Rect r;
   Eina_Vector2 vec, nvec, vec0, vec1, vec2, vec3;
   Eina_Matrix2 mat;
#ifdef EFL_UI_TEXTPATH_LINE_DEBUG
   static Eina_Bool yello_color_flag = EINA_FALSE;
   yello_color_flag = !yello_color_flag;
#endif

   r = efl_gfx_entity_geometry_get(pd->text_obj);
   eina_matrix2_values_set(&mat, 0.0, -1.0, 1.0, 0.0);

   eina_bezier_values_get(&bezier, NULL, NULL, NULL, NULL, NULL, NULL, &px2, &py2);
   t = 0;
   eina_bezier_point_at(&bezier, t, &px, &py);
   eina_bezier_point_at(&bezier, t + dt, &px2, &py2);

   vec.x = (px2 - px);
   vec.y = (py2 - py);
   eina_vector2_normalize(&nvec, &vec);

   eina_vector2_transform(&vec, &mat, &nvec);
   eina_vector2_normalize(&nvec, &vec);
   eina_vector2_scale(&vec, &nvec, ((double) r.h) * 0.5);

   vec1.x = (vec.x + px);
   vec1.y = (vec.y + py);
   vec2.x = (-vec.x + px);
   vec2.y = (-vec.y + py);

   if (cmp == 0)
     {
        *last_x1 = (int) round(vec1.x + r.x);
        *last_y1 = (int) round(vec1.y + r.y);
        *last_x2 = (int) round(vec2.x + r.x);
        *last_y2 = (int) round(vec2.y + r.y);
     }

   //add points to map
   for (i = 0; i < slice_no; i++)
     {
        int mp0_x, mp0_y;
        int mp1_x, mp1_y;
        int mp2_x, mp2_y;
        int mp3_x, mp3_y;
        double next_dt = dt;

        //v0, v3
        vec0.x = vec1.x;
        vec0.y = vec1.y;
        vec3.x = vec2.x;
        vec3.y = vec2.y;

        //UV
        u0 = w1 + i * dist;
        u1 = u0 + dist;
        if (u1 > r.w)
          u1 = r.w;
        v0 = (double) 0;
        v1 = (double) r.h;

        /* If u1 is modified not to exceed its end,
         * modify next_dt according to changes of dist. */
        if (u1 < u0 + dist)
          next_dt = dt * (u1 - u0) / dist;

        //v1, v2
        t = (double) (i * dt) + next_dt;
        eina_bezier_point_at(&bezier, t, &px, &py);
        eina_bezier_point_at(&bezier, t + next_dt, &px2, &py2);

        vec.x = (px2 - px);
        vec.y = (py2 - py);
        eina_vector2_normalize(&nvec, &vec);
        eina_vector2_transform(&vec, &mat, &nvec);
        eina_vector2_normalize(&nvec, &vec);
        eina_vector2_scale(&vec, &nvec, ((double) r.h) * 0.5);

        vec1.x = (vec.x + px);
        vec1.y = (vec.y + py);
        vec2.x = (-vec.x + px);
        vec2.y = (-vec.y + py);

        /* Set mp1, mp2 position according to difference between
         * previous points and next points.
         * It improves smoothness of curve's slope changing.
         * But, it can cause huge differeces from actual positions. */
        mp0_x = *last_x1;
        mp0_y = *last_y1;
        mp1_x = *last_x1 + (int) round(vec1.x - vec0.x);
        mp1_y = *last_y1 + (int) round(vec1.y - vec0.y);
        mp2_x = *last_x2 + (int) round(vec2.x - vec3.x);
        mp2_y = *last_y2 + (int) round(vec2.y - vec3.y);
        mp3_x = *last_x2;
        mp3_y = *last_y2;

        /* It reduces differences between actual position and modified position. */
        mp1_x += (int)round(((double)vec1.x - mp1_x) / 2);
        mp1_y += (int)round(((double)vec1.y - mp1_y) / 2);
        mp2_x += (int)round(((double)vec2.x - mp2_x) / 2);
        mp2_y += (int)round(((double)vec2.y - mp2_y) / 2);

        evas_map_point_coord_set(map, cmp + i * 4, mp0_x, mp0_y, 0);
        evas_map_point_coord_set(map, cmp + i * 4 + 1, mp1_x, mp1_y, 0);
        evas_map_point_coord_set(map, cmp + i * 4 + 2, mp2_x, mp2_y, 0);
        evas_map_point_coord_set(map, cmp + i * 4 + 3, mp3_x, mp3_y, 0);

        evas_map_point_image_uv_set(map, cmp + i * 4, u0, v0);
        evas_map_point_image_uv_set(map, cmp + i * 4 + 1, u1, v0);
        evas_map_point_image_uv_set(map, cmp + i * 4 + 2, u1, v1);
        evas_map_point_image_uv_set(map, cmp + i * 4 + 3, u0, v1);

        *last_x1 = mp1_x;
        *last_y1 = mp1_y;
        *last_x2 = mp2_x;
        *last_y2 = mp2_y;

#ifdef EFL_UI_TEXTPATH_LINE_DEBUG
        Evas_Object *line = evas_object_line_add(evas_object_evas_get(pd->text_obj));
        pd->lines = eina_list_append(pd->lines, line);
        if (yello_color_flag)
          evas_object_color_set(line, 255, 255, 0, 255);
        else
          evas_object_color_set(line, 255, 0, 0, 255);
        evas_object_line_xy_set(line,
                                mp0_x, mp0_y,
                                mp1_x, mp1_y);
        evas_object_show(line);

        line = evas_object_line_add(evas_object_evas_get(pd->text_obj));
        pd->lines = eina_list_append(pd->lines, line);
        if (yello_color_flag)
          evas_object_color_set(line, 255, 255, 0, 255);
        else
          evas_object_color_set(line, 255, 0, 0, 255);
        evas_object_line_xy_set(line,
                                mp1_x, mp1_y,
                                mp2_x, mp2_y);
        evas_object_show(line);

        line = evas_object_line_add(evas_object_evas_get(pd->text_obj));
        pd->lines = eina_list_append(pd->lines, line);
        if (yello_color_flag)
          evas_object_color_set(line, 255, 255, 0, 255);
        else
          evas_object_color_set(line, 255, 0, 0, 255);
        evas_object_line_xy_set(line,
                                mp2_x, mp2_y,
                                mp3_x, mp3_y);
        evas_object_show(line);

        line = evas_object_line_add(evas_object_evas_get(pd->text_obj));
        pd->lines = eina_list_append(pd->lines, line);
        if (yello_color_flag)
          evas_object_color_set(line, 255, 255, 0, 255);
        else
          evas_object_color_set(line, 255, 0, 0, 255);
        evas_object_line_xy_set(line,
                                mp3_x, mp3_y,
                                mp0_x, mp0_y);
        evas_object_show(line);
#endif

        if (u1 >= r.w) break;
     }
}

/**
 * @brief Draws a segment of the text along a straight line using Evas_Map.
 *
 * This function maps a rectangular portion of the text object onto a quadrilateral
 * defined by the line segment and the text height.
 *
 * @param pd Private data of the textpath widget.
 * @param w1 Starting horizontal position (u-coordinate) in the source text image for this segment.
 * @param w2 Ending horizontal position (u-coordinate) in the source text image for this segment.
 * @param cmp Current map point index to start adding points from (expects 4 points for a quad).
 * @param map The Evas_Map to populate with transformation points.
 * @param line The line segment definition.
 */
static void
_text_on_line_draw(Efl_Ui_Textpath_Data *pd, int w1, int w2, int cmp, Evas_Map *map, Efl_Ui_Textpath_Line line)
{
   double x1, x2, y1, y2;
   double line_len_2, line_len, len, sina, cosa;
   Eina_Rect r;

   x1 = line.start.x;
   y1 = line.start.y;
   x2 = line.end.x;
   y2 = line.end.y;

   line_len_2 = (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1);
   len = w2 - w1;
   if (line_len_2 > (len * len))
     {
        line_len = sqrt(line_len_2);
        x2 = x1 + len * (x2 - x1) / line_len;
        y2 = y1 + len * (y2 - y1) / line_len;
        line_len_2 = (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1);
     }

   len = sqrt(line_len_2);
   sina = (y2 - y1) / len;
   cosa = (x2 - x1) / len;

   r = efl_gfx_entity_geometry_get(pd->text_obj);
   r.h /= 2;
   evas_map_point_coord_set(map, cmp + 3, x1 - r.h * sina, y1 + r.h * cosa, 0);
   evas_map_point_coord_set(map, cmp + 2, x2 - r.h * sina, y2 + r.h * cosa, 0);
   evas_map_point_coord_set(map, cmp + 1, x2 + r.h * sina, y2 - r.h * cosa, 0);
   evas_map_point_coord_set(map, cmp + 0, x1 + r.h * sina, y1 - r.h * cosa, 0);

   r.h *= 2;
   evas_map_point_image_uv_set(map, cmp + 0, w1, 0);
   evas_map_point_image_uv_set(map, cmp + 1, w2, 0);
   evas_map_point_image_uv_set(map, cmp + 2, w2, r.h);
   evas_map_point_image_uv_set(map, cmp + 3, w1, r.h);
}

/**
 * @brief Calculates the total number of Evas_Map points required to draw the text along the path.
 *
 * Each line segment requires 4 map points (for one quadrilateral).
 * Each Bezier curve segment requires `slice_no_for_segment * 4` map points,
 * where `slice_no_for_segment` is proportional to the segment's length relative
 * to the total path length and the global `slice_no` setting.
 *
 * @param pd Private data of the textpath widget.
 * @return The total number of map points needed.
 */
static int
_map_point_calc(Efl_Ui_Textpath_Data *pd)
{
   int map_no = 0;
   Efl_Ui_Textpath_Segment *seg;

   EINA_INLIST_FOREACH(pd->segments, seg)
     {
        if (seg->type == EFL_GFX_PATH_COMMAND_TYPE_LINE_TO)
          {
             map_no++;
          }
        else if (seg->type == EFL_GFX_PATH_COMMAND_TYPE_CUBIC_TO)
          {
             int no = (int)ceil(pd->slice_no * seg->length / (double)pd->total_length);
             if (no == 0) no = 1;
             map_no += no;
          }
     }
   map_no *= 4;

   return map_no;
}

/**
 * @brief Main function to draw the text along the defined path.
 *
 * It iterates through all path segments (lines or Bezier curves) and calls
 * the appropriate drawing function (_text_on_line_draw or _segment_draw)
 * to populate an Evas_Map. This map is then applied to the text object
 * to render it along the path.
 *
 * @param pd Private data of the textpath widget.
 */
static void
_text_draw(Efl_Ui_Textpath_Data *pd)
{
   Efl_Ui_Textpath_Segment *seg;
   Evas_Map *map;
   int w1, w2;
   int remained_w;
   int cur_map_point = 0, map_point_no;
   Eina_Size2D sz;
   int last_x1, last_y1, last_x2, last_y2;

   last_x1 = last_y1 = last_x2 = last_y2 = 0;

   sz = efl_gfx_entity_size_get(pd->text_obj);
   remained_w = sz.w;

   map_point_no = _map_point_calc(pd);
   if (map_point_no == 0)
     {
        evas_object_map_enable_set(pd->text_obj, EINA_FALSE);
        return;
     }
   map = evas_map_new(map_point_no);

#ifdef EFL_UI_TEXTPATH_LINE_DEBUG
   Evas_Object *line;
   EINA_LIST_FREE(pd->lines, line)
      evas_object_del(line);
#endif

   w1 = w2 = 0;
   EINA_INLIST_FOREACH(pd->segments, seg)
     {
        int len = seg->length;
        if (remained_w <= 0)
          break;
        w2 = w1 + len;
        if (w2 > sz.w)
          w2 = sz.w;
        if (seg->type == EFL_GFX_PATH_COMMAND_TYPE_LINE_TO)
          {
             _text_on_line_draw(pd, w1, w2, cur_map_point, map, seg->line);
             cur_map_point += 4;
          }
        else
          {
             double slice_value, dt, dist;
             int slice_no;

             slice_value = pd->slice_no * seg->length / (double)pd->total_length;
             dt = (double)pd->total_length / (pd->slice_no * seg->length);
             dist = (double)pd->total_length / (double)pd->slice_no;

             slice_no = (int)ceil(slice_value);
             dt = (double)slice_value * dt / (double)slice_no;
             dist = (double)slice_value * dist / (double)slice_no;

             _segment_draw(pd, slice_no, dt, dist,
                           w1, cur_map_point, map, seg->bezier,
                           &last_x1, &last_y1, &last_x2, &last_y2);
             cur_map_point += slice_no * 4;
          }
        w1 = w2;
        remained_w -= seg->length;
     }
   evas_object_map_enable_set(pd->text_obj, EINA_TRUE);
   evas_object_anti_alias_set(pd->text_obj, EINA_TRUE);
   evas_object_map_set(pd->text_obj, map);
   evas_map_free(map);

   pd->need_redraw = EINA_FALSE;
}

/**
 * @brief Evas pre-render callback.
 *
 * This function is called before rendering each frame. If a redraw is needed
 * (pd->need_redraw is true), it calls _text_draw to update the text rendering.
 *
 * @param data User data, which is the Efl_Ui_Textpath_Data pointer.
 * @param e The Evas canvas.
 * @param ev Event specific information (unused).
 */
static void
_render_pre_cb(void *data, Evas *e EINA_UNUSED, void *ev EINA_UNUSED)
{
   _text_draw(data);
}

/**
 * @brief Parses the Efl_Gfx_Path data and populates the internal segment list.
 *
 * This function iterates over the commands and points in the object's path,
 * converting them into a list of _Efl_Ui_Textpath_Segment structures.
 * It calculates the length of each segment and the total path length.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 */
static void
_path_data_get(Eo *obj, Efl_Ui_Textpath_Data *pd)
{
   const Efl_Gfx_Path_Command_Type *cmd;
   const double *points;
   Efl_Ui_Textpath_Segment *seg;
   Eina_Position2D obj_pos;

   EINA_INLIST_FREE(pd->segments, seg)
     {
        pd->segments = eina_inlist_remove(pd->segments, EINA_INLIST_GET(seg));
        free(seg);
     }

   obj_pos = efl_gfx_entity_position_get(obj);

   /* textpath calculates boundary with the middle of text height.
      this has better precise boundary than circle_set() behavior. */
   if (pd->circular)
     {
        Eina_Size2D text_size = efl_gfx_entity_size_get(pd->text_obj);
        obj_pos.x += (text_size.h / 2);
        obj_pos.y += (text_size.h / 2);
     }

   pd->total_length = 0;
   efl_gfx_path_get(obj, &cmd, &points);
   if (cmd)
     {
        int pos = -1;
        double px0 = 0.0, py0 = 0.0, ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1, px1, py1;
        Eina_Rect rect = EINA_RECT_ZERO();

        while (*cmd != EFL_GFX_PATH_COMMAND_TYPE_END)
          {
             if (*cmd == EFL_GFX_PATH_COMMAND_TYPE_MOVE_TO)
               {
                  pos++;
                  px0 = points[pos] + obj_pos.x;
                  pos++;
                  py0 = points[pos] + obj_pos.y;
               }
             else if (*cmd == EFL_GFX_PATH_COMMAND_TYPE_CUBIC_TO)
               {
                  Eina_Bezier bz;
                  double bx, by, bw, bh;
                  Eina_Rect brect;

                  pos++;
                  ctrl_x0 = points[pos] + obj_pos.x;
                  pos++;
                  ctrl_y0 = points[pos] + obj_pos.y;
                  pos++;
                  ctrl_x1 = points[pos] + obj_pos.x;
                  pos++;
                  ctrl_y1 = points[pos] + obj_pos.y;
                  pos++;
                  px1 = points[pos] + obj_pos.x;
                  pos++;
                  py1 = points[pos] + obj_pos.y;

                  eina_bezier_values_set(&bz, px0, py0, ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1, px1, py1);
                  seg = malloc(sizeof(Efl_Ui_Textpath_Segment));
                  if (!seg)
                    {
                       ERR("Failed to allocate segment");
                       px0 = px1;
                       py0 = py1;
                       continue;
                    }
                  seg->length = eina_bezier_length_get(&bz);
                  seg->bezier = bz;
                  seg->type = EFL_GFX_PATH_COMMAND_TYPE_CUBIC_TO;
                  pd->segments = eina_inlist_append(pd->segments, EINA_INLIST_GET(seg));
                  pd->total_length += seg->length;

                  //move points
                  px0 = px1;
                  py0 = py1;

                  eina_bezier_bounds_get(&bz, &bx, &by, &bw, &bh);
                  brect = EINA_RECT(bx, by, bw, bh);
                  eina_rectangle_union(&rect.rect, &brect.rect);
               }
             else if (*cmd == EFL_GFX_PATH_COMMAND_TYPE_LINE_TO)
               {
                  Eina_Rect lrect;

                  pos++;
                  px1 = points[pos] + obj_pos.x;
                  pos++;
                  py1 = points[pos] + obj_pos.y;

                  seg = malloc(sizeof(Efl_Ui_Textpath_Segment));
                  if (!seg)
                    {
                       ERR("Failed to allocate segment");
                       px0 = px1;
                       py0 = py1;
                       continue;
                    }
                  seg->type = EFL_GFX_PATH_COMMAND_TYPE_LINE_TO;
                  seg->line.start.x = px0;
                  seg->line.start.y = py0;
                  seg->line.end.x = px1;
                  seg->line.end.y = py1;
                  seg->length = sqrt((px1 - px0)*(px1 - px0) + (py1 - py0)*(py1 - py0));
                  pd->segments = eina_inlist_append(pd->segments, EINA_INLIST_GET(seg));
                  pd->total_length += seg->length;

                  lrect = EINA_RECT(px0, py0, px1 - px0, py1 - py0);
                  eina_rectangle_union(&rect.rect, &lrect.rect);
               }
             cmd++;
          }
     }
}

/**
 * @brief Marks the textpath for redraw and recalculation.
 *
 * Typically called when properties affecting the layout or appearance change.
 * Sets the need_redraw flag.
 *
 * @param pd Private data of the textpath widget.
 */
static void
_sizing_eval(Efl_Ui_Textpath_Data *pd)
{
   pd->need_redraw = EINA_TRUE;
}

/**
 * @brief Applies or removes the ellipsis style to the text part of the internal Edje object.
 *
 * Modifies the user style string to include or exclude "ellipsis=1.0"
 * and pushes it to the Edje object.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @param enabled EINA_TRUE to enable ellipsis, EINA_FALSE to disable.
 */
static void
_textpath_ellipsis_set(Eo *obj, Efl_Ui_Textpath_Data *pd, Eina_Bool enabled)
{
   char *text_part;
   if (elm_widget_is_legacy(obj))
     text_part = LEGACY_TEXT_PART_NAME;
   else
     text_part = EFL_UI_TEXT_PART_NAME;

   edje_object_part_text_style_user_pop(pd->text_obj, text_part);

   if (enabled)
     {
        if (pd->user_style)
          {
             eina_strbuf_replace_first(pd->user_style, "DEFAULT='", "DEFAULT='ellipsis=1.0 ");
             edje_object_part_text_style_user_push(pd->text_obj, text_part,
                                                   eina_strbuf_string_get(pd->user_style));
          }
        else
          {
             edje_object_part_text_style_user_push(pd->text_obj, text_part,
                                                   "DEFAULT='ellipsis=1.0 '");
             return;
          }
     }
   else
     {
        if (pd->user_style)
          {
             eina_strbuf_replace_first(pd->user_style, "DEFAULT='ellipsis=1.0 ", "DEFAULT='");
             edje_object_part_text_style_user_push(pd->text_obj, text_part,
                                                   eina_strbuf_string_get(pd->user_style));
          }
     }
}

/**
 * @brief Configures ellipsis behavior based on text length versus path length.
 *
 * This function checks if the native width of the text exceeds the total path length.
 * If ellipsis is enabled (pd->ellipsis is EINA_TRUE) and the text is too long,
 * it enables the ellipsis style via _textpath_ellipsis_set and adjusts the
 * size of the text object to the path length. Otherwise, it disables the ellipsis style.
 *
 * @param pd Private data of the textpath widget.
 * @param obj The Efl_Ui_Textpath object.
 */
static void
_ellipsis_set(Efl_Ui_Textpath_Data *pd, Eo *obj)
{
   if (!pd->text_obj) return;

   Evas_Coord w = 0, h = 0;
   Eina_Bool is_ellipsis = EINA_FALSE;
   const Evas_Object *tb;

   if (elm_widget_is_legacy(obj))
     tb = edje_object_part_object_get(pd->text_obj, "elm.text");
   else
     tb = edje_object_part_object_get(pd->text_obj, "efl.text");

   evas_object_textblock_size_native_get(tb, &w, &h);
   efl_gfx_hint_size_restricted_min_set(pd->text_obj, EINA_SIZE2D(w, h));
   if (pd->ellipsis)
     {
        if (w > pd->total_length)
          {
             is_ellipsis = EINA_TRUE;
             w = pd->total_length;
          }
     }
   efl_gfx_entity_size_set(pd->text_obj, EINA_SIZE2D(w,  h));
   _textpath_ellipsis_set(obj, pd, is_ellipsis);
}

/**
 * @brief Called when the graphics path of the textpath object is committed (updated).
 *
 * This function re-parses the path data using _path_data_get and triggers
 * a sizing evaluation/redraw.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @implements Efl_Gfx_Path_Commit
 */
static void
_efl_ui_textpath_efl_gfx_path_commit(Eo *obj, Efl_Ui_Textpath_Data *pd)
{
   _path_data_get(obj, pd);
   _sizing_eval(pd);
}

/**
 * @brief Adjusts the start angle of a circular path to center the text.
 *
 * For circular paths with `EFL_UI_TEXTPATH_DIRECTION_CW_CENTER` or
 * `EFL_UI_TEXTPATH_DIRECTION_CCW_CENTER`, this function calculates the
 * angular extent of the text on the circle and adjusts the path's start
 * angle so that the text appears centered around the original start_angle.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 */
static void
_path_start_angle_adjust(Eo *obj, Efl_Ui_Textpath_Data *pd)
{
   Eina_Rect r;
   Efl_Ui_Textpath_Segment *seg;
   Eina_Vector2 first = { 0, 0 };
   Eina_Vector2 last = { 0, 0 };
   int remained_w, len;
   double rad, t, offset_angle;

   if (pd->direction != EFL_UI_TEXTPATH_DIRECTION_CW_CENTER  &&
       pd->direction != EFL_UI_TEXTPATH_DIRECTION_CCW_CENTER)
     return;

   r = efl_gfx_entity_geometry_get(pd->text_obj);
   remained_w = r.w;

   EINA_INLIST_FOREACH(pd->segments, seg)
     {
        if (remained_w <= 0)
          break;

        len = seg->length;
        if (remained_w < len)
          {
             t = remained_w / (double)len;
             eina_bezier_point_at(&seg->bezier, t, &last.x, &last.y);
          }

        if (remained_w == r.w)
          eina_bezier_point_at(&seg->bezier, 0, &first.x, &first.y);

        remained_w -= len;
     }

   first.x -= (pd->circle.x + r.x);
   first.y -= (pd->circle.y + r.y);
   last.x -= (pd->circle.x + r.x);
   last.y -= (pd->circle.y + r.y);

   eina_vector2_normalize(&first, &first);
   eina_vector2_normalize(&last, &last);
   rad = acos(eina_vector2_dot_product(&first, &last));
   if (EINA_DBL_EQ(rad, 0)) return;

   offset_angle = _rad_to_deg(rad);
   if (r.w > pd->total_length / 2)
     offset_angle = 360 - offset_angle;
   offset_angle /= 2.0;

   efl_gfx_path_reset(obj);

   if (pd->direction == EFL_UI_TEXTPATH_DIRECTION_CW_CENTER)
     {
        efl_gfx_path_append_arc(obj,
                                pd->circle.x - pd->circle.radius,
                                pd->circle.y - pd->circle.radius,
                                pd->circle.radius * 2,
                                pd->circle.radius * 2,
                                pd->circle.start_angle + offset_angle,
                                -360);
     }
   else
     {
        efl_gfx_path_append_arc(obj,
                                pd->circle.x - pd->circle.radius,
                                pd->circle.y - pd->circle.radius,
                                pd->circle.radius * 2,
                                pd->circle.radius * 2,
                                pd->circle.start_angle - offset_angle,
                                360);
     }
   _path_data_get(obj, pd);
}

/**
 * @brief Internal function to set the text on a specified part of the Edje object.
 *
 * This function updates the text in the Edje object, handles ellipsis configuration,
 * adjusts the start angle for circular paths if necessary, and triggers a sizing evaluation.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @param part The Edje part name where the text should be set (e.g., "efl.text").
 * @param text The text string to set. If NULL, it's treated as an empty string.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_textpath_text_set_internal(Eo *obj, Efl_Ui_Textpath_Data *pd, const char *part, const char *text)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);
   Eina_Bool ret = EINA_TRUE;

   if (!text) text = "";
   ret = edje_object_part_text_set(pd->text_obj, part, text);
   _ellipsis_set(pd, obj);

   //Only if circlular textpath
   if (pd->circle.radius > 0)
     _path_start_angle_adjust(obj, pd);

   _sizing_eval(pd);

   return ret;
}

/**
 * @brief Implements Efl.Canvas.Group.group_calculate.
 *
 * Clears the recalculate flag and triggers a sizing evaluation to update the layout.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 */
EOLIAN static void
_efl_ui_textpath_efl_canvas_group_group_calculate(Eo *obj, Efl_Ui_Textpath_Data *pd)
{
   efl_canvas_group_need_recalculate_set(obj, EINA_FALSE);
   _sizing_eval(pd);
}

/**
 * @brief Implements Efl.Canvas.Group.group_add.
 *
 * Initializes the textpath widget by creating and configuring the internal
 * Edje object used for text rendering.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param priv Private data of the textpath widget.
 */
EOLIAN static void
_efl_ui_textpath_efl_canvas_group_group_add(Eo *obj, Efl_Ui_Textpath_Data *priv)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   priv->text_obj = edje_object_add(evas_object_evas_get(obj));
   elm_widget_theme_object_set(obj, priv->text_obj, "textpath", "base",
                               elm_widget_style_get(obj));
   efl_gfx_hint_weight_set(priv->text_obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   efl_gfx_hint_align_set(priv->text_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
   efl_gfx_entity_visible_set(priv->text_obj, EINA_TRUE);

   evas_object_smart_member_add(priv->text_obj, obj);
   elm_widget_sub_object_add(obj, priv->text_obj);
}

/**
 * @brief Implements Efl.Object.constructor.
 *
 * Initializes the textpath object, sets default values (slice_no, direction),
 * and registers the pre-render callback.
 *
 * @param obj The Efl_Ui_Textpath object being constructed.
 * @param pd Private data of the textpath widget.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_ui_textpath_efl_object_constructor(Eo *obj, Efl_Ui_Textpath_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   pd->slice_no = SLICE_DEFAULT_NO;
   pd->direction = EFL_UI_TEXTPATH_DIRECTION_CW;

   evas_event_callback_add(evas_object_evas_get(obj), EVAS_CALLBACK_RENDER_PRE, _render_pre_cb, pd);

   return obj;
}

/**
 * @brief Implements Efl.Object.destructor.
 *
 * Cleans up resources used by the textpath object, including deleting
 * the pre-render callback, freeing allocated memory for text, user style,
 * path segments, and the internal Edje object.
 *
 * @param obj The Efl_Ui_Textpath object being destructed.
 * @param pd Private data of the textpath widget.
 */
EOLIAN static void
_efl_ui_textpath_efl_object_destructor(Eo *obj, Efl_Ui_Textpath_Data *pd)
{
   evas_event_callback_del_full(evas_object_evas_get(obj), EVAS_CALLBACK_RENDER_PRE, _render_pre_cb, pd);

   Efl_Ui_Textpath_Segment *seg;

   if (pd->text) free(pd->text);
   if (pd->text_obj) evas_object_del(pd->text_obj);
   if (pd->user_style) eina_strbuf_free(pd->user_style);
   EINA_INLIST_FREE(pd->segments, seg)
     {
        pd->segments = eina_inlist_remove(pd->segments, EINA_INLIST_GET(seg));
        free(seg);
     }

#ifdef EFL_UI_TEXTPATH_LINE_DEBUG
   Evas_Object *line;
   EINA_LIST_FREE(pd->lines, line)
      evas_object_del(line);
#endif

   efl_gfx_path_reset(obj);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Implements Elm.Layout.text_set. (Legacy part text setting)
 *
 * Sets the text for a given part.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @param part The name of the part to set text for.
 * @param text The text to set.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_textpath_text_set(Eo *obj, Efl_Ui_Textpath_Data *pd, const char *part, const char *text)
{
   return _textpath_text_set_internal(obj, pd, part, text);
}

/**
 * @brief Implements Elm.Layout.text_get. (Legacy part text retrieval)
 *
 * Gets the text from a given part.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @param part The name of the part to get text from.
 * @return The text of the part, or NULL on failure.
 */
EOLIAN static const char *
_efl_ui_textpath_text_get(Eo *obj EINA_UNUSED, Efl_Ui_Textpath_Data *pd, const char *part)
{
   return edje_object_part_text_get(pd->text_obj, part);
}

/**
 * @brief Implements Efl.Text.text_set.
 *
 * Sets the main text content of the textpath. This typically corresponds to the "efl.text" part.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @param text The text to set.
 */
EOLIAN static void
_efl_ui_textpath_efl_text_text_set(Eo *obj, Efl_Ui_Textpath_Data *pd, const char *text)
{
   _textpath_text_set_internal(obj, pd, "efl.text", text);
}

/**
 * @brief Implements Efl.Text.text_get.
 *
 * Gets the main text content of the textpath. This typically corresponds to the "efl.text" part.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @return The main text content.
 */
EOLIAN static const char *
_efl_ui_textpath_efl_text_text_get(const Eo *obj EINA_UNUSED, Efl_Ui_Textpath_Data *pd)
{
   return edje_object_part_text_get(pd->text_obj, "efl.text");
}

/**
 * @brief Implements Efl.Ui.Widget.theme_apply.
 *
 * Applies the current theme to the textpath widget and its internal Edje object.
 * Also re-evaluates ellipsis settings as theme changes might affect text rendering.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @return Eina_Error indicating success or failure.
 */
EOLIAN static Eina_Error
_efl_ui_textpath_efl_ui_widget_theme_apply(Eo *obj, Efl_Ui_Textpath_Data *pd)
{
   Eina_Error ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;

   ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return EFL_UI_THEME_APPLY_ERROR_GENERIC;

   elm_widget_theme_object_set(obj, pd->text_obj, "textpath", "base",
                               elm_widget_style_get(obj));
   _ellipsis_set(pd, obj);

   return ret;
}

/**
 * @brief Implements Efl.Gfx.Entity.position_set.
 *
 * Sets the position of the textpath object. This also adjusts the coordinates
 * of all internal path segments to reflect the new position and triggers a redraw.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @param pos The new position (x, y) for the object.
 */
EOLIAN static void
_efl_ui_textpath_efl_gfx_entity_position_set(Eo *obj, Efl_Ui_Textpath_Data *pd, Eina_Position2D pos)
{
   Eina_Position2D ppos, diff;
   Efl_Ui_Textpath_Segment *seg;
   double sx, sy, csx, csy, cex, cey, ex, ey;

   ppos = efl_gfx_entity_position_get(obj);
   efl_gfx_entity_position_set(efl_super(obj, MY_CLASS), pos);

   if (ppos.x == pos.x && ppos.y == pos.y) return;

   diff.x = pos.x - ppos.x;
   diff.y = pos.y - ppos.y;

   EINA_INLIST_FOREACH(pd->segments, seg)
     {
        eina_bezier_values_get(&seg->bezier, &sx, &sy, &csx, &csy,
                               &cex, &cey, &ex, &ey);
        sx += diff.x;
        sy += diff.y;
        csx += diff.x;
        csy += diff.y;
        cex += diff.x;
        cey += diff.y;
        ex += diff.x;
        ey += diff.y;

        eina_bezier_values_set(&seg->bezier, sx, sy, csx, csy,
                               cex, cey, ex, ey);
     }

   _text_draw(pd);
}

/**
 * @brief Implements Efl.Gfx.Entity.size_set.
 *
 * Sets the size of the textpath object. If the path is not circular (or legacy circle_set is not used),
 * this function adjusts the coordinates of all internal path segments to scale them
 * relative to the center of the object and triggers a redraw.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @param size The new size (width, height) for the object.
 */
EOLIAN static void
_efl_ui_textpath_efl_gfx_entity_size_set(Eo *obj, Efl_Ui_Textpath_Data *pd EINA_UNUSED, Eina_Size2D size)
{
   Eina_Size2D psize, diff;
   Efl_Ui_Textpath_Segment *seg;
   double sx, sy, csx, csy, cex, cey, ex, ey;

   psize = efl_gfx_entity_size_get(obj);
   efl_gfx_entity_size_set(efl_super(obj, MY_CLASS), size);

   if (psize.w == size.w && psize.h == size.h) return;

   //TODO: Remove this condition if circle_set() is removed
   if (pd->circle.radius > 0 && !pd->circular) return;

   diff.w = (size.w - psize.w) * 0.5;
   diff.h = (size.h - psize.h) * 0.5;

   EINA_INLIST_FOREACH(pd->segments, seg)
     {
        eina_bezier_values_get(&seg->bezier, &sx, &sy, &csx, &csy,
                               &cex, &cey, &ex, &ey);
        sx += diff.w;
        sy += diff.h;
        csx += diff.w;
        csy += diff.h;
        cex += diff.w;
        cey += diff.h;
        ex += diff.w;
        ey += diff.h;

        eina_bezier_values_set(&seg->bezier, sx, sy, csx, csy,
                               cex, cey, ex, ey);
     }

   _text_draw(pd);
}

/**
 * @brief Sets the textpath to follow a circular trajectory.
 *
 * This function defines a circular path for the text. The circle's center is
 * automatically calculated to be within the object's bounds, considering the text height.
 * The path is then generated as an arc.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @param radius The radius of the circular path.
 * @param start_angle The starting angle on the circle for the text, in degrees.
 * @param direction The direction of the text along the circle (clockwise or counter-clockwise).
 *                  Example: EFL_UI_TEXTPATH_DIRECTION_CW
 */
EOLIAN static void
_efl_ui_textpath_circular_set(Eo *obj, Efl_Ui_Textpath_Data *pd, double radius, double start_angle, Efl_Ui_Textpath_Direction direction)
{
   Eina_Size2D text_size;
   double sweep_length, x, y;

   if (EINA_DBL_EQ(pd->circle.radius, radius) &&
       EINA_DBL_EQ(pd->circle.start_angle, start_angle) &&
       pd->direction == direction &&
       _map_point_calc(pd) > 0)
        return;

   Eina_Size2D obj_size = efl_gfx_entity_size_get(obj);

   //textpath min size is same to circle bounadary */
   text_size = efl_gfx_entity_size_get(pd->text_obj);

   x = (obj_size.w - text_size.h - (2 * radius)) * 0.5;
   y = (obj_size.h - text_size.h - (2 * radius)) * 0.5;

   /* User leaves center position to textpath itself.
      Now textpath automatically updates circle text according to
      object position. */
   pd->circle.x = radius + x;
   pd->circle.y = radius + y;
   pd->circle.radius = radius;
   pd->circle.start_angle = start_angle;
   pd->direction = direction;
   pd->circular = EINA_TRUE;

   efl_gfx_path_reset(obj);

   if (direction == EFL_UI_TEXTPATH_DIRECTION_CW ||
       direction == EFL_UI_TEXTPATH_DIRECTION_CW_CENTER)
     sweep_length = 360;
   else
     sweep_length = -360;

   efl_gfx_path_append_arc(obj,
                           pd->circle.x - pd->circle.radius,
                           pd->circle.y - pd->circle.radius,
                           radius * 2,
                           radius * 2,  start_angle, sweep_length);

   _path_data_get(obj, pd);
   _path_start_angle_adjust(obj, pd);
   _sizing_eval(pd);

   efl_gfx_hint_size_restricted_min_set(obj, EINA_SIZE2D((radius * 2) + text_size.h, (radius * 2) + text_size.h));
}

/**
 * @brief Gets the number of slices used for rendering curves.
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @return The number of slices.
 */
EOLIAN static int
_efl_ui_textpath_slice_number_get(const Eo *obj EINA_UNUSED, Efl_Ui_Textpath_Data *pd)
{
   return pd->slice_no;
}

/**
 * @brief Sets the number of slices used for rendering curves.
 *
 * A higher number results in smoother curves but may impact performance.
 * Default is SLICE_DEFAULT_NO (99).
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @param slice_no The number of slices. Example: 50
 */
EOLIAN static void
_efl_ui_textpath_slice_number_set(Eo *obj EINA_UNUSED, Efl_Ui_Textpath_Data *pd, int slice_no)
{
   if (pd->slice_no == slice_no) return;
   pd->slice_no = slice_no;
   _sizing_eval(pd);
}

/**
 * @brief Enables or disables ellipsis for overflowing text.
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @param ellipsis EINA_TRUE to enable ellipsis, EINA_FALSE to disable.
 */
EOLIAN static void
_efl_ui_textpath_ellipsis_set(Eo *obj, Efl_Ui_Textpath_Data *pd, Eina_Bool ellipsis)
{
   if (pd->ellipsis == ellipsis) return;
   pd->ellipsis = ellipsis;

   _ellipsis_set(pd, obj);
   _sizing_eval(pd);
}

/**
 * @brief Gets whether ellipsis is enabled for overflowing text.
 * @param obj The Efl_Ui_Textpath object.
 * @param pd Private data of the textpath widget.
 * @return EINA_TRUE if ellipsis is enabled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_textpath_ellipsis_get(const Eo *obj EINA_UNUSED, Efl_Ui_Textpath_Data *pd)
{
   return pd->ellipsis;
}

/* Efl.Part begin */

/**
 * @brief Checks if a given part name is specific to Efl_Ui_Textpath.
 *
 * This is used by the ELM_PART_OVERRIDE_PARTIAL macro to determine if
 * the textpath should handle this part or defer to its parent class.
 * Textpath handles its own text parts but uses Efl.Ui.Widget's "background"
 * and "shadow" parts.
 *
 * @param obj The Efl_Ui_Textpath object (unused).
 * @param part The name of the part to check.
 * @return EINA_TRUE if it's a textpath-specific part, EINA_FALSE otherwise.
 */
static Eina_Bool
_part_is_efl_ui_textpath_part(const Eo *obj EINA_UNUSED, const char *part)
{
   //Use Efl.Ui.Widget's "background" and "shadow" parts
   if (eina_streq(part, "background") || eina_streq(part, "shadow"))
     return EINA_FALSE;

   return EINA_TRUE;
}

ELM_PART_OVERRIDE_PARTIAL(efl_ui_textpath, EFL_UI_TEXTPATH, Efl_Ui_Textpath_Data, _part_is_efl_ui_textpath_part)
ELM_PART_OVERRIDE_TEXT_SET(efl_ui_textpath, EFL_UI_TEXTPATH, Efl_Ui_Textpath_Data)
ELM_PART_OVERRIDE_TEXT_GET(efl_ui_textpath, EFL_UI_TEXTPATH, Efl_Ui_Textpath_Data)
#include "efl_ui_textpath_part.eo.c"
/* Efl.Part end */

#define EFL_UI_TEXTPATH_EXTRA_OPS \
      EFL_CANVAS_GROUP_ADD_OPS(efl_ui_textpath)

#include "efl_ui_textpath.eo.c"
#include "efl_ui_textpath_eo.legacy.c"

#include "efl_ui_textpath_legacy_eo.h"

#define MY_CLASS_NAME_LEGACY "elm_textpath"
/* Legacy APIs */

/**
 * @brief Legacy class constructor for elm_textpath.
 * Registers the legacy type name.
 * @param klass The Efl_Class.
 */
static void
_efl_ui_textpath_legacy_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @brief Legacy object constructor for elm_textpath.
 * Sets the Evas object type.
 * @param obj The Eo object.
 * @param _pd Private data (unused).
 * @return The constructed Eo object.
 */
EOLIAN static Eo *
_efl_ui_textpath_legacy_efl_object_constructor(Eo *obj, void *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, EFL_UI_TEXTPATH_LEGACY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   return obj;
}

/**
 * @brief Adds a new textpath widget to the given parent Evas object.
 * @deprecated Use efl_add(EFL_UI_TEXTPATH_CLASS, parent) instead.
 * @param parent The parent Evas object.
 * @return The new Evas_Object, or NULL on failure.
 */
EAPI Evas_Object *
elm_textpath_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(EFL_UI_TEXTPATH_LEGACY_CLASS, parent);
}

/**
 * @brief Sets the textpath to follow a circular trajectory with a user-defined center.
 * @deprecated Use efl_ui_textpath_circular_set() instead.
 *
 * This function defines a circular path for the text with an explicitly set center (x, y).
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param x The x-coordinate of the circle's center.
 * @param y The y-coordinate of the circle's center.
 * @param radius The radius of the circular path.
 * @param start_angle The starting angle on the circle for the text, in degrees.
 * @param direction The direction of the text along the circle.
 *                  Example: EFL_UI_TEXTPATH_DIRECTION_CW
 */
EAPI void
elm_textpath_circle_set(Eo *obj, double x, double y, double radius, double start_angle, Efl_Ui_Textpath_Direction direction)
{
   double sweep_length;

   EFL_UI_TEXTPATH_DATA_GET(obj, pd);
   if (!pd) return;

   if (EINA_DBL_EQ(pd->circle.x, x) && EINA_DBL_EQ(pd->circle.y, y) &&
       EINA_DBL_EQ(pd->circle.radius, radius) &&
       EINA_DBL_EQ(pd->circle.start_angle, start_angle) &&
       pd->direction == direction &&
       _map_point_calc(pd) > 0)
        return;

   pd->circle.x = x;
   pd->circle.y = y;
   pd->circle.radius = radius;
   pd->circle.start_angle = start_angle;
   pd->direction = direction;
   pd->circular = EINA_FALSE;

   efl_gfx_path_reset(obj);

   if (direction == EFL_UI_TEXTPATH_DIRECTION_CW ||
       direction == EFL_UI_TEXTPATH_DIRECTION_CW_CENTER)
     sweep_length = - 360;
   else
     sweep_length = 360;

   efl_gfx_path_append_arc(obj, x - radius, y - radius, radius * 2,
                           radius * 2,  start_angle, sweep_length);

   _path_data_get(obj, pd);
   _path_start_angle_adjust(obj, pd);
   _sizing_eval(pd);

   efl_gfx_hint_size_restricted_min_set(obj, EINA_SIZE2D(x * 2, y * 2));
}

/**
 * @brief Sets a custom style for the text.
 * @deprecated Use Edje direct text styling capabilities or other Efl_Text properties.
 *
 * This function allows applying a custom Edje text style string to the text.
 *
 * @param obj The Efl_Ui_Textpath object.
 * @param style The Edje style string. Example: "DEFAULT='font=Sans font_size=20 color=#FF0000'"
 *              Pass NULL to clear the user style.
 */
EAPI void
elm_textpath_text_user_style_set(Eo *obj, const char *style)
{
   EFL_UI_TEXTPATH_DATA_GET(obj, pd);
   if (!pd) return;

   char *text_part;
   if (elm_widget_is_legacy(obj))
     text_part = LEGACY_TEXT_PART_NAME;
   else
     text_part = EFL_UI_TEXT_PART_NAME;

   if (pd->user_style)
     {
        edje_object_part_text_style_user_pop(pd->text_obj, text_part);
        eina_strbuf_free(pd->user_style);
        pd->user_style = NULL;
     }

   if (style)
     {
        pd->user_style = eina_strbuf_new();
        eina_strbuf_append(pd->user_style, style);

        edje_object_part_text_style_user_pop(pd->text_obj, text_part);
        edje_object_part_text_style_user_push(pd->text_obj, text_part, eina_strbuf_string_get(pd->user_style));
     }

   _ellipsis_set(pd, obj);
   _sizing_eval(pd);
}

#include "efl_ui_textpath_legacy_eo.c"

