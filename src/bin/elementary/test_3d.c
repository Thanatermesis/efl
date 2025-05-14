#include "test.h"
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Represents a vertex in 3D space with texture coordinates.
 *
 * Contains 3D coordinates (x, y, z) and 2D texture coordinates (u, v) for a point.
 */
typedef struct _Point
{
   Evas_Coord x, y, z, u, v;
} Point;

/**
 * @brief Represents a single face of a 3D shape (e.g., a cube side).
 *
 * It consists of an Evas_Object to render the face and an array of 4 points
 * that define its vertices.
 */
typedef struct _Side
{
   Evas_Object *o; /**< The Evas object used to draw the side. */
   Point pt[4];    /**< An array of 4 vertices defining the corners of the side. */
} Side;

/**
 * @brief Represents a cube composed of 6 sides.
 */
typedef struct _Cube
{
   Side side[6]; /**< An array of 6 sides, one for each face of the cube. */
} Cube;

static Cube *cube;
static double rotx = 0.0, roty = 0.0, rotz = 0.0;
static double cxo = 0.0, cyo = 0.0, focv = 256.0, z0v = 0.0;
#define POINT(n, p, xx, yy, zz, uu, vv) \
   c->side[n].pt[p].x = xx; \
   c->side[n].pt[p].y = yy; \
   c->side[n].pt[p].z = zz; \
   c->side[n].pt[p].u = uu; \
   c->side[n].pt[p].v = vv

/**
 * @brief Creates and initializes a new Cube object.
 *
 * This function allocates memory for a Cube, creates six Evas_Object images
 * for the cube faces, and sets their initial properties and vertex coordinates.
 * Each side is a square plane defined by four vertices.
 *
 * @param evas The Evas canvas on which to create the cube's objects.
 * @param w The width of the cube.
 * @param h The height of the cube.
 * @param d The depth of the cube.
 * @return A pointer to the newly created Cube object.
 */
static Cube *
_cube_new(Evas *evas, Evas_Coord w, Evas_Coord h, Evas_Coord d)
{
   Cube *c;
   int i;

   w -= (w / 2);
   h -= (h / 2);
   d -= (d / 2);
   c = calloc(1, sizeof(Cube));
   for (i = 0; i < 6; i++)
     {
        Evas_Object *o;
        char buf[PATH_MAX];
        o = evas_object_image_filled_add(evas);
        c->side[i].o = o;
        snprintf(buf, sizeof(buf), "%s/images/%s",
                 elm_app_data_dir_get(), "twofish.jpg");
        evas_object_image_file_set(o, buf, NULL);
        evas_object_resize(o, 256, 256);
        evas_object_pass_events_set(o, EINA_TRUE);
        evas_object_color_set(o, 235, 235, 235, 235);
        evas_object_show(o);
     }
   /*
    * Define the 6 faces of the cube. Each face has 4 vertices.
    * The vertices are defined in what should be a counter-clockwise order when
    * viewed from outside the cube for back-face culling to work correctly.
    * The texture coordinates (u,v) map a 256x256 image to each face.
    *
    * Face 0: Front face
    * Face 1: Right face
    * Face 2: Back face
    * Face 3: Left face
    * Face 4: Bottom face
    * Face 5: Top face
    */
   POINT(0, 0, -w, -h, -d,   0,   0);
   POINT(0, 1,  w, -h, -d, 256,   0);
   POINT(0, 2,  w,  h, -d, 256, 256);
   POINT(0, 3, -w,  h, -d,   0, 256);

   POINT(1, 0,  w, -h, -d,   0,   0);
   POINT(1, 1,  w, -h,  d, 256,   0);
   POINT(1, 2,  w,  h,  d, 256, 256);
   POINT(1, 3,  w,  h, -d,   0, 256);

   POINT(2, 0,  w, -h,  d,   0,   0);
   POINT(2, 1, -w, -h,  d, 256,   0);
   POINT(2, 2, -w,  h,  d, 256, 256);
   POINT(2, 3,  w,  h,  d,   0, 256);

   POINT(3, 0, -w, -h,  d,   0,   0);
   POINT(3, 1, -w, -h, -d, 256,   0);
   POINT(3, 2, -w,  h, -d, 256, 256);
   POINT(3, 3, -w,  h,  d,   0, 256);

   POINT(4, 0, -w, -h,  d,   0,   0);
   POINT(4, 1,  w, -h,  d, 256,   0);
   POINT(4, 2,  w, -h, -d, 256, 256);
   POINT(4, 3, -w, -h, -d,   0, 256);

   POINT(5, 0, -w,  h, -d,   0,   0);
   POINT(5, 1,  w,  h, -d, 256,   0);
   POINT(5, 2,  w,  h,  d, 256, 256);
   POINT(5, 3, -w,  h,  d,   0, 256);

   return c;
}

