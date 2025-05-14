#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

typedef struct _State State;
typedef struct _Slice Slice;

typedef struct _Vertex2 Vertex2;
typedef struct _Vertex3 Vertex3;

/**
 * @brief Holds the overall state of the page flip animation.
 */
struct _State
{
   Evas_Object *front, *back; /**< The front and back page Evas objects. */
   Evas_Coord down_x, down_y, x, y; /**< Mouse coordinates for flip calculation. */
   Eina_Bool down : 1; /**< EINA_TRUE if the mouse is down. */
   Eina_Bool backflip : 1; /**< EINA_TRUE to flip the back page texture. */

   Ecore_Animator *anim; /**< Animator for the flip/unflip animation. */
   Ecore_Job *job; /**< Job to coalesce mouse move events. */
   Evas_Coord ox, oy, w, h; /**< Geometry of the page. */
   int slices_w, slices_h; /**< Number of slices in width and height. */
   Slice **slices, **slices2; /**< 2D arrays of slices for front and back pages. */
   int dir; /**< Flip direction: 0=left, 1=right, 2=up, 3=down. */
   int finish; /**< Flag to indicate if the animation should complete the flip. */
};

/**
 * @brief Represents a rectangular subdivision of the page for rendering the curl.
 * Each slice is a small piece of the original image that gets transformed in 3D.
 */
struct _Slice
{
   Evas_Object *obj; /**< The Evas image object for this slice. */
   /**
    * @brief Vertex ordering for coordinates:
    * @code
    * (0)---(1)
    *  |     |
    *  |     |
    * (3)---(2)
    * @endcode
    */
   double u[4], v[4], x[4], y[4], z[4]; /**< UV and XYZ coordinates for the 4 vertices. */
};

/**
 * @brief A simple 2D vertex.
 */
struct _Vertex2
{
   double x, y;
};

/**
 * @brief A simple 3D vertex.
 */
struct _Vertex3
{
   double x, y, z;
};

static State state =
{
   NULL, NULL,
   0, 0, 0, 0,
   0,
   0,

   NULL,
   NULL,
   0, 0, 0, 0,
   0, 0,
   NULL, NULL,
   -1,
   0
};

/**
 * @brief Creates and initializes a new slice object.
 *
 * A slice is a small rectangular part of the original Evas object that will be
 * individually transformed to create the page curl effect.
 *
 * @param st The application state.
 * @param obj The source Evas object to create the slice from.
 * @return A new Slice object, or NULL on failure.
 */
static Slice *
_slice_new(State *st EINA_UNUSED, Evas_Object *obj)
{
   Slice *sl;

   sl = calloc(1, sizeof(Slice));
   if (!sl) return NULL;
   sl->obj = evas_object_image_add(evas_object_evas_get(obj));
   evas_object_image_smooth_scale_set(sl->obj, EINA_FALSE);
   evas_object_pass_events_set(sl->obj, EINA_TRUE);
   evas_object_image_source_set(sl->obj, obj);
   return sl;
}

/**
 * @brief Frees the resources associated with a slice.
 * @param sl The slice to free.
 */
static void
_slice_free(Slice *sl)
{
   evas_object_del(sl->obj);
   free(sl);
}

/**
 * @brief Applies the calculated 3D transformation to a slice using an Evas_Map.
 *
 * This function takes the transformed vertex coordinates from the slice
 * structure and applies them to the slice's Evas object via an Evas_Map.
 * It handles the different orientations of the flip (left, right, up, down)
 * by re-ordering the vertices appropriately.
 *
 * @param st The application state, used to get flip direction.
 * @param sl The slice to apply the transformation to.
 * @param w Width of the page (used for right-side flips).
 * @param ox, oy, ow, oh The geometry of the source image on the canvas.
 */
static void
_slice_apply(State *st, Slice *sl,
             Evas_Coord x EINA_UNUSED, Evas_Coord y EINA_UNUSED, Evas_Coord w, Evas_Coord h EINA_UNUSED,
             Evas_Coord ox, Evas_Coord oy, Evas_Coord ow, Evas_Coord oh)
{
   Evas_Map *m;
   int i;

   m = evas_map_new(4);
   if (!m) return;
   evas_map_smooth_set(m, EINA_FALSE);
   for (i = 0; i < 4; i++)
     {
        evas_map_point_color_set(m, i, 255, 255, 255, 255);
        if (st->dir == 0)
          {
             int p[4] = { 0, 1, 2, 3 };
             evas_map_point_coord_set(m, i, ox + sl->x[p[i]], oy + sl->y[p[i]], sl->z[p[i]]);
             evas_map_point_image_uv_set(m, i, sl->u[p[i]] , sl->v[p[i]]);
          }
        else if (st->dir == 1)
          {
             int p[4] = { 1, 0, 3, 2 };
             evas_map_point_coord_set(m, i, ox + (w - sl->x[p[i]]), oy + sl->y[p[i]], sl->z[p[i]]);
             evas_map_point_image_uv_set(m, i, ow - sl->u[p[i]] , sl->v[p[i]]);
          }
        else if (st->dir == 2)
          {
             int p[4] = { 1, 0, 3, 2 };
             evas_map_point_coord_set(m, i, ox + sl->y[p[i]], oy + sl->x[p[i]], sl->z[p[i]]);
             evas_map_point_image_uv_set(m, i, sl->v[p[i]] , sl->u[p[i]]);
          }
        else if (st->dir == 3)
          {
             int p[4] = { 0, 1, 2, 3 };
             evas_map_point_coord_set(m, i, ox + sl->y[p[i]], oy + (w - sl->x[p[i]]), sl->z[p[i]]);
             evas_map_point_image_uv_set(m, i, sl->v[p[i]] , oh - sl->u[p[i]]);
          }
     }
   evas_object_map_enable_set(sl->obj, EINA_TRUE);
   evas_object_image_fill_set(sl->obj, 0, 0, ow, oh);
   evas_object_map_set(sl->obj, m);
   evas_map_free(m);
}

