#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>
#include <Elementary.h>
#ifndef M_PI
#define M_PI 3.14159265
#endif

/**
 * @struct _Gear
 * @brief  Represents a single gear, including its geometry and GL buffer.
 */
typedef struct _Gear Gear;
typedef struct _GLData GLData;
struct _Gear
{
   GLfloat *vertices; /**< Vertex data array (position and normal) */
   GLuint vbo;        /**< Vertex Buffer Object ID */
   int count;         /**< Number of vertices in the gear */
};

/**
 * @struct _GLData
 * @brief  Holds all GL-related data and application state for the gears demo.
 */
struct _GLData
{
   Evas_GL_API *glapi;   /**< Pointer to the Evas GL API structure */
   GLuint       program;    /**< GL program object */
   GLuint       vtx_shader; /**< Vertex shader object */
   GLuint       fgmt_shader;/**< Fragment shader object */
   int          initialized : 1; /**< Flag to check if GL is initialized */
   int          mouse_down : 1;  /**< Flag to track mouse button state */

   GLfloat      view_rotx; /**< Rotation of the view on the X axis */
   GLfloat      view_roty; /**< Rotation of the view on the Y axis */
   GLfloat      view_rotz; /**< Rotation of the view on the Z axis */

   Gear        *gear1;    /**< The first gear object */
   Gear        *gear2;    /**< The second gear object */
   Gear        *gear3;    /**< The third gear object */

   GLfloat      angle;     /**< Current angle for gear animation */

   GLuint       proj_location;  /**< Location of the projection matrix uniform */
   GLuint       light_location; /**< Location of the light position uniform */
   GLuint       color_location; /**< Location of the color uniform */

   GLfloat      proj[16];  /**< Projection matrix */
   GLfloat      light[3];  /**< Light source position */
};

static void gears_init(GLData *gld);
static void free_gear(Gear *gear);
static void gears_reshape(GLData *gld, int width, int height);
static void render_gears(GLData *gld);

/**
 * @brief Fills a vertex data array with position and normal.
 *
 * This function populates a given buffer with interleaved vertex data,
 * consisting of a 3D position (x, y, z) and a 3D normal vector (n).
 * The array structure is [x, y, z, nx, ny, nz].
 *
 * @param p Pointer to the current position in the vertex buffer.
 * @param x X coordinate of the vertex.
 * @param y Y coordinate of the vertex.
 * @param z Z coordinate of the vertex.
 * @param n Pointer to an array of 3 GLfloats representing the normal vector.
 * @return Pointer to the next position in the vertex buffer to be filled.
 */
static GLfloat *
vert(GLfloat *p, GLfloat x, GLfloat y, GLfloat z, GLfloat *n)
{
   p[0] = x;
   p[1] = y;
   p[2] = z;
   p[3] = n[0];
   p[4] = n[1];
   p[5] = n[2];

   return p + 6;
}

/**
 * @brief Create a gear wheel.
 *
 * This function generates the vertices for a gear wheel and stores them in a
 * vertex buffer object (VBO). The gear is drawn using triangle strips.
 *
 * @param gld The GL data structure.
 * @param inner_radius Radius of the hole at the center.
 * @param outer_radius Radius at the center of the teeth.
 * @param width Width of the gear.
 * @param teeth Number of teeth.
 * @param tooth_depth Depth of the teeth.
 * @return A newly allocated Gear object, or NULL on failure.
 */