/**
 * @brief Positions and transforms the cube in 3D space.
 *
 * This function applies 3D rotation, lighting, and perspective projection
 * to each side of the cube. It uses Evas_Map to perform these transformations.
 * It also handles back-face culling by hiding sides that are not facing
 * the camera and sorts the visible sides by depth to ensure correct rendering order.
 *
 * @param c The cube to position.
 * @param x The x-coordinate of the cube's center in canvas coordinates.
 * @param y The y-coordinate of the cube's center in canvas coordinates.
 * @param z The z-coordinate of the cube's center in canvas coordinates.
 * @param dx Rotation angle around the x-axis, in degrees.
 * @param dy Rotation angle around the y-axis, in degrees.
 * @param dz Rotation angle around the z-axis, in degrees.
 * @param cx The x-coordinate of the perspective vanishing point on the screen.
 * @param cy The y-coordinate of the perspective vanishing point on the screen.
 * @param z0 The z-coordinate of the "eye" or camera position relative to the screen plane.
 * @param foc The focal length, affecting the strength of the perspective effect.
 */
static void
_cube_pos(Cube *c,
          Evas_Coord x, Evas_Coord y, Evas_Coord z,
          double dx, double dy, double dz,
          Evas_Coord cx, Evas_Coord cy, Evas_Coord z0, Evas_Coord foc)
{
   Evas_Map *m;
   int i, j, order[6], sorted;
   Evas_Coord mz[6];

   m = evas_map_new(4);

   for (i = 0; i < 6; i++)
     {
        Evas_Coord tz[4];

        for (j = 0; j < 4; j++)
          {
             evas_map_point_coord_set(m, j,
                                      c->side[i].pt[j].x + x,
                                      c->side[i].pt[j].y + y,
                                      c->side[i].pt[j].z + z);
             evas_map_point_image_uv_set(m, j,
                                         c->side[i].pt[j].u,
                                         c->side[i].pt[j].v);
             evas_map_point_color_set(m, j, 255, 255, 255, 255);
          }
        evas_map_util_3d_rotate(m, dx, dy, dz, x, y, z);
        evas_map_util_3d_lighting(m, -1000, -1000, -1000,
                                  255, 255, 255,
                                  20, 20, 20);
        evas_map_util_3d_perspective(m, cx, cy, z0, foc);
        if (evas_map_util_clockwise_get(m))
          {
             evas_object_map_enable_set(c->side[i].o, EINA_TRUE);
             evas_object_map_set(c->side[i].o, m);
             evas_object_show(c->side[i].o);
          }
        else
           evas_object_hide(c->side[i].o);

        order[i] = i;
        for (j = 0; j < 4; j++)
           evas_map_point_coord_get(m, j, NULL, NULL, &(tz[j]));
        mz[i] = (tz[0] + tz[1] + tz[2] + tz[3]) / 4;
     }
   do
     {
        sorted = 1;
        for (i = 0; i < 5; i++)
          {
             if (mz[order[i]] > mz[order[i + 1]])
               {
                  j = order[i];
                  order[i] = order[i + 1];
                  order[i + 1] = j;
                  sorted = 0;
               }
          }
     }
   while (!sorted);

   evas_object_raise(c->side[order[0]].o);
   for (i = 1; i < 6; i++)
      evas_object_stack_below(c->side[order[i]].o, c->side[order[i - 1]].o);
   evas_map_free(m);
}