/**
 * @brief Applies a 3D perspective effect to a slice.
 *
 * It uses a vanishing point at the center of the page to give the illusion of
 * depth. It also determines if the slice is facing the camera and hides it
 * if it's facing away (backface culling).
 *
 * @param st The application state.
 * @param sl The slice to apply perspective to.
 * @param x, y, w, h The geometry of the page.
 */
static void
_slice_3d(State *st EINA_UNUSED, Slice *sl, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   Evas_Map *m = evas_map_dup(evas_object_map_get(sl->obj));
   int i;

   if (!m) return;
   // vanishing point is center of page, and focal dist is 1024
   evas_map_util_3d_perspective(m, x + (w / 2), y + (h / 2), 0, 1024);
   for (i = 0; i < 4; i++)
     {
        Evas_Coord xx, yy, zz;
        evas_map_point_coord_get(m, i, &xx, &yy, &zz);
        evas_map_point_coord_set(m, i, xx, yy, 0);
     }
   if (evas_map_util_clockwise_get(m)) evas_object_show(sl->obj);
   else evas_object_hide(sl->obj);
   evas_object_map_set(sl->obj, m);
   evas_map_free(m);
}

/**
 * @brief Applies a lighting effect to a slice.
 *
 * This simulates a light source to create shading on the curled page,
 * enhancing the 3D effect. The light is positioned above the center of the
 * page, towards the viewer.
 *
 * @param st The application state.
 * @param sl The slice to light.
 * @param x, y, w, h The geometry of the page.
 */
static void
_slice_light(State *st EINA_UNUSED, Slice *sl, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   Evas_Map *m = evas_map_dup(evas_object_map_get(sl->obj));
   int i;

   if (!m) return;
   evas_map_util_3d_lighting(m,
                             // light position
                             // (centered over page 10 * h toward camera)
                             x + (w / 2)  , y + (h / 2)  , -10000,
                             255, 255, 255, // light color
                             0 , 0 , 0); // ambient minimum
   // multiply brightness by 1.2 to make lightish bits all white so we dont
   // add shading where we could otherwise be pure white
   for (i = 0; i < 4; i++)
     {
        int r, g, b, a;

        evas_map_point_color_get(m, i, &r, &g, &b, &a);
        r = (double)r * 1.2; if (r > 255) r = 255;
        g = (double)g * 1.2; if (g > 255) g = 255;
        b = (double)b * 1.2; if (b > 255) b = 255;
        evas_map_point_color_set(m, i, r, g, b, a);
     }
   evas_object_map_set(sl->obj, m);
   evas_map_free(m);
}

/**
 * @brief Sets the 3D world coordinates of a slice's four vertices.
 *
 * @param st The application state.
 * @param sl The slice to modify.
 * @param xx1, yy1, zz1 Coordinates for vertex 0.
 * @param xx2, yy2, zz2 Coordinates for vertex 1.
 * @param xx3, yy3, zz3 Coordinates for vertex 2.
 * @param xx4, yy4, zz4 Coordinates for vertex 3.
 */
static void
_slice_xyz(State *st EINA_UNUSED, Slice *sl,
           double xx1, double yy1, double zz1,
           double xx2, double yy2, double zz2,
           double xx3, double yy3, double zz3,
           double xx4, double yy4, double zz4)
{
   sl->x[0] = xx1; sl->y[0] = yy1; sl->z[0] = zz1;
   sl->x[1] = xx2; sl->y[1] = yy2; sl->z[1] = zz2;
   sl->x[2] = xx3; sl->y[2] = yy3; sl->z[2] = zz3;
   sl->x[3] = xx4; sl->y[3] = yy4; sl->z[3] = zz4;
}

/**
 * @brief Sets the UV texture coordinates of a slice's four vertices.
 *
 * These coordinates map a portion of the source image onto the slice.
 *
 * @param st The application state.
 * @param sl The slice to modify.
 * @param u1, v1 Texture coordinates for vertex 0.
 * @param u2, v2 Texture coordinates for vertex 1.
 * @param u3, v3 Texture coordinates for vertex 2.
 * @param u4, v4 Texture coordinates for vertex 3.
 */