static Gear *
make_gear(GLData *gld, GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
          GLint teeth, GLfloat tooth_depth)
{
   GLint i;
   GLfloat r0, r1, r2;
   GLfloat da;
   GLfloat *v;
   Gear *gear;
   double s[5], c[5];
   GLfloat normal[3];
   const int tris_per_tooth = 20;
   Evas_GL_API *gl = gld->glapi;

   gear = (Gear*)malloc(sizeof(Gear));
   if (gear == NULL)
     return NULL;

   r0 = inner_radius;
   r1 = outer_radius - tooth_depth / 2.0;
   r2 = outer_radius + tooth_depth / 2.0;

   da = 2.0 * M_PI / teeth / 4.0;

   gear->vertices = calloc(teeth * tris_per_tooth * 3 * 6,
                           sizeof *gear->vertices);
   s[4] = 0;
   c[4] = 1;
   v = gear->vertices;
   for (i = 0; i < teeth; i++)
     {
        s[0] = s[4];
        c[0] = c[4];
        s[1] = sin(i * 2.0 * M_PI / teeth + da);
        c[1] = cos(i * 2.0 * M_PI / teeth + da);
        s[2] = sin(i * 2.0 * M_PI / teeth + da * 2);
        c[2] = cos(i * 2.0 * M_PI / teeth + da * 2);
        s[3] = sin(i * 2.0 * M_PI / teeth + da * 3);
        c[3] = cos(i * 2.0 * M_PI / teeth + da * 3);
        s[4] = sin(i * 2.0 * M_PI / teeth + da * 4);
        c[4] = cos(i * 2.0 * M_PI / teeth + da * 4);

        normal[0] = 0.0;
        normal[1] = 0.0;
        normal[2] = 1.0;

        v = vert(v, r2 * c[1], r2 * s[1], width * 0.5, normal);

        v = vert(v, r2 * c[1], r2 * s[1], width * 0.5, normal);
        v = vert(v, r2 * c[2], r2 * s[2], width * 0.5, normal);
        v = vert(v, r1 * c[0], r1 * s[0], width * 0.5, normal);
        v = vert(v, r1 * c[3], r1 * s[3], width * 0.5, normal);
        v = vert(v, r0 * c[0], r0 * s[0], width * 0.5, normal);
        v = vert(v, r1 * c[4], r1 * s[4], width * 0.5, normal);
        v = vert(v, r0 * c[4], r0 * s[4], width * 0.5, normal);

        v = vert(v, r0 * c[4], r0 * s[4], width * 0.5, normal);
        v = vert(v, r0 * c[0], r0 * s[0], width * 0.5, normal);
        v = vert(v, r0 * c[4], r0 * s[4], -width * 0.5, normal);
        v = vert(v, r0 * c[0], r0 * s[0], -width * 0.5, normal);

        normal[0] = 0.0;
        normal[1] = 0.0;
        normal[2] = -1.0;

        v = vert(v, r0 * c[4], r0 * s[4], -width * 0.5, normal);

        v = vert(v, r0 * c[4], r0 * s[4], -width * 0.5, normal);
        v = vert(v, r1 * c[4], r1 * s[4], -width * 0.5, normal);
        v = vert(v, r0 * c[0], r0 * s[0], -width * 0.5, normal);
        v = vert(v, r1 * c[3], r1 * s[3], -width * 0.5, normal);
        v = vert(v, r1 * c[0], r1 * s[0], -width * 0.5, normal);
        v = vert(v, r2 * c[2], r2 * s[2], -width * 0.5, normal);
        v = vert(v, r2 * c[1], r2 * s[1], -width * 0.5, normal);

        v = vert(v, r1 * c[0], r1 * s[0], width * 0.5, normal);

        v = vert(v, r1 * c[0], r1 * s[0], width * 0.5, normal);
        v = vert(v, r1 * c[0], r1 * s[0], -width * 0.5, normal);
        v = vert(v, r2 * c[1], r2 * s[1], width * 0.5, normal);
        v = vert(v, r2 * c[1], r2 * s[1], -width * 0.5, normal);
        v = vert(v, r2 * c[2], r2 * s[2], width * 0.5, normal);
        v = vert(v, r2 * c[2], r2 * s[2], -width * 0.5, normal);
        v = vert(v, r1 * c[3], r1 * s[3], width * 0.5, normal);
        v = vert(v, r1 * c[3], r1 * s[3], -width * 0.5, normal);
        v = vert(v, r1 * c[4], r1 * s[4], width * 0.5, normal);
        v = vert(v, r1 * c[4], r1 * s[4], -width * 0.5, normal);

        v = vert(v, r1 * c[4], r1 * s[4], -width * 0.5, normal);
     }

   gear->count = (v - gear->vertices) / 6;

   gl->glGenBuffers(1, &gear->vbo);
   gl->glBindBuffer(GL_ARRAY_BUFFER, gear->vbo);
   gl->glBufferData(GL_ARRAY_BUFFER, gear->count * 6 * 4,
                    gear->vertices, GL_STATIC_DRAW);

   return gear;
}