/**
 * @brief Updates the cube's position and transformation.
 *
 * This function retrieves the window dimensions and calls _cube_pos()
 * with the current global rotation and perspective settings. It is a convenience
 * wrapper to redraw the cube when any parameter changes.
 *
 * @param win The window object containing the cube.
 * @param c The cube to update.
 */
static void
_cube_update(Evas_Object *win, Cube *c)
{
   Evas_Coord w, h;

   evas_object_geometry_get(win, NULL, NULL, &w, &h);
   _cube_pos(c,
             (w / 2), (h / 2), 512,
             rotx, roty, rotz,
             (w / 2) + cxo, (h / 2) + cyo, z0v, focv);
}

/**
 * @brief Callback for changes to the X rotation slider.
 *
 * Updates the global X rotation angle and triggers a cube redraw.
 *
 * @param data The user data (the window object).
 * @param obj The slider object that triggered the callback.
 * @param event_info Unused event information.
 */
void
_ch_rot_x(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   rotx = elm_slider_value_get(obj);
   _cube_update(win, cube);
}

/**
 * @brief Callback for changes to the Y rotation slider.
 *
 * Updates the global Y rotation angle and triggers a cube redraw.
 *
 * @param data The user data (the window object).
 * @param obj The slider object that triggered the callback.
 * @param event_info Unused event information.
 */
void
_ch_rot_y(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   roty = elm_slider_value_get(obj);
   _cube_update(win, cube);
}

/**
 * @brief Callback for changes to the Z rotation slider.
 *
 * Updates the global Z rotation angle and triggers a cube redraw.
 *
 * @param data The user data (the window object).
 * @param obj The slider object that triggered the callback.
 * @param event_info Unused event information.
 */
void
_ch_rot_z(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   rotz = elm_slider_value_get(obj);
   _cube_update(win, cube);
}

/**
 * @brief Callback for changes to the perspective center X offset slider.
 *
 * Updates the global perspective center X offset and triggers a cube redraw.
 *
 * @param data The user data (the window object).
 * @param obj The slider object that triggered the callback.
 * @param event_info Unused event information.
 */
void
_ch_cx(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   cxo = elm_slider_value_get(obj);
   _cube_update(win, cube);
}

/**
 * @brief Callback for changes to the perspective center Y offset slider.
 *
 * Updates the global perspective center Y offset and triggers a cube redraw.
 *
 * @param data The user data (the window object).
 * @param obj The slider object that triggered the callback.
 * @param event_info Unused event information.
 */
void
_ch_cy(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   cyo = elm_slider_value_get(obj);
   _cube_update(win, cube);
}

/**
 * @brief Callback for changes to the focal length slider.
 *
 * Updates the global focal length value and triggers a cube redraw.
 *
 * @param data The user data (the window object).
 * @param obj The slider object that triggered the callback.
 * @param event_info Unused event information.
 */
void
_ch_foc(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   focv = elm_slider_value_get(obj);
   _cube_update(win, cube);
}

/**
 * @brief Callback for changes to the Z0 (camera Z position) slider.
 *
 * Updates the global Z0 value and triggers a cube redraw.
 *
 * @param data The user data (the window object).
 * @param obj The slider object that triggered the callback.
 * @param event_info Unused event information.
 */
void
_ch_z0(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   z0v = elm_slider_value_get(obj);
   _cube_update(win, cube);
}