static void
_slice_uv(State *st EINA_UNUSED, Slice *sl,
           double u1, double v1,
           double u2, double v2,
           double u3, double v3,
           double u4, double v4)
{
   sl->u[0] = u1; sl->v[0] = v1;
   sl->u[1] = u2; sl->v[1] = v2;
   sl->u[2] = u3; sl->v[2] = v3;
   sl->u[3] = u4; sl->v[3] = v4;
}

/**
 * @brief Deforms a 2D point into a 3D point to simulate a page curl.
 *
 * This function is the core of the curling effect. It maps a point from a flat
 * 2D page onto the surface of a cone, which is then rotated to create the
 * visual effect of a turning page.
 *
 * @param vi The input 2D vertex on the flat page.
 * @param vo The output 3D vertex on the curled page.
 * @param rho The rotation angle of the cone around the Y-axis (controls how
 *            much the page is turned). Range: ...-PI/2 to PI/2...
 * @param theta The angle of the cone (controls the "curliness" of the page).
 *              Range: 0 to PI/2.
 * @param A The distance of the cone's apex from the origin along the Y-axis.
 */
static void
_deform_point(Vertex2 *vi, Vertex3 *vo, double rho, double theta, double A)
{
   // ^Y
   // |
   // |    X
   // +---->
   // theta == cone angle (0 -> PI/2)
   // A     == distance of cone apex from origin
   // rho   == angle of cone from vertical axis (...-PI/2 to PI/2...)
   Vertex3  v1;
   double d, r, b;

   d = sqrt((vi->x * vi->x) + pow(vi->y - A, 2));
   r = d * sin(theta);
   b = asin(vi->x / d) / sin(theta);

   v1.x = r * sin(b);
   v1.y = d + A - (r * (1 - cos(b)) * sin(theta));
   v1.z = r * (1 - cos(b)) * cos(theta);

   vo->x = (v1.x * cos(rho)) - (v1.z * sin(rho));
   vo->y = v1.y;
   vo->z = (v1.x * sin(rho)) + (v1.z * cos(rho));
}

/**
 * @brief Performs linear interpolation between two 3D points.
 * @param vi1 The first input vertex.
 * @param vi2 The second input vertex.
 * @param vo The output interpolated vertex.
 * @param v The interpolation factor (0.0 gives vi1, 1.0 gives vi2).
 */
static void
_interp_point(Vertex3 *vi1, Vertex3 *vi2, Vertex3 *vo, double v)
{
   vo->x = (v * vi2->x) + ((1.0 - v) * vi1->x);
   vo->y = (v * vi2->y) + ((1.0 - v) * vi1->y);
   vo->z = (v * vi2->z) + ((1.0 - v) * vi1->z);
}

/**
 * @brief Frees all slices and resets slice-related state.
 *
 * This is called when the animation ends or when the number of slices needs
 * to change.
 *
 * @param st The application state.
 */
static void
_state_slices_clear(State *st)
{
   int i, j, num;

   if (st->slices)
     {
        num = 0;
        for (j = 0; j < st->slices_h; j++)
          {
             for (i = 0; i < st->slices_w; i++)
               {
                  if (st->slices[num]) _slice_free(st->slices[num]);
                  if (st->slices2[num]) _slice_free(st->slices2[num]);
                  num++;
               }
          }
        free(st->slices);
        free(st->slices2);
        st->slices = NULL;
        st->slices2 = NULL;
     }
   st->slices_w = 0;
   st->slices_h = 0;
}

/**
 * @brief Gets the color of a slice vertex and adds it to accumulator variables.
 *
 * This is a helper function for `_slice_obj_vert_color_merge`.
 *
 * @param s The slice to get the color from.
 * @param p The index of the vertex (0-3).
 * @param r, g, b, a Pointers to accumulate the color components.
 * @return 1 on success, 0 on failure.
 */
static int
_slice_obj_color_sum(Slice *s, int p, int *r, int *g, int *b, int *a)
{
   Evas_Map *m;
   int rr = 0, gg = 0, bb = 0, aa = 0;

   if (!s) return 0;
   m = (Evas_Map *)evas_object_map_get(s->obj);
   if (!m) return 0;
   evas_map_point_color_get(m, p, &rr, &gg, &bb, &aa);
   *r += rr; *g += gg; *b += bb; *a += aa;
   return 1;
}

/**
 * @brief Sets the color of a specific vertex on a slice.
 *
 * @param s The slice to modify.
 * @param p The index of the vertex (0-3).
 * @param r, g, b, a The color to set.
 */
static void
_slice_obj_color_set(Slice *s, int p, int r, int g, int b, int a)
{
   Evas_Map *m;

   if (!s) return;
   m = (Evas_Map *)evas_object_map_get(s->obj);
   if (!m) return;
   evas_map_point_color_set(m, p, r, g, b, a);
   evas_object_map_set(s->obj, m);
}