/**
 * @brief Frees the memory allocated for a Gear object.
 * @param gear The Gear object to free.
 */
static void
free_gear(Gear *gear)
{
    if (!gear) return;
    free(gear->vertices);
    free(gear);
}

/**
 * @brief Multiplies two 4x4 matrices.
 *
 * The result of m * n is stored in m. The matrices are expected to be
 * in column-major order.
 *
 * @param m The first matrix, also the destination for the result.
 * @param n The second matrix.
 */
static void
multiply(GLfloat *m, const GLfloat *n)
{
   GLfloat tmp[16];
   const GLfloat *row, *column;
   div_t d;
   int i, j;

   for (i = 0; i < 16; i++)
     {
        tmp[i] = 0;
        d = div(i, 4);
        row = n + d.quot * 4;
        column = m + d.rem;
        for (j = 0; j < 4; j++)
          tmp[i] += row[j] * column[j * 4];
     }
   memcpy(m, &tmp, sizeof tmp);
}

/**
 * @brief Applies a rotation transformation to a matrix.
 *
 * Multiplies matrix m by a rotation matrix.
 *
 * @param m The matrix to rotate.
 * @param angle The angle of rotation in radians.
 * @param x The x component of the rotation axis.
 * @param y The y component of the rotation axis.
 * @param z The z component of the rotation axis.
 */
static void
rotate(GLfloat *m, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
   double s, c;

   s = sin(angle);
   c = cos(angle);
   GLfloat r[16] =
     {
        x * x * (1 - c) + c,     y * x * (1 - c) + z * s, x * z * (1 - c) - y * s, 0,
        x * y * (1 - c) - z * s, y * y * (1 - c) + c,     y * z * (1 - c) + x * s, 0,
        x * z * (1 - c) + y * s, y * z * (1 - c) - x * s, z * z * (1 - c) + c,     0,
        0, 0, 0, 1
     };

   multiply(m, r);
}

/**
 * @brief Applies a translation to a matrix.
 *
 * Multiplies matrix m by a translation matrix.
 *
 * @param m The matrix to translate.
 * @param x Translation along the X axis.
 * @param y Translation along the Y axis.
 * @param z Translation along the Z axis.
 */
static void
translate(GLfloat *m, GLfloat x, GLfloat y, GLfloat z)
{
   GLfloat t[16] = { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  x, y, z, 1 };

   multiply(m, t);
}

/**
 * @brief Draws a single gear.
 *
 * This function sets up the model-view matrix for a gear, applies translation
 * and rotation, sets shader uniforms, and then draws the gear.
 *
 * @param gld The GL data structure.
 * @param gear The gear to draw.
 * @param m The base model-view matrix.
 * @param x X-axis translation for the gear.
 * @param y Y-axis translation for the gear.
 * @param angle Rotation angle in degrees around the Z axis.
 * @param color The color of the gear as a 4-element float array (RGBA).
 */
static void
draw_gear(GLData *gld, Gear *gear, GLfloat *m,
          GLfloat x, GLfloat y, GLfloat angle, const GLfloat *color)
{
   Evas_GL_API *gl = gld->glapi;
   GLfloat tmp[16];

   memcpy(tmp, m, sizeof tmp);
   translate(tmp, x, y, 0);
   rotate(tmp, 2 * M_PI * angle / 360.0, 0, 0, 1);
   gl->glUniformMatrix4fv(gld->proj_location, 1, GL_FALSE, tmp);
   gl->glUniform3fv(gld->light_location, 1, gld->light);
   gl->glUniform4fv(gld->color_location, 1, color);

   gl->glBindBuffer(GL_ARRAY_BUFFER, gear->vbo);

   gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                             6 * sizeof(GLfloat), NULL);
   gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                             6 * sizeof(GLfloat), (GLfloat *)(0 + 3 * sizeof(GLfloat)));
   gl->glEnableVertexAttribArray(0);
   gl->glEnableVertexAttribArray(1);
   gl->glDrawArrays(GL_TRIANGLE_STRIP, 0, gear->count);
}