/**
 * @brief Main function for the 3D cube test.
 *
 * This function sets up the main window, creates the cube, and adds
 * sliders to control the cube's rotation and perspective. It initializes
 * the UI and shows the window.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_3d(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *sl;

   win = elm_win_util_standard_add("evas-map-3d", "Evas Map 3D");
   elm_win_autodel_set(win, EINA_TRUE);

   cube = _cube_new(evas_object_evas_get(win), 240, 240, 240);

   bx = elm_box_add(win);
   evas_object_layer_set(bx, 10);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   sl = elm_slider_add(win);
   elm_object_text_set(sl, "Rot X");
   elm_slider_unit_format_set(sl, "%1.0f units");
   elm_slider_indicator_format_set(sl, "%1.0f units");
   elm_slider_span_size_set(sl, 360);
   elm_slider_min_max_set(sl, 0, 360);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_smart_callback_add(sl, "changed", _ch_rot_x, win);
   evas_object_show(sl);

   sl = elm_slider_add(win);
   elm_object_text_set(sl, "Rot Y");
   elm_slider_unit_format_set(sl, "%1.0f units");
   elm_slider_indicator_format_set(sl, "%1.0f units");
   elm_slider_span_size_set(sl, 360);
   elm_slider_min_max_set(sl, 0, 360);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_smart_callback_add(sl, "changed", _ch_rot_y, win);
   evas_object_show(sl);

   sl = elm_slider_add(win);
   elm_object_text_set(sl, "Rot Z");
   elm_slider_unit_format_set(sl, "%1.0f units");
   elm_slider_indicator_format_set(sl, "%1.0f units");
   elm_slider_span_size_set(sl, 360);
   elm_slider_min_max_set(sl, 0, 360);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_smart_callback_add(sl, "changed", _ch_rot_z, win);
   evas_object_show(sl);

   sl = elm_slider_add(win);
   elm_object_text_set(sl, "PX Off");
   elm_slider_unit_format_set(sl, "%1.0f units");
   elm_slider_indicator_format_set(sl, "%1.0f units");
   elm_slider_span_size_set(sl, 360);
   elm_slider_min_max_set(sl, -320, 320);
   elm_slider_value_set(sl, cxo);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_smart_callback_add(sl, "changed", _ch_cx, win);
   evas_object_show(sl);

   sl = elm_slider_add(win);
   elm_object_text_set(sl, "PY Off");
   elm_slider_unit_format_set(sl, "%1.0f units");
   elm_slider_indicator_format_set(sl, "%1.0f units");
   elm_slider_span_size_set(sl, 360);
   elm_slider_min_max_set(sl, -320, 320);
   elm_slider_value_set(sl, cyo);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_smart_callback_add(sl, "changed", _ch_cy, win);
   evas_object_show(sl);

   sl = elm_slider_add(win);
   elm_object_text_set(sl, "Foc");
   elm_slider_unit_format_set(sl, "%1.0f units");
   elm_slider_indicator_format_set(sl, "%1.0f units");
   elm_slider_span_size_set(sl, 360);
   elm_slider_min_max_set(sl, 1, 2000);
   elm_slider_value_set(sl, focv);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_smart_callback_add(sl, "changed", _ch_foc, win);
   evas_object_show(sl);

   sl = elm_slider_add(win);
   elm_object_text_set(sl, "Z0");
   elm_slider_unit_format_set(sl, "%1.0f units");
   elm_slider_indicator_format_set(sl, "%1.0f units");
   elm_slider_span_size_set(sl, 360);
   elm_slider_min_max_set(sl, -2000, 2000);
   elm_slider_value_set(sl, z0v);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_smart_callback_add(sl, "changed", _ch_z0, win);
   evas_object_show(sl);

   evas_object_resize(win, 480 * elm_config_scale_get(),
                           480 * elm_config_scale_get());
   _cube_update(win, cube);
   evas_object_show(win);
}