/**
 * @brief Merges the colors of adjacent vertices from up to four slices.
 *
 * This function averages the colors of vertices that meet at a single point
 * in the grid of slices. This is crucial for creating smooth shading across
 * the slice boundaries, avoiding visible seams.
 *
 * @param s1, s2, s3, s4 Pointers to the four adjacent slices (can be NULL).
 * @param p1, p2, p3, p4 The vertex indices within each respective slice.
 */
static void
_slice_obj_vert_color_merge(Slice *s1, int p1, Slice *s2, int p2,
                            Slice *s3, int p3, Slice *s4, int p4)
{
   int r = 0, g = 0, b = 0, a = 0, n = 0;

   n += _slice_obj_color_sum(s1, p1, &r, &g, &b, &a);
   n += _slice_obj_color_sum(s2, p2, &r, &g, &b, &a);
   n += _slice_obj_color_sum(s3, p3, &r, &g, &b, &a);
   n += _slice_obj_color_sum(s4, p4, &r, &g, &b, &a);

   if (n < 1) return;
   r /= n; g /= n; b /= n; a /= n;

   _slice_obj_color_set(s1, p1, r, g, b, a);
   _slice_obj_color_set(s2, p2, r, g, b, a);
   _slice_obj_color_set(s3, p3, r, g, b, a);
   _slice_obj_color_set(s4, p4, r, g, b, a);
}

/**
 * @brief The main update function for the page flip effect.
 *
 * This function is called whenever the flip state needs to be recalculated,
 * typically in response to mouse movement. It performs the entire process of:
 * 1. Determining the flip direction and progress based on mouse coordinates.
 * 2. Calculating the geometric parameters (rho, theta, A) for the curl.
 * 3. Creating or re-configuring the grid of slices if needed.
 * 4. Deforming the grid of vertices using `_deform_point`.
 * 5. Setting up the front and back faces of the page using the deformed grid.
 * 6. Applying transformations, lighting, and color merging to all slices.
 *
 * @param st The application state.
 * @return 1 if the state was updated and slices are active, 0 otherwise.
 */