/**
 * @brief Draws the complete scene with all gears.
 *
 * Clears the color and depth buffers, sets up the view matrix based on user
 * rotation, and then draws each of the three gears with its specific
 * transformation and color.
 *
 * @param gld The GL data structure.
 */
static void
gears_draw(GLData *gld)
{
   Evas_GL_API *gl = gld->glapi;

   static const GLfloat red[4] = { 0.8, 0.1, 0.0, 1.0 };
   static const GLfloat green[4] = { 0.0, 0.8, 0.2, 1.0 };
   static const GLfloat blue[4] = { 0.2, 0.2, 1.0, 1.0 };
   GLfloat m[16];

   gl->glClearColor(0x25 / 255., 0x13 / 255., 0.0, 1.0);
   gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   memcpy(m, gld->proj, sizeof m);
   rotate(m, 2 * M_PI * gld->view_rotx / 360.0, 1, 0, 0);
   rotate(m, 2 * M_PI * gld->view_roty / 360.0, 0, 1, 0);
   rotate(m, 2 * M_PI * gld->view_rotz / 360.0, 0, 0, 1);

   draw_gear(gld, gld->gear1, m, -3.0, -2.0, gld->angle, red);
   draw_gear(gld, gld->gear2, m, 3.1, -2.0, -2 * gld->angle - 9.0, green);
   draw_gear(gld, gld->gear3, m, -3.1, 4.2, -2 * gld->angle - 25.0, blue);
}

/**
 * @brief Renders a single frame of the gears animation.
 *
 * This function is called for each frame. It draws the gears and updates
 * the animation angle for the next frame.
 *
 * @param gld The GL data structure.
 */
static void render_gears(GLData *gld)
{
   gears_draw(gld);

   gld->angle += 2.0;
}

/**
 * @brief Updates the GL viewport and projection matrix on window resize.
 *
 * This is called when the GLView is resized. It adapts the projection to
 * maintain the aspect ratio of the rendered content.
 *
 * @param gld The GL data structure.
 * @param width The new width of the GLView.
 * @param height The new height of the GLView.
 */
static void
gears_reshape(GLData *gld, int width, int height)
{
   Evas_GL_API *gl = gld->glapi;

   GLfloat ar, m[16] = {
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 0.1, 0.0,
      0.0, 0.0, 0.0, 1.0
   };

   if (width < height)
     ar = width;
   else
     ar = height;

   m[0] = 0.1 * ar / width;
   m[5] = 0.1 * ar / height;
   memcpy(gld->proj, m, sizeof gld->proj);
   gl->glViewport(0, 0, (GLint) width, (GLint) height);
}

static const char vertex_shader[] =
   "uniform mat4 proj;\n"
   "attribute vec4 position;\n"
   "attribute vec4 normal;\n"
   "varying vec3 rotated_normal;\n"
   "varying vec3 rotated_position;\n"
   "vec4 tmp;\n"
   "void main()\n"
   "{\n"
   "   gl_Position = proj * position;\n"
   "   rotated_position = gl_Position.xyz;\n"
   "   tmp = proj * normal;\n"
   "   rotated_normal = tmp.xyz;\n"
   "}\n";

 static const char fragment_shader[] =
   "#ifdef GL_ES\n"
   "precision mediump float;\n"
   "#endif\n"
   "uniform vec4 color;\n"
   "uniform vec3 light;\n"
   "varying vec3 rotated_normal;\n"
   "varying vec3 rotated_position;\n"
   "vec3 light_direction;\n"
   "vec4 white = vec4(0.5, 0.5, 0.5, 1.0);\n"
   "void main()\n"
   "{\n"
   "   light_direction = normalize(light - rotated_position);\n"
   "   gl_FragColor = color + white * dot(light_direction, rotated_normal);\n"
   "}\n";

/**
 * @brief Prints the info log for a shader or program object.
 *
 * Useful for debugging GLSL compilation and linking errors.
 *
 * @param gl Pointer to the Evas GL API.
 * @param id The ID of the shader or program object.
 */