static int
_state_update(State *st)
{
   Evas_Coord xx1, yy1, xx2, yy2, mx, my, dst, dx, dy;
   Evas_Coord x, y, w, h, ox, oy, ow, oh;
   int i, j, num, nn, jump, num2;
   Slice *sl;
   double b, minv = 0.0, minva, mgrad;
   int gx, gy, gszw, gszh, gw, gh, col, row, nw, nh;
   double rho, A, theta, perc, n, rhol, Al, thetal;
   Vertex3 *tvo, *tvol;

   st->backflip = 0;

   evas_object_geometry_get(st->front, &x, &y, &w, &h);
   ox = x; oy = y; ow = w; oh = h;
   xx1 = st->down_x;
   yy1 = st->down_y;
   xx2 = st->x;
   yy2 = st->y;

   dx = xx2 - xx1;
   dy = yy2 - yy1;
   dst = sqrt((dx * dx) + (dy * dy));
   if (st->dir == -1)
     {
        if (dst < 20) // MAGIC: 20 == drag hysterisis
           return 0;
     }
   if (st->dir == -1)
     {
        if      ((xx1 > (w / 2)) && (dx <  0) && (abs(dx) >  abs(dy))) st->dir = 0; // left
        else if ((xx1 < (w / 2)) && (dx >= 0) && (abs(dx) >  abs(dy))) st->dir = 1; // right
        else if ((yy1 > (h / 2)) && (dy <  0) && (abs(dy) >= abs(dx))) st->dir = 2; // up
        else if ((yy1 < (h / 2)) && (dy >= 0) && (abs(dy) >= abs(dx))) st->dir = 3; // down
        if (st->dir == -1) return 0;
     }
   if (st->dir == 0)
     {
        // no nothing. left drag is standard
     }
   else if (st->dir == 1)
     {
        xx1 = (w - 1) - xx1;
        xx2 = (w - 1) - xx2;
     }
   else if (st->dir == 2)
     {
        Evas_Coord tmp;

        tmp = xx1; xx1 = yy1; yy1 = tmp;
        tmp = xx2; xx2 = yy2; yy2 = tmp;
        tmp = w; w = h; h = tmp;
     }
   else if (st->dir == 3)
     {
        Evas_Coord tmp;

        tmp = xx1; xx1 = yy1; yy1 = tmp;
        tmp = xx2; xx2 = yy2; yy2 = tmp;
        tmp = w; w = h; h = tmp;
        xx1 = (w - 1) - xx1;
        xx2 = (w - 1) - xx2;
     }

   if (xx2 >= xx1) xx2 = xx1 - 1;
   mx = (xx1 + xx2) / 2;
   my = (yy1 + yy2) / 2;

   if (mx < 0) mx = 0;
   else if (mx >= w) mx = w - 1;
   if (my < 0) my = 0;
   else if (my >= h) my = h - 1;

   mgrad = (double)(yy1 - yy2) / (double)(xx1 - xx2);

   if (mx < 1) mx = 1; // quick hack to keep curl line visible

   if (EINA_DBL_EQ(mgrad, 0.0)) // special horizontal case
      mgrad = 0.001; // quick dirty hack for now
   // else
     {
        minv = 1.0 / mgrad;
        // y = (m * x) + b
        b = my + (minv * mx);
     }
   if ((b >= -5) && (b <= (h + 5)))
     {
        if (minv > 0.0) // clamp to h
          {
             minv = (double)(h + 5 - my) / (double)(mx);
             b = my + (minv * mx);
          }
        else // clamp to 0
          {
             minv = (double)(-5 - my) / (double)(mx);
             b = my + (minv * mx);
          }
     }

   perc = (double)xx2 / (double)xx1;
   if (perc < 0.0) perc = 0.0;
   else if (perc > 1.0) perc = 1.0;

   minva = atan(minv) / (M_PI / 2);
   if (minva < 0.0) minva = -minva;

   // A = apex of cone
   if (b <= 0) A = b;
   else A = h - b;
   if (A < -(h * 20)) A = -h * 20;
   //--//
   Al = -5;

   // rho = is how much the page is turned
   n = 1.0 - perc;
   n = 1.0 - cos(n * M_PI / 2.0);
   n = n * n;
   rho = -(n * M_PI);
   //--//
   rhol = -(n * M_PI);

   // theta == curliness (how much page culrs in on itself
   n = sin((1.0 - perc) * M_PI);
   n = n * 1.2;
   theta = 7.86 + n;
   //--//
   n = sin((1.0 - perc) * M_PI);
   n = 1.0 - n;
   n = n * n;
   n = 1.0 - n;
   thetal = 7.86 + n;

   nw = 16;
   nh = 16;
   gszw = w / nw;
   gszh = h / nh;
   if (gszw < 4) gszw = 4;
   if (gszh < 4) gszh = 4;

   nw = (w + gszw - 1) / gszw;
   nh = (h + gszh - 1) / gszh;
   if ((st->slices_w != nw) || (st->slices_h != nh)) _state_slices_clear(st);
   st->slices_w = nw;
   st->slices_h = nh;
   if (!st->slices)
     {
        st->slices = calloc(st->slices_w * st->slices_h, sizeof(Slice *));
        if (!st->slices) return 0;
        st->slices2 = calloc(st->slices_w * st->slices_h, sizeof(Slice *));
        if (!st->slices2)
          {
             free(st->slices);
             st->slices = NULL;
             return 0;
          }
     }

   num = (st->slices_w + 1) * (st->slices_h + 1);

   tvo = alloca(sizeof(Vertex3) * num);
   tvol = alloca(sizeof(Vertex3) * (st->slices_w + 1));

   for (col = 0, gx = 0; gx <= (w + gszw - 1); gx += gszw, col++)
     {
        Vertex2 vil;

        vil.x = gx;
        vil.y = h - gx;
        _deform_point(&vil, &(tvol[col]), rhol, thetal, Al);
     }

   n = minva * sin(perc * M_PI);
   n = n * n;

   num = 0;
   for (col = 0, gx = 0; gx <= (w + gszw - 1); gx += gszw, col++)
     {
        for (gy = 0; gy <= (h + gszh - 1); gy += gszh)
          {
             Vertex2 vi;
             Vertex3 vo, tvo1;

             if (gx > w) vi.x = w;
             else vi.x = gx;
             if (gy > h) vi.y = h;
             else vi.y = gy;
             _deform_point(&vi, &vo, rho, theta, A);
             tvo1 = tvol[col];
             if (gy > h) tvo1.y = h;
             else tvo1.y = gy;
             _interp_point(&vo, &tvo1, &(tvo[num]), n);
             num++;
          }
     }

   jump = st->slices_h + 1;
   for (col = 0, gx = 0; gx < w; gx += gszw, col++)
     {
        num = st->slices_h * col;
        num2 = jump * col;

        gw = gszw;
        if ((gx + gw) > w) gw = w - gx;

        for (row = 0, gy = 0; gy < h; gy += gszh, row++)
          {
             Vertex3 vo[4];

             memset(vo, 0, sizeof(vo));

             if (b > 0) nn = num + st->slices_h - row - 1;
             else nn = num + row;

             gh = gszh;
             if ((gy + gh) > h) gh = h - gy;

             vo[0] = tvo[num2 + row];
             vo[1] = tvo[num2 + row + jump];
             vo[2] = tvo[num2 + row + jump + 1];
             vo[3] = tvo[num2 + row + 1];
#define SWP(a, b) do {typeof(a) vt; vt = (a); (a) = (b); (b) = vt;} while (0)
             if (b > 0)
               {
                  SWP(vo[0], vo[3]);
                  SWP(vo[1], vo[2]);
                  vo[0].y = h - vo[0].y;
                  vo[1].y = h - vo[1].y;
                  vo[2].y = h - vo[2].y;
                  vo[3].y = h - vo[3].y;
               }

             // FRONT
             sl = st->slices[nn];
             if (!sl)
               {
                  sl = _slice_new(st, st->front);
                  st->slices[nn] = sl;
               }
             _slice_xyz(st, sl,
                        vo[0].x, vo[0].y, vo[0].z,
                        vo[1].x, vo[1].y, vo[1].z,
                        vo[2].x, vo[2].y, vo[2].z,
                        vo[3].x, vo[3].y, vo[3].z);
             if (b <= 0)
                _slice_uv(st, sl,
                          gx,       gy,       gx + gw,  gy,
                          gx + gw,  gy + gh,  gx,       gy + gh);
             else
                _slice_uv(st, sl,
                          gx,       h - (gy + gh), gx + gw,  h - (gy + gh),
                          gx + gw,  h - gy,        gx,       h - gy);

             // BACK
             sl = st->slices2[nn];
             if (!sl)
               {
                  sl = _slice_new(st, st->back);
                  st->slices2[nn] = sl;
               }

             _slice_xyz(st, sl,
                        vo[1].x, vo[1].y, vo[1].z,
                        vo[0].x, vo[0].y, vo[0].z,
                        vo[3].x, vo[3].y, vo[3].z,
                        vo[2].x, vo[2].y, vo[2].z);
             if (st->backflip)
               {
                  if (b <= 0)
                     _slice_uv(st, sl,
                               gx + gw, gy,       gx,       gy,
                               gx,      gy + gh,  gx + gw,  gy + gh);
                  else
                     _slice_uv(st, sl,
                               gx + gw, h - (gy + gh), gx,      h - (gy + gh),
                               gx,      h - gy,        gx + gw, h - gy);
               }
             else
               {
                  if (b <= 0)
                     _slice_uv(st, sl,
                               w - (gx + gw), gy,       w - (gx),      gy,
                               w - (gx),      gy + gh,  w - (gx + gw), gy + gh);
                  else
                     _slice_uv(st, sl,
                               w - (gx + gw), h - (gy + gh), w - (gx),      h - (gy + gh),
                               w - (gx),      h - gy,        w - (gx + gw), h - gy);
               }
          }
     }

   num = 0;
   for (j = 0; j < st->slices_h; j++)
     {
        for (i = 0; i < st->slices_w; i++)
          {
             _slice_apply(st, st->slices[num], x, y, w, h, ox, oy, ow, oh);
             _slice_apply(st, st->slices2[num], x, y, w, h, ox, oy, ow, oh);
             _slice_light(st, st->slices[num], ox, oy, ow, oh);
             _slice_light(st, st->slices2[num], ox, oy, ow, oh);
             num++;
          }
     }

   for (i = 0; i <= st->slices_w; i++)
     {
        num = i * st->slices_h;
        for (j = 0; j <= st->slices_h; j++)
          {
             Slice *s[4];

             s[0] = s[1] = s[2] = s[3] = NULL;
             if ((i > 0)            && (j > 0))
                s[0] = st->slices[num - 1 - st->slices_h];
             if ((i < st->slices_w) && (j > 0))
                s[1] = st->slices[num - 1];
             if ((i > 0)            && (j < st->slices_h))
                s[2] = st->slices[num - st->slices_h];
             if ((i < st->slices_w) && (j < st->slices_h))
                s[3] = st->slices[num];
             if (st->dir == 0)
                _slice_obj_vert_color_merge(s[0], 2, s[1], 3,
                                            s[2], 1, s[3], 0);
             else if (st->dir == 1)
                _slice_obj_vert_color_merge(s[0], 3, s[1], 2,
                                            s[2], 0, s[3], 1);
             else if (st->dir == 2)
                _slice_obj_vert_color_merge(s[0], 3, s[1], 2,
                                            s[2], 0, s[3], 1);
             else if (st->dir == 3)
                _slice_obj_vert_color_merge(s[0], 2, s[1], 3,
                                            s[2], 1, s[3], 0);
             s[0] = s[1] = s[2] = s[3] = NULL;
             if ((i > 0)            && (j > 0))
                s[0] = st->slices2[num - 1 - st->slices_h];
             if ((i < st->slices_w) && (j > 0))
                s[1] = st->slices2[num - 1];
             if ((i > 0)            && (j < st->slices_h))
                s[2] = st->slices2[num - st->slices_h];
             if ((i < st->slices_w) && (j < st->slices_h))
                s[3] = st->slices2[num];
             if (st->dir == 0)
                _slice_obj_vert_color_merge(s[0], 3, s[1], 2,
                                            s[2], 0, s[3], 1);
             else if (st->dir == 1)
                _slice_obj_vert_color_merge(s[0], 2, s[1], 3,
                                            s[2], 1, s[3], 0);
             else if (st->dir == 2)
                _slice_obj_vert_color_merge(s[0], 2, s[1], 3,
                                            s[2], 1, s[3], 0);
             else if (st->dir == 3)
                _slice_obj_vert_color_merge(s[0], 3, s[1], 2,
                                            s[2], 0, s[3], 1);
             num++;
          }
     }
   num = 0;
   for (i = 0; i < st->slices_w; i++)
     {
        for (j = 0; j < st->slices_h; j++)
          {
             _slice_3d(st, st->slices[num], ox, oy, ow, oh);
             _slice_3d(st, st->slices2[num], ox, oy, ow, oh);
             num++;
          }
     }

   return 1;
}