static void
_print_gl_log(Evas_GL_API *gl, GLuint id)
{
   GLint log_len = 0;
   char *log_info;

   if (gl->glIsShader(id))
     gl->glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_len);
   else if (gl->glIsProgram(id))
     gl->glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_len);
   if (!log_len) return;

   log_info = malloc(log_len * sizeof(char));

   if (gl->glIsShader(id))
     gl->glGetShaderInfoLog(id, log_len, NULL, log_info);
   else if (gl->glIsProgram(id))
     gl->glGetProgramInfoLog(id, log_len, NULL, log_info);

   printf("%s", log_info);
   free(log_info);
}

/**
 * @brief Initializes GL state, shaders, and gear models.
 *
 * This function sets up everything needed for rendering: compiles and links
 * the GLSL shaders, retrieves uniform locations, enables depth testing and
 * face culling, and creates the geometry for the three gears.
 *
 * @param gld The GL data structure.
 */
static void
gears_init(GLData *gld)
{
   Evas_GL_API *gl = gld->glapi;

   const char *p;

   gl->glEnable(GL_CULL_FACE);
   gl->glEnable(GL_DEPTH_TEST);

   p = vertex_shader;
   gld->vtx_shader = gl->glCreateShader(GL_VERTEX_SHADER);
   gl->glShaderSource(gld->vtx_shader, 1, &p, NULL);
   gl->glCompileShader(gld->vtx_shader);
   _print_gl_log(gl, gld->vtx_shader);

   p = fragment_shader;
   gld->fgmt_shader = gl->glCreateShader(GL_FRAGMENT_SHADER);
   gl->glShaderSource(gld->fgmt_shader, 1, &p, NULL);
   gl->glCompileShader(gld->fgmt_shader);
   _print_gl_log(gl, gld->fgmt_shader);

   gld->program = gl->glCreateProgram();
   gl->glAttachShader(gld->program, gld->vtx_shader);
   gl->glAttachShader(gld->program, gld->fgmt_shader);
   gl->glBindAttribLocation(gld->program, 0, "position");
   gl->glBindAttribLocation(gld->program, 1, "normal");

   gl->glLinkProgram(gld->program);
   _print_gl_log(gl, gld->program);

   gl->glUseProgram(gld->program);
   gld->proj_location  = gl->glGetUniformLocation(gld->program, "proj");
   gld->light_location = gl->glGetUniformLocation(gld->program, "light");
   gld->color_location = gl->glGetUniformLocation(gld->program, "color");

   /* make the gears */
   gld->gear1 = make_gear(gld, 1.0, 4.0, 1.0, 20, 0.7);
   gld->gear2 = make_gear(gld, 0.5, 2.0, 2.0, 10, 0.7);
   gld->gear3 = make_gear(gld, 1.3, 2.0, 0.5, 10, 0.7);
}

/**
 * @brief Initializes the GLData structure with default values.
 *
 * Sets initial rotation, animation angle, and light position.
 *
 * @param gld The GL data structure to initialize.
 */
static void
gldata_init(GLData *gld)
{
   gld->initialized = 0;
   gld->mouse_down = 0;

   gld->view_rotx = -20.0;
   gld->view_roty = -30.0;
   gld->view_rotz = 0.0;
   gld->angle = 0.0;

   gld->light[0] = 1.0;
   gld->light[1] = 1.0;
   gld->light[2] = -5.0;
}

//-------------------------//

/**
 * @brief The init callback for the GLView object.
 *
 * This function is called when the GLView is ready for GL initialization.
 * It retrieves the GL API, and initializes the gears demo.
 *
 * @param obj The GLView object.
 */
static void
_init_gl(Evas_Object *obj)
{
   GLData *gld = evas_object_data_get(obj, "gld");

   gld->glapi = elm_glview_gl_api_get(obj);
   printf("GL_VERSION: %s\n", gld->glapi->glGetString(GL_VERSION));
   fflush(stdout);

   gears_init(gld);
}

/**
 * @brief The deletion callback for the GLView object.
 *
 * This function is called when the GLView is being deleted. It is responsible
 * for cleaning up all allocated GL resources and application data.
 *
 * @param obj The GLView object.
 */