/**
 * @brief Cleans up the state after a flip animation is complete.
 * @param st The application state.
 */
static void
_state_end(State *st)
{
   _state_slices_clear(st);
}

/**
 * @brief The animator callback for the flip completion/reversion animation.
 *
 * This function is called by Ecore_Animator on each frame of the animation
 * that occurs after the user releases the mouse. It interpolates the
 * cursor position to either complete the page turn or snap it back to its
 * original state.
 *
 * @param data The application state (State *).
 * @param pos The animation position, from 0.0 (start) to 1.0 (end).
 * @return EINA_TRUE to continue animating, EINA_FALSE when done.
 */
static Eina_Bool
_state_anim(void *data, double pos)
{
   State *st = data;
   double p;

   p = ecore_animator_pos_map(pos, ECORE_POS_MAP_ACCELERATE, 0.0, 0.0);
   if (st->finish)
     {
        if (st->dir == 0)
           st->x = st->ox * (1.0 - p);
        else if (st->dir == 1)
           st->x = st->ox + ((st->w - st->ox) * p);
        else if (st->dir == 2)
           st->y = st->oy * (1.0 - p);
        else if (st->dir == 3)
           st->y = st->oy + ((st->h - st->oy) * p);
     }
   else
     {
        if (st->dir == 0)
           st->x = st->ox + ((st->w - st->ox) * p);
        else if (st->dir == 1)
           st->x = st->ox * (1.0 - p);
        else if (st->dir == 2)
           st->y = st->oy + ((st->h - st->oy) * p);
        else if (st->dir == 3)
           st->y = st->oy * (1.0 - p);
     }
   _state_update(st);
   if (pos < 1.0) return EINA_TRUE;
   evas_object_show(st->front);
   evas_object_show(st->back);
   _state_end(st);
   st->anim = NULL;
   return EINA_FALSE;
}