static void
_del_gl(Evas_Object *obj)
{
   GLData *gld = evas_object_data_get(obj, "gld");
   if (!gld)
     {
        printf("Unable to get GLData.\n");
        fflush(stdout);
        return;
     }
   Evas_GL_API *gl = gld->glapi;

   if (gl)
     {
        gl->glDeleteShader(gld->vtx_shader);
        gl->glDeleteShader(gld->fgmt_shader);
        gl->glDeleteProgram(gld->program);
        gl->glDeleteBuffers(1, &gld->gear1->vbo);
        gl->glDeleteBuffers(1, &gld->gear2->vbo);
        gl->glDeleteBuffers(1, &gld->gear3->vbo);
     }

   free_gear(gld->gear1);
   free_gear(gld->gear2);
   free_gear(gld->gear3);

   evas_object_data_del((Evas_Object*)obj, "..gld");
   free(gld);
}

/**
 * @brief The resize callback for the GLView object.
 *
 * This function is called when the GLView object is resized.
 *
 * @param obj The GLView object.
 */
static void
_resize_gl(Evas_Object *obj)
{
   int w, h;
   GLData *gld = evas_object_data_get(obj, "gld");

   elm_glview_size_get(obj, &w, &h);

   // GL Viewport stuff. you can avoid doing this if viewport is all the
   // same as last frame if you want
   gears_reshape(gld, w,h);
}

/**
 * @brief The rendering callback for the GLView object.
 *
 * This function is called when the GLView needs to be redrawn.
 *
 * @param obj The GLView object.
 */
static void
_draw_gl(Evas_Object *obj)
{
   Evas_GL_API *gl = elm_glview_gl_api_get(obj);
   GLData *gld = evas_object_data_get(obj, "gld");
   if (!gld) return;

   render_gears(gld);
   gl->glFinish();
}

/**
 * @brief Ecore animator callback for animation.
 *
 * This function is called repeatedly by the main loop's animator. It marks
 * the GLView as 'changed', which triggers a redraw, creating the animation.
 *
 * @param data The GLView object.
 * @return EINA_TRUE to continue the animation, EINA_FALSE to stop.
 */
static Eina_Bool
_anim(void *data)
{
   elm_glview_changed_set(data);
   return EINA_TRUE;
}

/**
 * @brief Ecore idler callback to quit the application.
 *
 * Using an idler ensures that the object deletion happens from the main loop
 * when it's safe to do so, not from within an event callback.
 *
 * @param data The window object to delete.
 * @return ECORE_CALLBACK_CANCEL to remove the idler after it runs.
 */