/**
 * @brief Ecore_Job callback to update the curl state.
 *
 * Using a job coalesces multiple rapid mouse move events into a single
 * update, preventing the expensive `_state_update` function from being
 * called too frequently and bogging down the application.
 *
 * @param data The application state (State *).
 */
static void
_update_curl_job(void *data)
{
   State *st = data;
   st->job = NULL;
   if (_state_update(st))
     {
        evas_object_hide(st->front);
        evas_object_hide(st->back);
     }
}

/**
 * @brief Callback for the MOUSE_DOWN event.
 *
 * Initializes the page flip state when the user presses the mouse button.
 * It records the initial position and prepares for the flip interaction.
 *
 * @param data The front page Evas_Object.
 * @param e The Evas canvas.
 * @param obj The object that received the event.
 * @param event_info The mouse down event details.
 */
static void
im_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   State *st = &state;
   Evas_Event_Mouse_Down *ev = event_info;
   Evas_Coord x, y, w, h;

   if (ev->button != 1) return;
   st->front = data;
   st->back = evas_object_data_get(data, "im2");
   st->backflip = 1;
   st->down = 1;
   evas_object_geometry_get(st->front, &x, &y, &w, &h);
   st->x = ev->canvas.x - x;
   st->y = ev->canvas.y - y;
   st->w = w;
   st->h = h;
   st->down_x = st->x;
   st->down_y = st->y;
   st->dir = -1;
   if (_state_update(st))
     {
        evas_object_hide(st->front);
        evas_object_hide(st->back);
     }
}

/**
 * @brief Callback for the MOUSE_UP event.
 *
 * Finalizes the flip interaction when the user releases the mouse button.
 * It determines whether to complete the page turn or snap back to the
 * original state, and starts the corresponding animation.
 *
 * @param data The front page Evas_Object.
 * @param e The Evas canvas.
 * @param obj The object that received the event.
 * @param event_info The mouse up event details.
 */
static void
im_up_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   State *st = &state;
   Evas_Event_Mouse_Up *ev = event_info;
   Evas_Coord x, y, w, h;
   double tm = 0.5;

   if (ev->button != 1) return;
   st->down = 0;
   evas_object_geometry_get(st->front, &x, &y, &w, &h);
   st->x = ev->canvas.x - x;
   st->y = ev->canvas.y - y;
   st->w = w;
   st->h = h;
   st->ox = st->x;
   st->oy = st->y;
   if (st->job)
     {
        ecore_job_del(st->job);
        st->job = NULL;
     }
   if (st->anim) ecore_animator_del(st->anim);
   st->finish = 0;
   if (st->dir == 0)
     {
        tm = (double)st->x / (double)st->w;
        if (st->x < (st->w / 2)) st->finish = 1;
     }
   else if (st->dir == 1)
     {
        if (st->x > (st->w / 2)) st->finish = 1;
        tm = 1.0 - ((double)st->x / (double)st->w);
     }
   else if (st->dir == 2)
     {
        if (st->y < (st->h / 2)) st->finish = 1;
        tm = (double)st->y / (double)st->h;
     }
   else if (st->dir == 3)
     {
        if (st->y > (st->h / 2)) st->finish = 1;
        tm = 1.0 - ((double)st->y / (double)st->h);
     }
   if (tm < 0.01) tm = 0.01;
   else if (tm > 0.99) tm = 0.99;
   if (!st->finish) tm = 1.0 - tm;
   tm *= 0.5;
   st->anim = ecore_animator_timeline_add(tm, _state_anim, st);
   _state_anim(st, 0.0);
}

/**
 * @brief Callback for the MOUSE_MOVE event.
 *
 * Updates the page curl in real-time as the user drags the mouse. It schedules
 * an Ecore_Job to perform the update, which helps to throttle the update rate.
 *
 * @param data The front page Evas_Object.
 * @param e The Evas canvas.
 * @param obj The object that received the event.
 * @param event_info The mouse move event details.
 */
static void
im_move_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   State *st = &state;
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Coord x, y, w, h;

   if (!st->down) return;
   evas_object_geometry_get(st->front, &x, &y, &w, &h);
   st->x = ev->cur.canvas.x - x;
   st->y = ev->cur.canvas.y - y;
   st->w = w;
   st->h = h;
   if (st->job) ecore_job_del(st->job);
   st->job = ecore_job_add(_update_curl_job, st);
}

/**
 * @brief The main test function that sets up the flip page example.
 *
 * This function creates a window, loads the front and back images for the
 * page, and sets up transparent event-catching rectangles. These rectangles
 * are used to initiate different flip directions (from corners and edges)
 * when clicked and dragged.
 */
void
test_flip_page(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *im, *im2, *rc;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("flip-page", "Flip Page");
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   elm_win_autodel_set(win, EINA_TRUE);

   im2 = evas_object_image_filled_add(evas_object_evas_get(win));
   snprintf(buf, sizeof(buf), "%s/images/%s",
            elm_app_data_dir_get(), "sky_04.jpg");
   evas_object_image_file_set(im2, buf, NULL);
   evas_object_move(im2, 40, 40);
   evas_object_resize(im2, 400, 400);
   evas_object_show(im2);

#if 0
   im = elm_layout_add(win);
   snprintf(buf, sizeof(buf), "%s/objects/test.edj", elm_app_data_dir_get());
   elm_layout_file_set(im, buf, "layout");
#else
   im = evas_object_image_filled_add(evas_object_evas_get(win));
   snprintf(buf, sizeof(buf), "%s/images/%s",
            elm_app_data_dir_get(), "twofish.jpg");
   evas_object_image_file_set(im, buf, NULL);
#endif
   evas_object_move(im, 40, 40);
   evas_object_resize(im, 400, 400);
   evas_object_show(im);

   evas_object_data_set(im, "im2", im2);

   rc = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_color_set(rc, 0, 0, 0, 0);
   evas_object_move(rc, 40, 340);
   evas_object_resize(rc, 400, 100);
   evas_object_show(rc);

   evas_object_event_callback_add(rc, EVAS_CALLBACK_MOUSE_DOWN, im_down_cb, im);
   evas_object_event_callback_add(rc, EVAS_CALLBACK_MOUSE_UP,   im_up_cb,   im);
   evas_object_event_callback_add(rc, EVAS_CALLBACK_MOUSE_MOVE, im_move_cb, im);

   rc = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_color_set(rc, 0, 0, 0, 0);
   evas_object_move(rc, 40, 40);
   evas_object_resize(rc, 400, 100);
   evas_object_show(rc);

   evas_object_event_callback_add(rc, EVAS_CALLBACK_MOUSE_DOWN, im_down_cb, im);
   evas_object_event_callback_add(rc, EVAS_CALLBACK_MOUSE_UP,   im_up_cb,   im);
   evas_object_event_callback_add(rc, EVAS_CALLBACK_MOUSE_MOVE, im_move_cb, im);

   rc = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_color_set(rc, 0, 0, 0, 0);
   evas_object_move(rc, 340, 40);
   evas_object_resize(rc, 100, 400);
   evas_object_show(rc);

   evas_object_event_callback_add(rc, EVAS_CALLBACK_MOUSE_DOWN, im_down_cb, im);
   evas_object_event_callback_add(rc, EVAS_CALLBACK_MOUSE_UP,   im_up_cb,   im);
   evas_object_event_callback_add(rc, EVAS_CALLBACK_MOUSE_MOVE, im_move_cb, im);

   rc = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_color_set(rc, 0, 0, 0, 0);
   evas_object_move(rc, 40, 40);
   evas_object_resize(rc, 100, 400);
   evas_object_show(rc);

   evas_object_event_callback_add(rc, EVAS_CALLBACK_MOUSE_DOWN, im_down_cb, im);
   evas_object_event_callback_add(rc, EVAS_CALLBACK_MOUSE_UP,   im_up_cb,   im);
   evas_object_event_callback_add(rc, EVAS_CALLBACK_MOUSE_MOVE, im_move_cb, im);

   evas_object_resize(win, 480 * elm_config_scale_get(),
                           480 * elm_config_scale_get());
   evas_object_show(win);
}