static Eina_Bool
_quit_idler(void *data)
{
   evas_object_del(data);

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Callback for the "Close" button.
 *
 * Schedules the window to be deleted via an idler.
 *
 * @param data The window object.
 * @param obj The button object that was clicked.
 * @param event_info Event-specific information (unused).
 */
static void
_on_done(void *data,
         Evas_Object *obj EINA_UNUSED,
         void *event_info EINA_UNUSED)
{
   ecore_idler_add(_quit_idler, data);
}

/**
 * @brief Callback for the "Direct Mode" button.
 *
 * Switches the GLView to use direct rendering, which can be more efficient
 * as it avoids rendering through an intermediate buffer.
 *
 * @param data The GLView object.
 * @param obj The button object that was clicked.
 * @param event_info Event-specific information (unused).
 */
static void
_on_direct(void *data,
           Evas_Object *obj EINA_UNUSED,
           void *event_info EINA_UNUSED)
{
   if (!data) return;

   // ON_DEMAND is necessary for Direct Rendering
   elm_glview_render_policy_set(data, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);
   elm_glview_mode_set(data, 0
                       | ELM_GLVIEW_ALPHA
                       | ELM_GLVIEW_DEPTH
                       | ELM_GLVIEW_DIRECT
                      );
}

/**
 * @brief Callback for the "Indirect Mode" button.
 *
 * Switches the GLView to use indirect rendering (the default). The scene is
 * rendered to an FBO, which is then blended with the rest of the Elementary UI.
 *
 * @param data The GLView object.
 * @param obj The button object that was clicked.
 * @param event_info Event-specific information (unused).
 */
static void
_on_indirect(void *data,
           Evas_Object *obj EINA_UNUSED,
           void *event_info EINA_UNUSED)
{
   if (!data) return;

   // note that with policy ALWAYS the window will flicker on resize
   elm_glview_render_policy_set(data, ELM_GLVIEW_RENDER_POLICY_ALWAYS);
   elm_glview_mode_set(data, 0
                       | ELM_GLVIEW_ALPHA
                       | ELM_GLVIEW_DEPTH
                      );
}

/**
 * @brief Evas object deletion callback for the GLView.
 *
 * This is called when the GLView object itself is deleted. It cleans up
 * the associated animator.
 *
 * @param data User data (unused).
 * @param evas The Evas canvas (unused).
 * @param obj The object being deleted.
 * @param event_info Event-specific information (unused).
 */
static void
_del(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Ecore_Animator *ani = evas_object_data_get(obj, "ani");
   ecore_animator_del(ani);
}

/**
 * @brief Key down event callback for the GLView.
 *
 * Handles arrow keys to rotate the view.
 *
 * @param data User data (unused).
 * @param e The Evas canvas (unused).
 * @param obj The object that received the event.
 * @param event_info The key down event details.
 */
static void
_key_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Evas_Event_Key_Down *ev;
   ev = (Evas_Event_Key_Down *)event_info;
   GLData *gld = evas_object_data_get(obj, "gld");

   if (strcmp(ev->key, "Left") == 0)
     {
        gld->view_roty += 5.0;
        return;
     }

   if (strcmp(ev->key, "Right") == 0)
     {
        gld->view_roty -= 5.0;
        return;
     }

   if (strcmp(ev->key, "Up") == 0)
     {
        gld->view_rotx += 5.0;
        return;
     }

   if (strcmp(ev->key, "Down") == 0)
     {
        gld->view_rotx -= 5.0;
        return;
     }
   if ((strcmp(ev->key, "Escape") == 0) ||
       (strcmp(ev->key, "Return") == 0))
     {
        //_on_done(data, obj, event_info);
        return;
     }
}

/**
 * @brief Mouse down event callback for the GLView.
 *
 * Sets a flag to indicate that the mouse button is pressed, to be used
 * by the mouse move handler.
 *
 * @param data User data (unused).
 * @param e The Evas canvas (unused).
 * @param obj The object that received the event.
 * @param event_info The mouse down event details (unused).
 */
static void
_mouse_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   GLData *gld = evas_object_data_get(obj, "gld");
   gld->mouse_down = 1;
}

/**
 * @brief Mouse move event callback for the GLView.
 *
 * Rotates the view when the mouse is moved while the button is down.
 *
 * @param data User data (unused).
 * @param e The Evas canvas (unused).
 * @param obj The object that received the event.
 * @param event_info The mouse move event details.
 */
static void
_mouse_move(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Event_Mouse_Move *ev;
   ev = (Evas_Event_Mouse_Move *)event_info;
   GLData *gld = evas_object_data_get(obj, "gld");
   float dx = 0, dy = 0;

   if (gld->mouse_down)
     {
        dx = ev->cur.canvas.x - ev->prev.canvas.x;
        dy = ev->cur.canvas.y - ev->prev.canvas.y;

        gld->view_roty += -1.0 * dx;
        gld->view_rotx += -1.0 * dy;
     }
}

/**
 * @brief Mouse up event callback for the GLView.
 *
 * Clears the flag that indicates the mouse button is pressed.
 *
 * @param data User data (unused).
 * @param e The Evas canvas (unused).
 * @param obj The object that received the event.
 * @param event_info The mouse up event details (unused).
 */
static void
_mouse_up(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   GLData *gld = evas_object_data_get(obj, "gld");
   gld->mouse_down = 0;
}

/**
 * @brief Sets up and runs the GLView gears test.
 *
 * This function creates the window, layout, GLView widget, buttons, and
 * sets up all the necessary callbacks for initialization, rendering, and
 * event handling.
 *
 * @param version The GLES version to request for the context.
 */
static void
_test_glview_do(Evas_GL_Context_Version version)
{
   Evas_Object *win, *bx, *bt, *gl, *lb;
   Ecore_Animator *ani;
   GLData *gld = NULL;

   // alloc a data struct to hold our relevant gl info in
   if (!(gld = calloc(1, sizeof(GLData)))) return;
   gldata_init(gld);

#if 1
   // add a Z-depth buffer to the window and try to use GL
   Eina_Stringshare *accel;
   accel = eina_stringshare_add(elm_config_accel_preference_get());
   elm_config_accel_preference_set("gl:depth");

   // new window - do the usual and give it a name, title and delete handler
   win = elm_win_util_standard_add("glview", "GLView");
   elm_win_autodel_set(win, EINA_TRUE);

   // restore previous accel preference
   elm_config_accel_preference_set(accel);
   eina_stringshare_del(accel);
#else
   win = efl_add_ref(EFL_UI_WIN_CLASS, NULL,
                efl_ui_win_name_set(efl_added, "glview"),
                efl_text_set(efl_added, "GLView"),
                efl_ui_win_accel_preference_set(efl_added, "gl:depth"));
   elm_win_autodel_set(win, EINA_TRUE);
#endif

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   // Add a GLView
   gl = elm_glview_version_add(win, version);
   if (gl)
     {
        evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_glview_mode_set(gl, 0
                            | ELM_GLVIEW_ALPHA
                            | ELM_GLVIEW_DEPTH
                           );
        elm_glview_resize_policy_set(gl, ELM_GLVIEW_RESIZE_POLICY_RECREATE);
        elm_glview_render_policy_set(gl, ELM_GLVIEW_RENDER_POLICY_ALWAYS);
        elm_glview_init_func_set(gl, _init_gl);
        elm_glview_del_func_set(gl, _del_gl);
        elm_glview_resize_func_set(gl, _resize_gl);
        elm_glview_render_func_set(gl, _draw_gl);
        elm_box_pack_end(bx, gl);
        evas_object_show(gl);

        // Add Mouse/Key Event Callbacks
        elm_object_focus_set(gl, EINA_TRUE);
        evas_object_event_callback_add(gl, EVAS_CALLBACK_KEY_DOWN, _key_down, gl);
        evas_object_event_callback_add(gl, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, gl);
        evas_object_event_callback_add(gl, EVAS_CALLBACK_MOUSE_UP, _mouse_up, gl);
        evas_object_event_callback_add(gl, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move, gl);

        // Animator and other vars
        ani = ecore_animator_add(_anim, gl);
        evas_object_data_set(gl, "ani", ani);
        evas_object_data_set(gl, "gld", gld);
        evas_object_event_callback_add(gl, EVAS_CALLBACK_DEL, _del, gl);

        bt = elm_button_add(win);
        elm_object_text_set(bt, "Direct Mode");
        evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
        elm_box_pack_end(bx, bt);
        evas_object_show(bt);
        evas_object_smart_callback_add(bt, "clicked", _on_direct, gl);

        bt = elm_button_add(win);
        elm_object_text_set(bt, "Indirect Mode");
        evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
        elm_box_pack_end(bx, bt);
        evas_object_show(bt);
        evas_object_smart_callback_add(bt, "clicked", _on_indirect, gl);
     }
   else
     {
        lb = elm_label_add(bx);
        elm_object_text_set(lb, "<align=left> GL backend engine is not supported.<br/>"
                            " 1. Check your back-end engine or<br/>"
                            " 2. Run elementary_test with engine option or<br/>"
                            "    ex) $ <b>ELM_ACCEL=gl</b> elementary_test<br/>"
                            " 3. Change your back-end engine from elementary_config.<br/></align>");
        evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_pack_end(bx, lb);
        evas_object_show(lb);
        free(gld);
     }

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   evas_object_smart_callback_add(bt, "clicked", _on_done, win);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           480 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test entry point for GLView with GLES 2.x.
 * @ingroup Elementary_Tests
 */
void
test_glview(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _test_glview_do(EVAS_GL_GLES_2_X);
}

/**
 * @brief Test entry point for GLView with GLES 3.x.
 * @ingroup Elementary_Tests
 */
void
test_glview_gles3(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _test_glview_do(EVAS_GL_GLES_3_X);
}
